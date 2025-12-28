/* iinuji_renderings.cpp */
#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

#include <sstream>
#include <unordered_set>
#include <cctype>    // isalpha, isalnum, tolower
#include <cstring>   // std::strlen
#include <cstdio>    // std::fprintf
#include <limits>    // std::numeric_limits

#if IINUJI_RENDERINGS_DEBUG
  #include <iostream>
#endif

#include "camahjucunu/BNF/BNF_AST.h"
#include "piaabo/dutils.h"

RUNTIME_WARNING("(iinuji_renderings.cpp)[] decoder accepts ident chars anywhere and does not enforce BNF's first-char alpha rule; relies on validation for shape.\n");
RUNTIME_WARNING("(iinuji_renderings.cpp)[] normalize_bnf_lexeme() unquotes any token wrapped in quotes; fragile across lexer/AST encoding changes and can break dq_string capture.\n");
RUNTIME_WARNING("(iinuji_renderings.cpp)[] parse_bool_from_lex() scans quoted content too; could mis-detect true/false if tokenization changes. Consider quote-aware bool parsing.\n");
RUNTIME_WARNING("(iinuji_renderings.cpp)[] visited_nodes guard prevents double-walk but can hide traversal bugs or suppress valid DAG/shared-node visits; unify traversal semantics.\n");

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
DEFINE_HASH(IIN_RENDER_HASH_opt_text_color, "<opt_text_color>");
DEFINE_HASH(IIN_RENDER_HASH_opt_back_color, "<opt_back_color>");
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

// IMPORTANT: your BNF uses <opt__capacity> (double underscore in the nonterminal name)
DEFINE_HASH(IIN_RENDER_HASH_opt_capacity,   "<opt__capacity>");

// ------------------------------------------------------------------
// Internal decode state
// ------------------------------------------------------------------
struct iinuji_renderings_decoder_t::State {
  iinuji_renderings_instruction_t* inst = nullptr;

  // If true, ignore all terminals until the closing "*/" terminal appears.
  bool in_block_comment = false;

  // Guard against double-walk (some ASTs traverse inside accept()).
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
    Capacity,   // NEW
    ZIndex,
    Title,
    Border,
    Value,
    Legend,
    Type,
    Triggers,
    Form
  } prop = Prop::None;

  // kind parsing fallback
  enum class KindTarget { None, Screen, Panel, Figure, Event };
  KindTarget  expect_kind = KindTarget::None;
  std::string kind_buffer;

  // numeric helpers
  double num_value   = 0.0;
  double num_frac    = 0.1;
  bool   num_has_dot = false;

  int    int_value   = 0;

  // coords / shape parsing
  struct {
    int  x = 0;
    int  y = 0;
    bool parsing_y = false;
  } point;

  // __key
  std::string key_buffer;

  // identifiers (__name, __type)
  std::string ident_buffer;

  // generic string buffer (used for colors)
  std::string string_buffer;

  // robust color parsing: # + 6 hexdigits across split terminals
  bool color_in_progress = false;
  int  color_digits      = 0;

  // booleans ("true"/"false") across split terminals
  bool bool_flag      = false;
  bool bool_flag_set  = false;
  std::string word_buffer;

  // triggers
  std::vector<std::string> triggers;
  std::string trigger_buffer;

  // form bindings
  enum class FormPhase { None, Local, Path } form_phase = FormPhase::None;
  std::string form_local;
  std::string form_path;

  // ------------------------------------------------------------
  // robust string literal capture (titles/legends/values)
  // ------------------------------------------------------------
  int         dq_quote_count = 0;
  bool        dq_escaped     = false;
  std::string dq_current;
  std::string dq_last;
};

// ------------------------------------------------------------------
// Small helpers
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

inline bool is_digit_char(char c) { return c >= '0' && c <= '9'; }
inline bool is_hex_char(char c) {
  return is_digit_char(c) ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

// IMPORTANT: match your BNF <name_ident> tail, which includes '.' and '-'
inline bool is_ident_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.';
}

