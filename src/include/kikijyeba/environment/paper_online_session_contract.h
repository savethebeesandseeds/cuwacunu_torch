// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu::kikijyeba::environment {

inline constexpr const char *kPaperOnlineSessionContractV1 =
    "paper_online_session_contract.v1";
inline constexpr const char *kPaperOnlineReadinessContractV1 =
    "paper_online_readiness_contract.v1";
inline constexpr const char *kPaperOnlineReadinessTargetReadyV1 =
    "paper_online_readiness_contract_ready";
inline constexpr const char *kPaperOnlineReadinessFactSchemaV1 =
    "kikijyeba.lattice.paper_online_readiness.v1";
inline constexpr const char *kPaperOnlineEnvironmentContractV1 =
    "kikijyeba.environment.paper_online.v1";
inline constexpr const char *kPaperOnlineSessionStateSchemaV1 =
    "kikijyeba.paper_online.session_state.v1";
inline constexpr const char *kPaperOnlineSessionManifestSchemaV1 =
    "kikijyeba.paper_online.session_manifest.v1";
inline constexpr const char *kPaperOnlineSessionEventLogSchemaV1 =
    "kikijyeba.paper_online.session_events.v1";
inline constexpr const char *kPaperOnlineMarketEventLogSchemaV1 =
    "kikijyeba.paper_online.market_events.v1";
inline constexpr const char *kPaperOnlineActionIntentLogSchemaV1 =
    "kikijyeba.paper_online.action_intents.v1";
inline constexpr const char *kPaperOnlineExecutionIntentLogSchemaV1 =
    "kikijyeba.paper_online.execution_intents.v1";
inline constexpr const char *kPaperOnlinePaperLedgerSchemaV1 =
    "kikijyeba.paper_online.paper_ledger.v1";
inline constexpr const char *kPaperOnlineRewardReportLogSchemaV1 =
    "kikijyeba.paper_online.reward_reports.v1";
inline constexpr const char *kPaperOnlineTargetWeightsActionSchemaV1 =
    "kikijyeba.environment.action.target_node_weights.v1";
inline constexpr const char *kPaperOnlineRewardContractV1 =
    "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
    "drawdown.v1";
inline constexpr const char *kPaperOnlineDirectEdgeUniversePolicyV1 =
    "direct_graph_node_pair_universe_required.v1";
inline constexpr const char *kPaperOnlineMissingDirectPairPolicyV1 =
    "skip_pair_warn";
inline constexpr const char *kPaperOnlineSyntheticExecutionMarketPolicyV1 =
    "synthetic_execution_markets_forbidden.v1";
inline constexpr const char *kPaperOnlinePersistentLedgerRecoveryPolicyV1 =
    "recover_persistent_paper_ledger_before_session.v1";
inline constexpr const char *kPaperOnlineLedgerStoragePolicyV1 =
    "durable_paper_ledger_artifacts.v1";
inline constexpr const char *kPaperOnlineSessionLifecyclePolicyV1 =
    "operator_start_stop_required.v1";
inline constexpr const char *kPaperOnlineIdempotencyPolicyV1 =
    "execution_intent_idempotency.v1";
inline constexpr const char *kPaperOnlineExecutionIntentIdPolicyV1 =
    "stable_intent_id_from_target_digest.v1";
inline constexpr const char *kPaperOnlineDuplicateActionPolicyV1 =
    "reject_duplicate_action_digest.v1";
inline constexpr const char *kPaperOnlineDuplicateExecutionIntentPolicyV1 =
    "reject_duplicate_execution_intent_id.v1";
inline constexpr const char *kPaperOnlineClockPolicyV1 =
    "monotonic_runtime_clock_required.v1";
inline constexpr const char *kPaperOnlineTimestampPolicyV1 =
    "exchange_event_time_then_runtime_receive.v1";
inline constexpr const char *kPaperOnlineMarketDataStalenessPolicyV1 =
    "max_receive_age_before_action.v1";
inline constexpr const char *kPaperOnlineRewardReportArtifactPolicyV1 =
    "durable_reward_and_report_artifacts.v1";
inline constexpr const char *kPaperOnlineOperatorAbortPolicyV1 =
    "operator_abort_closes_session.v1";
inline constexpr const char *kPaperOnlineKillSwitchPolicyV1 =
    "kill_switch_no_new_intents.v1";

