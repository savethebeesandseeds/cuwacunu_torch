/* tsiemene_board.cpp */
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"

#include <cctype>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {

namespace {

std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

} /* namespace */

std::optional<tsiemene::DirectiveId> parse_directive_ref(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (!s.empty() && s.front() == '@') s.erase(s.begin());
  if (s == "payload") return tsiemene::directive_id::Payload;
  if (s == "loss") return tsiemene::directive_id::Loss;
  if (s == "meta") return tsiemene::directive_id::Meta;
  return std::nullopt;
}

std::optional<tsiemene::PayloadKind> parse_kind_ref(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (s == "tensor" || s == ":tensor") return tsiemene::PayloadKind::Tensor;
  if (s == "str" || s == ":str") return tsiemene::PayloadKind::String;
  return std::nullopt;
}

std::string circuit_invoke_symbol(const tsiemene_circuit_decl_t& circuit) {
  std::string s = trim_ascii_ws_copy(circuit.invoke_payload);
  const std::size_t lb = s.find('[');
  if (lb == std::string::npos) return s;
  return trim_ascii_ws_copy(s.substr(0, lb));
}

bool resolve_hops(
    const tsiemene_circuit_decl_t& circuit,
    std::vector<tsiemene_resolved_hop_t>* out_hops,
    std::string* error) {
  if (!out_hops) return false;
  out_hops->clear();
  out_hops->reserve(circuit.hops.size());

  for (const auto& h : circuit.hops) {
    tsiemene_resolved_hop_t resolved{};
    if (!resolve_hop_decl(h, &resolved, error)) return false;
    out_hops->push_back(std::move(resolved));
  }

  return true;
}

bool resolve_hop_decl(const tsiemene_hop_decl_t& hop,
                      tsiemene_resolved_hop_t* out,
                      std::string* error) {
  if (!out) return false;

  const auto from_dir = parse_directive_ref(hop.from.directive);
  const auto to_dir = parse_directive_ref(hop.to.directive);
  const auto from_kind = parse_kind_ref(hop.from.kind);
  const auto to_kind = parse_kind_ref(hop.to.kind);
  if (!from_dir || !to_dir || !from_kind || !to_kind) {
    if (error) {
      *error = "invalid directive/kind in hop: " +
               hop.from.instance + "@" + hop.from.directive + ":" + hop.from.kind +
               " -> " +
               hop.to.instance + "@" + hop.to.directive + ":" + hop.to.kind;
    }
    return false;
  }

  out->from.instance = hop.from.instance;
  out->from.directive = *from_dir;
  out->from.kind = *from_kind;

  out->to.instance = hop.to.instance;
  out->to.directive = *to_dir;
  out->to.kind = *to_kind;
  return true;
}

