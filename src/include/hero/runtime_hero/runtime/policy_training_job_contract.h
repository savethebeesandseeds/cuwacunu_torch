// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "hero/runtime_hero/runtime/policy_training_causal_schedule.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::hero::runtime {

inline constexpr const char *k_policy_input_schema_v1 =
    "kikijyeba.environment.policy_input.v1";
inline constexpr const char *k_target_node_weights_action_schema_v1 =
    "kikijyeba.environment.action.target_node_weights.v1";
inline constexpr const char *k_target_node_weights_simplex_adapter_v1 =
    "target_node_weights_simplex.v1";
inline constexpr const char *k_post_execution_reward_contract_v1 =
    "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
    "drawdown.v1";

struct policy_training_job_contract_t {
  std::string schema_version{
      "kikijyeba.runtime.policy_training_job_contract.v1"};
  std::string artifact_schema_id{"kikijyeba.lattice.policy_training.v1"};
  std::string runtime_job_kind{"policy_training"};
  std::string protocol_id{"cwu_02v"};
  std::string protocol_contract_fingerprint{};
  std::string graph_order_fingerprint{};
  std::string source_cursor_token{};
  std::string split_policy_fingerprint{};
  std::string component_assembly_fingerprint{};
  std::string policy_id{};
  std::string policy_kind{};
  std::string policy_architecture_digest{};
  std::string training_config_digest{};
  std::string training_range_digest{};
  std::string validation_range_digest{};
  std::string test_range_digest{};
  std::string environment_contract_id{"kikijyeba.environment.replay.v1"};
  std::string observation_schema_digest{};
  std::string action_schema_digest{};
  std::string reward_contract_digest{};
  std::string policy_input_schema_id{k_policy_input_schema_v1};
  std::string action_adapter_id{k_target_node_weights_simplex_adapter_v1};
  std::string reward_contract_id{k_post_execution_reward_contract_v1};
  std::string execution_profile_digest{};
  std::string training_schedule_mode{k_policy_training_schedule_mode_causal_v1};
  std::string causal_schedule_schema_id{
      k_policy_training_causal_schedule_schema_v1};
  std::string causal_schedule_digest{};
  std::string causal_schedule_cursor_key_kind{"numeric_anchor_index"};
  std::string causal_schedule_no_future_snapshot_use_source{
      k_policy_training_schedule_no_future_snapshot_source_v1};
  std::string normalization_fit_range_digest{};
  std::string replay_buffer_source_range_digest{};
  std::string early_stopping_policy_digest{};
  std::string hyperparameter_selection_policy_digest{};
  std::string selector_split{"validation"};
  std::string selector_policy_digest{};
  std::string parent_checkpoint_digest{};
  std::string parent_forecast_eval_fact_digest{};
  std::string parent_observer_belief_fact_digest{};
  std::string parent_allocation_engine_fact_digest{};
  std::string parent_replay_environment_fact_digest{};
  std::string final_refit_parent_selected_checkpoint_digest{};
  std::int64_t random_seed{0};
  bool random_seed_bound{false};
  std::int64_t max_episodes{0};
  std::int64_t max_steps{0};
  std::int64_t max_parallel_jobs{1};
  std::int64_t max_wall_clock_seconds{0};
  bool causal_schedule_readiness_eligible{true};
  bool causal_schedule_no_future_snapshot_use{true};
  bool offline_full_window_research_allowed{false};
  bool final_refit_uses_validation{false};
  bool validation_no_longer_proof{false};
  bool sealed_test_required{false};
  bool live_execution_allowed{false};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream out;
    out << "schema_version=" << schema_version << "\n";
    out << "artifact_schema_id=" << artifact_schema_id << "\n";
    out << "runtime_job_kind=" << runtime_job_kind << "\n";
    out << "protocol_id=" << protocol_id << "\n";
    out << "protocol_contract_fingerprint=" << protocol_contract_fingerprint
        << "\n";
    out << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    out << "source_cursor_token=" << source_cursor_token << "\n";
    out << "split_policy_fingerprint=" << split_policy_fingerprint << "\n";
    out << "component_assembly_fingerprint=" << component_assembly_fingerprint
        << "\n";
    out << "policy_id=" << policy_id << "\n";
    out << "policy_kind=" << policy_kind << "\n";
    out << "policy_architecture_digest=" << policy_architecture_digest << "\n";
    out << "training_config_digest=" << training_config_digest << "\n";
    out << "training_range_digest=" << training_range_digest << "\n";
    out << "validation_range_digest=" << validation_range_digest << "\n";
    out << "test_range_digest=" << test_range_digest << "\n";
    out << "environment_contract_id=" << environment_contract_id << "\n";
    out << "observation_schema_digest=" << observation_schema_digest << "\n";
    out << "action_schema_digest=" << action_schema_digest << "\n";
    out << "reward_contract_digest=" << reward_contract_digest << "\n";
    out << "policy_input_schema_id=" << policy_input_schema_id << "\n";
    out << "action_adapter_id=" << action_adapter_id << "\n";
    out << "reward_contract_id=" << reward_contract_id << "\n";
    out << "execution_profile_digest=" << execution_profile_digest << "\n";
    out << "training_schedule_mode=" << training_schedule_mode << "\n";
    out << "causal_schedule_schema_id=" << causal_schedule_schema_id << "\n";
    out << "causal_schedule_digest=" << causal_schedule_digest << "\n";
    out << "causal_schedule_cursor_key_kind=" << causal_schedule_cursor_key_kind
        << "\n";
    out << "causal_schedule_no_future_snapshot_use_source="
        << causal_schedule_no_future_snapshot_use_source << "\n";
    out << "normalization_fit_range_digest=" << normalization_fit_range_digest
        << "\n";
    out << "replay_buffer_source_range_digest="
        << replay_buffer_source_range_digest << "\n";
    out << "early_stopping_policy_digest=" << early_stopping_policy_digest
        << "\n";
    out << "hyperparameter_selection_policy_digest="
        << hyperparameter_selection_policy_digest << "\n";
    out << "selector_split=" << selector_split << "\n";
    out << "selector_policy_digest=" << selector_policy_digest << "\n";
    out << "parent_checkpoint_digest=" << parent_checkpoint_digest << "\n";
    out << "parent_forecast_eval_fact_digest="
        << parent_forecast_eval_fact_digest << "\n";
    out << "parent_observer_belief_fact_digest="
        << parent_observer_belief_fact_digest << "\n";
    out << "parent_allocation_engine_fact_digest="
        << parent_allocation_engine_fact_digest << "\n";
    out << "parent_replay_environment_fact_digest="
        << parent_replay_environment_fact_digest << "\n";
    out << "final_refit_parent_selected_checkpoint_digest="
        << final_refit_parent_selected_checkpoint_digest << "\n";
    out << "random_seed=" << random_seed << "\n";
    out << "random_seed_bound=" << (random_seed_bound ? "true" : "false")
        << "\n";
    out << "max_episodes=" << max_episodes << "\n";
    out << "max_steps=" << max_steps << "\n";
    out << "max_parallel_jobs=" << max_parallel_jobs << "\n";
    out << "max_wall_clock_seconds=" << max_wall_clock_seconds << "\n";
    out << "causal_schedule_readiness_eligible="
        << (causal_schedule_readiness_eligible ? "true" : "false") << "\n";
    out << "causal_schedule_no_future_snapshot_use="
        << (causal_schedule_no_future_snapshot_use ? "true" : "false") << "\n";
    out << "offline_full_window_research_allowed="
        << (offline_full_window_research_allowed ? "true" : "false") << "\n";
    out << "final_refit_uses_validation="
        << (final_refit_uses_validation ? "true" : "false") << "\n";
    out << "validation_no_longer_proof="
        << (validation_no_longer_proof ? "true" : "false") << "\n";
    out << "sealed_test_required=" << (sealed_test_required ? "true" : "false")
        << "\n";
    out << "live_execution_allowed="
        << (live_execution_allowed ? "true" : "false") << "\n";
    return out.str();
  }
};

