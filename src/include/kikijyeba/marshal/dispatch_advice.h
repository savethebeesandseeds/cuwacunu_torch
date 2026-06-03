// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "kikijyeba/marshal/digest.h"

namespace cuwacunu::kikijyeba::marshal {

inline constexpr const char *k_marshal_dispatch_advice_schema_v1 =
    "kikijyeba.marshal.dispatch_advice.v1";
inline constexpr const char *k_marshal_dispatch_request_schema_v1 =
    "kikijyeba.marshal.dispatch_request.v1";
inline constexpr const char *k_marshal_dispatch_non_authority_statement =
    "marshal dispatch advice and receipts are audit metadata only; target "
    "satisfaction remains a lattice proof over runtime evidence";

enum class marshal_dispatch_mode_t {
  unknown,
  plan,
  dry_run,
  execute,
};

[[nodiscard]] inline const char *to_string(marshal_dispatch_mode_t mode) {
  switch (mode) {
  case marshal_dispatch_mode_t::plan:
    return "plan";
  case marshal_dispatch_mode_t::dry_run:
    return "dry_run";
  case marshal_dispatch_mode_t::execute:
    return "execute";
  case marshal_dispatch_mode_t::unknown:
  default:
    return "unknown";
  }
}

enum class marshal_refusal_reason_t {
  missing_plan_basis,
  missing_suggested_wave,
  target_already_satisfied,
  target_status_not_dispatchable,
  stale_active_identity,
  stale_target_spec,
  stale_split_policy,
  max_waves_exhausted,
  malformed_wave_range,
  missing_model_state_input,
  forbidden_dispatch_mode,
  unknown_required_field,
  unsupported_wave_target,
  unsupported_source_range,
  advice_digest_mismatch,
  advice_runtime_root_mismatch,
  target_id_mismatch,
  runtime_policy_refused,
  runtime_wave_mismatch,
  checkpoint_input_mismatch,
  runtime_handoff_unavailable,
  unproven_lattice_advice,
  stale_lattice_advice,
  missing_confirmation,
  confirmation_mismatch,
  dry_run_receipt_missing,
  dry_run_receipt_mismatch,
  runtime_checkpoint_input_missing,
};

[[nodiscard]] inline const char *to_string(marshal_refusal_reason_t reason) {
  switch (reason) {
  case marshal_refusal_reason_t::missing_plan_basis:
    return "missing_plan_basis";
  case marshal_refusal_reason_t::missing_suggested_wave:
    return "missing_suggested_wave";
  case marshal_refusal_reason_t::target_already_satisfied:
    return "target_already_satisfied";
  case marshal_refusal_reason_t::target_status_not_dispatchable:
    return "target_status_not_dispatchable";
  case marshal_refusal_reason_t::stale_active_identity:
    return "stale_active_identity";
  case marshal_refusal_reason_t::stale_target_spec:
    return "stale_target_spec";
  case marshal_refusal_reason_t::stale_split_policy:
    return "stale_split_policy";
  case marshal_refusal_reason_t::max_waves_exhausted:
    return "max_waves_exhausted";
  case marshal_refusal_reason_t::malformed_wave_range:
    return "malformed_wave_range";
  case marshal_refusal_reason_t::missing_model_state_input:
    return "missing_model_state_input";
  case marshal_refusal_reason_t::forbidden_dispatch_mode:
    return "forbidden_dispatch_mode";
  case marshal_refusal_reason_t::unknown_required_field:
    return "unknown_required_field";
  case marshal_refusal_reason_t::unsupported_wave_target:
    return "unsupported_wave_target";
  case marshal_refusal_reason_t::unsupported_source_range:
    return "unsupported_source_range";
  case marshal_refusal_reason_t::advice_digest_mismatch:
    return "advice_digest_mismatch";
  case marshal_refusal_reason_t::advice_runtime_root_mismatch:
    return "advice_runtime_root_mismatch";
  case marshal_refusal_reason_t::target_id_mismatch:
    return "target_id_mismatch";
  case marshal_refusal_reason_t::runtime_policy_refused:
    return "runtime_policy_refused";
  case marshal_refusal_reason_t::runtime_wave_mismatch:
    return "runtime_wave_mismatch";
  case marshal_refusal_reason_t::checkpoint_input_mismatch:
    return "checkpoint_input_mismatch";
  case marshal_refusal_reason_t::runtime_handoff_unavailable:
    return "runtime_handoff_unavailable";
  case marshal_refusal_reason_t::unproven_lattice_advice:
    return "unproven_lattice_advice";
  case marshal_refusal_reason_t::stale_lattice_advice:
    return "stale_lattice_advice";
  case marshal_refusal_reason_t::missing_confirmation:
    return "missing_confirmation";
  case marshal_refusal_reason_t::confirmation_mismatch:
    return "confirmation_mismatch";
  case marshal_refusal_reason_t::dry_run_receipt_missing:
    return "dry_run_receipt_missing";
  case marshal_refusal_reason_t::dry_run_receipt_mismatch:
    return "dry_run_receipt_mismatch";
  case marshal_refusal_reason_t::runtime_checkpoint_input_missing:
    return "runtime_checkpoint_input_missing";
  default:
    return "unknown_required_field";
  }
}

struct marshal_active_identity_t {
  std::string protocol_contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string target_spec_fingerprint{};
  std::string split_policy_fingerprint{};
};

struct marshal_plan_basis_t {
  bool present{false};
  bool available{false};
  std::string source_range{"anchor_index"};
  std::optional<std::size_t> target_anchor_index_begin{std::nullopt};
  std::optional<std::size_t> target_anchor_index_end{std::nullopt};
  std::optional<std::int64_t> target_source_key_begin{std::nullopt};
  std::optional<std::int64_t> target_source_key_end{std::nullopt};
  std::string primary_deficit_key{};
  std::string primary_deficit_message{};
  std::vector<std::string> deficit_keys{};
  std::vector<std::string> deficit_priority_classes{};
};

struct marshal_suggested_wave_t {
  std::string target{};
  std::string mode{};
  std::string source_range{"anchor_index"};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
  std::map<std::string, std::string> plan_inputs{};