enum class paper_online_session_state_t {
  initialized,
  admitted,
  ledger_recovered,
  streaming,
  action_recorded,
  paper_executed,
  reward_reported,
  stopping,
  stopped,
  aborted,
  kill_switch_triggered,
};

[[nodiscard]] inline const char *
paper_online_session_state_name(paper_online_session_state_t state) {
  switch (state) {
  case paper_online_session_state_t::initialized:
    return "initialized";
  case paper_online_session_state_t::admitted:
    return "admitted";
  case paper_online_session_state_t::ledger_recovered:
    return "ledger_recovered";
  case paper_online_session_state_t::streaming:
    return "streaming";
  case paper_online_session_state_t::action_recorded:
    return "action_recorded";
  case paper_online_session_state_t::paper_executed:
    return "paper_executed";
  case paper_online_session_state_t::reward_reported:
    return "reward_reported";
  case paper_online_session_state_t::stopping:
    return "stopping";
  case paper_online_session_state_t::stopped:
    return "stopped";
  case paper_online_session_state_t::aborted:
    return "aborted";
  case paper_online_session_state_t::kill_switch_triggered:
    return "kill_switch_triggered";
  }
  return "unknown";
}

[[nodiscard]] inline bool
paper_online_session_terminal_state(paper_online_session_state_t state) {
  return state == paper_online_session_state_t::stopped ||
         state == paper_online_session_state_t::aborted;
}

[[nodiscard]] inline bool
paper_online_session_transition_allowed(paper_online_session_state_t from,
                                        paper_online_session_state_t to) {
  if (paper_online_session_terminal_state(from)) {
    return false;
  }
  if (from == paper_online_session_state_t::kill_switch_triggered) {
    return to == paper_online_session_state_t::stopped;
  }
  if (to == paper_online_session_state_t::aborted ||
      to == paper_online_session_state_t::kill_switch_triggered) {
    return true;
  }
  switch (from) {
  case paper_online_session_state_t::initialized:
    return to == paper_online_session_state_t::admitted;
  case paper_online_session_state_t::admitted:
    return to == paper_online_session_state_t::ledger_recovered;
  case paper_online_session_state_t::ledger_recovered:
    return to == paper_online_session_state_t::streaming;
  case paper_online_session_state_t::streaming:
    return to == paper_online_session_state_t::action_recorded ||
           to == paper_online_session_state_t::stopping;
  case paper_online_session_state_t::action_recorded:
    return to == paper_online_session_state_t::paper_executed;
  case paper_online_session_state_t::paper_executed:
    return to == paper_online_session_state_t::reward_reported;
  case paper_online_session_state_t::reward_reported:
    return to == paper_online_session_state_t::streaming ||
           to == paper_online_session_state_t::stopping;
  case paper_online_session_state_t::stopping:
    return to == paper_online_session_state_t::stopped;
  case paper_online_session_state_t::kill_switch_triggered:
    return false;
  case paper_online_session_state_t::stopped:
  case paper_online_session_state_t::aborted:
    return false;
  }
  return false;
}

struct paper_online_session_artifact_slot_t {
  std::string artifact_id{};
  std::string relative_path{};
  std::string schema_id{};
  bool required{true};
};

[[nodiscard]] inline std::vector<paper_online_session_artifact_slot_t>
default_paper_online_session_artifacts() {
  return {
      {"session_manifest", "session.manifest",
       kPaperOnlineSessionManifestSchemaV1, true},
      {"session_state", "session.state", kPaperOnlineSessionStateSchemaV1,
       true},
      {"session_events", "session.events.lls",
       kPaperOnlineSessionEventLogSchemaV1, true},
      {"market_events", "market_events.lls", kPaperOnlineMarketEventLogSchemaV1,
       true},
      {"action_intents", "action_intents.lls",
       kPaperOnlineActionIntentLogSchemaV1, true},
      {"execution_intents", "execution_intents.lls",
       kPaperOnlineExecutionIntentLogSchemaV1, true},
      {"paper_ledger", "paper_ledger.lls", kPaperOnlinePaperLedgerSchemaV1,
       true},
      {"reward_reports", "reward_reports.lls",
       kPaperOnlineRewardReportLogSchemaV1, true},
  };
}

