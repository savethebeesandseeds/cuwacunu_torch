// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
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
inline constexpr const char *k_masked_dirichlet_simplex_distribution_v1 =
    "masked_dirichlet_simplex.v1";
inline constexpr const char *k_masked_logistic_normal_simplex_distribution_v1 =
    "masked_logistic_normal_simplex.v1";
inline constexpr const char *k_post_execution_reward_contract_v1 =
    "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
    "drawdown.v1";
inline constexpr const char *k_ppo_policy_artifact_contract_v1 =
    "kikijyeba.runtime.ppo_policy_artifact_contract.v1";
inline constexpr const char *k_graph_node_allocation_policy_family_v1 =
    "wikimyei.policy.portfolio.graph_node_allocation";
inline constexpr const char *k_ppo_policy_checkpoint_schema_v1 =
    "wikimyei.policy.portfolio.graph_node_allocation.ppo_checkpoint.v1";
inline constexpr const char *k_ppo_rollout_collection_schema_v1 =
    "kikijyeba.runtime.ppo_rollout_collection.v1";
inline constexpr const char *k_ppo_update_report_schema_v1 =
    "kikijyeba.runtime.ppo_update_report.v1";
inline constexpr const char *k_policy_quality_report_schema_v1 =
    "kikijyeba.runtime.policy_quality_report.v1";
inline constexpr const char *k_gae_advantage_estimator_v1 = "gae.v1";
inline constexpr const char *k_ppo_advantage_normalization_policy_v1 =
    "batch_center_scale_advantages.v1";
inline constexpr const char *k_policy_training_purged_embargo_policy_v1 =
    "embargo_policy_anchor_v1";
inline constexpr const char *k_no_lookahead_artifact_provenance_certificate_v1 =
    "no_lookahead_artifact_provenance.v1";
inline constexpr const char
    *k_no_lookahead_artifact_provenance_anchor_schema_v1 =
        "no_lookahead_artifact_provenance.anchor_v1";
inline constexpr const char *k_policy_runtime_execution_provenance_v1 =
    "policy_runtime_execution_provenance.v1";
inline constexpr const char *k_causal_provenance_generalization_v1 =
    "causal_provenance_generalization.v1";
inline constexpr const char *k_causal_atom_anchor_interval_v1 =
    "causal_atom.anchor_interval_v1";
inline constexpr const char *k_causal_interval_set_anchor_half_open_v1 =
    "causal_interval_set.anchor_half_open_v1";
inline constexpr const char *k_runtime_artifact_production_inline_policy_v1 =
    "runtime.artifact_production.inline_policy_training.v1";
inline constexpr const char
    *k_interface_stability_trained_against_generation_vector_v1 =
        "interface_stability.trained_against_generation_vector.v1";

[[nodiscard]] inline bool
policy_kind_is_ppo_adapter(std::string_view policy_kind,
                           std::string_view ppo_contract_id = {}) {
  return policy_kind.find("ppo") != std::string_view::npos ||
         policy_kind.find("PPO") != std::string_view::npos ||
         !ppo_contract_id.empty();
}

