#include "hero/runtime_hero/hero_runtime_tools.h"

#include "hero/mcp_schema_compat.h"
#include "hero/runtime_hero/hero_runtime.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cuwacunu::hero::runtime {
namespace {

namespace fs = std::filesystem;

constexpr int kPolicyErrorCode = -32050;
constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultRuntimePolicyPath =
    "/cuwacunu/src/config/hero.runtime.dsl";

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

constexpr tool_descriptor_t kTools[] = {
    {"hero.runtime.status",
     "Summarize Runtime Hero policy, executable, active wave, and job root.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.runtime.schema", "List Runtime Hero policy keys and constraints.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.runtime.wave",
     "Decode active wave intent from the configured runtime .config.",
     R"({"type":"object","properties":{"config_path":{"type":"string"}},"additionalProperties":false})"},
    {"hero.runtime.dry_run",
     "Run cuwacunu_exec with --dry-run and return stdout plus job artifacts.",
     R"({"type":"object","properties":{"config_path":{"type":"string"},"job_dir":{"type":"string"},"force_rebuild_cache":{"type":"boolean"},"timeout_seconds":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.runtime.execute",
     "Run cuwacunu_exec with policy guards; dry_run defaults to true.",
     R"({"type":"object","properties":{"config_path":{"type":"string"},"job_dir":{"type":"string"},"dry_run":{"type":"boolean"},"force_rebuild_cache":{"type":"boolean"},"confirm_execute":{"type":"boolean"},"timeout_seconds":{"type":"integer"},"marshal_expected_wave":{"type":"object","properties":{"target_component_family_id":{"type":"string"},"mode":{"type":"string"},"wave_target":{"type":"string"},"wave_mode":{"type":"string"},"source_range":{"type":"string"},"source_order":{"type":"string"},"anchor_index_begin":{"type":"string"},"anchor_index_end":{"type":"string"},"source_key_begin":{"type":"string"},"source_key_end":{"type":"string"},"model_state_inputs":{"type":"object"}}}},"additionalProperties":false})"},
    {"hero.runtime.dev_nuke",
     "Developer reset for runtime-root contents with dry-run, idle checks, and "
     "optional backup snapshot.",
     R"({"type":"object","properties":{"runtime_root":{"type":"string"},"dry_run":{"type":"boolean"},"backup":{"type":"boolean"},"confirm_dev_nuke":{"type":"boolean"}},"additionalProperties":false})"},
    {"hero.runtime.list_jobs", "List Runtime Hero job directories.",
     R"({"type":"object","properties":{"root":{"type":"string"},"limit":{"type":"integer"},"include_artifacts":{"type":"boolean"}},"additionalProperties":false})"},
    {"hero.runtime.get_job",
     "Inspect one runtime job directory by job_id or job_dir.",
     R"({"type":"object","properties":{"job_id":{"type":"string"},"job_dir":{"type":"string"},"include_text":{"type":"boolean"},"max_bytes":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.runtime.read_artifact",
     "Read a bounded job artifact: manifest, state, report, or explicit path.",
     R"({"type":"object","properties":{"job_id":{"type":"string"},"job_dir":{"type":"string"},"artifact":{"type":"string"},"path":{"type":"string"},"max_bytes":{"type":"integer"}},"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.runtime.status" || name == "hero.runtime.schema" ||
         name == "hero.runtime.wave" || name == "hero.runtime.list_jobs" ||
         name == "hero.runtime.get_job" || name == "hero.runtime.read_artifact";
}

[[nodiscard]] bool tool_is_destructive(std::string_view name) {
  return name == "hero.runtime.execute" || name == "hero.runtime.dev_nuke";
}

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

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] bool parse_bool(std::string_view raw, bool *out) {
  const std::string value = lowercase_ascii(trim_ascii(raw));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int(std::string_view raw, int *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view raw) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream in{std::string(raw)};
  while (std::getline(in, item, ',')) {
    item = trim_ascii(item);
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

[[nodiscard]] std::string json_quote(std::string_view s) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : s) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (c < 0x20) {
        out << "\\u00";
        static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                                   '6', '7', '8', '9', 'a', 'b',
                                                   'c', 'd', 'e', 'f'};
        out << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] const char *bool_json(bool value) {
  return value ? "true" : "false";
}

void skip_ws(const std::string &s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] bool parse_json_string_token(const std::string &s,
                                           std::size_t *idx, std::string *out) {
  if (*idx >= s.size() || s[*idx] != '"') {
    return false;
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[(*idx)++];
    if (c == '"') {
      if (out) {
        *out = std::move(value);
      }
      return true;
    }
    if (c != '\\') {
      if (static_cast<unsigned char>(c) < 0x20) {
        return false;
      }
      value.push_back(c);
      continue;
    }
    if (*idx >= s.size()) {
      return false;
    }
    const char escaped = s[(*idx)++];
    switch (escaped) {
    case '"':
    case '\\':
    case '/':
      value.push_back(escaped);
      break;
    case 'b':
      value.push_back('\b');
      break;
    case 'f':
      value.push_back('\f');
      break;
    case 'n':
      value.push_back('\n');
      break;
    case 'r':
      value.push_back('\r');
      break;
    case 't':
      value.push_back('\t');
      break;
    case 'u':
      if (*idx + 4 > s.size()) {
        return false;
      }
      value.append("\\u");
      value.append(s.substr(*idx, 4));
      *idx += 4;
      break;
    default:
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool skip_json_value(const std::string &s, std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size()) {
    return false;
  }
  if (s[*idx] == '"') {
    return parse_json_string_token(s, idx, nullptr);
  }
  if (s[*idx] == '{' || s[*idx] == '[') {
    std::vector<char> stack;
    stack.push_back(s[*idx]);
    ++(*idx);
    bool in_string = false;
    bool escape = false;
    while (*idx < s.size()) {
      const char c = s[(*idx)++];
      if (in_string) {
        if (escape) {
          escape = false;
        } else if (c == '\\') {
          escape = true;
        } else if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == '"') {
        in_string = true;
      } else if (c == '{' || c == '[') {
        stack.push_back(c);
      } else if (c == '}') {
        if (stack.empty() || stack.back() != '{') {
          return false;
        }
        stack.pop_back();
      } else if (c == ']') {
        if (stack.empty() || stack.back() != '[') {
          return false;
        }
        stack.pop_back();
      }
      if (stack.empty()) {
        return true;
      }
    }
    return false;
  }
  const std::size_t begin = *idx;
  while (*idx < s.size()) {
    const char c = s[*idx];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++(*idx);
  }
  return *idx > begin;
}

[[nodiscard]] bool extract_json_raw_field(const std::string &json,
                                          std::string_view key,
                                          std::string *out) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    return false;
  }
  ++idx;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
    std::string current_key;
    if (!parse_json_string_token(json, &idx, &current_key)) {
      return false;
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      return false;
    }
    if (current_key == key) {
      if (out) {
        *out = trim_ascii(
            std::string_view(json).substr(value_begin, idx - value_begin));
      }
      return true;
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  std::string value;
  if (!parse_json_string_token(raw, &idx, &value)) {
    return false;
  }
  skip_ws(raw, &idx);
  if (idx != raw.size()) {
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

[[nodiscard]] bool
extract_json_first_string_field(const std::string &json,
                                std::initializer_list<std::string_view> keys,
                                std::string *out) {
  for (const auto key : keys) {
    if (extract_json_string_field(json, key, out)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool
extract_json_string_object(const std::string &json,
                           std::unordered_map<std::string, std::string> *out) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    return false;
  }
  ++idx;
  std::unordered_map<std::string, std::string> values;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        return false;
      }
      if (out) {
        *out = std::move(values);
      }
      return true;
    }
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      return false;
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    std::string value;
    if (!parse_json_string_token(json, &idx, &value)) {
      return false;
    }
    values[std::move(key)] = std::move(value);
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      continue;
    }
    return false;
  }
  return false;
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           std::string_view key, bool *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  const std::string value = trim_ascii(raw);
  if (value == "true") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_int_field(const std::string &json,
                                          std::string_view key, int *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  return parse_int(raw, out);
}

[[nodiscard]] fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  const fs::path canonical = fs::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : canonical;
}

[[nodiscard]] fs::path resolve_against(const fs::path &base_file,
                                       std::string_view raw) {
  fs::path path(trim_ascii(raw));
  if (path.empty()) {
    return path;
  }
  if (path.is_absolute()) {
    return normalize_path(path);
  }
  return normalize_path(base_file.parent_path() / path);
}

[[nodiscard]] bool path_within(const fs::path &root, const fs::path &path) {
  const fs::path norm_root = normalize_path(root);
  const fs::path norm_path = normalize_path(path);
  auto root_it = norm_root.begin();
  auto path_it = norm_path.begin();
  for (; root_it != norm_root.end(); ++root_it, ++path_it) {
    if (path_it == norm_path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool under_any_root(const fs::path &path,
                                  const std::vector<std::string> &roots,
                                  const fs::path &base_file) {
  for (const std::string &raw : roots) {
    const fs::path root = resolve_against(base_file, raw);
    if (path_within(root, path)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::string strip_ini_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
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
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] bool read_text_file(const fs::path &path, std::string *out,
                                  std::string *err) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err) {
      *err = "cannot open file: " + path.string();
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (in.bad()) {
    if (err) {
      *err = "cannot read file: " + path.string();
    }
    return false;
  }
  if (out) {
    *out = buffer.str();
  }
  return true;
}

[[nodiscard]] std::string read_text_file_limited(const fs::path &path,
                                                 std::size_t max_bytes,
                                                 bool *truncated) {
  if (truncated) {
    *truncated = false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::string out;
  out.resize(max_bytes);
  in.read(out.data(), static_cast<std::streamsize>(max_bytes));
  out.resize(static_cast<std::size_t>(in.gcount()));
  if (in.peek() != EOF && truncated) {
    *truncated = true;
  }
  return out;
}

[[nodiscard]] std::optional<std::string>
read_ini_value(const fs::path &ini_path, std::string_view section,
               std::string_view key) {
  std::ifstream in(ini_path);
  if (!in) {
    return std::nullopt;
  }
  std::string current_section;
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) {
      continue;
    }
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs == key) {
      return trim_ascii(trimmed.substr(eq + 1));
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string strip_dsl_comments(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_block = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (!in_block && i + 1 < in.size() && in[i] == '/' && in[i + 1] == '*') {
      in_block = true;
      ++i;
      continue;
    }
    if (in_block && i + 1 < in.size() && in[i] == '*' && in[i + 1] == '/') {
      in_block = false;
      ++i;
      continue;
    }
    if (in_block) {
      continue;
    }
    out.push_back(in[i]);
  }
  return out;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_assignment_text(std::string_view text, bool semicolon_terminated) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines{std::string(text)};
  std::string line;
  while (std::getline(lines, line)) {
    line = trim_ascii(strip_ini_comment(line));
    if (line.empty() || line == "};" || line == "}" ||
        line.find('{') != std::string::npos ||
        (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = trim_ascii(line.substr(0, eq));
    const std::size_t domain = key.find_first_of("[:");
    if (domain != std::string::npos) {
      key = trim_ascii(key.substr(0, domain));
    }
    std::string value = trim_ascii(line.substr(eq + 1));
    if (semicolon_terminated && !value.empty() && value.back() == ';') {
      value.pop_back();
      value = trim_ascii(value);
    }
    if (!key.empty()) {
      out[std::move(key)] = std::move(value);
    }
  }
  return out;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_kv_file(const fs::path &path) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return parse_assignment_text(text, false);
}

[[nodiscard]] std::string
kv_map_to_json(const std::unordered_map<std::string, std::string> &map) {
  std::vector<std::string> keys;
  keys.reserve(map.size());
  for (const auto &entry : map) {
    keys.push_back(entry.first);
  }
  std::sort(keys.begin(), keys.end());
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(keys[i]) << ":" << json_quote(map.at(keys[i]));
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string policy_get(const runtime_policy_t &policy,
                                     std::string_view key) {
  const auto found = policy.values.find(std::string(key));
  if (found != policy.values.end()) {
    return found->second;
  }
  for (const auto &descriptor : kRuntimePolicyDescriptors) {
    if (descriptor.key == key) {
      return std::string(descriptor.default_value);
    }
  }
  return {};
}

[[nodiscard]] bool policy_bool_or(const runtime_policy_t &policy,
                                  std::string_view key, bool fallback) {
  bool parsed = fallback;
  (void)parse_bool(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] int policy_int_or(const runtime_policy_t &policy,
                                std::string_view key, int fallback) {
  int parsed = fallback;
  (void)parse_int(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] fs::path policy_path(const runtime_policy_t &policy,
                                   std::string_view key) {
  return resolve_against(policy.policy_path, policy_get(policy, key));
}

[[nodiscard]] bool executable_file(const fs::path &path) {
  return ::access(path.c_str(), X_OK) == 0;
}

[[nodiscard]] std::string make_tool_result(std::string_view tool_name,
                                           std::string_view structured_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string(tool_name) + " executed")
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":false}";
  return out.str();
}

[[nodiscard]] std::string make_error_result(std::string_view message,
                                            int code) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(message)
      << "}],\"structuredContent\":{\"ok\":false,\"error_code\":" << code
      << ",\"message\":" << json_quote(message) << "},\"isError\":true}";
  return out.str();
}

struct wave_info_t {
  fs::path config_path{};
  fs::path wave_path{};
  bool readable{false};
  std::string error{};
  std::unordered_map<std::string, std::string> values{};
};

[[nodiscard]] fs::path effective_config_path(const runtime_context_t &ctx,
                                             const std::string &arg) {
  if (!trim_ascii(arg).empty()) {
    return normalize_path(fs::path(arg));
  }
  if (!ctx.global_config_path.empty()) {
    return normalize_path(ctx.global_config_path);
  }
  return policy_path(ctx.policy, "default_config_path");
}

[[nodiscard]] wave_info_t load_wave_info(const runtime_context_t &ctx,
                                         const fs::path &config_path) {
  wave_info_t info{};
  info.config_path = config_path;
  const auto maybe_wave = read_ini_value(config_path, "KIKIJYEBA",
                                         "kikijyeba_settings_wave_dsl_path");
  if (!maybe_wave.has_value()) {
    info.error = "missing [KIKIJYEBA].kikijyeba_settings_wave_dsl_path in " +
                 config_path.string();
    return info;
  }
  info.wave_path = resolve_against(config_path, *maybe_wave);
  std::string text;
  if (!read_text_file(info.wave_path, &text, &info.error)) {
    return info;
  }
  info.readable = true;
  info.values = parse_assignment_text(strip_dsl_comments(text), true);
  return info;
}

[[nodiscard]] std::string wave_action_from_mode(std::string_view mode) {
  const std::string lower = lowercase_ascii(mode);
  bool saw_run = false;
  bool saw_train = false;
  std::string token;
  for (const char c : lower) {
    if (c == '|' || c == '+' || c == ',' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      if (token == "run") {
        saw_run = true;
      } else if (token == "train") {
        saw_train = true;
      }
      token.clear();
      continue;
    }
    token.push_back(c);
  }
  if (token == "run") {
    saw_run = true;
  } else if (token == "train") {
    saw_train = true;
  }
  if (saw_train && !saw_run) {
    return "train";
  }
  return "run";
}

[[nodiscard]] bool wave_debug_from_mode(std::string_view mode) {
  return lowercase_ascii(mode).find("debug") != std::string::npos;
}

[[nodiscard]] std::string canonical_source_range(std::string_view value) {
  const std::string lower = lowercase_ascii(trim_ascii(value));
  return lower == "anchor_key" ? "source_key" : lower;
}

[[nodiscard]] std::string job_kind_from_target(std::string_view target) {
  const std::string lower = lowercase_ascii(target);
  if (lower == "wikimyei.representation.encoding.vicreg" ||
      lower == "vicreg_representation") {
    return "channel_representation_vicreg";
  }
  if (lower == "wikimyei.inference.expected_value.mdn" ||
      lower == "inference_mdn" || lower == "inference_channel_mdn" ||
      lower == "mdn_expected_value_inference") {
    return "channel_inference_mdn";
  }
  return "invalid_wave_target";
}

[[nodiscard]] std::unordered_map<std::string, std::string>
wave_model_state_inputs(const wave_info_t &info) {
  std::unordered_map<std::string, std::string> inputs;
  for (const auto &[key, value] : info.values) {
    if (key.rfind("INPUT_", 0) != 0 || value.empty()) {
      continue;
    }
    inputs["PLAN_" + key] = fs::path(value).lexically_normal().string();
  }
  return inputs;
}

[[nodiscard]] bool wave_source_order_is_explicit(const wave_info_t &info) {
  const auto it = info.values.find("SOURCE_ORDER");
  return it != info.values.end() && !trim_ascii(it->second).empty();
}

[[nodiscard]] std::string wave_source_order_from_info(const wave_info_t &info,
                                                      std::string_view action) {
  if (wave_source_order_is_explicit(info)) {
    return lowercase_ascii(trim_ascii(info.values.at("SOURCE_ORDER")));
  }
  return action == "train" ? "random_per_epoch" : "sequential";
}

[[nodiscard]] std::string
wave_source_order_warning_token(std::string_view action,
                                std::string_view source_order,
                                bool source_order_explicit) {
  if (action != "train" ||
      lowercase_ascii(trim_ascii(source_order)) != "sequential") {
    return {};
  }
  return source_order_explicit ? "train_wave_explicit_sequential_source_order"
                               : "train_wave_effective_sequential_source_order";
}

[[nodiscard]] std::string execution_chain(std::string_view target,
                                          std::string_view action) {
  const std::string job_kind = job_kind_from_target(target);
  if (job_kind == "channel_representation_vicreg") {
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.vicreg:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.vicreg:run";
  }
  if (job_kind == "channel_inference_mdn") {
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.vicreg:run_frozen -> "
             "wikimyei.inference.expected_value.mdn:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.vicreg:run_frozen -> "
           "wikimyei.inference.expected_value.mdn:run";
  }
  return {};
}

[[nodiscard]] std::string wave_info_json(const wave_info_t &info) {
  const std::string wave_id = info.values.count("WAVE_ID") != 0
                                  ? info.values.at("WAVE_ID")
                                  : std::string{};
  const std::string target = info.values.count("TARGET") != 0
                                 ? info.values.at("TARGET")
                                 : std::string{};
  const std::string mode = info.values.count("MODE") != 0
                               ? info.values.at("MODE")
                               : std::string{"run"};
  const std::string action = wave_action_from_mode(mode);
  const std::string source_range = canonical_source_range(
      info.values.count("SOURCE_RANGE") != 0 ? info.values.at("SOURCE_RANGE")
                                             : std::string{"all"});
  const bool source_order_explicit = wave_source_order_is_explicit(info);
  const std::string source_order = wave_source_order_from_info(info, action);
  const std::string source_order_warning = wave_source_order_warning_token(
      action, source_order, source_order_explicit);
  const std::string cursor_kind = info.values.count("SOURCE_CURSOR_KIND") != 0
                                      ? info.values.at("SOURCE_CURSOR_KIND")
                                      : std::string{"graph_anchor"};
  const std::string cursor_scope = info.values.count("SOURCE_CURSOR_SCOPE") != 0
                                       ? info.values.at("SOURCE_CURSOR_SCOPE")
                                       : std::string{"wave_batch"};
  std::ostringstream out;
  out << "{\"config_path\":" << json_quote(info.config_path.string())
      << ",\"wave_path\":" << json_quote(info.wave_path.string())
      << ",\"readable\":" << bool_json(info.readable);
  if (!info.error.empty()) {
    out << ",\"error\":" << json_quote(info.error);
  }
  out << ",\"wave_id\":" << json_quote(wave_id)
      << ",\"target_component_family_id\":" << json_quote(target)
      << ",\"mode\":" << json_quote(mode)
      << ",\"action\":" << json_quote(action)
      << ",\"debug\":" << bool_json(wave_debug_from_mode(mode))
      << ",\"source_cursor_kind\":" << json_quote(cursor_kind)
      << ",\"source_cursor_scope\":" << json_quote(cursor_scope)
      << ",\"source_range\":" << json_quote(source_range)
      << ",\"source_order\":" << json_quote(source_order)
      << ",\"source_order_explicit\":" << bool_json(source_order_explicit)
      << ",\"source_order_warning_level\":"
      << json_quote(source_order_warning.empty() ? "none" : "warning")
      << ",\"source_order_warnings\":" << json_quote(source_order_warning)
      << ",\"anchor_index_begin\":";
  if (info.values.count("ANCHOR_INDEX_BEGIN") != 0) {
    out << json_quote(info.values.at("ANCHOR_INDEX_BEGIN"));
  } else {
    out << "null";
  }
  out << ",\"anchor_index_end\":";
  if (info.values.count("ANCHOR_INDEX_END") != 0) {
    out << json_quote(info.values.at("ANCHOR_INDEX_END"));
  } else {
    out << "null";
  }
  out << ",\"source_key_begin\":";
  if (info.values.count("SOURCE_KEY_BEGIN") != 0) {
    out << json_quote(info.values.at("SOURCE_KEY_BEGIN"));
  } else if (info.values.count("ANCHOR_KEY_BEGIN") != 0) {
    out << json_quote(info.values.at("ANCHOR_KEY_BEGIN"));
  } else {
    out << "null";
  }
  out << ",\"source_key_end\":";
  if (info.values.count("SOURCE_KEY_END") != 0) {
    out << json_quote(info.values.at("SOURCE_KEY_END"));
  } else if (info.values.count("ANCHOR_KEY_END") != 0) {
    out << json_quote(info.values.at("ANCHOR_KEY_END"));
  } else {
    out << "null";
  }
  out << ",\"job_kind\":" << json_quote(job_kind_from_target(target))
      << ",\"train_target\":" << bool_json(action == "train")
      << ",\"execution_chain\":" << json_quote(execution_chain(target, action))
      << ",\"model_state_inputs\":"
      << kv_map_to_json(wave_model_state_inputs(info)) << "}";
  return out.str();
}

struct process_result_t {
  int exit_code{-1};
  bool signaled{false};
  int signal_number{0};
  bool timed_out{false};
  bool stdout_truncated{false};
  bool stderr_truncated{false};
  std::string stdout_text{};
  std::string stderr_text{};
};

void append_capped(std::string *out, const char *data, std::size_t size,
                   std::size_t cap, bool *truncated) {
  if (!out) {
    return;
  }
  if (out->size() < cap) {
    const std::size_t room = cap - out->size();
    out->append(data, std::min(room, size));
  }
  if (size > 0 && out->size() >= cap && truncated) {
    *truncated = true;
  }
}

void set_nonblocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags >= 0) {
    (void)::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
}

void drain_fd(int fd, std::string *out, std::size_t cap, bool *truncated,
              bool *open) {
  std::array<char, 4096> buffer{};
  while (true) {
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    if (n > 0) {
      append_capped(out, buffer.data(), static_cast<std::size_t>(n), cap,
                    truncated);
      continue;
    }
    if (n == 0) {
      *open = false;
      (void)::close(fd);
      return;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    *open = false;
    (void)::close(fd);
    return;
  }
}

[[nodiscard]] process_result_t run_process(const std::vector<std::string> &argv,
                                           int timeout_seconds,
                                           std::size_t capture_cap) {
  process_result_t result{};
  if (argv.empty()) {
    result.stderr_text = "empty argv";
    return result;
  }
  int stdout_pipe[2]{-1, -1};
  int stderr_pipe[2]{-1, -1};
  if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
    result.stderr_text = "pipe failed";
    return result;
  }

  const pid_t pid = ::fork();
  if (pid < 0) {
    result.stderr_text = "fork failed";
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    return result;
  }
  if (pid == 0) {
    (void)::dup2(stdout_pipe[1], STDOUT_FILENO);
    (void)::dup2(stderr_pipe[1], STDERR_FILENO);
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1);
    for (const std::string &arg : argv) {
      cargv.push_back(const_cast<char *>(arg.c_str()));
    }
    cargv.push_back(nullptr);
    ::execv(cargv[0], cargv.data());
    const std::string err =
        std::string("execv failed: ") + std::strerror(errno) + "\n";
    const ssize_t ignored_write =
        ::write(STDERR_FILENO, err.data(), err.size());
    (void)ignored_write;
    _exit(127);
  }

  (void)::close(stdout_pipe[1]);
  (void)::close(stderr_pipe[1]);
  set_nonblocking(stdout_pipe[0]);
  set_nonblocking(stderr_pipe[0]);
  bool stdout_open = true;
  bool stderr_open = true;
  bool exited = false;
  int status = 0;
  const auto start = std::chrono::steady_clock::now();

  while (stdout_open || stderr_open || !exited) {
    if (!exited) {
      const pid_t waited = ::waitpid(pid, &status, WNOHANG);
      if (waited == pid) {
        exited = true;
      }
    }
    if (!exited && timeout_seconds > 0) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start);
      if (elapsed.count() > timeout_seconds) {
        result.timed_out = true;
        (void)::kill(pid, SIGKILL);
        (void)::waitpid(pid, &status, 0);
        exited = true;
      }
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfd = -1;
    if (stdout_open) {
      FD_SET(stdout_pipe[0], &readfds);
      maxfd = std::max(maxfd, stdout_pipe[0]);
    }
    if (stderr_open) {
      FD_SET(stderr_pipe[0], &readfds);
      maxfd = std::max(maxfd, stderr_pipe[0]);
    }
    if (maxfd >= 0) {
      timeval tv{};
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      const int ready = ::select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
      if (ready > 0) {
        if (stdout_open && FD_ISSET(stdout_pipe[0], &readfds)) {
          drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
                   &result.stdout_truncated, &stdout_open);
        }
        if (stderr_open && FD_ISSET(stderr_pipe[0], &readfds)) {
          drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
                   &result.stderr_truncated, &stderr_open);
        }
      }
    } else if (exited) {
      break;
    }
  }
  if (stdout_open) {
    drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
             &result.stdout_truncated, &stdout_open);
  }
  if (stderr_open) {
    drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
             &result.stderr_truncated, &stderr_open);
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.signaled = true;
    result.signal_number = WTERMSIG(status);
    result.exit_code = 128 + result.signal_number;
  }
  return result;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_process_stdout_kv(const std::string &stdout_text) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(stdout_text);
  std::string line;
  while (std::getline(lines, line)) {
    line = trim_ascii(line);
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos || eq == 0) {
      continue;
    }
    const std::string key = trim_ascii(line.substr(0, eq));
    bool key_ok = true;
    for (const char c : key) {
      const auto u = static_cast<unsigned char>(c);
      if (!(std::isalnum(u) || c == '_')) {
        key_ok = false;
        break;
      }
    }
    if (key_ok) {
      out[key] = trim_ascii(line.substr(eq + 1));
    }
  }
  return out;
}

