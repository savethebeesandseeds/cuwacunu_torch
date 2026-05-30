// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "kikijyeba/marshal/codex_assist.h"
#include "kikijyeba/marshal/dispatch_receipt.h"
#include "kikijyeba/marshal/operational_report.h"
#include "kikijyeba/marshal/run_compare.h"
#include "kikijyeba/marshal/status.h"
#include "kikijyeba/marshal/target_driver.h"
#include "kikijyeba/marshal/tool_schema.h"

namespace cuwacunu::kikijyeba::marshal {

using marshal_lattice_tool_callback_t =
    bool (*)(const std::string &tool_name, const std::string &arguments_json,
             std::string *out_tool_result_json, std::string *out_error_message);

[[nodiscard]] inline marshal_lattice_tool_callback_t &
marshal_lattice_tool_callback_slot() {
  static marshal_lattice_tool_callback_t callback = nullptr;
  return callback;
}

inline void
set_marshal_lattice_tool_callback(marshal_lattice_tool_callback_t callback) {
  marshal_lattice_tool_callback_slot() = callback;
}

namespace tool_detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

inline void skip_ws(const std::string &text, std::size_t *idx) {
  while (*idx < text.size() &&
         std::isspace(static_cast<unsigned char>(text[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] inline bool parse_json_string_token(const std::string &text,
                                                  std::size_t *idx,
                                                  std::string *out) {
  skip_ws(text, idx);
  if (*idx >= text.size() || text[*idx] != '"') {
    return false;
  }
  ++(*idx);
  std::string value;
  while (*idx < text.size()) {
    const char c = text[*idx];
    ++(*idx);
    if (c == '"') {
      if (out) {
        *out = std::move(value);
      }
      return true;
    }
    if (c == '\\') {
      if (*idx >= text.size()) {
        return false;
      }
      const char escaped = text[*idx];
      ++(*idx);
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
      default:
        return false;
      }
      continue;
    }
    value.push_back(c);
  }
  return false;
}

[[nodiscard]] inline bool skip_json_value(const std::string &text,
                                          std::size_t *idx) {
  skip_ws(text, idx);
  if (*idx >= text.size()) {
    return false;
  }
  if (text[*idx] == '"') {
    return parse_json_string_token(text, idx, nullptr);
  }
  if (text[*idx] == '{' || text[*idx] == '[') {
    std::vector<char> stack;
    stack.push_back(text[*idx] == '{' ? '}' : ']');
    ++(*idx);
    while (*idx < text.size() && !stack.empty()) {
      if (text[*idx] == '"') {
        if (!parse_json_string_token(text, idx, nullptr)) {
          return false;
        }
        continue;
      }
      if (text[*idx] == '{') {
        stack.push_back('}');
      } else if (text[*idx] == '[') {
        stack.push_back(']');
      } else if (text[*idx] == stack.back()) {
        stack.pop_back();
      }
      ++(*idx);
    }
    return stack.empty();
  }
  const std::size_t begin = *idx;
  while (*idx < text.size()) {
    const char c = text[*idx];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++(*idx);
  }
  return *idx > begin;
}

[[nodiscard]] inline std::map<std::string, std::string>
object_fields(const std::string &json) {
  std::map<std::string, std::string> fields;
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    throw std::runtime_error("expected JSON object");
  }
  ++idx;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        throw std::runtime_error("trailing content after JSON object");
      }
      return fields;
    }
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      throw std::runtime_error("expected JSON object key");
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      throw std::runtime_error("expected ':' after JSON object key");
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t begin = idx;
    if (!skip_json_value(json, &idx)) {
      throw std::runtime_error("invalid JSON value for key " + key);
    }
    fields[key] = trim_ascii(std::string_view(json).substr(begin, idx - begin));
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      continue;
    }
    throw std::runtime_error("expected ',' or '}' in JSON object");
  }
  throw std::runtime_error("unterminated JSON object");
}

inline void validate_fields(const std::map<std::string, std::string> &fields,
                            const std::set<std::string> &allowed,
                            const std::set<std::string> &required,
                            const std::string &label) {
  for (const auto &[key, _] : fields) {
    if (allowed.find(key) == allowed.end()) {
      throw std::runtime_error(label + " unknown field: " + key);
    }
  }
  for (const auto &key : required) {
    if (fields.find(key) == fields.end()) {
      throw std::runtime_error(label + " missing required field: " + key);
    }
  }
}

[[nodiscard]] inline std::optional<std::string>
optional_raw(const std::map<std::string, std::string> &fields,
             const std::string &key) {
  const auto found = fields.find(key);
  if (found == fields.end()) {
    return std::nullopt;
  }
  return found->second;
}

[[nodiscard]] inline bool raw_is_json_null(const std::string &raw) {
  return trim_ascii(raw) == "null";
}

[[nodiscard]] inline std::optional<std::string>
optional_non_null_raw(const std::map<std::string, std::string> &fields,
                      const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || raw_is_json_null(*raw)) {
    return std::nullopt;
  }
  return raw;
}

[[nodiscard]] inline std::string parse_string_raw(const std::string &raw,
                                                  const std::string &label) {
  std::size_t idx = 0;
  std::string out;
  if (!parse_json_string_token(raw, &idx, &out)) {
    throw std::runtime_error(label + " must be a JSON string");
  }
  skip_ws(raw, &idx);
  if (idx != raw.size()) {
    throw std::runtime_error(label + " has trailing content");
  }
  return out;
}

[[nodiscard]] inline std::string
optional_string(const std::map<std::string, std::string> &fields,
                const std::string &key, const std::string &fallback = {}) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value()) {
    return fallback;
  }
  return parse_string_raw(*raw, key);
}

[[nodiscard]] inline std::string
first_non_empty(std::initializer_list<std::string> values) {
  for (const auto &value : values) {
    if (!value.empty()) {
      return value;
    }
  }
  return {};
}

[[nodiscard]] inline bool
optional_bool(const std::map<std::string, std::string> &fields,
              const std::string &key, bool fallback = false) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value()) {
    return fallback;
  }
  const auto value = trim_ascii(*raw);
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error(key + " must be boolean");
}

[[nodiscard]] inline std::int64_t
optional_i64(const std::map<std::string, std::string> &fields,
             const std::string &key, std::int64_t fallback = 0) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value()) {
    return fallback;
  }
  try {
    return std::stoll(trim_ascii(*raw));
  } catch (const std::exception &) {
    throw std::runtime_error(key + " must be integer");
  }
}

[[nodiscard]] inline std::optional<std::size_t>
optional_size(const std::map<std::string, std::string> &fields,
              const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || trim_ascii(*raw) == "null") {
    return std::nullopt;
  }
  try {
    const auto value = std::stoll(trim_ascii(*raw));
    if (value < 0) {
      throw std::runtime_error(key + " must be non-negative");
    }
    return static_cast<std::size_t>(value);
  } catch (const std::exception &) {
    throw std::runtime_error(key + " must be non-negative integer or null");
  }
}

[[nodiscard]] inline std::optional<std::int64_t>
optional_i64_nullable(const std::map<std::string, std::string> &fields,
                      const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || trim_ascii(*raw) == "null") {
    return std::nullopt;
  }
  try {
    return std::stoll(trim_ascii(*raw));
  } catch (const std::exception &) {
    throw std::runtime_error(key + " must be integer or null");
  }
}

[[nodiscard]] inline std::vector<std::string>
array_values(const std::string &raw) {
  std::vector<std::string> out;
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  if (idx >= raw.size() || raw[idx] != '[') {
    throw std::runtime_error("expected JSON array");
  }
  ++idx;
  while (idx < raw.size()) {
    skip_ws(raw, &idx);
    if (idx < raw.size() && raw[idx] == ']') {
      ++idx;
      skip_ws(raw, &idx);
      if (idx != raw.size()) {
        throw std::runtime_error("trailing content after JSON array");
      }
      return out;
    }
    const std::size_t begin = idx;
    if (!skip_json_value(raw, &idx)) {
      throw std::runtime_error("invalid JSON array value");
    }
    out.push_back(trim_ascii(std::string_view(raw).substr(begin, idx - begin)));
    skip_ws(raw, &idx);
    if (idx < raw.size() && raw[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < raw.size() && raw[idx] == ']') {
      continue;
    }
    throw std::runtime_error("expected ',' or ']' in JSON array");
  }
  throw std::runtime_error("unterminated JSON array");
}

[[nodiscard]] inline std::vector<std::string>
optional_string_array(const std::map<std::string, std::string> &fields,
                      const std::string &key) {
  std::vector<std::string> out;
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value()) {
    return out;
  }
  for (const auto &value_raw : array_values(*raw)) {
    out.push_back(parse_string_raw(value_raw, key + "[]"));
  }
  return out;
}

[[nodiscard]] inline std::map<std::string, std::string>
optional_string_map(const std::map<std::string, std::string> &fields,
                    const std::string &key) {
  std::map<std::string, std::string> out;
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value()) {
    return out;
  }
  const auto nested = object_fields(*raw);
  for (const auto &[map_key, value_raw] : nested) {
    out[map_key] = parse_string_raw(value_raw, key + "." + map_key);
  }
  return out;
}

[[nodiscard]] inline marshal_dispatch_mode_t
parse_mode_text(const std::string &value) {
  if (value == "dry_run") {
    return marshal_dispatch_mode_t::dry_run;
  }
  if (value == "execute") {
    return marshal_dispatch_mode_t::execute;
  }
  return marshal_dispatch_mode_t::unknown;
}

[[nodiscard]] inline marshal_target_drive_mode_t
parse_drive_mode_text(const std::string &value) {
  if (value == "one_step") {
    return marshal_target_drive_mode_t::one_step;
  }
  if (value == "budgeted") {
    return marshal_target_drive_mode_t::budgeted;
  }
  return marshal_target_drive_mode_t::unknown;
}

[[nodiscard]] inline marshal_target_driver_policy_t
parse_driver_policy(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"max_waves", "max_wall_clock_seconds", "allow_execute",
                   "allow_train_execute", "require_runtime_job_completion",
                   "require_post_wave_lattice_satisfied_check",
                   "stop_on_warning_severity", "stop_on_lattice_warning",
                   "stop_on_runtime_warning", "no_progress_window"},
                  {}, "driver_policy");
  marshal_target_driver_policy_t out{};
  out.max_waves = optional_i64(fields, "max_waves", 1);
  out.max_wall_clock_seconds =
      optional_i64(fields, "max_wall_clock_seconds", 0);
  out.allow_execute = optional_bool(fields, "allow_execute", false);
  out.allow_train_execute = optional_bool(fields, "allow_train_execute", false);
  out.require_runtime_job_completion =
      optional_bool(fields, "require_runtime_job_completion", true);
  out.require_post_wave_lattice_satisfied_check =
      optional_bool(fields, "require_post_wave_lattice_satisfied_check", true);
  out.stop_on_warning_severity =
      optional_string(fields, "stop_on_warning_severity");
  out.stop_on_lattice_warning =
      optional_bool(fields, "stop_on_lattice_warning", false);
  out.stop_on_runtime_warning =
      optional_bool(fields, "stop_on_runtime_warning", false);
  out.no_progress_window = optional_i64(fields, "no_progress_window", 1);
  return out;
}

struct target_driver_resume_state_t {
  bool present{false};
  std::string target_driver_run_id{};
  std::string target_id{};
  std::string drive_mode{};
  std::string requested_mode{};
  std::string driver_policy_digest{};
  std::int64_t iteration_count{0};
  std::int64_t runtime_handoff_attempt_count{0};
  std::int64_t execution_attempt_count{0};
  std::string last_target_deficit_digest{};
  std::string last_suggested_wave_digest{};
  std::string last_progress_signature{};
  std::string last_runtime_job_id{};
  std::string last_runtime_job_state_digest{};
  std::string last_runtime_job_manifest_digest{};
  std::string last_runtime_terminal_fact_digest{};
  std::string last_runtime_checkpoint_io_fact_digest{};
  std::string last_runtime_handoff_id{};
  std::string last_runtime_handoff_digest{};
  std::string last_runtime_policy_digest{};
  std::string ledger_digest{};
  std::string terminal_state{};
  std::vector<std::string> compacted_fields{};
};

[[nodiscard]] inline target_driver_resume_state_t
parse_target_driver_resume_state(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"schema_version",
                   "target_driver_run_id",
                   "target_id",
                   "active_identity",
                   "drive_mode",
                   "requested_mode",
                   "driver_policy",
                   "driver_policy_digest",
                   "resumed_from_run_id",
                   "resumed_iteration_count",
                   "iteration_count",
                   "runtime_handoff_attempt_count",
                   "execution_attempt_count",
                   "last_target_deficit_digest",
                   "last_suggested_wave_digest",
                   "last_progress_signature",
                   "last_runtime_job_id",
                   "last_runtime_job_state_digest",
                   "last_runtime_job_manifest_digest",
                   "last_runtime_terminal_fact_digest",
                   "last_runtime_checkpoint_io_fact_digest",
                   "last_runtime_handoff_id",
                   "last_runtime_handoff_digest",
                   "last_runtime_policy_digest",
                   "terminal_state",
                   "terminal_reason",
                   "last_safe_point",
                   "next_safe_recheck",
                   "ledger_digest",
                   "compacted_fields",
                   "non_authority_statement",
                   "iterations"},
                  {}, "resume_ledger");
  target_driver_resume_state_t out{};
  out.present = true;
  out.target_driver_run_id = optional_string(fields, "target_driver_run_id");
  out.target_id = optional_string(fields, "target_id");
  out.drive_mode = optional_string(fields, "drive_mode");
  out.requested_mode = optional_string(fields, "requested_mode");
  out.driver_policy_digest = optional_string(fields, "driver_policy_digest");
  out.iteration_count = optional_i64(fields, "iteration_count", 0);
  out.runtime_handoff_attempt_count =
      optional_i64(fields, "runtime_handoff_attempt_count", 0);
  out.execution_attempt_count =
      optional_i64(fields, "execution_attempt_count", 0);
  out.last_target_deficit_digest =
      optional_string(fields, "last_target_deficit_digest");
  out.last_suggested_wave_digest =
      optional_string(fields, "last_suggested_wave_digest");
  out.last_progress_signature =
      optional_string(fields, "last_progress_signature");
  out.last_runtime_job_id = optional_string(fields, "last_runtime_job_id");
  out.last_runtime_job_state_digest =
      optional_string(fields, "last_runtime_job_state_digest");
  out.last_runtime_job_manifest_digest =
      optional_string(fields, "last_runtime_job_manifest_digest");
  out.last_runtime_terminal_fact_digest =
      optional_string(fields, "last_runtime_terminal_fact_digest");
  out.last_runtime_checkpoint_io_fact_digest =
      optional_string(fields, "last_runtime_checkpoint_io_fact_digest");
  out.last_runtime_handoff_id =
      optional_string(fields, "last_runtime_handoff_id");
  out.last_runtime_handoff_digest =
      optional_string(fields, "last_runtime_handoff_digest");
  out.last_runtime_policy_digest =
      optional_string(fields, "last_runtime_policy_digest");
  out.ledger_digest = optional_string(fields, "ledger_digest");
  out.terminal_state = optional_string(fields, "terminal_state");
  out.compacted_fields = optional_string_array(fields, "compacted_fields");
  return out;
}

[[nodiscard]] inline marshal_active_identity_t
parse_active_identity(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"protocol_contract_fingerprint", "graph_order_fingerprint",
                   "target_spec_fingerprint", "split_policy_fingerprint"},
                  {}, "active_identity");
  marshal_active_identity_t out{};
  out.protocol_contract_fingerprint =
      optional_string(fields, "protocol_contract_fingerprint");
  out.graph_order_fingerprint =
      optional_string(fields, "graph_order_fingerprint");
  out.target_spec_fingerprint =
      optional_string(fields, "target_spec_fingerprint");
  out.split_policy_fingerprint =
      optional_string(fields, "split_policy_fingerprint");
  return out;
}

[[nodiscard]] inline marshal_plan_basis_t
parse_plan_basis(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"present", "available", "source_range",
                   "target_anchor_index_begin", "target_anchor_index_end",
                   "target_source_key_begin", "target_source_key_end",
                   "primary_deficit_key", "primary_deficit_message",
                   "deficit_keys", "deficit_priority_classes"},
                  {}, "plan_basis");
  marshal_plan_basis_t out{};
  out.present = optional_bool(fields, "present", false);
  out.available = optional_bool(fields, "available", false);
  out.source_range = optional_string(fields, "source_range", "anchor_index");
  out.target_anchor_index_begin =
      optional_size(fields, "target_anchor_index_begin");
  out.target_anchor_index_end =
      optional_size(fields, "target_anchor_index_end");
  out.target_source_key_begin =
      optional_i64_nullable(fields, "target_source_key_begin");
  out.target_source_key_end =
      optional_i64_nullable(fields, "target_source_key_end");
  out.primary_deficit_key = optional_string(fields, "primary_deficit_key");
  out.primary_deficit_message =
      optional_string(fields, "primary_deficit_message");
  out.deficit_keys = optional_string_array(fields, "deficit_keys");
  out.deficit_priority_classes =
      optional_string_array(fields, "deficit_priority_classes");
  return out;
}

[[nodiscard]] inline marshal_suggested_wave_t
parse_suggested_wave(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"target", "mode", "source_range", "anchor_index_begin",
                   "anchor_index_end", "source_key_begin", "source_key_end",
                   "plan_inputs"},
                  {}, "suggested_wave");
  marshal_suggested_wave_t out{};
  out.target = optional_string(fields, "target");
  out.mode = optional_string(fields, "mode");
  out.source_range = optional_string(fields, "source_range", "anchor_index");
  out.anchor_index_begin = optional_size(fields, "anchor_index_begin");
  out.anchor_index_end = optional_size(fields, "anchor_index_end");
  out.source_key_begin = optional_i64_nullable(fields, "source_key_begin");
  out.source_key_end = optional_i64_nullable(fields, "source_key_end");
  out.plan_inputs = optional_string_map(fields, "plan_inputs");
  return out;
}

[[nodiscard]] inline marshal_dispatch_advice_t
parse_advice(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(
      fields,
      {"schema_version", "config_path", "runtime_root", "target_id",
       "target_status", "active_identity", "plan_basis", "plan_basis_digest",
       "suggested_wave", "suggested_wave_digest", "plan_input_digest",
       "max_waves", "recommendation_attempt_count", "required_plan_inputs",
       "source_lattice_tool", "source_lattice_timestamp",
       "advice_receipt_digest", "advice_signature_key_id", "advice_signature"},
      {}, "advice");
  marshal_dispatch_advice_t out{};
  out.schema_version = optional_string(fields, "schema_version",
                                       k_marshal_dispatch_advice_schema_v1);
  out.config_path = optional_string(fields, "config_path");
  out.runtime_root = optional_string(fields, "runtime_root");
  out.target_id = optional_string(fields, "target_id");
  out.target_status = optional_string(fields, "target_status");
  if (const auto raw_identity = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_active_identity(*raw_identity);
  }
  if (const auto raw_basis = optional_raw(fields, "plan_basis")) {
    out.plan_basis = parse_plan_basis(*raw_basis);
  }
  out.plan_basis_digest = optional_string(fields, "plan_basis_digest");
  if (const auto raw_wave = optional_raw(fields, "suggested_wave")) {
    out.suggested_wave = parse_suggested_wave(*raw_wave);
  }
  out.suggested_wave_digest = optional_string(fields, "suggested_wave_digest");
  out.plan_input_digest = optional_string(fields, "plan_input_digest");
  out.max_waves = optional_i64(fields, "max_waves", -1);
  out.recommendation_attempt_count =
      optional_i64(fields, "recommendation_attempt_count", 0);
  out.required_plan_inputs =
      optional_string_array(fields, "required_plan_inputs");
  out.source_lattice_tool = optional_string(fields, "source_lattice_tool");
  out.source_lattice_timestamp =
      optional_string(fields, "source_lattice_timestamp");
  out.advice_receipt_digest = optional_string(fields, "advice_receipt_digest");
  out.advice_signature_key_id =
      optional_string(fields, "advice_signature_key_id");
  out.advice_signature = optional_string(fields, "advice_signature");
  return out;
}

[[nodiscard]] inline marshal_dispatch_request_t
parse_request(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"schema_version", "requested_mode", "target_id",
                   "config_path", "runtime_root", "advice_digest",
                   "operator_confirmation_token", "requested_overrides"},
                  {}, "request");
  marshal_dispatch_request_t out{};
  out.schema_version = optional_string(fields, "schema_version",
                                       k_marshal_dispatch_request_schema_v1);
  out.requested_mode =
      parse_mode_text(optional_string(fields, "requested_mode", "dry_run"));
  out.target_id = optional_string(fields, "target_id");
  out.config_path = optional_string(fields, "config_path");
  out.runtime_root = optional_string(fields, "runtime_root");
  out.advice_digest = optional_string(fields, "advice_digest");
  out.operator_confirmation_token =
      optional_string(fields, "operator_confirmation_token");
  out.requested_overrides = optional_string_map(fields, "requested_overrides");
  return out;
}

[[nodiscard]] inline marshal_dispatch_validation_context_t
parse_context(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"active_config_path", "active_runtime_root",
                   "active_identity", "supported_wave_targets",
                   "supported_source_ranges", "allowed_lattice_tools",
                   "allowed_model_state_roots", "freshness_check_timestamp_utc",
                   "max_advice_age_seconds", "allowed_clock_skew_seconds",
                   "require_signed_lattice_advice",
                   "trusted_lattice_advice_key_id",
                   "trusted_lattice_advice_verification_key", "allow_execute",
                   "allow_execute_mode"},
                  {}, "context");
  marshal_dispatch_validation_context_t out{};
  out.active_config_path = optional_string(fields, "active_config_path");
  out.active_runtime_root = optional_string(fields, "active_runtime_root");
  if (const auto raw_identity = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_active_identity(*raw_identity);
  }
  if (const auto values =
          optional_string_array(fields, "supported_wave_targets");
      !values.empty()) {
    out.supported_wave_targets =
        std::set<std::string>(values.begin(), values.end());
  }
  if (const auto values =
          optional_string_array(fields, "supported_source_ranges");
      !values.empty()) {
    out.supported_source_ranges =
        std::set<std::string>(values.begin(), values.end());
  }
  if (const auto values =
          optional_string_array(fields, "allowed_lattice_tools");
      !values.empty()) {
    out.allowed_lattice_tools =
        std::set<std::string>(values.begin(), values.end());
  }
  out.allowed_model_state_roots =
      optional_string_array(fields, "allowed_model_state_roots");
  out.freshness_check_timestamp_utc =
      optional_string(fields, "freshness_check_timestamp_utc");
  out.max_advice_age_seconds =
      optional_i64(fields, "max_advice_age_seconds", 3600);
  out.allowed_clock_skew_seconds =
      optional_i64(fields, "allowed_clock_skew_seconds", 300);
  out.require_signed_lattice_advice =
      optional_bool(fields, "require_signed_lattice_advice", false);
  out.trusted_lattice_advice_key_id =
      optional_string(fields, "trusted_lattice_advice_key_id");
  out.trusted_lattice_advice_verification_key =
      optional_string(fields, "trusted_lattice_advice_verification_key");
  out.allow_execute = optional_bool(fields, "allow_execute", false);
  out.allow_execute_mode = optional_bool(fields, "allow_execute_mode", false);
  return out;
}

[[nodiscard]] inline marshal_runtime_policy_snapshot_t
parse_runtime_policy(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"runtime_hero_available", "runtime_exec_exists",
                   "runtime_exec_executable", "default_dry_run",
                   "allow_execute", "allow_train_execute", "runtime_root"},
                  {}, "runtime_policy");
  marshal_runtime_policy_snapshot_t out{};
  out.runtime_hero_available =
      optional_bool(fields, "runtime_hero_available", true);
  out.runtime_exec_exists = optional_bool(fields, "runtime_exec_exists", true);
  out.runtime_exec_executable =
      optional_bool(fields, "runtime_exec_executable", true);
  out.default_dry_run = optional_bool(fields, "default_dry_run", true);
  out.allow_execute = optional_bool(fields, "allow_execute", false);
  out.allow_train_execute = optional_bool(fields, "allow_train_execute", false);
  out.runtime_root = optional_string(fields, "runtime_root");
  return out;
}