struct paper_online_session_contract_t {
  std::string contract_id{kPaperOnlineSessionContractV1};
  std::string environment_contract_id{kPaperOnlineEnvironmentContractV1};
  std::string readiness_target_id{kPaperOnlineReadinessTargetReadyV1};
  std::string session_state_schema_id{kPaperOnlineSessionStateSchemaV1};
  std::string action_schema_id{kPaperOnlineTargetWeightsActionSchemaV1};
  std::string reward_contract_id{kPaperOnlineRewardContractV1};
  std::string direct_edge_universe_validation_policy_id{
      kPaperOnlineDirectEdgeUniversePolicyV1};
  std::string missing_direct_pair_policy{kPaperOnlineMissingDirectPairPolicyV1};
  std::string synthetic_execution_market_policy_id{
      kPaperOnlineSyntheticExecutionMarketPolicyV1};
  std::string persistent_paper_ledger_recovery_policy_id{
      kPaperOnlinePersistentLedgerRecoveryPolicyV1};
  std::string paper_ledger_storage_policy_id{kPaperOnlineLedgerStoragePolicyV1};
  std::string session_lifecycle_policy_id{kPaperOnlineSessionLifecyclePolicyV1};
  std::string idempotency_policy_id{kPaperOnlineIdempotencyPolicyV1};
  std::string execution_intent_id_policy{kPaperOnlineExecutionIntentIdPolicyV1};
  std::string duplicate_action_policy{kPaperOnlineDuplicateActionPolicyV1};
  std::string duplicate_execution_intent_policy{
      kPaperOnlineDuplicateExecutionIntentPolicyV1};
  std::string clock_policy_id{kPaperOnlineClockPolicyV1};
  std::string timestamp_policy_id{kPaperOnlineTimestampPolicyV1};
  std::string market_data_staleness_policy_id{
      kPaperOnlineMarketDataStalenessPolicyV1};
  std::string reward_report_artifact_policy_id{
      kPaperOnlineRewardReportArtifactPolicyV1};
  std::string operator_abort_policy_id{kPaperOnlineOperatorAbortPolicyV1};
  std::string kill_switch_policy_id{kPaperOnlineKillSwitchPolicyV1};
  std::vector<paper_online_session_artifact_slot_t> durable_artifacts{
      default_paper_online_session_artifacts()};
  bool requires_paper_online_readiness_contract_ready{true};
  bool requires_fresh_readiness_proof{true};
  bool requires_market_stream_intake{true};
  bool requires_cajtucu_paper_execution{true};
  bool requires_persistent_paper_ledger_recovery{true};
  bool requires_idempotent_execution_intents{true};
  bool requires_duplicate_action_rejection{true};
  bool requires_operator_abort{true};
  bool requires_kill_switch{true};
  bool session_runner_implemented{false};
  bool policy_selection_authority{false};
  bool checkpoint_selection_authority{false};
  bool broker_execution_allowed{false};
  bool live_execution_allowed{false};
  bool direct_policy_to_broker_allowed{false};
  bool direct_policy_to_cajtucu_allowed{false};
};

