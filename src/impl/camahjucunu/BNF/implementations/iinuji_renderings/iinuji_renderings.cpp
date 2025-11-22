/* iinuji_renderings.cpp */
#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/* ─────────────────────────────────────────────
   Local helpers
   ───────────────────────────────────────────── */

static inline std::string strip_quotes(std::string s) {
  if (!s.empty() && (s.front()=='"' || s.front()=='\'')) s.erase(s.begin());
  if (!s.empty() && (s.back()=='"'  || s.back()=='\''))  s.pop_back();
  return s;
}

static inline const std::string& node_name(const ASTNode* n) {
  if (auto r = dynamic_cast<const RootNode*>(n))         return r->name;
  if (auto m = dynamic_cast<const IntermediaryNode*>(n)) return m->name;
  if (auto t = dynamic_cast<const TerminalNode*>(n))     return t->name;
  static const std::string kEmpty;
  return kEmpty;
}

static bool stack_ends_with(const VisitorContext& ctx,
                            std::initializer_list<const char*> names) {
  const auto& st = ctx.stack;
  if (st.size() < names.size()) return false;
  auto it = names.end(); --it;
  size_t i = st.size() - 1;
  for (;;) {
    if (node_name(st[i]) != *it) return false;
    if (it == names.begin()) break;
    --it; if (i == 0) return false; --i;
  }
  return true;
}

/* Thread-local decode state (not exposed in the header) */
thread_local iinuji_renderings_instruction_t* g_out = nullptr;

// Small token accumulators that we commit on boundaries.
thread_local std::string g_fcacc;                // screen fcode digits
thread_local std::string g_rect_acc;             // panel x/y/w/h
thread_local int         g_rect_i = 0;
thread_local std::string g_zacc;                 // panel z
thread_local std::string g_sacc;                 // panel scale
thread_local std::string g_present_k, g_present_v; // presenter kv
thread_local std::string g_draw_op, g_draw_k, g_draw_v; // draw op & kv
thread_local std::string g_trig_k, g_trig_v;     // triggers kv

static inline void reset_all_accumulators() {
  g_fcacc.clear();
  g_rect_acc.clear(); g_rect_i = 0;
  g_zacc.clear();
  g_sacc.clear();
  g_present_k.clear(); g_present_v.clear();
  g_draw_op.clear();   g_draw_k.clear(); g_draw_v.clear();
  g_trig_k.clear();    g_trig_v.clear();
}

static inline void commit_fcode_if_pending() {
  if (!g_out || !g_out->_ps.scr) { g_fcacc.clear(); return; }
  if (!g_fcacc.empty()) {
    int v = 0; try { v = std::stoi(g_fcacc); } catch (...) {}
    g_out->_ps.scr->fcode = v;
    g_fcacc.clear();
  }
}

static inline void commit_rect_if_pending() {
  if (!g_out || !g_out->_ps.pan) { g_rect_acc.clear(); return; }
  if (!g_rect_acc.empty()) {
    int v = 0; try { v = std::stoi(g_rect_acc); } catch (...) {}
    auto* p = g_out->_ps.pan;
    if      (g_rect_i == 0) p->x = v;
    else if (g_rect_i == 1) p->y = v;
    else if (g_rect_i == 2) p->w = v;
    else if (g_rect_i == 3) p->h = v;
    g_rect_i++;
    g_rect_acc.clear();
  }
}

static inline void commit_z_if_pending() {
  if (!g_out || !g_out->_ps.pan) { g_zacc.clear(); return; }
  if (!g_zacc.empty()) {
    int v = 0; try { v = std::stoi(g_zacc); } catch (...) {}
    g_out->_ps.pan->z = v;
    g_zacc.clear();
  }
}