[[nodiscard]] inline std::string
policy_training_contract_digest_for_text(std::string_view text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.runtime.policy_training_job_contract.digest.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(hash, text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string policy_training_contract_digest(
    const policy_training_job_contract_t &contract) {
  return policy_training_contract_digest_for_text(contract.to_text());
}

[[nodiscard]] inline bool
policy_training_contract_split_name_valid(const std::string &value) {
  return value == "none" || value == "train" || value == "training" ||
         value == "validation";
}

[[nodiscard]] inline std::vector<std::string>
validate_policy_training_job_contract(
    const policy_training_job_contract_t &contract) {
  std::vector<std::string> issues;
  const auto require_nonempty = [&issues](const std::string &value,
                                          const char *field) {
    if (value.empty()) {
      issues.emplace_back(std::string("missing_") + field);
    }
  };

  require_nonempty(contract.protocol_id, "protocol_id");
  require_nonempty(contract.protocol_contract_fingerprint,
                   "protocol_contract_fingerprint");
  require_nonempty(contract.graph_order_fingerprint, "graph_order_fingerprint");
  require_nonempty(contract.source_cursor_token, "source_cursor_token");
  require_nonempty(contract.policy_id, "policy_id");
  require_nonempty(contract.policy_kind, "policy_kind");
  require_nonempty(contract.policy_architecture_digest,
                   "policy_architecture_digest");
  require_nonempty(contract.training_config_digest, "training_config_digest");
  require_nonempty(contract.training_range_digest, "training_range_digest");
  require_nonempty(contract.validation_range_digest, "validation_range_digest");
  require_nonempty(contract.test_range_digest, "test_range_digest");
  require_nonempty(contract.environment_contract_id, "environment_contract_id");
  require_nonempty(contract.observation_schema_digest,
                   "observation_schema_digest");
  require_nonempty(contract.action_schema_digest, "action_schema_digest");
  require_nonempty(contract.reward_contract_digest, "reward_contract_digest");
  require_nonempty(contract.policy_input_schema_id, "policy_input_schema_id");
  require_nonempty(contract.action_adapter_id, "action_adapter_id");
  require_nonempty(contract.reward_contract_id, "reward_contract_id");
  require_nonempty(contract.execution_profile_digest,
                   "execution_profile_digest");
  require_nonempty(contract.training_schedule_mode, "training_schedule_mode");
  require_nonempty(contract.causal_schedule_schema_id,
                   "causal_schedule_schema_id");
  require_nonempty(contract.causal_schedule_digest, "causal_schedule_digest");
  require_nonempty(contract.causal_schedule_cursor_key_kind,
                   "causal_schedule_cursor_key_kind");
  require_nonempty(contract.causal_schedule_no_future_snapshot_use_source,
                   "causal_schedule_no_future_snapshot_use_source");
  require_nonempty(contract.normalization_fit_range_digest,
                   "normalization_fit_range_digest");
  require_nonempty(contract.replay_buffer_source_range_digest,
                   "replay_buffer_source_range_digest");
  require_nonempty(contract.early_stopping_policy_digest,
                   "early_stopping_policy_digest");
  require_nonempty(contract.hyperparameter_selection_policy_digest,
                   "hyperparameter_selection_policy_digest");
  require_nonempty(contract.selector_policy_digest, "selector_policy_digest");
  require_nonempty(contract.parent_checkpoint_digest,
                   "parent_checkpoint_digest");
  require_nonempty(contract.parent_forecast_eval_fact_digest,
                   "parent_forecast_eval_fact_digest");
  require_nonempty(contract.parent_observer_belief_fact_digest,
                   "parent_observer_belief_fact_digest");
  require_nonempty(contract.parent_allocation_engine_fact_digest,
                   "parent_allocation_engine_fact_digest");
  require_nonempty(contract.parent_replay_environment_fact_digest,
                   "parent_replay_environment_fact_digest");

  if (!contract.training_range_digest.empty() &&
      contract.training_range_digest == contract.validation_range_digest) {
    issues.emplace_back("training_validation_range_overlap");
  }
  if (!contract.training_range_digest.empty() &&
      contract.training_range_digest == contract.test_range_digest) {
    issues.emplace_back("training_test_range_overlap");
  }
  if (!contract.validation_range_digest.empty() &&
      contract.validation_range_digest == contract.test_range_digest) {
    issues.emplace_back("validation_test_range_overlap");
  }
  if (!contract.normalization_fit_range_digest.empty() &&
      !contract.training_range_digest.empty() &&
      contract.normalization_fit_range_digest !=
          contract.training_range_digest) {
    issues.emplace_back("normalization_fit_range_not_training");
  }
  if (!contract.replay_buffer_source_range_digest.empty() &&
      !contract.training_range_digest.empty() &&
      contract.replay_buffer_source_range_digest !=
          contract.training_range_digest) {
    issues.emplace_back("replay_buffer_source_range_not_training");
  }
  if (!contract.policy_input_schema_id.empty() &&
      contract.policy_input_schema_id != k_policy_input_schema_v1) {
    issues.emplace_back("unsupported_policy_input_schema_id");
  }
  if (!contract.action_schema_digest.empty() &&
      contract.action_schema_digest != k_target_node_weights_action_schema_v1) {
    issues.emplace_back("unsupported_action_schema_digest");
  }
  if (!contract.action_adapter_id.empty() &&
      contract.action_adapter_id != k_target_node_weights_simplex_adapter_v1) {
    issues.emplace_back("unsupported_action_adapter_id");
  }
  if (!contract.reward_contract_id.empty() &&
      contract.reward_contract_id != k_post_execution_reward_contract_v1) {
    issues.emplace_back("unsupported_reward_contract_id");
  }
  if (!policy_training_schedule_mode_supported(
          contract.training_schedule_mode)) {
    issues.emplace_back("unsupported_training_schedule_mode");
  }
  if (!contract.causal_schedule_schema_id.empty() &&
      contract.causal_schedule_schema_id !=
          k_policy_training_causal_schedule_schema_v1) {
    issues.emplace_back("unsupported_causal_schedule_schema_id");
  }
  if (parse_cursor_key_kind(contract.causal_schedule_cursor_key_kind) ==
      cursor_key_kind_t::opaque_unsortable) {
    issues.emplace_back(
        "opaque_or_unsupported_causal_schedule_cursor_key_kind");
  }
  if (contract.causal_schedule_no_future_snapshot_use_source !=
      k_policy_training_schedule_no_future_snapshot_source_v1) {
    issues.emplace_back("unsupported_no_future_snapshot_use_source");
  }
  if (contract.training_schedule_mode ==
      k_policy_training_schedule_mode_causal_v1) {
    if (!contract.causal_schedule_readiness_eligible) {
      issues.emplace_back("causal_schedule_not_readiness_eligible");
    }
    if (!contract.causal_schedule_no_future_snapshot_use) {
      issues.emplace_back("causal_schedule_future_snapshot_use_not_proven");
    }
    if (contract.offline_full_window_research_allowed) {
      issues.emplace_back(
          "causal_schedule_must_not_enable_full_window_research");
    }
  }
  if (contract.training_schedule_mode ==
      k_policy_training_schedule_mode_offline_research) {
    if (contract.causal_schedule_readiness_eligible) {
      issues.emplace_back(
          "offline_full_window_research_not_readiness_eligible");
    }
    if (!contract.offline_full_window_research_allowed) {
      issues.emplace_back(
          "offline_full_window_research_requires_explicit_allow");
    }
  }
  if (contract.training_schedule_mode ==
      k_policy_training_schedule_mode_final_refit) {
    if (contract.causal_schedule_readiness_eligible) {
      issues.emplace_back("batch_final_refit_not_policy_training_readiness");
    }
    if (!contract.final_refit_uses_validation) {
      issues.emplace_back("batch_final_refit_must_declare_validation_use");
    }
    if (contract.final_refit_parent_selected_checkpoint_digest.empty()) {
      issues.emplace_back(
          "missing_final_refit_parent_selected_checkpoint_digest");
    }
    if (!contract.validation_no_longer_proof) {
      issues.emplace_back("final_refit_validation_no_longer_proof_required");
    }
    if (!contract.sealed_test_required) {
      issues.emplace_back("final_refit_requires_sealed_test");
    }
  }
  if (contract.final_refit_uses_validation &&
      (!contract.validation_no_longer_proof ||
       !contract.sealed_test_required)) {
    issues.emplace_back("validation_refit_requires_sealed_test_disclaimer");
  }
  if (!policy_training_contract_split_name_valid(contract.selector_split)) {
    issues.emplace_back("unsupported_selector_split");
  }
  if (contract.selector_split == "test") {
    issues.emplace_back("selector_split_must_not_be_test");
  }
  if (contract.max_episodes <= 0) {
    issues.emplace_back("nonpositive_max_episodes");
  }
  if (contract.max_steps <= 0) {
    issues.emplace_back("nonpositive_max_steps");
  }
  if (contract.max_parallel_jobs <= 0) {
    issues.emplace_back("nonpositive_max_parallel_jobs");
  }
  if (contract.max_wall_clock_seconds <= 0) {
    issues.emplace_back("nonpositive_max_wall_clock_seconds");
  }
  if (!contract.random_seed_bound) {
    issues.emplace_back("missing_random_seed");
  }
  if (contract.live_execution_allowed) {
    issues.emplace_back("live_execution_forbidden");
  }
  return issues;
}

[[nodiscard]] inline bool policy_training_job_contract_valid(
    const policy_training_job_contract_t &contract) {
  return validate_policy_training_job_contract(contract).empty();
}

} // namespace cuwacunu::hero::runtime
