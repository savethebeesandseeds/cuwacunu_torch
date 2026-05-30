#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

[[nodiscard]] bool has_non_ws_ascii_(std::string_view text) {
  for (const char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) return true;
  }
  return false;
}

[[nodiscard]] const std::string& require_non_ws_grammar_text_(
    const std::string& grammar_text) {
  if (!has_non_ws_ascii_(grammar_text)) {
    throw std::runtime_error("latent_lineage_state grammar text is empty");
  }
  return grammar_text;
}

[[nodiscard]] std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string strip_comment_line_(std::string_view line,
                                              bool* in_block_comment) {
  bool block_comment = (in_block_comment != nullptr) ? *in_block_comment : false;
  bool in_single = false;
  bool in_double = false;
  std::string out;
  out.reserve(line.size());

  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';

    if (block_comment) {
      if (c == '*' && next == '/') {
        block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }

    if (!in_single && !in_double && c == '/' && next == '*') {
      block_comment = true;
      i += 2;
      continue;
    }

    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      ++i;
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      ++i;
      continue;
    }
    if ((c == '#' || c == ';') && !in_single && !in_double) {
      break;
    }

    out.push_back(c);
    ++i;
  }

  if (in_block_comment != nullptr) *in_block_comment = block_comment;
  return out;
}

[[nodiscard]] std::string strip_comments_preserve_lines_(std::string_view text) {
  std::istringstream input{std::string(text)};
  std::string line;
  std::string out;
  bool in_block_comment = false;
  while (std::getline(input, line)) {
    out.append(strip_comment_line_(line, &in_block_comment));
    out.push_back('\n');
  }
  return out;
}

[[nodiscard]] std::string terminal_text_from_unit_(const cuwacunu::camahjucunu::dsl::ProductionUnit& unit) {
  std::string lex = unit.lexeme;
  if (lex.size() >= 2 &&
      ((lex.front() == '"' && lex.back() == '"') ||
       (lex.front() == '\'' && lex.back() == '\''))) {
    lex = lex.substr(1, lex.size() - 2);
  }
  std::string out;
  out.reserve(lex.size());
  for (std::size_t i = 0; i < lex.size(); ++i) {
    if (lex[i] == '\\' && i + 1 < lex.size()) {
      const char esc = lex[++i];
      switch (esc) {
        case 'n':
          out.push_back('\n');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case 't':
          out.push_back('\t');
          break;
        case '\\':
          out.push_back('\\');
          break;
        case '"':
          out.push_back('"');
          break;
        case '\'':
          out.push_back('\'');
          break;
        default:
          out.push_back('\\');
          out.push_back(esc);
          break;
      }
      continue;
    }
    out.push_back(lex[i]);
  }
  return out;
}

void append_all_terminals_(const cuwacunu::camahjucunu::dsl::ASTNode* node,
                           std::string* out) {
  if (!node || !out) return;
  if (const auto* term =
          dynamic_cast<const cuwacunu::camahjucunu::dsl::TerminalNode*>(node)) {
    if (term->unit.type == cuwacunu::camahjucunu::dsl::ProductionUnit::Type::Terminal) {
      out->append(terminal_text_from_unit_(term->unit));
    }
    return;
  }
  if (const auto* root =
          dynamic_cast<const cuwacunu::camahjucunu::dsl::RootNode*>(node)) {
    for (const auto& child : root->children) {
      append_all_terminals_(child.get(), out);
    }
    return;
  }
  if (const auto* mid =
          dynamic_cast<const cuwacunu::camahjucunu::dsl::IntermediaryNode*>(node)) {
    for (const auto& child : mid->children) {
      append_all_terminals_(child.get(), out);
    }
  }
}

[[nodiscard]] std::string flatten_node_text_(
    const cuwacunu::camahjucunu::dsl::ASTNode* node) {
  std::string out;
  append_all_terminals_(node, &out);
  return out;
}