  [[nodiscard]] bool empty() const {
    return target.empty() || mode.empty() || source_range.empty();
  }
};

struct marshal_dispatch_advice_t {
  std::string schema_version{k_marshal_dispatch_advice_schema_v1};
  std::string config_path{};
  std::string runtime_root{};
  std::string target_id{};
  std::string target_status{};
  marshal_active_identity_t active_identity{};
  marshal_plan_basis_t plan_basis{};
  std::string plan_basis_digest{};
  marshal_suggested_wave_t suggested_wave{};
  std::string suggested_wave_digest{};
  std::string plan_input_digest{};
  std::int64_t max_waves{-1};
  std::int64_t recommendation_attempt_count{0};
  std::vector<std::string> required_plan_inputs{};
  std::string source_lattice_tool{};
  std::string source_lattice_timestamp{};
  std::string advice_receipt_digest{};
  std::string advice_signature_key_id{};
  std::string advice_signature{};
};

struct marshal_dispatch_request_t {
  std::string schema_version{k_marshal_dispatch_request_schema_v1};
  marshal_dispatch_mode_t requested_mode{marshal_dispatch_mode_t::dry_run};
  std::string target_id{};
  std::string config_path{};
  std::string runtime_root{};
  std::string advice_digest{};
  std::string operator_confirmation_token{};
  std::string target_driver_run_id{};
  std::map<std::string, std::string> requested_overrides{};
  std::map<std::string, std::string> lattice_certificate_refs{};
};

struct marshal_dispatch_validation_context_t {
  std::string active_config_path{};
  std::string active_runtime_root{};
  marshal_active_identity_t active_identity{};
  std::set<std::string> supported_wave_targets{};
  std::set<std::string> supported_source_ranges{"anchor_index", "source_key"};
  std::set<std::string> allowed_lattice_tools{"hero.lattice.target_deficit",
                                              "hero.lattice.evaluate_target",
                                              "hero.lattice.evaluate_targets"};
  std::vector<std::string> allowed_model_state_roots{};
  std::string freshness_check_timestamp_utc{};
  std::int64_t max_advice_age_seconds{3600};
  std::int64_t allowed_clock_skew_seconds{300};
  bool require_signed_lattice_advice{false};
  std::string trusted_lattice_advice_key_id{};
  std::string trusted_lattice_advice_verification_key{};
  bool allow_execute{false};
  bool allow_execute_mode{false};
};

struct marshal_dispatch_validation_result_t {
  bool dispatchable{false};
  std::vector<marshal_refusal_reason_t> refusal_reasons{};
  std::vector<std::string> messages{};
  std::string advice_digest{};
  std::string request_digest{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};