[[nodiscard]] bool explicit_job_dir_allowed(const runtime_policy_t &policy,
                                            const fs::path &path) {
  return under_any_root(path,
                        split_csv(policy_get(policy, "allowed_job_roots")),
                        policy.policy_path);
}

[[nodiscard]] bool dev_nuke_root_allowed(const runtime_policy_t &policy,
                                         const fs::path &path) {
  return under_any_root(path,
                        split_csv(policy_get(policy, "allowed_dev_nuke_roots")),
                        policy.policy_path);
}

[[nodiscard]] fs::path runtime_root(const runtime_policy_t &policy) {
  return policy_path(policy, "runtime_root");
}

[[nodiscard]] bool looks_safe_to_clear_runtime_root(const fs::path &path) {
  const fs::path normalized = normalize_path(path);
  if (normalized.empty() || !normalized.is_absolute()) {
    return false;
  }
  if (normalized == normalized.root_path()) {
    return false;
  }
  if (!normalized.has_filename() || normalized.filename() == "." ||
      normalized.filename() == "..") {
    return false;
  }
  const fs::path parent = normalized.parent_path();
  return !parent.empty() && parent != normalized.root_path();
}

[[nodiscard]] std::string path_vector_json(const std::vector<fs::path> &paths) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(paths[i].string());
  }
  out << "]";
  return out.str();
}

