// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "kikijyeba/marshal/dispatch_receipt.h"

namespace cuwacunu::kikijyeba::marshal {

inline constexpr const char *k_marshal_target_driver_ledger_schema_v1 =
    "kikijyeba.marshal.target_driver_ledger.v1";

enum class marshal_target_drive_mode_t {
  unknown,
  one_step,
  budgeted,
};

[[nodiscard]] inline const char *to_string(marshal_target_drive_mode_t mode) {
  switch (mode) {
  case marshal_target_drive_mode_t::one_step:
    return "one_step";
  case marshal_target_drive_mode_t::budgeted:
    return "budgeted";
  case marshal_target_drive_mode_t::unknown:
  default:
    return "unknown";
  }
}

struct marshal_target_driver_policy_t {
  std::int64_t max_waves{1};
  std::int64_t max_wall_clock_seconds{0};
  bool allow_execute{false};
  bool allow_train_execute{false};
  bool require_runtime_job_completion{true};
  bool require_post_wave_lattice_satisfied_check{true};
  std::string stop_on_warning_severity{};
  bool stop_on_lattice_warning{false};
  bool stop_on_runtime_warning{false};
  std::int64_t no_progress_window{1};
};

[[nodiscard]] inline std::string canonical_target_driver_policy_text(
    const marshal_target_driver_policy_t &policy) {
  std::ostringstream out;
  detail::append_kv(out, "max_waves", std::to_string(policy.max_waves));
  detail::append_kv(out, "max_wall_clock_seconds",
                    std::to_string(policy.max_wall_clock_seconds));
  detail::append_kv(out, "allow_execute",
                    detail::bool_text(policy.allow_execute));
  detail::append_kv(out, "allow_train_execute",
                    detail::bool_text(policy.allow_train_execute));
  detail::append_kv(out, "require_runtime_job_completion",
                    detail::bool_text(policy.require_runtime_job_completion));
  detail::append_kv(
      out, "require_post_wave_lattice_satisfied_check",
      detail::bool_text(policy.require_post_wave_lattice_satisfied_check));
  detail::append_kv(out, "stop_on_warning_severity",
                    policy.stop_on_warning_severity);
  detail::append_kv(out, "stop_on_lattice_warning",
                    detail::bool_text(policy.stop_on_lattice_warning));
  detail::append_kv(out, "stop_on_runtime_warning",
                    detail::bool_text(policy.stop_on_runtime_warning));
  detail::append_kv(out, "no_progress_window",
                    std::to_string(policy.no_progress_window));
  return out.str();
}

[[nodiscard]] inline std::string
target_driver_policy_digest(const marshal_target_driver_policy_t &policy) {
  return marshal_digest_for_text("kikijyeba.marshal.target_driver_policy.v1",
                                 canonical_target_driver_policy_text(policy));
}

struct marshal_target_driver_iteration_t {
  std::int64_t iteration_index{0};
  marshal_active_identity_t active_identity{};
  std::string target_status{};
  std::string dispatch_state{};
  std::string blocker_bucket{};
  std::string next_action{};
  std::string terminal_state{};
  std::string terminal_reason{};
  std::string target_deficit_digest{};
  std::string suggested_wave_digest{};
  std::string plan_input_digest{};
  std::string advice_digest{};
  std::string request_digest{};
  std::string runtime_policy_digest{};
  std::string runtime_wave_digest{};
  std::string runtime_preview_request_digest{};
  std::string runtime_execution_request_digest{};
  std::string runtime_handoff_arguments_digest{};
  std::string runtime_handoff_id{};
  std::string runtime_handoff_digest{};
  std::string runtime_response_digest{};
  std::string dry_run_response_digest{};
  std::string runtime_job_id{};
  std::string runtime_job_state_digest{};
  std::string runtime_job_manifest_digest{};
  std::string runtime_terminal_fact_digest{};
  std::string runtime_checkpoint_io_fact_digest{};
  std::string runtime_terminal_status{};
  std::string post_run_lattice_evaluation_digest{};
  std::string progress_signature{};
  bool dry_run_attempted{false};
  bool dry_run_accepted{false};
  bool execution_attempted{false};
  bool execution_accepted{false};
  bool runtime_job_completion_observed{false};
  bool lattice_target_satisfied_after_wave{false};
  std::vector<std::string> refusal_reasons{};
};

[[nodiscard]] inline std::string canonical_target_driver_iteration_text(
    const marshal_target_driver_iteration_t &iteration) {
  std::ostringstream out;
  detail::append_kv(out, "iteration_index",
                    std::to_string(iteration.iteration_index));
  detail::append_kv(out, "active_identity",
                    canonical_active_identity_text(iteration.active_identity));
  detail::append_kv(out, "target_status", iteration.target_status);
  detail::append_kv(out, "dispatch_state", iteration.dispatch_state);
  detail::append_kv(out, "blocker_bucket", iteration.blocker_bucket);
  detail::append_kv(out, "next_action", iteration.next_action);
  detail::append_kv(out, "terminal_state", iteration.terminal_state);
  detail::append_kv(out, "terminal_reason", iteration.terminal_reason);
  detail::append_kv(out, "target_deficit_digest",
                    iteration.target_deficit_digest);
  detail::append_kv(out, "suggested_wave_digest",
                    iteration.suggested_wave_digest);
  detail::append_kv(out, "plan_input_digest", iteration.plan_input_digest);
  detail::append_kv(out, "advice_digest", iteration.advice_digest);
  detail::append_kv(out, "request_digest", iteration.request_digest);
  detail::append_kv(out, "runtime_policy_digest",
                    iteration.runtime_policy_digest);
  detail::append_kv(out, "runtime_wave_digest", iteration.runtime_wave_digest);
  detail::append_kv(out, "runtime_preview_request_digest",
                    iteration.runtime_preview_request_digest);
  detail::append_kv(out, "runtime_execution_request_digest",
                    iteration.runtime_execution_request_digest);
  detail::append_kv(out, "runtime_handoff_arguments_digest",
                    iteration.runtime_handoff_arguments_digest);
  detail::append_kv(out, "runtime_handoff_id", iteration.runtime_handoff_id);
  detail::append_kv(out, "runtime_handoff_digest",
                    iteration.runtime_handoff_digest);
  detail::append_kv(out, "runtime_response_digest",
                    iteration.runtime_response_digest);
  detail::append_kv(out, "dry_run_response_digest",
                    iteration.dry_run_response_digest);
  detail::append_kv(out, "runtime_job_id", iteration.runtime_job_id);
  detail::append_kv(out, "runtime_job_state_digest",
                    iteration.runtime_job_state_digest);
  detail::append_kv(out, "runtime_job_manifest_digest",
                    iteration.runtime_job_manifest_digest);
  detail::append_kv(out, "runtime_terminal_fact_digest",
                    iteration.runtime_terminal_fact_digest);
  detail::append_kv(out, "runtime_checkpoint_io_fact_digest",
                    iteration.runtime_checkpoint_io_fact_digest);
  detail::append_kv(out, "runtime_terminal_status",
                    iteration.runtime_terminal_status);
  detail::append_kv(out, "post_run_lattice_evaluation_digest",
                    iteration.post_run_lattice_evaluation_digest);
  detail::append_kv(out, "progress_signature", iteration.progress_signature);
  detail::append_kv(out, "dry_run_attempted",
                    detail::bool_text(iteration.dry_run_attempted));
  detail::append_kv(out, "dry_run_accepted",
                    detail::bool_text(iteration.dry_run_accepted));
  detail::append_kv(out, "execution_attempted",
                    detail::bool_text(iteration.execution_attempted));
  detail::append_kv(out, "execution_accepted",
                    detail::bool_text(iteration.execution_accepted));
  detail::append_kv(
      out, "runtime_job_completion_observed",
      detail::bool_text(iteration.runtime_job_completion_observed));
  detail::append_kv(
      out, "lattice_target_satisfied_after_wave",
      detail::bool_text(iteration.lattice_target_satisfied_after_wave));
  detail::append_string_vector(out, "refusal_reasons",
                               iteration.refusal_reasons);
  return out.str();
}

[[nodiscard]] inline std::string target_driver_iteration_digest(
    const marshal_target_driver_iteration_t &iteration) {
  return marshal_digest_for_text(
      "kikijyeba.marshal.target_driver_iteration.v1",
      canonical_target_driver_iteration_text(iteration));
}

struct marshal_target_driver_ledger_t {
  std::string schema_version{k_marshal_target_driver_ledger_schema_v1};
  std::string target_driver_run_id{};
  std::string target_driver_run_key{};
  std::string ledger_created_at_utc{};
  std::string ledger_nonce{};
  std::string target_id{};
  marshal_active_identity_t active_identity{};
  marshal_target_drive_mode_t drive_mode{marshal_target_drive_mode_t::one_step};
  marshal_dispatch_mode_t requested_mode{marshal_dispatch_mode_t::dry_run};
  marshal_target_driver_policy_t driver_policy{};
  std::string driver_policy_digest{};
  std::string resumed_from_run_id{};
  std::int64_t resumed_iteration_count{0};
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
  std::string terminal_state{};
  std::string terminal_reason{};
  std::vector<marshal_target_driver_iteration_t> iterations{};
  std::string non_authority_statement{
      k_marshal_dispatch_non_authority_statement};
};

[[nodiscard]] inline std::string canonical_target_driver_ledger_text(
    const marshal_target_driver_ledger_t &ledger) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", ledger.schema_version);
  detail::append_kv(out, "target_driver_run_id", ledger.target_driver_run_id);
  detail::append_kv(out, "target_driver_run_key", ledger.target_driver_run_key);
  detail::append_kv(out, "ledger_created_at_utc", ledger.ledger_created_at_utc);
  detail::append_kv(out, "ledger_nonce", ledger.ledger_nonce);
  detail::append_kv(out, "target_id", ledger.target_id);
  detail::append_kv(out, "active_identity",
                    canonical_active_identity_text(ledger.active_identity));
  detail::append_kv(out, "drive_mode", to_string(ledger.drive_mode));
  detail::append_kv(out, "requested_mode", to_string(ledger.requested_mode));
  detail::append_kv(out, "driver_policy",
                    canonical_target_driver_policy_text(ledger.driver_policy));
  detail::append_kv(out, "driver_policy_digest", ledger.driver_policy_digest);
  detail::append_kv(out, "resumed_from_run_id", ledger.resumed_from_run_id);
  detail::append_kv(out, "resumed_iteration_count",
                    std::to_string(ledger.resumed_iteration_count));
  detail::append_kv(out, "iteration_count",
                    std::to_string(ledger.iteration_count));
  detail::append_kv(out, "runtime_handoff_attempt_count",
                    std::to_string(ledger.runtime_handoff_attempt_count));
  detail::append_kv(out, "execution_attempt_count",
                    std::to_string(ledger.execution_attempt_count));
  detail::append_kv(out, "last_target_deficit_digest",
                    ledger.last_target_deficit_digest);
  detail::append_kv(out, "last_suggested_wave_digest",
                    ledger.last_suggested_wave_digest);
  detail::append_kv(out, "last_progress_signature",
                    ledger.last_progress_signature);
  detail::append_kv(out, "last_runtime_job_id", ledger.last_runtime_job_id);
  detail::append_kv(out, "last_runtime_job_state_digest",
                    ledger.last_runtime_job_state_digest);
  detail::append_kv(out, "last_runtime_job_manifest_digest",
                    ledger.last_runtime_job_manifest_digest);
  detail::append_kv(out, "last_runtime_terminal_fact_digest",
                    ledger.last_runtime_terminal_fact_digest);
  detail::append_kv(out, "last_runtime_checkpoint_io_fact_digest",
                    ledger.last_runtime_checkpoint_io_fact_digest);
  detail::append_kv(out, "last_runtime_handoff_id",
                    ledger.last_runtime_handoff_id);
  detail::append_kv(out, "last_runtime_handoff_digest",
                    ledger.last_runtime_handoff_digest);
  detail::append_kv(out, "last_runtime_policy_digest",
                    ledger.last_runtime_policy_digest);
  detail::append_kv(out, "terminal_state", ledger.terminal_state);
  detail::append_kv(out, "terminal_reason", ledger.terminal_reason);
  detail::append_kv(out, "iterations.count",
                    std::to_string(ledger.iterations.size()));
  for (std::size_t i = 0; i < ledger.iterations.size(); ++i) {
    detail::append_kv(
        out, "iterations." + std::to_string(i),
        canonical_target_driver_iteration_text(ledger.iterations[i]));
  }
  detail::append_kv(out, "non_authority_statement",
                    ledger.non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
target_driver_ledger_digest(const marshal_target_driver_ledger_t &ledger) {
  return marshal_digest_for_text("kikijyeba.marshal.target_driver_ledger.v1",
                                 canonical_target_driver_ledger_text(ledger));
}

[[nodiscard]] inline std::string canonical_target_driver_run_key_text(
    const marshal_target_driver_ledger_t &ledger) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", ledger.schema_version);
  detail::append_kv(out, "target_id", ledger.target_id);
  detail::append_kv(out, "active_identity",
                    canonical_active_identity_text(ledger.active_identity));
  detail::append_kv(out, "drive_mode", to_string(ledger.drive_mode));
  detail::append_kv(out, "requested_mode", to_string(ledger.requested_mode));
  detail::append_kv(out, "driver_policy_digest", ledger.driver_policy_digest);
  detail::append_kv(out, "non_authority_statement",
                    ledger.non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
target_driver_run_key_digest(const marshal_target_driver_ledger_t &ledger) {
  return marshal_digest_for_text("kikijyeba.marshal.target_driver_run_key.v1",
                                 canonical_target_driver_run_key_text(ledger));
}

[[nodiscard]] inline std::string canonical_target_driver_run_id_text(
    const marshal_target_driver_ledger_t &ledger) {
  std::ostringstream out;
  detail::append_kv(out, "schema_version", ledger.schema_version);
  detail::append_kv(out, "target_driver_run_key", ledger.target_driver_run_key);
  detail::append_kv(out, "ledger_created_at_utc", ledger.ledger_created_at_utc);
  detail::append_kv(out, "ledger_nonce", ledger.ledger_nonce);
  detail::append_kv(out, "resumed_from_run_id", ledger.resumed_from_run_id);
  detail::append_kv(out, "non_authority_statement",
                    ledger.non_authority_statement);
  return out.str();
}

[[nodiscard]] inline std::string
target_driver_run_id_digest(const marshal_target_driver_ledger_t &ledger) {
  return marshal_digest_for_text("kikijyeba.marshal.target_driver_run_id.v1",
                                 canonical_target_driver_run_id_text(ledger));
}

inline void
finalize_target_driver_run_id(marshal_target_driver_ledger_t *ledger) {
  if (!ledger) {
    return;
  }
  if (ledger->target_driver_run_key.empty()) {
    ledger->target_driver_run_key =
        "marshal_target_driver_key_" + target_driver_run_key_digest(*ledger);
  }
  if (ledger->target_driver_run_id.empty()) {
    ledger->target_driver_run_id =
        "marshal_target_driver_run_" + target_driver_run_id_digest(*ledger);
  }
}

} // namespace cuwacunu::kikijyeba::marshal