struct policy_training_job_contract_t {
  std::string schema_version{
      "kikijyeba.runtime.policy_training_job_contract.v1"};
  std::string artifact_schema_id{"kikijyeba.lattice.policy_training.v1"};
  std::string runtime_job_kind{"policy_training"};
  std::string job_id{};
  std::string job_dir{};
  std::string report_path{};
  std::string replay_job_dir{};
  std::string certified_replay_bank_schema{"certified_replay_bank_manifest.v1"};
  std::string certified_replay_bank_manifest_path{};
  std::string certified_replay_bank_manifest_digest{};
  std::string policy_experience_set_digest{};
  std::string policy_experience_mode{"on_policy_current_rollout"};
  std::int64_t certified_replay_chunk_count{0};
  bool certified_replay_chunk_count_bound{false};
  std::int64_t certified_replay_sample_count{0};
  bool certified_replay_sample_count_bound{false};
  std::int64_t certified_replay_coverage_anchor_begin{0};
  std::int64_t certified_replay_coverage_anchor_end_exclusive{0};
  bool certified_replay_coverage_anchor_range_bound{false};
  std::string config_path{};
  std::string accounting_numeraire_node_id{};
  std::string target_node_ids{};
  std::string experiment_id{};
  std::string policy_set_digest{};
  std::string protocol_id{};
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
  std::string action_distribution_id{
      k_masked_dirichlet_simplex_distribution_v1};
  std::string reward_contract_id{k_post_execution_reward_contract_v1};
  std::string execution_profile_digest{};
  std::string ppo_policy_artifact_contract_id{};
  std::string policy_family_id{};
  std::string policy_checkpoint_schema_id{};
  std::string policy_dsl_digest{};
  std::string policy_net_digest{};
  std::string policy_input_feature_manifest_digest{};
  std::string policy_jkimyei_digest{};
  std::string policy_jkimyei_no_lookahead_contract_id{};
  std::string policy_jkimyei_no_lookahead_contract_digest{};
  std::string policy_jkimyei_component_order_contract_id{};
  std::string policy_jkimyei_component_order_contract_digest{};
  std::string policy_jkimyei_component_role{};
  std::int64_t policy_jkimyei_serving_order_index{0};
  bool policy_jkimyei_serving_order_index_bound{false};
  std::string target_node_universe_digest{};
  std::string action_distribution_config_digest{};
  std::string snapshot_family_digest{};
  std::string snapshot_bundle_schema{"snapshot_bundle_manifest.v1"};
  std::string snapshot_bundle_id{};
  std::string snapshot_bundle_kind{"readiness_candidate"};
  std::string snapshot_bundle_selection_basis{};
  std::string snapshot_bundle_no_lookahead_contract_id{};
  std::string snapshot_bundle_no_lookahead_contract_digest{};
  std::string snapshot_bundle_component_order_contract_id{};
  std::string snapshot_bundle_component_order_contract_digest{};
  std::string snapshot_bundle_protocol_contract_fingerprint{};
  std::string snapshot_bundle_generation_vector_digest{};
  std::int64_t snapshot_bundle_valid_from_anchor{0};
  bool snapshot_bundle_valid_from_anchor_bound{false};
  std::int64_t snapshot_bundle_valid_until_anchor{0};
  bool snapshot_bundle_valid_until_anchor_bound{false};
  std::int64_t snapshot_bundle_influence_anchor_end_exclusive_max{0};
  bool snapshot_bundle_influence_anchor_end_exclusive_max_bound{false};
  std::string snapshot_bundle_compatibility_closure_digest{};
  std::string snapshot_bundle_no_lookahead_certificate_digests{};
  std::string snapshot_bundle_policy_execution_certificate_digest{};
  std::string snapshot_bundle_created_by_lattice_target{};
  std::string bundle_representation_component_id{};
  std::string bundle_representation_jkimyei_digest{};
  std::string bundle_representation_checkpoint_digest{};
  std::string bundle_representation_generation_id{};
  std::string bundle_representation_generation_vector_digest{};
  std::string bundle_representation_generation_manifest_digest{};
  std::string bundle_mdn_component_id{};
  std::string bundle_mdn_jkimyei_digest{};
  std::string bundle_mdn_checkpoint_digest{};
  std::string bundle_mdn_generation_id{};
  std::string bundle_mdn_generation_vector_digest{};
  std::string bundle_mdn_generation_manifest_digest{};
  std::string bundle_policy_component_id{};
  std::string bundle_policy_jkimyei_digest{};
  std::string bundle_policy_checkpoint_digest{};
  std::string bundle_policy_generation_id{};
  std::string bundle_policy_generation_vector_digest{};
  std::string bundle_policy_generation_manifest_digest{};
  std::string mdn_trained_against_representation_generation_id{};
  std::string mdn_trained_against_representation_checkpoint_digest{};
  std::string mdn_trained_against_representation_generation_vector_digest{};
  std::string mdn_representation_feature_schema_digest{};
  std::string policy_trained_against_forecast_eval_fact_digest{};
  std::string policy_trained_against_replay_environment_fact_digest{};
  std::string policy_trained_against_representation_generation_vector_digest{};
  std::string policy_trained_against_mdn_generation_vector_digest{};
  std::string policy_trained_against_policy_parent_generation_vector_digest{};
  std::string policy_execution_input_lock_digest{};
  std::string policy_trained_against_no_lookahead_certificate_digest{};
  std::string actor_architecture_digest{};
  std::string critic_architecture_digest{};
  std::string actor_checkpoint_digest{};
  std::string critic_checkpoint_digest{};
  std::string optimizer_state_digest{};
  std::string ppo_config_digest{};
  std::string optimizer_state_schema_id{
      "kikijyeba.runtime.ppo_optimizer_state.v1"};
  std::string device_policy{"require_cuda"};
  std::int64_t cuda_device_index{0};
  bool cuda_device_index_bound{false};
  std::string resume_mode{"fresh_spawn"};
  std::string resume_actor_checkpoint_path{};
  std::string resume_actor_checkpoint_digest{};
  std::string resume_optimizer_state_path{};
  std::string resume_optimizer_state_digest{};
  std::string advantage_estimator_id{};
  std::string advantage_normalization_policy{};
  std::string rollout_collection_schema_id{};
  std::string rollout_collection_digest{};
  std::string ppo_update_report_schema_id{};
  std::string ppo_update_report_digest{};
  std::string validation_rollout_report_digest{};
  std::string policy_quality_report_digest{};
  double ppo_gamma{0.0};
  double ppo_gae_lambda{0.0};
  double ppo_clip_epsilon{0.0};
  double ppo_target_kl{0.0};
  double ppo_learning_rate{0.0};
  double ppo_entropy_coeff{0.0};
  double ppo_value_loss_coeff{0.0};
  double ppo_max_grad_norm{0.0};
  double linear_transaction_cost_rate{0.0};
  double initial_equity_numeraire{0.0};
  double max_node_weight{0.0};
  double max_turnover_l1{0.0};
  bool ppo_gamma_bound{false};
  bool ppo_gae_lambda_bound{false};
  bool ppo_clip_epsilon_bound{false};
  bool ppo_target_kl_bound{false};
  bool ppo_learning_rate_bound{false};
  bool ppo_entropy_coeff_bound{false};
  bool ppo_value_loss_coeff_bound{false};
  bool ppo_max_grad_norm_bound{false};
  bool linear_transaction_cost_rate_bound{false};
  bool initial_equity_numeraire_bound{false};
  bool max_node_weight_bound{false};
  bool max_turnover_l1_bound{false};
  std::int64_t ppo_minibatch_size{0};
  std::int64_t ppo_epochs_per_rollout{0};
  bool ppo_minibatch_size_bound{false};
  bool ppo_epochs_per_rollout_bound{false};
  std::string training_schedule_mode{k_policy_training_schedule_mode_causal_v1};
  std::string causal_schedule_schema_id{
      k_policy_training_causal_schedule_schema_v1};
  std::string causal_schedule_digest{};
  std::string causal_schedule_cursor_key_kind{"numeric_anchor_index"};
  std::string causal_schedule_no_future_snapshot_use_source{
      k_policy_training_schedule_no_future_snapshot_source_v1};
  std::string causal_provenance_schema{k_causal_provenance_generalization_v1};
  std::string causal_atom_schema{k_causal_atom_anchor_interval_v1};
  std::string causal_interval_set_schema{
      k_causal_interval_set_anchor_half_open_v1};
  std::string causal_label_reward_horizon_policy_fingerprint{
      "label_reward_horizon.anchor_scalar_v1"};
  std::string causal_fold_policy_fingerprint{
      "fold_policy.single_time_split_v1"};
  std::string causal_purged_embargo_policy_fingerprint{};
  std::string causal_artifact_production_schema{
      k_runtime_artifact_production_inline_policy_v1};
  std::string causal_artifact_production_closure_digest{};
  std::string causal_interface_stability_contract_digest{
      k_interface_stability_trained_against_generation_vector_v1};
  std::string causal_provenance_closure_digest{};
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
  std::string parent_replay_environment_report_digest{};
  std::string final_refit_parent_selected_checkpoint_digest{};
  std::string policy_execution_no_lookahead_certificate_schema{};
  std::string policy_execution_no_lookahead_certificate_digest{};
  std::string policy_execution_evidence_snapshot_digest{};
  std::string policy_execution_provenance_closure_digest{};
  std::string policy_execution_target_id{};
  std::int64_t policy_execution_target_anchor_begin{0};
  std::int64_t policy_execution_target_anchor_end_exclusive{0};
  bool policy_execution_target_anchor_begin_bound{false};
  bool policy_execution_target_anchor_end_exclusive_bound{false};
  std::string policy_execution_no_lookahead_contract_digest{};
  std::string embargo_policy_fingerprint{};
  std::int64_t embargo_purged_window_anchor_begin{0};
  std::int64_t embargo_purged_window_anchor_end_exclusive{0};
  bool embargo_purged_window_anchor_range_bound{false};
  std::string policy_execution_input_parent_forecast_eval_fact_digest{};
  std::string policy_execution_input_parent_forecast_artifact_digest{};
  std::string policy_execution_input_parent_replay_environment_fact_digest{};
  std::string policy_execution_input_parent_replay_environment_report_digest{};
  std::string policy_execution_replay_job_dir_digest{};
  std::string policy_execution_consumed_artifact_digests{};
  std::string policy_execution_consumed_checkpoint_digests{};
  std::string policy_execution_consumed_generation_vector_digests{};
  std::string parent_policy_generation_id{};
  std::string parent_policy_generation_vector_digest{};
  std::string parent_policy_influence_schema{
      k_no_lookahead_artifact_provenance_anchor_schema_v1};
  bool parent_policy_influence_complete{false};
  std::int64_t parent_policy_influence_anchor_begin_min{0};
  std::int64_t parent_policy_influence_anchor_end_exclusive_max{0};
  bool parent_policy_influence_anchor_begin_min_bound{false};
  bool parent_policy_influence_anchor_end_exclusive_max_bound{false};
  std::int64_t parent_policy_label_or_reward_availability_end_exclusive_max{0};
  bool parent_policy_label_or_reward_availability_end_exclusive_max_bound{
      false};
  std::int64_t parent_policy_valid_from_anchor{0};
  bool parent_policy_valid_from_anchor_bound{false};
  std::string parent_policy_provenance_closure_digest{};
  std::string parent_policy_no_lookahead_contract_digest{};
  std::string policy_output_generation_id{};
  std::string policy_output_generation_vector_digest{};
  std::int64_t policy_output_fit_anchor_begin{0};
  std::int64_t policy_output_fit_anchor_end_exclusive{0};
  bool policy_output_fit_anchor_begin_bound{false};
  bool policy_output_fit_anchor_end_exclusive_bound{false};
  std::int64_t policy_output_valid_from_anchor{0};
  bool policy_output_valid_from_anchor_bound{false};
  std::string policy_output_provenance_closure_digest{};
  std::int64_t random_seed{0};
  bool random_seed_bound{false};
  std::int64_t max_episodes{0};
  std::int64_t max_steps{0};
  std::int64_t max_parallel_jobs{1};
  bool max_parallel_jobs_bound{false};
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
    out << "job_id=" << job_id << "\n";
    out << "job_dir=" << job_dir << "\n";
    out << "report_path=" << report_path << "\n";
    out << "replay_job_dir=" << replay_job_dir << "\n";
    out << "certified_replay_bank_schema=" << certified_replay_bank_schema
        << "\n";
    out << "certified_replay_bank_manifest_path="
        << certified_replay_bank_manifest_path << "\n";
    out << "certified_replay_bank_manifest_digest="
        << certified_replay_bank_manifest_digest << "\n";
    out << "policy_experience_set_digest=" << policy_experience_set_digest
        << "\n";
    out << "policy_experience_mode=" << policy_experience_mode << "\n";
    out << "certified_replay_chunk_count=" << certified_replay_chunk_count
        << "\n";
    out << "certified_replay_chunk_count_bound="
        << (certified_replay_chunk_count_bound ? "true" : "false") << "\n";
    out << "certified_replay_sample_count=" << certified_replay_sample_count
        << "\n";
    out << "certified_replay_sample_count_bound="
        << (certified_replay_sample_count_bound ? "true" : "false") << "\n";
    out << "certified_replay_coverage_anchor_begin="
        << certified_replay_coverage_anchor_begin << "\n";
    out << "certified_replay_coverage_anchor_end_exclusive="
        << certified_replay_coverage_anchor_end_exclusive << "\n";
    out << "certified_replay_coverage_anchor_range_bound="
        << (certified_replay_coverage_anchor_range_bound ? "true" : "false")
        << "\n";
    out << "config_path=" << config_path << "\n";
    out << "accounting_numeraire_node_id=" << accounting_numeraire_node_id
        << "\n";
    out << "target_node_ids=" << target_node_ids << "\n";
    out << "experiment_id=" << experiment_id << "\n";
    out << "policy_set_digest=" << policy_set_digest << "\n";
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
    out << "action_distribution_id=" << action_distribution_id << "\n";
    out << "reward_contract_id=" << reward_contract_id << "\n";
    out << "execution_profile_digest=" << execution_profile_digest << "\n";
    out << "ppo_policy_artifact_contract_id=" << ppo_policy_artifact_contract_id
        << "\n";
    out << "policy_family_id=" << policy_family_id << "\n";
    out << "policy_checkpoint_schema_id=" << policy_checkpoint_schema_id
        << "\n";
    out << "policy_dsl_digest=" << policy_dsl_digest << "\n";
    out << "policy_net_digest=" << policy_net_digest << "\n";
    out << "policy_input_feature_manifest_digest="
        << policy_input_feature_manifest_digest << "\n";
    out << "policy_jkimyei_digest=" << policy_jkimyei_digest << "\n";
    out << "policy_jkimyei_no_lookahead_contract_id="
        << policy_jkimyei_no_lookahead_contract_id << "\n";
    out << "policy_jkimyei_no_lookahead_contract_digest="
        << policy_jkimyei_no_lookahead_contract_digest << "\n";
    out << "policy_jkimyei_component_order_contract_id="
        << policy_jkimyei_component_order_contract_id << "\n";
    out << "policy_jkimyei_component_order_contract_digest="
        << policy_jkimyei_component_order_contract_digest << "\n";
    out << "policy_jkimyei_component_role=" << policy_jkimyei_component_role
        << "\n";
    out << "policy_jkimyei_serving_order_index="
        << policy_jkimyei_serving_order_index << "\n";
    out << "policy_jkimyei_serving_order_index_bound="
        << (policy_jkimyei_serving_order_index_bound ? "true" : "false")
        << "\n";
    out << "target_node_universe_digest=" << target_node_universe_digest
        << "\n";
    out << "action_distribution_config_digest="
        << action_distribution_config_digest << "\n";
    out << "snapshot_family_digest=" << snapshot_family_digest << "\n";
    out << "snapshot_bundle_schema=" << snapshot_bundle_schema << "\n";
    out << "snapshot_bundle_id=" << snapshot_bundle_id << "\n";
    out << "snapshot_bundle_kind=" << snapshot_bundle_kind << "\n";
    out << "snapshot_bundle_selection_basis=" << snapshot_bundle_selection_basis
        << "\n";
    out << "snapshot_bundle_no_lookahead_contract_id="
        << snapshot_bundle_no_lookahead_contract_id << "\n";
    out << "snapshot_bundle_no_lookahead_contract_digest="
        << snapshot_bundle_no_lookahead_contract_digest << "\n";
    out << "snapshot_bundle_component_order_contract_id="
        << snapshot_bundle_component_order_contract_id << "\n";
    out << "snapshot_bundle_component_order_contract_digest="
        << snapshot_bundle_component_order_contract_digest << "\n";
    out << "snapshot_bundle_protocol_contract_fingerprint="
        << snapshot_bundle_protocol_contract_fingerprint << "\n";
    out << "snapshot_bundle_generation_vector_digest="
        << snapshot_bundle_generation_vector_digest << "\n";
    out << "snapshot_bundle_valid_from_anchor="
        << snapshot_bundle_valid_from_anchor << "\n";
    out << "snapshot_bundle_valid_from_anchor_bound="
        << (snapshot_bundle_valid_from_anchor_bound ? "true" : "false") << "\n";
    out << "snapshot_bundle_valid_until_anchor="
        << snapshot_bundle_valid_until_anchor << "\n";
    out << "snapshot_bundle_valid_until_anchor_bound="
        << (snapshot_bundle_valid_until_anchor_bound ? "true" : "false")
        << "\n";
    out << "snapshot_bundle_influence_anchor_end_exclusive_max="
        << snapshot_bundle_influence_anchor_end_exclusive_max << "\n";
    out << "snapshot_bundle_influence_anchor_end_exclusive_max_bound="
        << (snapshot_bundle_influence_anchor_end_exclusive_max_bound ? "true"
                                                                     : "false")
        << "\n";
    out << "snapshot_bundle_compatibility_closure_digest="
        << snapshot_bundle_compatibility_closure_digest << "\n";
    out << "snapshot_bundle_no_lookahead_certificate_digests="
        << snapshot_bundle_no_lookahead_certificate_digests << "\n";
    out << "snapshot_bundle_policy_execution_certificate_digest="
        << snapshot_bundle_policy_execution_certificate_digest << "\n";
    out << "snapshot_bundle_created_by_lattice_target="
        << snapshot_bundle_created_by_lattice_target << "\n";
    out << "bundle_representation_component_id="
        << bundle_representation_component_id << "\n";
    out << "bundle_representation_jkimyei_digest="
        << bundle_representation_jkimyei_digest << "\n";
    out << "bundle_representation_checkpoint_digest="
        << bundle_representation_checkpoint_digest << "\n";
    out << "bundle_representation_generation_id="
        << bundle_representation_generation_id << "\n";
    out << "bundle_representation_generation_vector_digest="
        << bundle_representation_generation_vector_digest << "\n";
    out << "bundle_representation_generation_manifest_digest="
        << bundle_representation_generation_manifest_digest << "\n";
    out << "bundle_mdn_component_id=" << bundle_mdn_component_id << "\n";
    out << "bundle_mdn_jkimyei_digest=" << bundle_mdn_jkimyei_digest << "\n";
    out << "bundle_mdn_checkpoint_digest=" << bundle_mdn_checkpoint_digest
        << "\n";
    out << "bundle_mdn_generation_id=" << bundle_mdn_generation_id << "\n";
    out << "bundle_mdn_generation_vector_digest="
        << bundle_mdn_generation_vector_digest << "\n";
    out << "bundle_mdn_generation_manifest_digest="
        << bundle_mdn_generation_manifest_digest << "\n";
    out << "bundle_policy_component_id=" << bundle_policy_component_id << "\n";
    out << "bundle_policy_jkimyei_digest=" << bundle_policy_jkimyei_digest
        << "\n";
    out << "bundle_policy_checkpoint_digest=" << bundle_policy_checkpoint_digest
        << "\n";
    out << "bundle_policy_generation_id=" << bundle_policy_generation_id
        << "\n";
    out << "bundle_policy_generation_vector_digest="
        << bundle_policy_generation_vector_digest << "\n";
    out << "bundle_policy_generation_manifest_digest="
        << bundle_policy_generation_manifest_digest << "\n";
    out << "mdn_trained_against_representation_generation_id="
        << mdn_trained_against_representation_generation_id << "\n";
    out << "mdn_trained_against_representation_checkpoint_digest="
        << mdn_trained_against_representation_checkpoint_digest << "\n";
    out << "mdn_trained_against_representation_generation_vector_digest="
        << mdn_trained_against_representation_generation_vector_digest << "\n";
    out << "mdn_representation_feature_schema_digest="
        << mdn_representation_feature_schema_digest << "\n";
    out << "policy_trained_against_forecast_eval_fact_digest="
        << policy_trained_against_forecast_eval_fact_digest << "\n";
    out << "policy_trained_against_replay_environment_fact_digest="
        << policy_trained_against_replay_environment_fact_digest << "\n";
    out << "policy_trained_against_representation_generation_vector_digest="
        << policy_trained_against_representation_generation_vector_digest
        << "\n";
    out << "policy_trained_against_mdn_generation_vector_digest="
        << policy_trained_against_mdn_generation_vector_digest << "\n";
    out << "policy_trained_against_policy_parent_generation_vector_digest="
        << policy_trained_against_policy_parent_generation_vector_digest
        << "\n";
    out << "policy_execution_input_lock_digest="
        << policy_execution_input_lock_digest << "\n";
    out << "policy_trained_against_no_lookahead_certificate_digest="
        << policy_trained_against_no_lookahead_certificate_digest << "\n";
    out << "actor_architecture_digest=" << actor_architecture_digest << "\n";
    out << "critic_architecture_digest=" << critic_architecture_digest << "\n";
    out << "actor_checkpoint_digest=" << actor_checkpoint_digest << "\n";
    out << "critic_checkpoint_digest=" << critic_checkpoint_digest << "\n";
    out << "optimizer_state_digest=" << optimizer_state_digest << "\n";
    out << "ppo_config_digest=" << ppo_config_digest << "\n";
    out << "optimizer_state_schema_id=" << optimizer_state_schema_id << "\n";
    out << "device_policy=" << device_policy << "\n";
    out << "cuda_device_index=" << cuda_device_index << "\n";
    out << "cuda_device_index_bound="
        << (cuda_device_index_bound ? "true" : "false") << "\n";
    out << "resume_mode=" << resume_mode << "\n";
    out << "resume_actor_checkpoint_path=" << resume_actor_checkpoint_path
        << "\n";
    out << "resume_actor_checkpoint_digest=" << resume_actor_checkpoint_digest
        << "\n";
    out << "resume_optimizer_state_path=" << resume_optimizer_state_path
        << "\n";
    out << "resume_optimizer_state_digest=" << resume_optimizer_state_digest
        << "\n";
    out << "advantage_estimator_id=" << advantage_estimator_id << "\n";
    out << "advantage_normalization_policy=" << advantage_normalization_policy
        << "\n";
    out << "rollout_collection_schema_id=" << rollout_collection_schema_id
        << "\n";
    out << "rollout_collection_digest=" << rollout_collection_digest << "\n";
    out << "ppo_update_report_schema_id=" << ppo_update_report_schema_id
        << "\n";
    out << "ppo_update_report_digest=" << ppo_update_report_digest << "\n";
    out << "validation_rollout_report_digest="
        << validation_rollout_report_digest << "\n";
    out << "policy_quality_report_digest=" << policy_quality_report_digest
        << "\n";
    out << "ppo_gamma=" << ppo_gamma << "\n";
    out << "ppo_gamma_bound=" << (ppo_gamma_bound ? "true" : "false") << "\n";
    out << "ppo_gae_lambda=" << ppo_gae_lambda << "\n";
    out << "ppo_gae_lambda_bound=" << (ppo_gae_lambda_bound ? "true" : "false")
        << "\n";
    out << "ppo_clip_epsilon=" << ppo_clip_epsilon << "\n";
    out << "ppo_clip_epsilon_bound="
        << (ppo_clip_epsilon_bound ? "true" : "false") << "\n";
    out << "ppo_target_kl=" << ppo_target_kl << "\n";
    out << "ppo_target_kl_bound=" << (ppo_target_kl_bound ? "true" : "false")
        << "\n";
    out << "ppo_learning_rate=" << ppo_learning_rate << "\n";
    out << "ppo_learning_rate_bound="
        << (ppo_learning_rate_bound ? "true" : "false") << "\n";
    out << "ppo_entropy_coeff=" << ppo_entropy_coeff << "\n";
    out << "ppo_entropy_coeff_bound="
        << (ppo_entropy_coeff_bound ? "true" : "false") << "\n";
    out << "ppo_value_loss_coeff=" << ppo_value_loss_coeff << "\n";
    out << "ppo_value_loss_coeff_bound="
        << (ppo_value_loss_coeff_bound ? "true" : "false") << "\n";
    out << "ppo_max_grad_norm=" << ppo_max_grad_norm << "\n";
    out << "ppo_max_grad_norm_bound="
        << (ppo_max_grad_norm_bound ? "true" : "false") << "\n";
    out << "linear_transaction_cost_rate=" << linear_transaction_cost_rate
        << "\n";
    out << "linear_transaction_cost_rate_bound="
        << (linear_transaction_cost_rate_bound ? "true" : "false") << "\n";
    out << "initial_equity_numeraire=" << initial_equity_numeraire << "\n";
    out << "initial_equity_numeraire_bound="
        << (initial_equity_numeraire_bound ? "true" : "false") << "\n";
    out << "max_node_weight=" << max_node_weight << "\n";
    out << "max_node_weight_bound="
        << (max_node_weight_bound ? "true" : "false") << "\n";
    out << "max_turnover_l1=" << max_turnover_l1 << "\n";
    out << "max_turnover_l1_bound="
        << (max_turnover_l1_bound ? "true" : "false") << "\n";
    out << "ppo_minibatch_size=" << ppo_minibatch_size << "\n";
    out << "ppo_minibatch_size_bound="
        << (ppo_minibatch_size_bound ? "true" : "false") << "\n";
    out << "ppo_epochs_per_rollout=" << ppo_epochs_per_rollout << "\n";
    out << "ppo_epochs_per_rollout_bound="
        << (ppo_epochs_per_rollout_bound ? "true" : "false") << "\n";
    out << "training_schedule_mode=" << training_schedule_mode << "\n";
    out << "causal_schedule_schema_id=" << causal_schedule_schema_id << "\n";
    out << "causal_schedule_digest=" << causal_schedule_digest << "\n";
    out << "causal_schedule_cursor_key_kind=" << causal_schedule_cursor_key_kind
        << "\n";
    out << "causal_schedule_no_future_snapshot_use_source="
        << causal_schedule_no_future_snapshot_use_source << "\n";
    out << "causal_provenance_schema=" << causal_provenance_schema << "\n";
    out << "causal_atom_schema=" << causal_atom_schema << "\n";
    out << "causal_interval_set_schema=" << causal_interval_set_schema << "\n";
    out << "causal_label_reward_horizon_policy_fingerprint="
        << causal_label_reward_horizon_policy_fingerprint << "\n";
    out << "causal_fold_policy_fingerprint=" << causal_fold_policy_fingerprint
        << "\n";
    out << "causal_purged_embargo_policy_fingerprint="
        << causal_purged_embargo_policy_fingerprint << "\n";
    out << "causal_artifact_production_schema="
        << causal_artifact_production_schema << "\n";
    out << "causal_artifact_production_closure_digest="
        << causal_artifact_production_closure_digest << "\n";
    out << "causal_interface_stability_contract_digest="
        << causal_interface_stability_contract_digest << "\n";
    out << "causal_provenance_closure_digest="
        << causal_provenance_closure_digest << "\n";
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
    out << "parent_replay_environment_report_digest="
        << parent_replay_environment_report_digest << "\n";
    out << "final_refit_parent_selected_checkpoint_digest="
        << final_refit_parent_selected_checkpoint_digest << "\n";
    out << "policy_execution_no_lookahead_certificate_schema="
        << policy_execution_no_lookahead_certificate_schema << "\n";
    out << "policy_execution_no_lookahead_certificate_digest="
        << policy_execution_no_lookahead_certificate_digest << "\n";
    out << "policy_execution_evidence_snapshot_digest="
        << policy_execution_evidence_snapshot_digest << "\n";
    out << "policy_execution_provenance_closure_digest="
        << policy_execution_provenance_closure_digest << "\n";
    out << "policy_execution_target_id=" << policy_execution_target_id << "\n";
    out << "policy_execution_target_anchor_begin="
        << policy_execution_target_anchor_begin << "\n";
    out << "policy_execution_target_anchor_begin_bound="
        << (policy_execution_target_anchor_begin_bound ? "true" : "false")
        << "\n";
    out << "policy_execution_target_anchor_end_exclusive="
        << policy_execution_target_anchor_end_exclusive << "\n";
    out << "policy_execution_target_anchor_end_exclusive_bound="
        << (policy_execution_target_anchor_end_exclusive_bound ? "true"
                                                               : "false")
        << "\n";
    out << "policy_execution_no_lookahead_contract_digest="
        << policy_execution_no_lookahead_contract_digest << "\n";
    out << "embargo_policy_fingerprint=" << embargo_policy_fingerprint << "\n";
    out << "embargo_purged_window_anchor_begin="
        << embargo_purged_window_anchor_begin << "\n";
    out << "embargo_purged_window_anchor_end_exclusive="
        << embargo_purged_window_anchor_end_exclusive << "\n";
    out << "embargo_purged_window_anchor_range_bound="
        << (embargo_purged_window_anchor_range_bound ? "true" : "false")
        << "\n";
    out << "policy_execution_input_parent_forecast_eval_fact_digest="
        << policy_execution_input_parent_forecast_eval_fact_digest << "\n";
    out << "policy_execution_input_parent_forecast_artifact_digest="
        << policy_execution_input_parent_forecast_artifact_digest << "\n";
    out << "policy_execution_input_parent_replay_environment_fact_digest="
        << policy_execution_input_parent_replay_environment_fact_digest << "\n";
    out << "policy_execution_input_parent_replay_environment_report_digest="
        << policy_execution_input_parent_replay_environment_report_digest
        << "\n";
    out << "policy_execution_replay_job_dir_digest="
        << policy_execution_replay_job_dir_digest << "\n";
    out << "policy_execution_consumed_artifact_digests="
        << policy_execution_consumed_artifact_digests << "\n";
    out << "policy_execution_consumed_checkpoint_digests="
        << policy_execution_consumed_checkpoint_digests << "\n";
    out << "policy_execution_consumed_generation_vector_digests="
        << policy_execution_consumed_generation_vector_digests << "\n";
    out << "parent_policy_generation_id=" << parent_policy_generation_id
        << "\n";
    out << "parent_policy_generation_vector_digest="
        << parent_policy_generation_vector_digest << "\n";
    out << "parent_policy_influence_schema=" << parent_policy_influence_schema
        << "\n";
    out << "parent_policy_influence_complete="
        << (parent_policy_influence_complete ? "true" : "false") << "\n";
    out << "parent_policy_influence_anchor_begin_min="
        << parent_policy_influence_anchor_begin_min << "\n";
    out << "parent_policy_influence_anchor_begin_min_bound="
        << (parent_policy_influence_anchor_begin_min_bound ? "true" : "false")
        << "\n";
    out << "parent_policy_influence_anchor_end_exclusive_max="
        << parent_policy_influence_anchor_end_exclusive_max << "\n";
    out << "parent_policy_influence_anchor_end_exclusive_max_bound="
        << (parent_policy_influence_anchor_end_exclusive_max_bound ? "true"
                                                                   : "false")
        << "\n";
    out << "parent_policy_label_or_reward_availability_end_exclusive_max="
        << parent_policy_label_or_reward_availability_end_exclusive_max << "\n";
    out << "parent_policy_label_or_reward_availability_end_exclusive_max_bound="
        << (parent_policy_label_or_reward_availability_end_exclusive_max_bound
                ? "true"
                : "false")
        << "\n";
    out << "parent_policy_valid_from_anchor=" << parent_policy_valid_from_anchor
        << "\n";
    out << "parent_policy_valid_from_anchor_bound="
        << (parent_policy_valid_from_anchor_bound ? "true" : "false") << "\n";
    out << "parent_policy_provenance_closure_digest="
        << parent_policy_provenance_closure_digest << "\n";
    out << "parent_policy_no_lookahead_contract_digest="
        << parent_policy_no_lookahead_contract_digest << "\n";
    out << "policy_output_generation_id=" << policy_output_generation_id
        << "\n";
    out << "policy_output_generation_vector_digest="
        << policy_output_generation_vector_digest << "\n";
    out << "policy_output_fit_anchor_begin=" << policy_output_fit_anchor_begin
        << "\n";
    out << "policy_output_fit_anchor_begin_bound="
        << (policy_output_fit_anchor_begin_bound ? "true" : "false") << "\n";
    out << "policy_output_fit_anchor_end_exclusive="
        << policy_output_fit_anchor_end_exclusive << "\n";
    out << "policy_output_fit_anchor_end_exclusive_bound="
        << (policy_output_fit_anchor_end_exclusive_bound ? "true" : "false")
        << "\n";
    out << "policy_output_valid_from_anchor=" << policy_output_valid_from_anchor
        << "\n";
    out << "policy_output_valid_from_anchor_bound="
        << (policy_output_valid_from_anchor_bound ? "true" : "false") << "\n";
    out << "policy_output_provenance_closure_digest="
        << policy_output_provenance_closure_digest << "\n";
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

[[nodiscard]] inline std::string canonical_policy_operator_surface_text(
    const policy_training_job_contract_t &contract) {
  std::ostringstream out;
  out << "schema=policy_operator_surface_and_identity_standardization.v1\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_dsl_digest=" << contract.policy_dsl_digest << "\n";
  out << "policy_net_digest=" << contract.policy_net_digest << "\n";
  out << "policy_input_feature_manifest_digest="
      << contract.policy_input_feature_manifest_digest << "\n";
  out << "policy_jkimyei_digest=" << contract.policy_jkimyei_digest << "\n";
  out << "policy_jkimyei_no_lookahead_contract_id="
      << contract.policy_jkimyei_no_lookahead_contract_id << "\n";
  out << "policy_jkimyei_no_lookahead_contract_digest="
      << contract.policy_jkimyei_no_lookahead_contract_digest << "\n";
  out << "policy_jkimyei_component_order_contract_id="
      << contract.policy_jkimyei_component_order_contract_id << "\n";
  out << "policy_jkimyei_component_order_contract_digest="
      << contract.policy_jkimyei_component_order_contract_digest << "\n";
  out << "policy_jkimyei_component_role="
      << contract.policy_jkimyei_component_role << "\n";
  out << "policy_jkimyei_serving_order_index="
      << contract.policy_jkimyei_serving_order_index << "\n";
  out << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
  out << "action_schema_digest=" << contract.action_schema_digest << "\n";
  out << "action_adapter_id=" << contract.action_adapter_id << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "action_distribution_config_digest="
      << contract.action_distribution_config_digest << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "reward_contract_digest=" << contract.reward_contract_digest << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "training_schedule_mode=" << contract.training_schedule_mode << "\n";
  out << "causal_schedule_schema_id=" << contract.causal_schedule_schema_id
      << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "causal_schedule_cursor_key_kind="
      << contract.causal_schedule_cursor_key_kind << "\n";
  out << "graph_order_fingerprint=" << contract.graph_order_fingerprint << "\n";
  out << "target_node_universe_digest=" << contract.target_node_universe_digest
      << "\n";
  out << "ppo_policy_artifact_contract_id="
      << contract.ppo_policy_artifact_contract_id << "\n";
  out << "policy_checkpoint_schema_id=" << contract.policy_checkpoint_schema_id
      << "\n";
  return out.str();
}

[[nodiscard]] inline std::string
policy_operator_surface_digest(const policy_training_job_contract_t &contract) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.runtime.policy_operator_surface.digest.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, canonical_policy_operator_surface_text(contract));
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