struct paper_online_readiness_evidence_t {
  std::string readiness_target_id{kPaperOnlineReadinessTargetReadyV1};
  std::string readiness_contract_id{kPaperOnlineReadinessContractV1};
  std::string readiness_fact_schema_id{kPaperOnlineReadinessFactSchemaV1};
  std::string readiness_fact_digest{};
  std::string readiness_proof_certificate_digest{};
  std::string readiness_id{};
  std::string accepted_policy_id{};
  std::string accepted_actor_checkpoint_digest{};
  std::string accepted_checkpoint_digest{};
  std::string execution_profile_digest{};
  std::string locked_execution_profile_digest{};
  std::string accounting_numeraire_node_id{};
  std::string paper_online_profile_digest{};
  std::string direct_edge_universe_digest{};
  std::string paper_online_environment_contract_id{
      kPaperOnlineEnvironmentContractV1};
  std::string session_state_schema_id{kPaperOnlineSessionStateSchemaV1};
  std::string reward_contract_id{kPaperOnlineRewardContractV1};
  std::string direct_edge_universe_validation_policy_id{
      kPaperOnlineDirectEdgeUniversePolicyV1};
  std::string missing_direct_pair_policy{kPaperOnlineMissingDirectPairPolicyV1};
  std::string synthetic_execution_market_policy_id{
      kPaperOnlineSyntheticExecutionMarketPolicyV1};
  std::string persistent_paper_ledger_recovery_policy_id{
      kPaperOnlinePersistentLedgerRecoveryPolicyV1};
  std::string paper_ledger_storage_policy_id{kPaperOnlineLedgerStoragePolicyV1};
  std::string session_lifecycle_policy_id{kPaperOnlineSessionLifecyclePolicyV1};
  std::string idempotency_policy_id{kPaperOnlineIdempotencyPolicyV1};
  std::string execution_intent_id_policy{kPaperOnlineExecutionIntentIdPolicyV1};
  std::string duplicate_action_policy{kPaperOnlineDuplicateActionPolicyV1};
  std::string duplicate_execution_intent_policy{
      kPaperOnlineDuplicateExecutionIntentPolicyV1};
  std::string clock_policy_id{kPaperOnlineClockPolicyV1};
  std::string timestamp_policy_id{kPaperOnlineTimestampPolicyV1};
  std::string market_data_staleness_policy_id{
      kPaperOnlineMarketDataStalenessPolicyV1};
  std::string reward_report_artifact_policy_id{
      kPaperOnlineRewardReportArtifactPolicyV1};
  std::string operator_abort_policy_id{kPaperOnlineOperatorAbortPolicyV1};
  std::string kill_switch_policy_id{kPaperOnlineKillSwitchPolicyV1};
  std::int64_t max_market_data_staleness_ms{0};
  std::int64_t clock_skew_tolerance_ms{0};
  std::int64_t readiness_proof_checked_at_ms{0};
  std::int64_t session_admission_requested_at_ms{0};
  std::int64_t max_readiness_proof_age_ms{0};
  bool paper_online_readiness_contract_ready{false};
  bool readiness_proof_fresh{false};
  bool policy_acceptance_ready{false};
  bool tsodao_settings_protection_ready{false};
  bool accepted_policy_bound{false};
  bool protected_settings_bound{false};
  bool direct_edge_universe_validated{false};
  bool locked_execution_profile_bound{false};
  bool persistent_paper_ledger_recovery_bound{false};
  bool idempotency_bound{false};
  bool duplicate_action_protection_bound{false};
  bool session_lifecycle_bound{false};
  bool clock_timestamp_policy_bound{false};
  bool market_data_staleness_bound{false};
  bool reward_report_artifact_path_bound{false};
  bool operator_abort_bound{false};
  bool kill_switch_bound{false};
  bool synthetic_execution_markets_allowed{false};
  bool numeraire_fallback_allowed{false};
  bool paper_online_execution_allowed{false};
  bool live_execution_allowed{false};
  bool broker_execution_allowed{false};
  bool direct_policy_to_broker_allowed{false};
  bool direct_policy_to_cajtucu_allowed{false};
  bool artifact_evidence{true};
  bool visibility_only{true};
  bool paper_online_readiness_artifact{true};
  bool session_runner_authority{false};
  bool policy_selection_authority{false};
  bool checkpoint_selection_authority{false};
};

[[nodiscard]] inline paper_online_session_contract_t
default_paper_online_session_contract() {
  return {};
}

namespace paper_online_session_detail {

[[nodiscard]] inline bool blank(std::string_view value) {
  return std::all_of(value.begin(), value.end(),
                     [](unsigned char ch) { return std::isspace(ch) != 0; });
}

inline void require_nonblank(const std::string &value, const char *issue,
                             std::vector<std::string> *issues) {
  if (blank(value)) {
    issues->emplace_back(issue);
  }
}

inline void require_equal(const std::string &actual, std::string_view expected,
                          const char *issue, std::vector<std::string> *issues) {
  if (actual != expected) {
    issues->emplace_back(issue);
  }
}

inline void require_true(bool value, const char *issue,
                         std::vector<std::string> *issues) {
  if (!value) {
    issues->emplace_back(issue);
  }
}

[[nodiscard]] inline bool has_artifact_slot(
    const std::vector<paper_online_session_artifact_slot_t> &slots,
    const paper_online_session_artifact_slot_t &required) {
  return std::any_of(slots.begin(), slots.end(), [&](const auto &slot) {
    return slot.artifact_id == required.artifact_id &&
           slot.relative_path == required.relative_path &&
           slot.schema_id == required.schema_id && slot.required;
  });
}

} // namespace paper_online_session_detail

