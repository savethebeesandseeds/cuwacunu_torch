/* network_design.cpp */
#include "camahjucunu/dsl/network_design/network_design.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace {

[[nodiscard]] bool has_non_ws_ascii_(std::string_view text) {
  for (const char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) return true;
  }
  return false;
}

[[nodiscard]] std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string lower_ascii_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
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
    if ((c == '#' || (c == '/' && next == '/')) && !in_single && !in_double) {
      break;
    }

    out.push_back(c);
    ++i;
  }

  if (in_block_comment != nullptr) *in_block_comment = block_comment;
  return out;
}

[[nodiscard]] std::vector<std::pair<std::size_t, std::string>>
normalized_lines_with_numbers_(std::string_view text) {
  std::istringstream input{std::string(text)};
  std::string line;
  bool in_block_comment = false;
  std::size_t line_no = 0;
  std::vector<std::pair<std::size_t, std::string>> out{};
  while (std::getline(input, line)) {
    ++line_no;
    std::string stripped = strip_comment_line_(line, &in_block_comment);
    stripped = trim_ascii_copy(std::move(stripped));
    if (!stripped.empty()) out.emplace_back(line_no, std::move(stripped));
  }
  return out;
}

[[noreturn]] void fail_line_(std::size_t line, const std::string& reason) {
  throw std::runtime_error(
      "network_design decode error at line " + std::to_string(line) + ": " +
      reason);
}

[[nodiscard]] bool is_identifier_char_(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' ||
         c == '-' || c == ':';
}

[[nodiscard]] bool is_valid_identifier_(std::string_view s) {
  if (s.empty()) return false;
  for (const char c : s) {
    if (!is_identifier_char_(c)) return false;
  }
  return true;
}

[[nodiscard]] bool is_valid_declared_type_(std::string_view s) {
  if (s.empty()) return false;
  int bracket_depth = 0;
  bool has_token_char = false;
  for (const char c : s) {
    if (is_identifier_char_(c)) {
      has_token_char = true;
      continue;
    }
    if (c == '[') {
      ++bracket_depth;
      continue;
    }
    if (c == ']') {
      --bracket_depth;
      if (bracket_depth < 0) return false;
      continue;
    }
    if (c == ',') {
      continue;
    }
    return false;
  }
  return has_token_char && bracket_depth == 0;
}

[[nodiscard]] bool starts_with_kw_(std::string_view line,
                                   std::string_view kw) {
  if (line.size() < kw.size()) return false;
  if (line.substr(0, kw.size()) != kw) return false;
  if (line.size() == kw.size()) return true;
  const char n = line[kw.size()];
  return std::isspace(static_cast<unsigned char>(n)) || n == '"' || n == '{';
}

struct parser_t {
  explicit parser_t(std::string instruction)
      : lines_(normalized_lines_with_numbers_(instruction)) {}