inline bool is_alpha_char(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) != 0;
}
inline char lower_char(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

inline void reset_color_capture(iinuji_renderings_decoder_t::State& st) {
  st.color_in_progress = false;
  st.color_digits      = 0;
  st.string_buffer.clear();
}

inline void reset_bool_capture(iinuji_renderings_decoder_t::State& st) {
  st.bool_flag     = false;
  st.bool_flag_set = false;
  st.word_buffer.clear();
}

inline void try_flush_bool_word(iinuji_renderings_decoder_t::State& st) {
  if (st.bool_flag_set) {
    st.word_buffer.clear();
    return;
  }
  if (st.word_buffer == "true") {
    st.bool_flag = true;
    st.bool_flag_set = true;
  } else if (st.word_buffer == "false") {
    st.bool_flag = false;
    st.bool_flag_set = true;
  }
  st.word_buffer.clear();
}

inline void reset_dq_capture(iinuji_renderings_decoder_t::State& st) {
  st.dq_quote_count = 0;
  st.dq_escaped     = false;
  st.dq_current.clear();
  st.dq_last.clear();
}

// Collect quoted segments, keep the last non-empty one.
// Also works across split terminals.
inline void consume_dq_segments(iinuji_renderings_decoder_t::State& st,
                               const std::string& lex) {
  for (char c : lex) {
    if (st.dq_escaped) {
      if ((st.dq_quote_count % 2) == 1) st.dq_current.push_back(c);
      st.dq_escaped = false;
      continue;
    }

    if (c == '\\') {
      st.dq_escaped = true;
      continue;
    }

    if (c == '"') {
      st.dq_quote_count++;

      // closing quote => finalize current segment
      if ((st.dq_quote_count % 2) == 0) {
        if (!st.dq_current.empty()) {
          st.dq_last = st.dq_current;  // keep last non-empty segment
        }
        st.dq_current.clear();
      }
      continue;
    }

    // inside quotes
    if ((st.dq_quote_count % 2) == 1) {
      st.dq_current.push_back(c);
    }
  }
}

inline std::string dq_final_string(const iinuji_renderings_decoder_t::State& st) {
  if (!st.dq_last.empty()) return st.dq_last;
  return st.dq_current; // if unterminated for some reason
}

inline void arm_kind(iinuji_renderings_decoder_t::State& st,
                     iinuji_renderings_decoder_t::State::KindTarget tgt) {
  st.expect_kind = tgt;
  st.kind_buffer.clear();
}
inline void clear_kind(iinuji_renderings_decoder_t::State& st) {
  st.expect_kind = iinuji_renderings_decoder_t::State::KindTarget::None;
  st.kind_buffer.clear();
}

inline bool looks_like_property_token(const std::string& lex) {
  return lex.find("__") != std::string::npos;
}

inline void assign_kind(iinuji_renderings_decoder_t::State& st,
                        const std::string& kind) {
  using KT = iinuji_renderings_decoder_t::State::KindTarget;
  switch (st.expect_kind) {
    case KT::Screen: if (st.in_screen) current_screen(st).kind_raw = kind; break;
    case KT::Panel:  if (st.in_panel)  current_panel(st).kind_raw  = kind; break;
    case KT::Figure: if (st.in_figure) current_figure(st).kind_raw = kind; break;
    case KT::Event:  if (st.in_event)  current_event(st).kind_raw  = kind; break;
    default: break;
  }
  clear_kind(st);
}

// Kind capture (inlined kinds supported)
inline void consume_kind(iinuji_renderings_decoder_t::State& st,
                         const std::string& lex) {
  if (st.expect_kind == iinuji_renderings_decoder_t::State::KindTarget::None) return;

  if (looks_like_property_token(lex)) {
    if (!st.kind_buffer.empty()) assign_kind(st, st.kind_buffer);
    else clear_kind(st);
    return;
  }

  for (char c : lex) {
    if (st.kind_buffer.empty()) {
      if (c == '_') st.kind_buffer.push_back('_');
      continue;
    }
    if (is_ident_char(c)) st.kind_buffer.push_back(c);
    else {
      if (!st.kind_buffer.empty()) assign_kind(st, st.kind_buffer);
      else clear_kind(st);
      return;
    }
  }

  if (st.kind_buffer == "_screen" ||
      st.kind_buffer == "_rectangle" ||
      st.kind_buffer == "_label" ||
      st.kind_buffer == "_horizontal_plot" ||
      st.kind_buffer == "_input_box" ||
      st.kind_buffer == "_buffer" ||
      st.kind_buffer == "_update" ||
      st.kind_buffer == "_action") {
    assign_kind(st, st.kind_buffer);
  }
}

inline void consume_color_hex(iinuji_renderings_decoder_t::State& st,
                              const std::string& lex) {
  for (char c : lex) {
    if (c == '#') {
      st.string_buffer.clear();
      st.string_buffer.push_back('#');
      st.color_in_progress = true;
      st.color_digits = 0;
      continue;
    }
    if (st.color_in_progress && is_hex_char(c)) {
      if (st.color_digits < 6) {
        st.string_buffer.push_back(c);
        st.color_digits++;
      }
      if (st.color_digits >= 6) st.color_in_progress = false;
    }
  }
}

inline void consume_point(iinuji_renderings_decoder_t::State& st,
                          const std::string& lex) {
  for (char c : lex) {
    if (is_digit_char(c)) {
      if (!st.point.parsing_y) st.point.x = st.point.x * 10 + (c - '0');
      else st.point.y = st.point.y * 10 + (c - '0');
    } else if (c == ',') {
      st.point.parsing_y = true;
    }
  }
}

inline void consume_uint(iinuji_renderings_decoder_t::State& st,
                         const std::string& lex) {
  for (char c : lex) if (is_digit_char(c)) st.int_value = st.int_value * 10 + (c - '0');
}

inline void consume_float(iinuji_renderings_decoder_t::State& st,
                          const std::string& lex) {
  for (char c : lex) {
    if (is_digit_char(c)) {
      if (!st.num_has_dot) st.num_value = st.num_value * 10.0 + (c - '0');
      else {
        st.num_value += (c - '0') * st.num_frac;
        st.num_frac  *= 0.1;
      }
    } else if (c == '.' && !st.num_has_dot) {
      st.num_has_dot = true;
    }
  }
}

// Parse "true"/"false" from any lexeme chunk (robust against weird quoting wrappers)
inline void parse_bool_from_lex(iinuji_renderings_decoder_t::State& st,
                                const std::string& lex) {
  if (st.bool_flag_set) return;
  for (char c : lex) {
    if (is_alpha_char(c)) st.word_buffer.push_back(lower_char(c));
    else try_flush_bool_word(st);
  }
}

inline void begin_prop(iinuji_renderings_decoder_t::State& st,
                       iinuji_renderings_decoder_t::State::Prop p) {
  st.prop = p;
  switch (p) {
    case iinuji_renderings_decoder_t::State::Prop::Name:
    case iinuji_renderings_decoder_t::State::Prop::Type:
      st.ident_buffer.clear();
      break;

    case iinuji_renderings_decoder_t::State::Prop::Key:
      st.key_buffer.clear();
      st.int_value = 0;
      break;

    case iinuji_renderings_decoder_t::State::Prop::LineColor:
    case iinuji_renderings_decoder_t::State::Prop::TextColor:
    case iinuji_renderings_decoder_t::State::Prop::BackColor:
      reset_color_capture(st);
      break;

    case iinuji_renderings_decoder_t::State::Prop::Tickness:
    case iinuji_renderings_decoder_t::State::Prop::Capacity: // NEW
      st.num_value   = 0.0;
      st.num_frac    = 0.1;
      st.num_has_dot = false;
      break;

    case iinuji_renderings_decoder_t::State::Prop::Coords:
    case iinuji_renderings_decoder_t::State::Prop::Shape:
      st.point = {0, 0, false};
      break;

    case iinuji_renderings_decoder_t::State::Prop::ZIndex:
      st.int_value = 0;
      break;

    case iinuji_renderings_decoder_t::State::Prop::Title:
    case iinuji_renderings_decoder_t::State::Prop::Legend:
      reset_dq_capture(st);
      reset_bool_capture(st);
      break;

    case iinuji_renderings_decoder_t::State::Prop::Value:
      reset_dq_capture(st);
      break;

    case iinuji_renderings_decoder_t::State::Prop::Border:
      reset_bool_capture(st);
      break;

    case iinuji_renderings_decoder_t::State::Prop::Triggers:
      st.triggers.clear();
      st.trigger_buffer.clear();
      break;

    case iinuji_renderings_decoder_t::State::Prop::Form:
      st.form_phase = iinuji_renderings_decoder_t::State::FormPhase::Local;
      st.form_local.clear();
      st.form_path.clear();
      if (st.in_event) current_event(st).bindings.clear();
      break;

    default:
      break;
  }
}

inline void end_prop(iinuji_renderings_decoder_t::State& st) {
  st.prop = iinuji_renderings_decoder_t::State::Prop::None;
}

inline void commit_color(iinuji_renderings_decoder_t::State& st,
                         std::size_t opt_hash) {
  if (st.string_buffer.empty()) return;

  auto apply = [&](auto& obj) {
    if (opt_hash == IIN_RENDER_HASH_opt_line_color) obj.line_color = st.string_buffer;
    else if (opt_hash == IIN_RENDER_HASH_opt_text_color) obj.text_color = st.string_buffer;
    else if (opt_hash == IIN_RENDER_HASH_opt_back_color) obj.back_color = st.string_buffer;
  };

  if (st.in_figure) apply(current_figure(st));
  else if (st.in_panel) apply(current_panel(st));
  else if (st.in_screen) apply(current_screen(st));

  reset_color_capture(st);
}

inline void flush_form_binding_if_complete(iinuji_renderings_decoder_t::State& st) {
  if (!st.in_event) return;
  if (st.form_local.empty() || st.form_path.empty()) return;

  current_event(st).bindings.push_back(
    iinuji_event_binding_t{ st.form_local, "." + st.form_path }
  );
  st.form_local.clear();
  st.form_path.clear();
  st.form_phase = iinuji_renderings_decoder_t::State::FormPhase::Local;
}

inline std::string normalize_bnf_lexeme(const std::string& lex) {
  // If the AST stores grammar literals with surrounding quotes, unwrap them:
  //   "\"\""  -> \"   -> "
  //   "A"     -> A
  //   "_"     -> _
  if (lex.size() >= 2 && lex.front() == '"' && lex.back() == '"') {
    std::string inner = lex.substr(1, lex.size() - 2);

    // Unescape \" and \\ (BNF-style)
    std::string out;
    out.reserve(inner.size());
    bool esc = false;
    for (char c : inner) {
      if (esc) {
        out.push_back(c);
        esc = false;
      } else if (c == '\\') {
        esc = true;
      } else {
        out.push_back(c);
      }
    }
    return out;
  }
  return lex;
}

} // anonymous namespace