struct dev_nuke_target_t {
  fs::path path{};
  bool is_directory{false};
  bool is_regular_file{false};
  std::uintmax_t entry_count{0};
};

struct active_job_marker_t {
  fs::path state_path{};
  std::string status{};
};

[[nodiscard]] bool terminal_job_status(std::string_view status) {
  const std::string value = lowercase_ascii(trim_ascii(status));
  return value == "dry_run" || value == "completed" || value == "failed" ||
         value == "stopped" || value == "cancelled" || value == "canceled" ||
         value == "interrupted";
}

[[nodiscard]] std::uintmax_t count_tree_entries(const fs::path &path) {
  std::error_code ec;
  if (!fs::exists(path, ec) || ec) {
    return 0;
  }
  std::uintmax_t count = 1;
  if (!fs::is_directory(path, ec) || ec) {
    return count;
  }
  fs::recursive_directory_iterator it(
      path, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  for (; !ec && it != end; it.increment(ec)) {
    ++count;
  }
  return count;
}

[[nodiscard]] bool
collect_dev_nuke_targets(const fs::path &root,
                         std::vector<dev_nuke_target_t> *targets,
                         std::string *err) {
  if (!targets) {
    if (err) {
      *err = "missing dev_nuke target output";
    }
    return false;
  }
  targets->clear();
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err = "failed to inspect runtime_root: " + root.string() + ": " +
             ec.message();
    }
    return false;
  }
  if (!fs::is_directory(root, ec) || ec) {
    if (err) {
      *err = "runtime_root is not a directory: " + root.string();
    }
    return false;
  }
  for (fs::directory_iterator it(root, ec), end; !ec && it != end;
       it.increment(ec)) {
    dev_nuke_target_t target{};
    target.path = normalize_path(it->path());
    target.is_directory = it->is_directory(ec);
    if (ec) {
      target.is_directory = false;
      ec.clear();
    }
    target.is_regular_file = it->is_regular_file(ec);
    if (ec) {
      target.is_regular_file = false;
      ec.clear();
    }
    target.entry_count = count_tree_entries(target.path);
    targets->push_back(std::move(target));
  }
  if (ec) {
    if (err) {
      *err =
          "failed to list runtime_root: " + root.string() + ": " + ec.message();
    }
    return false;
  }
  std::sort(targets->begin(), targets->end(),
            [](const dev_nuke_target_t &a, const dev_nuke_target_t &b) {
              return a.path.string() < b.path.string();
            });
  return true;
}