  cuwacunu::camahjucunu::network_design_instruction_t parse() {
    using cuwacunu::camahjucunu::network_design_instruction_t;
    using cuwacunu::camahjucunu::network_design_node_t;
    using cuwacunu::camahjucunu::network_design_param_t;
    using cuwacunu::camahjucunu::network_design_export_t;

    enum class State { ExpectNetworkStart, InNetwork, InNode, Ended };
    State state = State::ExpectNetworkStart;

    network_design_instruction_t out{};
    network_design_node_t active_node{};
    bool has_active_node = false;
    std::unordered_set<std::string> node_ids{};
    std::unordered_set<std::string> export_names{};

    for (const auto& [line_no, line_raw] : lines_) {
      const std::string line = trim_ascii_copy(line_raw);

      if (state == State::ExpectNetworkStart) {
        if (!starts_with_kw_(line, "NETWORK")) {
          fail_line_(line_no, "expected `NETWORK \"<id>\" {`");
        }
        const std::string rest = trim_ascii_copy(line.substr(7));
        const std::size_t q0 = rest.find('"');
        if (q0 == std::string::npos) fail_line_(line_no, "missing network id quote");
        const std::size_t q1 = rest.find('"', q0 + 1);
        if (q1 == std::string::npos) fail_line_(line_no, "unterminated network id quote");
        out.network_id = rest.substr(q0 + 1, q1 - (q0 + 1));
        if (!is_valid_identifier_(out.network_id)) {
          fail_line_(line_no, "invalid network id `" + out.network_id + "`");
        }
        const std::string tail = trim_ascii_copy(rest.substr(q1 + 1));
        if (tail != "{") fail_line_(line_no, "expected `{` after network id");
        state = State::InNetwork;
        continue;
      }

      if (state == State::InNetwork) {
        if (line == "}") {
          state = State::Ended;
          continue;
        }

        if (starts_with_kw_(line, "ASSEMBLY_TAG")) {
          const std::size_t eq = line.find('=');
          if (eq == std::string::npos) {
            fail_line_(line_no, "ASSEMBLY_TAG requires `=`");
          }
          std::string value = trim_ascii_copy(line.substr(eq + 1));
          if (!value.empty() && value.back() == ';') value.pop_back();
          value = trim_ascii_copy(std::move(value));
          if (!is_valid_identifier_(value)) {
            fail_line_(line_no, "invalid ASSEMBLY_TAG token `" + value + "`");
          }
          out.assembly_tag = value;
          continue;
        }

        if (starts_with_kw_(line, "node")) {
          const std::string rest = trim_ascii_copy(line.substr(4));
          const std::size_t at = rest.find('@');
          if (at == std::string::npos) {
            fail_line_(line_no, "node declaration requires `<id>@<kind>`");
          }
          std::string id = trim_ascii_copy(rest.substr(0, at));
          std::string kind_and_tail = trim_ascii_copy(rest.substr(at + 1));
          const std::size_t brace = kind_and_tail.find('{');
          if (brace == std::string::npos) {
            fail_line_(line_no, "node declaration requires trailing `{`");
          }
          std::string kind = trim_ascii_copy(kind_and_tail.substr(0, brace));
          std::string tail = trim_ascii_copy(kind_and_tail.substr(brace + 1));
          if (!tail.empty()) {
            fail_line_(line_no, "unexpected tokens after `{` in node declaration");
          }
          if (!is_valid_identifier_(id)) {
            fail_line_(line_no, "invalid node id `" + id + "`");
          }
          if (!is_valid_identifier_(kind)) {
            fail_line_(line_no, "invalid node kind `" + kind + "`");
          }
          if (!node_ids.insert(id).second) {
            fail_line_(line_no, "duplicate node id `" + id + "`");
          }
          active_node = network_design_node_t{};
          active_node.id = std::move(id);
          active_node.kind = std::move(kind);
          has_active_node = true;
          state = State::InNode;
          continue;
        }

        if (starts_with_kw_(line, "export")) {
          const std::string rest = trim_ascii_copy(line.substr(6));
          const std::size_t eq = rest.find('=');
          if (eq == std::string::npos) {
            fail_line_(line_no, "export declaration requires `=`");
          }
          std::string export_name = trim_ascii_copy(rest.substr(0, eq));
          std::string node_id = trim_ascii_copy(rest.substr(eq + 1));
          if (!node_id.empty() && node_id.back() == ';') node_id.pop_back();
          node_id = trim_ascii_copy(std::move(node_id));
          if (!is_valid_identifier_(export_name)) {
            fail_line_(line_no, "invalid export name `" + export_name + "`");
          }
          if (!is_valid_identifier_(node_id)) {
            fail_line_(line_no, "invalid export node id `" + node_id + "`");
          }
          if (!export_names.insert(export_name).second) {
            fail_line_(line_no, "duplicate export `" + export_name + "`");
          }
          out.exports.push_back(network_design_export_t{
              .name = std::move(export_name),
              .node_id = std::move(node_id),
          });
          continue;
        }

        fail_line_(line_no, "unknown network-level statement `" + line + "`");
      }

      if (state == State::InNode) {
        if (line == "}") {
          if (!has_active_node) fail_line_(line_no, "internal parser state error");
          out.nodes.push_back(std::move(active_node));
          active_node = network_design_node_t{};
          has_active_node = false;
          state = State::InNetwork;
          continue;
        }

        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
          fail_line_(line_no, "node parameter requires `=`");
        }
        std::string lhs = trim_ascii_copy(line.substr(0, eq));
        std::string rhs = trim_ascii_copy(line.substr(eq + 1));
        if (!rhs.empty() && rhs.back() == ';') rhs.pop_back();
        rhs = trim_ascii_copy(std::move(rhs));
        if (rhs.empty()) {
          fail_line_(line_no, "node parameter value is empty");
        }

        std::string key{};
        std::string declared_type{};
        const std::size_t colon = lhs.find(':');
        if (colon == std::string::npos) {
          key = trim_ascii_copy(std::move(lhs));
        } else {
          key = trim_ascii_copy(lhs.substr(0, colon));
          declared_type = trim_ascii_copy(lhs.substr(colon + 1));
        }
        if (!is_valid_identifier_(key)) {
          fail_line_(line_no, "invalid parameter key `" + key + "`");
        }
        if (!declared_type.empty() && !is_valid_declared_type_(declared_type)) {
          fail_line_(line_no, "invalid parameter type `" + declared_type + "`");
        }

        active_node.params.push_back(network_design_param_t{
            .key = std::move(key),
            .declared_type = std::move(declared_type),
            .value = std::move(rhs),
            .line = line_no,
        });
        continue;
      }

      if (state == State::Ended) {
        fail_line_(line_no, "unexpected content after NETWORK block end");
      }
    }

