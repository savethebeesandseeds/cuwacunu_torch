/* tsiemene_circuit_decode_text.cpp */
#include "tsiemene_circuit_decode_internal.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {
namespace tsiemene_circuit_decode_internal {

namespace {

std::string unescape_like_parser(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (std::size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
        case 'n':
          result += '\n';
          ++i;
          break;
        case 'r':
          result += '\r';
          ++i;
          break;
        case 't':
          result += '\t';
          ++i;
          break;
        case '\\':
          result += '\\';
          ++i;
          break;
        case '"':
          result += '"';
          ++i;
          break;
        case '\'':
          result += '\'';
          ++i;
          break;
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

const ASTNode* find_direct_child_by_hash(const IntermediaryNode* parent,
                                         std::size_t wanted_hash) {
  if (!parent) return nullptr;
  for (const auto& ch : parent->children) {
    if (ch && ch->hash == wanted_hash) return ch.get();
  }
  return nullptr;
}

bool parse_endpoint_text(const std::string& endpoint_text,
                         cuwacunu::camahjucunu::tsiemene_endpoint_t* out,
                         bool require_kind) {
  if (!out) return false;
  std::string line = normalize_line(endpoint_text);
  const std::size_t at = line.find('@');
  if (at == std::string::npos || at == 0 || at + 1 >= line.size()) {
    return false;
  }
  if (require_kind) {
    const std::size_t colon = line.rfind(':');
    if (colon == std::string::npos || colon <= at + 1 || colon + 1 >= line.size()) {
      return false;
    }
    out->instance = trim_ascii_ws(line.substr(0, at));
    out->directive = trim_ascii_ws(line.substr(at + 1, colon - (at + 1)));
    out->kind = trim_ascii_ws(line.substr(colon + 1));
    return !(out->instance.empty() || out->directive.empty() || out->kind.empty());
  }

  if (line.find(':', at + 1) != std::string::npos) return false;
  out->instance = trim_ascii_ws(line.substr(0, at));
  out->directive = trim_ascii_ws(line.substr(at + 1));
  out->kind.clear();
  return !(out->instance.empty() || out->directive.empty());
}

bool parse_instance_decl_text(
    const std::string& decl_text,
    cuwacunu::camahjucunu::tsiemene_instance_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) return false;
  out->alias = trim_ascii_ws(line.substr(0, eq));
  out->tsi_type = trim_ascii_ws(line.substr(eq + 1));
  return !(out->alias.empty() || out->tsi_type.empty());
}

bool parse_hop_decl_text(const std::string& decl_text,
                         cuwacunu::camahjucunu::tsiemene_hop_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t arrow = line.find("->");
  if (arrow == std::string::npos || arrow == 0 || arrow + 2 >= line.size()) return false;
  const std::string lhs = trim_ascii_ws(line.substr(0, arrow));
  const std::string rhs = trim_ascii_ws(line.substr(arrow + 2));
  if (!parse_endpoint_text(lhs, &out->from, /*require_kind=*/true)) return false;
  if (!parse_endpoint_text(rhs, &out->to, /*require_kind=*/false)) return false;
  return true;
}

bool parse_circuit_header_text(const std::string& header_text,
                               std::string* out_name) {
  if (!out_name) return false;
  std::string line = normalize_line(header_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0) return false;
  *out_name = trim_ascii_ws(line.substr(0, eq));
  return !out_name->empty();
}

bool parse_circuit_invoke_text(const std::string& invoke_text,
                               std::string* out_name,
                               std::string* out_payload) {
  if (!out_name || !out_payload) return false;
  std::string line = normalize_line(invoke_text);
  if (!line.empty() && line.back() == ';') {
    line.pop_back();
    line = trim_ascii_ws(std::move(line));
  }
  const std::size_t lp = line.find('(');
  const std::size_t rp = line.rfind(')');
  if (lp == std::string::npos || rp == std::string::npos || lp == 0 || rp <= lp) {
    return false;
  }
  *out_name = trim_ascii_ws(line.substr(0, lp));
  *out_payload = trim_ascii_ws(line.substr(lp + 1, rp - (lp + 1)));
  return !out_name->empty();
}

} /* namespace */

cuwacunu::camahjucunu::tsiemene_circuit_decl_t parse_circuit_node(
    const IntermediaryNode* node) {
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t out{};
  if (!node) return out;

  if (const ASTNode* n_header =
          find_direct_child_by_hash(node, TSIEMENE_CIRCUIT_HASH_circuit_header)) {
    const auto* im = dynamic_cast<const IntermediaryNode*>(n_header);
    if (im) {
      if (const ASTNode* n_name =
              find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_circuit_name)) {
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

    if (im->hash == TSIEMENE_CIRCUIT_HASH_instance_decl) {
      cuwacunu::camahjucunu::tsiemene_instance_decl_t inst{};
      const ASTNode* n_alias =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_instance_alias);
      const ASTNode* n_type =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_tsi_type);
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

    if (im->hash == TSIEMENE_CIRCUIT_HASH_hop_decl) {
      cuwacunu::camahjucunu::tsiemene_hop_decl_t hop{};
      const ASTNode* n_from =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_endpoint_from);
      const ASTNode* n_to =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_endpoint_to);
      bool ok = false;
      if (n_from && n_to) {
        ok = parse_endpoint_text(
                 flatten_node_text(n_from), &hop.from, /*require_kind=*/true) &&
             parse_endpoint_text(
                 flatten_node_text(n_to), &hop.to, /*require_kind=*/false);
      }
      if (!ok) ok = parse_hop_decl_text(flatten_node_text(im), &hop);
      if (ok) out.hops.push_back(std::move(hop));
      continue;
    }

    if (im->hash == TSIEMENE_CIRCUIT_HASH_circuit_invoke) {
      const ASTNode* n_name =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_invoke_name);
      const ASTNode* n_payload =
          find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_invoke_payload);
      if (n_name && n_payload) {
        out.invoke_name = trim_ascii_ws(flatten_node_text(n_name));
        out.invoke_payload = trim_ascii_ws(flatten_node_text(n_payload));
      } else {
        parse_circuit_invoke_text(
            flatten_node_text(im), &out.invoke_name, &out.invoke_payload);
      }
      continue;
    }
  }

  if (out.name.empty()) out.name = out.invoke_name;
  if (out.invoke_name.empty()) out.invoke_name = out.name;
  return out;
}

} /* namespace tsiemene_circuit_decode_internal */
} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
