#include "hero/lattice_hero/hero_lattice_tools.h"

#include "hero/lattice_hero/hero_lattice.h"
#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "kikijyeba/lattice/target/lattice_target_evaluator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cuwacunu::hero::lattice {
namespace {

namespace fs = std::filesystem;
namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
namespace target = cuwacunu::kikijyeba::lattice::target;

constexpr int kPolicyErrorCode = -32060;
constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultLatticePolicyPath =
    "/cuwacunu/src/config/hero.lattice.dsl";

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

constexpr tool_descriptor_t kTools[] = {
    {"hero.lattice.status",
     "Summarize Lattice Hero policy, target DSL, runtime root, exposure facts, "
     "and inferred active identity.",
     R"({"type":"object","properties":{"config_path":{"type":"string"},"runtime_root":{"type":"string"}},"additionalProperties":false})"},
    {"hero.lattice.schema", "List Lattice Hero policy keys and constraints.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.lattice.list_targets",
     "List configured lattice targets from the active global config.",
     R"({"type":"object","properties":{"config_path":{"type":"string"}},"additionalProperties":false})"},
    {"hero.lattice.explain_target",
     "Explain a compiled lattice target proof obligation without evaluating "
     "runtime evidence.",
     R"({"type":"object","properties":{"target_id":{"type":"string"},"config_path":{"type":"string"}},"required":["target_id"],"additionalProperties":false})"},
    {"hero.lattice.evaluate_target",
     "Evaluate one lattice target over runtime evidence without executing it; "
     "coverage targets include exposure-load summaries and non-blocking "
     "warning results.",
     R"({"type":"object","properties":{"target_id":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"}},"required":["target_id"],"additionalProperties":false})"},
    {"hero.lattice.plan_target",
     "Evaluate one lattice target and return only status plus any suggested "
     "next wave; coverage targets include exposure-load summaries and "
     "non-blocking warning results.",
     R"({"type":"object","properties":{"target_id":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"}},"required":["target_id"],"additionalProperties":false})"},
    {"hero.lattice.scan_exposure",
     "Scan runtime jobs into an in-memory lattice exposure ledger preview, "
     "including anchor-domain health and derived MDN node-exposure facts.",
     R"({"type":"object","properties":{"runtime_root":{"type":"string"},"limit":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.lattice.checkpoint_closure",
     "Inspect checkpoint exposure closure and unresolved input lineage.",
     R"({"type":"object","properties":{"checkpoint_path":{"type":"string"},"runtime_root":{"type":"string"},"limit":{"type":"integer"}},"required":["checkpoint_path"],"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.lattice.status" || name == "hero.lattice.schema" ||
         name == "hero.lattice.list_targets" ||
         name == "hero.lattice.explain_target" ||
         name == "hero.lattice.evaluate_target" ||
         name == "hero.lattice.plan_target" ||
         name == "hero.lattice.scan_exposure" ||
         name == "hero.lattice.checkpoint_closure";
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
        static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                                   '6', '7', '8', '9', 'a', 'b',
                                                   'c', 'd', 'e', 'f'};
        out << "\\u00" << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
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

