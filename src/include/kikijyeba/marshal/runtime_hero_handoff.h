// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "hero/runtime_hero/hero_runtime_tools.h"
#include "kikijyeba/marshal/dispatch_adapter.h"

namespace cuwacunu::kikijyeba::marshal {

struct marshal_runtime_hero_handoff_options_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path policy_path{};
  int timeout_seconds{600};
};

struct marshal_runtime_hero_handoff_result_t {
  bool attempted{false};
  bool tool_call_ok{false};
  bool runtime_dry_run_ok{false};
  bool ok{false};
  bool tool_result_error{false};
  bool decoded_wave_checked{false};
  bool decoded_wave_matches_request{false};
  std::string tool_name{"hero.runtime.execute"};
  std::string arguments_json{};
  std::string arguments_digest{};
  std::string decoded_wave_json{};
  std::string decoded_wave_digest{};
  std::string tool_result_json{};
  std::string error_message{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

namespace detail {

[[nodiscard]] inline std::string json_quote(std::string_view s) {
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
        static constexpr char kHex[] = "0123456789abcdef";
        out << "\\u00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

inline void append_json_string_field(std::ostringstream &out,
                                     const std::string &key,
                                     const std::string &value, bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":" << json_quote(value);
}

inline void append_json_bool_field(std::ostringstream &out,
                                   const std::string &key, bool value,
                                   bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  out << json_quote(key) << ":" << (value ? "true" : "false");
}

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
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

[[nodiscard]] inline bool is_symbolic_model_state_input(std::string_view raw) {
  const std::string value = trim_ascii(raw);
  return value.rfind("latest_satisfying:", 0) == 0 ||
         value.rfind("lattice:", 0) == 0 || value.rfind("selector:", 0) == 0;
}

[[nodiscard]] inline std::vector<std::string> unresolved_model_state_symbols(
    const marshal_runtime_dry_run_request_t &request) {
  std::vector<std::string> symbols;
  for (const auto &[key, value] : request.model_state_inputs) {
    if (is_symbolic_model_state_input(value)) {
      symbols.push_back(key + "=" + trim_ascii(value));
    }
  }
  return symbols;
}

[[nodiscard]] inline std::string utc_now_rfc3339() {
  const std::time_t now = std::time(nullptr);
  std::tm tm{};
  if (gmtime_r(&now, &tm) == nullptr) {
    return "1970-01-01T00:00:00Z";
  }
  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

[[nodiscard]] inline std::string
file_content_digest_or_empty(const std::filesystem::path &path,
                             const std::string &domain) {
  if (path.empty()) {
    return {};
  }
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return {};
  }
  std::ostringstream text;
  text << in.rdbuf();
  return marshal_digest_for_text(domain, text.str());
}

inline void
append_json_string_map(std::ostringstream &out,
                       const std::map<std::string, std::string> &m) {
  out << "{";
  bool first = true;
  for (const auto &[key, value] : m) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << json_quote(key) << ":" << json_quote(normalize_path_text(value));
  }
  out << "}";
}

inline void append_json_string_array(std::ostringstream &out,
                                     const std::vector<std::string> &values) {
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0U) {
      out << ",";
    }
    out << json_quote(values[i]);
  }
  out << "]";
}

[[nodiscard]] inline bool json_bool_field_is_true(const std::string &json,
                                                  const std::string &key) {
  const std::string needle = "\"" + key + "\"";
  std::size_t pos = json.find(needle);
  while (pos != std::string::npos) {
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
      return false;
    }
    ++pos;
    while (pos < json.size() &&
           std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
      ++pos;
    }
    if (json.compare(pos, 4U, "true") == 0) {
      return true;
    }
    pos = json.find(needle, pos);
  }
  return false;
}

[[nodiscard]] inline bool skip_json_string(const std::string &json,
                                           std::size_t *pos) {
  if (*pos >= json.size() || json[*pos] != '"') {
    return false;
  }
  ++(*pos);
  while (*pos < json.size()) {
    const char c = json[*pos];
    ++(*pos);
    if (c == '"') {
      return true;
    }
    if (c == '\\') {
      if (*pos >= json.size()) {
        return false;
      }
      ++(*pos);
    }
  }
  return false;
}

