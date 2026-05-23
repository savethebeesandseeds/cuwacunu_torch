#include "hero/mcp_schema_compat.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace schema = cuwacunu::hero::mcp_schema_compat;
namespace fs = std::filesystem;

namespace {

struct tool_schema_t {
  std::string name{};
  std::string input_schema{};
};

[[nodiscard]] std::string read_command_stdout(const std::string &command) {
  std::array<char, 4096> buffer{};
  std::string out;
  FILE *pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen failed for command: " + command);
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    out.append(buffer.data());
  }
  const int status = pclose(pipe);
  if (status != 0) {
    throw std::runtime_error("command failed: " + command + "\n" + out);
  }
  return out;
}

[[nodiscard]] fs::path first_existing(std::vector<fs::path> candidates) {
  for (const auto &candidate : candidates) {
    if (fs::exists(candidate)) {
      return candidate;
    }
  }
  std::ostringstream msg;
  msg << "missing expected Hero binary:";
  for (const auto &candidate : candidates) {
    msg << " " << candidate.string();
  }
  throw std::runtime_error(msg.str());
}

void skip_ws(const std::string &s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] std::string parse_json_string_at(const std::string &s,
                                               std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size() || s[*idx] != '"') {
    throw std::runtime_error("expected JSON string");
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[*idx];
    ++(*idx);
    if (c == '"') {
      return value;
    }
    if (c == '\\') {
      if (*idx >= s.size()) {
        throw std::runtime_error("unterminated JSON escape");
      }
      value.push_back(s[*idx]);
      ++(*idx);
      continue;
    }
    value.push_back(c);
  }
  throw std::runtime_error("unterminated JSON string");
}

[[nodiscard]] std::string capture_json_object_at(const std::string &s,
                                                 std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size() || s[*idx] != '{') {
    throw std::runtime_error("expected JSON object");
  }
  const std::size_t begin = *idx;
  int depth = 0;
  while (*idx < s.size()) {
    if (s[*idx] == '"') {
      (void)parse_json_string_at(s, idx);
      continue;
    }
    if (s[*idx] == '{') {
      ++depth;
    } else if (s[*idx] == '}') {
      --depth;
      if (depth == 0) {
        ++(*idx);
        return s.substr(begin, *idx - begin);
      }
    }
    ++(*idx);
  }
  throw std::runtime_error("unterminated JSON object");
}

[[nodiscard]] std::vector<tool_schema_t>
extract_tool_schemas(const std::string &catalog_json) {
  std::vector<tool_schema_t> out;
  std::size_t pos = 0;
  while ((pos = catalog_json.find("\"name\"", pos)) != std::string::npos) {
    pos = catalog_json.find(':', pos);
    if (pos == std::string::npos) {
      throw std::runtime_error("tool name missing ':'");
    }
    ++pos;
    tool_schema_t row{};
    row.name = parse_json_string_at(catalog_json, &pos);
    const std::size_t schema_key = catalog_json.find("\"inputSchema\"", pos);
    if (schema_key == std::string::npos) {
      throw std::runtime_error("tool inputSchema missing for " + row.name);
    }
    pos = catalog_json.find(':', schema_key);
    if (pos == std::string::npos) {
      throw std::runtime_error("tool inputSchema missing ':' for " + row.name);
    }
    ++pos;
    row.input_schema = capture_json_object_at(catalog_json, &pos);
    out.push_back(std::move(row));
  }
  return out;
}

void check_catalog(const std::string &label, const fs::path &binary,
                   bool require_lattice_checkpoint_closure) {
  const std::string output = read_command_stdout(
      binary.string() + " --global-config /cuwacunu/src/config/.config "
                        "--list-tools-json");
  const auto tools = extract_tool_schemas(output);
  assert(!tools.empty());
  bool saw_lattice_checkpoint_closure = false;
  for (const auto &tool : tools) {
    const auto issues =
        schema::validate_tool_input_schema(tool.name, tool.input_schema);
    if (!issues.empty()) {
      throw std::runtime_error(label + " schema compatibility failure: " +
                               schema::format_issue(issues.front()));
    }
    if (tool.name == "hero.lattice.checkpoint_closure") {
      saw_lattice_checkpoint_closure = true;
    }
  }
  if (require_lattice_checkpoint_closure && !saw_lattice_checkpoint_closure) {
    throw std::runtime_error(
        "missing regression tool hero.lattice.checkpoint_closure");
  }
}

} // namespace

int main() {
  const auto one_of_issues = schema::validate_tool_input_schema(
      "hero_lattice_checkpoint_closure",
      R"({"type":"object","oneOf":[],"properties":{}})");
  assert(!one_of_issues.empty());
  assert(one_of_issues.front().tool_name == "hero_lattice_checkpoint_closure");
  assert(one_of_issues.front().schema_path == "inputSchema.oneOf");
  assert(one_of_issues.front().construct == "oneOf");
  assert(schema::format_issue(one_of_issues.front())
             .find("hero_lattice_checkpoint_closure") != std::string::npos);

  const auto enum_issues = schema::validate_tool_input_schema(
      "hero.bad.enum", R"({"type":"object","enum":["x"]})");
  assert(!enum_issues.empty());
  assert(enum_issues.front().schema_path == "inputSchema.enum");
  assert(enum_issues.front().construct == "enum");

  const auto type_issues = schema::validate_tool_input_schema(
      "hero.bad.type", R"({"type":"string","properties":{}})");
  assert(!type_issues.empty());
  assert(type_issues.front().schema_path == "inputSchema.type");
  assert(type_issues.front().construct == "type");

  const auto ok_issues = schema::validate_tool_input_schema(
      "hero.good", R"({"type":"object","properties":{},"required":[]})");
  assert(ok_issues.empty());

  const fs::path hero_root = "/cuwacunu/.build/hero";
  check_catalog("Config",
                first_existing({hero_root / "hero_config.mcp",
                                hero_root / "hero_config_mcp"}),
                false);
  check_catalog("Runtime",
                first_existing({hero_root / "hero_runtime.mcp",
                                hero_root / "hero_runtime_mcp"}),
                false);
  check_catalog("Hashimyei",
                first_existing({hero_root / "hero_hashimyei.mcp",
                                hero_root / "hero_hashimyei_mcp"}),
                false);
  check_catalog("Lattice",
                first_existing({hero_root / "hero_lattice.mcp",
                                hero_root / "hero_lattice_mcp"}),
                true);

  return 0;
}
