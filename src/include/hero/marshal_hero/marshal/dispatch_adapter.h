// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "hero/marshal_hero/marshal/dispatch_advice.h"

namespace cuwacunu::hero::marshal {

struct marshal_runtime_policy_snapshot_t {
  bool runtime_hero_available{true};
  bool runtime_exec_exists{true};
  bool runtime_exec_executable{true};
  bool default_dry_run{true};
  bool allow_execute{false};
  bool allow_train_execute{false};
  std::string runtime_root{};
};

struct marshal_runtime_wave_snapshot_t {
  bool available{false};
  std::string wave_id{};
  std::string target_component_family_id{};
  std::string mode{};
  std::string source_range{"anchor_index"};
  std::string source_order{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
  std::string job_kind{};
  bool train_target{false};
  std::map<std::string, std::string> model_state_inputs{};
};

struct marshal_field_derivation_t {
  std::string field{};
  std::string source{};
};

struct marshal_runtime_dry_run_request_t {
  std::string target_id{};
  std::string config_path{};
  std::string runtime_root{};
  bool dry_run{true};
  bool force_rebuild_cache{false};
  std::string wave_target{};
  std::string wave_mode{};
  std::string source_range{};
  std::string source_order{};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
  std::map<std::string, std::string> model_state_inputs{};
  std::map<std::string, std::string> lattice_certificate_refs{};
  std::string target_driver_run_id{};
};

struct marshal_dispatch_decision_t {
  bool accepted{false};
  bool runtime_handoff_available{false};
  std::string expected_runtime_policy_digest{};
  std::string expected_runtime_wave_digest{};
  marshal_dispatch_validation_result_t advice_validation{};
  marshal_runtime_dry_run_request_t runtime_request{};
  std::vector<marshal_field_derivation_t> field_derivations{};
  std::vector<marshal_refusal_reason_t> refusal_reasons{};
  std::vector<std::string> messages{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};

  [[nodiscard]] bool has(marshal_refusal_reason_t reason) const {
    return std::find(refusal_reasons.begin(), refusal_reasons.end(), reason) !=
           refusal_reasons.end();
  }
};

inline void add_refusal(marshal_dispatch_decision_t &decision,
                        marshal_refusal_reason_t reason,
                        const std::string &message) {
  if (!decision.has(reason)) {
    decision.refusal_reasons.push_back(reason);
  }
  decision.messages.push_back(std::string(to_string(reason)) + ": " + message);
}

inline void add_derivation(marshal_dispatch_decision_t &decision,
                           std::string field, std::string source) {
  decision.field_derivations.push_back(
      marshal_field_derivation_t{std::move(field), std::move(source)});
}

[[nodiscard]] inline std::string canonical_runtime_dry_run_request_text(
    const marshal_runtime_dry_run_request_t &request) {
  std::ostringstream out;
  detail::append_kv(out, "target_id", request.target_id);
  detail::append_kv(out, "config_path",
                    detail::normalize_path_text(request.config_path));
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(request.runtime_root));
  detail::append_kv(out, "dry_run", detail::bool_text(request.dry_run));
  detail::append_kv(out, "force_rebuild_cache",
                    detail::bool_text(request.force_rebuild_cache));
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
  detail::append_string_map(out, "model_state_inputs",
                            request.model_state_inputs);
  detail::append_string_map(out, "lattice_certificate_refs",
                            request.lattice_certificate_refs);
  detail::append_kv(out, "target_driver_run_id", request.target_driver_run_id);
  detail::append_kv(out, "non_authority_statement",
                    k_marshal_dispatch_non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string runtime_dry_run_request_digest(
    const marshal_runtime_dry_run_request_t &request) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.runtime_dry_run_request.v1",
      canonical_runtime_dry_run_request_text(request));
}

[[nodiscard]] inline marshal_runtime_dry_run_request_t
runtime_execution_request_from_preview(
    const marshal_runtime_dry_run_request_t &request) {
  auto execution = request;
  execution.dry_run = false;
  return execution;
}

[[nodiscard]] inline std::string runtime_execution_request_digest(
    const marshal_runtime_dry_run_request_t &request) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.runtime_execution_request.v1",
      canonical_runtime_dry_run_request_text(
          runtime_execution_request_from_preview(request)));
}

[[nodiscard]] inline std::string canonical_runtime_policy_snapshot_text(
    const marshal_runtime_policy_snapshot_t &policy) {
  std::ostringstream out;
  detail::append_kv(out, "runtime_hero_available",
                    detail::bool_text(policy.runtime_hero_available));
  detail::append_kv(out, "runtime_exec_exists",
                    detail::bool_text(policy.runtime_exec_exists));
  detail::append_kv(out, "runtime_exec_executable",
                    detail::bool_text(policy.runtime_exec_executable));
  detail::append_kv(out, "default_dry_run",
                    detail::bool_text(policy.default_dry_run));
  detail::append_kv(out, "allow_execute",
                    detail::bool_text(policy.allow_execute));
  detail::append_kv(out, "allow_train_execute",
                    detail::bool_text(policy.allow_train_execute));
  detail::append_kv(out, "runtime_root",
                    detail::normalize_path_text(policy.runtime_root));
  return out.str();
}

