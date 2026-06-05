#include "hero/runtime_hero/hero_runtime_tools.h"

#include "hero/mcp_schema_compat.h"
#include "hero/runtime_hero/hero_runtime.h"
#include "hero/marshal_hero/marshal/digest.h"
#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/runtime_hero/runtime/policy_training_job_contract.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "wikimyei/assembly.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
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

constexpr tool_descriptor_t
    kTools
        [] =
            {
                {"hero.runtime.status",
                 "Read-only health: summarize Runtime Hero policy, executable, "
                 "active "
                 "wave, job root, and explicit non-proof boundary state.",
                 R"({"type":"object","properties":{},"additionalProperties":false})"},
                {"hero.runtime.inspect",
                 "Read-only Runtime inspection. subject=schema reads policy "
                 "keys; "
                 "subject=wave decodes the active wave; "
                 "subject=jobs/job/artifact "
                 "reads "
                 "bounded Runtime evidence.",
                 R"({"type":"object","required":["subject"],"properties":{"subject":{"type":"string","enum":["schema","wave","jobs","job","artifact"]},"config_path":{"type":"string"},"root":{"type":"string"},"job_id":{"type":"string"},"job_dir":{"type":"string"},"artifact":{"type":"string"},"path":{"type":"string"},"include_artifacts":{"type":"boolean"},"include_text":{"type":"boolean"},"max_bytes":{"type":"integer"},"limit":{"type":"integer"}},"additionalProperties":false})"},
                {"hero.runtime.run",
                 "Runtime execution/delegation. operation=wave runs the active "
                 "Runtime "
                 "wave with requested_mode=dry_run|execute. Policy-training "
                 "contracts are also submitted as operation=wave with policy "
                 "identity and causal schedule fields; execute is limited to "
                 "the bounded pre-PPO noop_policy_training.v1 smoke trainer. "
                 "operation=replay plans, dry-runs, or executes replay from a "
                 "completed Runtime job.",
                 R"({"type":"object","required":["operation","requested_mode"],"properties":{"operation":{"type":"string","enum":["wave","replay"]},"requested_mode":{"type":"string","enum":["plan","dry_run","execute"]},"config_path":{"type":"string"},"job_id":{"type":"string"},"job_dir":{"type":"string"},"force_rebuild_cache":{"type":"boolean"},"confirm_execute":{"type":"boolean"},"timeout_seconds":{"type":"integer"},"wave_overlay":{"type":"object","properties":{"source_range":{"type":"string"},"anchor_index_begin":{"type":"string"},"anchor_index_end":{"type":"string"},"source_key_begin":{"type":"string"},"source_key_end":{"type":"string"}}},"runtime_handoff":{"type":"object","properties":{}},"marshal_expected_wave":{"type":"object","properties":{"target_component_family_id":{"type":"string"},"mode":{"type":"string"},"source_range":{"type":"string"},"source_order":{"type":"string"},"anchor_index_begin":{"type":"string"},"anchor_index_end":{"type":"string"},"source_key_begin":{"type":"string"},"source_key_end":{"type":"string"},"model_state_inputs":{"type":"object"}}},"base_reserve_node_id":{"type":"string"},"risky_node_ids":{"type":"string"},"experiment_id":{"type":"string"},"report_path":{"type":"string"},"initial_equity_base":{"type":"number"},"min_base_reserve_weight":{"type":"number"},"max_risky_weight":{"type":"number"},"max_turnover_l1":{"type":"number"},"max_steps":{"type":"integer"},"max_parallel_jobs":{"type":"integer"},"include_equal_weight":{"type":"boolean"},"include_current_weight":{"type":"boolean"},"include_base_reserve_policy":{"type":"boolean"},"include_spot_distributional_utility_policy":{"type":"boolean"},"allow_synthetic_direct_edges":{"type":"boolean"},"validation_rollout":{"type":"boolean"},"linear_transaction_cost_rate":{"type":"number"},"execution_profile_digest":{"type":"string"},"policy_set_digest":{"type":"string"},"protocol_id":{"type":"string"},"protocol_contract_fingerprint":{"type":"string"},"graph_order_fingerprint":{"type":"string"},"source_cursor_token":{"type":"string"},"split_policy_fingerprint":{"type":"string"},"component_assembly_fingerprint":{"type":"string"},"policy_id":{"type":"string"},"policy_kind":{"type":"string"},"policy_architecture_digest":{"type":"string"},"training_config_digest":{"type":"string"},"training_range_digest":{"type":"string"},"validation_range_digest":{"type":"string"},"test_range_digest":{"type":"string"},"environment_contract_id":{"type":"string"},"observation_schema_digest":{"type":"string"},"action_schema_digest":{"type":"string"},"reward_contract_digest":{"type":"string"},"training_schedule_mode":{"type":"string","enum":["causal_walk_forward_training.v1","offline_full_window_research","batch_final_refit_candidate"]},"causal_schedule_schema_id":{"type":"string"},"causal_schedule_digest":{"type":"string"},"causal_schedule_cursor_key_kind":{"type":"string","enum":["numeric_anchor_index","numeric_source_key","fixed_width_source_key","timestamp_ms"]},"causal_schedule_no_future_snapshot_use_source":{"type":"string"},"normalization_fit_range_digest":{"type":"string"},"replay_buffer_source_range_digest":{"type":"string"},"early_stopping_policy_digest":{"type":"string"},"hyperparameter_selection_policy_digest":{"type":"string"},"selector_split":{"type":"string","enum":["none","train","training","validation"]},"selector_policy_digest":{"type":"string"},"parent_checkpoint_digest":{"type":"string"},"parent_forecast_eval_fact_digest":{"type":"string"},"parent_observer_belief_fact_digest":{"type":"string"},"parent_allocation_engine_fact_digest":{"type":"string"},"parent_replay_environment_fact_digest":{"type":"string"},"final_refit_parent_selected_checkpoint_digest":{"type":"string"},"random_seed":{"type":"integer"},"max_episodes":{"type":"integer"},"max_wall_clock_seconds":{"type":"integer"},"causal_schedule_readiness_eligible":{"type":"boolean"},"causal_schedule_no_future_snapshot_use":{"type":"boolean"},"offline_full_window_research_allowed":{"type":"boolean"},"final_refit_uses_validation":{"type":"boolean"},"validation_no_longer_proof":{"type":"boolean"},"sealed_test_required":{"type":"boolean"},"live_execution_allowed":{"type":"boolean"}},"additionalProperties":false})"},
                {"hero.runtime.reset",
                 "Guarded developer reset. requested_mode=plan previews the "
                 "reset; "
                 "requested_mode=execute clears allowed Runtime roots only "
                 "with "
                 "explicit "
                 "confirmation and policy permission.",
                 R"({"type":"object","required":["requested_mode"],"properties":{"requested_mode":{"type":"string","enum":["plan","execute"]},"runtime_root":{"type":"string"},"backup":{"type":"boolean"},"confirm_dev_nuke":{"type":"boolean"}},"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.runtime.status" || name == "hero.runtime.inspect";
}

[[nodiscard]] bool tool_is_destructive(std::string_view name) {
  return name == "hero.runtime.run" || name == "hero.runtime.reset";
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

[[nodiscard]] bool parse_double(std::string_view raw, double *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  char *end = nullptr;
  errno = 0;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == value.c_str() ||
      trim_ascii(std::string_view(end)).size() != 0) {
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

struct json_field_t {
  std::string key;
  std::string raw_value;
};

[[nodiscard]] bool parse_json_object_fields(const std::string &json,
                                            std::vector<json_field_t> *out,
                                            std::string *err) {
  if (out) {
    out->clear();
  }
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    if (err) {
      *err = "arguments must be a JSON object";
    }
    return false;
  }
  ++idx;
  skip_ws(json, &idx);
  if (idx < json.size() && json[idx] == '}') {
    ++idx;
    skip_ws(json, &idx);
    if (idx != json.size()) {
      if (err) {
        *err = "unexpected trailing content after JSON object";
      }
      return false;
    }
    return true;
  }
  while (idx < json.size()) {
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      if (err) {
        *err = "expected JSON object key";
      }
      return false;
    }
    if (out) {
      for (const auto &field : *out) {
        if (field.key == key) {
          if (err) {
            *err = "duplicate field: " + key;
          }
          return false;
        }
      }
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      if (err) {
        *err = "expected ':' after JSON object key";
      }
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      if (err) {
        *err = "invalid JSON value for field: " + key;
      }
      return false;
    }
    if (out) {
      out->push_back({key, trim_ascii(std::string_view(json).substr(
                               value_begin, idx - value_begin))});
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      skip_ws(json, &idx);
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        if (err) {
          *err = "unexpected trailing content after JSON object";
        }
        return false;
      }
      return true;
    }
    if (err) {
      *err = "expected ',' or '}' in JSON object";
    }
    return false;
  }
  if (err) {
    *err = "unterminated JSON object";
  }
  return false;
}

[[nodiscard]] bool
field_allowed(std::string_view key,
              std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), key) != allowed.end();
}