[[nodiscard]] inline std::vector<std::string>
paper_online_session_contract_issues(
    const paper_online_session_contract_t &contract) {
  namespace detail = paper_online_session_detail;
  std::vector<std::string> issues;
  detail::require_equal(contract.contract_id, kPaperOnlineSessionContractV1,
                        "unsupported_session_contract_id", &issues);
  detail::require_equal(
      contract.environment_contract_id, kPaperOnlineEnvironmentContractV1,
      "unsupported_paper_online_environment_contract_id", &issues);
  detail::require_equal(contract.readiness_target_id,
                        kPaperOnlineReadinessTargetReadyV1,
                        "unsupported_readiness_target_id", &issues);
  detail::require_equal(contract.session_state_schema_id,
                        kPaperOnlineSessionStateSchemaV1,
                        "unsupported_session_state_schema_id", &issues);
  detail::require_equal(contract.action_schema_id,
                        kPaperOnlineTargetWeightsActionSchemaV1,
                        "unsupported_action_schema_id", &issues);
  detail::require_equal(contract.reward_contract_id,
                        kPaperOnlineRewardContractV1,
                        "unsupported_reward_contract_id", &issues);
  detail::require_equal(contract.direct_edge_universe_validation_policy_id,
                        kPaperOnlineDirectEdgeUniversePolicyV1,
                        "unsupported_direct_edge_universe_policy_id", &issues);
  detail::require_equal(contract.missing_direct_pair_policy,
                        kPaperOnlineMissingDirectPairPolicyV1,
                        "unsupported_missing_direct_pair_policy", &issues);
  detail::require_equal(contract.synthetic_execution_market_policy_id,
                        kPaperOnlineSyntheticExecutionMarketPolicyV1,
                        "synthetic_execution_market_policy_mismatch", &issues);
  detail::require_equal(contract.persistent_paper_ledger_recovery_policy_id,
                        kPaperOnlinePersistentLedgerRecoveryPolicyV1,
                        "unsupported_ledger_recovery_policy_id", &issues);
  detail::require_equal(contract.paper_ledger_storage_policy_id,
                        kPaperOnlineLedgerStoragePolicyV1,
                        "unsupported_paper_ledger_storage_policy_id", &issues);
  detail::require_equal(contract.session_lifecycle_policy_id,
                        kPaperOnlineSessionLifecyclePolicyV1,
                        "unsupported_session_lifecycle_policy_id", &issues);
  detail::require_equal(contract.idempotency_policy_id,
                        kPaperOnlineIdempotencyPolicyV1,
                        "unsupported_idempotency_policy_id", &issues);
  detail::require_equal(contract.execution_intent_id_policy,
                        kPaperOnlineExecutionIntentIdPolicyV1,
                        "unsupported_execution_intent_id_policy", &issues);
  detail::require_equal(contract.duplicate_action_policy,
                        kPaperOnlineDuplicateActionPolicyV1,
                        "unsupported_duplicate_action_policy", &issues);
  detail::require_equal(contract.duplicate_execution_intent_policy,
                        kPaperOnlineDuplicateExecutionIntentPolicyV1,
                        "unsupported_duplicate_execution_intent_policy",
                        &issues);
  detail::require_equal(contract.clock_policy_id, kPaperOnlineClockPolicyV1,
                        "unsupported_clock_policy_id", &issues);
  detail::require_equal(contract.timestamp_policy_id,
                        kPaperOnlineTimestampPolicyV1,
                        "unsupported_timestamp_policy_id", &issues);
  detail::require_equal(contract.market_data_staleness_policy_id,
                        kPaperOnlineMarketDataStalenessPolicyV1,
                        "unsupported_market_data_staleness_policy_id", &issues);
  detail::require_equal(contract.reward_report_artifact_policy_id,
                        kPaperOnlineRewardReportArtifactPolicyV1,
                        "unsupported_reward_report_artifact_policy_id",
                        &issues);
  detail::require_equal(contract.operator_abort_policy_id,
                        kPaperOnlineOperatorAbortPolicyV1,
                        "unsupported_operator_abort_policy_id", &issues);
  detail::require_equal(contract.kill_switch_policy_id,
                        kPaperOnlineKillSwitchPolicyV1,
                        "unsupported_kill_switch_policy_id", &issues);

  for (const auto &required : default_paper_online_session_artifacts()) {
    if (!detail::has_artifact_slot(contract.durable_artifacts, required)) {
      issues.emplace_back("missing_session_artifact_slot:" +
                          required.artifact_id);
    }
  }
  detail::require_true(contract.requires_paper_online_readiness_contract_ready,
                       "readiness_contract_ready_requirement_missing", &issues);
  detail::require_true(contract.requires_fresh_readiness_proof,
                       "fresh_readiness_proof_requirement_missing", &issues);
  detail::require_true(contract.requires_market_stream_intake,
                       "market_stream_intake_requirement_missing", &issues);
  detail::require_true(contract.requires_cajtucu_paper_execution,
                       "cajtucu_paper_execution_requirement_missing", &issues);
  detail::require_true(contract.requires_persistent_paper_ledger_recovery,
                       "persistent_paper_ledger_recovery_requirement_missing",
                       &issues);
  detail::require_true(contract.requires_idempotent_execution_intents,
                       "idempotent_execution_intent_requirement_missing",
                       &issues);
  detail::require_true(contract.requires_duplicate_action_rejection,
                       "duplicate_action_rejection_requirement_missing",
                       &issues);
  detail::require_true(contract.requires_operator_abort,
                       "operator_abort_requirement_missing", &issues);
  detail::require_true(contract.requires_kill_switch,
                       "kill_switch_requirement_missing", &issues);
  if (contract.session_runner_implemented ||
      contract.policy_selection_authority ||
      contract.checkpoint_selection_authority ||
      contract.broker_execution_allowed || contract.live_execution_allowed ||
      contract.direct_policy_to_broker_allowed ||
      contract.direct_policy_to_cajtucu_allowed) {
    issues.emplace_back(
        "paper_online_session_contract_must_not_claim_execution_authority");
  }
  return issues;
}