[[nodiscard]] inline std::string runtime_policy_snapshot_digest(
    const marshal_runtime_policy_snapshot_t &policy) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.runtime_policy_snapshot.v1",
      canonical_runtime_policy_snapshot_text(policy));
}

[[nodiscard]] inline std::string canonical_runtime_wave_snapshot_text(
    const marshal_runtime_wave_snapshot_t &wave) {
  std::ostringstream out;
  detail::append_kv(out, "available", detail::bool_text(wave.available));
  detail::append_kv(out, "wave_id", wave.wave_id);
  detail::append_kv(out, "target_component_family_id",
                    wave.target_component_family_id);
  detail::append_kv(out, "mode", wave.mode);
  detail::append_kv(out, "source_range", wave.source_range);
  detail::append_kv(out, "source_order", wave.source_order);
  detail::append_kv(out, "anchor_index_begin",
                    detail::optional_size_text(wave.anchor_index_begin));
  detail::append_kv(out, "anchor_index_end",
                    detail::optional_size_text(wave.anchor_index_end));
  detail::append_kv(out, "source_key_begin",
                    detail::optional_i64_text(wave.source_key_begin));
  detail::append_kv(out, "source_key_end",
                    detail::optional_i64_text(wave.source_key_end));
  detail::append_kv(out, "job_kind", wave.job_kind);
  detail::append_kv(out, "train_target", detail::bool_text(wave.train_target));
  detail::append_string_map(out, "model_state_inputs", wave.model_state_inputs);
  return out.str();
}

[[nodiscard]] inline std::string
runtime_wave_snapshot_digest(const marshal_runtime_wave_snapshot_t &wave) {
  return marshal_digest_for_text("kikijyeba.marshal.runtime_wave_snapshot.v1",
                                 canonical_runtime_wave_snapshot_text(wave));
}

[[nodiscard]] inline bool
wave_matches_advice_range(const marshal_runtime_wave_snapshot_t &wave,
                          const marshal_suggested_wave_t &suggested_wave) {
  const bool launch_overlay_profile =
      wave.source_range == "all" && !wave.anchor_index_begin.has_value() &&
      !wave.anchor_index_end.has_value() &&
      !wave.source_key_begin.has_value() && !wave.source_key_end.has_value();
  const bool advised_concrete_overlay =
      (suggested_wave.source_range == "anchor_index" &&
       suggested_wave.anchor_index_begin.has_value() &&
       suggested_wave.anchor_index_end.has_value() &&
       !suggested_wave.source_key_begin.has_value() &&
       !suggested_wave.source_key_end.has_value()) ||
      (suggested_wave.source_range == "source_key" &&
       suggested_wave.source_key_begin.has_value() &&
       suggested_wave.source_key_end.has_value() &&
       !suggested_wave.anchor_index_begin.has_value() &&
       !suggested_wave.anchor_index_end.has_value());
  if (launch_overlay_profile && advised_concrete_overlay) {
    return true;
  }
  return wave.source_range == suggested_wave.source_range &&
         wave.anchor_index_begin == suggested_wave.anchor_index_begin &&
         wave.anchor_index_end == suggested_wave.anchor_index_end &&
         wave.source_key_begin == suggested_wave.source_key_begin &&
         wave.source_key_end == suggested_wave.source_key_end;
}