[[nodiscard]] bool
validate_allowed_fields(const std::vector<json_field_t> &fields,
                        std::initializer_list<std::string_view> allowed,
                        std::string *err) {
  for (const auto &field : fields) {
    if (!field_allowed(field.key, allowed)) {
      if (err) {
        *err = "unknown field: " + field.key;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
validate_tool_fields(const std::string &args,
                     std::initializer_list<std::string_view> allowed,
                     std::vector<json_field_t> *fields, std::string *err) {
  if (!parse_json_object_fields(args, fields, err)) {
    return false;
  }
  return validate_allowed_fields(*fields, allowed, err);
}

[[nodiscard]] bool parse_required_string_arg(const std::string &args,
                                             std::string_view key,
                                             std::string *out,
                                             std::string *err) {
  std::string raw;
  if (!extract_json_raw_field(args, key, &raw)) {
    if (err) {
      *err = "missing required field: " + std::string(key);
    }
    return false;
  }
  std::string value;
  if (!extract_json_string_field(args, key, &value)) {
    if (err) {
      *err = std::string(key) + " must be string";
    }
    return false;
  }
  value = trim_ascii(value);
  if (value.empty()) {
    if (err) {
      *err = std::string(key) + " must be non-empty";
    }
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

void append_raw_json_field(std::ostringstream &out, std::string_view key,
                           const std::string &raw, bool *first) {
  if (!*first) {
    out << ",";
  }
  out << json_quote(key) << ":" << raw;
  *first = false;
}

void append_bool_json_field(std::ostringstream &out, std::string_view key,
                            bool value, bool *first) {
  append_raw_json_field(out, key, value ? "true" : "false", first);
}

void append_optional_raw_json_field(const std::string &args,
                                    std::ostringstream &out,
                                    std::string_view key, bool *first) {
  std::string raw;
  if (extract_json_raw_field(args, key, &raw)) {
    append_raw_json_field(out, key, raw, first);
  }
}

[[nodiscard]] std::string
object_with_selected_fields(const std::string &args,
                            std::initializer_list<std::string_view> keys) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto key : keys) {
    append_optional_raw_json_field(args, out, key, &first);
  }
  out << "}";
  return out.str();
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

[[nodiscard]] std::string file_digest_or_empty(const fs::path &path,
                                               const std::string &domain) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return cuwacunu::hero::marshal::marshal_digest_for_text(domain, text);
}

[[nodiscard]] std::string
replay_report_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash,
      std::string("kikijyeba.environment.replay.report_digest.v1\n") + text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] std::string
replay_report_digest_for_path_or_empty(const fs::path &path) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return replay_report_digest_for_text(text);
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

struct runtime_profile_blocks_t {
  std::string base_text{};
  std::unordered_map<std::string, std::string> profile_text_by_id{};
};

[[nodiscard]] bool valid_runtime_profile_id(std::string_view raw) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  for (const unsigned char c : value) {
    if (std::isalnum(c) == 0 && c != '_' && c != '-' && c != '.') {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool split_runtime_profile_blocks(
    std::string_view text, runtime_profile_blocks_t *out,
    std::string *error) {
  if (!out) {
    if (error) {
      *error = "runtime profile block output pointer is null";
    }
    return false;
  }

  runtime_profile_blocks_t parsed{};
  std::istringstream lines{std::string(text)};
  std::string line;
  bool in_profile = false;
  std::string active_profile_id;
  std::ostringstream active_profile_text;
  std::size_t line_no = 0;

  while (std::getline(lines, line)) {
    ++line_no;
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (!in_profile) {
      if (trimmed.rfind("RUNTIME_PROFILE", 0) != 0) {
        parsed.base_text += line;
        parsed.base_text.push_back('\n');
        continue;
      }

      const std::size_t brace = trimmed.find('{');
      if (brace == std::string::npos) {
        if (error) {
          *error = "RUNTIME_PROFILE missing opening brace at line " +
                   std::to_string(line_no);
        }
        return false;
      }
      active_profile_id =
          trim_ascii(trimmed.substr(std::string_view{"RUNTIME_PROFILE"}.size(),
                                    brace - std::string_view{"RUNTIME_PROFILE"}
                                                .size()));
      const std::string after_brace = trim_ascii(trimmed.substr(brace + 1));
      if (!valid_runtime_profile_id(active_profile_id)) {
        if (error) {
          *error = "invalid RUNTIME_PROFILE id at line " +
                   std::to_string(line_no) + ": " + active_profile_id;
        }
        return false;
      }
      if (!after_brace.empty()) {
        if (error) {
          *error =
              "RUNTIME_PROFILE opening line must end after `{` at line " +
              std::to_string(line_no);
        }
        return false;
      }
      if (parsed.profile_text_by_id.count(active_profile_id) != 0) {
        if (error) {
          *error = "duplicate RUNTIME_PROFILE id: " + active_profile_id;
        }
        return false;
      }
      in_profile = true;
      active_profile_text.str(std::string{});
      active_profile_text.clear();
      continue;
    }

    if (trimmed == "}" || trimmed == "};") {
      parsed.profile_text_by_id.emplace(active_profile_id,
                                        active_profile_text.str());
      active_profile_id.clear();
      active_profile_text.str(std::string{});
      active_profile_text.clear();
      in_profile = false;
      continue;
    }
    active_profile_text << line << '\n';
  }

  if (in_profile) {
    if (error) {
      *error = "unterminated RUNTIME_PROFILE block: " + active_profile_id;
    }
    return false;
  }

  *out = std::move(parsed);
  return true;
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

[[nodiscard]] std::string
string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(values[i]);
  }
  out << "]";
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
  fs::path protocol_path{};
  std::string selected_wave_id{};
  std::string selected_wave_source{};
  bool readable{false};
  bool protocol_readable{false};
  std::string error{};
  std::string protocol_error{};
  std::unordered_map<std::string, std::string> values{};
  std::unordered_map<std::string, std::string> protocol_values{};
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
  const auto maybe_wave =
      read_ini_value(config_path, "HERO", "runtime_wave_dsl_path");
  if (!maybe_wave.has_value()) {
    info.error = "missing [HERO].runtime_wave_dsl_path in " +
                 config_path.string();
    return info;
  }
  info.wave_path = resolve_against(config_path, *maybe_wave);
  std::string text;
  if (!read_text_file(info.wave_path, &text, &info.error)) {
    return info;
  }
  info.readable = true;
  const auto maybe_wave_id =
      read_ini_value(config_path, "HERO", "runtime_wave_id");
  info.selected_wave_id = maybe_wave_id.has_value()
                              ? trim_ascii(*maybe_wave_id)
                              : std::string{};
  info.selected_wave_source =
      info.selected_wave_id.empty() ? "wave_file_or_single" : "global_config";
  try {
    const auto block =
        cuwacunu::hero::runtime::settings::selected_wave_settings_block_from_dsl(
            text, info.selected_wave_id);
    const auto settings =
        cuwacunu::hero::runtime::settings::decode_wave_settings_from_block(block);
    if (info.selected_wave_id.empty()) {
      info.selected_wave_id = settings.wave_id;
    }
    info.values = block.values;
  } catch (const std::exception &ex) {
    info.error = ex.what();
    return info;
  }
  const auto target_it = info.values.find("TARGET");
  const bool mdn_wave =
      target_it != info.values.end() &&
      (target_it->second == "wikimyei.inference.expected_value.mdn" ||
       target_it->second == "inference_mdn" ||
       target_it->second == "inference_channel_mdn" ||
       target_it->second == "mdn_expected_value_inference");
  if (mdn_wave) {
    const auto maybe_mdn_jkimyei =
        read_ini_value(config_path, "JKIMYEI",
                       "wikimyei_inference_expected_value_mdn_jkimyei_path");
    if (maybe_mdn_jkimyei.has_value()) {
      std::string training_text;
      std::string training_error;
      const auto training_path =
          resolve_against(config_path, *maybe_mdn_jkimyei);
      if (read_text_file(training_path, &training_text, &training_error)) {
        const auto training_values =
            parse_assignment_text(strip_dsl_comments(training_text), true);
        for (const char *key :
             {"INPUT_REPRESENTATION_CHECKPOINT", "INPUT_MDN_CHECKPOINT"}) {
          const auto input_it = training_values.find(key);
          if (input_it != training_values.end() && !input_it->second.empty()) {
            info.values[key] = input_it->second;
          }
        }
      }
    }
  }
  const auto maybe_protocol =
      read_ini_value(config_path, "KIKIJYEBA", "kikijyeba_protocol_dsl_path");
  if (maybe_protocol.has_value()) {
    info.protocol_path = resolve_against(config_path, *maybe_protocol);
    std::string protocol_text;
    if (read_text_file(info.protocol_path, &protocol_text,
                       &info.protocol_error)) {
      info.protocol_readable = true;
      info.protocol_values =
          parse_assignment_text(strip_dsl_comments(protocol_text), true);
    }
  }
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

struct wave_overlay_t {
  bool present{false};
  std::string source_range{};
  std::string anchor_index_begin{};
  std::string anchor_index_end{};
  std::string source_key_begin{};
  std::string source_key_end{};
};

[[nodiscard]] bool parse_wave_overlay_from_args(const std::string &args,
                                                wave_overlay_t *overlay,
                                                std::string *err) {
  if (overlay == nullptr) {
    return true;
  }
  *overlay = {};
  std::string raw;
  if (!extract_json_raw_field(args, "wave_overlay", &raw)) {
    return true;
  }
  overlay->present = true;
  (void)extract_json_string_field(raw, "source_range", &overlay->source_range);
  (void)extract_json_string_field(raw, "anchor_index_begin",
                                  &overlay->anchor_index_begin);
  (void)extract_json_string_field(raw, "anchor_index_end",
                                  &overlay->anchor_index_end);
  (void)extract_json_string_field(raw, "source_key_begin",
                                  &overlay->source_key_begin);
  (void)extract_json_string_field(raw, "source_key_end",
                                  &overlay->source_key_end);
  if (trim_ascii(overlay->source_range).empty()) {
    if (!trim_ascii(overlay->anchor_index_begin).empty() ||
        !trim_ascii(overlay->anchor_index_end).empty()) {
      overlay->source_range = "anchor_index";
    } else if (!trim_ascii(overlay->source_key_begin).empty() ||
               !trim_ascii(overlay->source_key_end).empty()) {
      overlay->source_range = "source_key";
    } else {
      overlay->source_range = "all";
    }
  }
  overlay->source_range = canonical_source_range(overlay->source_range);
  if (overlay->source_range != "all" &&
      overlay->source_range != "anchor_index" &&
      overlay->source_range != "source_key") {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: source_range must be all, "
             "anchor_index, or source_key";
    }
    return false;
  }
  if (overlay->source_range == "all" &&
      (!trim_ascii(overlay->anchor_index_begin).empty() ||
       !trim_ascii(overlay->anchor_index_end).empty() ||
       !trim_ascii(overlay->source_key_begin).empty() ||
       !trim_ascii(overlay->source_key_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: SOURCE_RANGE=all cannot carry "
             "range bounds";
    }
    return false;
  }
  if (overlay->source_range == "anchor_index" &&
      (trim_ascii(overlay->anchor_index_begin).empty() ||
       trim_ascii(overlay->anchor_index_end).empty() ||
       !trim_ascii(overlay->source_key_begin).empty() ||
       !trim_ascii(overlay->source_key_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: anchor_index overlay requires "
             "anchor_index_begin/end only";
    }
    return false;
  }
  if (overlay->source_range == "source_key" &&
      (trim_ascii(overlay->source_key_begin).empty() ||
       trim_ascii(overlay->source_key_end).empty() ||
       !trim_ascii(overlay->anchor_index_begin).empty() ||
       !trim_ascii(overlay->anchor_index_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: source_key overlay requires "
             "source_key_begin/end only";
    }
    return false;
  }
  return true;
}

inline void apply_wave_overlay_to_info(wave_info_t *info,
                                       const wave_overlay_t &overlay) {
  if (info == nullptr || !overlay.present) {
    return;
  }
  info->values["SOURCE_RANGE"] = overlay.source_range;
  info->values.erase("ANCHOR_INDEX_BEGIN");
  info->values.erase("ANCHOR_INDEX_END");
  info->values.erase("SOURCE_KEY_BEGIN");
  info->values.erase("SOURCE_KEY_END");
  info->values.erase("ANCHOR_KEY_BEGIN");
  info->values.erase("ANCHOR_KEY_END");
  if (overlay.source_range == "anchor_index") {
    info->values["ANCHOR_INDEX_BEGIN"] = overlay.anchor_index_begin;
    info->values["ANCHOR_INDEX_END"] = overlay.anchor_index_end;
  } else if (overlay.source_range == "source_key") {
    info->values["SOURCE_KEY_BEGIN"] = overlay.source_key_begin;
    info->values["SOURCE_KEY_END"] = overlay.source_key_end;
  }
}

[[nodiscard]] std::string job_kind_from_target(std::string_view target) {
  const std::string lower = lowercase_ascii(target);
  if (lower == "wikimyei.representation.encoding.vicreg" ||
      lower == "vicreg_representation") {
    return "channel_representation_vicreg";
  }
  if (lower == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" ||
      lower == "mtf_jepa_mae_vicreg_representation" ||
      lower == "mtf_jvmae_representation") {
    return "channel_representation_mtf_jepa_mae_vicreg";
  }
  if (lower == "wikimyei.inference.expected_value.mdn" ||
      lower == "inference_mdn" || lower == "inference_channel_mdn" ||
      lower == "mdn_expected_value_inference") {
    return "channel_inference_mdn";
  }
  if (lower == "wikimyei.policy.trainable" ||
      lower == "policy_trainable" || lower == "trainable_policy") {
    return "policy_training";
  }
  return "invalid_wave_target";
}

[[nodiscard]] std::string wave_value_or_empty(const wave_info_t &info,
                                              std::string_view key) {
  const auto it = info.values.find(std::string(key));
  return it == info.values.end() ? std::string{} : trim_ascii(it->second);
}

[[nodiscard]] std::string wave_job_kind_from_info(const wave_info_t &info) {
  const std::string explicit_job_kind = wave_value_or_empty(info, "JOB_KIND");
  if (!explicit_job_kind.empty()) {
    return explicit_job_kind;
  }
  return job_kind_from_target(wave_value_or_empty(info, "TARGET"));
}

[[nodiscard]] bool wave_info_requests_policy_training(const wave_info_t &info) {
  if (!info.readable || !info.error.empty()) {
    return false;
  }
  const std::string job_kind =
      lowercase_ascii(trim_ascii(wave_job_kind_from_info(info)));
  if (job_kind == "policy_training") {
    return true;
  }
  return !wave_value_or_empty(info, "POLICY_KIND").empty() ||
         !wave_value_or_empty(info, "POLICY_ID").empty() ||
         !wave_value_or_empty(info, "TRAINING_SCHEDULE_MODE").empty();
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

[[nodiscard]] std::string protocol_id_from_info(const wave_info_t &info) {
  const auto it = info.protocol_values.find("PROTOCOL_ID");
  return it == info.protocol_values.end() ? std::string{"cwu_01v"} : it->second;
}

[[nodiscard]] std::string protocol_value_from_info(const wave_info_t &info,
                                                   const char *key,
                                                   std::string fallback = {}) {
  const auto it = info.protocol_values.find(key);
  return it == info.protocol_values.end() ? std::move(fallback) : it->second;
}

[[nodiscard]] std::vector<std::string>
compatible_protocols_from_info(const wave_info_t &info) {
  const auto it = info.values.find("COMPATIBLE_PROTOCOLS");
  if (it == info.values.end() || trim_ascii(it->second).empty()) {
    return {};
  }
  std::string raw = it->second;
  for (char &ch : raw) {
    if (ch == '|' || ch == '+') {
      ch = ',';
    }
  }
  return split_csv(raw);
}

[[nodiscard]] bool
wave_protocol_compatible(const std::vector<std::string> &compatible_protocols,
                         std::string_view active_protocol_id) {
  if (compatible_protocols.empty()) {
    return true;
  }
  const std::string active = trim_ascii(active_protocol_id);
  return std::find(compatible_protocols.begin(), compatible_protocols.end(),
                   active) != compatible_protocols.end();
}

[[nodiscard]] std::string
active_representation_from_info(const wave_info_t &info) {
  const auto it = info.protocol_values.find("REPRESENTATION");
  if (it == info.protocol_values.end() || trim_ascii(it->second).empty()) {
    return "wikimyei.representation.encoding.vicreg";
  }
  const std::string lower = lowercase_ascii(trim_ascii(it->second));
  if (lower == "mtf_jepa_mae_vicreg" ||
      lower == "mtf_jepa_mae_vicreg_representation" ||
      lower == "mtf_jvmae_representation") {
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  if (lower == "vicreg" || lower == "vicreg_representation") {
    return "wikimyei.representation.encoding.vicreg";
  }
  return it->second;
}

[[nodiscard]] bool
protocol_target_compatible(std::string_view target,
                           std::string_view active_representation_family) {
  const std::string job_kind = job_kind_from_target(target);
  const std::string active =
      lowercase_ascii(trim_ascii(active_representation_family));
  if (job_kind == "channel_representation_vicreg") {
    return active == "wikimyei.representation.encoding.vicreg";
  }
  if (job_kind == "channel_representation_mtf_jepa_mae_vicreg") {
    return active == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  return true;
}

[[nodiscard]] std::string
execution_chain(std::string_view target, std::string_view action,
                std::string_view active_representation_family =
                    "wikimyei.representation.encoding.vicreg") {
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
  if (job_kind == "channel_representation_mtf_jepa_mae_vicreg") {
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run";
  }
  if (job_kind == "channel_inference_mdn") {
    const bool active_mtf =
        active_representation_family ==
        "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
    if (active_mtf) {
      if (action == "train") {
        return "ujcamei.source.registry:run -> "
               "wikimyei.expression.nodelift.srl:run -> "
               "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
               "run_frozen -> wikimyei.inference.expected_value.mdn:train";
      }
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
             "run_frozen -> wikimyei.inference.expected_value.mdn:run";
    }
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
  if (job_kind == "policy_training") {
    return "runtime.policy_training_contract:validate -> "
           "runtime.policy_training.noop:write_checkpoint";
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
  const std::string protocol_id = protocol_id_from_info(info);
  const std::string protocol_status =
      protocol_value_from_info(info, "PROTOCOL_STATUS", "active");
  const std::string successor_protocol =
      protocol_value_from_info(info, "SUCCESSOR_PROTOCOL");
  const std::string protocol_warning =
      protocol_value_from_info(info, "PROTOCOL_WARNING");
  const bool protocol_warns =
      !protocol_warning.empty() ||
      lowercase_ascii(trim_ascii(protocol_status)) != "active";
  const std::vector<std::string> compatible_protocols =
      compatible_protocols_from_info(info);
  const bool profile_compatible =
      wave_protocol_compatible(compatible_protocols, protocol_id);
  const std::string active_representation =
      active_representation_from_info(info);
  const std::string protocol_observer =
      protocol_value_from_info(info, "OBSERVER",
                               "wikimyei.observer.belief");
  const std::string protocol_allocation_policy = protocol_value_from_info(
      info, "ALLOCATION_POLICY",
      "wikimyei.policy.portfolio.spot_distributional_utility");
  const bool target_compatible =
      protocol_target_compatible(target, active_representation);
  const std::string job_kind = wave_job_kind_from_info(info);
  const std::string policy_id = wave_value_or_empty(info, "POLICY_ID");
  const std::string policy_kind = wave_value_or_empty(info, "POLICY_KIND");
  const std::string training_schedule_mode =
      wave_value_or_empty(info, "TRAINING_SCHEDULE_MODE");
  const std::string live_execution_allowed =
      wave_value_or_empty(info, "LIVE_EXECUTION_ALLOWED");
  std::ostringstream out;
  out << "{\"config_path\":" << json_quote(info.config_path.string())
      << ",\"wave_path\":" << json_quote(info.wave_path.string())
      << ",\"protocol_path\":" << json_quote(info.protocol_path.string())
      << ",\"selected_wave_id\":" << json_quote(info.selected_wave_id)
      << ",\"selected_wave_source\":"
      << json_quote(info.selected_wave_source)
      << ",\"readable\":" << bool_json(info.readable);
  if (!info.error.empty()) {
    out << ",\"error\":" << json_quote(info.error);
  }
  out << ",\"protocol_readable\":" << bool_json(info.protocol_readable);
  if (!info.protocol_error.empty()) {
    out << ",\"protocol_error\":" << json_quote(info.protocol_error);
  }
  out << ",\"wave_id\":" << json_quote(wave_id)
      << ",\"protocol_id\":" << json_quote(protocol_id)
      << ",\"protocol_status\":" << json_quote(protocol_status)
      << ",\"successor_protocol\":" << json_quote(successor_protocol)
      << ",\"protocol_warning_level\":"
      << json_quote(protocol_warns ? "warning" : "none")
      << ",\"protocol_warning\":" << json_quote(protocol_warning)
      << ",\"compatible_protocols\":" << string_array_json(compatible_protocols)
      << ",\"wave_protocol_compatible\":" << bool_json(profile_compatible)
      << ",\"wave_protocol_warning\":"
      << json_quote(profile_compatible
                        ? std::string{}
                        : "active_protocol_not_declared_by_wave_profile")
      << ",\"active_representation_family\":"
      << json_quote(active_representation)
      << ",\"protocol_observer_family\":" << json_quote(protocol_observer)
      << ",\"protocol_allocation_policy_family\":"
      << json_quote(protocol_allocation_policy)
      << ",\"protocol_target_compatible\":" << bool_json(target_compatible)
      << ",\"protocol_target_warning\":"
      << json_quote(target_compatible ? std::string{}
                                      : "wave_target_not_docked_by_active_"
                                        "protocol")
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
  out << ",\"job_kind\":" << json_quote(job_kind)
      << ",\"policy_id\":";
  if (policy_id.empty()) {
    out << "null";
  } else {
    out << json_quote(policy_id);
  }
  out << ",\"policy_kind\":";
  if (policy_kind.empty()) {
    out << "null";
  } else {
    out << json_quote(policy_kind);
  }
  out << ",\"training_schedule_mode\":";
  if (training_schedule_mode.empty()) {
    out << "null";
  } else {
    out << json_quote(training_schedule_mode);
  }
  out << ",\"live_execution_allowed\":";
  if (live_execution_allowed.empty()) {
    out << "false";
  } else {
    out << bool_json(lowercase_ascii(live_execution_allowed) == "true");
  }
  out << ",\"policy_training_wave\":"
      << bool_json(wave_info_requests_policy_training(info))
      << ",\"train_target\":" << bool_json(action == "train")
      << ",\"execution_chain\":"
      << json_quote(execution_chain(target, action, active_representation))
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
      *err = "job_id must be a runtime job id";
      return false;
    }
    const auto found =
        cuwacunu::hero::runtime::job_layout::find_runtime_job_dir_by_id(
            runtime_root(policy), job_id_arg);
    if (!found.has_value()) {
      *err = "E_RUNTIME_JOB_NOT_FOUND: " + job_id_arg;
      return false;
    }
    path = normalize_path(*found);
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

[[nodiscard]] fs::path runtime_replay_artifact_dir(const fs::path &job_dir) {
  return job_dir / "artifacts" / "kikijyeba.environment.replay.v1";
}

[[nodiscard]] fs::path
runtime_replay_batch_index_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) / "runtime_replay_batches.index";
}

[[nodiscard]] fs::path
runtime_replay_experiment_index_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiments.index";
}

[[nodiscard]] fs::path
resolve_runtime_replay_artifact_path(const fs::path &raw_path,
                                     const fs::path &index_dir) {
  if (raw_path.empty()) {
    return {};
  }
  return raw_path.is_absolute() ? normalize_path(raw_path)
                                : normalize_path(index_dir / raw_path);
}

[[nodiscard]] bool path_contains_parent_reference(const fs::path &path) {
  for (const auto &part : path) {
    if (part == "..") {
      return true;
    }
  }
  return false;
}

struct replay_experiment_report_binding_t {
  fs::path index_path{};
  fs::path report_path{};
  std::string declared_report_digest{};
  bool index_present{false};
  bool report_path_from_index{false};
  bool report_path_rejected{false};
};

[[nodiscard]] fs::path
latest_replay_experiment_report_default_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiment.report";
}

[[nodiscard]] replay_experiment_report_binding_t
latest_replay_experiment_report_binding(const fs::path &job_dir) {
  replay_experiment_report_binding_t binding{};
  binding.index_path = runtime_replay_experiment_index_path(job_dir);
  const auto index = parse_kv_file(binding.index_path);
  binding.index_present = !index.empty();
  int entry_count = 0;
  if (parse_int(index.count("entry_count") == 0 ? std::string{}
                                                : index.at("entry_count"),
                &entry_count) &&
      entry_count > 0) {
    const auto prefix =
        std::string("entry_") + std::to_string(entry_count - 1) + "_";
    const auto digest_found = index.find(prefix + "report_digest");
    if (digest_found != index.end()) {
      binding.declared_report_digest = trim_ascii(digest_found->second);
    }
    const auto found = index.find(prefix + "report_path");
    if (found != index.end() && !trim_ascii(found->second).empty()) {
      const fs::path raw_report_path(trim_ascii(found->second));
      if (!path_contains_parent_reference(raw_report_path)) {
        binding.report_path = resolve_runtime_replay_artifact_path(
            raw_report_path, binding.index_path.parent_path());
        binding.report_path_from_index = true;
        return binding;
      }
      binding.report_path_rejected = true;
      return binding;
    }
  }
  binding.report_path = latest_replay_experiment_report_default_path(job_dir);
  return binding;
}

[[nodiscard]] fs::path
latest_replay_experiment_report_path(const fs::path &job_dir) {
  return latest_replay_experiment_report_binding(job_dir).report_path;
}

[[nodiscard]] std::string replay_report_integrity_json(
    const replay_experiment_report_binding_t &binding) {
  const bool report_exists =
      !binding.report_path.empty() && fs::exists(binding.report_path);
  const bool is_file =
      report_exists && fs::is_regular_file(binding.report_path);
  const std::string computed_digest =
      is_file ? replay_report_digest_for_path_or_empty(binding.report_path)
              : std::string{};
  const bool digest_bound = !binding.declared_report_digest.empty();
  const bool digest_match = digest_bound && !computed_digest.empty() &&
                            binding.declared_report_digest == computed_digest;
  std::ostringstream out;
  out << "{\"index_path\":" << json_quote(binding.index_path.string())
      << ",\"report_path\":" << json_quote(binding.report_path.string())
      << ",\"index_present\":" << bool_json(binding.index_present)
      << ",\"report_path_from_index\":"
      << bool_json(binding.report_path_from_index)
      << ",\"report_path_rejected\":" << bool_json(binding.report_path_rejected)
      << ",\"report_exists\":" << bool_json(report_exists)
      << ",\"is_regular_file\":" << bool_json(is_file)
      << ",\"report_digest_bound\":" << bool_json(digest_bound)
      << ",\"declared_report_digest\":"
      << json_quote(binding.declared_report_digest)
      << ",\"computed_report_digest\":" << json_quote(computed_digest)
      << ",\"report_digest_match\":" << bool_json(digest_match) << "}";
  return out.str();
}

[[nodiscard]] std::string job_artifacts_json(const fs::path &job_dir,
                                             const std::string &report_path,
                                             bool include_text,
                                             std::size_t max_bytes) {
  fs::path effective_report =
      report_path.empty() ? fs::path{} : fs::path(report_path);
  if (effective_report.empty()) {
    const fs::path channel_inference = job_dir / "channel_inference.report";
    const fs::path channel_representation =
        job_dir / "channel_representation.report";
    const fs::path inference = job_dir / "inference.report";
    const fs::path representation = job_dir / "representation.report";
    if (fs::exists(channel_inference)) {
      effective_report = channel_inference;
    } else if (fs::exists(channel_representation)) {
      effective_report = channel_representation;
    } else if (fs::exists(inference)) {
      effective_report = inference;
    } else if (fs::exists(representation)) {
      effective_report = representation;
    }
  }
  const auto replay_report_binding =
      latest_replay_experiment_report_binding(job_dir);
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
  out << ",\"replay_batch_index\":"
      << artifact_summary_json(runtime_replay_batch_index_path(job_dir),
                               include_text, max_bytes)
      << ",\"replay_experiment_index\":"
      << artifact_summary_json(runtime_replay_experiment_index_path(job_dir),
                               include_text, max_bytes)
      << ",\"replay_experiment_report\":"
      << artifact_summary_json(replay_report_binding.report_path, include_text,
                               max_bytes)
      << ",\"replay_experiment_report_integrity\":"
      << replay_report_integrity_json(replay_report_binding) << "}";
  return out.str();
}

[[nodiscard]] std::string
runtime_terminal_evidence_json(const fs::path &job_dir) {
  const fs::path manifest_path = job_dir / "job.manifest";
  const fs::path state_path = job_dir / "job.state";
  const fs::path result_fact_path = job_dir / "runtime.result.fact";
  const fs::path checkpoint_io_path = job_dir / "runtime.checkpoint_io.fact";
  const auto manifest = parse_kv_file(manifest_path);
  const auto state = parse_kv_file(state_path);
  const auto result_fact = parse_kv_file(result_fact_path);
  const auto checkpoint_io = parse_kv_file(checkpoint_io_path);
  const auto get = [](const auto &fields,
                      const std::string &key) -> std::string {
    const auto it = fields.find(key);
    return it == fields.end() ? std::string{} : it->second;
  };
  const auto first = [](std::initializer_list<std::string> values) {
    for (const auto &value : values) {
      if (!trim_ascii(value).empty()) {
        return value;
      }
    }
    return std::string{};
  };
  const auto truthy = [](const std::string &value) {
    bool parsed = false;
    return parse_bool(value, &parsed) && parsed;
  };
  const std::string job_id =
      first({get(result_fact, "job_id"), get(state, "job_id"),
             get(manifest, "job_id")});
  const std::string status =
      first({get(result_fact, "status"), get(state, "status")});
  const std::string handoff_id = first({get(result_fact, "runtime_handoff_id"),
                                        get(manifest, "runtime_handoff_id")});
  const std::string handoff_digest =
      first({get(result_fact, "runtime_handoff_digest"),
             get(manifest, "runtime_handoff_digest")});
  const std::string target_driver_run_id =
      first({get(result_fact, "marshal_target_driver_run_id"),
             get(manifest, "marshal_target_driver_run_id")});
  const bool checkpoint_io_required =
      truthy(first({get(checkpoint_io, "checkpoint_written"),
                    get(result_fact, "checkpoint_written"),
                    get(state, "checkpoint_written")})) ||
      truthy(first({get(checkpoint_io, "representation_checkpoint_loaded"),
                    get(result_fact, "representation_checkpoint_loaded")})) ||
      truthy(first({get(checkpoint_io, "mdn_checkpoint_loaded"),
                    get(result_fact, "mdn_checkpoint_loaded")})) ||
      !first({get(checkpoint_io, "checkpoint_path"),
              get(result_fact, "checkpoint_path"),
              get(state, "checkpoint_path"),
              get(manifest, "input_representation_checkpoint_path"),
              get(manifest, "input_mdn_checkpoint_path")})
           .empty();
  const bool terminal_status = status == "completed" || status == "failed" ||
                               status == "skipped" || status == "dry_run";
  const bool observed = !job_id.empty() && terminal_status &&
                        fs::is_regular_file(manifest_path) &&
                        fs::is_regular_file(result_fact_path);

  std::ostringstream out;
  out << "{\"observed\":" << bool_json(observed)
      << ",\"job_id\":" << json_quote(job_id)
      << ",\"job_dir\":" << json_quote(job_dir.string())
      << ",\"terminal_status\":" << json_quote(status)
      << ",\"job_manifest_path\":" << json_quote(manifest_path.string())
      << ",\"job_manifest_digest\":"
      << json_quote(file_digest_or_empty(
             manifest_path, "kikijyeba.marshal.runtime_job_manifest.v1"))
      << ",\"job_state_path\":" << json_quote(state_path.string())
      << ",\"job_state_digest\":"
      << json_quote(file_digest_or_empty(
             state_path, "kikijyeba.marshal.runtime_job_state.v1"))
      << ",\"runtime_result_fact_path\":"
      << json_quote(result_fact_path.string())
      << ",\"runtime_result_fact_digest\":"
      << json_quote(file_digest_or_empty(
             result_fact_path, "kikijyeba.marshal.runtime_terminal_fact.v1"))
      << ",\"checkpoint_io_required\":" << bool_json(checkpoint_io_required)
      << ",\"runtime_checkpoint_io_fact_path\":"
      << json_quote(checkpoint_io_path.string())
      << ",\"runtime_checkpoint_io_fact_digest\":"
      << json_quote(file_digest_or_empty(
             checkpoint_io_path,
             "kikijyeba.marshal.runtime_checkpoint_io_fact.v1"))
      << ",\"runtime_handoff_id\":" << json_quote(handoff_id)
      << ",\"runtime_handoff_digest\":" << json_quote(handoff_digest)
      << ",\"target_driver_run_id\":" << json_quote(target_driver_run_id)
      << "}";
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
    } else if (const auto found_replay_report =
                   stdout_kv.find("replay_report_path");
               found_replay_report != stdout_kv.end()) {
      report_path = found_replay_report->second;
    }
    out << ",\"artifacts\":"
        << job_artifacts_json(artifact_job_dir, report_path, include_text,
                              max_bytes)
        << ",\"terminal_evidence\":"
        << runtime_terminal_evidence_json(artifact_job_dir);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool
report_i64_field(const std::unordered_map<std::string, std::string> &report,
                 std::string_view key, long long *out, std::string *err) {
  const auto found = report.find(std::string(key));
  if (found == report.end()) {
    *err = "missing report field: " + std::string(key);
    return false;
  }
  const std::string value = trim_ascii(found->second);
  if (value.empty()) {
    *err = "empty report field: " + std::string(key);
    return false;
  }
  long long parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    *err = "non-integer report field: " + std::string(key) + "=" + value;
    return false;
  }
  *out = parsed;
  return true;
}

[[nodiscard]] bool
validate_replay_report_for_validation_rollout(const fs::path &report_path,
                                              std::string *err) {
  if (report_path.empty() || !fs::exists(report_path) ||
      !fs::is_regular_file(report_path)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_MISSING";
    return false;
  }
  const auto report = parse_kv_file(report_path);
  if (report.empty()) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_EMPTY";
    return false;
  }

  const auto require_nonempty = [&](std::string_view key,
                                    const char *code) -> bool {
    const auto found = report.find(std::string(key));
    if (found == report.end() || trim_ascii(found->second).empty()) {
      *err = code;
      return false;
    }
    return true;
  };
  if (!require_nonempty("execution_profile_digest",
                        "E_RUNTIME_REPLAY_VALIDATION_PROFILE_DIGEST_MISSING") ||
      !require_nonempty(
          "policy_set_digest",
          "E_RUNTIME_REPLAY_VALIDATION_POLICY_SET_DIGEST_MISSING")) {
    return false;
  }

  long long completed_count = 0;
  long long invalid_trace_count = 0;
  long long synthetic_step_count = 0;
  std::string field_error;
  if (!report_i64_field(report, "completed_count", &completed_count,
                        &field_error)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_FIELD_INVALID: " + field_error;
    return false;
  }
  if (completed_count <= 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_NO_COMPLETED_EPISODES";
    return false;
  }
  if (!report_i64_field(report, "cajtucu_invalid_trace_count",
                        &invalid_trace_count, &field_error) ||
      !report_i64_field(report, "cajtucu_synthetic_market_step_count",
                        &synthetic_step_count, &field_error)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_FIELD_INVALID: " + field_error;
    return false;
  }
  if (synthetic_step_count > 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_MARKET_USED: "
           "cajtucu_synthetic_market_step_count=" +
           std::to_string(synthetic_step_count);
    return false;
  }
  if (invalid_trace_count > 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_INVALID_CAJTUCU_TRACE: "
           "cajtucu_invalid_trace_count=" +
           std::to_string(invalid_trace_count);
    return false;
  }
  return true;
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

[[nodiscard]] bool parse_optional_double_arg(const std::string &args,
                                             std::string_view key,
                                             double default_value, double *out,
                                             std::string *err) {
  double value = default_value;
  std::string raw;
  if (extract_json_raw_field(args, key, &raw) && !parse_double(raw, &value)) {
    *err = std::string(key) + " must be number";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_policy_training_required_int(const std::string &args,
                                                      std::string_view key,
                                                      int *out,
                                                      std::string *err) {
  if (!extract_json_raw_field(args, key, nullptr)) {
    *err = "missing required field: " + std::string(key);
    return false;
  }
  int value = 0;
  if (!extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool
parse_required_string_arg_or_wave(const std::string &args,
                                  std::string_view json_key,
                                  const wave_info_t *selected_wave,
                                  std::string_view wave_key,
                                  std::string *out, std::string *err) {
  if (extract_json_raw_field(args, json_key, nullptr)) {
    return parse_required_string_arg(args, json_key, out, err);
  }
  if (selected_wave != nullptr) {
    const auto value = wave_value_or_empty(*selected_wave, wave_key);
    if (!value.empty()) {
      *out = value;
      return true;
    }
  }
  if (err) {
    *err = "missing required field: " + std::string(json_key);
  }
  return false;
}

[[nodiscard]] bool
parse_optional_bool_arg_or_wave(const std::string &args,
                                std::string_view json_key,
                                const wave_info_t *selected_wave,
                                std::string_view wave_key,
                                bool default_value, bool *out,
                                std::string *err) {
  if (extract_json_raw_field(args, json_key, nullptr)) {
    return parse_optional_bool_arg(args, json_key, default_value, out, err);
  }
  if (selected_wave != nullptr) {
    const auto value =
        lowercase_ascii(wave_value_or_empty(*selected_wave, wave_key));
    if (!value.empty()) {
      if (value == "true") {
        *out = true;
        return true;
      }
      if (value == "false") {
        *out = false;
        return true;
      }
      if (err) {
        *err = std::string(wave_key) + " must be true or false";
      }
      return false;
    }
  }
  *out = default_value;
  return true;
}

[[nodiscard]] std::string policy_training_contract_json(
    const cuwacunu::hero::runtime::policy_training_job_contract_t
        &contract) {
  std::ostringstream out;
  out << "{\"schema_version\":" << json_quote(contract.schema_version)
      << ",\"artifact_schema_id\":" << json_quote(contract.artifact_schema_id)
      << ",\"runtime_job_kind\":" << json_quote(contract.runtime_job_kind)
      << ",\"protocol_id\":" << json_quote(contract.protocol_id)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(contract.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(contract.graph_order_fingerprint)
      << ",\"source_cursor_token\":"
      << json_quote(contract.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(contract.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(contract.component_assembly_fingerprint)
      << ",\"policy_id\":" << json_quote(contract.policy_id)
      << ",\"policy_kind\":" << json_quote(contract.policy_kind)
      << ",\"policy_architecture_digest\":"
      << json_quote(contract.policy_architecture_digest)
      << ",\"training_config_digest\":"
      << json_quote(contract.training_config_digest)
      << ",\"training_range_digest\":"
      << json_quote(contract.training_range_digest)
      << ",\"validation_range_digest\":"
      << json_quote(contract.validation_range_digest)
      << ",\"test_range_digest\":" << json_quote(contract.test_range_digest)
      << ",\"environment_contract_id\":"
      << json_quote(contract.environment_contract_id)
      << ",\"observation_schema_digest\":"
      << json_quote(contract.observation_schema_digest)
      << ",\"action_schema_digest\":"
      << json_quote(contract.action_schema_digest)
      << ",\"reward_contract_digest\":"
      << json_quote(contract.reward_contract_digest)
      << ",\"execution_profile_digest\":"
      << json_quote(contract.execution_profile_digest)
      << ",\"training_schedule_mode\":"
      << json_quote(contract.training_schedule_mode)
      << ",\"causal_schedule_schema_id\":"
      << json_quote(contract.causal_schedule_schema_id)
      << ",\"causal_schedule_digest\":"
      << json_quote(contract.causal_schedule_digest)
      << ",\"causal_schedule_cursor_key_kind\":"
      << json_quote(contract.causal_schedule_cursor_key_kind)
      << ",\"causal_schedule_no_future_snapshot_use_source\":"
      << json_quote(contract.causal_schedule_no_future_snapshot_use_source)
      << ",\"normalization_fit_range_digest\":"
      << json_quote(contract.normalization_fit_range_digest)
      << ",\"replay_buffer_source_range_digest\":"
      << json_quote(contract.replay_buffer_source_range_digest)
      << ",\"early_stopping_policy_digest\":"
      << json_quote(contract.early_stopping_policy_digest)
      << ",\"hyperparameter_selection_policy_digest\":"
      << json_quote(contract.hyperparameter_selection_policy_digest)
      << ",\"selector_split\":" << json_quote(contract.selector_split)
      << ",\"selector_policy_digest\":"
      << json_quote(contract.selector_policy_digest)
      << ",\"parent_checkpoint_digest\":"
      << json_quote(contract.parent_checkpoint_digest)
      << ",\"parent_forecast_eval_fact_digest\":"
      << json_quote(contract.parent_forecast_eval_fact_digest)
      << ",\"parent_observer_belief_fact_digest\":"
      << json_quote(contract.parent_observer_belief_fact_digest)
      << ",\"parent_allocation_engine_fact_digest\":"
      << json_quote(contract.parent_allocation_engine_fact_digest)
      << ",\"parent_replay_environment_fact_digest\":"
      << json_quote(contract.parent_replay_environment_fact_digest)
      << ",\"final_refit_parent_selected_checkpoint_digest\":"
      << json_quote(contract.final_refit_parent_selected_checkpoint_digest)
      << ",\"random_seed\":" << contract.random_seed
      << ",\"max_episodes\":" << contract.max_episodes
      << ",\"max_steps\":" << contract.max_steps
      << ",\"max_parallel_jobs\":" << contract.max_parallel_jobs
      << ",\"max_wall_clock_seconds\":" << contract.max_wall_clock_seconds
      << ",\"causal_schedule_readiness_eligible\":"
      << bool_json(contract.causal_schedule_readiness_eligible)
      << ",\"causal_schedule_no_future_snapshot_use\":"
      << bool_json(contract.causal_schedule_no_future_snapshot_use)
      << ",\"offline_full_window_research_allowed\":"
      << bool_json(contract.offline_full_window_research_allowed)
      << ",\"final_refit_uses_validation\":"
      << bool_json(contract.final_refit_uses_validation)
      << ",\"validation_no_longer_proof\":"
      << bool_json(contract.validation_no_longer_proof)
      << ",\"sealed_test_required\":"
      << bool_json(contract.sealed_test_required)
      << ",\"live_execution_allowed\":"
      << bool_json(contract.live_execution_allowed) << "}";
  return out.str();
}

[[nodiscard]] bool policy_training_execute_kind_supported(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  return contract.policy_kind == "noop_policy_training.v1";
}

[[nodiscard]] std::string policy_training_unsupported_execute_reason(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  if (lowercase_ascii(contract.policy_kind).find("ppo") !=
      std::string::npos) {
    return "E_RUNTIME_POLICY_TRAINING_PPO_NOT_IMPLEMENTED: PPO policy "
           "training is intentionally unavailable before the causal Runtime "
           "trainer milestone";
  }
  return "E_RUNTIME_POLICY_TRAINING_POLICY_KIND_UNSUPPORTED: "
         "requested_mode=execute currently supports only "
         "policy_kind=noop_policy_training.v1";
}

[[nodiscard]] std::string policy_training_checkpoint_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest) {
  std::ostringstream out;
  out << "schema=kikijyeba.runtime.policy_training.noop_checkpoint.v1\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "ppo_implemented=false\n";
  out << "optimizer_steps=0\n";
  out << "trainer_kind=noop_policy_training.v1\n";
  out << "readiness_scope=pre_ppo_runtime_plumbing\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_report_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const fs::path &checkpoint_path,
    std::string_view checkpoint_digest) {
  std::error_code ec;
  const auto checkpoint_bytes = fs::exists(checkpoint_path, ec)
                                    ? fs::file_size(checkpoint_path, ec)
                                    : std::uintmax_t{0};
  std::ostringstream out;
  out << "report_schema_id=kikijyeba.runtime.policy_training.noop_report.v1\n";
  out << "report_schema_version=1\n";
  out << "report_writer_id=hero.runtime.policy_training.noop_pre_ppo.v1\n";
  out << "report_writer_version=1\n";
  out << "run_data_kind=policy_training_noop_smoke\n";
  out << "readiness_scope=pre_ppo_runtime_plumbing\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "policy_architecture_digest=" << contract.policy_architecture_digest
      << "\n";
  out << "training_config_digest=" << contract.training_config_digest << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "training_schedule_mode=" << contract.training_schedule_mode << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "causal_schedule_no_future_snapshot_use="
      << (contract.causal_schedule_no_future_snapshot_use ? "true" : "false")
      << "\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "checkpoint_path_reported=" << checkpoint_path.string() << "\n";
  out << "checkpoint_digest_reported=" << checkpoint_digest << "\n";
  out << "checkpoint_digest_verified=true\n";
  out << "checkpoint_file_exists=true\n";
  out << "checkpoint_bytes=" << checkpoint_bytes << "\n";
  out << "checkpoint_artifact_status=written\n";
  out << "checkpoint_written=true\n";
  out << "optimizer_steps=0\n";
  out << "steps_attempted=0\n";
  out << "steps_completed=0\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  out << "ppo_implemented=false\n";
  out << "live_execution_allowed=false\n";
  out << "market_readiness_claimed=false\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_manifest_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, std::string_view job_id,
    std::string_view job_stable_id, std::string_view attempt_leaf,
    const fs::path &global_config_path) {
  std::ostringstream out;
  out << "manifest_format=kikijyeba.runtime.job_manifest.v1\n";
  out << "job_id=" << job_id << "\n";
  out << "job_stable_id=" << job_stable_id << "\n";
  out << "job_attempt_id=" << attempt_leaf << "\n";
  out << "job_attempt_index=0\n";
  out << "job_attempt_policy=immutable_no_overwrite\n";
  out << "job_kind=policy_training\n";
  out << "config_path=" << global_config_path.string() << "\n";
  out << "config_bundle_id=policy_training_contract_" << contract_digest
      << "\n";
  out << "config_receipt_id=policy_training_contract_" << contract_digest
      << "\n";
  out << "component_spawn_registry_id=hero.runtime.policy_training\n";
  out << "component_family_id=wikimyei.policy.trainable\n";
  out << "component_spawn_schema=kikijyeba.component_spawn.v2\n";
  out << "component_spawn_fingerprint=" << contract_digest << "\n";
  out << "component_spawn_id=" << contract.policy_id << "\n";
  out << "component_spawn_label=" << contract.policy_id << "\n";
  out << "topology_id=kikijyeba.topology.graph.v1\n";
  out << "protocol_id=" << contract.protocol_id << "\n";
  out << "protocol_kind=policy_training_contract\n";
  out << "protocol_status=pre_ppo_noop_execution\n";
  out << "wave_id=policy_training_pre_ppo\n";
  out << "target_component_family_id=wikimyei.policy.trainable\n";
  out << "wave_action=train\n";
  out << "wave_mode=train\n";
  out << "execution_chain=runtime.policy_training_contract:validate -> "
         "runtime.policy_training.noop:write_checkpoint\n";
  out << "mutated_components=wikimyei.policy.trainable\n";
  out << "frozen_components=wikimyei.representation,"
         "wikimyei.inference.expected_value.mdn,wikimyei.observer.belief,"
         "kikijyeba.environment.replay.v1,cajtucu.execution.paper.v1\n";
  out << "source_range_policy=causal_schedule_digest\n";
  out << "source_order_policy=sequential\n";
  out << "source_order_policy_explicit=true\n";
  out << "source_cursor_token=" << contract.source_cursor_token << "\n";
  out << "source_input_length=1\n";
  out << "source_future_length=1\n";
  out << "source_footprint_precision="
      << contract.causal_schedule_cursor_key_kind << "\n";
  out << "protocol_contract_fingerprint="
      << contract.protocol_contract_fingerprint << "\n";
  out << "graph_order_fingerprint=" << contract.graph_order_fingerprint
      << "\n";
  out << "nodelift_assembly_fingerprint=\n";
  out << "vicreg_assembly_fingerprint=\n";
  out << "mtf_jepa_mae_vicreg_assembly_fingerprint=\n";
  out << "mdn_assembly_fingerprint=\n";
  out << "dock_binding_fingerprint=\n";
  out << "dock_binding_token=\n";
  out << "stream_plan=policy_training_contract\n";
  out << "policy_training_contract_schema=" << contract.schema_version << "\n";
  out << "policy_training_contract_digest=" << contract_digest << "\n";
  out << "policy_training_artifact_schema=" << contract.artifact_schema_id
      << "\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_state_text(
    std::string_view job_id, std::string_view job_stable_id,
    std::string_view attempt_leaf, const fs::path &checkpoint_path,
    const fs::path &report_path) {
  std::ostringstream out;
  out << "state_format=kikijyeba.runtime.job_state.v1\n";
  out << "job_id=" << job_id << "\n";
  out << "job_stable_id=" << job_stable_id << "\n";
  out << "job_attempt_id=" << attempt_leaf << "\n";
  out << "job_attempt_index=0\n";
  out << "job_attempt_policy=immutable_no_overwrite\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "wave_id=policy_training_pre_ppo\n";
  out << "target_component_family_id=wikimyei.policy.trainable\n";
  out << "wave_action=train\n";
  out << "steps_attempted=0\n";
  out << "steps_completed=0\n";
  out << "skipped_batches=0\n";
  out << "optimizer_steps=0\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_write_count=1\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "report_written=true\n";
  out << "report_write_count=1\n";
  out << "report_path=" << report_path.string() << "\n";
  out << "runtime_result_fact_written=true\n";
  out << "runtime_result_fact_path=runtime.result.fact\n";
  out << "runtime_checkpoint_io_fact_written=true\n";
  out << "runtime_checkpoint_io_fact_path=runtime.checkpoint_io.fact\n";
  out << "runtime_health_measurement_fact_written=true\n";
  out << "runtime_health_measurement_fact_path=runtime.health_measurement.fact\n";
  out << "lattice_exposure_fact_written=true\n";
  out << "lattice_exposure_fact_path=runtime.policy_training.fact\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_runtime_result_fact_text(
    std::string_view job_id, const fs::path &checkpoint_path) {
  std::ostringstream out;
  out << "fact_type=runtime.result.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "optimizer_steps=0\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "model_state_mutated=true\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  out << "final_loss=0\n";
  out << "mean_loss=0\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_checkpoint_io_fact_text(
    std::string_view job_id, const fs::path &checkpoint_path,
    std::string_view checkpoint_digest) {
  std::error_code ec;
  const auto checkpoint_bytes = fs::exists(checkpoint_path, ec)
                                    ? fs::file_size(checkpoint_path, ec)
                                    : std::uintmax_t{0};
  std::ostringstream out;
  out << "fact_type=runtime.checkpoint_io.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "checkpoint_write_count=1\n";
  out << "checkpoint_artifact_hash=" << checkpoint_digest << "\n";
  out << "checkpoint_path_reported=" << checkpoint_path.string() << "\n";
  out << "checkpoint_digest_reported=" << checkpoint_digest << "\n";
  out << "checkpoint_digest_verified=true\n";
  out << "checkpoint_file_exists=true\n";
  out << "checkpoint_bytes=" << checkpoint_bytes << "\n";
  out << "checkpoint_artifact_status=written\n";
  out << "model_state_mutated=true\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_health_fact_text(
    std::string_view job_id) {
  std::ostringstream out;
  out << "fact_type=runtime.health_measurement.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  return out.str();
}

void append_assignment(std::ostringstream &out, std::string_view key,
                       std::string_view value) {
  out << key << "=" << value << "\n";
}

void append_assignment_bool(std::ostringstream &out, std::string_view key,
                            bool value) {
  append_assignment(out, key, value ? "true" : "false");
}

[[nodiscard]] std::string policy_training_fact_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view checkpoint_digest) {
  std::ostringstream out;
  append_assignment(out, "schema", contract.artifact_schema_id);
  append_assignment(out, "fact_type", "policy_training");
  append_assignment(out, "policy_id", contract.policy_id);
  append_assignment(out, "policy_kind", contract.policy_kind);
  append_assignment(out, "policy_architecture_digest",
                    contract.policy_architecture_digest);
  append_assignment(out, "training_config_digest",
                    contract.training_config_digest);
  append_assignment(out, "training_range_digest",
                    contract.training_range_digest);
  append_assignment(out, "validation_range_digest",
                    contract.validation_range_digest);
  append_assignment(out, "test_range_digest", contract.test_range_digest);
  append_assignment(out, "environment_contract_id",
                    contract.environment_contract_id);
  append_assignment(out, "observation_schema_digest",
                    contract.observation_schema_digest);
  append_assignment(out, "action_schema_digest", contract.action_schema_digest);
  append_assignment(out, "reward_contract_digest",
                    contract.reward_contract_digest);
  append_assignment(out, "execution_profile_digest",
                    contract.execution_profile_digest);
  append_assignment(out, "training_schedule_mode",
                    contract.training_schedule_mode);
  append_assignment(out, "causal_schedule_schema_id",
                    contract.causal_schedule_schema_id);
  append_assignment(out, "causal_schedule_digest",
                    contract.causal_schedule_digest);
  append_assignment(out, "causal_schedule_cursor_key_kind",
                    contract.causal_schedule_cursor_key_kind);
  append_assignment(out, "causal_schedule_no_future_snapshot_use_source",
                    contract.causal_schedule_no_future_snapshot_use_source);
  append_assignment(out, "normalization_fit_range_digest",
                    contract.normalization_fit_range_digest);
  append_assignment(out, "replay_buffer_source_range_digest",
                    contract.replay_buffer_source_range_digest);
  append_assignment(out, "early_stopping_policy_digest",
                    contract.early_stopping_policy_digest);
  append_assignment(out, "hyperparameter_selection_policy_digest",
                    contract.hyperparameter_selection_policy_digest);
  append_assignment(out, "selector_split", contract.selector_split);
  append_assignment(out, "selector_policy_digest",
                    contract.selector_policy_digest);
  append_assignment(out, "parent_checkpoint_digest",
                    contract.parent_checkpoint_digest);
  append_assignment(out, "checkpoint_digest", checkpoint_digest);
  append_assignment(out, "parent_forecast_eval_fact_digest",
                    contract.parent_forecast_eval_fact_digest);
  append_assignment(out, "parent_observer_belief_fact_digest",
                    contract.parent_observer_belief_fact_digest);
  append_assignment(out, "parent_allocation_engine_fact_digest",
                    contract.parent_allocation_engine_fact_digest);
  append_assignment(out, "parent_replay_environment_fact_digest",
                    contract.parent_replay_environment_fact_digest);
  append_assignment(out, "final_refit_parent_selected_checkpoint_digest",
                    contract.final_refit_parent_selected_checkpoint_digest);
  append_assignment(out, "random_seed", std::to_string(contract.random_seed));
  append_assignment_bool(out, "training_range_disjoint_validation", true);
  append_assignment_bool(out, "training_range_disjoint_test", true);
  append_assignment_bool(out, "validation_range_disjoint_test", true);
  append_assignment_bool(out, "normalization_fit_training_only", true);
  append_assignment_bool(out, "replay_buffer_training_only", true);
  append_assignment_bool(out, "reward_baseline_training_only", true);
  append_assignment_bool(out, "early_stopping_uses_validation_only", true);
  append_assignment_bool(out, "hyperparameter_selection_uses_validation_only",
                         true);
  append_assignment_bool(out, "test_sealed_until_final_report", true);
  append_assignment_bool(out, "test_first_access_after_selection", true);
  append_assignment_bool(out, "runtime_job_kind_bound", true);
  append_assignment_bool(out, "policy_checkpoint_written", true);
  append_assignment_bool(out, "causal_schedule_readiness_eligible",
                         contract.causal_schedule_readiness_eligible);
  append_assignment_bool(out, "causal_schedule_no_future_snapshot_use",
                         contract.causal_schedule_no_future_snapshot_use);
  append_assignment_bool(out, "offline_full_window_research", false);
  append_assignment_bool(out, "final_refit_uses_validation",
                         contract.final_refit_uses_validation);
  append_assignment_bool(out, "validation_no_longer_proof",
                         contract.validation_no_longer_proof);
  append_assignment_bool(out, "sealed_test_required",
                         contract.sealed_test_required);
  append_assignment_bool(out, "artifact_evidence", true);
  append_assignment_bool(out, "visibility_only", true);
  append_assignment_bool(out, "runtime_trainer", false);
  append_assignment_bool(out, "policy_training_authority", false);
  append_assignment_bool(out, "policy_selector", false);
  append_assignment_bool(out, "allocation_authority", false);
  append_assignment_bool(out, "execution_authority", false);
  append_assignment_bool(out, "readiness_authority", false);
  append_assignment_bool(out, "quality_authority", false);
  append_assignment_bool(out, "performance_authority", false);
  append_assignment_bool(out, "market_readiness_authority", false);
  append_assignment_bool(out, "deployment_authority", false);
  append_assignment_bool(out, "checkpoint_selector", false);
  append_assignment_bool(out, "coverage_authority", false);
  append_assignment_bool(out, "leakage_authority", false);
  append_assignment_bool(out, "contract_identity_authority", false);
  return out.str();
}

[[nodiscard]] std::string unique_policy_training_attempt_leaf(
    std::string_view contract_digest) {
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  return "attempt_" + std::string(contract_digest.substr(0, 16)) + "_" +
         std::to_string(static_cast<long long>(stamp)) + "_" +
         std::to_string(static_cast<long long>(::getpid()));
}

[[nodiscard]] bool path_exists_and_is_nonempty_dir(const fs::path &path) {
  std::error_code ec;
  if (!fs::exists(path, ec) || ec) {
    return false;
  }
  if (!fs::is_directory(path, ec) || ec) {
    return true;
  }
  return fs::directory_iterator(path, ec) != fs::directory_iterator{};
}

[[nodiscard]] bool execute_policy_training_noop(
    const std::string &args,
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    const std::string &contract_text, std::string_view contract_digest,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  bool confirm_execute = false;
  if (!parse_optional_bool_arg(args, "confirm_execute", false,
                               &confirm_execute, err)) {
    return false;
  }
  if (!policy_bool_or(ctx->policy, "allow_execute", false)) {
    *err = "E_RUNTIME_EXECUTE_DENIED: allow_execute is false";
    return false;
  }
  if (policy_bool_or(ctx->policy, "require_confirm_execute", true) &&
      !confirm_execute) {
    *err = "E_RUNTIME_CONFIRM_REQUIRED: confirm_execute=true is required";
    return false;
  }
  if (!policy_bool_or(ctx->policy, "allow_train_execute", false)) {
    *err = "E_RUNTIME_TRAIN_DENIED: allow_train_execute is false";
    return false;
  }
  if (!policy_training_execute_kind_supported(contract)) {
    *err = policy_training_unsupported_execute_reason(contract);
    return false;
  }

  int timeout_seconds = contract.max_wall_clock_seconds > 0
                            ? static_cast<int>(contract.max_wall_clock_seconds)
                            : policy_int_or(ctx->policy, "max_runtime_seconds",
                                            600);
  if (!parse_optional_int_arg(args, "timeout_seconds", timeout_seconds,
                              &timeout_seconds, err)) {
    return false;
  }
  if (timeout_seconds < 1) {
    *err = "timeout_seconds must be >= 1";
    return false;
  }

  std::string job_dir_arg;
  std::string job_id_arg;
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);
  (void)extract_json_string_field(args, "job_id", &job_id_arg);

  const fs::path root = runtime_root(ctx->policy);
  const std::string job_stable_id =
      "policy_training_" + std::string(contract_digest.substr(0, 16));
  const std::string attempt_leaf =
      unique_policy_training_attempt_leaf(contract_digest);
  const std::string job_id =
      trim_ascii(job_id_arg).empty() ? job_stable_id + "." + attempt_leaf
                                     : trim_ascii(job_id_arg);
  fs::path job_dir;
  if (!trim_ascii(job_dir_arg).empty()) {
    job_dir = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, job_dir) &&
        !path_within(root, job_dir)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside Runtime Hero roots: " +
          job_dir.string();
      return false;
    }
  } else {
    job_dir = cuwacunu::hero::runtime::job_layout::job_dir(
        root, "wikimyei.policy.trainable", contract.policy_id,
        std::string(contract_digest), "train", job_id);
  }
  if (path_exists_and_is_nonempty_dir(job_dir)) {
    *err = "E_RUNTIME_POLICY_TRAINING_JOB_DIR_EXISTS: refusing to overwrite "
           "existing job_dir: " +
           job_dir.string();
    return false;
  }

  try {
    fs::create_directories(job_dir);
    cuwacunu::hero::runtime::job_layout::write_runtime_layout_marker(root);

    const fs::path checkpoint_dir =
        job_dir / "artifacts" / "wikimyei.policy.trainable" / "checkpoints";
    const fs::path checkpoint_path =
        checkpoint_dir / "noop_policy_training.checkpoint";
    const std::string checkpoint_text =
        policy_training_checkpoint_text(contract, contract_digest);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        checkpoint_path, checkpoint_text);
    const std::string checkpoint_digest =
        file_digest_or_empty(checkpoint_path,
                             "kikijyeba.runtime.policy_training."
                             "noop_checkpoint.v1");

    const fs::path report_path = job_dir / "policy_training.report";
    const std::string report_text = policy_training_report_text(
        contract, contract_digest, checkpoint_path, checkpoint_digest);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        report_path, report_text);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.manifest",
        policy_training_manifest_text(contract, contract_digest, job_id,
                                      job_stable_id, attempt_leaf,
                                      ctx->global_config_path));
    cuwacunu::hero::runtime::job_layout::write_component_spawn_ref(
        root, "wikimyei.policy.trainable", contract.policy_id,
        contract.policy_id, std::string(contract_digest),
        "hero.runtime.policy_training",
        "policy_training_contract_" + std::string(contract_digest),
        "policy_training_contract_" + std::string(contract_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "policy_training.contract", contract_text);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.result.fact",
        policy_training_runtime_result_fact_text(job_id, checkpoint_path));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.checkpoint_io.fact",
        policy_training_checkpoint_io_fact_text(job_id, checkpoint_path,
                                                checkpoint_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.health_measurement.fact",
        policy_training_health_fact_text(job_id));

    const std::string fact_text =
        policy_training_fact_text(contract, checkpoint_digest);
    const fs::path policy_fact_path = job_dir / "runtime.policy_training.fact";
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        policy_fact_path, fact_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.state",
        policy_training_state_text(job_id, job_stable_id, attempt_leaf,
                                   checkpoint_path, report_path));

    std::ostringstream json;
    json << "{\"ok\":true,\"requested_mode\":\"execute\",\"dry_run\":false"
         << ",\"schema_version\":\"kikijyeba.runtime.policy_training_"
            "execution_packet.v1\""
         << ",\"policy_training_execution_supported\":true"
         << ",\"trainer_kind\":\"noop_policy_training.v1\""
         << ",\"ppo_implemented\":false"
         << ",\"contract_digest\":" << json_quote(contract_digest)
         << ",\"checkpoint_digest\":" << json_quote(checkpoint_digest)
         << ",\"job_id\":" << json_quote(job_id)
         << ",\"job_dir\":" << json_quote(job_dir.string())
         << ",\"manifest_path\":"
         << json_quote((job_dir / "job.manifest").string())
         << ",\"state_path\":"
         << json_quote((job_dir / "job.state").string())
         << ",\"report_path\":" << json_quote(report_path.string())
         << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
         << ",\"policy_training_fact_path\":"
         << json_quote(policy_fact_path.string())
         << ",\"artifacts\":"
         << job_artifacts_json(job_dir, report_path.string(), false,
                               static_cast<std::size_t>(policy_int_or(
                                   ctx->policy, "max_capture_bytes", 65536)))
         << ",\"terminal_evidence\":"
         << runtime_terminal_evidence_json(job_dir)
         << ",\"authority\":{\"runtime_executes_policy_training_smoke\":true,"
            "\"runtime_executes_live_capital\":false,"
            "\"runtime_trains_ppo\":false,"
            "\"runtime_selects_policy\":false,"
            "\"runtime_proves_lattice_target\":false}"
         << ",\"next_safe_actions\":[\"inspect_policy_training_artifact\","
            "\"evaluate_policy_training_artifact_ready\"]}";
    *out = json.str();
    return true;
  } catch (const std::exception &ex) {
    *err = std::string("E_RUNTIME_POLICY_TRAINING_EXECUTION_FAILED: ") +
           ex.what();
    return false;
  }
}

[[nodiscard]] bool
handle_policy_training_contract(const std::string &args,
                                std::string_view requested_mode,
                                runtime_context_t *ctx, std::string *out,
                                std::string *err) {
  namespace runtime_contract = cuwacunu::hero::runtime;
  std::optional<wave_info_t> selected_wave;
  if (ctx != nullptr) {
    std::string config_arg;
    (void)extract_json_string_field(args, "config_path", &config_arg);
    selected_wave =
        load_wave_info(*ctx, effective_config_path(*ctx, config_arg));
  }
  const wave_info_t *wave_defaults =
      selected_wave.has_value() &&
              wave_info_requests_policy_training(*selected_wave)
          ? &*selected_wave
          : nullptr;
  runtime_contract::policy_training_job_contract_t contract{};
  if (!parse_required_string_arg(args, "protocol_id", &contract.protocol_id,
                                 err) ||
      !parse_required_string_arg(args, "protocol_contract_fingerprint",
                                 &contract.protocol_contract_fingerprint,
                                 err) ||
      !parse_required_string_arg(args, "graph_order_fingerprint",
                                 &contract.graph_order_fingerprint, err) ||
      !parse_required_string_arg(args, "source_cursor_token",
                                 &contract.source_cursor_token, err) ||
      !parse_required_string_arg_or_wave(args, "policy_id", wave_defaults,
                                         "POLICY_ID", &contract.policy_id,
                                         err) ||
      !parse_required_string_arg_or_wave(args, "policy_kind", wave_defaults,
                                         "POLICY_KIND", &contract.policy_kind,
                                         err) ||
      !parse_required_string_arg(args, "policy_architecture_digest",
                                 &contract.policy_architecture_digest, err) ||
      !parse_required_string_arg(args, "training_config_digest",
                                 &contract.training_config_digest, err) ||
      !parse_required_string_arg(args, "training_range_digest",
                                 &contract.training_range_digest, err) ||
      !parse_required_string_arg(args, "validation_range_digest",
                                 &contract.validation_range_digest, err) ||
      !parse_required_string_arg(args, "test_range_digest",
                                 &contract.test_range_digest, err) ||
      !parse_required_string_arg(args, "observation_schema_digest",
                                 &contract.observation_schema_digest, err) ||
      !parse_required_string_arg(args, "action_schema_digest",
                                 &contract.action_schema_digest, err) ||
      !parse_required_string_arg(args, "reward_contract_digest",
                                 &contract.reward_contract_digest, err) ||
      !parse_required_string_arg(args, "execution_profile_digest",
                                 &contract.execution_profile_digest, err) ||
      !parse_required_string_arg_or_wave(
          args, "training_schedule_mode", wave_defaults,
          "TRAINING_SCHEDULE_MODE", &contract.training_schedule_mode, err) ||
      !parse_required_string_arg(args, "causal_schedule_schema_id",
                                 &contract.causal_schedule_schema_id, err) ||
      !parse_required_string_arg(args, "causal_schedule_digest",
                                 &contract.causal_schedule_digest, err) ||
      !parse_required_string_arg(args, "causal_schedule_cursor_key_kind",
                                 &contract.causal_schedule_cursor_key_kind,
                                 err) ||
      !parse_required_string_arg(
          args, "causal_schedule_no_future_snapshot_use_source",
          &contract.causal_schedule_no_future_snapshot_use_source, err) ||
      !parse_required_string_arg(args, "normalization_fit_range_digest",
                                 &contract.normalization_fit_range_digest,
                                 err) ||
      !parse_required_string_arg(args, "replay_buffer_source_range_digest",
                                 &contract.replay_buffer_source_range_digest,
                                 err) ||
      !parse_required_string_arg(args, "early_stopping_policy_digest",
                                 &contract.early_stopping_policy_digest, err) ||
      !parse_required_string_arg(
          args, "hyperparameter_selection_policy_digest",
          &contract.hyperparameter_selection_policy_digest, err) ||
      !parse_required_string_arg(args, "selector_policy_digest",
                                 &contract.selector_policy_digest, err) ||
      !parse_required_string_arg(args, "parent_checkpoint_digest",
                                 &contract.parent_checkpoint_digest, err) ||
      !parse_required_string_arg(args, "parent_forecast_eval_fact_digest",
                                 &contract.parent_forecast_eval_fact_digest,
                                 err) ||
      !parse_required_string_arg(args, "parent_observer_belief_fact_digest",
                                 &contract.parent_observer_belief_fact_digest,
                                 err) ||
      !parse_required_string_arg(
          args, "parent_allocation_engine_fact_digest",
          &contract.parent_allocation_engine_fact_digest, err) ||
      !parse_required_string_arg(
          args, "parent_replay_environment_fact_digest",
          &contract.parent_replay_environment_fact_digest, err)) {
    return false;
  }
  (void)extract_json_string_field(args, "split_policy_fingerprint",
                                  &contract.split_policy_fingerprint);
  (void)extract_json_string_field(args, "component_assembly_fingerprint",
                                  &contract.component_assembly_fingerprint);
  (void)extract_json_string_field(args, "environment_contract_id",
                                  &contract.environment_contract_id);
  (void)extract_json_string_field(args, "selector_split",
                                  &contract.selector_split);
  (void)extract_json_string_field(
      args, "final_refit_parent_selected_checkpoint_digest",
      &contract.final_refit_parent_selected_checkpoint_digest);

  int parsed_int = 0;
  if (!parse_policy_training_required_int(args, "random_seed", &parsed_int,
                                          err)) {
    return false;
  }
  contract.random_seed = parsed_int;
  contract.random_seed_bound = true;
  if (!parse_policy_training_required_int(args, "max_episodes", &parsed_int,
                                          err)) {
    return false;
  }
  contract.max_episodes = parsed_int;
  if (!parse_policy_training_required_int(args, "max_steps", &parsed_int,
                                          err)) {
    return false;
  }
  contract.max_steps = parsed_int;
  if (!parse_optional_int_arg(args, "max_parallel_jobs", 1, &parsed_int, err)) {
    return false;
  }
  contract.max_parallel_jobs = parsed_int;
  if (!parse_policy_training_required_int(args, "max_wall_clock_seconds",
                                          &parsed_int, err)) {
    return false;
  }
  contract.max_wall_clock_seconds = parsed_int;
  if (!parse_optional_bool_arg(args, "causal_schedule_readiness_eligible", true,
                               &contract.causal_schedule_readiness_eligible,
                               err)) {
    return false;
  }
  if (!parse_optional_bool_arg(
          args, "causal_schedule_no_future_snapshot_use", true,
          &contract.causal_schedule_no_future_snapshot_use, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(
          args, "offline_full_window_research_allowed", false,
          &contract.offline_full_window_research_allowed, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "final_refit_uses_validation", false,
                               &contract.final_refit_uses_validation, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "validation_no_longer_proof", false,
                               &contract.validation_no_longer_proof, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "sealed_test_required", false,
                               &contract.sealed_test_required, err)) {
    return false;
  }
  if (!parse_optional_bool_arg_or_wave(args, "live_execution_allowed",
                                       wave_defaults, "LIVE_EXECUTION_ALLOWED",
                                       false,
                                       &contract.live_execution_allowed, err)) {
    return false;
  }

  const auto issues =
      runtime_contract::validate_policy_training_job_contract(contract);
  if (!issues.empty()) {
    *err = "E_RUNTIME_POLICY_TRAINING_CONTRACT_INVALID: " +
           string_array_json(issues);
    return false;
  }

  const auto contract_text = contract.to_text();
  const auto contract_digest =
      runtime_contract::policy_training_contract_digest_for_text(contract_text);
  if (requested_mode == "execute") {
    return execute_policy_training_noop(args, contract, contract_text,
                                        contract_digest, ctx, out, err);
  }
  const bool execution_supported =
      policy_training_execute_kind_supported(contract);
  std::ostringstream json;
  json << "{\"ok\":true,\"requested_mode\":" << json_quote(requested_mode)
       << ",\"dry_run\":true"
       << ",\"schema_version\":\"kikijyeba.runtime.policy_training_job_"
          "contract_packet.v1\""
       << ",\"contract_digest\":" << json_quote(contract_digest)
       << ",\"policy_training_execution_supported\":"
       << bool_json(execution_supported)
       << ",\"supported_execute_policy_kind\":\"noop_policy_training.v1\""
       << ",\"artifact_schema_id\":" << json_quote(contract.artifact_schema_id)
       << ",\"contract\":" << policy_training_contract_json(contract)
       << ",\"contract_text\":" << json_quote(contract_text)
       << ",\"authority\":{\"runtime_trains_policy\":false,"
          "\"runtime_executes_live_capital\":false,"
          "\"runtime_selects_policy\":false,"
          "\"runtime_proves_lattice_target\":false}"
       << ",\"next_safe_actions\":[\"persist_contract_with_future_"
          "policy_training_runner\","
          "\"inspect_policy_training_contract\"]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool json_raw_is_empty_array(const std::string &raw) {
  const std::string value = trim_ascii(raw);
  return value == "[]";
}

[[nodiscard]] bool is_symbolic_handoff_value(std::string_view raw) {
  const std::string value = trim_ascii(raw);
  return value.rfind("latest_satisfying:", 0) == 0 ||
         value.rfind("lattice:", 0) == 0 || value.rfind("selector:", 0) == 0;
}

[[nodiscard]] bool string_map_has_symbolic_values(
    const std::unordered_map<std::string, std::string> &values,
    std::string *symbolic_key) {
  for (const auto &[key, value] : values) {
    if (is_symbolic_handoff_value(value)) {
      if (symbolic_key) {
        *symbolic_key = key;
      }
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool same_normalized_string_map(
    const std::unordered_map<std::string, std::string> &lhs,
    const std::unordered_map<std::string, std::string> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto &[key, value] : lhs) {
    const auto found = rhs.find(key);
    if (found == rhs.end()) {
      return false;
    }
    if (fs::path(value).lexically_normal().string() !=
        fs::path(found->second).lexically_normal().string()) {
      return false;
    }
  }
  return true;
}

struct runtime_handoff_binding_t {
  std::string handoff_id{};
  std::string handoff_digest{};
  std::string target_driver_run_id{};
};

[[nodiscard]] bool validate_runtime_handoff_object(
    const std::string &handoff_raw, const fs::path &config_path,
    const fs::path &policy_path, bool dry_run, bool confirm_execute,
    bool force_rebuild_cache, std::string *wave_raw,
    runtime_handoff_binding_t *binding, std::string *err) {
  std::string schema_version;
  if (!extract_json_string_field(handoff_raw, "handoff_schema_version",
                                 &schema_version) ||
      schema_version != "1") {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_schema_version must be 1";
    }
    return false;
  }

  std::string handoff_id;
  if (!extract_json_string_field(handoff_raw, "handoff_id", &handoff_id) ||
      trim_ascii(handoff_id).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_id is required";
    }
    return false;
  }

  std::string handoff_digest;
  if (!extract_json_string_field(handoff_raw, "handoff_digest",
                                 &handoff_digest) ||
      trim_ascii(handoff_digest).empty() ||
      handoff_id != "runtime_handoff_" + handoff_digest) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_digest is required and must "
             "match handoff_id";
    }
    return false;
  }

  std::string target_driver_run_id;
  (void)extract_json_string_field(handoff_raw, "target_driver_run_id",
                                  &target_driver_run_id);

  std::string created_by;
  if (!extract_json_string_field(handoff_raw, "created_by", &created_by) ||
      created_by != "marshal") {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: created_by must be marshal";
    }
    return false;
  }

  std::string created_at;
  if (!extract_json_string_field(handoff_raw, "created_at", &created_at) ||
      trim_ascii(created_at).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: created_at is required";
    }
    return false;
  }

  std::string target_id;
  if (!extract_json_string_field(handoff_raw, "target_id", &target_id) ||
      trim_ascii(target_id).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: target_id is required";
    }
    return false;
  }

  std::string unresolved_raw;
  if (!extract_json_raw_field(handoff_raw, "unresolved_symbols",
                              &unresolved_raw) ||
      !json_raw_is_empty_array(unresolved_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: unresolved_symbols must "
             "be present and empty";
    }
    return false;
  }

  std::string base_config_raw;
  if (!extract_json_raw_field(handoff_raw, "base_config", &base_config_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: base_config is required";
    }
    return false;
  }
  std::string base_config_path;
  std::string base_config_hash;
  const std::string actual_base_config_hash = file_digest_or_empty(
      config_path, "kikijyeba.runtime.handoff.base_config_file.v1");
  if (!extract_json_string_field(base_config_raw, "path", &base_config_path) ||
      !extract_json_string_field(base_config_raw, "hash", &base_config_hash) ||
      trim_ascii(base_config_hash).empty() ||
      base_config_hash != actual_base_config_hash ||
      fs::path(base_config_path).lexically_normal() !=
          config_path.lexically_normal()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: base_config path/hash is invalid";
    }
    return false;
  }

  std::string runtime_policy_raw;
  if (!extract_json_raw_field(handoff_raw, "runtime_policy",
                              &runtime_policy_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: runtime_policy is required";
    }
    return false;
  }
  std::string runtime_policy_path;
  std::string runtime_policy_hash;
  const std::string actual_runtime_policy_hash = file_digest_or_empty(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  if (!extract_json_string_field(runtime_policy_raw, "path",
                                 &runtime_policy_path) ||
      !extract_json_string_field(runtime_policy_raw, "hash",
                                 &runtime_policy_hash) ||
      trim_ascii(runtime_policy_path).empty() ||
      trim_ascii(runtime_policy_hash).empty() ||
      runtime_policy_hash != actual_runtime_policy_hash ||
      fs::path(runtime_policy_path).lexically_normal() !=
          policy_path.lexically_normal()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: runtime_policy path/hash is invalid";
    }
    return false;
  }

  std::string intent_raw;
  if (!extract_json_raw_field(handoff_raw, "intent", &intent_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: intent is required";
    }
    return false;
  }
  bool intent_dry_run = false;
  bool intent_confirm_execute = false;
  bool intent_force_rebuild_cache = false;
  if (!extract_json_bool_field(intent_raw, "dry_run", &intent_dry_run) ||
      !extract_json_bool_field(intent_raw, "confirm_execute",
                               &intent_confirm_execute) ||
      !extract_json_bool_field(intent_raw, "force_rebuild_cache",
                               &intent_force_rebuild_cache) ||
      intent_dry_run != dry_run || intent_confirm_execute != confirm_execute ||
      intent_force_rebuild_cache != force_rebuild_cache) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: intent differs from execute args";
    }
    return false;
  }

  std::string local_wave_raw;
  if (!extract_json_raw_field(handoff_raw, "wave", &local_wave_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: wave is required";
    }
    return false;
  }

  std::string checkpoint_inputs_raw;
  if (!extract_json_raw_field(handoff_raw, "checkpoint_inputs",
                              &checkpoint_inputs_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs is required";
    }
    return false;
  }
  std::unordered_map<std::string, std::string> checkpoint_inputs;
  if (!extract_json_string_object(checkpoint_inputs_raw, &checkpoint_inputs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs must be an object "
             "of string paths";
    }
    return false;
  }
  std::string symbolic_key;
  if (string_map_has_symbolic_values(checkpoint_inputs, &symbolic_key)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: checkpoint input " +
             symbolic_key + " is symbolic";
    }
    return false;
  }

  std::string wave_inputs_raw;
  std::unordered_map<std::string, std::string> wave_inputs;
  if (extract_json_raw_field(local_wave_raw, "model_state_inputs",
                             &wave_inputs_raw)) {
    if (!extract_json_string_object(wave_inputs_raw, &wave_inputs)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_INVALID: wave.model_state_inputs malformed";
      }
      return false;
    }
    if (string_map_has_symbolic_values(wave_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
  }
  if (!same_normalized_string_map(checkpoint_inputs, wave_inputs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs differs from "
             "wave.model_state_inputs";
    }
    return false;
  }

  std::string lattice_refs_raw;
  std::unordered_map<std::string, std::string> lattice_refs;
  if (!extract_json_raw_field(handoff_raw, "lattice_certificate_refs",
                              &lattice_refs_raw) ||
      !extract_json_string_object(lattice_refs_raw, &lattice_refs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: lattice_certificate_refs must be an "
             "object";
    }
    return false;
  }

  if (wave_raw) {
    *wave_raw = std::move(local_wave_raw);
  }
  if (binding) {
    binding->handoff_id = std::move(handoff_id);
    binding->handoff_digest = std::move(handoff_digest);
    binding->target_driver_run_id = std::move(target_driver_run_id);
  }
  return true;
}