[[nodiscard]] inline marshal_runtime_wave_snapshot_t
parse_runtime_wave(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"available", "wave_id", "target_component_family_id",
                   "target_component", "mode", "source_range", "source_order",
                   "anchor_index_begin", "anchor_index_end", "source_key_begin",
                   "source_key_end", "job_kind", "train_target",
                   "model_state_inputs"},
                  {}, "runtime_wave");
  marshal_runtime_wave_snapshot_t out{};
  out.available = optional_bool(fields, "available", false);
  out.wave_id = optional_string(fields, "wave_id");
  out.target_component_family_id =
      first_non_empty({optional_string(fields, "target_component_family_id"),
                       optional_string(fields, "target_component")});
  out.mode = optional_string(fields, "mode");
  out.source_range = optional_string(fields, "source_range", "anchor_index");
  out.source_order = optional_string(fields, "source_order");
  out.anchor_index_begin = optional_size(fields, "anchor_index_begin");
  out.anchor_index_end = optional_size(fields, "anchor_index_end");
  out.source_key_begin = optional_i64_nullable(fields, "source_key_begin");
  out.source_key_end = optional_i64_nullable(fields, "source_key_end");
  out.job_kind = optional_string(fields, "job_kind");
  out.train_target = optional_bool(fields, "train_target", false);
  out.model_state_inputs = optional_string_map(fields, "model_state_inputs");
  return out;
}

[[nodiscard]] inline marshal_prior_dry_run_evidence_t
parse_prior_dry_run(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"present", "accepted", "advice_digest",
                   "dispatch_request_identity_digest",
                   "runtime_preview_request_digest", "suggested_wave_digest",
                   "plan_input_digest", "runtime_policy_digest",
                   "runtime_wave_digest", "runtime_response_digest",
                   "dry_run_response_digest", "evidence_digest"},
                  {}, "prior_dry_run");
  marshal_prior_dry_run_evidence_t out{};
  out.present = optional_bool(fields, "present", false);
  out.accepted = optional_bool(fields, "accepted", false);
  out.advice_digest = optional_string(fields, "advice_digest");
  out.dispatch_request_identity_digest =
      optional_string(fields, "dispatch_request_identity_digest");
  out.runtime_preview_request_digest =
      optional_string(fields, "runtime_preview_request_digest");
  out.suggested_wave_digest = optional_string(fields, "suggested_wave_digest");
  out.plan_input_digest = optional_string(fields, "plan_input_digest");
  out.runtime_policy_digest = optional_string(fields, "runtime_policy_digest");
  out.runtime_wave_digest = optional_string(fields, "runtime_wave_digest");
  out.runtime_response_digest =
      optional_string(fields, "runtime_response_digest");
  out.dry_run_response_digest =
      optional_string(fields, "dry_run_response_digest");
  out.evidence_digest = optional_string(fields, "evidence_digest");
  return out;
}

[[nodiscard]] inline marshal_dispatch_receipt_t
parse_receipt(const std::string &raw) {
  const auto fields = object_fields(raw);
  validate_fields(fields,
                  {"schema_version",
                   "retention_class",
                   "receipt_id",
                   "receipt_kind",
                   "timestamp",
                   "target_id",
                   "lattice_proof_context",
                   "plan_basis_digest",
                   "suggested_wave_digest",
                   "advice_digest",
                   "request_digest",
                   "decision_digest",
                   "runtime_request_digest",
                   "runtime_preview_request_digest",
                   "runtime_execution_request_digest",
                   "runtime_handoff_arguments_digest",
                   "runtime_response_digest",
                   "policy_decision",
                   "confirmation_status",
                   "refusal_reasons",
                   "compacted_fields",
                   "runtime_tool_result_json",
                   "post_run_verification",
                   "non_authority_statement"},
                  {}, "receipt");
  marshal_dispatch_receipt_t out{};
  out.schema_version = optional_string(fields, "schema_version",
                                       k_marshal_dispatch_receipt_schema_v1);
  out.retention_class = optional_string(fields, "retention_class", "full");
  out.receipt_id = optional_string(fields, "receipt_id");
  out.receipt_kind = optional_string(fields, "receipt_kind");
  out.timestamp = optional_string(fields, "timestamp");
  out.target_id = optional_string(fields, "target_id");
  if (const auto raw_identity = optional_raw(fields, "lattice_proof_context")) {
    out.lattice_proof_context = parse_active_identity(*raw_identity);
  }
  out.plan_basis_digest = optional_string(fields, "plan_basis_digest");
  out.suggested_wave_digest = optional_string(fields, "suggested_wave_digest");
  out.advice_digest = optional_string(fields, "advice_digest");
  out.request_digest = optional_string(fields, "request_digest");
  out.decision_digest = optional_string(fields, "decision_digest");
  out.runtime_request_digest =
      optional_string(fields, "runtime_request_digest");
  out.runtime_preview_request_digest =
      optional_string(fields, "runtime_preview_request_digest");
  out.runtime_execution_request_digest =
      optional_string(fields, "runtime_execution_request_digest");
  out.runtime_handoff_arguments_digest =
      optional_string(fields, "runtime_handoff_arguments_digest");
  out.runtime_response_digest =
      optional_string(fields, "runtime_response_digest");
  out.policy_decision = optional_string(fields, "policy_decision");
  out.confirmation_status = optional_string(fields, "confirmation_status");
  out.refusal_reasons = optional_string_array(fields, "refusal_reasons");
  out.compacted_fields = optional_string_array(fields, "compacted_fields");
  out.runtime_tool_result_json =
      optional_string(fields, "runtime_tool_result_json");
  out.post_run_verification = optional_string(fields, "post_run_verification");
  out.non_authority_statement =
      optional_string(fields, "non_authority_statement",
                      k_marshal_dispatch_non_authority_statement);
  return out;
}

struct target_driver_replay_context_t {
  std::string target_id{};
  std::string drive_mode{};
  std::string requested_mode{};
  std::string driver_policy_digest{};
  marshal_active_identity_t active_identity{};
  std::string runtime_policy_digest{};
};

struct target_driver_replay_audit_t {
  bool accepted{true};
  bool stale{false};
  bool compact{false};
  bool non_authoritative{true};
  std::vector<std::string> issues{};
  std::string explanation{"target-driver ledger replay checks Marshal audit "
                          "integrity only; it does "
                          "not prove target satisfaction"};
};

[[nodiscard]] inline bool
string_vector_contains(const std::vector<std::string> &values,
                       const std::string &needle) {
  for (const auto &value : values) {
    if (value == needle) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline marshal_target_driver_iteration_t
parse_target_driver_iteration_for_replay(const std::string &raw,
                                         std::string *iteration_digest) {
  const auto fields = object_fields(raw);
  marshal_target_driver_iteration_t out{};
  out.iteration_index = optional_i64(fields, "iteration_index", 0);
  if (const auto raw_identity = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_active_identity(*raw_identity);
  }
  out.target_status = optional_string(fields, "target_status");
  out.dispatch_state = optional_string(fields, "dispatch_state");
  out.blocker_bucket = optional_string(fields, "blocker_bucket");
  out.next_action = optional_string(fields, "next_action");
  out.terminal_state = optional_string(fields, "terminal_state");
  out.terminal_reason = optional_string(fields, "terminal_reason");
  out.target_deficit_digest = optional_string(fields, "target_deficit_digest");
  out.suggested_wave_digest = optional_string(fields, "suggested_wave_digest");
  out.plan_input_digest = optional_string(fields, "plan_input_digest");
  out.advice_digest = optional_string(fields, "advice_digest");
  out.request_digest = optional_string(fields, "request_digest");
  out.runtime_policy_digest = optional_string(fields, "runtime_policy_digest");
  out.runtime_wave_digest = optional_string(fields, "runtime_wave_digest");
  out.runtime_preview_request_digest =
      optional_string(fields, "runtime_preview_request_digest");
  out.runtime_execution_request_digest =
      optional_string(fields, "runtime_execution_request_digest");
  out.runtime_handoff_arguments_digest =
      optional_string(fields, "runtime_handoff_arguments_digest");
  out.runtime_handoff_id = optional_string(fields, "runtime_handoff_id");
  out.runtime_handoff_digest =
      optional_string(fields, "runtime_handoff_digest");
  out.runtime_response_digest =
      optional_string(fields, "runtime_response_digest");
  out.dry_run_response_digest =
      optional_string(fields, "dry_run_response_digest");
  out.runtime_job_id = optional_string(fields, "runtime_job_id");
  out.runtime_job_state_digest =
      optional_string(fields, "runtime_job_state_digest");
  out.runtime_job_manifest_digest =
      optional_string(fields, "runtime_job_manifest_digest");
  out.runtime_terminal_fact_digest =
      optional_string(fields, "runtime_terminal_fact_digest");
  out.runtime_checkpoint_io_fact_digest =
      optional_string(fields, "runtime_checkpoint_io_fact_digest");
  out.runtime_terminal_status =
      optional_string(fields, "runtime_terminal_status");
  out.post_run_lattice_evaluation_digest =
      optional_string(fields, "post_run_lattice_evaluation_digest");
  out.progress_signature = optional_string(fields, "progress_signature");
  out.dry_run_attempted = optional_bool(fields, "dry_run_attempted", false);
  out.dry_run_accepted = optional_bool(fields, "dry_run_accepted", false);
  out.execution_attempted = optional_bool(fields, "execution_attempted", false);
  out.execution_accepted = optional_bool(fields, "execution_accepted", false);
  out.runtime_job_completion_observed =
      optional_bool(fields, "runtime_job_completion_observed", false);
  out.lattice_target_satisfied_after_wave =
      optional_bool(fields, "lattice_target_satisfied_after_wave", false);
  out.refusal_reasons = optional_string_array(fields, "refusal_reasons");
  if (iteration_digest != nullptr) {
    *iteration_digest = optional_string(fields, "iteration_digest");
  }
  return out;
}

[[nodiscard]] inline marshal_target_driver_ledger_t
parse_target_driver_ledger_for_replay(
    const std::string &raw, std::string *ledger_digest,
    std::vector<std::string> *compacted_fields, bool *has_iteration_bodies,
    target_driver_replay_audit_t *audit) {
  const auto fields = object_fields(raw);
  marshal_target_driver_ledger_t out{};
  out.schema_version = optional_string(
      fields, "schema_version", k_marshal_target_driver_ledger_schema_v1);
  out.target_driver_run_id = optional_string(fields, "target_driver_run_id");
  out.target_id = optional_string(fields, "target_id");
  if (const auto raw_identity = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_active_identity(*raw_identity);
  }
  out.drive_mode =
      parse_drive_mode_text(optional_string(fields, "drive_mode", "one_step"));
  out.requested_mode =
      parse_mode_text(optional_string(fields, "requested_mode", "dry_run"));
  if (const auto raw_policy = optional_raw(fields, "driver_policy")) {
    out.driver_policy = parse_driver_policy(*raw_policy);
  }
  out.driver_policy_digest = optional_string(fields, "driver_policy_digest");
  out.resumed_from_run_id = optional_string(fields, "resumed_from_run_id");
  out.resumed_iteration_count =
      optional_i64(fields, "resumed_iteration_count", 0);
  out.iteration_count = optional_i64(fields, "iteration_count", 0);
  out.runtime_handoff_attempt_count =
      optional_i64(fields, "runtime_handoff_attempt_count", 0);
  out.execution_attempt_count =
      optional_i64(fields, "execution_attempt_count", 0);
  out.last_target_deficit_digest =
      optional_string(fields, "last_target_deficit_digest");
  out.last_suggested_wave_digest =
      optional_string(fields, "last_suggested_wave_digest");
  out.last_progress_signature =
      optional_string(fields, "last_progress_signature");
  out.last_runtime_job_id = optional_string(fields, "last_runtime_job_id");
  out.last_runtime_job_state_digest =
      optional_string(fields, "last_runtime_job_state_digest");
  out.last_runtime_job_manifest_digest =
      optional_string(fields, "last_runtime_job_manifest_digest");
  out.last_runtime_terminal_fact_digest =
      optional_string(fields, "last_runtime_terminal_fact_digest");
  out.last_runtime_checkpoint_io_fact_digest =
      optional_string(fields, "last_runtime_checkpoint_io_fact_digest");
  out.last_runtime_handoff_id =
      optional_string(fields, "last_runtime_handoff_id");
  out.last_runtime_handoff_digest =
      optional_string(fields, "last_runtime_handoff_digest");
  out.last_runtime_policy_digest =
      optional_string(fields, "last_runtime_policy_digest");
  out.terminal_state = optional_string(fields, "terminal_state");
  out.terminal_reason = optional_string(fields, "terminal_reason");
  out.non_authority_statement =
      optional_string(fields, "non_authority_statement",
                      k_marshal_dispatch_non_authority_statement);
  if (ledger_digest != nullptr) {
    *ledger_digest = optional_string(fields, "ledger_digest");
  }
  if (compacted_fields != nullptr) {
    *compacted_fields = optional_string_array(fields, "compacted_fields");
  }
  if (has_iteration_bodies != nullptr) {
    *has_iteration_bodies = false;
  }
  if (const auto iterations_raw = optional_raw(fields, "iterations")) {
    const auto iteration_values = array_values(*iterations_raw);
    if (has_iteration_bodies != nullptr) {
      *has_iteration_bodies = !iteration_values.empty();
    }
    for (const auto &iteration_raw : iteration_values) {
      std::string supplied_digest;
      auto iteration = parse_target_driver_iteration_for_replay(
          iteration_raw, &supplied_digest);
      const auto expected_digest = target_driver_iteration_digest(iteration);
      if (audit != nullptr && supplied_digest.empty()) {
        audit->accepted = false;
        audit->issues.emplace_back("missing_iteration_digest");
      } else if (audit != nullptr && supplied_digest != expected_digest) {
        audit->accepted = false;
        audit->issues.emplace_back("iteration_digest_mismatch");
      }
      out.iterations.push_back(std::move(iteration));
    }
  }
  return out;
}

[[nodiscard]] inline target_driver_replay_audit_t
replay_target_driver_ledger(const std::string &ledger_json,
                            const target_driver_replay_context_t &context) {
  target_driver_replay_audit_t audit{};
  std::string supplied_ledger_digest;
  std::vector<std::string> compacted_fields;
  bool has_iteration_bodies = false;
  marshal_target_driver_ledger_t ledger{};
  try {
    ledger = parse_target_driver_ledger_for_replay(
        ledger_json, &supplied_ledger_digest, &compacted_fields,
        &has_iteration_bodies, &audit);
  } catch (const std::exception &ex) {
    audit.accepted = false;
    audit.issues.emplace_back(std::string("parse_failed:") + ex.what());
    return audit;
  }
  audit.compact = !has_iteration_bodies && ledger.iteration_count > 0;
  if (ledger.schema_version != k_marshal_target_driver_ledger_schema_v1) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("unsupported_schema_version");
  }
  if (ledger.non_authority_statement.find("target satisfaction remains") ==
      std::string::npos) {
    audit.accepted = false;
    audit.issues.emplace_back("missing_non_authority_statement");
  }
  if (!context.target_id.empty() && ledger.target_id != context.target_id) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_target_id");
  }
  if (!context.drive_mode.empty() &&
      to_string(ledger.drive_mode) != context.drive_mode) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_drive_mode");
  }
  if (!context.requested_mode.empty() &&
      to_string(ledger.requested_mode) != context.requested_mode) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_requested_mode");
  }
  if (!context.driver_policy_digest.empty() &&
      ledger.driver_policy_digest != context.driver_policy_digest) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_driver_policy_digest");
  }
  const auto &expected_identity = context.active_identity;
  const auto &actual_identity = ledger.active_identity;
  if (!expected_identity.protocol_contract_fingerprint.empty() &&
      actual_identity.protocol_contract_fingerprint !=
          expected_identity.protocol_contract_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_protocol_contract_fingerprint");
  }
  if (!expected_identity.graph_order_fingerprint.empty() &&
      actual_identity.graph_order_fingerprint !=
          expected_identity.graph_order_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_graph_order_fingerprint");
  }
  if (!expected_identity.target_spec_fingerprint.empty() &&
      actual_identity.target_spec_fingerprint !=
          expected_identity.target_spec_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_target_spec_fingerprint");
  }
  if (!expected_identity.split_policy_fingerprint.empty() &&
      actual_identity.split_policy_fingerprint !=
          expected_identity.split_policy_fingerprint) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_split_policy_fingerprint");
  }
  if (!context.runtime_policy_digest.empty() &&
      ledger.last_runtime_policy_digest != context.runtime_policy_digest) {
    audit.accepted = false;
    audit.stale = true;
    audit.issues.emplace_back("stale_runtime_policy_digest");
  }
  if (ledger.execution_attempt_count > 0 &&
      (ledger.last_runtime_job_id.empty() ||
       ledger.last_runtime_job_manifest_digest.empty() ||
       ledger.last_runtime_terminal_fact_digest.empty() ||
       ledger.last_runtime_handoff_digest.empty())) {
    audit.accepted = false;
    audit.issues.emplace_back("missing_runtime_terminal_evidence_identity");
  }
  if (supplied_ledger_digest.empty()) {
    audit.accepted = false;
    audit.issues.emplace_back("missing_ledger_digest");
  } else if (!audit.compact) {
    const auto expected_ledger_digest = target_driver_ledger_digest(ledger);
    if (supplied_ledger_digest != expected_ledger_digest) {
      audit.accepted = false;
      audit.issues.emplace_back("ledger_digest_mismatch");
    }
    if (ledger.iteration_count !=
        static_cast<std::int64_t>(ledger.iterations.size())) {
      audit.accepted = false;
      audit.issues.emplace_back("iteration_count_mismatch");
    }
  } else {
    if (!string_vector_contains(compacted_fields, "iterations")) {
      audit.accepted = false;
      audit.issues.emplace_back("compact_ledger_missing_compaction_note");
    }
    audit.explanation =
        "compact target-driver ledger retained identity and durable evidence "
        "digests but removed iteration bodies; Runtime and Lattice evidence "
        "remain authoritative, and Marshal replay does not prove target "
        "satisfaction";
  }
  return audit;
}

[[nodiscard]] inline std::string current_utc_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&now_time, &tm);
  char buffer[32]{};
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buffer);
}

[[nodiscard]] inline std::string
structured_content_json(const std::string &tool_result_or_structured_json) {
  const auto top = object_fields(tool_result_or_structured_json);
  if (const auto structured = optional_raw(top, "structuredContent")) {
    return *structured;
  }
  return tool_result_or_structured_json;
}

