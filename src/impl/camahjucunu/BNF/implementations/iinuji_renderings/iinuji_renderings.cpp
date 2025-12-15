#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

#include <sstream>
#include <unordered_set>

#if IINUJI_RENDERINGS_DEBUG
  #include <iostream>
#endif

// Adjust these to your actual headers:
#include "camahjucunu/BNF/BNF_AST.h"        // RootNode / IntermediaryNode / TerminalNode / ASTNode
#include "piaabo/dutils.h"                 // DEFINE_HASH, hash function used for node->hash

namespace cuwacunu {
namespace camahjucunu {

using namespace BNF;

// ------------------------------------------------------------------
// Simple debug macro
// ------------------------------------------------------------------

#if IINUJI_RENDERINGS_DEBUG
  #define IIN_LOG(msg) \
    do { std::cerr << "[iinuji_renderings] " << msg << '\n'; } while (0)
#else
  #define IIN_LOG(msg) do { } while (0)
#endif

// ------------------------------------------------------------------
// Hash constants for the grammar symbols we care about
// ------------------------------------------------------------------

DEFINE_HASH(IIN_RENDER_HASH_screen,         "<screen>");
DEFINE_HASH(IIN_RENDER_HASH_panel_stmt,     "<panel_stmt>");
DEFINE_HASH(IIN_RENDER_HASH_figure_stmt,    "<figure_stmt>");
DEFINE_HASH(IIN_RENDER_HASH_event_block,    "<event_block>");

DEFINE_HASH(IIN_RENDER_HASH_opt_name,       "<opt_name>");
DEFINE_HASH(IIN_RENDER_HASH_opt_key,        "<opt_key>");
DEFINE_HASH(IIN_RENDER_HASH_opt_line_color, "<opt_line_color>");
DEFINE_HASH(IIN_RENDER_HASH_opt_text_color,"<opt_text_color>");
DEFINE_HASH(IIN_RENDER_HASH_opt_back_color,"<opt_back_color>");
DEFINE_HASH(IIN_RENDER_HASH_opt_tickness,   "<opt_tickness>");
DEFINE_HASH(IIN_RENDER_HASH_opt_coords,     "<opt_coords>");
DEFINE_HASH(IIN_RENDER_HASH_opt_shape,      "<opt_shape>");
DEFINE_HASH(IIN_RENDER_HASH_opt_z_index,    "<opt_z_index>");
DEFINE_HASH(IIN_RENDER_HASH_opt_title,      "<opt_title>");
DEFINE_HASH(IIN_RENDER_HASH_opt_border,     "<opt_border>");
DEFINE_HASH(IIN_RENDER_HASH_opt_value,      "<opt_value>");
DEFINE_HASH(IIN_RENDER_HASH_opt_legend,     "<opt_legend>");
DEFINE_HASH(IIN_RENDER_HASH_opt_type,       "<opt_type>");
DEFINE_HASH(IIN_RENDER_HASH_opt_triggers,   "<opt_triggers>");
DEFINE_HASH(IIN_RENDER_HASH_opt_form,       "<opt_form>");

DEFINE_HASH(IIN_RENDER_HASH_screen_type,    "<screen_type>");
DEFINE_HASH(IIN_RENDER_HASH_panel_type,     "<panel_type>");
DEFINE_HASH(IIN_RENDER_HASH_figure_kind,    "<figure_kind>");
DEFINE_HASH(IIN_RENDER_HASH_event_kind,     "<event_kind>");

DEFINE_HASH(IIN_RENDER_HASH_form_binding,   "<form_binding>");

DEFINE_HASH(IIN_RENDER_HASH_digit,          "<digit>");
DEFINE_HASH(IIN_RENDER_HASH_bool,           "<bool>");
DEFINE_HASH(IIN_RENDER_HASH_color_hex,      "<color_hex>");
DEFINE_HASH(IIN_RENDER_HASH_lower,          "<lower>");
DEFINE_HASH(IIN_RENDER_HASH_upper,          "<upper>");
DEFINE_HASH(IIN_RENDER_HASH_dq_char,        "<dq_char>");

// ------------------------------------------------------------------
// Internal decode state
// ------------------------------------------------------------------

struct iinuji_renderings_decoder_t::State {
  iinuji_renderings_instruction_t* inst = nullptr;