[[nodiscard]] std::string
dev_nuke_targets_json(const std::vector<dev_nuke_target_t> &targets) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"path\":" << json_quote(targets[i].path.string())
        << ",\"is_directory\":" << bool_json(targets[i].is_directory)
        << ",\"is_regular_file\":" << bool_json(targets[i].is_regular_file)
        << ",\"entry_count\":" << targets[i].entry_count << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool
scan_active_runtime_jobs(const fs::path &root,
                         std::vector<active_job_marker_t> *active_jobs,
                         std::string *err) {
  if (!active_jobs) {
    if (err) {
      *err = "missing active job output";
    }
    return false;
  }
  active_jobs->clear();
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err =
          "failed to inspect runtime_root for active jobs: " + root.string() +
          ": " + ec.message();
    }
    return false;
  }
  fs::recursive_directory_iterator it(
      root, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->path().filename() != "job.state") {
      continue;
    }
    const auto fields = parse_kv_file(it->path());
    const auto found = fields.find("status");
    const std::string status =
        found == fields.end() ? std::string{"<missing>"} : found->second;
    if (!terminal_job_status(status)) {
      active_jobs->push_back(active_job_marker_t{it->path(), status});
    }
  }
  if (ec) {
    if (err) {
      *err = "failed to scan runtime_root for active jobs: " + root.string() +
             ": " + ec.message();
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string
active_jobs_json(const std::vector<active_job_marker_t> &active_jobs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < active_jobs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"state_path\":" << json_quote(active_jobs[i].state_path.string())
        << ",\"status\":" << json_quote(active_jobs[i].status) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string sanitize_path_component(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const unsigned char c : value) {
    if (std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-') {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('_');
    }
  }
  return out.empty() ? std::string{"runtime_root"} : out;
}

[[nodiscard]] bool make_dev_nuke_snapshot_root(const fs::path &root,
                                               const fs::path &backup_root,
                                               fs::path *out,
                                               std::string *err) {
  if (!out) {
    if (err) {
      *err = "missing backup snapshot output";
    }
    return false;
  }
  const std::string root_leaf =
      sanitize_path_component(root.filename().string());
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  const std::string stamp_text = std::to_string(static_cast<long long>(stamp));
  const fs::path family_root = normalize_path(backup_root / root_leaf);
  fs::path candidate = family_root / stamp_text;
  std::error_code ec;
  int disambiguator = 1;
  while (fs::exists(candidate, ec)) {
    if (ec) {
      if (err) {
        *err = "failed to inspect dev_nuke backup path: " + candidate.string() +
               ": " + ec.message();
      }
      return false;
    }
    candidate =
        family_root / (stamp_text + "." + std::to_string(disambiguator++));
  }
  fs::create_directories(candidate, ec);
  if (ec) {
    if (err) {
      *err = "failed to create dev_nuke backup path: " + candidate.string() +
             ": " + ec.message();
    }
    return false;
  }
  *out = normalize_path(candidate);
  return true;
}

[[nodiscard]] bool move_dev_nuke_target(const fs::path &root,
                                        const fs::path &target,
                                        const fs::path &snapshot_root,
                                        fs::path *out_backup,
                                        std::string *err) {
  if (out_backup) {
    out_backup->clear();
  }
  if (!path_within(root, target) ||
      normalize_path(root) == normalize_path(target)) {
    if (err) {
      *err = "dev_nuke target escapes runtime_root: " + target.string();
    }
    return false;
  }
  const fs::path relative =
      normalize_path(target).lexically_relative(normalize_path(root));
  if (relative.empty() || relative == ".") {
    if (err) {
      *err = "failed to derive dev_nuke backup path for: " + target.string();
    }
    return false;
  }
  const fs::path backup_path = normalize_path(snapshot_root / relative);
  std::error_code ec;
  fs::create_directories(backup_path.parent_path(), ec);
  if (ec) {
    if (err) {
      *err = "failed to prepare dev_nuke backup path: " +
             backup_path.parent_path().string() + ": " + ec.message();
    }
    return false;
  }
  fs::rename(target, backup_path, ec);
  if (ec) {
    if (err) {
      *err = "failed to move runtime target into dev_nuke backup: " +
             target.string() + " -> " + backup_path.string() + ": " +
             ec.message();
    }
    return false;
  }
  if (out_backup) {
    *out_backup = backup_path;
  }
  return true;
}

[[nodiscard]] bool resolve_job_dir_from_args(const std::string &args,
                                             const runtime_policy_t &policy,
                                             fs::path *out, std::string *err) {
  std::string job_dir_arg;
  std::string job_id_arg;
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);
  (void)extract_json_string_field(args, "job_id", &job_id_arg);
  if (!trim_ascii(job_dir_arg).empty() && !trim_ascii(job_id_arg).empty()) {
    *err = "provide job_id or job_dir, not both";
    return false;
  }
  fs::path path;
  if (!trim_ascii(job_dir_arg).empty()) {
    path = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(policy, path)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside allowed_job_roots: " +
          path.string();
      return false;
    }
  } else if (!trim_ascii(job_id_arg).empty()) {
    if (job_id_arg.find('/') != std::string::npos ||
        job_id_arg.find('\\') != std::string::npos ||
        job_id_arg.find("..") != std::string::npos) {
      *err = "job_id must be a runtime root leaf";
      return false;
    }
    path = normalize_path(runtime_root(policy) / job_id_arg);
  } else {
    *err = "hero.runtime job lookup requires job_id or job_dir";
    return false;
  }
  if (out) {
    *out = path;
  }
  return true;
}