void collect_kv_line_nodes_(
    const cuwacunu::camahjucunu::dsl::ASTNode* node,
    std::vector<const cuwacunu::camahjucunu::dsl::IntermediaryNode*>* out) {
  if (!node || !out) return;
  if (const auto* root =
          dynamic_cast<const cuwacunu::camahjucunu::dsl::RootNode*>(node)) {
    for (const auto& child : root->children) {
      collect_kv_line_nodes_(child.get(), out);
    }
    return;
  }
  if (const auto* mid =
          dynamic_cast<const cuwacunu::camahjucunu::dsl::IntermediaryNode*>(node)) {
    if (mid->name == "<kv_line>") out->push_back(mid);
    for (const auto& child : mid->children) {
      collect_kv_line_nodes_(child.get(), out);
    }
  }
}

[[nodiscard]] const cuwacunu::camahjucunu::dsl::ASTNode* find_direct_child_by_name_(
    const cuwacunu::camahjucunu::dsl::IntermediaryNode* parent,
    std::string_view name) {
  if (!parent) return nullptr;
  for (const auto& child : parent->children) {
    if (child && child->name == name) return child.get();
  }
  return nullptr;
}

}  // namespace

namespace cuwacunu {
namespace camahjucunu {

std::string latent_lineage_state_instruction_t::str() const {
  std::ostringstream oss;
  oss << "latent_lineage_state_instruction_t: entries=" << entries.size() << "\n";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto& e = entries[i];
    oss << "  [" << i << "] key=" << e.key
        << " domain=" << e.declared_domain
        << " type=" << e.declared_type
        << " value=" << e.value
        << " line=" << e.line << "\n";
  }
  return oss.str();
}

std::map<std::string, std::string> latent_lineage_state_instruction_t::to_map() const {
  std::map<std::string, std::string> out;
  for (const auto& e : entries) {
    if (!e.key.empty()) out[e.key] = e.value;
  }
  return out;
}

namespace dsl {

latentLineageStatePipeline::latentLineageStatePipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)),
      grammar_lexer_(require_non_ws_grammar_text_(grammar_text_)),
      grammar_parser_(grammar_lexer_),
      grammar_(parseGrammarDefinition()),
      instruction_parser_(instruction_lexer_, grammar_) {}

latent_lineage_state_instruction_t latentLineageStatePipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);

  latent_lineage_state_instruction_t out{};
  const std::string normalized =
      strip_comments_preserve_lines_(std::string_view(instruction));
  ASTNodePtr parsed_ast = instruction_parser_.parse_Instruction(normalized);

  std::vector<const IntermediaryNode*> kv_lines{};
  collect_kv_line_nodes_(parsed_ast.get(), &kv_lines);
  for (std::size_t idx = 0; idx < kv_lines.size(); ++idx) {
    const auto* kv_node = kv_lines[idx];
    const ASTNode* lhs_node = find_direct_child_by_name_(kv_node, "<lhs>");
    if (!lhs_node) {
      throw std::runtime_error("latent_lineage_state decode error: kv_line missing <lhs>");
    }
    const ASTNode* value_node = find_direct_child_by_name_(kv_node, "<value>");

    const std::string lhs = trim_ascii_copy(flatten_node_text_(lhs_node));
    const std::string rhs =
        value_node ? trim_ascii_copy(flatten_node_text_(value_node)) : std::string{};
    latent_lineage_state_entry_t e{};
    const auto parsed = dsl::parse_latent_lineage_state_lhs(lhs);
    e.key = parsed.key;
    e.declared_domain = parsed.declared_domain;
    e.declared_type = parsed.declared_type;
    if (e.key.empty()) {
      throw std::runtime_error("latent_lineage_state decode error: empty key in <kv_line>");
    }
    e.value = std::move(rhs);
    e.line = idx + 1;
    out.entries.push_back(std::move(e));
  }

  return out;
}

ProductionGrammar latentLineageStatePipeline::parseGrammarDefinition() {
  grammar_parser_.parseGrammar();
  return grammar_parser_.getGrammar();
}

latent_lineage_state_instruction_t decode_latent_lineage_state_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  latentLineageStatePipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