inline void skip_json_ws(const std::string &json, std::size_t *pos) {
  while (*pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[*pos])) != 0) {
    ++(*pos);
  }
}

[[nodiscard]] inline bool skip_json_value(const std::string &json,
                                          std::size_t *pos) {
  skip_json_ws(json, pos);
  if (*pos >= json.size()) {
    return false;
  }
  if (json[*pos] == '"') {
    return skip_json_string(json, pos);
  }
  if (json[*pos] == '{' || json[*pos] == '[') {
    const char open = json[*pos];
    const char close = open == '{' ? '}' : ']';
    std::size_t depth = 1;
    ++(*pos);
    while (*pos < json.size()) {
      if (json[*pos] == '"') {
        if (!skip_json_string(json, pos)) {
          return false;
        }
        continue;
      }
      if (json[*pos] == open) {
        ++depth;
      } else if (json[*pos] == close) {
        --depth;
        if (depth == 0U) {
          ++(*pos);
          return true;
        }
      }
      ++(*pos);
    }
    return false;
  }
  const std::size_t begin = *pos;
  while (*pos < json.size()) {
    const char c = json[*pos];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++(*pos);
  }
  return *pos > begin;
}

[[nodiscard]] inline bool json_object_field_raw(const std::string &json,
                                                const std::string &key,
                                                std::string *raw) {
  std::size_t pos = 0;
  skip_json_ws(json, &pos);
  if (pos >= json.size() || json[pos] != '{') {
    return false;
  }
  ++pos;
  while (pos < json.size()) {
    skip_json_ws(json, &pos);
    if (pos < json.size() && json[pos] == '}') {
      return false;
    }
    if (pos >= json.size() || json[pos] != '"') {
      return false;
    }
    const std::size_t key_begin = pos + 1U;
    if (!skip_json_string(json, &pos)) {
      return false;
    }
    const std::string field_key = json.substr(key_begin, pos - key_begin - 1U);
    skip_json_ws(json, &pos);
    if (pos >= json.size() || json[pos] != ':') {
      return false;
    }
    ++pos;
    skip_json_ws(json, &pos);
    const std::size_t value_begin = pos;
    if (!skip_json_value(json, &pos)) {
      return false;
    }
    if (field_key == key) {
      if (raw) {
        *raw = json.substr(value_begin, pos - value_begin);
      }
      return true;
    }
    skip_json_ws(json, &pos);
    if (pos < json.size() && json[pos] == ',') {
      ++pos;
      continue;
    }
  }
  return false;
}

[[nodiscard]] inline bool json_bool_raw_is_true(const std::string &raw) {
  std::size_t pos = 0;
  skip_json_ws(raw, &pos);
  return raw.compare(pos, 4U, "true") == 0;
}

[[nodiscard]] inline bool mcp_tool_result_is_error(const std::string &json) {
  std::string raw;
  return json_object_field_raw(json, "isError", &raw) &&
         json_bool_raw_is_true(raw);
}

[[nodiscard]] inline bool
mcp_structured_content_bool_is_true(const std::string &json,
                                    const std::string &key) {
  std::string structured;
  if (!json_object_field_raw(json, "structuredContent", &structured)) {
    return false;
  }
  std::string raw;
  return json_object_field_raw(structured, key, &raw) &&
         json_bool_raw_is_true(raw);
}

[[nodiscard]] inline bool
json_has_string_field_value(const std::string &json, const std::string &key,
                            const std::string &value) {
  const std::string needle = "\"" + key + "\":" + json_quote(value);
  const std::string spaced_needle = "\"" + key + "\": " + json_quote(value);
  return json.find(needle) != std::string::npos ||
         json.find(spaced_needle) != std::string::npos;
}