[[nodiscard]] std::string artifact_summary_json(const fs::path &path,
                                                bool include_text,
                                                std::size_t max_bytes) {
  const bool exists = fs::exists(path);
  const bool is_file = exists && fs::is_regular_file(path);
  std::ostringstream out;
  out << "{\"path\":" << json_quote(path.string())
      << ",\"exists\":" << bool_json(exists)
      << ",\"is_regular_file\":" << bool_json(is_file);
  if (is_file) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    out << ",\"size\":" << (ec ? 0 : size)
        << ",\"fields\":" << kv_map_to_json(parse_kv_file(path));
    if (include_text) {
      bool truncated = false;
      const std::string text =
          read_text_file_limited(path, max_bytes, &truncated);
      out << ",\"truncated\":" << bool_json(truncated)
          << ",\"text\":" << json_quote(text);
    }
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string job_artifacts_json(const fs::path &job_dir,
                                             const std::string &report_path,
                                             bool include_text,
                                             std::size_t max_bytes) {
  fs::path effective_report =
      report_path.empty() ? fs::path{} : fs::path(report_path);
  if (effective_report.empty()) {
    const fs::path inference = job_dir / "inference.report";
    const fs::path representation = job_dir / "representation.report";
    if (fs::exists(inference)) {
      effective_report = inference;
    } else if (fs::exists(representation)) {
      effective_report = representation;
    }
  }
  std::ostringstream out;
  out << "{\"job_dir\":" << json_quote(job_dir.string()) << ",\"manifest\":"
      << artifact_summary_json(job_dir / "job.manifest", include_text,
                               max_bytes)
      << ",\"state\":"
      << artifact_summary_json(job_dir / "job.state", include_text, max_bytes)
      << ",\"report\":";
  if (effective_report.empty()) {
    out << "null";
  } else {
    out << artifact_summary_json(effective_report, include_text, max_bytes);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string process_result_json(
    const std::vector<std::string> &argv, const process_result_t &result,
    const fs::path &job_dir,
    const std::unordered_map<std::string, std::string> &stdout_kv,
    bool include_text, std::size_t max_bytes) {
  std::ostringstream out;
  out << "{\"ok\":" << bool_json(result.exit_code == 0 && !result.timed_out)
      << ",\"exit_code\":" << result.exit_code
      << ",\"timed_out\":" << bool_json(result.timed_out)
      << ",\"signaled\":" << bool_json(result.signaled)
      << ",\"signal_number\":" << result.signal_number << ",\"argv\":[";
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(argv[i]);
  }
  out << "],\"stdout_truncated\":" << bool_json(result.stdout_truncated)
      << ",\"stderr_truncated\":" << bool_json(result.stderr_truncated)
      << ",\"stdout\":" << json_quote(result.stdout_text)
      << ",\"stderr\":" << json_quote(result.stderr_text)
      << ",\"stdout_fields\":" << kv_map_to_json(stdout_kv);
  fs::path artifact_job_dir = job_dir;
  if (artifact_job_dir.empty()) {
    const auto found = stdout_kv.find("manifest_path");
    if (found != stdout_kv.end()) {
      artifact_job_dir = fs::path(found->second).parent_path();
    }
  }
  if (!artifact_job_dir.empty()) {
    std::string report_path;
    const auto found_report = stdout_kv.find("report_path");
    if (found_report != stdout_kv.end()) {
      report_path = found_report->second;
    }
    out << ",\"artifacts\":"
        << job_artifacts_json(artifact_job_dir, report_path, include_text,
                              max_bytes);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool parse_optional_bool_arg(const std::string &args,
                                           std::string_view key,
                                           bool default_value, bool *out,
                                           std::string *err) {
  bool value = default_value;
  if (extract_json_raw_field(args, key, nullptr) &&
      !extract_json_bool_field(args, key, &value)) {
    *err = std::string(key) + " must be boolean";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_optional_int_arg(const std::string &args,
                                          std::string_view key,
                                          int default_value, int *out,
                                          std::string *err) {
  int value = default_value;
  if (extract_json_raw_field(args, key, nullptr) &&
      !extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool expected_wave_matches_runtime_wave(const std::string &args,
                                                      const wave_info_t &wave,
                                                      std::string *err) {
  std::string expected_raw;
  if (!extract_json_raw_field(args, "marshal_expected_wave", &expected_raw)) {
    return true;
  }

  std::string expected_target;
  if (extract_json_first_string_field(
          expected_raw,
          {"target_component_family_id", "target_component", "wave_target"},
          &expected_target)) {
    const std::string actual_target = wave.values.count("TARGET") != 0
                                          ? wave.values.at("TARGET")
                                          : std::string{};
    if (expected_target != actual_target) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: "
               "target_component_family_id differs";
      }
      return false;
    }
  }

  std::string expected_mode;
  if (extract_json_first_string_field(expected_raw, {"mode", "wave_mode"},
                                      &expected_mode)) {
    const std::string actual_mode = wave.values.count("MODE") != 0
                                        ? wave.values.at("MODE")
                                        : std::string{"run"};
    if (expected_mode != actual_mode) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: mode differs";
      }
      return false;
    }
  }

  std::string expected_source_range;
  if (extract_json_string_field(expected_raw, "source_range",
                                &expected_source_range)) {
    expected_source_range = canonical_source_range(expected_source_range);
    const std::string actual_source_range = canonical_source_range(
        wave.values.count("SOURCE_RANGE") != 0 ? wave.values.at("SOURCE_RANGE")
                                               : std::string{"all"});
    if (expected_source_range != actual_source_range) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_range differs";
      }
      return false;
    }
  }

  std::string expected_source_order;
  if (extract_json_string_field(expected_raw, "source_order",
                                &expected_source_order)) {
    expected_source_order = lowercase_ascii(trim_ascii(expected_source_order));
    const std::string actual_mode = wave.values.count("MODE") != 0
                                        ? wave.values.at("MODE")
                                        : std::string{"run"};
    const std::string actual_source_order =
        wave_source_order_from_info(wave, wave_action_from_mode(actual_mode));
    if (expected_source_order != actual_source_order) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_order differs";
      }
      return false;
    }
  }

  std::string expected_begin;
  if (extract_json_string_field(expected_raw, "anchor_index_begin",
                                &expected_begin)) {
    const std::string actual_begin =
        wave.values.count("ANCHOR_INDEX_BEGIN") != 0
            ? wave.values.at("ANCHOR_INDEX_BEGIN")
            : std::string{};
    if (expected_begin != actual_begin) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: anchor_index_begin differs";
      }
      return false;
    }
  }

  std::string expected_end;
  if (extract_json_string_field(expected_raw, "anchor_index_end",
                                &expected_end)) {
    const std::string actual_end = wave.values.count("ANCHOR_INDEX_END") != 0
                                       ? wave.values.at("ANCHOR_INDEX_END")
                                       : std::string{};
    if (expected_end != actual_end) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: anchor_index_end differs";
      }
      return false;
    }
  }

  std::string expected_source_key_begin;
  if (extract_json_string_field(expected_raw, "source_key_begin",
                                &expected_source_key_begin)) {
    const std::string actual_begin =
        wave.values.count("SOURCE_KEY_BEGIN") != 0
            ? wave.values.at("SOURCE_KEY_BEGIN")
            : (wave.values.count("ANCHOR_KEY_BEGIN") != 0
                   ? wave.values.at("ANCHOR_KEY_BEGIN")
                   : std::string{});
    if (expected_source_key_begin != actual_begin) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_key_begin differs";
      }
      return false;
    }
  }

  std::string expected_source_key_end;
  if (extract_json_string_field(expected_raw, "source_key_end",
                                &expected_source_key_end)) {
    const std::string actual_end =
        wave.values.count("SOURCE_KEY_END") != 0
            ? wave.values.at("SOURCE_KEY_END")
            : (wave.values.count("ANCHOR_KEY_END") != 0
                   ? wave.values.at("ANCHOR_KEY_END")
                   : std::string{});
    if (expected_source_key_end != actual_end) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_key_end differs";
      }
      return false;
    }
  }

  std::string expected_inputs_raw;
  if (extract_json_raw_field(expected_raw, "model_state_inputs",
                             &expected_inputs_raw)) {
    std::unordered_map<std::string, std::string> expected_inputs;
    if (!extract_json_string_object(expected_inputs_raw, &expected_inputs)) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: model_state_inputs malformed";
      }
      return false;
    }
    const auto actual_inputs = wave_model_state_inputs(wave);
    for (const auto &[key, expected_value] : expected_inputs) {
      const auto found = actual_inputs.find(key);
      const std::string normalized_expected =
          fs::path(expected_value).lexically_normal().string();
      if (found == actual_inputs.end() ||
          found->second != normalized_expected) {
        if (err) {
          *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: model_state_inputs differs";
        }
        return false;
      }
    }
  }

  return true;
}

