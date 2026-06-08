#include "hero/lattice_hero/hero_lattice_tools.h"

#include "hero/lattice_hero/hero_lattice.h"
#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/lattice_hero/lattice/target/lattice_target_evaluator.h"
#include "hero/mcp_schema_compat.h"
#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/short_ref.h"
#include "wikimyei/assembly.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/assembly.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
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
namespace display = cuwacunu::hero::display;
namespace exposure = cuwacunu::hero::lattice::exposure;
namespace target = cuwacunu::hero::lattice::target;

[[nodiscard]] std::string fact_integrity_summary_json(
    const exposure::lattice_fact_integrity_summary_t &summary);

constexpr int kPolicyErrorCode = -32060;
constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultLatticePolicyPath =
    "/cuwacunu/src/config/hero.lattice.dsl";
constexpr const char *kDerivedQueryResultsSchema =
    "kikijyeba.lattice.derived_query_results.v1";
constexpr const char *kDerivedQueryRuleVocabularyDigestSchema =
    "kikijyeba.lattice.derived_query_rule_vocabulary.v1";
constexpr const char *kDerivedQueryResultProjectionDigestSchema =
    "kikijyeba.lattice.derived_query_results_projection.v1";

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

struct runtime_scan_session_cache_t {
  bool available{false};
  fs::path runtime_root{};
  std::string watched_file_metadata_digest{};
  exposure::exposure_ledger_scan_result_t scan{};
};

runtime_scan_session_cache_t g_runtime_scan_session_cache{};

constexpr tool_descriptor_t kTools[] = {
    {"hero.lattice.status",
     "Summarize Lattice Hero policy, target DSL, runtime root, "
     "exposure "
     "facts, "
     "and inferred active proof_context.",
     R"({"type":"object","properties":{"config_path":{"type":"string"},"runtime_root":{"type":"string"}},"additionalProperties":false})"},
    {"hero.lattice.inspect",
     "Read-only Lattice inspection. subject=schema reads policy keys; "
     "subject=targets lists configured targets; subject=target explains "
     "one target; subject=exposure scans runtime exposure; "
     "subject=fact_families lists fact families; subject=facts with "
     "mode=scan|summary|lineage|preview inspects fact evidence; "
     "subject=index with mode=status|query inspects the audit index; "
     "subject=derived answers one derived query; subject=checkpoint "
     "inspects checkpoint closure.",
     R"({"type":"object","properties":{"subject":{"type":"string","enum":["schema","targets","target","exposure","fact_families","facts","index","derived","checkpoint"]},"mode":{"type":"string","enum":["scan","summary","lineage","preview","status","query","explain","closure"]},"target_id":{"type":"string"},"target_ids":{"type":"array","items":{"type":"string"}},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"family":{"type":"string"},"limit":{"type":"integer"},"include_facts":{"type":"boolean"},"digest":{"type":"string"},"fact_digest":{"type":"string"},"digest_prefix":{"type":"string"},"fact_digest_prefix":{"type":"string"},"fact_index":{"type":"integer"},"index":{"type":"integer"},"index_path":{"type":"string"},"validation_strength":{"type":"string"},"relation":{"type":"string"},"key":{"type":"string"},"key_contains":{"type":"string"},"compare_live_scan":{"type":"boolean"},"allow_unproven_cache":{"type":"boolean"},"checkpoint_path":{"type":"string"},"checkpoint_id":{"type":"string"},"checkpoint_file_digest":{"type":"string"},"ancestor_checkpoint_path":{"type":"string"},"ancestor_checkpoint_id":{"type":"string"},"protocol_id":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mtf_jepa_mae_vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"}},"required":["subject"],"additionalProperties":false})"},
    {"hero.lattice.evaluate",
     "Read-only Lattice target evaluation. operation=target evaluates one "
     "target; operation=targets evaluates several targets with one scan; "
     "operation=deficit returns proof deficits and target-authored advice; "
     "operation=latest_satisfying_checkpoint resolves one checkpoint "
     "candidate using Lattice proof semantics.",
     R"({"type":"object","properties":{"operation":{"type":"string","enum":["target","targets","deficit","latest_satisfying_checkpoint"]},"target_id":{"type":"string"},"target_ids":{"type":"array","items":{"type":"string"}},"symbolic_hint":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"protocol_id":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mtf_jepa_mae_vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"},"limit":{"type":"integer"}},"required":["operation"],"additionalProperties":false})"},
    {"hero.lattice.compare",
     "Read-only Pareto comparison for two clean satisfying target evidence "
     "vectors. Reports explicit dimensions and never emits a scalar score, "
     "winner, deployment decision, or model selection.",
     R"({"type":"object","properties":{"left_target_id":{"type":"string"},"right_target_id":{"type":"string"},"config_path":{"type":"string"},"runtime_root":{"type":"string"},"protocol_id":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"vicreg_assembly_fingerprint":{"type":"string"},"mtf_jepa_mae_vicreg_assembly_fingerprint":{"type":"string"},"mdn_assembly_fingerprint":{"type":"string"}},"required":["left_target_id","right_target_id"],"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.lattice.status" || name == "hero.lattice.inspect" ||
         name == "hero.lattice.evaluate" || name == "hero.lattice.compare";
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

[[nodiscard]] std::string
map_get_or(const std::unordered_map<std::string, std::string> &map,
           std::string_view key, std::string fallback) {
  const auto value = map_get(map, key);
  return trim_ascii(value).empty() ? fallback : value;
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

[[nodiscard]] bool parse_optional_bool_arg(const std::string &json,
                                           std::string_view key, bool fallback,
                                           bool *out, std::string *err) {
  bool value = fallback;
  std::string raw;
  if (extract_json_raw_field(json, key, &raw) && !parse_bool(raw, &value)) {
    if (err) {
      *err = "invalid boolean argument: " + std::string(key);
    }
    return false;
  }
  if (out) {
    *out = value;
  }
  return true;
}

[[nodiscard]] bool parse_optional_string_arg(const std::string &json,
                                             std::string_view key,
                                             std::string fallback,
                                             std::string *out,
                                             std::string *err) {
  std::string value = std::move(fallback);
  if (extract_json_raw_field(json, key, nullptr) &&
      !extract_json_string_field(json, key, &value)) {
    if (err) {
      *err = "invalid string argument: " + std::string(key);
    }
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

[[nodiscard]] bool
parse_json_string_array_token(const std::string &raw,
                              std::vector<std::string> *out) {
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  if (idx >= raw.size() || raw[idx] != '[') {
    return false;
  }
  ++idx;
  std::vector<std::string> values;
  while (idx < raw.size()) {
    skip_ws(raw, &idx);
    if (idx < raw.size() && raw[idx] == ']') {
      ++idx;
      skip_ws(raw, &idx);
      if (idx != raw.size()) {
        return false;
      }
      if (out) {
        *out = std::move(values);
      }
      return true;
    }
    std::string value;
    if (!parse_json_string_token(raw, &idx, &value)) {
      return false;
    }
    values.push_back(std::move(value));
    skip_ws(raw, &idx);
    if (idx < raw.size() && raw[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < raw.size() && raw[idx] == ']') {
      continue;
    }
    return false;
  }
  return false;
}

[[nodiscard]] bool
parse_optional_string_array_arg(const std::string &json, std::string_view key,
                                std::vector<std::string> fallback,
                                std::vector<std::string> *out,
                                std::string *err) {
  std::vector<std::string> value = std::move(fallback);
  std::string raw;
  if (extract_json_raw_field(json, key, &raw) &&
      !parse_json_string_array_token(raw, &value)) {
    if (err) {
      *err = "invalid string array argument: " + std::string(key);
    }
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

[[nodiscard]] bool parse_runtime_index_validation_strength_arg(
    const std::string &json,
    exposure::lattice_runtime_index_validation_strength_t fallback,
    exposure::lattice_runtime_index_validation_strength_t *out,
    std::string *err) {
  std::string raw;
  if (!parse_optional_string_arg(
          json, "validation_strength",
          exposure::runtime_index_validation_strength_name(fallback), &raw,
          err)) {
    return false;
  }
  raw = lowercase_ascii(trim_ascii(raw));
  if (raw.empty() || raw == "metadata_digest" ||
      raw == "full_runtime_metadata_digest" || raw == "full") {
    if (out) {
      *out = exposure::lattice_runtime_index_validation_strength_t::
          full_runtime_metadata_digest;
    }
    return true;
  }
  if (raw == "watched_file_manifest" || raw == "watched" || raw == "manifest") {
    if (out) {
      *out = exposure::lattice_runtime_index_validation_strength_t::
          watched_file_manifest;
    }
    return true;
  }
  if (raw == "header_only" || raw == "header") {
    if (out) {
      *out = exposure::lattice_runtime_index_validation_strength_t::header_only;
    }
    return true;
  }
  if (err) {
    *err = "invalid validation_strength: " + raw;
  }
  return false;
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
interval_array_json(const std::vector<exposure::anchor_interval_t> &intervals) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < intervals.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << interval_json(intervals[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string optional_interval_json(
    const std::optional<exposure::anchor_interval_t> &interval) {
  if (!interval.has_value()) {
    return "null";
  }
  return interval_json(*interval);
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

[[nodiscard]] std::string fact_identity_envelope_json(
    const exposure::lattice_fact_identity_envelope_t &envelope) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(envelope.schema)
      << ",\"schema_id\":" << json_quote(envelope.schema_id)
      << ",\"schema_version\":" << json_quote(envelope.schema_version)
      << ",\"fact_identity_contract_schema\":"
      << json_quote(envelope.fact_identity_contract_schema)
      << ",\"fact_identity_contract_id\":"
      << json_quote(envelope.fact_identity_contract_id)
      << ",\"fact_family\":" << json_quote(envelope.fact_family)
      << ",\"fact_type\":" << json_quote(envelope.fact_type)
      << ",\"fact_id\":" << json_quote(envelope.fact_id)
      << ",\"fact_digest\":" << json_quote(envelope.fact_digest)
      << ",\"protocol_id\":" << json_quote(envelope.protocol_id)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(envelope.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(envelope.graph_order_fingerprint)
      << ",\"cursor_domain\":" << json_quote(envelope.cursor_domain)
      << ",\"source_cursor_token\":" << json_quote(envelope.source_cursor_token)
      << ",\"split_name\":" << json_quote(envelope.split_name)
      << ",\"split_role\":" << json_quote(envelope.split_role)
      << ",\"split_policy_fingerprint\":"
      << json_quote(envelope.split_policy_fingerprint)
      << ",\"anchor_range\":" << optional_interval_json(envelope.anchor_range)
      << ",\"completed_anchor_range\":"
      << optional_interval_json(envelope.completed_anchor_range)
      << ",\"component_family_id\":" << json_quote(envelope.component_family_id)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(envelope.component_assembly_fingerprint)
      << ",\"job_id\":" << json_quote(envelope.job_id)
      << ",\"wave_id\":" << json_quote(envelope.wave_id)
      << ",\"support_count\":";
  if (envelope.support_count.has_value()) {
    out << *envelope.support_count;
  } else {
    out << "null";
  }
  out << ",\"valid_count\":";
  if (envelope.valid_count.has_value()) {
    out << *envelope.valid_count;
  } else {
    out << "null";
  }
  out << ",\"missing_count\":";
  if (envelope.missing_count.has_value()) {
    out << *envelope.missing_count;
  } else {
    out << "null";
  }
  out << ",\"units\":" << json_quote(envelope.units)
      << ",\"mask_policy_digest\":" << json_quote(envelope.mask_policy_digest)
      << ",\"parent_exposure_fact_digests\":"
      << string_array_json(envelope.parent_exposure_fact_digests)
      << ",\"parent_checkpoint_digests\":"
      << string_array_json(envelope.parent_checkpoint_digests)
      << ",\"parent_forecast_artifact_digests\":"
      << string_array_json(envelope.parent_forecast_artifact_digests)
      << ",\"parent_fact_digests\":"
      << string_array_json(envelope.parent_fact_digests)
      << ",\"row_index_interval_authority\":"
      << bool_json(envelope.row_index_interval_authority)
      << ",\"source_key_window_audit_only\":"
      << bool_json(envelope.source_key_window_audit_only)
      << ",\"read_only\":" << bool_json(envelope.read_only)
      << ",\"target_proof\":" << bool_json(envelope.target_proof)
      << ",\"dispatchable\":" << bool_json(envelope.dispatchable)
      << ",\"runtime_executor\":" << bool_json(envelope.runtime_executor)
      << ",\"fact_families_are_not_target_kinds\":"
      << bool_json(envelope.fact_families_are_not_target_kinds)
      << ",\"facts_used_for_target_satisfaction\":"
      << bool_json(envelope.facts_used_for_target_satisfaction)
      << ",\"checkpoint_selected\":" << bool_json(envelope.checkpoint_selected)
      << ",\"model_selector\":" << bool_json(envelope.model_selector) << "}";
  return out.str();
}

[[nodiscard]] std::string double_array_json(const std::vector<double> &xs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << double_json(xs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string i64_array_json(const std::vector<std::int64_t> &xs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << xs[i];
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
        << ",\"below\":" << double_json(warning.below)
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

[[nodiscard]] std::string representation_geometry_requirement_specs_json(
    const std::vector<
        target::lattice_representation_geometry_requirement_spec_t>
        &requirements) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < requirements.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    const auto &requirement = requirements[i];
    out << "{\"requirement_id\":" << json_quote(requirement.requirement_id)
        << ",\"metric\":" << json_quote(requirement.metric)
        << ",\"op\":" << json_quote(requirement.op)
        << ",\"value\":" << double_json(requirement.value) << ",\"effect\":"
        << json_quote(requirement.require_mutated_component
                          ? "mutated_component"
                          : "any")
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_scope_previews_json(
    const target::lattice_target_spec_t &spec,
    const std::optional<cuwacunu::hero::lattice::split::lattice_split_policy_t>
        &split_policy) {
  namespace detail = target::lattice_target_eval_detail;

  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < spec.warning_specs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    const auto &warning = spec.warning_specs[i];
    auto effective_spec = spec;
    auto effective_warning = warning;
    std::string scope_source = "all_matching_evidence";
    std::string resolution_status = "unbounded_all_matching_evidence";
    std::string resolution_error{};

    if (warning.anchor_index_begin.has_value() &&
        warning.anchor_index_end.has_value()) {
      scope_source = "explicit_anchor_range";
      resolution_status = "resolved";
    } else if (!warning.split.empty()) {
      scope_source = "warning_split";
      if (!split_policy.has_value()) {
        resolution_status = "unresolved_no_split_policy";
        resolution_error = "warning split requires split policy";
      } else if (const auto *split = split_policy->find_split(warning.split)) {
        effective_warning.anchor_index_begin =
            static_cast<std::size_t>(split->anchor_range.begin);
        effective_warning.anchor_index_end =
            static_cast<std::size_t>(split->anchor_range.end);
        resolution_status = "resolved";
      } else {
        resolution_status = "unresolved_unknown_split";
        resolution_error = "unknown warning split";
      }
    } else if (spec.anchor_index_begin.has_value() &&
               spec.anchor_index_end.has_value()) {
      scope_source = "target_anchor_range";
      resolution_status = "resolved";
    } else if (!spec.train_split.empty()) {
      scope_source = "target_split";
      if (!split_policy.has_value()) {
        resolution_status = "unresolved_no_split_policy";
        resolution_error = "target split requires split policy";
      } else if (const auto *split =
                     split_policy->find_split(spec.train_split)) {
        effective_spec.anchor_index_begin =
            static_cast<std::size_t>(split->anchor_range.begin);
        effective_spec.anchor_index_end =
            static_cast<std::size_t>(split->anchor_range.end);
        resolution_status = "resolved";
      } else {
        resolution_status = "unresolved_unknown_split";
        resolution_error = "unknown target split";
      }
    }

    const auto range =
        resolution_status == "resolved"
            ? detail::warning_anchor_interval(effective_spec, effective_warning)
            : exposure::anchor_interval_t{};
    out << "{\"warning_id\":" << json_quote(warning.warning_id)
        << ",\"kind\":" << json_quote(warning.kind)
        << ",\"split\":" << json_quote(warning.split)
        << ",\"scope_source\":" << json_quote(scope_source)
        << ",\"resolution_status\":" << json_quote(resolution_status)
        << ",\"resolution_error\":" << json_quote(resolution_error)
        << ",\"resolved_warning_anchor_range\":" << interval_json(range) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string artifact_readiness_proof_template_json(
    const target::lattice_artifact_readiness_proof_template_t &proof_template) {
  std::ostringstream out;
  out << "{\"subject_fact_family\":"
      << json_quote(proof_template.subject_fact_family)
      << ",\"proof_kind\":" << json_quote(proof_template.proof_kind)
      << ",\"proof_claim\":" << json_quote(proof_template.proof_claim)
      << ",\"target_class\":\"artifact_readiness\""
      << ",\"target_kind_required\":false"
      << ",\"runtime_dispatch_authority\":false"
      << ",\"checkpoint_selection_authority\":false"
      << ",\"quality_authority\":false"
      << ",\"performance_authority\":false"
      << ",\"allocation_authority\":false"
      << ",\"market_readiness_authority\":false"
      << ",\"deployment_authority\":false"
      << ",\"policy_gate_authority\":false"
      << ",\"target_dependency_authority\":false"
      << ",\"runtime_wave_authority\":false"
      << ",\"marshal_reachability\":false"
      << ",\"checkpoint_source_authority\":false"
      << ",\"plan_checkpoint_input_authority\":false}";
  return out.str();
}

[[nodiscard]] std::string artifact_readiness_proof_template_registry_json() {
  const auto &proof_templates =
      target::lattice_artifact_readiness_proof_templates();
  std::vector<std::string> proofable_fact_families;
  std::vector<std::string> proof_kinds;
  proofable_fact_families.reserve(proof_templates.size());
  proof_kinds.reserve(proof_templates.size());
  std::ostringstream templates_json;
  templates_json << "[";
  for (std::size_t i = 0; i < proof_templates.size(); ++i) {
    if (i != 0) {
      templates_json << ",";
    }
    proofable_fact_families.push_back(proof_templates[i].subject_fact_family);
    proof_kinds.push_back(proof_templates[i].proof_kind);
    templates_json << artifact_readiness_proof_template_json(
        proof_templates[i]);
  }
  templates_json << "]";

  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.lattice.artifact_readiness_proof_template_"
         "registry.v1\""
      << ",\"target_class\":\"artifact_readiness\""
      << ",\"proof_template_count\":" << proof_templates.size()
      << ",\"proofable_fact_families\":"
      << string_array_json(proofable_fact_families)
      << ",\"proof_kinds\":" << string_array_json(proof_kinds)
      << ",\"warning_summary_only_fact_families\":[\"source_analytics\"]"
      << ",\"requires_explicit_template\":true"
      << ",\"new_fact_family_default\":\"warning_or_summary_only\""
      << ",\"target_kind_expansion_required\":false"
      << ",\"runtime_dispatch_authority\":false"
      << ",\"checkpoint_selection_authority\":false"
      << ",\"quality_authority\":false"
      << ",\"performance_authority\":false"
      << ",\"allocation_authority\":false"
      << ",\"market_readiness_authority\":false"
      << ",\"deployment_authority\":false"
      << ",\"policy_gate_authority\":false"
      << ",\"target_dependency_authority\":false"
      << ",\"runtime_wave_authority\":false"
      << ",\"marshal_reachability\":false"
      << ",\"checkpoint_source_authority\":false"
      << ",\"plan_checkpoint_input_authority\":false"
      << ",\"proof_templates\":" << templates_json.str() << "}";
  return out.str();
}

[[nodiscard]] std::string artifact_readiness_target_proof_template_json(
    const target::lattice_target_spec_t &spec) {
  if (!target::is_artifact_readiness_target_class(spec.target_class)) {
    return "null";
  }
  const auto *proof_template =
      target::artifact_readiness_proof_template_for_subject_fact_family(
          spec.subject_fact_family);
  if (proof_template == nullptr ||
      proof_template->proof_kind != spec.proof_kind) {
    return "null";
  }
  return artifact_readiness_proof_template_json(*proof_template);
}

[[nodiscard]] std::string
artifact_readiness_target_promotion_blocked_reason(const std::string &family) {
  if (target::artifact_readiness_proof_template_for_subject_fact_family(
          family) != nullptr) {
    return {};
  }
  if (family == "source_analytics") {
    return "warning_summary_only";
  }
  if (family == "selection_signal") {
    return "leakage_visibility_only";
  }
  if (family == "representation_support") {
    return "support_visibility_only";
  }
  if (family == "source_receipt") {
    return "source_receipt_audit_only";
  }
  if (family == "node_exposure") {
    return "node_support_visibility_only";
  }
  if (family == "checkpoint") {
    return "checkpoint_identity_not_artifact_readiness";
  }
  if (family == "exposure") {
    return "core_exposure_proof_evidence_not_artifact_readiness";
  }
  return "no_artifact_proof_template";
}

[[nodiscard]] std::string
target_spec_json(const target::lattice_target_spec_t &spec) {
  const bool artifact_readiness_target =
      target::is_artifact_readiness_target_class(spec.target_class);
  const bool dispatchable_target = !artifact_readiness_target;
  const bool runtime_wave_dispatchable =
      dispatchable_target && spec.max_waves > 0 &&
      lowercase_ascii(trim_ascii(spec.plan_mode)) != "none";
  std::ostringstream out;
  out << "{\"target_id\":" << json_quote(spec.target_id)
      << ",\"target_spec_fingerprint\":"
      << json_quote(spec.target_spec_fingerprint)
      << ",\"use_profile\":" << json_quote(spec.use_profile)
      << ",\"guard_id\":" << json_quote(spec.guard_id)
      << ",\"target_class\":" << json_quote(spec.target_class) << ",\"kind\":"
      << json_quote(target::lattice_target_effective_kind_name(spec))
      << ",\"target_kind_applicable\":"
      << bool_json(target::lattice_target_kind_applicable(spec))
      << ",\"target_kind_effective\":"
      << json_quote(target::lattice_target_effective_kind_selector(spec))
      << ",\"proof_kind\":" << json_quote(spec.proof_kind)
      << ",\"subject_fact_family\":" << json_quote(spec.subject_fact_family)
      << ",\"artifact_proof_template\":"
      << artifact_readiness_target_proof_template_json(spec)
      << ",\"target_surface_kind\":"
      << json_quote(artifact_readiness_target ? "evidence_catalog_artifact"
                                              : "runtime_readiness")
      << ",\"evidence_catalog_target\":" << bool_json(artifact_readiness_target)
      << ",\"dispatchable_target\":" << bool_json(dispatchable_target)
      << ",\"runtime_wave_dispatchable\":"
      << bool_json(runtime_wave_dispatchable)
      << ",\"recommended_operator_action\":"
      << json_quote(artifact_readiness_target
                        ? "inspect"
                        : (runtime_wave_dispatchable
                               ? "evaluate_target_then_review_suggested_wave"
                               : "inspect_lattice_status"))
      << ",\"definition_dispatch_boundary\":"
      << json_quote(artifact_readiness_target
                        ? "artifact_readiness targets are evidence catalog "
                          "proof obligations; inspect evidence panels rather "
                          "than reaching Runtime waves"
                        : "runtime readiness targets may emit suggested waves "
                          "only after target evaluation reports plan_ready")
      << ",\"protocol_id\":" << json_quote(spec.protocol_id)
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
      << ",\"warning_specs\":" << warning_specs_json(spec.warning_specs)
      << ",\"representation_geometry_requirements\":"
      << representation_geometry_requirement_specs_json(
             spec.representation_geometry_requirements)
      << "}";
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
policy_gate_reservation_json(const target::lattice_policy_gate_spec_t &gate) {
  const bool policy_fingerprint_verified =
      gate.policy_fingerprint == target::lattice_policy_gate_fingerprint(gate);
  const auto missing_required_inputs =
      target::lattice_policy_gate_missing_required_inputs(gate);
  const bool input_contract_complete = missing_required_inputs.empty();
  std::ostringstream out;
  out << "{\"policy_id\":" << json_quote(gate.policy_id)
      << ",\"policy_kind\":" << json_quote(gate.policy_kind)
      << ",\"target_id\":" << json_quote(gate.target_id)
      << ",\"metric\":" << json_quote(gate.metric)
      << ",\"metric_definition\":" << json_quote(gate.metric_definition)
      << ",\"baseline\":" << json_quote(gate.baseline)
      << ",\"baseline_definition\":" << json_quote(gate.baseline_definition)
      << ",\"threshold\":" << double_json(gate.threshold)
      << ",\"uncertainty_policy\":" << json_quote(gate.uncertainty_policy)
      << ",\"uncertainty_model\":" << json_quote(gate.uncertainty_model)
      << ",\"support_minimum\":" << double_json(gate.support_minimum)
      << ",\"selector_split\":" << json_quote(gate.selector_split)
      << ",\"anti_leakage_policy\":" << json_quote(gate.anti_leakage_policy)
      << ",\"tie_policy\":" << json_quote(gate.tie_policy)
      << ",\"negative_tests\":" << json_quote(gate.negative_tests)
      << ",\"calibration_requirements\":"
      << json_quote(gate.calibration_requirements)
      << ",\"holdout_declaration\":" << json_quote(gate.holdout_declaration)
      << ",\"threshold_selection_audit\":"
      << json_quote(gate.threshold_selection_audit)
      << ",\"enabled\":" << bool_json(gate.enabled)
      << ",\"status\":" << json_quote(gate.status)
      << ",\"policy_fingerprint\":" << json_quote(gate.policy_fingerprint)
      << ",\"policy_fingerprint_verified\":"
      << bool_json(policy_fingerprint_verified)
      << ",\"policy_fingerprint_mismatch\":"
      << bool_json(!policy_fingerprint_verified)
      << ",\"policy_input_contract_schema\":"
      << json_quote("kikijyeba.lattice.policy_gate.input_contract.v1")
      << ",\"required_policy_inputs\":"
      << string_array_json(target::lattice_policy_gate_required_input_fields())
      << ",\"missing_policy_inputs\":"
      << string_array_json(missing_required_inputs)
      << ",\"policy_input_contract_complete\":"
      << bool_json(input_contract_complete)
      << ",\"decision_policy_authority\":false"
      << ",\"disabled_reason\":" << json_quote(gate.disabled_reason)
      << ",\"target_status_authority\":false"
      << ",\"target_spec_fingerprint_authority\":false"
      << ",\"proof_certificate_authority\":false"
      << ",\"runtime_execution_authority\":false"
      << ",\"allocation_authority\":false"
      << ",\"market_readiness_authority\":false"
      << ",\"deployment_authority\":false"
      << ",\"fields\":" << clause_fields_json(gate.fields) << "}";
  return out.str();
}

[[nodiscard]] std::string policy_gate_reservations_json(
    const std::vector<target::lattice_policy_gate_spec_t> &gates,
    const std::string &target_id_filter = {}) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto &gate : gates) {
    if (!target_id_filter.empty() && gate.target_id != target_id_filter) {
      continue;
    }
    if (!first) {
      out << ",";
    }
    first = false;
    out << policy_gate_reservation_json(gate);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string policy_gate_reservation_summary_json(
    const std::vector<target::lattice_policy_gate_spec_t> &gates,
    const std::string &target_id_filter = {}) {
  std::int64_t reservation_count = 0;
  std::int64_t enabled_count = 0;
  std::int64_t disabled_count = 0;
  std::int64_t target_match_count = 0;
  std::int64_t policy_fingerprint_verified_count = 0;
  std::int64_t policy_fingerprint_mismatch_count = 0;
  std::int64_t input_contract_complete_count = 0;
  std::int64_t missing_input_contract_count = 0;
  std::set<std::string> policy_kinds;
  for (const auto &gate : gates) {
    const bool target_match =
        target_id_filter.empty() || gate.target_id == target_id_filter;
    if (target_match) {
      ++target_match_count;
    }
    if (!target_match) {
      continue;
    }
    ++reservation_count;
    if (gate.enabled) {
      ++enabled_count;
    } else {
      ++disabled_count;
    }
    if (gate.policy_fingerprint ==
        target::lattice_policy_gate_fingerprint(gate)) {
      ++policy_fingerprint_verified_count;
    } else {
      ++policy_fingerprint_mismatch_count;
    }
    if (target::lattice_policy_gate_input_contract_complete(gate)) {
      ++input_contract_complete_count;
    } else {
      ++missing_input_contract_count;
    }
    if (!gate.policy_kind.empty()) {
      policy_kinds.insert(gate.policy_kind);
    }
  }
  std::vector<std::string> policy_kind_values(policy_kinds.begin(),
                                              policy_kinds.end());
  std::ostringstream out;
  out << "{\"schema\":"
      << json_quote("kikijyeba.lattice.policy_gate_reservation.summary.v1")
      << ",\"reservation_count\":" << reservation_count
      << ",\"target_match_count\":" << target_match_count
      << ",\"enabled_policy_gate_count\":" << enabled_count
      << ",\"disabled_policy_gate_count\":" << disabled_count
      << ",\"policy_fingerprint_verified_count\":"
      << policy_fingerprint_verified_count
      << ",\"policy_fingerprint_mismatch_count\":"
      << policy_fingerprint_mismatch_count
      << ",\"policy_input_contract_complete_count\":"
      << input_contract_complete_count
      << ",\"missing_policy_input_contract_count\":"
      << missing_input_contract_count << ",\"required_policy_inputs\":"
      << string_array_json(target::lattice_policy_gate_required_input_fields())
      << ",\"policy_kind_count\":" << policy_kind_values.size()
      << ",\"policy_kinds\":" << string_array_json(policy_kind_values)
      << ",\"all_reservations_disabled\":" << bool_json(enabled_count == 0)
      << ",\"all_policy_fingerprints_verified\":"
      << bool_json(policy_fingerprint_mismatch_count == 0)
      << ",\"all_policy_input_contracts_complete\":"
      << bool_json(missing_input_contract_count == 0)
      << ",\"target_status_authority_count\":0"
      << ",\"target_spec_fingerprint_authority_count\":0"
      << ",\"proof_certificate_authority_count\":0"
      << ",\"decision_policy_authority_count\":0"
      << ",\"runtime_execution_authority_count\":0"
      << ",\"allocation_authority_count\":0"
      << ",\"market_readiness_authority_count\":0"
      << ",\"deployment_authority_count\":0"
      << ",\"enabled_policy_gates_fail_closed\":true"
      << ",\"status_effect\":"
      << json_quote("disabled_reservation_only_no_target_status_effect") << "}";
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
      << ",\"protocol_id\":" << json_quote(evidence.protocol_id)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(evidence.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(evidence.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(evidence.source_cursor_token)
      << ",\"component_fingerprint\":"
      << json_quote(evidence.component_fingerprint)
      << ",\"config_bundle_id\":" << json_quote(evidence.config_bundle_id)
      << ",\"config_receipt_id\":" << json_quote(evidence.config_receipt_id)
      << ",\"component_spawn_registry_id\":"
      << json_quote(evidence.component_spawn_registry_id)
      << ",\"component_family_id\":" << json_quote(evidence.component_family_id)
      << ",\"component_spawn_fingerprint\":"
      << json_quote(evidence.component_spawn_fingerprint)
      << ",\"component_spawn_id\":" << json_quote(evidence.component_spawn_id)
      << ",\"component_spawn_label\":"
      << json_quote(evidence.component_spawn_label)
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
      << ",\"target_component_family_id\":"
      << json_quote(summary.target_component_family_id)
      << ",\"require_mutated_component\":"
      << bool_json(summary.require_mutated_component)
      << ",\"unique_coverage_algebra\":"
      << json_quote(summary.unique_coverage_algebra)
      << ",\"load_algebra\":" << json_quote(summary.load_algebra)
      << ",\"unique_coverage_unit\":"
      << json_quote(summary.unique_coverage_unit)
      << ",\"load_unit\":" << json_quote(summary.load_unit)
      << ",\"anchor_event_unit\":" << json_quote(summary.anchor_event_unit)
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
      << double_json(summary.valid_target_cursor_epochs)
      << ",\"valid_target_success_count_for_uncertainty\":"
      << summary.valid_target_success_count_for_uncertainty
      << ",\"valid_target_opportunity_count_for_uncertainty\":"
      << summary.valid_target_opportunity_count_for_uncertainty
      << ",\"valid_target_fraction_estimate\":"
      << double_json(summary.valid_target_fraction_estimate)
      << ",\"valid_target_wilson_lower_95\":"
      << double_json(summary.valid_target_wilson_lower_95)
      << ",\"valid_target_wilson_upper_95\":"
      << double_json(summary.valid_target_wilson_upper_95) << "}";
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

[[nodiscard]] std::string exposure_measure_algebra_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      exposure::lattice_exposure_measure_algebra_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"measure\":" << json_quote(vocabulary[i].measure)
        << ",\"algebra\":" << json_quote(vocabulary[i].algebra)
        << ",\"operation\":" << json_quote(vocabulary[i].operation)
        << ",\"unit\":" << json_quote(vocabulary[i].unit)
        << ",\"idempotent\":" << bool_json(vocabulary[i].idempotent)
        << ",\"additive\":" << bool_json(vocabulary[i].additive) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string exposure_measure_algebra_summary_json() {
  const auto summary = exposure::lattice_exposure_measure_algebra_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"measure_count\":" << summary.measure_count
      << ",\"idempotent_measure_count\":" << summary.idempotent_measure_count
      << ",\"additive_measure_count\":" << summary.additive_measure_count
      << ",\"coverage_measure_count\":" << summary.coverage_measure_count
      << ",\"load_measure_count\":" << summary.load_measure_count
      << ",\"dual_idempotent_additive_count\":"
      << summary.dual_idempotent_additive_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"unique_coverage_is_idempotent_union\":"
      << bool_json(summary.unique_coverage_is_idempotent_union)
      << ",\"exposure_load_is_additive_measure\":"
      << bool_json(summary.exposure_load_is_additive_measure)
      << ",\"coverage_not_additive\":"
      << bool_json(summary.coverage_not_additive)
      << ",\"load_not_idempotent\":" << bool_json(summary.load_not_idempotent)
      << ",\"measure_units_declared\":"
      << bool_json(summary.measure_units_declared)
      << ",\"coverage_and_load_units_distinct\":"
      << bool_json(summary.coverage_and_load_units_distinct)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"measure_names\":" << string_array_json(summary.measure_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string source_key_coordinate_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      exposure::lattice_source_key_coordinate_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"coordinate\":" << json_quote(vocabulary[i].coordinate)
        << ",\"authority\":" << json_quote(vocabulary[i].authority)
        << ",\"audit_fields\":" << string_array_json(vocabulary[i].audit_fields)
        << ",\"invariant\":" << json_quote(vocabulary[i].invariant)
        << ",\"failure_effect\":" << json_quote(vocabulary[i].failure_effect)
        << ",\"coverage_authority\":"
        << bool_json(vocabulary[i].coverage_authority)
        << ",\"leakage_authority\":"
        << bool_json(vocabulary[i].leakage_authority)
        << ",\"audit_only\":" << bool_json(vocabulary[i].audit_only) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string source_key_coordinate_policy_summary_json() {
  const auto summary = exposure::lattice_source_key_coordinate_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"coverage_authority_count\":" << summary.coverage_authority_count
      << ",\"leakage_authority_count\":" << summary.leakage_authority_count
      << ",\"audit_only_count\":" << summary.audit_only_count
      << ",\"row_index_coordinate_count\":"
      << summary.row_index_coordinate_count
      << ",\"source_key_coordinate_count\":"
      << summary.source_key_coordinate_count
      << ",\"row_to_source_key_map_count\":"
      << summary.row_to_source_key_map_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"row_index_authority_present\":"
      << bool_json(summary.row_index_authority_present)
      << ",\"source_key_window_audit_present\":"
      << bool_json(summary.source_key_window_audit_present)
      << ",\"order_preserving_map_check_present\":"
      << bool_json(summary.order_preserving_map_check_present)
      << ",\"affine_step_consistency_check_present\":"
      << bool_json(summary.affine_step_consistency_check_present)
      << ",\"gap_and_irregular_key_check_present\":"
      << bool_json(summary.gap_and_irregular_key_check_present)
      << ",\"row_index_is_coverage_and_leakage_authority\":"
      << bool_json(summary.row_index_is_coverage_and_leakage_authority)
      << ",\"source_key_rows_are_audit_only\":"
      << bool_json(summary.source_key_rows_are_audit_only)
      << ",\"audit_rows_have_no_coverage_or_leakage_authority\":"
      << bool_json(summary.audit_rows_have_no_coverage_or_leakage_authority)
      << ",\"order_preserving_fields_declared\":"
      << bool_json(summary.order_preserving_fields_declared)
      << ",\"affine_fields_declared\":"
      << bool_json(summary.affine_fields_declared)
      << ",\"gap_fields_declared\":" << bool_json(summary.gap_fields_declared)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string source_receipt_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = exposure::lattice_source_receipt_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"evidence_field\":" << json_quote(vocabulary[i].evidence_field)
        << ",\"authority\":" << json_quote(vocabulary[i].authority)
        << ",\"receipt_fields\":"
        << string_array_json(vocabulary[i].receipt_fields)
        << ",\"allowed_use\":" << json_quote(vocabulary[i].allowed_use)
        << ",\"failure_effect\":" << json_quote(vocabulary[i].failure_effect)
        << ",\"coverage_authority\":"
        << bool_json(vocabulary[i].coverage_authority)
        << ",\"leakage_authority\":"
        << bool_json(vocabulary[i].leakage_authority)
        << ",\"contract_identity_authority\":"
        << bool_json(vocabulary[i].contract_identity_authority)
        << ",\"audit_only\":" << bool_json(vocabulary[i].audit_only)
        << ",\"structured_fact_available\":"
        << bool_json(vocabulary[i].structured_fact_available) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string source_receipt_policy_summary_json() {
  const auto summary = exposure::lattice_source_receipt_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"coverage_authority_count\":" << summary.coverage_authority_count
      << ",\"leakage_authority_count\":" << summary.leakage_authority_count
      << ",\"contract_identity_authority_count\":"
      << summary.contract_identity_authority_count
      << ",\"audit_only_count\":" << summary.audit_only_count
      << ",\"structured_fact_available_count\":"
      << summary.structured_fact_available_count
      << ",\"future_policy_count\":" << summary.future_policy_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"compact_receipts_audit_only\":"
      << bool_json(summary.compact_receipts_audit_only)
      << ",\"row_index_overlap_authority_present\":"
      << bool_json(summary.row_index_overlap_authority_present)
      << ",\"structured_receipt_available\":"
      << bool_json(summary.structured_receipt_available)
      << ",\"receipt_identity_non_contract\":"
      << bool_json(summary.receipt_identity_non_contract)
      << ",\"only_row_index_has_coverage_leakage_authority\":"
      << bool_json(summary.only_row_index_has_coverage_leakage_authority)
      << ",\"no_contract_identity_authority\":"
      << bool_json(summary.no_contract_identity_authority)
      << ",\"structured_receipts_audit_only\":"
      << bool_json(summary.structured_receipts_audit_only)
      << ",\"audit_rows_have_no_coverage_or_leakage_authority\":"
      << bool_json(summary.audit_rows_have_no_coverage_or_leakage_authority)
      << ",\"compact_receipt_fields_declared\":"
      << bool_json(summary.compact_receipt_fields_declared)
      << ",\"structured_receipt_fields_declared\":"
      << bool_json(summary.structured_receipt_fields_declared)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string valid_target_uncertainty_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      exposure::lattice_valid_target_uncertainty_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"scope\":" << json_quote(vocabulary[i].scope)
        << ",\"method\":" << json_quote(vocabulary[i].method)
        << ",\"estimator\":" << json_quote(vocabulary[i].estimator)
        << ",\"confidence_level\":"
        << double_json(vocabulary[i].confidence_level)
        << ",\"success_count_field\":"
        << json_quote(vocabulary[i].success_count_field)
        << ",\"opportunity_count_field\":"
        << json_quote(vocabulary[i].opportunity_count_field)
        << ",\"fraction_field\":" << json_quote(vocabulary[i].fraction_field)
        << ",\"lower_bound_field\":"
        << json_quote(vocabulary[i].lower_bound_field)
        << ",\"upper_bound_field\":"
        << json_quote(vocabulary[i].upper_bound_field)
        << ",\"no_trials_policy\":"
        << json_quote(vocabulary[i].no_trials_policy) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string valid_target_uncertainty_summary_json() {
  const auto summary = exposure::lattice_valid_target_uncertainty_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"scope_count\":" << summary.scope_count
      << ",\"wilson_method_count\":" << summary.wilson_method_count
      << ",\"binomial_estimator_count\":" << summary.binomial_estimator_count
      << ",\"confidence_95_count\":" << summary.confidence_95_count
      << ",\"no_trials_non_claiming_count\":"
      << summary.no_trials_non_claiming_count
      << ",\"exposure_load_scope_count\":" << summary.exposure_load_scope_count
      << ",\"node_support_scope_count\":" << summary.node_support_scope_count
      << ",\"weakest_node_scope_count\":" << summary.weakest_node_scope_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"exposure_load_support_scope_present\":"
      << bool_json(summary.exposure_load_support_scope_present)
      << ",\"node_aggregate_support_scope_present\":"
      << bool_json(summary.node_aggregate_support_scope_present)
      << ",\"weakest_node_support_scope_present\":"
      << bool_json(summary.weakest_node_support_scope_present)
      << ",\"all_scopes_use_wilson_95\":"
      << bool_json(summary.all_scopes_use_wilson_95)
      << ",\"all_estimators_binomial_fraction\":"
      << bool_json(summary.all_estimators_binomial_fraction)
      << ",\"all_no_trials_non_claiming\":"
      << bool_json(summary.all_no_trials_non_claiming)
      << ",\"all_count_fields_declared\":"
      << bool_json(summary.all_count_fields_declared)
      << ",\"all_fraction_fields_declared\":"
      << bool_json(summary.all_fraction_fields_declared)
      << ",\"all_interval_bounds_declared\":"
      << bool_json(summary.all_interval_bounds_declared)
      << ",\"support_visibility_not_performance_gate\":"
      << bool_json(summary.support_visibility_not_performance_gate)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"scope_names\":" << string_array_json(summary.scope_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
exposure_use_set_json(const exposure::exposure_use_set_t &use) {
  std::ostringstream out;
  out << "{\"observed_input\":" << bool_json(use.observed_input)
      << ",\"target_supervision\":" << bool_json(use.target_supervision)
      << ",\"evaluation_metric\":" << bool_json(use.evaluation_metric)
      << ",\"selection_signal\":" << bool_json(use.selection_signal)
      << ",\"mutated_component\":" << bool_json(use.mutated_component) << "}";
  return out.str();
}

[[nodiscard]] std::string source_key_window_endpoint_json(
    const exposure::source_key_window_audit_t::endpoint_t &endpoint) {
  std::ostringstream out;
  out << "{\"label\":" << json_quote(endpoint.label)
      << ",\"row\":" << endpoint.row << ",\"key\":" << json_quote(endpoint.key)
      << ",\"numeric\":" << bool_json(endpoint.numeric) << ",\"numeric_key\":";
  if (endpoint.numeric) {
    out << endpoint.numeric_key;
  } else {
    out << "null";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string source_key_window_endpoints_json(
    const std::vector<exposure::source_key_window_audit_t::endpoint_t>
        &endpoints) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < endpoints.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << source_key_window_endpoint_json(endpoints[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
source_key_window_audit_json(const exposure::source_key_window_audit_t &audit) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(audit.schema)
      << ",\"available\":" << bool_json(audit.available)
      << ",\"precision\":" << json_quote(audit.precision)
      << ",\"complete\":" << bool_json(audit.complete)
      << ",\"numeric\":" << bool_json(audit.numeric)
      << ",\"internally_monotone\":" << bool_json(audit.internally_monotone)
      << ",\"order_preserving\":" << bool_json(audit.order_preserving)
      << ",\"affine_step_available\":" << bool_json(audit.affine_step_available)
      << ",\"reference_key_step\":" << audit.reference_key_step
      << ",\"affine_consistent\":" << bool_json(audit.affine_consistent)
      << ",\"missing_endpoint_pair_count\":"
      << audit.missing_endpoint_pair_count
      << ",\"irregular_key_warning_count\":"
      << audit.irregular_key_warning_count
      << ",\"source_key_gap_warning_count\":"
      << audit.source_key_gap_warning_count
      << ",\"row_source_key_mismatch_count\":"
      << audit.row_source_key_mismatch_count
      << ",\"source_key_gap_found\":" << bool_json(audit.source_key_gap_found)
      << ",\"row_source_key_mismatch_found\":"
      << bool_json(audit.row_source_key_mismatch_found)
      << ",\"issues\":" << string_array_json(audit.issues)
      << ",\"endpoints\":" << source_key_window_endpoints_json(audit.endpoints)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
node_support_row_json(const exposure::node_support_row_t &row) {
  std::ostringstream out;
  out << "{\"parent_exposure_fact_digest\":"
      << json_quote(row.parent_exposure_fact_digest)
      << ",\"node_id\":" << json_quote(row.node_id)
      << ",\"node_index\":" << row.node_index
      << ",\"use\":" << exposure_use_set_json(row.use)
      << ",\"routed_row_count\":" << row.routed_row_count
      << ",\"active_row_count\":" << row.active_row_count
      << ",\"trained_row_count\":" << row.trained_row_count
      << ",\"evaluated_row_count\":" << row.evaluated_row_count
      << ",\"valid_target_count\":" << row.valid_target_count
      << ",\"valid_target_denominator\":" << row.valid_target_denominator
      << ",\"valid_target_fraction\":" << double_json(row.valid_target_fraction)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
node_support_rows_json(const std::vector<exposure::node_support_row_t> &rows) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << node_support_row_json(rows[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
node_support_summary_json(const exposure::node_support_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"target_component_family_id\":"
      << json_quote(summary.target_component_family_id)
      << ",\"split_name\":" << json_quote(summary.split_name)
      << ",\"use\":" << exposure_use_set_json(summary.use)
      << ",\"support_rows\":" << node_support_rows_json(summary.support_rows)
      << ",\"node_count\":" << summary.node_count
      << ",\"unique_node_count\":" << summary.unique_node_count
      << ",\"observed_input_support_row_count\":"
      << summary.observed_input_support_row_count
      << ",\"target_supervision_support_row_count\":"
      << summary.target_supervision_support_row_count
      << ",\"evaluation_metric_support_row_count\":"
      << summary.evaluation_metric_support_row_count
      << ",\"selection_signal_support_row_count\":"
      << summary.selection_signal_support_row_count
      << ",\"mutating_support_row_count\":"
      << summary.mutating_support_row_count
      << ",\"non_mutating_support_row_count\":"
      << summary.non_mutating_support_row_count
      << ",\"routed_row_count_total\":" << summary.routed_row_count_total
      << ",\"active_row_count_total\":" << summary.active_row_count_total
      << ",\"trained_row_count_total\":" << summary.trained_row_count_total
      << ",\"evaluated_row_count_total\":" << summary.evaluated_row_count_total
      << ",\"valid_target_count_total\":" << summary.valid_target_count_total
      << ",\"valid_target_opportunity_count_total\":"
      << summary.valid_target_opportunity_count_total
      << ",\"valid_target_fraction_estimate\":"
      << double_json(summary.valid_target_fraction_estimate)
      << ",\"valid_target_wilson_lower_95\":"
      << double_json(summary.valid_target_wilson_lower_95)
      << ",\"valid_target_wilson_upper_95\":"
      << double_json(summary.valid_target_wilson_upper_95)
      << ",\"finite_valid_target_fraction_count\":"
      << summary.finite_valid_target_fraction_count
      << ",\"min_valid_target_fraction\":"
      << double_json(summary.min_valid_target_fraction)
      << ",\"max_valid_target_fraction\":"
      << double_json(summary.max_valid_target_fraction)
      << ",\"mean_valid_target_fraction\":"
      << double_json(summary.mean_valid_target_fraction)
      << ",\"weakest_valid_target_node_id\":"
      << json_quote(summary.weakest_valid_target_node_id)
      << ",\"weakest_valid_target_node_index\":"
      << summary.weakest_valid_target_node_index
      << ",\"weakest_valid_target_count\":"
      << summary.weakest_valid_target_count
      << ",\"weakest_valid_target_denominator\":"
      << summary.weakest_valid_target_denominator
      << ",\"weakest_valid_target_fraction\":"
      << double_json(summary.weakest_valid_target_fraction)
      << ",\"weakest_valid_target_wilson_lower_95\":"
      << double_json(summary.weakest_valid_target_wilson_lower_95)
      << ",\"weakest_valid_target_wilson_upper_95\":"
      << double_json(summary.weakest_valid_target_wilson_upper_95)
      << ",\"valid_target_count_mean\":"
      << double_json(summary.valid_target_count_mean)
      << ",\"valid_target_count_coefficient_of_variation\":"
      << double_json(summary.valid_target_count_coefficient_of_variation)
      << ",\"valid_target_count_gini\":"
      << double_json(summary.valid_target_count_gini)
      << ",\"valid_target_count_normalized_entropy\":"
      << double_json(summary.valid_target_count_normalized_entropy) << "}";
  return out.str();
}

[[nodiscard]] std::string node_support_summaries_json(
    const std::vector<exposure::node_support_summary_t> &summaries) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < summaries.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << node_support_summary_json(summaries[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_result_json(
    const target::lattice_target_evaluation_t::warning_result_t &warning) {
  std::ostringstream out;
  out << "{\"warning_id\":" << json_quote(warning.warning_id)
      << ",\"target_id\":" << json_quote(warning.target_id)
      << ",\"kind\":" << json_quote(warning.kind)
      << ",\"warning_family\":" << json_quote(warning.warning_family)
      << ",\"status\":" << json_quote(warning.status)
      << ",\"severity\":" << json_quote(warning.severity)
      << ",\"source\":" << json_quote(warning.source)
      << ",\"component\":" << json_quote(warning.component)
      << ",\"blocking\":" << bool_json(warning.blocking)
      << ",\"threshold_triggered\":" << bool_json(warning.threshold_triggered)
      << ",\"threshold_relation\":" << json_quote(warning.threshold_relation)
      << ",\"measured_value\":" << double_json(warning.measured_value)
      << ",\"threshold\":" << double_json(warning.threshold)
      << ",\"threshold_direction\":" << json_quote(warning.threshold_direction)
      << ",\"unit\":" << json_quote(warning.unit)
      << ",\"split\":" << json_quote(warning.split)
      << ",\"warning_anchor_range\":"
      << interval_json(warning.warning_anchor_range)
      << ",\"use\":" << json_quote(warning.use)
      << ",\"scope\":" << json_quote(warning.scope)
      << ",\"effect\":" << json_quote(warning.effect)
      << ",\"metric\":" << json_quote(warning.metric)
      << ",\"evidence_basis\":" << json_quote(warning.evidence_basis)
      << ",\"evidence_digest\":" << json_quote(warning.evidence_digest)
      << ",\"fact_family\":" << json_quote(warning.fact_family)
      << ",\"fact_digest\":" << json_quote(warning.fact_digest)
      << ",\"target_ids_observed_against\":"
      << string_array_json(warning.target_ids_observed_against)
      << ",\"machine_reason_code\":" << json_quote(warning.machine_reason_code)
      << ",\"human_explanation\":" << json_quote(warning.human_explanation)
      << ",\"suggested_inspection_panel\":"
      << json_quote(warning.suggested_inspection_panel)
      << ",\"readiness_effect\":" << json_quote(warning.readiness_effect)
      << ",\"message\":" << json_quote(warning.message)
      << ",\"measurement_available\":"
      << bool_json(warning.measurement_available)
      << ",\"diagnostic_metric_family\":"
      << json_quote(warning.diagnostic_metric_family)
      << ",\"diagnostic_uncertainty_method\":"
      << json_quote(warning.diagnostic_uncertainty_method)
      << ",\"diagnostic_sample_count\":" << warning.diagnostic_sample_count
      << ",\"diagnostic_evaluated_mdn_checkpoint\":"
      << json_quote(warning.diagnostic_evaluated_mdn_checkpoint.string())
      << ",\"diagnostic_representation_checkpoint\":"
      << json_quote(warning.diagnostic_representation_checkpoint.string())
      << ",\"diagnostic_exact_checkpoint_binding_proven\":"
      << bool_json(warning.diagnostic_exact_checkpoint_binding_proven)
      << ",\"diagnostic_mdn_checkpoint_loaded\":"
      << bool_json(warning.diagnostic_mdn_checkpoint_loaded)
      << ",\"diagnostic_representation_checkpoint_loaded\":"
      << bool_json(warning.diagnostic_representation_checkpoint_loaded)
      << ",\"diagnostic_split_policy_fingerprint\":"
      << json_quote(warning.diagnostic_split_policy_fingerprint)
      << ",\"diagnostic_active_contract_fingerprint\":"
      << json_quote(warning.diagnostic_active_contract_fingerprint)
      << ",\"diagnostic_active_graph_order_fingerprint\":"
      << json_quote(warning.diagnostic_active_graph_order_fingerprint)
      << ",\"diagnostic_active_source_cursor_token\":"
      << json_quote(warning.diagnostic_active_source_cursor_token)
      << ",\"diagnostic_validation_split\":"
      << json_quote(warning.diagnostic_validation_split)
      << ",\"diagnostic_validation_split_bound\":"
      << bool_json(warning.diagnostic_validation_split_bound)
      << ",\"diagnostic_readiness_effect\":"
      << json_quote(warning.diagnostic_readiness_effect)
      << ",\"exposure_summary_available\":"
      << bool_json(warning.exposure_summary_available)
      << ",\"node_support_summary_available\":"
      << bool_json(warning.node_support_summary_available)
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
      << exposure_load_summary_json(warning.exposure_summary)
      << ",\"node_support_summary\":"
      << node_support_summary_json(warning.node_support_summary) << "}";
  return out.str();
}

[[nodiscard]] std::string warning_messages_json(
    const std::vector<target::lattice_target_evaluation_t::warning_result_t>
        &warning_results,
    const std::vector<std::string> &legacy_messages) {
  std::vector<std::string> messages = legacy_messages;
  for (const auto &warning : warning_results) {
    if (!warning.message.empty() &&
        std::find(messages.begin(), messages.end(), warning.message) ==
            messages.end()) {
      messages.push_back(warning.message);
    }
  }
  return string_array_json(messages);
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

[[nodiscard]] std::string warning_results_json(
    const std::vector<target::lattice_target_evaluation_t> &evaluations) {
  std::ostringstream out;
  out << "[";
  bool first = true;
  for (const auto &eval : evaluations) {
    for (const auto &warning : eval.warning_results) {
      if (!first) {
        out << ",";
      }
      first = false;
      out << warning_result_json(warning);
    }
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_messages_json(
    const std::vector<target::lattice_target_evaluation_t> &evaluations) {
  std::vector<std::string> messages;
  for (const auto &eval : evaluations) {
    for (const auto &message : eval.warnings) {
      if (std::find(messages.begin(), messages.end(), message) ==
          messages.end()) {
        messages.push_back(message);
      }
    }
    for (const auto &warning : eval.warning_results) {
      if (!warning.message.empty() &&
          std::find(messages.begin(), messages.end(), warning.message) ==
              messages.end()) {
        messages.push_back(warning.message);
      }
    }
  }
  return string_array_json(messages);
}

[[nodiscard]] std::string warning_summary_json(
    const target::lattice_target_evaluation_t::warning_summary_t &summary) {
  std::ostringstream out;
  out << "{\"warning_result_count\":" << summary.warning_result_count
      << ",\"triggered_warning_count\":" << summary.triggered_warning_count
      << ",\"unavailable_warning_count\":" << summary.unavailable_warning_count
      << ",\"clear_measured_warning_count\":"
      << summary.clear_measured_warning_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"blocking_warning_count\":" << summary.blocking_warning_count
      << ",\"non_blocking_warning_count\":"
      << summary.non_blocking_warning_count << ",\"all_warnings_non_blocking\":"
      << bool_json(summary.all_warnings_non_blocking)
      << ",\"relation_counts\":{\"above_threshold\":"
      << summary.above_threshold_count
      << ",\"not_above_threshold\":" << summary.not_above_threshold_count
      << ",\"below_threshold\":" << summary.below_threshold_count
      << ",\"not_below_threshold\":" << summary.not_below_threshold_count
      << ",\"unavailable\":" << summary.unavailable_relation_count
      << ",\"no_threshold\":" << summary.no_threshold_relation_count
      << ",\"unknown_direction\":" << summary.unknown_direction_relation_count
      << "}}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_result_json(
    const target::lattice_target_evaluation_t::
        representation_geometry_gate_result_t &gate) {
  std::ostringstream out;
  out << "{\"requirement_id\":" << json_quote(gate.requirement_id)
      << ",\"metric\":" << json_quote(gate.metric)
      << ",\"op\":" << json_quote(gate.op)
      << ",\"threshold\":" << double_json(gate.threshold)
      << ",\"measured_value\":" << double_json(gate.measured_value)
      << ",\"unit\":" << json_quote(gate.unit)
      << ",\"fact_count\":" << gate.fact_count
      << ",\"measurement_available\":" << bool_json(gate.measurement_available)
      << ",\"passed\":" << bool_json(gate.passed)
      << ",\"status\":" << json_quote(gate.status)
      << ",\"message\":" << json_quote(gate.message) << "}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_results_json(
    const std::vector<target::lattice_target_evaluation_t::
                          representation_geometry_gate_result_t> &gates) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < gates.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << representation_geometry_gate_result_json(gates[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_threshold_relation_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_warning_threshold_relation_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"relation\":" << json_quote(vocabulary[i].relation)
        << ",\"measurement_available\":"
        << bool_json(vocabulary[i].measurement_available)
        << ",\"triggered\":" << bool_json(vocabulary[i].triggered) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string warning_anchor_scope_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_warning_anchor_scope_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"scope_resolution\":"
        << json_quote(vocabulary[i].scope_resolution)
        << ",\"overlap_basis\":" << json_quote(vocabulary[i].overlap_basis)
        << ",\"readiness_effect\":"
        << json_quote(vocabulary[i].readiness_effect)
        << ",\"identity_effect\":" << json_quote(vocabulary[i].identity_effect)
        << ",\"visibility_only\":" << bool_json(vocabulary[i].visibility_only)
        << ",\"can_satisfy_readiness\":"
        << bool_json(vocabulary[i].can_satisfy_readiness)
        << ",\"contributes_to_certificate_digest\":"
        << bool_json(vocabulary[i].contributes_to_certificate_digest) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string proof_context_json(
    const target::lattice_target_proof_certificate_t::proof_context_t &proof) {
  std::ostringstream out;
  out << "{\"require_contract_match\":"
      << bool_json(proof.require_contract_match)
      << ",\"require_component_match\":"
      << bool_json(proof.require_component_match)
      << ",\"require_graph_anchor_identity\":"
      << bool_json(proof.require_graph_anchor_identity)
      << ",\"active_contract_fingerprint\":"
      << json_quote(proof.active_contract_fingerprint)
      << ",\"evidence_contract_fingerprint\":"
      << json_quote(proof.evidence_contract_fingerprint)
      << ",\"contract_match\":" << bool_json(proof.contract_match)
      << ",\"expected_component_fingerprint\":"
      << json_quote(proof.expected_component_fingerprint)
      << ",\"evidence_component_fingerprint\":"
      << json_quote(proof.evidence_component_fingerprint)
      << ",\"component_match\":" << bool_json(proof.component_match)
      << ",\"active_graph_order_fingerprint\":"
      << json_quote(proof.active_graph_order_fingerprint)
      << ",\"evidence_graph_order_fingerprint\":"
      << json_quote(proof.evidence_graph_order_fingerprint)
      << ",\"graph_order_match\":" << bool_json(proof.graph_order_match)
      << ",\"active_source_cursor_token\":"
      << json_quote(proof.active_source_cursor_token)
      << ",\"evidence_source_cursor_token\":"
      << json_quote(proof.evidence_source_cursor_token)
      << ",\"source_cursor_match\":" << bool_json(proof.source_cursor_match)
      << ",\"component_spawn_schema\":"
      << json_quote(proof.component_spawn_schema)
      << ",\"component_family_id\":" << json_quote(proof.component_family_id)
      << ",\"target_spec_fingerprint\":"
      << json_quote(proof.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << json_quote(proof.split_policy_fingerprint)
      << ",\"config_bundle_id\":" << json_quote(proof.config_bundle_id)
      << ",\"config_receipt_id\":" << json_quote(proof.config_receipt_id)
      << ",\"component_spawn_registry_id\":"
      << json_quote(proof.component_spawn_registry_id)
      << ",\"component_spawn_id\":" << json_quote(proof.component_spawn_id)
      << ",\"component_spawn_label\":"
      << json_quote(proof.component_spawn_label)
      << ",\"evidence_component_spawn_fingerprint\":"
      << json_quote(proof.evidence_component_spawn_fingerprint)
      << ",\"computed_component_spawn_fingerprint\":"
      << json_quote(proof.computed_component_spawn_fingerprint)
      << ",\"config_receipt_available\":"
      << bool_json(proof.config_receipt_available)
      << ",\"spawn_id_readability_hint_only\":"
      << bool_json(proof.spawn_id_readability_hint_only)
      << ",\"provenance_warnings\":"
      << string_array_json(proof.provenance_warnings) << "}";
  return out.str();
}

[[nodiscard]] std::string dependency_proof_json(
    const target::lattice_target_proof_certificate_t::dependency_proof_t
        &proof) {
  std::ostringstream out;
  out << "{\"kind\":" << json_quote(proof.kind)
      << ",\"source_target_id\":" << json_quote(proof.source_target_id)
      << ",\"status\":" << json_quote(proof.status)
      << ",\"expected_mdn_checkpoint\":"
      << json_quote(proof.expected_mdn_checkpoint.string())
      << ",\"actual_mdn_checkpoint\":"
      << json_quote(proof.actual_mdn_checkpoint.string())
      << ",\"mdn_checkpoint_match\":" << bool_json(proof.mdn_checkpoint_match)
      << ",\"expected_representation_checkpoint\":"
      << json_quote(proof.expected_representation_checkpoint.string())
      << ",\"actual_representation_checkpoint\":"
      << json_quote(proof.actual_representation_checkpoint.string())
      << ",\"representation_checkpoint_match\":"
      << bool_json(proof.representation_checkpoint_match) << "}";
  return out.str();
}

[[nodiscard]] std::string dependency_proofs_json(
    const std::vector<
        target::lattice_target_proof_certificate_t::dependency_proof_t>
        &proofs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < proofs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << dependency_proof_json(proofs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string fact_preview_hint_json(const std::string &family,
                                                 const std::string &digest) {
  const bool available = !family.empty() && !digest.empty();
  std::ostringstream out;
  out << "{\"available\":" << bool_json(available)
      << ",\"tool\":\"hero.lattice.inspect\""
      << ",\"subject\":\"facts\""
      << ",\"mode\":\"preview\""
      << ",\"marshal_tool\":\"hero.marshal.inspect\""
      << ",\"fact_family\":" << json_quote(family)
      << ",\"fact_ref\":" << json_quote(display::fact_ref(family, digest))
      << ",\"fact_digest_prefix\":"
      << json_quote(display::digest_prefix(digest))
      << ",\"fact_digest\":" << json_quote(digest) << ",\"lattice_args\":";
  if (available) {
    out << "{\"subject\":\"facts\",\"mode\":\"preview\",\"family\":"
        << json_quote(family) << ",\"fact_digest\":" << json_quote(digest)
        << ",\"limit\":1}";
  } else {
    out << "null";
  }
  out << ",\"marshal_args\":";
  if (available) {
    out << "{\"fact_family\":" << json_quote(family)
        << ",\"fact_digest\":" << json_quote(digest)
        << ",\"include_preview\":true"
        << ",\"include_lineage\":true}";
  } else {
    out << "null";
  }
  out << ",\"read_only\":true"
      << ",\"target_proof\":false"
      << ",\"dispatchable\":false"
      << ",\"facts_used_for_target_satisfaction\":false"
      << ",\"checkpoint_selected\":false"
      << ",\"model_selector\":false}";
  return out.str();
}

[[nodiscard]] std::string artifact_fact_preview_hint_json(
    const target::lattice_target_proof_certificate_t::artifact_proof_t &proof) {
  return fact_preview_hint_json(proof.fact_family, proof.fact_digest);
}

[[nodiscard]] std::string plan_basis_fact_preview_hint_json(
    const target::lattice_target_evaluation_t::plan_basis_t::fact_preview_hint_t
        &hint) {
  std::ostringstream out;
  out << "{\"available\":"
      << bool_json(!hint.fact_family.empty() && !hint.fact_digest.empty())
      << ",\"tool\":" << json_quote(hint.tool) << ",\"subject\":\"facts\""
      << ",\"mode\":\"preview\""
      << ",\"marshal_tool\":" << json_quote(hint.marshal_tool)
      << ",\"fact_family\":" << json_quote(hint.fact_family) << ",\"fact_ref\":"
      << json_quote(display::fact_ref(hint.fact_family, hint.fact_digest))
      << ",\"fact_digest_prefix\":"
      << json_quote(display::digest_prefix(hint.fact_digest))
      << ",\"fact_digest\":" << json_quote(hint.fact_digest)
      << ",\"lattice_args\":{\"subject\":\"facts\",\"mode\":\"preview\","
         "\"family\":"
      << json_quote(hint.fact_family)
      << ",\"fact_digest\":" << json_quote(hint.fact_digest) << ",\"limit\":1}"
      << ",\"marshal_args\":{\"fact_family\":" << json_quote(hint.fact_family)
      << ",\"fact_digest\":" << json_quote(hint.fact_digest)
      << ",\"include_preview\":" << bool_json(hint.include_preview)
      << ",\"include_lineage\":" << bool_json(hint.include_lineage) << "}"
      << ",\"read_only\":" << bool_json(hint.read_only)
      << ",\"target_proof\":" << bool_json(hint.target_proof)
      << ",\"dispatchable\":" << bool_json(hint.dispatchable)
      << ",\"facts_used_for_target_satisfaction\":"
      << bool_json(hint.facts_used_for_target_satisfaction)
      << ",\"checkpoint_selected\":" << bool_json(hint.checkpoint_selected)
      << ",\"model_selector\":" << bool_json(hint.model_selector) << "}";
  return out.str();
}

[[nodiscard]] std::string plan_basis_fact_preview_hints_json(
    const std::vector<
        target::lattice_target_evaluation_t::plan_basis_t::fact_preview_hint_t>
        &hints) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < hints.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << plan_basis_fact_preview_hint_json(hints[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string artifact_proof_json(
    const target::lattice_target_proof_certificate_t::artifact_proof_t &proof) {
  std::ostringstream out;
  out << "{\"proof_kind\":" << json_quote(proof.proof_kind)
      << ",\"proof_template_bound\":" << bool_json(proof.proof_template_bound)
      << ",\"proof_template_claim\":" << json_quote(proof.proof_template_claim)
      << ",\"fact_family\":" << json_quote(proof.fact_family)
      << ",\"fact_schema\":" << json_quote(proof.fact_schema)
      << ",\"fact_type\":" << json_quote(proof.fact_type) << ",\"fact_ref\":"
      << json_quote(display::fact_ref(proof.fact_family, proof.fact_digest))
      << ",\"fact_digest_prefix\":"
      << json_quote(display::digest_prefix(proof.fact_digest))
      << ",\"fact_digest\":" << json_quote(proof.fact_digest)
      << ",\"fact_preview_hint\":" << artifact_fact_preview_hint_json(proof)
      << ",\"fact_identity_contract_schema\":"
      << json_quote(proof.fact_identity_contract_schema)
      << ",\"fact_identity_contract_id\":"
      << json_quote(proof.fact_identity_contract_id)
      << ",\"fact_identity_contract_bound\":"
      << bool_json(proof.fact_identity_contract_bound)
      << ",\"fact_identity_envelope_complete\":"
      << bool_json(proof.fact_identity_envelope_complete)
      << ",\"row_index_interval_authority\":"
      << bool_json(proof.row_index_interval_authority)
      << ",\"source_key_window_audit_only\":"
      << bool_json(proof.source_key_window_audit_only)
      << ",\"fact_identity_target_kind_authority\":"
      << bool_json(proof.fact_identity_target_kind_authority)
      << ",\"fact_identity_runtime_wave_authority\":"
      << bool_json(proof.fact_identity_runtime_wave_authority)
      << ",\"fact_identity_marshal_reachability\":"
      << bool_json(proof.fact_identity_marshal_reachability)
      << ",\"fact_identity_policy_gate_authority\":"
      << bool_json(proof.fact_identity_policy_gate_authority)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(proof.parent_exposure_fact_digest)
      << ",\"job_id\":" << json_quote(proof.job_id)
      << ",\"wave_id\":" << json_quote(proof.wave_id)
      << ",\"protocol_id\":" << json_quote(proof.protocol_id)
      << ",\"contract_fingerprint\":" << json_quote(proof.contract_fingerprint)
      << ",\"component\":" << json_quote(proof.component)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(proof.component_assembly_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(proof.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(proof.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(proof.split_policy_fingerprint)
      << ",\"split_name\":" << json_quote(proof.split_name)
      << ",\"anchor_range\":" << interval_json(proof.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(proof.completed_anchor_range)
      << ",\"identity_match\":" << bool_json(proof.identity_match)
      << ",\"artifact_evidence\":" << bool_json(proof.artifact_evidence)
      << ",\"deterministic_artifact\":"
      << bool_json(proof.deterministic_artifact)
      << ",\"visibility_only\":" << bool_json(proof.visibility_only)
      << ",\"authority_clean\":" << bool_json(proof.authority_clean)
      << ",\"readiness_authority\":" << bool_json(proof.readiness_authority)
      << ",\"quality_authority\":" << bool_json(proof.quality_authority)
      << ",\"performance_authority\":" << bool_json(proof.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(proof.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(proof.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(proof.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(proof.contract_identity_authority)
      << ",\"allocation_authority\":" << bool_json(proof.allocation_authority)
      << ",\"execution_authority\":" << bool_json(proof.execution_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(proof.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(proof.deployment_authority)
      << ",\"policy_gate\":" << bool_json(proof.policy_gate)
      << ",\"target_dependency_authority\":"
      << bool_json(proof.target_dependency_authority)
      << ",\"runtime_wave_authority\":"
      << bool_json(proof.runtime_wave_authority)
      << ",\"marshal_reachability\":" << bool_json(proof.marshal_reachability)
      << ",\"checkpoint_source_authority\":"
      << bool_json(proof.checkpoint_source_authority)
      << ",\"plan_checkpoint_input_authority\":"
      << bool_json(proof.plan_checkpoint_input_authority)
      << ",\"model_state_mutation\":" << bool_json(proof.model_state_mutation)
      << ",\"raw_potential_tradable_return\":"
      << bool_json(proof.raw_potential_tradable_return)
      << ",\"replay_executor\":" << bool_json(proof.replay_executor)
      << ",\"lineage_bound\":" << bool_json(proof.lineage_bound)
      << ",\"passed\":" << bool_json(proof.passed)
      << ",\"issues\":" << string_array_json(proof.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string artifact_proofs_json(
    const std::vector<
        target::lattice_target_proof_certificate_t::artifact_proof_t> &proofs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < proofs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << artifact_proof_json(proofs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string coverage_proof_json(
    const target::lattice_target_proof_certificate_t::coverage_proof_t &proof) {
  std::ostringstream out;
  out << "{\"use\":" << json_quote(proof.use)
      << ",\"algebra\":" << json_quote(proof.algebra)
      << ",\"target_range\":" << interval_json(proof.target_range)
      << ",\"contributing_intervals\":"
      << interval_array_json(proof.contributing_intervals)
      << ",\"covered_intervals\":"
      << interval_array_json(proof.covered_intervals)
      << ",\"missing_intervals\":"
      << interval_array_json(proof.missing_intervals)
      << ",\"required_fraction\":" << double_json(proof.required_fraction)
      << ",\"actual_fraction\":" << double_json(proof.actual_fraction)
      << ",\"covered_anchors\":" << proof.covered_anchors
      << ",\"missing_anchors\":" << proof.missing_anchors
      << ",\"require_mutated_component\":"
      << bool_json(proof.require_mutated_component)
      << ",\"passed\":" << bool_json(proof.passed)
      << ",\"load_summary\":" << exposure_load_summary_json(proof.load_summary)
      << "}";
  return out.str();
}

[[nodiscard]] std::string coverage_proofs_json(
    const std::vector<
        target::lattice_target_proof_certificate_t::coverage_proof_t> &proofs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < proofs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << coverage_proof_json(proofs[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
causal_exposure_json(const target::lattice_target_proof_certificate_t::
                         closure_proof_t::causal_exposure_t &exposure) {
  std::ostringstream out;
  out << "{\"fact_digest\":" << json_quote(exposure.fact_digest)
      << ",\"contract_fingerprint\":"
      << json_quote(exposure.contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(exposure.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(exposure.source_cursor_token)
      << ",\"cursor_domain\":" << json_quote(exposure.cursor_domain)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(exposure.component_assembly_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << json_quote(exposure.split_policy_fingerprint)
      << ",\"job_id\":" << json_quote(exposure.job_id)
      << ",\"wave_id\":" << json_quote(exposure.wave_id)
      << ",\"wave_action\":" << json_quote(exposure.wave_action)
      << ",\"job_status\":" << json_quote(exposure.job_status)
      << ",\"runtime_handoff_id\":" << json_quote(exposure.runtime_handoff_id)
      << ",\"runtime_handoff_digest\":"
      << json_quote(exposure.runtime_handoff_digest)
      << ",\"marshal_target_driver_run_id\":"
      << json_quote(exposure.marshal_target_driver_run_id)
      << ",\"target_component_family_id\":"
      << json_quote(exposure.target_component_family_id)
      << ",\"uses\":" << string_array_json(exposure.uses)
      << ",\"mutated_component\":" << bool_json(exposure.mutated_component)
      << ",\"split_name\":" << json_quote(exposure.split_name)
      << ",\"split_role\":"
      << json_quote(cuwacunu::hero::lattice::exposure::exposure_split_role_name(
             exposure.split_role))
      << ",\"anchor_range\":" << interval_json(exposure.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(exposure.completed_anchor_range)
      << ",\"observed_footprint\":"
      << interval_json(exposure.observed_footprint)
      << ",\"target_footprint\":" << interval_json(exposure.target_footprint)
      << ",\"source_key_window\":{\"precision\":"
      << json_quote(exposure.source_key_footprint_precision)
      << ",\"first_anchor_key\":" << json_quote(exposure.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(exposure.last_anchor_key)
      << ",\"observed_begin\":"
      << json_quote(exposure.observed_source_key_begin)
      << ",\"observed_end\":" << json_quote(exposure.observed_source_key_end)
      << ",\"target_begin\":" << json_quote(exposure.target_source_key_begin)
      << ",\"target_end\":" << json_quote(exposure.target_source_key_end) << "}"
      << ",\"source_key_window_audit\":"
      << source_key_window_audit_json(exposure.source_key_window_audit)
      << ",\"output_checkpoint\":"
      << json_quote(exposure.output_checkpoint.lexically_normal().string())
      << ",\"input_checkpoints\":"
      << path_array_json(exposure.input_checkpoints) << "}";
  return out.str();
}

[[nodiscard]] std::string causal_exposures_json(
    const std::vector<target::lattice_target_proof_certificate_t::
                          closure_proof_t::causal_exposure_t> &exposures) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < exposures.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << causal_exposure_json(exposures[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string checkpoint_identity_preview_json(
    const target::lattice_target_proof_certificate_t::closure_proof_t::
        checkpoint_identity_preview_t &preview) {
  std::ostringstream out;
  out << "{\"checkpoint_path\":"
      << json_quote(preview.checkpoint_path.lexically_normal().string())
      << ",\"checkpoint_id\":" << json_quote(preview.checkpoint_id)
      << ",\"checkpoint_file_digest\":"
      << json_quote(preview.checkpoint_file_digest) << ",\"checkpoint_ref\":"
      << json_quote(display::checkpoint_ref(preview.checkpoint_file_digest))
      << ",\"component\":" << json_quote(preview.component)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(preview.component_assembly_fingerprint)
      << ",\"direct_exposure_digest\":"
      << json_quote(preview.direct_exposure_digest)
      << ",\"created_by_job_id\":" << json_quote(preview.created_by_job_id)
      << ",\"created_by_wave_id\":" << json_quote(preview.created_by_wave_id)
      << "}";
  return out.str();
}

[[nodiscard]] std::string checkpoint_identity_previews_json(
    const std::vector<target::lattice_target_proof_certificate_t::
                          closure_proof_t::checkpoint_identity_preview_t>
        &previews) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < previews.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << checkpoint_identity_preview_json(previews[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string closure_proof_json(
    const target::lattice_target_proof_certificate_t::closure_proof_t &proof) {
  std::ostringstream out;
  out << "{\"checked\":" << bool_json(proof.checked)
      << ",\"checkpoint_path\":" << json_quote(proof.checkpoint_path.string())
      << ",\"complete\":" << bool_json(proof.complete)
      << ",\"fact_count\":" << proof.fact_count
      << ",\"resolution_authority\":" << json_quote(proof.resolution_authority)
      << ",\"root_checkpoint_id\":" << json_quote(proof.root_checkpoint_id)
      << ",\"root_checkpoint_file_digest\":"
      << json_quote(proof.root_checkpoint_file_digest)
      << ",\"root_checkpoint_ref\":"
      << json_quote(display::checkpoint_ref(proof.root_checkpoint_file_digest))
      << ",\"identity_mismatches\":"
      << string_array_json(proof.identity_mismatches)
      << ",\"unresolved_input_checkpoints\":"
      << path_array_json(proof.unresolved_input_checkpoints)
      << ",\"fact_digests\":" << string_array_json(proof.fact_digests)
      << ",\"causal_exposures\":"
      << causal_exposures_json(proof.causal_exposures)
      << ",\"checkpoint_identity_previews\":"
      << checkpoint_identity_previews_json(proof.checkpoint_identity_previews)
      << "}";
  return out.str();
}

[[nodiscard]] std::string leakage_overlap_witness_json(
    const exposure::forbidden_exposure_overlap_t &witness) {
  std::ostringstream out;
  out << "{\"fact_digest\":" << json_quote(witness.fact_digest)
      << ",\"job_id\":" << json_quote(witness.job_id)
      << ",\"wave_id\":" << json_quote(witness.wave_id)
      << ",\"target_component_family_id\":"
      << json_quote(witness.target_component_family_id)
      << ",\"split_name\":" << json_quote(witness.split_name)
      << ",\"use\":" << json_quote(exposure::exposure_use_name(witness.use))
      << ",\"mutated_component\":" << bool_json(witness.mutated_component)
      << ",\"selector_id\":" << json_quote(witness.selector_id)
      << ",\"selector_kind\":" << json_quote(witness.selector_kind)
      << ",\"selection_event_digest\":"
      << json_quote(witness.selection_event_digest)
      << ",\"selected_checkpoint\":"
      << json_quote(witness.selected_checkpoint.lexically_normal().string())
      << ",\"source_footprint\":" << interval_json(witness.source_footprint)
      << ",\"protected_range\":" << interval_json(witness.protected_range)
      << ",\"intersection\":" << interval_json(witness.intersection) << "}";
  return out.str();
}

[[nodiscard]] std::string leakage_overlap_witnesses_json(
    const std::vector<exposure::forbidden_exposure_overlap_t> &witnesses) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < witnesses.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << leakage_overlap_witness_json(witnesses[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string leakage_rule_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_leakage_rule_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"rule\":" << json_quote(vocabulary[i].rule)
        << ",\"protected_set_algebra\":"
        << json_quote(vocabulary[i].protected_set_algebra)
        << ",\"dilation_kernel\":" << json_quote(vocabulary[i].dilation_kernel)
        << ",\"predicate\":" << json_quote(vocabulary[i].predicate)
        << ",\"witness_basis\":" << json_quote(vocabulary[i].witness_basis)
        << ",\"requires_closure_complete\":"
        << bool_json(vocabulary[i].requires_closure_complete)
        << ",\"protected_range_contains_base\":"
        << bool_json(vocabulary[i].protected_range_contains_base) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string leakage_rule_summary_json() {
  const auto summary = target::lattice_leakage_rule_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"rule_count\":" << summary.rule_count
      << ",\"temporal_dilation_rule_count\":"
      << summary.temporal_dilation_rule_count
      << ",\"closure_complete_required_count\":"
      << summary.closure_complete_required_count
      << ",\"protected_contains_base_count\":"
      << summary.protected_contains_base_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"single_v1_rule\":" << bool_json(summary.single_v1_rule)
      << ",\"protected_split_dilation_intersection_rule_present\":"
      << bool_json(summary.protected_split_dilation_intersection_rule_present)
      << ",\"predicate_is_forbidden_use_empty_intersection\":"
      << bool_json(summary.predicate_is_forbidden_use_empty_intersection)
      << ",\"witness_basis_is_labeled_causal_reachability\":"
      << bool_json(summary.witness_basis_is_labeled_causal_reachability)
      << ",\"closure_completeness_required\":"
      << bool_json(summary.closure_completeness_required)
      << ",\"protected_range_contains_base\":"
      << bool_json(summary.protected_range_contains_base)
      << ",\"all_rules_have_policy_text\":"
      << bool_json(summary.all_rules_have_policy_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"rule_names\":" << string_array_json(summary.rule_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string causal_edge_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_causal_edge_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"edge\":" << json_quote(vocabulary[i].edge)
        << ",\"source\":" << json_quote(vocabulary[i].source)
        << ",\"target\":" << json_quote(vocabulary[i].target)
        << ",\"proof_field\":" << json_quote(vocabulary[i].proof_field)
        << ",\"closure_reachability_edge\":"
        << bool_json(vocabulary[i].closure_reachability_edge)
        << ",\"leakage_relevant_when_forbidden\":"
        << bool_json(vocabulary[i].leakage_relevant_when_forbidden) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string causal_edge_summary_json() {
  const auto summary = target::lattice_causal_edge_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"edge_count\":" << summary.edge_count
      << ",\"source_row_edge_count\":" << summary.source_row_edge_count
      << ",\"checkpoint_reachability_edge_count\":"
      << summary.checkpoint_reachability_edge_count
      << ",\"leakage_relevant_edge_count\":"
      << summary.leakage_relevant_edge_count
      << ",\"mutation_edge_count\":" << summary.mutation_edge_count
      << ",\"checkpoint_edge_leakage_overlap_count\":"
      << summary.checkpoint_edge_leakage_overlap_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"source_row_use_edges_present\":"
      << bool_json(summary.source_row_use_edges_present)
      << ",\"checkpoint_load_create_edges_present\":"
      << bool_json(summary.checkpoint_load_create_edges_present)
      << ",\"mutation_edge_present\":"
      << bool_json(summary.mutation_edge_present)
      << ",\"checkpoint_reachability_distinct_from_leakage_edges\":"
      << bool_json(summary.checkpoint_reachability_distinct_from_leakage_edges)
      << ",\"selection_signal_is_leakage_relevant\":"
      << bool_json(summary.selection_signal_is_leakage_relevant)
      << ",\"evaluation_metric_is_leakage_relevant_when_forbidden\":"
      << bool_json(summary.evaluation_metric_is_leakage_relevant_when_forbidden)
      << ",\"all_edges_have_proof_fields\":"
      << bool_json(summary.all_edges_have_proof_fields)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"edge_names\":" << string_array_json(summary.edge_names)
      << ",\"missing_required_edges\":"
      << string_array_json(summary.missing_required_edges)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string selection_signal_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_selection_signal_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"relation\":" << json_quote(vocabulary[i].relation)
        << ",\"proof_field\":" << json_quote(vocabulary[i].proof_field)
        << ",\"source_footprint_basis\":"
        << json_quote(vocabulary[i].source_footprint_basis)
        << ",\"status_effect\":" << json_quote(vocabulary[i].status_effect)
        << ",\"v1_scope\":" << json_quote(vocabulary[i].v1_scope)
        << ",\"forbidden_use\":" << bool_json(vocabulary[i].forbidden_use)
        << ",\"requires_closure_complete\":"
        << bool_json(vocabulary[i].requires_closure_complete)
        << ",\"first_class_event_stream\":"
        << bool_json(vocabulary[i].first_class_event_stream)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string selection_signal_policy_summary_json() {
  const auto summary = target::lattice_selection_signal_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"forbidden_use_policy_count\":"
      << summary.forbidden_use_policy_count
      << ",\"requires_closure_complete_count\":"
      << summary.requires_closure_complete_count
      << ",\"first_class_event_stream_count\":"
      << summary.first_class_event_stream_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"exposure_failed_policy_count\":"
      << summary.exposure_failed_policy_count
      << ",\"future_policy_count\":" << summary.future_policy_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"forbidden_use_policy_present\":"
      << bool_json(summary.forbidden_use_policy_present)
      << ",\"causal_edge_policy_present\":"
      << bool_json(summary.causal_edge_policy_present)
      << ",\"event_stream_deferred\":"
      << bool_json(summary.event_stream_deferred)
      << ",\"event_stream_available\":"
      << bool_json(summary.event_stream_available)
      << ",\"path_policy_future_fail_closed\":"
      << bool_json(summary.path_policy_future_fail_closed)
      << ",\"known_selector_path_fail_closed\":"
      << bool_json(summary.known_selector_path_fail_closed)
      << ",\"anchor_range_is_v1_source_footprint\":"
      << bool_json(summary.anchor_range_is_v1_source_footprint)
      << ",\"all_policies_require_closure_complete\":"
      << bool_json(summary.all_policies_require_closure_complete)
      << ",\"no_first_class_event_stream_in_v1\":"
      << bool_json(summary.no_first_class_event_stream_in_v1)
      << ",\"first_class_event_stream_is_read_only\":"
      << bool_json(summary.first_class_event_stream_is_read_only)
      << ",\"no_runtime_executor\":" << bool_json(summary.no_runtime_executor)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string derived_query_rule_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"relation\":" << json_quote(vocabulary[i].relation)
        << ",\"rule\":" << json_quote(vocabulary[i].rule)
        << ",\"input_relations\":"
        << string_array_json(vocabulary[i].input_relations)
        << ",\"proof_field\":" << json_quote(vocabulary[i].proof_field)
        << ",\"fail_closed_policy\":"
        << json_quote(vocabulary[i].fail_closed_policy)
        << ",\"read_only\":" << bool_json(vocabulary[i].read_only)
        << ",\"db_cacheable\":" << bool_json(vocabulary[i].db_cacheable)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << ",\"projection_scope\":"
        << json_quote(vocabulary[i].projection_scope)
        << ",\"projection_quantifier\":"
        << json_quote(vocabulary[i].projection_quantifier)
        << ",\"empty_projection_policy\":"
        << json_quote(vocabulary[i].empty_projection_policy)
        << ",\"result_projection_scope\":"
        << json_quote(vocabulary[i].result_projection_scope)
        << ",\"result_projection_quantifier\":"
        << json_quote(vocabulary[i].result_projection_quantifier)
        << ",\"result_empty_projection_policy\":"
        << json_quote(vocabulary[i].result_empty_projection_policy) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string derived_query_rule_entry_json(
    const target::lattice_derived_query_rule_entry_t &entry) {
  std::ostringstream out;
  out << "{\"relation\":" << json_quote(entry.relation)
      << ",\"rule\":" << json_quote(entry.rule)
      << ",\"input_relations\":" << string_array_json(entry.input_relations)
      << ",\"proof_field\":" << json_quote(entry.proof_field)
      << ",\"fail_closed_policy\":" << json_quote(entry.fail_closed_policy)
      << ",\"read_only\":" << bool_json(entry.read_only)
      << ",\"db_cacheable\":" << bool_json(entry.db_cacheable)
      << ",\"runtime_executor\":" << bool_json(entry.runtime_executor)
      << ",\"projection_scope\":" << json_quote(entry.projection_scope)
      << ",\"projection_quantifier\":"
      << json_quote(entry.projection_quantifier)
      << ",\"empty_projection_policy\":"
      << json_quote(entry.empty_projection_policy)
      << ",\"result_projection_scope\":"
      << json_quote(entry.result_projection_scope)
      << ",\"result_projection_quantifier\":"
      << json_quote(entry.result_projection_quantifier)
      << ",\"result_empty_projection_policy\":"
      << json_quote(entry.result_empty_projection_policy) << "}";
  return out.str();
}

[[nodiscard]] const target::lattice_derived_query_rule_entry_t *
find_derived_query_rule_entry(const std::string &relation) {
  static const auto vocabulary =
      target::lattice_derived_query_rule_vocabulary();
  const auto it =
      std::find_if(vocabulary.begin(), vocabulary.end(),
                   [&](const auto &row) { return row.relation == relation; });
  if (it == vocabulary.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] std::string derived_query_rule_vocabulary_digest() {
  std::ostringstream text;
  text << "schema=" << kDerivedQueryRuleVocabularyDigestSchema << "\n";
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  for (const auto &entry : vocabulary) {
    text << "relation=" << entry.relation << "\n"
         << "rule=" << entry.rule << "\n";
    for (const auto &input_relation : entry.input_relations) {
      text << "input_relation=" << input_relation << "\n";
    }
    text << "proof_field=" << entry.proof_field << "\n"
         << "fail_closed_policy=" << entry.fail_closed_policy << "\n"
         << "read_only=" << (entry.read_only ? "true" : "false") << "\n"
         << "db_cacheable=" << (entry.db_cacheable ? "true" : "false") << "\n"
         << "runtime_executor=" << (entry.runtime_executor ? "true" : "false")
         << "\n"
         << "projection_scope=" << entry.projection_scope << "\n"
         << "projection_quantifier=" << entry.projection_quantifier << "\n"
         << "empty_projection_policy=" << entry.empty_projection_policy << "\n"
         << "result_projection_scope=" << entry.result_projection_scope << "\n"
         << "result_projection_quantifier="
         << entry.result_projection_quantifier << "\n"
         << "result_empty_projection_policy="
         << entry.result_empty_projection_policy << "\n";
  }
  return exposure::exposure_digest_for_text(text.str());
}

[[nodiscard]] std::string derived_query_projection_semantics_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_derived_query_projection_semantics_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"field\":" << json_quote(vocabulary[i].field)
        << ",\"value\":" << json_quote(vocabulary[i].value)
        << ",\"meaning\":" << json_quote(vocabulary[i].meaning)
        << ",\"result_alias\":" << bool_json(vocabulary[i].result_alias)
        << ",\"compact_projection\":"
        << bool_json(vocabulary[i].compact_projection) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string db_cache_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_db_cache_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"cached_relations\":"
        << string_array_json(vocabulary[i].cached_relations)
        << ",\"source_of_truth\":" << json_quote(vocabulary[i].source_of_truth)
        << ",\"allowed_use\":" << json_quote(vocabulary[i].allowed_use)
        << ",\"invalidation_basis\":"
        << json_quote(vocabulary[i].invalidation_basis)
        << ",\"fail_closed_policy\":"
        << json_quote(vocabulary[i].fail_closed_policy)
        << ",\"db_writes_evidence\":"
        << bool_json(vocabulary[i].db_writes_evidence)
        << ",\"rebuildable_from_runtime_files\":"
        << bool_json(vocabulary[i].rebuildable_from_runtime_files)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << ",\"contract_identity_authority\":"
        << bool_json(vocabulary[i].contract_identity_authority) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_retention_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_evidence_retention_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"artifact_family\":" << json_quote(vocabulary[i].artifact_family)
        << ",\"retention_role\":" << json_quote(vocabulary[i].retention_role)
        << ",\"proof_replay_obligation\":"
        << json_quote(vocabulary[i].proof_replay_obligation)
        << ",\"required_bindings\":"
        << string_array_json(vocabulary[i].required_bindings)
        << ",\"prune_policy\":" << json_quote(vocabulary[i].prune_policy)
        << ",\"source_of_truth\":" << bool_json(vocabulary[i].source_of_truth)
        << ",\"compact_receipt_authority\":"
        << bool_json(vocabulary[i].compact_receipt_authority)
        << ",\"cache_row\":" << bool_json(vocabulary[i].cache_row)
        << ",\"human_receipt\":" << bool_json(vocabulary[i].human_receipt)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_retention_audit_scenario_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_evidence_retention_audit_scenario_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"scenario\":" << json_quote(vocabulary[i].scenario)
        << ",\"expected_result\":" << json_quote(vocabulary[i].expected_result)
        << ",\"evidence_required\":"
        << json_quote(vocabulary[i].evidence_required)
        << ",\"preserves_replay_authority\":"
        << bool_json(vocabulary[i].preserves_replay_authority)
        << ",\"refuses_or_warns\":" << bool_json(vocabulary[i].refuses_or_warns)
        << ",\"cache_non_authority\":"
        << bool_json(vocabulary[i].cache_non_authority)
        << ",\"compact_receipt_non_authority\":"
        << bool_json(vocabulary[i].compact_receipt_non_authority) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_retention_policy_summary_json() {
  const auto summary = target::lattice_evidence_retention_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"source_of_truth_count\":" << summary.source_of_truth_count
      << ",\"compact_receipt_authority_count\":"
      << summary.compact_receipt_authority_count
      << ",\"cache_row_count\":" << summary.cache_row_count
      << ",\"human_receipt_count\":" << summary.human_receipt_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"scenario_count\":" << summary.scenario_count
      << ",\"replay_preserving_scenario_count\":"
      << summary.replay_preserving_scenario_count
      << ",\"refuse_or_warn_scenario_count\":"
      << summary.refuse_or_warn_scenario_count
      << ",\"empty_binding_count\":" << summary.empty_binding_count
      << ",\"reports_preserved_for_replay\":"
      << bool_json(summary.reports_preserved_for_replay)
      << ",\"sidecars_preserved_for_replay\":"
      << bool_json(summary.sidecars_preserved_for_replay)
      << ",\"checkpoints_preserved_for_lineage\":"
      << bool_json(summary.checkpoints_preserved_for_lineage)
      << ",\"source_receipts_audit_only\":"
      << bool_json(summary.source_receipts_audit_only)
      << ",\"selection_signals_preserved_for_leakage\":"
      << bool_json(summary.selection_signals_preserved_for_leakage)
      << ",\"proof_certificates_non_authority\":"
      << bool_json(summary.proof_certificates_non_authority)
      << ",\"cache_rows_rebuildable_non_authority\":"
      << bool_json(summary.cache_rows_rebuildable_non_authority)
      << ",\"human_receipts_non_authority\":"
      << bool_json(summary.human_receipts_non_authority)
      << ",\"archive_manifest_binds_identity\":"
      << bool_json(summary.archive_manifest_binds_identity)
      << ",\"compact_receipts_non_authority\":"
      << bool_json(summary.compact_receipts_non_authority)
      << ",\"pruning_checks_unresolved_lineage\":"
      << bool_json(summary.pruning_checks_unresolved_lineage)
      << ",\"stale_cache_after_archive_non_authority\":"
      << bool_json(summary.stale_cache_after_archive_non_authority)
      << ",\"complete_archive_replay_scenario_present\":"
      << bool_json(summary.complete_archive_replay_scenario_present)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"artifact_families\":"
      << string_array_json(summary.artifact_families)
      << ",\"scenario_names\":" << string_array_json(summary.scenario_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string benchmark_regression_budget_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_benchmark_regression_budget_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"benchmark_id\":" << json_quote(vocabulary[i].benchmark_id)
        << ",\"timing_layer\":" << json_quote(vocabulary[i].timing_layer)
        << ",\"proof_mode\":" << json_quote(vocabulary[i].proof_mode)
        << ",\"measured_surface\":"
        << json_quote(vocabulary[i].measured_surface)
        << ",\"required_measurement\":"
        << json_quote(vocabulary[i].required_measurement)
        << ",\"regression_guard\":"
        << json_quote(vocabulary[i].regression_guard)
        << ",\"expected_fast_path\":"
        << json_quote(vocabulary[i].expected_fast_path)
        << ",\"full_live_scan_allowed\":"
        << bool_json(vocabulary[i].full_live_scan_allowed)
        << ",\"metadata_digest_allowed\":"
        << bool_json(vocabulary[i].metadata_digest_allowed)
        << ",\"cache_target_authority\":"
        << bool_json(vocabulary[i].cache_target_authority)
        << ",\"process_startup_included\":"
        << bool_json(vocabulary[i].process_startup_included)
        << ",\"session_reuse_required\":"
        << bool_json(vocabulary[i].session_reuse_required)
        << ",\"regression_smoke_required\":"
        << bool_json(vocabulary[i].regression_smoke_required)
        << ",\"baseline_receipt_required\":"
        << bool_json(vocabulary[i].baseline_receipt_required) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string benchmark_regression_budget_summary_json() {
  const auto summary = target::lattice_benchmark_regression_budget_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"budget_count\":" << summary.budget_count
      << ",\"library_layer_count\":" << summary.library_layer_count
      << ",\"long_lived_mcp_layer_count\":"
      << summary.long_lived_mcp_layer_count
      << ",\"direct_cli_layer_count\":" << summary.direct_cli_layer_count
      << ",\"header_only_mode_count\":" << summary.header_only_mode_count
      << ",\"watched_manifest_mode_count\":"
      << summary.watched_manifest_mode_count
      << ",\"full_metadata_mode_count\":" << summary.full_metadata_mode_count
      << ",\"live_scan_mode_count\":" << summary.live_scan_mode_count
      << ",\"live_parity_mode_count\":" << summary.live_parity_mode_count
      << ",\"full_live_scan_allowed_count\":"
      << summary.full_live_scan_allowed_count
      << ",\"metadata_digest_allowed_count\":"
      << summary.metadata_digest_allowed_count
      << ",\"cache_target_authority_count\":"
      << summary.cache_target_authority_count
      << ",\"process_startup_included_count\":"
      << summary.process_startup_included_count
      << ",\"session_reuse_required_count\":"
      << summary.session_reuse_required_count
      << ",\"regression_smoke_required_count\":"
      << summary.regression_smoke_required_count
      << ",\"baseline_receipt_required_count\":"
      << summary.baseline_receipt_required_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"all_required_layers_present\":"
      << bool_json(summary.all_required_layers_present)
      << ",\"all_required_proof_modes_present\":"
      << bool_json(summary.all_required_proof_modes_present)
      << ",\"header_only_fast_paths_forbid_live_scan\":"
      << bool_json(summary.header_only_fast_paths_forbid_live_scan)
      << ",\"proof_modes_separate_from_fast_path\":"
      << bool_json(summary.proof_modes_separate_from_fast_path)
      << ",\"long_lived_mcp_session_reuse_present\":"
      << bool_json(summary.long_lived_mcp_session_reuse_present)
      << ",\"direct_cli_startup_separated\":"
      << bool_json(summary.direct_cli_startup_separated)
      << ",\"regression_smoke_declared\":"
      << bool_json(summary.regression_smoke_declared)
      << ",\"baseline_receipt_required_for_all_rows\":"
      << bool_json(summary.baseline_receipt_required_for_all_rows)
      << ",\"no_cache_target_satisfaction_authority\":"
      << bool_json(summary.no_cache_target_satisfaction_authority)
      << ",\"all_rows_have_measurement_and_guard\":"
      << bool_json(summary.all_rows_have_measurement_and_guard)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"benchmark_ids\":" << string_array_json(summary.benchmark_ids)
      << ",\"proof_modes\":" << string_array_json(summary.proof_modes)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string evidence_abstraction_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_evidence_abstraction_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"abstraction\":" << json_quote(vocabulary[i].abstraction)
        << ",\"concrete_inputs\":"
        << string_array_json(vocabulary[i].concrete_inputs)
        << ",\"abstract_outputs\":"
        << string_array_json(vocabulary[i].abstract_outputs)
        << ",\"soundness_condition\":"
        << json_quote(vocabulary[i].soundness_condition)
        << ",\"conservative_policy\":"
        << json_quote(vocabulary[i].conservative_policy)
        << ",\"join_semantics\":" << json_quote(vocabulary[i].join_semantics)
        << ",\"read_only\":" << bool_json(vocabulary[i].read_only)
        << ",\"db_cacheable\":" << bool_json(vocabulary[i].db_cacheable) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string product_evidence_state_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_product_evidence_state_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"factor\":" << json_quote(vocabulary[i].factor)
        << ",\"mathematical_object\":"
        << json_quote(vocabulary[i].mathematical_object)
        << ",\"partial_order\":" << json_quote(vocabulary[i].partial_order)
        << ",\"join_semantics\":" << json_quote(vocabulary[i].join_semantics)
        << ",\"proof_fields\":" << string_array_json(vocabulary[i].proof_fields)
        << ",\"clean_growth_rule\":"
        << json_quote(vocabulary[i].clean_growth_rule)
        << ",\"target_effect\":" << json_quote(vocabulary[i].target_effect)
        << ",\"join_requires_identity_match\":"
        << bool_json(vocabulary[i].join_requires_identity_match)
        << ",\"status_monotone_dimension\":"
        << bool_json(vocabulary[i].status_monotone_dimension)
        << ",\"db_cacheable\":" << bool_json(vocabulary[i].db_cacheable) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string join_law_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_join_law_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"factor\":" << json_quote(vocabulary[i].factor)
        << ",\"join_operator\":" << json_quote(vocabulary[i].join_operator)
        << ",\"algebraic_laws\":"
        << string_array_json(vocabulary[i].algebraic_laws)
        << ",\"identity_scope\":" << json_quote(vocabulary[i].identity_scope)
        << ",\"duplicate_policy\":"
        << json_quote(vocabulary[i].duplicate_policy)
        << ",\"unsafe_join_policy\":"
        << json_quote(vocabulary[i].unsafe_join_policy)
        << ",\"cache_requirement\":"
        << json_quote(vocabulary[i].cache_requirement)
        << ",\"associative\":" << bool_json(vocabulary[i].associative)
        << ",\"commutative\":" << bool_json(vocabulary[i].commutative)
        << ",\"idempotent\":" << bool_json(vocabulary[i].idempotent)
        << ",\"additive\":" << bool_json(vocabulary[i].additive)
        << ",\"requires_identity_match\":"
        << bool_json(vocabulary[i].requires_identity_match)
        << ",\"cache_safe\":" << bool_json(vocabulary[i].cache_safe) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string node_support_scope_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_node_support_scope_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"evidence_scope\":" << json_quote(vocabulary[i].evidence_scope)
        << ",\"authority\":" << json_quote(vocabulary[i].authority)
        << ",\"allowed_claim\":" << json_quote(vocabulary[i].allowed_claim)
        << ",\"forbidden_claim\":" << json_quote(vocabulary[i].forbidden_claim)
        << ",\"future_evidence_required\":"
        << json_quote(vocabulary[i].future_evidence_required)
        << ",\"mdn_node_support_authority\":"
        << bool_json(vocabulary[i].mdn_node_support_authority)
        << ",\"vicreg_per_node_readiness_authority\":"
        << bool_json(vocabulary[i].vicreg_per_node_readiness_authority)
        << ",\"synthetic_backfill_allowed\":"
        << bool_json(vocabulary[i].synthetic_backfill_allowed)
        << ",\"available_in_v1\":" << bool_json(vocabulary[i].available_in_v1)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string node_support_scope_policy_summary_json() {
  const auto summary = target::lattice_node_support_scope_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"available_in_v1_count\":" << summary.available_in_v1_count
      << ",\"deferred_future_count\":" << summary.deferred_future_count
      << ",\"mdn_node_support_authority_count\":"
      << summary.mdn_node_support_authority_count
      << ",\"vicreg_per_node_readiness_authority_count\":"
      << summary.vicreg_per_node_readiness_authority_count
      << ",\"synthetic_backfill_allowed_count\":"
      << summary.synthetic_backfill_allowed_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"mdn_node_support_scope_available\":"
      << bool_json(summary.mdn_node_support_scope_available)
      << ",\"vicreg_shared_encoder_boundary_available\":"
      << bool_json(summary.vicreg_shared_encoder_boundary_available)
      << ",\"future_representation_node_support_deferred\":"
      << bool_json(summary.future_representation_node_support_deferred)
      << ",\"node_support_matrix_join_boundary_available\":"
      << bool_json(summary.node_support_matrix_join_boundary_available)
      << ",\"no_vicreg_per_node_readiness_authority\":"
      << bool_json(summary.no_vicreg_per_node_readiness_authority)
      << ",\"no_synthetic_backfill\":"
      << bool_json(summary.no_synthetic_backfill)
      << ",\"no_runtime_executor\":" << bool_json(summary.no_runtime_executor)
      << ",\"future_representation_requires_nodelift\":"
      << bool_json(summary.future_representation_requires_nodelift)
      << ",\"mdn_scope_forbids_vicreg_per_node_readiness\":"
      << bool_json(summary.mdn_scope_forbids_vicreg_per_node_readiness)
      << ",\"vicreg_boundary_forbids_mdn_backfill\":"
      << bool_json(summary.vicreg_boundary_forbids_mdn_backfill)
      << ",\"matrix_join_boundary_forbids_cross_component\":"
      << bool_json(summary.matrix_join_boundary_forbids_cross_component)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
validation_performance_scope_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_validation_performance_scope_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"evidence_scope\":" << json_quote(vocabulary[i].evidence_scope)
        << ",\"authority\":" << json_quote(vocabulary[i].authority)
        << ",\"allowed_claim\":" << json_quote(vocabulary[i].allowed_claim)
        << ",\"forbidden_claim\":" << json_quote(vocabulary[i].forbidden_claim)
        << ",\"future_evidence_required\":"
        << json_quote(vocabulary[i].future_evidence_required)
        << ",\"evaluation_readiness_authority\":"
        << bool_json(vocabulary[i].evaluation_readiness_authority)
        << ",\"performance_gate_authority\":"
        << bool_json(vocabulary[i].performance_gate_authority)
        << ",\"uncertainty_required_for_future_gate\":"
        << bool_json(vocabulary[i].uncertainty_required_for_future_gate)
        << ",\"available_in_v1\":" << bool_json(vocabulary[i].available_in_v1)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string validation_performance_scope_policy_summary_json() {
  const auto summary =
      target::lattice_validation_performance_scope_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"available_in_v1_count\":" << summary.available_in_v1_count
      << ",\"deferred_future_count\":" << summary.deferred_future_count
      << ",\"evaluation_readiness_authority_count\":"
      << summary.evaluation_readiness_authority_count
      << ",\"performance_gate_authority_count\":"
      << summary.performance_gate_authority_count
      << ",\"uncertainty_required_for_future_gate_count\":"
      << summary.uncertainty_required_for_future_gate_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"validation_eval_readiness_scope_available\":"
      << bool_json(summary.validation_eval_readiness_scope_available)
      << ",\"evaluation_metric_coverage_not_quality_present\":"
      << bool_json(summary.evaluation_metric_coverage_not_quality_present)
      << ",\"mdn_calibration_warning_not_gate_present\":"
      << bool_json(summary.mdn_calibration_warning_not_gate_present)
      << ",\"future_validation_performance_target_deferred\":"
      << bool_json(summary.future_validation_performance_target_deferred)
      << ",\"v1_evaluation_readiness_not_performance_gate\":"
      << bool_json(summary.v1_evaluation_readiness_not_performance_gate)
      << ",\"future_gate_requires_uncertainty\":"
      << bool_json(summary.future_gate_requires_uncertainty)
      << ",\"future_gate_mentions_selection_leakage\":"
      << bool_json(summary.future_gate_mentions_selection_leakage)
      << ",\"all_policies_require_uncertainty_for_future_gate\":"
      << bool_json(summary.all_policies_require_uncertainty_for_future_gate)
      << ",\"no_runtime_executor\":" << bool_json(summary.no_runtime_executor)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
validation_performance_evidence_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_validation_performance_evidence_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"metric\":" << json_quote(vocabulary[i].metric)
        << ",\"metric_family\":" << json_quote(vocabulary[i].metric_family)
        << ",\"fact_schema\":" << json_quote(vocabulary[i].fact_schema)
        << ",\"split_field\":" << json_quote(vocabulary[i].split_field)
        << ",\"checkpoint_identity_fields\":"
        << string_array_json(vocabulary[i].checkpoint_identity_fields)
        << ",\"evaluated_checkpoint_binding_field\":"
        << json_quote(vocabulary[i].evaluated_checkpoint_binding_field)
        << ",\"sample_count_field\":"
        << json_quote(vocabulary[i].sample_count_field)
        << ",\"point_estimate_field\":"
        << json_quote(vocabulary[i].point_estimate_field)
        << ",\"uncertainty_method\":"
        << json_quote(vocabulary[i].uncertainty_method)
        << ",\"uncertainty_bound_fields\":"
        << string_array_json(vocabulary[i].uncertainty_bound_fields)
        << ",\"target_syntax\":" << json_quote(vocabulary[i].target_syntax)
        << ",\"selection_leakage_policy\":"
        << json_quote(vocabulary[i].selection_leakage_policy)
        << ",\"fail_closed_cases\":"
        << string_array_json(vocabulary[i].fail_closed_cases)
        << ",\"readiness_effect\":"
        << json_quote(vocabulary[i].readiness_effect)
        << ",\"available_in_v3c\":" << bool_json(vocabulary[i].available_in_v3c)
        << ",\"performance_gate_authority\":"
        << bool_json(vocabulary[i].performance_gate_authority)
        << ",\"warning_visibility\":"
        << bool_json(vocabulary[i].warning_visibility)
        << ",\"future_gate_candidate\":"
        << bool_json(vocabulary[i].future_gate_candidate)
        << ",\"requires_active_identity\":"
        << bool_json(vocabulary[i].requires_active_identity)
        << ",\"requires_exact_checkpoint_binding\":"
        << bool_json(vocabulary[i].requires_exact_checkpoint_binding)
        << ",\"requires_selection_signal_clean\":"
        << bool_json(vocabulary[i].requires_selection_signal_clean)
        << ",\"requires_uncertainty_bounds\":"
        << bool_json(vocabulary[i].requires_uncertainty_bounds) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
validation_performance_evidence_policy_summary_json() {
  const auto summary =
      target::lattice_validation_performance_evidence_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"available_in_v3c_count\":" << summary.available_in_v3c_count
      << ",\"deferred_future_count\":" << summary.deferred_future_count
      << ",\"performance_gate_authority_count\":"
      << summary.performance_gate_authority_count
      << ",\"warning_visibility_count\":" << summary.warning_visibility_count
      << ",\"future_gate_candidate_count\":"
      << summary.future_gate_candidate_count
      << ",\"active_identity_required_count\":"
      << summary.active_identity_required_count
      << ",\"exact_checkpoint_binding_required_count\":"
      << summary.exact_checkpoint_binding_required_count
      << ",\"selection_signal_clean_required_count\":"
      << summary.selection_signal_clean_required_count
      << ",\"uncertainty_bounds_required_count\":"
      << summary.uncertainty_bounds_required_count
      << ",\"point_estimate_without_uncertainty_count\":"
      << summary.point_estimate_without_uncertainty_count
      << ",\"point_estimate_without_uncertainty_gate_count\":"
      << summary.point_estimate_without_uncertainty_gate_count
      << ",\"missing_fail_closed_case_count\":"
      << summary.missing_fail_closed_case_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"finite_supported_metrics_documented\":"
      << bool_json(summary.finite_supported_metrics_documented)
      << ",\"fact_fields_name_split_checkpoint_sample_uncertainty\":"
      << bool_json(summary.fact_fields_name_split_checkpoint_sample_uncertainty)
      << ",\"available_point_estimates_are_bounded_or_warning_only\":"
      << bool_json(
             summary.available_point_estimates_are_bounded_or_warning_only)
      << ",\"no_point_estimate_gate_without_uncertainty\":"
      << bool_json(summary.no_point_estimate_gate_without_uncertainty)
      << ",\"exact_checkpoint_binding_required\":"
      << bool_json(summary.exact_checkpoint_binding_required)
      << ",\"active_identity_required\":"
      << bool_json(summary.active_identity_required)
      << ",\"selection_signal_clean_required\":"
      << bool_json(summary.selection_signal_clean_required)
      << ",\"default_warning_visibility\":"
      << bool_json(summary.default_warning_visibility)
      << ",\"no_v3c_performance_gate_authority\":"
      << bool_json(summary.no_v3c_performance_gate_authority)
      << ",\"readiness_syntax_distinct_from_performance_acceptance\":"
      << bool_json(
             summary.readiness_syntax_distinct_from_performance_acceptance)
      << ",\"failure_cases_fail_closed_named\":"
      << bool_json(summary.failure_cases_fail_closed_named)
      << ",\"mean_nll_warning_only_until_uncertainty\":"
      << bool_json(summary.mean_nll_warning_only_until_uncertainty)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"supported_metrics\":"
      << string_array_json(summary.supported_metrics)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_representation_geometry_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"metric\":" << json_quote(vocabulary[i].metric)
        << ",\"statistic_family\":"
        << json_quote(vocabulary[i].statistic_family)
        << ",\"required_runtime_fields\":"
        << string_array_json(vocabulary[i].required_runtime_fields)
        << ",\"unit\":" << json_quote(vocabulary[i].unit)
        << ",\"threshold_direction\":"
        << json_quote(vocabulary[i].threshold_direction)
        << ",\"warning_kind\":" << json_quote(vocabulary[i].warning_kind)
        << ",\"readiness_effect\":"
        << json_quote(vocabulary[i].readiness_effect)
        << ",\"v1_authority\":" << json_quote(vocabulary[i].v1_authority)
        << ",\"available_in_v1\":" << bool_json(vocabulary[i].available_in_v1)
        << ",\"geometry_metric\":" << bool_json(vocabulary[i].geometry_metric)
        << ",\"performance_gate\":" << bool_json(vocabulary[i].performance_gate)
        << ",\"future_hard_gate_candidate\":"
        << bool_json(vocabulary[i].future_hard_gate_candidate) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_summary_json() {
  const auto summary = target::lattice_representation_geometry_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"metric_count\":" << summary.metric_count
      << ",\"available_in_v1_count\":" << summary.available_in_v1_count
      << ",\"geometry_metric_count\":" << summary.geometry_metric_count
      << ",\"performance_gate_count\":" << summary.performance_gate_count
      << ",\"future_hard_gate_candidate_count\":"
      << summary.future_hard_gate_candidate_count
      << ",\"non_blocking_warning_count\":"
      << summary.non_blocking_warning_count
      << ",\"representation_health_authority_count\":"
      << summary.representation_health_authority_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"loss_component_metrics_present\":"
      << bool_json(summary.loss_component_metrics_present)
      << ",\"optimization_health_metrics_present\":"
      << bool_json(summary.optimization_health_metrics_present)
      << ",\"projection_support_metric_present\":"
      << bool_json(summary.projection_support_metric_present)
      << ",\"adapter_support_metric_present\":"
      << bool_json(summary.adapter_support_metric_present)
      << ",\"augmented_view_support_metrics_present\":"
      << bool_json(summary.augmented_view_support_metrics_present)
      << ",\"finite_parameter_check_present\":"
      << bool_json(summary.finite_parameter_check_present)
      << ",\"embedding_spectrum_geometry_present\":"
      << bool_json(summary.embedding_spectrum_geometry_present)
      << ",\"threshold_directions_present\":"
      << bool_json(summary.threshold_directions_present)
      << ",\"all_metrics_available_in_v1\":"
      << bool_json(summary.all_metrics_available_in_v1)
      << ",\"all_metrics_non_blocking_warnings\":"
      << bool_json(summary.all_metrics_non_blocking_warnings)
      << ",\"all_metrics_use_representation_health_authority\":"
      << bool_json(summary.all_metrics_use_representation_health_authority)
      << ",\"no_performance_gates\":" << bool_json(summary.no_performance_gates)
      << ",\"future_gate_candidates_are_not_active_gates\":"
      << bool_json(summary.future_gate_candidates_are_not_active_gates)
      << ",\"all_metrics_have_runtime_fields\":"
      << bool_json(summary.all_metrics_have_runtime_fields)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"metric_names\":" << string_array_json(summary.metric_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_review_metric_json(
    const target::lattice_representation_geometry_gate_review_metric_t
        &metric) {
  std::ostringstream out;
  out << "{\"metric\":" << json_quote(metric.metric)
      << ",\"unit\":" << json_quote(metric.unit)
      << ",\"future_hard_gate_candidate\":"
      << bool_json(metric.future_hard_gate_candidate)
      << ",\"geometry_metric\":" << bool_json(metric.geometry_metric)
      << ",\"observed_count\":" << metric.observed_count
      << ",\"min_value\":" << double_json(metric.min_value)
      << ",\"max_value\":" << double_json(metric.max_value)
      << ",\"mean_value\":" << double_json(metric.mean_value)
      << ",\"observed_distribution_available\":"
      << bool_json(metric.observed_distribution_available)
      << ",\"proposed_threshold\":" << bool_json(metric.proposed_threshold)
      << ",\"promoted_hard_gate\":" << bool_json(metric.promoted_hard_gate)
      << "}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_review_metrics_json(
    const std::vector<
        target::lattice_representation_geometry_gate_review_metric_t>
        &metrics) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < metrics.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << representation_geometry_gate_review_metric_json(metrics[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_review_summary_json(
    const target::lattice_representation_geometry_gate_review_summary_t
        &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"metric_count\":" << summary.metric_count
      << ",\"geometry_metric_count\":" << summary.geometry_metric_count
      << ",\"future_hard_gate_candidate_count\":"
      << summary.future_hard_gate_candidate_count
      << ",\"observed_run_count\":" << summary.observed_run_count
      << ",\"observed_geometry_fact_count\":"
      << summary.observed_geometry_fact_count
      << ",\"observed_candidate_metric_count\":"
      << summary.observed_candidate_metric_count
      << ",\"observed_metric_sample_count\":"
      << summary.observed_metric_sample_count
      << ",\"missing_geometry_fact_count\":"
      << summary.missing_geometry_fact_count
      << ",\"proposed_threshold_count\":" << summary.proposed_threshold_count
      << ",\"promoted_hard_gate_count\":" << summary.promoted_hard_gate_count
      << ",\"opt_in_target_syntax_required\":"
      << bool_json(summary.opt_in_target_syntax_required)
      << ",\"missing_geometry_fails_closed_for_gate\":"
      << bool_json(summary.missing_geometry_fails_closed_for_gate)
      << ",\"hard_gate_default_enabled\":"
      << bool_json(summary.hard_gate_default_enabled)
      << ",\"thresholds_justified_by_observed_data\":"
      << bool_json(summary.thresholds_justified_by_observed_data)
      << ",\"default_readiness_unchanged\":"
      << bool_json(summary.default_readiness_unchanged)
      << ",\"no_performance_gate_authority\":"
      << bool_json(summary.no_performance_gate_authority)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"metric_summaries\":"
      << representation_geometry_gate_review_metrics_json(
             summary.metric_summaries)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string representation_geometry_gate_review_summary_json() {
  return representation_geometry_gate_review_summary_json(
      target::summarize_representation_geometry_gate_review({}));
}

[[nodiscard]] std::string performance_uncertainty_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_performance_uncertainty_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"statistic_family\":"
        << json_quote(vocabulary[i].statistic_family)
        << ",\"applies_to\":" << string_array_json(vocabulary[i].applies_to)
        << ",\"estimator\":" << json_quote(vocabulary[i].estimator)
        << ",\"confidence_policy\":"
        << json_quote(vocabulary[i].confidence_policy)
        << ",\"required_runtime_fields\":"
        << string_array_json(vocabulary[i].required_runtime_fields)
        << ",\"gate_rule\":" << json_quote(vocabulary[i].gate_rule)
        << ",\"v1_scope\":" << json_quote(vocabulary[i].v1_scope)
        << ",\"future_requirement\":"
        << json_quote(vocabulary[i].future_requirement)
        << ",\"available_in_v1\":" << bool_json(vocabulary[i].available_in_v1)
        << ",\"performance_gate_authority\":"
        << bool_json(vocabulary[i].performance_gate_authority)
        << ",\"point_estimate_gate_allowed\":"
        << bool_json(vocabulary[i].point_estimate_gate_allowed)
        << ",\"confidence_bound_required\":"
        << bool_json(vocabulary[i].confidence_bound_required)
        << ",\"selection_leakage_policy_required\":"
        << bool_json(vocabulary[i].selection_leakage_policy_required) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string performance_uncertainty_policy_summary_json() {
  const auto summary = target::lattice_performance_uncertainty_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"available_in_v1_count\":" << summary.available_in_v1_count
      << ",\"deferred_future_count\":" << summary.deferred_future_count
      << ",\"performance_gate_authority_count\":"
      << summary.performance_gate_authority_count
      << ",\"point_estimate_gate_allowed_count\":"
      << summary.point_estimate_gate_allowed_count
      << ",\"confidence_bound_required_count\":"
      << summary.confidence_bound_required_count
      << ",\"selection_leakage_policy_required_count\":"
      << summary.selection_leakage_policy_required_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"support_fraction_wilson_v1_available\":"
      << bool_json(summary.support_fraction_wilson_v1_available)
      << ",\"proper_scoring_loss_deferred\":"
      << bool_json(summary.proper_scoring_loss_deferred)
      << ",\"calibration_fraction_deferred\":"
      << bool_json(summary.calibration_fraction_deferred)
      << ",\"pit_uniformity_deferred\":"
      << bool_json(summary.pit_uniformity_deferred)
      << ",\"node_stratified_performance_deferred\":"
      << bool_json(summary.node_stratified_performance_deferred)
      << ",\"selection_safe_gate_policy_deferred\":"
      << bool_json(summary.selection_safe_gate_policy_deferred)
      << ",\"all_policies_require_confidence_bounds\":"
      << bool_json(summary.all_policies_require_confidence_bounds)
      << ",\"all_policies_require_selection_leakage_policy\":"
      << bool_json(summary.all_policies_require_selection_leakage_policy)
      << ",\"no_point_estimate_gates\":"
      << bool_json(summary.no_point_estimate_gates)
      << ",\"no_v1_performance_gate_authority\":"
      << bool_json(summary.no_v1_performance_gate_authority)
      << ",\"future_policies_deferred\":"
      << bool_json(summary.future_policies_deferred)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"policy_names\":" << string_array_json(summary.policy_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string mdn_distribution_calibration_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_mdn_distribution_calibration_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"metric\":" << json_quote(vocabulary[i].metric)
        << ",\"statistic_family\":"
        << json_quote(vocabulary[i].statistic_family)
        << ",\"required_runtime_fields\":"
        << string_array_json(vocabulary[i].required_runtime_fields)
        << ",\"unit\":" << json_quote(vocabulary[i].unit)
        << ",\"threshold_direction\":"
        << json_quote(vocabulary[i].threshold_direction)
        << ",\"warning_kind\":" << json_quote(vocabulary[i].warning_kind)
        << ",\"readiness_effect\":"
        << json_quote(vocabulary[i].readiness_effect)
        << ",\"uncertainty_method\":"
        << json_quote(vocabulary[i].uncertainty_method)
        << ",\"available_in_v1\":" << bool_json(vocabulary[i].available_in_v1)
        << ",\"performance_gate\":" << bool_json(vocabulary[i].performance_gate)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string mdn_distribution_calibration_summary_json() {
  const auto summary = target::lattice_mdn_distribution_calibration_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"metric_count\":" << summary.metric_count
      << ",\"available_in_v1_count\":" << summary.available_in_v1_count
      << ",\"deferred_future_count\":" << summary.deferred_future_count
      << ",\"performance_gate_count\":" << summary.performance_gate_count
      << ",\"non_blocking_warning_count\":"
      << summary.non_blocking_warning_count
      << ",\"above_threshold_direction_count\":"
      << summary.above_threshold_direction_count
      << ",\"future_uncertainty_method_count\":"
      << summary.future_uncertainty_method_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"mean_nll_available_in_v1\":"
      << bool_json(summary.mean_nll_available_in_v1)
      << ",\"crps_deferred_future\":" << bool_json(summary.crps_deferred_future)
      << ",\"pit_ks_deferred_future\":"
      << bool_json(summary.pit_ks_deferred_future)
      << ",\"interval_calibration_deferred_future\":"
      << bool_json(summary.interval_calibration_deferred_future)
      << ",\"tail_calibration_deferred_future\":"
      << bool_json(summary.tail_calibration_deferred_future)
      << ",\"calibration_slope_deferred_future\":"
      << bool_json(summary.calibration_slope_deferred_future)
      << ",\"all_metrics_non_blocking_warnings\":"
      << bool_json(summary.all_metrics_non_blocking_warnings)
      << ",\"all_metrics_high_bad_above\":"
      << bool_json(summary.all_metrics_high_bad_above)
      << ",\"no_performance_gates\":" << bool_json(summary.no_performance_gates)
      << ",\"future_metrics_require_uncertainty_methods\":"
      << bool_json(summary.future_metrics_require_uncertainty_methods)
      << ",\"only_mean_nll_available_in_v1\":"
      << bool_json(summary.only_mean_nll_available_in_v1)
      << ",\"all_metrics_have_runtime_fields\":"
      << bool_json(summary.all_metrics_have_runtime_fields)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"metric_names\":" << string_array_json(summary.metric_names)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string
mdn_distribution_calibration_diagnostic_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_mdn_distribution_calibration_diagnostic_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"metric\":" << json_quote(vocabulary[i].metric)
        << ",\"diagnostic_family\":"
        << json_quote(vocabulary[i].diagnostic_family)
        << ",\"evidence_basis\":" << json_quote(vocabulary[i].evidence_basis)
        << ",\"runtime_fields\":"
        << string_array_json(vocabulary[i].runtime_fields)
        << ",\"sample_count_field\":"
        << json_quote(vocabulary[i].sample_count_field)
        << ",\"point_estimate_field\":"
        << json_quote(vocabulary[i].point_estimate_field)
        << ",\"uncertainty_method\":"
        << json_quote(vocabulary[i].uncertainty_method)
        << ",\"uncertainty_bound_fields\":"
        << string_array_json(vocabulary[i].uncertainty_bound_fields)
        << ",\"checkpoint_binding\":"
        << json_quote(vocabulary[i].checkpoint_binding)
        << ",\"split_binding\":" << json_quote(vocabulary[i].split_binding)
        << ",\"active_identity_binding\":"
        << json_quote(vocabulary[i].active_identity_binding)
        << ",\"warning_metric\":" << json_quote(vocabulary[i].warning_metric)
        << ",\"availability\":" << json_quote(vocabulary[i].availability)
        << ",\"validation_run_binding_required\":"
        << bool_json(vocabulary[i].validation_run_binding_required)
        << ",\"exact_mdn_checkpoint_required\":"
        << bool_json(vocabulary[i].exact_mdn_checkpoint_required)
        << ",\"representation_checkpoint_required\":"
        << bool_json(vocabulary[i].representation_checkpoint_required)
        << ",\"split_policy_identity_required\":"
        << bool_json(vocabulary[i].split_policy_identity_required)
        << ",\"active_identity_required\":"
        << bool_json(vocabulary[i].active_identity_required)
        << ",\"warning_only\":" << bool_json(vocabulary[i].warning_only)
        << ",\"performance_gate\":" << bool_json(vocabulary[i].performance_gate)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
mdn_distribution_calibration_diagnostic_summary_json() {
  const auto summary =
      target::lattice_mdn_distribution_calibration_diagnostic_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"diagnostic_count\":" << summary.diagnostic_count
      << ",\"available_metric_count\":" << summary.available_metric_count
      << ",\"future_metric_count\":" << summary.future_metric_count
      << ",\"warning_only_count\":" << summary.warning_only_count
      << ",\"performance_gate_count\":" << summary.performance_gate_count
      << ",\"validation_binding_required_count\":"
      << summary.validation_binding_required_count
      << ",\"exact_mdn_checkpoint_required_count\":"
      << summary.exact_mdn_checkpoint_required_count
      << ",\"representation_checkpoint_required_count\":"
      << summary.representation_checkpoint_required_count
      << ",\"split_policy_identity_required_count\":"
      << summary.split_policy_identity_required_count
      << ",\"active_identity_required_count\":"
      << summary.active_identity_required_count
      << ",\"point_estimate_only_count\":" << summary.point_estimate_only_count
      << ",\"future_uncertainty_count\":" << summary.future_uncertainty_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"finite_first_set_declared\":"
      << bool_json(summary.finite_first_set_declared)
      << ",\"proper_scoring_metrics_present\":"
      << bool_json(summary.proper_scoring_metrics_present)
      << ",\"pit_histogram_summary_deferred\":"
      << bool_json(summary.pit_histogram_summary_deferred)
      << ",\"interval_and_tail_coverage_deferred\":"
      << bool_json(summary.interval_and_tail_coverage_deferred)
      << ",\"per_node_calibration_present\":"
      << bool_json(summary.per_node_calibration_present)
      << ",\"all_diagnostics_bind_identity_and_checkpoints\":"
      << bool_json(summary.all_diagnostics_bind_identity_and_checkpoints)
      << ",\"all_diagnostics_warning_only\":"
      << bool_json(summary.all_diagnostics_warning_only)
      << ",\"no_hard_calibration_gates\":"
      << bool_json(summary.no_hard_calibration_gates)
      << ",\"available_point_estimates_are_warning_only\":"
      << bool_json(summary.available_point_estimates_are_warning_only)
      << ",\"future_metrics_have_uncertainty_methods\":"
      << bool_json(summary.future_metrics_have_uncertainty_methods)
      << ",\"all_diagnostics_have_runtime_fields\":"
      << bool_json(summary.all_diagnostics_have_runtime_fields)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"diagnostic_metrics\":"
      << string_array_json(summary.diagnostic_metrics)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string target_numeric_dimension_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_target_numeric_dimension_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"context\":" << json_quote(vocabulary[i].context)
        << ",\"field\":" << json_quote(vocabulary[i].field)
        << ",\"unit\":" << json_quote(vocabulary[i].unit)
        << ",\"numeric_kind\":" << json_quote(vocabulary[i].numeric_kind)
        << ",\"has_minimum\":" << bool_json(vocabulary[i].has_minimum)
        << ",\"minimum\":";
    if (vocabulary[i].has_minimum) {
      out << double_json(vocabulary[i].minimum);
    } else {
      out << "null";
    }
    out << ",\"has_maximum\":" << bool_json(vocabulary[i].has_maximum)
        << ",\"maximum\":";
    if (vocabulary[i].has_maximum) {
      out << double_json(vocabulary[i].maximum);
    } else {
      out << "null";
    }
    out << ",\"integral\":" << bool_json(vocabulary[i].integral)
        << ",\"threshold_direction\":"
        << json_quote(vocabulary[i].threshold_direction)
        << ",\"validation_rule\":" << json_quote(vocabulary[i].validation_rule)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string target_numeric_dimension_summary_json() {
  const auto summary = target::lattice_target_numeric_dimension_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"dimension_count\":" << summary.dimension_count
      << ",\"unit_count\":" << summary.unit_count
      << ",\"numeric_kind_count\":" << summary.numeric_kind_count
      << ",\"closed_unit_interval_count\":"
      << summary.closed_unit_interval_count
      << ",\"non_negative_integer_count\":"
      << summary.non_negative_integer_count
      << ",\"non_negative_real_count\":" << summary.non_negative_real_count
      << ",\"real_at_least_one_count\":" << summary.real_at_least_one_count
      << ",\"integral_count\":" << summary.integral_count
      << ",\"bounded_unit_interval_count\":"
      << summary.bounded_unit_interval_count
      << ",\"minimum_direction_count\":" << summary.minimum_direction_count
      << ",\"above_direction_count\":" << summary.above_direction_count
      << ",\"below_direction_count\":" << summary.below_direction_count
      << ",\"metric_declared_direction_count\":"
      << summary.metric_declared_direction_count
      << ",\"planning_budget_direction_count\":"
      << summary.planning_budget_direction_count
      << ",\"empty_field_count\":" << summary.empty_field_count
      << ",\"malformed_bound_count\":" << summary.malformed_bound_count
      << ",\"duplicate_context_field_count\":"
      << summary.duplicate_context_field_count
      << ",\"all_dimensions_have_units\":"
      << bool_json(summary.all_dimensions_have_units)
      << ",\"all_dimensions_have_numeric_kind\":"
      << bool_json(summary.all_dimensions_have_numeric_kind)
      << ",\"all_context_fields_unique\":"
      << bool_json(summary.all_context_fields_unique)
      << ",\"bounds_are_well_formed\":"
      << bool_json(summary.bounds_are_well_formed)
      << ",\"unit_interval_rows_are_bounded\":"
      << bool_json(summary.unit_interval_rows_are_bounded)
      << ",\"integral_rows_are_count_like\":"
      << bool_json(summary.integral_rows_are_count_like)
      << ",\"threshold_directions_are_known\":"
      << bool_json(summary.threshold_directions_are_known)
      << ",\"coverage_cursor_epochs_dimension_present\":"
      << bool_json(summary.coverage_cursor_epochs_dimension_present)
      << ",\"repeated_load_cursor_epoch_dimension_present\":"
      << bool_json(summary.repeated_load_cursor_epoch_dimension_present)
      << ",\"warning_anchor_fraction_dimension_present\":"
      << bool_json(summary.warning_anchor_fraction_dimension_present)
      << ",\"representation_condition_number_dimension_present\":"
      << bool_json(summary.representation_condition_number_dimension_present)
      << ",\"node_support_floor_count_dimension_present\":"
      << bool_json(summary.node_support_floor_count_dimension_present)
      << ",\"mdn_mean_nll_dimension_present\":"
      << bool_json(summary.mdn_mean_nll_dimension_present)
      << ",\"coverage_and_load_units_separated\":"
      << bool_json(summary.coverage_and_load_units_separated)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"units\":" << string_array_json(summary.units)
      << ",\"numeric_kinds\":" << string_array_json(summary.numeric_kinds)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string monotonicity_invariant_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_monotonicity_invariant_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"invariant\":" << json_quote(vocabulary[i].invariant)
        << ",\"scope\":" << json_quote(vocabulary[i].scope)
        << ",\"order\":" << json_quote(vocabulary[i].order)
        << ",\"stronger_dimensions\":"
        << string_array_json(vocabulary[i].stronger_dimensions)
        << ",\"weaker_dimensions\":"
        << string_array_json(vocabulary[i].weaker_dimensions)
        << ",\"neutral_dimensions\":"
        << string_array_json(vocabulary[i].neutral_dimensions)
        << ",\"clean_growth_condition\":"
        << json_quote(vocabulary[i].clean_growth_condition)
        << ",\"failure_mode\":" << json_quote(vocabulary[i].failure_mode)
        << ",\"proof_basis\":" << json_quote(vocabulary[i].proof_basis)
        << ",\"target_status_monotone\":"
        << bool_json(vocabulary[i].target_status_monotone) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string proof_obligation_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_proof_obligation_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"obligation\":" << json_quote(vocabulary[i].obligation)
        << ",\"certificate_field\":"
        << json_quote(vocabulary[i].certificate_field)
        << ",\"derived_relation\":"
        << json_quote(vocabulary[i].derived_relation)
        << ",\"required_scope\":" << json_quote(vocabulary[i].required_scope)
        << ",\"failure_status\":" << json_quote(vocabulary[i].failure_status)
        << ",\"deficit_kind\":" << json_quote(vocabulary[i].deficit_kind)
        << ",\"non_claiming_condition\":"
        << json_quote(vocabulary[i].non_claiming_condition)
        << ",\"self_check_basis\":"
        << json_quote(vocabulary[i].self_check_basis)
        << ",\"required_for_target_satisfaction\":"
        << bool_json(vocabulary[i].required_for_target_satisfaction)
        << ",\"contributes_to_certificate_digest\":"
        << bool_json(vocabulary[i].contributes_to_certificate_digest)
        << ",\"planner_relevant\":" << bool_json(vocabulary[i].planner_relevant)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string proof_certificate_digest_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_proof_certificate_digest_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"digest_role\":" << json_quote(vocabulary[i].digest_role)
        << ",\"included_fields\":" << json_quote(vocabulary[i].included_fields)
        << ",\"excluded_fields\":" << json_quote(vocabulary[i].excluded_fields)
        << ",\"self_check_rule\":" << json_quote(vocabulary[i].self_check_rule)
        << ",\"boundary\":" << json_quote(vocabulary[i].boundary)
        << ",\"contributes_to_certificate_digest\":"
        << bool_json(vocabulary[i].contributes_to_certificate_digest)
        << ",\"status_authority\":" << bool_json(vocabulary[i].status_authority)
        << ",\"advisory_only\":" << bool_json(vocabulary[i].advisory_only)
        << ",\"visibility_only\":" << bool_json(vocabulary[i].visibility_only)
        << ",\"audit_only\":" << bool_json(vocabulary[i].audit_only) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string proof_certificate_digest_policy_summary_json() {
  const auto summary =
      target::lattice_proof_certificate_digest_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"digest_contributing_count\":" << summary.digest_contributing_count
      << ",\"digest_excluded_count\":" << summary.digest_excluded_count
      << ",\"status_authority_count\":" << summary.status_authority_count
      << ",\"advisory_only_count\":" << summary.advisory_only_count
      << ",\"visibility_only_count\":" << summary.visibility_only_count
      << ",\"audit_only_count\":" << summary.audit_only_count
      << ",\"digest_non_status_authority_count\":"
      << summary.digest_non_status_authority_count
      << ",\"status_authority_not_digest_count\":"
      << summary.status_authority_not_digest_count
      << ",\"advisory_digest_overlap_count\":"
      << summary.advisory_digest_overlap_count
      << ",\"status_authority_visibility_overlap_count\":"
      << summary.status_authority_visibility_overlap_count
      << ",\"empty_policy_field_count\":" << summary.empty_policy_field_count
      << ",\"all_required_surfaces_present\":"
      << bool_json(summary.all_required_surfaces_present)
      << ",\"digest_has_core_status_authority_surfaces\":"
      << bool_json(summary.digest_has_core_status_authority_surfaces)
      << ",\"certificate_digest_self_excluded\":"
      << bool_json(summary.certificate_digest_self_excluded)
      << ",\"advisory_report_surfaces_outside_digest\":"
      << bool_json(summary.advisory_report_surfaces_outside_digest)
      << ",\"visibility_surfaces_do_not_drive_status\":"
      << bool_json(summary.visibility_surfaces_do_not_drive_status)
      << ",\"digest_contributors_have_self_checks\":"
      << bool_json(summary.digest_contributors_have_self_checks)
      << ",\"all_policies_have_boundary_text\":"
      << bool_json(summary.all_policies_have_boundary_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"digest_contributing_surfaces\":"
      << string_array_json(summary.digest_contributing_surfaces)
      << ",\"digest_excluded_surfaces\":"
      << string_array_json(summary.digest_excluded_surfaces)
      << ",\"missing_required_surfaces\":"
      << string_array_json(summary.missing_required_surfaces)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string checkpoint_identity_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_checkpoint_identity_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"v1_authority\":" << json_quote(vocabulary[i].v1_authority)
        << ",\"preview_fields\":"
        << string_array_json(vocabulary[i].preview_fields)
        << ",\"binding_requirement\":"
        << json_quote(vocabulary[i].binding_requirement)
        << ",\"stale_sidecar_policy\":"
        << json_quote(vocabulary[i].stale_sidecar_policy)
        << ",\"v2_promotion\":" << json_quote(vocabulary[i].v2_promotion)
        << ",\"contract_identity_effect\":"
        << json_quote(vocabulary[i].contract_identity_effect)
        << ",\"closure_authority\":"
        << bool_json(vocabulary[i].closure_authority)
        << ",\"db_cache_key_ready\":"
        << bool_json(vocabulary[i].db_cache_key_ready) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string checkpoint_selection_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_checkpoint_selection_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"selection_rule\":" << json_quote(vocabulary[i].selection_rule)
        << ",\"allowed_claim\":" << json_quote(vocabulary[i].allowed_claim)
        << ",\"forbidden_claim\":" << json_quote(vocabulary[i].forbidden_claim)
        << ",\"binding_requirement\":"
        << json_quote(vocabulary[i].binding_requirement)
        << ",\"future_extension\":"
        << json_quote(vocabulary[i].future_extension)
        << ",\"deterministic_readiness_selector\":"
        << bool_json(vocabulary[i].deterministic_readiness_selector)
        << ",\"requires_target_satisfied\":"
        << bool_json(vocabulary[i].requires_target_satisfied)
        << ",\"exact_runtime_binding_required\":"
        << bool_json(vocabulary[i].exact_runtime_binding_required)
        << ",\"performance_selector\":"
        << bool_json(vocabulary[i].performance_selector)
        << ",\"pareto_optimizer\":" << bool_json(vocabulary[i].pareto_optimizer)
        << ",\"contract_identity_authority\":"
        << bool_json(vocabulary[i].contract_identity_authority)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string checkpoint_selection_policy_summary_json() {
  const auto summary = target::lattice_checkpoint_selection_policy_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"policy_count\":" << summary.policy_count
      << ",\"latest_satisfying_surface_count\":"
      << summary.latest_satisfying_surface_count
      << ",\"deterministic_readiness_selector_count\":"
      << summary.deterministic_readiness_selector_count
      << ",\"requires_target_satisfied_count\":"
      << summary.requires_target_satisfied_count
      << ",\"exact_runtime_binding_required_count\":"
      << summary.exact_runtime_binding_required_count
      << ",\"performance_selector_count\":"
      << summary.performance_selector_count
      << ",\"pareto_optimizer_count\":" << summary.pareto_optimizer_count
      << ",\"contract_identity_authority_count\":"
      << summary.contract_identity_authority_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"empty_policy_field_count\":" << summary.empty_policy_field_count
      << ",\"all_policies_require_satisfied_target\":"
      << bool_json(summary.all_policies_require_satisfied_target)
      << ",\"latest_satisfying_is_readiness_selection\":"
      << bool_json(summary.latest_satisfying_is_readiness_selection)
      << ",\"validation_eval_requires_exact_runtime_binding\":"
      << bool_json(summary.validation_eval_requires_exact_runtime_binding)
      << ",\"plan_input_hint_is_runtime_model_state_advice\":"
      << bool_json(summary.plan_input_hint_is_runtime_model_state_advice)
      << ",\"pareto_order_is_not_checkpoint_selector\":"
      << bool_json(summary.pareto_order_is_not_checkpoint_selector)
      << ",\"no_performance_selector\":"
      << bool_json(summary.no_performance_selector)
      << ",\"no_pareto_optimizer\":" << bool_json(summary.no_pareto_optimizer)
      << ",\"no_contract_identity_authority\":"
      << bool_json(summary.no_contract_identity_authority)
      << ",\"no_runtime_executor\":" << bool_json(summary.no_runtime_executor)
      << ",\"all_policies_have_claim_text\":"
      << bool_json(summary.all_policies_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"deterministic_selector_policies\":"
      << string_array_json(summary.deterministic_selector_policies)
      << ",\"exact_runtime_binding_policies\":"
      << string_array_json(summary.exact_runtime_binding_policies)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string lattice_no_decision_authority_json() {
  return "\"model_selector\":false"
         ",\"best_model_selector\":false"
         ",\"performance_selector\":false"
         ",\"pareto_optimizer\":false"
         ",\"scalar_score_emitted\":false"
         ",\"quality_acceptance\":false"
         ",\"policy_gate\":false"
         ",\"allocation_decision\":false"
         ",\"market_readiness_decision\":false"
         ",\"deployment_decision\":false"
         ",\"automatic_deployment_decision\":false";
}

[[nodiscard]] std::string plan_advice_scope_policy_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_plan_advice_scope_policy_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"policy\":" << json_quote(vocabulary[i].policy)
        << ",\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"source_relation\":" << json_quote(vocabulary[i].source_relation)
        << ",\"allowed_claim\":" << json_quote(vocabulary[i].allowed_claim)
        << ",\"forbidden_claim\":" << json_quote(vocabulary[i].forbidden_claim)
        << ",\"execution_boundary\":"
        << json_quote(vocabulary[i].execution_boundary)
        << ",\"advice_only\":" << bool_json(vocabulary[i].advice_only)
        << ",\"evidence_authority\":"
        << bool_json(vocabulary[i].evidence_authority)
        << ",\"scheduler_authority\":"
        << bool_json(vocabulary[i].scheduler_authority)
        << ",\"lattice_executor\":" << bool_json(vocabulary[i].lattice_executor)
        << ",\"affects_contract_identity\":"
        << bool_json(vocabulary[i].affects_contract_identity)
        << ",\"attempt_budget_guard\":"
        << bool_json(vocabulary[i].attempt_budget_guard) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string operational_readiness_v1_scope_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_operational_readiness_v1_scope_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"item\":" << json_quote(vocabulary[i].item)
        << ",\"category\":" << json_quote(vocabulary[i].category)
        << ",\"authority\":" << json_quote(vocabulary[i].authority)
        << ",\"allowed_claim\":" << json_quote(vocabulary[i].allowed_claim)
        << ",\"excluded_claim\":" << json_quote(vocabulary[i].excluded_claim)
        << ",\"future_work\":" << json_quote(vocabulary[i].future_work)
        << ",\"included_in_v1\":" << bool_json(vocabulary[i].included_in_v1)
        << ",\"deferred_beyond_v1\":"
        << bool_json(vocabulary[i].deferred_beyond_v1)
        << ",\"read_only_evidence_authority\":"
        << bool_json(vocabulary[i].read_only_evidence_authority)
        << ",\"runtime_executor_authority\":"
        << bool_json(vocabulary[i].runtime_executor_authority)
        << ",\"performance_gate_authority\":"
        << bool_json(vocabulary[i].performance_gate_authority)
        << ",\"db_source_of_truth_authority\":"
        << bool_json(vocabulary[i].db_source_of_truth_authority) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string operational_readiness_v1_scope_summary_json() {
  const auto summary = target::lattice_operational_readiness_v1_scope_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"item_count\":" << summary.item_count
      << ",\"included_in_v1_count\":" << summary.included_in_v1_count
      << ",\"deferred_beyond_v1_count\":" << summary.deferred_beyond_v1_count
      << ",\"included_and_deferred_count\":"
      << summary.included_and_deferred_count
      << ",\"read_only_evidence_authority_count\":"
      << summary.read_only_evidence_authority_count
      << ",\"runtime_executor_authority_count\":"
      << summary.runtime_executor_authority_count
      << ",\"performance_gate_authority_count\":"
      << summary.performance_gate_authority_count
      << ",\"db_source_of_truth_authority_count\":"
      << summary.db_source_of_truth_authority_count
      << ",\"empty_claim_field_count\":" << summary.empty_claim_field_count
      << ",\"included_deferred_partition_complete\":"
      << bool_json(summary.included_deferred_partition_complete)
      << ",\"included_items_are_read_only\":"
      << bool_json(summary.included_items_are_read_only)
      << ",\"no_runtime_executor_authority\":"
      << bool_json(summary.no_runtime_executor_authority)
      << ",\"no_performance_gate_authority\":"
      << bool_json(summary.no_performance_gate_authority)
      << ",\"no_db_source_of_truth_authority\":"
      << bool_json(summary.no_db_source_of_truth_authority)
      << ",\"all_scope_items_have_claim_text\":"
      << bool_json(summary.all_scope_items_have_claim_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"included_items\":" << string_array_json(summary.included_items)
      << ",\"deferred_items\":" << string_array_json(summary.deferred_items)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string operational_readiness_v1_gate_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_operational_readiness_v1_gate_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"gate\":" << json_quote(vocabulary[i].gate)
        << ",\"required_evidence\":"
        << json_quote(vocabulary[i].required_evidence)
        << ",\"target_ids\":" << string_array_json(vocabulary[i].target_ids)
        << ",\"hero_surfaces\":"
        << string_array_json(vocabulary[i].hero_surfaces)
        << ",\"pass_condition\":" << json_quote(vocabulary[i].pass_condition)
        << ",\"fail_condition\":" << json_quote(vocabulary[i].fail_condition)
        << ",\"v1_boundary\":" << json_quote(vocabulary[i].v1_boundary)
        << ",\"required_for_v1\":" << bool_json(vocabulary[i].required_for_v1)
        << ",\"read_only_evidence_authority\":"
        << bool_json(vocabulary[i].read_only_evidence_authority)
        << ",\"runtime_executor_authority\":"
        << bool_json(vocabulary[i].runtime_executor_authority)
        << ",\"performance_gate_authority\":"
        << bool_json(vocabulary[i].performance_gate_authority)
        << ",\"db_source_of_truth_authority\":"
        << bool_json(vocabulary[i].db_source_of_truth_authority) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string operational_readiness_v1_gate_summary_json() {
  const auto summary = target::lattice_operational_readiness_v1_gate_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"gate_count\":" << summary.gate_count
      << ",\"required_for_v1_count\":" << summary.required_for_v1_count
      << ",\"read_only_evidence_authority_count\":"
      << summary.read_only_evidence_authority_count
      << ",\"runtime_executor_authority_count\":"
      << summary.runtime_executor_authority_count
      << ",\"performance_gate_authority_count\":"
      << summary.performance_gate_authority_count
      << ",\"db_source_of_truth_authority_count\":"
      << summary.db_source_of_truth_authority_count
      << ",\"empty_hero_surface_count\":" << summary.empty_hero_surface_count
      << ",\"empty_pass_condition_count\":"
      << summary.empty_pass_condition_count
      << ",\"empty_fail_condition_count\":"
      << summary.empty_fail_condition_count
      << ",\"unique_target_id_count\":" << summary.unique_target_id_count
      << ",\"all_gates_required_for_v1\":"
      << bool_json(summary.all_gates_required_for_v1)
      << ",\"all_gates_read_only\":" << bool_json(summary.all_gates_read_only)
      << ",\"no_runtime_executor_authority\":"
      << bool_json(summary.no_runtime_executor_authority)
      << ",\"no_performance_gate_authority\":"
      << bool_json(summary.no_performance_gate_authority)
      << ",\"no_db_source_of_truth_authority\":"
      << bool_json(summary.no_db_source_of_truth_authority)
      << ",\"all_gates_have_hero_surface\":"
      << bool_json(summary.all_gates_have_hero_surface)
      << ",\"all_gates_have_pass_fail_conditions\":"
      << bool_json(summary.all_gates_have_pass_fail_conditions)
      << ",\"all_v1_targets_referenced\":"
      << bool_json(summary.all_v1_targets_referenced)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"referenced_target_ids\":"
      << string_array_json(summary.referenced_target_ids)
      << ",\"missing_v1_target_ids\":"
      << string_array_json(summary.missing_v1_target_ids)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string contract_identity_boundary_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_contract_identity_boundary_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"surface\":" << json_quote(vocabulary[i].surface)
        << ",\"category\":" << json_quote(vocabulary[i].category)
        << ",\"participates_in\":" << json_quote(vocabulary[i].participates_in)
        << ",\"excluded_from\":" << json_quote(vocabulary[i].excluded_from)
        << ",\"proof_authority\":" << json_quote(vocabulary[i].proof_authority)
        << ",\"failure_policy\":" << json_quote(vocabulary[i].failure_policy)
        << ",\"protocol_contract_identity\":"
        << bool_json(vocabulary[i].protocol_contract_identity)
        << ",\"target_spec_identity\":"
        << bool_json(vocabulary[i].target_spec_identity)
        << ",\"active_runtime_identity\":"
        << bool_json(vocabulary[i].active_runtime_identity)
        << ",\"runtime_model_state\":"
        << bool_json(vocabulary[i].runtime_model_state)
        << ",\"plan_advice\":" << bool_json(vocabulary[i].plan_advice)
        << ",\"warning_visibility\":"
        << bool_json(vocabulary[i].warning_visibility)
        << ",\"audit_metadata\":" << bool_json(vocabulary[i].audit_metadata)
        << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string contract_identity_boundary_summary_json() {
  const auto summary = target::lattice_contract_identity_boundary_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"boundary_count\":" << summary.boundary_count
      << ",\"protocol_contract_identity_count\":"
      << summary.protocol_contract_identity_count
      << ",\"target_spec_identity_count\":"
      << summary.target_spec_identity_count
      << ",\"active_runtime_identity_count\":"
      << summary.active_runtime_identity_count
      << ",\"runtime_model_state_count\":" << summary.runtime_model_state_count
      << ",\"plan_advice_count\":" << summary.plan_advice_count
      << ",\"warning_visibility_count\":" << summary.warning_visibility_count
      << ",\"audit_metadata_count\":" << summary.audit_metadata_count
      << ",\"runtime_model_state_identity_overlap_count\":"
      << summary.runtime_model_state_identity_overlap_count
      << ",\"plan_advice_identity_overlap_count\":"
      << summary.plan_advice_identity_overlap_count
      << ",\"warning_identity_overlap_count\":"
      << summary.warning_identity_overlap_count
      << ",\"audit_identity_overlap_count\":"
      << summary.audit_identity_overlap_count
      << ",\"empty_policy_field_count\":" << summary.empty_policy_field_count
      << ",\"model_state_is_not_contract_identity\":"
      << bool_json(summary.model_state_is_not_contract_identity)
      << ",\"advice_warning_audit_not_identity\":"
      << bool_json(summary.advice_warning_audit_not_identity)
      << ",\"active_identity_surfaces_present\":"
      << bool_json(summary.active_identity_surfaces_present)
      << ",\"proof_identity_surfaces_present\":"
      << bool_json(summary.proof_identity_surfaces_present)
      << ",\"all_boundaries_have_policy_text\":"
      << bool_json(summary.all_boundaries_have_policy_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"missing_required_surfaces\":"
      << string_array_json(summary.missing_required_surfaces)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string mathematical_readiness_v1_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary =
      target::lattice_mathematical_readiness_v1_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"item\":" << json_quote(vocabulary[i].item)
        << ",\"mathematical_claim\":"
        << json_quote(vocabulary[i].mathematical_claim) << ",\"hero_surfaces\":"
        << string_array_json(vocabulary[i].hero_surfaces)
        << ",\"v1_status\":" << json_quote(vocabulary[i].v1_status)
        << ",\"boundary\":" << json_quote(vocabulary[i].boundary)
        << ",\"proof_basis\":" << json_quote(vocabulary[i].proof_basis)
        << ",\"future_work\":" << json_quote(vocabulary[i].future_work)
        << ",\"included_in_v1\":" << bool_json(vocabulary[i].included_in_v1)
        << ",\"deferred_beyond_v1\":"
        << bool_json(vocabulary[i].deferred_beyond_v1)
        << ",\"read_only\":" << bool_json(vocabulary[i].read_only)
        << ",\"runtime_executor\":" << bool_json(vocabulary[i].runtime_executor)
        << ",\"performance_gate\":" << bool_json(vocabulary[i].performance_gate)
        << ",\"db_source_of_truth\":"
        << bool_json(vocabulary[i].db_source_of_truth) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string mathematical_readiness_v1_summary_json() {
  const auto summary = target::lattice_mathematical_readiness_v1_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"item_count\":" << summary.item_count
      << ",\"included_in_v1_count\":" << summary.included_in_v1_count
      << ",\"deferred_beyond_v1_count\":" << summary.deferred_beyond_v1_count
      << ",\"read_only_count\":" << summary.read_only_count
      << ",\"runtime_executor_count\":" << summary.runtime_executor_count
      << ",\"performance_gate_count\":" << summary.performance_gate_count
      << ",\"db_source_of_truth_count\":" << summary.db_source_of_truth_count
      << ",\"empty_hero_surface_count\":" << summary.empty_hero_surface_count
      << ",\"all_items_included_in_v1\":"
      << bool_json(summary.all_items_included_in_v1)
      << ",\"no_items_deferred_beyond_v1\":"
      << bool_json(summary.no_items_deferred_beyond_v1)
      << ",\"all_items_read_only\":" << bool_json(summary.all_items_read_only)
      << ",\"no_runtime_executor\":" << bool_json(summary.no_runtime_executor)
      << ",\"no_performance_gate\":" << bool_json(summary.no_performance_gate)
      << ",\"no_db_source_of_truth\":"
      << bool_json(summary.no_db_source_of_truth)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string leakage_proof_json(
    const target::lattice_target_proof_certificate_t::leakage_proof_t &proof) {
  std::ostringstream out;
  out << "{\"checked\":" << bool_json(proof.checked)
      << ",\"rule\":" << json_quote(proof.rule)
      << ",\"split_name\":" << json_quote(proof.split_name)
      << ",\"base_range\":" << interval_json(proof.base_range)
      << ",\"protected_range\":" << interval_json(proof.protected_range)
      << ",\"dilation_left\":" << proof.dilation_left
      << ",\"dilation_right\":" << proof.dilation_right
      << ",\"forbidden_uses\":" << string_array_json(proof.forbidden_uses)
      << ",\"require_mutated_component\":"
      << bool_json(proof.require_mutated_component)
      << ",\"overlap_found\":" << bool_json(proof.overlap_found)
      << ",\"overlap_witnesses\":"
      << leakage_overlap_witnesses_json(proof.overlap_witnesses)
      << ",\"passed\":" << bool_json(proof.passed) << "}";
  return out.str();
}

[[nodiscard]] std::string proof_certificate_json(
    const target::lattice_target_proof_certificate_t &proof) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(proof.schema) << ",\"certificate_ref\":"
      << json_quote(display::certificate_ref(proof.certificate_digest))
      << ",\"certificate_digest\":" << json_quote(proof.certificate_digest)
      << ",\"target_id\":" << json_quote(proof.target_id)
      << ",\"target_spec_fingerprint\":"
      << json_quote(proof.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << json_quote(proof.split_policy_fingerprint)
      << ",\"proof_context\":" << proof_context_json(proof.proof_context)
      << ",\"dependencies\":" << dependency_proofs_json(proof.dependencies)
      << ",\"artifacts\":" << artifact_proofs_json(proof.artifacts)
      << ",\"coverage\":" << coverage_proofs_json(proof.coverage)
      << ",\"closure\":" << closure_proof_json(proof.closure)
      << ",\"leakage\":" << leakage_proof_json(proof.leakage)
      << ",\"node_support_summaries\":"
      << node_support_summaries_json(proof.node_support_summaries) << "}";
  return out.str();
}

[[nodiscard]] std::string proof_certificate_check_json(
    const target::lattice_target_proof_certificate_check_t &check) {
  std::ostringstream out;
  out << "{\"passed\":" << bool_json(check.passed)
      << ",\"issues\":" << string_array_json(check.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string proof_deficit_json(
    const target::lattice_target_evaluation_t::proof_deficit_t &deficit) {
  std::ostringstream out;
  out << "{\"key\":" << json_quote(deficit.key)
      << ",\"kind\":" << json_quote(deficit.kind)
      << ",\"dimension\":" << json_quote(deficit.dimension)
      << ",\"status\":" << json_quote(deficit.status)
      << ",\"priority\":" << deficit.priority
      << ",\"priority_class\":" << json_quote(deficit.priority_class)
      << ",\"message\":" << json_quote(deficit.message)
      << ",\"required\":" << double_json(deficit.required)
      << ",\"actual\":" << double_json(deficit.actual)
      << ",\"unit\":" << json_quote(deficit.unit)
      << ",\"range\":" << interval_json(deficit.range)
      << ",\"missing_intervals\":"
      << interval_array_json(deficit.missing_intervals)
      << ",\"plan_relevant\":" << bool_json(deficit.plan_relevant)
      << ",\"related_fact_integrity_issue_codes\":"
      << string_array_json(deficit.related_fact_integrity_issue_codes) << "}";
  return out.str();
}

[[nodiscard]] std::string proof_deficits_json(
    const std::vector<target::lattice_target_evaluation_t::proof_deficit_t>
        &deficits) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < deficits.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << proof_deficit_json(deficits[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evaluation_fact_integrity_summary_json(
    const target::lattice_target_evaluation_t &eval) {
  if (!eval.fact_integrity_summary_available) {
    return "null";
  }
  return fact_integrity_summary_json(eval.fact_integrity_summary);
}

[[nodiscard]] std::string deficit_priority_vocabulary_json() {
  std::ostringstream out;
  out << "{\"order\":\"lower_priority_is_primary\",\"classes\":[";
  const auto vocabulary = target::lattice_deficit_priority_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"priority_class\":" << json_quote(vocabulary[i].priority_class)
        << ",\"priority\":" << vocabulary[i].priority << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string deficit_vector_planning_summary_json() {
  const auto summary = target::lattice_deficit_vector_planning_summary();
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"deficit_priority_class_count\":"
      << summary.deficit_priority_class_count
      << ",\"plan_advice_policy_count\":" << summary.plan_advice_policy_count
      << ",\"plan_advice_only_count\":" << summary.plan_advice_only_count
      << ",\"evidence_authority_count\":" << summary.evidence_authority_count
      << ",\"scheduler_authority_count\":" << summary.scheduler_authority_count
      << ",\"lattice_executor_count\":" << summary.lattice_executor_count
      << ",\"affects_contract_identity_count\":"
      << summary.affects_contract_identity_count
      << ",\"attempt_budget_guard_count\":"
      << summary.attempt_budget_guard_count
      << ",\"duplicate_priority_count\":" << summary.duplicate_priority_count
      << ",\"empty_policy_field_count\":" << summary.empty_policy_field_count
      << ",\"lower_priority_is_primary\":"
      << bool_json(summary.lower_priority_is_primary)
      << ",\"priority_order_strictly_increasing\":"
      << bool_json(summary.priority_order_strictly_increasing)
      << ",\"fallback_priority_is_terminal\":"
      << bool_json(summary.fallback_priority_is_terminal)
      << ",\"checkpoint_binding_outranks_dependency\":"
      << bool_json(summary.checkpoint_binding_outranks_dependency)
      << ",\"identity_outranks_certificate\":"
      << bool_json(summary.identity_outranks_certificate)
      << ",\"leakage_outranks_coverage\":"
      << bool_json(summary.leakage_outranks_coverage)
      << ",\"metric_outranks_status\":"
      << bool_json(summary.metric_outranks_status)
      << ",\"plan_basis_advice_present\":"
      << bool_json(summary.plan_basis_advice_present)
      << ",\"suggested_wave_advice_present\":"
      << bool_json(summary.suggested_wave_advice_present)
      << ",\"plan_input_model_state_hint_present\":"
      << bool_json(summary.plan_input_model_state_hint_present)
      << ",\"attempt_budget_guard_present\":"
      << bool_json(summary.attempt_budget_guard_present)
      << ",\"runtime_hero_executor_boundary_present\":"
      << bool_json(summary.runtime_hero_executor_boundary_present)
      << ",\"plan_surfaces_are_advice_not_evidence\":"
      << bool_json(summary.plan_surfaces_are_advice_not_evidence)
      << ",\"runtime_hero_is_only_executor\":"
      << bool_json(summary.runtime_hero_is_only_executor)
      << ",\"plan_advice_does_not_affect_contract_identity\":"
      << bool_json(summary.plan_advice_does_not_affect_contract_identity)
      << ",\"all_policies_have_boundary_text\":"
      << bool_json(summary.all_policies_have_boundary_text)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"priority_classes\":" << string_array_json(summary.priority_classes)
      << ",\"plan_advice_surfaces\":"
      << string_array_json(summary.plan_advice_surfaces)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string plan_basis_json(
    const target::lattice_target_evaluation_t::plan_basis_t &basis) {
  std::ostringstream out;
  out << "{\"available\":" << bool_json(basis.available)
      << ",\"reason\":" << json_quote(basis.reason)
      << ",\"primary_deficit_key\":" << json_quote(basis.primary_deficit_key)
      << ",\"primary_deficit_message\":"
      << json_quote(basis.primary_deficit_message)
      << ",\"primary_deficit_priority\":" << basis.primary_deficit_priority
      << ",\"primary_deficit_priority_class\":"
      << json_quote(basis.primary_deficit_priority_class)
      << ",\"deficit_priority_vocabulary\":"
      << deficit_priority_vocabulary_json()
      << ",\"deficit_keys\":" << string_array_json(basis.deficit_keys)
      << ",\"deficit_messages\":" << string_array_json(basis.deficit_messages)
      << ",\"deficit_priority_classes\":"
      << string_array_json(basis.deficit_priority_classes)
      << ",\"target_range\":" << interval_json(basis.target_range)
      << ",\"missing_intervals\":"
      << interval_array_json(basis.missing_intervals)
      << ",\"suggested_action\":" << json_quote(basis.suggested_action)
      << ",\"fact_preview_hint_count\":" << basis.fact_preview_hints.size()
      << ",\"primary_fact_preview_hint\":";
  if (basis.fact_preview_hints.empty()) {
    out << "null";
  } else {
    out << plan_basis_fact_preview_hint_json(basis.fact_preview_hints.front());
  }
  out << ",\"fact_preview_hints\":"
      << plan_basis_fact_preview_hints_json(basis.fact_preview_hints) << "}";
  return out.str();
}

[[nodiscard]] std::string evidence_order_dimension_order_json() {
  std::ostringstream out;
  out << "{";
  const auto vocabulary = target::lattice_evidence_order_dimension_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(vocabulary[i].dimension) << ":"
        << json_quote(vocabulary[i].polarity);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string evidence_order_dimension_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_evidence_order_dimension_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"dimension\":" << json_quote(vocabulary[i].dimension)
        << ",\"polarity\":" << json_quote(vocabulary[i].polarity)
        << ",\"comparison_dimension\":"
        << bool_json(vocabulary[i].comparison_dimension) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_order_relation_vocabulary_json() {
  std::ostringstream out;
  out << "[";
  const auto vocabulary = target::lattice_evidence_order_relation_vocabulary();
  for (std::size_t i = 0; i < vocabulary.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(
        target::lattice_evidence_order_relation_name(vocabulary[i]));
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_order_vector_json(
    const target::lattice_target_evaluation_t::evidence_order_vector_t
        &vector) {
  std::ostringstream out;
  out << "{\"order\":" << json_quote(vector.order)
      << ",\"comparison_relation\":\"pareto_vector_dominance\""
      << ",\"comparison_helper\":\"compare_lattice_evidence_order_vectors\""
      << ",\"dimension_order\":" << evidence_order_dimension_order_json()
      << ",\"dimension_vocabulary\":"
      << evidence_order_dimension_vocabulary_json()
      << ",\"relation_vocabulary\":"
      << evidence_order_relation_vocabulary_json()
      << ",\"available\":" << bool_json(vector.available)
      << ",\"satisfied\":" << bool_json(vector.satisfied)
      << ",\"proof_certificate_check_passed\":"
      << bool_json(vector.proof_certificate_check_passed)
      << ",\"identity_contract_match\":"
      << bool_json(vector.identity_contract_match)
      << ",\"identity_component_match\":"
      << bool_json(vector.identity_component_match)
      << ",\"identity_graph_order_match\":"
      << bool_json(vector.identity_graph_order_match)
      << ",\"identity_source_cursor_match\":"
      << bool_json(vector.identity_source_cursor_match)
      << ",\"min_unique_coverage_fraction\":"
      << double_json(vector.min_unique_coverage_fraction)
      << ",\"max_cursor_exposure_load\":"
      << double_json(vector.max_cursor_exposure_load)
      << ",\"closure_checked\":" << bool_json(vector.closure_checked)
      << ",\"closure_complete\":" << bool_json(vector.closure_complete)
      << ",\"leakage_checked\":" << bool_json(vector.leakage_checked)
      << ",\"leakage_passed\":" << bool_json(vector.leakage_passed)
      << ",\"selection_signal_leakage_found\":"
      << bool_json(vector.selection_signal_leakage_found)
      << ",\"aggregate_valid_target_wilson_lower_95\":"
      << double_json(vector.aggregate_valid_target_wilson_lower_95)
      << ",\"weakest_node_valid_target_wilson_lower_95\":"
      << double_json(vector.weakest_node_valid_target_wilson_lower_95)
      << ",\"max_node_support_coefficient_of_variation\":"
      << double_json(vector.max_node_support_coefficient_of_variation)
      << ",\"max_node_support_gini\":"
      << double_json(vector.max_node_support_gini)
      << ",\"source_key_audit_count\":" << vector.source_key_audit_count
      << ",\"source_key_audit_issue_count\":"
      << vector.source_key_audit_issue_count
      << ",\"source_key_affine_inconsistent_count\":"
      << vector.source_key_affine_inconsistent_count
      << ",\"blocking_deficit_count\":" << vector.blocking_deficit_count
      << ",\"unresolved_lineage_count\":" << vector.unresolved_lineage_count
      << ",\"warning_result_count\":" << vector.warning_result_count
      << ",\"triggered_warning_count\":" << vector.triggered_warning_count
      << ",\"unavailable_warning_count\":" << vector.unavailable_warning_count
      << ",\"warning_count\":" << vector.warning_count << "}";
  return out.str();
}

[[nodiscard]] bool dependency_result_satisfied(
    const target::lattice_target_proof_certificate_t::dependency_proof_t
        &dependency) {
  if (dependency.status != target::lattice_target_status_name(
                               target::lattice_target_status_t::satisfied)) {
    return false;
  }
  if (dependency.kind == "checkpoint_source" ||
      dependency.kind == "evaluated_checkpoint_source") {
    if (!dependency.mdn_checkpoint_match) {
      return false;
    }
  }
  if (dependency.kind == "evaluated_checkpoint_source" &&
      !dependency.expected_representation_checkpoint.empty() &&
      !dependency.representation_checkpoint_match) {
    return false;
  }
  return true;
}

[[nodiscard]] bool dependency_results_satisfied(
    const std::vector<
        target::lattice_target_proof_certificate_t::dependency_proof_t>
        &dependencies) {
  return std::all_of(dependencies.begin(), dependencies.end(),
                     dependency_result_satisfied);
}

[[nodiscard]] bool coverage_results_satisfied(
    const std::vector<
        target::lattice_target_proof_certificate_t::coverage_proof_t>
        &coverage) {
  return std::all_of(coverage.begin(), coverage.end(),
                     [](const auto &proof) { return proof.passed; });
}

[[nodiscard]] bool proof_context_result_satisfied(
    const target::lattice_target_proof_certificate_t::proof_context_t
        &proof_context) {
  return (!proof_context.require_contract_match ||
          proof_context.contract_match) &&
         (!proof_context.require_component_match ||
          proof_context.component_match) &&
         (!proof_context.require_graph_anchor_identity ||
          (proof_context.graph_order_match &&
           proof_context.source_cursor_match));
}

[[nodiscard]] std::string
derived_query_rule_projection_scope(const std::string &relation) {
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  const auto rule = std::find_if(
      vocabulary.begin(), vocabulary.end(),
      [&](const auto &entry) { return entry.relation == relation; });
  if (rule == vocabulary.end()) {
    return "unknown_relation";
  }
  return rule->projection_scope;
}

[[nodiscard]] std::string
derived_query_rule_projection_quantifier(const std::string &relation) {
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  const auto rule = std::find_if(
      vocabulary.begin(), vocabulary.end(),
      [&](const auto &entry) { return entry.relation == relation; });
  if (rule == vocabulary.end()) {
    return "unknown_relation";
  }
  return rule->projection_quantifier;
}

[[nodiscard]] std::string
derived_query_rule_empty_projection_policy(const std::string &relation) {
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  const auto rule = std::find_if(
      vocabulary.begin(), vocabulary.end(),
      [&](const auto &entry) { return entry.relation == relation; });
  if (rule == vocabulary.end()) {
    return "unknown_relation";
  }
  return rule->empty_projection_policy;
}

struct derived_query_result_row_t {
  std::string relation{};
  bool known{false};
  bool value{false};
  std::string proof_field{};
  std::string basis{};
  std::string result_projection_scope{"target"};
  std::int64_t projection_row_count{1};
  std::int64_t projection_true_count{-1};
  std::int64_t projection_false_count{-1};
  std::string result_projection_quantifier{"single"};
  std::string result_empty_projection_policy{"not_applicable"};
  std::string projection_row_source{};
};

[[nodiscard]] bool derived_query_result_projection_shape_matches(
    const derived_query_result_row_t &row) {
  const auto vocabulary = target::lattice_derived_query_rule_vocabulary();
  const auto rule = std::find_if(
      vocabulary.begin(), vocabulary.end(),
      [&](const auto &entry) { return entry.relation == row.relation; });
  if (rule == vocabulary.end()) {
    return false;
  }
  return row.result_projection_scope == rule->result_projection_scope &&
         row.result_projection_quantifier ==
             rule->result_projection_quantifier &&
         row.result_empty_projection_policy ==
             rule->result_empty_projection_policy;
}

[[nodiscard]] derived_query_result_row_t make_derived_query_result(
    const std::string &relation, bool known, bool value,
    const std::string &proof_field, const std::string &basis,
    const std::string &result_projection_scope = "target",
    std::int64_t projection_row_count = 1,
    std::int64_t projection_true_count = -1,
    std::int64_t projection_false_count = -1,
    const std::string &result_projection_quantifier = "single",
    const std::string &result_empty_projection_policy = "not_applicable",
    const std::string &projection_row_source = "") {
  derived_query_result_row_t row{};
  row.relation = relation;
  row.known = known;
  row.value = value;
  row.proof_field = proof_field;
  row.basis = basis;
  row.result_projection_scope = result_projection_scope;
  row.projection_row_count = projection_row_count;
  row.projection_true_count = projection_true_count;
  row.projection_false_count = projection_false_count;
  row.result_projection_quantifier = result_projection_quantifier;
  row.result_empty_projection_policy = result_empty_projection_policy;
  row.projection_row_source =
      projection_row_source.empty() ? proof_field : projection_row_source;
  if (projection_true_count < 0) {
    row.projection_true_count = known && value ? 1 : 0;
  }
  if (projection_false_count < 0) {
    row.projection_false_count = known && !value ? 1 : 0;
  }
  return row;
}

[[nodiscard]] std::string
derived_query_result_json(const derived_query_result_row_t &row) {
  std::ostringstream out;
  out << "{\"relation\":" << json_quote(row.relation)
      << ",\"known\":" << bool_json(row.known)
      << ",\"value\":" << bool_json(row.value)
      << ",\"proof_field\":" << json_quote(row.proof_field)
      << ",\"rule_projection_scope\":"
      << json_quote(derived_query_rule_projection_scope(row.relation))
      << ",\"rule_projection_quantifier\":"
      << json_quote(derived_query_rule_projection_quantifier(row.relation))
      << ",\"rule_empty_projection_policy\":"
      << json_quote(derived_query_rule_empty_projection_policy(row.relation))
      << ",\"result_projection_scope\":"
      << json_quote(row.result_projection_scope)
      << ",\"projection_scope\":" << json_quote(row.result_projection_scope)
      << ",\"result_projection_quantifier\":"
      << json_quote(row.result_projection_quantifier)
      << ",\"projection_quantifier\":"
      << json_quote(row.result_projection_quantifier)
      << ",\"result_empty_projection_policy\":"
      << json_quote(row.result_empty_projection_policy)
      << ",\"empty_projection_policy\":"
      << json_quote(row.result_empty_projection_policy)
      << ",\"projection_row_source\":" << json_quote(row.projection_row_source)
      << ",\"projection_row_count\":" << row.projection_row_count
      << ",\"projection_true_count\":" << row.projection_true_count
      << ",\"projection_false_count\":" << row.projection_false_count
      << ",\"basis\":" << json_quote(row.basis) << "}";
  return out.str();
}

[[nodiscard]] std::string
derived_query_results_json(const target::lattice_target_evaluation_t &eval) {
  const auto &proof = eval.proof_certificate;
  const auto dependency_row_count =
      static_cast<std::int64_t>(proof.dependencies.size());
  const auto dependency_true_count = static_cast<std::int64_t>(
      std::count_if(proof.dependencies.begin(), proof.dependencies.end(),
                    dependency_result_satisfied));
  const auto coverage_row_count =
      static_cast<std::int64_t>(proof.coverage.size());
  const auto coverage_true_count = static_cast<std::int64_t>(
      std::count_if(proof.coverage.begin(), proof.coverage.end(),
                    [](const auto &coverage) { return coverage.passed; }));
  const auto checkpoint_ancestor_count =
      proof.closure.checked
          ? static_cast<std::int64_t>(proof.closure.causal_exposures.size())
          : 0;
  const auto unresolved_lineage_witness_count =
      proof.closure.checked
          ? static_cast<std::int64_t>(
                proof.closure.unresolved_input_checkpoints.size() +
                proof.closure.identity_mismatches.size())
          : 0;
  const auto overlap_witness_count =
      static_cast<std::int64_t>(proof.leakage.overlap_witnesses.size());
  const auto warning_row_count =
      static_cast<std::int64_t>(eval.warning_results.size());
  const auto warning_true_count = eval.warning_summary.triggered_warning_count;
  const auto plan_deficit_count = static_cast<std::int64_t>(
      std::count_if(eval.deficits.begin(), eval.deficits.end(),
                    [](const auto &deficit) { return deficit.plan_relevant; }));
  const auto blocking_deficit_count =
      static_cast<std::int64_t>(eval.deficits.size());
  const bool identity_match =
      proof_context_result_satisfied(proof.proof_context);
  const bool dependency_satisfied =
      dependency_results_satisfied(proof.dependencies);
  const bool coverage_satisfied = coverage_results_satisfied(proof.coverage);
  const bool closure_complete = proof.closure.checked && proof.closure.complete;
  const bool checkpoint_ancestor = proof.closure.checked &&
                                   proof.closure.complete &&
                                   checkpoint_ancestor_count > 0;
  const bool unresolved_lineage =
      proof.closure.checked && unresolved_lineage_witness_count > 0;
  const bool closure_clean_if_checked =
      !proof.closure.checked || proof.closure.complete;
  const bool forbidden_overlap =
      proof.leakage.checked && proof.leakage.overlap_found;
  const bool no_forbidden_overlap =
      !proof.leakage.checked || !proof.leakage.overlap_found;
  const bool warning_triggered =
      eval.warning_summary.triggered_warning_count > 0;
  const bool plan_deficit = plan_deficit_count > 0;
  const bool proof_certificate_check_passed =
      eval.proof_certificate_check.passed;
  const bool no_blocking_deficit = eval.deficits.empty();
  const bool status_satisfied =
      eval.status == target::lattice_target_status_t::satisfied;
  const bool target_satisfied =
      status_satisfied && identity_match && dependency_satisfied &&
      coverage_satisfied && closure_clean_if_checked && no_forbidden_overlap &&
      proof_certificate_check_passed && no_blocking_deficit;
  const std::int64_t target_premise_true_count =
      (status_satisfied ? 1 : 0) + (identity_match ? 1 : 0) +
      (dependency_satisfied ? 1 : 0) + (coverage_satisfied ? 1 : 0) +
      (closure_clean_if_checked ? 1 : 0) + (no_forbidden_overlap ? 1 : 0) +
      (proof_certificate_check_passed ? 1 : 0) + (no_blocking_deficit ? 1 : 0);

  std::vector<derived_query_result_row_t> results;
  results.push_back(make_derived_query_result(
      "identity_match", /*known=*/true, identity_match,
      "proof_certificate.proof_context",
      "derived from required identity fields and active/evidence match "
      "booleans"));
  results.push_back(make_derived_query_result(
      "dependency_satisfied", /*known=*/true, dependency_satisfied,
      "proof_certificate.dependencies[]",
      "compact projection is true only when every dependency_satisfied(T,D) "
      "row is true",
      "all_dependency_rows_compact", dependency_row_count,
      dependency_true_count, dependency_row_count - dependency_true_count,
      "all", "vacuously_true", "proof_certificate.dependencies[]"));
  results.push_back(make_derived_query_result(
      "all_dependencies_satisfied", /*known=*/true, dependency_satisfied,
      "proof_certificate.dependencies[]",
      "derived from all dependency proof statuses and exact checkpoint "
      "bindings",
      "all_dependency_rows_compact", dependency_row_count,
      dependency_true_count, dependency_row_count - dependency_true_count,
      "all", "vacuously_true", "proof_certificate.dependencies[]"));
  results.push_back(make_derived_query_result(
      "coverage_satisfied", /*known=*/true, coverage_satisfied,
      "proof_certificate.coverage[]",
      "compact projection is true only when every coverage_satisfied(T,U) row "
      "is true",
      "all_coverage_rows_compact", coverage_row_count, coverage_true_count,
      coverage_row_count - coverage_true_count, "all", "vacuously_true",
      "proof_certificate.coverage[]"));
  results.push_back(make_derived_query_result(
      "all_coverage_satisfied", /*known=*/true, coverage_satisfied,
      "proof_certificate.coverage[]",
      "derived from all coverage obligations and idempotent union thresholds",
      "all_coverage_rows_compact", coverage_row_count, coverage_true_count,
      coverage_row_count - coverage_true_count, "all", "vacuously_true",
      "proof_certificate.coverage[]"));
  results.push_back(make_derived_query_result(
      "closure_complete", proof.closure.checked, closure_complete,
      "proof_certificate.closure",
      "derived from checked checkpoint lineage and unresolved input list",
      "root_checkpoint_closure", proof.closure.checked ? 1 : 0,
      closure_complete ? 1 : 0,
      proof.closure.checked && !closure_complete ? 1 : 0, "single",
      "not_applicable", "proof_certificate.closure"));
  results.push_back(make_derived_query_result(
      "checkpoint_ancestor", proof.closure.checked, checkpoint_ancestor,
      "checkpoint_closure.facts[]",
      "derived from checked complete checkpoint closure causal exposure rows; "
      "incomplete closure fails closed",
      "any_checkpoint_ancestor", checkpoint_ancestor_count,
      checkpoint_ancestor ? checkpoint_ancestor_count : 0,
      checkpoint_ancestor ? 0 : checkpoint_ancestor_count, "any",
      "false_when_empty", "proof_certificate.closure.causal_exposures[]"));
  results.push_back(make_derived_query_result(
      "unresolved_lineage", proof.closure.checked, unresolved_lineage,
      "checkpoint_closure.unresolved_input_checkpoints[]",
      "derived from unresolved checkpoint inputs and checkpoint identity "
      "mismatches",
      "any_unresolved_lineage_witness", unresolved_lineage_witness_count,
      unresolved_lineage_witness_count, 0, "any", "false_when_empty",
      "proof_certificate.closure.unresolved_input_checkpoints[] and "
      "identity_mismatches[]"));
  results.push_back(make_derived_query_result(
      "closure_clean_if_checked", /*known=*/true, closure_clean_if_checked,
      "proof_certificate.closure",
      "derived from closure checked/complete fields and target proof "
      "obligations"));
  results.push_back(make_derived_query_result(
      "forbidden_overlap", proof.leakage.checked, forbidden_overlap,
      "proof_certificate.leakage.overlap_witnesses[]",
      "derived from leakage proof overlap_found and overlap witnesses",
      "any_overlap_witness", overlap_witness_count, overlap_witness_count, 0,
      "any", "false_when_empty",
      "proof_certificate.leakage.overlap_witnesses[]"));
  results.push_back(make_derived_query_result(
      "no_forbidden_overlap", /*known=*/true, no_forbidden_overlap,
      "proof_certificate.leakage",
      "derived from leakage checked/overlap fields and target proof "
      "obligations",
      "target", overlap_witness_count, 0, overlap_witness_count, "none",
      "true_when_empty", "proof_certificate.leakage.overlap_witnesses[]"));
  results.push_back(make_derived_query_result(
      "warning_triggered", /*known=*/true, warning_triggered,
      "warning_results[]", "derived from warning threshold_triggered flags",
      "any_warning_result", warning_row_count, warning_true_count,
      warning_row_count - warning_true_count, "any", "false_when_empty",
      "warning_results[]"));
  results.push_back(make_derived_query_result(
      "stale_cache", /*known=*/false, /*value=*/false,
      "runtime_index_cache_validation.issues[]",
      "cache validation is unavailable in target-evaluation projection; use "
      "hero.lattice.inspect subject=derived for concrete cache witnesses",
      "any_cache_validation_issue", 0, 0, 0, "any", "false_when_empty",
      "runtime_index_cache_validation"));
  results.push_back(make_derived_query_result(
      "plan_deficit", /*known=*/true, plan_deficit, "plan_basis",
      "derived from proof deficits marked plan_relevant",
      "any_plan_relevant_deficit", plan_deficit_count, plan_deficit_count, 0,
      "any", "false_when_empty", "deficits[] where plan_relevant"));
  results.push_back(make_derived_query_result(
      "proof_certificate_check_passed", /*known=*/true,
      proof_certificate_check_passed, "proof_certificate_check",
      "derived from proof certificate self-check result"));
  results.push_back(make_derived_query_result(
      "no_blocking_deficit", /*known=*/true, no_blocking_deficit, "deficits[]",
      "derived from absence of proof deficits", "target",
      blocking_deficit_count, 0, blocking_deficit_count, "none",
      "true_when_empty", "deficits[]"));
  results.push_back(make_derived_query_result(
      "status_satisfied", /*known=*/true, status_satisfied, "status",
      "derived from target status enum"));
  results.push_back(make_derived_query_result(
      "target_satisfied", /*known=*/true, target_satisfied, "status",
      "derived from target status, proof relations, certificate check, and "
      "deficits",
      "target", 8, target_premise_true_count, 8 - target_premise_true_count,
      "all", "fixed_nonempty", "target_satisfied fixed premise set"));

  const std::int64_t result_count = static_cast<std::int64_t>(results.size());
  const auto aggregate_projection_count = static_cast<std::int64_t>(
      std::count_if(results.begin(), results.end(), [](const auto &row) {
        return row.result_projection_quantifier != "single";
      }));
  const std::int64_t single_projection_count =
      result_count - aggregate_projection_count;
  const auto known_count = static_cast<std::int64_t>(
      std::count_if(results.begin(), results.end(),
                    [](const auto &row) { return row.known; }));
  const std::int64_t unknown_count = result_count - known_count;
  const auto known_true_count = static_cast<std::int64_t>(
      std::count_if(results.begin(), results.end(),
                    [](const auto &row) { return row.known && row.value; }));
  const std::int64_t known_false_count = known_count - known_true_count;
  const auto rule_vocabulary = target::lattice_derived_query_rule_vocabulary();
  const std::int64_t rule_vocabulary_relation_count =
      static_cast<std::int64_t>(rule_vocabulary.size());
  const auto rule_vocabulary_digest = derived_query_rule_vocabulary_digest();
  const auto result_has_relation = [&](const std::string &relation) {
    return std::any_of(results.begin(), results.end(), [&](const auto &row) {
      return row.relation == relation;
    });
  };
  const auto rule_has_relation = [&](const std::string &relation) {
    return std::any_of(
        rule_vocabulary.begin(), rule_vocabulary.end(),
        [&](const auto &entry) { return entry.relation == relation; });
  };
  std::vector<std::string> missing_rule_relations;
  for (const auto &entry : rule_vocabulary) {
    if (!result_has_relation(entry.relation)) {
      missing_rule_relations.push_back(entry.relation);
    }
  }
  std::vector<std::string> extra_result_relations;
  for (const auto &row : results) {
    if (!rule_has_relation(row.relation)) {
      extra_result_relations.push_back(row.relation);
    }
  }
  const auto projection_semantics_vocabulary =
      target::lattice_derived_query_projection_semantics_vocabulary();
  const auto projection_semantics_value_known = [&](const std::string &field,
                                                    const std::string &value) {
    return std::any_of(projection_semantics_vocabulary.begin(),
                       projection_semantics_vocabulary.end(),
                       [&](const auto &entry) {
                         return !entry.result_alias && entry.field == field &&
                                entry.value == value;
                       });
  };
  const std::int64_t missing_rule_relation_count =
      static_cast<std::int64_t>(missing_rule_relations.size());
  const std::int64_t extra_result_relation_count =
      static_cast<std::int64_t>(extra_result_relations.size());
  std::map<std::string, std::int64_t> result_relation_counts;
  for (const auto &row : results) {
    ++result_relation_counts[row.relation];
  }
  std::int64_t result_relation_count_total = 0;
  std::vector<std::string> duplicate_result_relations;
  for (const auto &[relation, count] : result_relation_counts) {
    result_relation_count_total += count;
    if (count > 1) {
      duplicate_result_relations.push_back(relation);
    }
  }
  const std::int64_t unique_result_relation_count =
      static_cast<std::int64_t>(result_relation_counts.size());
  const std::int64_t duplicate_result_relation_count =
      static_cast<std::int64_t>(duplicate_result_relations.size());
  const bool projection_basis_complete =
      !proof.target_id.empty() && !proof.target_spec_fingerprint.empty() &&
      !proof.certificate_digest.empty() &&
      !std::string_view(target::lattice_target_status_name(eval.status))
           .empty();
  constexpr std::int64_t projection_basis_field_count = 4;
  constexpr std::int64_t result_projection_digest_basis_field_count = 6;
  const bool result_projection_digest_rule_vocabulary_bound =
      !std::string_view(kDerivedQueryRuleVocabularyDigestSchema).empty() &&
      !rule_vocabulary_digest.empty();
  std::ostringstream result_projection_digest_text;
  result_projection_digest_text
      << "schema=" << kDerivedQueryResultProjectionDigestSchema << "\n"
      << "rule_vocabulary_digest_schema="
      << kDerivedQueryRuleVocabularyDigestSchema << "\n"
      << "rule_vocabulary_digest=" << rule_vocabulary_digest << "\n"
      << "target_id=" << proof.target_id << "\n"
      << "target_spec_fingerprint=" << proof.target_spec_fingerprint << "\n"
      << "certificate_digest=" << proof.certificate_digest << "\n"
      << "status=" << target::lattice_target_status_name(eval.status) << "\n";
  for (const auto &row : results) {
    result_projection_digest_text
        << "relation=" << row.relation << "\n"
        << "known=" << (row.known ? "true" : "false") << "\n"
        << "value=" << (row.value ? "true" : "false") << "\n"
        << "rule_projection_scope="
        << derived_query_rule_projection_scope(row.relation) << "\n"
        << "rule_projection_quantifier="
        << derived_query_rule_projection_quantifier(row.relation) << "\n"
        << "rule_empty_projection_policy="
        << derived_query_rule_empty_projection_policy(row.relation) << "\n"
        << "result_projection_scope=" << row.result_projection_scope << "\n"
        << "result_projection_quantifier=" << row.result_projection_quantifier
        << "\n"
        << "result_empty_projection_policy="
        << row.result_empty_projection_policy << "\n"
        << "projection_row_source=" << row.projection_row_source << "\n"
        << "projection_row_count=" << row.projection_row_count << "\n"
        << "projection_true_count=" << row.projection_true_count << "\n"
        << "projection_false_count=" << row.projection_false_count << "\n";
  }
  const auto result_projection_digest =
      exposure::exposure_digest_for_text(result_projection_digest_text.str());
  const bool result_covers_rule_vocabulary =
      result_count == rule_vocabulary_relation_count &&
      missing_rule_relation_count == 0 && extra_result_relation_count == 0 &&
      duplicate_result_relation_count == 0;
  std::vector<std::string> summary_issues;
  if (result_relation_count_total != result_count) {
    summary_issues.push_back(
        "result relation multiplicity total does not equal result_count");
  }
  if (duplicate_result_relation_count !=
      static_cast<std::int64_t>(duplicate_result_relations.size())) {
    summary_issues.push_back("duplicate_result_relation_count does not match "
                             "duplicate_result_relations");
  }
  if (duplicate_result_relation_count != 0) {
    summary_issues.push_back("duplicate result relations are not allowed");
  }
  if (!projection_basis_complete) {
    summary_issues.push_back(
        "projection digest basis requires target id, target spec fingerprint, "
        "certificate digest, and status");
  }
  if (!result_projection_digest_rule_vocabulary_bound) {
    summary_issues.push_back(
        "result projection digest basis requires rule vocabulary schema and "
        "digest");
  }
  if (known_count + unknown_count != result_count) {
    summary_issues.push_back(
        "known_count + unknown_count does not equal result_count");
  }
  if (known_true_count + known_false_count != known_count) {
    summary_issues.push_back(
        "known_true_count + known_false_count does not equal known_count");
  }
  if (aggregate_projection_count + single_projection_count != result_count) {
    summary_issues.push_back(
        "aggregate_projection_count + single_projection_count does not equal "
        "result_count");
  }
  if (missing_rule_relation_count !=
      static_cast<std::int64_t>(missing_rule_relations.size())) {
    summary_issues.push_back(
        "missing_rule_relation_count does not match missing_rule_relations");
  }
  if (missing_rule_relation_count != 0) {
    summary_issues.push_back("missing rule relations are not allowed");
  }
  if (extra_result_relation_count !=
      static_cast<std::int64_t>(extra_result_relations.size())) {
    summary_issues.push_back(
        "extra_result_relation_count does not match extra_result_relations");
  }
  if (extra_result_relation_count != 0) {
    summary_issues.push_back("extra result relations are not allowed");
  }
  for (const auto &row : results) {
    if (row.relation.empty()) {
      summary_issues.push_back("result relation must be non-empty");
    }
    if (row.proof_field.empty()) {
      summary_issues.push_back("proof field must be non-empty for relation " +
                               row.relation);
    }
    if (row.basis.empty()) {
      summary_issues.push_back("basis must be non-empty for relation " +
                               row.relation);
    }
    if (row.result_projection_scope.empty()) {
      summary_issues.push_back(
          "result projection scope must be non-empty for relation " +
          row.relation);
    }
    const auto rule_projection_scope =
        derived_query_rule_projection_scope(row.relation);
    const auto rule_projection_quantifier =
        derived_query_rule_projection_quantifier(row.relation);
    const auto rule_empty_projection_policy =
        derived_query_rule_empty_projection_policy(row.relation);
    if (!projection_semantics_value_known("projection_scope",
                                          rule_projection_scope)) {
      summary_issues.push_back("unknown rule projection scope for relation " +
                               row.relation);
    }
    if (!projection_semantics_value_known("projection_scope",
                                          row.result_projection_scope)) {
      summary_issues.push_back("unknown result projection scope for relation " +
                               row.relation);
    }
    if (!projection_semantics_value_known("projection_quantifier",
                                          rule_projection_quantifier)) {
      summary_issues.push_back(
          "unknown rule projection quantifier for relation " + row.relation);
    }
    if (!projection_semantics_value_known("empty_projection_policy",
                                          rule_empty_projection_policy)) {
      summary_issues.push_back(
          "unknown rule empty projection policy for relation " + row.relation);
    }
    if (!derived_query_result_projection_shape_matches(row)) {
      summary_issues.push_back(
          "result projection shape is incompatible with declared relation "
          "for relation " +
          row.relation);
    }
    if (row.projection_row_source.empty()) {
      summary_issues.push_back(
          "projection row source must be non-empty for relation " +
          row.relation);
    }
    if (!projection_semantics_value_known("projection_quantifier",
                                          row.result_projection_quantifier)) {
      summary_issues.push_back(
          "unknown result projection quantifier for relation " + row.relation);
    }
    if (!projection_semantics_value_known("empty_projection_policy",
                                          row.result_empty_projection_policy)) {
      summary_issues.push_back(
          "unknown result empty projection policy for relation " +
          row.relation);
    }
    const auto projection_policy_compatible = [&]() {
      if (row.result_projection_quantifier == "single") {
        return row.result_empty_projection_policy == "not_applicable";
      }
      if (row.result_projection_quantifier == "all") {
        return row.result_empty_projection_policy == "vacuously_true" ||
               row.result_empty_projection_policy == "fixed_nonempty";
      }
      if (row.result_projection_quantifier == "any") {
        return row.result_empty_projection_policy == "false_when_empty";
      }
      if (row.result_projection_quantifier == "none") {
        return row.result_empty_projection_policy == "true_when_empty";
      }
      return false;
    }();
    if (!projection_policy_compatible) {
      summary_issues.push_back(
          "projection quantifier and empty policy are incompatible for "
          "relation " +
          row.relation);
    }
    if (row.projection_row_count < 0 || row.projection_true_count < 0 ||
        row.projection_false_count < 0) {
      summary_issues.push_back(
          "projection counts must be non-negative for relation " +
          row.relation);
    }
    const bool projection_counts_non_negative =
        row.projection_row_count >= 0 && row.projection_true_count >= 0 &&
        row.projection_false_count >= 0;
    const bool projection_counts_within_row_count =
        projection_counts_non_negative &&
        row.projection_true_count + row.projection_false_count <=
            row.projection_row_count;
    if (!projection_counts_within_row_count) {
      summary_issues.push_back(
          "projection true/false counts exceed row count for relation " +
          row.relation);
    }
    if (row.known && projection_counts_within_row_count &&
        row.projection_true_count + row.projection_false_count !=
            row.projection_row_count) {
      summary_issues.push_back(
          "known projection true/false counts must cover row count for "
          "relation " +
          row.relation);
    }
    if (row.known && row.result_projection_quantifier == "single" &&
        row.projection_row_count != 1) {
      summary_issues.push_back(
          "known single projection must have exactly one row for relation " +
          row.relation);
    }
    if (row.result_empty_projection_policy == "fixed_nonempty" &&
        row.projection_row_count == 0) {
      summary_issues.push_back(
          "fixed nonempty projection has zero rows for relation " +
          row.relation);
    }
    if (row.projection_row_count == 0) {
      if (row.result_empty_projection_policy == "vacuously_true" &&
          row.value != true) {
        summary_issues.push_back(
            "vacuously true empty projection has false value for relation " +
            row.relation);
      }
      if (row.result_empty_projection_policy == "false_when_empty" &&
          row.value != false) {
        summary_issues.push_back(
            "false-when-empty projection has true value for relation " +
            row.relation);
      }
      if (row.result_empty_projection_policy == "true_when_empty" &&
          row.value != true) {
        summary_issues.push_back(
            "true-when-empty projection has false value for relation " +
            row.relation);
      }
    }
    if (row.known && projection_counts_within_row_count) {
      std::optional<bool> expected_value;
      if (row.result_projection_quantifier == "single") {
        expected_value = row.projection_true_count > 0;
      } else if (row.result_projection_quantifier == "all") {
        expected_value =
            row.projection_true_count == row.projection_row_count &&
            row.projection_false_count == 0;
      } else if (row.result_projection_quantifier == "any") {
        expected_value = row.projection_true_count > 0;
      } else if (row.result_projection_quantifier == "none") {
        expected_value = row.projection_row_count == 0;
      }
      if (expected_value.has_value() && row.value != *expected_value) {
        summary_issues.push_back(
            "projection value disagrees with quantifier/count semantics for "
            "relation " +
            row.relation);
      }
    }
  }
  const bool summary_self_check_passed = summary_issues.empty();
  const std::int64_t summary_issue_count =
      static_cast<std::int64_t>(summary_issues.size());

  std::ostringstream out;
  out << "{\"source\":\"existing_evaluation_projection\""
      << ",\"schema\":" << json_quote(kDerivedQueryResultsSchema)
      << ",\"schema_version\":1"
      << ",\"rule_vocabulary_digest_schema\":"
      << json_quote(kDerivedQueryRuleVocabularyDigestSchema)
      << ",\"result_projection_digest_schema\":"
      << json_quote(kDerivedQueryResultProjectionDigestSchema)
      << ",\"projection_target_id\":" << json_quote(proof.target_id)
      << ",\"projection_target_spec_fingerprint\":"
      << json_quote(proof.target_spec_fingerprint)
      << ",\"projection_certificate_digest\":"
      << json_quote(proof.certificate_digest) << ",\"projection_status\":"
      << json_quote(target::lattice_target_status_name(eval.status))
      << ",\"projection_basis_complete\":"
      << bool_json(projection_basis_complete)
      << ",\"projection_basis_field_count\":" << projection_basis_field_count
      << ",\"result_projection_digest_rule_vocabulary_bound\":"
      << bool_json(result_projection_digest_rule_vocabulary_bound)
      << ",\"result_projection_digest_basis_field_count\":"
      << result_projection_digest_basis_field_count
      << ",\"summary_source\":\"emitted_result_rows\""
      << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"db_source_of_truth\":false"
      << ",\"affects_certificate_digest\":false"
      << ",\"summary_self_check_passed\":"
      << bool_json(summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary_issue_count
      << ",\"summary_issues\":" << string_array_json(summary_issues)
      << ",\"result_count\":" << result_count
      << ",\"rule_vocabulary_digest\":" << json_quote(rule_vocabulary_digest)
      << ",\"result_projection_digest\":"
      << json_quote(result_projection_digest)
      << ",\"unique_result_relation_count\":" << unique_result_relation_count
      << ",\"rule_vocabulary_relation_count\":"
      << rule_vocabulary_relation_count << ",\"result_covers_rule_vocabulary\":"
      << bool_json(result_covers_rule_vocabulary)
      << ",\"missing_rule_relation_count\":" << missing_rule_relation_count
      << ",\"extra_result_relation_count\":" << extra_result_relation_count
      << ",\"duplicate_result_relation_count\":"
      << duplicate_result_relation_count << ",\"missing_rule_relations\":"
      << string_array_json(missing_rule_relations)
      << ",\"extra_result_relations\":"
      << string_array_json(extra_result_relations)
      << ",\"duplicate_result_relations\":"
      << string_array_json(duplicate_result_relations)
      << ",\"known_count\":" << known_count
      << ",\"unknown_count\":" << unknown_count
      << ",\"known_true_count\":" << known_true_count
      << ",\"known_false_count\":" << known_false_count
      << ",\"aggregate_projection_count\":" << aggregate_projection_count
      << ",\"single_projection_count\":" << single_projection_count
      << ",\"results\":[";
  for (std::size_t i = 0; i < results.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << derived_query_result_json(results[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string
evaluation_json(const target::lattice_target_evaluation_t &eval) {
  const bool artifact_readiness_target =
      target::is_artifact_readiness_target_class(eval.target_class);
  std::ostringstream out;
  out << "{\"target_id\":" << json_quote(eval.target_id) << ",\"kind\":"
      << json_quote(target::lattice_target_effective_kind_name(eval))
      << ",\"target_kind_applicable\":"
      << bool_json(target::lattice_target_kind_applicable(eval))
      << ",\"target_kind_effective\":"
      << json_quote(target::lattice_target_effective_kind_selector(eval))
      << ",\"target_class\":" << json_quote(eval.target_class)
      << ",\"proof_kind\":" << json_quote(eval.proof_kind)
      << ",\"subject_fact_family\":" << json_quote(eval.subject_fact_family)
      << ",\"artifact_readiness_proof_template_registry\":"
      << artifact_readiness_proof_template_registry_json()
      << ",\"target_surface_kind\":"
      << json_quote(artifact_readiness_target ? "evidence_catalog_artifact"
                                              : "runtime_readiness")
      << ",\"evidence_catalog_target\":" << bool_json(artifact_readiness_target)
      << ",\"dispatchable_target\":" << bool_json(!artifact_readiness_target)
      << ",\"runtime_wave_dispatchable\":"
      << bool_json(!artifact_readiness_target && eval.plan_ready &&
                   !eval.suggested_wave.empty())
      << ",\"recommended_operator_action\":"
      << json_quote(artifact_readiness_target
                        ? "inspect"
                        : (eval.plan_ready ? "review_suggested_wave"
                                           : "inspect_lattice_status"))
      << ",\"component\":" << json_quote(eval.component) << ",\"status\":"
      << json_quote(target::lattice_target_status_name(eval.status))
      << ",\"split_policy_fingerprint\":"
      << json_quote(eval.split_policy_fingerprint)
      << ",\"plan_ready\":" << bool_json(eval.plan_ready)
      << ",\"reasons\":" << string_array_json(eval.reasons)
      << ",\"suggested_wave\":" << suggested_wave_json(eval.suggested_wave)
      << ",\"evidence\":" << evidence_json(eval.evidence)
      << ",\"exposure_measure_algebra_vocabulary\":"
      << exposure_measure_algebra_vocabulary_json()
      << ",\"exposure_measure_algebra_summary\":"
      << exposure_measure_algebra_summary_json()
      << ",\"source_key_coordinate_policy_vocabulary\":"
      << source_key_coordinate_policy_vocabulary_json()
      << ",\"source_key_coordinate_policy_summary\":"
      << source_key_coordinate_policy_summary_json()
      << ",\"source_receipt_policy_vocabulary\":"
      << source_receipt_policy_vocabulary_json()
      << ",\"source_receipt_policy_summary\":"
      << source_receipt_policy_summary_json()
      << ",\"valid_target_uncertainty_vocabulary\":"
      << valid_target_uncertainty_vocabulary_json()
      << ",\"valid_target_uncertainty_summary\":"
      << valid_target_uncertainty_summary_json()
      << ",\"leakage_rule_vocabulary\":" << leakage_rule_vocabulary_json()
      << ",\"leakage_rule_summary\":" << leakage_rule_summary_json()
      << ",\"causal_edge_vocabulary\":" << causal_edge_vocabulary_json()
      << ",\"causal_edge_summary\":" << causal_edge_summary_json()
      << ",\"selection_signal_policy_vocabulary\":"
      << selection_signal_policy_vocabulary_json()
      << ",\"selection_signal_policy_summary\":"
      << selection_signal_policy_summary_json()
      << ",\"derived_query_rule_vocabulary\":"
      << derived_query_rule_vocabulary_json()
      << ",\"derived_query_rule_vocabulary_digest_schema\":"
      << json_quote(kDerivedQueryRuleVocabularyDigestSchema)
      << ",\"derived_query_rule_vocabulary_digest\":"
      << json_quote(derived_query_rule_vocabulary_digest())
      << ",\"derived_query_projection_semantics_vocabulary\":"
      << derived_query_projection_semantics_vocabulary_json()
      << ",\"derived_query_results\":" << derived_query_results_json(eval)
      << ",\"db_cache_policy_vocabulary\":" << db_cache_policy_vocabulary_json()
      << ",\"evidence_retention_policy_vocabulary\":"
      << evidence_retention_policy_vocabulary_json()
      << ",\"evidence_retention_audit_scenario_vocabulary\":"
      << evidence_retention_audit_scenario_vocabulary_json()
      << ",\"evidence_retention_policy_summary\":"
      << evidence_retention_policy_summary_json()
      << ",\"benchmark_regression_budget_vocabulary\":"
      << benchmark_regression_budget_vocabulary_json()
      << ",\"benchmark_regression_budget_summary\":"
      << benchmark_regression_budget_summary_json()
      << ",\"evidence_abstraction_vocabulary\":"
      << evidence_abstraction_vocabulary_json()
      << ",\"product_evidence_state_vocabulary\":"
      << product_evidence_state_vocabulary_json()
      << ",\"join_law_vocabulary\":" << join_law_vocabulary_json()
      << ",\"deficit_priority_vocabulary\":"
      << deficit_priority_vocabulary_json()
      << ",\"deficit_vector_planning_summary\":"
      << deficit_vector_planning_summary_json()
      << ",\"evidence_order_dimension_vocabulary\":"
      << evidence_order_dimension_vocabulary_json()
      << ",\"evidence_order_relation_vocabulary\":"
      << evidence_order_relation_vocabulary_json()
      << ",\"node_support_scope_policy_vocabulary\":"
      << node_support_scope_policy_vocabulary_json()
      << ",\"node_support_scope_policy_summary\":"
      << node_support_scope_policy_summary_json()
      << ",\"validation_performance_evidence_policy_vocabulary\":"
      << validation_performance_evidence_policy_vocabulary_json()
      << ",\"validation_performance_evidence_policy_summary\":"
      << validation_performance_evidence_policy_summary_json()
      << ",\"validation_performance_scope_policy_vocabulary\":"
      << validation_performance_scope_policy_vocabulary_json()
      << ",\"validation_performance_scope_policy_summary\":"
      << validation_performance_scope_policy_summary_json()
      << ",\"representation_geometry_vocabulary\":"
      << representation_geometry_vocabulary_json()
      << ",\"representation_geometry_summary\":"
      << representation_geometry_summary_json()
      << ",\"representation_geometry_gate_review_summary\":"
      << representation_geometry_gate_review_summary_json()
      << ",\"performance_uncertainty_policy_vocabulary\":"
      << performance_uncertainty_policy_vocabulary_json()
      << ",\"performance_uncertainty_policy_summary\":"
      << performance_uncertainty_policy_summary_json()
      << ",\"mdn_distribution_calibration_vocabulary\":"
      << mdn_distribution_calibration_vocabulary_json()
      << ",\"mdn_distribution_calibration_summary\":"
      << mdn_distribution_calibration_summary_json()
      << ",\"mdn_distribution_calibration_diagnostic_vocabulary\":"
      << mdn_distribution_calibration_diagnostic_vocabulary_json()
      << ",\"mdn_distribution_calibration_diagnostic_summary\":"
      << mdn_distribution_calibration_diagnostic_summary_json()
      << ",\"target_numeric_dimension_vocabulary\":"
      << target_numeric_dimension_vocabulary_json()
      << ",\"target_numeric_dimension_summary\":"
      << target_numeric_dimension_summary_json()
      << ",\"monotonicity_invariant_vocabulary\":"
      << monotonicity_invariant_vocabulary_json()
      << ",\"proof_obligation_vocabulary\":"
      << proof_obligation_vocabulary_json()
      << ",\"proof_certificate_digest_policy_vocabulary\":"
      << proof_certificate_digest_policy_vocabulary_json()
      << ",\"proof_certificate_digest_policy_summary\":"
      << proof_certificate_digest_policy_summary_json()
      << ",\"checkpoint_identity_policy_vocabulary\":"
      << checkpoint_identity_policy_vocabulary_json()
      << ",\"checkpoint_selection_policy_vocabulary\":"
      << checkpoint_selection_policy_vocabulary_json()
      << ",\"checkpoint_selection_policy_summary\":"
      << checkpoint_selection_policy_summary_json()
      << ",\"plan_advice_scope_policy_vocabulary\":"
      << plan_advice_scope_policy_vocabulary_json()
      << ",\"operational_readiness_v1_scope_vocabulary\":"
      << operational_readiness_v1_scope_vocabulary_json()
      << ",\"operational_readiness_v1_scope_summary\":"
      << operational_readiness_v1_scope_summary_json()
      << ",\"operational_readiness_v1_gate_vocabulary\":"
      << operational_readiness_v1_gate_vocabulary_json()
      << ",\"operational_readiness_v1_gate_summary\":"
      << operational_readiness_v1_gate_summary_json()
      << ",\"contract_identity_boundary_vocabulary\":"
      << contract_identity_boundary_vocabulary_json()
      << ",\"contract_identity_boundary_summary\":"
      << contract_identity_boundary_summary_json()
      << ",\"mathematical_readiness_v1_vocabulary\":"
      << mathematical_readiness_v1_vocabulary_json()
      << ",\"mathematical_readiness_v1_summary\":"
      << mathematical_readiness_v1_summary_json() << ",\"exposure_summaries\":"
      << exposure_load_summaries_json(eval.exposure_summaries)
      << ",\"node_support_summaries\":"
      << node_support_summaries_json(eval.node_support_summaries)
      << ",\"warnings\":" << warning_results_json(eval.warning_results)
      << ",\"warning_messages\":"
      << warning_messages_json(eval.warning_results, eval.warnings)
      << ",\"warning_results\":" << warning_results_json(eval.warning_results)
      << ",\"warning_summary\":" << warning_summary_json(eval.warning_summary)
      << ",\"fact_integrity_summary_available\":"
      << bool_json(eval.fact_integrity_summary_available)
      << ",\"fact_integrity_summary\":"
      << evaluation_fact_integrity_summary_json(eval)
      << ",\"representation_geometry_gate_results\":"
      << representation_geometry_gate_results_json(
             eval.representation_geometry_gate_results)
      << ",\"warning_threshold_relation_vocabulary\":"
      << warning_threshold_relation_vocabulary_json()
      << ",\"warning_anchor_scope_policy_vocabulary\":"
      << warning_anchor_scope_policy_vocabulary_json()
      << ",\"deficits\":" << proof_deficits_json(eval.deficits)
      << ",\"plan_basis\":" << plan_basis_json(eval.plan_basis)
      << ",\"evidence_order_vector\":"
      << evidence_order_vector_json(eval.evidence_order_vector)
      << ",\"proof_certificate\":"
      << proof_certificate_json(eval.proof_certificate)
      << ",\"proof_certificate_check\":"
      << proof_certificate_check_json(eval.proof_certificate_check) << "}";
  return out.str();
}

[[nodiscard]] std::string evaluation_json_with_policy_gate_reservations(
    const target::lattice_target_evaluation_t &eval,
    const std::vector<target::lattice_policy_gate_spec_t> &policy_gates) {
  std::string json = evaluation_json(eval);
  if (json.empty() || json.back() != '}') {
    return json;
  }
  json.pop_back();
  json += ",\"policy_gate_reservation_summary\":";
  json += policy_gate_reservation_summary_json(policy_gates, eval.target_id);
  json += ",\"policy_gate_reservations\":";
  json += policy_gate_reservations_json(policy_gates, eval.target_id);
  json += "}";
  return json;
}

[[nodiscard]] std::string
fact_json(const exposure::lattice_exposure_fact_t &f) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(f.schema)
      << ",\"fact_type\":" << json_quote(f.fact_type)
      << ",\"contract_fingerprint\":" << json_quote(f.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(f.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(f.graph_order_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(f.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(f.target_component_family_id)
      << ",\"config_bundle_id\":" << json_quote(f.config_bundle_id)
      << ",\"config_receipt_id\":" << json_quote(f.config_receipt_id)
      << ",\"component_spawn_registry_id\":"
      << json_quote(f.component_spawn_registry_id)
      << ",\"component_family_id\":" << json_quote(f.component_family_id)
      << ",\"component_spawn_fingerprint\":"
      << json_quote(f.component_spawn_fingerprint)
      << ",\"component_spawn_id\":" << json_quote(f.component_spawn_id)
      << ",\"component_spawn_label\":" << json_quote(f.component_spawn_label)
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
      << ",\"source_key_window\":{\"precision\":"
      << json_quote(f.source_key_footprint_precision)
      << ",\"first_anchor_key\":" << json_quote(f.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(f.last_anchor_key)
      << ",\"observed_begin\":" << json_quote(f.observed_source_key_begin)
      << ",\"observed_end\":" << json_quote(f.observed_source_key_end)
      << ",\"target_begin\":" << json_quote(f.target_source_key_begin)
      << ",\"target_end\":" << json_quote(f.target_source_key_end) << "}"
      << ",\"source_key_window_audit\":"
      << source_key_window_audit_json(exposure::audit_source_key_window(f))
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
      << ",\"checkpoint_digest_reported\":"
      << json_quote(f.checkpoint_digest_reported)
      << ",\"checkpoint_digest_verified\":"
      << bool_json(f.checkpoint_digest_verified)
      << ",\"checkpoint_file_exists\":" << bool_json(f.checkpoint_file_exists)
      << ",\"checkpoint_bytes\":" << f.checkpoint_bytes
      << ",\"finite_loss\":" << bool_json(f.finite_loss)
      << ",\"run_data_kind\":" << json_quote(f.run_data_kind)
      << ",\"readiness_scope\":" << json_quote(f.readiness_scope)
      << ",\"market_readiness_claimed\":"
      << bool_json(f.market_readiness_claimed)
      << ",\"mean_loss\":" << double_json(f.mean_loss)
      << ",\"mean_nll\":" << double_json(f.mean_nll)
      << ",\"report_schema\":{\"id\":" << json_quote(f.report_schema_id)
      << ",\"version\":" << f.report_schema_version
      << ",\"writer_id\":" << json_quote(f.report_writer_id)
      << ",\"writer_version\":" << json_quote(f.report_writer_version)
      << "},\"flattening\":{\"input_nodelift_shape\":"
      << json_quote(f.input_nodelift_shape)
      << ",\"mtf_training_shape\":" << json_quote(f.mtf_training_shape)
      << ",\"flattening_contract\":" << json_quote(f.flattening_contract)
      << ",\"anchor_batch_count\":" << f.anchor_batch_count
      << ",\"node_count\":" << f.node_count
      << ",\"flattened_sample_count\":" << f.flattened_sample_count
      << ",\"recommended_graph_restore_shape\":"
      << json_quote(f.recommended_graph_restore_shape)
      << ",\"graph_restore_available\":" << bool_json(f.graph_restore_available)
      << ",\"reshape_lossless\":" << bool_json(f.reshape_lossless) << "}"
      << ",\"representation_health\":{\"available\":"
      << bool_json(f.representation_health_available)
      << ",\"mean_invariance_loss\":" << double_json(f.mean_invariance_loss)
      << ",\"mean_variance_loss\":" << double_json(f.mean_variance_loss)
      << ",\"mean_covariance_loss\":" << double_json(f.mean_covariance_loss)
      << ",\"loss_jepa_mean\":" << double_json(f.loss_jepa_mean)
      << ",\"loss_mae_time_mean\":" << double_json(f.loss_mae_time_mean)
      << ",\"loss_mae_frequency_mean\":"
      << double_json(f.loss_mae_frequency_mean)
      << ",\"loss_tf_align_mean\":" << double_json(f.loss_tf_align_mean)
      << ",\"loss_vicreg_global_mean\":"
      << double_json(f.loss_vicreg_global_mean)
      << ",\"loss_vicreg_channel_mean\":"
      << double_json(f.loss_vicreg_channel_mean)
      << ",\"last_grad_norm\":" << double_json(f.last_grad_norm)
      << ",\"max_grad_norm\":" << double_json(f.max_grad_norm)
      << ",\"gradients_finite\":" << bool_json(f.gradients_finite)
      << ",\"sample_valid_count\":" << f.sample_valid_count
      << ",\"sample_valid_fraction\":" << double_json(f.sample_valid_fraction)
      << ",\"channel_valid_count\":" << f.channel_valid_count
      << ",\"channel_valid_fraction\":" << double_json(f.channel_valid_fraction)
      << ",\"valid_latent_rows\":" << f.valid_latent_rows
      << ",\"tf_pair_count\":" << f.tf_pair_count
      << ",\"tf_pair_valid_count\":" << f.tf_pair_valid_count
      << ",\"vicreg_global_valid_rows\":" << f.vicreg_global_valid_rows
      << ",\"vicreg_channel_valid_rows\":" << f.vicreg_channel_valid_rows
      << ",\"context_starved_sample_count\":" << f.context_starved_sample_count
      << ",\"reduced_targets_for_context_count\":"
      << f.reduced_targets_for_context_count
      << ",\"min_context_satisfied_count\":" << f.min_context_satisfied_count
      << ",\"target_ema_distance\":" << double_json(f.target_ema_distance)
      << ",\"latent_std\":" << double_json(f.latent_std)
      << ",\"latent_norm\":" << double_json(f.latent_norm)
      << ",\"total_valid_projection_rows\":" << f.total_valid_projection_rows
      << ",\"mean_adapter_valid_channel_time_fraction\":"
      << double_json(f.mean_adapter_valid_channel_time_fraction)
      << ",\"finite_parameter_check\":" << bool_json(f.finite_parameter_check)
      << ",\"representation_embedding_dim\":" << f.representation_embedding_dim
      << ",\"representation_effective_rank\":"
      << double_json(f.representation_effective_rank)
      << ",\"representation_effective_rank_fraction\":"
      << double_json(f.representation_effective_rank_fraction)
      << ",\"representation_min_dimension_variance\":"
      << double_json(f.representation_min_dimension_variance)
      << ",\"representation_max_dimension_variance\":"
      << double_json(f.representation_max_dimension_variance)
      << ",\"representation_condition_number\":"
      << double_json(f.representation_condition_number)
      << ",\"representation_isotropy_score\":"
      << double_json(f.representation_isotropy_score) << "}"
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

[[nodiscard]] std::string source_receipt_fact_json(
    const exposure::lattice_source_receipt_fact_t &receipt) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(receipt.schema)
      << ",\"fact_type\":" << json_quote(receipt.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(receipt.parent_exposure_fact_digest)
      << ",\"receipt_index\":" << receipt.receipt_index
      << ",\"contract_fingerprint\":"
      << json_quote(receipt.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(receipt.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(receipt.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(receipt.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(receipt.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(receipt.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(receipt.target_component_family_id)
      << ",\"job_id\":" << json_quote(receipt.job_id)
      << ",\"wave_id\":" << json_quote(receipt.wave_id)
      << ",\"split_name\":" << json_quote(receipt.split_name)
      << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(receipt.split_role))
      << ",\"anchor_range\":" << interval_json(receipt.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(receipt.completed_anchor_range)
      << ",\"source_key_window\":{\"precision\":"
      << json_quote(receipt.source_key_footprint_precision)
      << ",\"first_anchor_key\":" << json_quote(receipt.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(receipt.last_anchor_key)
      << ",\"observed_begin\":" << json_quote(receipt.observed_source_key_begin)
      << ",\"observed_end\":" << json_quote(receipt.observed_source_key_end)
      << ",\"target_begin\":" << json_quote(receipt.target_source_key_begin)
      << ",\"target_end\":" << json_quote(receipt.target_source_key_end) << "}"
      << ",\"edge\":" << json_quote(receipt.edge)
      << ",\"instrument\":" << json_quote(receipt.instrument)
      << ",\"interval\":" << json_quote(receipt.interval)
      << ",\"record_type\":" << json_quote(receipt.record_type)
      << ",\"source\":" << json_quote(receipt.source)
      << ",\"raw_receipt\":" << json_quote(receipt.raw_receipt)
      << ",\"audit_only\":true"
      << ",\"coverage_authority\":false"
      << ",\"leakage_authority\":false"
      << ",\"contract_identity_authority\":false"
      << ",\"digest\":"
      << json_quote(exposure::source_receipt_fact_digest(receipt)) << "}";
  return out.str();
}

[[nodiscard]] std::string source_receipt_facts_json(
    const std::vector<exposure::lattice_source_receipt_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << source_receipt_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
source_receipt_summary_json(const exposure::source_receipt_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"exposure_facts_with_receipts\":"
      << summary.exposure_facts_with_receipts
      << ",\"exposure_facts_missing_receipts\":"
      << summary.exposure_facts_missing_receipts
      << ",\"source_receipt_fact_count\":" << summary.source_receipt_fact_count
      << ",\"malformed_receipt_count\":" << summary.malformed_receipt_count
      << ",\"unique_edge_count\":" << summary.unique_edge_count
      << ",\"unique_instrument_count\":" << summary.unique_instrument_count
      << ",\"unique_source_count\":" << summary.unique_source_count
      << ",\"audit_only\":" << bool_json(summary.audit_only)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"row_index_authority_preserved\":"
      << bool_json(summary.row_index_authority_preserved)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string source_key_map_audit_summary_json(
    const exposure::source_key_map_audit_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"source_receipt_fact_count\":" << summary.source_receipt_fact_count
      << ",\"source_receipt_bound_parent_count\":"
      << summary.source_receipt_bound_parent_count
      << ",\"available_audit_count\":" << summary.available_audit_count
      << ",\"unavailable_audit_count\":" << summary.unavailable_audit_count
      << ",\"complete_count\":" << summary.complete_count
      << ",\"numeric_count\":" << summary.numeric_count
      << ",\"internally_monotone_count\":" << summary.internally_monotone_count
      << ",\"order_preserving_count\":" << summary.order_preserving_count
      << ",\"affine_step_available_count\":"
      << summary.affine_step_available_count
      << ",\"affine_consistent_count\":" << summary.affine_consistent_count
      << ",\"source_key_gap_warning_count\":"
      << summary.source_key_gap_warning_count
      << ",\"irregular_key_warning_count\":"
      << summary.irregular_key_warning_count
      << ",\"missing_endpoint_pair_count\":"
      << summary.missing_endpoint_pair_count
      << ",\"row_source_key_mismatch_count\":"
      << summary.row_source_key_mismatch_count
      << ",\"non_numeric_issue_count\":" << summary.non_numeric_issue_count
      << ",\"nonmonotone_issue_count\":" << summary.nonmonotone_issue_count
      << ",\"order_inverted_issue_count\":"
      << summary.order_inverted_issue_count
      << ",\"issue_count\":" << summary.issue_count
      << ",\"audits_with_graph_order_count\":"
      << summary.audits_with_graph_order_count
      << ",\"audits_with_source_cursor_count\":"
      << summary.audits_with_source_cursor_count
      << ",\"audits_with_source_receipts_count\":"
      << summary.audits_with_source_receipts_count
      << ",\"unique_graph_order_count\":" << summary.unique_graph_order_count
      << ",\"unique_source_cursor_count\":"
      << summary.unique_source_cursor_count
      << ",\"audit_only\":" << bool_json(summary.audit_only)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"row_index_authority_preserved\":"
      << bool_json(summary.row_index_authority_preserved)
      << ",\"explicit_target_rule_required_for_blocking\":"
      << bool_json(summary.explicit_target_rule_required_for_blocking)
      << ",\"summary_self_check_passed\":"
      << bool_json(summary.summary_self_check_passed)
      << ",\"summary_issue_count\":" << summary.summary_issue_count
      << ",\"issues\":" << string_array_json(summary.issues)
      << ",\"summary_issues\":" << string_array_json(summary.summary_issues)
      << "}";
  return out.str();
}

[[nodiscard]] std::string source_analytics_metric_summary_json(
    const exposure::source_analytics_metric_summary_t &summary) {
  std::ostringstream out;
  out << "{\"count\":" << summary.count
      << ",\"min\":" << double_json(summary.min)
      << ",\"max\":" << double_json(summary.max)
      << ",\"mean\":" << double_json(summary.mean) << "}";
  return out.str();
}

[[nodiscard]] std::string source_analytics_fact_json(
    const exposure::lattice_source_analytics_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"source_key_window\":{\"precision\":"
      << json_quote(fact.source_key_footprint_precision)
      << ",\"first_anchor_key\":" << json_quote(fact.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(fact.last_anchor_key)
      << ",\"observed_begin\":" << json_quote(fact.observed_source_key_begin)
      << ",\"observed_end\":" << json_quote(fact.observed_source_key_end)
      << ",\"target_begin\":" << json_quote(fact.target_source_key_begin)
      << ",\"target_end\":" << json_quote(fact.target_source_key_end) << "}"
      << ",\"source_receipt_fact_count\":" << fact.source_receipt_fact_count
      << ",\"entropy\":" << double_json(fact.entropy)
      << ",\"entropy_rate\":" << double_json(fact.entropy_rate)
      << ",\"information_density\":" << double_json(fact.information_density)
      << ",\"compression_ratio\":" << double_json(fact.compression_ratio)
      << ",\"power_spectrum_entropy\":"
      << double_json(fact.power_spectrum_entropy)
      << ",\"source_volatility\":" << double_json(fact.source_volatility)
      << ",\"feature_variance\":" << double_json(fact.feature_variance)
      << ",\"sample_validity_fraction\":"
      << double_json(fact.sample_validity_fraction)
      << ",\"missingness_fraction\":" << double_json(fact.missingness_fraction)
      << ",\"duplicate_sample_count\":" << fact.duplicate_sample_count
      << ",\"source_health_level\":" << json_quote(fact.source_health_level)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::source_analytics_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::source_analytics_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string source_analytics_facts_json(
    const std::vector<exposure::lattice_source_analytics_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << source_analytics_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string source_analytics_summary_json(
    const exposure::source_analytics_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"source_analytics_fact_count\":"
      << summary.source_analytics_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"source_cursor_bound_count\":" << summary.source_cursor_bound_count
      << ",\"graph_order_bound_count\":" << summary.graph_order_bound_count
      << ",\"source_receipt_bound_count\":"
      << summary.source_receipt_bound_count
      << ",\"duplicate_sample_count_total\":"
      << summary.duplicate_sample_count_total
      << ",\"train_source_analytics_fact_count\":"
      << summary.train_source_analytics_fact_count
      << ",\"validation_source_analytics_fact_count\":"
      << summary.validation_source_analytics_fact_count
      << ",\"source_regime_pair_count\":" << summary.source_regime_pair_count
      << ",\"source_regime_shift_warning_count\":"
      << summary.source_regime_shift_warning_count
      << ",\"warning_count\":" << summary.warning_count << ",\"entropy\":"
      << source_analytics_metric_summary_json(summary.entropy)
      << ",\"entropy_rate\":"
      << source_analytics_metric_summary_json(summary.entropy_rate)
      << ",\"information_density\":"
      << source_analytics_metric_summary_json(summary.information_density)
      << ",\"compression_ratio\":"
      << source_analytics_metric_summary_json(summary.compression_ratio)
      << ",\"power_spectrum_entropy\":"
      << source_analytics_metric_summary_json(summary.power_spectrum_entropy)
      << ",\"source_volatility\":"
      << source_analytics_metric_summary_json(summary.source_volatility)
      << ",\"feature_variance\":"
      << source_analytics_metric_summary_json(summary.feature_variance)
      << ",\"sample_validity_fraction\":"
      << source_analytics_metric_summary_json(summary.sample_validity_fraction)
      << ",\"missingness_fraction\":"
      << source_analytics_metric_summary_json(summary.missingness_fraction)
      << ",\"entropy_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.entropy_train_validation_delta)
      << ",\"entropy_rate_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.entropy_rate_train_validation_delta)
      << ",\"information_density_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.information_density_train_validation_delta)
      << ",\"compression_ratio_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.compression_ratio_train_validation_delta)
      << ",\"power_spectrum_entropy_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.power_spectrum_entropy_train_validation_delta)
      << ",\"source_volatility_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.source_volatility_train_validation_delta)
      << ",\"feature_variance_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.feature_variance_train_validation_delta)
      << ",\"sample_validity_fraction_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.sample_validity_fraction_train_validation_delta)
      << ",\"missingness_fraction_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.missingness_fraction_train_validation_delta)
      << ",\"duplicate_sample_count_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.duplicate_sample_count_train_validation_delta)
      << ",\"anchor_support_count_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.anchor_support_count_train_validation_delta)
      << ",\"completed_anchor_support_count_train_validation_delta\":"
      << source_analytics_metric_summary_json(
             summary.completed_anchor_support_count_train_validation_delta)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"row_index_authority_preserved\":"
      << bool_json(summary.row_index_authority_preserved)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string target_transform_fact_json(
    const exposure::lattice_target_transform_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"target_feature_ids\":"
      << string_array_json(fact.target_feature_ids)
      << ",\"horizon\":" << fact.horizon
      << ",\"target_mode\":" << json_quote(fact.target_mode)
      << ",\"normalization_contract\":"
      << json_quote(fact.normalization_contract)
      << ",\"inverse_transform_contract\":"
      << json_quote(fact.inverse_transform_contract)
      << ",\"units\":" << json_quote(fact.units)
      << ",\"target_mask_policy_digest\":"
      << json_quote(fact.target_mask_policy_digest)
      << ",\"support_surface_identity\":"
      << json_quote(fact.support_surface_identity)
      << ",\"support_surface_digest\":"
      << json_quote(fact.support_surface_digest)
      << ",\"artifact_contract_prerequisite\":"
      << bool_json(fact.artifact_contract_prerequisite)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::target_transform_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::target_transform_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string target_transform_facts_json(
    const std::vector<exposure::lattice_target_transform_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << target_transform_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string target_transform_summary_json(
    const exposure::target_transform_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"target_transform_fact_count\":"
      << summary.target_transform_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"target_feature_id_count\":" << summary.target_feature_id_count
      << ",\"unique_target_feature_id_count\":"
      << summary.unique_target_feature_id_count
      << ",\"horizon_bound_count\":" << summary.horizon_bound_count
      << ",\"missing_units_count\":" << summary.missing_units_count
      << ",\"missing_mask_policy_count\":" << summary.missing_mask_policy_count
      << ",\"missing_normalization_contract_count\":"
      << summary.missing_normalization_contract_count
      << ",\"missing_inverse_transform_contract_count\":"
      << summary.missing_inverse_transform_contract_count
      << ",\"artifact_contract_prerequisite_count\":"
      << summary.artifact_contract_prerequisite_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"artifact_contract_prerequisite\":"
      << bool_json(summary.artifact_contract_prerequisite)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string forecast_baseline_fact_json(
    const exposure::lattice_forecast_baseline_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"target_feature_ids\":"
      << string_array_json(fact.target_feature_ids)
      << ",\"horizon\":" << fact.horizon
      << ",\"baseline_kind\":" << json_quote(fact.baseline_kind)
      << ",\"baseline_parameters\":" << json_quote(fact.baseline_parameters)
      << ",\"target_transform_fact_digest\":"
      << json_quote(fact.target_transform_fact_digest)
      << ",\"support_count\":" << fact.support_count
      << ",\"valid_count\":" << fact.valid_count
      << ",\"missing_count\":" << fact.missing_count
      << ",\"metric_status\":" << json_quote(fact.metric_status)
      << ",\"computed_metric_value_count\":"
      << exposure::forecast_baseline_finite_metric_count(fact)
      << ",\"baseline_mean_nll\":" << double_json(fact.baseline_mean_nll)
      << ",\"baseline_ev_mae\":" << double_json(fact.baseline_ev_mae)
      << ",\"baseline_ev_rmse\":" << double_json(fact.baseline_ev_rmse)
      << ",\"baseline_signed_error\":"
      << double_json(fact.baseline_signed_error)
      << ",\"baseline_directional_accuracy\":"
      << double_json(fact.baseline_directional_accuracy)
      << ",\"evidence_prerequisite\":" << bool_json(fact.evidence_prerequisite)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::forecast_baseline_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::forecast_baseline_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string forecast_baseline_facts_json(
    const std::vector<exposure::lattice_forecast_baseline_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << forecast_baseline_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string forecast_baseline_summary_json(
    const exposure::forecast_baseline_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"forecast_baseline_fact_count\":"
      << summary.forecast_baseline_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"target_feature_id_count\":" << summary.target_feature_id_count
      << ",\"unique_target_feature_id_count\":"
      << summary.unique_target_feature_id_count
      << ",\"horizon_bound_count\":" << summary.horizon_bound_count
      << ",\"target_transform_declared_count\":"
      << summary.target_transform_declared_count
      << ",\"target_transform_bound_count\":"
      << summary.target_transform_bound_count
      << ",\"unresolved_target_transform_count\":"
      << summary.unresolved_target_transform_count
      << ",\"target_transform_identity_mismatch_count\":"
      << summary.target_transform_identity_mismatch_count
      << ",\"support_count_total\":" << summary.support_count_total
      << ",\"valid_count_total\":" << summary.valid_count_total
      << ",\"missing_count_total\":" << summary.missing_count_total
      << ",\"unique_baseline_kind_count\":"
      << summary.unique_baseline_kind_count
      << ",\"previous_value_baseline_count\":"
      << summary.previous_value_baseline_count
      << ",\"zero_return_baseline_count\":"
      << summary.zero_return_baseline_count
      << ",\"moving_average_baseline_count\":"
      << summary.moving_average_baseline_count
      << ",\"last_valid_channel_baseline_count\":"
      << summary.last_valid_channel_baseline_count
      << ",\"unknown_baseline_kind_count\":"
      << summary.unknown_baseline_kind_count
      << ",\"missing_baseline_kind_count\":"
      << summary.missing_baseline_kind_count
      << ",\"computed_metric_fact_count\":"
      << summary.computed_metric_fact_count
      << ",\"partial_metric_fact_count\":" << summary.partial_metric_fact_count
      << ",\"deferred_metric_fact_count\":"
      << summary.deferred_metric_fact_count
      << ",\"missing_metric_status_count\":"
      << summary.missing_metric_status_count
      << ",\"metric_status_mismatch_count\":"
      << summary.metric_status_mismatch_count
      << ",\"computed_metric_value_count\":"
      << summary.computed_metric_value_count
      << ",\"missing_target_transform_count\":"
      << summary.missing_target_transform_count
      << ",\"evidence_prerequisite_count\":"
      << summary.evidence_prerequisite_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"baseline_mean_nll\":"
      << source_analytics_metric_summary_json(summary.baseline_mean_nll)
      << ",\"baseline_ev_mae\":"
      << source_analytics_metric_summary_json(summary.baseline_ev_mae)
      << ",\"baseline_ev_rmse\":"
      << source_analytics_metric_summary_json(summary.baseline_ev_rmse)
      << ",\"baseline_signed_error\":"
      << source_analytics_metric_summary_json(summary.baseline_signed_error)
      << ",\"baseline_directional_accuracy\":"
      << source_analytics_metric_summary_json(
             summary.baseline_directional_accuracy)
      << ",\"evidence_prerequisite\":"
      << bool_json(summary.evidence_prerequisite)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string
forecast_eval_fact_json(const exposure::lattice_forecast_eval_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"target_feature_ids\":"
      << string_array_json(fact.target_feature_ids)
      << ",\"horizon\":" << fact.horizon
      << ",\"support_count\":" << fact.support_count
      << ",\"valid_count\":" << fact.valid_count
      << ",\"missing_count\":" << fact.missing_count
      << ",\"weakest_support_rows\":" << fact.weakest_support_rows
      << ",\"mean_nll\":" << double_json(fact.mean_nll)
      << ",\"mean_nll_per_channel\":"
      << double_array_json(fact.mean_nll_per_channel)
      << ",\"mean_nll_per_target_feature\":"
      << double_array_json(fact.mean_nll_per_target_feature)
      << ",\"mean_nll_per_channel_target_feature\":"
      << double_array_json(fact.mean_nll_per_channel_target_feature)
      << ",\"mean_nll_per_horizon\":"
      << double_array_json(fact.mean_nll_per_horizon)
      << ",\"valid_target_count_per_channel\":"
      << i64_array_json(fact.valid_target_count_per_channel)
      << ",\"valid_target_count_per_target_feature\":"
      << i64_array_json(fact.valid_target_count_per_target_feature)
      << ",\"valid_target_count_per_channel_target_feature\":"
      << i64_array_json(fact.valid_target_count_per_channel_target_feature)
      << ",\"valid_target_count_per_horizon\":"
      << i64_array_json(fact.valid_target_count_per_horizon)
      << ",\"ev_mae\":" << double_json(fact.ev_mae)
      << ",\"ev_rmse\":" << double_json(fact.ev_rmse)
      << ",\"signed_error\":" << double_json(fact.signed_error)
      << ",\"directional_accuracy\":" << double_json(fact.directional_accuracy)
      << ",\"calibration_coverage\":" << double_json(fact.calibration_coverage)
      << ",\"pit_summary\":" << json_quote(fact.pit_summary)
      << ",\"sigma_scale_sanity\":" << json_quote(fact.sigma_scale_sanity)
      << ",\"support_by_node\":" << json_quote(fact.support_by_node)
      << ",\"support_by_channel\":" << json_quote(fact.support_by_channel)
      << ",\"support_by_target_feature\":"
      << json_quote(fact.support_by_target_feature)
      << ",\"support_by_horizon\":" << json_quote(fact.support_by_horizon)
      << ",\"forecast_artifact_digest\":"
      << json_quote(fact.forecast_artifact_digest)
      << ",\"evaluated_representation_checkpoint_digest\":"
      << json_quote(fact.evaluated_representation_checkpoint_digest)
      << ",\"evaluated_mdn_checkpoint_digest\":"
      << json_quote(fact.evaluated_mdn_checkpoint_digest)
      << ",\"target_transform_fact_digest\":"
      << json_quote(fact.target_transform_fact_digest)
      << ",\"baseline_fact_digests\":"
      << string_array_json(fact.baseline_fact_digests)
      << ",\"selection_signal_fact_digests\":"
      << string_array_json(fact.selection_signal_fact_digests)
      << ",\"artifact_evidence\":" << bool_json(fact.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"model_state_mutation\":" << bool_json(fact.model_state_mutation)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::forecast_eval_fact_issues(fact))
      << ",\"digest\":" << json_quote(exposure::forecast_eval_fact_digest(fact))
      << "}";
  return out.str();
}

[[nodiscard]] std::string forecast_eval_facts_json(
    const std::vector<exposure::lattice_forecast_eval_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << forecast_eval_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
forecast_eval_summary_json(const exposure::forecast_eval_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"forecast_eval_fact_count\":" << summary.forecast_eval_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"target_feature_id_count\":" << summary.target_feature_id_count
      << ",\"unique_target_feature_id_count\":"
      << summary.unique_target_feature_id_count
      << ",\"horizon_bound_count\":" << summary.horizon_bound_count
      << ",\"forecast_artifact_bound_count\":"
      << summary.forecast_artifact_bound_count
      << ",\"representation_checkpoint_bound_count\":"
      << summary.representation_checkpoint_bound_count
      << ",\"mdn_checkpoint_bound_count\":"
      << summary.mdn_checkpoint_bound_count
      << ",\"target_transform_declared_count\":"
      << summary.target_transform_declared_count
      << ",\"target_transform_bound_count\":"
      << summary.target_transform_bound_count
      << ",\"unresolved_target_transform_count\":"
      << summary.unresolved_target_transform_count
      << ",\"target_transform_identity_mismatch_count\":"
      << summary.target_transform_identity_mismatch_count
      << ",\"baseline_declared_count\":" << summary.baseline_declared_count
      << ",\"baseline_bound_count\":" << summary.baseline_bound_count
      << ",\"unresolved_baseline_count\":" << summary.unresolved_baseline_count
      << ",\"baseline_identity_mismatch_count\":"
      << summary.baseline_identity_mismatch_count
      << ",\"selection_signal_declared_count\":"
      << summary.selection_signal_declared_count
      << ",\"selection_signal_audit_count\":"
      << summary.selection_signal_audit_count
      << ",\"unresolved_selection_signal_count\":"
      << summary.unresolved_selection_signal_count
      << ",\"selection_signal_identity_mismatch_count\":"
      << summary.selection_signal_identity_mismatch_count
      << ",\"support_count_total\":" << summary.support_count_total
      << ",\"valid_count_total\":" << summary.valid_count_total
      << ",\"missing_count_total\":" << summary.missing_count_total
      << ",\"nll_surface_bound_count\":" << summary.nll_surface_bound_count
      << ",\"channel_support_surface_bound_count\":"
      << summary.channel_support_surface_bound_count
      << ",\"target_feature_support_surface_bound_count\":"
      << summary.target_feature_support_surface_bound_count
      << ",\"channel_target_feature_support_surface_bound_count\":"
      << summary.channel_target_feature_support_surface_bound_count
      << ",\"horizon_support_surface_bound_count\":"
      << summary.horizon_support_surface_bound_count
      << ",\"horizon_nll_surface_bound_count\":"
      << summary.horizon_nll_surface_bound_count
      << ",\"calibration_coverage_bound_count\":"
      << summary.calibration_coverage_bound_count
      << ",\"missing_calibration_coverage_count\":"
      << summary.missing_calibration_coverage_count
      << ",\"pit_summary_bound_count\":" << summary.pit_summary_bound_count
      << ",\"missing_pit_summary_count\":" << summary.missing_pit_summary_count
      << ",\"sigma_scale_sanity_bound_count\":"
      << summary.sigma_scale_sanity_bound_count
      << ",\"missing_sigma_scale_sanity_count\":"
      << summary.missing_sigma_scale_sanity_count
      << ",\"calibration_visibility_warning_count\":"
      << summary.calibration_visibility_warning_count
      << ",\"missing_forecast_artifact_count\":"
      << summary.missing_forecast_artifact_count
      << ",\"missing_representation_checkpoint_count\":"
      << summary.missing_representation_checkpoint_count
      << ",\"missing_mdn_checkpoint_count\":"
      << summary.missing_mdn_checkpoint_count
      << ",\"missing_target_transform_count\":"
      << summary.missing_target_transform_count
      << ",\"missing_baseline_count\":" << summary.missing_baseline_count
      << ",\"model_state_mutation_count\":"
      << summary.model_state_mutation_count
      << ",\"artifact_evidence_count\":" << summary.artifact_evidence_count
      << ",\"warning_count\":" << summary.warning_count << ",\"mean_nll\":"
      << source_analytics_metric_summary_json(summary.mean_nll)
      << ",\"ev_mae\":" << source_analytics_metric_summary_json(summary.ev_mae)
      << ",\"ev_rmse\":"
      << source_analytics_metric_summary_json(summary.ev_rmse)
      << ",\"signed_error\":"
      << source_analytics_metric_summary_json(summary.signed_error)
      << ",\"directional_accuracy\":"
      << source_analytics_metric_summary_json(summary.directional_accuracy)
      << ",\"calibration_coverage\":"
      << source_analytics_metric_summary_json(summary.calibration_coverage)
      << ",\"skill_vs_baseline_mean_nll\":"
      << source_analytics_metric_summary_json(
             summary.skill_vs_baseline_mean_nll)
      << ",\"skill_vs_baseline_ev_mae\":"
      << source_analytics_metric_summary_json(summary.skill_vs_baseline_ev_mae)
      << ",\"skill_vs_baseline_ev_rmse\":"
      << source_analytics_metric_summary_json(summary.skill_vs_baseline_ev_rmse)
      << ",\"directional_accuracy_delta_vs_baseline\":"
      << source_analytics_metric_summary_json(
             summary.directional_accuracy_delta_vs_baseline)
      << ",\"artifact_evidence\":" << bool_json(summary.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"model_state_mutation\":" << bool_json(summary.model_state_mutation)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string observer_belief_fact_json(
    const exposure::lattice_observer_belief_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"belief_kind\":" << json_quote(fact.belief_kind)
      << ",\"channel_consensus\":" << json_quote(fact.channel_consensus)
      << ",\"potential_surface_diagnostics\":"
      << json_quote(fact.potential_surface_diagnostics)
      << ",\"nodelift_return_projection\":"
      << json_quote(fact.nodelift_return_projection)
      << ",\"covariance_coupling\":" << json_quote(fact.covariance_coupling)
      << ",\"scenario_bank_digest\":" << json_quote(fact.scenario_bank_digest)
      << ",\"nodelift_residual_quality\":"
      << json_quote(fact.nodelift_residual_quality)
      << ",\"projection_validation_scores\":"
      << json_quote(fact.projection_validation_scores)
      << ",\"confidence\":" << double_json(fact.confidence)
      << ",\"data_quality\":" << double_json(fact.data_quality)
      << ",\"liquidity\":" << double_json(fact.liquidity)
      << ",\"forecast_artifact_digest\":"
      << json_quote(fact.forecast_artifact_digest)
      << ",\"forecast_artifact_lineage\":"
      << json_quote(fact.forecast_artifact_lineage)
      << ",\"feature_semantics_fingerprint\":"
      << json_quote(fact.feature_semantics_fingerprint)
      << ",\"dock_binding_fingerprint\":"
      << json_quote(fact.dock_binding_fingerprint)
      << ",\"artifact_evidence\":" << bool_json(fact.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"raw_potential_tradable_return\":"
      << bool_json(fact.raw_potential_tradable_return)
      << ",\"allocation_authority\":" << bool_json(fact.allocation_authority)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(fact.market_readiness_authority) << ",\"issues\":"
      << string_array_json(exposure::observer_belief_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::observer_belief_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string observer_belief_facts_json(
    const std::vector<exposure::lattice_observer_belief_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << observer_belief_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string observer_belief_summary_json(
    const exposure::observer_belief_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"observer_belief_fact_count\":"
      << summary.observer_belief_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"raw_nodelift_potential_count\":"
      << summary.raw_nodelift_potential_count
      << ",\"allocation_belief_count\":" << summary.allocation_belief_count
      << ",\"forecast_artifact_bound_count\":"
      << summary.forecast_artifact_bound_count
      << ",\"forecast_artifact_lineage_declared_count\":"
      << summary.forecast_artifact_lineage_declared_count
      << ",\"forecast_artifact_lineage_bound_count\":"
      << summary.forecast_artifact_lineage_bound_count
      << ",\"unresolved_forecast_artifact_lineage_count\":"
      << summary.unresolved_forecast_artifact_lineage_count
      << ",\"forecast_artifact_lineage_identity_mismatch_count\":"
      << summary.forecast_artifact_lineage_identity_mismatch_count
      << ",\"forecast_artifact_digest_mismatch_count\":"
      << summary.forecast_artifact_digest_mismatch_count
      << ",\"channel_consensus_bound_count\":"
      << summary.channel_consensus_bound_count
      << ",\"potential_surface_diagnostics_bound_count\":"
      << summary.potential_surface_diagnostics_bound_count
      << ",\"nodelift_return_projection_bound_count\":"
      << summary.nodelift_return_projection_bound_count
      << ",\"covariance_coupling_bound_count\":"
      << summary.covariance_coupling_bound_count
      << ",\"scenario_bank_bound_count\":" << summary.scenario_bank_bound_count
      << ",\"nodelift_residual_quality_bound_count\":"
      << summary.nodelift_residual_quality_bound_count
      << ",\"projection_validation_scores_bound_count\":"
      << summary.projection_validation_scores_bound_count
      << ",\"feature_semantics_bound_count\":"
      << summary.feature_semantics_bound_count
      << ",\"dock_binding_bound_count\":" << summary.dock_binding_bound_count
      << ",\"missing_belief_kind_count\":" << summary.missing_belief_kind_count
      << ",\"missing_forecast_artifact_count\":"
      << summary.missing_forecast_artifact_count
      << ",\"missing_channel_consensus_count\":"
      << summary.missing_channel_consensus_count
      << ",\"missing_potential_surface_diagnostics_count\":"
      << summary.missing_potential_surface_diagnostics_count
      << ",\"missing_nodelift_return_projection_count\":"
      << summary.missing_nodelift_return_projection_count
      << ",\"missing_covariance_coupling_count\":"
      << summary.missing_covariance_coupling_count
      << ",\"missing_scenario_bank_count\":"
      << summary.missing_scenario_bank_count
      << ",\"missing_nodelift_residual_quality_count\":"
      << summary.missing_nodelift_residual_quality_count
      << ",\"missing_projection_validation_scores_count\":"
      << summary.missing_projection_validation_scores_count
      << ",\"missing_feature_semantics_count\":"
      << summary.missing_feature_semantics_count
      << ",\"missing_dock_binding_count\":"
      << summary.missing_dock_binding_count
      << ",\"low_confidence_count\":" << summary.low_confidence_count
      << ",\"low_data_quality_count\":" << summary.low_data_quality_count
      << ",\"low_liquidity_count\":" << summary.low_liquidity_count
      << ",\"diagnostic_completeness_warning_count\":"
      << summary.diagnostic_completeness_warning_count
      << ",\"observer_quality_warning_count\":"
      << summary.observer_quality_warning_count
      << ",\"artifact_evidence_count\":" << summary.artifact_evidence_count
      << ",\"warning_count\":" << summary.warning_count << ",\"confidence\":"
      << source_analytics_metric_summary_json(summary.confidence)
      << ",\"data_quality\":"
      << source_analytics_metric_summary_json(summary.data_quality)
      << ",\"liquidity\":"
      << source_analytics_metric_summary_json(summary.liquidity)
      << ",\"artifact_evidence\":" << bool_json(summary.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"raw_potential_tradable_return\":"
      << bool_json(summary.raw_potential_tradable_return)
      << ",\"allocation_authority\":" << bool_json(summary.allocation_authority)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(summary.market_readiness_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string allocation_engine_fact_json(
    const exposure::lattice_allocation_engine_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"target_node_weights\":" << json_quote(fact.target_node_weights)
      << ",\"accounting_numeraire_node_id\":"
      << json_quote(fact.accounting_numeraire_node_id)
      << ",\"accounting_numeraire_node_source\":"
      << json_quote(fact.accounting_numeraire_node_source)
      << ",\"base_policy_accounting_numeraire_node_id\":"
      << json_quote(fact.base_policy_accounting_numeraire_node_id)
      << ",\"accounting_numeraire_node_graph_bound\":"
      << bool_json(fact.accounting_numeraire_node_graph_bound)
      << ",\"numeraire_weight\":" << double_json(fact.numeraire_weight)
      << ",\"turnover\":" << double_json(fact.turnover)
      << ",\"objective_terms\":" << json_quote(fact.objective_terms)
      << ",\"cvar_loss\":" << double_json(fact.cvar_loss)
      << ",\"transaction_cost_estimate\":"
      << double_json(fact.transaction_cost_estimate)
      << ",\"constraint_diagnostics\":"
      << json_quote(fact.constraint_diagnostics)
      << ",\"cap_diagnostics\":" << json_quote(fact.cap_diagnostics)
      << ",\"scenario_growth_floor_status\":"
      << json_quote(fact.scenario_growth_floor_status)
      << ",\"fallback_reasons\":" << json_quote(fact.fallback_reasons)
      << ",\"derisk_reasons\":" << json_quote(fact.derisk_reasons)
      << ",\"observer_belief_fact_digest\":"
      << json_quote(fact.observer_belief_fact_digest)
      << ",\"forecast_artifact_digest\":"
      << json_quote(fact.forecast_artifact_digest)
      << ",\"base_policy_digest\":" << json_quote(fact.base_policy_digest)
      << ",\"deterministic_artifact\":"
      << bool_json(fact.deterministic_artifact)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"allocation_authority\":" << bool_json(fact.allocation_authority)
      << ",\"execution_authority\":" << bool_json(fact.execution_authority)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(fact.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(fact.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::allocation_engine_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::allocation_engine_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string allocation_engine_facts_json(
    const std::vector<exposure::lattice_allocation_engine_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << allocation_engine_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string allocation_engine_summary_json(
    const exposure::allocation_engine_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"allocation_engine_fact_count\":"
      << summary.allocation_engine_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"accounting_numeraire_node_bound_count\":"
      << summary.accounting_numeraire_node_bound_count
      << ",\"accounting_numeraire_node_source_base_policy_count\":"
      << summary.accounting_numeraire_node_source_base_policy_count
      << ",\"base_policy_accounting_numeraire_node_bound_count\":"
      << summary.base_policy_accounting_numeraire_node_bound_count
      << ",\"accounting_numeraire_node_base_policy_match_count\":"
      << summary.accounting_numeraire_node_base_policy_match_count
      << ",\"accounting_numeraire_node_graph_bound_count\":"
      << summary.accounting_numeraire_node_graph_bound_count
      << ",\"observer_belief_declared_count\":"
      << summary.observer_belief_declared_count
      << ",\"observer_belief_bound_count\":"
      << summary.observer_belief_bound_count
      << ",\"unresolved_observer_belief_count\":"
      << summary.unresolved_observer_belief_count
      << ",\"observer_belief_identity_mismatch_count\":"
      << summary.observer_belief_identity_mismatch_count
      << ",\"forecast_artifact_declared_count\":"
      << summary.forecast_artifact_declared_count
      << ",\"forecast_artifact_bound_count\":"
      << summary.forecast_artifact_bound_count
      << ",\"unresolved_forecast_artifact_count\":"
      << summary.unresolved_forecast_artifact_count
      << ",\"forecast_artifact_identity_mismatch_count\":"
      << summary.forecast_artifact_identity_mismatch_count
      << ",\"observer_forecast_artifact_mismatch_count\":"
      << summary.observer_forecast_artifact_mismatch_count
      << ",\"base_policy_bound_count\":" << summary.base_policy_bound_count
      << ",\"objective_terms_bound_count\":"
      << summary.objective_terms_bound_count
      << ",\"cvar_bound_count\":" << summary.cvar_bound_count
      << ",\"transaction_cost_bound_count\":"
      << summary.transaction_cost_bound_count
      << ",\"constraint_diagnostics_bound_count\":"
      << summary.constraint_diagnostics_bound_count
      << ",\"cap_diagnostics_bound_count\":"
      << summary.cap_diagnostics_bound_count
      << ",\"scenario_growth_floor_status_bound_count\":"
      << summary.scenario_growth_floor_status_bound_count
      << ",\"scenario_growth_floor_met_count\":"
      << summary.scenario_growth_floor_met_count
      << ",\"scenario_growth_floor_attention_count\":"
      << summary.scenario_growth_floor_attention_count
      << ",\"fallback_reason_contract_count\":"
      << summary.fallback_reason_contract_count
      << ",\"fallback_reason_none_count\":"
      << summary.fallback_reason_none_count
      << ",\"fallback_reason_active_count\":"
      << summary.fallback_reason_active_count
      << ",\"derisk_reason_contract_count\":"
      << summary.derisk_reason_contract_count
      << ",\"derisk_reason_none_count\":" << summary.derisk_reason_none_count
      << ",\"derisk_reason_active_count\":"
      << summary.derisk_reason_active_count
      << ",\"deterministic_artifact_count\":"
      << summary.deterministic_artifact_count
      << ",\"missing_accounting_numeraire_node_count\":"
      << summary.missing_accounting_numeraire_node_count
      << ",\"missing_accounting_numeraire_node_source_count\":"
      << summary.missing_accounting_numeraire_node_source_count
      << ",\"missing_base_policy_accounting_numeraire_node_count\":"
      << summary.missing_base_policy_accounting_numeraire_node_count
      << ",\"accounting_numeraire_node_source_mismatch_count\":"
      << summary.accounting_numeraire_node_source_mismatch_count
      << ",\"accounting_numeraire_node_base_policy_mismatch_count\":"
      << summary.accounting_numeraire_node_base_policy_mismatch_count
      << ",\"accounting_numeraire_node_not_graph_bound_count\":"
      << summary.accounting_numeraire_node_not_graph_bound_count
      << ",\"missing_observer_belief_count\":"
      << summary.missing_observer_belief_count
      << ",\"missing_forecast_artifact_count\":"
      << summary.missing_forecast_artifact_count
      << ",\"missing_base_policy_count\":" << summary.missing_base_policy_count
      << ",\"missing_cap_diagnostics_count\":"
      << summary.missing_cap_diagnostics_count
      << ",\"missing_scenario_growth_floor_status_count\":"
      << summary.missing_scenario_growth_floor_status_count
      << ",\"allocation_diagnostic_warning_count\":"
      << summary.allocation_diagnostic_warning_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"numeraire_weight\":"
      << source_analytics_metric_summary_json(summary.numeraire_weight)
      << ",\"turnover\":"
      << source_analytics_metric_summary_json(summary.turnover)
      << ",\"cvar_loss\":"
      << source_analytics_metric_summary_json(summary.cvar_loss)
      << ",\"transaction_cost_estimate\":"
      << source_analytics_metric_summary_json(summary.transaction_cost_estimate)
      << ",\"deterministic_artifact\":"
      << bool_json(summary.deterministic_artifact)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"allocation_authority\":" << bool_json(summary.allocation_authority)
      << ",\"execution_authority\":" << bool_json(summary.execution_authority)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(summary.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(summary.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string replay_environment_fact_json(
    const exposure::lattice_replay_environment_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range) << ",\"batch_index_path\":"
      << json_quote(fact.batch_index_path.lexically_normal().string())
      << ",\"experiment_index_path\":"
      << json_quote(fact.experiment_index_path.lexically_normal().string())
      << ",\"experiment_report_path\":"
      << json_quote(fact.experiment_report_path.lexically_normal().string())
      << ",\"runtime_replay_batch_index_schema\":"
      << json_quote(fact.runtime_replay_batch_index_schema)
      << ",\"runtime_replay_experiment_index_schema\":"
      << json_quote(fact.runtime_replay_experiment_index_schema)
      << ",\"replay_experiment_report_schema\":"
      << json_quote(fact.replay_experiment_report_schema)
      << ",\"experiment_index_report_digest\":"
      << json_quote(fact.experiment_index_report_digest)
      << ",\"experiment_report_digest\":"
      << json_quote(fact.experiment_report_digest)
      << ",\"experiment_id\":" << json_quote(fact.experiment_id)
      << ",\"runtime_run_id\":" << json_quote(fact.runtime_run_id)
      << ",\"environment_run_id\":" << json_quote(fact.environment_run_id)
      << ",\"execution_profile_digest\":"
      << json_quote(fact.execution_profile_digest)
      << ",\"policy_set_digest\":" << json_quote(fact.policy_set_digest)
      << ",\"replay_environment_version\":"
      << json_quote(fact.replay_environment_version)
      << ",\"replay_environment_component_assembly_id\":"
      << json_quote(fact.replay_environment_component_assembly_id)
      << ",\"replay_environment_world_mode\":"
      << json_quote(fact.replay_environment_world_mode)
      << ",\"replay_environment_api_contract\":"
      << json_quote(fact.replay_environment_api_contract)
      << ",\"replay_environment_spawn_model\":"
      << json_quote(fact.replay_environment_spawn_model)
      << ",\"replay_environment_range_source\":"
      << json_quote(fact.replay_environment_range_source)
      << ",\"replay_environment_source_range_policy\":"
      << json_quote(fact.replay_environment_source_range_policy)
      << ",\"replay_environment_source_order_policy\":"
      << json_quote(fact.replay_environment_source_order_policy)
      << ",\"replay_environment_range_resolution\":"
      << json_quote(fact.replay_environment_range_resolution)
      << ",\"replay_environment_observation_time_law\":"
      << json_quote(fact.replay_environment_observation_time_law)
      << ",\"replay_environment_realization_reveal\":"
      << json_quote(fact.replay_environment_realization_reveal)
      << ",\"replay_environment_realization_key_policy\":"
      << json_quote(fact.replay_environment_realization_key_policy)
      << ",\"replay_environment_action_kind\":"
      << json_quote(fact.replay_environment_action_kind)
      << ",\"replay_environment_action_time_policy\":"
      << json_quote(fact.replay_environment_action_time_policy)
      << ",\"replay_environment_graph_node_universe_policy\":"
      << json_quote(fact.replay_environment_graph_node_universe_policy)
      << ",\"replay_environment_reward_policy\":"
      << json_quote(fact.replay_environment_reward_policy)
      << ",\"replay_environment_projection_validation\":"
      << json_quote(fact.replay_environment_projection_validation)
      << ",\"replay_environment_policy_surface\":"
      << json_quote(fact.replay_environment_policy_surface)
      << ",\"replay_environment_action_policy_identity\":"
      << json_quote(fact.replay_environment_action_policy_identity)
      << ",\"replay_environment_initial_policy_kind\":"
      << json_quote(fact.replay_environment_initial_policy_kind)
      << ",\"replay_environment_experiment_task_identity\":"
      << json_quote(fact.replay_environment_experiment_task_identity)
      << ",\"replay_environment_experiment_run_identity\":"
      << json_quote(fact.replay_environment_experiment_run_identity)
      << ",\"replay_environment_step_artifact_identity\":"
      << json_quote(fact.replay_environment_step_artifact_identity)
      << ",\"replay_environment_experiment_report_count_policy\":"
      << json_quote(fact.replay_environment_experiment_report_count_policy)
      << ",\"replay_environment_artifact_schema\":"
      << json_quote(fact.replay_environment_artifact_schema)
      << ",\"replay_environment_lattice_fact_family\":"
      << json_quote(fact.replay_environment_lattice_fact_family)
      << ",\"replay_environment_lattice_target\":"
      << json_quote(fact.replay_environment_lattice_target)
      << ",\"replay_environment_require_resolved_cursor\":"
      << bool_json(fact.replay_environment_require_resolved_cursor)
      << ",\"replay_environment_require_no_future_leakage\":"
      << bool_json(fact.replay_environment_require_no_future_leakage)
      << ",\"replay_environment_require_projection_validation\":"
      << bool_json(fact.replay_environment_require_projection_validation)
      << ",\"replay_environment_default_max_parallel_jobs\":"
      << fact.replay_environment_default_max_parallel_jobs
      << ",\"experiment_requested_max_parallel_jobs\":"
      << fact.experiment_requested_max_parallel_jobs
      << ",\"experiment_resolved_parallelism\":"
      << fact.experiment_resolved_parallelism
      << ",\"batch_entry_count\":" << fact.batch_entry_count
      << ",\"experiment_entry_count\":" << fact.experiment_entry_count
      << ",\"replay_bundle_count\":" << fact.replay_bundle_count
      << ",\"policy_count\":" << fact.policy_count
      << ",\"policy_summary_count\":" << fact.policy_summary_count
      << ",\"attempted_count\":" << fact.attempted_count
      << ",\"completed_count\":" << fact.completed_count
      << ",\"failed_count\":" << fact.failed_count
      << ",\"episode_count\":" << fact.episode_count
      << ",\"episode_requested_range_bound_count\":"
      << fact.episode_requested_range_bound_count
      << ",\"episode_cursor_bound_count\":" << fact.episode_cursor_bound_count
      << ",\"episode_anchor_interval_bound_count\":"
      << fact.episode_anchor_interval_bound_count
      << ",\"episode_anchor_keys_bound_count\":"
      << fact.episode_anchor_keys_bound_count
      << ",\"time_law_expected_step_count\":"
      << fact.time_law_expected_step_count
      << ",\"time_law_observation_step_count\":"
      << fact.time_law_observation_step_count
      << ",\"time_law_action_step_count\":" << fact.time_law_action_step_count
      << ",\"time_law_execution_step_count\":"
      << fact.time_law_execution_step_count
      << ",\"time_law_realization_after_action_count\":"
      << fact.time_law_realization_after_action_count
      << ",\"time_law_future_observation_violation_count\":"
      << fact.time_law_future_observation_violation_count
      << ",\"mixed_future_realization_key_count\":"
      << fact.mixed_future_realization_key_count
      << ",\"projection_validation_step_count\":"
      << fact.projection_validation_step_count
      << ",\"cajtucu_valid_trace_count\":" << fact.cajtucu_valid_trace_count
      << ",\"cajtucu_invalid_trace_count\":" << fact.cajtucu_invalid_trace_count
      << ",\"cajtucu_missing_direct_pair_count\":"
      << fact.cajtucu_missing_direct_pair_count
      << ",\"cajtucu_numeraire_fallback_pair_count\":"
      << fact.cajtucu_numeraire_fallback_pair_count
      << ",\"cajtucu_nontradable_edge_reject_count\":"
      << fact.cajtucu_nontradable_edge_reject_count
      << ",\"cajtucu_below_min_notional_reject_count\":"
      << fact.cajtucu_below_min_notional_reject_count
      << ",\"cajtucu_above_max_notional_reject_count\":"
      << fact.cajtucu_above_max_notional_reject_count
      << ",\"cajtucu_insufficient_sell_units_reject_count\":"
      << fact.cajtucu_insufficient_sell_units_reject_count
      << ",\"cajtucu_insufficient_units_reject_count\":"
      << fact.cajtucu_insufficient_units_reject_count
      << ",\"cajtucu_invalid_sell_price_count\":"
      << fact.cajtucu_invalid_sell_price_count
      << ",\"cajtucu_large_equity_mismatch_count\":"
      << fact.cajtucu_large_equity_mismatch_count
      << ",\"cajtucu_synthetic_market_step_count\":"
      << fact.cajtucu_synthetic_market_step_count
      << ",\"requested_order_count\":" << fact.requested_order_count
      << ",\"executed_order_count\":" << fact.executed_order_count
      << ",\"rejected_order_count\":" << fact.rejected_order_count
      << ",\"partial_order_count\":" << fact.partial_order_count
      << ",\"mean_total_reward\":" << double_json(fact.mean_total_reward)
      << ",\"mean_total_log_growth\":"
      << double_json(fact.mean_total_log_growth)
      << ",\"mean_final_equity_numeraire\":"
      << double_json(fact.mean_final_equity_numeraire)
      << ",\"mean_max_drawdown\":" << double_json(fact.mean_max_drawdown)
      << ",\"mean_total_turnover\":" << double_json(fact.mean_total_turnover)
      << ",\"mean_total_transaction_cost_numeraire\":"
      << double_json(fact.mean_total_transaction_cost_numeraire)
      << ",\"requested_notional_numeraire\":"
      << double_json(fact.requested_notional_numeraire)
      << ",\"executed_notional_numeraire\":"
      << double_json(fact.executed_notional_numeraire)
      << ",\"rejected_notional_numeraire\":"
      << double_json(fact.rejected_notional_numeraire)
      << ",\"fill_ratio\":" << double_json(fact.fill_ratio)
      << ",\"fee_cost_numeraire\":" << double_json(fact.fee_cost_numeraire)
      << ",\"spread_cost_numeraire\":"
      << double_json(fact.spread_cost_numeraire)
      << ",\"slippage_cost_numeraire\":"
      << double_json(fact.slippage_cost_numeraire)
      << ",\"mean_target_weight_error_l1\":"
      << double_json(fact.mean_target_weight_error_l1)
      << ",\"mean_target_weight_error_linf\":"
      << double_json(fact.mean_target_weight_error_linf)
      << ",\"mean_projection_mae\":" << double_json(fact.mean_projection_mae)
      << ",\"mean_projection_signed_bias\":"
      << double_json(fact.mean_projection_signed_bias)
      << ",\"mean_projection_directional_accuracy\":"
      << double_json(fact.mean_projection_directional_accuracy)
      << ",\"mean_projection_interval_coverage\":"
      << double_json(fact.mean_projection_interval_coverage)
      << ",\"artifact_evidence\":" << bool_json(fact.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"replay_executor\":" << bool_json(fact.replay_executor)
      << ",\"allocation_authority\":" << bool_json(fact.allocation_authority)
      << ",\"execution_authority\":" << bool_json(fact.execution_authority)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(fact.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(fact.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::replay_environment_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::replay_environment_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string replay_environment_facts_json(
    const std::vector<exposure::lattice_replay_environment_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << replay_environment_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string replay_environment_summary_json(
    const exposure::replay_environment_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"replay_environment_fact_count\":"
      << summary.replay_environment_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"batch_index_bound_count\":" << summary.batch_index_bound_count
      << ",\"experiment_index_bound_count\":"
      << summary.experiment_index_bound_count
      << ",\"experiment_report_bound_count\":"
      << summary.experiment_report_bound_count
      << ",\"batch_index_schema_bound_count\":"
      << summary.batch_index_schema_bound_count
      << ",\"experiment_index_schema_bound_count\":"
      << summary.experiment_index_schema_bound_count
      << ",\"experiment_report_schema_bound_count\":"
      << summary.experiment_report_schema_bound_count
      << ",\"experiment_index_report_digest_bound_count\":"
      << summary.experiment_index_report_digest_bound_count
      << ",\"experiment_report_digest_bound_count\":"
      << summary.experiment_report_digest_bound_count
      << ",\"experiment_report_digest_match_count\":"
      << summary.experiment_report_digest_match_count
      << ",\"experiment_report_digest_mismatch_count\":"
      << summary.experiment_report_digest_mismatch_count
      << ",\"experiment_id_bound_count\":" << summary.experiment_id_bound_count
      << ",\"runtime_run_id_bound_count\":"
      << summary.runtime_run_id_bound_count
      << ",\"environment_run_id_bound_count\":"
      << summary.environment_run_id_bound_count
      << ",\"execution_profile_digest_bound_count\":"
      << summary.execution_profile_digest_bound_count
      << ",\"policy_set_digest_bound_count\":"
      << summary.policy_set_digest_bound_count
      << ",\"replay_contract_version_bound_count\":"
      << summary.replay_contract_version_bound_count
      << ",\"replay_contract_component_bound_count\":"
      << summary.replay_contract_component_bound_count
      << ",\"replay_contract_world_mode_bound_count\":"
      << summary.replay_contract_world_mode_bound_count
      << ",\"replay_contract_api_bound_count\":"
      << summary.replay_contract_api_bound_count
      << ",\"replay_contract_spawn_model_bound_count\":"
      << summary.replay_contract_spawn_model_bound_count
      << ",\"replay_contract_range_source_bound_count\":"
      << summary.replay_contract_range_source_bound_count
      << ",\"replay_contract_source_range_policy_bound_count\":"
      << summary.replay_contract_source_range_policy_bound_count
      << ",\"replay_contract_source_order_policy_bound_count\":"
      << summary.replay_contract_source_order_policy_bound_count
      << ",\"replay_contract_range_resolution_bound_count\":"
      << summary.replay_contract_range_resolution_bound_count
      << ",\"replay_contract_policy_surface_bound_count\":"
      << summary.replay_contract_policy_surface_bound_count
      << ",\"replay_contract_time_law_bound_count\":"
      << summary.replay_contract_time_law_bound_count
      << ",\"replay_contract_realization_reveal_bound_count\":"
      << summary.replay_contract_realization_reveal_bound_count
      << ",\"replay_contract_realization_key_bound_count\":"
      << summary.replay_contract_realization_key_bound_count
      << ",\"replay_contract_action_kind_bound_count\":"
      << summary.replay_contract_action_kind_bound_count
      << ",\"replay_contract_action_time_policy_bound_count\":"
      << summary.replay_contract_action_time_policy_bound_count
      << ",\"replay_contract_graph_node_universe_policy_bound_count\":"
      << summary.replay_contract_graph_node_universe_policy_bound_count
      << ",\"replay_contract_reward_policy_bound_count\":"
      << summary.replay_contract_reward_policy_bound_count
      << ",\"replay_contract_projection_validation_bound_count\":"
      << summary.replay_contract_projection_validation_bound_count
      << ",\"replay_contract_action_policy_identity_bound_count\":"
      << summary.replay_contract_action_policy_identity_bound_count
      << ",\"replay_contract_initial_policy_kind_bound_count\":"
      << summary.replay_contract_initial_policy_kind_bound_count
      << ",\"replay_contract_task_identity_bound_count\":"
      << summary.replay_contract_task_identity_bound_count
      << ",\"replay_contract_experiment_run_identity_bound_count\":"
      << summary.replay_contract_experiment_run_identity_bound_count
      << ",\"replay_contract_step_artifact_identity_bound_count\":"
      << summary.replay_contract_step_artifact_identity_bound_count
      << ",\"replay_contract_report_count_policy_bound_count\":"
      << summary.replay_contract_report_count_policy_bound_count
      << ",\"replay_contract_parallelism_bound_count\":"
      << summary.replay_contract_parallelism_bound_count
      << ",\"replay_contract_artifact_schema_bound_count\":"
      << summary.replay_contract_artifact_schema_bound_count
      << ",\"replay_contract_lattice_binding_bound_count\":"
      << summary.replay_contract_lattice_binding_bound_count
      << ",\"replay_contract_guard_bound_count\":"
      << summary.replay_contract_guard_bound_count
      << ",\"completed_all_attempted_count\":"
      << summary.completed_all_attempted_count
      << ",\"projection_validation_metric_bound_count\":"
      << summary.projection_validation_metric_bound_count
      << ",\"time_law_step_evidence_bound_count\":"
      << summary.time_law_step_evidence_bound_count
      << ",\"projection_validation_step_evidence_bound_count\":"
      << summary.projection_validation_step_evidence_bound_count
      << ",\"episode_requested_range_bound_count_total\":"
      << summary.episode_requested_range_bound_count_total
      << ",\"episode_cursor_bound_count_total\":"
      << summary.episode_cursor_bound_count_total
      << ",\"episode_anchor_interval_bound_count_total\":"
      << summary.episode_anchor_interval_bound_count_total
      << ",\"episode_anchor_keys_bound_count_total\":"
      << summary.episode_anchor_keys_bound_count_total
      << ",\"time_law_expected_step_count_total\":"
      << summary.time_law_expected_step_count_total
      << ",\"time_law_observation_step_count_total\":"
      << summary.time_law_observation_step_count_total
      << ",\"time_law_action_step_count_total\":"
      << summary.time_law_action_step_count_total
      << ",\"time_law_execution_step_count_total\":"
      << summary.time_law_execution_step_count_total
      << ",\"time_law_realization_after_action_count_total\":"
      << summary.time_law_realization_after_action_count_total
      << ",\"time_law_future_observation_violation_count_total\":"
      << summary.time_law_future_observation_violation_count_total
      << ",\"mixed_future_realization_key_count_total\":"
      << summary.mixed_future_realization_key_count_total
      << ",\"projection_validation_step_count_total\":"
      << summary.projection_validation_step_count_total
      << ",\"cajtucu_valid_trace_count_total\":"
      << summary.cajtucu_valid_trace_count_total
      << ",\"cajtucu_invalid_trace_count_total\":"
      << summary.cajtucu_invalid_trace_count_total
      << ",\"cajtucu_missing_direct_pair_count_total\":"
      << summary.cajtucu_missing_direct_pair_count_total
      << ",\"cajtucu_numeraire_fallback_pair_count_total\":"
      << summary.cajtucu_numeraire_fallback_pair_count_total
      << ",\"cajtucu_nontradable_edge_reject_count_total\":"
      << summary.cajtucu_nontradable_edge_reject_count_total
      << ",\"cajtucu_below_min_notional_reject_count_total\":"
      << summary.cajtucu_below_min_notional_reject_count_total
      << ",\"cajtucu_above_max_notional_reject_count_total\":"
      << summary.cajtucu_above_max_notional_reject_count_total
      << ",\"cajtucu_insufficient_sell_units_reject_count_total\":"
      << summary.cajtucu_insufficient_sell_units_reject_count_total
      << ",\"cajtucu_insufficient_units_reject_count_total\":"
      << summary.cajtucu_insufficient_units_reject_count_total
      << ",\"cajtucu_invalid_sell_price_count_total\":"
      << summary.cajtucu_invalid_sell_price_count_total
      << ",\"cajtucu_large_equity_mismatch_count_total\":"
      << summary.cajtucu_large_equity_mismatch_count_total
      << ",\"cajtucu_synthetic_market_step_count_total\":"
      << summary.cajtucu_synthetic_market_step_count_total
      << ",\"requested_order_count_total\":"
      << summary.requested_order_count_total
      << ",\"executed_order_count_total\":"
      << summary.executed_order_count_total
      << ",\"rejected_order_count_total\":"
      << summary.rejected_order_count_total
      << ",\"partial_order_count_total\":" << summary.partial_order_count_total
      << ",\"missing_batch_index_count\":" << summary.missing_batch_index_count
      << ",\"missing_experiment_index_count\":"
      << summary.missing_experiment_index_count
      << ",\"missing_experiment_report_count\":"
      << summary.missing_experiment_report_count
      << ",\"missing_execution_profile_digest_count\":"
      << summary.missing_execution_profile_digest_count
      << ",\"missing_policy_set_digest_count\":"
      << summary.missing_policy_set_digest_count
      << ",\"incomplete_replay_attempt_count\":"
      << summary.incomplete_replay_attempt_count
      << ",\"missing_projection_validation_metric_count\":"
      << summary.missing_projection_validation_metric_count
      << ",\"missing_time_law_step_evidence_count\":"
      << summary.missing_time_law_step_evidence_count
      << ",\"missing_projection_validation_step_evidence_count\":"
      << summary.missing_projection_validation_step_evidence_count
      << ",\"missing_cajtucu_trace_evidence_count\":"
      << summary.missing_cajtucu_trace_evidence_count
      << ",\"cajtucu_invalid_trace_fact_count\":"
      << summary.cajtucu_invalid_trace_fact_count
      << ",\"cajtucu_synthetic_market_fact_count\":"
      << summary.cajtucu_synthetic_market_fact_count
      << ",\"missing_cajtucu_cost_metric_count\":"
      << summary.missing_cajtucu_cost_metric_count
      << ",\"invalid_cajtucu_order_metric_count\":"
      << summary.invalid_cajtucu_order_metric_count
      << ",\"invalid_cajtucu_notional_metric_count\":"
      << summary.invalid_cajtucu_notional_metric_count
      << ",\"invalid_cajtucu_fill_ratio_count\":"
      << summary.invalid_cajtucu_fill_ratio_count
      << ",\"invalid_cajtucu_target_tracking_count\":"
      << summary.invalid_cajtucu_target_tracking_count
      << ",\"time_law_violation_count\":" << summary.time_law_violation_count
      << ",\"missing_episode_requested_range_count\":"
      << summary.missing_episode_requested_range_count
      << ",\"missing_episode_cursor_evidence_count\":"
      << summary.missing_episode_cursor_evidence_count
      << ",\"missing_episode_anchor_interval_count\":"
      << summary.missing_episode_anchor_interval_count
      << ",\"missing_episode_anchor_keys_count\":"
      << summary.missing_episode_anchor_keys_count
      << ",\"artifact_evidence_count\":" << summary.artifact_evidence_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"batch_entry_count_total\":" << summary.batch_entry_count_total
      << ",\"experiment_entry_count_total\":"
      << summary.experiment_entry_count_total
      << ",\"replay_bundle_count_total\":" << summary.replay_bundle_count_total
      << ",\"policy_count_total\":" << summary.policy_count_total
      << ",\"attempted_count_total\":" << summary.attempted_count_total
      << ",\"completed_count_total\":" << summary.completed_count_total
      << ",\"mean_total_reward\":"
      << source_analytics_metric_summary_json(summary.mean_total_reward)
      << ",\"mean_total_log_growth\":"
      << source_analytics_metric_summary_json(summary.mean_total_log_growth)
      << ",\"mean_final_equity_numeraire\":"
      << source_analytics_metric_summary_json(
             summary.mean_final_equity_numeraire)
      << ",\"mean_max_drawdown\":"
      << source_analytics_metric_summary_json(summary.mean_max_drawdown)
      << ",\"mean_total_turnover\":"
      << source_analytics_metric_summary_json(summary.mean_total_turnover)
      << ",\"mean_total_transaction_cost_numeraire\":"
      << source_analytics_metric_summary_json(
             summary.mean_total_transaction_cost_numeraire)
      << ",\"requested_notional_numeraire\":"
      << source_analytics_metric_summary_json(
             summary.requested_notional_numeraire)
      << ",\"executed_notional_numeraire\":"
      << source_analytics_metric_summary_json(
             summary.executed_notional_numeraire)
      << ",\"rejected_notional_numeraire\":"
      << source_analytics_metric_summary_json(
             summary.rejected_notional_numeraire)
      << ",\"fill_ratio\":"
      << source_analytics_metric_summary_json(summary.fill_ratio)
      << ",\"fee_cost_numeraire\":"
      << source_analytics_metric_summary_json(summary.fee_cost_numeraire)
      << ",\"spread_cost_numeraire\":"
      << source_analytics_metric_summary_json(summary.spread_cost_numeraire)
      << ",\"slippage_cost_numeraire\":"
      << source_analytics_metric_summary_json(summary.slippage_cost_numeraire)
      << ",\"mean_target_weight_error_l1\":"
      << source_analytics_metric_summary_json(
             summary.mean_target_weight_error_l1)
      << ",\"mean_target_weight_error_linf\":"
      << source_analytics_metric_summary_json(
             summary.mean_target_weight_error_linf)
      << ",\"mean_projection_mae\":"
      << source_analytics_metric_summary_json(summary.mean_projection_mae)
      << ",\"mean_projection_signed_bias\":"
      << source_analytics_metric_summary_json(
             summary.mean_projection_signed_bias)
      << ",\"mean_projection_directional_accuracy\":"
      << source_analytics_metric_summary_json(
             summary.mean_projection_directional_accuracy)
      << ",\"mean_projection_interval_coverage\":"
      << source_analytics_metric_summary_json(
             summary.mean_projection_interval_coverage)
      << ",\"artifact_evidence\":" << bool_json(summary.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"replay_executor\":" << bool_json(summary.replay_executor)
      << ",\"allocation_authority\":" << bool_json(summary.allocation_authority)
      << ",\"execution_authority\":" << bool_json(summary.execution_authority)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(summary.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(summary.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string policy_training_fact_json(
    const exposure::lattice_policy_training_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"policy_id\":" << json_quote(fact.policy_id)
      << ",\"policy_kind\":" << json_quote(fact.policy_kind)
      << ",\"policy_architecture_digest\":"
      << json_quote(fact.policy_architecture_digest)
      << ",\"training_config_digest\":"
      << json_quote(fact.training_config_digest)
      << ",\"training_range_digest\":" << json_quote(fact.training_range_digest)
      << ",\"validation_range_digest\":"
      << json_quote(fact.validation_range_digest)
      << ",\"test_range_digest\":" << json_quote(fact.test_range_digest)
      << ",\"environment_contract_id\":"
      << json_quote(fact.environment_contract_id)
      << ",\"observation_schema_digest\":"
      << json_quote(fact.observation_schema_digest)
      << ",\"action_schema_digest\":" << json_quote(fact.action_schema_digest)
      << ",\"reward_contract_digest\":"
      << json_quote(fact.reward_contract_digest)
      << ",\"execution_profile_digest\":"
      << json_quote(fact.execution_profile_digest)
      << ",\"normalization_fit_range_digest\":"
      << json_quote(fact.normalization_fit_range_digest)
      << ",\"replay_buffer_source_range_digest\":"
      << json_quote(fact.replay_buffer_source_range_digest)
      << ",\"early_stopping_policy_digest\":"
      << json_quote(fact.early_stopping_policy_digest)
      << ",\"hyperparameter_selection_policy_digest\":"
      << json_quote(fact.hyperparameter_selection_policy_digest)
      << ",\"selector_split\":" << json_quote(fact.selector_split)
      << ",\"selector_policy_digest\":"
      << json_quote(fact.selector_policy_digest)
      << ",\"parent_checkpoint_digest\":"
      << json_quote(fact.parent_checkpoint_digest)
      << ",\"checkpoint_digest\":" << json_quote(fact.checkpoint_digest)
      << ",\"parent_forecast_eval_fact_digest\":"
      << json_quote(fact.parent_forecast_eval_fact_digest)
      << ",\"parent_observer_belief_fact_digest\":"
      << json_quote(fact.parent_observer_belief_fact_digest)
      << ",\"parent_allocation_engine_fact_digest\":"
      << json_quote(fact.parent_allocation_engine_fact_digest)
      << ",\"parent_replay_environment_fact_digest\":"
      << json_quote(fact.parent_replay_environment_fact_digest)
      << ",\"parent_replay_environment_report_digest\":"
      << json_quote(fact.parent_replay_environment_report_digest)
      << ",\"random_seed\":" << fact.random_seed
      << ",\"random_seed_bound\":" << bool_json(fact.random_seed_bound)
      << ",\"training_range_disjoint_validation\":"
      << bool_json(fact.training_range_disjoint_validation)
      << ",\"training_range_disjoint_test\":"
      << bool_json(fact.training_range_disjoint_test)
      << ",\"validation_range_disjoint_test\":"
      << bool_json(fact.validation_range_disjoint_test)
      << ",\"normalization_fit_training_only\":"
      << bool_json(fact.normalization_fit_training_only)
      << ",\"replay_buffer_training_only\":"
      << bool_json(fact.replay_buffer_training_only)
      << ",\"reward_baseline_training_only\":"
      << bool_json(fact.reward_baseline_training_only)
      << ",\"early_stopping_uses_validation_only\":"
      << bool_json(fact.early_stopping_uses_validation_only)
      << ",\"hyperparameter_selection_uses_validation_only\":"
      << bool_json(fact.hyperparameter_selection_uses_validation_only)
      << ",\"test_sealed_until_final_report\":"
      << bool_json(fact.test_sealed_until_final_report)
      << ",\"test_first_access_after_selection\":"
      << bool_json(fact.test_first_access_after_selection)
      << ",\"runtime_job_kind_bound\":"
      << bool_json(fact.runtime_job_kind_bound)
      << ",\"policy_checkpoint_written\":"
      << bool_json(fact.policy_checkpoint_written)
      << ",\"artifact_evidence\":" << bool_json(fact.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"runtime_trainer\":" << bool_json(fact.runtime_trainer)
      << ",\"policy_training_authority\":"
      << bool_json(fact.policy_training_authority)
      << ",\"policy_selector\":" << bool_json(fact.policy_selector)
      << ",\"allocation_authority\":" << bool_json(fact.allocation_authority)
      << ",\"execution_authority\":" << bool_json(fact.execution_authority)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"quality_authority\":" << bool_json(fact.quality_authority)
      << ",\"performance_authority\":" << bool_json(fact.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(fact.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(fact.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(fact.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(fact.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(fact.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(fact.contract_identity_authority) << ",\"issues\":"
      << string_array_json(exposure::policy_training_fact_issues(fact))
      << ",\"digest\":"
      << json_quote(exposure::policy_training_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string policy_training_facts_json(
    const std::vector<exposure::lattice_policy_training_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << policy_training_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string policy_training_summary_json(
    const exposure::policy_training_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"policy_training_fact_count\":"
      << summary.policy_training_fact_count
      << ",\"parent_exposure_fact_count\":"
      << summary.parent_exposure_fact_count
      << ",\"policy_id_bound_count\":" << summary.policy_id_bound_count
      << ",\"policy_kind_bound_count\":" << summary.policy_kind_bound_count
      << ",\"architecture_digest_bound_count\":"
      << summary.architecture_digest_bound_count
      << ",\"training_config_digest_bound_count\":"
      << summary.training_config_digest_bound_count
      << ",\"training_range_digest_bound_count\":"
      << summary.training_range_digest_bound_count
      << ",\"validation_range_digest_bound_count\":"
      << summary.validation_range_digest_bound_count
      << ",\"test_range_digest_bound_count\":"
      << summary.test_range_digest_bound_count
      << ",\"environment_contract_bound_count\":"
      << summary.environment_contract_bound_count
      << ",\"observation_schema_bound_count\":"
      << summary.observation_schema_bound_count
      << ",\"action_schema_bound_count\":" << summary.action_schema_bound_count
      << ",\"reward_contract_bound_count\":"
      << summary.reward_contract_bound_count
      << ",\"policy_input_schema_bound_count\":"
      << summary.policy_input_schema_bound_count
      << ",\"action_adapter_bound_count\":"
      << summary.action_adapter_bound_count
      << ",\"action_distribution_bound_count\":"
      << summary.action_distribution_bound_count
      << ",\"reward_contract_id_bound_count\":"
      << summary.reward_contract_id_bound_count
      << ",\"execution_profile_digest_bound_count\":"
      << summary.execution_profile_digest_bound_count
      << ",\"ppo_policy_artifact_contract_bound_count\":"
      << summary.ppo_policy_artifact_contract_bound_count
      << ",\"ppo_policy_family_bound_count\":"
      << summary.ppo_policy_family_bound_count
      << ",\"actor_architecture_bound_count\":"
      << summary.actor_architecture_bound_count
      << ",\"critic_architecture_bound_count\":"
      << summary.critic_architecture_bound_count
      << ",\"input_policy_checkpoint_bound_count\":"
      << summary.input_policy_checkpoint_bound_count
      << ",\"actor_checkpoint_bound_count\":"
      << summary.actor_checkpoint_bound_count
      << ",\"critic_checkpoint_bound_count\":"
      << summary.critic_checkpoint_bound_count
      << ",\"optimizer_state_bound_count\":"
      << summary.optimizer_state_bound_count
      << ",\"ppo_config_bound_count\":" << summary.ppo_config_bound_count
      << ",\"rollout_collection_bound_count\":"
      << summary.rollout_collection_bound_count
      << ",\"ppo_update_report_bound_count\":"
      << summary.ppo_update_report_bound_count
      << ",\"ppo_hyperparameter_bound_count\":"
      << summary.ppo_hyperparameter_bound_count
      << ",\"normalization_fit_range_bound_count\":"
      << summary.normalization_fit_range_bound_count
      << ",\"replay_buffer_source_range_bound_count\":"
      << summary.replay_buffer_source_range_bound_count
      << ",\"early_stopping_policy_bound_count\":"
      << summary.early_stopping_policy_bound_count
      << ",\"hyperparameter_selection_policy_bound_count\":"
      << summary.hyperparameter_selection_policy_bound_count
      << ",\"selector_split_bound_count\":"
      << summary.selector_split_bound_count
      << ",\"selector_policy_bound_count\":"
      << summary.selector_policy_bound_count
      << ",\"random_seed_bound_count\":" << summary.random_seed_bound_count
      << ",\"parent_checkpoint_bound_count\":"
      << summary.parent_checkpoint_bound_count
      << ",\"checkpoint_digest_bound_count\":"
      << summary.checkpoint_digest_bound_count
      << ",\"forecast_eval_declared_count\":"
      << summary.forecast_eval_declared_count
      << ",\"forecast_eval_bound_count\":" << summary.forecast_eval_bound_count
      << ",\"unresolved_forecast_eval_count\":"
      << summary.unresolved_forecast_eval_count
      << ",\"forecast_eval_identity_mismatch_count\":"
      << summary.forecast_eval_identity_mismatch_count
      << ",\"observer_belief_declared_count\":"
      << summary.observer_belief_declared_count
      << ",\"observer_belief_bound_count\":"
      << summary.observer_belief_bound_count
      << ",\"unresolved_observer_belief_count\":"
      << summary.unresolved_observer_belief_count
      << ",\"observer_belief_identity_mismatch_count\":"
      << summary.observer_belief_identity_mismatch_count
      << ",\"allocation_engine_declared_count\":"
      << summary.allocation_engine_declared_count
      << ",\"allocation_engine_bound_count\":"
      << summary.allocation_engine_bound_count
      << ",\"unresolved_allocation_engine_count\":"
      << summary.unresolved_allocation_engine_count
      << ",\"allocation_engine_identity_mismatch_count\":"
      << summary.allocation_engine_identity_mismatch_count
      << ",\"replay_environment_declared_count\":"
      << summary.replay_environment_declared_count
      << ",\"replay_environment_bound_count\":"
      << summary.replay_environment_bound_count
      << ",\"unresolved_replay_environment_count\":"
      << summary.unresolved_replay_environment_count
      << ",\"replay_environment_identity_mismatch_count\":"
      << summary.replay_environment_identity_mismatch_count
      << ",\"disjoint_range_contract_count\":"
      << summary.disjoint_range_contract_count
      << ",\"training_only_normalization_count\":"
      << summary.training_only_normalization_count
      << ",\"training_only_replay_buffer_count\":"
      << summary.training_only_replay_buffer_count
      << ",\"training_only_reward_baseline_count\":"
      << summary.training_only_reward_baseline_count
      << ",\"validation_selector_policy_count\":"
      << summary.validation_selector_policy_count
      << ",\"sealed_test_count\":" << summary.sealed_test_count
      << ",\"runtime_job_kind_bound_count\":"
      << summary.runtime_job_kind_bound_count
      << ",\"policy_checkpoint_written_count\":"
      << summary.policy_checkpoint_written_count
      << ",\"artifact_evidence_count\":" << summary.artifact_evidence_count
      << ",\"leakage_contract_issue_count\":"
      << summary.leakage_contract_issue_count
      << ",\"authority_drift_count\":" << summary.authority_drift_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"artifact_evidence\":" << bool_json(summary.artifact_evidence)
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"runtime_trainer\":" << bool_json(summary.runtime_trainer)
      << ",\"policy_training_authority\":"
      << bool_json(summary.policy_training_authority)
      << ",\"policy_selector\":" << bool_json(summary.policy_selector)
      << ",\"allocation_authority\":" << bool_json(summary.allocation_authority)
      << ",\"execution_authority\":" << bool_json(summary.execution_authority)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(summary.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(summary.deployment_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string selection_signal_fact_json(
    const exposure::lattice_selection_signal_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"selector_id\":" << json_quote(fact.selector_id)
      << ",\"selector_kind\":" << json_quote(fact.selector_kind)
      << ",\"selector_rule\":" << json_quote(fact.selector_rule)
      << ",\"selector_split\":" << json_quote(fact.selector_split)
      << ",\"selector_metric\":" << json_quote(fact.selector_metric)
      << ",\"tie_policy\":" << json_quote(fact.tie_policy)
      << ",\"selected_checkpoint_source\":"
      << json_quote(fact.selected_checkpoint_source)
      << ",\"selected_checkpoint\":"
      << json_quote(fact.selected_checkpoint.lexically_normal().string())
      << ",\"output_checkpoint\":"
      << json_quote(fact.output_checkpoint.lexically_normal().string())
      << ",\"input_checkpoints\":" << path_array_json(fact.input_checkpoints)
      << ",\"candidate_checkpoints\":"
      << path_array_json(fact.candidate_checkpoints)
      << ",\"candidate_checkpoint_count\":" << fact.candidate_checkpoint_count
      << ",\"candidate_checkpoint_digest\":"
      << json_quote(fact.candidate_checkpoint_digest)
      << ",\"parent_evaluation_fact_digests\":"
      << string_array_json(fact.parent_evaluation_fact_digests)
      << ",\"mutated_component\":" << bool_json(fact.mutated_component)
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"selection_footprint\":" << interval_json(fact.selection_footprint)
      << ",\"footprint_basis\":" << json_quote(fact.footprint_basis)
      << ",\"source_key_window\":{\"precision\":"
      << json_quote(fact.source_key_footprint_precision)
      << ",\"first_anchor_key\":" << json_quote(fact.first_anchor_key)
      << ",\"last_anchor_key\":" << json_quote(fact.last_anchor_key)
      << ",\"observed_begin\":" << json_quote(fact.observed_source_key_begin)
      << ",\"observed_end\":" << json_quote(fact.observed_source_key_end)
      << ",\"target_begin\":" << json_quote(fact.target_source_key_begin)
      << ",\"target_end\":" << json_quote(fact.target_source_key_end) << "}"
      << ",\"read_only_lattice_fact\":true"
      << ",\"runtime_executor\":false"
      << ",\"coverage_authority\":false"
      << ",\"contract_identity_authority\":false"
      << ",\"leakage_relevant_when_forbidden\":true"
      << ",\"digest\":"
      << json_quote(exposure::selection_signal_fact_digest(fact))
      << ",\"event_digest\":"
      << json_quote(exposure::selection_signal_event_digest(fact))
      << ",\"provenance_digest\":"
      << json_quote(exposure::selection_signal_provenance_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string selection_signal_facts_json(
    const std::vector<exposure::lattice_selection_signal_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << selection_signal_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string selection_signal_summary_json(
    const exposure::selection_signal_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"selection_signal_fact_count\":"
      << summary.selection_signal_fact_count
      << ",\"unique_selector_count\":" << summary.unique_selector_count
      << ",\"selected_checkpoint_count\":" << summary.selected_checkpoint_count
      << ",\"missing_selected_checkpoint_count\":"
      << summary.missing_selected_checkpoint_count
      << ",\"selector_split_bound_count\":"
      << summary.selector_split_bound_count
      << ",\"selector_metric_bound_count\":"
      << summary.selector_metric_bound_count
      << ",\"tie_policy_bound_count\":" << summary.tie_policy_bound_count
      << ",\"candidate_checkpoint_bound_count\":"
      << summary.candidate_checkpoint_bound_count
      << ",\"candidate_checkpoint_digest_bound_count\":"
      << summary.candidate_checkpoint_digest_bound_count
      << ",\"missing_candidate_checkpoint_count\":"
      << summary.missing_candidate_checkpoint_count
      << ",\"missing_candidate_checkpoint_digest_count\":"
      << summary.missing_candidate_checkpoint_digest_count
      << ",\"candidate_checkpoint_digest_mismatch_count\":"
      << summary.candidate_checkpoint_digest_mismatch_count
      << ",\"parent_evaluation_fact_bound_count\":"
      << summary.parent_evaluation_fact_bound_count
      << ",\"parent_evaluation_fact_declared_count\":"
      << summary.parent_evaluation_fact_declared_count
      << ",\"unresolved_parent_evaluation_fact_count\":"
      << summary.unresolved_parent_evaluation_fact_count
      << ",\"parent_evaluation_fact_identity_mismatch_count\":"
      << summary.parent_evaluation_fact_identity_mismatch_count
      << ",\"parent_evaluation_back_reference_mismatch_count\":"
      << summary.parent_evaluation_back_reference_mismatch_count
      << ",\"mutating_selector_count\":" << summary.mutating_selector_count
      << ",\"non_mutating_selector_count\":"
      << summary.non_mutating_selector_count << ",\"first_class_event_stream\":"
      << bool_json(summary.first_class_event_stream)
      << ",\"read_only_lattice_fact\":"
      << bool_json(summary.read_only_lattice_fact)
      << ",\"runtime_executor\":" << bool_json(summary.runtime_executor)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"leakage_relevant_when_forbidden\":"
      << bool_json(summary.leakage_relevant_when_forbidden)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string representation_support_fact_json(
    const exposure::lattice_representation_support_fact_t &fact) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(fact.schema)
      << ",\"fact_type\":" << json_quote(fact.fact_type)
      << ",\"parent_exposure_fact_digest\":"
      << json_quote(fact.parent_exposure_fact_digest)
      << ",\"contract_fingerprint\":" << json_quote(fact.contract_fingerprint)
      << ",\"protocol_id\":" << json_quote(fact.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(fact.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(fact.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(fact.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(fact.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(fact.target_component_family_id)
      << ",\"job_id\":" << json_quote(fact.job_id)
      << ",\"wave_id\":" << json_quote(fact.wave_id)
      << ",\"job_status\":" << json_quote(fact.job_status)
      << ",\"wave_action\":" << json_quote(fact.wave_action)
      << ",\"support_scope\":" << json_quote(fact.support_scope)
      << ",\"node_indexed\":" << bool_json(fact.node_indexed)
      << ",\"visibility_only\":" << bool_json(fact.visibility_only)
      << ",\"readiness_authority\":" << bool_json(fact.readiness_authority)
      << ",\"hard_gate_authority\":" << bool_json(fact.hard_gate_authority)
      << ",\"mdn_node_support_reused\":"
      << bool_json(fact.mdn_node_support_reused)
      << ",\"node_id\":" << json_quote(fact.node_id)
      << ",\"node_index\":" << fact.node_index
      << ",\"split_name\":" << json_quote(fact.split_name) << ",\"split_role\":"
      << json_quote(exposure::exposure_split_role_name(fact.split_role))
      << ",\"anchor_range\":" << interval_json(fact.anchor_range)
      << ",\"completed_anchor_range\":"
      << interval_json(fact.completed_anchor_range)
      << ",\"use\":" << exposure_use_set_json(fact.use)
      << ",\"last_valid_projection_rows\":" << fact.last_valid_projection_rows
      << ",\"total_valid_projection_rows\":" << fact.total_valid_projection_rows
      << ",\"adapter_original_rows\":" << fact.adapter_original_rows
      << ",\"adapter_kept_rows\":" << fact.adapter_kept_rows
      << ",\"adapter_dropped_rows\":" << fact.adapter_dropped_rows
      << ",\"adapter_valid_channel_time_count\":"
      << fact.adapter_valid_channel_time_count
      << ",\"adapter_kept_valid_channel_time_count\":"
      << fact.adapter_kept_valid_channel_time_count
      << ",\"node_support_denominator\":" << fact.node_support_denominator
      << ",\"node_valid_lifted_rows\":" << fact.node_valid_lifted_rows
      << ",\"node_valid_lifted_feature_count\":"
      << fact.node_valid_lifted_feature_count
      << ",\"node_valid_lifted_cell_any_count\":"
      << fact.node_valid_lifted_cell_any_count
      << ",\"node_valid_lifted_cell_all_count\":"
      << fact.node_valid_lifted_cell_all_count
      << ",\"node_valid_projection_rows\":" << fact.node_valid_projection_rows
      << ",\"node_valid_lifted_feature_fraction\":"
      << double_json(fact.node_valid_lifted_feature_fraction)
      << ",\"mean_adapter_valid_channel_time_fraction\":"
      << double_json(fact.mean_adapter_valid_channel_time_fraction)
      << ",\"mean_adapter_kept_valid_channel_time_fraction\":"
      << double_json(fact.mean_adapter_kept_valid_channel_time_fraction)
      << ",\"nodelift_runtime_available\":"
      << bool_json(fact.nodelift_runtime_available)
      << ",\"nodelift_anchor_count\":" << fact.nodelift_anchor_count
      << ",\"nodelift_node_count\":" << fact.nodelift_node_count
      << ",\"nodelift_edge_count\":" << fact.nodelift_edge_count
      << ",\"nodelift_channel_count\":" << fact.nodelift_channel_count
      << ",\"nodelift_input_length\":" << fact.nodelift_input_length
      << ",\"nodelift_future_length\":" << fact.nodelift_future_length
      << ",\"nodelift_feature_width\":" << fact.nodelift_feature_width
      << ",\"nodelift_lift_future\":" << bool_json(fact.nodelift_lift_future)
      << ",\"nodelift_future_lift_emitted\":"
      << bool_json(fact.nodelift_future_lift_emitted)
      << ",\"nodelift_observed_node_mask_fraction\":"
      << double_json(fact.nodelift_observed_node_mask_fraction)
      << ",\"nodelift_observed_residual_energy_mean\":"
      << double_json(fact.nodelift_observed_residual_energy_mean)
      << ",\"nodelift_observed_valid_edge_count_mean\":"
      << double_json(fact.nodelift_observed_valid_edge_count_mean)
      << ",\"nodelift_future_node_mask_fraction\":"
      << double_json(fact.nodelift_future_node_mask_fraction)
      << ",\"nodelift_future_residual_energy_mean\":"
      << double_json(fact.nodelift_future_residual_energy_mean)
      << ",\"nodelift_future_valid_edge_count_mean\":"
      << double_json(fact.nodelift_future_valid_edge_count_mean)
      << ",\"output_checkpoint\":"
      << json_quote(fact.output_checkpoint.lexically_normal().string())
      << ",\"digest\":"
      << json_quote(exposure::representation_support_fact_digest(fact)) << "}";
  return out.str();
}

[[nodiscard]] std::string representation_support_facts_json(
    const std::vector<exposure::lattice_representation_support_fact_t> &facts,
    std::size_t limit) {
  const std::size_t n = std::min(facts.size(), limit);
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << representation_support_fact_json(facts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string representation_support_summary_json(
    const exposure::representation_support_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"exposure_fact_count\":" << summary.exposure_fact_count
      << ",\"representation_exposure_fact_count\":"
      << summary.representation_exposure_fact_count
      << ",\"representation_support_fact_count\":"
      << summary.representation_support_fact_count
      << ",\"aggregate_fact_count\":" << summary.aggregate_fact_count
      << ",\"node_indexed_fact_count\":" << summary.node_indexed_fact_count
      << ",\"mdn_backfill_fact_count\":" << summary.mdn_backfill_fact_count
      << ",\"nodelift_runtime_fact_count\":"
      << summary.nodelift_runtime_fact_count
      << ",\"unique_node_count\":" << summary.unique_node_count
      << ",\"visibility_only\":" << bool_json(summary.visibility_only)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"hard_gate_authority\":" << bool_json(summary.hard_gate_authority)
      << ",\"mdn_node_support_reused\":"
      << bool_json(summary.mdn_node_support_reused)
      << ",\"shared_representation_scope\":"
      << bool_json(summary.shared_representation_scope)
      << ",\"total_valid_projection_rows\":"
      << summary.total_valid_projection_rows
      << ",\"adapter_original_rows_total\":"
      << summary.adapter_original_rows_total
      << ",\"adapter_kept_rows_total\":" << summary.adapter_kept_rows_total
      << ",\"adapter_dropped_rows_total\":"
      << summary.adapter_dropped_rows_total
      << ",\"adapter_valid_channel_time_count_total\":"
      << summary.adapter_valid_channel_time_count_total
      << ",\"adapter_kept_valid_channel_time_count_total\":"
      << summary.adapter_kept_valid_channel_time_count_total
      << ",\"node_support_denominator_total\":"
      << summary.node_support_denominator_total
      << ",\"node_valid_lifted_rows_total\":"
      << summary.node_valid_lifted_rows_total
      << ",\"node_valid_lifted_feature_count_total\":"
      << summary.node_valid_lifted_feature_count_total
      << ",\"node_valid_lifted_cell_any_count_total\":"
      << summary.node_valid_lifted_cell_any_count_total
      << ",\"node_valid_lifted_cell_all_count_total\":"
      << summary.node_valid_lifted_cell_all_count_total
      << ",\"node_valid_projection_rows_total\":"
      << summary.node_valid_projection_rows_total
      << ",\"finite_adapter_valid_fraction_count\":"
      << summary.finite_adapter_valid_fraction_count
      << ",\"finite_node_valid_lifted_feature_fraction_count\":"
      << summary.finite_node_valid_lifted_feature_fraction_count
      << ",\"min_adapter_valid_channel_time_fraction\":"
      << double_json(summary.min_adapter_valid_channel_time_fraction)
      << ",\"max_adapter_valid_channel_time_fraction\":"
      << double_json(summary.max_adapter_valid_channel_time_fraction)
      << ",\"mean_adapter_valid_channel_time_fraction\":"
      << double_json(summary.mean_adapter_valid_channel_time_fraction)
      << ",\"min_node_valid_lifted_feature_fraction\":"
      << double_json(summary.min_node_valid_lifted_feature_fraction)
      << ",\"max_node_valid_lifted_feature_fraction\":"
      << double_json(summary.max_node_valid_lifted_feature_fraction)
      << ",\"mean_node_valid_lifted_feature_fraction\":"
      << double_json(summary.mean_node_valid_lifted_feature_fraction)
      << ",\"weakest_node_id\":" << json_quote(summary.weakest_node_id)
      << ",\"weakest_node_index\":" << summary.weakest_node_index
      << ",\"weakest_node_valid_projection_rows\":"
      << summary.weakest_node_valid_projection_rows
      << ",\"weakest_node_support_denominator\":"
      << summary.weakest_node_support_denominator
      << ",\"weakest_node_valid_lifted_feature_fraction\":"
      << double_json(summary.weakest_node_valid_lifted_feature_fraction)
      << ",\"node_valid_projection_rows_mean\":"
      << double_json(summary.node_valid_projection_rows_mean)
      << ",\"node_valid_projection_rows_coefficient_of_variation\":"
      << double_json(
             summary.node_valid_projection_rows_coefficient_of_variation)
      << ",\"node_valid_projection_rows_gini\":"
      << double_json(summary.node_valid_projection_rows_gini)
      << ",\"max_nodelift_node_count\":" << summary.max_nodelift_node_count
      << ",\"max_nodelift_edge_count\":" << summary.max_nodelift_edge_count
      << ",\"max_nodelift_channel_count\":"
      << summary.max_nodelift_channel_count
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string
runtime_index_row_json(const exposure::lattice_runtime_index_row_t &row) {
  std::ostringstream out;
  out << "{\"relation\":" << json_quote(row.relation)
      << ",\"key\":" << json_quote(row.key)
      << ",\"digest\":" << json_quote(row.digest) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_index_rows_json(
    const std::vector<exposure::lattice_runtime_index_row_t> &rows,
    std::size_t limit) {
  std::ostringstream out;
  out << "[";
  const std::size_t n = std::min(rows.size(), limit);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << runtime_index_row_json(rows[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string runtime_index_relation_counts_json(
    const std::vector<exposure::lattice_runtime_index_relation_count_t>
        &counts);

[[nodiscard]] std::string runtime_index_cache_validation_json(
    const exposure::lattice_runtime_index_cache_validation_t &validation) {
  std::ostringstream out;
  out << "{\"validation_strength\":"
      << json_quote(validation.validation_strength)
      << ",\"available\":" << bool_json(validation.available)
      << ",\"schema_matches\":" << bool_json(validation.schema_matches)
      << ",\"metadata_checked\":" << bool_json(validation.metadata_checked)
      << ",\"metadata_matches\":" << bool_json(validation.metadata_matches)
      << ",\"watched_metadata_checked\":"
      << bool_json(validation.watched_metadata_checked)
      << ",\"watched_metadata_matches\":"
      << bool_json(validation.watched_metadata_matches)
      << ",\"row_set_checked\":" << bool_json(validation.row_set_checked)
      << ",\"row_set_matches\":" << bool_json(validation.row_set_matches)
      << ",\"relation_counts_checked\":"
      << bool_json(validation.relation_counts_checked)
      << ",\"relation_counts_match\":"
      << bool_json(validation.relation_counts_match)
      << ",\"cache_valid\":" << bool_json(validation.cache_valid)
      << ",\"expected_runtime_metadata_digest\":"
      << json_quote(validation.expected_runtime_metadata_digest)
      << ",\"observed_runtime_metadata_digest\":"
      << json_quote(validation.observed_runtime_metadata_digest)
      << ",\"expected_watched_file_metadata_digest\":"
      << json_quote(validation.expected_watched_file_metadata_digest)
      << ",\"observed_watched_file_metadata_digest\":"
      << json_quote(validation.observed_watched_file_metadata_digest)
      << ",\"expected_row_set_digest\":"
      << json_quote(validation.expected_row_set_digest)
      << ",\"observed_row_set_digest\":"
      << json_quote(validation.observed_row_set_digest)
      << ",\"issues\":" << string_array_json(validation.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string
runtime_index_cache_json(const exposure::lattice_runtime_index_cache_t &cache,
                         std::size_t limit) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(cache.schema)
      << ",\"runtime_root\":" << json_quote(cache.runtime_root)
      << ",\"runtime_metadata_digest\":"
      << json_quote(cache.runtime_metadata_digest)
      << ",\"watched_file_metadata_digest\":"
      << json_quote(cache.watched_file_metadata_digest)
      << ",\"row_set_digest\":" << json_quote(cache.row_set_digest)
      << ",\"source_of_truth\":" << json_quote(cache.source_of_truth)
      << ",\"db_writes_evidence\":" << bool_json(cache.db_writes_evidence)
      << ",\"runtime_executor\":" << bool_json(cache.runtime_executor)
      << ",\"rebuildable_from_runtime_files\":"
      << bool_json(cache.rebuildable_from_runtime_files)
      << ",\"fact_count\":" << cache.fact_count
      << ",\"node_exposure_fact_count\":" << cache.node_exposure_fact_count
      << ",\"checkpoint_fact_count\":" << cache.checkpoint_fact_count
      << ",\"source_receipt_fact_count\":" << cache.source_receipt_fact_count
      << ",\"source_analytics_fact_count\":"
      << cache.source_analytics_fact_count
      << ",\"target_transform_fact_count\":"
      << cache.target_transform_fact_count
      << ",\"forecast_baseline_fact_count\":"
      << cache.forecast_baseline_fact_count
      << ",\"forecast_eval_fact_count\":" << cache.forecast_eval_fact_count
      << ",\"observer_belief_fact_count\":" << cache.observer_belief_fact_count
      << ",\"allocation_engine_fact_count\":"
      << cache.allocation_engine_fact_count
      << ",\"replay_environment_fact_count\":"
      << cache.replay_environment_fact_count
      << ",\"policy_training_fact_count\":" << cache.policy_training_fact_count
      << ",\"selection_signal_fact_count\":"
      << cache.selection_signal_fact_count
      << ",\"representation_support_fact_count\":"
      << cache.representation_support_fact_count << ",\"relation_counts\":"
      << runtime_index_relation_counts_json(cache.relation_counts)
      << ",\"row_count\":" << cache.rows.size() << ",\"returned_row_count\":"
      << std::min<std::size_t>(cache.rows.size(), limit)
      << ",\"rows\":" << runtime_index_rows_json(cache.rows, limit)
      << ",\"scan_warning_count\":" << cache.scan_warnings.size()
      << ",\"scan_warnings\":" << string_array_json(cache.scan_warnings) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_index_relation_count_json(
    const exposure::lattice_runtime_index_relation_count_t &count) {
  std::ostringstream out;
  out << "{\"relation\":" << json_quote(count.relation)
      << ",\"count\":" << count.count << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_index_relation_counts_json(
    const std::vector<exposure::lattice_runtime_index_relation_count_t>
        &counts) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < counts.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << runtime_index_relation_count_json(counts[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
runtime_index_query_json(const exposure::lattice_runtime_index_query_t &query) {
  std::ostringstream out;
  out << "{\"relation\":" << json_quote(query.relation)
      << ",\"key\":" << json_quote(query.key)
      << ",\"key_contains\":" << json_quote(query.key_contains)
      << ",\"digest\":" << json_quote(query.digest)
      << ",\"digest_prefix\":" << json_quote(query.digest_prefix) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_index_query_result_json(
    const exposure::lattice_runtime_index_query_result_t &result) {
  std::ostringstream out;
  out << "{\"scanned_row_count\":" << result.scanned_row_count
      << ",\"matching_row_count\":" << result.matching_row_count
      << ",\"returned_row_count\":" << result.rows.size() << ",\"rows\":"
      << runtime_index_rows_json(result.rows, result.rows.size()) << "}";
  return out.str();
}

[[nodiscard]] std::string
runtime_index_parity_json(const exposure::lattice_runtime_index_parity_t &p) {
  std::ostringstream out;
  out << "{\"checked\":" << bool_json(p.checked)
      << ",\"passed\":" << bool_json(p.passed)
      << ",\"row_count_matches\":" << bool_json(p.row_count_matches)
      << ",\"relation_counts_match\":" << bool_json(p.relation_counts_match)
      << ",\"rows_match\":" << bool_json(p.rows_match)
      << ",\"indexed_row_count\":" << p.indexed_row_count
      << ",\"live_row_count\":" << p.live_row_count
      << ",\"missing_from_index_count\":" << p.missing_from_index_count
      << ",\"extra_in_index_count\":" << p.extra_in_index_count
      << ",\"indexed_relation_counts\":"
      << runtime_index_relation_counts_json(p.indexed_relation_counts)
      << ",\"live_relation_counts\":"
      << runtime_index_relation_counts_json(p.live_relation_counts)
      << ",\"missing_from_index_preview\":"
      << runtime_index_rows_json(p.missing_from_index,
                                 p.missing_from_index.size())
      << ",\"extra_in_index_preview\":"
      << runtime_index_rows_json(p.extra_in_index, p.extra_in_index.size())
      << ",\"issues\":" << string_array_json(p.issues) << "}";
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
      << ",\"protocol_id\":" << json_quote(f.protocol_id)
      << ",\"graph_order_fingerprint\":"
      << json_quote(f.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(f.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(f.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(f.component_assembly_fingerprint)
      << ",\"target_component_family_id\":"
      << json_quote(f.target_component_family_id)
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
      << ",\"valid_target_opportunity_count\":"
      << f.valid_target_opportunity_count
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

[[nodiscard]] std::vector<exposure::node_support_summary_t>
node_support_summaries_from_facts(
    const std::vector<exposure::lattice_node_exposure_fact_t> &facts) {
  std::map<std::pair<std::string, std::string>,
           std::vector<exposure::lattice_node_exposure_fact_t>>
      grouped;
  for (const auto &fact : facts) {
    grouped[{fact.target_component_family_id, fact.split_name}].push_back(fact);
  }
  std::vector<exposure::node_support_summary_t> out;
  out.reserve(grouped.size());
  for (const auto &[key, group] : grouped) {
    auto summary =
        exposure::summarize_node_support(group, key.first, key.second);
    if (summary.node_count > 0) {
      out.push_back(std::move(summary));
    }
  }
  return out;
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
      << ",\"protocol_id\":" << json_quote(f.protocol_id)
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

[[nodiscard]] std::vector<std::string>
fact_identity_lineage_digest_fields(const std::string &family) {
  if (family == "checkpoint") {
    return {"direct_exposure_digest", "closure_digest"};
  }
  if (family == "source_analytics") {
    return {"parent_exposure_fact_digest", "source_receipt_digests"};
  }
  if (family == "target_transform") {
    return {"parent_exposure_fact_digest", "support_surface_digest"};
  }
  if (family == "forecast_baseline") {
    return {"parent_exposure_fact_digest", "target_transform_fact_digest"};
  }
  if (family == "forecast_eval") {
    return {"parent_exposure_fact_digest",
            "forecast_artifact_digest",
            "evaluated_representation_checkpoint_digest",
            "evaluated_mdn_checkpoint_digest",
            "target_transform_fact_digest",
            "baseline_fact_digests",
            "selection_signal_fact_digests"};
  }
  if (family == "observer_belief") {
    return {"parent_exposure_fact_digest", "forecast_artifact_digest",
            "forecast_artifact_lineage"};
  }
  if (family == "allocation_engine") {
    return {"parent_exposure_fact_digest", "observer_belief_fact_digest",
            "forecast_artifact_digest"};
  }
  if (family == "replay_environment") {
    return {"parent_exposure_fact_digest", "runtime_replay_batch_digest",
            "runtime_replay_experiment_digest", "runtime_replay_report_digest"};
  }
  if (family == "selection_signal") {
    return {"parent_exposure_fact_digest", "parent_evaluation_fact_digests",
            "candidate_checkpoint_digest", "selected_checkpoint"};
  }
  if (family == "representation_support") {
    return {"parent_exposure_fact_digest", "representation_checkpoint_digest"};
  }
  if (family == "node_exposure" || family == "source_receipt") {
    return {"parent_exposure_fact_digest"};
  }
  return {};
}

[[nodiscard]] std::vector<std::string>
fact_identity_support_fields(const std::string &family) {
  if (family == "source_analytics") {
    return {"sample_validity_fraction", "missingness_fraction",
            "duplicate_sample_count"};
  }
  if (family == "target_transform") {
    return {"target_feature_ids", "horizon", "target_mask_policy_digest",
            "support_surface_identity"};
  }
  if (family == "forecast_baseline") {
    return {"support_count", "valid_count", "missing_count",
            "baseline_error_metrics"};
  }
  if (family == "forecast_eval") {
    return {"support_count",      "valid_count",
            "missing_count",      "support_by_node",
            "support_by_channel", "support_by_target_feature",
            "support_by_horizon", "weakest_support_rows"};
  }
  if (family == "observer_belief") {
    return {"confidence", "data_quality", "liquidity"};
  }
  if (family == "allocation_engine") {
    return {"accounting_numeraire_node_id",
            "numeraire_weight",
            "target_node_weights",
            "turnover",
            "constraint_diagnostics",
            "fallback_reasons",
            "derisk_reasons"};
  }
  if (family == "replay_environment") {
    return {"episode_requested_range", "episode_anchor_interval",
            "attempted_count", "completed_count"};
  }
  if (family == "selection_signal") {
    return {"candidate_checkpoint_count", "selector_split", "selector_metric",
            "tie_policy"};
  }
  if (family == "representation_support") {
    return {"node_count", "channel_count", "support_count"};
  }
  return {};
}

[[nodiscard]] std::string fact_identity_contract_json(
    const exposure::lattice_fact_family_descriptor_t &descriptor) {
  const std::vector<std::string> identity_fields{
      "schema",
      "fact_type",
      "digest",
      "parent_exposure_fact_digest",
      "contract_fingerprint",
      "protocol_id",
      "graph_order_fingerprint",
      "source_cursor_token",
      "split_policy_fingerprint",
      "component_assembly_fingerprint",
      "target_component_family_id",
      "job_id",
      "wave_id",
      "split_name",
      "split_role",
      "anchor_range",
      "completed_anchor_range",
  };
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.lattice.fact_identity_contract.v1\""
      << ",\"contract_id\":\"runtime_evidence_identity_envelope\""
      << ",\"fact_family\":" << json_quote(descriptor.family_name)
      << ",\"fact_schema\":" << json_quote(descriptor.fact_schema)
      << ",\"parent_exposure_bound\":"
      << bool_json(descriptor.parent_exposure_bound)
      << ",\"identity_fields\":" << string_array_json(identity_fields)
      << ",\"lineage_digest_fields\":"
      << string_array_json(
             fact_identity_lineage_digest_fields(descriptor.family_name))
      << ",\"support_fields\":"
      << string_array_json(fact_identity_support_fields(descriptor.family_name))
      << ",\"fact_digest_required\":true"
      << ",\"row_index_interval_authority\":true"
      << ",\"source_key_window_audit_only\":true"
      << ",\"target_kind_authority\":false"
      << ",\"runtime_wave_authority\":false"
      << ",\"marshal_reachability\":false"
      << ",\"policy_gate_authority\":false}";
  return out.str();
}

[[nodiscard]] std::string fact_family_descriptor_json(
    const exposure::lattice_fact_family_descriptor_t &descriptor) {
  const auto *artifact_proof_template =
      target::artifact_readiness_proof_template_for_subject_fact_family(
          descriptor.family_name);
  const bool artifact_readiness_proofable = artifact_proof_template != nullptr;
  const auto target_promotion_blocked_reason =
      artifact_readiness_target_promotion_blocked_reason(
          descriptor.family_name);
  const bool artifact_readiness_warning_summary_only =
      !artifact_readiness_proofable &&
      descriptor.family == exposure::lattice_fact_family_t::source_analytics;
  std::ostringstream out;
  out << "{\"family\":" << json_quote(descriptor.family_name)
      << ",\"fact_schema\":" << json_quote(descriptor.fact_schema)
      << ",\"relation\":" << json_quote(descriptor.relation)
      << ",\"summary_schema\":" << json_quote(descriptor.summary_schema)
      << ",\"authority_model\":" << json_quote(descriptor.authority_model)
      << ",\"artifact_readiness_proofable\":"
      << bool_json(artifact_readiness_proofable)
      << ",\"artifact_readiness_proof_kind\":";
  if (artifact_proof_template != nullptr) {
    out << json_quote(artifact_proof_template->proof_kind);
  } else {
    out << "null";
  }
  out << ",\"artifact_readiness_proof_claim\":";
  if (artifact_proof_template != nullptr) {
    out << json_quote(artifact_proof_template->proof_claim);
  } else {
    out << "null";
  }
  out << ",\"artifact_readiness_requires_explicit_template\":true"
      << ",\"artifact_readiness_target_kind_required\":false"
      << ",\"artifact_readiness_target_promotion_allowed\":"
      << bool_json(artifact_readiness_proofable)
      << ",\"artifact_readiness_target_promotion_blocked\":"
      << bool_json(!target_promotion_blocked_reason.empty())
      << ",\"artifact_readiness_target_promotion_blocked_reason\":";
  if (target_promotion_blocked_reason.empty()) {
    out << "null";
  } else {
    out << json_quote(target_promotion_blocked_reason);
  }
  out << ",\"artifact_readiness_warning_summary_only\":"
      << bool_json(artifact_readiness_warning_summary_only)
      << ",\"parent_exposure_bound\":"
      << bool_json(descriptor.parent_exposure_bound)
      << ",\"target_kind\":" << bool_json(descriptor.target_kind)
      << ",\"dispatchable\":" << bool_json(descriptor.dispatchable)
      << ",\"runtime_executor\":" << bool_json(descriptor.runtime_executor)
      << ",\"readiness_authority\":"
      << bool_json(descriptor.readiness_authority)
      << ",\"coverage_authority\":" << bool_json(descriptor.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(descriptor.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(descriptor.contract_identity_authority)
      << ",\"quality_authority\":" << bool_json(descriptor.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(descriptor.performance_authority)
      << ",\"checkpoint_selector\":"
      << bool_json(descriptor.checkpoint_selector)
      << ",\"allocation_authority\":"
      << bool_json(descriptor.allocation_authority)
      << ",\"execution_authority\":"
      << bool_json(descriptor.execution_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(descriptor.market_readiness_authority)
      << ",\"deployment_authority\":"
      << bool_json(descriptor.deployment_authority)
      << ",\"policy_gate\":" << bool_json(descriptor.policy_gate)
      << ",\"target_dependency_authority\":"
      << bool_json(descriptor.target_dependency_authority)
      << ",\"runtime_wave_authority\":"
      << bool_json(descriptor.runtime_wave_authority)
      << ",\"marshal_reachability\":"
      << bool_json(descriptor.marshal_reachability)
      << ",\"checkpoint_source_authority\":"
      << bool_json(descriptor.checkpoint_source_authority)
      << ",\"plan_checkpoint_input_authority\":"
      << bool_json(descriptor.plan_checkpoint_input_authority)
      << ",\"fact_identity_contract\":"
      << fact_identity_contract_json(descriptor) << "}";
  return out.str();
}

[[nodiscard]] std::string fact_family_registry_json() {
  const auto registry = exposure::lattice_fact_family_registry();
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < registry.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << fact_family_descriptor_json(registry[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string fact_catalog_family_summary_json(
    const exposure::lattice_fact_catalog_family_summary_t &summary) {
  const auto *artifact_proof_template =
      target::artifact_readiness_proof_template_for_subject_fact_family(
          summary.family);
  const bool artifact_readiness_proofable = artifact_proof_template != nullptr;
  const auto target_promotion_blocked_reason =
      artifact_readiness_target_promotion_blocked_reason(summary.family);
  const bool artifact_readiness_warning_summary_only =
      !artifact_readiness_proofable && summary.family == "source_analytics";
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"family\":" << json_quote(summary.family)
      << ",\"fact_schema\":" << json_quote(summary.fact_schema)
      << ",\"relation\":" << json_quote(summary.relation)
      << ",\"summary_schema\":" << json_quote(summary.summary_schema)
      << ",\"authority_model\":" << json_quote(summary.authority_model)
      << ",\"artifact_readiness_proofable\":"
      << bool_json(artifact_readiness_proofable)
      << ",\"artifact_readiness_proof_kind\":";
  if (artifact_proof_template != nullptr) {
    out << json_quote(artifact_proof_template->proof_kind);
  } else {
    out << "null";
  }
  out << ",\"artifact_readiness_proof_claim\":";
  if (artifact_proof_template != nullptr) {
    out << json_quote(artifact_proof_template->proof_claim);
  } else {
    out << "null";
  }
  out << ",\"artifact_readiness_requires_explicit_template\":true"
      << ",\"artifact_readiness_target_kind_required\":false"
      << ",\"artifact_readiness_target_promotion_allowed\":"
      << bool_json(artifact_readiness_proofable)
      << ",\"artifact_readiness_target_promotion_blocked\":"
      << bool_json(!target_promotion_blocked_reason.empty())
      << ",\"artifact_readiness_target_promotion_blocked_reason\":";
  if (target_promotion_blocked_reason.empty()) {
    out << "null";
  } else {
    out << json_quote(target_promotion_blocked_reason);
  }
  out << ",\"artifact_readiness_warning_summary_only\":"
      << bool_json(artifact_readiness_warning_summary_only)
      << ",\"fact_count\":" << summary.fact_count
      << ",\"parent_exposure_bound_count\":"
      << summary.parent_exposure_bound_count
      << ",\"issue_count\":" << summary.issue_count
      << ",\"parent_exposure_bound\":"
      << bool_json(summary.parent_exposure_bound)
      << ",\"target_kind\":" << bool_json(summary.target_kind)
      << ",\"dispatchable\":" << bool_json(summary.dispatchable)
      << ",\"runtime_executor\":" << bool_json(summary.runtime_executor)
      << ",\"readiness_authority\":" << bool_json(summary.readiness_authority)
      << ",\"coverage_authority\":" << bool_json(summary.coverage_authority)
      << ",\"leakage_authority\":" << bool_json(summary.leakage_authority)
      << ",\"contract_identity_authority\":"
      << bool_json(summary.contract_identity_authority)
      << ",\"quality_authority\":" << bool_json(summary.quality_authority)
      << ",\"performance_authority\":"
      << bool_json(summary.performance_authority)
      << ",\"checkpoint_selector\":" << bool_json(summary.checkpoint_selector)
      << ",\"allocation_authority\":" << bool_json(summary.allocation_authority)
      << ",\"execution_authority\":" << bool_json(summary.execution_authority)
      << ",\"market_readiness_authority\":"
      << bool_json(summary.market_readiness_authority)
      << ",\"deployment_authority\":" << bool_json(summary.deployment_authority)
      << ",\"policy_gate\":" << bool_json(summary.policy_gate)
      << ",\"target_dependency_authority\":"
      << bool_json(summary.target_dependency_authority)
      << ",\"runtime_wave_authority\":"
      << bool_json(summary.runtime_wave_authority)
      << ",\"marshal_reachability\":" << bool_json(summary.marshal_reachability)
      << ",\"checkpoint_source_authority\":"
      << bool_json(summary.checkpoint_source_authority)
      << ",\"plan_checkpoint_input_authority\":"
      << bool_json(summary.plan_checkpoint_input_authority)
      << ",\"issues\":" << string_array_json(summary.issues)
      << ",\"fact_identity_contract\":";
  if (const auto descriptor = exposure::lattice_fact_family_descriptor(
          exposure::parse_lattice_fact_family(summary.family).value())) {
    out << fact_identity_contract_json(*descriptor);
  } else {
    out << "null";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string fact_catalog_family_summaries_json(
    const std::vector<exposure::lattice_fact_catalog_family_summary_t>
        &summaries) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < summaries.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << fact_catalog_family_summary_json(summaries[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string fact_catalog_summary_json(
    const exposure::lattice_fact_catalog_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"family_count\":" << summary.family_count
      << ",\"total_fact_count\":" << summary.total_fact_count
      << ",\"target_kind_family_count\":" << summary.target_kind_family_count
      << ",\"dispatchable_family_count\":" << summary.dispatchable_family_count
      << ",\"runtime_executor_family_count\":"
      << summary.runtime_executor_family_count
      << ",\"readiness_authority_family_count\":"
      << summary.readiness_authority_family_count
      << ",\"coverage_authority_family_count\":"
      << summary.coverage_authority_family_count
      << ",\"leakage_authority_family_count\":"
      << summary.leakage_authority_family_count
      << ",\"contract_identity_authority_family_count\":"
      << summary.contract_identity_authority_family_count
      << ",\"quality_authority_family_count\":"
      << summary.quality_authority_family_count
      << ",\"performance_authority_family_count\":"
      << summary.performance_authority_family_count
      << ",\"checkpoint_selector_family_count\":"
      << summary.checkpoint_selector_family_count
      << ",\"allocation_authority_family_count\":"
      << summary.allocation_authority_family_count
      << ",\"execution_authority_family_count\":"
      << summary.execution_authority_family_count
      << ",\"market_readiness_authority_family_count\":"
      << summary.market_readiness_authority_family_count
      << ",\"deployment_authority_family_count\":"
      << summary.deployment_authority_family_count
      << ",\"policy_gate_family_count\":" << summary.policy_gate_family_count
      << ",\"target_dependency_authority_family_count\":"
      << summary.target_dependency_authority_family_count
      << ",\"runtime_wave_authority_family_count\":"
      << summary.runtime_wave_authority_family_count
      << ",\"marshal_reachability_family_count\":"
      << summary.marshal_reachability_family_count
      << ",\"checkpoint_source_authority_family_count\":"
      << summary.checkpoint_source_authority_family_count
      << ",\"plan_checkpoint_input_authority_family_count\":"
      << summary.plan_checkpoint_input_authority_family_count
      << ",\"decision_authority_family_count\":"
      << summary.decision_authority_family_count
      << ",\"catalog_is_read_only\":" << bool_json(summary.catalog_is_read_only)
      << ",\"fact_families_are_not_target_kinds\":"
      << bool_json(summary.fact_families_are_not_target_kinds)
      << ",\"non_dispatchable_families_not_reachable\":"
      << bool_json(summary.non_dispatchable_families_not_reachable)
      << ",\"decision_authority_clean\":"
      << bool_json(summary.decision_authority_clean) << ",\"families\":"
      << fact_catalog_family_summaries_json(summary.families)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string fact_integrity_family_summary_json(
    const exposure::lattice_fact_integrity_family_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"family\":" << json_quote(summary.family)
      << ",\"summary_schema\":" << json_quote(summary.summary_schema)
      << ",\"relation_declared_count\":" << summary.relation_declared_count
      << ",\"relation_bound_count\":" << summary.relation_bound_count
      << ",\"unresolved_relation_count\":" << summary.unresolved_relation_count
      << ",\"identity_mismatch_count\":" << summary.identity_mismatch_count
      << ",\"digest_mismatch_count\":" << summary.digest_mismatch_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"relation_integrity_clean\":"
      << bool_json(summary.relation_integrity_clean)
      << ",\"issues\":" << string_array_json(summary.issues) << "}";
  return out.str();
}

[[nodiscard]] std::string fact_integrity_family_summaries_json(
    const std::vector<exposure::lattice_fact_integrity_family_summary_t>
        &summaries) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < summaries.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << fact_integrity_family_summary_json(summaries[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string fact_integrity_summary_json(
    const exposure::lattice_fact_integrity_summary_t &summary) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(summary.schema)
      << ",\"inspected_family_count\":" << summary.inspected_family_count
      << ",\"reported_family_count\":" << summary.reported_family_count
      << ",\"relation_declared_count\":" << summary.relation_declared_count
      << ",\"relation_bound_count\":" << summary.relation_bound_count
      << ",\"unresolved_relation_count\":" << summary.unresolved_relation_count
      << ",\"identity_mismatch_count\":" << summary.identity_mismatch_count
      << ",\"digest_mismatch_count\":" << summary.digest_mismatch_count
      << ",\"warning_count\":" << summary.warning_count
      << ",\"relation_integrity_clean\":"
      << bool_json(summary.relation_integrity_clean)
      << ",\"read_only\":" << bool_json(summary.read_only)
      << ",\"target_proof\":" << bool_json(summary.target_proof)
      << ",\"dispatchable\":" << bool_json(summary.dispatchable)
      << ",\"runtime_executor\":" << bool_json(summary.runtime_executor)
      << ",\"families_with_unresolved_relation\":"
      << string_array_json(summary.families_with_unresolved_relation)
      << ",\"families_with_identity_mismatch\":"
      << string_array_json(summary.families_with_identity_mismatch)
      << ",\"families_with_digest_mismatch\":"
      << string_array_json(summary.families_with_digest_mismatch)
      << ",\"integrity_flags\":" << string_array_json(summary.integrity_flags)
      << ",\"issue_codes\":" << string_array_json(summary.issue_codes)
      << ",\"families\":"
      << fact_integrity_family_summaries_json(summary.families) << "}";
  return out.str();
}

[[nodiscard]] std::string
simple_fact_family_summary_json(const char *schema, std::int64_t fact_count) {
  std::ostringstream out;
  out << "{\"schema\":" << json_quote(schema)
      << ",\"fact_count\":" << fact_count << "}";
  return out.str();
}

[[nodiscard]] std::string fact_family_payload_summary_json(
    const exposure::lattice_exposure_ledger_t &ledger,
    exposure::lattice_fact_family_t family) {
  switch (family) {
  case exposure::lattice_fact_family_t::exposure:
    return simple_fact_family_summary_json(
        "kikijyeba.lattice.exposure_summary.v1",
        static_cast<std::int64_t>(ledger.facts().size()));
  case exposure::lattice_fact_family_t::node_exposure: {
    const auto summaries =
        node_support_summaries_from_facts(ledger.node_facts());
    std::ostringstream out;
    out << "{\"schema\":\"kikijyeba.lattice.node_support_summary.v1\""
        << ",\"fact_count\":" << ledger.node_facts().size()
        << ",\"summary_count\":" << summaries.size()
        << ",\"summaries\":" << node_support_summaries_json(summaries) << "}";
    return out.str();
  }
  case exposure::lattice_fact_family_t::checkpoint:
    return simple_fact_family_summary_json(
        "kikijyeba.lattice.checkpoint_summary.v1",
        static_cast<std::int64_t>(ledger.checkpoint_facts().size()));
  case exposure::lattice_fact_family_t::source_receipt:
    return source_receipt_summary_json(exposure::summarize_source_receipts(
        ledger.facts(), ledger.source_receipt_facts()));
  case exposure::lattice_fact_family_t::source_analytics:
    return source_analytics_summary_json(exposure::summarize_source_analytics(
        ledger.facts(), ledger.source_analytics_facts()));
  case exposure::lattice_fact_family_t::target_transform:
    return target_transform_summary_json(exposure::summarize_target_transforms(
        ledger.facts(), ledger.target_transform_facts()));
  case exposure::lattice_fact_family_t::forecast_baseline:
    return forecast_baseline_summary_json(
        exposure::summarize_forecast_baselines(
            ledger.facts(), ledger.forecast_baseline_facts(),
            ledger.target_transform_facts()));
  case exposure::lattice_fact_family_t::forecast_eval:
    return forecast_eval_summary_json(exposure::summarize_forecast_evals(
        ledger.facts(), ledger.forecast_eval_facts(),
        ledger.target_transform_facts(), ledger.forecast_baseline_facts(),
        ledger.selection_signal_facts()));
  case exposure::lattice_fact_family_t::observer_belief:
    return observer_belief_summary_json(exposure::summarize_observer_beliefs(
        ledger.facts(), ledger.observer_belief_facts(),
        ledger.forecast_eval_facts()));
  case exposure::lattice_fact_family_t::allocation_engine:
    return allocation_engine_summary_json(
        exposure::summarize_allocation_engines(
            ledger.facts(), ledger.allocation_engine_facts(),
            ledger.observer_belief_facts(), ledger.forecast_eval_facts()));
  case exposure::lattice_fact_family_t::replay_environment:
    return replay_environment_summary_json(
        exposure::summarize_replay_environments(
            ledger.facts(), ledger.replay_environment_facts()));
  case exposure::lattice_fact_family_t::policy_training:
    return policy_training_summary_json(exposure::summarize_policy_trainings(
        ledger.facts(), ledger.policy_training_facts(),
        ledger.forecast_eval_facts(), ledger.observer_belief_facts(),
        ledger.allocation_engine_facts(), ledger.replay_environment_facts()));
  case exposure::lattice_fact_family_t::selection_signal:
    return selection_signal_summary_json(exposure::summarize_selection_signals(
        ledger.facts(), ledger.selection_signal_facts(),
        ledger.forecast_eval_facts()));
  case exposure::lattice_fact_family_t::representation_support:
    return representation_support_summary_json(
        exposure::summarize_representation_support(
            ledger.facts(), ledger.representation_support_facts()));
  }
  throw std::runtime_error("[lattice_hero] unknown fact family");
}

[[nodiscard]] std::string
fact_family_facts_json(const exposure::lattice_exposure_ledger_t &ledger,
                       exposure::lattice_fact_family_t family,
                       std::size_t limit) {
  switch (family) {
  case exposure::lattice_fact_family_t::exposure:
    return facts_json(ledger.facts(), limit);
  case exposure::lattice_fact_family_t::node_exposure:
    return node_facts_json(ledger.node_facts(), limit);
  case exposure::lattice_fact_family_t::checkpoint:
    return checkpoint_facts_json(ledger.checkpoint_facts(), limit);
  case exposure::lattice_fact_family_t::source_receipt:
    return source_receipt_facts_json(ledger.source_receipt_facts(), limit);
  case exposure::lattice_fact_family_t::source_analytics:
    return source_analytics_facts_json(ledger.source_analytics_facts(), limit);
  case exposure::lattice_fact_family_t::target_transform:
    return target_transform_facts_json(ledger.target_transform_facts(), limit);
  case exposure::lattice_fact_family_t::forecast_baseline:
    return forecast_baseline_facts_json(ledger.forecast_baseline_facts(),
                                        limit);
  case exposure::lattice_fact_family_t::forecast_eval:
    return forecast_eval_facts_json(ledger.forecast_eval_facts(), limit);
  case exposure::lattice_fact_family_t::observer_belief:
    return observer_belief_facts_json(ledger.observer_belief_facts(), limit);
  case exposure::lattice_fact_family_t::allocation_engine:
    return allocation_engine_facts_json(ledger.allocation_engine_facts(),
                                        limit);
  case exposure::lattice_fact_family_t::replay_environment:
    return replay_environment_facts_json(ledger.replay_environment_facts(),
                                         limit);
  case exposure::lattice_fact_family_t::policy_training:
    return policy_training_facts_json(ledger.policy_training_facts(), limit);
  case exposure::lattice_fact_family_t::selection_signal:
    return selection_signal_facts_json(ledger.selection_signal_facts(), limit);
  case exposure::lattice_fact_family_t::representation_support:
    return representation_support_facts_json(
        ledger.representation_support_facts(), limit);
  }
  throw std::runtime_error("[lattice_hero] unknown fact family");
}

struct fact_preview_filter_t {
  std::string digest{};
  std::string fact_digest_prefix{};
  std::string resolved_prefix_digest{};
  std::optional<std::size_t> fact_index{};
  std::size_t limit{64};
};

[[nodiscard]] const std::string &
fact_preview_effective_digest_filter(const fact_preview_filter_t &filter) {
  return !filter.resolved_prefix_digest.empty() ? filter.resolved_prefix_digest
                                                : filter.digest;
}

[[nodiscard]] bool
fact_preview_digest_matches(const std::string &digest,
                            const fact_preview_filter_t &filter) {
  const auto &effective_digest = fact_preview_effective_digest_filter(filter);
  if (!effective_digest.empty() && digest != effective_digest) {
    return false;
  }
  if (filter.resolved_prefix_digest.empty() &&
      !filter.fact_digest_prefix.empty() &&
      !digest.starts_with(filter.fact_digest_prefix)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool
fact_preview_row_matches(std::size_t index, const std::string &digest,
                         const fact_preview_filter_t &filter) {
  if (filter.fact_index.has_value() && index != *filter.fact_index) {
    return false;
  }
  return fact_preview_digest_matches(digest, filter);
}

template <typename Fact>
[[nodiscard]] std::string
fact_preview_identity_envelope_json(const Fact &fact, std::string_view family,
                                    const std::string &digest) {
  const auto envelope = exposure::make_lattice_fact_identity_envelope(
      fact, std::string(family), digest);
  return fact_identity_envelope_json(envelope);
}

template <typename Fact, typename DigestFn, typename JsonFn>
[[nodiscard]] std::string fact_preview_rows_json(
    const std::vector<Fact> &facts, const fact_preview_filter_t &filter,
    std::string_view family, DigestFn digest_fn, JsonFn json_fn,
    std::size_t *matching_count, bool *truncated) {
  std::ostringstream out;
  out << "[";
  std::size_t matched = 0;
  std::size_t returned = 0;
  bool first = true;
  for (std::size_t i = 0; i < facts.size(); ++i) {
    const std::string digest = digest_fn(facts[i]);
    if (!fact_preview_row_matches(i, digest, filter)) {
      continue;
    }
    ++matched;
    if (returned >= filter.limit) {
      continue;
    }
    if (!first) {
      out << ",";
    }
    first = false;
    ++returned;
    out << "{\"fact_index\":" << i << ",\"fact_digest\":" << json_quote(digest)
        << ",\"fact_digest_prefix\":"
        << json_quote(display::digest_prefix(digest))
        << ",\"fact_ref\":" << json_quote(display::fact_ref(family, digest))
        << ",\"identity_envelope\":"
        << fact_preview_identity_envelope_json(facts[i], family, digest)
        << ",\"fact\":" << json_fn(facts[i]) << "}";
  }
  out << "]";
  if (matching_count != nullptr) {
    *matching_count = matched;
  }
  if (truncated != nullptr) {
    *truncated = returned < matched;
  }
  return out.str();
}

struct fact_digest_prefix_resolution_t {
  std::size_t match_count{0};
  std::string digest{};
};

template <typename Fact, typename DigestFn>
[[nodiscard]] fact_digest_prefix_resolution_t
resolve_fact_digest_prefix(const std::vector<Fact> &facts,
                           const std::string &prefix, DigestFn digest_fn) {
  fact_digest_prefix_resolution_t out{};
  if (prefix.empty()) {
    return out;
  }
  for (const auto &fact : facts) {
    const std::string digest = digest_fn(fact);
    if (digest.starts_with(prefix)) {
      ++out.match_count;
      if (out.match_count == 1) {
        out.digest = digest;
      } else {
        out.digest.clear();
      }
    }
  }
  return out;
}

[[nodiscard]] std::string checkpoint_fact_digest_for_preview(
    const exposure::lattice_checkpoint_fact_t &fact) {
  return exposure::exposure_digest_for_text(
      exposure::canonical_checkpoint_fact_text(fact));
}

[[nodiscard]] fact_digest_prefix_resolution_t fact_family_digest_prefix_resolve(
    const exposure::lattice_exposure_ledger_t &ledger,
    exposure::lattice_fact_family_t family, const std::string &prefix) {
  if (prefix.empty()) {
    return {};
  }
  switch (family) {
  case exposure::lattice_fact_family_t::exposure:
    return resolve_fact_digest_prefix(ledger.facts(), prefix,
                                      exposure::exposure_fact_digest);
  case exposure::lattice_fact_family_t::node_exposure:
    return resolve_fact_digest_prefix(ledger.node_facts(), prefix,
                                      exposure::node_exposure_index_row_digest);
  case exposure::lattice_fact_family_t::checkpoint:
    return resolve_fact_digest_prefix(ledger.checkpoint_facts(), prefix,
                                      checkpoint_fact_digest_for_preview);
  case exposure::lattice_fact_family_t::source_receipt:
    return resolve_fact_digest_prefix(ledger.source_receipt_facts(), prefix,
                                      exposure::source_receipt_fact_digest);
  case exposure::lattice_fact_family_t::source_analytics:
    return resolve_fact_digest_prefix(ledger.source_analytics_facts(), prefix,
                                      exposure::source_analytics_fact_digest);
  case exposure::lattice_fact_family_t::target_transform:
    return resolve_fact_digest_prefix(ledger.target_transform_facts(), prefix,
                                      exposure::target_transform_fact_digest);
  case exposure::lattice_fact_family_t::forecast_baseline:
    return resolve_fact_digest_prefix(ledger.forecast_baseline_facts(), prefix,
                                      exposure::forecast_baseline_fact_digest);
  case exposure::lattice_fact_family_t::forecast_eval:
    return resolve_fact_digest_prefix(ledger.forecast_eval_facts(), prefix,
                                      exposure::forecast_eval_fact_digest);
  case exposure::lattice_fact_family_t::observer_belief:
    return resolve_fact_digest_prefix(ledger.observer_belief_facts(), prefix,
                                      exposure::observer_belief_fact_digest);
  case exposure::lattice_fact_family_t::allocation_engine:
    return resolve_fact_digest_prefix(ledger.allocation_engine_facts(), prefix,
                                      exposure::allocation_engine_fact_digest);
  case exposure::lattice_fact_family_t::replay_environment:
    return resolve_fact_digest_prefix(ledger.replay_environment_facts(), prefix,
                                      exposure::replay_environment_fact_digest);
  case exposure::lattice_fact_family_t::policy_training:
    return resolve_fact_digest_prefix(ledger.policy_training_facts(), prefix,
                                      exposure::policy_training_fact_digest);
  case exposure::lattice_fact_family_t::selection_signal:
    return resolve_fact_digest_prefix(ledger.selection_signal_facts(), prefix,
                                      exposure::selection_signal_fact_digest);
  case exposure::lattice_fact_family_t::representation_support:
    return resolve_fact_digest_prefix(
        ledger.representation_support_facts(), prefix,
        exposure::representation_support_fact_digest);
  }
  throw std::runtime_error("[lattice_hero] unknown fact family");
}

[[nodiscard]] std::string
fact_family_preview_rows_json(const exposure::lattice_exposure_ledger_t &ledger,
                              exposure::lattice_fact_family_t family,
                              const fact_preview_filter_t &filter,
                              std::size_t *matching_count, bool *truncated) {
  switch (family) {
  case exposure::lattice_fact_family_t::exposure:
    return fact_preview_rows_json(
        ledger.facts(), filter, exposure::lattice_fact_family_name(family),
        exposure::exposure_fact_digest, fact_json, matching_count, truncated);
  case exposure::lattice_fact_family_t::node_exposure:
    return fact_preview_rows_json(ledger.node_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::node_exposure_index_row_digest,
                                  node_fact_json, matching_count, truncated);
  case exposure::lattice_fact_family_t::checkpoint:
    return fact_preview_rows_json(ledger.checkpoint_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  checkpoint_fact_digest_for_preview,
                                  checkpoint_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::source_receipt:
    return fact_preview_rows_json(ledger.source_receipt_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::source_receipt_fact_digest,
                                  source_receipt_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::source_analytics:
    return fact_preview_rows_json(ledger.source_analytics_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::source_analytics_fact_digest,
                                  source_analytics_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::target_transform:
    return fact_preview_rows_json(ledger.target_transform_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::target_transform_fact_digest,
                                  target_transform_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::forecast_baseline:
    return fact_preview_rows_json(ledger.forecast_baseline_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::forecast_baseline_fact_digest,
                                  forecast_baseline_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::forecast_eval:
    return fact_preview_rows_json(ledger.forecast_eval_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::forecast_eval_fact_digest,
                                  forecast_eval_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::observer_belief:
    return fact_preview_rows_json(ledger.observer_belief_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::observer_belief_fact_digest,
                                  observer_belief_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::allocation_engine:
    return fact_preview_rows_json(ledger.allocation_engine_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::allocation_engine_fact_digest,
                                  allocation_engine_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::replay_environment:
    return fact_preview_rows_json(ledger.replay_environment_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::replay_environment_fact_digest,
                                  replay_environment_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::policy_training:
    return fact_preview_rows_json(ledger.policy_training_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::policy_training_fact_digest,
                                  policy_training_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::selection_signal:
    return fact_preview_rows_json(ledger.selection_signal_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::selection_signal_fact_digest,
                                  selection_signal_fact_json, matching_count,
                                  truncated);
  case exposure::lattice_fact_family_t::representation_support:
    return fact_preview_rows_json(ledger.representation_support_facts(), filter,
                                  exposure::lattice_fact_family_name(family),
                                  exposure::representation_support_fact_digest,
                                  representation_support_fact_json,
                                  matching_count, truncated);
  }
  throw std::runtime_error("[lattice_hero] unknown fact family");
}

[[nodiscard]] std::string
scan_fact_family_json(const exposure::lattice_exposure_ledger_t &ledger,
                      exposure::lattice_fact_family_t family, std::size_t limit,
                      bool include_facts) {
  const auto summary = exposure::summarize_lattice_fact_family(ledger, family);
  std::ostringstream out;
  out << "{\"catalog_summary\":" << fact_catalog_family_summary_json(summary)
      << ",\"payload_summary\":"
      << fact_family_payload_summary_json(ledger, family)
      << ",\"returned_fact_count\":"
      << std::min<std::size_t>(static_cast<std::size_t>(summary.fact_count),
                               limit);
  if (include_facts) {
    out << ",\"facts\":" << fact_family_facts_json(ledger, family, limit);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::vector<exposure::lattice_fact_family_t>
selected_fact_families_from_arg(const std::string &args) {
  std::string family_arg;
  (void)extract_json_string_field(args, "family", &family_arg);
  auto parsed = exposure::parse_lattice_fact_family(family_arg);
  if (parsed.has_value()) {
    return {*parsed};
  }
  std::vector<exposure::lattice_fact_family_t> families;
  for (const auto &descriptor : exposure::lattice_fact_family_registry()) {
    families.push_back(descriptor.family);
  }
  return families;
}

[[nodiscard]] std::string
relation_for_fact_family(exposure::lattice_fact_family_t family) {
  for (const auto &descriptor : exposure::lattice_fact_family_registry()) {
    if (descriptor.family == family) {
      return descriptor.relation;
    }
  }
  return exposure::lattice_fact_family_name(family);
}

[[nodiscard]] std::vector<std::string> relation_names_for_fact_families(
    const std::vector<exposure::lattice_fact_family_t> &families) {
  std::vector<std::string> relations;
  relations.reserve(families.size());
  for (const auto family : families) {
    relations.push_back(relation_for_fact_family(family));
  }
  return relations;
}

[[nodiscard]] bool
relation_is_selected(const std::vector<std::string> &selected_relations,
                     const std::string &relation) {
  return std::find(selected_relations.begin(), selected_relations.end(),
                   relation) != selected_relations.end();
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
  for (const auto &job_dir :
       cuwacunu::hero::runtime::job_layout::discover_runtime_job_dirs(
           runtime_root)) {
    push_job_dir(job_dir.dir);
  }
  std::sort(rows.begin(), rows.end(),
            [](const row_t &a, const row_t &b) { return a.time > b.time; });

  target::lattice_target_active_identity_t id{};
  for (const auto &row : rows) {
    const auto manifest = parse_kv_file(row.dir / "job.manifest");
    if (id.protocol_id.empty()) {
      id.protocol_id = map_get(manifest, "protocol_id");
    }
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
    if (id.mtf_jepa_mae_vicreg_assembly_fingerprint.empty()) {
      id.mtf_jepa_mae_vicreg_assembly_fingerprint =
          map_get(manifest, "mtf_jepa_mae_vicreg_assembly_fingerprint");
    }
    if (id.mdn_assembly_fingerprint.empty()) {
      id.mdn_assembly_fingerprint =
          map_get(manifest, "mdn_assembly_fingerprint");
    }
  }
  return id;
}

void overlay_active_identity_from_config(
    const fs::path &config_path, target::lattice_target_active_identity_t *id) {
  if (!id) {
    return;
  }
  const auto config = parse_kv_file(config_path);
  if (id->protocol_id.empty()) {
    const fs::path protocol_dsl_path =
        map_get_or(config, "kikijyeba_protocol_dsl_path",
                   "/cuwacunu/src/config/kikijyeba.protocol.cwu_01v.dsl");
    const auto protocol_dsl = parse_kv_file(protocol_dsl_path);
    id->protocol_id = map_get_or(protocol_dsl, "PROTOCOL_ID", "cwu_01v");
  }
  if (!id->mtf_jepa_mae_vicreg_assembly_fingerprint.empty()) {
    return;
  }
  const fs::path mtf_dsl_path = map_get_or(
      config, "wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path",
      "/cuwacunu/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl");
  const auto mtf_dsl = parse_kv_file(mtf_dsl_path);
  const auto component_assembly_id =
      map_get_or(mtf_dsl, "COMPONENT_ASSEMBLY_ID", "mtf_jepa_mae_vicreg_v1");
  const auto version_token = map_get_or(
      mtf_dsl, "VERSION", "wikimyei.representation.mtf_jepa_mae_vicreg.v1");
  const auto assembly = cuwacunu::wikimyei::representation::encoding::
      mtf_jepa_mae_vicreg::make_mtf_jepa_mae_vicreg_assembly(
          component_assembly_id, version_token);
  id->mtf_jepa_mae_vicreg_assembly_fingerprint =
      cuwacunu::wikimyei::assembly::assembly_fingerprint(assembly);
}

void overlay_active_identity_from_args(
    const std::string &args, target::lattice_target_active_identity_t *id) {
  if (!id) {
    return;
  }
  std::string value;
  if (extract_json_string_field(args, "protocol_id", &value)) {
    id->protocol_id = value;
  }
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
  if (extract_json_string_field(
          args, "mtf_jepa_mae_vicreg_assembly_fingerprint", &value)) {
    id->mtf_jepa_mae_vicreg_assembly_fingerprint = value;
  }
  if (extract_json_string_field(args, "mdn_assembly_fingerprint", &value)) {
    id->mdn_assembly_fingerprint = value;
  }
}

[[nodiscard]] std::string
active_identity_json(const target::lattice_target_active_identity_t &id) {
  std::ostringstream out;
  out << "{\"protocol_id\":" << json_quote(id.protocol_id)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(id.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(id.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(id.source_cursor_token)
      << ",\"vicreg_assembly_fingerprint\":"
      << json_quote(id.vicreg_assembly_fingerprint)
      << ",\"mtf_jepa_mae_vicreg_assembly_fingerprint\":"
      << json_quote(id.mtf_jepa_mae_vicreg_assembly_fingerprint)
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

[[nodiscard]] const exposure::exposure_ledger_scan_result_t &
session_scan_for_runtime_root(const fs::path &runtime_root) {
  const auto watched_digest =
      exposure::watched_runtime_metadata_digest(runtime_root);
  if (!g_runtime_scan_session_cache.available ||
      g_runtime_scan_session_cache.runtime_root != runtime_root ||
      g_runtime_scan_session_cache.watched_file_metadata_digest !=
          watched_digest) {
    g_runtime_scan_session_cache.available = true;
    g_runtime_scan_session_cache.runtime_root = runtime_root;
    g_runtime_scan_session_cache.watched_file_metadata_digest = watched_digest;
    exposure::exposure_scan_options_t options{};
    // Interactive Hero queries read sidecars as the runtime evidence surface.
    // Sidecar-vs-derived parity belongs to audit mode, not default status.
    options.compare_sidecar_to_derived_fact = false;
    // replay_environment_artifact_ready is an active Lattice target; its facts
    // are derived from Runtime replay indexes and reports.
    options.derive_replay_environment_facts = true;
    g_runtime_scan_session_cache.scan =
        exposure::scan_exposure_ledger_from_runtime_root(runtime_root, {},
                                                         options);
  }
  return g_runtime_scan_session_cache.scan;
}

[[nodiscard]] exposure::lattice_runtime_index_cache_t
session_runtime_index_cache_for_runtime_root(const fs::path &runtime_root) {
  return exposure::make_runtime_index_cache_from_scan(
      runtime_root, session_scan_for_runtime_root(runtime_root));
}

[[nodiscard]] fs::path
default_runtime_index_path(const fs::path &runtime_root) {
  return runtime_root / "indexes" / "lattice_runtime_index.v1.lls";
}

[[nodiscard]] bool resolve_runtime_index_path_arg(const std::string &args,
                                                  lattice_context_t *ctx,
                                                  const fs::path &runtime_root,
                                                  fs::path *out,
                                                  std::string *err) {
  std::string index_arg;
  (void)extract_json_string_field(args, "index_path", &index_arg);
  fs::path index_path = trim_ascii(index_arg).empty()
                            ? default_runtime_index_path(runtime_root)
                            : fs::path(index_arg);
  if (index_path.is_relative()) {
    index_path = runtime_root / index_path;
  }
  index_path = normalize_path(index_path);
  if (!runtime_root_allowed(ctx->policy, index_path)) {
    if (err) {
      *err = "E_LATTICE_INDEX_DENIED: index_path is outside allowed runtime "
             "roots: " +
             index_path.string();
    }
    return false;
  }
  if (out) {
    *out = std::move(index_path);
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
  overlay_active_identity_from_config(config_path, &identity);
  overlay_active_identity_from_args(args, &identity);
  exposure::exposure_ledger_scan_result_t scan{};
  if (policy_bool_or(ctx->policy, "auto_build_exposure_ledger", true)) {
    scan = session_scan_for_runtime_root(runtime_root);
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
  std::vector<target::lattice_policy_gate_spec_t> policy_gates;
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
    policy_gates = target::load_lattice_policy_gates_from_config(config_path);
  } catch (...) {
  }
  std::optional<cuwacunu::hero::lattice::split::lattice_split_policy_t>
      split_policy{};
  try {
    split_policy =
        target::load_lattice_split_policy_from_config_if_available(config_path);
  } catch (...) {
  }
  const auto validation = target::validate_lattice_target_specs(targets);
  const auto geometry_gate_review_summary =
      target::summarize_representation_geometry_gate_review(
          scan.ledger.facts());
  std::ostringstream json;
  json << "{\"ok\":true"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
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
                         ? cuwacunu::hero::lattice::split::
                               lattice_split_policy_fingerprint(*split_policy)
                         : std::string{})
       << ",\"target_count\":" << targets.size()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"policy_gate_reservation_summary\":"
       << policy_gate_reservation_summary_json(policy_gates)
       << ",\"policy_gate_reservations\":"
       << policy_gate_reservations_json(policy_gates)
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"source_receipt_fact_count\":"
       << scan.ledger.source_receipt_facts().size()
       << ",\"source_analytics_fact_count\":"
       << scan.ledger.source_analytics_facts().size()
       << ",\"target_transform_fact_count\":"
       << scan.ledger.target_transform_facts().size()
       << ",\"forecast_baseline_fact_count\":"
       << scan.ledger.forecast_baseline_facts().size()
       << ",\"forecast_eval_fact_count\":"
       << scan.ledger.forecast_eval_facts().size()
       << ",\"observer_belief_fact_count\":"
       << scan.ledger.observer_belief_facts().size()
       << ",\"allocation_engine_fact_count\":"
       << scan.ledger.allocation_engine_facts().size()
       << ",\"replay_environment_fact_count\":"
       << scan.ledger.replay_environment_facts().size()
       << ",\"policy_training_fact_count\":"
       << scan.ledger.policy_training_facts().size()
       << ",\"selection_signal_fact_count\":"
       << scan.ledger.selection_signal_facts().size()
       << ",\"representation_support_fact_count\":"
       << scan.ledger.representation_support_facts().size()
       << ",\"representation_geometry_gate_review_summary\":"
       << representation_geometry_gate_review_summary_json(
              geometry_gate_review_summary)
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"target_warnings\":" << string_array_json(validation.warnings)
       << ",\"active_identity\":" << active_identity_json(identity) << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_schema(const std::string &, lattice_context_t *,
                                 std::string *out, std::string *) {
  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.hero_policy_schema.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"writes_evidence\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << "," << lattice_no_decision_authority_json() << ",\"policy_keys\":[";
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
  std::vector<target::lattice_policy_gate_spec_t> policy_gates;
  target::lattice_target_config_paths_t paths{};
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
    targets = target::load_lattice_targets_from_config(config_path);
    policy_gates = target::load_lattice_policy_gates_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }
  const auto validation = target::validate_lattice_target_specs(targets);
  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.target_catalog.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"writes_evidence\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << "," << lattice_no_decision_authority_json()
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"targets_path\":" << json_quote(paths.targets_dsl_path.string())
       << ",\"splits_path\":" << json_quote(paths.splits_dsl_path.string())
       << ",\"target_count\":" << targets.size()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"policy_gate_reservation_summary\":"
       << policy_gate_reservation_summary_json(policy_gates)
       << ",\"policy_gate_reservations\":"
       << policy_gate_reservations_json(policy_gates)
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
  std::vector<target::lattice_policy_gate_spec_t> policy_gates;
  target::lattice_target_config_paths_t paths{};
  try {
    paths = target::resolve_lattice_target_config_paths(config_path);
    compiled_targets =
        target::load_lattice_compiled_targets_from_config(config_path);
    policy_gates = target::load_lattice_policy_gates_from_config(config_path);
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

  std::optional<cuwacunu::hero::lattice::split::lattice_split_policy_t>
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
  if (target::is_artifact_readiness_target_class(spec.target_class)) {
    text << " This is a non-dispatchable evidence-catalog proof; inspect "
            "evidence panels rather than reaching a Runtime wave.";
  }
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
    if (clause.kind == "representation_geometry") {
      const auto metric =
          target::lattice_clause_field_value(clause.fields, "METRIC", "");
      const auto op =
          target::lattice_clause_field_value(clause.fields, "OP", "");
      auto value =
          target::lattice_clause_field_value(clause.fields, "VALUE", "");
      if (value.empty()) {
        value =
            target::lattice_clause_field_value(clause.fields, "MIN_VALUE", "");
      }
      if (!metric.empty() && !value.empty()) {
        requirements.push_back("representation_geometry " + metric + " " +
                               (op.empty() ? "passes" : op) + " " + value);
      }
      continue;
    }
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
    text << " The suggested wave carries symbolic model-state input hints";
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
  json << "{\"schema\":\"kikijyeba.lattice.target_explanation.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"writes_evidence\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << "," << lattice_no_decision_authority_json()
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"targets_path\":" << json_quote(paths.targets_dsl_path.string())
       << ",\"splits_path\":" << json_quote(paths.splits_dsl_path.string())
       << ",\"target\":" << compiled_target_json(*found)
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"policy_gate_reservation_summary\":"
       << policy_gate_reservation_summary_json(policy_gates, spec.target_id)
       << ",\"policy_gate_reservations\":"
       << policy_gate_reservations_json(policy_gates, spec.target_id)
       << ",\"policy_gate_catalog_summary\":"
       << policy_gate_reservation_summary_json(policy_gates)
       << ",\"exposure_measure_algebra_vocabulary\":"
       << exposure_measure_algebra_vocabulary_json()
       << ",\"exposure_measure_algebra_summary\":"
       << exposure_measure_algebra_summary_json()
       << ",\"source_key_coordinate_policy_vocabulary\":"
       << source_key_coordinate_policy_vocabulary_json()
       << ",\"source_key_coordinate_policy_summary\":"
       << source_key_coordinate_policy_summary_json()
       << ",\"source_receipt_policy_vocabulary\":"
       << source_receipt_policy_vocabulary_json()
       << ",\"source_receipt_policy_summary\":"
       << source_receipt_policy_summary_json()
       << ",\"valid_target_uncertainty_vocabulary\":"
       << valid_target_uncertainty_vocabulary_json()
       << ",\"valid_target_uncertainty_summary\":"
       << valid_target_uncertainty_summary_json()
       << ",\"leakage_rule_vocabulary\":" << leakage_rule_vocabulary_json()
       << ",\"leakage_rule_summary\":" << leakage_rule_summary_json()
       << ",\"causal_edge_vocabulary\":" << causal_edge_vocabulary_json()
       << ",\"causal_edge_summary\":" << causal_edge_summary_json()
       << ",\"selection_signal_policy_vocabulary\":"
       << selection_signal_policy_vocabulary_json()
       << ",\"selection_signal_policy_summary\":"
       << selection_signal_policy_summary_json()
       << ",\"derived_query_rule_vocabulary\":"
       << derived_query_rule_vocabulary_json()
       << ",\"derived_query_rule_vocabulary_digest_schema\":"
       << json_quote(kDerivedQueryRuleVocabularyDigestSchema)
       << ",\"derived_query_rule_vocabulary_digest\":"
       << json_quote(derived_query_rule_vocabulary_digest())
       << ",\"derived_query_projection_semantics_vocabulary\":"
       << derived_query_projection_semantics_vocabulary_json()
       << ",\"db_cache_policy_vocabulary\":"
       << db_cache_policy_vocabulary_json()
       << ",\"evidence_retention_policy_vocabulary\":"
       << evidence_retention_policy_vocabulary_json()
       << ",\"evidence_retention_audit_scenario_vocabulary\":"
       << evidence_retention_audit_scenario_vocabulary_json()
       << ",\"evidence_retention_policy_summary\":"
       << evidence_retention_policy_summary_json()
       << ",\"benchmark_regression_budget_vocabulary\":"
       << benchmark_regression_budget_vocabulary_json()
       << ",\"benchmark_regression_budget_summary\":"
       << benchmark_regression_budget_summary_json()
       << ",\"evidence_abstraction_vocabulary\":"
       << evidence_abstraction_vocabulary_json()
       << ",\"product_evidence_state_vocabulary\":"
       << product_evidence_state_vocabulary_json()
       << ",\"join_law_vocabulary\":" << join_law_vocabulary_json()
       << ",\"deficit_priority_vocabulary\":"
       << deficit_priority_vocabulary_json()
       << ",\"deficit_vector_planning_summary\":"
       << deficit_vector_planning_summary_json()
       << ",\"evidence_order_dimension_vocabulary\":"
       << evidence_order_dimension_vocabulary_json()
       << ",\"evidence_order_relation_vocabulary\":"
       << evidence_order_relation_vocabulary_json()
       << ",\"node_support_scope_policy_vocabulary\":"
       << node_support_scope_policy_vocabulary_json()
       << ",\"node_support_scope_policy_summary\":"
       << node_support_scope_policy_summary_json()
       << ",\"validation_performance_evidence_policy_vocabulary\":"
       << validation_performance_evidence_policy_vocabulary_json()
       << ",\"validation_performance_evidence_policy_summary\":"
       << validation_performance_evidence_policy_summary_json()
       << ",\"validation_performance_scope_policy_vocabulary\":"
       << validation_performance_scope_policy_vocabulary_json()
       << ",\"validation_performance_scope_policy_summary\":"
       << validation_performance_scope_policy_summary_json()
       << ",\"representation_geometry_vocabulary\":"
       << representation_geometry_vocabulary_json()
       << ",\"representation_geometry_summary\":"
       << representation_geometry_summary_json()
       << ",\"representation_geometry_gate_review_summary\":"
       << representation_geometry_gate_review_summary_json()
       << ",\"performance_uncertainty_policy_vocabulary\":"
       << performance_uncertainty_policy_vocabulary_json()
       << ",\"performance_uncertainty_policy_summary\":"
       << performance_uncertainty_policy_summary_json()
       << ",\"mdn_distribution_calibration_vocabulary\":"
       << mdn_distribution_calibration_vocabulary_json()
       << ",\"mdn_distribution_calibration_summary\":"
       << mdn_distribution_calibration_summary_json()
       << ",\"mdn_distribution_calibration_diagnostic_vocabulary\":"
       << mdn_distribution_calibration_diagnostic_vocabulary_json()
       << ",\"mdn_distribution_calibration_diagnostic_summary\":"
       << mdn_distribution_calibration_diagnostic_summary_json()
       << ",\"proof_obligation_vocabulary\":"
       << proof_obligation_vocabulary_json()
       << ",\"proof_certificate_digest_policy_vocabulary\":"
       << proof_certificate_digest_policy_vocabulary_json()
       << ",\"proof_certificate_digest_policy_summary\":"
       << proof_certificate_digest_policy_summary_json()
       << ",\"checkpoint_selection_policy_vocabulary\":"
       << checkpoint_selection_policy_vocabulary_json()
       << ",\"checkpoint_selection_policy_summary\":"
       << checkpoint_selection_policy_summary_json()
       << ",\"plan_advice_scope_policy_vocabulary\":"
       << plan_advice_scope_policy_vocabulary_json()
       << ",\"contract_identity_boundary_vocabulary\":"
       << contract_identity_boundary_vocabulary_json()
       << ",\"contract_identity_boundary_summary\":"
       << contract_identity_boundary_summary_json()
       << ",\"operational_readiness_v1_scope_vocabulary\":"
       << operational_readiness_v1_scope_vocabulary_json()
       << ",\"operational_readiness_v1_scope_summary\":"
       << operational_readiness_v1_scope_summary_json()
       << ",\"operational_readiness_v1_gate_vocabulary\":"
       << operational_readiness_v1_gate_vocabulary_json()
       << ",\"operational_readiness_v1_gate_summary\":"
       << operational_readiness_v1_gate_summary_json()
       << ",\"mathematical_readiness_v1_vocabulary\":"
       << mathematical_readiness_v1_vocabulary_json()
       << ",\"mathematical_readiness_v1_summary\":"
       << mathematical_readiness_v1_summary_json()
       << ",\"warning_threshold_relation_vocabulary\":"
       << warning_threshold_relation_vocabulary_json()
       << ",\"target_numeric_dimension_vocabulary\":"
       << target_numeric_dimension_vocabulary_json()
       << ",\"target_numeric_dimension_summary\":"
       << target_numeric_dimension_summary_json()
       << ",\"monotonicity_invariant_vocabulary\":"
       << monotonicity_invariant_vocabulary_json()
       << ",\"checkpoint_identity_policy_vocabulary\":"
       << checkpoint_identity_policy_vocabulary_json()
       << ",\"warning_anchor_scope_policy_vocabulary\":"
       << warning_anchor_scope_policy_vocabulary_json()
       << ",\"warning_scope_previews\":"
       << warning_scope_previews_json(spec, split_policy)
       << ",\"component_spawn_policy\":{"
       << "\"schema\":" << json_quote("kikijyeba.component_spawn.v2")
       << ",\"required_tuple\":[" << json_quote("component_family_id") << ","
       << json_quote("protocol_contract_fingerprint") << ","
       << json_quote("graph_order_fingerprint") << ","
       << json_quote("component_assembly_fingerprint") << "]"
       << ",\"config_bridge\":"
       << json_quote("config_bundle_id/config_receipt_id")
       << ",\"source_cursor_scope\":"
       << json_quote("job_evidence_checkpoint_lineage_only")
       << ",\"spawn_id_readability_hint_only\":true"
       << ",\"full_fingerprint_required_for_proof\":true}"
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
  std::vector<target::lattice_policy_gate_spec_t> policy_gates;
  try {
    policy_gates = target::load_lattice_policy_gates_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
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
  json << "{\"schema\":"
       << json_quote(plan_only
                         ? "kikijyeba.lattice.target_deficit_response.v1"
                         : "kikijyeba.lattice.target_evaluation_response.v1")
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":true"
       << ",\"target_proof_engine\":\"lattice_target_evaluator\""
       << ",\"plan_advice_only\":" << bool_json(plan_only)
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"writes_evidence\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << "," << lattice_no_decision_authority_json()
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"active_identity\":" << active_identity_json(identity)
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"source_receipt_fact_count\":"
       << scan.ledger.source_receipt_facts().size()
       << ",\"source_analytics_fact_count\":"
       << scan.ledger.source_analytics_facts().size()
       << ",\"target_transform_fact_count\":"
       << scan.ledger.target_transform_facts().size()
       << ",\"forecast_baseline_fact_count\":"
       << scan.ledger.forecast_baseline_facts().size()
       << ",\"forecast_eval_fact_count\":"
       << scan.ledger.forecast_eval_facts().size()
       << ",\"observer_belief_fact_count\":"
       << scan.ledger.observer_belief_facts().size()
       << ",\"allocation_engine_fact_count\":"
       << scan.ledger.allocation_engine_facts().size()
       << ",\"replay_environment_fact_count\":"
       << scan.ledger.replay_environment_facts().size()
       << ",\"policy_training_fact_count\":"
       << scan.ledger.policy_training_facts().size()
       << ",\"selection_signal_fact_count\":"
       << scan.ledger.selection_signal_facts().size()
       << ",\"representation_support_fact_count\":"
       << scan.ledger.representation_support_facts().size()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"policy_gate_reservation_summary\":"
       << policy_gate_reservation_summary_json(policy_gates, target_id)
       << ",\"policy_gate_reservations\":"
       << policy_gate_reservations_json(policy_gates, target_id)
       << ",\"policy_gate_catalog_summary\":"
       << policy_gate_reservation_summary_json(policy_gates)
       << ",\"scan_warnings\":" << string_array_json(scan.warnings)
       << ",\"warnings\":" << warning_results_json(eval.warning_results)
       << ",\"warning_messages\":"
       << warning_messages_json(eval.warning_results, eval.warnings);
  if (plan_only) {
    const bool artifact_readiness_target =
        target::is_artifact_readiness_target_class(eval.target_class);
    json << ",\"target_id\":" << json_quote(eval.target_id) << ",\"status\":"
         << json_quote(target::lattice_target_status_name(eval.status))
         << ",\"target_class\":" << json_quote(eval.target_class)
         << ",\"proof_kind\":" << json_quote(eval.proof_kind)
         << ",\"subject_fact_family\":" << json_quote(eval.subject_fact_family)
         << ",\"target_surface_kind\":"
         << json_quote(artifact_readiness_target ? "evidence_catalog_artifact"
                                                 : "runtime_readiness")
         << ",\"evidence_catalog_target\":"
         << bool_json(artifact_readiness_target)
         << ",\"dispatchable_target\":" << bool_json(!artifact_readiness_target)
         << ",\"runtime_wave_dispatchable\":"
         << bool_json(!artifact_readiness_target && eval.plan_ready &&
                      !eval.suggested_wave.empty())
         << ",\"recommended_operator_action\":"
         << json_quote(artifact_readiness_target
                           ? "inspect"
                           : (eval.plan_ready ? "review_suggested_wave"
                                              : "inspect_lattice_status"))
         << ",\"kind\":"
         << json_quote(target::lattice_target_effective_kind_name(eval))
         << ",\"target_kind_applicable\":"
         << bool_json(target::lattice_target_kind_applicable(eval))
         << ",\"target_kind_effective\":"
         << json_quote(target::lattice_target_effective_kind_selector(eval))
         << ",\"component\":" << json_quote(eval.component)
         << ",\"split_policy_fingerprint\":"
         << json_quote(eval.split_policy_fingerprint)
         << ",\"plan_ready\":" << bool_json(eval.plan_ready)
         << ",\"reasons\":" << string_array_json(eval.reasons)
         << ",\"exposure_measure_algebra_vocabulary\":"
         << exposure_measure_algebra_vocabulary_json()
         << ",\"exposure_measure_algebra_summary\":"
         << exposure_measure_algebra_summary_json()
         << ",\"source_key_coordinate_policy_vocabulary\":"
         << source_key_coordinate_policy_vocabulary_json()
         << ",\"source_key_coordinate_policy_summary\":"
         << source_key_coordinate_policy_summary_json()
         << ",\"source_receipt_policy_vocabulary\":"
         << source_receipt_policy_vocabulary_json()
         << ",\"source_receipt_policy_summary\":"
         << source_receipt_policy_summary_json()
         << ",\"valid_target_uncertainty_vocabulary\":"
         << valid_target_uncertainty_vocabulary_json()
         << ",\"valid_target_uncertainty_summary\":"
         << valid_target_uncertainty_summary_json()
         << ",\"leakage_rule_vocabulary\":" << leakage_rule_vocabulary_json()
         << ",\"leakage_rule_summary\":" << leakage_rule_summary_json()
         << ",\"causal_edge_vocabulary\":" << causal_edge_vocabulary_json()
         << ",\"causal_edge_summary\":" << causal_edge_summary_json()
         << ",\"selection_signal_policy_vocabulary\":"
         << selection_signal_policy_vocabulary_json()
         << ",\"selection_signal_policy_summary\":"
         << selection_signal_policy_summary_json()
         << ",\"derived_query_rule_vocabulary\":"
         << derived_query_rule_vocabulary_json()
         << ",\"derived_query_rule_vocabulary_digest_schema\":"
         << json_quote(kDerivedQueryRuleVocabularyDigestSchema)
         << ",\"derived_query_rule_vocabulary_digest\":"
         << json_quote(derived_query_rule_vocabulary_digest())
         << ",\"derived_query_projection_semantics_vocabulary\":"
         << derived_query_projection_semantics_vocabulary_json()
         << ",\"derived_query_results\":" << derived_query_results_json(eval)
         << ",\"db_cache_policy_vocabulary\":"
         << db_cache_policy_vocabulary_json()
         << ",\"evidence_retention_policy_vocabulary\":"
         << evidence_retention_policy_vocabulary_json()
         << ",\"evidence_retention_audit_scenario_vocabulary\":"
         << evidence_retention_audit_scenario_vocabulary_json()
         << ",\"evidence_retention_policy_summary\":"
         << evidence_retention_policy_summary_json()
         << ",\"benchmark_regression_budget_vocabulary\":"
         << benchmark_regression_budget_vocabulary_json()
         << ",\"benchmark_regression_budget_summary\":"
         << benchmark_regression_budget_summary_json()
         << ",\"evidence_abstraction_vocabulary\":"
         << evidence_abstraction_vocabulary_json()
         << ",\"product_evidence_state_vocabulary\":"
         << product_evidence_state_vocabulary_json()
         << ",\"join_law_vocabulary\":" << join_law_vocabulary_json()
         << ",\"deficit_priority_vocabulary\":"
         << deficit_priority_vocabulary_json()
         << ",\"deficit_vector_planning_summary\":"
         << deficit_vector_planning_summary_json()
         << ",\"evidence_order_dimension_vocabulary\":"
         << evidence_order_dimension_vocabulary_json()
         << ",\"evidence_order_relation_vocabulary\":"
         << evidence_order_relation_vocabulary_json()
         << ",\"node_support_scope_policy_vocabulary\":"
         << node_support_scope_policy_vocabulary_json()
         << ",\"node_support_scope_policy_summary\":"
         << node_support_scope_policy_summary_json()
         << ",\"validation_performance_evidence_policy_vocabulary\":"
         << validation_performance_evidence_policy_vocabulary_json()
         << ",\"validation_performance_evidence_policy_summary\":"
         << validation_performance_evidence_policy_summary_json()
         << ",\"validation_performance_scope_policy_vocabulary\":"
         << validation_performance_scope_policy_vocabulary_json()
         << ",\"validation_performance_scope_policy_summary\":"
         << validation_performance_scope_policy_summary_json()
         << ",\"representation_geometry_vocabulary\":"
         << representation_geometry_vocabulary_json()
         << ",\"representation_geometry_summary\":"
         << representation_geometry_summary_json()
         << ",\"representation_geometry_gate_review_summary\":"
         << representation_geometry_gate_review_summary_json()
         << ",\"performance_uncertainty_policy_vocabulary\":"
         << performance_uncertainty_policy_vocabulary_json()
         << ",\"performance_uncertainty_policy_summary\":"
         << performance_uncertainty_policy_summary_json()
         << ",\"mdn_distribution_calibration_vocabulary\":"
         << mdn_distribution_calibration_vocabulary_json()
         << ",\"mdn_distribution_calibration_summary\":"
         << mdn_distribution_calibration_summary_json()
         << ",\"mdn_distribution_calibration_diagnostic_vocabulary\":"
         << mdn_distribution_calibration_diagnostic_vocabulary_json()
         << ",\"mdn_distribution_calibration_diagnostic_summary\":"
         << mdn_distribution_calibration_diagnostic_summary_json()
         << ",\"target_numeric_dimension_vocabulary\":"
         << target_numeric_dimension_vocabulary_json()
         << ",\"target_numeric_dimension_summary\":"
         << target_numeric_dimension_summary_json()
         << ",\"monotonicity_invariant_vocabulary\":"
         << monotonicity_invariant_vocabulary_json()
         << ",\"proof_obligation_vocabulary\":"
         << proof_obligation_vocabulary_json()
         << ",\"proof_certificate_digest_policy_vocabulary\":"
         << proof_certificate_digest_policy_vocabulary_json()
         << ",\"proof_certificate_digest_policy_summary\":"
         << proof_certificate_digest_policy_summary_json()
         << ",\"checkpoint_identity_policy_vocabulary\":"
         << checkpoint_identity_policy_vocabulary_json()
         << ",\"checkpoint_selection_policy_vocabulary\":"
         << checkpoint_selection_policy_vocabulary_json()
         << ",\"checkpoint_selection_policy_summary\":"
         << checkpoint_selection_policy_summary_json()
         << ",\"plan_advice_scope_policy_vocabulary\":"
         << plan_advice_scope_policy_vocabulary_json()
         << ",\"operational_readiness_v1_scope_vocabulary\":"
         << operational_readiness_v1_scope_vocabulary_json()
         << ",\"operational_readiness_v1_scope_summary\":"
         << operational_readiness_v1_scope_summary_json()
         << ",\"operational_readiness_v1_gate_vocabulary\":"
         << operational_readiness_v1_gate_vocabulary_json()
         << ",\"operational_readiness_v1_gate_summary\":"
         << operational_readiness_v1_gate_summary_json()
         << ",\"contract_identity_boundary_vocabulary\":"
         << contract_identity_boundary_vocabulary_json()
         << ",\"contract_identity_boundary_summary\":"
         << contract_identity_boundary_summary_json()
         << ",\"mathematical_readiness_v1_vocabulary\":"
         << mathematical_readiness_v1_vocabulary_json()
         << ",\"mathematical_readiness_v1_summary\":"
         << mathematical_readiness_v1_summary_json()
         << ",\"exposure_summaries\":"
         << exposure_load_summaries_json(eval.exposure_summaries)
         << ",\"node_support_summaries\":"
         << node_support_summaries_json(eval.node_support_summaries)
         << ",\"target_warnings\":"
         << warning_results_json(eval.warning_results)
         << ",\"target_warning_messages\":"
         << warning_messages_json(eval.warning_results, eval.warnings)
         << ",\"warning_results\":"
         << warning_results_json(eval.warning_results)
         << ",\"warning_summary\":"
         << warning_summary_json(eval.warning_summary)
         << ",\"fact_integrity_summary_available\":"
         << bool_json(eval.fact_integrity_summary_available)
         << ",\"fact_integrity_summary\":"
         << evaluation_fact_integrity_summary_json(eval)
         << ",\"representation_geometry_gate_results\":"
         << representation_geometry_gate_results_json(
                eval.representation_geometry_gate_results)
         << ",\"warning_threshold_relation_vocabulary\":"
         << warning_threshold_relation_vocabulary_json()
         << ",\"warning_anchor_scope_policy_vocabulary\":"
         << warning_anchor_scope_policy_vocabulary_json()
         << ",\"deficits\":" << proof_deficits_json(eval.deficits)
         << ",\"plan_basis\":" << plan_basis_json(eval.plan_basis)
         << ",\"evidence_order_vector\":"
         << evidence_order_vector_json(eval.evidence_order_vector)
         << ",\"suggested_wave\":" << suggested_wave_json(eval.suggested_wave)
         << ",\"proof_certificate\":"
         << proof_certificate_json(eval.proof_certificate)
         << ",\"proof_certificate_check\":"
         << proof_certificate_check_json(eval.proof_certificate_check);
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

[[nodiscard]] bool handle_evaluate_targets(const std::string &args,
                                           lattice_context_t *ctx,
                                           std::string *out, std::string *err) {
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
  std::vector<std::string> requested_target_ids;
  if (!parse_optional_string_array_arg(args, "target_ids", {},
                                       &requested_target_ids, err)) {
    return false;
  }
  for (auto &target_id : requested_target_ids) {
    target_id = trim_ascii(target_id);
  }
  requested_target_ids.erase(std::remove_if(requested_target_ids.begin(),
                                            requested_target_ids.end(),
                                            [](const std::string &target_id) {
                                              return target_id.empty();
                                            }),
                             requested_target_ids.end());

  std::vector<target::lattice_target_spec_t> targets;
  exposure::exposure_ledger_scan_result_t scan;
  target::lattice_target_active_identity_t identity;
  fs::path config_path;
  fs::path runtime_root;
  if (!build_target_evaluator(args, ctx, &targets, &scan, &identity,
                              &config_path, &runtime_root, err)) {
    return false;
  }
  std::vector<target::lattice_policy_gate_spec_t> policy_gates;
  try {
    policy_gates = target::load_lattice_policy_gates_from_config(config_path);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  std::vector<std::string> target_ids = requested_target_ids;
  if (target_ids.empty()) {
    target_ids.reserve(targets.size());
    for (const auto &spec : targets) {
      target_ids.push_back(spec.target_id);
    }
  }
  if (static_cast<int>(target_ids.size()) > limit) {
    target_ids.resize(static_cast<std::size_t>(limit));
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

  std::vector<target::lattice_target_evaluation_t> evaluations;
  evaluations.reserve(target_ids.size());
  try {
    for (const auto &target_id : target_ids) {
      evaluations.push_back(evaluator.evaluate(target_id));
    }
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.target_evaluation_batch.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":true"
       << ",\"target_proof_engine\":\"lattice_target_evaluator\""
       << ",\"plan_advice_only\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"writes_evidence\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << "," << lattice_no_decision_authority_json()
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"active_identity\":" << active_identity_json(identity)
       << ",\"requested_target_count\":" << requested_target_ids.size()
       << ",\"configured_target_count\":" << targets.size()
       << ",\"evaluated_target_count\":" << evaluations.size()
       << ",\"single_runtime_scan\":true"
       << ",\"read_only\":true"
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"source_receipt_fact_count\":"
       << scan.ledger.source_receipt_facts().size()
       << ",\"source_analytics_fact_count\":"
       << scan.ledger.source_analytics_facts().size()
       << ",\"target_transform_fact_count\":"
       << scan.ledger.target_transform_facts().size()
       << ",\"forecast_baseline_fact_count\":"
       << scan.ledger.forecast_baseline_facts().size()
       << ",\"forecast_eval_fact_count\":"
       << scan.ledger.forecast_eval_facts().size()
       << ",\"observer_belief_fact_count\":"
       << scan.ledger.observer_belief_facts().size()
       << ",\"allocation_engine_fact_count\":"
       << scan.ledger.allocation_engine_facts().size()
       << ",\"replay_environment_fact_count\":"
       << scan.ledger.replay_environment_facts().size()
       << ",\"policy_training_fact_count\":"
       << scan.ledger.policy_training_facts().size()
       << ",\"selection_signal_fact_count\":"
       << scan.ledger.selection_signal_facts().size()
       << ",\"representation_support_fact_count\":"
       << scan.ledger.representation_support_facts().size()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"policy_gate_reservation_summary\":"
       << policy_gate_reservation_summary_json(policy_gates)
       << ",\"policy_gate_reservations\":"
       << policy_gate_reservations_json(policy_gates)
       << ",\"scan_warnings\":" << string_array_json(scan.warnings)
       << ",\"warnings\":" << warning_results_json(evaluations)
       << ",\"warning_messages\":" << warning_messages_json(evaluations)
       << ",\"evaluations\":[";
  for (std::size_t i = 0; i < evaluations.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << evaluation_json_with_policy_gate_reservations(evaluations[i],
                                                          policy_gates);
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_target_deficit(const std::string &args,
                                         lattice_context_t *ctx,
                                         std::string *out, std::string *err) {
  return evaluate_target_common(args, ctx, true, out, err);
}

[[nodiscard]] bool evaluate_target_for_derived_query(
    const std::string &args, lattice_context_t *ctx,
    const std::string &target_id, target::lattice_target_evaluation_t *eval_out,
    fs::path *config_path_out, fs::path *runtime_root_out,
    target::lattice_target_active_identity_t *identity_out,
    exposure::exposure_ledger_scan_result_t *scan_out, std::string *err);

[[nodiscard]] bool target_satisfied_relation_value(
    const target::lattice_target_evaluation_t &eval);

[[nodiscard]] bool handle_latest_satisfying_checkpoint(const std::string &args,
                                                       lattice_context_t *ctx,
                                                       std::string *out,
                                                       std::string *err) {
  std::string symbolic_hint;
  if (!parse_optional_string_arg(args, "symbolic_hint", {}, &symbolic_hint,
                                 err)) {
    return false;
  }
  std::string source_target_id;
  if (!parse_optional_string_arg(args, "target_id", {}, &source_target_id,
                                 err)) {
    return false;
  }
  symbolic_hint = trim_ascii(symbolic_hint);
  source_target_id = trim_ascii(source_target_id);
  if (!symbolic_hint.empty()) {
    const auto from_hint = target::lattice_target_eval_detail::
        latest_satisfying_target_id_from_reference(symbolic_hint);
    if (from_hint.empty()) {
      if (err) {
        *err = "symbolic_hint must be latest_satisfying:<target_id>";
      }
      return false;
    }
    if (!source_target_id.empty() && source_target_id != from_hint) {
      if (err) {
        *err = "symbolic_hint target_id differs from target_id";
      }
      return false;
    }
    source_target_id = from_hint;
  }
  if (source_target_id.empty()) {
    if (err) {
      *err = "symbolic_hint or target_id is required";
    }
    return false;
  }
  if (symbolic_hint.empty()) {
    symbolic_hint = "latest_satisfying:" + source_target_id;
  }

  target::lattice_target_evaluation_t eval;
  fs::path config_path;
  fs::path runtime_root;
  target::lattice_target_active_identity_t identity;
  exposure::exposure_ledger_scan_result_t scan;
  if (!evaluate_target_for_derived_query(args, ctx, source_target_id, &eval,
                                         &config_path, &runtime_root, &identity,
                                         &scan, err)) {
    return false;
  }

  const auto &closure = eval.proof_certificate.closure;
  const bool checkpoint_selectable_source_target =
      target::is_latest_satisfying_checkpoint_selectable_target_class(
          eval.target_class);
  const bool clean_satisfied = target_satisfied_relation_value(eval);
  const bool closure_checked = closure.checked;
  const bool closure_complete = closure.checked && closure.complete;
  const bool checkpoint_path_present = !closure.checkpoint_path.empty();
  const bool resolved = checkpoint_selectable_source_target &&
                        clean_satisfied && closure_checked &&
                        closure_complete && checkpoint_path_present &&
                        eval.proof_certificate_check.passed;
  const bool target_status_satisfied =
      eval.status == target::lattice_target_status_t::satisfied;
  std::string resolution_status = "resolved";
  if (!checkpoint_selectable_source_target) {
    resolution_status = "non_checkpoint_target_class";
  } else if (!eval.proof_certificate_check.passed) {
    resolution_status = "proof_certificate_check_failed";
  } else if (!target_status_satisfied) {
    resolution_status = "target_not_satisfied";
  } else if (!closure_checked) {
    resolution_status = "checkpoint_closure_not_checked";
  } else if (!closure_complete) {
    resolution_status = "checkpoint_closure_incomplete";
  } else if (!checkpoint_path_present) {
    resolution_status = "checkpoint_path_missing";
  } else if (!clean_satisfied) {
    resolution_status = "target_not_satisfied";
  }

  const std::string concrete_path =
      resolved ? closure.checkpoint_path.lexically_normal().string()
               : std::string{};
  std::ostringstream receipt_text;
  receipt_text
      << "symbolic_hint=" << symbolic_hint << "\n"
      << "source_target_id=" << source_target_id << "\n"
      << "source_target_class=" << eval.target_class << "\n"
      << "source_proof_kind=" << eval.proof_kind << "\n"
      << "source_subject_fact_family=" << eval.subject_fact_family << "\n"
      << "checkpoint_selectable_source_target="
      << bool_json(checkpoint_selectable_source_target) << "\n"
      << "config_path=" << config_path.lexically_normal().string() << "\n"
      << "runtime_root=" << runtime_root.lexically_normal().string() << "\n"
      << "protocol_contract_fingerprint="
      << identity.protocol_contract_fingerprint << "\n"
      << "graph_order_fingerprint=" << identity.graph_order_fingerprint << "\n"
      << "source_cursor_token=" << identity.source_cursor_token << "\n"
      << "vicreg_assembly_fingerprint=" << identity.vicreg_assembly_fingerprint
      << "\n"
      << "mtf_jepa_mae_vicreg_assembly_fingerprint="
      << identity.mtf_jepa_mae_vicreg_assembly_fingerprint << "\n"
      << "mdn_assembly_fingerprint=" << identity.mdn_assembly_fingerprint
      << "\n"
      << "target_status=" << target::lattice_target_status_name(eval.status)
      << "\n"
      << "resolved=" << bool_json(resolved) << "\n"
      << "resolution_status=" << resolution_status << "\n"
      << "concrete_path=" << concrete_path << "\n"
      << "certificate_digest=" << eval.proof_certificate.certificate_digest
      << "\n"
      << "closure_checked=" << bool_json(closure_checked) << "\n"
      << "closure_complete=" << bool_json(closure_complete) << "\n"
      << "closure_fact_count=" << closure.fact_count << "\n"
      << "proof_certificate_check_passed="
      << bool_json(eval.proof_certificate_check.passed) << "\n"
      << "root_checkpoint_id=" << closure.root_checkpoint_id << "\n"
      << "root_checkpoint_file_digest=" << closure.root_checkpoint_file_digest
      << "\n";
  const std::string resolver_receipt_digest =
      exposure::exposure_digest_for_text(receipt_text.str());

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.latest_satisfying_resolution.v1\""
       << ",\"schema_version\":1"
       << ",\"ok\":true"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"checkpoint_selector\":true"
       << ",\"checkpoint_selector_kind\":\"deterministic_readiness_latest_"
          "satisfying\""
       << ",\"fallback_selection\":false"
       << ",\"requires_checkpoint_producing_target\":true"
       << ",\"checkpoint_selectable_source_target\":"
       << bool_json(checkpoint_selectable_source_target)
       << ",\"selection_basis\":\"latest_satisfying\""
       << ",\"symbolic_hint\":" << json_quote(symbolic_hint)
       << ",\"source_target_id\":" << json_quote(source_target_id)
       << ",\"source_target_class\":" << json_quote(eval.target_class)
       << ",\"source_proof_kind\":" << json_quote(eval.proof_kind)
       << ",\"source_subject_fact_family\":"
       << json_quote(eval.subject_fact_family) << ",\"config_path\":"
       << json_quote(config_path.lexically_normal().string())
       << ",\"runtime_root\":"
       << json_quote(runtime_root.lexically_normal().string())
       << ",\"active_identity\":{\"protocol_contract_fingerprint\":"
       << json_quote(identity.protocol_contract_fingerprint)
       << ",\"graph_order_fingerprint\":"
       << json_quote(identity.graph_order_fingerprint)
       << ",\"source_cursor_token\":"
       << json_quote(identity.source_cursor_token)
       << ",\"vicreg_assembly_fingerprint\":"
       << json_quote(identity.vicreg_assembly_fingerprint)
       << ",\"mdn_assembly_fingerprint\":"
       << json_quote(identity.mdn_assembly_fingerprint) << "}"
       << ",\"target_status\":"
       << json_quote(target::lattice_target_status_name(eval.status))
       << ",\"resolved\":" << bool_json(resolved)
       << ",\"resolution_status\":" << json_quote(resolution_status)
       << ",\"concrete_path\":";
  if (resolved) {
    json << json_quote(concrete_path);
  } else {
    json << "null";
  }
  json << ",\"checkpoint_closure_checked\":" << bool_json(closure_checked)
       << ",\"checkpoint_closure_complete\":" << bool_json(closure_complete)
       << ",\"closure_fact_count\":" << closure.fact_count
       << ",\"checkpoint_fact_count\":" << closure.fact_count
       << ",\"checkpoint_path_present\":" << bool_json(checkpoint_path_present)
       << ",\"checkpoint_id\":" << json_quote(closure.root_checkpoint_id)
       << ",\"checkpoint_file_digest\":"
       << json_quote(closure.root_checkpoint_file_digest)
       << ",\"checkpoint_ref\":"
       << json_quote(
              display::checkpoint_ref(closure.root_checkpoint_file_digest))
       << ",\"certificate_ref\":"
       << json_quote(display::certificate_ref(
              eval.proof_certificate.certificate_digest))
       << ",\"certificate_digest\":"
       << json_quote(eval.proof_certificate.certificate_digest)
       << ",\"proof_certificate_check_passed\":"
       << bool_json(eval.proof_certificate_check.passed)
       << ",\"resolver_receipt_digest\":" << json_quote(resolver_receipt_digest)
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count_total\":"
       << scan.ledger.checkpoint_facts().size() << ",\"identity_mismatches\":"
       << string_array_json(closure.identity_mismatches)
       << ",\"unresolved_input_checkpoints\":"
       << path_array_json(closure.unresolved_input_checkpoints) << "}";
  *out = json.str();
  return true;
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
  const auto scan = session_scan_for_runtime_root(runtime_root);
  const auto node_support_summaries =
      node_support_summaries_from_facts(scan.ledger.node_facts());
  const auto source_receipt_summary = exposure::summarize_source_receipts(
      scan.ledger.facts(), scan.ledger.source_receipt_facts());
  const auto source_key_map_audit_summary =
      exposure::summarize_source_key_map_audits(
          scan.ledger.facts(), scan.ledger.source_receipt_facts());
  const auto source_analytics_summary = exposure::summarize_source_analytics(
      scan.ledger.facts(), scan.ledger.source_analytics_facts());
  const auto target_transform_summary = exposure::summarize_target_transforms(
      scan.ledger.facts(), scan.ledger.target_transform_facts());
  const auto forecast_baseline_summary = exposure::summarize_forecast_baselines(
      scan.ledger.facts(), scan.ledger.forecast_baseline_facts(),
      scan.ledger.target_transform_facts());
  const auto forecast_eval_summary = exposure::summarize_forecast_evals(
      scan.ledger.facts(), scan.ledger.forecast_eval_facts(),
      scan.ledger.target_transform_facts(),
      scan.ledger.forecast_baseline_facts(),
      scan.ledger.selection_signal_facts());
  const auto observer_belief_summary = exposure::summarize_observer_beliefs(
      scan.ledger.facts(), scan.ledger.observer_belief_facts(),
      scan.ledger.forecast_eval_facts());
  const auto allocation_engine_summary = exposure::summarize_allocation_engines(
      scan.ledger.facts(), scan.ledger.allocation_engine_facts(),
      scan.ledger.observer_belief_facts(), scan.ledger.forecast_eval_facts());
  const auto replay_environment_summary =
      exposure::summarize_replay_environments(
          scan.ledger.facts(), scan.ledger.replay_environment_facts());
  const auto selection_signal_summary = exposure::summarize_selection_signals(
      scan.ledger.facts(), scan.ledger.selection_signal_facts(),
      scan.ledger.forecast_eval_facts());
  const auto representation_support_summary =
      exposure::summarize_representation_support(
          scan.ledger.facts(), scan.ledger.representation_support_facts());
  const auto representation_geometry_gate_review_summary =
      target::summarize_representation_geometry_gate_review(
          scan.ledger.facts());
  std::ostringstream json;
  json << "{\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"source_receipt_fact_count\":"
       << scan.ledger.source_receipt_facts().size()
       << ",\"source_analytics_fact_count\":"
       << scan.ledger.source_analytics_facts().size()
       << ",\"target_transform_fact_count\":"
       << scan.ledger.target_transform_facts().size()
       << ",\"forecast_baseline_fact_count\":"
       << scan.ledger.forecast_baseline_facts().size()
       << ",\"forecast_eval_fact_count\":"
       << scan.ledger.forecast_eval_facts().size()
       << ",\"observer_belief_fact_count\":"
       << scan.ledger.observer_belief_facts().size()
       << ",\"allocation_engine_fact_count\":"
       << scan.ledger.allocation_engine_facts().size()
       << ",\"replay_environment_fact_count\":"
       << scan.ledger.replay_environment_facts().size()
       << ",\"policy_training_fact_count\":"
       << scan.ledger.policy_training_facts().size()
       << ",\"selection_signal_fact_count\":"
       << scan.ledger.selection_signal_facts().size()
       << ",\"representation_support_fact_count\":"
       << scan.ledger.representation_support_facts().size()
       << ",\"returned_fact_count\":"
       << std::min<std::size_t>(scan.ledger.facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_node_exposure_fact_count\":"
       << std::min<std::size_t>(scan.ledger.node_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_checkpoint_fact_count\":"
       << std::min<std::size_t>(scan.ledger.checkpoint_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_source_receipt_fact_count\":"
       << std::min<std::size_t>(scan.ledger.source_receipt_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_source_analytics_fact_count\":"
       << std::min<std::size_t>(scan.ledger.source_analytics_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_target_transform_fact_count\":"
       << std::min<std::size_t>(scan.ledger.target_transform_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_forecast_baseline_fact_count\":"
       << std::min<std::size_t>(scan.ledger.forecast_baseline_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_forecast_eval_fact_count\":"
       << std::min<std::size_t>(scan.ledger.forecast_eval_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_observer_belief_fact_count\":"
       << std::min<std::size_t>(scan.ledger.observer_belief_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_allocation_engine_fact_count\":"
       << std::min<std::size_t>(scan.ledger.allocation_engine_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_replay_environment_fact_count\":"
       << std::min<std::size_t>(scan.ledger.replay_environment_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_policy_training_fact_count\":"
       << std::min<std::size_t>(scan.ledger.policy_training_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_selection_signal_fact_count\":"
       << std::min<std::size_t>(scan.ledger.selection_signal_facts().size(),
                                static_cast<std::size_t>(limit))
       << ",\"returned_representation_support_fact_count\":"
       << std::min<std::size_t>(
              scan.ledger.representation_support_facts().size(),
              static_cast<std::size_t>(limit))
       << ",\"warnings\":" << string_array_json(scan.warnings) << ",\"facts\":"
       << facts_json(scan.ledger.facts(), static_cast<std::size_t>(limit))
       << ",\"node_exposure_facts\":"
       << node_facts_json(scan.ledger.node_facts(),
                          static_cast<std::size_t>(limit))
       << ",\"node_support_summaries\":"
       << node_support_summaries_json(node_support_summaries)
       << ",\"checkpoint_facts\":"
       << checkpoint_facts_json(scan.ledger.checkpoint_facts(),
                                static_cast<std::size_t>(limit))
       << ",\"source_receipt_facts\":"
       << source_receipt_facts_json(scan.ledger.source_receipt_facts(),
                                    static_cast<std::size_t>(limit))
       << ",\"source_receipt_summary\":"
       << source_receipt_summary_json(source_receipt_summary)
       << ",\"source_key_map_audit_summary\":"
       << source_key_map_audit_summary_json(source_key_map_audit_summary)
       << ",\"source_analytics_facts\":"
       << source_analytics_facts_json(scan.ledger.source_analytics_facts(),
                                      static_cast<std::size_t>(limit))
       << ",\"source_analytics_summary\":"
       << source_analytics_summary_json(source_analytics_summary)
       << ",\"target_transform_facts\":"
       << target_transform_facts_json(scan.ledger.target_transform_facts(),
                                      static_cast<std::size_t>(limit))
       << ",\"target_transform_summary\":"
       << target_transform_summary_json(target_transform_summary)
       << ",\"forecast_baseline_facts\":"
       << forecast_baseline_facts_json(scan.ledger.forecast_baseline_facts(),
                                       static_cast<std::size_t>(limit))
       << ",\"forecast_baseline_summary\":"
       << forecast_baseline_summary_json(forecast_baseline_summary)
       << ",\"forecast_eval_facts\":"
       << forecast_eval_facts_json(scan.ledger.forecast_eval_facts(),
                                   static_cast<std::size_t>(limit))
       << ",\"forecast_eval_summary\":"
       << forecast_eval_summary_json(forecast_eval_summary)
       << ",\"observer_belief_facts\":"
       << observer_belief_facts_json(scan.ledger.observer_belief_facts(),
                                     static_cast<std::size_t>(limit))
       << ",\"observer_belief_summary\":"
       << observer_belief_summary_json(observer_belief_summary)
       << ",\"allocation_engine_facts\":"
       << allocation_engine_facts_json(scan.ledger.allocation_engine_facts(),
                                       static_cast<std::size_t>(limit))
       << ",\"allocation_engine_summary\":"
       << allocation_engine_summary_json(allocation_engine_summary)
       << ",\"replay_environment_facts\":"
       << replay_environment_facts_json(scan.ledger.replay_environment_facts(),
                                        static_cast<std::size_t>(limit))
       << ",\"replay_environment_summary\":"
       << replay_environment_summary_json(replay_environment_summary)
       << ",\"selection_signal_facts\":"
       << selection_signal_facts_json(scan.ledger.selection_signal_facts(),
                                      static_cast<std::size_t>(limit))
       << ",\"selection_signal_summary\":"
       << selection_signal_summary_json(selection_signal_summary)
       << ",\"representation_support_facts\":"
       << representation_support_facts_json(
              scan.ledger.representation_support_facts(),
              static_cast<std::size_t>(limit))
       << ",\"representation_support_summary\":"
       << representation_support_summary_json(representation_support_summary)
       << ",\"representation_geometry_gate_review_summary\":"
       << representation_geometry_gate_review_summary_json(
              representation_geometry_gate_review_summary)
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_list_fact_families(const std::string &args,
                                             lattice_context_t *ctx,
                                             std::string *out,
                                             std::string *err) {
  std::string runtime_root_arg;
  (void)extract_json_string_field(args, "runtime_root", &runtime_root_arg);

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.fact_family_registry.v1\""
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"registry\":" << fact_family_registry_json()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json();
  if (!trim_ascii(runtime_root_arg).empty()) {
    fs::path runtime_root;
    if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
      return false;
    }
    const auto scan = session_scan_for_runtime_root(runtime_root);
    json << ",\"runtime_root\":" << json_quote(runtime_root.string())
         << ",\"catalog_summary\":"
         << fact_catalog_summary_json(
                exposure::summarize_lattice_fact_catalog(scan.ledger))
         << ",\"warnings\":" << string_array_json(scan.warnings);
  }
  json << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_scan_facts(const std::string &args,
                                     lattice_context_t *ctx, std::string *out,
                                     std::string *err) {
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
  bool include_facts = true;
  if (!parse_optional_bool_arg(args, "include_facts", true, &include_facts,
                               err)) {
    return false;
  }
  std::vector<exposure::lattice_fact_family_t> families;
  try {
    families = selected_fact_families_from_arg(args);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  const auto scan = session_scan_for_runtime_root(runtime_root);
  const auto fact_integrity_summary =
      exposure::summarize_lattice_fact_integrity(scan.ledger, families);
  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.fact_catalog_scan.v1\""
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"catalog_summary\":"
       << fact_catalog_summary_json(
              exposure::summarize_lattice_fact_catalog(scan.ledger))
       << ",\"fact_integrity_summary\":"
       << fact_integrity_summary_json(fact_integrity_summary)
       << ",\"returned_family_count\":" << families.size()
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"families\":[";
  for (std::size_t i = 0; i < families.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << scan_fact_family_json(scan.ledger, families[i],
                                  static_cast<std::size_t>(limit),
                                  include_facts);
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_fact_summary(const std::string &args,
                                       lattice_context_t *ctx, std::string *out,
                                       std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  std::vector<exposure::lattice_fact_family_t> families;
  try {
    families = selected_fact_families_from_arg(args);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  const auto scan = session_scan_for_runtime_root(runtime_root);
  const auto fact_integrity_summary =
      exposure::summarize_lattice_fact_integrity(scan.ledger, families);
  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.fact_summary.v1\""
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"catalog_summary\":"
       << fact_catalog_summary_json(
              exposure::summarize_lattice_fact_catalog(scan.ledger))
       << ",\"fact_integrity_summary\":"
       << fact_integrity_summary_json(fact_integrity_summary)
       << ",\"returned_family_count\":" << families.size()
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"families\":[";
  for (std::size_t i = 0; i < families.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    const auto summary =
        exposure::summarize_lattice_fact_family(scan.ledger, families[i]);
    json << "{\"catalog_summary\":" << fact_catalog_family_summary_json(summary)
         << ",\"payload_summary\":"
         << fact_family_payload_summary_json(scan.ledger, families[i]) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_fact_lineage(const std::string &args,
                                       lattice_context_t *ctx, std::string *out,
                                       std::string *err) {
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
  std::vector<exposure::lattice_fact_family_t> families;
  try {
    families = selected_fact_families_from_arg(args);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  const auto scan = session_scan_for_runtime_root(runtime_root);
  const auto cache =
      exposure::make_runtime_index_cache_from_scan(runtime_root, scan);
  const auto selected_relations = relation_names_for_fact_families(families);
  const auto fact_integrity_summary =
      exposure::summarize_lattice_fact_integrity(scan.ledger, families);

  std::vector<exposure::lattice_runtime_index_row_t> selected_rows;
  for (const auto &row : cache.rows) {
    if (relation_is_selected(selected_relations, row.relation)) {
      selected_rows.push_back(row);
    }
  }
  const auto selected_relation_counts =
      exposure::runtime_index_relation_counts(selected_rows);
  const std::size_t returned_row_count = std::min<std::size_t>(
      selected_rows.size(), static_cast<std::size_t>(limit));

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.fact_lineage.v1\""
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"lineage_rows_are_audit_only\":true"
       << ",\"cache_rows_used_for_target_satisfaction\":false"
       << ",\"artifact_readiness_proof_template_registry\":"
       << artifact_readiness_proof_template_registry_json()
       << ",\"catalog_summary\":"
       << fact_catalog_summary_json(
              exposure::summarize_lattice_fact_catalog(scan.ledger))
       << ",\"fact_integrity_summary\":"
       << fact_integrity_summary_json(fact_integrity_summary)
       << ",\"selected_family_count\":" << families.size()
       << ",\"selected_relation_count\":" << selected_relations.size()
       << ",\"selected_relations\":" << string_array_json(selected_relations)
       << ",\"runtime_index_row_set_digest\":"
       << json_quote(cache.row_set_digest) << ",\"lineage_row_set_digest\":"
       << json_quote(exposure::runtime_index_rows_digest(selected_rows))
       << ",\"relation_counts\":"
       << runtime_index_relation_counts_json(selected_relation_counts)
       << ",\"matching_row_count\":" << selected_rows.size()
       << ",\"returned_row_count\":" << returned_row_count << ",\"truncated\":"
       << bool_json(returned_row_count < selected_rows.size())
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"lineage_rows\":"
       << runtime_index_rows_json(selected_rows,
                                  static_cast<std::size_t>(limit))
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_fact_preview(const std::string &args,
                                       lattice_context_t *ctx, std::string *out,
                                       std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  std::string family_arg;
  if (!extract_json_string_field(args, "family", &family_arg) ||
      trim_ascii(family_arg).empty()) {
    if (err) {
      *err = "fact_preview requires family";
    }
    return false;
  }
  const auto family = exposure::parse_lattice_fact_family(family_arg);
  if (!family.has_value()) {
    if (err) {
      *err = "unknown fact family: " + family_arg;
    }
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

  fact_preview_filter_t filter{};
  filter.limit = static_cast<std::size_t>(limit);
  if (extract_json_raw_field(args, "digest", nullptr)) {
    if (err) {
      *err =
          "hero.lattice.inspect subject=facts mode=preview uses fact_digest; "
          "digest is reserved for subject=index queries";
    }
    return false;
  }
  if (extract_json_raw_field(args, "digest_prefix", nullptr)) {
    if (err) {
      *err = "hero.lattice.inspect subject=facts mode=preview uses "
             "fact_digest_prefix; digest_prefix is reserved for subject=index "
             "queries";
    }
    return false;
  }
  if (extract_json_raw_field(args, "index", nullptr)) {
    if (err) {
      *err = "hero.lattice.inspect subject=facts mode=preview uses fact_index; "
             "index is reserved for subject=index queries";
    }
    return false;
  }
  if (!parse_optional_string_arg(args, "fact_digest", "", &filter.digest,
                                 err)) {
    return false;
  }
  if (!parse_optional_string_arg(args, "fact_digest_prefix", "",
                                 &filter.fact_digest_prefix, err)) {
    return false;
  }
  int fact_index = -1;
  if (!parse_optional_int_arg(args, "fact_index", -1, &fact_index, err)) {
    return false;
  }
  if (fact_index < -1) {
    if (err) {
      *err = "fact_index must be >= 0";
    }
    return false;
  }
  if (fact_index >= 0) {
    filter.fact_index = static_cast<std::size_t>(fact_index);
  }

  const auto scan = session_scan_for_runtime_root(runtime_root);
  if (!filter.fact_digest_prefix.empty()) {
    const auto prefix_resolution = fact_family_digest_prefix_resolve(
        scan.ledger, *family, filter.fact_digest_prefix);
    if (prefix_resolution.match_count == 0) {
      if (err) {
        *err = std::string(
                   "E_LATTICE_FACT_REF_NOT_FOUND: fact_digest_prefix matched "
                   "zero facts in family ") +
               exposure::lattice_fact_family_name(*family);
      }
      return false;
    }
    if (prefix_resolution.match_count > 1) {
      if (err) {
        *err = "E_LATTICE_FACT_REF_AMBIGUOUS: fact_digest_prefix matched " +
               std::to_string(prefix_resolution.match_count) +
               " facts in family " +
               exposure::lattice_fact_family_name(*family);
      }
      return false;
    }
    if (!filter.digest.empty() && filter.digest != prefix_resolution.digest) {
      if (err) {
        *err = "E_LATTICE_FACT_REF_NOT_FOUND: fact_digest and "
               "fact_digest_prefix resolve to different facts in family " +
               std::string(exposure::lattice_fact_family_name(*family));
      }
      return false;
    }
    filter.resolved_prefix_digest = prefix_resolution.digest;
  }
  const auto catalog_family_summary =
      exposure::summarize_lattice_fact_family(scan.ledger, *family);
  const auto fact_integrity_summary =
      exposure::summarize_lattice_fact_integrity(scan.ledger, {*family});
  std::size_t matching_fact_count = 0;
  bool truncated = false;
  const auto preview_rows = fact_family_preview_rows_json(
      scan.ledger, *family, filter, &matching_fact_count, &truncated);
  const std::size_t returned_fact_count =
      std::min<std::size_t>(matching_fact_count, filter.limit);

  const auto cache =
      exposure::make_runtime_index_cache_from_scan(runtime_root, scan);
  const auto relation = relation_for_fact_family(*family);
  std::vector<exposure::lattice_runtime_index_row_t> selected_rows;
  for (const auto &row : cache.rows) {
    if (row.relation != relation) {
      continue;
    }
    if (!fact_preview_digest_matches(row.digest, filter)) {
      continue;
    }
    selected_rows.push_back(row);
  }
  const auto selected_relation_counts =
      exposure::runtime_index_relation_counts(selected_rows);
  const std::size_t returned_lineage_row_count =
      std::min<std::size_t>(selected_rows.size(), filter.limit);

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.fact_preview.v1\""
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"family\":"
       << json_quote(exposure::lattice_fact_family_name(*family))
       << ",\"relation\":" << json_quote(relation) << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"preview_rows_are_audit_only\":true"
       << ",\"facts_used_for_target_satisfaction\":false"
       << ",\"cache_rows_used_for_target_satisfaction\":false"
       << ",\"checkpoint_selected\":false"
       << ",\"model_selector\":false"
       << ",\"short_ref_scheme\":\"kikijyeba.hero.short_ref.v1\""
       << ",\"display_digest_prefix_length\":"
       << display::kDefaultDigestDisplayPrefixLength
       << ",\"fact_digest_filter\":" << json_quote(filter.digest)
       << ",\"fact_digest_prefix_filter\":"
       << json_quote(filter.fact_digest_prefix) << ",\"fact_index_filter\":";
  if (filter.fact_index.has_value()) {
    json << *filter.fact_index;
  } else {
    json << "null";
  }
  json << ",\"catalog_summary\":"
       << fact_catalog_summary_json(
              exposure::summarize_lattice_fact_catalog(scan.ledger))
       << ",\"catalog_family_summary\":"
       << fact_catalog_family_summary_json(catalog_family_summary)
       << ",\"payload_summary\":"
       << fact_family_payload_summary_json(scan.ledger, *family)
       << ",\"fact_integrity_summary\":"
       << fact_integrity_summary_json(fact_integrity_summary)
       << ",\"family_fact_count\":" << catalog_family_summary.fact_count
       << ",\"matching_fact_count\":" << matching_fact_count
       << ",\"returned_fact_count\":" << returned_fact_count
       << ",\"truncated\":" << bool_json(truncated)
       << ",\"runtime_index_row_set_digest\":"
       << json_quote(cache.row_set_digest) << ",\"lineage_row_set_digest\":"
       << json_quote(exposure::runtime_index_rows_digest(selected_rows))
       << ",\"relation_counts\":"
       << runtime_index_relation_counts_json(selected_relation_counts)
       << ",\"matching_lineage_row_count\":" << selected_rows.size()
       << ",\"returned_lineage_row_count\":" << returned_lineage_row_count
       << ",\"lineage_rows\":"
       << runtime_index_rows_json(selected_rows, filter.limit)
       << ",\"warnings\":" << string_array_json(scan.warnings)
       << ",\"facts\":" << preview_rows << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_index_status(const std::string &args,
                                       lattice_context_t *ctx, std::string *out,
                                       std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  fs::path index_path;
  if (!resolve_runtime_index_path_arg(args, ctx, runtime_root, &index_path,
                                      err)) {
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
  exposure::lattice_runtime_index_validation_strength_t validation_strength =
      exposure::lattice_runtime_index_validation_strength_t::
          full_runtime_metadata_digest;
  if (!parse_runtime_index_validation_strength_arg(
          args,
          exposure::lattice_runtime_index_validation_strength_t::
              full_runtime_metadata_digest,
          &validation_strength, err)) {
    return false;
  }

  exposure::lattice_runtime_index_cache_t cached{};
  std::error_code ec;
  const bool cache_file_exists = fs::exists(index_path, ec);
  if (cache_file_exists) {
    cached = exposure::read_runtime_index_cache(index_path);
  } else {
    cached.schema.clear();
  }
  const auto validation = exposure::validate_runtime_index_cache(
      cached, runtime_root, validation_strength);
  const bool use_cache = validation.cache_valid;
  const auto selected =
      use_cache ? cached
                : session_runtime_index_cache_for_runtime_root(runtime_root);

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.runtime_index_status.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"checkpoint_selector\":false"
       << ",\"automatic_checkpoint_selection\":false"
       << ",\"db_source_of_truth\":false"
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"index_path\":" << json_quote(index_path.string())
       << ",\"index_schema\":\"kikijyeba.lattice.runtime_index_cache.v1\""
       << ",\"cache_file_exists\":" << bool_json(cache_file_exists)
       << ",\"validation_strength\":"
       << json_quote(exposure::runtime_index_validation_strength_name(
              validation_strength))
       << ",\"result_source\":" << json_quote(use_cache ? "cache" : "live_scan")
       << ",\"cache_used_for_target_satisfaction\":false"
       << ",\"index_written\":false"
       << ",\"stale_or_missing_cache_upgrades_satisfaction\":false"
       << ",\"cache_validation\":"
       << runtime_index_cache_validation_json(validation) << ",\"index\":"
       << runtime_index_cache_json(selected, static_cast<std::size_t>(limit))
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_index_query(const std::string &args,
                                      lattice_context_t *ctx, std::string *out,
                                      std::string *err) {
  fs::path runtime_root;
  if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
    return false;
  }
  fs::path index_path;
  if (!resolve_runtime_index_path_arg(args, ctx, runtime_root, &index_path,
                                      err)) {
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
  bool compare_live_scan = true;
  if (!parse_optional_bool_arg(args, "compare_live_scan", true,
                               &compare_live_scan, err)) {
    return false;
  }
  bool allow_unproven_cache = false;
  if (!parse_optional_bool_arg(args, "allow_unproven_cache", false,
                               &allow_unproven_cache, err)) {
    return false;
  }
  exposure::lattice_runtime_index_validation_strength_t validation_strength =
      exposure::lattice_runtime_index_validation_strength_t::
          full_runtime_metadata_digest;
  if (!parse_runtime_index_validation_strength_arg(
          args,
          exposure::lattice_runtime_index_validation_strength_t::
              full_runtime_metadata_digest,
          &validation_strength, err)) {
    return false;
  }

  exposure::lattice_runtime_index_query_t query{};
  (void)extract_json_string_field(args, "relation", &query.relation);
  (void)extract_json_string_field(args, "key", &query.key);
  (void)extract_json_string_field(args, "key_contains", &query.key_contains);
  (void)extract_json_string_field(args, "digest", &query.digest);
  (void)extract_json_string_field(args, "digest_prefix", &query.digest_prefix);
  query.relation = trim_ascii(query.relation);
  query.key = trim_ascii(query.key);
  query.key_contains = trim_ascii(query.key_contains);
  query.digest = trim_ascii(query.digest);
  query.digest_prefix = trim_ascii(query.digest_prefix);

  exposure::lattice_runtime_index_cache_t cached{};
  std::error_code ec;
  const bool cache_file_exists = fs::exists(index_path, ec);
  if (cache_file_exists) {
    cached = exposure::read_runtime_index_cache(index_path);
  } else {
    cached.schema.clear();
  }
  const auto validation = exposure::validate_runtime_index_cache(
      cached, runtime_root, validation_strength);

  exposure::lattice_runtime_index_cache_t live_cache{};
  bool live_built = false;
  const auto get_live_cache =
      [&]() -> const exposure::lattice_runtime_index_cache_t & {
    if (!live_built) {
      live_cache = session_runtime_index_cache_for_runtime_root(runtime_root);
      live_built = true;
    }
    return live_cache;
  };

  exposure::lattice_runtime_index_parity_t stored_cache_parity{};
  if (compare_live_scan) {
    if (cache_file_exists) {
      stored_cache_parity = exposure::compare_runtime_index_caches(
          cached, get_live_cache(), static_cast<std::size_t>(limit));
    } else {
      stored_cache_parity.checked = false;
      stored_cache_parity.issues.push_back("missing_cache");
    }
  }

  const bool use_proven_cache =
      validation.cache_valid && compare_live_scan && stored_cache_parity.passed;
  const bool use_unproven_cache =
      validation.cache_valid && !compare_live_scan && allow_unproven_cache;
  const bool use_cache = use_proven_cache || use_unproven_cache;
  const auto &selected = use_cache ? cached : get_live_cache();
  const auto selected_query = exposure::query_runtime_index_cache(
      selected, query, static_cast<std::size_t>(limit));

  exposure::lattice_runtime_index_parity_t selected_answer_parity{};
  if (compare_live_scan) {
    const auto &live = get_live_cache();
    const auto selected_query_all = exposure::query_runtime_index_cache(
        selected, query, std::numeric_limits<std::size_t>::max());
    const auto live_query_all = exposure::query_runtime_index_cache(
        live, query, std::numeric_limits<std::size_t>::max());
    selected_answer_parity = exposure::compare_runtime_index_query_results(
        selected_query_all, live_query_all, static_cast<std::size_t>(limit));
  }

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.runtime_index_query_result.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"checkpoint_selector\":false"
       << ",\"automatic_checkpoint_selection\":false"
       << ",\"db_source_of_truth\":false"
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"index_path\":" << json_quote(index_path.string())
       << ",\"index_query_schema\":"
          "\"kikijyeba.lattice.runtime_index_query.v1\""
       << ",\"index_schema\":\"kikijyeba.lattice.runtime_index_cache.v1\""
       << ",\"cache_file_exists\":" << bool_json(cache_file_exists)
       << ",\"compare_live_scan\":" << bool_json(compare_live_scan)
       << ",\"allow_unproven_cache\":" << bool_json(allow_unproven_cache)
       << ",\"validation_strength\":"
       << json_quote(exposure::runtime_index_validation_strength_name(
              validation_strength))
       << ",\"result_source\":" << json_quote(use_cache ? "cache" : "live_scan")
       << ",\"cache_valid_for_audit_query\":" << bool_json(use_proven_cache)
       << ",\"cache_used_unproven_for_audit_query\":"
       << bool_json(use_unproven_cache)
       << ",\"cache_used_for_target_satisfaction\":false"
       << ",\"index_written\":false"
       << ",\"stale_or_mismatched_cache_upgrades_query\":false"
       << ",\"stale_or_missing_cache_upgrades_satisfaction\":false"
       << ",\"query\":" << runtime_index_query_json(query)
       << ",\"cache_validation\":"
       << runtime_index_cache_validation_json(validation)
       << ",\"stored_cache_parity\":"
       << runtime_index_parity_json(stored_cache_parity)
       << ",\"selected_answer_parity\":"
       << runtime_index_parity_json(selected_answer_parity)
       << ",\"answer\":" << runtime_index_query_result_json(selected_query)
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] std::string
canonical_derived_query_relation(std::string relation) {
  relation = lowercase_ascii(trim_ascii(relation));
  if (relation == "satisfied_target") {
    return "target_satisfied";
  }
  return relation;
}

[[nodiscard]] bool evaluate_target_for_derived_query(
    const std::string &args, lattice_context_t *ctx,
    const std::string &target_id, target::lattice_target_evaluation_t *eval_out,
    fs::path *config_path_out, fs::path *runtime_root_out,
    target::lattice_target_active_identity_t *identity_out,
    exposure::exposure_ledger_scan_result_t *scan_out, std::string *err) {
  if (target_id.empty()) {
    if (err) {
      *err = "target_id is required for this derived relation";
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
  if (eval_out) {
    *eval_out = std::move(eval);
  }
  if (config_path_out) {
    *config_path_out = std::move(config_path);
  }
  if (runtime_root_out) {
    *runtime_root_out = std::move(runtime_root);
  }
  if (identity_out) {
    *identity_out = std::move(identity);
  }
  if (scan_out) {
    *scan_out = std::move(scan);
  }
  return true;
}

[[nodiscard]] bool target_satisfied_relation_value(
    const target::lattice_target_evaluation_t &eval) {
  const auto &proof = eval.proof_certificate;
  return eval.status == target::lattice_target_status_t::satisfied &&
         proof_context_result_satisfied(proof.proof_context) &&
         dependency_results_satisfied(proof.dependencies) &&
         coverage_results_satisfied(proof.coverage) &&
         (!proof.closure.checked || proof.closure.complete) &&
         (!proof.leakage.checked || !proof.leakage.overlap_found) &&
         eval.proof_certificate_check.passed && eval.deficits.empty();
}

struct evidence_comparison_participation_t {
  bool participates{false};
  std::vector<std::string> exclusion_reasons{};
};

[[nodiscard]] evidence_comparison_participation_t
evidence_comparison_participation(
    const target::lattice_target_evaluation_t &eval) {
  evidence_comparison_participation_t out{};
  const auto &proof = eval.proof_certificate;
  const auto &vector = eval.evidence_order_vector;

  if (!vector.available) {
    out.exclusion_reasons.push_back("evidence_order_vector_unavailable");
  }
  if (eval.status != target::lattice_target_status_t::satisfied) {
    out.exclusion_reasons.push_back(
        "target_status_" +
        std::string(target::lattice_target_status_name(eval.status)));
  }
  if (!eval.proof_certificate_check.passed) {
    out.exclusion_reasons.push_back("proof_certificate_check_failed");
  }
  if (!target_satisfied_relation_value(eval)) {
    out.exclusion_reasons.push_back("target_satisfaction_not_clean");
  }
  if (!proof.closure.checked) {
    out.exclusion_reasons.push_back("checkpoint_closure_not_checked");
  }
  if (proof.closure.checkpoint_path.empty()) {
    out.exclusion_reasons.push_back("checkpoint_path_missing");
  }
  if (!proof.closure.complete) {
    out.exclusion_reasons.push_back("checkpoint_closure_incomplete");
  }
  if (vector.unresolved_lineage_count != 0) {
    out.exclusion_reasons.push_back("unresolved_lineage_present");
  }
  if (!proof.leakage.checked) {
    out.exclusion_reasons.push_back("leakage_not_checked");
  }
  if (proof.leakage.overlap_found || !proof.leakage.passed) {
    out.exclusion_reasons.push_back("forbidden_leakage_present");
  }
  if (vector.selection_signal_leakage_found) {
    out.exclusion_reasons.push_back("selection_signal_leakage_present");
  }

  out.participates = out.exclusion_reasons.empty();
  return out;
}

[[nodiscard]] std::string evidence_comparison_side_json(
    const std::string &side, const target::lattice_target_evaluation_t &eval,
    const evidence_comparison_participation_t &participation) {
  std::ostringstream out;
  out << "{\"side\":" << json_quote(side)
      << ",\"target_id\":" << json_quote(eval.target_id) << ",\"status\":"
      << json_quote(target::lattice_target_status_name(eval.status))
      << ",\"checkpoint_path\":"
      << json_quote(eval.proof_certificate.closure.checkpoint_path.string())
      << ",\"checkpoint_id\":"
      << json_quote(eval.proof_certificate.closure.root_checkpoint_id)
      << ",\"checkpoint_file_digest\":"
      << json_quote(eval.proof_certificate.closure.root_checkpoint_file_digest)
      << ",\"checkpoint_ref\":"
      << json_quote(display::checkpoint_ref(
             eval.proof_certificate.closure.root_checkpoint_file_digest))
      << ",\"certificate_ref\":"
      << json_quote(display::certificate_ref(
             eval.proof_certificate.certificate_digest))
      << ",\"certificate_digest\":"
      << json_quote(eval.proof_certificate.certificate_digest)
      << ",\"proof_certificate_check\":"
      << proof_certificate_check_json(eval.proof_certificate_check)
      << ",\"participates\":" << bool_json(participation.participates)
      << ",\"exclusion_reasons\":"
      << string_array_json(participation.exclusion_reasons)
      << ",\"selection_signal_leakage_checked_before_participation\":true"
      << ",\"evidence_order_vector\":"
      << evidence_order_vector_json(eval.evidence_order_vector) << "}";
  return out.str();
}

struct evidence_dimension_comparison_row_t {
  std::string dimension{};
  std::string polarity{};
  bool comparison_dimension{true};
  std::string left_value_json{};
  std::string right_value_json{};
  std::string relation{};
};

[[nodiscard]] int compare_bool_true_is_stronger(bool lhs, bool rhs) {
  if (lhs == rhs) {
    return 0;
  }
  return lhs ? 1 : -1;
}

[[nodiscard]] int compare_bool_false_is_stronger(bool lhs, bool rhs) {
  if (lhs == rhs) {
    return 0;
  }
  return !lhs ? 1 : -1;
}

[[nodiscard]] int compare_count_lower_is_stronger(std::int64_t lhs,
                                                  std::int64_t rhs) {
  if (lhs == rhs) {
    return 0;
  }
  return lhs < rhs ? 1 : -1;
}

[[nodiscard]] int compare_double_higher_is_stronger(double lhs, double rhs) {
  const bool lhs_finite = std::isfinite(lhs);
  const bool rhs_finite = std::isfinite(rhs);
  if (!lhs_finite && !rhs_finite) {
    return 0;
  }
  if (lhs_finite != rhs_finite) {
    return lhs_finite ? 1 : -1;
  }
  if (std::abs(lhs - rhs) <= 1e-9) {
    return 0;
  }
  return lhs > rhs ? 1 : -1;
}

[[nodiscard]] int compare_double_lower_is_stronger(double lhs, double rhs) {
  const bool lhs_finite = std::isfinite(lhs);
  const bool rhs_finite = std::isfinite(rhs);
  if (!lhs_finite && !rhs_finite) {
    return 0;
  }
  if (lhs_finite != rhs_finite) {
    return lhs_finite ? 1 : -1;
  }
  if (std::abs(lhs - rhs) <= 1e-9) {
    return 0;
  }
  return lhs < rhs ? 1 : -1;
}

[[nodiscard]] std::string comparison_relation_name(int cmp) {
  if (cmp > 0) {
    return "left_stronger";
  }
  if (cmp < 0) {
    return "right_stronger";
  }
  return "equal";
}

[[nodiscard]] std::vector<evidence_dimension_comparison_row_t>
evidence_dimension_comparison_rows(
    const target::lattice_target_evaluation_t::evidence_order_vector_t &left,
    const target::lattice_target_evaluation_t::evidence_order_vector_t &right,
    std::vector<std::string> *left_better_dimensions,
    std::vector<std::string> *right_better_dimensions,
    std::int64_t *equal_comparison_dimension_count) {
  std::vector<evidence_dimension_comparison_row_t> rows;
  std::int64_t equal_count = 0;
  const auto add_comparison =
      [&](const std::string &dimension, const std::string &polarity,
          bool comparison_dimension, const std::string &left_value_json,
          const std::string &right_value_json, int cmp) {
        evidence_dimension_comparison_row_t row{};
        row.dimension = dimension;
        row.polarity = polarity;
        row.comparison_dimension = comparison_dimension;
        row.left_value_json = left_value_json;
        row.right_value_json = right_value_json;
        row.relation = comparison_dimension ? comparison_relation_name(cmp)
                                            : "visibility_only";
        rows.push_back(std::move(row));
        if (!comparison_dimension) {
          return;
        }
        if (cmp > 0) {
          left_better_dimensions->push_back(dimension);
        } else if (cmp < 0) {
          right_better_dimensions->push_back(dimension);
        } else {
          ++equal_count;
        }
      };
  const auto add_bool_true = [&](const std::string &dimension, bool lhs,
                                 bool rhs) {
    add_comparison(dimension, "true_is_stronger", true, bool_json(lhs),
                   bool_json(rhs), compare_bool_true_is_stronger(lhs, rhs));
  };
  const auto add_bool_false = [&](const std::string &dimension, bool lhs,
                                  bool rhs) {
    add_comparison(dimension, "false_required_for_participation", true,
                   bool_json(lhs), bool_json(rhs),
                   compare_bool_false_is_stronger(lhs, rhs));
  };
  const auto add_double_higher = [&](const std::string &dimension,
                                     const std::string &polarity, double lhs,
                                     double rhs) {
    add_comparison(dimension, polarity, true, double_json(lhs),
                   double_json(rhs),
                   compare_double_higher_is_stronger(lhs, rhs));
  };
  const auto add_double_lower = [&](const std::string &dimension, double lhs,
                                    double rhs) {
    add_comparison(dimension, "lower_is_stronger", true, double_json(lhs),
                   double_json(rhs),
                   compare_double_lower_is_stronger(lhs, rhs));
  };
  const auto add_count_lower = [&](const std::string &dimension,
                                   std::int64_t lhs, std::int64_t rhs) {
    add_comparison(dimension, "lower_is_stronger", true, std::to_string(lhs),
                   std::to_string(rhs),
                   compare_count_lower_is_stronger(lhs, rhs));
  };
  const auto add_visibility_count = [&](const std::string &dimension,
                                        const std::string &polarity,
                                        std::int64_t lhs, std::int64_t rhs) {
    add_comparison(dimension, polarity, false, std::to_string(lhs),
                   std::to_string(rhs), 0);
  };

  add_bool_true("satisfied", left.satisfied, right.satisfied);
  add_bool_true("proof_certificate_check_passed",
                left.proof_certificate_check_passed,
                right.proof_certificate_check_passed);
  add_bool_true("identity_contract_match", left.identity_contract_match,
                right.identity_contract_match);
  add_bool_true("identity_component_match", left.identity_component_match,
                right.identity_component_match);
  add_bool_true("identity_graph_order_match", left.identity_graph_order_match,
                right.identity_graph_order_match);
  add_bool_true("identity_source_cursor_match",
                left.identity_source_cursor_match,
                right.identity_source_cursor_match);
  add_double_higher("min_unique_coverage_fraction", "higher_is_stronger",
                    left.min_unique_coverage_fraction,
                    right.min_unique_coverage_fraction);
  add_double_higher("max_cursor_exposure_load", "higher_is_stronger_clean_load",
                    left.max_cursor_exposure_load,
                    right.max_cursor_exposure_load);
  add_bool_true("closure_checked", left.closure_checked, right.closure_checked);
  add_bool_true("closure_complete", left.closure_complete,
                right.closure_complete);
  add_bool_true("leakage_checked", left.leakage_checked, right.leakage_checked);
  add_bool_true("leakage_passed", left.leakage_passed, right.leakage_passed);
  add_bool_false("selection_signal_leakage_found",
                 left.selection_signal_leakage_found,
                 right.selection_signal_leakage_found);
  add_double_higher("aggregate_valid_target_wilson_lower_95",
                    "higher_is_stronger",
                    left.aggregate_valid_target_wilson_lower_95,
                    right.aggregate_valid_target_wilson_lower_95);
  add_double_higher("weakest_node_valid_target_wilson_lower_95",
                    "higher_is_stronger",
                    left.weakest_node_valid_target_wilson_lower_95,
                    right.weakest_node_valid_target_wilson_lower_95);
  add_double_lower("max_node_support_coefficient_of_variation",
                   left.max_node_support_coefficient_of_variation,
                   right.max_node_support_coefficient_of_variation);
  add_double_lower("max_node_support_gini", left.max_node_support_gini,
                   right.max_node_support_gini);
  add_visibility_count("source_key_audit_count", "visibility_only",
                       left.source_key_audit_count,
                       right.source_key_audit_count);
  add_count_lower("source_key_audit_issue_count",
                  left.source_key_audit_issue_count,
                  right.source_key_audit_issue_count);
  add_count_lower("source_key_affine_inconsistent_count",
                  left.source_key_affine_inconsistent_count,
                  right.source_key_affine_inconsistent_count);
  add_count_lower("blocking_deficit_count", left.blocking_deficit_count,
                  right.blocking_deficit_count);
  add_count_lower("unresolved_lineage_count", left.unresolved_lineage_count,
                  right.unresolved_lineage_count);
  add_visibility_count("warning_result_count", "visibility_only",
                       left.warning_result_count, right.warning_result_count);
  add_count_lower("triggered_warning_count", left.triggered_warning_count,
                  right.triggered_warning_count);
  add_count_lower("unavailable_warning_count", left.unavailable_warning_count,
                  right.unavailable_warning_count);
  add_visibility_count("warning_count", "alias_for_triggered_warning_count",
                       left.warning_count, right.warning_count);

  if (equal_comparison_dimension_count) {
    *equal_comparison_dimension_count = equal_count;
  }
  return rows;
}

[[nodiscard]] std::string evidence_dimension_comparison_rows_json(
    const std::vector<evidence_dimension_comparison_row_t> &rows) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"dimension\":" << json_quote(rows[i].dimension)
        << ",\"polarity\":" << json_quote(rows[i].polarity)
        << ",\"comparison_dimension\":"
        << bool_json(rows[i].comparison_dimension)
        << ",\"left_value\":" << rows[i].left_value_json
        << ",\"right_value\":" << rows[i].right_value_json
        << ",\"relation\":" << json_quote(rows[i].relation) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string evidence_dominance_reason_json(
    target::lattice_evidence_order_relation_t relation,
    const evidence_comparison_participation_t &left_participation,
    const evidence_comparison_participation_t &right_participation,
    const std::vector<std::string> &left_better_dimensions,
    const std::vector<std::string> &right_better_dimensions,
    std::int64_t equal_comparison_dimension_count) {
  std::vector<std::string> unavailable_reasons;
  unavailable_reasons.reserve(left_participation.exclusion_reasons.size() +
                              right_participation.exclusion_reasons.size());
  for (const auto &reason : left_participation.exclusion_reasons) {
    unavailable_reasons.push_back("left:" + reason);
  }
  for (const auto &reason : right_participation.exclusion_reasons) {
    unavailable_reasons.push_back("right:" + reason);
  }
  const bool selector_leakage_excluded =
      std::find(left_participation.exclusion_reasons.begin(),
                left_participation.exclusion_reasons.end(),
                "selection_signal_leakage_present") !=
          left_participation.exclusion_reasons.end() ||
      std::find(right_participation.exclusion_reasons.begin(),
                right_participation.exclusion_reasons.end(),
                "selection_signal_leakage_present") !=
          right_participation.exclusion_reasons.end();

  std::ostringstream out;
  out << "{\"basis\":\"pareto_partial_order_no_scalar_score\""
      << ",\"relation\":"
      << json_quote(target::lattice_evidence_order_relation_name(relation))
      << ",\"partial_order\":true"
      << ",\"scalar_score_emitted\":false"
      << ",\"model_selector\":false"
      << ",\"best_model_selector\":false"
      << ",\"checkpoint_selector\":false"
      << ",\"automatic_checkpoint_selection\":false"
      << ",\"performance_selector\":false"
      << ",\"pareto_optimizer\":false"
      << ",\"quality_acceptance\":false"
      << ",\"policy_gate\":false"
      << ",\"allocation_decision\":false"
      << ",\"market_readiness_decision\":false"
      << ",\"automatic_deployment_decision\":false"
      << ",\"deployment_decision\":false"
      << ",\"latest_satisfying_policy_separate\":true"
      << ",\"selector_leakage_excluded\":"
      << bool_json(selector_leakage_excluded) << ",\"incomparable\":"
      << bool_json(relation ==
                   target::lattice_evidence_order_relation_t::incomparable)
      << ",\"left_better_dimensions\":"
      << string_array_json(left_better_dimensions)
      << ",\"right_better_dimensions\":"
      << string_array_json(right_better_dimensions)
      << ",\"equal_comparison_dimension_count\":"
      << equal_comparison_dimension_count
      << ",\"unavailable_reasons\":" << string_array_json(unavailable_reasons)
      << "}";
  return out.str();
}

[[nodiscard]] bool handle_compare_evidence(const std::string &args,
                                           lattice_context_t *ctx,
                                           std::string *out, std::string *err) {
  std::string left_target_id;
  std::string right_target_id;
  if (!extract_json_string_field(args, "left_target_id", &left_target_id) ||
      trim_ascii(left_target_id).empty()) {
    if (err) {
      *err = "left_target_id is required";
    }
    return false;
  }
  if (!extract_json_string_field(args, "right_target_id", &right_target_id) ||
      trim_ascii(right_target_id).empty()) {
    if (err) {
      *err = "right_target_id is required";
    }
    return false;
  }
  left_target_id = trim_ascii(left_target_id);
  right_target_id = trim_ascii(right_target_id);

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

  target::lattice_target_evaluation_t left_eval;
  target::lattice_target_evaluation_t right_eval;
  try {
    left_eval = evaluator.evaluate(left_target_id);
    right_eval = evaluator.evaluate(right_target_id);
  } catch (const std::exception &ex) {
    if (err) {
      *err = ex.what();
    }
    return false;
  }

  const auto left_participation = evidence_comparison_participation(left_eval);
  const auto right_participation =
      evidence_comparison_participation(right_eval);
  auto relation = target::compare_lattice_evidence_order_vectors(
      left_eval.evidence_order_vector, right_eval.evidence_order_vector);
  if (!left_participation.participates || !right_participation.participates) {
    relation =
        (left_eval.evidence_order_vector.selection_signal_leakage_found ||
         right_eval.evidence_order_vector.selection_signal_leakage_found)
            ? target::lattice_evidence_order_relation_t::selector_leaked
            : target::lattice_evidence_order_relation_t::unavailable;
  }

  std::vector<std::string> left_better_dimensions;
  std::vector<std::string> right_better_dimensions;
  std::int64_t equal_comparison_dimension_count = 0;
  const auto dimension_rows = evidence_dimension_comparison_rows(
      left_eval.evidence_order_vector, right_eval.evidence_order_vector,
      &left_better_dimensions, &right_better_dimensions,
      &equal_comparison_dimension_count);

  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.evidence_comparison.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << ",\"db_source_of_truth\":false"
       << ",\"cache_rows_used_for_target_satisfaction\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"checkpoint_selector\":false"
       << ",\"automatic_checkpoint_selection\":false"
       << ",\"latest_satisfying_policy_separate\":true"
       << ",\"config_path\":" << json_quote(config_path.string())
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"active_identity\":" << active_identity_json(identity)
       << ",\"exposure_fact_count\":" << scan.ledger.facts().size()
       << ",\"node_exposure_fact_count\":" << scan.ledger.node_facts().size()
       << ",\"checkpoint_fact_count\":" << scan.ledger.checkpoint_facts().size()
       << ",\"source_receipt_fact_count\":"
       << scan.ledger.source_receipt_facts().size()
       << ",\"source_analytics_fact_count\":"
       << scan.ledger.source_analytics_facts().size()
       << ",\"target_transform_fact_count\":"
       << scan.ledger.target_transform_facts().size()
       << ",\"forecast_baseline_fact_count\":"
       << scan.ledger.forecast_baseline_facts().size()
       << ",\"forecast_eval_fact_count\":"
       << scan.ledger.forecast_eval_facts().size()
       << ",\"observer_belief_fact_count\":"
       << scan.ledger.observer_belief_facts().size()
       << ",\"allocation_engine_fact_count\":"
       << scan.ledger.allocation_engine_facts().size()
       << ",\"replay_environment_fact_count\":"
       << scan.ledger.replay_environment_facts().size()
       << ",\"policy_training_fact_count\":"
       << scan.ledger.policy_training_facts().size()
       << ",\"selection_signal_fact_count\":"
       << scan.ledger.selection_signal_facts().size()
       << ",\"representation_support_fact_count\":"
       << scan.ledger.representation_support_facts().size()
       << ",\"warnings\":" << string_array_json(scan.warnings) << ",\"left\":"
       << evidence_comparison_side_json("left", left_eval, left_participation)
       << ",\"right\":"
       << evidence_comparison_side_json("right", right_eval,
                                        right_participation)
       << ",\"relation\":"
       << json_quote(target::lattice_evidence_order_relation_name(relation))
       << ",\"relation_vocabulary\":"
       << evidence_order_relation_vocabulary_json()
       << ",\"dimension_vocabulary\":"
       << evidence_order_dimension_vocabulary_json()
       << ",\"dimension_comparisons\":"
       << evidence_dimension_comparison_rows_json(dimension_rows)
       << ",\"dominance_reason\":"
       << evidence_dominance_reason_json(
              relation, left_participation, right_participation,
              left_better_dimensions, right_better_dimensions,
              equal_comparison_dimension_count)
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] std::string derived_query_common_prefix_json(
    const std::string &relation, const std::string &canonical_relation,
    const target::lattice_derived_query_rule_entry_t &rule) {
  std::ostringstream out;
  out << "{\"schema\":\"kikijyeba.lattice.derived_query.v1\""
      << ",\"schema_version\":1"
      << ",\"relation\":" << json_quote(relation)
      << ",\"canonical_relation\":" << json_quote(canonical_relation)
      << ",\"rule_version\":1"
      << ",\"rule_vocabulary_digest_schema\":"
      << json_quote(kDerivedQueryRuleVocabularyDigestSchema)
      << ",\"rule_vocabulary_digest\":"
      << json_quote(derived_query_rule_vocabulary_digest())
      << ",\"rule\":" << derived_query_rule_entry_json(rule)
      << ",\"read_only\":true"
      << ",\"target_proof\":false"
      << ",\"dispatchable\":false"
      << ",\"runtime_executor\":false"
      << ",\"fact_families_are_not_target_kinds\":true"
      << ",\"db_source_of_truth\":false"
      << ",\"writes_evidence\":false"
      << ",\"cache_rows_used_for_target_satisfaction\":false"
      << "," << lattice_no_decision_authority_json()
      << ",\"checkpoint_selector\":false"
      << ",\"automatic_checkpoint_selection\":false";
  return out.str();
}

[[nodiscard]] std::string
target_satisfied_witness_json(const target::lattice_target_evaluation_t &eval) {
  std::ostringstream out;
  out << "{\"witness_type\":\"target_evaluation\""
      << ",\"target_id\":" << json_quote(eval.target_id) << ",\"status\":"
      << json_quote(target::lattice_target_status_name(eval.status))
      << ",\"target_spec_fingerprint\":"
      << json_quote(eval.proof_certificate.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << json_quote(eval.proof_certificate.split_policy_fingerprint)
      << ",\"certificate_ref\":"
      << json_quote(display::certificate_ref(
             eval.proof_certificate.certificate_digest))
      << ",\"certificate_digest\":"
      << json_quote(eval.proof_certificate.certificate_digest)
      << ",\"proof_certificate_check_passed\":"
      << bool_json(eval.proof_certificate_check.passed)
      << ",\"deficit_count\":" << eval.deficits.size() << "}";
  return out.str();
}

[[nodiscard]] std::string unresolved_lineage_witnesses_json(
    const std::vector<fs::path> &unresolved_input_checkpoints,
    const std::vector<std::string> &identity_mismatches, std::size_t limit) {
  std::ostringstream out;
  out << "[";
  std::size_t emitted = 0;
  for (const auto &checkpoint : unresolved_input_checkpoints) {
    if (emitted >= limit) {
      break;
    }
    if (emitted != 0) {
      out << ",";
    }
    out << "{\"witness_type\":\"unresolved_input_checkpoint\""
        << ",\"checkpoint_path\":"
        << json_quote(checkpoint.lexically_normal().string()) << "}";
    ++emitted;
  }
  for (const auto &mismatch : identity_mismatches) {
    if (emitted >= limit) {
      break;
    }
    if (emitted != 0) {
      out << ",";
    }
    out << "{\"witness_type\":\"checkpoint_identity_mismatch\""
        << ",\"message\":" << json_quote(mismatch) << "}";
    ++emitted;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
cache_validation_issue_witnesses_json(const std::vector<std::string> &issues,
                                      std::size_t limit) {
  std::ostringstream out;
  out << "[";
  const std::size_t n = std::min(issues.size(), limit);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"witness_type\":\"runtime_index_cache_validation_issue\""
        << ",\"issue\":" << json_quote(issues[i]) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string checkpoint_path_key(const fs::path &path) {
  return normalize_path(path).lexically_normal().string();
}

[[nodiscard]] const exposure::lattice_checkpoint_fact_t *
find_checkpoint_fact_for_path(
    const std::vector<exposure::lattice_checkpoint_fact_t> &checkpoint_facts,
    const fs::path &path) {
  const std::string key = checkpoint_path_key(path);
  const auto it = std::find_if(
      checkpoint_facts.begin(), checkpoint_facts.end(), [&](const auto &fact) {
        return checkpoint_path_key(fact.checkpoint_path) == key;
      });
  if (it == checkpoint_facts.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] std::string checkpoint_ancestor_witnesses_json(
    const std::vector<exposure::lattice_exposure_fact_t> &facts,
    const std::vector<exposure::lattice_checkpoint_fact_t> &checkpoint_facts,
    const fs::path &ancestor_checkpoint_path,
    const std::string &ancestor_checkpoint_id, bool has_ancestor_path,
    bool has_ancestor_id, std::size_t limit, std::size_t *match_count_out) {
  std::ostringstream out;
  out << "[";
  std::size_t matches = 0;
  std::size_t emitted = 0;
  const std::string requested_path_key =
      has_ancestor_path ? checkpoint_path_key(ancestor_checkpoint_path)
                        : std::string{};
  for (const auto &fact : facts) {
    const auto *checkpoint_fact =
        find_checkpoint_fact_for_path(checkpoint_facts, fact.output_checkpoint);
    const std::string fact_path_key =
        checkpoint_path_key(fact.output_checkpoint);
    const bool path_matches =
        !has_ancestor_path || fact_path_key == requested_path_key;
    const bool id_matches =
        !has_ancestor_id ||
        (checkpoint_fact != nullptr &&
         checkpoint_fact->checkpoint_id == ancestor_checkpoint_id);
    if (!path_matches || !id_matches) {
      continue;
    }
    ++matches;
    if (emitted >= limit) {
      continue;
    }
    if (emitted != 0) {
      out << ",";
    }
    out << "{\"witness_type\":\"checkpoint_ancestor\""
        << ",\"fact_digest\":"
        << json_quote(exposure::exposure_fact_digest(fact))
        << ",\"job_id\":" << json_quote(fact.job_id)
        << ",\"wave_id\":" << json_quote(fact.wave_id)
        << ",\"target_component_family_id\":"
        << json_quote(fact.target_component_family_id)
        << ",\"output_checkpoint\":"
        << json_quote(fact.output_checkpoint.lexically_normal().string())
        << ",\"checkpoint_id\":"
        << json_quote(checkpoint_fact != nullptr
                          ? checkpoint_fact->checkpoint_id
                          : std::string{})
        << ",\"checkpoint_file_digest\":"
        << json_quote(checkpoint_fact != nullptr
                          ? checkpoint_fact->checkpoint_file_digest
                          : std::string{})
        << ",\"fact\":" << fact_json(fact) << "}";
    ++emitted;
  }
  out << "]";
  if (match_count_out) {
    *match_count_out = matches;
  }
  return out.str();
}

[[nodiscard]] bool handle_derived_query(const std::string &args,
                                        lattice_context_t *ctx,
                                        std::string *out, std::string *err) {
  std::string requested_relation;
  if (!parse_optional_string_arg(args, "relation", {}, &requested_relation,
                                 err)) {
    return false;
  }
  requested_relation = trim_ascii(requested_relation);
  const std::string relation =
      canonical_derived_query_relation(requested_relation);
  if (relation.empty()) {
    if (err) {
      *err = "relation is required";
    }
    return false;
  }
  const auto *rule = find_derived_query_rule_entry(relation);
  if (rule == nullptr) {
    if (err) {
      *err = "unknown derived relation: " + relation;
    }
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
  const std::size_t witness_limit = static_cast<std::size_t>(limit);
  std::string target_id;
  (void)extract_json_string_field(args, "target_id", &target_id);
  target_id = trim_ascii(target_id);

  if (relation == "target_satisfied" || relation == "forbidden_overlap" ||
      (relation == "unresolved_lineage" && !target_id.empty())) {
    target::lattice_target_evaluation_t eval;
    fs::path config_path;
    fs::path runtime_root;
    target::lattice_target_active_identity_t identity;
    exposure::exposure_ledger_scan_result_t scan;
    if (!evaluate_target_for_derived_query(args, ctx, target_id, &eval,
                                           &config_path, &runtime_root,
                                           &identity, &scan, err)) {
      return false;
    }
    if (relation == "target_satisfied") {
      const bool value = target_satisfied_relation_value(eval);
      std::ostringstream json;
      json << derived_query_common_prefix_json(requested_relation, relation,
                                               *rule)
           << ",\"result_source\":\"target_evaluation\""
           << ",\"config_path\":" << json_quote(config_path.string())
           << ",\"runtime_root\":" << json_quote(runtime_root.string())
           << ",\"active_identity\":" << active_identity_json(identity)
           << ",\"target_id\":" << json_quote(target_id) << ",\"known\":true"
           << ",\"value\":" << bool_json(value)
           << ",\"fail_closed\":" << bool_json(!value)
           << ",\"unsafe_or_incomplete_join_failed_closed\":false"
           << ",\"missing_witness_rejected\":false"
           << ",\"witness_count\":1"
           << ",\"returned_witness_count\":1"
           << ",\"witnesses\":[" << target_satisfied_witness_json(eval) << "]}";
      *out = json.str();
      return true;
    }
    if (relation == "forbidden_overlap") {
      const bool known = eval.proof_certificate.leakage.checked;
      const bool value = known && eval.proof_certificate.leakage.overlap_found;
      const auto witness_count =
          eval.proof_certificate.leakage.overlap_witnesses.size();
      std::ostringstream json;
      json << derived_query_common_prefix_json(requested_relation, relation,
                                               *rule)
           << ",\"result_source\":\"target_evaluation.leakage_proof\""
           << ",\"config_path\":" << json_quote(config_path.string())
           << ",\"runtime_root\":" << json_quote(runtime_root.string())
           << ",\"active_identity\":" << active_identity_json(identity)
           << ",\"target_id\":" << json_quote(target_id)
           << ",\"known\":" << bool_json(known)
           << ",\"value\":" << bool_json(value)
           << ",\"fail_closed\":" << bool_json(!known)
           << ",\"unsafe_or_incomplete_join_failed_closed\":"
           << bool_json(!known) << ",\"missing_witness_rejected\":false"
           << ",\"witness_count\":" << witness_count
           << ",\"returned_witness_count\":"
           << std::min(witness_count, witness_limit) << ",\"protected_split\":"
           << json_quote(eval.proof_certificate.leakage.split_name)
           << ",\"witnesses\":"
           << leakage_overlap_witnesses_json(
                  std::vector<exposure::forbidden_exposure_overlap_t>(
                      eval.proof_certificate.leakage.overlap_witnesses.begin(),
                      eval.proof_certificate.leakage.overlap_witnesses.begin() +
                          static_cast<std::ptrdiff_t>(
                              std::min(witness_count, witness_limit))))
           << "}";
      *out = json.str();
      return true;
    }
    const bool known = eval.proof_certificate.closure.checked;
    const auto witness_count =
        eval.proof_certificate.closure.unresolved_input_checkpoints.size() +
        eval.proof_certificate.closure.identity_mismatches.size();
    const bool value = known && witness_count > 0;
    std::ostringstream json;
    json << derived_query_common_prefix_json(requested_relation, relation,
                                             *rule)
         << ",\"result_source\":\"target_evaluation.closure_proof\""
         << ",\"config_path\":" << json_quote(config_path.string())
         << ",\"runtime_root\":" << json_quote(runtime_root.string())
         << ",\"active_identity\":" << active_identity_json(identity)
         << ",\"target_id\":" << json_quote(target_id)
         << ",\"known\":" << bool_json(known)
         << ",\"value\":" << bool_json(value)
         << ",\"fail_closed\":" << bool_json(!known || value)
         << ",\"unsafe_or_incomplete_join_failed_closed\":"
         << bool_json(!known || value) << ",\"missing_witness_rejected\":false"
         << ",\"witness_count\":" << witness_count
         << ",\"returned_witness_count\":"
         << std::min(witness_count, witness_limit) << ",\"witnesses\":"
         << unresolved_lineage_witnesses_json(
                eval.proof_certificate.closure.unresolved_input_checkpoints,
                eval.proof_certificate.closure.identity_mismatches,
                witness_limit)
         << "}";
    *out = json.str();
    return true;
  }

  if (relation == "stale_cache") {
    fs::path runtime_root;
    if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
      return false;
    }
    fs::path index_path;
    if (!resolve_runtime_index_path_arg(args, ctx, runtime_root, &index_path,
                                        err)) {
      return false;
    }
    exposure::lattice_runtime_index_validation_strength_t validation_strength =
        exposure::lattice_runtime_index_validation_strength_t::
            watched_file_manifest;
    if (!parse_runtime_index_validation_strength_arg(
            args,
            exposure::lattice_runtime_index_validation_strength_t::
                watched_file_manifest,
            &validation_strength, err)) {
      return false;
    }
    exposure::lattice_runtime_index_cache_t cached{};
    std::error_code ec;
    const bool cache_file_exists = fs::exists(index_path, ec);
    if (cache_file_exists) {
      cached = exposure::read_runtime_index_cache(index_path);
    } else {
      cached.schema.clear();
    }
    const auto validation = exposure::validate_runtime_index_cache(
        cached, runtime_root, validation_strength);
    const bool value = !validation.cache_valid;
    std::ostringstream json;
    json << derived_query_common_prefix_json(requested_relation, relation,
                                             *rule)
         << ",\"result_source\":\"runtime_index_cache_validation\""
         << ",\"runtime_root\":" << json_quote(runtime_root.string())
         << ",\"index_path\":" << json_quote(index_path.string())
         << ",\"cache_file_exists\":" << bool_json(cache_file_exists)
         << ",\"validation_strength\":"
         << json_quote(exposure::runtime_index_validation_strength_name(
                validation_strength))
         << ",\"known\":true"
         << ",\"value\":" << bool_json(value)
         << ",\"fail_closed\":" << bool_json(value)
         << ",\"unsafe_or_incomplete_join_failed_closed\":" << bool_json(value)
         << ",\"missing_witness_rejected\":false"
         << ",\"cache_validation\":"
         << runtime_index_cache_validation_json(validation)
         << ",\"witness_count\":" << validation.issues.size()
         << ",\"returned_witness_count\":"
         << std::min(validation.issues.size(), witness_limit)
         << ",\"witnesses\":"
         << cache_validation_issue_witnesses_json(validation.issues,
                                                  witness_limit)
         << "}";
    *out = json.str();
    return true;
  }

  if (relation == "checkpoint_ancestor" || relation == "unresolved_lineage") {
    fs::path runtime_root;
    if (!resolve_runtime_root_arg(args, ctx, &runtime_root, err)) {
      return false;
    }
    std::string checkpoint_arg;
    std::string checkpoint_id;
    std::string checkpoint_file_digest;
    const bool has_checkpoint_path =
        extract_json_string_field(args, "checkpoint_path", &checkpoint_arg) &&
        !trim_ascii(checkpoint_arg).empty();
    const bool has_checkpoint_identity =
        extract_json_string_field(args, "checkpoint_id", &checkpoint_id) &&
        !trim_ascii(checkpoint_id).empty() &&
        extract_json_string_field(args, "checkpoint_file_digest",
                                  &checkpoint_file_digest) &&
        !trim_ascii(checkpoint_file_digest).empty();
    if (!has_checkpoint_path && !has_checkpoint_identity) {
      if (err) {
        *err = "checkpoint_path or checkpoint_id plus checkpoint_file_digest "
               "is required for this derived relation";
      }
      return false;
    }
    fs::path checkpoint_path;
    if (has_checkpoint_path) {
      checkpoint_path = fs::path(checkpoint_arg);
      if (checkpoint_path.is_relative()) {
        checkpoint_path = runtime_root / checkpoint_path;
      }
      checkpoint_path = normalize_path(checkpoint_path);
      if (!runtime_root_allowed(ctx->policy, checkpoint_path)) {
        if (err) {
          *err = "E_LATTICE_CHECKPOINT_DENIED: checkpoint_path is outside "
                 "allowed runtime roots: " +
                 checkpoint_path.string();
        }
        return false;
      }
    }
    const auto &scan = session_scan_for_runtime_root(runtime_root);
    const auto closure =
        has_checkpoint_path
            ? scan.ledger.checkpoint_closure_result(checkpoint_path)
            : scan.ledger.checkpoint_closure_result_for_identity(
                  checkpoint_id, checkpoint_file_digest);
    if (relation == "unresolved_lineage") {
      const auto witness_count = closure.unresolved_input_checkpoints.size() +
                                 closure.identity_mismatches.size();
      const bool value = witness_count > 0;
      std::ostringstream json;
      json << derived_query_common_prefix_json(requested_relation, relation,
                                               *rule)
           << ",\"result_source\":\"checkpoint_closure\""
           << ",\"runtime_root\":" << json_quote(runtime_root.string())
           << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
           << ",\"checkpoint_id\":" << json_quote(checkpoint_id)
           << ",\"checkpoint_file_digest\":"
           << json_quote(checkpoint_file_digest) << ",\"checkpoint_ref\":"
           << json_quote(display::checkpoint_ref(checkpoint_file_digest))
           << ",\"known\":true"
           << ",\"value\":" << bool_json(value)
           << ",\"fail_closed\":" << bool_json(value)
           << ",\"unsafe_or_incomplete_join_failed_closed\":"
           << bool_json(value) << ",\"missing_witness_rejected\":false"
           << ",\"closure_complete\":" << bool_json(closure.complete())
           << ",\"witness_count\":" << witness_count
           << ",\"returned_witness_count\":"
           << std::min(witness_count, witness_limit) << ",\"witnesses\":"
           << unresolved_lineage_witnesses_json(
                  closure.unresolved_input_checkpoints,
                  closure.identity_mismatches, witness_limit)
           << "}";
      *out = json.str();
      return true;
    }

    std::string ancestor_checkpoint_arg;
    std::string ancestor_checkpoint_id;
    const bool has_ancestor_path =
        extract_json_string_field(args, "ancestor_checkpoint_path",
                                  &ancestor_checkpoint_arg) &&
        !trim_ascii(ancestor_checkpoint_arg).empty();
    const bool has_ancestor_id =
        extract_json_string_field(args, "ancestor_checkpoint_id",
                                  &ancestor_checkpoint_id) &&
        !trim_ascii(ancestor_checkpoint_id).empty();
    fs::path ancestor_checkpoint_path;
    if (has_ancestor_path) {
      ancestor_checkpoint_path = fs::path(ancestor_checkpoint_arg);
      if (ancestor_checkpoint_path.is_relative()) {
        ancestor_checkpoint_path = runtime_root / ancestor_checkpoint_path;
      }
      ancestor_checkpoint_path = normalize_path(ancestor_checkpoint_path);
      if (!runtime_root_allowed(ctx->policy, ancestor_checkpoint_path)) {
        if (err) {
          *err = "E_LATTICE_CHECKPOINT_DENIED: ancestor_checkpoint_path is "
                 "outside allowed runtime roots: " +
                 ancestor_checkpoint_path.string();
        }
        return false;
      }
    }
    std::size_t match_count = 0;
    const std::string witnesses = checkpoint_ancestor_witnesses_json(
        closure.facts, scan.ledger.checkpoint_facts(), ancestor_checkpoint_path,
        ancestor_checkpoint_id, has_ancestor_path, has_ancestor_id,
        witness_limit, &match_count);
    const bool filtered = has_ancestor_path || has_ancestor_id;
    const bool missing_witness_rejected = filtered && match_count == 0;
    const bool complete = closure.complete();
    const bool value = complete && match_count > 0;
    std::ostringstream json;
    json << derived_query_common_prefix_json(requested_relation, relation,
                                             *rule)
         << ",\"result_source\":\"checkpoint_closure\""
         << ",\"runtime_root\":" << json_quote(runtime_root.string())
         << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
         << ",\"checkpoint_id\":" << json_quote(checkpoint_id)
         << ",\"checkpoint_file_digest\":" << json_quote(checkpoint_file_digest)
         << ",\"checkpoint_ref\":"
         << json_quote(display::checkpoint_ref(checkpoint_file_digest))
         << ",\"ancestor_checkpoint_path\":"
         << json_quote(ancestor_checkpoint_path.string())
         << ",\"ancestor_checkpoint_id\":" << json_quote(ancestor_checkpoint_id)
         << ",\"known\":true"
         << ",\"value\":" << bool_json(value) << ",\"fail_closed\":"
         << bool_json(!complete || missing_witness_rejected)
         << ",\"unsafe_or_incomplete_join_failed_closed\":"
         << bool_json(!complete) << ",\"missing_witness_rejected\":"
         << bool_json(missing_witness_rejected)
         << ",\"closure_complete\":" << bool_json(complete)
         << ",\"closure_fact_count\":" << closure.facts.size()
         << ",\"witness_count\":" << match_count
         << ",\"returned_witness_count\":"
         << std::min(match_count, witness_limit)
         << ",\"unresolved_input_checkpoints\":"
         << path_array_json(closure.unresolved_input_checkpoints)
         << ",\"identity_mismatches\":"
         << string_array_json(closure.identity_mismatches)
         << ",\"witnesses\":" << witnesses << "}";
    *out = json.str();
    return true;
  }

  if (err) {
    *err = "derived relation is vocabulary-only in this pilot and has no "
           "standalone query handler: " +
           relation;
  }
  return false;
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
  std::string checkpoint_id;
  std::string checkpoint_file_digest;
  const bool has_checkpoint_path =
      extract_json_string_field(args, "checkpoint_path", &checkpoint_arg) &&
      !trim_ascii(checkpoint_arg).empty();
  const bool has_checkpoint_identity =
      extract_json_string_field(args, "checkpoint_id", &checkpoint_id) &&
      !trim_ascii(checkpoint_id).empty() &&
      extract_json_string_field(args, "checkpoint_file_digest",
                                &checkpoint_file_digest) &&
      !trim_ascii(checkpoint_file_digest).empty();
  if (!has_checkpoint_path && !has_checkpoint_identity) {
    if (err) {
      *err = "checkpoint_path or checkpoint_id plus checkpoint_file_digest is "
             "required";
    }
    return false;
  }
  fs::path checkpoint_path;
  if (has_checkpoint_path) {
    checkpoint_path = fs::path(checkpoint_arg);
    if (checkpoint_path.is_relative()) {
      checkpoint_path = runtime_root / checkpoint_path;
    }
    checkpoint_path = normalize_path(checkpoint_path);
    if (!runtime_root_allowed(ctx->policy, checkpoint_path)) {
      if (err) {
        *err =
            "E_LATTICE_CHECKPOINT_DENIED: checkpoint_path is outside allowed "
            "runtime roots: " +
            checkpoint_path.string();
      }
      return false;
    }
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
  const auto scan = session_scan_for_runtime_root(runtime_root);
  const auto closure =
      has_checkpoint_path
          ? scan.ledger.checkpoint_closure_result(checkpoint_path)
          : scan.ledger.checkpoint_closure_result_for_identity(
                checkpoint_id, checkpoint_file_digest);
  std::ostringstream json;
  json << "{\"schema\":\"kikijyeba.lattice.checkpoint_closure.v1\""
       << ",\"schema_version\":1"
       << ",\"read_only\":true"
       << ",\"target_proof\":false"
       << ",\"dispatchable\":false"
       << ",\"runtime_executor\":false"
       << ",\"fact_families_are_not_target_kinds\":true"
       << ",\"writes_evidence\":false"
       << "," << lattice_no_decision_authority_json()
       << ",\"checkpoint_selector\":false"
       << ",\"automatic_checkpoint_selection\":false"
       << ",\"runtime_root\":" << json_quote(runtime_root.string())
       << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
       << ",\"checkpoint_id\":" << json_quote(checkpoint_id)
       << ",\"checkpoint_file_digest\":" << json_quote(checkpoint_file_digest)
       << ",\"checkpoint_ref\":"
       << json_quote(display::checkpoint_ref(checkpoint_file_digest))
       << ",\"complete\":" << bool_json(closure.complete())
       << ",\"fact_count\":" << closure.facts.size()
       << ",\"resolution_authority\":"
       << json_quote(closure.resolution_authority)
       << ",\"root_checkpoint_id\":" << json_quote(closure.root_checkpoint_id)
       << ",\"root_checkpoint_file_digest\":"
       << json_quote(closure.root_checkpoint_file_digest)
       << ",\"root_checkpoint_ref\":"
       << json_quote(
              display::checkpoint_ref(closure.root_checkpoint_file_digest))
       << ",\"identity_mismatches\":"
       << string_array_json(closure.identity_mismatches)
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

[[nodiscard]] bool handle_inspect(const std::string &args,
                                  lattice_context_t *ctx, std::string *out,
                                  std::string *err) {
  std::string subject;
  if (!extract_json_string_field(args, "subject", &subject) ||
      trim_ascii(subject).empty()) {
    if (err) {
      *err = "hero.lattice.inspect requires subject";
    }
    return false;
  }
  subject = lowercase_ascii(trim_ascii(subject));

  std::string mode;
  (void)extract_json_string_field(args, "mode", &mode);
  mode = lowercase_ascii(trim_ascii(mode));

  if (subject == "schema") {
    return handle_schema(args, ctx, out, err);
  }
  if (subject == "targets") {
    return handle_list_targets(args, ctx, out, err);
  }
  if (subject == "target") {
    if (!mode.empty() && mode != "explain") {
      if (err) {
        *err = "hero.lattice.inspect subject=target mode must be explain";
      }
      return false;
    }
    return handle_explain_target(args, ctx, out, err);
  }
  if (subject == "exposure") {
    return handle_scan_exposure(args, ctx, out, err);
  }
  if (subject == "fact_families") {
    return handle_list_fact_families(args, ctx, out, err);
  }
  if (subject == "facts") {
    if (mode.empty() || mode == "summary") {
      return handle_fact_summary(args, ctx, out, err);
    }
    if (mode == "scan") {
      return handle_scan_facts(args, ctx, out, err);
    }
    if (mode == "lineage") {
      return handle_fact_lineage(args, ctx, out, err);
    }
    if (mode == "preview") {
      return handle_fact_preview(args, ctx, out, err);
    }
    if (err) {
      *err = "hero.lattice.inspect subject=facts mode must be scan, summary, "
             "lineage, or preview";
    }
    return false;
  }
  if (subject == "index") {
    if (mode.empty() || mode == "status") {
      return handle_index_status(args, ctx, out, err);
    }
    if (mode == "query") {
      return handle_index_query(args, ctx, out, err);
    }
    if (err) {
      *err = "hero.lattice.inspect subject=index mode must be status or query";
    }
    return false;
  }
  if (subject == "derived") {
    return handle_derived_query(args, ctx, out, err);
  }
  if (subject == "checkpoint") {
    if (!mode.empty() && mode != "closure") {
      if (err) {
        *err = "hero.lattice.inspect subject=checkpoint mode must be closure";
      }
      return false;
    }
    return handle_checkpoint_closure(args, ctx, out, err);
  }
  if (err) {
    *err = "hero.lattice.inspect subject must be one of schema, targets, "
           "target, exposure, fact_families, facts, index, derived, or "
           "checkpoint";
  }
  return false;
}

[[nodiscard]] bool handle_evaluate(const std::string &args,
                                   lattice_context_t *ctx, std::string *out,
                                   std::string *err) {
  std::string operation;
  if (!extract_json_string_field(args, "operation", &operation) ||
      trim_ascii(operation).empty()) {
    if (err) {
      *err = "hero.lattice.evaluate requires operation";
    }
    return false;
  }
  operation = lowercase_ascii(trim_ascii(operation));
  if (operation == "target") {
    return handle_evaluate_target(args, ctx, out, err);
  }
  if (operation == "targets") {
    return handle_evaluate_targets(args, ctx, out, err);
  }
  if (operation == "deficit") {
    return handle_target_deficit(args, ctx, out, err);
  }
  if (operation == "latest_satisfying_checkpoint") {
    return handle_latest_satisfying_checkpoint(args, ctx, out, err);
  }
  if (err) {
    *err = "hero.lattice.evaluate operation must be target, targets, deficit, "
           "or latest_satisfying_checkpoint";
  }
  return false;
}

[[nodiscard]] bool handle_compare(const std::string &args,
                                  lattice_context_t *ctx, std::string *out,
                                  std::string *err) {
  return handle_compare_evidence(args, ctx, out, err);
}

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.lattice.status") {
    return handle_status;
  }
  if (name == "hero.lattice.inspect") {
    return handle_inspect;
  }
  if (name == "hero.lattice.evaluate") {
    return handle_evaluate;
  }
  if (name == "hero.lattice.compare") {
    return handle_compare;
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