  // Avoid decoding the same AST node multiple times (some AST implementations
  // recurse inside accept(); we also recurse inside visit(), which can cause
  // repeated visits without this guard).
  std::unordered_set<const ASTNode*> visited_nodes;

  // where we are
  bool in_screen = false;
  bool in_panel  = false;
  bool in_figure = false;
  bool in_event  = false;

  enum class Prop {
    None,
    Name,
    Key,
    LineColor,
    TextColor,
    BackColor,
    Tickness,
    Coords,
    Shape,
    ZIndex,
    Title,
    Border,
    Value,
    Legend,
    Type,
    Triggers,
    Form
  } prop = Prop::None;

  // numeric helpers
  double num_value   = 0.0;
  double num_frac    = 0.1;
  bool   num_has_dot = false;

  int    int_value   = 0;   // z-index, fcode number part, etc.

  // coords / shape
  struct {
    int  x = 0;
    int  y = 0;
    bool parsing_y = false;
  } point;

  // __key
  std::string key_buffer;

  // identifiers (__name, __type, etc.)
  std::string ident_buffer;

  // generic string buffer (titles, legends, values, colors)
  std::string string_buffer;

  // booleans
  bool bool_flag      = false;
  bool bool_flag_set  = false;

  // triggers
  std::vector<std::string> triggers;
  std::string trigger_buffer;