[[nodiscard]] bool handle_status(const std::string &, runtime_context_t *ctx,
                                 std::string *out, std::string *) {
  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  const fs::path root = runtime_root(ctx->policy);
  const wave_info_t wave =
      load_wave_info(*ctx, effective_config_path(*ctx, ""));
  std::size_t job_count = 0;
  std::error_code ec;
  if (fs::is_directory(root, ec)) {
    for (fs::directory_iterator it(root, ec), end; !ec && it != end;
         it.increment(ec)) {
      if (it->is_directory(ec)) {
        ++job_count;
      }
    }
  }
  std::ostringstream json;
  json << "{\"policy_path\":" << json_quote(ctx->policy.policy_path.string())
       << ",\"global_config_path\":"
       << json_quote(ctx->global_config_path.string()) << ",\"protocol_layer\":"
       << json_quote(policy_get(ctx->policy, "protocol_layer"))
       << ",\"runtime_exec_path\":" << json_quote(exec_path.string())
       << ",\"runtime_exec_exists\":" << bool_json(fs::exists(exec_path))
       << ",\"runtime_exec_executable\":"
       << bool_json(executable_file(exec_path))
       << ",\"runtime_root\":" << json_quote(root.string())
       << ",\"runtime_root_exists\":" << bool_json(fs::exists(root))
       << ",\"job_count\":" << job_count << ",\"default_dry_run\":"
       << bool_json(policy_bool_or(ctx->policy, "default_dry_run", true))
       << ",\"allow_execute\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_execute", false))
       << ",\"allow_train_execute\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_train_execute", false))
       << ",\"allow_dev_nuke\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_dev_nuke", false))
       << ",\"dev_nuke_backup_enabled\":"
       << bool_json(
              policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", true))
       << ",\"dev_nuke_backup_root\":"
       << json_quote(policy_path(ctx->policy, "dev_nuke_backup_root").string())
       << ",\"wave\":" << wave_info_json(wave) << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_schema(const std::string &, runtime_context_t *,
                                 std::string *out, std::string *) {
  std::ostringstream json;
  json << "{\"keys\":[";
  for (std::size_t i = 0; i < kRuntimePolicyDescriptors.size(); ++i) {
    const auto &d = kRuntimePolicyDescriptors[i];
    if (i != 0) {
      json << ",";
    }
    json << "{\"key\":" << json_quote(d.key)
         << ",\"type\":" << json_quote(d.type)
         << ",\"required\":" << bool_json(d.required)
         << ",\"default\":" << json_quote(d.default_value)
         << ",\"range\":" << json_quote(d.range)
         << ",\"allowed\":" << json_quote(d.allowed)
         << ",\"summary\":" << json_quote(d.summary) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_wave(const std::string &args, runtime_context_t *ctx,
                               std::string *out, std::string *) {
  std::string config_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  *out = wave_info_json(
      load_wave_info(*ctx, effective_config_path(*ctx, config_arg)));
  return true;
}

[[nodiscard]] bool execute_runtime(const std::string &args,
                                   runtime_context_t *ctx, bool force_dry_run,
                                   std::string *out, std::string *err) {
  std::string config_arg;
  std::string job_dir_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);

  bool dry_run = policy_bool_or(ctx->policy, "default_dry_run", true);
  if (force_dry_run) {
    dry_run = true;
  } else if (!parse_optional_bool_arg(args, "dry_run", dry_run, &dry_run,
                                      err)) {
    return false;
  }

  bool force_rebuild_cache = false;
  if (!parse_optional_bool_arg(args, "force_rebuild_cache", false,
                               &force_rebuild_cache, err)) {
    return false;
  }
  if (force_rebuild_cache &&
      !policy_bool_or(ctx->policy, "allow_force_rebuild_cache", false)) {
    *err = "E_RUNTIME_FORCE_REBUILD_DENIED: allow_force_rebuild_cache is false";
    return false;
  }

  bool confirm_execute = false;
  if (!parse_optional_bool_arg(args, "confirm_execute", false, &confirm_execute,
                               err)) {
    return false;
  }
  const fs::path config_path = effective_config_path(*ctx, config_arg);
  const wave_info_t wave = load_wave_info(*ctx, config_path);
  if (!expected_wave_matches_runtime_wave(args, wave, err)) {
    return false;
  }
  const std::string mode = wave.values.count("MODE") != 0
                               ? wave.values.at("MODE")
                               : std::string{"run"};
  const std::string action = wave_action_from_mode(mode);

  if (!dry_run) {
    if (!policy_bool_or(ctx->policy, "allow_execute", false)) {
      *err = "E_RUNTIME_EXECUTE_DENIED: allow_execute is false";
      return false;
    }
    if (policy_bool_or(ctx->policy, "require_confirm_execute", true) &&
        !confirm_execute) {
      *err = "E_RUNTIME_CONFIRM_REQUIRED: confirm_execute=true is required";
      return false;
    }
    if (action == "train" &&
        !policy_bool_or(ctx->policy, "allow_train_execute", false)) {
      *err = "E_RUNTIME_TRAIN_DENIED: allow_train_execute is false";
      return false;
    }
  }

  fs::path job_dir;
  if (!trim_ascii(job_dir_arg).empty()) {
    job_dir = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, job_dir)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside allowed_job_roots: " +
          job_dir.string();
      return false;
    }
  }

  int timeout_seconds = policy_int_or(ctx->policy, "max_runtime_seconds", 600);
  if (!parse_optional_int_arg(args, "timeout_seconds", timeout_seconds,
                              &timeout_seconds, err)) {
    return false;
  }
  if (timeout_seconds < 1) {
    *err = "timeout_seconds must be >= 1";
    return false;
  }
  int max_capture = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (max_capture < 1024) {
    max_capture = 1024;
  }

  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  if (!executable_file(exec_path)) {
    *err = "E_RUNTIME_EXEC_MISSING: runtime_exec_path is not executable: " +
           exec_path.string();
    return false;
  }

  std::vector<std::string> argv{exec_path.string(), "--config",
                                config_path.string()};
  if (!job_dir.empty()) {
    argv.push_back("--job-dir");
    argv.push_back(job_dir.string());
  }
  if (dry_run) {
    argv.push_back("--dry-run");
  }
  if (force_rebuild_cache) {
    argv.push_back("--force-rebuild-cache");
  }

  process_result_t result =
      run_process(argv, timeout_seconds, static_cast<std::size_t>(max_capture));
  const auto stdout_kv = parse_process_stdout_kv(result.stdout_text);
  *out = process_result_json(argv, result, job_dir, stdout_kv, false,
                             static_cast<std::size_t>(max_capture));
  return true;
}

[[nodiscard]] bool handle_dry_run(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  return execute_runtime(args, ctx, true, out, err);
}

[[nodiscard]] bool handle_execute(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  return execute_runtime(args, ctx, false, out, err);
}