bool validate_circuit_decl(const tsiemene_circuit_decl_t& circuit,
                           std::string* error) {
  const std::string circuit_name = trim_ascii_ws_copy(circuit.name);
  if (circuit_name.empty()) {
    if (error) *error = "empty circuit name";
    return false;
  }
  if (trim_ascii_ws_copy(circuit.invoke_name).empty()) {
    if (error) *error = "empty circuit invoke name";
    return false;
  }
  if (trim_ascii_ws_copy(circuit.invoke_payload).empty()) {
    if (error) *error = "empty circuit invoke payload";
    return false;
  }
  if (circuit.instances.empty()) {
    if (error) *error = "circuit has no instance declarations";
    return false;
  }
  if (circuit.hops.empty()) {
    if (error) *error = "circuit has no hop declarations";
    return false;
  }

  std::unordered_map<std::string, std::string> alias_to_type;
  alias_to_type.reserve(circuit.instances.size());
  for (const auto& inst : circuit.instances) {
    const std::string alias = trim_ascii_ws_copy(inst.alias);
    const std::string type = trim_ascii_ws_copy(inst.tsi_type);
    if (alias.empty()) {
      if (error) *error = "empty instance alias";
      return false;
    }
    if (type.empty()) {
      if (error) *error = "empty tsi_type for alias: " + alias;
      return false;
    }
    if (!alias_to_type.emplace(alias, type).second) {
      if (error) *error = "duplicated instance alias: " + alias;
      return false;
    }
  }

  std::vector<tsiemene_resolved_hop_t> resolved_hops;
  if (!resolve_hops(circuit, &resolved_hops, error)) return false;

  std::unordered_map<std::string, std::vector<std::string>> adj;
  std::unordered_map<std::string, std::size_t> in_degree;
  std::unordered_map<std::string, std::size_t> out_degree;
  std::unordered_set<std::string> referenced;
  referenced.reserve(circuit.instances.size());

  for (const auto& h : resolved_hops) {
    if (alias_to_type.find(h.from.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.from.instance;
      return false;
    }
    if (alias_to_type.find(h.to.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.to.instance;
      return false;
    }
    referenced.insert(h.from.instance);
    referenced.insert(h.to.instance);
    adj[h.from.instance].push_back(h.to.instance);
    (void)adj[h.to.instance];
    ++in_degree[h.to.instance];
    (void)in_degree[h.from.instance];
    ++out_degree[h.from.instance];
    (void)out_degree[h.to.instance];
  }

  if (referenced.empty()) {
    if (error) *error = "no valid hop endpoints";
    return false;
  }

  for (const auto& [alias, _] : alias_to_type) {
    if (!referenced.count(alias)) {
      if (error) *error = "orphan instance not referenced by any hop: " + alias;
      return false;
    }
  }

  std::vector<std::string> roots;
  roots.reserve(referenced.size());
  for (const auto& alias : referenced) {
    const auto in_it = in_degree.find(alias);
    const std::size_t id = (in_it == in_degree.end()) ? 0 : in_it->second;
    if (id == 0) roots.push_back(alias);
  }

  if (roots.empty()) {
    if (error) *error = "circuit has no root instance";
    return false;
  }
  if (roots.size() != 1) {
    if (error) *error = "circuit must have exactly one root instance";
    return false;
  }

  std::unordered_map<std::string, int> color;
  std::unordered_set<std::string> reachable;
  bool cycle = false;

  std::function<void(const std::string&)> dfs = [&](const std::string& u) {
    if (cycle) return;
    color[u] = 1;
    reachable.insert(u);
    const auto it = adj.find(u);
    if (it != adj.end()) {
      for (const std::string& v : it->second) {
        const int state = color[v];
        if (state == 1) {
          cycle = true;
          return;
        }
        if (state == 0) dfs(v);
        if (cycle) return;
      }
    }
    color[u] = 2;
  };

  dfs(roots[0]);
  if (cycle) {
    if (error) *error = "cycle detected in circuit hops";
    return false;
  }

  if (reachable.size() != referenced.size()) {
    if (error) *error = "unreachable instance from circuit root";
    return false;
  }

  for (const auto& alias : referenced) {
    const auto od_it = out_degree.find(alias);
    const std::size_t od = (od_it == out_degree.end()) ? 0 : od_it->second;
    if (od != 0) continue;
    const std::string& type = alias_to_type[alias];
    constexpr const char* kSinkPrefix = "tsi.sink.";
    if (type.compare(0, 9, kSinkPrefix) != 0) {
      if (error) *error = "terminal instance must be sink type: " + alias + "=" + type;
      return false;
    }
  }

  return true;
}

bool validate_board_instruction(const tsiemene_board_instruction_t& board,
                                std::string* error) {
  if (board.circuits.empty()) {
    if (error) *error = "board has no circuits";
    return false;
  }

  std::unordered_set<std::string> circuit_names;
  std::unordered_set<std::string> invoke_names;
  circuit_names.reserve(board.circuits.size());
  invoke_names.reserve(board.circuits.size());

  for (std::size_t i = 0; i < board.circuits.size(); ++i) {
    const auto& c = board.circuits[i];
    const std::string cname = trim_ascii_ws_copy(c.name);
    const std::string iname = trim_ascii_ws_copy(c.invoke_name);
    if (!circuit_names.insert(cname).second) {
      if (error) *error = "duplicated circuit name: " + cname;
      return false;
    }
    if (!invoke_names.insert(iname).second) {
      if (error) *error = "duplicated circuit invoke name: " + iname;
      return false;
    }

    std::string local_error;
    if (!validate_circuit_decl(c, &local_error)) {
      if (error) *error = "circuit[" + std::to_string(i) + "] " + local_error;
      return false;
    }
  }

  return true;
}

std::string tsiemene_board_instruction_t::str() const {
  std::ostringstream oss;
  oss << "tsiemene_board_instruction_t: circuits=" << circuits.size() << "\n";
  for (std::size_t i = 0; i < circuits.size(); ++i) {
    const auto& c = circuits[i];
    oss << "  [" << i << "] " << c.name
        << " instances=" << c.instances.size()
        << " hops=" << c.hops.size()
        << " invoke=" << c.invoke_name
        << "(\"" << c.invoke_payload << "\")\n";
  }
  return oss.str();
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

namespace {

std::string unescape_like_parser(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (std::size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
        case 'n': result += '\n'; ++i; break;
        case 'r': result += '\r'; ++i; break;
        case 't': result += '\t'; ++i; break;
        case '\\': result += '\\'; ++i; break;
        case '"': result += '"'; ++i; break;
        case '\'': result += '\''; ++i; break;
        default:
          result += '\\';
          result += str[i + 1];
          ++i;
          break;
      }
    } else {
      result += str[i];
    }
  }
  return result;
}

std::string terminal_text_from_unit(const ProductionUnit& unit) {
  std::string lex = unit.lexeme;
  if (lex.size() >= 2 &&
      ((lex.front() == '"' && lex.back() == '"') ||
       (lex.front() == '\'' && lex.back() == '\''))) {
    lex = lex.substr(1, lex.size() - 2);
  }
  return unescape_like_parser(lex);
}

void append_all_terminals(const ASTNode* node, std::string& out) {
  if (!node) return;

  if (const auto* term = dynamic_cast<const TerminalNode*>(node)) {
    if (term->unit.type == ProductionUnit::Type::Terminal) {
      out += terminal_text_from_unit(term->unit);
    }
    return;
  }

  if (const auto* root = dynamic_cast<const RootNode*>(node)) {
    for (const auto& ch : root->children) append_all_terminals(ch.get(), out);
    return;
  }

  if (const auto* mid = dynamic_cast<const IntermediaryNode*>(node)) {
    for (const auto& ch : mid->children) append_all_terminals(ch.get(), out);
    return;
  }
}

std::string flatten_node_text(const ASTNode* node) {
  std::string out;
  append_all_terminals(node, out);
  return out;
}

std::string trim_ascii_ws(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::string normalize_line(std::string s) {
  for (char& c : s) {
    if (c == '\r' || c == '\n') c = ' ';
  }
  return trim_ascii_ws(std::move(s));
}

const ASTNode* find_direct_child_by_hash(const IntermediaryNode* parent, std::size_t wanted_hash) {
  if (!parent) return nullptr;
  for (const auto& ch : parent->children) {
    if (ch && ch->hash == wanted_hash) return ch.get();
  }
  return nullptr;
}

bool parse_endpoint_text(const std::string& endpoint_text, cuwacunu::camahjucunu::tsiemene_endpoint_t* out) {
  if (!out) return false;
  std::string line = normalize_line(endpoint_text);
  const std::size_t at = line.find('@');
  const std::size_t colon = line.rfind(':');
  if (at == std::string::npos || colon == std::string::npos || at == 0 || colon <= at + 1 || colon + 1 >= line.size()) {
    return false;
  }
  out->instance = trim_ascii_ws(line.substr(0, at));
  out->directive = trim_ascii_ws(line.substr(at + 1, colon - (at + 1)));
  out->kind = trim_ascii_ws(line.substr(colon + 1));
  return !(out->instance.empty() || out->directive.empty() || out->kind.empty());
}

bool parse_instance_decl_text(const std::string& decl_text, cuwacunu::camahjucunu::tsiemene_instance_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) return false;
  out->alias = trim_ascii_ws(line.substr(0, eq));
  out->tsi_type = trim_ascii_ws(line.substr(eq + 1));
  return !(out->alias.empty() || out->tsi_type.empty());
}

bool parse_hop_decl_text(const std::string& decl_text, cuwacunu::camahjucunu::tsiemene_hop_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t arrow = line.find("->");
  if (arrow == std::string::npos || arrow == 0 || arrow + 2 >= line.size()) return false;
  const std::string lhs = trim_ascii_ws(line.substr(0, arrow));
  const std::string rhs = trim_ascii_ws(line.substr(arrow + 2));
  if (!parse_endpoint_text(lhs, &out->from)) return false;
  if (!parse_endpoint_text(rhs, &out->to)) return false;
  return true;
}

bool parse_circuit_header_text(const std::string& header_text, std::string* out_name) {
  if (!out_name) return false;
  std::string line = normalize_line(header_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0) return false;
  *out_name = trim_ascii_ws(line.substr(0, eq));
  return !out_name->empty();
}

bool parse_circuit_invoke_text(const std::string& invoke_text, std::string* out_name, std::string* out_payload) {
  if (!out_name || !out_payload) return false;
  std::string line = normalize_line(invoke_text);
  if (!line.empty() && line.back() == ';') {
    line.pop_back();
    line = trim_ascii_ws(std::move(line));
  }
  const std::size_t lp = line.find('(');
  const std::size_t rp = line.rfind(')');
  if (lp == std::string::npos || rp == std::string::npos || lp == 0 || rp <= lp) return false;
  *out_name = trim_ascii_ws(line.substr(0, lp));
  *out_payload = trim_ascii_ws(line.substr(lp + 1, rp - (lp + 1)));
  return !out_name->empty();
}

cuwacunu::camahjucunu::tsiemene_circuit_decl_t parse_circuit_node(const IntermediaryNode* node) {
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t out{};
  if (!node) return out;

  if (const ASTNode* n_header = find_direct_child_by_hash(node, TSIEMENE_BOARD_HASH_circuit_header)) {
    const auto* im = dynamic_cast<const IntermediaryNode*>(n_header);
    if (im) {
      if (const ASTNode* n_name = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_circuit_name)) {
        out.name = trim_ascii_ws(flatten_node_text(n_name));
      }
    }
    if (out.name.empty()) {
      parse_circuit_header_text(flatten_node_text(n_header), &out.name);
    }
  }

  for (const auto& ch : node->children) {
    const auto* im = dynamic_cast<const IntermediaryNode*>(ch.get());
    if (!im) continue;

    if (im->hash == TSIEMENE_BOARD_HASH_instance_decl) {
      cuwacunu::camahjucunu::tsiemene_instance_decl_t inst{};
      const ASTNode* n_alias = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_instance_alias);
      const ASTNode* n_type = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_tsi_type);
      if (n_alias && n_type) {
        inst.alias = trim_ascii_ws(flatten_node_text(n_alias));
        inst.tsi_type = trim_ascii_ws(flatten_node_text(n_type));
      } else {
        parse_instance_decl_text(flatten_node_text(im), &inst);
      }
      if (!inst.alias.empty() && !inst.tsi_type.empty()) {
        out.instances.push_back(std::move(inst));
      }
      continue;
    }

    if (im->hash == TSIEMENE_BOARD_HASH_hop_decl) {
      cuwacunu::camahjucunu::tsiemene_hop_decl_t hop{};
      const ASTNode* n_from = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_endpoint_from);
      const ASTNode* n_to = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_endpoint_to);
      bool ok = false;
      if (n_from && n_to) {
        ok = parse_endpoint_text(flatten_node_text(n_from), &hop.from) &&
             parse_endpoint_text(flatten_node_text(n_to), &hop.to);
      }
      if (!ok) ok = parse_hop_decl_text(flatten_node_text(im), &hop);
      if (ok) out.hops.push_back(std::move(hop));
      continue;
    }

    if (im->hash == TSIEMENE_BOARD_HASH_circuit_invoke) {
      const ASTNode* n_name = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_invoke_name);
      const ASTNode* n_payload = find_direct_child_by_hash(im, TSIEMENE_BOARD_HASH_invoke_payload);
      if (n_name && n_payload) {
        out.invoke_name = trim_ascii_ws(flatten_node_text(n_name));
        out.invoke_payload = trim_ascii_ws(flatten_node_text(n_payload));
      } else {
        parse_circuit_invoke_text(flatten_node_text(im), &out.invoke_name, &out.invoke_payload);
      }
      continue;
    }
  }

  if (out.name.empty()) out.name = out.invoke_name;
  if (out.invoke_name.empty()) out.invoke_name = out.name;
  return out;
}

} /* namespace */