// ------------------------------------------------------------------
// Decoder ctor
// ------------------------------------------------------------------
iinuji_renderings_decoder_t::iinuji_renderings_decoder_t(bool debug) {
  (void)debug;
}

// ------------------------------------------------------------------
// Decoder entry point
// ------------------------------------------------------------------
iinuji_renderings_instruction_t
iinuji_renderings_decoder_t::decode(const ASTNode* root) {
  iinuji_renderings_instruction_t inst;
  if (!root) return inst;

  State st;
  st.inst = &inst;
  VisitorContext ctx(static_cast<void*>(&st));
  root->accept(*this, ctx);
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
  if (!inserted) return;

  ctx.stack.push_back(node);
  for (const auto& child : node->children) child->accept(*this, ctx);
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
  if (!inserted) return;

  ctx.stack.push_back(node);

  // ENTER
  switch (node->hash) {
    case IIN_RENDER_HASH_screen:
      st->inst->screens.emplace_back();
      st->in_screen = true;
      st->in_panel  = false;
      st->in_figure = false;
      st->in_event  = false;
      arm_kind(*st, State::KindTarget::Screen);
      break;

    case IIN_RENDER_HASH_panel_stmt:
      current_screen(*st).panels.emplace_back();
      st->in_panel  = true;
      st->in_figure = false;
      arm_kind(*st, State::KindTarget::Panel);
      break;

    case IIN_RENDER_HASH_figure_stmt:
      current_panel(*st).figures.emplace_back();
      st->in_figure = true;
      arm_kind(*st, State::KindTarget::Figure);
      break;

    case IIN_RENDER_HASH_event_block:
      current_screen(*st).events.emplace_back();
      st->in_event = true;
      arm_kind(*st, State::KindTarget::Event);
      break;

    case IIN_RENDER_HASH_opt_name:       begin_prop(*st, State::Prop::Name); break;
    case IIN_RENDER_HASH_opt_key:        begin_prop(*st, State::Prop::Key); break;
    case IIN_RENDER_HASH_opt_line_color: begin_prop(*st, State::Prop::LineColor); break;
    case IIN_RENDER_HASH_opt_text_color: begin_prop(*st, State::Prop::TextColor); break;
    case IIN_RENDER_HASH_opt_back_color: begin_prop(*st, State::Prop::BackColor); break;
    case IIN_RENDER_HASH_opt_tickness:   begin_prop(*st, State::Prop::Tickness); break;
    case IIN_RENDER_HASH_opt_coords:     begin_prop(*st, State::Prop::Coords); break;
    case IIN_RENDER_HASH_opt_shape:      begin_prop(*st, State::Prop::Shape); break;
    case IIN_RENDER_HASH_opt_capacity:   begin_prop(*st, State::Prop::Capacity); break; // NEW
    case IIN_RENDER_HASH_opt_z_index:    begin_prop(*st, State::Prop::ZIndex); break;
    case IIN_RENDER_HASH_opt_title:      begin_prop(*st, State::Prop::Title); break;
    case IIN_RENDER_HASH_opt_border:     begin_prop(*st, State::Prop::Border); break;
    case IIN_RENDER_HASH_opt_value:      begin_prop(*st, State::Prop::Value); break;
    case IIN_RENDER_HASH_opt_legend:     begin_prop(*st, State::Prop::Legend); break;
    case IIN_RENDER_HASH_opt_type:       begin_prop(*st, State::Prop::Type); break;
    case IIN_RENDER_HASH_opt_triggers:   begin_prop(*st, State::Prop::Triggers); break;
    case IIN_RENDER_HASH_opt_form:       begin_prop(*st, State::Prop::Form); break;

    default:
      break;
  }

  // Traverse children
  for (const auto& child : node->children) child->accept(*this, ctx);

  // EXIT / COMMIT
  switch (node->hash) {
    case IIN_RENDER_HASH_screen:
      st->in_screen = false;
      clear_kind(*st);
      break;

    case IIN_RENDER_HASH_panel_stmt:
      st->in_panel  = false;
      st->in_figure = false;
      clear_kind(*st);
      break;

    case IIN_RENDER_HASH_figure_stmt:
      st->in_figure = false;
      clear_kind(*st);
      break;

    case IIN_RENDER_HASH_event_block:
      st->in_event = false;
      clear_kind(*st);
      break;

    case IIN_RENDER_HASH_opt_name: {
      if (!st->ident_buffer.empty()) {
        if (st->in_event) current_event(*st).name = st->ident_buffer;
        else if (st->in_screen && !st->in_panel && !st->in_figure) current_screen(*st).name = st->ident_buffer;
      }
      st->ident_buffer.clear();
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_key: {
      if (st->in_screen && !st->in_panel && !st->in_event && !st->in_figure) {
        auto& scr = current_screen(*st);
        scr.key_raw = st->key_buffer;
        scr.fcode   = st->int_value;
      }
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_line_color:
    case IIN_RENDER_HASH_opt_text_color:
    case IIN_RENDER_HASH_opt_back_color: {
      commit_color(*st, node->hash);
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_tickness: {
      if (st->in_figure)      current_figure(*st).tickness = st->num_value;
      else if (st->in_panel)  current_panel(*st).tickness  = st->num_value;
      else if (st->in_screen) current_screen(*st).tickness = st->num_value;
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_coords: {
      iinuji_point_t pt{true, st->point.x, st->point.y};
      if (st->in_figure) current_figure(*st).coords = pt;
      else if (st->in_panel) current_panel(*st).coords = pt;
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_shape: {
      iinuji_point_t pt{true, st->point.x, st->point.y};
      if (st->in_figure) current_figure(*st).shape = pt;
      else if (st->in_panel) current_panel(*st).shape = pt;
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_capacity: {
      // Figure-only
      if (st->in_figure) {
        // clamp safely before converting double -> int
        double v = st->num_value;
        int cap = 0;
        if (v > 0.0) {
          const double imax = (double)std::numeric_limits<int>::max();
          if (v > imax) cap = std::numeric_limits<int>::max();
          else cap = (int)v; // truncate (1000.0 -> 1000)
        }

        auto& fig = current_figure(*st);
        fig.has_capacity = true;
        fig.capacity     = cap;
      }
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_z_index: {
      if (st->in_panel) current_panel(*st).z_index = st->int_value;
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_title: {
      // finalize bool if last token was "true"/"false"
      try_flush_bool_word(*st);

      const std::string s = dq_final_string(*st);

      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.title_on = st->bool_flag;
        fig.title    = s;
      } else if (st->in_panel) {
        auto& pan = current_panel(*st);
        pan.title_on = st->bool_flag;
        pan.title    = s;
      }

      reset_dq_capture(*st);
      reset_bool_capture(*st);
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_border: {
      try_flush_bool_word(*st);
      const bool val = st->bool_flag;

      if (st->in_figure)      current_figure(*st).border = val;
      else if (st->in_panel)  current_panel(*st).border  = val;
      else if (st->in_screen) current_screen(*st).border = val;

      reset_bool_capture(*st);
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_value: {
      const std::string s = dq_final_string(*st);
      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.has_value = true;
        fig.value     = s;
      }
      reset_dq_capture(*st);
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_legend: {
      try_flush_bool_word(*st);

      const std::string s = dq_final_string(*st);
      if (st->in_figure) {
        auto& fig = current_figure(*st);
        fig.legend_on = st->bool_flag;
        fig.legend    = s;
      }

      reset_dq_capture(*st);
      reset_bool_capture(*st);
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_type: {
      if (st->in_figure) current_figure(*st).type_raw = st->ident_buffer;
      st->ident_buffer.clear();
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_triggers: {
      if (st->in_figure) {
        if (!st->trigger_buffer.empty()) {
          st->triggers.push_back(st->trigger_buffer);
          st->trigger_buffer.clear();
        }
        current_figure(*st).triggers = st->triggers;
      }
      st->triggers.clear();
      end_prop(*st);
      break;
    }

    case IIN_RENDER_HASH_opt_form: {
      flush_form_binding_if_complete(*st);
      st->form_phase = State::FormPhase::None;
      end_prop(*st);
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
  if (!inserted) return;

  std::string lex = normalize_bnf_lexeme(node->unit.lexeme);
  if (lex.empty()) return;

  // ------------------------------------------------------------
  // Block comments: treat everything between "/*" and "*/" as whitespace.
  // This is important because property decoders scan terminals loosely.
  // ------------------------------------------------------------
  if (st->in_block_comment) {
    if (lex == "*/") st->in_block_comment = false;
    return;
  }
  if (lex == "/*") {
    st->in_block_comment = true;
    return;
  }
  if (lex == "*/") {
    // stray close; ignore
    return;
  }

  // Kind parsing is independent of prop
  consume_kind(*st, lex);

  switch (st->prop) {
    case State::Prop::Name: {
      std::string val = lex;
      constexpr const char* tag = "__name";
      auto pos = val.find(tag);
      if (pos != std::string::npos) val.erase(0, pos + std::strlen(tag));
      for (char c : val) if (is_ident_char(c)) st->ident_buffer.push_back(c);
      break;
    }

    case State::Prop::Type: {
      std::string val = lex;
      constexpr const char* tag = "__type";
      auto pos = val.find(tag);
      if (pos != std::string::npos) val.erase(0, pos + std::strlen(tag));
      for (char c : val) if (is_ident_char(c)) st->ident_buffer.push_back(c);
      break;
    }

    case State::Prop::Key: {
      for (char c : lex) {
        if (c == 'F' || c == '+') st->key_buffer.push_back(c);
        else if (is_digit_char(c)) {
          st->key_buffer.push_back(c);
          st->int_value = st->int_value * 10 + (c - '0');
        }
      }
      break;
    }

    case State::Prop::LineColor:
    case State::Prop::TextColor:
    case State::Prop::BackColor:
      consume_color_hex(*st, lex);
      break;

    case State::Prop::Tickness:
      consume_float(*st, lex);
      break;

    case State::Prop::Coords:
    case State::Prop::Shape:
      consume_point(*st, lex);
      break;

    case State::Prop::Capacity: // NEW
      consume_float(*st, lex);
      break;

    case State::Prop::ZIndex:
      consume_uint(*st, lex);
      break;

    // title/legend
    case State::Prop::Title:
    case State::Prop::Legend: {
      parse_bool_from_lex(*st, lex);
      consume_dq_segments(*st, lex);
      break;
    }

    case State::Prop::Value: {
      consume_dq_segments(*st, lex);
      break;
    }

    // border bool only
    case State::Prop::Border: {
      parse_bool_from_lex(*st, lex);
      break;
    }

    // triggers
    case State::Prop::Triggers: {
      std::string s = lex;
      auto pos = s.find("__triggers");
      if (pos != std::string::npos) s.erase(pos, std::strlen("__triggers"));

      for (char c : s) {
        if (is_ident_char(c)) {
          st->trigger_buffer.push_back(c);
        } else if (c == ',') {
          if (!st->trigger_buffer.empty()) {
            st->triggers.push_back(st->trigger_buffer);
            st->trigger_buffer.clear();
          }
        }
      }
      break;
    }

    // form (flush on ',' and at <opt_form> exit)
    case State::Prop::Form: {
      std::string s = lex;
      auto pos = s.find("__form");
      if (pos != std::string::npos) s.erase(pos, std::strlen("__form"));

      for (char c : s) {
        if (c == ':') {
          st->form_phase = State::FormPhase::Path;
        } else if (c == '.') {
          // ignore '.'; we add it when storing (and parser is robust to sys.stdout vs sysstdout)
        } else if (c == ',') {
          flush_form_binding_if_complete(*st);
        } else if (is_ident_char(c)) {
          if (st->form_phase == State::FormPhase::Local) st->form_local.push_back(c);
          else if (st->form_phase == State::FormPhase::Path) st->form_path.push_back(c);
        }
      }
      break;
    }

    default:
      break;
  }
}

// ------------------------------------------------------------------
// Pretty-print helpers (unchanged except capacity printing)
// ------------------------------------------------------------------
static std::string indent(unsigned n) { return std::string(n, ' '); }

std::string iinuji_figure_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "FIGURE " << kind_raw;
  if (has_capacity) oss << " capacity=" << capacity;
  if (has_value) oss << " value=\"" << value << "\"";
  if (title_on)  oss << " title(on,\"" << title << "\")";
  if (legend_on) oss << " legend(on,\"" << legend << "\")";
  oss << "\n";
  return oss.str();
}

std::string iinuji_panel_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "PANEL " << kind_raw;
  if (title_on) oss << " title(on,\"" << title << "\")";
  oss << "\n";
  for (const auto& f : figures) oss << f.str(ind + 2);
  return oss.str();
}

std::string iinuji_event_t::str(unsigned ind) const {
  std::ostringstream oss;
  std::string pad = indent(ind);
  oss << pad << "EVENT " << kind_raw;
  if (!name.empty()) oss << " name=" << name;
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
  if (!key_raw.empty()) oss << " key=" << key_raw;
  if (!name.empty())    oss << " name=" << name;
  oss << "\n";
  for (const auto& p : panels) oss << p.str(ind + 2);
  for (const auto& e : events) oss << e.str(ind + 2);
  oss << pad << "ENDSCREEN\n";
  return oss.str();
}

std::string iinuji_renderings_instruction_t::str() const {
  std::ostringstream oss;
  oss << "Number of screens: " << screens.size() << "\n\n";
  for (const auto& s : screens) oss << s.str(0);
  return oss.str();
}

std::ostream& operator<<(std::ostream& os,
                         const iinuji_renderings_instruction_t& inst) {
  os << inst.str();
  return os;
}

} // namespace camahjucunu
} // namespace cuwacunu