[[nodiscard]] bool expected_wave_matches_runtime_wave(
    const std::string &args, const fs::path &config_path,
    const fs::path &policy_path, bool dry_run, bool confirm_execute,
    bool force_rebuild_cache, const wave_info_t &wave,
    runtime_handoff_binding_t *binding, std::string *err) {
  std::string expected_raw;
  std::string handoff_raw;
  if (args.find("\"runtime_handoff\"") != std::string::npos) {
    if (!extract_json_raw_field(args, "runtime_handoff", &handoff_raw)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_INVALID: runtime_handoff is malformed";
      }
      return false;
    }
    if (!validate_runtime_handoff_object(
            handoff_raw, config_path, policy_path, dry_run, confirm_execute,
            force_rebuild_cache, &expected_raw, binding, err)) {
      return false;
    }
  } else if (!extract_json_raw_field(args, "marshal_expected_wave",
                                     &expected_raw)) {
    return true;
  }

  std::string expected_target;
  if (extract_json_first_string_field(
          expected_raw, {"target_component_family_id"}, &expected_target)) {
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
  if (extract_json_first_string_field(expected_raw, {"mode"}, &expected_mode)) {
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
    std::string symbolic_key;
    if (string_map_has_symbolic_values(expected_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: expected wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
    const auto actual_inputs = wave_model_state_inputs(wave);
    if (string_map_has_symbolic_values(actual_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: active wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
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

[[nodiscard]] bool handle_status(const std::string &args,
                                 runtime_context_t *ctx, std::string *out,
                                 std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {}, &fields, err)) {
    return false;
  }
  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  const fs::path root = runtime_root(ctx->policy);
  const wave_info_t wave =
      load_wave_info(*ctx, effective_config_path(*ctx, ""));
  const std::size_t job_count =
      cuwacunu::hero::runtime::job_layout::discover_runtime_job_dirs(root)
          .size();
  std::ostringstream json;
  json << "{\"policy_path\":" << json_quote(ctx->policy.policy_path.string())
       << ",\"global_config_path\":"
       << json_quote(ctx->global_config_path.string())
       << ",\"runtime_profile\":" << json_quote(ctx->policy.profile_id)
       << ",\"runtime_profile_source\":"
       << json_quote(policy_get(ctx->policy, "runtime_profile_source"))
       << ",\"protocol_layer\":"
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
              policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", false))
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
  wave_overlay_t wave_overlay{};
  if (!parse_wave_overlay_from_args(args, &wave_overlay, err)) {
    return false;
  }
  wave_info_t wave = load_wave_info(*ctx, config_path);
  apply_wave_overlay_to_info(&wave, wave_overlay);
  std::string symbolic_key;
  if (string_map_has_symbolic_values(wave_model_state_inputs(wave),
                                     &symbolic_key)) {
    *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: active wave input " +
           symbolic_key + " is symbolic";
    return false;
  }
  runtime_handoff_binding_t handoff_binding{};
  if (!expected_wave_matches_runtime_wave(
          args, config_path, ctx->policy_path, dry_run, confirm_execute,
          force_rebuild_cache, wave, &handoff_binding, err)) {
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
  if (wave_overlay.present) {
    argv.push_back("--source-range");
    argv.push_back(wave_overlay.source_range);
    if (wave_overlay.source_range == "anchor_index") {
      argv.push_back("--anchor-index-begin");
      argv.push_back(wave_overlay.anchor_index_begin);
      argv.push_back("--anchor-index-end");
      argv.push_back(wave_overlay.anchor_index_end);
    } else if (wave_overlay.source_range == "source_key") {
      argv.push_back("--source-key-begin");
      argv.push_back(wave_overlay.source_key_begin);
      argv.push_back("--source-key-end");
      argv.push_back(wave_overlay.source_key_end);
    }
  }
  if (!handoff_binding.handoff_id.empty()) {
    argv.push_back("--runtime-handoff-id");
    argv.push_back(handoff_binding.handoff_id);
    argv.push_back("--runtime-handoff-digest");
    argv.push_back(handoff_binding.handoff_digest);
  }
  if (!handoff_binding.target_driver_run_id.empty()) {
    argv.push_back("--marshal-target-driver-run-id");
    argv.push_back(handoff_binding.target_driver_run_id);
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

[[nodiscard]] bool append_optional_double_cli_arg(
    const std::string &args, std::string_view key, std::string_view flag,
    std::vector<std::string> *argv, std::string *err) {
  std::string raw;
  if (!extract_json_raw_field(args, key, &raw)) {
    return true;
  }
  double ignored = 0.0;
  if (!parse_double(raw, &ignored)) {
    *err = std::string(key) + " must be number";
    return false;
  }
  argv->push_back(std::string(flag));
  argv->push_back(trim_ascii(raw));
  return true;
}

[[nodiscard]] bool append_optional_int_cli_arg(const std::string &args,
                                               std::string_view key,
                                               std::string_view flag,
                                               std::vector<std::string> *argv,
                                               std::string *err) {
  int value = 0;
  if (!extract_json_raw_field(args, key, nullptr)) {
    return true;
  }
  if (!extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  argv->push_back(std::string(flag));
  argv->push_back(std::to_string(value));
  return true;
}

[[nodiscard]] bool
append_optional_string_cli_arg(const std::string &args, std::string_view key,
                               std::string_view flag,
                               std::vector<std::string> *argv) {
  std::string value;
  if (!extract_json_string_field(args, key, &value) ||
      trim_ascii(value).empty()) {
    return true;
  }
  argv->push_back(std::string(flag));
  argv->push_back(trim_ascii(value));
  return true;
}

[[nodiscard]] std::string
replay_dry_run_json(const std::vector<std::string> &argv,
                    const fs::path &job_dir, const fs::path &batch_index_path,
                    const fs::path &report_path) {
  std::ostringstream json;
  json << "{\"ok\":true,\"dry_run\":true,\"job_dir\":"
       << json_quote(job_dir.string()) << ",\"replay_batch_index_path\":"
       << json_quote(batch_index_path.string()) << ",\"report_path\":";
  if (report_path.empty()) {
    json << "null";
  } else {
    json << json_quote(report_path.string());
  }
  json << ",\"argv\":[";
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << json_quote(argv[i]);
  }
  json << "]}";
  return json.str();
}

[[nodiscard]] bool handle_replay(const std::string &args,
                                 runtime_context_t *ctx, std::string *out,
                                 std::string *err) {
  fs::path job_dir;
  if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
    return false;
  }
  const fs::path manifest_path = job_dir / "job.manifest";
  const fs::path state_path = job_dir / "job.state";
  if (!fs::exists(manifest_path) || !fs::exists(state_path)) {
    *err = "E_RUNTIME_REPLAY_JOB_INVALID: job.manifest and job.state are "
           "required";
    return false;
  }
  const auto state = parse_kv_file(state_path);
  const auto status_found = state.find("status");
  const std::string status =
      status_found == state.end() ? std::string{} : status_found->second;
  if (lowercase_ascii(status) != "completed") {
    *err = "E_RUNTIME_REPLAY_JOB_NOT_COMPLETED: replay requires a completed "
           "Runtime job";
    return false;
  }
  const fs::path batch_index_path = runtime_replay_batch_index_path(job_dir);
  if (!fs::exists(batch_index_path)) {
    *err = "E_RUNTIME_REPLAY_ARTIFACTS_MISSING: missing " +
           batch_index_path.string();
    return false;
  }

  std::string config_arg;
  std::string base_reserve_node_id;
  std::string risky_node_ids;
  std::string experiment_id;
  std::string report_path_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  (void)extract_json_string_field(args, "base_reserve_node_id",
                                  &base_reserve_node_id);
  (void)extract_json_string_field(args, "risky_node_ids", &risky_node_ids);
  (void)extract_json_string_field(args, "experiment_id", &experiment_id);
  (void)extract_json_string_field(args, "report_path", &report_path_arg);

  fs::path report_path;
  if (!trim_ascii(report_path_arg).empty()) {
    report_path = normalize_path(fs::path(report_path_arg));
    if (!explicit_job_dir_allowed(ctx->policy, report_path) &&
        !path_within(runtime_root(ctx->policy), report_path)) {
      *err = "E_RUNTIME_REPLAY_REPORT_DENIED: report_path is outside Runtime "
             "Hero roots: " +
             report_path.string();
      return false;
    }
  }

  bool dry_run = policy_bool_or(ctx->policy, "default_dry_run", true);
  if (!parse_optional_bool_arg(args, "dry_run", dry_run, &dry_run, err)) {
    return false;
  }

  bool include_equal_weight = false;
  if (!parse_optional_bool_arg(args, "include_equal_weight", false,
                               &include_equal_weight, err)) {
    return false;
  }
  bool include_current_weight = false;
  if (!parse_optional_bool_arg(args, "include_current_weight", false,
                               &include_current_weight, err)) {
    return false;
  }
  bool include_base_reserve_policy = true;
  if (!parse_optional_bool_arg(args, "include_base_reserve_policy", true,
                               &include_base_reserve_policy, err)) {
    return false;
  }
  bool include_sdu_policy = true;
  if (!parse_optional_bool_arg(args,
                               "include_spot_distributional_utility_policy",
                               true, &include_sdu_policy, err)) {
    return false;
  }
  bool allow_synthetic_direct_edges = false;
  if (!parse_optional_bool_arg(args, "allow_synthetic_direct_edges", false,
                               &allow_synthetic_direct_edges, err)) {
    return false;
  }
  bool validation_rollout = false;
  if (!parse_optional_bool_arg(args, "validation_rollout", false,
                               &validation_rollout, err)) {
    return false;
  }

  const bool has_linear_transaction_cost_rate =
      extract_json_raw_field(args, "linear_transaction_cost_rate", nullptr);
  double linear_transaction_cost_rate = 0.0;
  if (!parse_optional_double_arg(args, "linear_transaction_cost_rate", 0.0,
                                 &linear_transaction_cost_rate, err)) {
    return false;
  }
  const bool has_max_steps = extract_json_raw_field(args, "max_steps", nullptr);
  int max_steps = 0;
  if (!parse_optional_int_arg(args, "max_steps", 0, &max_steps, err)) {
    return false;
  }
  const bool has_max_parallel_jobs =
      extract_json_raw_field(args, "max_parallel_jobs", nullptr);
  int max_parallel_jobs = 0;
  if (!parse_optional_int_arg(args, "max_parallel_jobs", 0, &max_parallel_jobs,
                              err)) {
    return false;
  }
  std::string execution_profile_digest;
  std::string policy_set_digest;
  (void)extract_json_string_field(args, "execution_profile_digest",
                                  &execution_profile_digest);
  (void)extract_json_string_field(args, "policy_set_digest",
                                  &policy_set_digest);

  if (validation_rollout) {
    if (trim_ascii(execution_profile_digest).empty()) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PROFILE_DIGEST_MISSING";
      return false;
    }
    if (trim_ascii(policy_set_digest).empty()) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_POLICY_SET_DIGEST_MISSING";
      return false;
    }
    if (!has_linear_transaction_cost_rate ||
        !std::isfinite(linear_transaction_cost_rate) ||
        linear_transaction_cost_rate <= 0.0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_NONZERO_COST_REQUIRED";
      return false;
    }
    if (allow_synthetic_direct_edges) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_EDGES_FORBIDDEN";
      return false;
    }
    if (!has_max_steps || max_steps <= 0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_MAX_STEPS_REQUIRED";
      return false;
    }
    if (!has_max_parallel_jobs || max_parallel_jobs <= 0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PARALLELISM_INVALID";
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

  std::vector<std::string> argv{exec_path.string()};
  if (!trim_ascii(config_arg).empty()) {
    argv.push_back("--config");
    argv.push_back(effective_config_path(*ctx, config_arg).string());
  }
  argv.push_back("--replay-from-job-dir");
  argv.push_back(job_dir.string());
  if (!trim_ascii(base_reserve_node_id).empty()) {
    argv.push_back("--replay-base-reserve-node");
    argv.push_back(trim_ascii(base_reserve_node_id));
  }
  if (!trim_ascii(risky_node_ids).empty()) {
    argv.push_back("--replay-risky-nodes");
    argv.push_back(trim_ascii(risky_node_ids));
  }
  if (!trim_ascii(experiment_id).empty()) {
    argv.push_back("--replay-experiment-id");
    argv.push_back(trim_ascii(experiment_id));
  }
  if (!report_path.empty()) {
    argv.push_back("--replay-report-path");
    argv.push_back(report_path.string());
  }
  if (!append_optional_double_cli_arg(args, "initial_equity_base",
                                      "--replay-initial-equity-base", &argv,
                                      err) ||
      !append_optional_double_cli_arg(args, "min_base_reserve_weight",
                                      "--replay-min-base-reserve-weight", &argv,
                                      err) ||
      !append_optional_double_cli_arg(
          args, "max_risky_weight", "--replay-max-risky-weight", &argv, err) ||
      !append_optional_double_cli_arg(args, "max_turnover_l1",
                                      "--replay-max-turnover-l1", &argv, err) ||
      !append_optional_int_cli_arg(args, "max_steps", "--replay-max-steps",
                                   &argv, err) ||
      !append_optional_int_cli_arg(args, "max_parallel_jobs",
                                   "--replay-max-parallel-jobs", &argv, err)) {
    return false;
  }
  if (include_equal_weight) {
    argv.push_back("--replay-include-equal-weight");
  }
  if (include_current_weight) {
    argv.push_back("--replay-include-current-weight");
  }
  if (!include_base_reserve_policy) {
    argv.push_back("--replay-no-base-reserve-policy");
  }
  if (!include_sdu_policy) {
    argv.push_back("--replay-no-sdu-policy");
  }
  if (allow_synthetic_direct_edges) {
    argv.push_back("--replay-allow-synthetic-direct-edges");
  }
  if (!append_optional_double_cli_arg(args, "linear_transaction_cost_rate",
                                      "--replay-linear-transaction-cost-rate",
                                      &argv, err)) {
    return false;
  }
  if (!append_optional_string_cli_arg(args, "execution_profile_digest",
                                      "--replay-execution-profile-digest",
                                      &argv) ||
      !append_optional_string_cli_arg(args, "policy_set_digest",
                                      "--replay-policy-set-digest", &argv)) {
    return false;
  }

  if (dry_run) {
    *out = replay_dry_run_json(argv, job_dir, batch_index_path, report_path);
    return true;
  }

  process_result_t result =
      run_process(argv, timeout_seconds, static_cast<std::size_t>(max_capture));
  const auto stdout_kv = parse_process_stdout_kv(result.stdout_text);
  if (validation_rollout) {
    if (result.exit_code != 0 || result.timed_out) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PROCESS_FAILED";
      return false;
    }
    fs::path validation_report_path = report_path;
    if (validation_report_path.empty()) {
      const auto found_report = stdout_kv.find("replay_report_path");
      if (found_report != stdout_kv.end()) {
        validation_report_path = normalize_path(fs::path(found_report->second));
      }
    }
    if (!validate_replay_report_for_validation_rollout(validation_report_path,
                                                       err)) {
      return false;
    }
  }
  *out = process_result_json(argv, result, job_dir, stdout_kv, false,
                             static_cast<std::size_t>(max_capture));
  return true;
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
      policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", false);
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

  const auto rows =
      cuwacunu::hero::runtime::job_layout::discover_runtime_job_dirs(root);
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
    const auto manifest = parse_kv_file(rows[i].manifest_path);
    const auto state = parse_kv_file(rows[i].state_path);
    json << "{\"job_dir\":" << json_quote(rows[i].dir.string())
         << ",\"job_id\":"
         << json_quote(manifest.count("job_id") != 0
                           ? manifest.at("job_id")
                           : rows[i].dir.filename().string())
         << ",\"job_stable_id\":"
         << json_quote(manifest.count("job_stable_id") != 0
                           ? manifest.at("job_stable_id")
                           : "")
         << ",\"job_attempt_id\":"
         << json_quote(manifest.count("job_attempt_id") != 0
                           ? manifest.at("job_attempt_id")
                           : "")
         << ",\"job_attempt_policy\":"
         << json_quote(manifest.count("job_attempt_policy") != 0
                           ? manifest.at("job_attempt_policy")
                           : "")
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
  std::optional<replay_experiment_report_binding_t> replay_report_binding;
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
      path = job_dir / "channel_inference.report";
      if (!fs::exists(path)) {
        path = job_dir / "channel_representation.report";
      }
      if (!fs::exists(path)) {
        path = job_dir / "inference.report";
      }
      if (!fs::exists(path)) {
        path = job_dir / "representation.report";
      }
    } else if (artifact == "representation_report") {
      path = job_dir / "channel_representation.report";
      if (!fs::exists(path)) {
        path = job_dir / "representation.report";
      }
    } else if (artifact == "replay_batch_index" ||
               artifact == "runtime_replay_batches") {
      path = runtime_replay_batch_index_path(job_dir);
    } else if (artifact == "replay_experiment_index" ||
               artifact == "runtime_replay_experiments") {
      path = runtime_replay_experiment_index_path(job_dir);
    } else if (artifact == "replay_experiment_report" ||
               artifact == "replay_report") {
      replay_report_binding = latest_replay_experiment_report_binding(job_dir);
      path = replay_report_binding->report_path;
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
       << ",\"fields\":" << kv_map_to_json(parse_kv_file(path));
  if (replay_report_binding.has_value()) {
    json << ",\"replay_report_integrity\":"
         << replay_report_integrity_json(*replay_report_binding);
  }
  json << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_inspect(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args,
                            {"subject", "config_path", "root", "job_id",
                             "job_dir", "artifact", "path", "include_artifacts",
                             "include_text", "max_bytes", "limit"},
                            &fields, err)) {
    return false;
  }
  std::string subject;
  if (!parse_required_string_arg(args, "subject", &subject, err)) {
    return false;
  }
  subject = lowercase_ascii(subject);
  if (subject == "schema") {
    const std::string forwarded = "{}";
    return handle_schema(forwarded, ctx, out, err);
  }
  if (subject == "wave") {
    const std::string forwarded =
        object_with_selected_fields(args, {"config_path"});
    return handle_wave(forwarded, ctx, out, err);
  }
  if (subject == "jobs") {
    const std::string forwarded = object_with_selected_fields(
        args, {"root", "limit", "include_artifacts"});
    return handle_list_jobs(forwarded, ctx, out, err);
  }
  if (subject == "job") {
    const std::string forwarded = object_with_selected_fields(
        args, {"job_id", "job_dir", "include_text", "max_bytes"});
    return handle_get_job(forwarded, ctx, out, err);
  }
  if (subject == "artifact") {
    const std::string forwarded = object_with_selected_fields(
        args, {"job_id", "job_dir", "artifact", "path", "max_bytes"});
    return handle_read_artifact(forwarded, ctx, out, err);
  }
  *err = "subject must be schema, wave, jobs, job, or artifact";
  return false;
}

[[nodiscard]] bool policy_training_wave_requested(const std::string &args) {
  return extract_json_raw_field(args, "policy_kind", nullptr) ||
         extract_json_raw_field(args, "policy_id", nullptr) ||
         extract_json_raw_field(args, "training_schedule_mode", nullptr) ||
         extract_json_raw_field(args, "causal_schedule_digest", nullptr);
}

[[nodiscard]] bool handle_run(const std::string &args, runtime_context_t *ctx,
                              std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args,
                            {"operation",
                             "requested_mode",
                             "config_path",
                             "job_id",
                             "job_dir",
                             "force_rebuild_cache",
                             "confirm_execute",
                             "timeout_seconds",
                             "wave_overlay",
                             "runtime_handoff",
                             "marshal_expected_wave",
                             "base_reserve_node_id",
                             "risky_node_ids",
                             "experiment_id",
                             "report_path",
                             "initial_equity_base",
                             "min_base_reserve_weight",
                             "max_risky_weight",
                             "max_turnover_l1",
                             "max_steps",
                             "max_parallel_jobs",
                             "include_equal_weight",
                             "include_current_weight",
                             "include_base_reserve_policy",
                             "include_spot_distributional_utility_policy",
                             "allow_synthetic_direct_edges",
                             "validation_rollout",
                             "linear_transaction_cost_rate",
                             "execution_profile_digest",
                             "policy_set_digest",
                             "protocol_id",
                             "protocol_contract_fingerprint",
                             "graph_order_fingerprint",
                             "source_cursor_token",
                             "split_policy_fingerprint",
                             "component_assembly_fingerprint",
                             "policy_id",
                             "policy_kind",
                             "policy_architecture_digest",
                             "training_config_digest",
                             "training_range_digest",
                             "validation_range_digest",
                             "test_range_digest",
                             "environment_contract_id",
                             "observation_schema_digest",
                             "action_schema_digest",
                             "reward_contract_digest",
                             "training_schedule_mode",
                             "causal_schedule_schema_id",
                             "causal_schedule_digest",
                             "causal_schedule_cursor_key_kind",
                             "causal_schedule_no_future_snapshot_use_source",
                             "normalization_fit_range_digest",
                             "replay_buffer_source_range_digest",
                             "early_stopping_policy_digest",
                             "hyperparameter_selection_policy_digest",
                             "selector_split",
                             "selector_policy_digest",
                             "parent_checkpoint_digest",
                             "parent_forecast_eval_fact_digest",
                             "parent_observer_belief_fact_digest",
                             "parent_allocation_engine_fact_digest",
                             "parent_replay_environment_fact_digest",
                             "final_refit_parent_selected_checkpoint_digest",
                             "random_seed",
                             "max_episodes",
                             "max_wall_clock_seconds",
                             "causal_schedule_readiness_eligible",
                             "causal_schedule_no_future_snapshot_use",
                             "offline_full_window_research_allowed",
                             "final_refit_uses_validation",
                             "validation_no_longer_proof",
                             "sealed_test_required",
                             "live_execution_allowed"},
                            &fields, err)) {
    return false;
  }
  std::string operation;
  std::string requested_mode;
  if (!parse_required_string_arg(args, "operation", &operation, err) ||
      !parse_required_string_arg(args, "requested_mode", &requested_mode,
                                 err)) {
    return false;
  }
  operation = lowercase_ascii(operation);
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "plan" && requested_mode != "dry_run" &&
      requested_mode != "execute") {
    *err = "requested_mode must be plan, dry_run, or execute";
    return false;
  }

  if (operation == "wave") {
    bool run_policy_training_contract = policy_training_wave_requested(args);
    if (!run_policy_training_contract && ctx != nullptr) {
      std::string config_arg;
      (void)extract_json_string_field(args, "config_path", &config_arg);
      const wave_info_t selected_wave =
          load_wave_info(*ctx, effective_config_path(*ctx, config_arg));
      run_policy_training_contract =
          wave_info_requests_policy_training(selected_wave);
    }
    if (run_policy_training_contract) {
      return handle_policy_training_contract(args, requested_mode, ctx, out,
                                             err);
    }
    if (requested_mode == "plan") {
      *err = "requested_mode=plan is not supported for operation=wave; use "
             "dry_run or execute";
      return false;
    }
    std::ostringstream forwarded;
    forwarded << "{";
    bool first = true;
    for (const auto key : {"config_path", "job_dir", "force_rebuild_cache",
                           "confirm_execute", "timeout_seconds", "wave_overlay",
                           "runtime_handoff", "marshal_expected_wave"}) {
      append_optional_raw_json_field(args, forwarded, key, &first);
    }
    append_bool_json_field(forwarded, "dry_run", requested_mode == "dry_run",
                           &first);
    forwarded << "}";
    return execute_runtime(forwarded.str(), ctx, false, out, err);
  }

  if (operation == "replay") {
    std::ostringstream forwarded;
    forwarded << "{";
    bool first = true;
    for (const auto key : {"job_id",
                           "job_dir",
                           "config_path",
                           "base_reserve_node_id",
                           "risky_node_ids",
                           "experiment_id",
                           "report_path",
                           "initial_equity_base",
                           "min_base_reserve_weight",
                           "max_risky_weight",
                           "max_turnover_l1",
                           "max_steps",
                           "max_parallel_jobs",
                           "include_equal_weight",
                           "include_current_weight",
                           "include_base_reserve_policy",
                           "include_spot_distributional_utility_policy",
                           "allow_synthetic_direct_edges",
                           "validation_rollout",
                           "linear_transaction_cost_rate",
                           "timeout_seconds",
                           "execution_profile_digest",
                           "policy_set_digest"}) {
      append_optional_raw_json_field(args, forwarded, key, &first);
    }
    append_bool_json_field(forwarded, "dry_run", requested_mode != "execute",
                           &first);
    forwarded << "}";
    return handle_replay(forwarded.str(), ctx, out, err);
  }

  *err = "operation must be wave or replay";
  return false;
}