  // form bindings
  enum class FormPhase { None, Local, Path } form_phase = FormPhase::None;
  std::string form_local;
  std::string form_path;
};

// ------------------------------------------------------------------
// Small helpers for current objects
// ------------------------------------------------------------------

namespace {

inline iinuji_screen_t& current_screen(iinuji_renderings_decoder_t::State& st) {
  return st.inst->screens.back();
}

inline iinuji_panel_t& current_panel(iinuji_renderings_decoder_t::State& st) {
  return current_screen(st).panels.back();
}

inline iinuji_figure_t& current_figure(iinuji_renderings_decoder_t::State& st) {
  return current_panel(st).figures.back();
}

inline iinuji_event_t& current_event(iinuji_renderings_decoder_t::State& st) {
  return current_screen(st).events.back();
}

} // anonymous namespace

// ------------------------------------------------------------------
// Decoder ctor
// ------------------------------------------------------------------

iinuji_renderings_decoder_t::iinuji_renderings_decoder_t(bool debug) {
  (void)debug;  // logging is controlled only via IINUJI_RENDERINGS_DEBUG
}

// ------------------------------------------------------------------
// Decoder entry point
// ------------------------------------------------------------------

iinuji_renderings_instruction_t
iinuji_renderings_decoder_t::decode(const ASTNode* root) {
  iinuji_renderings_instruction_t inst;
  if (!root) {
    IIN_LOG("decode(): null root, returning empty instruction");
    return inst;
  }

  State st;
  st.inst = &inst;

  VisitorContext ctx(static_cast<void*>(&st));

  IIN_LOG("decode(): starting traversal");

  root->accept(*this, ctx);

  IIN_LOG("decode(): finished traversal, screens=" << inst.screens.size());

  return inst;
}

// ------------------------------------------------------------------
// ASTVisitor – RootNode
// ------------------------------------------------------------------

void iinuji_renderings_decoder_t::visit(const RootNode* node,
                                        VisitorContext& ctx) {
  auto* st = static_cast<State*>(ctx.user_data);
  if (!st) return;

  const ASTNode* as_node = static_cast<const ASTNode*>(node);
  auto [_, inserted] = st->visited_nodes.insert(as_node);
  if (!inserted) {
    // Already processed this subtree; avoid re-decoding it.
    IIN_LOG("visit RootNode (revisit skipped)");
    return;
  }

  IIN_LOG("visit RootNode, children=" << node->children.size());

  ctx.stack.push_back(node);
  for (const auto& child : node->children) {
    child->accept(*this, ctx);
  }
  ctx.stack.pop_back();
}

// ------------------------------------------------------------------
// ASTVisitor – IntermediaryNode
// ------------------------------------------------------------------

void iinuji_renderings_decoder_t::visit(const IntermediaryNode* node,
                                        VisitorContext& ctx) {
  auto* st = static_cast<State*>(ctx.user_data);
  if (!st || !st->inst) return;

  const ASTNode* as_node = static_cast<const ASTNode*>(node);
  auto [_, inserted] = st->visited_nodes.insert(as_node);
  if (!inserted) {
    // This node (and its subtree) has already been decoded earlier.
    return;
  }

  ctx.stack.push_back(node);

  // ---------------- ENTER node -------------------------------------
  switch (node->hash) {
    // structural blocks
    case IIN_RENDER_HASH_screen:
      st->inst->screens.emplace_back();
      st->in_screen = true;
      st->in_panel  = false;
      st->in_figure = false;
      st->in_event  = false;
      IIN_LOG("  created SCREEN index=" << (st->inst->screens.size() - 1));
      break;

    case IIN_RENDER_HASH_panel_stmt:
      current_screen(*st).panels.emplace_back();
      st->in_panel  = true;
      st->in_figure = false;
      IIN_LOG("  created PANEL index="
              << (current_screen(*st).panels.size() - 1));
      break;

    case IIN_RENDER_HASH_figure_stmt:
      current_panel(*st).figures.emplace_back();
      st->in_figure = true;
      IIN_LOG("  created FIGURE index="
              << (current_panel(*st).figures.size() - 1));
      break;

    case IIN_RENDER_HASH_event_block:
      current_screen(*st).events.emplace_back();
      st->in_event  = true;
      IIN_LOG("  created EVENT index="
              << (current_screen(*st).events.size() - 1));
      break;

    // properties begin (set mode + reset buffers)
    case IIN_RENDER_HASH_opt_name:
      st->prop = State::Prop::Name;
      st->ident_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_key:
      st->prop = State::Prop::Key;
      st->key_buffer.clear();
      st->int_value = 0;
      break;

    case IIN_RENDER_HASH_opt_line_color:
      st->prop = State::Prop::LineColor;
      st->string_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_text_color:
      st->prop = State::Prop::TextColor;
      st->string_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_back_color:
      st->prop = State::Prop::BackColor;
      st->string_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_tickness:
      st->prop = State::Prop::Tickness;
      st->num_value   = 0.0;
      st->num_frac    = 0.1;
      st->num_has_dot = false;
      break;

    case IIN_RENDER_HASH_opt_coords:
      st->prop = State::Prop::Coords;
      st->point = {0, 0, false};
      break;

    case IIN_RENDER_HASH_opt_shape:
      st->prop = State::Prop::Shape;
      st->point = {0, 0, false};
      break;

    case IIN_RENDER_HASH_opt_z_index:
      st->prop = State::Prop::ZIndex;
      st->int_value = 0;
      break;

    case IIN_RENDER_HASH_opt_title:
      st->prop = State::Prop::Title;
      st->string_buffer.clear();
      st->bool_flag     = false;
      st->bool_flag_set = false;
      break;

    case IIN_RENDER_HASH_opt_border:
      st->prop = State::Prop::Border;
      st->bool_flag     = false;
      st->bool_flag_set = false;
      break;

    case IIN_RENDER_HASH_opt_value:
      st->prop = State::Prop::Value;
      st->string_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_legend:
      st->prop = State::Prop::Legend;
      st->string_buffer.clear();
      st->bool_flag     = false;
      st->bool_flag_set = false;
      break;

    case IIN_RENDER_HASH_opt_type:
      st->prop = State::Prop::Type;
      st->ident_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_triggers:
      st->prop = State::Prop::Triggers;
      st->triggers.clear();
      st->trigger_buffer.clear();
      break;

    case IIN_RENDER_HASH_opt_form:
      st->prop = State::Prop::Form;
      st->form_phase = State::FormPhase::Local;
      st->form_local.clear();
      st->form_path.clear();
      if (st->in_event) {
        current_event(*st).bindings.clear();
      }
      break;

    default:
      break;
  }

  // ---------------- Traverse children ------------------------------
  for (const auto& child : node->children) {
    child->accept(*this, ctx);
  }

  // ---------------- EXIT node --------------------------------------
  switch (node->hash) {
    case IIN_RENDER_HASH_screen:
      st->in_screen = false;
      break;

    case IIN_RENDER_HASH_panel_stmt:
      st->in_panel  = false;
      st->in_figure = false;
      break;

    case IIN_RENDER_HASH_figure_stmt:
      st->in_figure = false;
      break;

    case IIN_RENDER_HASH_event_block:
      st->in_event  = false;
      break;

    case IIN_RENDER_HASH_opt_name: {
      if (!st->ident_buffer.empty()) {
        if (st->in_event) {
          current_event(*st).name = st->ident_buffer;
        } else if (st->in_screen && !st->in_panel && !st->in_figure) {
          current_screen(*st).name = st->ident_buffer;
        }
      }
      st->ident_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_key: {
      if (st->in_screen && !st->in_panel && !st->in_event && !st->in_figure) {
        auto& scr = current_screen(*st);
        scr.key_raw = st->key_buffer; // e.g. "F+1"
        scr.fcode   = st->int_value;  // e.g. 1
      }
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_line_color:
    case IIN_RENDER_HASH_opt_text_color:
    case IIN_RENDER_HASH_opt_back_color: {
      if (!st->string_buffer.empty()) {
        if (st->in_figure) {
          auto& fig = current_figure(*st);
          if (node->hash == IIN_RENDER_HASH_opt_line_color) fig.line_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_text_color) fig.text_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_back_color) fig.back_color = st->string_buffer;
        } else if (st->in_panel) {
          auto& pan = current_panel(*st);
          if (node->hash == IIN_RENDER_HASH_opt_line_color) pan.line_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_text_color) pan.text_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_back_color) pan.back_color = st->string_buffer;
        } else if (st->in_screen) {
          auto& scr = current_screen(*st);
          if (node->hash == IIN_RENDER_HASH_opt_line_color) scr.line_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_text_color) scr.text_color = st->string_buffer;
          if (node->hash == IIN_RENDER_HASH_opt_back_color) scr.back_color = st->string_buffer;
        }
      }
      st->string_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_tickness: {
      if (st->in_figure)      current_figure(*st).tickness = st->num_value;
      else if (st->in_panel)  current_panel(*st).tickness  = st->num_value;
      else if (st->in_screen) current_screen(*st).tickness = st->num_value;

      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_coords: {
      iinuji_point_t pt;
      pt.set = true;
      pt.x   = st->point.x;
      pt.y   = st->point.y;

      if (st->in_figure)      current_figure(*st).coords = pt;
      else if (st->in_panel)  current_panel(*st).coords  = pt;

      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_shape: {
      iinuji_point_t pt;
      pt.set = true;
      pt.x   = st->point.x;
      pt.y   = st->point.y;

      if (st->in_figure)      current_figure(*st).shape = pt;
      else if (st->in_panel)  current_panel(*st).shape  = pt;

      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_z_index: {
      if (st->in_panel) {
        current_panel(*st).z_index = st->int_value;
      }
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_title: {
      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.title_on = st->bool_flag;
        fig.title    = st->string_buffer;
      } else if (st->in_panel) {
        auto& pan = current_panel(*st);
        pan.title_on = st->bool_flag;
        pan.title    = st->string_buffer;
      }
      st->string_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_border: {
      bool val = st->bool_flag;
      if (st->in_figure)      current_figure(*st).border = val;
      else if (st->in_panel)  current_panel(*st).border  = val;
      else if (st->in_screen) current_screen(*st).border = val;
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_value: {
      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.has_value = true;
        fig.value     = st->string_buffer;
      }
      st->string_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_legend: {
      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.legend_on = st->bool_flag;
        fig.legend    = st->string_buffer;
      }
      st->string_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_type: {
      if (st->in_figure) {
        current_figure(*st).type_raw = st->ident_buffer;
      }
      st->ident_buffer.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_opt_triggers: {
      if (st->in_figure) {
        // flush last trigger if any
        if (!st->trigger_buffer.empty()) {
          st->triggers.push_back(st->trigger_buffer);
          st->trigger_buffer.clear();
        }
        current_figure(*st).triggers = st->triggers;
      }
      st->triggers.clear();
      st->prop = State::Prop::None;
      break;
    }

    case IIN_RENDER_HASH_form_binding: {
      // one binding finished
      if (st->in_event && !st->form_local.empty() && !st->form_path.empty()) {
        std::string full_path = "." + st->form_path; // store with leading '.'
        current_event(*st).bindings.push_back(
          iinuji_event_binding_t{ st->form_local, full_path }
        );
      }
      st->form_local.clear();
      st->form_path.clear();
      st->form_phase = State::FormPhase::Local;
      break;
    }

    case IIN_RENDER_HASH_opt_form: {
      // flush last binding if not flushed via form_binding exit
      if (st->in_event && !st->form_local.empty() && !st->form_path.empty()) {
        std::string full_path = "." + st->form_path;
        current_event(*st).bindings.push_back(
          iinuji_event_binding_t{ st->form_local, full_path }
        );
      }
      st->form_local.clear();
      st->form_path.clear();
      st->form_phase = State::FormPhase::None;
      st->prop = State::Prop::None;
      break;
    }

    default:
      break;
  }

  ctx.stack.pop_back();
}

// ------------------------------------------------------------------
// ASTVisitor – TerminalNode
// ------------------------------------------------------------------

void iinuji_renderings_decoder_t::visit(const TerminalNode* node,
                                        VisitorContext& ctx) {
  auto* st = static_cast<State*>(ctx.user_data);
  if (!st) return;

  const ASTNode* as_node = static_cast<const ASTNode*>(node);
  auto [_, inserted] = st->visited_nodes.insert(as_node);
  if (!inserted) {
    // Already processed this terminal.
    return;
  }

  const std::string& lex = node->unit.lexeme;
  const char ch = lex.empty() ? '\0' : lex[0];

  // ----------------- Property-dependent accumulation ---------------

  switch (st->prop) {
    case State::Prop::Name:
    case State::Prop::Type: {
      // identifier: first char from <lower>/<upper>, rest via <id_tail>
      if (node->hash == IIN_RENDER_HASH_lower ||
          node->hash == IIN_RENDER_HASH_upper ||
          node->hash == IIN_RENDER_HASH_digit) {
        st->ident_buffer.push_back(ch);
      } else if (lex == "_" || lex == "-") {
        st->ident_buffer.push_back(ch);
      }
      break;
    }

    case State::Prop::Key: {
      // fcode = "F" "+" <int>
      if (lex == "F" || lex == "+") {
        st->key_buffer += lex;
      } else if (node->hash == IIN_RENDER_HASH_digit) {
        st->key_buffer.push_back(ch);
        st->int_value = st->int_value * 10 + (ch - '0');
      }
      break;
    }

    case State::Prop::LineColor:
    case State::Prop::TextColor:
    case State::Prop::BackColor: {
      // Color: "#" then 6 hex characters
      if (node->hash == IIN_RENDER_HASH_color_hex && lex == "#") {
        st->string_buffer.push_back('#');
      } else if (node->hash == IIN_RENDER_HASH_digit ||
                 node->hash == IIN_RENDER_HASH_lower ||
                 node->hash == IIN_RENDER_HASH_upper) {
        st->string_buffer.push_back(ch);
      }
      break;
    }

    case State::Prop::Tickness: {
      if (node->hash == IIN_RENDER_HASH_digit) {
        if (!st->num_has_dot) {
          st->num_value = st->num_value * 10.0 + (ch - '0');
        } else {
          st->num_value += (ch - '0') * st->num_frac;
          st->num_frac  *= 0.1;
        }
      } else if (lex == ".") {
        st->num_has_dot = true;
      }
      break;
    }

    case State::Prop::Coords:
    case State::Prop::Shape: {
      if (node->hash == IIN_RENDER_HASH_digit) {
        if (!st->point.parsing_y) {
          st->point.x = st->point.x * 10 + (ch - '0');
        } else {
          st->point.y = st->point.y * 10 + (ch - '0');
        }
      } else if (lex == ",") {
        st->point.parsing_y = true;
      }
      break;
    }

    case State::Prop::ZIndex: {
      if (node->hash == IIN_RENDER_HASH_digit) {
        st->int_value = st->int_value * 10 + (ch - '0');
      }
      break;
    }

    case State::Prop::Title:
    case State::Prop::Legend:
    case State::Prop::Value: {
      if (node->hash == IIN_RENDER_HASH_bool) {
        st->bool_flag     = (lex == "true");
        st->bool_flag_set = true;
      } else if (node->hash == IIN_RENDER_HASH_dq_char) {
        st->string_buffer.push_back(ch);
      }
      break;
    }

    case State::Prop::Border: {
      if (node->hash == IIN_RENDER_HASH_bool) {
        st->bool_flag     = (lex == "true");
        st->bool_flag_set = true;
      }
      break;
    }

    case State::Prop::Triggers: {
      // trigger_list: event_name [, event_name]*
      if (node->hash == IIN_RENDER_HASH_lower ||
          node->hash == IIN_RENDER_HASH_upper ||
          node->hash == IIN_RENDER_HASH_digit) {
        st->trigger_buffer.push_back(ch);
      } else if (lex == "_" || lex == "-") {
        st->trigger_buffer.push_back(ch);
      } else if (lex == ",") {
        if (!st->trigger_buffer.empty()) {
          st->triggers.push_back(st->trigger_buffer);
          st->trigger_buffer.clear();
        }
      }
      break;
    }

    case State::Prop::Form: {
      // __form local:.path
      if (lex == ":") {
        // finished the local name; subsequent identifier chars go to the path
        st->form_phase = State::FormPhase::Path;
      } else if (lex == ".") {
        // ignore the dot itself; we add a leading '.' later when building full_path
      } else if (node->hash == IIN_RENDER_HASH_lower ||
                 node->hash == IIN_RENDER_HASH_upper ||
                 node->hash == IIN_RENDER_HASH_digit) {
        if (st->form_phase == State::FormPhase::Local) {
          st->form_local.push_back(ch);
        } else if (st->form_phase == State::FormPhase::Path) {
          st->form_path.push_back(ch);
        }
      } else if (lex == "_" || lex == "-") {
        if (st->form_phase == State::FormPhase::Local) {
          st->form_local.push_back(ch);
        } else if (st->form_phase == State::FormPhase::Path) {
          st->form_path.push_back(ch);
        }
      } else if (lex == ",") {
        // multi-binding: flushing is handled on <form_binding> exit
      }
      break;
    }

    default:
      break;
  }
}

// ------------------------------------------------------------------
// Pretty-print helpers
// ------------------------------------------------------------------

static std::string indent(unsigned n) {
  return std::string(n, ' ');
}

std::string iinuji_figure_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "FIGURE " << kind_raw;
  if (has_value) {
    oss << " value=\"" << value << "\"";
  }
  if (title_on) {
    oss << " title(on,\"" << title << "\")";
  }
  if (legend_on) {
    oss << " legend(on,\"" << legend << "\")";
  }
  oss << "\n";

  return oss.str();
}

std::string iinuji_panel_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "PANEL " << kind_raw;
  if (title_on) {
    oss << " title(on,\"" << title << "\")";
  }
  oss << "\n";
  for (const auto& f : figures) {
    oss << f.str(ind + 2);
  }
  return oss.str();
}

std::string iinuji_event_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "EVENT " << kind_raw;
  if (!name.empty()) {
    oss << " name=" << name;
  }
  if (!bindings.empty()) {
    oss << " form{";
    bool first = true;
    for (const auto& b : bindings) {
      if (!first) oss << ",";
      first = false;
      oss << b.local_name << ":" << b.path_name;
    }
    oss << "}";
  }
  oss << "\n";
  return oss.str();
}

std::string iinuji_screen_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "SCREEN " << kind_raw;
  if (!key_raw.empty()) {
    oss << " key=" << key_raw;
  }
  if (!name.empty()) {
    oss << " name=" << name;
  }
  oss << "\n";

  for (const auto& p : panels) {
    oss << p.str(ind + 2);
  }
  for (const auto& e : events) {
    oss << e.str(ind + 2);
  }
  oss << pad << "ENDSCREEN\n";
  return oss.str();
}

std::string iinuji_renderings_instruction_t::str() const {
  std::ostringstream oss;
  oss << "Number of screens: " << screens.size() << "\n\n";
  for (const auto& s : screens) {
    oss << s.str(0);
  }
  return oss.str();
}

std::ostream& operator<<(std::ostream& os,
                         const iinuji_renderings_instruction_t& inst) {
  os << inst.str();
  return os;
}

} // namespace camahjucunu
} // namespace cuwacunu