[[nodiscard]] std::string double_json(double value) {
  if (!std::isfinite(value)) {
    return "null";
  }
  std::ostringstream out;
  out << value;
  return out.str();
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
  for (const auto &raw : roots) {
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

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_assignment_text(std::string_view text) {
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
    if (!value.empty() && value.back() == ';') {
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
  return parse_assignment_text(text);
}

[[nodiscard]] std::string
map_get(const std::unordered_map<std::string, std::string> &map,
        std::string_view key) {
  const auto it = map.find(std::string(key));
  return it == map.end() ? std::string{} : it->second;
}

[[nodiscard]] std::string policy_get(const lattice_policy_t &policy,
                                     std::string_view key) {
  const auto found = policy.values.find(std::string(key));
  if (found != policy.values.end()) {
    return found->second;
  }
  for (const auto &descriptor : kLatticePolicyDescriptors) {
    if (descriptor.key == key) {
      return std::string(descriptor.default_value);
    }
  }
  return {};
}

[[nodiscard]] bool policy_bool_or(const lattice_policy_t &policy,
                                  std::string_view key, bool fallback) {
  bool parsed = fallback;
  (void)parse_bool(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] int policy_int_or(const lattice_policy_t &policy,
                                std::string_view key, int fallback) {
  int parsed = fallback;
  (void)parse_int(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] fs::path policy_path(const lattice_policy_t &policy,
                                   std::string_view key) {
  return resolve_against(policy.policy_path, policy_get(policy, key));
}

[[nodiscard]] fs::path effective_config_path(const lattice_context_t &ctx,
                                             const std::string &arg) {
  if (!trim_ascii(arg).empty()) {
    return normalize_path(fs::path(arg));
  }
  if (!ctx.global_config_path.empty()) {
    return normalize_path(ctx.global_config_path);
  }
  return policy_path(ctx.policy, "default_config_path");
}

[[nodiscard]] fs::path effective_runtime_root(const lattice_context_t &ctx,
                                              const std::string &arg) {
  if (!trim_ascii(arg).empty()) {
    return normalize_path(fs::path(arg));
  }
  return policy_path(ctx.policy, "runtime_root");
}

[[nodiscard]] bool runtime_root_allowed(const lattice_policy_t &policy,
                                        const fs::path &path) {
  return under_any_root(path,
                        split_csv(policy_get(policy, "allowed_runtime_roots")),
                        policy.policy_path);
}

[[nodiscard]] bool parse_optional_int_arg(const std::string &json,
                                          std::string_view key, int fallback,
                                          int *out, std::string *err) {
  int value = fallback;
  if (extract_json_raw_field(json, key, nullptr) &&
      !extract_json_int_field(json, key, &value)) {
    if (err) {
      *err = "invalid integer argument: " + std::string(key);
    }
    return false;
  }
  if (out) {
    *out = value;
  }
  return true;
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

[[nodiscard]] std::string interval_json(exposure::anchor_interval_t interval) {
  std::ostringstream out;
  out << "{\"begin\":" << interval.begin << ",\"end\":" << interval.end
      << ",\"length\":" << interval.length() << "}";
  return out.str();
}

[[nodiscard]] std::string
string_array_json(const std::vector<std::string> &xs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(xs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string path_array_json(const std::vector<fs::path> &xs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(xs[i].lexically_normal().string());
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
suggested_wave_json(const target::lattice_suggested_wave_t &wave) {
  if (wave.empty()) {
    return "null";
  }
  std::ostringstream out;
  out << "{\"target\":" << json_quote(wave.target)
      << ",\"mode\":" << json_quote(wave.mode)
      << ",\"source_range\":" << json_quote(wave.source_range)
      << ",\"anchor_index_begin\":";
  if (wave.anchor_index_begin.has_value()) {
    out << *wave.anchor_index_begin;
  } else {
    out << "null";
  }
  out << ",\"anchor_index_end\":";
  if (wave.anchor_index_end.has_value()) {
    out << *wave.anchor_index_end;
  } else {
    out << "null";
  }
  out << ",\"input_mdn_checkpoint\":" << json_quote(wave.input_mdn_checkpoint)
      << ",\"input_representation_checkpoint\":"
      << json_quote(wave.input_representation_checkpoint);
  out << ",\"text\":" << json_quote(wave.to_text()) << "}";
  return out.str();
}

[[nodiscard]] std::string
exposure_uses_json(const std::vector<exposure::exposure_use_t> &uses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < uses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(exposure::exposure_use_name(uses[i]));
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_specs_json(
    const std::vector<target::lattice_warning_spec_t> &warnings) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < warnings.size(); ++i) {
    const auto &warning = warnings[i];
    if (i != 0) {
      out << ",";
    }
    out << "{\"warning_id\":" << json_quote(warning.warning_id)
        << ",\"kind\":" << json_quote(warning.kind)
        << ",\"split\":" << json_quote(warning.split)
        << ",\"anchor_index_begin\":";
    if (warning.anchor_index_begin.has_value()) {
      out << *warning.anchor_index_begin;
    } else {
      out << "null";
    }
    out << ",\"anchor_index_end\":";
    if (warning.anchor_index_end.has_value()) {
      out << *warning.anchor_index_end;
    } else {
      out << "null";
    }
    out << ",\"use\":" << json_quote(exposure::exposure_use_name(warning.use))
        << ",\"scope\":" << json_quote(warning.scope) << ",\"effect\":"
        << json_quote(warning.require_mutated_component ? "mutated_component"
                                                        : "any")
        << ",\"cursor_epochs_above\":"
        << double_json(warning.cursor_epochs_above)
        << ",\"metric\":" << json_quote(warning.metric)
        << ",\"above\":" << double_json(warning.above)
        << ",\"accepted_fraction_below\":"
        << double_json(warning.accepted_fraction_below)
        << ",\"skipped_failed_fetch_probe_above\":";
    if (warning.skipped_failed_fetch_probe_above.has_value()) {
      out << *warning.skipped_failed_fetch_probe_above;
    } else {
      out << "null";
    }
    out << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
target_spec_json(const target::lattice_target_spec_t &spec) {
  std::ostringstream out;
  out << "{\"target_id\":" << json_quote(spec.target_id)
      << ",\"use_profile\":" << json_quote(spec.use_profile)
      << ",\"guard_id\":" << json_quote(spec.guard_id)
      << ",\"target_class\":" << json_quote(spec.target_class)
      << ",\"kind\":" << json_quote(target::lattice_target_kind_name(spec.kind))
      << ",\"component\":" << json_quote(spec.component)
      << ",\"checkpoint_source\":" << json_quote(spec.checkpoint_source)
      << ",\"evaluated_checkpoint_source\":"
      << json_quote(spec.evaluated_checkpoint_source)
      << ",\"source_range\":" << json_quote(spec.source_range)
      << ",\"train_split\":" << json_quote(spec.train_split)
      << ",\"anchor_index_begin\":";
  if (spec.anchor_index_begin.has_value()) {
    out << *spec.anchor_index_begin;
  } else {
    out << "null";
  }
  out << ",\"anchor_index_end\":";
  if (spec.anchor_index_end.has_value()) {
    out << *spec.anchor_index_end;
  } else {
    out << "null";
  }
  out << ",\"forbid_split\":" << json_quote(spec.forbid_split)
      << ",\"forbid_exposure_anchor_index_begin\":";
  if (spec.forbid_exposure_anchor_index_begin.has_value()) {
    out << *spec.forbid_exposure_anchor_index_begin;
  } else {
    out << "null";
  }
  out << ",\"forbid_exposure_anchor_index_end\":";
  if (spec.forbid_exposure_anchor_index_end.has_value()) {
    out << *spec.forbid_exposure_anchor_index_end;
  } else {
    out << "null";
  }
  out << ",\"forbid_exposure_uses\":"
      << exposure_uses_json(spec.forbid_exposure_uses)
      << ",\"forbid_exposure_uses_from_split_policy\":"
      << bool_json(spec.forbid_exposure_uses_from_split_policy)
      << ",\"forbid_exposure_requires_mutated_component\":"
      << bool_json(spec.forbid_exposure_requires_mutated_component)
      << ",\"upstream_target_id\":" << json_quote(spec.upstream_target_id)
      << ",\"require_contract_match\":"
      << bool_json(spec.require_contract_match)
      << ",\"require_component_match\":"
      << bool_json(spec.require_component_match)
      << ",\"min_optimizer_steps\":" << spec.min_optimizer_steps
      << ",\"min_valid_target_fraction\":" << spec.min_valid_target_fraction
      << ",\"require_finite_loss\":" << bool_json(spec.require_finite_loss)
      << ",\"require_active_node_head_count\":"
      << spec.require_active_node_head_count
      << ",\"require_trained_node_head_count\":"
      << spec.require_trained_node_head_count
      << ",\"require_evaluated_node_head_count\":"
      << spec.require_evaluated_node_head_count
      << ",\"min_observed_input_coverage\":" << spec.min_observed_input_coverage
      << ",\"min_target_supervision_coverage\":"
      << spec.min_target_supervision_coverage
      << ",\"min_evaluation_metric_coverage\":"
      << spec.min_evaluation_metric_coverage
      << ",\"require_checkpoint_exists\":"
      << bool_json(spec.require_checkpoint_exists)
      << ",\"plan_mode\":" << json_quote(spec.plan_mode)
      << ",\"plan_input_mdn_checkpoint\":"
      << json_quote(spec.plan_input_mdn_checkpoint)
      << ",\"plan_input_representation_checkpoint\":"
      << json_quote(spec.plan_input_representation_checkpoint)
      << ",\"max_waves\":" << spec.max_waves
      << ",\"warning_specs\":" << warning_specs_json(spec.warning_specs) << "}";
  return out.str();
}

[[nodiscard]] std::string
clause_fields_json(const std::vector<target::lattice_clause_field_t> &fields) {
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(lowercase_ascii(fields[i].key)) << ":"
        << json_quote(fields[i].value);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string
dependency_clause_json(const target::lattice_dependency_clause_t &clause) {
  std::ostringstream out;
  out << "{\"clause_id\":" << json_quote(clause.clause_id)
      << ",\"target_id\":" << json_quote(clause.target_id)
      << ",\"upstream_target_id\":" << json_quote(clause.upstream_target_id)
      << ",\"binding\":" << json_quote(clause.binding)
      << ",\"require_exact_loaded_checkpoint\":"
      << bool_json(clause.require_exact_loaded_checkpoint)
      << ",\"fields\":" << clause_fields_json(clause.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string dependency_clauses_json(
    const std::vector<target::lattice_dependency_clause_t> &clauses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < clauses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << dependency_clause_json(clauses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
requirement_clause_json(const target::lattice_requirement_clause_t &clause) {
  std::ostringstream out;
  out << "{\"clause_id\":" << json_quote(clause.clause_id)
      << ",\"target_id\":" << json_quote(clause.target_id)
      << ",\"kind\":" << json_quote(clause.kind)
      << ",\"fields\":" << clause_fields_json(clause.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string requirement_clauses_json(
    const std::vector<target::lattice_requirement_clause_t> &clauses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < clauses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << requirement_clause_json(clauses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
forbid_clause_json(const target::lattice_forbid_clause_t &clause) {
  std::ostringstream out;
  out << "{\"clause_id\":" << json_quote(clause.clause_id)
      << ",\"target_id\":" << json_quote(clause.target_id)
      << ",\"kind\":" << json_quote(clause.kind)
      << ",\"fields\":" << clause_fields_json(clause.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string forbid_clauses_json(
    const std::vector<target::lattice_forbid_clause_t> &clauses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < clauses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << forbid_clause_json(clauses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
plan_clause_json(const target::lattice_plan_clause_t &clause) {
  std::ostringstream out;
  out << "{\"clause_id\":" << json_quote(clause.clause_id)
      << ",\"target_id\":" << json_quote(clause.target_id)
      << ",\"fields\":" << clause_fields_json(clause.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string
plan_clauses_json(const std::vector<target::lattice_plan_clause_t> &clauses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < clauses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << plan_clause_json(clauses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
warning_clause_json(const target::lattice_warning_clause_t &clause) {
  std::ostringstream out;
  out << "{\"clause_id\":" << json_quote(clause.clause_id)
      << ",\"target_id\":" << json_quote(clause.target_id)
      << ",\"kind\":" << json_quote(clause.kind)
      << ",\"fields\":" << clause_fields_json(clause.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string warning_clauses_json(
    const std::vector<target::lattice_warning_clause_t> &clauses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < clauses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << warning_clause_json(clauses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
compiled_target_json(const target::lattice_target_compiled_t &compiled) {
  std::ostringstream out;
  out << "{\"target_spec_fingerprint\":"
      << json_quote(compiled.target_spec_fingerprint)
      << ",\"lowered_v0\":" << target_spec_json(compiled.lowered_v0)
      << ",\"applied_profiles\":"
      << string_array_json(compiled.applied_profiles)
      << ",\"applied_guards\":" << string_array_json(compiled.applied_guards)
      << ",\"warnings\":" << string_array_json(compiled.warnings)
      << ",\"dependencies\":" << dependency_clauses_json(compiled.dependencies)
      << ",\"requirements\":" << requirement_clauses_json(compiled.requirements)
      << ",\"forbids\":" << forbid_clauses_json(compiled.forbids)
      << ",\"plans\":" << plan_clauses_json(compiled.plans)
      << ",\"warning_clauses\":"
      << warning_clauses_json(compiled.warning_clauses) << "}";
  return out.str();
}

[[nodiscard]] std::string
evidence_json(const target::lattice_target_evidence_t &evidence) {
  std::ostringstream out;
  out << "{\"job_dir\":" << json_quote(evidence.job_dir.string())
      << ",\"manifest_path\":" << json_quote(evidence.manifest_path.string())
      << ",\"state_path\":" << json_quote(evidence.state_path.string())
      << ",\"report_path\":" << json_quote(evidence.report_path.string())
      << ",\"report_exists\":" << bool_json(evidence.report_exists)
      << ",\"checkpoint_path\":"
      << json_quote(evidence.checkpoint_path.string())
      << ",\"checkpoint_written\":" << bool_json(evidence.checkpoint_written)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(evidence.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(evidence.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(evidence.source_cursor_token)
      << ",\"component_fingerprint\":"
      << json_quote(evidence.component_fingerprint)
      << ",\"status\":" << json_quote(evidence.status)
      << ",\"matching_job_count\":" << evidence.matching_job_count
      << ",\"matching_train_attempt_count\":"
      << evidence.matching_train_attempt_count
      << ",\"optimizer_steps\":" << evidence.optimizer_steps
      << ",\"loss\":" << double_json(evidence.loss)
      << ",\"valid_target_fraction\":"
      << double_json(evidence.valid_target_fraction)
      << ",\"active_node_head_count\":" << evidence.active_node_head_count
      << ",\"trained_node_head_count\":" << evidence.trained_node_head_count
      << ",\"evaluated_node_head_count\":" << evidence.evaluated_node_head_count
      << ",\"representation_checkpoint_path\":"
      << json_quote(evidence.representation_checkpoint_path.string())
      << ",\"representation_checkpoint_loaded\":"
      << bool_json(evidence.representation_checkpoint_loaded)
      << ",\"mdn_checkpoint_path\":"
      << json_quote(evidence.mdn_checkpoint_path.string())
      << ",\"mdn_checkpoint_loaded\":"
      << bool_json(evidence.mdn_checkpoint_loaded)
      << ",\"allow_untrained_representation\":"
      << bool_json(evidence.allow_untrained_representation) << "}";
  return out.str();
}

[[nodiscard]] std::string
exposure_load_summary_json(const exposure::exposure_load_summary_t &summary) {
  std::ostringstream out;
  out << "{\"use\":" << json_quote(exposure::exposure_use_name(summary.use))
      << ",\"component_scope\":" << json_quote(summary.component_scope)
      << ",\"target_component\":" << json_quote(summary.target_component)
      << ",\"require_mutated_component\":"
      << bool_json(summary.require_mutated_component)
      << ",\"target_range\":" << interval_json(summary.target_range)
      << ",\"unique_covered_anchors\":" << summary.unique_covered_anchors
      << ",\"unique_coverage_fraction\":"
      << double_json(summary.unique_coverage_fraction)
      << ",\"loaded_anchor_events\":" << summary.loaded_anchor_events
      << ",\"cursor_exposure_load\":"
      << double_json(summary.cursor_exposure_load)
      << ",\"fact_count\":" << summary.fact_count
      << ",\"optimizer_steps_total\":" << summary.optimizer_steps_total
      << ",\"optimizer_steps_per_unique_anchor\":"
      << double_json(summary.optimizer_steps_per_unique_anchor)
      << ",\"optimizer_steps_per_cursor_epoch\":"
      << double_json(summary.optimizer_steps_per_cursor_epoch)
      << ",\"valid_target_count_total\":" << summary.valid_target_count_total
      << ",\"valid_target_cursor_epochs\":"
      << double_json(summary.valid_target_cursor_epochs) << "}";
  return out.str();
}

[[nodiscard]] std::string exposure_load_summaries_json(
    const std::vector<exposure::exposure_load_summary_t> &summaries) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < summaries.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << exposure_load_summary_json(summaries[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_result_json(
    const target::lattice_target_evaluation_t::warning_result_t &warning) {
  std::ostringstream out;
  out << "{\"warning_id\":" << json_quote(warning.warning_id)
      << ",\"kind\":" << json_quote(warning.kind)
      << ",\"status\":" << json_quote(warning.status)
      << ",\"severity\":" << json_quote(warning.severity)
      << ",\"measured_value\":" << double_json(warning.measured_value)
      << ",\"threshold\":" << double_json(warning.threshold)
      << ",\"unit\":" << json_quote(warning.unit)
      << ",\"split\":" << json_quote(warning.split)
      << ",\"use\":" << json_quote(warning.use)
      << ",\"scope\":" << json_quote(warning.scope)
      << ",\"effect\":" << json_quote(warning.effect)
      << ",\"metric\":" << json_quote(warning.metric)
      << ",\"message\":" << json_quote(warning.message)
      << ",\"fact_count\":" << warning.exposure_summary.fact_count
      << ",\"unique_coverage_fraction\":"
      << double_json(warning.exposure_summary.unique_coverage_fraction)
      << ",\"cursor_exposure_load\":"
      << double_json(warning.exposure_summary.cursor_exposure_load)
      << ",\"optimizer_steps_total\":"
      << warning.exposure_summary.optimizer_steps_total
      << ",\"optimizer_steps_per_unique_anchor\":"
      << double_json(warning.exposure_summary.optimizer_steps_per_unique_anchor)
      << ",\"optimizer_steps_per_cursor_epoch\":"
      << double_json(warning.exposure_summary.optimizer_steps_per_cursor_epoch)
      << ",\"exposure_summary\":"
      << exposure_load_summary_json(warning.exposure_summary) << "}";
  return out.str();
}

[[nodiscard]] std::string warning_results_json(
    const std::vector<target::lattice_target_evaluation_t::warning_result_t>
        &warnings) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < warnings.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << warning_result_json(warnings[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
evaluation_json(const target::lattice_target_evaluation_t &eval) {
  std::ostringstream out;
  out << "{\"target_id\":" << json_quote(eval.target_id)
      << ",\"kind\":" << json_quote(target::lattice_target_kind_name(eval.kind))
      << ",\"component\":" << json_quote(eval.component) << ",\"status\":"
      << json_quote(target::lattice_target_status_name(eval.status))
      << ",\"split_policy_fingerprint\":"
      << json_quote(eval.split_policy_fingerprint)
      << ",\"plan_ready\":" << bool_json(eval.plan_ready)
      << ",\"reasons\":" << string_array_json(eval.reasons)
      << ",\"suggested_wave\":" << suggested_wave_json(eval.suggested_wave)
      << ",\"evidence\":" << evidence_json(eval.evidence)
      << ",\"exposure_summaries\":"
      << exposure_load_summaries_json(eval.exposure_summaries)
      << ",\"warnings\":" << string_array_json(eval.warnings)
      << ",\"warning_results\":" << warning_results_json(eval.warning_results)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
fact_json(const exposure::lattice_exposure_fact_t &f) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(f.schema)
      << ",\"fact_type\":" << json_quote(f.fact_type)
      << ",\"contract_fingerprint\":" << json_quote(f.contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(f.graph_order_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(f.component_assembly_fingerprint)
      << ",\"target_component\":" << json_quote(f.target_component)
      << ",\"job_id\":" << json_quote(f.job_id)
      << ",\"wave_id\":" << json_quote(f.wave_id)
      << ",\"job_status\":" << json_quote(f.job_status)
      << ",\"wave_action\":" << json_quote(f.wave_action)
      << ",\"cursor_domain\":" << json_quote(f.cursor_domain)
      << ",\"source_cursor_token\":" << json_quote(f.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(f.split_policy_fingerprint)
      << ",\"anchor_range\":" << interval_json(f.anchor_range)
      << ",\"coverage_precision\":" << json_quote(f.coverage_precision)
      << ",\"completed_anchor_range\":"
      << interval_json(f.completed_anchor_range)
      << ",\"observed_source_row_footprint\":"
      << interval_json(f.observed_footprint)
      << ",\"target_source_row_footprint\":"
      << interval_json(f.target_footprint)
      << ",\"footprint_precision\":" << json_quote(f.footprint_precision)
      << ",\"source_input_length\":" << f.source_input_length
      << ",\"source_future_length\":" << f.source_future_length
      << ",\"first_anchor_key\":" << json_quote(f.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(f.last_anchor_key)
      << ",\"anchor_domain_health\":{\"candidate_anchor_count\":"
      << f.candidate_anchor_count
      << ",\"accepted_anchor_count\":" << f.accepted_anchor_count
      << ",\"accepted_anchor_fraction\":"
      << double_json(f.accepted_anchor_fraction)
      << ",\"skipped_outside_common_range\":" << f.skipped_outside_common_range
      << ",\"skipped_missing_edge_coverage\":"
      << f.skipped_missing_edge_coverage
      << ",\"skipped_failed_fetch_probe\":" << f.skipped_failed_fetch_probe
      << ",\"duplicate_anchor_count\":" << f.duplicate_anchor_count
      << ",\"warning_level\":" << json_quote(f.anchor_domain_warning_level)
      << ",\"warnings\":" << json_quote(f.anchor_domain_warnings)
      << ",\"common_left_key\":" << json_quote(f.common_left_key)
      << ",\"common_right_key\":" << json_quote(f.common_right_key)
      << ",\"reference_left_key\":" << json_quote(f.reference_left_key)
      << ",\"reference_right_key\":" << json_quote(f.reference_right_key) << "}"
      << ",\"use\":{\"observed_input\":" << bool_json(f.use.observed_input)
      << ",\"target_supervision\":" << bool_json(f.use.target_supervision)
      << ",\"evaluation_metric\":" << bool_json(f.use.evaluation_metric)
      << ",\"selection_signal\":" << bool_json(f.use.selection_signal)
      << ",\"mutated_component\":" << bool_json(f.use.mutated_component)
      << "},\"effort\":{\"anchors_seen\":" << f.anchors_seen
      << ",\"batches_seen\":" << f.batches_seen
      << ",\"optimizer_steps\":" << f.optimizer_steps
      << ",\"wave_pulses_completed\":" << f.wave_pulses_completed
      << ",\"valid_target_count\":" << f.valid_target_count
      << ",\"valid_target_fraction\":" << double_json(f.valid_target_fraction)
      << "},\"output_checkpoint\":"
      << json_quote(f.output_checkpoint.lexically_normal().string())
      << ",\"input_checkpoints\":" << path_array_json(f.input_checkpoints)
      << ",\"finite_loss\":" << bool_json(f.finite_loss)
      << ",\"mean_loss\":" << double_json(f.mean_loss)
      << ",\"mean_nll\":" << double_json(f.mean_nll)
      << ",\"digest\":" << json_quote(exposure::exposure_fact_digest(f)) << "}";
  return out.str();
}

[[nodiscard]] std::string
facts_json(const std::vector<exposure::lattice_exposure_fact_t> &facts,
           std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
node_fact_json(const exposure::lattice_node_exposure_fact_t &f) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(f.schema)
      << ",\"fact_type\":" << json_quote(f.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(f.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(f.contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(f.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(f.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(f.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(f.component_assembly_fingerprint)
      << ",\"target_component\":" << json_quote(f.target_component)
      << ",\"job_id\":" << json_quote(f.job_id)
      << ",\"wave_id\":" << json_quote(f.wave_id)
      << ",\"job_status\":" << json_quote(f.job_status)
      << ",\"wave_action\":" << json_quote(f.wave_action)
      << ",\"node_id\":" << json_quote(f.node_id)
      << ",\"node_index\":" << f.node_index
      << ",\"split_name\":" << json_quote(f.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(f.split_role))
      << ",\"anchor_range\":" << interval_json(f.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(f.completed_anchor_range)
      << ",\"use\":{\"observed_input\":" << bool_json(f.use.observed_input)
      << ",\"target_supervision\":" << bool_json(f.use.target_supervision)
      << ",\"evaluation_metric\":" << bool_json(f.use.evaluation_metric)
      << ",\"selection_signal\":" << bool_json(f.use.selection_signal)
      << ",\"mutated_component\":" << bool_json(f.use.mutated_component)
      << "},\"valid_target_count\":" << f.valid_target_count
      << ",\"routed_row_count\":" << f.routed_row_count
      << ",\"active_row_count\":" << f.active_row_count
      << ",\"trained_row_count\":" << f.trained_row_count
      << ",\"evaluated_row_count\":" << f.evaluated_row_count
      << ",\"valid_target_fraction\":" << double_json(f.valid_target_fraction)
      << ",\"mean_nll\":" << double_json(f.mean_nll)
      << ",\"output_checkpoint\":"
      << json_quote(f.output_checkpoint.lexically_normal().string()) << "}";
  return out.str();
}

[[nodiscard]] std::string node_facts_json(
    const std::vector<exposure::lattice_node_exposure_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << node_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
checkpoint_fact_json(const exposure::lattice_checkpoint_fact_t &f) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(f.schema)
      << ",\"fact_type\":" << json_quote(f.fact_type)
      << ",\"checkpoint_id\":" << json_quote(f.checkpoint_id)
      << ",\"checkpoint_path\":"
      << json_quote(f.checkpoint_path.lexically_normal().string())
      << ",\"checkpoint_file_digest\":" << json_quote(f.checkpoint_file_digest)
      << ",\"component\":" << json_quote(f.component)
      << ",\"contract_fingerprint\":" << json_quote(f.contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(f.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(f.source_cursor_token)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(f.component_assembly_fingerprint)
      << ",\"created_by_job_id\":" << json_quote(f.created_by_job_id)
      << ",\"created_by_wave_id\":" << json_quote(f.created_by_wave_id)
      << ",\"created_by_exposure_fact_id\":"
      << json_quote(f.created_by_exposure_fact_id)
      << ",\"input_checkpoints\":" << path_array_json(f.input_checkpoints)
      << ",\"direct_exposure_digest\":" << json_quote(f.direct_exposure_digest)
      << ",\"closure_digest\":" << json_quote(f.closure_digest) << "}";
  return out.str();
}

[[nodiscard]] std::string checkpoint_facts_json(
    const std::vector<exposure::lattice_checkpoint_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << checkpoint_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] target::lattice_target_active_identity_t
derive_active_identity_from_runtime_root(const fs::path &runtime_root) {
  struct row_t {
    fs::path dir{};
    fs::file_time_type time{};
  };
  std::vector<row_t> rows;
  std::error_code ec;
  if (!fs::is_directory(runtime_root, ec)) {
    return {};
  }
  const auto push_job_dir = [&](const fs::path &dir) {
    const fs::path marker = fs::exists(dir / "job.state")
                                ? dir / "job.state"
                                : dir / "job.manifest";
    std::error_code time_ec;
    auto time = fs::last_write_time(marker, time_ec);
    if (time_ec) {
      time = fs::file_time_type::min();
    }
    rows.push_back(row_t{.dir = dir, .time = time});
  };
  if (fs::exists(runtime_root / "job.manifest")) {
    push_job_dir(runtime_root);
  } else {
    for (fs::directory_iterator it(runtime_root, ec), end; !ec && it != end;
         it.increment(ec)) {
      if (!it->is_directory(ec) || !fs::exists(it->path() / "job.manifest")) {
        continue;
      }
      push_job_dir(it->path());
    }
  }
  std::sort(rows.begin(), rows.end(),
            [](const row_t &a, const row_t &b) { return a.time > b.time; });

  target::lattice_target_active_identity_t id{};
  for (const auto &row : rows) {
    const auto manifest = parse_kv_file(row.dir / "job.manifest");
    if (id.protocol_contract_fingerprint.empty()) {
      id.protocol_contract_fingerprint =
          map_get(manifest, "protocol_contract_fingerprint");
    }
    if (id.graph_order_fingerprint.empty()) {
      id.graph_order_fingerprint = map_get(manifest, "graph_order_fingerprint");
    }
    if (id.source_cursor_token.empty()) {
      id.source_cursor_token = map_get(manifest, "source_cursor_token");
    }
    if (id.vicreg_assembly_fingerprint.empty()) {
      id.vicreg_assembly_fingerprint =
          map_get(manifest, "vicreg_assembly_fingerprint");
    }
    if (id.mdn_assembly_fingerprint.empty()) {
      id.mdn_assembly_fingerprint =
          map_get(manifest, "mdn_assembly_fingerprint");
    }
  }
  return id;
}

void overlay_active_identity_from_args(
    const std::string &args, target::lattice_target_active_identity_t *id) {
  if (!id) {
    return;
  }
  std::string value;
  if (extract_json_string_field(args, "protocol_contract_fingerprint",
                                &value)) {
    id->protocol_contract_fingerprint = value;
  }
  if (extract_json_string_field(args, "graph_order_fingerprint", &value)) {
    id->graph_order_fingerprint = value;
  }
  if (extract_json_string_field(args, "source_cursor_token", &value)) {
    id->source_cursor_token = value;
  }
  if (extract_json_string_field(args, "vicreg_assembly_fingerprint", &value)) {
    id->vicreg_assembly_fingerprint = value;
  }
  if (extract_json_string_field(args, "mdn_assembly_fingerprint", &value)) {
    id->mdn_assembly_fingerprint = value;
  }
}

[[nodiscard]] std::string
active_identity_json(const target::lattice_target_active_identity_t &id) {
  std::ostringstream out;
  out << "{\"protocol_contract_fingerprint\":"
      << json_quote(id.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(id.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(id.source_cursor_token)
      << ",\"vicreg_assembly_fingerprint\":"
      << json_quote(id.vicreg_assembly_fingerprint)
      << ",\"mdn_assembly_fingerprint\":"
      << json_quote(id.mdn_assembly_fingerprint) << "}";
  return out.str();
}

[[nodiscard]] bool resolve_runtime_root_arg(const std::string &args,
                                            lattice_context_t *ctx,
                                            fs::path *out, std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "runtime_root", &root_arg);
  const fs::path root = effective_runtime_root(*ctx, root_arg);
  if (!runtime_root_allowed(ctx->policy, root)) {
    if (err) {
      *err = "E_LATTICE_ROOT_DENIED: runtime_root is outside allowed roots: " +
             root.string();
    }
    return false;
  }
  if (out) {
    *out = root;
  }
  return true;
}

[[nodiscard]] bool
build_target_evaluator(const std::string &args, lattice_context_t *ctx,
                       std::vector<target::lattice_target_spec_t> *targets_out,
                       exposure::exposure_ledger_scan_result_t *scan_out,
                       target::lattice_target_active_identity_t *identity_out,
                       fs::path *config_out, fs::path *runtime_root_out,
                       std::string *err) {
  std::string config_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  const fs::path config_path = effective_config_path(*ctx, config_arg);
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  std::vector<target::lattice_target_spec_t> targets;
  try {
    targets = target::load_lattice_targets_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  auto identity = derive_active_identity_from_runtime_root(runtime_root);
  overlay_active_identity_from_args(args, &identity);
  exposure::exposure_ledger_scan_result_t scan{};
  if (policy_bool_or(ctx->policy, "auto_build_exposure_ledger", true)) {
    scan = exposure::scan_exposure_ledger_from_runtime_root(runtime_root);
  }
  if (targets_out) {
    *targets_out = std::move(targets);
  }
  if (scan_out) {
    *scan_out = std::move(scan);
  }
  if (identity_out) {
    *identity_out = std::move(identity);
  }
  if (config_out) {
    *config_out = config_path;
  }
  if (runtime_root_out) {
    *runtime_root_out = runtime_root;
  }
  return true;
}

[[nodiscard]] bool handle_status(const std::string &args,
                                 lattice_context_t *ctx, std::string *out,
                                 std::string *err) {
  std::vector<target::lattice_target_spec_t> targets;
  exposure::exposure_ledger_scan_result_t scan;
  target::lattice_target_active_identity_t identity;
  fs::path config_path;
  fs::path runtime_root;
  if (!build_target_evaluator(args, ctx, &targets, &scan, &identity,
                              &config_path, &runtime_root, err)) {
    return false;
  }
  target::lattice_target_config_paths_t paths{};
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
  } catch (...) {
  }
  std::optional<cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
      split_policy{};
  try {
    split_policy =
        target::load_lattice_split_policy_from_config_if_available(config_path);
  } catch (...) {
  }
  const auto validation = target::validate_lattice_target_specs(targets);
  std::ostringstream json;
  json << "{\"ok\":true"
       << ",\"policy_path\":" << json_quote(ctx->policy_path.string())
       << ",\"policy_from_template\":" << bool_json(ctx->policy.from_template)
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"targets_path\":" << json_quote(paths.targets_dsl_path.string())
       << ",\"splits_path\":" << json_quote(paths.splits_dsl_path.string())
       << ",\"split_count\":"
       << (split_policy.has_value() ? split_policy->splits.size() : 0)
       << ",\"split_policy_fingerprint\":"
       << json_quote(split_policy.has_value()
                         ? cuwacunu::kikijyeba::lattice::split::
                               lattice_split_policy_fingerprint(*split_policy)
                         : std::string{})
       << ",\"target_count\":" << targets.size()
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"target_warnings\":" << string_array_json(validation.warnings)
       << ",\"active_identity\":" << active_identity_json(identity) << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_schema(const std::string &, lattice_context_t *,
                                 std::string *out, std::string *) {
  std::ostringstream json;
  json << "{\"policy_keys\":[";
  for (std::size_t i = 0; i < kLatticePolicyDescriptors.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    const auto &d = kLatticePolicyDescriptors[i];
    json << "{\"key\":" << json_quote(d.key)
         << ",\"type\":" << json_quote(d.type)
         << ",\"required\":" << bool_json(d.required)
         << ",\"default_value\":" << json_quote(d.default_value)
         << ",\"range\":" << json_quote(d.range)
         << ",\"allowed\":" << json_quote(d.allowed)
         << ",\"summary\":" << json_quote(d.summary) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_list_targets(const std::string &args,
                                       lattice_context_t *ctx, std::string *out,
                                       std::string *err) {
  std::string config_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  const fs::path config_path = effective_config_path(*ctx, config_arg);
  std::vector<target::lattice_target_spec_t> targets;
  target::lattice_target_config_paths_t paths{};
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
    targets = target::load_lattice_targets_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  const auto validation = target::validate_lattice_target_specs(targets);
  std::ostringstream json;
  json << "{\"config_path\":" << json_quote(config_path.string())
       << ",\"targets_path\":" << json_quote(paths.targets_dsl_path.string())
       << ",\"splits_path\":" << json_quote(paths.splits_dsl_path.string())
       << ",\"target_count\":" << targets.size()
       << ",\"warnings\":" << string_array_json(validation.warnings)
       << ",\"targets\":[";
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << target_spec_json(targets[i]);
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_explain_target(const std::string &args,
                                         lattice_context_t *ctx,
                                         std::string *out, std::string *err) {
  std::string target_id;
  if (!extract_json_string_field(args, "target_id", &target_id) ||
      trim_ascii(target_id).empty()) {
    if (err) {
      *err = "target_id is required";
    }
    return false;
  }
  std::string config_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  const fs::path config_path = effective_config_path(*ctx, config_arg);
  std::vector<target::lattice_target_compiled_t> compiled_targets;
  target::lattice_target_config_paths_t paths{};
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
    compiled_targets =
        target::load_lattice_compiled_targets_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  const target::lattice_target_compiled_t *found = nullptr;
  for (const auto &compiled : compiled_targets) {
    if (compiled.lowered_v0.target_id == target_id) {
      found = &compiled;
      break;
    }
  }
  if (found == nullptr) {
    if (err) {
      *err = "unknown target_id: " + target_id;
    }
    return false;
  }

  std::optional<cuwacunu::kikijyeba::lattice::split::lattice_split_policy_t>
      split_policy{};
  try {
    split_policy =
        target::load_lattice_split_policy_from_config_if_available(config_path);
  } catch (...) {
  }

  std::ostringstream text;
  const auto &spec = found->lowered_v0;
  text << "Target " << spec.target_id << " asks whether " << spec.component
       << " has " << spec.target_class << " evidence over ";
  if (!spec.train_split.empty()) {
    text << "split " << spec.train_split;
  } else if (spec.anchor_index_begin.has_value() &&
             spec.anchor_index_end.has_value()) {
    text << "anchor_index[" << *spec.anchor_index_begin << ","
         << *spec.anchor_index_end << ")";
  } else {
    text << spec.source_range;
  }
  text << ".";
  if (!spec.upstream_target_id.empty()) {
    text << " It depends on " << spec.upstream_target_id << ".";
  }
  if (spec.checkpoint_source != "output_checkpoint") {
    text << " It inspects checkpoint source " << spec.checkpoint_source << ".";
  }
  if (!spec.evaluated_checkpoint_source.empty()) {
    text << " It requires evaluation evidence to load "
         << spec.evaluated_checkpoint_source << ".";
  }
  std::vector<std::string> requirements;
  if (spec.require_checkpoint_exists) {
    requirements.push_back("a checkpoint");
  }
  if (spec.require_finite_loss) {
    requirements.push_back("finite loss");
  }
  if (spec.min_optimizer_steps > 0) {
    requirements.push_back("optimizer_steps >= " +
                           std::to_string(spec.min_optimizer_steps));
  }
  if (spec.min_valid_target_fraction > 0.0) {
    std::ostringstream req;
    req << "valid_target_fraction >= " << spec.min_valid_target_fraction;
    requirements.push_back(req.str());
  }
  if (spec.require_active_node_head_count > 0) {
    requirements.push_back("active_node_heads >= " +
                           std::to_string(spec.require_active_node_head_count));
  }
  if (spec.require_trained_node_head_count > 0) {
    requirements.push_back(
        "trained_node_heads >= " +
        std::to_string(spec.require_trained_node_head_count));
  }
  if (spec.require_evaluated_node_head_count > 0) {
    requirements.push_back(
        "evaluated_node_heads >= " +
        std::to_string(spec.require_evaluated_node_head_count));
  }
  for (const auto &clause : found->requirements) {
    if (clause.kind != "exposure_coverage") {
      continue;
    }
    const auto use =
        target::lattice_clause_field_value(clause.fields, "USE", "exposure");
    const auto split = target::lattice_clause_field_value(
        clause.fields, "SPLIT", spec.train_split);
    auto cursor_epochs =
        target::lattice_clause_field_value(clause.fields, "CURSOR_EPOCHS", "");
    if (cursor_epochs.empty()) {
      cursor_epochs =
          target::lattice_clause_field_value(clause.fields, "MIN_COVERAGE", "");
    }
    if (!cursor_epochs.empty()) {
      requirements.push_back(use + " >= " + cursor_epochs +
                             " cursor_epoch over " + split);
    }
  }
  if (!requirements.empty()) {
    text << " It requires ";
    for (std::size_t i = 0; i < requirements.size(); ++i) {
      if (i != 0) {
        text << ", ";
      }
      text << requirements[i];
    }
    text << ".";
  }
  if (!spec.forbid_split.empty()) {
    if (spec.forbid_exposure_uses_from_split_policy) {
      text << " It applies split protection for " << spec.forbid_split << ".";
    } else {
      text << " It forbids configured exposure overlap with split "
           << spec.forbid_split << ".";
    }
  }
  if (!found->plans.empty()) {
    text << " If unsatisfied, it may suggest a wave.";
  }
  if (!spec.plan_input_mdn_checkpoint.empty() ||
      !spec.plan_input_representation_checkpoint.empty()) {
    text << " The suggested wave carries model-state inputs";
    if (!spec.plan_input_mdn_checkpoint.empty()) {
      text << " INPUT_MDN_CHECKPOINT=" << spec.plan_input_mdn_checkpoint;
    }
    if (!spec.plan_input_representation_checkpoint.empty()) {
      text << " INPUT_REPRESENTATION_CHECKPOINT="
           << spec.plan_input_representation_checkpoint;
    }
    text << ".";
  }
  if (!found->warning_clauses.empty()) {
    text << " It also evaluates non-blocking warning clauses.";
  }

  std::ostringstream json;
  json << "{\"config_path\":" << json_quote(config_path.string())
       << ",\"targets_path\":" << json_quote(paths.targets_dsl_path.string())
       << ",\"splits_path\":" << json_quote(paths.splits_dsl_path.string())
       << ",\"target\":" << compiled_target_json(*found)
       << ",\"text\":" << json_quote(text.str()) << ",\"over\":{"
       << "\"source_range\":" << json_quote(spec.source_range)
       << ",\"split\":" << json_quote(spec.train_split)
       << ",\"anchor_index_begin\":";
  if (spec.anchor_index_begin.has_value()) {
    json << *spec.anchor_index_begin;
  } else {
    json << "null";
  }
  json << ",\"anchor_index_end\":";
  if (spec.anchor_index_end.has_value()) {
    json << *spec.anchor_index_end;
  } else {
    json << "null";
  }
  json << ",\"resolved_split_anchor_range\":";
  if (split_policy.has_value() && !spec.train_split.empty()) {
    if (const auto *split = split_policy->find_split(spec.train_split)) {
      json << interval_json(split->anchor_range);
    } else {
      json << "null";
    }
  } else {
    json << "null";
  }
  json << "}}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool evaluate_target_common(const std::string &args,
                                          lattice_context_t *ctx,
                                          bool plan_only, std::string *out,
                                          std::string *err) {
  std::string target_id;
  if (!extract_json_string_field(args, "target_id", &target_id) ||
      trim_ascii(target_id).empty()) {
    if (err) {
      *err = "target_id is required";
    }
    return false;
  }
  std::vector<target::lattice_target_spec_t> targets;
  exposure::exposure_ledger_scan_result_t scan;
  target::lattice_target_active_identity_t identity;
  fs::path config_path;
  fs::path runtime_root;
  if (!build_target_evaluator(args, ctx, &targets, &scan, &identity,
                              &config_path, &runtime_root, err)) {
    return false;
  }
  target::lattice_target_evaluator_options_t options{};
  options.runtime_root = runtime_root;
  options.active_identity = identity;
  try {
    options.split_policy =
        target::load_lattice_split_policy_from_config_if_available(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  if (policy_bool_or(ctx->policy, "auto_build_exposure_ledger", true)) {
    options.exposure_ledger = &scan.ledger;
  }
  options.auto_build_exposure_ledger =
      policy_bool_or(ctx->policy, "auto_build_exposure_ledger", true);
  target::lattice_target_evaluator_t evaluator(targets, options);
  target::lattice_target_evaluation_t eval;
  try {
    eval = evaluator.evaluate(target_id);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  std::ostringstream json;
  json << "{\"config_path\":" << json_quote(config_path.string())
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"active_identity\":" << active_identity_json(identity)
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"warnings\":" << string_array_json(scan.warnings);
  if (plan_only) {
    json << ",\"target_id\":" << json_quote(eval.target_id) << ",\"status\":"
         << json_quote(target::lattice_target_status_name(eval.status))
         << ",\"split_policy_fingerprint\":"
         << json_quote(eval.split_policy_fingerprint)
         << ",\"plan_ready\":" << bool_json(eval.plan_ready)
         << ",\"reasons\":" << string_array_json(eval.reasons)
         << ",\"exposure_summaries\":"
         << exposure_load_summaries_json(eval.exposure_summaries)
         << ",\"target_warnings\":" << string_array_json(eval.warnings)
         << ",\"warning_results\":"
         << warning_results_json(eval.warning_results)
         << ",\"suggested_wave\":" << suggested_wave_json(eval.suggested_wave);
  } else {
    json << ",\"evaluation\":" << evaluation_json(eval);
  }
  json << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_evaluate_target(const std::string &args,
                                          lattice_context_t *ctx,
                                          std::string *out, std::string *err) {
  return evaluate_target_common(args, ctx, false, out, err);
}

[[nodiscard]] bool handle_plan_target(const std::string &args,
                                      lattice_context_t *ctx, std::string *out,
                                      std::string *err) {
  return evaluate_target_common(args, ctx, true, out, err);
}

[[nodiscard]] bool handle_scan_exposure(const std::string &args,
                                        lattice_context_t *ctx,
                                        std::string *out, std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  int limit = policy_int_or(ctx->policy, "max_fact_preview", 64);
  if (!parse_optional_int_arg(args, "limit", limit, &limit, err)) {
    return false;
  }
  if (limit < 1) {
    if (err) {
      *err = "limit must be >= 1";
    }
    return false;
  }
  const auto scan =
      exposure::scan_exposure_ledger_from_runtime_root(runtime_root);
  std::ostringstream json;
  json << "{\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"returned_fact_count\":"
       << std::min<std::size_t>(scan.ledger.facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_node_exposure_fact_count\":"
       << std::min<std::size_t>(scan.ledger.node_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_checkpoint_fact_count\":"
       << std::min<std::size_t>(scan.ledger.checkpoint_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"warnings\":" << string_array_json(scan.warnings) << ",\"facts\":"
       << facts_json(scan.ledger.facts(), static_cast<std::size_t>(limit))
       << ",\"node_exposure_facts\":"
       << node_facts_json(scan.ledger.node_facts(),
                          static_cast<std::size_t>(limit))
       << ",\"checkpoint_facts\":"
       << checkpoint_facts_json(scan.ledger.checkpoint_facts(),
                                static_cast<std::size_t>(limit))
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_checkpoint_closure(const std::string &args,
                                             lattice_context_t *ctx,
                                             std::string *out,
                                             std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  std::string checkpoint_arg;
  if (!extract_json_string_field(args, "checkpoint_path", &checkpoint_arg) ||
      trim_ascii(checkpoint_arg).empty()) {
    if (err) {
      *err = "checkpoint_path is required";
    }
    return false;
  }
  fs::path checkpoint_path = fs::path(checkpoint_arg);
  if (checkpoint_path.is_relative()) {
    checkpoint_path = runtime_root / checkpoint_path;
  }
  checkpoint_path = normalize_path(checkpoint_path);
  if (!runtime_root_allowed(ctx->policy, checkpoint_path)) {
    if (err) {
      *err = "E_LATTICE_CHECKPOINT_DENIED: checkpoint_path is outside allowed "
             "runtime roots: " +
             checkpoint_path.string();
    }
    return false;
  }
  int limit = policy_int_or(ctx->policy, "max_closure_facts", 256);
  if (!parse_optional_int_arg(args, "limit", limit, &limit, err)) {
    return false;
  }
  if (limit < 1) {
    if (err) {
      *err = "limit must be >= 1";
    }
    return false;
  }
  const auto scan =
      exposure::scan_exposure_ledger_from_runtime_root(runtime_root);
  const auto closure = scan.ledger.checkpoint_closure_result(checkpoint_path);
  std::ostringstream json;
  json << "{\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
       << ",\"complete\":" << bool_json(closure.complete())
       << ",\"fact_count\":" << closure.facts.size()
       << ",\"returned_fact_count\":"
       << std::min<std::size_t>(closure.facts.size(),
                                static_cast<std::size_t>(limit))
       << ",\"unresolved_input_checkpoints\":"
       << path_array_json(closure.unresolved_input_checkpoints)
       << ",\"warnings\":" << string_array_json(scan.warnings) << ",\"facts\":"
       << facts_json(closure.facts, static_cast<std::size_t>(limit)) << "}";
  *out = json.str();
  return true;
}

using handler_fn = bool (*)(const std::string &, lattice_context_t *,
                            std::string *, std::string *);

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.lattice.status") {
    return handle_status;
  }
  if (name == "hero.lattice.schema") {
    return handle_schema;
  }
  if (name == "hero.lattice.list_targets") {
    return handle_list_targets;
  }
  if (name == "hero.lattice.explain_target") {
    return handle_explain_target;
  }
  if (name == "hero.lattice.evaluate_target") {
    return handle_evaluate_target;
  }
  if (name == "hero.lattice.plan_target") {
    return handle_plan_target;
  }
  if (name == "hero.lattice.scan_exposure") {
    return handle_scan_exposure;
  }
  if (name == "hero.lattice.checkpoint_closure") {
    return handle_checkpoint_closure;
  }
  return std::nullopt;
}

} // namespace

std::filesystem::path
resolve_lattice_hero_dsl_path(const std::filesystem::path &global_config_path) {
  if (const auto fresh =
          read_ini_value(global_config_path, "HERO", "lattice_hero_dsl_path")) {
    return resolve_against(global_config_path, *fresh);
  }
  return kDefaultLatticePolicyPath;
}

bool load_lattice_policy(const std::filesystem::path &policy_path,
                         const std::filesystem::path &global_config,
                         lattice_policy_t *out, std::string *error) {
  if (!out) {
    if (error) {
      *error = "lattice policy output pointer is null";
    }
    return false;
  }
  lattice_policy_t policy{};
  policy.policy_path = policy_path;
  policy.global_config_path = global_config;
  for (const auto &descriptor : kLatticePolicyDescriptors) {
    policy.values[std::string(descriptor.key)] =
        std::string(descriptor.default_value);
  }
  std::string text;
  if (!read_text_file(policy_path, &text, error)) {
    policy.from_template = true;
    text = std::string(kLatticePolicyTemplateText);
  }
  for (auto &[key, value] : parse_assignment_text(text)) {
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
                       lattice_context_t *ctx,
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
      *out_error_message = "Lattice Hero context pointer is null";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("Lattice Hero context pointer is null", -32603);
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
      const int code = err.find("E_LATTICE_") == 0 ? kPolicyErrorCode : -32602;
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
  constexpr std::size_t kToolCount = sizeof(kTools) / sizeof(kTools[0]);
  for (std::size_t i = 0; i < kToolCount; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << json_quote(kTools[i].name)
        << ",\"description\":" << json_quote(kTools[i].description)
        << ",\"inputSchema\":" << kTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << bool_json(tool_is_read_only(kTools[i].name))
        << ",\"destructiveHint\":false,\"openWorldHint\":false}}";
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

void run_jsonrpc_stdio_loop(lattice_context_t *ctx) {
  std::string line;
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
                   "\"name\":\"hero_lattice\",\"version\":\"0\"}}}"
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
  }
}

} // namespace cuwacunu::hero::lattice
