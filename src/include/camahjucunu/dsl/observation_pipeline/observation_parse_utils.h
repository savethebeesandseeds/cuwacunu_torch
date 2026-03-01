#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "camahjucunu/dsl/parser_types.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

namespace detail {

inline std::string unescape_like_parser(const std::string& str) {
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

inline std::string terminal_text_from_unit(const ProductionUnit& unit) {
  std::string lex = unit.lexeme;

  if (lex.size() >= 2) {
    if ((lex.front() == '"' && lex.back() == '"') ||
        (lex.front() == '\'' && lex.back() == '\'')) {
      lex = lex.substr(1, lex.size() - 2);
    }
  }

  return unescape_like_parser(lex);
}

inline std::string trim_spaces_tabs(std::string s) {
  auto is_ws = [](unsigned char c) { return c == ' ' || c == '\t'; };

  std::size_t start = 0;
  while (start < s.size() && is_ws(static_cast<unsigned char>(s[start]))) start++;

  std::size_t end = s.size();
  while (end > start && is_ws(static_cast<unsigned char>(s[end - 1]))) end--;

  return s.substr(start, end - start);
}

inline void append_all_terminals(const ASTNode* node, std::string& out) {
  if (!node) return;

  if (auto term = dynamic_cast<const TerminalNode*>(node)) {
    if (term->unit.type == ProductionUnit::Type::Terminal) {
      out += terminal_text_from_unit(term->unit);
    }
    return;
  }

  if (auto root = dynamic_cast<const RootNode*>(node)) {
    for (const auto& ch : root->children) append_all_terminals(ch.get(), out);
    return;
  }

  if (auto mid = dynamic_cast<const IntermediaryNode*>(node)) {
    for (const auto& ch : mid->children) append_all_terminals(ch.get(), out);
    return;
  }
}

inline std::string flatten_node_text(const ASTNode* node) {
  std::string out;
  append_all_terminals(node, out);
  return out;
}

inline const ASTNode* find_direct_child_by_hash(const IntermediaryNode* parent,
                                                 uint64_t wanted_hash) {
  if (!parent) return nullptr;
  for (const auto& ch : parent->children) {
    if (ch && ch->hash == wanted_hash) return ch.get();
  }
  return nullptr;
}

} // namespace detail
} // namespace dsl
} // namespace camahjucunu
} // namespace cuwacunu