[[nodiscard]] bool handle_dev_nuke(const std::string &args,
                                   runtime_context_t *ctx, std::string *out,
                                   std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "runtime_root", &root_arg);
  fs::path root = trim_ascii(root_arg).empty()
                      ? runtime_root(ctx->policy)
                      : normalize_path(fs::path(root_arg));
  root = normalize_path(root);

  bool dry_run = true;
  if (!parse_optional_bool_arg(args, "dry_run", true, &dry_run, err)) {
    return false;
  }
  bool backup_enabled =
      policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", true);
  if (!parse_optional_bool_arg(args, "backup", backup_enabled, &backup_enabled,
                               err)) {
    return false;
  }
  bool confirm_dev_nuke = false;
  if (!parse_optional_bool_arg(args, "confirm_dev_nuke", false,
                               &confirm_dev_nuke, err)) {
    return false;
  }

  if (!looks_safe_to_clear_runtime_root(root)) {
    *err = "E_RUNTIME_DEV_NUKE_UNSAFE_ROOT: refusing unsafe runtime_root: " +
           root.string();
    return false;
  }
  if (!dev_nuke_root_allowed(ctx->policy, root)) {
    *err = "E_RUNTIME_DEV_NUKE_ROOT_DENIED: runtime_root is outside "
           "allowed_dev_nuke_roots: " +
           root.string();
    return false;
  }

  fs::path backup_root = policy_path(ctx->policy, "dev_nuke_backup_root");
  if (backup_enabled) {
    backup_root = normalize_path(backup_root);
    if (path_within(root, backup_root)) {
      *err =
          "E_RUNTIME_DEV_NUKE_BACKUP_DENIED: dev_nuke_backup_root must not be "
          "inside runtime_root";
      return false;
    }
  }

  std::vector<dev_nuke_target_t> targets;
  if (!collect_dev_nuke_targets(root, &targets, err)) {
    return false;
  }
  std::vector<active_job_marker_t> active_jobs;
  if (!scan_active_runtime_jobs(root, &active_jobs, err)) {
    return false;
  }

  std::uintmax_t planned_entries = 0;
  for (const auto &target : targets) {
    planned_entries += target.entry_count;
  }

  const bool allow_dev_nuke =
      policy_bool_or(ctx->policy, "allow_dev_nuke", false);
  const bool confirm_required =
      policy_bool_or(ctx->policy, "require_confirm_dev_nuke", true);
  if (dry_run) {
    std::ostringstream json;
    json << "{\"ok\":true,\"dry_run\":true,\"would_clear\":"
         << bool_json(targets.size() > 0) << ",\"would_block_on_active_jobs\":"
         << bool_json(!active_jobs.empty())
         << ",\"runtime_root\":" << json_quote(root.string())
         << ",\"runtime_root_exists\":" << bool_json(fs::exists(root))
         << ",\"target_count\":" << targets.size()
         << ",\"planned_entry_count\":" << planned_entries
         << ",\"backup_enabled\":" << bool_json(backup_enabled)
         << ",\"backup_root\":"
         << (backup_enabled ? json_quote(backup_root.string())
                            : std::string{"null"})
         << ",\"allow_dev_nuke\":" << bool_json(allow_dev_nuke)
         << ",\"confirm_required\":" << bool_json(confirm_required)
         << ",\"targets\":" << dev_nuke_targets_json(targets)
         << ",\"active_jobs\":" << active_jobs_json(active_jobs) << "}";
    *out = json.str();
    return true;
  }

  if (!allow_dev_nuke) {
    *err = "E_RUNTIME_DEV_NUKE_DENIED: allow_dev_nuke is false";
    return false;
  }
  if (confirm_required && !confirm_dev_nuke) {
    *err = "E_RUNTIME_DEV_NUKE_CONFIRM_REQUIRED: confirm_dev_nuke=true is "
           "required";
    return false;
  }
  if (!active_jobs.empty()) {
    *err = "E_RUNTIME_DEV_NUKE_ACTIVE_JOBS: refusing reset while active or "
           "unknown-status jobs exist";
    return false;
  }

  std::vector<fs::path> backed_up_paths;
  std::vector<fs::path> removed_paths;
  fs::path snapshot_root;
  std::uintmax_t cleared_entries = 0;
  if (backup_enabled && !targets.empty()) {
    if (!make_dev_nuke_snapshot_root(root, backup_root, &snapshot_root, err)) {
      return false;
    }
    for (const auto &target : targets) {
      fs::path backup_path;
      if (!move_dev_nuke_target(root, target.path, snapshot_root, &backup_path,
                                err)) {
        return false;
      }
      backed_up_paths.push_back(backup_path);
      cleared_entries += target.entry_count;
    }
  } else {
    for (const auto &target : targets) {
      std::error_code ec;
      const std::uintmax_t removed = fs::remove_all(target.path, ec);
      if (ec) {
        *err = "failed to remove runtime target during dev_nuke: " +
               target.path.string() + ": " + ec.message();
        return false;
      }
      removed_paths.push_back(target.path);
      cleared_entries += removed;
    }
  }

  std::ostringstream json;
  json << "{\"ok\":true,\"dry_run\":false,\"cleared\":true"
       << ",\"runtime_root\":" << json_quote(root.string())
       << ",\"target_count\":" << targets.size()
       << ",\"cleared_entry_count\":" << cleared_entries
       << ",\"backup_enabled\":" << bool_json(backup_enabled)
       << ",\"backup_snapshot_path\":";
  if (snapshot_root.empty()) {
    json << "null";
  } else {
    json << json_quote(snapshot_root.string());
  }
  json << ",\"backed_up_paths\":" << path_vector_json(backed_up_paths)
       << ",\"removed_paths\":" << path_vector_json(removed_paths)
       << ",\"active_jobs\":[]"
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_list_jobs(const std::string &args,
                                    runtime_context_t *ctx, std::string *out,
                                    std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "root", &root_arg);
  fs::path root = trim_ascii(root_arg).empty() ? runtime_root(ctx->policy)
                                               : normalize_path(root_arg);
  if (!explicit_job_dir_allowed(ctx->policy, root) &&
      !path_within(runtime_root(ctx->policy), root)) {
    *err = "E_RUNTIME_JOB_DIR_DENIED: root is outside Runtime Hero roots: " +
           root.string();
    return false;
  }
  int limit = 20;
  if (!parse_optional_int_arg(args, "limit", limit, &limit, err)) {
    return false;
  }
  bool include_artifacts = false;
  if (!parse_optional_bool_arg(args, "include_artifacts", false,
                               &include_artifacts, err)) {
    return false;
  }

  struct row_t {
    fs::path dir;
    fs::file_time_type time;
  };
  std::vector<row_t> rows;
  std::error_code ec;
  for (fs::directory_iterator it(root, ec), end; !ec && it != end;
       it.increment(ec)) {
    if (!it->is_directory(ec)) {
      continue;
    }
    fs::path marker = it->path() / "job.state";
    if (!fs::exists(marker)) {
      marker = it->path() / "job.manifest";
    }
    rows.push_back(row_t{it->path(), fs::last_write_time(marker, ec)});
  }
  std::sort(rows.begin(), rows.end(),
            [](const row_t &a, const row_t &b) { return a.time > b.time; });
  if (limit < 0) {
    limit = 0;
  }
  if (limit > 200) {
    limit = 200;
  }

  std::ostringstream json;
  json << "{\"root\":" << json_quote(root.string())
       << ",\"count\":" << rows.size() << ",\"jobs\":[";
  const std::size_t n =
      std::min<std::size_t>(rows.size(), static_cast<std::size_t>(limit));
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      json << ",";
    }
    const auto manifest = parse_kv_file(rows[i].dir / "job.manifest");
    const auto state = parse_kv_file(rows[i].dir / "job.state");
    json << "{\"job_dir\":" << json_quote(rows[i].dir.string())
         << ",\"job_id\":"
         << json_quote(manifest.count("job_id") != 0
                           ? manifest.at("job_id")
                           : rows[i].dir.filename().string())
         << ",\"status\":"
         << json_quote(state.count("status") != 0 ? state.at("status") : "")
         << ",\"job_kind\":"
         << json_quote(manifest.count("job_kind") != 0 ? manifest.at("job_kind")
                                                       : "")
         << ",\"target_component_family_id\":"
         << json_quote(manifest.count("target_component_family_id") != 0
                           ? manifest.at("target_component_family_id")
                           : "")
         << ",\"wave_action\":"
         << json_quote(manifest.count("wave_action") != 0
                           ? manifest.at("wave_action")
                           : "")
         << ",\"accepted_anchor_count\":"
         << json_quote(manifest.count("accepted_anchor_count") != 0
                           ? manifest.at("accepted_anchor_count")
                           : "");
    if (include_artifacts) {
      json << ",\"artifacts\":"
           << job_artifacts_json(rows[i].dir, "", false,
                                 static_cast<std::size_t>(policy_int_or(
                                     ctx->policy, "max_capture_bytes", 65536)));
    }
    json << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_get_job(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  fs::path job_dir;
  if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
    return false;
  }
  bool include_text = false;
  if (!parse_optional_bool_arg(args, "include_text", false, &include_text,
                               err)) {
    return false;
  }
  int max_bytes = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (!parse_optional_int_arg(args, "max_bytes", max_bytes, &max_bytes, err)) {
    return false;
  }
  if (max_bytes < 1) {
    *err = "max_bytes must be >= 1";
    return false;
  }
  *out = job_artifacts_json(job_dir, "", include_text,
                            static_cast<std::size_t>(max_bytes));
  return true;
}