[[nodiscard]] inline std::vector<std::string>
paper_online_readiness_evidence_issues(
    const paper_online_readiness_evidence_t &evidence) {
  namespace detail = paper_online_session_detail;
  std::vector<std::string> issues;
  detail::require_equal(evidence.readiness_target_id,
                        kPaperOnlineReadinessTargetReadyV1,
                        "unsupported_readiness_target_id", &issues);
  detail::require_equal(evidence.readiness_contract_id,
                        kPaperOnlineReadinessContractV1,
                        "unsupported_readiness_contract_id", &issues);
  detail::require_equal(evidence.readiness_fact_schema_id,
                        kPaperOnlineReadinessFactSchemaV1,
                        "unsupported_readiness_fact_schema_id", &issues);
  detail::require_nonblank(evidence.readiness_fact_digest,
                           "missing_readiness_fact_digest", &issues);
  detail::require_nonblank(evidence.readiness_proof_certificate_digest,
                           "missing_readiness_proof_certificate_digest",
                           &issues);
  detail::require_nonblank(evidence.readiness_id, "missing_readiness_id",
                           &issues);
  detail::require_nonblank(evidence.accepted_policy_id,
                           "missing_accepted_policy_id", &issues);
  detail::require_nonblank(evidence.accepted_actor_checkpoint_digest,
                           "missing_accepted_actor_checkpoint_digest", &issues);
  detail::require_nonblank(evidence.accepted_checkpoint_digest,
                           "missing_accepted_checkpoint_digest", &issues);
  detail::require_nonblank(evidence.execution_profile_digest,
                           "missing_execution_profile_digest", &issues);
  detail::require_nonblank(evidence.locked_execution_profile_digest,
                           "missing_locked_execution_profile_digest", &issues);
  detail::require_nonblank(evidence.accounting_numeraire_node_id,
                           "missing_accounting_numeraire_node_id", &issues);
  detail::require_nonblank(evidence.paper_online_profile_digest,
                           "missing_paper_online_profile_digest", &issues);
  detail::require_nonblank(evidence.direct_edge_universe_digest,
                           "missing_direct_edge_universe_digest", &issues);
  detail::require_equal(evidence.paper_online_environment_contract_id,
                        kPaperOnlineEnvironmentContractV1,
                        "unsupported_paper_online_environment_contract_id",
                        &issues);
  detail::require_equal(evidence.session_state_schema_id,
                        kPaperOnlineSessionStateSchemaV1,
                        "unsupported_session_state_schema_id", &issues);
  detail::require_equal(evidence.reward_contract_id,
                        kPaperOnlineRewardContractV1,
                        "unsupported_reward_contract_id", &issues);
  detail::require_equal(evidence.direct_edge_universe_validation_policy_id,
                        kPaperOnlineDirectEdgeUniversePolicyV1,
                        "unsupported_direct_edge_universe_policy_id", &issues);
  detail::require_equal(evidence.missing_direct_pair_policy,
                        kPaperOnlineMissingDirectPairPolicyV1,
                        "unsupported_missing_direct_pair_policy", &issues);
  detail::require_equal(evidence.synthetic_execution_market_policy_id,
                        kPaperOnlineSyntheticExecutionMarketPolicyV1,
                        "synthetic_execution_market_policy_mismatch", &issues);
  detail::require_equal(evidence.persistent_paper_ledger_recovery_policy_id,
                        kPaperOnlinePersistentLedgerRecoveryPolicyV1,
                        "unsupported_ledger_recovery_policy_id", &issues);
  detail::require_equal(evidence.paper_ledger_storage_policy_id,
                        kPaperOnlineLedgerStoragePolicyV1,
                        "unsupported_paper_ledger_storage_policy_id", &issues);
  detail::require_equal(evidence.session_lifecycle_policy_id,
                        kPaperOnlineSessionLifecyclePolicyV1,
                        "unsupported_session_lifecycle_policy_id", &issues);
  detail::require_equal(evidence.idempotency_policy_id,
                        kPaperOnlineIdempotencyPolicyV1,
                        "unsupported_idempotency_policy_id", &issues);
  detail::require_equal(evidence.execution_intent_id_policy,
                        kPaperOnlineExecutionIntentIdPolicyV1,
                        "unsupported_execution_intent_id_policy", &issues);
  detail::require_equal(evidence.duplicate_action_policy,
                        kPaperOnlineDuplicateActionPolicyV1,
                        "unsupported_duplicate_action_policy", &issues);
  detail::require_equal(evidence.duplicate_execution_intent_policy,
                        kPaperOnlineDuplicateExecutionIntentPolicyV1,
                        "unsupported_duplicate_execution_intent_policy",
                        &issues);
  detail::require_equal(evidence.clock_policy_id, kPaperOnlineClockPolicyV1,
                        "unsupported_clock_policy_id", &issues);
  detail::require_equal(evidence.timestamp_policy_id,
                        kPaperOnlineTimestampPolicyV1,
                        "unsupported_timestamp_policy_id", &issues);
  detail::require_equal(evidence.market_data_staleness_policy_id,
                        kPaperOnlineMarketDataStalenessPolicyV1,
                        "unsupported_market_data_staleness_policy_id", &issues);
  detail::require_equal(evidence.reward_report_artifact_policy_id,
                        kPaperOnlineRewardReportArtifactPolicyV1,
                        "unsupported_reward_report_artifact_policy_id",
                        &issues);
  detail::require_equal(evidence.operator_abort_policy_id,
                        kPaperOnlineOperatorAbortPolicyV1,
                        "unsupported_operator_abort_policy_id", &issues);
  detail::require_equal(evidence.kill_switch_policy_id,
                        kPaperOnlineKillSwitchPolicyV1,
                        "unsupported_kill_switch_policy_id", &issues);
  if (evidence.locked_execution_profile_digest !=
      evidence.execution_profile_digest) {
    issues.emplace_back("locked_execution_profile_digest_mismatch");
  }
  if (evidence.max_market_data_staleness_ms <= 0) {
    issues.emplace_back("invalid_max_market_data_staleness_ms");
  }
  if (evidence.clock_skew_tolerance_ms < 0) {
    issues.emplace_back("invalid_clock_skew_tolerance_ms");
  }
  if (evidence.readiness_proof_checked_at_ms <= 0) {
    issues.emplace_back("missing_readiness_proof_checked_at_ms");
  }
  if (evidence.session_admission_requested_at_ms <= 0) {
    issues.emplace_back("missing_session_admission_requested_at_ms");
  }
  if (evidence.max_readiness_proof_age_ms <= 0) {
    issues.emplace_back("invalid_max_readiness_proof_age_ms");
  }
  if (evidence.readiness_proof_checked_at_ms > 0 &&
      evidence.session_admission_requested_at_ms > 0 &&
      evidence.session_admission_requested_at_ms <
          evidence.readiness_proof_checked_at_ms) {
    issues.emplace_back("readiness_proof_checked_after_admission_request");
  }
  if (evidence.readiness_proof_checked_at_ms > 0 &&
      evidence.session_admission_requested_at_ms > 0 &&
      evidence.max_readiness_proof_age_ms > 0 &&
      evidence.session_admission_requested_at_ms -
              evidence.readiness_proof_checked_at_ms >
          evidence.max_readiness_proof_age_ms) {
    issues.emplace_back("stale_readiness_proof");
  }
  detail::require_true(evidence.paper_online_readiness_contract_ready,
                       "paper_online_readiness_contract_not_ready", &issues);
  detail::require_true(evidence.readiness_proof_fresh,
                       "readiness_proof_not_fresh", &issues);
  detail::require_true(evidence.policy_acceptance_ready,
                       "policy_acceptance_ready_not_bound", &issues);
  detail::require_true(evidence.tsodao_settings_protection_ready,
                       "tsodao_settings_protection_ready_not_bound", &issues);
  detail::require_true(evidence.accepted_policy_bound,
                       "accepted_policy_not_bound", &issues);
  detail::require_true(evidence.protected_settings_bound,
                       "protected_settings_not_bound", &issues);
  detail::require_true(evidence.direct_edge_universe_validated,
                       "direct_edge_universe_not_validated", &issues);
  detail::require_true(evidence.locked_execution_profile_bound,
                       "locked_execution_profile_not_bound", &issues);
  detail::require_true(evidence.persistent_paper_ledger_recovery_bound,
                       "persistent_paper_ledger_recovery_not_bound", &issues);
  detail::require_true(evidence.idempotency_bound, "idempotency_not_bound",
                       &issues);
  detail::require_true(evidence.duplicate_action_protection_bound,
                       "duplicate_action_protection_not_bound", &issues);
  detail::require_true(evidence.session_lifecycle_bound,
                       "session_lifecycle_not_bound", &issues);
  detail::require_true(evidence.clock_timestamp_policy_bound,
                       "clock_timestamp_policy_not_bound", &issues);
  detail::require_true(evidence.market_data_staleness_bound,
                       "market_data_staleness_not_bound", &issues);
  detail::require_true(evidence.reward_report_artifact_path_bound,
                       "reward_report_artifact_path_not_bound", &issues);
  detail::require_true(evidence.operator_abort_bound,
                       "operator_abort_not_bound", &issues);
  detail::require_true(evidence.kill_switch_bound, "kill_switch_not_bound",
                       &issues);
  if (evidence.synthetic_execution_markets_allowed ||
      evidence.numeraire_fallback_allowed ||
      evidence.paper_online_execution_allowed ||
      evidence.live_execution_allowed || evidence.broker_execution_allowed ||
      evidence.direct_policy_to_broker_allowed ||
      evidence.direct_policy_to_cajtucu_allowed ||
      !evidence.artifact_evidence || !evidence.visibility_only ||
      !evidence.paper_online_readiness_artifact ||
      evidence.session_runner_authority ||
      evidence.policy_selection_authority ||
      evidence.checkpoint_selection_authority) {
    issues.emplace_back(
        "readiness_evidence_must_not_claim_execution_authority");
  }
  return issues;
}

[[nodiscard]] inline std::vector<std::string>
paper_online_session_admission_issues(
    const paper_online_session_contract_t &contract,
    const paper_online_readiness_evidence_t &evidence) {
  auto issues = paper_online_session_contract_issues(contract);
  auto evidence_issues = paper_online_readiness_evidence_issues(evidence);
  issues.insert(issues.end(), evidence_issues.begin(), evidence_issues.end());
  if (contract.environment_contract_id !=
      evidence.paper_online_environment_contract_id) {
    issues.emplace_back("session_environment_contract_mismatch");
  }
  if (contract.session_state_schema_id != evidence.session_state_schema_id) {
    issues.emplace_back("session_state_schema_mismatch");
  }
  if (contract.reward_contract_id != evidence.reward_contract_id) {
    issues.emplace_back("session_reward_contract_mismatch");
  }
  return issues;
}

[[nodiscard]] inline bool paper_online_session_admission_ready(
    const paper_online_session_contract_t &contract,
    const paper_online_readiness_evidence_t &evidence) {
  return paper_online_session_admission_issues(contract, evidence).empty();
}

} // namespace cuwacunu::kikijyeba::environment
