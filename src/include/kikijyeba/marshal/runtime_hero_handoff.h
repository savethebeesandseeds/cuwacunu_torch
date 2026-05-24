// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

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
  const bool target_matches =
      json_has_string_field_value(wave_json, "target_component_family_id",
                                  request.wave_target) ||
      json_has_string_field_value(wave_json, "target_component",
                                  request.wave_target);
  if (!target_matches ||
      !json_has_string_field_value(wave_json, "mode", request.wave_mode) ||
      !json_has_string_field_value(wave_json, "source_range",
                                   request.source_range)) {
    return false;
  }
  if (request.anchor_index_begin.has_value() &&
      !json_has_string_field_value(
          wave_json, "anchor_index_begin",
          std::to_string(*request.anchor_index_begin))) {
    return false;
  }
  if (request.anchor_index_end.has_value() &&
      !json_has_string_field_value(wave_json, "anchor_index_end",
                                   std::to_string(*request.anchor_index_end))) {
    return false;
  }
  if (request.source_key_begin.has_value() &&
      !json_has_string_field_value(wave_json, "source_key_begin",
                                   std::to_string(*request.source_key_begin))) {
    return false;
  }
  if (request.source_key_end.has_value() &&
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
runtime_hero_execute_args_json(const marshal_runtime_dry_run_request_t &request,
                               int timeout_seconds, bool dry_run = true,
                               bool confirm_execute = false) {
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
  out << ",\"marshal_expected_wave\":{";
  bool first = true;
  detail::append_json_string_field(out, "target_component_family_id",
                                   request.wave_target, &first);
  detail::append_json_string_field(out, "mode", request.wave_mode, &first);
  detail::append_json_string_field(out, "wave_target", request.wave_target,
                                   &first);
  detail::append_json_string_field(out, "wave_mode", request.wave_mode, &first);
  detail::append_json_string_field(out, "source_range", request.source_range,
                                   &first);
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
  out.arguments_json = runtime_hero_execute_args_json(
      request, options.timeout_seconds, dry_run, confirm_execute);
  out.arguments_digest = marshal_digest_for_text(
      "kikijyeba.marshal.runtime_handoff_arguments.v1", out.arguments_json);

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