[[nodiscard]] inline bool
policy_training_action_distribution_supported(const std::string &value) {
  return value == k_masked_dirichlet_simplex_distribution_v1 ||
         value == k_masked_logistic_normal_simplex_distribution_v1;
}

[[nodiscard]] inline bool
policy_training_policy_kind_mentions_ppo(const std::string &value) {
  return value.find("ppo") != std::string::npos ||
         value.find("PPO") != std::string::npos;
}

[[nodiscard]] inline bool policy_training_requires_ppo_artifact_contract(
    const policy_training_job_contract_t &contract) {
  return policy_training_policy_kind_mentions_ppo(contract.policy_kind) ||
         !contract.ppo_policy_artifact_contract_id.empty();
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
  require_nonempty(contract.action_distribution_id, "action_distribution_id");
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
  require_nonempty(contract.causal_provenance_schema,
                   "causal_provenance_schema");
  require_nonempty(contract.causal_atom_schema, "causal_atom_schema");
  require_nonempty(contract.causal_interval_set_schema,
                   "causal_interval_set_schema");
  require_nonempty(contract.causal_label_reward_horizon_policy_fingerprint,
                   "causal_label_reward_horizon_policy_fingerprint");
  require_nonempty(contract.causal_fold_policy_fingerprint,
                   "causal_fold_policy_fingerprint");
  require_nonempty(contract.causal_purged_embargo_policy_fingerprint,
                   "causal_purged_embargo_policy_fingerprint");
  require_nonempty(contract.causal_artifact_production_schema,
                   "causal_artifact_production_schema");
  require_nonempty(contract.causal_interface_stability_contract_digest,
                   "causal_interface_stability_contract_digest");
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
  if (!policy_kind_is_ppo_adapter(contract.policy_kind,
                                  contract.ppo_policy_artifact_contract_id)) {
    require_nonempty(contract.parent_allocation_engine_fact_digest,
                     "parent_allocation_engine_fact_digest");
  }
  if (!policy_kind_is_ppo_adapter(contract.policy_kind,
                                  contract.ppo_policy_artifact_contract_id) &&
      contract.parent_replay_environment_fact_digest.empty() &&
      contract.parent_replay_environment_report_digest.empty()) {
    issues.emplace_back(
        "missing_parent_replay_environment_fact_or_report_digest");
  }

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
  if (!contract.action_distribution_id.empty() &&
      !policy_training_action_distribution_supported(
          contract.action_distribution_id)) {
    issues.emplace_back("unsupported_action_distribution_id");
  }
  if (!contract.reward_contract_id.empty() &&
      contract.reward_contract_id != k_post_execution_reward_contract_v1) {
    issues.emplace_back("unsupported_reward_contract_id");
  }
  if (policy_training_requires_ppo_artifact_contract(contract)) {
    require_nonempty(contract.ppo_policy_artifact_contract_id,
                     "ppo_policy_artifact_contract_id");
    require_nonempty(contract.policy_family_id, "policy_family_id");
    require_nonempty(contract.policy_checkpoint_schema_id,
                     "policy_checkpoint_schema_id");
    require_nonempty(contract.policy_dsl_digest, "policy_dsl_digest");
    require_nonempty(contract.policy_net_digest, "policy_net_digest");
    require_nonempty(contract.policy_input_feature_manifest_digest,
                     "policy_input_feature_manifest_digest");
    require_nonempty(contract.policy_jkimyei_digest, "policy_jkimyei_digest");
    require_nonempty(contract.policy_jkimyei_no_lookahead_contract_id,
                     "policy_jkimyei_no_lookahead_contract_id");
    require_nonempty(contract.policy_jkimyei_no_lookahead_contract_digest,
                     "policy_jkimyei_no_lookahead_contract_digest");
    require_nonempty(contract.policy_jkimyei_component_order_contract_id,
                     "policy_jkimyei_component_order_contract_id");
    require_nonempty(contract.policy_jkimyei_component_order_contract_digest,
                     "policy_jkimyei_component_order_contract_digest");
    require_nonempty(contract.policy_jkimyei_component_role,
                     "policy_jkimyei_component_role");
    if (contract.policy_jkimyei_no_lookahead_contract_id !=
            contract.policy_jkimyei_component_order_contract_id ||
        contract.policy_jkimyei_no_lookahead_contract_digest !=
            contract.policy_jkimyei_component_order_contract_digest ||
        (!contract.policy_execution_no_lookahead_contract_digest.empty() &&
         contract.policy_jkimyei_no_lookahead_contract_digest !=
             contract.policy_execution_no_lookahead_contract_digest) ||
        (!contract.parent_policy_no_lookahead_contract_digest.empty() &&
         contract.policy_jkimyei_no_lookahead_contract_digest !=
             contract.parent_policy_no_lookahead_contract_digest)) {
      issues.emplace_back("policy_jkimyei_contract_ref_mismatch");
    }
    if (contract.policy_jkimyei_component_role != "policy") {
      issues.emplace_back("policy_jkimyei_component_role_mismatch");
    }
    if (!contract.policy_jkimyei_serving_order_index_bound ||
        contract.policy_jkimyei_serving_order_index != 2) {
      issues.emplace_back("policy_jkimyei_serving_order_index_mismatch");
    }
    require_nonempty(contract.target_node_universe_digest,
                     "target_node_universe_digest");
    require_nonempty(contract.action_distribution_config_digest,
                     "action_distribution_config_digest");
    require_nonempty(contract.snapshot_family_digest, "snapshot_family_digest");
    require_nonempty(contract.actor_architecture_digest,
                     "actor_architecture_digest");
    require_nonempty(contract.critic_architecture_digest,
                     "critic_architecture_digest");
    require_nonempty(contract.ppo_config_digest, "ppo_config_digest");
    require_nonempty(contract.optimizer_state_schema_id,
                     "optimizer_state_schema_id");
    require_nonempty(contract.device_policy, "device_policy");
    require_nonempty(contract.resume_mode, "resume_mode");
    require_nonempty(contract.advantage_estimator_id, "advantage_estimator_id");
    require_nonempty(contract.advantage_normalization_policy,
                     "advantage_normalization_policy");
    require_nonempty(contract.rollout_collection_schema_id,
                     "rollout_collection_schema_id");
    require_nonempty(contract.ppo_update_report_schema_id,
                     "ppo_update_report_schema_id");
    if (!contract.ppo_policy_artifact_contract_id.empty() &&
        contract.ppo_policy_artifact_contract_id !=
            k_ppo_policy_artifact_contract_v1) {
      issues.emplace_back("unsupported_ppo_policy_artifact_contract_id");
    }
    if (!contract.policy_family_id.empty() &&
        contract.policy_family_id != k_graph_node_allocation_policy_family_v1) {
      issues.emplace_back("unsupported_policy_family_id");
    }
    if (!contract.policy_checkpoint_schema_id.empty() &&
        contract.policy_checkpoint_schema_id !=
            k_ppo_policy_checkpoint_schema_v1) {
      issues.emplace_back("unsupported_policy_checkpoint_schema_id");
    }
    if (!contract.rollout_collection_schema_id.empty() &&
        contract.rollout_collection_schema_id !=
            k_ppo_rollout_collection_schema_v1) {
      issues.emplace_back("unsupported_rollout_collection_schema_id");
    }
    if (!contract.ppo_update_report_schema_id.empty() &&
        contract.ppo_update_report_schema_id != k_ppo_update_report_schema_v1) {
      issues.emplace_back("unsupported_ppo_update_report_schema_id");
    }
    if (!contract.optimizer_state_schema_id.empty() &&
        contract.optimizer_state_schema_id !=
            "kikijyeba.runtime.ppo_optimizer_state.v1") {
      issues.emplace_back("unsupported_optimizer_state_schema_id");
    }
    if (!contract.device_policy.empty() &&
        contract.device_policy != "require_cuda") {
      issues.emplace_back("unsupported_device_policy");
    }
    if (contract.cuda_device_index_bound && contract.cuda_device_index != 0) {
      issues.emplace_back("unsupported_cuda_device_index");
    }
    if (!contract.resume_mode.empty() &&
        contract.resume_mode != "fresh_spawn" &&
        contract.resume_mode != "resume_weights" &&
        contract.resume_mode != "resume_weights_and_optimizer") {
      issues.emplace_back("unsupported_resume_mode");
    }
    if (contract.resume_mode == "fresh_spawn") {
      if (!contract.resume_actor_checkpoint_path.empty() ||
          !contract.resume_actor_checkpoint_digest.empty() ||
          !contract.resume_optimizer_state_path.empty() ||
          !contract.resume_optimizer_state_digest.empty()) {
        issues.emplace_back("fresh_spawn_must_not_bind_resume_artifacts");
      }
    }
    if (contract.resume_mode == "resume_weights" ||
        contract.resume_mode == "resume_weights_and_optimizer") {
      if (contract.resume_actor_checkpoint_path.empty()) {
        issues.emplace_back("missing_resume_actor_checkpoint_path");
      }
      if (contract.resume_actor_checkpoint_digest.empty()) {
        issues.emplace_back("missing_resume_actor_checkpoint_digest");
      }
    }
    if (contract.resume_mode == "resume_weights_and_optimizer") {
      if (contract.resume_optimizer_state_path.empty()) {
        issues.emplace_back("missing_resume_optimizer_state_path");
      }
      if (contract.resume_optimizer_state_digest.empty()) {
        issues.emplace_back("missing_resume_optimizer_state_digest");
      }
    }
    if (!contract.advantage_estimator_id.empty() &&
        contract.advantage_estimator_id != k_gae_advantage_estimator_v1) {
      issues.emplace_back("unsupported_advantage_estimator_id");
    }
    if (!contract.advantage_normalization_policy.empty() &&
        contract.advantage_normalization_policy !=
            k_ppo_advantage_normalization_policy_v1) {
      issues.emplace_back("unsupported_advantage_normalization_policy");
    }
    if (!contract.ppo_gamma_bound || !std::isfinite(contract.ppo_gamma) ||
        contract.ppo_gamma <= 0.0 || contract.ppo_gamma > 1.0) {
      issues.emplace_back("invalid_ppo_gamma");
    }
    if (!contract.ppo_gae_lambda_bound ||
        !std::isfinite(contract.ppo_gae_lambda) ||
        contract.ppo_gae_lambda < 0.0 || contract.ppo_gae_lambda > 1.0) {
      issues.emplace_back("invalid_ppo_gae_lambda");
    }
    if (!contract.ppo_clip_epsilon_bound ||
        !std::isfinite(contract.ppo_clip_epsilon) ||
        contract.ppo_clip_epsilon <= 0.0) {
      issues.emplace_back("invalid_ppo_clip_epsilon");
    }
    if (!contract.ppo_target_kl_bound ||
        !std::isfinite(contract.ppo_target_kl) ||
        contract.ppo_target_kl <= 0.0) {
      issues.emplace_back("invalid_ppo_target_kl");
    }
    if (contract.ppo_learning_rate_bound &&
        (!std::isfinite(contract.ppo_learning_rate) ||
         contract.ppo_learning_rate <= 0.0)) {
      issues.emplace_back("invalid_ppo_learning_rate");
    }
    if (!contract.ppo_entropy_coeff_bound ||
        !std::isfinite(contract.ppo_entropy_coeff) ||
        contract.ppo_entropy_coeff < 0.0) {
      issues.emplace_back("invalid_ppo_entropy_coeff");
    }
    if (!contract.ppo_value_loss_coeff_bound ||
        !std::isfinite(contract.ppo_value_loss_coeff) ||
        contract.ppo_value_loss_coeff < 0.0) {
      issues.emplace_back("invalid_ppo_value_loss_coeff");
    }
    if (!contract.ppo_max_grad_norm_bound ||
        !std::isfinite(contract.ppo_max_grad_norm) ||
        contract.ppo_max_grad_norm <= 0.0) {
      issues.emplace_back("invalid_ppo_max_grad_norm");
    }
    if (!contract.ppo_minibatch_size_bound ||
        contract.ppo_minibatch_size <= 0) {
      issues.emplace_back("invalid_ppo_minibatch_size");
    }
    if (!contract.ppo_epochs_per_rollout_bound ||
        contract.ppo_epochs_per_rollout <= 0) {
      issues.emplace_back("invalid_ppo_epochs_per_rollout");
    }
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
  if (contract.causal_provenance_schema !=
      k_causal_provenance_generalization_v1) {
    issues.emplace_back("unsupported_causal_provenance_schema");
  }
  if (contract.causal_atom_schema != k_causal_atom_anchor_interval_v1) {
    issues.emplace_back("unsupported_causal_atom_schema");
  }
  if (contract.causal_interval_set_schema !=
      k_causal_interval_set_anchor_half_open_v1) {
    issues.emplace_back("unsupported_causal_interval_set_schema");
  }
  if (contract.causal_artifact_production_schema !=
      k_runtime_artifact_production_inline_policy_v1) {
    issues.emplace_back("unsupported_causal_artifact_production_schema");
  }
  if (contract.causal_interface_stability_contract_digest !=
      k_interface_stability_trained_against_generation_vector_v1) {
    issues.emplace_back(
        "unsupported_causal_interface_stability_contract_digest");
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