[[nodiscard]] bool handle_read_artifact(const std::string &args,
                                        runtime_context_t *ctx,
                                        std::string *out, std::string *err) {
  std::string explicit_path_arg;
  (void)extract_json_string_field(args, "path", &explicit_path_arg);
  fs::path path;
  if (!trim_ascii(explicit_path_arg).empty()) {
    path = normalize_path(explicit_path_arg);
    if (!explicit_job_dir_allowed(ctx->policy, path) &&
        !path_within(runtime_root(ctx->policy), path)) {
      *err = "E_RUNTIME_ARTIFACT_DENIED: path is outside Runtime Hero roots: " +
             path.string();
      return false;
    }
  } else {
    fs::path job_dir;
    if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
      return false;
    }
    std::string artifact;
    (void)extract_json_string_field(args, "artifact", &artifact);
    artifact = lowercase_ascii(trim_ascii(artifact));
    if (artifact.empty() || artifact == "manifest") {
      path = job_dir / "job.manifest";
    } else if (artifact == "state") {
      path = job_dir / "job.state";
    } else if (artifact == "inference_report" || artifact == "report") {
      path = job_dir / "inference.report";
      if (!fs::exists(path)) {
        path = job_dir / "representation.report";
      }
    } else if (artifact == "representation_report") {
      path = job_dir / "representation.report";
    } else {
      *err = "unknown artifact: " + artifact;
      return false;
    }
  }
  int max_bytes = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (!parse_optional_int_arg(args, "max_bytes", max_bytes, &max_bytes, err)) {
    return false;
  }
  bool truncated = false;
  const std::string text = read_text_file_limited(
      path, static_cast<std::size_t>(max_bytes), &truncated);
  std::ostringstream json;
  json << "{\"path\":" << json_quote(path.string())
       << ",\"exists\":" << bool_json(fs::exists(path))
       << ",\"truncated\":" << bool_json(truncated)
       << ",\"text\":" << json_quote(text)
       << ",\"fields\":" << kv_map_to_json(parse_kv_file(path)) << "}";
  *out = json.str();
  return true;
}

using handler_fn = bool (*)(const std::string &, runtime_context_t *,
                            std::string *, std::string *);

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.runtime.status") {
    return handle_status;
  }
  if (name == "hero.runtime.schema") {
    return handle_schema;
  }
  if (name == "hero.runtime.wave") {
    return handle_wave;
  }
  if (name == "hero.runtime.dry_run") {
    return handle_dry_run;
  }
  if (name == "hero.runtime.execute") {
    return handle_execute;
  }
  if (name == "hero.runtime.dev_nuke") {
    return handle_dev_nuke;
  }
  if (name == "hero.runtime.list_jobs") {
    return handle_list_jobs;
  }
  if (name == "hero.runtime.get_job") {
    return handle_get_job;
  }
  if (name == "hero.runtime.read_artifact") {
    return handle_read_artifact;
  }
  return std::nullopt;
}

} // namespace

std::filesystem::path
resolve_runtime_hero_dsl_path(const std::filesystem::path &global_config_path) {
  if (const auto fresh =
          read_ini_value(global_config_path, "HERO", "runtime_hero_dsl_path")) {
    return resolve_against(global_config_path, *fresh);
  }
  return kDefaultRuntimePolicyPath;
}

bool load_runtime_policy(const std::filesystem::path &policy_path,
                         const std::filesystem::path &global_config,
                         runtime_policy_t *out, std::string *error) {
  if (!out) {
    if (error) {
      *error = "runtime policy output pointer is null";
    }
    return false;
  }
  runtime_policy_t policy{};
  policy.policy_path = policy_path;
  policy.global_config_path = global_config;
  for (const auto &descriptor : kRuntimePolicyDescriptors) {
    policy.values[std::string(descriptor.key)] =
        std::string(descriptor.default_value);
  }
  std::string text;
  if (!read_text_file(policy_path, &text, error)) {
    policy.from_template = true;
    text = std::string(kRuntimePolicyTemplateText);
  }
  for (auto &[key, value] : parse_assignment_text(text, false)) {
    policy.values[std::move(key)] = std::move(value);
  }
  if (policy_get(policy, "protocol_layer") != kProtocolLayerStdio) {
    if (error) {
      *error = std::string(kProtocolLayerHttpsSseFailFastMessage);
    }
    return false;
  }
  *out = std::move(policy);
  return true;
}

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       runtime_context_t *ctx,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json) {
    out_tool_result_json->clear();
  }
  if (out_error_message) {
    out_error_message->clear();
  }
  if (!ctx) {
    if (out_error_message) {
      *out_error_message = "Runtime Hero context pointer is null";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("Runtime Hero context pointer is null", -32603);
    }
    return false;
  }
  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) {
    arguments_json = "{}";
  }
  if (arguments_json.front() != '{') {
    if (out_error_message) {
      *out_error_message = "--args-json must be a JSON object";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("--args-json must be a JSON object", -32602);
    }
    return false;
  }

  const auto handler = find_handler(tool_name);
  if (!handler.has_value()) {
    const std::string err = "unknown tool: " + tool_name;
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      *out_tool_result_json = make_error_result(err, -32601);
    }
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok = (*handler)(arguments_json, ctx, &structured, &err);
  if (!ok) {
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      const int code = err.find("E_RUNTIME_") == 0 ? kPolicyErrorCode : -32602;
      *out_tool_result_json = make_error_result(err, code);
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json = make_tool_result(tool_name, structured);
  }
  return true;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kTools); ++i) {
    mcp_schema_compat::assert_tool_input_schema_compatible(
        kTools[i].name, kTools[i].input_schema_json);
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << json_quote(kTools[i].name)
        << ",\"description\":" << json_quote(kTools[i].description)
        << ",\"inputSchema\":" << kTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << bool_json(tool_is_read_only(kTools[i].name))
        << ",\"destructiveHint\":"
        << bool_json(tool_is_destructive(kTools[i].name))
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

void run_jsonrpc_stdio_loop(runtime_context_t *ctx) {
  std::string line;
  bool shutdown_seen = false;
  while (std::getline(std::cin, line)) {
    line = trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    std::string id_raw = "null";
    (void)extract_json_raw_field(line, "id", &id_raw);
    std::string method;
    if (!extract_json_string_field(line, "method", &method)) {
      continue;
    }
    if (method == "notifications/initialized") {
      continue;
    }
    if (method == "exit") {
      break;
    }
    if (method == "initialize") {
      std::string protocol = "2024-11-05";
      (void)extract_json_string_field(line, "protocolVersion", &protocol);
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"protocolVersion\":" << json_quote(protocol)
                << ",\"capabilities\":{\"tools\":{}},\"serverInfo\":{"
                   "\"name\":\"hero_runtime\",\"version\":\"2\"}}}"
                << std::endl;
      continue;
    }
    if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << build_tools_list_result_json() << "}"
                << std::endl;
      continue;
    }
    if (method == "resources/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"resources\":[]}}" << std::endl;
      continue;
    }
    if (method == "resources/templates/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"resourceTemplates\":[]}}" << std::endl;
      continue;
    }
    if (method == "shutdown") {
      shutdown_seen = true;
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":null}" << std::endl;
      continue;
    }
    if (method == "tools/call") {
      std::string params;
      if (!extract_json_raw_field(line, "params", &params)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32602,\"message\":\"missing "
                     "params\"}}"
                  << std::endl;
        continue;
      }
      std::string name;
      if (!extract_json_string_field(params, "name", &name)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32602,\"message\":\"missing tool "
                     "name\"}}"
                  << std::endl;
        continue;
      }
      std::string arguments = "{}";
      (void)extract_json_raw_field(params, "arguments", &arguments);
      std::string result;
      std::string error;
      (void)execute_tool_json(name, arguments, ctx, &result, &error);
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << result << "}" << std::endl;
      continue;
    }
    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
              << ",\"error\":{\"code\":-32601,\"message\":\"unknown method\"}}"
              << std::endl;
    if (shutdown_seen) {
      break;
    }
  }
}

} // namespace cuwacunu::hero::runtime