[[nodiscard]] inline bool
tool_result_has_error_marker(const std::string &tool_result_json) {
  try {
    const auto fields = object_fields(tool_result_json);
    if (const auto raw = optional_raw(fields, "isError")) {
      return trim_ascii(*raw) == "true";
    }
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

[[nodiscard]] inline std::string lowercase_ascii(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const unsigned char c : value) {
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

[[nodiscard]] inline int warning_severity_rank(const std::string &severity) {
  const std::string value = lowercase_ascii(trim_ascii(severity));
  if (value.empty()) {
    return 0;
  }
  if (value == "info" || value == "notice") {
    return 1;
  }
  if (value == "watch") {
    return 2;
  }
  if (value == "warn" || value == "warning") {
    return 3;
  }
  if (value == "error" || value == "severe" || value == "critical" ||
      value == "blocking") {
    return 4;
  }
  return -1;
}

struct typed_warning_envelope_t {
  bool valid{false};
  std::string warning_id{};
  std::string severity{};
  int severity_rank{0};
  std::string source{};
  std::string component{};
  std::string scope{};
  bool blocking{false};
  std::string evidence_digest{};
  std::vector<std::string> issues{};
};

struct warning_stop_decision_t {
  bool stop{false};
  bool malformed{false};
  typed_warning_envelope_t warning{};
  std::string reason_code{};
};

[[nodiscard]] inline typed_warning_envelope_t parse_typed_warning_envelope(
    const std::map<std::string, std::string> &warning) {
  typed_warning_envelope_t out{};
  out.warning_id = optional_string(warning, "warning_id");
  out.severity = optional_string(warning, "severity");
  out.source = optional_string(warning, "source");
  out.component = optional_string(warning, "component");
  out.scope = optional_string(warning, "scope");
  out.evidence_digest =
      first_non_empty({optional_string(warning, "evidence_digest"),
                       optional_string(warning, "evidence_basis")});
  if (out.warning_id.empty()) {
    out.issues.emplace_back("missing_warning_id");
  }
  if (out.severity.empty()) {
    out.issues.emplace_back("missing_severity");
  } else {
    out.severity_rank = warning_severity_rank(out.severity);
    if (out.severity_rank < 0) {
      out.issues.emplace_back("unknown_severity");
    }
  }
  if (out.source.empty()) {
    out.issues.emplace_back("missing_source");
  } else if (out.source != "runtime" && out.source != "lattice") {
    out.issues.emplace_back("unknown_source");
  }
  if (out.component.empty()) {
    out.issues.emplace_back("missing_component");
  }
  if (out.scope.empty()) {
    out.issues.emplace_back("missing_scope");
  }
  if (out.evidence_digest.empty()) {
    out.issues.emplace_back("missing_evidence_digest");
  }
  const auto blocking_it = warning.find("blocking");
  if (blocking_it == warning.end()) {
    out.issues.emplace_back("missing_blocking");
  } else {
    out.blocking = optional_bool(warning, "blocking", false);
  }
  out.valid = out.issues.empty();
  return out;
}

[[nodiscard]] inline std::string
warning_malformed_reason_code(const std::string &expected_source,
                              const typed_warning_envelope_t &warning) {
  const std::string prefix = expected_source + "_warning_";
  for (const auto &issue : warning.issues) {
    if (issue == "missing_severity") {
      return prefix + "missing_severity";
    }
    if (issue == "unknown_severity") {
      return prefix + "unknown_severity";
    }
    if (issue == "missing_warning_id") {
      return prefix + "missing_warning_id";
    }
    if (issue == "missing_source" || issue == "unknown_source") {
      return prefix + issue;
    }
    if (issue == "missing_blocking") {
      return prefix + "missing_blocking";
    }
    if (issue == "missing_evidence_digest") {
      return prefix + "missing_evidence_digest";
    }
  }
  return prefix + "malformed";
}

[[nodiscard]] inline warning_stop_decision_t
warning_stop_decision(const std::string &tool_result_json,
                      const std::string &expected_source,
                      const std::string &threshold) {
  warning_stop_decision_t decision{};
  std::string structured = tool_result_json;
  try {
    structured = structured_content_json(tool_result_json);
  } catch (const std::exception &) {
  }
  std::map<std::string, std::string> fields;
  try {
    fields = object_fields(structured);
  } catch (const std::exception &) {
    return decision;
  }
  const auto warnings_raw = optional_raw(fields, "warnings");
  if (!warnings_raw.has_value()) {
    return decision;
  }
  const int threshold_rank =
      trim_ascii(threshold).empty() ? 2 : warning_severity_rank(threshold);
  if (threshold_rank < 0) {
    decision.stop = true;
    decision.malformed = true;
    decision.reason_code = expected_source + "_warning_unknown_threshold";
    return decision;
  }
  std::optional<typed_warning_envelope_t> highest;
  std::optional<typed_warning_envelope_t> malformed;
  for (const auto &warning_raw : array_values(*warnings_raw)) {
    const auto warning = object_fields(warning_raw);
    auto typed = parse_typed_warning_envelope(warning);
    if (!typed.source.empty() && typed.source != expected_source) {
      continue;
    }
    if (!typed.valid) {
      malformed = typed;
      continue;
    }
    if (!highest.has_value() || typed.severity_rank > highest->severity_rank ||
        (typed.severity_rank == highest->severity_rank && typed.blocking &&
         !highest->blocking)) {
      highest = std::move(typed);
    }
  }
  if (malformed.has_value()) {
    decision.stop = true;
    decision.malformed = true;
    decision.warning = *malformed;
    decision.reason_code =
        warning_malformed_reason_code(expected_source, *malformed);
    return decision;
  }
  if (!highest.has_value()) {
    return decision;
  }
  decision.warning = *highest;
  if (highest->blocking) {
    decision.stop = true;
    decision.reason_code = expected_source + "_warning_blocking";
    return decision;
  }
  if (highest->severity_rank >= threshold_rank) {
    decision.stop = true;
    decision.reason_code = expected_source + "_warning_severity";
  }
  return decision;
}

[[nodiscard]] inline bool
warning_stop_triggered(const std::string &tool_result_json,
                       const std::string &expected_source,
                       const std::string &threshold) {
  return warning_stop_decision(tool_result_json, expected_source, threshold)
      .stop;
}

struct runtime_terminal_evidence_t {
  bool observed{false};
  std::string job_id{};
  std::filesystem::path job_dir{};
  std::filesystem::path job_state_path{};
  std::filesystem::path job_manifest_path{};
  std::filesystem::path terminal_fact_path{};
  std::filesystem::path checkpoint_io_fact_path{};
  std::string job_state_digest{};
  std::string job_manifest_digest{};
  std::string terminal_fact_digest{};
  std::string checkpoint_io_fact_digest{};
  std::string terminal_status{};
  std::string runtime_handoff_id{};
  std::string runtime_handoff_digest{};
  bool runtime_result_fact_available{false};
  bool checkpoint_io_fact_available{false};
  bool checkpoint_io_required{false};
};

[[nodiscard]] inline bool terminal_runtime_status(const std::string &status) {
  const std::string value = trim_ascii(status);
  return value == "completed" || value == "failed" || value == "skipped" ||
         value == "dry_run";
}

[[nodiscard]] inline std::filesystem::path
path_from_artifact_summary(const std::map<std::string, std::string> &summary) {
  return std::filesystem::path(optional_string(summary, "path"));
}

[[nodiscard]] inline bool checkpoint_io_required_from_terminal_fields(
    const std::map<std::string, std::string> &state,
    const std::map<std::string, std::string> &manifest,
    const std::map<std::string, std::string> &result_fact,
    const std::map<std::string, std::string> &checkpoint_io_fact) {
  const auto checkpoint_written = first_non_empty(
      {operational_report_detail::get(checkpoint_io_fact, "checkpoint_written"),
       operational_report_detail::get(result_fact, "checkpoint_written"),
       operational_report_detail::get(state, "checkpoint_written")});
  if (operational_report_detail::bool_value(checkpoint_written)) {
    return true;
  }
  const auto representation_loaded = first_non_empty(
      {operational_report_detail::get(checkpoint_io_fact,
                                      "representation_checkpoint_loaded"),
       operational_report_detail::get(result_fact,
                                      "representation_checkpoint_loaded")});
  const auto mdn_loaded = first_non_empty(
      {operational_report_detail::get(checkpoint_io_fact,
                                      "mdn_checkpoint_loaded"),
       operational_report_detail::get(result_fact, "mdn_checkpoint_loaded")});
  if (operational_report_detail::bool_value(representation_loaded) ||
      operational_report_detail::bool_value(mdn_loaded)) {
    return true;
  }
  const auto representation_path = first_non_empty(
      {operational_report_detail::get(checkpoint_io_fact,
                                      "representation_checkpoint_path"),
       operational_report_detail::get(result_fact,
                                      "representation_checkpoint_path"),
       operational_report_detail::get(manifest,
                                      "input_representation_checkpoint_path")});
  const auto mdn_path = first_non_empty(
      {operational_report_detail::get(checkpoint_io_fact,
                                      "mdn_checkpoint_path"),
       operational_report_detail::get(result_fact, "mdn_checkpoint_path"),
       operational_report_detail::get(manifest, "input_mdn_checkpoint_path")});
  return !representation_path.empty() || !mdn_path.empty();
}

[[nodiscard]] inline runtime_terminal_evidence_t
runtime_terminal_evidence_from_handoff(
    const marshal_runtime_hero_handoff_result_t &handoff) {
  runtime_terminal_evidence_t out{};
  std::string structured;
  try {
    structured = structured_content_json(handoff.tool_result_json);
  } catch (const std::exception &) {
    return out;
  }
  std::map<std::string, std::string> fields;
  try {
    fields = object_fields(structured);
  } catch (const std::exception &) {
    return out;
  }

  if (const auto artifacts_raw = optional_raw(fields, "artifacts")) {
    const auto artifacts = object_fields(*artifacts_raw);
    out.job_dir = std::filesystem::path(optional_string(artifacts, "job_dir"));
    if (const auto state_raw = optional_raw(artifacts, "state")) {
      out.job_state_path =
          path_from_artifact_summary(object_fields(*state_raw));
    }
    if (const auto manifest_raw = optional_raw(artifacts, "manifest")) {
      out.job_manifest_path =
          path_from_artifact_summary(object_fields(*manifest_raw));
    }
  }

  if (const auto stdout_raw = optional_raw(fields, "stdout_fields")) {
    const auto stdout_fields = optional_string_map(fields, "stdout_fields");
    const auto manifest_it = stdout_fields.find("manifest_path");
    if (manifest_it != stdout_fields.end() && out.job_manifest_path.empty()) {
      out.job_manifest_path = std::filesystem::path(manifest_it->second);
    }
    const auto job_id_it = stdout_fields.find("job_id");
    if (job_id_it != stdout_fields.end()) {
      out.job_id = job_id_it->second;
    }
  }

  if (out.job_dir.empty() && !out.job_manifest_path.empty()) {
    out.job_dir = out.job_manifest_path.parent_path();
  }
  if (!out.job_dir.empty()) {
    if (out.job_state_path.empty()) {
      out.job_state_path = out.job_dir / "job.state";
    }
    if (out.job_manifest_path.empty()) {
      out.job_manifest_path = out.job_dir / "job.manifest";
    }
    out.terminal_fact_path = out.job_dir / "runtime.result.fact";
    out.checkpoint_io_fact_path = out.job_dir / "runtime.checkpoint_io.fact";
  }

  const auto state =
      operational_report_detail::read_kv_file(out.job_state_path);
  const auto manifest =
      operational_report_detail::read_kv_file(out.job_manifest_path);
  const auto result_fact =
      operational_report_detail::read_kv_file(out.terminal_fact_path);
  const auto checkpoint_io_fact =
      operational_report_detail::read_kv_file(out.checkpoint_io_fact_path);
  out.job_id = first_non_empty(
      {out.job_id, operational_report_detail::get(state, "job_id"),
       operational_report_detail::get(manifest, "job_id"),
       operational_report_detail::get(result_fact, "job_id")});
  out.terminal_status =
      first_non_empty({operational_report_detail::get(result_fact, "status"),
                       operational_report_detail::get(state, "status")});
  out.runtime_handoff_id = first_non_empty(
      {operational_report_detail::get(result_fact, "runtime_handoff_id"),
       operational_report_detail::get(manifest, "runtime_handoff_id")});
  out.runtime_handoff_digest = first_non_empty(
      {operational_report_detail::get(result_fact, "runtime_handoff_digest"),
       operational_report_detail::get(manifest, "runtime_handoff_digest")});
  out.job_state_digest = detail::file_content_digest_or_empty(
      out.job_state_path, "kikijyeba.marshal.runtime_job_state.v1");
  out.job_manifest_digest = detail::file_content_digest_or_empty(
      out.job_manifest_path, "kikijyeba.marshal.runtime_job_manifest.v1");
  out.terminal_fact_digest = detail::file_content_digest_or_empty(
      out.terminal_fact_path, "kikijyeba.marshal.runtime_terminal_fact.v1");
  out.checkpoint_io_fact_digest = detail::file_content_digest_or_empty(
      out.checkpoint_io_fact_path,
      "kikijyeba.marshal.runtime_checkpoint_io_fact.v1");
  out.runtime_result_fact_available = !result_fact.empty();
  out.checkpoint_io_fact_available = !checkpoint_io_fact.empty();
  out.checkpoint_io_required = checkpoint_io_required_from_terminal_fields(
      state, manifest, result_fact, checkpoint_io_fact);
  out.observed =
      !out.job_id.empty() && terminal_runtime_status(out.terminal_status) &&
      !out.job_manifest_digest.empty() && !out.terminal_fact_digest.empty();
  return out;
}

[[nodiscard]] inline marshal_active_identity_t
parse_lattice_active_identity(const std::string &raw,
                              const std::string &target_spec_fingerprint,
                              const std::string &split_policy_fingerprint) {
  const auto fields = object_fields(raw);
  marshal_active_identity_t out{};
  out.protocol_contract_fingerprint =
      optional_string(fields, "protocol_contract_fingerprint");
  out.graph_order_fingerprint =
      optional_string(fields, "graph_order_fingerprint");
  out.target_spec_fingerprint = target_spec_fingerprint;
  out.split_policy_fingerprint = split_policy_fingerprint;
  return out;
}

[[nodiscard]] inline marshal_suggested_wave_t
parse_lattice_suggested_wave(const std::string &raw) {
  const auto fields = object_fields(raw);
  marshal_suggested_wave_t out{};
  out.target = optional_string(fields, "target");
  out.mode = optional_string(fields, "mode");
  out.source_range = optional_string(fields, "source_range", "anchor_index");
  out.anchor_index_begin = optional_size(fields, "anchor_index_begin");
  out.anchor_index_end = optional_size(fields, "anchor_index_end");
  out.source_key_begin = optional_i64_nullable(fields, "source_key_begin");
  out.source_key_end = optional_i64_nullable(fields, "source_key_end");
  if (const auto raw_plan_inputs = optional_raw(fields, "plan_inputs")) {
    out.plan_inputs = optional_string_map(fields, "plan_inputs");
    (void)raw_plan_inputs;
  }
  const std::string mdn_checkpoint =
      optional_string(fields, "input_mdn_checkpoint");
  if (!mdn_checkpoint.empty()) {
    out.plan_inputs["PLAN_INPUT_MDN_CHECKPOINT"] = mdn_checkpoint;
  }
  const std::string representation_checkpoint =
      optional_string(fields, "input_representation_checkpoint");
  if (!representation_checkpoint.empty()) {
    out.plan_inputs["PLAN_INPUT_REPRESENTATION_CHECKPOINT"] =
        representation_checkpoint;
  }
  return out;
}

[[nodiscard]] inline marshal_plan_basis_t
parse_lattice_plan_basis(const std::string &raw,
                         const marshal_suggested_wave_t &wave) {
  const auto fields = object_fields(raw);
  marshal_plan_basis_t out{};
  out.present = true;
  out.available = optional_bool(fields, "available", false);
  out.source_range =
      wave.source_range.empty() ? "anchor_index" : wave.source_range;
  out.target_anchor_index_begin = wave.anchor_index_begin;
  out.target_anchor_index_end = wave.anchor_index_end;
  out.target_source_key_begin = wave.source_key_begin;
  out.target_source_key_end = wave.source_key_end;
  out.primary_deficit_key = optional_string(fields, "primary_deficit_key");
  out.primary_deficit_message =
      first_non_empty({optional_string(fields, "primary_deficit_message"),
                       optional_string(fields, "reason")});
  out.deficit_keys = optional_string_array(fields, "deficit_keys");
  out.deficit_priority_classes =
      optional_string_array(fields, "deficit_priority_classes");
  return out;
}

[[nodiscard]] inline marshal_dispatch_advice_t
materialize_advice_from_lattice_plan_result(
    const std::string &lattice_tool_result_json,
    const std::string &source_lattice_timestamp, std::int64_t max_waves,
    std::int64_t recommendation_attempt_count) {
  const auto structured_json =
      structured_content_json(lattice_tool_result_json);
  const auto fields = object_fields(structured_json);
  const auto suggested_wave_raw =
      optional_non_null_raw(fields, "suggested_wave");
  const auto target_status = optional_string(fields, "status");
  if (!suggested_wave_raw.has_value() && target_status != "satisfied") {
    throw std::runtime_error("Lattice plan result missing suggested_wave");
  }
  const auto proof_raw = optional_non_null_raw(fields, "proof_certificate");
  std::string target_spec_fingerprint;
  std::string split_policy_fingerprint =
      optional_string(fields, "split_policy_fingerprint");
  if (proof_raw.has_value()) {
    const auto proof_fields = object_fields(*proof_raw);
    target_spec_fingerprint =
        optional_string(proof_fields, "target_spec_fingerprint");
    split_policy_fingerprint = first_non_empty(
        {optional_string(proof_fields, "split_policy_fingerprint"),
         split_policy_fingerprint});
  }

  marshal_dispatch_advice_t out{};
  out.config_path = optional_string(fields, "config_path");
  out.runtime_root = optional_string(fields, "runtime_root");
  out.target_id = optional_string(fields, "target_id");
  out.target_status = target_status;
  if (suggested_wave_raw.has_value()) {
    out.suggested_wave = parse_lattice_suggested_wave(*suggested_wave_raw);
  }
  if (const auto active_raw = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_lattice_active_identity(
        *active_raw, target_spec_fingerprint, split_policy_fingerprint);
  } else {
    out.active_identity.target_spec_fingerprint = target_spec_fingerprint;
    out.active_identity.split_policy_fingerprint = split_policy_fingerprint;
  }
  if (const auto plan_basis_raw = optional_non_null_raw(fields, "plan_basis")) {
    out.plan_basis =
        parse_lattice_plan_basis(*plan_basis_raw, out.suggested_wave);
  }
  out.plan_basis_digest = plan_basis_digest(out.plan_basis);
  out.suggested_wave_digest = suggested_wave_digest(out.suggested_wave);
  out.plan_input_digest = plan_input_digest(out.suggested_wave.plan_inputs);
  out.max_waves = max_waves;
  out.recommendation_attempt_count = recommendation_attempt_count;
  for (const auto &[key, value] : out.suggested_wave.plan_inputs) {
    if (!value.empty()) {
      out.required_plan_inputs.push_back(key);
    }
  }
  std::sort(out.required_plan_inputs.begin(), out.required_plan_inputs.end());
  out.source_lattice_tool = "hero.lattice.target_deficit";
  out.source_lattice_timestamp = source_lattice_timestamp.empty()
                                     ? current_utc_timestamp()
                                     : source_lattice_timestamp;
  out.advice_receipt_digest = marshal_digest_for_text(
      "kikijyeba.marshal.materialized_lattice_plan_result.v1", structured_json);
  return out;
}

[[nodiscard]] inline std::string
optional_size_json(const std::optional<std::size_t> &value) {
  return value.has_value() ? std::to_string(*value) : "null";
}

[[nodiscard]] inline std::string
optional_i64_json(const std::optional<std::int64_t> &value) {
  return value.has_value() ? std::to_string(*value) : "null";
}

[[nodiscard]] inline std::string
string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << detail::json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::string
string_map_json(const std::map<std::string, std::string> &values) {
  std::ostringstream out;
  out << "{";
  std::size_t i = 0;
  for (const auto &[key, value] : values) {
    if (i != 0) {
      out << ",";
    }
    out << detail::json_quote(key) << ":" << detail::json_quote(value);
    ++i;
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
driver_policy_json(const marshal_target_driver_policy_t &policy) {
  std::ostringstream out;
  out << "{\"max_waves\":" << policy.max_waves
      << ",\"max_wall_clock_seconds\":" << policy.max_wall_clock_seconds
      << ",\"allow_execute\":" << (policy.allow_execute ? "true" : "false")
      << ",\"allow_train_execute\":"
      << (policy.allow_train_execute ? "true" : "false")
      << ",\"require_runtime_job_completion\":"
      << (policy.require_runtime_job_completion ? "true" : "false")
      << ",\"require_post_wave_lattice_satisfied_check\":"
      << (policy.require_post_wave_lattice_satisfied_check ? "true" : "false")
      << ",\"stop_on_warning_severity\":"
      << detail::json_quote(policy.stop_on_warning_severity)
      << ",\"stop_on_lattice_warning\":"
      << (policy.stop_on_lattice_warning ? "true" : "false")
      << ",\"stop_on_runtime_warning\":"
      << (policy.stop_on_runtime_warning ? "true" : "false")
      << ",\"no_progress_window\":" << policy.no_progress_window << "}";
  return out.str();
}

[[nodiscard]] inline std::string target_driver_iteration_json(
    const marshal_target_driver_iteration_t &iteration) {
  std::ostringstream out;
  out << "{\"iteration_index\":" << iteration.iteration_index
      << ",\"active_identity\":{\"protocol_contract_fingerprint\":"
      << detail::json_quote(
             iteration.active_identity.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << detail::json_quote(iteration.active_identity.graph_order_fingerprint)
      << ",\"target_spec_fingerprint\":"
      << detail::json_quote(iteration.active_identity.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << detail::json_quote(iteration.active_identity.split_policy_fingerprint)
      << "}"
      << ",\"target_status\":" << detail::json_quote(iteration.target_status)
      << ",\"dispatch_state\":" << detail::json_quote(iteration.dispatch_state)
      << ",\"blocker_bucket\":" << detail::json_quote(iteration.blocker_bucket)
      << ",\"next_action\":" << detail::json_quote(iteration.next_action)
      << ",\"terminal_state\":" << detail::json_quote(iteration.terminal_state)
      << ",\"terminal_reason\":"
      << detail::json_quote(iteration.terminal_reason)
      << ",\"target_deficit_digest\":"
      << detail::json_quote(iteration.target_deficit_digest)
      << ",\"suggested_wave_digest\":"
      << detail::json_quote(iteration.suggested_wave_digest)
      << ",\"plan_input_digest\":"
      << detail::json_quote(iteration.plan_input_digest)
      << ",\"advice_digest\":" << detail::json_quote(iteration.advice_digest)
      << ",\"request_digest\":" << detail::json_quote(iteration.request_digest)
      << ",\"runtime_policy_digest\":"
      << detail::json_quote(iteration.runtime_policy_digest)
      << ",\"runtime_wave_digest\":"
      << detail::json_quote(iteration.runtime_wave_digest)
      << ",\"runtime_preview_request_digest\":"
      << detail::json_quote(iteration.runtime_preview_request_digest)
      << ",\"runtime_execution_request_digest\":"
      << detail::json_quote(iteration.runtime_execution_request_digest)
      << ",\"runtime_handoff_arguments_digest\":"
      << detail::json_quote(iteration.runtime_handoff_arguments_digest)
      << ",\"runtime_handoff_id\":"
      << detail::json_quote(iteration.runtime_handoff_id)
      << ",\"runtime_handoff_digest\":"
      << detail::json_quote(iteration.runtime_handoff_digest)
      << ",\"runtime_response_digest\":"
      << detail::json_quote(iteration.runtime_response_digest)
      << ",\"dry_run_response_digest\":"
      << detail::json_quote(iteration.dry_run_response_digest)
      << ",\"runtime_job_id\":" << detail::json_quote(iteration.runtime_job_id)
      << ",\"runtime_job_state_digest\":"
      << detail::json_quote(iteration.runtime_job_state_digest)
      << ",\"runtime_job_manifest_digest\":"
      << detail::json_quote(iteration.runtime_job_manifest_digest)
      << ",\"runtime_terminal_fact_digest\":"
      << detail::json_quote(iteration.runtime_terminal_fact_digest)
      << ",\"runtime_checkpoint_io_fact_digest\":"
      << detail::json_quote(iteration.runtime_checkpoint_io_fact_digest)
      << ",\"runtime_terminal_status\":"
      << detail::json_quote(iteration.runtime_terminal_status)
      << ",\"post_run_lattice_evaluation_digest\":"
      << detail::json_quote(iteration.post_run_lattice_evaluation_digest)
      << ",\"progress_signature\":"
      << detail::json_quote(iteration.progress_signature)
      << ",\"dry_run_attempted\":"
      << (iteration.dry_run_attempted ? "true" : "false")
      << ",\"dry_run_accepted\":"
      << (iteration.dry_run_accepted ? "true" : "false")
      << ",\"execution_attempted\":"
      << (iteration.execution_attempted ? "true" : "false")
      << ",\"execution_accepted\":"
      << (iteration.execution_accepted ? "true" : "false")
      << ",\"runtime_job_completion_observed\":"
      << (iteration.runtime_job_completion_observed ? "true" : "false")
      << ",\"lattice_target_satisfied_after_wave\":"
      << (iteration.lattice_target_satisfied_after_wave ? "true" : "false")
      << ",\"refusal_reasons\":" << string_array_json(iteration.refusal_reasons)
      << ",\"iteration_digest\":"
      << detail::json_quote(target_driver_iteration_digest(iteration)) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
target_driver_ledger_json(const marshal_target_driver_ledger_t &ledger) {
  std::ostringstream out;
  out << "{\"schema_version\":" << detail::json_quote(ledger.schema_version)
      << ",\"target_driver_run_id\":"
      << detail::json_quote(ledger.target_driver_run_id)
      << ",\"target_id\":" << detail::json_quote(ledger.target_id)
      << ",\"active_identity\":{\"protocol_contract_fingerprint\":"
      << detail::json_quote(
             ledger.active_identity.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << detail::json_quote(ledger.active_identity.graph_order_fingerprint)
      << ",\"target_spec_fingerprint\":"
      << detail::json_quote(ledger.active_identity.target_spec_fingerprint)
      << ",\"split_policy_fingerprint\":"
      << detail::json_quote(ledger.active_identity.split_policy_fingerprint)
      << "}"
      << ",\"drive_mode\":" << detail::json_quote(to_string(ledger.drive_mode))
      << ",\"requested_mode\":"
      << detail::json_quote(to_string(ledger.requested_mode))
      << ",\"driver_policy\":" << driver_policy_json(ledger.driver_policy)
      << ",\"driver_policy_digest\":"
      << detail::json_quote(ledger.driver_policy_digest)
      << ",\"resumed_from_run_id\":"
      << detail::json_quote(ledger.resumed_from_run_id)
      << ",\"resumed_iteration_count\":" << ledger.resumed_iteration_count
      << ",\"iteration_count\":" << ledger.iteration_count
      << ",\"runtime_handoff_attempt_count\":"
      << ledger.runtime_handoff_attempt_count
      << ",\"execution_attempt_count\":" << ledger.execution_attempt_count
      << ",\"last_target_deficit_digest\":"
      << detail::json_quote(ledger.last_target_deficit_digest)
      << ",\"last_suggested_wave_digest\":"
      << detail::json_quote(ledger.last_suggested_wave_digest)
      << ",\"last_progress_signature\":"
      << detail::json_quote(ledger.last_progress_signature)
      << ",\"last_runtime_job_id\":"
      << detail::json_quote(ledger.last_runtime_job_id)
      << ",\"last_runtime_job_state_digest\":"
      << detail::json_quote(ledger.last_runtime_job_state_digest)
      << ",\"last_runtime_job_manifest_digest\":"
      << detail::json_quote(ledger.last_runtime_job_manifest_digest)
      << ",\"last_runtime_terminal_fact_digest\":"
      << detail::json_quote(ledger.last_runtime_terminal_fact_digest)
      << ",\"last_runtime_checkpoint_io_fact_digest\":"
      << detail::json_quote(ledger.last_runtime_checkpoint_io_fact_digest)
      << ",\"last_runtime_handoff_id\":"
      << detail::json_quote(ledger.last_runtime_handoff_id)
      << ",\"last_runtime_handoff_digest\":"
      << detail::json_quote(ledger.last_runtime_handoff_digest)
      << ",\"last_runtime_policy_digest\":"
      << detail::json_quote(ledger.last_runtime_policy_digest)
      << ",\"terminal_state\":" << detail::json_quote(ledger.terminal_state)
      << ",\"terminal_reason\":" << detail::json_quote(ledger.terminal_reason)
      << ",\"last_safe_point\":"
      << detail::json_quote(ledger.last_runtime_terminal_fact_digest.empty()
                                ? (ledger.last_suggested_wave_digest.empty()
                                       ? "none"
                                       : "lattice_target_deficit")
                                : "runtime_terminal_evidence")
      << ",\"next_safe_recheck\":"
      << detail::json_quote(ledger.last_runtime_terminal_fact_digest.empty()
                                ? "rerun_reach_lattice_target"
                                : "ask_lattice_to_recheck_target")
      << ",\"iterations\":[";
  for (std::size_t i = 0; i < ledger.iterations.size(); ++i) {
    if (i != 0U) {
      out << ",";
    }
    out << target_driver_iteration_json(ledger.iterations[i]);
  }
  out << "],\"ledger_digest\":"
      << detail::json_quote(target_driver_ledger_digest(ledger))
      << ",\"non_authority_statement\":"
      << detail::json_quote(ledger.non_authority_statement) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
active_identity_json(const marshal_active_identity_t &identity) {
  return "{\"protocol_contract_fingerprint\":" +
         detail::json_quote(identity.protocol_contract_fingerprint) +
         ",\"graph_order_fingerprint\":" +
         detail::json_quote(identity.graph_order_fingerprint) +
         ",\"target_spec_fingerprint\":" +
         detail::json_quote(identity.target_spec_fingerprint) +
         ",\"split_policy_fingerprint\":" +
         detail::json_quote(identity.split_policy_fingerprint) + "}";
}

[[nodiscard]] inline std::string
plan_basis_json(const marshal_plan_basis_t &basis) {
  std::ostringstream out;
  out << "{\"present\":" << (basis.present ? "true" : "false")
      << ",\"available\":" << (basis.available ? "true" : "false")
      << ",\"source_range\":" << detail::json_quote(basis.source_range)
      << ",\"target_anchor_index_begin\":"
      << optional_size_json(basis.target_anchor_index_begin)
      << ",\"target_anchor_index_end\":"
      << optional_size_json(basis.target_anchor_index_end)
      << ",\"target_source_key_begin\":"
      << optional_i64_json(basis.target_source_key_begin)
      << ",\"target_source_key_end\":"
      << optional_i64_json(basis.target_source_key_end)
      << ",\"primary_deficit_key\":"
      << detail::json_quote(basis.primary_deficit_key)
      << ",\"primary_deficit_message\":"
      << detail::json_quote(basis.primary_deficit_message)
      << ",\"deficit_keys\":" << string_array_json(basis.deficit_keys)
      << ",\"deficit_priority_classes\":"
      << string_array_json(basis.deficit_priority_classes) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
suggested_wave_json(const marshal_suggested_wave_t &wave) {
  std::ostringstream out;
  out << "{\"target\":" << detail::json_quote(wave.target)
      << ",\"mode\":" << detail::json_quote(wave.mode)
      << ",\"source_range\":" << detail::json_quote(wave.source_range)
      << ",\"anchor_index_begin\":"
      << optional_size_json(wave.anchor_index_begin)
      << ",\"anchor_index_end\":" << optional_size_json(wave.anchor_index_end)
      << ",\"source_key_begin\":" << optional_i64_json(wave.source_key_begin)
      << ",\"source_key_end\":" << optional_i64_json(wave.source_key_end)
      << ",\"plan_inputs\":" << string_map_json(wave.plan_inputs) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
advice_json(const marshal_dispatch_advice_t &advice) {
  std::ostringstream out;
  out << "{\"schema_version\":" << detail::json_quote(advice.schema_version)
      << ",\"config_path\":" << detail::json_quote(advice.config_path)
      << ",\"runtime_root\":" << detail::json_quote(advice.runtime_root)
      << ",\"target_id\":" << detail::json_quote(advice.target_id)
      << ",\"target_status\":" << detail::json_quote(advice.target_status)
      << ",\"active_identity\":" << active_identity_json(advice.active_identity)
      << ",\"plan_basis\":" << plan_basis_json(advice.plan_basis)
      << ",\"plan_basis_digest\":"
      << detail::json_quote(advice.plan_basis_digest)
      << ",\"suggested_wave\":" << suggested_wave_json(advice.suggested_wave)
      << ",\"suggested_wave_digest\":"
      << detail::json_quote(advice.suggested_wave_digest)
      << ",\"plan_input_digest\":"
      << detail::json_quote(advice.plan_input_digest)
      << ",\"max_waves\":" << advice.max_waves
      << ",\"recommendation_attempt_count\":"
      << advice.recommendation_attempt_count << ",\"required_plan_inputs\":"
      << string_array_json(advice.required_plan_inputs)
      << ",\"source_lattice_tool\":"
      << detail::json_quote(advice.source_lattice_tool)
      << ",\"source_lattice_timestamp\":"
      << detail::json_quote(advice.source_lattice_timestamp)
      << ",\"advice_receipt_digest\":"
      << detail::json_quote(advice.advice_receipt_digest)
      << ",\"advice_signature_key_id\":"
      << detail::json_quote(advice.advice_signature_key_id)
      << ",\"advice_signature\":" << detail::json_quote(advice.advice_signature)
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
request_json(const marshal_dispatch_request_t &request) {
  std::ostringstream out;
  out << "{\"schema_version\":" << detail::json_quote(request.schema_version)
      << ",\"requested_mode\":"
      << detail::json_quote(to_string(request.requested_mode))
      << ",\"target_id\":" << detail::json_quote(request.target_id)
      << ",\"config_path\":" << detail::json_quote(request.config_path)
      << ",\"runtime_root\":" << detail::json_quote(request.runtime_root)
      << ",\"advice_digest\":" << detail::json_quote(request.advice_digest)
      << ",\"operator_confirmation_token\":"
      << detail::json_quote(request.operator_confirmation_token)
      << ",\"requested_overrides\":"
      << string_map_json(request.requested_overrides) << "}";
  return out.str();
}

[[nodiscard]] inline std::optional<std::string>
optional_nullable_string(const std::map<std::string, std::string> &fields,
                         const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || trim_ascii(*raw) == "null") {
    return std::nullopt;
  }
  return parse_string_raw(*raw, key);
}

[[nodiscard]] inline bool is_latest_satisfying_hint(const std::string &value) {
  static constexpr std::string_view kPrefix = "latest_satisfying:";
  return value.rfind(std::string(kPrefix), 0) == 0;
}

[[nodiscard]] inline std::string
latest_satisfying_hint_target_id(const std::string &value) {
  static constexpr std::string_view kPrefix = "latest_satisfying:";
  if (value.rfind(std::string(kPrefix), 0) != 0) {
    return {};
  }
  return trim_ascii(std::string_view(value).substr(kPrefix.size()));
}

[[nodiscard]] inline std::optional<std::size_t>
optional_size_string_or_number(const std::map<std::string, std::string> &fields,
                               const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || trim_ascii(*raw) == "null") {
    return std::nullopt;
  }
  std::string text = trim_ascii(*raw);
  if (!text.empty() && text.front() == '"') {
    text = parse_string_raw(text, key);
  }
  if (text.empty()) {
    return std::nullopt;
  }
  try {
    const auto parsed = std::stoll(text);
    if (parsed < 0) {
      throw std::runtime_error(key + " must be non-negative");
    }
    return static_cast<std::size_t>(parsed);
  } catch (const std::exception &) {
    throw std::runtime_error(key + " must be non-negative integer or null");
  }
}

[[nodiscard]] inline std::optional<std::int64_t>
optional_i64_string_or_number(const std::map<std::string, std::string> &fields,
                              const std::string &key) {
  const auto raw = optional_raw(fields, key);
  if (!raw.has_value() || trim_ascii(*raw) == "null") {
    return std::nullopt;
  }
  std::string text = trim_ascii(*raw);
  if (!text.empty() && text.front() == '"') {
    text = parse_string_raw(text, key);
  }
  if (text.empty()) {
    return std::nullopt;
  }
  try {
    return std::stoll(text);
  } catch (const std::exception &) {
    throw std::runtime_error(key + " must be integer or null");
  }
}

struct resolved_plan_input_t {
  std::string key{};
  std::string symbolic_hint{};
  std::string concrete_path{};
  std::string status{"concrete"};
  std::string resolution_owner{"marshal"};
  std::string resolver_receipt_digest{};
  std::string source_target_id{};
};

[[nodiscard]] inline std::string
resolved_plan_inputs_json(const std::vector<resolved_plan_input_t> &rows) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"key\":" << detail::json_quote(rows[i].key)
        << ",\"symbolic_hint\":";
    if (rows[i].symbolic_hint.empty()) {
      out << "null";
    } else {
      out << detail::json_quote(rows[i].symbolic_hint);
    }
    out << ",\"concrete_path\":";
    if (rows[i].concrete_path.empty()) {
      out << "null";
    } else {
      out << detail::json_quote(rows[i].concrete_path);
    }
    out << ",\"status\":" << detail::json_quote(rows[i].status)
        << ",\"resolution_owner\":"
        << detail::json_quote(rows[i].resolution_owner)
        << ",\"source_target_id\":";
    if (rows[i].source_target_id.empty()) {
      out << "null";
    } else {
      out << detail::json_quote(rows[i].source_target_id);
    }
    out << ",\"resolver_receipt_digest\":";
    if (rows[i].resolver_receipt_digest.empty()) {
      out << "null";
    } else {
      out << detail::json_quote(rows[i].resolver_receipt_digest);
    }
    out << "}";
  }
  out << "]";
  return out.str();
}

inline void
append_optional_lattice_arg(std::ostringstream &out, bool *has_field,
                            const std::map<std::string, std::string> &args,
                            const std::string &key);

[[nodiscard]] inline std::string
lattice_latest_satisfying_checkpoint_arguments_json(
    const std::map<std::string, std::string> &args,
    const std::string &symbolic_hint) {
  std::ostringstream out;
  bool has_field = false;
  out << "{";
  out << "\"symbolic_hint\":" << detail::json_quote(symbolic_hint);
  has_field = true;
  for (const auto &key :
       {"config_path", "runtime_root", "protocol_contract_fingerprint",
        "graph_order_fingerprint", "source_cursor_token",
        "vicreg_assembly_fingerprint", "mdn_assembly_fingerprint"}) {
    append_optional_lattice_arg(out, &has_field, args, key);
  }
  out << "}";
  return out.str();
}

inline void refresh_advice_digests(marshal_dispatch_advice_t *advice,
                                   const std::string &receipt_basis) {
  advice->plan_basis_digest = plan_basis_digest(advice->plan_basis);
  advice->suggested_wave_digest = suggested_wave_digest(advice->suggested_wave);
  advice->plan_input_digest =
      plan_input_digest(advice->suggested_wave.plan_inputs);
  advice->advice_receipt_digest = marshal_digest_for_text(
      "kikijyeba.marshal.materialized_target_dispatch_advice.v1",
      receipt_basis);
}

[[nodiscard]] inline std::vector<resolved_plan_input_t>
materialize_plan_inputs(const std::map<std::string, std::string> &args,
                        marshal_lattice_tool_callback_t callback,
                        marshal_dispatch_advice_t *advice) {
  std::vector<resolved_plan_input_t> rows;
  std::ostringstream receipt_basis;
  receipt_basis << "original_advice=" << dispatch_advice_digest(*advice)
                << "\n";
  for (auto &[key, value] : advice->suggested_wave.plan_inputs) {
    resolved_plan_input_t row{};
    row.key = key;
    if (!is_latest_satisfying_hint(value)) {
      row.concrete_path = value;
      row.status = value.empty() ? "missing" : "concrete";
      rows.push_back(row);
      receipt_basis << key << "=" << value << "\n";
      continue;
    }

    row.symbolic_hint = value;
    row.status = "unresolved";
    row.resolution_owner = "lattice";
    std::string resolver_result;
    std::string resolver_error;
    const std::string resolver_args =
        lattice_latest_satisfying_checkpoint_arguments_json(args, value);
    if (callback == nullptr) {
      row.status = "resolver_unavailable";
    } else if (callback("hero.lattice.latest_satisfying_checkpoint",
                        resolver_args, &resolver_result, &resolver_error) &&
               !tool_result_has_error_marker(resolver_result)) {
      try {
        const auto structured = structured_content_json(resolver_result);
        const auto fields = object_fields(structured);
        row.status = optional_string(fields, "resolution_status", "unresolved");
        const std::string resolver_symbolic_hint =
            optional_string(fields, "symbolic_hint");
        row.source_target_id = optional_string(fields, "source_target_id");
        row.resolver_receipt_digest =
            optional_string(fields, "resolver_receipt_digest");
        row.concrete_path =
            optional_nullable_string(fields, "concrete_path").value_or("");

        const bool ok = optional_bool(fields, "ok", false);
        const bool read_only = optional_bool(fields, "read_only", false);
        const bool runtime_executor =
            optional_bool(fields, "runtime_executor", true);
        const bool writes_evidence =
            optional_bool(fields, "writes_evidence", true);
        const bool model_selector =
            optional_bool(fields, "model_selector", true);
        const std::string selection_basis =
            optional_string(fields, "selection_basis");
        const bool resolved = optional_bool(fields, "resolved", false);
        const bool closure_checked =
            optional_bool(fields, "checkpoint_closure_checked", false);
        const bool closure_complete =
            optional_bool(fields, "checkpoint_closure_complete", false);
        const bool checkpoint_path_present =
            optional_bool(fields, "checkpoint_path_present", false);
        const bool proof_certificate_check_passed =
            optional_bool(fields, "proof_certificate_check_passed", false);
        const std::string target_status =
            optional_string(fields, "target_status");
        const std::string expected_source_target_id =
            latest_satisfying_hint_target_id(row.symbolic_hint);

        if (resolved && row.status == "resolved" && ok && read_only &&
            !runtime_executor && !writes_evidence && !model_selector &&
            selection_basis == "latest_satisfying" &&
            resolver_symbolic_hint == row.symbolic_hint &&
            row.source_target_id == expected_source_target_id &&
            target_status == "satisfied" && closure_checked &&
            closure_complete && checkpoint_path_present &&
            proof_certificate_check_passed &&
            !row.resolver_receipt_digest.empty() &&
            !row.concrete_path.empty()) {
          value = row.concrete_path;
        } else if (resolved || row.status == "resolved") {
          if (!ok) {
            row.status = "resolver_not_ok";
          } else if (!read_only || runtime_executor || writes_evidence ||
                     model_selector) {
            row.status = "resolver_authority_violation";
          } else if (selection_basis != "latest_satisfying") {
            row.status = "resolver_selection_basis_mismatch";
          } else if (resolver_symbolic_hint != row.symbolic_hint) {
            row.status = "resolver_symbolic_hint_mismatch";
          } else if (row.source_target_id != expected_source_target_id) {
            row.status = "resolver_source_target_mismatch";
          } else if (target_status != "satisfied") {
            row.status = target_status.empty()
                             ? "resolver_target_status_missing"
                             : "target_not_satisfied";
          } else if (!closure_checked) {
            row.status = "checkpoint_closure_not_checked";
          } else if (!closure_complete) {
            row.status = "checkpoint_closure_incomplete";
          } else if (!checkpoint_path_present || row.concrete_path.empty()) {
            row.status = "checkpoint_path_missing";
          } else if (!proof_certificate_check_passed) {
            row.status = "proof_certificate_check_failed";
          } else if (row.resolver_receipt_digest.empty()) {
            row.status = "resolver_receipt_missing";
          } else {
            row.status = "resolver_untrusted";
          }
          row.concrete_path.clear();
        }
      } catch (const std::exception &) {
        row.status = "resolver_result_invalid";
        row.concrete_path.clear();
      }
    } else {
      row.status = resolver_error.empty() ? "resolver_failed" : resolver_error;
    }
    rows.push_back(row);
    receipt_basis << key << ".symbolic=" << row.symbolic_hint << "\n"
                  << key << ".status=" << row.status << "\n"
                  << key << ".concrete=" << row.concrete_path << "\n"
                  << key << ".resolver_receipt=" << row.resolver_receipt_digest
                  << "\n";
  }
  refresh_advice_digests(advice, receipt_basis.str());
  return rows;
}

[[nodiscard]] inline marshal_runtime_policy_snapshot_t
parse_runtime_status_policy_snapshot(const std::string &runtime_status_result) {
  const auto structured = structured_content_json(runtime_status_result);
  const auto fields = object_fields(structured);
  marshal_runtime_policy_snapshot_t out{};
  out.runtime_hero_available = true;
  out.runtime_exec_exists = optional_bool(fields, "runtime_exec_exists", false);
  out.runtime_exec_executable =
      optional_bool(fields, "runtime_exec_executable", false);
  out.default_dry_run = optional_bool(fields, "default_dry_run", true);
  out.allow_execute = optional_bool(fields, "allow_execute", false);
  out.allow_train_execute = optional_bool(fields, "allow_train_execute", false);
  out.runtime_root = optional_string(fields, "runtime_root");
  return out;
}

[[nodiscard]] inline marshal_runtime_wave_snapshot_t
parse_runtime_wave_tool_result(const std::string &runtime_wave_result) {
  const auto structured = structured_content_json(runtime_wave_result);
  const auto fields = object_fields(structured);
  marshal_runtime_wave_snapshot_t out{};
  out.available = optional_bool(fields, "readable", false);
  out.wave_id = optional_string(fields, "wave_id");
  out.target_component_family_id =
      first_non_empty({optional_string(fields, "target_component_family_id"),
                       optional_string(fields, "target_component")});
  out.mode = optional_string(fields, "mode");
  out.source_range = optional_string(fields, "source_range", "anchor_index");
  out.source_order = optional_string(fields, "source_order");
  out.anchor_index_begin =
      optional_size_string_or_number(fields, "anchor_index_begin");
  out.anchor_index_end =
      optional_size_string_or_number(fields, "anchor_index_end");
  out.source_key_begin =
      optional_i64_string_or_number(fields, "source_key_begin");
  out.source_key_end = optional_i64_string_or_number(fields, "source_key_end");
  out.job_kind = optional_string(fields, "job_kind");
  out.train_target = optional_bool(fields, "train_target", false);
  out.model_state_inputs = optional_string_map(fields, "model_state_inputs");
  return out;
}

inline void
load_live_runtime_snapshots(const std::map<std::string, std::string> &args,
                            const marshal_dispatch_advice_t &advice,
                            marshal_runtime_policy_snapshot_t *policy,
                            marshal_runtime_wave_snapshot_t *wave) {
  cuwacunu::hero::runtime::runtime_context_t ctx{};
  ctx.global_config_path = std::filesystem::path(
      first_non_empty({optional_string(args, "config_path"), advice.config_path,
                       std::string{"/cuwacunu/src/config/.config"}}));
  ctx.policy_path = cuwacunu::hero::runtime::resolve_runtime_hero_dsl_path(
      ctx.global_config_path);
  std::string error;
  if (!cuwacunu::hero::runtime::load_runtime_policy(
          ctx.policy_path, ctx.global_config_path, &ctx.policy, &error)) {
    *policy = marshal_runtime_policy_snapshot_t{};
    policy->runtime_hero_available = false;
    *wave = marshal_runtime_wave_snapshot_t{};
    return;
  }

  std::string status_result;
  std::string status_error;
  if (cuwacunu::hero::runtime::execute_tool_json(
          "hero.runtime.status", "{}", &ctx, &status_result, &status_error) &&
      status_result.find("\"isError\":true") == std::string::npos) {
    *policy = parse_runtime_status_policy_snapshot(status_result);
  } else {
    *policy = marshal_runtime_policy_snapshot_t{};
    policy->runtime_hero_available = false;
  }

  std::string wave_result;
  std::string wave_error;
  const std::string wave_args =
      "{\"config_path\":" +
      detail::json_quote(first_non_empty(
          {optional_string(args, "config_path"), advice.config_path})) +
      "}";
  if (cuwacunu::hero::runtime::execute_tool_json(
          "hero.runtime.wave", wave_args, &ctx, &wave_result, &wave_error) &&
      wave_result.find("\"isError\":true") == std::string::npos) {
    *wave = parse_runtime_wave_tool_result(wave_result);
  } else {
    *wave = marshal_runtime_wave_snapshot_t{};
  }
}

[[nodiscard]] inline bool
string_set_has(const std::vector<marshal_refusal_reason_t> &reasons,
               marshal_refusal_reason_t needle) {
  return std::find(reasons.begin(), reasons.end(), needle) != reasons.end();
}

[[nodiscard]] inline bool
required_plan_inputs_resolved(const std::vector<resolved_plan_input_t> &rows) {
  return std::all_of(rows.begin(), rows.end(), [](const auto &row) {
    return row.symbolic_hint.empty() || row.status == "resolved";
  });
}

[[nodiscard]] inline std::string
blocker_bucket(const marshal_dispatch_decision_t &decision,
               const std::vector<resolved_plan_input_t> &resolved_inputs) {
  if (string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::target_already_satisfied)) {
    return "target_already_satisfied";
  }
  if (!required_plan_inputs_resolved(resolved_inputs) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::missing_model_state_input)) {
    return "unresolved_model_state";
  }
  if (string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::runtime_policy_refused)) {
    return "runtime_policy_refused";
  }
  if (string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::runtime_wave_mismatch) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::checkpoint_input_mismatch) ||
      string_set_has(
          decision.refusal_reasons,
          marshal_refusal_reason_t::runtime_checkpoint_input_missing) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::runtime_handoff_unavailable)) {
    return "runtime_wave_not_aligned";
  }
  if (string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::unproven_lattice_advice) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::stale_lattice_advice)) {
    return "stale_or_untrusted_advice";
  }
  if (string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::missing_plan_basis) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::missing_suggested_wave) ||
      string_set_has(decision.refusal_reasons,
                     marshal_refusal_reason_t::max_waves_exhausted) ||
      string_set_has(
          decision.refusal_reasons,
          marshal_refusal_reason_t::target_status_not_dispatchable)) {
    return "lattice_plan_unavailable";
  }
  return decision.refusal_reasons.empty() ? "none" : "blocked";
}