  [[nodiscard]] bool has(marshal_refusal_reason_t reason) const {
    return std::find(refusal_reasons.begin(), refusal_reasons.end(), reason) !=
           refusal_reasons.end();
  }
};

namespace detail {

[[nodiscard]] inline std::string normalize_path_text(const std::string &path) {
  if (path.empty()) {
    return {};
  }
  return std::filesystem::path(path).lexically_normal().string();
}

[[nodiscard]] inline std::string bool_text(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] inline std::string
optional_size_text(const std::optional<std::size_t> &value) {
  if (!value.has_value()) {
    return "<none>";
  }
  return std::to_string(*value);
}

[[nodiscard]] inline std::string
optional_i64_text(const std::optional<std::int64_t> &value) {
  if (!value.has_value()) {
    return "<none>";
  }
  return std::to_string(*value);
}

inline void append_kv(std::ostringstream &out, const std::string &key,
                      const std::string &value) {
  out << key << "=" << value.size() << ":" << value << "\n";
}

inline void append_string_vector(std::ostringstream &out,
                                 const std::string &prefix,
                                 std::vector<std::string> values) {
  std::sort(values.begin(), values.end());
  append_kv(out, prefix + ".count", std::to_string(values.size()));
  for (std::size_t i = 0; i < values.size(); ++i) {
    append_kv(out, prefix + "." + std::to_string(i), values[i]);
  }
}

inline void append_string_map(std::ostringstream &out,
                              const std::string &prefix,
                              const std::map<std::string, std::string> &map) {
  append_kv(out, prefix + ".count", std::to_string(map.size()));
  std::size_t i = 0;
  for (const auto &[key, value] : map) {
    append_kv(out, prefix + "." + std::to_string(i) + ".key", key);
    append_kv(out, prefix + "." + std::to_string(i) + ".value", value);
    ++i;
  }
}

[[nodiscard]] inline bool starts_with(std::string_view text,
                                      std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] inline bool
dispatchable_target_status(const std::string &status) {
  return status == "blocked" || status == "exposure_failed" ||
         status == "metric_failed" || status == "missing_report" ||
         status == "missing_checkpoint" || status == "unsatisfied" ||
         status == "not_satisfied" || status == "pending" ||
         status == "stale_contract";
}

[[nodiscard]] inline bool
parse_utc_timestamp_seconds(const std::string &timestamp,
                            std::int64_t *seconds_out) {
  if (!seconds_out || timestamp.size() != 20U || timestamp.back() != 'Z') {
    return false;
  }
  std::tm tm{};
  std::istringstream in(timestamp);
  in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  if (in.fail()) {
    return false;
  }
  tm.tm_isdst = 0;
  const std::time_t seconds = timegm(&tm);
  if (seconds == static_cast<std::time_t>(-1)) {
    return false;
  }
  *seconds_out = static_cast<std::int64_t>(seconds);
  return true;
}

[[nodiscard]] inline bool
path_components_within(const std::filesystem::path &root,
                       const std::filesystem::path &path) {
  auto root_it = root.begin();
  auto path_it = path.begin();
  for (; root_it != root.end(); ++root_it, ++path_it) {
    if (path_it == path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::filesystem::path
canonicalize_path_for_boundary(const std::filesystem::path &path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  if (!ec) {
    return canonical;
  }
  return path.lexically_normal();
}

[[nodiscard]] inline bool
path_within_any_allowed_root(const std::string &path,
                             const std::vector<std::string> &roots) {
  if (roots.empty()) {
    return true;
  }
  const auto normalized_path =
      canonicalize_path_for_boundary(std::filesystem::path(path));
  for (const auto &root : roots) {
    if (root.empty()) {
      continue;
    }
    const auto normalized_root =
        canonicalize_path_for_boundary(std::filesystem::path(root));
    if (path_components_within(normalized_root, normalized_path)) {
      return true;
    }
  }
  return false;
}

} // namespace detail

[[nodiscard]] inline bool
source_lattice_tool_is_proven_advice(const std::string &tool) {
  return tool == "hero.lattice.target_deficit" ||
         tool == "hero.lattice.evaluate_target" ||
         tool == "hero.lattice.evaluate_targets";
}

[[nodiscard]] inline std::string
canonical_active_identity_text(const marshal_active_identity_t &identity) {
  std::ostringstream out;
  detail::append_kv(out, "protocol_contract_fingerprint",
                    identity.protocol_contract_fingerprint);
  detail::append_kv(out, "graph_order_fingerprint",
                    identity.graph_order_fingerprint);
  detail::append_kv(out, "target_spec_fingerprint",
                    identity.target_spec_fingerprint);
  detail::append_kv(out, "split_policy_fingerprint",
                    identity.split_policy_fingerprint);
  return out.str();
}

[[nodiscard]] inline std::string
canonical_plan_basis_text(const marshal_plan_basis_t &basis) {
  std::ostringstream out;
  detail::append_kv(out, "present", detail::bool_text(basis.present));
  detail::append_kv(out, "available", detail::bool_text(basis.available));
  detail::append_kv(out, "source_range", basis.source_range);
  detail::append_kv(
      out, "target_anchor_index_begin",
      detail::optional_size_text(basis.target_anchor_index_begin));
  detail::append_kv(out, "target_anchor_index_end",
                    detail::optional_size_text(basis.target_anchor_index_end));
  detail::append_kv(out, "target_source_key_begin",
                    detail::optional_i64_text(basis.target_source_key_begin));
  detail::append_kv(out, "target_source_key_end",
                    detail::optional_i64_text(basis.target_source_key_end));
  detail::append_kv(out, "primary_deficit_key", basis.primary_deficit_key);
  detail::append_kv(out, "primary_deficit_message",
                    basis.primary_deficit_message);
  detail::append_string_vector(out, "deficit_keys", basis.deficit_keys);
  detail::append_string_vector(out, "deficit_priority_classes",
                               basis.deficit_priority_classes);
  return out.str();
}

[[nodiscard]] inline std::string
plan_basis_digest(const marshal_plan_basis_t &basis) {
  return marshal_digest_for_text("kikijyeba.marshal.plan_basis.v1",
                                 canonical_plan_basis_text(basis));
}

[[nodiscard]] inline std::string
canonical_plan_inputs_text(const std::map<std::string, std::string> &inputs) {
  std::ostringstream out;
  detail::append_string_map(out, "plan_inputs", inputs);
  return out.str();
}

[[nodiscard]] inline std::string
plan_input_digest(const std::map<std::string, std::string> &inputs) {
  return marshal_digest_for_text("kikijyeba.marshal.plan_inputs.v1",
                                 canonical_plan_inputs_text(inputs));
}

[[nodiscard]] inline std::string
canonical_suggested_wave_text(const marshal_suggested_wave_t &wave) {
  std::ostringstream out;
  detail::append_kv(out, "target", wave.target);
  detail::append_kv(out, "mode", wave.mode);
  detail::append_kv(out, "source_range", wave.source_range);
  detail::append_kv(out, "anchor_index_begin",
                    detail::optional_size_text(wave.anchor_index_begin));
  detail::append_kv(out, "anchor_index_end",
                    detail::optional_size_text(wave.anchor_index_end));
  detail::append_kv(out, "source_key_begin",
                    detail::optional_i64_text(wave.source_key_begin));
  detail::append_kv(out, "source_key_end",
                    detail::optional_i64_text(wave.source_key_end));
  detail::append_kv(out, "plan_input_digest",
                    plan_input_digest(wave.plan_inputs));
  detail::append_string_map(out, "plan_inputs", wave.plan_inputs);
  return out.str();
}

[[nodiscard]] inline std::string
suggested_wave_digest(const marshal_suggested_wave_t &wave) {
  return marshal_digest_for_text("kikijyeba.marshal.suggested_wave.v1",
                                 canonical_suggested_wave_text(wave));
}

[[nodiscard]] inline std::string
canonical_dispatch_advice_text(const marshal_dispatch_advice_t &advice) {
  const auto computed_plan_basis_digest = plan_basis_digest(advice.plan_basis);
  const auto computed_suggested_wave_digest =
      suggested_wave_digest(advice.suggested_wave);
  const auto computed_plan_input_digest =
      plan_input_digest(advice.suggested_wave.plan_inputs);

  std::ostringstream out;
  detail::append_kv(out, "schema_version", advice.schema_version);
  detail::append_kv(out, "config_path",
                    detail::normalize_path_text(advice.config_path));
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(advice.runtime_root));
  detail::append_kv(out, "target_id", advice.target_id);
  detail::append_kv(out, "target_status", advice.target_status);
  detail::append_kv(out, "active_identity",
                    canonical_active_identity_text(advice.active_identity));
  detail::append_kv(out, "plan_basis",
                    canonical_plan_basis_text(advice.plan_basis));
  detail::append_kv(out, "plan_basis_digest",
                    advice.plan_basis_digest.empty()
                        ? computed_plan_basis_digest
                        : advice.plan_basis_digest);
  detail::append_kv(out, "suggested_wave",
                    canonical_suggested_wave_text(advice.suggested_wave));
  detail::append_kv(out, "suggested_wave_digest",
                    advice.suggested_wave_digest.empty()
                        ? computed_suggested_wave_digest
                        : advice.suggested_wave_digest);
  detail::append_kv(out, "plan_input_digest",
                    advice.plan_input_digest.empty()
                        ? computed_plan_input_digest
                        : advice.plan_input_digest);
  detail::append_kv(out, "max_waves", std::to_string(advice.max_waves));
  detail::append_kv(out, "recommendation_attempt_count",
                    std::to_string(advice.recommendation_attempt_count));
  detail::append_string_vector(out, "required_plan_inputs",
                               advice.required_plan_inputs);
  detail::append_kv(out, "source_lattice_tool", advice.source_lattice_tool);
  detail::append_kv(out, "source_lattice_timestamp",
                    advice.source_lattice_timestamp);
  detail::append_kv(out, "advice_receipt_digest", advice.advice_receipt_digest);
  detail::append_kv(out, "advice_signature_key_id",
                    advice.advice_signature_key_id);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_dispatch_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
dispatch_advice_digest(const marshal_dispatch_advice_t &advice) {
  return marshal_digest_for_text("kikijyeba.marshal.dispatch_advice.v1",
                                 canonical_dispatch_advice_text(advice));
}

[[nodiscard]] inline std::string canonical_lattice_advice_signature_payload(
    const marshal_dispatch_advice_t &advice) {
  std::ostringstream out;
  detail::append_kv(out, "advice_digest", dispatch_advice_digest(advice));
  detail::append_kv(out, "advice_receipt_digest", advice.advice_receipt_digest);
  detail::append_kv(out, "source_lattice_tool", advice.source_lattice_tool);
  detail::append_kv(out, "source_lattice_timestamp",
                    advice.source_lattice_timestamp);
  return out.str();
}

[[nodiscard]] inline std::string
sign_lattice_advice_receipt(const marshal_dispatch_advice_t &advice,
                            const std::string &key_id,
                            const std::string &verification_key) {
  std::ostringstream payload;
  detail::append_kv(payload, "key_id", key_id);
  detail::append_kv(payload, "verification_key", verification_key);
  detail::append_kv(payload, "payload",
                    canonical_lattice_advice_signature_payload(advice));
  return marshal_digest_for_text(
      "kikijyeba.marshal.lattice_advice_signature.v1", payload.str());
}

[[nodiscard]] inline std::string
canonical_dispatch_request_text(const marshal_dispatch_request_t &request) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", request.schema_version);
  detail::append_kv(out, "requested_mode", to_string(request.requested_mode));
  detail::append_kv(out, "target_id", request.target_id);
  detail::append_kv(out, "config_path",
                    detail::normalize_path_text(request.config_path));
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(request.runtime_root));
  detail::append_kv(out, "advice_digest", request.advice_digest);
  detail::append_kv(out, "operator_confirmation_token",
                    request.operator_confirmation_token);
  detail::append_kv(out, "target_driver_run_id", request.target_driver_run_id);
  detail::append_string_map(out, "requested_overrides",
                            request.requested_overrides);
  detail::append_string_map(out, "lattice_certificate_refs",
                            request.lattice_certificate_refs);
  return out.str();
}

[[nodiscard]] inline std::string
dispatch_request_digest(const marshal_dispatch_request_t &request) {
  return marshal_digest_for_text("kikijyeba.marshal.dispatch_request.v1",
                                 canonical_dispatch_request_text(request));
}

[[nodiscard]] inline std::string canonical_dispatch_request_identity_text(
    const marshal_dispatch_request_t &request) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", request.schema_version);
  detail::append_kv(out, "target_id", request.target_id);
  detail::append_kv(out, "config_path",
                    detail::normalize_path_text(request.config_path));
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(request.runtime_root));
  detail::append_kv(out, "advice_digest", request.advice_digest);
  detail::append_kv(out, "target_driver_run_id", request.target_driver_run_id);
  detail::append_string_map(out, "requested_overrides",
                            request.requested_overrides);
  detail::append_string_map(out, "lattice_certificate_refs",
                            request.lattice_certificate_refs);
  return out.str();
}