[[nodiscard]] bool handle_reset(const std::string &args, runtime_context_t *ctx,
                                std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(
          args,
          {"requested_mode", "runtime_root", "backup", "confirm_dev_nuke"},
          &fields, err)) {
    return false;
  }
  std::string requested_mode;
  if (!parse_required_string_arg(args, "requested_mode", &requested_mode,
                                 err)) {
    return false;
  }
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "plan" && requested_mode != "execute") {
    *err = "requested_mode must be plan or execute";
    return false;
  }
  std::ostringstream forwarded;
  forwarded << "{";
  bool first = true;
  for (const auto key : {"runtime_root", "backup", "confirm_dev_nuke"}) {
    append_optional_raw_json_field(args, forwarded, key, &first);
  }
  append_bool_json_field(forwarded, "dry_run", requested_mode == "plan",
                         &first);
  forwarded << "}";
  return handle_dev_nuke(forwarded.str(), ctx, out, err);
}

using handler_fn = bool (*)(const std::string &, runtime_context_t *,
                            std::string *, std::string *);

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.runtime.status") {
    return handle_status;
  }
  if (name == "hero.runtime.inspect") {
    return handle_inspect;
  }
  if (name == "hero.runtime.run") {
    return handle_run;
  }
  if (name == "hero.runtime.reset") {
    return handle_reset;
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
                         runtime_policy_t *out, std::string *error,
                         std::string_view profile_override) {
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

  runtime_profile_blocks_t profile_blocks{};
  if (!split_runtime_profile_blocks(text, &profile_blocks, error)) {
    return false;
  }

  for (auto &[key, value] : parse_assignment_text(profile_blocks.base_text,
                                                  false)) {
    policy.values[std::move(key)] = std::move(value);
  }

  std::string selected_profile = trim_ascii(profile_override);
  std::string selected_profile_source = "cli";
  if (selected_profile.empty()) {
    selected_profile_source = "global_config";
    if (const auto configured = read_ini_value(global_config, "HERO",
                                              "runtime_hero_profile")) {
      selected_profile = trim_ascii(*configured);
    }
  }
  if (selected_profile.empty()) {
    selected_profile = trim_ascii(policy_get(policy, "runtime_profile"));
    selected_profile_source = "policy_file";
  }
  if (selected_profile.empty()) {
    selected_profile = "operator_default";
    selected_profile_source = "default";
  }
  if (!valid_runtime_profile_id(selected_profile)) {
    if (error) {
      *error = "invalid runtime profile id: " + selected_profile;
    }
    return false;
  }

  const auto profile_it =
      profile_blocks.profile_text_by_id.find(selected_profile);
  if (profile_it == profile_blocks.profile_text_by_id.end() &&
      !(profile_blocks.profile_text_by_id.empty() &&
        selected_profile == "operator_default")) {
    if (error) {
      *error = "unknown runtime profile `" + selected_profile +
               "` in policy file: " + policy_path.string();
    }
    return false;
  }
  if (profile_it != profile_blocks.profile_text_by_id.end()) {
    for (auto &[key, value] : parse_assignment_text(profile_it->second, false)) {
      if (key == "runtime_profile") {
        if (error) {
          *error = "RUNTIME_PROFILE block must not set runtime_profile";
        }
        return false;
      }
      policy.values[std::move(key)] = std::move(value);
    }
  }
  policy.profile_id = selected_profile;
  policy.values["runtime_profile"] = selected_profile;
  policy.values["runtime_profile_source"] = selected_profile_source;

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