tsiemeneBoard::tsiemeneBoard()
  : bnfLexer(TSIEMENE_BOARD_BNF_GRAMMAR)
  , bnfParser(bnfLexer)
  , grammar(parseBnfGrammar())
  , iParser(iLexer, grammar)
{
#ifdef TSIEMENE_BOARD_DEBUG
  log_info("%s\n", TSIEMENE_BOARD_BNF_GRAMMAR);
#endif
}

cuwacunu::camahjucunu::tsiemene_board_instruction_t
tsiemeneBoard::decode(std::string instruction) {
#ifdef TSIEMENE_BOARD_DEBUG
  log_info("Request to decode tsiemeneBoard\n");
#endif
  LOCK_GUARD(current_mutex);

  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef TSIEMENE_BOARD_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  cuwacunu::camahjucunu::tsiemene_board_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));
  actualAST.get()->accept(*this, context);
  return current;
}

ProductionGrammar tsiemeneBoard::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

void tsiemeneBoard::visit(const RootNode* node, VisitorContext& context) {
#ifdef TSIEMENE_BOARD_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
  (void)node;
  (void)context;
}

void tsiemeneBoard::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef TSIEMENE_BOARD_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif
  auto* out = static_cast<cuwacunu::camahjucunu::tsiemene_board_instruction_t*>(context.user_data);
  if (!out) return;

  if (node->hash == TSIEMENE_BOARD_HASH_instruction) {
    out->circuits.clear();
    return;
  }
  if (node->hash == TSIEMENE_BOARD_HASH_circuit) {
    auto circuit = parse_circuit_node(node);
    if (!circuit.name.empty()) {
      out->circuits.push_back(std::move(circuit));
    }
    return;
  }
}

void tsiemeneBoard::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef TSIEMENE_BOARD_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif
  (void)node;
  (void)context;
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