[[nodiscard]] inline std::string
dispatch_request_identity_digest(const marshal_dispatch_request_t &request) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.dispatch_request_identity.v1",
      canonical_dispatch_request_identity_text(request));
}

[[nodiscard]] inline std::string canonical_dispatch_validation_context_text(
    const marshal_dispatch_validation_context_t &context) {
  std::vector<std::string> supported_wave_targets(
      context.supported_wave_targets.begin(),
      context.supported_wave_targets.end());
  std::vector<std::string> supported_source_ranges(
      context.supported_source_ranges.begin(),
      context.supported_source_ranges.end());
  std::vector<std::string> allowed_lattice_tools(
      context.allowed_lattice_tools.begin(),
      context.allowed_lattice_tools.end());
  std::ostringstream out;
  detail::append_kv(out, "active_config_path",
                    detail::normalize_path_text(context.active_config_path));
  detail::append_kv(out, "active_runtime_root",
                    detail::normalize_path_text(context.active_runtime_root));
  detail::append_kv(out, "active_identity",
                    canonical_active_identity_text(context.active_identity));
  detail::append_string_vector(out, "supported_wave_targets",
                               supported_wave_targets);
  detail::append_string_vector(out, "supported_source_ranges",
                               supported_source_ranges);
  detail::append_string_vector(out, "allowed_lattice_tools",
                               allowed_lattice_tools);
  detail::append_string_vector(out, "allowed_model_state_roots",
                               context.allowed_model_state_roots);
  detail::append_kv(out, "freshness_check_timestamp_utc",
                    context.freshness_check_timestamp_utc);
  detail::append_kv(out, "max_advice_age_seconds",
                    std::to_string(context.max_advice_age_seconds));
  detail::append_kv(out, "allowed_clock_skew_seconds",
                    std::to_string(context.allowed_clock_skew_seconds));
  detail::append_kv(out, "require_signed_lattice_advice",
                    detail::bool_text(context.require_signed_lattice_advice));
  detail::append_kv(out, "trusted_lattice_advice_key_id",
                    context.trusted_lattice_advice_key_id);
  detail::append_kv(out, "trusted_lattice_advice_verification_key",
                    context.trusted_lattice_advice_verification_key);
  detail::append_kv(out, "allow_execute",
                    detail::bool_text(context.allow_execute));
  detail::append_kv(out, "allow_execute_mode",
                    detail::bool_text(context.allow_execute_mode));
  return out.str();
}