[[nodiscard]] inline bool runtime_wave_json_matches_request(
    const std::string &json, const marshal_runtime_dry_run_request_t &request) {
  std::string wave_json = json;
  std::string structured;
  if (json_object_field_raw(json, "structuredContent", &structured)) {
    wave_json = structured;
  }
  if (!json_bool_field_is_true(wave_json, "readable")) {
    return false;
  }
  const bool target_matches = json_has_string_field_value(
      wave_json, "target_component_family_id", request.wave_target);
  if (!target_matches ||
      !json_has_string_field_value(wave_json, "mode", request.wave_mode)) {
    return false;
  }
  const bool exact_source_range = json_has_string_field_value(
      wave_json, "source_range", request.source_range);
  const bool overlay_anchor_range =
      request.source_range == "anchor_index" &&
      request.anchor_index_begin.has_value() &&
      request.anchor_index_end.has_value() &&
      *request.anchor_index_begin < *request.anchor_index_end;
  const bool overlay_source_key_range =
      request.source_range == "source_key" &&
      request.source_key_begin.has_value() &&
      request.source_key_end.has_value() &&
      *request.source_key_begin < *request.source_key_end;
  const bool reusable_all_profile =
      json_has_string_field_value(wave_json, "source_range", "all") &&
      (overlay_anchor_range || overlay_source_key_range);
  if (!exact_source_range && !reusable_all_profile) {
    return false;
  }
  if (!request.source_order.empty() &&
      !json_has_string_field_value(wave_json, "source_order",
                                   request.source_order)) {
    return false;
  }
  if (!reusable_all_profile && request.anchor_index_begin.has_value() &&
      !json_has_string_field_value(
          wave_json, "anchor_index_begin",
          std::to_string(*request.anchor_index_begin))) {
    return false;
  }
  if (!reusable_all_profile && request.anchor_index_end.has_value() &&
      !json_has_string_field_value(wave_json, "anchor_index_end",
                                   std::to_string(*request.anchor_index_end))) {
    return false;
  }
  if (!reusable_all_profile && request.source_key_begin.has_value() &&
      !json_has_string_field_value(wave_json, "source_key_begin",
                                   std::to_string(*request.source_key_begin))) {
    return false;
  }
  if (!reusable_all_profile && request.source_key_end.has_value() &&
      !json_has_string_field_value(wave_json, "source_key_end",
                                   std::to_string(*request.source_key_end))) {
    return false;
  }
  if (!request.model_state_inputs.empty()) {
    std::string inputs_raw;
    if (!json_object_field_raw(wave_json, "model_state_inputs", &inputs_raw)) {
      return false;
    }
    for (const auto &[key, value] : request.model_state_inputs) {
      std::string raw;
      if (!json_object_field_raw(inputs_raw, key, &raw)) {
        return false;
      }
      const std::string normalized =
          std::filesystem::path(value).lexically_normal().string();
      if (raw != json_quote(normalized)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace detail

[[nodiscard]] inline std::string
canonical_runtime_handoff_text(const marshal_runtime_dry_run_request_t &request,
                               bool dry_run, bool confirm_execute,
                               const std::filesystem::path &policy_path,
                               const std::string &created_at) {
  std::ostringstream out;
  detail::append_kv(out, "handoff_schema_version", "1");
  detail::append_kv(out, "created_by", "marshal");
  detail::append_kv(out, "created_at", created_at);
  detail::append_kv(out, "target_id", request.target_id);
  detail::append_kv(out, "base_config_path",
                    detail::normalize_path_text(request.config_path));
  detail::append_kv(out, "base_config_hash",
                    detail::file_content_digest_or_empty(
                        std::filesystem::path(request.config_path),
                        "kikijyeba.runtime.handoff.base_config_file.v1"));
  detail::append_kv(out, "wave_target", request.wave_target);
  detail::append_kv(out, "wave_mode", request.wave_mode);
  detail::append_kv(out, "source_range", request.source_range);
  detail::append_kv(out, "source_order", request.source_order);
  detail::append_kv(out, "anchor_index_begin",
                    detail::optional_size_text(request.anchor_index_begin));
  detail::append_kv(out, "anchor_index_end",
                    detail::optional_size_text(request.anchor_index_end));
  detail::append_kv(out, "source_key_begin",
                    detail::optional_i64_text(request.source_key_begin));
  detail::append_kv(out, "source_key_end",
                    detail::optional_i64_text(request.source_key_end));
  detail::append_string_map(out, "checkpoint_inputs",
                            request.model_state_inputs);
  detail::append_string_map(out, "lattice_certificate_refs",
                            request.lattice_certificate_refs);
  detail::append_kv(out, "target_driver_run_id", request.target_driver_run_id);
  detail::append_kv(out, "runtime_policy_path",
                    detail::normalize_path_text(policy_path.string()));
  detail::append_kv(
      out, "runtime_policy_hash",
      detail::file_content_digest_or_empty(
          policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1"));
  detail::append_kv(out, "dry_run", detail::bool_text(dry_run));
  detail::append_kv(out, "confirm_execute", detail::bool_text(confirm_execute));
  detail::append_kv(out, "force_rebuild_cache",
                    detail::bool_text(request.force_rebuild_cache));
  detail::append_string_vector(out, "unresolved_symbols",
                               detail::unresolved_model_state_symbols(request));
  return out.str();
}

[[nodiscard]] inline std::string
runtime_handoff_object_json(const marshal_runtime_dry_run_request_t &request,
                            bool dry_run, bool confirm_execute,
                            const std::filesystem::path &policy_path = {},
                            std::string created_at = {}) {
  if (created_at.empty()) {
    created_at = detail::utc_now_rfc3339();
  }
  const std::string canonical = canonical_runtime_handoff_text(
      request, dry_run, confirm_execute, policy_path, created_at);
  const std::string handoff_digest =
      marshal_digest_for_text("kikijyeba.runtime.handoff.object.v1", canonical);
  const std::string handoff_id = "runtime_handoff_" + handoff_digest;
  const std::string base_config_path =
      detail::normalize_path_text(request.config_path);
  const std::string base_config_hash = detail::file_content_digest_or_empty(
      std::filesystem::path(request.config_path),
      "kikijyeba.runtime.handoff.base_config_file.v1");
  const std::string runtime_policy_path =
      detail::normalize_path_text(policy_path.string());
  const std::string runtime_policy_hash = detail::file_content_digest_or_empty(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  const auto unresolved = detail::unresolved_model_state_symbols(request);

  std::ostringstream out;
  out << "{\"handoff_schema_version\":\"1\""
      << ",\"handoff_id\":" << detail::json_quote(handoff_id)
      << ",\"handoff_digest\":" << detail::json_quote(handoff_digest)
      << ",\"created_by\":\"marshal\""
      << ",\"created_at\":" << detail::json_quote(created_at)
      << ",\"target_id\":" << detail::json_quote(request.target_id);
  if (!request.target_driver_run_id.empty()) {
    out << ",\"target_driver_run_id\":"
        << detail::json_quote(request.target_driver_run_id);
  }
  out << ",\"base_config\":{\"path\":" << detail::json_quote(base_config_path)
      << ",\"hash\":" << detail::json_quote(base_config_hash) << "}"
      << ",\"wave\":{";
  bool first_wave = true;
  detail::append_json_string_field(out, "target_component_family_id",
                                   request.wave_target, &first_wave);
  detail::append_json_string_field(out, "mode", request.wave_mode, &first_wave);
  detail::append_json_string_field(out, "source_range", request.source_range,
                                   &first_wave);
  if (!request.source_order.empty()) {
    detail::append_json_string_field(out, "source_order", request.source_order,
                                     &first_wave);
  }
  if (request.anchor_index_begin.has_value()) {
    detail::append_json_string_field(
        out, "anchor_index_begin", std::to_string(*request.anchor_index_begin),
        &first_wave);
  }
  if (request.anchor_index_end.has_value()) {
    detail::append_json_string_field(out, "anchor_index_end",
                                     std::to_string(*request.anchor_index_end),
                                     &first_wave);
  }
  if (request.source_key_begin.has_value()) {
    detail::append_json_string_field(out, "source_key_begin",
                                     std::to_string(*request.source_key_begin),
                                     &first_wave);
  }
  if (request.source_key_end.has_value()) {
    detail::append_json_string_field(out, "source_key_end",
                                     std::to_string(*request.source_key_end),
                                     &first_wave);
  }
  out << ",\"model_state_inputs\":";
  detail::append_json_string_map(out, request.model_state_inputs);
  out << "},\"checkpoint_inputs\":";
  detail::append_json_string_map(out, request.model_state_inputs);
  out << ",\"checkpoint_artifact_hashes\":{}"
      << ",\"lattice_certificate_refs\":";
  detail::append_json_string_map(out, request.lattice_certificate_refs);
  out << ",\"runtime_policy\":{\"path\":"
      << detail::json_quote(runtime_policy_path)
      << ",\"hash\":" << detail::json_quote(runtime_policy_hash) << "}"
      << ",\"intent\":{\"dry_run\":" << (dry_run ? "true" : "false")
      << ",\"confirm_execute\":" << (confirm_execute ? "true" : "false")
      << ",\"force_rebuild_cache\":"
      << (request.force_rebuild_cache ? "true" : "false") << "}"
      << ",\"unresolved_symbols\":";
  detail::append_json_string_array(out, unresolved);
  out << "}";
  return out.str();
}

[[nodiscard]] inline std::string
runtime_hero_execute_args_json(const marshal_runtime_dry_run_request_t &request,
                               int timeout_seconds, bool dry_run = true,
                               bool confirm_execute = false,
                               const std::filesystem::path &policy_path = {}) {
  std::ostringstream out;
  out << "{\"config_path\":"
      << detail::json_quote(detail::normalize_path_text(request.config_path))
      << ",\"dry_run\":" << (dry_run ? "true" : "false")
      << ",\"confirm_execute\":" << (confirm_execute ? "true" : "false");
  if (request.force_rebuild_cache) {
    out << ",\"force_rebuild_cache\":true";
  }
  if (timeout_seconds > 0) {
    out << ",\"timeout_seconds\":" << timeout_seconds;
  }
  out << ",\"wave_overlay\":{";
  bool first_overlay = true;
  detail::append_json_string_field(out, "source_range", request.source_range,
                                   &first_overlay);
  if (request.anchor_index_begin.has_value()) {
    detail::append_json_string_field(
        out, "anchor_index_begin", std::to_string(*request.anchor_index_begin),
        &first_overlay);
  }
  if (request.anchor_index_end.has_value()) {
    detail::append_json_string_field(out, "anchor_index_end",
                                     std::to_string(*request.anchor_index_end),
                                     &first_overlay);
  }
  if (request.source_key_begin.has_value()) {
    detail::append_json_string_field(out, "source_key_begin",
                                     std::to_string(*request.source_key_begin),
                                     &first_overlay);
  }
  if (request.source_key_end.has_value()) {
    detail::append_json_string_field(out, "source_key_end",
                                     std::to_string(*request.source_key_end),
                                     &first_overlay);
  }
  out << "}";
  out << ",\"runtime_handoff\":"
      << runtime_handoff_object_json(request, dry_run, confirm_execute,
                                     policy_path);
  out << ",\"marshal_expected_wave\":{";
  bool first = true;
  detail::append_json_string_field(out, "target_component_family_id",
                                   request.wave_target, &first);
  detail::append_json_string_field(out, "mode", request.wave_mode, &first);
  detail::append_json_string_field(out, "source_range", request.source_range,
                                   &first);
  if (!request.source_order.empty()) {
    detail::append_json_string_field(out, "source_order", request.source_order,
                                     &first);
  }
  if (request.anchor_index_begin.has_value()) {
    detail::append_json_string_field(
        out, "anchor_index_begin", std::to_string(*request.anchor_index_begin),
        &first);
  }
  if (request.anchor_index_end.has_value()) {
    detail::append_json_string_field(out, "anchor_index_end",
                                     std::to_string(*request.anchor_index_end),
                                     &first);
  }
  if (request.source_key_begin.has_value()) {
    detail::append_json_string_field(out, "source_key_begin",
                                     std::to_string(*request.source_key_begin),
                                     &first);
  }
  if (request.source_key_end.has_value()) {
    detail::append_json_string_field(
        out, "source_key_end", std::to_string(*request.source_key_end), &first);
  }
  out << ",\"model_state_inputs\":{";
  bool first_input = true;
  for (const auto &[key, value] : request.model_state_inputs) {
    detail::append_json_string_field(
        out, key, detail::normalize_path_text(value), &first_input);
  }
  out << "}}";
  out << "}";
  return out.str();
}

namespace detail {

[[nodiscard]] inline marshal_runtime_hero_handoff_result_t
call_runtime_hero_execute_request(
    const marshal_runtime_dry_run_request_t &request,
    const marshal_runtime_hero_handoff_options_t &options, bool dry_run,
    bool confirm_execute) {
  marshal_runtime_hero_handoff_result_t out{};
  out.attempted = true;
  cuwacunu::hero::runtime::runtime_context_t ctx{};
  ctx.global_config_path = options.global_config_path.empty()
                               ? std::filesystem::path(request.config_path)
                               : options.global_config_path;
  ctx.policy_path =
      options.policy_path.empty()
          ? cuwacunu::hero::runtime::resolve_runtime_hero_dsl_path(
                ctx.global_config_path)
          : options.policy_path;

  std::string error;
  if (!cuwacunu::hero::runtime::load_runtime_policy(
          ctx.policy_path, ctx.global_config_path, &ctx.policy, &error)) {
    out.error_message = error;
    return out;
  }

  out.arguments_json =
      runtime_hero_execute_args_json(request, options.timeout_seconds, dry_run,
                                     confirm_execute, ctx.policy_path);
  out.arguments_digest = marshal_digest_for_text(
      "kikijyeba.marshal.runtime_handoff_arguments.v1", out.arguments_json);

  std::string wave_error;
  const std::string wave_args =
      "{\"config_path\":" +
      json_quote(detail::normalize_path_text(request.config_path)) + "}";
  out.decoded_wave_checked = true;
  const bool wave_call_ok = cuwacunu::hero::runtime::execute_tool_json(
      "hero.runtime.wave", wave_args, &ctx, &out.decoded_wave_json,
      &wave_error);
  out.decoded_wave_digest = marshal_digest_for_text(
      "kikijyeba.marshal.runtime_decoded_wave.v1", out.decoded_wave_json);
  out.decoded_wave_matches_request =
      wave_call_ok && !mcp_tool_result_is_error(out.decoded_wave_json) &&
      runtime_wave_json_matches_request(out.decoded_wave_json, request);
  if (!out.decoded_wave_matches_request) {
    out.error_message =
        "Runtime Hero decoded active wave does not match Marshal request";
    if (!wave_error.empty()) {
      out.error_message += ": " + wave_error;
    }
    return out;
  }

  out.tool_call_ok = cuwacunu::hero::runtime::execute_tool_json(
      out.tool_name, out.arguments_json, &ctx, &out.tool_result_json,
      &out.error_message);
  out.tool_result_error = mcp_tool_result_is_error(out.tool_result_json);
  out.runtime_dry_run_ok =
      mcp_structured_content_bool_is_true(out.tool_result_json, "ok");
  out.ok = out.tool_call_ok && !out.tool_result_error && out.runtime_dry_run_ok;
  return out;
}

} // namespace detail

[[nodiscard]] inline marshal_runtime_hero_handoff_result_t
call_runtime_hero_dry_run(
    const marshal_dispatch_decision_t &decision,
    const marshal_runtime_hero_handoff_options_t &options = {}) {
  if (!decision.accepted || !decision.runtime_handoff_available) {
    marshal_runtime_hero_handoff_result_t out{};
    out.arguments_json = runtime_hero_execute_args_json(
        decision.runtime_request, options.timeout_seconds, true, false);
    out.arguments_digest = marshal_digest_for_text(
        "kikijyeba.marshal.runtime_handoff_arguments.v1", out.arguments_json);
    out.error_message =
        "Marshal dispatch decision is not accepted for Runtime Hero handoff";
    return out;
  }
  return detail::call_runtime_hero_execute_request(decision.runtime_request,
                                                   options, true, false);
}

} // namespace cuwacunu::kikijyeba::marshal
