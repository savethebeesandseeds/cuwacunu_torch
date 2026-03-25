#include "camahjucunu/dsl/super_objective/super_objective.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace {

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string strip_inline_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    const char c = in[i];
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double) {
      if (c == '#') break;
      if (c == '/' && i + 1 < in.size() && in[i + 1] == '/') break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] std::string strip_block_comments_from_line(std::string_view line,
                                                         bool* in_block_comment) {
  std::string out;
  out.reserve(line.size());
  for (std::size_t i = 0; i < line.size();) {
    if (*in_block_comment) {
      const std::size_t end = line.find("*/", i);
      if (end == std::string_view::npos) return out;
      *in_block_comment = false;
      i = end + 2;
      continue;
    }
    if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '*') {
      *in_block_comment = true;
      i += 2;
      continue;
    }
    out.push_back(line[i]);
    ++i;
  }
  return out;
}

[[nodiscard]] std::string unquote_if_wrapped(std::string value) {
  value = trim_ascii(value);
  if (value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') ||
       (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

struct parsed_lhs_t {
  std::string key{};
  std::string declared_type{};
};

[[nodiscard]] parsed_lhs_t parse_lhs(std::string_view lhs, std::size_t line) {
  const std::string text = trim_ascii(lhs);
  const std::size_t colon = text.find(':');
  if (colon == std::string::npos) {
    throw std::runtime_error("line " + std::to_string(line) +
                             ": expected <key>:<type> on left-hand side");
  }
  parsed_lhs_t out{};
  out.key = trim_ascii(text.substr(0, colon));
  out.declared_type = trim_ascii(text.substr(colon + 1));
  if (out.key.empty() || out.declared_type.empty()) {
    throw std::runtime_error("line " + std::to_string(line) +
                             ": malformed super objective field declaration");
  }
  return out;
}

void assign_field(cuwacunu::camahjucunu::super_objective_instruction_t* out,
                  const parsed_lhs_t& lhs, std::string value, std::size_t line) {
  if (!out) return;
  value = unquote_if_wrapped(std::move(value));
  if (lhs.key == "campaign_dsl_path") {
    if (lhs.declared_type != "path") {
      throw std::runtime_error("line " + std::to_string(line) +
                               ": campaign_dsl_path must declare type path");
    }
    out->campaign_dsl_path = std::move(value);
    return;
  }
  if (lhs.key == "objective_prompt_path") {
    if (lhs.declared_type != "path") {
      throw std::runtime_error("line " + std::to_string(line) +
                               ": objective_prompt_path must declare type path");
    }
    out->objective_prompt_path = std::move(value);
    return;
  }
  if (lhs.key == "objective_name") {
    if (lhs.declared_type != "str") {
      throw std::runtime_error("line " + std::to_string(line) +
                               ": objective_name must declare type str");
    }
    out->objective_name = std::move(value);
    return;
  }
  if (lhs.key == "loop_id") {
    if (lhs.declared_type != "str") {
      throw std::runtime_error("line " + std::to_string(line) +
                               ": loop_id must declare type str");
    }
    out->loop_id = std::move(value);
    return;
  }
  throw std::runtime_error("line " + std::to_string(line) +
                           ": unknown super objective field: " + lhs.key);
}

}  // namespace

namespace cuwacunu {
namespace camahjucunu {

std::string super_objective_instruction_t::str() const {
  std::ostringstream out;
  out << "campaign_dsl_path:path = " << campaign_dsl_path << "\n"
      << "objective_prompt_path:path = " << objective_prompt_path << "\n";
  if (!objective_name.empty()) {
    out << "objective_name:str = " << objective_name << "\n";
  }
  if (!loop_id.empty()) {
    out << "loop_id:str = " << loop_id << "\n";
  }
  return out.str();
}

namespace dsl {

superObjectivePipeline::superObjectivePipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {}

super_objective_instruction_t superObjectivePipeline::decode(
    std::string instruction) {
  (void)grammar_text_;
  std::lock_guard<std::mutex> guard(current_mutex_);

  super_objective_instruction_t out{};
  std::unordered_set<std::string> seen{};
  std::istringstream in(instruction);
  std::string raw_line{};
  bool in_block_comment = false;
  std::size_t line_no = 0;
  while (std::getline(in, raw_line)) {
    ++line_no;
    std::string line = strip_block_comments_from_line(raw_line, &in_block_comment);
    line = trim_ascii(strip_inline_comment(line));
    if (line.empty()) continue;
    if (!line.empty() && line.back() == ';') {
      line.pop_back();
      line = trim_ascii(line);
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("line " + std::to_string(line_no) +
                               ": expected '=' in super objective field");
    }
    const parsed_lhs_t lhs = parse_lhs(line.substr(0, eq), line_no);
    if (!seen.insert(lhs.key).second) {
      throw std::runtime_error("line " + std::to_string(line_no) +
                               ": duplicate super objective field: " + lhs.key);
    }
    std::string rhs = trim_ascii(line.substr(eq + 1));
    if (rhs.empty()) {
      throw std::runtime_error("line " + std::to_string(line_no) +
                               ": super objective field value may not be empty");
    }
    assign_field(&out, lhs, std::move(rhs), line_no);
  }
  if (in_block_comment) {
    throw std::runtime_error("unterminated block comment in super objective DSL");
  }
  if (trim_ascii(out.campaign_dsl_path).empty()) {
    throw std::runtime_error("super objective DSL missing campaign_dsl_path:path");
  }
  if (trim_ascii(out.objective_prompt_path).empty()) {
    throw std::runtime_error(
        "super objective DSL missing objective_prompt_path:path");
  }
  return out;
}

super_objective_instruction_t decode_super_objective_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  superObjectivePipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