static inline void commit_scale_if_pending() {
  if (!g_out || !g_out->_ps.pan) { g_sacc.clear(); return; }
  if (!g_sacc.empty()) {
    float v = 1.0f; try { v = std::stof(g_sacc); } catch (...) {}
    g_out->_ps.pan->scale = v;
    g_sacc.clear();
  }
}

static inline void commit_present_if_pending() {
  if (!g_out || !g_out->_ps.arg) { g_present_k.clear(); g_present_v.clear(); return; }
  if (!g_present_k.empty() && !g_present_v.empty()) {
    g_out->_ps.arg->presenter.kv[g_present_k] = g_present_v;
    g_present_k.clear(); g_present_v.clear();
  }
}

static inline void commit_draw_kv_if_pending() {
  if (!g_out || !g_out->_ps.shp) { g_draw_k.clear(); g_draw_v.clear(); return; }
  if (!g_draw_k.empty() && !g_draw_v.empty()) {
    g_out->_ps.shp->kv[g_draw_k] = g_draw_v;
    g_draw_k.clear(); g_draw_v.clear();
  }
}

static inline void commit_trigger_if_pending() {
  if (!g_out || !g_out->_ps.arg) { g_trig_k.clear(); g_trig_v.clear(); return; }
  if (!g_trig_k.empty() && !g_trig_v.empty()) {
    g_out->_ps.arg->triggers.emplace_back(g_trig_k, g_trig_v);
    g_trig_k.clear(); g_trig_v.clear();
  }
}

