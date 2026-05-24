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

#include "kikijyeba/marshal/batch_preview.h"
#include "kikijyeba/marshal/codex_assist.h"
#include "kikijyeba/marshal/dispatch_receipt.h"
#include "kikijyeba/marshal/status.h"
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
                   "target_component", "mode", "source_range",
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
  const auto suggested_wave_raw = optional_raw(fields, "suggested_wave");
  if (!suggested_wave_raw.has_value()) {
    throw std::runtime_error("Lattice plan result missing suggested_wave");
  }
  const auto proof_raw = optional_raw(fields, "proof_certificate");
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
  out.target_status = optional_string(fields, "status");
  out.suggested_wave = parse_lattice_suggested_wave(*suggested_wave_raw);
  if (const auto active_raw = optional_raw(fields, "active_identity")) {
    out.active_identity = parse_lattice_active_identity(
        *active_raw, target_spec_fingerprint, split_policy_fingerprint);
  } else {
    out.active_identity.target_spec_fingerprint = target_spec_fingerprint;
    out.active_identity.split_policy_fingerprint = split_policy_fingerprint;
  }
  if (const auto plan_basis_raw = optional_raw(fields, "plan_basis")) {
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
  out.source_lattice_tool = "hero.lattice.plan_target";
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
lattice_resolve_latest_satisfying_arguments_json(
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
        lattice_resolve_latest_satisfying_arguments_json(args, value);
    if (callback == nullptr) {
      row.status = "resolver_unavailable";
    } else if (callback("hero.lattice.resolve_latest_satisfying", resolver_args,
                        &resolver_result, &resolver_error) &&
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
  const bool policy_match =
      policy.runtime_hero_available && policy.runtime_exec_exists &&
      policy.runtime_exec_executable && policy.default_dry_run;
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

[[nodiscard]] inline std::string lattice_plan_target_arguments_json(
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

[[nodiscard]] inline std::string
refusal_reasons_json(const std::vector<marshal_refusal_reason_t> &reasons) {
  std::vector<std::string> out;
  out.reserve(reasons.size());
  for (const auto reason : reasons) {
    out.emplace_back(to_string(reason));
  }
  return string_array_json(out);
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

    if (tool_name == "hero.marshal.lookup_target_advice") {
      tool_detail::validate_fields(
          args,
          {"target_id", "config_path", "runtime_root", "requested_mode",
           "source_lattice_timestamp", "max_waves",
           "recommendation_attempt_count", "context",
           "protocol_contract_fingerprint", "graph_order_fingerprint",
           "source_cursor_token", "vicreg_assembly_fingerprint",
           "mdn_assembly_fingerprint"},
          {"target_id"}, tool_name);
      auto callback = marshal_lattice_tool_callback_slot();
      if (callback == nullptr) {
        throw std::runtime_error(
            "Marshal target lookup requires a Lattice Hero callback");
      }
      std::string lattice_result;
      std::string lattice_error;
      const std::string lattice_args =
          tool_detail::lattice_plan_target_arguments_json(args);
      if (!callback("hero.lattice.plan_target", lattice_args, &lattice_result,
                    &lattice_error)) {
        throw std::runtime_error("Lattice Hero plan_target failed: " +
                                 lattice_error);
      }
      if (lattice_result.find("\"isError\":true") != std::string::npos ||
          lattice_result.find("\"isError\": true") != std::string::npos) {
        throw std::runtime_error("Lattice Hero plan_target returned isError");
      }
      const auto advice =
          tool_detail::materialize_advice_from_lattice_plan_result(
              lattice_result,
              tool_detail::optional_string(args, "source_lattice_timestamp"),
              tool_detail::optional_i64(args, "max_waves", -1),
              tool_detail::optional_i64(args, "recommendation_attempt_count",
                                        0));
      marshal_dispatch_request_t request{};
      request.requested_mode = tool_detail::parse_mode_text(
          tool_detail::optional_string(args, "requested_mode", "dry_run"));
      request.target_id = advice.target_id;
      request.config_path = advice.config_path;
      request.runtime_root = advice.runtime_root;
      request.advice_digest = dispatch_advice_digest(advice);
      marshal_dispatch_validation_context_t context{};
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        context = tool_detail::parse_context(*raw);
      }
      const auto validation =
          validate_dispatch_advice(advice, request, context);
      structured << "{\"ok\":true,\"source_lattice_tool\":"
                 << detail::json_quote("hero.lattice.plan_target")
                 << ",\"target_id\":" << detail::json_quote(advice.target_id)
                 << ",\"advice_digest\":"
                 << detail::json_quote(dispatch_advice_digest(advice))
                 << ",\"request_digest\":"
                 << detail::json_quote(dispatch_request_digest(request))
                 << ",\"dispatchable\":"
                 << (validation.dispatchable ? "true" : "false")
                 << ",\"refusal_reasons\":"
                 << tool_detail::refusal_reasons_json(
                        validation.refusal_reasons)
                 << ",\"advice\":" << tool_detail::advice_json(advice)
                 << ",\"request\":" << tool_detail::request_json(request)
                 << "}";
    } else if (tool_name == "hero.marshal.prepare_target_dispatch") {
      tool_detail::validate_fields(
          args,
          {"target_id", "config_path", "runtime_root", "requested_mode",
           "source_lattice_timestamp", "max_waves",
           "recommendation_attempt_count", "context",
           "protocol_contract_fingerprint", "graph_order_fingerprint",
           "source_cursor_token", "vicreg_assembly_fingerprint",
           "mdn_assembly_fingerprint", "materialize_plan_inputs",
           "include_runtime_dry_run", "include_machine_payload",
           "runtime_policy", "runtime_wave", "timeout_seconds"},
          {"target_id"}, tool_name);
      auto callback = marshal_lattice_tool_callback_slot();
      if (callback == nullptr) {
        throw std::runtime_error(
            "Marshal target dispatch preparation requires a Lattice Hero "
            "callback");
      }
      std::string lattice_result;
      std::string lattice_error;
      const std::string lattice_args =
          tool_detail::lattice_plan_target_arguments_json(args);
      if (!callback("hero.lattice.plan_target", lattice_args, &lattice_result,
                    &lattice_error)) {
        throw std::runtime_error("Lattice Hero plan_target failed: " +
                                 lattice_error);
      }
      if (lattice_result.find("\"isError\":true") != std::string::npos ||
          lattice_result.find("\"isError\": true") != std::string::npos) {
        throw std::runtime_error("Lattice Hero plan_target returned isError");
      }

      auto advice = tool_detail::materialize_advice_from_lattice_plan_result(
          lattice_result,
          tool_detail::optional_string(args, "source_lattice_timestamp"),
          tool_detail::optional_i64(args, "max_waves", -1),
          tool_detail::optional_i64(args, "recommendation_attempt_count", 0));
      const auto original_advice = advice;
      const bool materialize_inputs =
          tool_detail::optional_bool(args, "materialize_plan_inputs", true);
      std::vector<tool_detail::resolved_plan_input_t> model_state_inputs;
      if (materialize_inputs) {
        model_state_inputs =
            tool_detail::materialize_plan_inputs(args, callback, &advice);
      } else {
        for (const auto &[key, value] : advice.suggested_wave.plan_inputs) {
          tool_detail::resolved_plan_input_t row{};
          row.key = key;
          if (tool_detail::is_latest_satisfying_hint(value)) {
            row.symbolic_hint = value;
            row.status = "unresolved";
            row.resolution_owner = "lattice";
          } else {
            row.concrete_path = value;
            row.status = value.empty() ? "missing" : "concrete";
          }
          model_state_inputs.push_back(std::move(row));
        }
      }

      marshal_dispatch_request_t request{};
      request.requested_mode = tool_detail::parse_mode_text(
          tool_detail::optional_string(args, "requested_mode", "dry_run"));
      request.target_id = advice.target_id;
      request.config_path = advice.config_path;
      request.runtime_root = advice.runtime_root;
      request.advice_digest = dispatch_advice_digest(advice);

      marshal_dispatch_validation_context_t context{};
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        context = tool_detail::parse_context(*raw);
      }

      marshal_runtime_policy_snapshot_t policy{};
      marshal_runtime_wave_snapshot_t wave{};
      if (const auto raw = tool_detail::optional_raw(args, "runtime_policy")) {
        policy = tool_detail::parse_runtime_policy(*raw);
      }
      if (const auto raw = tool_detail::optional_raw(args, "runtime_wave")) {
        wave = tool_detail::parse_runtime_wave(*raw);
      }
      if (!tool_detail::optional_raw(args, "runtime_policy").has_value() ||
          !tool_detail::optional_raw(args, "runtime_wave").has_value()) {
        marshal_runtime_policy_snapshot_t live_policy{};
        marshal_runtime_wave_snapshot_t live_wave{};
        tool_detail::load_live_runtime_snapshots(args, advice, &live_policy,
                                                 &live_wave);
        if (!tool_detail::optional_raw(args, "runtime_policy").has_value()) {
          policy = live_policy;
        }
        if (!tool_detail::optional_raw(args, "runtime_wave").has_value()) {
          wave = live_wave;
        }
      }

      const auto decision = build_runtime_dry_run_dispatch_preview(
          advice, request, context, policy, wave);
      const bool include_runtime_dry_run =
          tool_detail::optional_bool(args, "include_runtime_dry_run", false);
      const bool include_machine_payload =
          tool_detail::optional_bool(args, "include_machine_payload", false);
      bool dry_run_attempted = false;
      bool dry_run_accepted = false;
      std::string dry_run_response_digest;
      std::string dry_run_runtime_request_digest;
      std::string dry_run_refusals_json = "[]";
      std::vector<marshal_refusal_reason_t> dry_run_refusal_reasons;
      std::string dry_run_bucket = "none";
      if (include_runtime_dry_run && decision.accepted) {
        marshal_runtime_hero_handoff_options_t options{};
        options.timeout_seconds = static_cast<int>(
            tool_detail::optional_i64(args, "timeout_seconds", 600));
        const auto response = run_marshal_dry_run_dispatch(
            advice, request, context, policy, wave, options);
        dry_run_attempted = true;
        dry_run_accepted = response.accepted;
        dry_run_response_digest = response.response_digest;
        dry_run_runtime_request_digest = response.runtime_request_digest;
        dry_run_refusal_reasons = response.decision.refusal_reasons;
        dry_run_refusals_json =
            tool_detail::refusal_reasons_json(dry_run_refusal_reasons);
        dry_run_bucket =
            tool_detail::blocker_bucket(response.decision, model_state_inputs);
      }
      auto bucket = tool_detail::blocker_bucket(decision, model_state_inputs);
      if (bucket == "none" && include_runtime_dry_run && dry_run_attempted &&
          !dry_run_accepted) {
        bucket = dry_run_bucket == "none" ? "runtime_dry_run_refused"
                                          : dry_run_bucket;
      }
      const auto state = tool_detail::dispatch_state_from_bucket(
          bucket, include_runtime_dry_run, dry_run_accepted);
      const auto next_action =
          tool_detail::next_action_for_state(bucket, state);
      const std::string prepare_digest = marshal_digest_for_text(
          "kikijyeba.marshal.prepare_target_dispatch.v1",
          dispatch_advice_digest(advice) + "\n" +
              dispatch_request_digest(request) + "\n" +
              runtime_policy_snapshot_digest(policy) + "\n" +
              runtime_wave_snapshot_digest(wave) + "\n" + bucket + "\n" +
              state + "\n");

      structured << "{\"ok\":true"
                 << ",\"tool\":"
                 << detail::json_quote("hero.marshal.prepare_target_dispatch")
                 << ",\"target_id\":" << detail::json_quote(advice.target_id)
                 << ",\"target_status\":"
                 << detail::json_quote(advice.target_status)
                 << ",\"dispatch_state\":" << detail::json_quote(state)
                 << ",\"blocker_bucket\":" << detail::json_quote(bucket)
                 << ",\"explanation\":"
                 << detail::json_quote(
                        tool_detail::operator_explanation(bucket, advice))
                 << ",\"suggested_wave\":"
                 << tool_detail::operator_wave_json(advice.suggested_wave)
                 << ",\"model_state_inputs\":"
                 << tool_detail::resolved_plan_inputs_json(model_state_inputs)
                 << ",\"runtime_wave_match\":"
                 << tool_detail::runtime_wave_match_json(advice, policy, wave,
                                                         model_state_inputs)
                 << ",\"next_action\":" << detail::json_quote(next_action)
                 << ",\"next_command\":{\"tool\":"
                 << detail::json_quote("hero.marshal.prepare_target_dispatch")
                 << ",\"args\":{\"target_id\":"
                 << detail::json_quote(advice.target_id)
                 << ",\"requested_mode\":\"dry_run\""
                 << ",\"include_runtime_dry_run\":"
                 << (next_action == "dry_run" ? "true" : "false")
                 << ",\"materialize_plan_inputs\":true}}"
                 << ",\"audit\":{\"receipt_id\":"
                 << detail::json_quote(prepare_digest)
                 << ",\"full_payload_available\":true"
                 << ",\"replay_tool\":\"hero.marshal.replay_receipt\"}"
                 << ",\"runtime_dry_run\":{\"requested\":"
                 << (include_runtime_dry_run ? "true" : "false")
                 << ",\"attempted\":" << (dry_run_attempted ? "true" : "false")
                 << ",\"accepted\":" << (dry_run_accepted ? "true" : "false")
                 << ",\"response_digest\":"
                 << detail::json_quote(dry_run_response_digest)
                 << ",\"runtime_request_digest\":"
                 << detail::json_quote(dry_run_runtime_request_digest)
                 << ",\"refusal_reasons\":" << dry_run_refusals_json << "}";
      if (include_machine_payload) {
        structured
            << ",\"machine_payload\":{\"source_lattice_tool\":"
            << detail::json_quote("hero.lattice.plan_target")
            << ",\"lattice_plan_result\":" << detail::json_quote(lattice_result)
            << ",\"original_advice\":"
            << tool_detail::advice_json(original_advice)
            << ",\"advice\":" << tool_detail::advice_json(advice)
            << ",\"request\":" << tool_detail::request_json(request)
            << ",\"validation\":{\"dispatchable\":"
            << (decision.advice_validation.dispatchable ? "true" : "false")
            << ",\"refusal_reasons\":"
            << tool_detail::refusal_reasons_json(
                   decision.advice_validation.refusal_reasons)
            << "},\"decision\":{\"accepted\":"
            << (decision.accepted ? "true" : "false")
            << ",\"runtime_handoff_available\":"
            << (decision.runtime_handoff_available ? "true" : "false")
            << ",\"refusal_reasons\":"
            << tool_detail::refusal_reasons_json(decision.refusal_reasons)
            << ",\"runtime_request_digest\":"
            << detail::json_quote(
                   runtime_dry_run_request_digest(decision.runtime_request))
            << "},\"runtime_policy_snapshot_digest\":"
            << detail::json_quote(runtime_policy_snapshot_digest(policy))
            << ",\"runtime_wave_snapshot_digest\":"
            << detail::json_quote(runtime_wave_snapshot_digest(wave)) << "}";
      }
      structured << "}";
    } else if (tool_name == "hero.marshal.validate_advice") {
      tool_detail::validate_fields(args, {"advice", "request", "context"},
                                   {"advice", "request"}, tool_name);
      const auto advice = tool_detail::parse_advice(args.at("advice"));
      const auto request = tool_detail::parse_request(args.at("request"));
      marshal_dispatch_validation_context_t context{};
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        context = tool_detail::parse_context(*raw);
      }
      const auto result = validate_dispatch_advice(advice, request, context);
      structured
          << "{\"ok\":" << (result.dispatchable ? "true" : "false")
          << ",\"dispatchable\":" << (result.dispatchable ? "true" : "false")
          << ",\"advice_digest\":" << detail::json_quote(result.advice_digest)
          << ",\"request_digest\":" << detail::json_quote(result.request_digest)
          << ",\"refusal_reasons\":"
          << tool_detail::refusal_reasons_json(result.refusal_reasons) << "}";
    } else if (tool_name == "hero.marshal.dry_run_dispatch") {
      tool_detail::validate_fields(args,
                                   {"advice", "request", "context",
                                    "runtime_policy", "runtime_wave",
                                    "timeout_seconds"},
                                   {"advice", "request"}, tool_name);
      const auto advice = tool_detail::parse_advice(args.at("advice"));
      const auto request = tool_detail::parse_request(args.at("request"));
      marshal_dispatch_validation_context_t context{};
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        context = tool_detail::parse_context(*raw);
      }
      marshal_runtime_policy_snapshot_t policy{};
      if (const auto raw = tool_detail::optional_raw(args, "runtime_policy")) {
        policy = tool_detail::parse_runtime_policy(*raw);
      }
      marshal_runtime_wave_snapshot_t wave{};
      if (const auto raw = tool_detail::optional_raw(args, "runtime_wave")) {
        wave = tool_detail::parse_runtime_wave(*raw);
      }
      marshal_runtime_hero_handoff_options_t options{};
      options.timeout_seconds = static_cast<int>(
          tool_detail::optional_i64(args, "timeout_seconds", 600));
      const auto response = run_marshal_dry_run_dispatch(
          advice, request, context, policy, wave, options);
      structured << "{\"ok\":" << (response.accepted ? "true" : "false")
                 << ",\"accepted\":" << (response.accepted ? "true" : "false")
                 << ",\"advice_digest\":"
                 << detail::json_quote(response.advice_digest)
                 << ",\"request_digest\":"
                 << detail::json_quote(response.request_digest)
                 << ",\"runtime_request_digest\":"
                 << detail::json_quote(response.runtime_request_digest)
                 << ",\"refusal_reasons\":"
                 << tool_detail::refusal_reasons_json(
                        response.decision.refusal_reasons)
                 << "}";
    } else if (tool_name == "hero.marshal.execution_gate") {
      tool_detail::validate_fields(
          args,
          {"advice", "request", "context", "prior_dry_run", "runtime_policy",
           "runtime_wave"},
          {"advice", "request", "prior_dry_run"}, tool_name);
      marshal_execution_gate_input_t input{};
      input.advice = tool_detail::parse_advice(args.at("advice"));
      input.request = tool_detail::parse_request(args.at("request"));
      input.prior_dry_run =
          tool_detail::parse_prior_dry_run(args.at("prior_dry_run"));
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        input.validation_context = tool_detail::parse_context(*raw);
      }
      if (const auto raw = tool_detail::optional_raw(args, "runtime_policy")) {
        input.policy = tool_detail::parse_runtime_policy(*raw);
      }
      if (const auto raw = tool_detail::optional_raw(args, "runtime_wave")) {
        input.active_wave = tool_detail::parse_runtime_wave(*raw);
      }
      const auto result = validate_execution_gate(input);
      structured << "{\"ok\":" << (result.accepted ? "true" : "false")
                 << ",\"accepted\":" << (result.accepted ? "true" : "false")
                 << ",\"expected_confirmation_token\":"
                 << detail::json_quote(result.expected_confirmation_token)
                 << ",\"refusal_reasons\":"
                 << tool_detail::refusal_reasons_json(result.refusal_reasons)
                 << "}";
    } else if (tool_name == "hero.marshal.replay_receipt") {
      tool_detail::validate_fields(args, {"receipt", "active_identity"},
                                   {"receipt", "active_identity"}, tool_name);
      const auto receipt = tool_detail::parse_receipt(args.at("receipt"));
      const auto active_identity =
          tool_detail::parse_active_identity(args.at("active_identity"));
      const auto audit = replay_dispatch_receipt(receipt, active_identity);
      structured << "{\"ok\":" << (audit.accepted ? "true" : "false")
                 << ",\"accepted\":" << (audit.accepted ? "true" : "false")
                 << ",\"stale\":" << (audit.stale ? "true" : "false")
                 << ",\"issues\":"
                 << tool_detail::string_array_json(audit.issues) << "}";
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
    } else if (tool_name == "hero.marshal.batch_preview") {
      tool_detail::validate_fields(
          args, {"items", "context", "runtime_policy", "timeout_seconds"},
          {"items"}, tool_name);
      marshal_dispatch_validation_context_t context{};
      if (const auto raw = tool_detail::optional_raw(args, "context")) {
        context = tool_detail::parse_context(*raw);
      }
      marshal_runtime_policy_snapshot_t policy{};
      if (const auto raw = tool_detail::optional_raw(args, "runtime_policy")) {
        policy = tool_detail::parse_runtime_policy(*raw);
      }
      std::vector<marshal_batch_preview_item_t> items;
      for (const auto &item_raw : tool_detail::array_values(args.at("items"))) {
        const auto item_fields = tool_detail::object_fields(item_raw);
        tool_detail::validate_fields(
            item_fields, {"item_id", "advice", "request", "runtime_wave"},
            {"advice", "request"}, "batch_item");
        marshal_batch_preview_item_t item{};
        item.item_id = tool_detail::optional_string(item_fields, "item_id");
        item.advice = tool_detail::parse_advice(item_fields.at("advice"));
        item.request = tool_detail::parse_request(item_fields.at("request"));
        if (const auto raw =
                tool_detail::optional_raw(item_fields, "runtime_wave")) {
          item.active_wave = tool_detail::parse_runtime_wave(*raw);
        }
        items.push_back(std::move(item));
      }
      marshal_runtime_hero_handoff_options_t options{};
      options.timeout_seconds = static_cast<int>(
          tool_detail::optional_i64(args, "timeout_seconds", 600));
      const auto preview = run_batch_preview(items, context, policy, options);
      std::size_t accepted_count = 0;
      for (const auto &response : preview.responses) {
        if (response.accepted) {
          ++accepted_count;
        }
      }
      structured << "{\"ok\":true,\"dry_run_only\":"
                 << (preview.dry_run_only ? "true" : "false")
                 << ",\"item_count\":" << preview.responses.size()
                 << ",\"accepted_count\":" << accepted_count << "}";
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