[[nodiscard]] inline std::string dispatch_validation_context_digest(
    const marshal_dispatch_validation_context_t &context) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.dispatch_validation_context.v1",
      canonical_dispatch_validation_context_text(context));
}

inline void add_refusal(marshal_dispatch_validation_result_t &result,
                        marshal_refusal_reason_t reason,
                        const std::string &message) {
  if (!result.has(reason)) {
    result.refusal_reasons.push_back(reason);
  }
  result.messages.push_back(std::string(to_string(reason)) + ": " + message);
}

[[nodiscard]] inline bool same_path_text(const std::string &lhs,
                                         const std::string &rhs) {
  return detail::normalize_path_text(lhs) == detail::normalize_path_text(rhs);
}

[[nodiscard]] inline marshal_dispatch_validation_result_t
validate_dispatch_advice(const marshal_dispatch_advice_t &advice,
                         const marshal_dispatch_request_t &request,
                         const marshal_dispatch_validation_context_t &context) {
  marshal_dispatch_validation_result_t result{};
  result.advice_digest = dispatch_advice_digest(advice);
  result.request_digest = dispatch_request_digest(request);

  if (advice.schema_version != k_marshal_dispatch_advice_schema_v1 ||
      request.schema_version != k_marshal_dispatch_request_schema_v1) {
    add_refusal(result, marshal_refusal_reason_t::unknown_required_field,
                "unsupported Marshal dispatch schema version");
  }

  if (advice.config_path.empty() || advice.runtime_root.empty() ||
      advice.target_id.empty() ||
      advice.active_identity.protocol_contract_fingerprint.empty() ||
      advice.active_identity.target_spec_fingerprint.empty() ||
      advice.active_identity.split_policy_fingerprint.empty()) {
    add_refusal(result, marshal_refusal_reason_t::unknown_required_field,
                "dispatch advice is missing required identity fields");
  }

  if (!source_lattice_tool_is_proven_advice(advice.source_lattice_tool) ||
      (!context.allowed_lattice_tools.empty() &&
       context.allowed_lattice_tools.find(advice.source_lattice_tool) ==
           context.allowed_lattice_tools.end())) {
    add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                "source_lattice_tool is not an allowed Lattice advice tool");
  }

  std::int64_t advice_timestamp_seconds = 0;
  if (!detail::parse_utc_timestamp_seconds(advice.source_lattice_timestamp,
                                           &advice_timestamp_seconds)) {
    add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                "source_lattice_timestamp must be RFC3339 UTC");
  } else if (!context.freshness_check_timestamp_utc.empty()) {
    std::int64_t now_seconds = 0;
    if (!detail::parse_utc_timestamp_seconds(
            context.freshness_check_timestamp_utc, &now_seconds)) {
      add_refusal(result, marshal_refusal_reason_t::unknown_required_field,
                  "freshness_check_timestamp_utc must be RFC3339 UTC");
    } else {
      if (advice_timestamp_seconds >
          now_seconds + context.allowed_clock_skew_seconds) {
        add_refusal(result, marshal_refusal_reason_t::stale_lattice_advice,
                    "source_lattice_timestamp is in the future");
      }
      if (now_seconds - advice_timestamp_seconds >
          context.max_advice_age_seconds) {
        add_refusal(result, marshal_refusal_reason_t::stale_lattice_advice,
                    "source_lattice_timestamp is older than max_advice_age");
      }
    }
  }

  const bool signature_fields_present = !advice.advice_signature.empty() ||
                                        !advice.advice_signature_key_id.empty();
  if (!advice.advice_receipt_digest.empty() &&
      !marshal_digest_is_strong_hex(advice.advice_receipt_digest)) {
    add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                "advice_receipt_digest must be a strong SHA-256 hex digest");
  }
  if (context.require_signed_lattice_advice && !signature_fields_present) {
    add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                "signed Lattice advice receipt is required");
  }
  if (signature_fields_present) {
    if (!marshal_digest_is_strong_hex(advice.advice_receipt_digest)) {
      add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                  "advice_receipt_digest must be a strong SHA-256 hex digest");
    }
    if (!marshal_digest_is_strong_hex(advice.advice_signature)) {
      add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                  "advice_signature must be a strong SHA-256 hex digest");
    }
    if (advice.advice_signature_key_id.empty() ||
        context.trusted_lattice_advice_key_id.empty() ||
        context.trusted_lattice_advice_verification_key.empty()) {
      add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                  "signed advice requires trusted verification context");
    } else if (advice.advice_signature_key_id !=
               context.trusted_lattice_advice_key_id) {
      add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                  "advice signature key id is not trusted");
    } else {
      const auto expected_signature = sign_lattice_advice_receipt(
          advice, context.trusted_lattice_advice_key_id,
          context.trusted_lattice_advice_verification_key);
      if (advice.advice_signature != expected_signature) {
        add_refusal(result, marshal_refusal_reason_t::unproven_lattice_advice,
                    "advice signature does not match canonical advice");
      }
    }
  }

  const auto computed_plan_basis_digest = plan_basis_digest(advice.plan_basis);
  const auto computed_suggested_wave_digest =
      suggested_wave_digest(advice.suggested_wave);
  const auto computed_plan_input_digest =
      plan_input_digest(advice.suggested_wave.plan_inputs);

  if ((!advice.plan_basis_digest.empty() &&
       advice.plan_basis_digest != computed_plan_basis_digest) ||
      (!advice.suggested_wave_digest.empty() &&
       advice.suggested_wave_digest != computed_suggested_wave_digest) ||
      (!advice.plan_input_digest.empty() &&
       advice.plan_input_digest != computed_plan_input_digest) ||
      (!request.advice_digest.empty() &&
       request.advice_digest != result.advice_digest)) {
    add_refusal(result, marshal_refusal_reason_t::advice_digest_mismatch,
                "advice or nested digest does not match canonical content");
  }

  if (!request.requested_overrides.empty()) {
    add_refusal(result, marshal_refusal_reason_t::unknown_required_field,
                "operator overrides are not accepted by M1 schema");
  }

  if (request.requested_mode == marshal_dispatch_mode_t::unknown ||
      (request.requested_mode == marshal_dispatch_mode_t::execute &&
       !context.allow_execute_mode)) {
    add_refusal(result, marshal_refusal_reason_t::forbidden_dispatch_mode,
                "requested mode is not allowed for M1 dispatch advice");
  }

  if (!request.target_id.empty() && request.target_id != advice.target_id) {
    add_refusal(result, marshal_refusal_reason_t::target_id_mismatch,
                "request target_id differs from advice target_id");
  }

  if (!request.runtime_root.empty() &&
      !same_path_text(request.runtime_root, advice.runtime_root)) {
    add_refusal(result, marshal_refusal_reason_t::advice_runtime_root_mismatch,
                "request runtime_root differs from advice runtime_root");
  }
  if (!context.active_runtime_root.empty() &&
      !same_path_text(context.active_runtime_root, advice.runtime_root)) {
    add_refusal(result, marshal_refusal_reason_t::advice_runtime_root_mismatch,
                "advice runtime_root differs from active runtime_root");
  }
  if (!request.config_path.empty() &&
      !same_path_text(request.config_path, advice.config_path)) {
    add_refusal(result, marshal_refusal_reason_t::stale_active_identity,
                "request config_path differs from advice config_path");
  }
  if (!context.active_config_path.empty() &&
      !same_path_text(context.active_config_path, advice.config_path)) {
    add_refusal(result, marshal_refusal_reason_t::stale_active_identity,
                "advice config_path differs from active config_path");
  }

  if (!context.active_identity.protocol_contract_fingerprint.empty() &&
      context.active_identity.protocol_contract_fingerprint !=
          advice.active_identity.protocol_contract_fingerprint) {
    add_refusal(result, marshal_refusal_reason_t::stale_active_identity,
                "protocol contract fingerprint differs from active identity");
  }
  if (!context.active_identity.graph_order_fingerprint.empty() &&
      context.active_identity.graph_order_fingerprint !=
          advice.active_identity.graph_order_fingerprint) {
    add_refusal(result, marshal_refusal_reason_t::stale_active_identity,
                "graph order fingerprint differs from active identity");
  }
  if (!context.active_identity.target_spec_fingerprint.empty() &&
      context.active_identity.target_spec_fingerprint !=
          advice.active_identity.target_spec_fingerprint) {
    add_refusal(result, marshal_refusal_reason_t::stale_target_spec,
                "target spec fingerprint differs from active target DSL");
  }
  if (!context.active_identity.split_policy_fingerprint.empty() &&
      context.active_identity.split_policy_fingerprint !=
          advice.active_identity.split_policy_fingerprint) {
    add_refusal(result, marshal_refusal_reason_t::stale_split_policy,
                "split policy fingerprint differs from active split DSL");
  }

  if (!advice.plan_basis.present || !advice.plan_basis.available) {
    add_refusal(result, marshal_refusal_reason_t::missing_plan_basis,
                "plan_basis is missing or unavailable");
  }

  if (advice.suggested_wave.empty()) {
    add_refusal(result, marshal_refusal_reason_t::missing_suggested_wave,
                "suggested_wave is missing target, mode, or source_range");
  }

  if (advice.target_status == "satisfied") {
    add_refusal(result, marshal_refusal_reason_t::target_already_satisfied,
                "satisfied targets are not dispatchable");
  } else if (!detail::dispatchable_target_status(advice.target_status)) {
    add_refusal(result,
                marshal_refusal_reason_t::target_status_not_dispatchable,
                "target_status is not a dispatchable unsatisfied status");
  }

  if (advice.max_waves >= 0 &&
      advice.recommendation_attempt_count >= advice.max_waves) {
    add_refusal(result, marshal_refusal_reason_t::max_waves_exhausted,
                "recommendation attempt count has reached max_waves");
  }

  if (!context.supported_wave_targets.empty() &&
      context.supported_wave_targets.find(advice.suggested_wave.target) ==
          context.supported_wave_targets.end()) {
    add_refusal(result, marshal_refusal_reason_t::unsupported_wave_target,
                "suggested_wave target is not supported by Marshal M1");
  }

  if (!context.supported_source_ranges.empty() &&
      context.supported_source_ranges.find(
          advice.suggested_wave.source_range) ==
          context.supported_source_ranges.end()) {
    add_refusal(result, marshal_refusal_reason_t::unsupported_source_range,
                "suggested_wave source_range is not supported by Marshal M1");
  }

  const bool wave_has_both_bounds =
      advice.suggested_wave.anchor_index_begin.has_value() &&
      advice.suggested_wave.anchor_index_end.has_value();
  const bool wave_has_both_key_bounds =
      advice.suggested_wave.source_key_begin.has_value() &&
      advice.suggested_wave.source_key_end.has_value();
  if (advice.suggested_wave.source_range == "anchor_index") {
    if (!wave_has_both_bounds || *advice.suggested_wave.anchor_index_begin >=
                                     *advice.suggested_wave.anchor_index_end) {
      add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                  "anchor_index suggested_wave requires begin < end");
    }
    if (wave_has_both_key_bounds) {
      add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                  "anchor_index suggested_wave must not carry source-key "
                  "bounds");
    }
  } else if (advice.suggested_wave.source_range == "source_key") {
    if (!wave_has_both_key_bounds ||
        *advice.suggested_wave.source_key_begin >=
            *advice.suggested_wave.source_key_end) {
      add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                  "source_key suggested_wave requires begin < end");
    }
    if (wave_has_both_bounds) {
      add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                  "source_key suggested_wave must not carry anchor-index "
                  "bounds");
    }
  }

  const bool plan_has_both_bounds =
      advice.plan_basis.target_anchor_index_begin.has_value() &&
      advice.plan_basis.target_anchor_index_end.has_value();
  const bool plan_has_both_key_bounds =
      advice.plan_basis.target_source_key_begin.has_value() &&
      advice.plan_basis.target_source_key_end.has_value();
  if (wave_has_both_bounds && plan_has_both_bounds &&
      (*advice.suggested_wave.anchor_index_begin !=
           *advice.plan_basis.target_anchor_index_begin ||
       *advice.suggested_wave.anchor_index_end !=
           *advice.plan_basis.target_anchor_index_end)) {
    add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                "suggested_wave range differs from plan_basis target range");
  }
  if (wave_has_both_key_bounds && plan_has_both_key_bounds &&
      (*advice.suggested_wave.source_key_begin !=
           *advice.plan_basis.target_source_key_begin ||
       *advice.suggested_wave.source_key_end !=
           *advice.plan_basis.target_source_key_end)) {
    add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                "suggested_wave source-key range differs from plan_basis "
                "target range");
  }

  if (!advice.plan_basis.source_range.empty() &&
      advice.plan_basis.source_range != advice.suggested_wave.source_range) {
    add_refusal(result, marshal_refusal_reason_t::malformed_wave_range,
                "suggested_wave source_range differs from plan_basis");
  }

  for (const auto &required_input : advice.required_plan_inputs) {
    const auto found = advice.suggested_wave.plan_inputs.find(required_input);
    if (found == advice.suggested_wave.plan_inputs.end() ||
        found->second.empty() ||
        detail::starts_with(found->second, "latest_satisfying:") ||
        !std::filesystem::path(found->second).is_absolute()) {
      add_refusal(result, marshal_refusal_reason_t::missing_model_state_input,
                  required_input +
                      " is missing, unresolved, or not an absolute path");
    } else if (!detail::path_within_any_allowed_root(
                   found->second, context.allowed_model_state_roots)) {
      add_refusal(result, marshal_refusal_reason_t::missing_model_state_input,
                  required_input + " is outside allowed model-state roots");
    }
  }

  result.dispatchable = result.refusal_reasons.empty();
  return result;
}

} // namespace cuwacunu::kikijyeba::marshal