[[nodiscard]] inline marshal_dispatch_decision_t
build_runtime_dry_run_dispatch_preview(
    const marshal_dispatch_advice_t &advice,
    const marshal_dispatch_request_t &request,
    const marshal_dispatch_validation_context_t &context,
    const marshal_runtime_policy_snapshot_t &policy,
    const marshal_runtime_wave_snapshot_t &active_wave) {
  marshal_dispatch_decision_t decision{};
  decision.advice_validation =
      validate_dispatch_advice(advice, request, context);
  decision.expected_runtime_policy_digest =
      runtime_policy_snapshot_digest(policy);
  decision.expected_runtime_wave_digest =
      runtime_wave_snapshot_digest(active_wave);
  decision.runtime_request.target_id = advice.target_id;
  decision.runtime_request.config_path = advice.config_path;
  decision.runtime_request.runtime_root = advice.runtime_root;
  decision.runtime_request.dry_run = true;
  decision.runtime_request.force_rebuild_cache = false;
  decision.runtime_request.wave_target = advice.suggested_wave.target;
  decision.runtime_request.wave_mode = advice.suggested_wave.mode;
  decision.runtime_request.source_range = advice.suggested_wave.source_range;
  decision.runtime_request.source_order = active_wave.source_order;
  decision.runtime_request.anchor_index_begin =
      advice.suggested_wave.anchor_index_begin;
  decision.runtime_request.anchor_index_end =
      advice.suggested_wave.anchor_index_end;
  decision.runtime_request.source_key_begin =
      advice.suggested_wave.source_key_begin;
  decision.runtime_request.source_key_end =
      advice.suggested_wave.source_key_end;
  decision.runtime_request.model_state_inputs =
      advice.suggested_wave.plan_inputs;
  decision.runtime_request.lattice_certificate_refs =
      request.lattice_certificate_refs;
  decision.runtime_request.target_driver_run_id = request.target_driver_run_id;

  add_derivation(decision, "config_path", "advice.config_path");
  add_derivation(decision, "runtime_root", "advice.runtime_root");
  add_derivation(decision, "dry_run", "Marshal M2 forced dry_run=true");
  add_derivation(decision, "force_rebuild_cache",
                 "Marshal M2 default force_rebuild_cache=false");
  add_derivation(decision, "wave_target", "advice.suggested_wave.target");
  add_derivation(decision, "wave_mode", "advice.suggested_wave.mode");
  add_derivation(decision, "source_range",
                 "advice.suggested_wave.source_range");
  add_derivation(decision, "anchor_index_begin",
                 "advice.suggested_wave.anchor_index_begin");
  add_derivation(decision, "anchor_index_end",
                 "advice.suggested_wave.anchor_index_end");
  add_derivation(decision, "source_key_begin",
                 "advice.suggested_wave.source_key_begin");
  add_derivation(decision, "source_key_end",
                 "advice.suggested_wave.source_key_end");
  add_derivation(decision, "model_state_inputs",
                 "advice.suggested_wave.plan_inputs");

  for (const auto &reason : decision.advice_validation.refusal_reasons) {
    add_refusal(decision, reason,
                "M1 advice validation refused the dispatch preview");
  }
  for (const auto &message : decision.advice_validation.messages) {
    decision.messages.push_back("advice_validation: " + message);
  }

  if (!policy.runtime_hero_available || !policy.runtime_exec_exists ||
      !policy.runtime_exec_executable) {
    add_refusal(decision, marshal_refusal_reason_t::runtime_policy_refused,
                "Runtime Hero or runtime executable is unavailable");
  }
  if (!policy.runtime_root.empty() &&
      !same_path_text(policy.runtime_root, advice.runtime_root)) {
    add_refusal(decision, marshal_refusal_reason_t::runtime_policy_refused,
                "Runtime Hero policy root differs from advice runtime_root");
  }
  if (!policy.default_dry_run) {
    add_refusal(decision, marshal_refusal_reason_t::runtime_policy_refused,
                "Runtime Hero default_dry_run is false");
  }

  if (!active_wave.available) {
    add_refusal(decision, marshal_refusal_reason_t::runtime_handoff_unavailable,
                "Runtime Hero active wave snapshot is unavailable");
  } else {
    if (active_wave.target_component_family_id !=
            advice.suggested_wave.target ||
        active_wave.mode != advice.suggested_wave.mode) {
      add_refusal(
          decision, marshal_refusal_reason_t::runtime_wave_mismatch,
          "Runtime Hero active wave target or mode differs from advice");
    }
    if (!wave_matches_advice_range(active_wave, advice.suggested_wave)) {
      add_refusal(decision, marshal_refusal_reason_t::runtime_wave_mismatch,
                  "Runtime Hero active wave range differs from advice");
    }
    for (const auto &required_input : advice.required_plan_inputs) {
      const auto expected =
          advice.suggested_wave.plan_inputs.find(required_input);
      const auto actual = active_wave.model_state_inputs.find(required_input);
      if (expected == advice.suggested_wave.plan_inputs.end() ||
          expected->second.empty()) {
        add_refusal(decision,
                    marshal_refusal_reason_t::missing_model_state_input,
                    required_input + " is missing from dispatch advice");
      } else if (actual != active_wave.model_state_inputs.end() &&
                 !actual->second.empty() &&
                 detail::normalize_path_text(expected->second) !=
                     detail::normalize_path_text(actual->second)) {
        add_refusal(
            decision, marshal_refusal_reason_t::checkpoint_input_mismatch,
            required_input + " differs between advice and Runtime Hero wave");
      }
    }
  }

  decision.accepted = decision.refusal_reasons.empty();
  decision.runtime_handoff_available =
      decision.accepted && active_wave.available;
  return decision;
}

} // namespace cuwacunu::hero::marshal