/* Map draw op → shape kind */
static inline iinuji_renderings_instruction_t::shape_t::kind_e
op_to_kind(std::string op) {
  std::transform(op.begin(), op.end(), op.begin(),
                 [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  using kind_e = iinuji_renderings_instruction_t::shape_t::kind_e;
  if (op == "curve")        return kind_e::Curve;
  if (op == "mask_scatter") return kind_e::MaskScatter;
  if (op == "embedding")    return kind_e::Embedding;
  if (op == "mdn_band")     return kind_e::MdnBand;
  if (op == "text")         return kind_e::Text;
  return kind_e::Curve; // default/fallback
}

/* ─────────────────────────────────────────────
   iinujiRenderings implementation
   ───────────────────────────────────────────── */

iinujiRenderings::iinujiRenderings()
  : bnfLexer(IINUJI_RENDERINGS_BNF_GRAMMAR.empty()
               ? std::string("<bad_file>")
               : IINUJI_RENDERINGS_BNF_GRAMMAR)
  , bnfParser(bnfLexer)
  , grammar(parseBnfGrammar())
  , iParser(iLexer, grammar)
{}

ProductionGrammar iinujiRenderings::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

iinuji_renderings_instruction_t
iinujiRenderings::decode(std::string instruction) {
  std::lock_guard<std::mutex> guard(current_mutex);

  reset_all_accumulators();

  auto ast = iParser.parse_Instruction(instruction);

  iinuji_renderings_instruction_t out;
  g_out = &out;
  out._ps.scr = nullptr;
  out._ps.arg = nullptr;
  out._ps.pan = nullptr;
  out._ps.shp = nullptr;

  VisitorContext ctx(static_cast<void*>(g_out));
  ast->accept(*this, ctx);

  // Flush any dangling accumulators at end of parse
  commit_fcode_if_pending();
  commit_rect_if_pending();
  commit_z_if_pending();
  commit_scale_if_pending();
  commit_present_if_pending();
  commit_draw_kv_if_pending();
  commit_trigger_if_pending();

  g_out = nullptr;
  return out;
}

/* ───────────── ASTVisitor: Root ───────────── */
void iinujiRenderings::visit(const RootNode* /*node*/, VisitorContext& /*context*/) {
  // No-op
}

/* ───────────── ASTVisitor: Intermediary ───────────── */
void iinujiRenderings::visit(const IntermediaryNode* node, VisitorContext& context) {
  // Screen lifecycle
  if (node->name == "<screen>") {
    if (!g_out) return;
    g_out->screens.emplace_back();
    g_out->_ps.scr = &g_out->screens.back();
    g_out->_ps.arg = nullptr;
    g_out->_ps.pan = nullptr;
    g_out->_ps.shp = nullptr;
    g_fcacc.clear();
  }

  // Arg
  if (node->name == "<arg_stmt>") {
    if (!g_out || !g_out->_ps.scr) return;
    g_out->_ps.scr->args.emplace_back();
    g_out->_ps.arg = &g_out->_ps.scr->args.back();
    g_present_k.clear(); g_present_v.clear();
  }
  if (stack_ends_with(context, {"<arg_stmt>", "<path>"})) {
    if (g_out && g_out->_ps.arg) g_out->_ps.arg->path_from_Arg1.clear();
  }
  if (node->name == "<presented_by>") {
    if (g_out && g_out->_ps.arg) g_out->_ps.arg->presenter.name.clear();
  }
  if (node->name == "<triggers_block>") {
    g_trig_k.clear(); g_trig_v.clear();
  }

  // Panel
  if (node->name == "<panel_stmt>") {
    if (!g_out || !g_out->_ps.scr) return;
    g_out->_ps.scr->panels.emplace_back();
    g_out->_ps.pan = &g_out->_ps.scr->panels.back();
    g_out->_ps.pan->z = 0;
    g_out->_ps.pan->scale = 1.0f;
    g_rect_acc.clear(); g_rect_i = 0;
    g_zacc.clear();     g_sacc.clear();
  }

  // Draw
  if (node->name == "<draw_stmt>") {
    if (!g_out || !g_out->_ps.pan) return;
    g_out->_ps.pan->shapes.emplace_back();
    g_out->_ps.shp = &g_out->_ps.pan->shapes.back();
    g_draw_op.clear(); g_draw_k.clear(); g_draw_v.clear();
  }
}

/* ───────────── ASTVisitor: Terminal ───────────── */
void iinujiRenderings::visit(const TerminalNode* node, VisitorContext& context) {
  if (!g_out) return;

  auto* scr = g_out->_ps.scr;
  auto* arg = g_out->_ps.arg;
  auto* pan = g_out->_ps.pan;
  auto* shp = g_out->_ps.shp;

  const std::string& lhs = node->name;
  std::string lex       = node->unit.lexeme;
  std::string s         = strip_quotes(lex);

  // -------- generic "late" flushes when leaving clauses --------
  if (!under(context, "<fcode>"))                commit_fcode_if_pending();
  if (under(context, "<panel_stmt>") && !under(context, "<int>")) commit_rect_if_pending();
  if (!under(context, "<z_clause>"))            commit_z_if_pending();
  if (!under(context, "<scale_clause>"))        commit_scale_if_pending();
  if (!under(context, "<present_kv_pair>"))     commit_present_if_pending();
  if (!under(context, "<draw_kv_pair>"))        commit_draw_kv_if_pending();
  if (!under(context, "<trigger_kv_pair>"))     commit_trigger_if_pending();


  /* screen fcode */
  if (stack_ends_with(context, {"<screen>", "<fcode>"})) {
    if (lhs == "<digit>") {
      g_fcacc += s;
    } else {
      // allow 'F+<digits>' etc. commit when a non-digit appears inside <fcode>
      commit_fcode_if_pending();
    }
  }

  /* arg name */
  if (stack_ends_with(context, {"<arg_stmt>", "<arg_name>"})) {
    if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>") {
      if (arg) arg->name += s;
    }
  }

  /* path Arg1.something → store only the suffix after Arg1. */
  if (stack_ends_with(context, {"<arg_stmt>", "<path>"})) {
    if (s == "Arg1") {
      // skip the literal Arg1
    } else if (s == "." && arg && arg->path_from_Arg1.empty()) {
      // drop the leading dot immediately after Arg1
    } else if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>" || s == ".") {
      if (arg) arg->path_from_Arg1 += s;
    }
  }

  /* presented_by name */
  if (stack_ends_with(context, {"<presented_by>", "<ident>"})) {
    if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>") {
      if (arg) arg->presenter.name += s;
    }
  }
  /* presented_by k/v */
  if (stack_ends_with(context, {"<present_kv_pair>", "<ident>"})) {
    commit_present_if_pending();
    g_present_k += s;
  }
  if (stack_ends_with(context, {"<present_kv_pair>", "<kv_word>"})) {
    g_present_v += s;
    if (s == "triggers" || s == "panel" || s == "arg" || s == "endscreen" || s == "endpanel") {
      commit_present_if_pending();
    }
  }

  /* triggers: collect as (key value) pairs per grammar */
  if (stack_ends_with(context, {"<trigger_kv_pair>", "<ident>"})) {
    commit_trigger_if_pending();
    g_trig_k += s;
  }
  if (stack_ends_with(context, {"<trigger_kv_pair>", "<kv_word>"})) {
    if (!g_trig_v.empty()) g_trig_v += " ";
    g_trig_v += s;
  }
  if (stack_ends_with(context, {"<triggers_block>"})) {
    if (s == "endtriggers") commit_trigger_if_pending();
  }

  /* panel id/type */
  if (stack_ends_with(context, {"<panel_stmt>", "<panel_id>"})) {
    if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>") {
      if (pan) pan->id += s;
    }
  }
  if (stack_ends_with(context, {"<panel_stmt>", "<panel_type>"})) {
    if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>") {
      if (pan) pan->type += s;
    }
  }

  /* at x y w h */
  if (stack_ends_with(context, {"<panel_stmt>"})) {
    if (stack_ends_with(context, {"<panel_stmt>", "<int>"})) {
      if (lhs == "<digit>") {
        g_rect_acc += s;
      }
    }
    if (s == "z" || s == "scale" || s == "bind" || s == "draw" || s == "endpanel") {
      commit_rect_if_pending();
      if (s == "endpanel") g_rect_i = 0;
    }
  }

  /* z */
  if (stack_ends_with(context, {"<panel_stmt>", "<z_clause>", "<int>"})) {
    if (lhs == "<digit>") {
      g_zacc += s;
    } else {
      commit_z_if_pending();
    }
    if (s == "scale" || s == "bind" || s == "draw" || s == "endpanel") {
      commit_z_if_pending();
    }
  }

  /* scale */
  if (stack_ends_with(context, {"<panel_stmt>", "<scale_clause>"})) {
    if (lhs == "<digit>" || s == ".") {
      g_sacc += s;
    } else {
      commit_scale_if_pending();
    }
    if (s == "bind" || s == "draw" || s == "endpanel") {
      commit_scale_if_pending();
    }
  }

  /* bind ArgN */
  if (stack_ends_with(context, {"<panel_stmt>", "<bind_clause>", "<ident>"})) {
    if (lhs == "<ident>" || lhs == "<alpha>" || lhs == "<id_tail>" || lhs == "<digit>") {
      if (pan) pan->bind_arg += s;
    }
  }

  /* draw: op + kv */
  if (stack_ends_with(context, {"<draw_stmt>", "<ident>"})) {
    if (shp && g_draw_op.empty()) {
      g_draw_op = s;
      shp->kind = op_to_kind(g_draw_op);
      shp->kv["op"] = g_draw_op; // preserve original op text
    }
  }
  if (stack_ends_with(context, {"<draw_kv_pair>", "<ident>"})) {
    commit_draw_kv_if_pending();
    g_draw_k += s;
  }
  if (stack_ends_with(context, {"<draw_kv_pair>", "<kv_word>"})) {
    g_draw_v += s;
    if (s == "draw" || s == "endpanel") {
      commit_draw_kv_if_pending();
    }
  }

  /* routes - ignored for now */
}

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