    if (state == State::ExpectNetworkStart) {
      throw std::runtime_error("network_design decode error: empty instruction");
    }
    if (state == State::InNode) {
      throw std::runtime_error("network_design decode error: unterminated node block");
    }
    if (state == State::InNetwork) {
      throw std::runtime_error("network_design decode error: missing closing `}` for NETWORK block");
    }

    for (const auto& ex : out.exports) {
      if (node_ids.find(ex.node_id) == node_ids.end()) {
        throw std::runtime_error(
            "network_design decode error: export `" + ex.name +
            "` references unknown node `" + ex.node_id + "`");
      }
    }

    return out;
  }

  const std::vector<std::pair<std::size_t, std::string>> lines_;
};

}  // namespace

namespace cuwacunu {
namespace camahjucunu {

std::map<std::string, std::string> network_design_node_t::to_param_map() const {
  std::map<std::string, std::string> out;
  for (const auto& p : params) {
    if (!p.key.empty()) out[p.key] = p.value;
  }
  return out;
}

const network_design_node_t* network_design_instruction_t::find_node_by_id(
    const std::string& id) const {
  const std::string wanted = trim_ascii_copy(id);
  if (wanted.empty()) return nullptr;
  for (const auto& node : nodes) {
    if (trim_ascii_copy(node.id) == wanted) return &node;
  }
  return nullptr;
}

std::vector<const network_design_node_t*>
network_design_instruction_t::find_nodes_by_kind(const std::string& kind) const {
  const std::string wanted = lower_ascii_copy(trim_ascii_copy(kind));
  std::vector<const network_design_node_t*> out;
  if (wanted.empty()) return out;
  for (const auto& node : nodes) {
    if (lower_ascii_copy(trim_ascii_copy(node.kind)) == wanted) out.push_back(&node);
  }
  return out;
}

std::string network_design_instruction_t::str() const {
  std::ostringstream oss;
  oss << "network_design: network_id=" << network_id
      << " assembly_tag=" << assembly_tag
      << " nodes=" << nodes.size()
      << " exports=" << exports.size() << "\n";
  for (const auto& node : nodes) {
    oss << "  node " << node.id << "@" << node.kind
        << " params=" << node.params.size() << "\n";
  }
  for (const auto& ex : exports) {
    oss << "  export " << ex.name << " = " << ex.node_id << "\n";
  }
  return oss.str();
}

namespace dsl {

networkDesignPipeline::networkDesignPipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {}

network_design_instruction_t networkDesignPipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  if (!has_non_ws_ascii_(grammar_text_)) {
    throw std::runtime_error("network_design grammar text is empty");
  }
  parser_t parser(std::move(instruction));
  return parser.parse();
}

network_design_instruction_t decode_network_design_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  networkDesignPipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