[[nodiscard]] inline std::string
dispatch_state_from_bucket(const std::string &bucket, bool include_dry_run,
                           bool dry_run_accepted) {
  if (bucket == "target_already_satisfied") {
    return "already_satisfied";
  }
  if (bucket != "none") {
    return "blocked";
  }
  if (include_dry_run) {
    return dry_run_accepted ? "ready_for_execution_gate" : "blocked";
  }
  return "ready_for_dry_run";
}

inline void append_diff_row(std::ostringstream &out, bool *first,
                            const std::string &field, const std::string &active,
                            const std::string &advised) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << "{\"field\":" << detail::json_quote(field)
      << ",\"active\":" << detail::json_quote(active)
      << ",\"advised\":" << detail::json_quote(advised) << "}";
}

[[nodiscard]] inline std::string
runtime_wave_match_json(const marshal_dispatch_advice_t &advice,
                        const marshal_runtime_policy_snapshot_t &policy,
                        const marshal_runtime_wave_snapshot_t &wave,
                        const std::vector<resolved_plan_input_t> &inputs) {
  const bool policy_match =
      policy.runtime_hero_available && policy.runtime_exec_exists &&
      policy.runtime_exec_executable && policy.default_dry_run;
  if (!advice.plan_basis.available || advice.suggested_wave.target.empty() ||
      advice.suggested_wave.mode.empty()) {
    std::ostringstream out;
    out << "{\"shape_match\":true"
        << ",\"checkpoint_inputs_match\":true"
        << ",\"policy_match\":" << (policy_match ? "true" : "false")
        << ",\"comparison\":"
        << detail::json_quote("not_applicable_no_suggested_wave")
        << ",\"differing_fields\":[]}";
    return out.str();
  }
  const bool shape_match =
      wave.available &&
      wave.target_component_family_id == advice.suggested_wave.target &&
      wave.mode == advice.suggested_wave.mode &&
      wave.source_range == advice.suggested_wave.source_range &&
      wave.anchor_index_begin == advice.suggested_wave.anchor_index_begin &&
      wave.anchor_index_end == advice.suggested_wave.anchor_index_end &&
      wave.source_key_begin == advice.suggested_wave.source_key_begin &&
      wave.source_key_end == advice.suggested_wave.source_key_end;
  bool any_unresolved = !required_plan_inputs_resolved(inputs);
  bool checkpoint_match = true;
  for (const auto &key : advice.required_plan_inputs) {
    const auto expected = advice.suggested_wave.plan_inputs.find(key);
    const auto actual = wave.model_state_inputs.find(key);
    if (expected == advice.suggested_wave.plan_inputs.end() ||
        expected->second.empty() ||
        is_latest_satisfying_hint(expected->second)) {
      any_unresolved = true;
      continue;
    }
    if (actual == wave.model_state_inputs.end() || actual->second.empty()) {
      checkpoint_match = false;
      continue;
    }
    if (detail::normalize_path_text(expected->second) !=
        detail::normalize_path_text(actual->second)) {
      checkpoint_match = false;
    }
  }
  std::ostringstream out;
  out << "{\"shape_match\":" << (shape_match ? "true" : "false")
      << ",\"checkpoint_inputs_match\":";
  if (any_unresolved) {
    out << detail::json_quote("pending");
  } else {
    out << (checkpoint_match ? "true" : "false");
  }
  out << ",\"policy_match\":" << (policy_match ? "true" : "false")
      << ",\"differing_fields\":[";
  bool first = true;
  if (!wave.available) {
    append_diff_row(out, &first, "available", "false", "true");
  }
  if (wave.target_component_family_id != advice.suggested_wave.target) {
    append_diff_row(out, &first, "target_component_family_id",
                    wave.target_component_family_id,
                    advice.suggested_wave.target);
  }
  if (wave.mode != advice.suggested_wave.mode) {
    append_diff_row(out, &first, "mode", wave.mode, advice.suggested_wave.mode);
  }
  if (wave.source_range != advice.suggested_wave.source_range) {
    append_diff_row(out, &first, "source_range", wave.source_range,
                    advice.suggested_wave.source_range);
  }
  if (wave.anchor_index_begin != advice.suggested_wave.anchor_index_begin) {
    append_diff_row(
        out, &first, "anchor_index_begin",
        wave.anchor_index_begin.has_value()
            ? std::to_string(*wave.anchor_index_begin)
            : "",
        advice.suggested_wave.anchor_index_begin.has_value()
            ? std::to_string(*advice.suggested_wave.anchor_index_begin)
            : "");
  }
  if (wave.anchor_index_end != advice.suggested_wave.anchor_index_end) {
    append_diff_row(
        out, &first, "anchor_index_end",
        wave.anchor_index_end.has_value()
            ? std::to_string(*wave.anchor_index_end)
            : "",
        advice.suggested_wave.anchor_index_end.has_value()
            ? std::to_string(*advice.suggested_wave.anchor_index_end)
            : "");
  }
  if (wave.source_key_begin != advice.suggested_wave.source_key_begin) {
    append_diff_row(
        out, &first, "source_key_begin",
        wave.source_key_begin.has_value()
            ? std::to_string(*wave.source_key_begin)
            : "",
        advice.suggested_wave.source_key_begin.has_value()
            ? std::to_string(*advice.suggested_wave.source_key_begin)
            : "");
  }
  if (wave.source_key_end != advice.suggested_wave.source_key_end) {
    append_diff_row(out, &first, "source_key_end",
                    wave.source_key_end.has_value()
                        ? std::to_string(*wave.source_key_end)
                        : "",
                    advice.suggested_wave.source_key_end.has_value()
                        ? std::to_string(*advice.suggested_wave.source_key_end)
                        : "");
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] inline std::string
next_action_for_state(const std::string &bucket, const std::string &state) {
  if (state == "already_satisfied") {
    return "ask_lattice_to_recheck";
  }
  if (bucket == "unresolved_model_state") {
    return "resolve_model_state";
  }
  if (bucket == "runtime_wave_not_aligned") {
    return "align_runtime_wave";
  }
  if (bucket == "runtime_policy_refused") {
    return "fix_runtime_policy";
  }
  if (bucket == "runtime_dry_run_refused") {
    return "inspect_runtime_dry_run";
  }
  if (bucket == "stale_or_untrusted_advice") {
    return "refresh_lattice_plan";
  }
  if (state == "ready_for_execution_gate") {
    return "execution_gate";
  }
  if (state == "ready_for_dry_run") {
    return "dry_run";
  }
  return "inspect_blocker";
}

[[nodiscard]] inline std::string
operator_explanation(const std::string &bucket,
                     const marshal_dispatch_advice_t &advice) {
  if (bucket == "unresolved_model_state") {
    return "Lattice advises " + advice.suggested_wave.target + " " +
           advice.suggested_wave.mode +
           ", but one or more model-state inputs still need Lattice-owned "
           "latest_satisfying resolution before dry-run.";
  }
  if (bucket == "runtime_wave_not_aligned") {
    return "Lattice advice was materialized, but Runtime active wave does not "
           "match the advised target, mode, range, or checkpoint inputs.";
  }
  if (bucket == "runtime_policy_refused") {
    return "Runtime Hero policy does not currently permit a safe handoff for "
           "this prepared dispatch.";
  }
  if (bucket == "runtime_dry_run_refused") {
    return "The target packet passed Marshal preparation, but the explicit "
           "Runtime dry-run handoff was refused or did not return accepted.";
  }
  if (bucket == "target_already_satisfied") {
    return "Lattice reports this target as already satisfied, so Marshal will "
           "not prepare a wave for it.";
  }
  if (bucket == "none") {
    return "Lattice advice is materialized and Marshal validation plus Runtime "
           "wave checks are ready for an explicit dry-run.";
  }
  return "Marshal prepared the target packet but found a blocking admission "
         "condition.";
}

[[nodiscard]] inline std::string
operator_wave_json(const marshal_suggested_wave_t &wave) {
  std::ostringstream out;
  out << "{\"target\":" << detail::json_quote(wave.target)
      << ",\"mode\":" << detail::json_quote(wave.mode)
      << ",\"source_range\":" << detail::json_quote(wave.source_range)
      << ",\"anchor_index_begin\":"
      << optional_size_json(wave.anchor_index_begin)
      << ",\"anchor_index_end\":" << optional_size_json(wave.anchor_index_end)
      << ",\"source_key_begin\":" << optional_i64_json(wave.source_key_begin)
      << ",\"source_key_end\":" << optional_i64_json(wave.source_key_end)
      << "}";
  return out.str();
}

inline void
append_optional_lattice_arg(std::ostringstream &out, bool *has_field,
                            const std::map<std::string, std::string> &args,
                            const std::string &key) {
  const auto value = optional_string(args, key);
  if (value.empty()) {
    return;
  }
  if (*has_field) {
    out << ",";
  }
  out << detail::json_quote(key) << ":" << detail::json_quote(value);
  *has_field = true;
}

[[nodiscard]] inline std::string lattice_target_deficit_arguments_json(
    const std::map<std::string, std::string> &args) {
  std::ostringstream out;
  bool has_field = false;
  out << "{";
  for (const auto &key :
       {"target_id", "config_path", "runtime_root",
        "protocol_contract_fingerprint", "graph_order_fingerprint",
        "source_cursor_token", "vicreg_assembly_fingerprint",
        "mdn_assembly_fingerprint"}) {
    append_optional_lattice_arg(out, &has_field, args, key);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string lattice_evaluate_targets_arguments_json(
    const std::map<std::string, std::string> &args,
    const std::vector<std::string> &target_ids,
    const std::filesystem::path &runtime_root,
    const std::filesystem::path &config_path) {
  std::ostringstream out;
  out << "{\"target_ids\":" << string_array_json(target_ids)
      << ",\"runtime_root\":" << detail::json_quote(runtime_root.string())
      << ",\"config_path\":" << detail::json_quote(config_path.string());
  bool has_field = true;
  (void)has_field;
  for (const auto &key :
       {"protocol_contract_fingerprint", "graph_order_fingerprint",
        "source_cursor_token", "vicreg_assembly_fingerprint",
        "mdn_assembly_fingerprint"}) {
    append_optional_lattice_arg(out, &has_field, args, key);
  }
  out << "}";
  return out.str();
}

inline void append_unique_text(std::vector<std::string> *out,
                               const std::string &value) {
  if (!out || value.empty()) {
    return;
  }
  if (std::find(out->begin(), out->end(), value) == out->end()) {
    out->push_back(value);
  }
}

inline void
append_optional_string_array(std::vector<std::string> *out,
                             const std::map<std::string, std::string> &fields,
                             const std::string &key) {
  try {
    for (const auto &value : optional_string_array(fields, key)) {
      append_unique_text(out, value);
    }
  } catch (const std::exception &) {
  }
}

inline void append_warning_ids_from_raw(std::vector<std::string> *out,
                                        const std::string &raw) {
  try {
    for (const auto &warning_raw : array_values(raw)) {
      try {
        if (!warning_raw.empty() && warning_raw.front() == '"') {
          append_unique_text(out, parse_string_raw(warning_raw, "warnings[]"));
          continue;
        }
        const auto warning = object_fields(warning_raw);
        append_unique_text(
            out, first_non_empty({optional_string(warning, "warning_id"),
                                  optional_string(warning, "id")}));
      } catch (const std::exception &) {
      }
    }
  } catch (const std::exception &) {
  }
}

[[nodiscard]] inline std::vector<marshal_lattice_target_status_t>
parse_lattice_evaluate_targets_statuses(const std::string &tool_result_json) {
  std::vector<marshal_lattice_target_status_t> out;
  const auto structured = structured_content_json(tool_result_json);
  const auto fields = object_fields(structured);
  const auto evaluations = optional_raw(fields, "evaluations");
  if (!evaluations.has_value()) {
    return out;
  }
  for (const auto &item_raw : array_values(*evaluations)) {
    const auto item = object_fields(item_raw);
    marshal_lattice_target_status_t row{};
    row.target_id = optional_string(item, "target_id");
    row.status = optional_string(item, "status", "unavailable");
    row.proof_certificate_check_passed = false;
    if (const auto proof_raw = optional_raw(item, "proof_certificate_check")) {
      const auto proof = object_fields(*proof_raw);
      row.proof_certificate_check_passed =
          optional_bool(proof, "passed", false);
      append_optional_string_array(&row.proof_certificate_issues, proof,
                                   "issues");
    }
    append_optional_string_array(&row.deficit_keys, item, "deficit_keys");
    if (const auto deficits_raw = optional_raw(item, "deficits")) {
      try {
        for (const auto &deficit_raw : array_values(*deficits_raw)) {
          if (!deficit_raw.empty() && deficit_raw.front() == '"') {
            append_unique_text(&row.deficit_keys,
                               parse_string_raw(deficit_raw, "deficits[]"));
          } else {
            const auto deficit = object_fields(deficit_raw);
            append_unique_text(
                &row.deficit_keys,
                first_non_empty({optional_string(deficit, "deficit_key"),
                                 optional_string(deficit, "key"),
                                 optional_string(deficit, "id")}));
          }
        }
      } catch (const std::exception &) {
      }
    }
    if (const auto plan_basis_raw = optional_raw(item, "plan_basis")) {
      try {
        const auto plan_basis = object_fields(*plan_basis_raw);
        append_optional_string_array(&row.deficit_keys, plan_basis,
                                     "deficit_keys");
        append_unique_text(&row.deficit_keys,
                           optional_string(plan_basis, "primary_deficit_key"));
      } catch (const std::exception &) {
      }
    }
    if (const auto warnings_raw = optional_raw(item, "warnings")) {
      append_warning_ids_from_raw(&row.warning_ids, *warnings_raw);
    }
    if (!row.target_id.empty()) {
      out.push_back(row);
    }
  }
  return out;
}

[[nodiscard]] inline std::string
refusal_reasons_json(const std::vector<marshal_refusal_reason_t> &reasons) {
  std::vector<std::string> out;
  out.reserve(reasons.size());
  for (const auto reason : reasons) {
    out.emplace_back(to_string(reason));
  }
  return string_array_json(out);
}

struct reach_lattice_target_step_result_t {
  marshal_dispatch_advice_t advice{};
  marshal_dispatch_advice_t original_advice{};
  marshal_dispatch_request_t request{};
  marshal_dispatch_validation_context_t context{};
  marshal_runtime_policy_snapshot_t policy{};
  marshal_runtime_wave_snapshot_t wave{};
  marshal_dispatch_decision_t decision{};
  std::vector<resolved_plan_input_t> model_state_inputs{};
  std::string lattice_result{};
  std::string bucket{"blocked"};
  std::string state{"blocked"};
  std::string next_action{"inspect_blocker"};
  std::string prepare_digest{};
  bool include_runtime_dry_run{false};
  bool include_machine_payload{false};
  bool dry_run_attempted{false};
  bool dry_run_accepted{false};
  marshal_dry_run_dispatch_response_t dry_run_response{};
  std::string dry_run_response_digest{};
  std::string dry_run_runtime_request_digest{};
  std::string dry_run_refusals_json{"[]"};
  std::vector<marshal_refusal_reason_t> dry_run_refusal_reasons{};
  std::string dry_run_bucket{"none"};
};

[[nodiscard]] inline reach_lattice_target_step_result_t
run_reach_lattice_target_step(const std::map<std::string, std::string> &args,
                              marshal_lattice_tool_callback_t callback,
                              std::int64_t max_waves,
                              std::int64_t recommendation_attempt_count,
                              marshal_dispatch_mode_t requested_mode,
                              bool include_runtime_dry_run,
                              bool include_machine_payload) {
  reach_lattice_target_step_result_t step{};
  step.include_runtime_dry_run = include_runtime_dry_run;
  step.include_machine_payload = include_machine_payload;

  std::string lattice_error;
  const std::string lattice_args = lattice_target_deficit_arguments_json(args);
  if (!callback("hero.lattice.target_deficit", lattice_args,
                &step.lattice_result, &lattice_error)) {
    throw std::runtime_error("Lattice Hero target_deficit failed: " +
                             lattice_error);
  }
  if (step.lattice_result.find("\"isError\":true") != std::string::npos ||
      step.lattice_result.find("\"isError\": true") != std::string::npos) {
    throw std::runtime_error("Lattice Hero target_deficit returned isError");
  }

  step.advice = materialize_advice_from_lattice_plan_result(
      step.lattice_result, optional_string(args, "source_lattice_timestamp"),
      max_waves, recommendation_attempt_count);
  step.original_advice = step.advice;
  const bool materialize_inputs =
      optional_bool(args, "materialize_plan_inputs", true);
  if (materialize_inputs) {
    step.model_state_inputs =
        materialize_plan_inputs(args, callback, &step.advice);
  } else {
    for (const auto &[key, value] : step.advice.suggested_wave.plan_inputs) {
      resolved_plan_input_t row{};
      row.key = key;
      if (is_latest_satisfying_hint(value)) {
        row.symbolic_hint = value;
        row.status = "unresolved";
        row.resolution_owner = "lattice";
      } else {
        row.concrete_path = value;
        row.status = value.empty() ? "missing" : "concrete";
      }
      step.model_state_inputs.push_back(std::move(row));
    }
  }

  step.request.requested_mode = requested_mode;
  step.request.target_id = step.advice.target_id;
  step.request.config_path = step.advice.config_path;
  step.request.runtime_root = step.advice.runtime_root;
  step.request.advice_digest = dispatch_advice_digest(step.advice);
  for (const auto &row : step.model_state_inputs) {
    if (!row.resolver_receipt_digest.empty()) {
      step.request.lattice_certificate_refs[row.key] =
          row.resolver_receipt_digest;
    }
  }

  if (const auto raw = optional_raw(args, "context")) {
    step.context = parse_context(*raw);
  }

  if (const auto raw = optional_raw(args, "runtime_policy")) {
    step.policy = parse_runtime_policy(*raw);
  }
  if (const auto raw = optional_raw(args, "runtime_wave")) {
    step.wave = parse_runtime_wave(*raw);
  }
  if (!optional_raw(args, "runtime_policy").has_value() ||
      !optional_raw(args, "runtime_wave").has_value()) {
    marshal_runtime_policy_snapshot_t live_policy{};
    marshal_runtime_wave_snapshot_t live_wave{};
    load_live_runtime_snapshots(args, step.advice, &live_policy, &live_wave);
    if (!optional_raw(args, "runtime_policy").has_value()) {
      step.policy = live_policy;
    }
    if (!optional_raw(args, "runtime_wave").has_value()) {
      step.wave = live_wave;
    }
  }

  step.decision = build_runtime_dry_run_dispatch_preview(
      step.advice, step.request, step.context, step.policy, step.wave);

  if (include_runtime_dry_run && step.decision.accepted) {
    marshal_runtime_hero_handoff_options_t options{};
    options.timeout_seconds =
        static_cast<int>(optional_i64(args, "timeout_seconds", 600));
    step.dry_run_response =
        run_marshal_dry_run_dispatch(step.advice, step.request, step.context,
                                     step.policy, step.wave, options);
    step.dry_run_attempted = true;
    step.dry_run_accepted = step.dry_run_response.accepted;
    step.dry_run_response_digest = step.dry_run_response.response_digest;
    step.dry_run_runtime_request_digest =
        step.dry_run_response.runtime_request_digest;
    step.dry_run_refusal_reasons =
        step.dry_run_response.decision.refusal_reasons;
    step.dry_run_refusals_json =
        refusal_reasons_json(step.dry_run_refusal_reasons);
    step.dry_run_bucket =
        blocker_bucket(step.dry_run_response.decision, step.model_state_inputs);
  }

  step.bucket = blocker_bucket(step.decision, step.model_state_inputs);
  if (step.bucket == "none" && include_runtime_dry_run &&
      step.dry_run_attempted && !step.dry_run_accepted) {
    step.bucket = step.dry_run_bucket == "none" ? "runtime_dry_run_refused"
                                                : step.dry_run_bucket;
  }
  step.state = dispatch_state_from_bucket(step.bucket, include_runtime_dry_run,
                                          step.dry_run_accepted);
  step.next_action = next_action_for_state(step.bucket, step.state);
  step.prepare_digest = marshal_digest_for_text(
      "kikijyeba.marshal.reach_lattice_target.v1",
      dispatch_advice_digest(step.advice) + "\n" +
          dispatch_request_digest(step.request) + "\n" +
          runtime_policy_snapshot_digest(step.policy) + "\n" +
          runtime_wave_snapshot_digest(step.wave) + "\n" + step.bucket + "\n" +
          step.state + "\n");
  return step;
}

[[nodiscard]] inline std::string
reach_terminal_state(const reach_lattice_target_step_result_t &step,
                     const marshal_target_driver_ledger_t *ledger) {
  if (ledger != nullptr && !ledger->terminal_state.empty()) {
    return ledger->terminal_state;
  }
  if (step.state == "already_satisfied") {
    return "reached";
  }
  if (step.state == "ready_for_execution_gate") {
    return "ready_for_execution_gate";
  }
  if (step.state == "ready_for_dry_run") {
    return "ready_for_dry_run";
  }
  return step.bucket == "none" ? "ready" : step.bucket;
}

[[nodiscard]] inline std::string
reach_blocking_owner(const std::string &bucket,
                     const std::string &terminal_state) {
  if (terminal_state == "reached" || terminal_state == "ready_for_dry_run" ||
      terminal_state == "ready_for_execution_gate") {
    return "none";
  }
  if (bucket == "unresolved_model_state" ||
      bucket == "lattice_plan_unavailable" ||
      bucket == "target_already_satisfied" ||
      terminal_state.find("lattice") != std::string::npos) {
    return "lattice";
  }
  if (bucket == "runtime_wave_not_aligned" ||
      bucket == "runtime_policy_refused" ||
      bucket == "runtime_dry_run_refused" ||
      terminal_state.find("runtime") != std::string::npos) {
    return "runtime";
  }
  if (terminal_state.find("max_waves") != std::string::npos ||
      terminal_state.find("timeout") != std::string::npos ||
      terminal_state.find("no_progress") != std::string::npos) {
    return "marshal";
  }
  return "marshal";
}

[[nodiscard]] inline std::string
reach_operator_summary_json(const reach_lattice_target_step_result_t &step,
                            const marshal_target_driver_ledger_t *ledger) {
  const auto terminal_state = reach_terminal_state(step, ledger);
  std::string headline = "Target dispatch prepared.";
  if (terminal_state == "reached") {
    headline = "Lattice target is reached.";
  } else if (step.state == "ready_for_execution_gate") {
    headline = "Runtime dry-run accepted; execution gate is the next step.";
  } else if (step.state == "ready_for_dry_run") {
    headline = "Target wave is ready for explicit Runtime dry-run.";
  } else if (step.state == "blocked") {
    headline = "Target driver stopped on a blocker.";
  }
  std::ostringstream out;
  out << "{\"headline\":" << detail::json_quote(headline)
      << ",\"target_id\":" << detail::json_quote(step.advice.target_id)
      << ",\"target_status\":" << detail::json_quote(step.advice.target_status)
      << ",\"dispatch_state\":" << detail::json_quote(step.state)
      << ",\"terminal_state\":" << detail::json_quote(terminal_state)
      << ",\"next_safe_action\":" << detail::json_quote(step.next_action)
      << ",\"runtime_dry_run_attempted\":"
      << (step.dry_run_attempted ? "true" : "false")
      << ",\"runtime_dry_run_accepted\":"
      << (step.dry_run_accepted ? "true" : "false");
  if (ledger != nullptr) {
    out << ",\"drive_mode\":"
        << detail::json_quote(to_string(ledger->drive_mode))
        << ",\"iteration_count\":" << ledger->iteration_count
        << ",\"runtime_handoff_attempt_count\":"
        << ledger->runtime_handoff_attempt_count
        << ",\"execution_attempt_count\":" << ledger->execution_attempt_count;
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
reach_stop_reason_json(const reach_lattice_target_step_result_t &step,
                       const marshal_target_driver_ledger_t *ledger) {
  const auto terminal_state = reach_terminal_state(step, ledger);
  const std::string reason =
      ledger != nullptr && !ledger->terminal_reason.empty()
          ? ledger->terminal_reason
          : operator_explanation(step.bucket, step.advice);
  std::ostringstream out;
  out << "{\"terminal_state\":" << detail::json_quote(terminal_state)
      << ",\"human_reason\":" << detail::json_quote(reason)
      << ",\"blocking_owner\":"
      << detail::json_quote(reach_blocking_owner(step.bucket, terminal_state))
      << ",\"next_safe_action\":" << detail::json_quote(step.next_action)
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
reach_wave_panel_json(const reach_lattice_target_step_result_t &step) {
  std::ostringstream out;
  out << "{\"suggested_wave\":"
      << operator_wave_json(step.advice.suggested_wave)
      << ",\"plan_basis\":" << plan_basis_json(step.advice.plan_basis)
      << ",\"model_state_inputs\":"
      << resolved_plan_inputs_json(step.model_state_inputs)
      << ",\"runtime_wave_match\":"
      << runtime_wave_match_json(step.advice, step.policy, step.wave,
                                 step.model_state_inputs)
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
reach_runtime_panel_json(const reach_lattice_target_step_result_t &step) {
  std::ostringstream out;
  out << "{\"policy\":{\"runtime_hero_available\":"
      << (step.policy.runtime_hero_available ? "true" : "false")
      << ",\"runtime_exec_exists\":"
      << (step.policy.runtime_exec_exists ? "true" : "false")
      << ",\"runtime_exec_executable\":"
      << (step.policy.runtime_exec_executable ? "true" : "false")
      << ",\"default_dry_run\":"
      << (step.policy.default_dry_run ? "true" : "false")
      << ",\"allow_execute\":" << (step.policy.allow_execute ? "true" : "false")
      << ",\"allow_train_execute\":"
      << (step.policy.allow_train_execute ? "true" : "false")
      << "},\"active_wave\":{\"available\":"
      << (step.wave.available ? "true" : "false")
      << ",\"wave_id\":" << detail::json_quote(step.wave.wave_id)
      << ",\"target_component_family_id\":"
      << detail::json_quote(step.wave.target_component_family_id)
      << ",\"mode\":" << detail::json_quote(step.wave.mode)
      << ",\"source_range\":" << detail::json_quote(step.wave.source_range)
      << ",\"anchor_index_begin\":"
      << optional_size_json(step.wave.anchor_index_begin)
      << ",\"anchor_index_end\":"
      << optional_size_json(step.wave.anchor_index_end)
      << "},\"dry_run\":{\"requested\":"
      << (step.include_runtime_dry_run ? "true" : "false")
      << ",\"attempted\":" << (step.dry_run_attempted ? "true" : "false")
      << ",\"accepted\":" << (step.dry_run_accepted ? "true" : "false")
      << ",\"response_digest\":"
      << detail::json_quote(step.dry_run_response_digest)
      << ",\"runtime_request_digest\":"
      << detail::json_quote(step.dry_run_runtime_request_digest)
      << ",\"refusal_reasons\":" << step.dry_run_refusals_json << "}}";
  return out.str();
}

[[nodiscard]] inline std::string
reach_lattice_panel_json(const reach_lattice_target_step_result_t &step) {
  std::ostringstream out;
  out << "{\"source_tool\":\"hero.lattice.target_deficit\""
      << ",\"target_id\":" << detail::json_quote(step.advice.target_id)
      << ",\"target_status\":" << detail::json_quote(step.advice.target_status)
      << ",\"plan_ready\":"
      << (step.advice.plan_basis.available ? "true" : "false")
      << ",\"primary_deficit_key\":"
      << detail::json_quote(step.advice.plan_basis.primary_deficit_key)
      << ",\"deficit_keys\":"
      << string_array_json(step.advice.plan_basis.deficit_keys)
      << ",\"required_plan_inputs\":"
      << string_array_json(step.advice.required_plan_inputs)
      << ",\"materialized_plan_inputs\":"
      << (required_plan_inputs_resolved(step.model_state_inputs) ? "true"
                                                                 : "false")
      << "}";
  return out.str();
}

[[nodiscard]] inline std::string
reach_audit_panel_json(const reach_lattice_target_step_result_t &step,
                       const marshal_target_driver_ledger_t *ledger) {
  std::ostringstream out;
  out << "{\"receipt_id\":" << detail::json_quote(step.prepare_digest)
      << ",\"full_payload_available\":true"
      << ",\"machine_payload_included\":"
      << (step.include_machine_payload ? "true" : "false")
      << ",\"target_satisfaction_claimed\":false"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"advice_digest\":"
      << detail::json_quote(dispatch_advice_digest(step.advice))
      << ",\"request_digest\":"
      << detail::json_quote(dispatch_request_digest(step.request))
      << ",\"runtime_policy_digest\":"
      << detail::json_quote(runtime_policy_snapshot_digest(step.policy))
      << ",\"runtime_wave_digest\":"
      << detail::json_quote(runtime_wave_snapshot_digest(step.wave));
  if (ledger != nullptr) {
    out << ",\"target_driver_run_id\":"
        << detail::json_quote(ledger->target_driver_run_id)
        << ",\"ledger_digest\":"
        << detail::json_quote(target_driver_ledger_digest(*ledger));
  }
  out << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement) << "}";
  return out.str();
}

[[nodiscard]] inline std::string reach_lattice_target_operator_packet_json(
    const reach_lattice_target_step_result_t &step,
    const marshal_target_driver_ledger_t *ledger = nullptr) {
  std::ostringstream structured;
  structured
      << "{\"ok\":true"
      << ",\"tool\":" << detail::json_quote("hero.marshal.reach_lattice_target")
      << ",\"target_id\":" << detail::json_quote(step.advice.target_id)
      << ",\"target_status\":" << detail::json_quote(step.advice.target_status)
      << ",\"dispatch_state\":" << detail::json_quote(step.state)
      << ",\"blocker_bucket\":" << detail::json_quote(step.bucket)
      << ",\"explanation\":"
      << detail::json_quote(operator_explanation(step.bucket, step.advice))
      << ",\"operator_summary\":" << reach_operator_summary_json(step, ledger)
      << ",\"stop_reason\":" << reach_stop_reason_json(step, ledger)
      << ",\"wave_panel\":" << reach_wave_panel_json(step)
      << ",\"runtime_panel\":" << reach_runtime_panel_json(step)
      << ",\"lattice_panel\":" << reach_lattice_panel_json(step)
      << ",\"audit_panel\":" << reach_audit_panel_json(step, ledger)
      << ",\"suggested_wave\":"
      << operator_wave_json(step.advice.suggested_wave)
      << ",\"model_state_inputs\":"
      << resolved_plan_inputs_json(step.model_state_inputs)
      << ",\"runtime_wave_match\":"
      << runtime_wave_match_json(step.advice, step.policy, step.wave,
                                 step.model_state_inputs)
      << ",\"next_action\":" << detail::json_quote(step.next_action)
      << ",\"next_command\":{\"tool\":"
      << detail::json_quote("hero.marshal.reach_lattice_target")
      << ",\"args\":{\"target_id\":"
      << detail::json_quote(step.advice.target_id)
      << ",\"requested_mode\":\"dry_run\""
      << ",\"include_runtime_dry_run\":"
      << (step.next_action == "dry_run" ? "true" : "false")
      << ",\"materialize_plan_inputs\":true}}"
      << ",\"audit\":{\"receipt_id\":"
      << detail::json_quote(step.prepare_digest)
      << ",\"full_payload_available\":true}"
      << ",\"runtime_dry_run\":{\"requested\":"
      << (step.include_runtime_dry_run ? "true" : "false")
      << ",\"attempted\":" << (step.dry_run_attempted ? "true" : "false")
      << ",\"accepted\":" << (step.dry_run_accepted ? "true" : "false")
      << ",\"response_digest\":"
      << detail::json_quote(step.dry_run_response_digest)
      << ",\"runtime_request_digest\":"
      << detail::json_quote(step.dry_run_runtime_request_digest)
      << ",\"refusal_reasons\":" << step.dry_run_refusals_json << "}";
  if (ledger != nullptr) {
    structured << ",\"drive_mode\":"
               << detail::json_quote(to_string(ledger->drive_mode))
               << ",\"driver_terminal_state\":"
               << detail::json_quote(ledger->terminal_state)
               << ",\"driver_terminal_reason\":"
               << detail::json_quote(ledger->terminal_reason);
    structured << ",\"target_driver\":" << target_driver_ledger_json(*ledger);
  }
  if (step.include_machine_payload) {
    structured
        << ",\"machine_payload\":{\"source_lattice_tool\":"
        << detail::json_quote("hero.lattice.target_deficit")
        << ",\"lattice_plan_result\":"
        << detail::json_quote(step.lattice_result)
        << ",\"original_advice\":" << advice_json(step.original_advice)
        << ",\"advice\":" << advice_json(step.advice)
        << ",\"request\":" << request_json(step.request)
        << ",\"validation\":{\"dispatchable\":"
        << (step.decision.advice_validation.dispatchable ? "true" : "false")
        << ",\"refusal_reasons\":"
        << refusal_reasons_json(step.decision.advice_validation.refusal_reasons)
        << "},\"decision\":{\"accepted\":"
        << (step.decision.accepted ? "true" : "false")
        << ",\"runtime_handoff_available\":"
        << (step.decision.runtime_handoff_available ? "true" : "false")
        << ",\"refusal_reasons\":"
        << refusal_reasons_json(step.decision.refusal_reasons)
        << ",\"runtime_request_digest\":"
        << detail::json_quote(
               runtime_dry_run_request_digest(step.decision.runtime_request))
        << "},\"runtime_policy_snapshot_digest\":"
        << detail::json_quote(runtime_policy_snapshot_digest(step.policy))
        << ",\"runtime_wave_snapshot_digest\":"
        << detail::json_quote(runtime_wave_snapshot_digest(step.wave)) << "}";
  }
  structured << "}";
  return structured.str();
}

[[nodiscard]] inline std::string
make_tool_result(const std::string &tool_name, const std::string &structured) {
  return "{\"content\":[{\"type\":\"text\",\"text\":" +
         detail::json_quote(tool_name + " executed") +
         "}],\"structuredContent\":" + structured + ",\"isError\":false}";
}

[[nodiscard]] inline std::string make_error_result(const std::string &message) {
  return "{\"content\":[{\"type\":\"text\",\"text\":" +
         detail::json_quote(message) +
         "}],\"structuredContent\":{\"ok\":false,\"message\":" +
         detail::json_quote(message) + "},\"isError\":true}";
}

[[nodiscard]] inline std::filesystem::path
evaluate_runtime_root(const std::map<std::string, std::string> &args) {
  return std::filesystem::path(optional_string(
      args, "runtime_root", "/cuwacunu/.runtime/cuwacunu_exec"));
}

[[nodiscard]] inline std::filesystem::path
evaluate_config_path(const std::map<std::string, std::string> &args) {
  return std::filesystem::path(
      optional_string(args, "config_path", "/cuwacunu/src/config/.config"));
}

[[nodiscard]] inline std::vector<operational_report_detail::job_summary_t>
discover_all_evaluate_jobs(const std::map<std::string, std::string> &args) {
  marshal_operational_report_options_t options{};
  options.runtime_root = evaluate_runtime_root(args);
  options.config_path = evaluate_config_path(args);
  options.job_ids = optional_string_array(args, "job_ids");
  if (options.job_ids.empty()) {
    const auto job_id = optional_string(args, "job_id");
    if (!job_id.empty()) {
      options.job_ids.push_back(job_id);
    }
  }
  if (!options.job_ids.empty()) {
    return operational_report_detail::discover_jobs(options);
  }

  std::vector<operational_report_detail::job_summary_t> jobs;
  if (!std::filesystem::exists(options.runtime_root)) {
    return jobs;
  }
  for (const auto &entry :
       cuwacunu::kikijyeba::runtime::job_layout::discover_runtime_job_dirs(
           options.runtime_root)) {
    if (std::filesystem::exists(entry.state_path)) {
      jobs.push_back(
          operational_report_detail::read_job(entry.dir, options.config_path));
    }
  }
  return jobs;
}

[[nodiscard]] inline std::string
job_first_value(const operational_report_detail::job_summary_t &job,
                std::initializer_list<std::string_view> keys) {
  return operational_report_detail::job_value(job, keys);
}

inline void append_unique_value(std::vector<std::string> *values,
                                const std::string &value) {
  if (!values || value.empty()) {
    return;
  }
  if (std::find(values->begin(), values->end(), value) == values->end()) {
    values->push_back(value);
  }
}

[[nodiscard]] inline std::vector<std::string> unique_job_values(
    const std::vector<operational_report_detail::job_summary_t> &jobs,
    std::initializer_list<std::string_view> keys) {
  std::vector<std::string> out;
  for (const auto &job : jobs) {
    append_unique_value(&out, job_first_value(job, keys));
  }
  std::sort(out.begin(), out.end());
  return out;
}

[[nodiscard]] inline std::string
evaluate_audit_json(const std::string &subject,
                    const std::map<std::string, std::string> &args,
                    std::size_t job_count) {
  std::ostringstream out;
  out << "{\"subject\":" << detail::json_quote(subject) << ",\"runtime_root\":"
      << detail::json_quote(evaluate_runtime_root(args).string())
      << ",\"config_path\":"
      << detail::json_quote(evaluate_config_path(args).string())
      << ",\"job_count\":" << job_count << ",\"read_only\":true"
      << ",\"runtime_executor\":false"
      << ",\"writes_evidence\":false"
      << ",\"target_satisfaction_claimed\":false"
      << ",\"model_selector\":false"
      << ",\"checkpoint_selected\":false"
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement) << "}";
  return out.str();
}

[[nodiscard]] inline std::string
protocol_field_json(const std::string &name, const std::string &expected,
                    const std::vector<std::string> &observed) {
  bool matched = expected.empty();
  if (!expected.empty()) {
    matched =
        std::find(observed.begin(), observed.end(), expected) != observed.end();
  }
  std::string status = "missing";
  if (!expected.empty() && observed.empty()) {
    status = "expected_unobserved";
  } else if (!expected.empty() && matched) {
    status = observed.size() > 1U ? "matched_with_conflicts" : "matched";
  } else if (!expected.empty() && !matched) {
    status = "mismatch";
  } else if (observed.size() > 1U) {
    status = "conflicting_observed_values";
  } else if (!observed.empty()) {
    status = "observed";
  }
  std::ostringstream out;
  out << detail::json_quote(name)
      << ":{\"expected\":" << detail::json_quote(expected)
      << ",\"observed\":" << string_array_json(observed)
      << ",\"status\":" << detail::json_quote(status) << "}";
  return out.str();
}

inline void append_protocol_issue(std::vector<std::string> *issues,
                                  const std::string &field,
                                  const std::string &expected,
                                  const std::vector<std::string> &observed) {
  if (!issues) {
    return;
  }
  if (!expected.empty() &&
      std::find(observed.begin(), observed.end(), expected) == observed.end()) {
    issues->push_back("identity_mismatch:" + field);
  }
  if (expected.empty() && observed.empty()) {
    issues->push_back("identity_missing:" + field);
  }
  if (observed.size() > 1U) {
    issues->push_back("identity_conflict:" + field);
  }
}

[[nodiscard]] inline std::string
evaluate_protocol_subject_json(const std::map<std::string, std::string> &args,
                               bool include_machine_payload) {
  const auto identity_mode = optional_string(args, "identity_mode", "report");
  if (identity_mode != "report" && identity_mode != "strict") {
    throw std::runtime_error(
        "hero.marshal.evaluate subject=protocol identity_mode must be "
        "report or strict");
  }
  const bool strict_identity = identity_mode == "strict";
  if (strict_identity) {
    std::vector<std::string> missing_expected;
    for (const auto &key :
         {"protocol_contract_fingerprint", "graph_order_fingerprint",
          "source_cursor_token", "vicreg_assembly_fingerprint",
          "mdn_assembly_fingerprint"}) {
      if (optional_string(args, key).empty()) {
        missing_expected.push_back(key);
      }
    }
    if (!missing_expected.empty()) {
      std::ostringstream missing;
      for (std::size_t idx = 0; idx < missing_expected.size(); ++idx) {
        if (idx != 0U) {
          missing << ",";
        }
        missing << missing_expected[idx];
      }
      throw std::runtime_error(
          "hero.marshal.evaluate subject=protocol identity_mode=strict "
          "requires expected identity fields: " +
          missing.str());
    }
  }
  const auto jobs = discover_all_evaluate_jobs(args);
  const auto protocol =
      unique_job_values(jobs, {"protocol_contract_fingerprint"});
  const auto graph = unique_job_values(jobs, {"graph_order_fingerprint"});
  const auto cursor = unique_job_values(jobs, {"source_cursor_token"});
  const auto vicreg = unique_job_values(jobs, {"vicreg_assembly_fingerprint"});
  const auto mdn = unique_job_values(jobs, {"mdn_assembly_fingerprint"});
  std::vector<std::string> issues;
  append_protocol_issue(&issues, "protocol_contract_fingerprint",
                        optional_string(args, "protocol_contract_fingerprint"),
                        protocol);
  append_protocol_issue(&issues, "graph_order_fingerprint",
                        optional_string(args, "graph_order_fingerprint"),
                        graph);
  append_protocol_issue(&issues, "source_cursor_token",
                        optional_string(args, "source_cursor_token"), cursor);
  append_protocol_issue(&issues, "vicreg_assembly_fingerprint",
                        optional_string(args, "vicreg_assembly_fingerprint"),
                        vicreg);
  append_protocol_issue(&issues, "mdn_assembly_fingerprint",
                        optional_string(args, "mdn_assembly_fingerprint"), mdn);
  const std::string expectation_policy =
      strict_identity ? "strict_expected_identity" : "observed_ok_report";
  const bool identity_verified = strict_identity && issues.empty();

  std::ostringstream identity;
  identity << "{"
           << protocol_field_json(
                  "protocol_contract_fingerprint",
                  optional_string(args, "protocol_contract_fingerprint"),
                  protocol)
           << ","
           << protocol_field_json(
                  "graph_order_fingerprint",
                  optional_string(args, "graph_order_fingerprint"), graph)
           << ","
           << protocol_field_json("source_cursor_token",
                                  optional_string(args, "source_cursor_token"),
                                  cursor)
           << ","
           << protocol_field_json(
                  "vicreg_assembly_fingerprint",
                  optional_string(args, "vicreg_assembly_fingerprint"), vicreg)
           << ","
           << protocol_field_json(
                  "mdn_assembly_fingerprint",
                  optional_string(args, "mdn_assembly_fingerprint"), mdn)
           << "}";

  std::ostringstream out;
  out << "{\"ok\":true,\"tool\":" << detail::json_quote("hero.marshal.evaluate")
      << ",\"subject\":\"protocol\",\"read_only\":true"
      << ",\"runtime_executor\":false,\"target_satisfaction_claimed\":false"
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"lattice_proof_context_included\":false"
      << ",\"operator_summary\":{\"headline\":"
      << detail::json_quote(issues.empty()
                                ? "Protocol identity evidence is consistent."
                                : "Protocol identity evidence needs review.")
      << ",\"issue_count\":" << issues.size()
      << ",\"identity_verified\":" << (identity_verified ? "true" : "false")
      << ",\"expectation_policy\":" << detail::json_quote(expectation_policy)
      << ",\"next_safe_action\":"
      << detail::json_quote(issues.empty() ? "inspect_protocol_identity"
                                           : "inspect_protocol_identity_issues")
      << "}"
      << ",\"stop_reason\":{\"terminal_state\":\"report_ready"
      << (issues.empty() ? "" : "_with_protocol_issues")
      << "\",\"blocking_owner\":\"" << (issues.empty() ? "none" : "runtime")
      << "\",\"human_reason\":"
      << detail::json_quote(issues.empty()
                                ? "Read-only protocol report generated."
                                : "Observed protocol identity is missing, "
                                  "conflicting, or mismatched.")
      << ",\"next_safe_action\":"
      << detail::json_quote(issues.empty() ? "inspect_protocol_identity"
                                           : "inspect_protocol_identity_issues")
      << "}"
      << ",\"protocol_panel\":{\"identity\":" << identity.str()
      << ",\"issues\":" << string_array_json(issues)
      << ",\"expectation_policy\":" << detail::json_quote(expectation_policy)
      << ",\"strict_audit\":" << (strict_identity ? "true" : "false")
      << ",\"identity_verified\":" << (identity_verified ? "true" : "false")
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"marshal_role\":\"evidence_grouping\"}"
      << ",\"runtime_panel\":"
      << operational_report_detail::runtime_panel_json(jobs)
      << ",\"audit_panel\":"
      << evaluate_audit_json("protocol", args, jobs.size())
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (include_machine_payload) {
    out << ",\"machine_payload\":{\"chain_summary\":"
        << operational_report_detail::chain_summary_json(jobs) << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline bool
job_matches_spawn_query(const operational_report_detail::job_summary_t &job,
                        const std::map<std::string, std::string> &args) {
  const std::string requested_spawn =
      first_non_empty({optional_string(args, "spawn_id"),
                       optional_string(args, "component_spawn_id")});
  if (!requested_spawn.empty() &&
      job_first_value(job, {"component_spawn_id"}) != requested_spawn) {
    return false;
  }
  for (const auto &key :
       {"component_spawn_registry_id", "component_spawn_label",
        "component_spawn_fingerprint"}) {
    const auto expected = optional_string(args, key);
    if (!expected.empty() && job_first_value(job, {key}) != expected) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
job_matches_component_query(const operational_report_detail::job_summary_t &job,
                            const std::string &component_family_id) {
  return job_first_value(
             job, {"component_family_id", "target_component_family_id"}) ==
             component_family_id ||
         job_first_value(job, {"target_component_family_id"}) ==
             component_family_id;
}

[[nodiscard]] inline std::string
evaluate_spawn_subject_json(const std::map<std::string, std::string> &args,
                            bool include_machine_payload) {
  const std::string requested_spawn =
      first_non_empty({optional_string(args, "spawn_id"),
                       optional_string(args, "component_spawn_id")});
  if (requested_spawn.empty() &&
      optional_string(args, "component_spawn_fingerprint").empty()) {
    throw std::runtime_error(
        "hero.marshal.evaluate subject=spawn requires spawn_id, "
        "component_spawn_id, or component_spawn_fingerprint");
  }
  const auto all_jobs = discover_all_evaluate_jobs(args);
  std::vector<operational_report_detail::job_summary_t> jobs;
  for (const auto &job : all_jobs) {
    if (job_matches_spawn_query(job, args)) {
      jobs.push_back(job);
    }
  }
  const bool found = !jobs.empty();
  std::ostringstream out;
  out << "{\"ok\":true,\"tool\":" << detail::json_quote("hero.marshal.evaluate")
      << ",\"subject\":\"spawn\",\"read_only\":true"
      << ",\"runtime_executor\":false,\"target_satisfaction_claimed\":false"
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"lattice_proof_context_included\":false"
      << ",\"operator_summary\":{\"headline\":"
      << detail::json_quote(found ? "Spawn runtime evidence found."
                                  : "No runtime evidence matched the spawn.")
      << ",\"match_count\":" << jobs.size() << ",\"next_safe_action\":"
      << detail::json_quote(found ? "inspect_spawn_runtime_evidence"
                                  : "inspect_spawn_identity_or_runtime_root")
      << "}"
      << ",\"stop_reason\":{\"terminal_state\":\"report_ready"
      << (found ? "" : "_with_missing_spawn_evidence")
      << "\",\"blocking_owner\":\"" << (found ? "none" : "runtime")
      << "\",\"human_reason\":"
      << detail::json_quote(found
                                ? "Read-only spawn report generated."
                                : "No Runtime job matched the requested spawn.")
      << ",\"next_safe_action\":"
      << detail::json_quote(found ? "inspect_spawn_runtime_evidence"
                                  : "inspect_spawn_identity_or_runtime_root")
      << "}"
      << ",\"spawn_panel\":{\"query\":{\"spawn_id\":"
      << detail::json_quote(requested_spawn)
      << ",\"component_spawn_fingerprint\":"
      << detail::json_quote(
             optional_string(args, "component_spawn_fingerprint"))
      << "},\"match_count\":" << jobs.size()
      << ",\"query_match_count\":" << jobs.size()
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"marshal_role\":\"evidence_grouping\""
      << ",\"component_families\":"
      << string_array_json(unique_job_values(
             jobs, {"component_family_id", "target_component_family_id"}))
      << ",\"spawn_fingerprints\":"
      << string_array_json(
             unique_job_values(jobs, {"component_spawn_fingerprint"}))
      << ",\"jobs\":" << operational_report_detail::chain_summary_json(jobs)
      << "}"
      << ",\"runtime_panel\":"
      << operational_report_detail::runtime_panel_json(jobs)
      << ",\"audit_panel\":" << evaluate_audit_json("spawn", args, jobs.size())
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (include_machine_payload) {
    out << ",\"machine_payload\":{\"all_runtime_job_count\":" << all_jobs.size()
        << ",\"matched_chain_summary\":"
        << operational_report_detail::chain_summary_json(jobs) << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
evaluate_component_subject_json(const std::map<std::string, std::string> &args,
                                bool include_machine_payload) {
  const std::string component_family_id =
      first_non_empty({optional_string(args, "component_family_id"),
                       optional_string(args, "component")});
  if (component_family_id.empty()) {
    throw std::runtime_error("hero.marshal.evaluate subject=component requires "
                             "component_family_id or component");
  }
  const auto all_jobs = discover_all_evaluate_jobs(args);
  std::vector<operational_report_detail::job_summary_t> jobs;
  for (const auto &job : all_jobs) {
    if (job_matches_component_query(job, component_family_id)) {
      jobs.push_back(job);
    }
  }
  const bool found = !jobs.empty();
  std::ostringstream out;
  out << "{\"ok\":true,\"tool\":" << detail::json_quote("hero.marshal.evaluate")
      << ",\"subject\":\"component\",\"read_only\":true"
      << ",\"runtime_executor\":false,\"target_satisfaction_claimed\":false"
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"lattice_proof_context_included\":false"
      << ",\"operator_summary\":{\"headline\":"
      << detail::json_quote(found
                                ? "Component runtime evidence found."
                                : "No runtime evidence matched the component.")
      << ",\"match_count\":" << jobs.size() << ",\"next_safe_action\":"
      << detail::json_quote(found ? "inspect_component_runtime_evidence"
                                  : "inspect_component_family_or_runtime_root")
      << "}"
      << ",\"stop_reason\":{\"terminal_state\":\"report_ready"
      << (found ? "" : "_with_missing_component_evidence")
      << "\",\"blocking_owner\":\"" << (found ? "none" : "runtime")
      << "\",\"human_reason\":"
      << detail::json_quote(
             found ? "Read-only component report generated."
                   : "No Runtime job matched the requested component family.")
      << ",\"next_safe_action\":"
      << detail::json_quote(found ? "inspect_component_runtime_evidence"
                                  : "inspect_component_family_or_runtime_root")
      << "}"
      << ",\"component_panel\":{\"component_family_id\":"
      << detail::json_quote(component_family_id)
      << ",\"match_count\":" << jobs.size()
      << ",\"query_match_count\":" << jobs.size()
      << ",\"evidence_scope\":\"runtime_jobs_only\""
      << ",\"proof_authority\":\"none\""
      << ",\"marshal_role\":\"evidence_grouping\""
      << ",\"spawn_ids\":"
      << string_array_json(unique_job_values(jobs, {"component_spawn_id"}))
      << ",\"spawn_fingerprints\":"
      << string_array_json(
             unique_job_values(jobs, {"component_spawn_fingerprint"}))
      << ",\"wave_actions\":"
      << string_array_json(unique_job_values(jobs, {"wave_action"}))
      << ",\"jobs\":" << operational_report_detail::chain_summary_json(jobs)
      << "}"
      << ",\"runtime_panel\":"
      << operational_report_detail::runtime_panel_json(jobs)
      << ",\"audit_panel\":"
      << evaluate_audit_json("component", args, jobs.size())
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (include_machine_payload) {
    out << ",\"machine_payload\":{\"all_runtime_job_count\":" << all_jobs.size()
        << ",\"matched_chain_summary\":"
        << operational_report_detail::chain_summary_json(jobs) << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
evaluate_target_subject_json(const std::map<std::string, std::string> &args,
                             marshal_lattice_tool_callback_t callback,
                             bool include_machine_payload) {
  const auto target_id = optional_string(args, "target_id");
  if (target_id.empty()) {
    throw std::runtime_error(
        "hero.marshal.evaluate subject=target requires target_id");
  }
  if (callback == nullptr) {
    throw std::runtime_error(
        "hero.marshal.evaluate subject=target requires a Lattice Hero "
        "callback");
  }
  const auto lattice_args = lattice_target_deficit_arguments_json(args);
  std::string lattice_result;
  std::string lattice_error;
  if (!callback("hero.lattice.target_deficit", lattice_args, &lattice_result,
                &lattice_error) ||
      tool_result_has_error_marker(lattice_result)) {
    throw std::runtime_error(lattice_error.empty()
                                 ? "hero.lattice.target_deficit failed"
                                 : lattice_error);
  }

  const auto structured_json = structured_content_json(lattice_result);
  const auto fields = object_fields(structured_json);
  const auto suggested_wave_raw =
      optional_non_null_raw(fields, "suggested_wave");
  marshal_suggested_wave_t wave{};
  if (suggested_wave_raw.has_value()) {
    wave = parse_lattice_suggested_wave(*suggested_wave_raw);
  }
  marshal_plan_basis_t basis{};
  if (const auto plan_basis_raw = optional_raw(fields, "plan_basis")) {
    basis = parse_lattice_plan_basis(*plan_basis_raw, wave);
  }
  const auto proof_raw = optional_non_null_raw(fields, "proof_certificate");
  const bool proof_certificate_present = proof_raw.has_value();
  std::string certificate_ref;
  std::string target_spec_fingerprint;
  std::string split_policy_fingerprint =
      optional_string(fields, "split_policy_fingerprint");
  if (proof_raw.has_value()) {
    const auto proof_fields = object_fields(*proof_raw);
    certificate_ref = first_non_empty(
        {optional_string(proof_fields, "certificate_ref"),
         optional_string(proof_fields, "certificate_id"),
         optional_string(proof_fields, "proof_certificate_id")});
    target_spec_fingerprint =
        optional_string(proof_fields, "target_spec_fingerprint");
    split_policy_fingerprint = first_non_empty(
        {optional_string(proof_fields, "split_policy_fingerprint"),
         split_policy_fingerprint});
  }
  marshal_active_identity_t identity{};
  if (const auto active_raw = optional_raw(fields, "active_identity")) {
    identity = parse_lattice_active_identity(
        *active_raw, target_spec_fingerprint, split_policy_fingerprint);
  } else {
    identity.target_spec_fingerprint = target_spec_fingerprint;
    identity.split_policy_fingerprint = split_policy_fingerprint;
  }
  const auto status = optional_string(fields, "status", "unavailable");
  const bool plan_ready = optional_bool(fields, "plan_ready", false) ||
                          suggested_wave_raw.has_value();
  const std::string certificate_status =
      proof_certificate_present
          ? "available"
          : (status == "satisfied" ? "unavailable" : "not_applicable");
  const std::string next_action =
      status == "satisfied"
          ? (proof_certificate_present ? "inspect_lattice_certificate"
                                       : "inspect_lattice_status")
          : (plan_ready ? "reach_lattice_target"
                        : "ask_lattice_to_recheck_target");
  std::ostringstream out;
  out << "{\"ok\":true,\"tool\":" << detail::json_quote("hero.marshal.evaluate")
      << ",\"subject\":\"target\",\"read_only\":true"
      << ",\"runtime_executor\":false,\"target_satisfaction_claimed\":false"
      << ",\"proof_authority\":\"lattice\""
      << ",\"operator_summary\":{\"headline\":"
      << detail::json_quote(status == "satisfied"
                                ? "Lattice reports target satisfied."
                                : "Lattice target needs action or review.")
      << ",\"target_id\":" << detail::json_quote(target_id)
      << ",\"target_status\":" << detail::json_quote(status)
      << ",\"target_status_source\":"
      << detail::json_quote("hero.lattice.target_deficit")
      << ",\"proof_authority\":\"lattice\""
      << ",\"certificate_status\":" << detail::json_quote(certificate_status)
      << ",\"next_safe_action\":" << detail::json_quote(next_action) << "}"
      << ",\"stop_reason\":{\"terminal_state\":\"report_ready"
      << (status == "satisfied" ? "" : "_with_target_blocker")
      << "\",\"blocking_owner\":\""
      << (status == "satisfied" ? "none" : "lattice") << "\",\"human_reason\":"
      << detail::json_quote(status == "satisfied"
                                ? "Read-only target report generated."
                                : "Lattice target status is not satisfied.")
      << ",\"next_safe_action\":" << detail::json_quote(next_action) << "}"
      << ",\"target_panel\":{\"target_id\":" << detail::json_quote(target_id)
      << ",\"status\":" << detail::json_quote(status)
      << ",\"source\":\"hero.lattice.target_deficit\""
      << ",\"target_status_source\":"
      << detail::json_quote("hero.lattice.target_deficit")
      << ",\"proof_authority\":\"lattice\""
      << ",\"certificate_status\":" << detail::json_quote(certificate_status)
      << ",\"certificate_ref\":" << detail::json_quote(certificate_ref)
      << ",\"proof_certificate_present\":"
      << (proof_certificate_present ? "true" : "false")
      << ",\"plan_ready\":" << (plan_ready ? "true" : "false")
      << ",\"active_identity\":" << active_identity_json(identity)
      << ",\"plan_basis\":" << plan_basis_json(basis)
      << ",\"suggested_wave\":" << suggested_wave_json(wave)
      << ",\"next_safe_action\":" << detail::json_quote(next_action)
      << ",\"target_satisfaction_claimed_by_marshal\":false}"
      << ",\"lattice_panel\":{\"source\":\"hero.lattice.target_deficit\""
      << ",\"target_id\":" << detail::json_quote(target_id)
      << ",\"status\":" << detail::json_quote(status)
      << ",\"target_status_source\":"
      << detail::json_quote("hero.lattice.target_deficit")
      << ",\"proof_authority\":\"lattice\""
      << ",\"certificate_status\":" << detail::json_quote(certificate_status)
      << ",\"certificate_ref\":" << detail::json_quote(certificate_ref)
      << ",\"proof_certificate_present\":"
      << (proof_certificate_present ? "true" : "false")
      << ",\"plan_ready\":" << (plan_ready ? "true" : "false") << "}"
      << ",\"audit_panel\":" << evaluate_audit_json("target", args, 0)
      << ",\"non_authority_statement\":"
      << detail::json_quote(k_marshal_dispatch_non_authority_statement);
  if (include_machine_payload) {
    out << ",\"machine_payload\":{\"lattice_arguments\":"
        << detail::json_quote(lattice_args)
        << ",\"lattice_result\":" << detail::json_quote(lattice_result) << "}";
  }
  out << "}";
  return out.str();
}

} // namespace tool_detail

[[nodiscard]] inline bool execute_marshal_tool_json(
    const std::string &tool_name, std::string arguments_json,
    std::string *out_tool_result_json, std::string *out_error_message) {
  if (out_tool_result_json) {
    out_tool_result_json->clear();
  }
  if (out_error_message) {
    out_error_message->clear();
  }

  try {
    arguments_json = tool_detail::trim_ascii(arguments_json);
    if (arguments_json.empty()) {
      arguments_json = "{}";
    }
    const auto args = tool_detail::object_fields(arguments_json);
    std::ostringstream structured;

    if (tool_name == "hero.marshal.evaluate") {
      tool_detail::validate_fields(args,
                                   {"subject",
                                    "mode",
                                    "identity_mode",
                                    "runtime_root",
                                    "config_path",
                                    "target_id",
                                    "job_id",
                                    "job_ids",
                                    "target_ids",
                                    "baseline_job_id",
                                    "candidate_job_id",
                                    "component_family_id",
                                    "component",
                                    "spawn_id",
                                    "component_spawn_id",
                                    "component_spawn_registry_id",
                                    "component_spawn_label",
                                    "component_spawn_fingerprint",
                                    "include_machine_payload",
                                    "protocol_contract_fingerprint",
                                    "graph_order_fingerprint",
                                    "source_cursor_token",
                                    "vicreg_assembly_fingerprint",
                                    "mdn_assembly_fingerprint"},
                                   {"subject"}, tool_name);
      const std::string subject = tool_detail::optional_string(args, "subject");
      const bool include_machine_payload =
          tool_detail::optional_bool(args, "include_machine_payload", false);
      if (subject == "run") {
        tool_detail::validate_fields(
            args,
            {"subject", "mode", "runtime_root", "config_path", "job_id",
             "job_ids", "target_ids", "baseline_job_id", "candidate_job_id",
             "include_machine_payload", "protocol_contract_fingerprint",
             "graph_order_fingerprint", "source_cursor_token",
             "vicreg_assembly_fingerprint", "mdn_assembly_fingerprint"},
            {"subject"}, "hero.marshal.evaluate subject=run");
        const std::string mode =
            tool_detail::optional_string(args, "mode", "latest_chain");
        if (mode == "compare") {
          marshal_run_compare_options_t options{};
          options.runtime_root =
              std::filesystem::path(tool_detail::optional_string(
                  args, "runtime_root", "/cuwacunu/.runtime/cuwacunu_exec"));
          options.config_path =
              std::filesystem::path(tool_detail::optional_string(
                  args, "config_path", "/cuwacunu/src/config/.config"));
          options.baseline_job_id =
              tool_detail::optional_string(args, "baseline_job_id");
          options.candidate_job_id =
              tool_detail::optional_string(args, "candidate_job_id");
          if (options.baseline_job_id.empty() ||
              options.candidate_job_id.empty()) {
            throw std::runtime_error(
                "hero.marshal.evaluate subject=run mode=compare requires "
                "baseline_job_id and candidate_job_id");
          }
          options.include_machine_payload = tool_detail::optional_bool(
              args, "include_machine_payload", false);
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.evaluate")
                     << ",\"subject\":\"run\",\"mode\":\"compare\",\"result\":"
                     << build_marshal_run_comparison_json(options) << "}";
        } else if (mode == "latest_chain" || mode == "training_state" ||
                   mode == "single_job") {
          marshal_operational_report_options_t options{};
          options.runtime_root =
              std::filesystem::path(tool_detail::optional_string(
                  args, "runtime_root", "/cuwacunu/.runtime/cuwacunu_exec"));
          options.config_path =
              std::filesystem::path(tool_detail::optional_string(
                  args, "config_path", "/cuwacunu/src/config/.config"));
          if (mode == "single_job") {
            const std::string job_id =
                tool_detail::optional_string(args, "job_id");
            if (job_id.empty()) {
              throw std::runtime_error(
                  "hero.marshal.evaluate subject=run mode=single_job requires "
                  "job_id");
            }
            options.job_ids = {job_id};
          } else {
            options.job_ids =
                tool_detail::optional_string_array(args, "job_ids");
          }
          options.target_ids =
              tool_detail::optional_string_array(args, "target_ids");
          if (options.target_ids.empty()) {
            options.target_ids =
                default_marshal_operational_report_target_ids();
          }
          options.include_machine_payload = tool_detail::optional_bool(
              args, "include_machine_payload", false);

          auto callback = marshal_lattice_tool_callback_slot();
          if (callback != nullptr && !options.target_ids.empty()) {
            std::string lattice_result;
            std::string lattice_error;
            const std::string lattice_args =
                tool_detail::lattice_evaluate_targets_arguments_json(
                    args, options.target_ids, options.runtime_root,
                    options.config_path);
            if (callback("hero.lattice.evaluate_targets", lattice_args,
                         &lattice_result, &lattice_error) &&
                !tool_detail::tool_result_has_error_marker(lattice_result)) {
              options.target_statuses =
                  tool_detail::parse_lattice_evaluate_targets_statuses(
                      lattice_result);
            }
          }
          if (options.target_statuses.empty()) {
            for (const auto &target_id : options.target_ids) {
              marshal_lattice_target_status_t status{};
              status.target_id = target_id;
              status.status = "unavailable";
              options.target_statuses.push_back(std::move(status));
            }
          }
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.evaluate")
                     << ",\"subject\":\"run\",\"mode\":"
                     << detail::json_quote(mode) << ",\"result\":"
                     << build_marshal_operational_report_json(options) << "}";
        } else {
          throw std::runtime_error(
              "hero.marshal.evaluate subject=run mode must be latest_chain, "
              "training_state, single_job, or compare");
        }
      } else if (subject == "target") {
        tool_detail::validate_fields(
            args,
            {"subject", "runtime_root", "config_path", "target_id",
             "include_machine_payload", "protocol_contract_fingerprint",
             "graph_order_fingerprint", "source_cursor_token",
             "vicreg_assembly_fingerprint", "mdn_assembly_fingerprint"},
            {"subject"}, "hero.marshal.evaluate subject=target");
        structured << tool_detail::evaluate_target_subject_json(
            args, marshal_lattice_tool_callback_slot(),
            include_machine_payload);
      } else if (subject == "protocol") {
        tool_detail::validate_fields(
            args,
            {"subject", "identity_mode", "runtime_root", "config_path",
             "job_id", "job_ids", "include_machine_payload",
             "protocol_contract_fingerprint", "graph_order_fingerprint",
             "source_cursor_token", "vicreg_assembly_fingerprint",
             "mdn_assembly_fingerprint"},
            {"subject"}, "hero.marshal.evaluate subject=protocol");
        structured << tool_detail::evaluate_protocol_subject_json(
            args, include_machine_payload);
      } else if (subject == "spawn") {
        tool_detail::validate_fields(
            args,
            {"subject", "runtime_root", "config_path", "job_id", "job_ids",
             "spawn_id", "component_spawn_id", "component_spawn_registry_id",
             "component_spawn_label", "component_spawn_fingerprint",
             "include_machine_payload"},
            {"subject"}, "hero.marshal.evaluate subject=spawn");
        structured << tool_detail::evaluate_spawn_subject_json(
            args, include_machine_payload);
      } else if (subject == "component") {
        tool_detail::validate_fields(
            args,
            {"subject", "runtime_root", "config_path", "job_id", "job_ids",
             "component_family_id", "component", "include_machine_payload"},
            {"subject"}, "hero.marshal.evaluate subject=component");
        structured << tool_detail::evaluate_component_subject_json(
            args, include_machine_payload);
      } else {
        throw std::runtime_error(
            "hero.marshal.evaluate subject must be run, target, protocol, "
            "spawn, or component");
      }
    } else if (tool_name == "hero.marshal.reach_lattice_target") {
      tool_detail::validate_fields(args,
                                   {"target_id",
                                    "config_path",
                                    "runtime_root",
                                    "requested_mode",
                                    "drive_mode",
                                    "driver_policy",
                                    "resume_ledger",
                                    "source_lattice_timestamp",
                                    "max_waves",
                                    "recommendation_attempt_count",
                                    "context",
                                    "protocol_contract_fingerprint",
                                    "graph_order_fingerprint",
                                    "source_cursor_token",
                                    "vicreg_assembly_fingerprint",
                                    "mdn_assembly_fingerprint",
                                    "materialize_plan_inputs",
                                    "include_runtime_dry_run",
                                    "include_machine_payload",
                                    "runtime_policy",
                                    "runtime_wave",
                                    "timeout_seconds"},
                                   {"target_id", "drive_mode"}, tool_name);
      auto callback = marshal_lattice_tool_callback_slot();
      if (callback == nullptr) {
        throw std::runtime_error(
            "Marshal target dispatch preparation requires a Lattice Hero "
            "callback");
      }
      const auto drive_mode = tool_detail::parse_drive_mode_text(
          tool_detail::optional_string(args, "drive_mode", "one_step"));
      if (drive_mode == marshal_target_drive_mode_t::unknown) {
        throw std::runtime_error(
            "hero.marshal.reach_lattice_target drive_mode must be one_step or "
            "budgeted");
      }
      const auto requested_mode = tool_detail::parse_mode_text(
          tool_detail::optional_string(args, "requested_mode", "dry_run"));
      if (requested_mode == marshal_dispatch_mode_t::unknown) {
        throw std::runtime_error(
            "hero.marshal.reach_lattice_target requested_mode must be dry_run "
            "or execute");
      }
      marshal_target_driver_policy_t driver_policy{};
      const auto driver_policy_raw =
          tool_detail::optional_raw(args, "driver_policy");
      bool driver_policy_has_max_waves = false;
      if (driver_policy_raw.has_value()) {
        const auto policy_fields =
            tool_detail::object_fields(*driver_policy_raw);
        driver_policy_has_max_waves =
            policy_fields.find("max_waves") != policy_fields.end();
        driver_policy = tool_detail::parse_driver_policy(*driver_policy_raw);
      } else {
        driver_policy.max_waves =
            tool_detail::optional_i64(args, "max_waves", 1);
      }
      if (drive_mode == marshal_target_drive_mode_t::budgeted &&
          !driver_policy_raw.has_value()) {
        throw std::runtime_error(
            "budgeted drive_mode requires explicit driver_policy");
      }
      if (drive_mode == marshal_target_drive_mode_t::budgeted &&
          !driver_policy_has_max_waves) {
        throw std::runtime_error(
            "budgeted drive_mode requires driver_policy.max_waves");
      }
      if (drive_mode == marshal_target_drive_mode_t::one_step) {
        driver_policy.max_waves = 1;
      }
      if (driver_policy.max_waves <= 0) {
        throw std::runtime_error("driver_policy.max_waves must be positive");
      }
      if (driver_policy.no_progress_window <= 0) {
        throw std::runtime_error(
            "driver_policy.no_progress_window must be positive");
      }
      if (drive_mode == marshal_target_drive_mode_t::budgeted &&
          requested_mode == marshal_dispatch_mode_t::execute &&
          driver_policy.max_wall_clock_seconds <= 0) {
        throw std::runtime_error("budgeted execute requires "
                                 "driver_policy.max_wall_clock_seconds");
      }

      tool_detail::target_driver_resume_state_t resume{};
      if (const auto raw = tool_detail::optional_raw(args, "resume_ledger")) {
        resume = tool_detail::parse_target_driver_resume_state(*raw);
      }

      const bool include_machine_payload =
          tool_detail::optional_bool(args, "include_machine_payload", false);
      bool include_runtime_dry_run =
          tool_detail::optional_bool(args, "include_runtime_dry_run", false);
      if (drive_mode == marshal_target_drive_mode_t::budgeted) {
        include_runtime_dry_run = true;
      }

      marshal_target_driver_ledger_t ledger{};
      ledger.target_id = tool_detail::optional_string(args, "target_id");
      ledger.drive_mode = drive_mode;
      ledger.requested_mode = requested_mode;
      ledger.driver_policy = driver_policy;
      ledger.driver_policy_digest = target_driver_policy_digest(driver_policy);
      bool reach_result_ready = false;
      if (resume.present) {
        ledger.resumed_from_run_id = resume.target_driver_run_id;
        ledger.resumed_iteration_count = resume.iteration_count;
        ledger.runtime_handoff_attempt_count =
            resume.runtime_handoff_attempt_count;
        ledger.execution_attempt_count = resume.execution_attempt_count;
        ledger.last_target_deficit_digest = resume.last_target_deficit_digest;
        ledger.last_suggested_wave_digest = resume.last_suggested_wave_digest;
        ledger.last_progress_signature = resume.last_progress_signature;
        ledger.last_runtime_job_id = resume.last_runtime_job_id;
        ledger.last_runtime_job_state_digest =
            resume.last_runtime_job_state_digest;
        ledger.last_runtime_job_manifest_digest =
            resume.last_runtime_job_manifest_digest;
        ledger.last_runtime_terminal_fact_digest =
            resume.last_runtime_terminal_fact_digest;
        ledger.last_runtime_checkpoint_io_fact_digest =
            resume.last_runtime_checkpoint_io_fact_digest;
        ledger.last_runtime_handoff_id = resume.last_runtime_handoff_id;
        ledger.last_runtime_handoff_digest = resume.last_runtime_handoff_digest;
        ledger.last_runtime_policy_digest = resume.last_runtime_policy_digest;
        if (resume.target_id != ledger.target_id ||
            (!resume.driver_policy_digest.empty() &&
             resume.driver_policy_digest != ledger.driver_policy_digest) ||
            (!resume.drive_mode.empty() &&
             resume.drive_mode != to_string(drive_mode)) ||
            (!resume.requested_mode.empty() &&
             resume.requested_mode != to_string(requested_mode))) {
          ledger.terminal_state = "blocked_stale_resume";
          ledger.terminal_reason =
              "resume_ledger target, mode, requested_mode, or driver_policy "
              "does not match this request";
          finalize_target_driver_run_id(&ledger);
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.reach_lattice_target")
                     << ",\"target_id\":"
                     << detail::json_quote(ledger.target_id)
                     << ",\"dispatch_state\":\"blocked\""
                     << ",\"blocker_bucket\":\"stale_resume\""
                     << ",\"next_action\":\"inspect_blocker\""
                     << ",\"drive_mode\":"
                     << detail::json_quote(to_string(ledger.drive_mode))
                     << ",\"driver_terminal_state\":"
                     << detail::json_quote(ledger.terminal_state)
                     << ",\"driver_terminal_reason\":"
                     << detail::json_quote(ledger.terminal_reason)
                     << ",\"target_driver\":"
                     << tool_detail::target_driver_ledger_json(ledger) << "}";
          reach_result_ready = true;
        } else if (resume.ledger_digest.empty()) {
          ledger.terminal_state = "blocked_stale_resume";
          ledger.terminal_reason = "resume_ledger is missing ledger_digest";
          finalize_target_driver_run_id(&ledger);
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.reach_lattice_target")
                     << ",\"target_id\":"
                     << detail::json_quote(ledger.target_id)
                     << ",\"dispatch_state\":\"blocked\""
                     << ",\"blocker_bucket\":\"stale_resume\""
                     << ",\"next_action\":\"inspect_blocker\""
                     << ",\"drive_mode\":"
                     << detail::json_quote(to_string(ledger.drive_mode))
                     << ",\"driver_terminal_state\":"
                     << detail::json_quote(ledger.terminal_state)
                     << ",\"driver_terminal_reason\":"
                     << detail::json_quote(ledger.terminal_reason)
                     << ",\"target_driver\":"
                     << tool_detail::target_driver_ledger_json(ledger) << "}";
          reach_result_ready = true;
        } else if (resume.execution_attempt_count > 0 &&
                   (resume.last_runtime_job_id.empty() ||
                    resume.last_runtime_job_manifest_digest.empty() ||
                    resume.last_runtime_terminal_fact_digest.empty() ||
                    resume.last_runtime_handoff_digest.empty())) {
          ledger.terminal_state = "blocked_stale_resume";
          ledger.terminal_reason =
              "resume_ledger with execution history is missing Runtime "
              "terminal evidence, job manifest, or handoff identity";
          finalize_target_driver_run_id(&ledger);
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.reach_lattice_target")
                     << ",\"target_id\":"
                     << detail::json_quote(ledger.target_id)
                     << ",\"dispatch_state\":\"blocked\""
                     << ",\"blocker_bucket\":\"stale_resume\""
                     << ",\"next_action\":\"inspect_blocker\""
                     << ",\"drive_mode\":"
                     << detail::json_quote(to_string(ledger.drive_mode))
                     << ",\"driver_terminal_state\":"
                     << detail::json_quote(ledger.terminal_state)
                     << ",\"driver_terminal_reason\":"
                     << detail::json_quote(ledger.terminal_reason)
                     << ",\"target_driver\":"
                     << tool_detail::target_driver_ledger_json(ledger) << "}";
          reach_result_ready = true;
        } else if (resume.terminal_state == "reached") {
          ledger.terminal_state = "reached";
          ledger.terminal_reason =
              "resume_ledger already records the target as reached";
          finalize_target_driver_run_id(&ledger);
          structured << "{\"ok\":true,\"tool\":"
                     << detail::json_quote("hero.marshal.reach_lattice_target")
                     << ",\"target_id\":"
                     << detail::json_quote(ledger.target_id)
                     << ",\"dispatch_state\":\"already_satisfied\""
                     << ",\"blocker_bucket\":\"none\""
                     << ",\"next_action\":\"inspect_lattice_certificate\""
                     << ",\"drive_mode\":"
                     << detail::json_quote(to_string(ledger.drive_mode))
                     << ",\"driver_terminal_state\":"
                     << detail::json_quote(ledger.terminal_state)
                     << ",\"driver_terminal_reason\":"
                     << detail::json_quote(ledger.terminal_reason)
                     << ",\"target_driver\":"
                     << tool_detail::target_driver_ledger_json(ledger) << "}";
          reach_result_ready = true;
        }
      }

      if (!reach_result_ready) {
        auto make_iteration =
            [&](const tool_detail::reach_lattice_target_step_result_t &step) {
              marshal_target_driver_iteration_t iteration{};
              iteration.iteration_index =
                  ledger.resumed_iteration_count + ledger.iteration_count;
              iteration.active_identity = step.advice.active_identity;
              iteration.target_status = step.advice.target_status;
              iteration.dispatch_state = step.state;
              iteration.blocker_bucket = step.bucket;
              iteration.next_action = step.next_action;
              iteration.target_deficit_digest = marshal_digest_for_text(
                  "kikijyeba.marshal.lattice_target_deficit_result.v1",
                  step.lattice_result);
              iteration.suggested_wave_digest =
                  suggested_wave_digest(step.advice.suggested_wave);
              iteration.plan_input_digest =
                  plan_input_digest(step.advice.suggested_wave.plan_inputs);
              iteration.advice_digest = dispatch_advice_digest(step.advice);
              iteration.request_digest = dispatch_request_digest(step.request);
              iteration.runtime_policy_digest =
                  runtime_policy_snapshot_digest(step.policy);
              iteration.runtime_wave_digest =
                  runtime_wave_snapshot_digest(step.wave);
              iteration.runtime_preview_request_digest =
                  runtime_dry_run_request_digest(step.decision.runtime_request);
              iteration.dry_run_attempted = step.dry_run_attempted;
              iteration.dry_run_accepted = step.dry_run_accepted;
              iteration.dry_run_response_digest = step.dry_run_response_digest;
              iteration.progress_signature = marshal_digest_for_text(
                  "kikijyeba.marshal.target_driver.progress_signature.v1",
                  iteration.target_deficit_digest + "\n" +
                      iteration.suggested_wave_digest + "\n" +
                      iteration.plan_input_digest + "\n");
              if (step.state == "already_satisfied") {
                iteration.refusal_reasons.emplace_back(
                    to_string(marshal_refusal_reason_t::
                                  target_already_satisfied));
              } else {
                for (const auto reason : step.decision.refusal_reasons) {
                  iteration.refusal_reasons.emplace_back(to_string(reason));
                }
                for (const auto reason : step.dry_run_refusal_reasons) {
                  iteration.refusal_reasons.emplace_back(to_string(reason));
                }
              }
              return iteration;
            };

        auto append_iteration =
            [&](const marshal_target_driver_iteration_t &iteration) {
              ledger.iterations.push_back(iteration);
              ledger.iteration_count =
                  static_cast<std::int64_t>(ledger.iterations.size());
              ledger.last_target_deficit_digest =
                  iteration.target_deficit_digest;
              ledger.last_suggested_wave_digest =
                  iteration.suggested_wave_digest;
              ledger.last_progress_signature = iteration.progress_signature;
              if (!iteration.active_identity.target_spec_fingerprint.empty() ||
                  !iteration.active_identity.split_policy_fingerprint.empty() ||
                  !iteration.active_identity.protocol_contract_fingerprint
                       .empty() ||
                  !iteration.active_identity.graph_order_fingerprint.empty()) {
                ledger.active_identity = iteration.active_identity;
              }
              if (!iteration.runtime_policy_digest.empty()) {
                ledger.last_runtime_policy_digest =
                    iteration.runtime_policy_digest;
              }
              if (!iteration.runtime_job_id.empty()) {
                ledger.last_runtime_job_id = iteration.runtime_job_id;
              }
              if (!iteration.runtime_job_state_digest.empty()) {
                ledger.last_runtime_job_state_digest =
                    iteration.runtime_job_state_digest;
              }
              if (!iteration.runtime_job_manifest_digest.empty()) {
                ledger.last_runtime_job_manifest_digest =
                    iteration.runtime_job_manifest_digest;
              }
              if (!iteration.runtime_terminal_fact_digest.empty()) {
                ledger.last_runtime_terminal_fact_digest =
                    iteration.runtime_terminal_fact_digest;
              }
              if (!iteration.runtime_checkpoint_io_fact_digest.empty()) {
                ledger.last_runtime_checkpoint_io_fact_digest =
                    iteration.runtime_checkpoint_io_fact_digest;
              }
              if (!iteration.runtime_handoff_id.empty()) {
                ledger.last_runtime_handoff_id = iteration.runtime_handoff_id;
              }
              if (!iteration.runtime_handoff_digest.empty()) {
                ledger.last_runtime_handoff_digest =
                    iteration.runtime_handoff_digest;
              }
            };

        tool_detail::reach_lattice_target_step_result_t last_step{};
        if (drive_mode == marshal_target_drive_mode_t::one_step) {
          last_step = tool_detail::run_reach_lattice_target_step(
              args, callback, 1,
              tool_detail::optional_i64(args, "recommendation_attempt_count",
                                        0),
              requested_mode, include_runtime_dry_run, include_machine_payload);
          auto iteration = make_iteration(last_step);
          if (last_step.state == "already_satisfied") {
            ledger.terminal_state = "reached";
            ledger.terminal_reason = "Lattice reported target satisfied";
          } else if (last_step.bucket != "none") {
            ledger.terminal_state = last_step.bucket == "runtime_policy_refused"
                                        ? "blocked_runtime_policy"
                                        : "blocked_" + last_step.bucket;
            ledger.terminal_reason = last_step.bucket;
          } else {
            ledger.terminal_state = "one_step_completed";
            ledger.terminal_reason =
                "one_step computes at most one movement toward the target";
          }
          iteration.terminal_state = ledger.terminal_state;
          iteration.terminal_reason = ledger.terminal_reason;
          append_iteration(iteration);
        } else {
          const auto started_at = std::chrono::steady_clock::now();
          std::string previous_progress_signature =
              ledger.last_progress_signature;
          std::int64_t repeated_progress_count = 0;
          for (std::int64_t attempt = ledger.runtime_handoff_attempt_count;
               attempt < driver_policy.max_waves; ++attempt) {
            if (driver_policy.max_wall_clock_seconds > 0) {
              const auto elapsed =
                  std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::steady_clock::now() - started_at)
                      .count();
              if (elapsed >= driver_policy.max_wall_clock_seconds) {
                ledger.terminal_state = "blocked_timeout";
                ledger.terminal_reason =
                    "driver_policy.max_wall_clock_seconds exhausted";
                break;
              }
            }

            last_step = tool_detail::run_reach_lattice_target_step(
                args, callback, driver_policy.max_waves, attempt,
                marshal_dispatch_mode_t::dry_run, true,
                include_machine_payload);
            auto iteration = make_iteration(last_step);

            if (last_step.state == "already_satisfied" ||
                last_step.advice.target_status == "satisfied") {
              ledger.terminal_state = "reached";
              ledger.terminal_reason = "Lattice reported target satisfied";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            const auto lattice_warning_stop =
                driver_policy.stop_on_lattice_warning
                    ? tool_detail::warning_stop_decision(
                          last_step.lattice_result, "lattice",
                          driver_policy.stop_on_warning_severity)
                    : tool_detail::warning_stop_decision_t{};
            if (lattice_warning_stop.stop) {
              ledger.terminal_state = "blocked_lattice_warning";
              ledger.terminal_reason = std::string("Lattice warning ") +
                                       lattice_warning_stop.reason_code +
                                       " matched driver_policy stop condition";
              iteration.refusal_reasons.emplace_back(
                  lattice_warning_stop.reason_code.empty()
                      ? "lattice_warning_stop"
                      : lattice_warning_stop.reason_code);
              if (!lattice_warning_stop.warning.warning_id.empty()) {
                iteration.refusal_reasons.emplace_back(
                    "lattice_warning_id:" +
                    lattice_warning_stop.warning.warning_id);
              }
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            if (!previous_progress_signature.empty() &&
                previous_progress_signature == iteration.progress_signature) {
              ++repeated_progress_count;
            } else {
              repeated_progress_count = 0;
            }
            previous_progress_signature = iteration.progress_signature;
            if (repeated_progress_count >= driver_policy.no_progress_window) {
              ledger.terminal_state = "blocked_no_progress";
              ledger.terminal_reason =
                  "target_deficit and suggested_wave repeated without new "
                  "Runtime evidence";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            if (last_step.bucket != "none") {
              ledger.terminal_state =
                  last_step.bucket == "runtime_policy_refused"
                      ? "blocked_runtime_policy"
                      : (last_step.bucket == "runtime_wave_not_aligned"
                             ? "blocked_runtime_wave_mismatch"
                             : "blocked_" + last_step.bucket);
              ledger.terminal_reason = last_step.bucket;
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }
            if (!last_step.dry_run_attempted || !last_step.dry_run_accepted) {
              ledger.terminal_state = "blocked_runtime_refused";
              ledger.terminal_reason = "Runtime dry-run was not accepted";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }
            ++ledger.runtime_handoff_attempt_count;

            const auto runtime_dry_run_warning_stop =
                driver_policy.stop_on_runtime_warning
                    ? tool_detail::warning_stop_decision(
                          last_step.dry_run_response.runtime_handoff
                              .tool_result_json,
                          "runtime", driver_policy.stop_on_warning_severity)
                    : tool_detail::warning_stop_decision_t{};
            if (runtime_dry_run_warning_stop.stop) {
              ledger.terminal_state = "blocked_runtime_warning";
              iteration.refusal_reasons.emplace_back(
                  runtime_dry_run_warning_stop.reason_code.empty()
                      ? "runtime_dry_run_warning_stop"
                      : runtime_dry_run_warning_stop.reason_code);
              if (!runtime_dry_run_warning_stop.warning.warning_id.empty()) {
                iteration.refusal_reasons.emplace_back(
                    "runtime_warning_id:" +
                    runtime_dry_run_warning_stop.warning.warning_id);
              }
              ledger.terminal_reason =
                  std::string("Runtime dry-run warning ") +
                  runtime_dry_run_warning_stop.reason_code +
                  " matched driver_policy stop condition";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            if (requested_mode == marshal_dispatch_mode_t::dry_run) {
              const bool budget_exhausted =
                  ledger.runtime_handoff_attempt_count >=
                  driver_policy.max_waves;
              if (budget_exhausted) {
                ledger.terminal_state = "blocked_max_waves";
                ledger.terminal_reason =
                    "budgeted dry_run exhausted driver_policy.max_waves";
              }
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason =
                  budget_exhausted
                      ? ledger.terminal_reason
                      : "budgeted dry_run handoff accepted; continuing until "
                        "budget or no-progress stop";
              if (!budget_exhausted) {
                iteration.terminal_state = "continued";
              }
              append_iteration(iteration);
              if (budget_exhausted) {
                break;
              }
              continue;
            }

            if (!driver_policy.allow_execute ||
                !last_step.policy.allow_execute) {
              ledger.terminal_state = "blocked_runtime_policy";
              ledger.terminal_reason = "driver_policy.allow_execute or Runtime "
                                       "allow_execute is false";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }
            if (last_step.wave.train_target &&
                (!driver_policy.allow_train_execute ||
                 !last_step.policy.allow_train_execute)) {
              ledger.terminal_state = "blocked_runtime_policy";
              ledger.terminal_reason =
                  "train wave requires driver_policy.allow_train_execute and "
                  "Runtime allow_train_execute";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            auto execute_request = last_step.request;
            execute_request.requested_mode = marshal_dispatch_mode_t::execute;
            execute_request.operator_confirmation_token =
                confirmation_token(last_step.advice, execute_request);
            marshal_execution_gate_input_t gate_input{};
            gate_input.advice = last_step.advice;
            gate_input.request = execute_request;
            gate_input.validation_context = last_step.context;
            gate_input.policy = last_step.policy;
            gate_input.active_wave = last_step.wave;
            gate_input.prior_dry_run = make_prior_dry_run_evidence(
                last_step.advice, last_step.request, last_step.dry_run_response,
                last_step.policy, last_step.wave);
            const auto gate = validate_execution_gate(gate_input);
            if (!gate.accepted) {
              ledger.terminal_state = "blocked_execution_gate";
              ledger.terminal_reason = "Marshal execution gate refused";
              for (const auto reason : gate.refusal_reasons) {
                iteration.refusal_reasons.emplace_back(to_string(reason));
              }
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            marshal_runtime_hero_handoff_options_t options{};
            options.timeout_seconds = static_cast<int>(
                tool_detail::optional_i64(args, "timeout_seconds", 600));
            const auto handoff = call_runtime_hero_execution(gate, options);
            iteration.execution_attempted = true;
            iteration.execution_accepted = handoff.ok;
            iteration.runtime_execution_request_digest =
                runtime_execution_request_digest(gate.decision.runtime_request);
            iteration.runtime_handoff_arguments_digest =
                handoff.arguments_digest;
            iteration.runtime_response_digest =
                marshal_digest_for_text("kikijyeba.marshal.runtime_response.v1",
                                        handoff.tool_result_json);
            ++ledger.execution_attempt_count;

            if (!handoff.ok) {
              ledger.terminal_state = "blocked_runtime_refused";
              ledger.terminal_reason =
                  handoff.error_message.empty()
                      ? "Runtime execution was not accepted"
                      : handoff.error_message;
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            const auto terminal_evidence =
                tool_detail::runtime_terminal_evidence_from_handoff(handoff);
            iteration.runtime_job_id = terminal_evidence.job_id;
            iteration.runtime_job_state_digest =
                terminal_evidence.job_state_digest;
            iteration.runtime_job_manifest_digest =
                terminal_evidence.job_manifest_digest;
            iteration.runtime_terminal_fact_digest =
                terminal_evidence.terminal_fact_digest;
            iteration.runtime_checkpoint_io_fact_digest =
                terminal_evidence.checkpoint_io_fact_digest;
            iteration.runtime_terminal_status =
                terminal_evidence.terminal_status;
            iteration.runtime_handoff_id = terminal_evidence.runtime_handoff_id;
            iteration.runtime_handoff_digest =
                terminal_evidence.runtime_handoff_digest;
            iteration.runtime_job_completion_observed =
                terminal_evidence.observed;
            if (driver_policy.require_runtime_job_completion &&
                !terminal_evidence.observed) {
              ledger.terminal_state = "blocked_runtime_completion_missing";
              ledger.terminal_reason =
                  "Runtime execution returned ok but no durable "
                  "runtime.result.fact and job.manifest were observed";
              iteration.refusal_reasons.emplace_back(
                  terminal_evidence.terminal_fact_digest.empty()
                      ? "runtime_result_fact_missing"
                      : "runtime_terminal_evidence_missing");
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }
            if (driver_policy.require_runtime_job_completion &&
                terminal_evidence.checkpoint_io_required &&
                terminal_evidence.checkpoint_io_fact_digest.empty()) {
              ledger.terminal_state = "blocked_runtime_checkpoint_io_missing";
              ledger.terminal_reason =
                  "Runtime terminal evidence reports checkpoint I/O but no "
                  "runtime.checkpoint_io.fact was observed";
              iteration.refusal_reasons.emplace_back(
                  "runtime_checkpoint_io_fact_missing");
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }
            if (driver_policy.require_runtime_job_completion &&
                terminal_evidence.terminal_status != "completed") {
              ledger.terminal_state = "blocked_runtime_refused";
              ledger.terminal_reason = "Runtime terminal job status was " +
                                       terminal_evidence.terminal_status;
              iteration.refusal_reasons.emplace_back(
                  "runtime_terminal_status_not_completed");
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            const auto runtime_execution_warning_stop =
                driver_policy.stop_on_runtime_warning
                    ? tool_detail::warning_stop_decision(
                          handoff.tool_result_json, "runtime",
                          driver_policy.stop_on_warning_severity)
                    : tool_detail::warning_stop_decision_t{};
            if (runtime_execution_warning_stop.stop) {
              ledger.terminal_state = "blocked_runtime_warning";
              iteration.refusal_reasons.emplace_back(
                  runtime_execution_warning_stop.reason_code.empty()
                      ? "runtime_execution_warning_stop"
                      : runtime_execution_warning_stop.reason_code);
              if (!runtime_execution_warning_stop.warning.warning_id.empty()) {
                iteration.refusal_reasons.emplace_back(
                    "runtime_warning_id:" +
                    runtime_execution_warning_stop.warning.warning_id);
              }
              ledger.terminal_reason =
                  std::string("Runtime execution warning ") +
                  runtime_execution_warning_stop.reason_code +
                  " matched driver_policy stop condition";
              iteration.terminal_state = ledger.terminal_state;
              iteration.terminal_reason = ledger.terminal_reason;
              append_iteration(iteration);
              break;
            }

            if (driver_policy.require_post_wave_lattice_satisfied_check) {
              auto post_step = tool_detail::run_reach_lattice_target_step(
                  args, callback, driver_policy.max_waves, attempt + 1,
                  marshal_dispatch_mode_t::dry_run, false,
                  include_machine_payload);
              iteration.post_run_lattice_evaluation_digest =
                  marshal_digest_for_text(
                      "kikijyeba.marshal.post_wave_lattice_evaluation.v1",
                      post_step.lattice_result);
              iteration.lattice_target_satisfied_after_wave =
                  post_step.advice.target_status == "satisfied";
              if (iteration.lattice_target_satisfied_after_wave) {
                ledger.terminal_state = "reached";
                ledger.terminal_reason =
                    "post-wave Lattice evaluation reported target satisfied";
                iteration.terminal_state = ledger.terminal_state;
                iteration.terminal_reason = ledger.terminal_reason;
                append_iteration(iteration);
                break;
              }
            }

            iteration.terminal_state = "continued";
            iteration.terminal_reason =
                "Runtime execution completed but target is not yet satisfied";
            append_iteration(iteration);
          }
          if (ledger.terminal_state.empty()) {
            ledger.terminal_state = "blocked_max_waves";
            ledger.terminal_reason = "driver_policy.max_waves exhausted";
          }
        }

        finalize_target_driver_run_id(&ledger);
        structured << tool_detail::reach_lattice_target_operator_packet_json(
            last_step, &ledger);
      }
    } else if (tool_name == "hero.marshal.status") {
      tool_detail::validate_fields(args,
                                   {"receipt_root", "limit", "receipts",
                                    "runtime_policy",
                                    "lattice_advice_surface_available"},
                                   {}, tool_name);
      std::vector<marshal_dispatch_receipt_t> receipts;
      if (const auto raw = tool_detail::optional_raw(args, "receipts")) {
        for (const auto &item_raw : tool_detail::array_values(*raw)) {
          receipts.push_back(tool_detail::parse_receipt(item_raw));
        }
      }
      marshal_runtime_policy_snapshot_t policy{};
      if (const auto raw = tool_detail::optional_raw(args, "runtime_policy")) {
        policy = tool_detail::parse_runtime_policy(*raw);
      }
      const auto status = make_marshal_status(
          tool_detail::optional_string(args, "receipt_root"), receipts, policy,
          tool_detail::optional_bool(args, "lattice_advice_surface_available",
                                     false));
      structured << "{\"ok\":true,\"recent_receipt_count\":"
                 << status.recent_receipt_count << ",\"last_refusal_reason\":"
                 << detail::json_quote(status.last_refusal_reason) << "}";
    } else {
      throw std::runtime_error("unknown tool: " + tool_name);
    }

    if (out_tool_result_json) {
      *out_tool_result_json =
          tool_detail::make_tool_result(tool_name, structured.str());
    }
    return true;
  } catch (const std::exception &ex) {
    if (out_error_message) {
      *out_error_message = ex.what();
    }
    if (out_tool_result_json) {
      *out_tool_result_json = tool_detail::make_error_result(ex.what());
    }
    return false;
  }
}

[[nodiscard]] inline std::string build_marshal_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < kMarshalTools.size(); ++i) {
    cuwacunu::hero::mcp_schema_compat::assert_tool_input_schema_compatible(
        kMarshalTools[i].name, kMarshalTools[i].input_schema_json);
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << detail::json_quote(kMarshalTools[i].name)
        << ",\"description\":"
        << detail::json_quote(kMarshalTools[i].description)
        << ",\"inputSchema\":" << kMarshalTools[i].input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] inline std::string build_marshal_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kMarshalTools) {
    out << tool.name << " - " << tool.description << "\n";
  }
  return out.str();
}

inline void run_marshal_jsonrpc_stdio_loop() {
  std::string line;
  while (std::getline(std::cin, line)) {
    line = tool_detail::trim_ascii(line);
    if (line.empty()) {
      continue;
    }
    std::string id_raw = "null";
    try {
      const auto request = tool_detail::object_fields(line);
      if (const auto id = tool_detail::optional_raw(request, "id")) {
        id_raw = *id;
      }
      const std::string method =
          tool_detail::optional_string(request, "method");
      if (method == "initialize") {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"result\":{\"protocolVersion\":\"2024-11-05\","
                     "\"capabilities\":{\"tools\":{}},\"serverInfo\":{"
                     "\"name\":\"hero-marshal\",\"version\":\"0.1\"}}}"
                  << std::endl;
        continue;
      }
      if (method == "tools/list") {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"result\":" << build_marshal_tools_list_result_json()
                  << "}" << std::endl;
        continue;
      }
      if (method == "tools/call") {
        const auto params_raw = tool_detail::optional_raw(request, "params");
        if (!params_raw.has_value()) {
          throw std::runtime_error("tools/call missing params");
        }
        const auto params = tool_detail::object_fields(*params_raw);
        const std::string name = tool_detail::optional_string(params, "name");
        const std::string arguments =
            tool_detail::optional_raw(params, "arguments").value_or("{}");
        std::string result;
        std::string error;
        (void)execute_marshal_tool_json(name, arguments, &result, &error);
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"result\":" << result << "}" << std::endl;
        continue;
      }
      throw std::runtime_error("unknown method: " + method);
    } catch (const std::exception &ex) {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"error\":{\"code\":-32602,\"message\":"
                << detail::json_quote(ex.what()) << "}}" << std::endl;
    }
  }
}

} // namespace cuwacunu::kikijyeba::marshal
