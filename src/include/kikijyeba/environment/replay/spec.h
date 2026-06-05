// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::environment {

struct replay_environment_spec_t {
  std::string version_token{"kikijyeba.environment.replay.v1"};
  std::string component_assembly_id{"replay_environment_v1"};
  std::string world_mode{"historical_replay"};
  std::string api_contract{"rl_compatible_reset_step"};
  std::string spawn_model{"episode_parallel_step_sequential"};
  std::string range_source{"ujcamei_component_stream_cursor"};
  std::string source_range_policy{"anchor_index_or_source_key"};
  std::string source_order_policy{"sequential"};
  std::string range_resolution{"runtime_resolved_cursor_identity"};
  std::string observation_time_law{"time_t_only"};
  std::string realization_reveal{"after_action_execution"};
  std::string realization_key_policy{"shared_key_per_frame"};
  std::string action_kind{"target_node_weights_with_base_reserve"};
  std::string action_schema_id{
      "kikijyeba.environment.action.target_weights.v1"};
  std::string action_time_policy{
      "decision_timestamp_after_knowledge_before_realization"};
  std::string reserve_node_policy{"graph_node_from_base_policy"};
  std::string graph_node_universe_policy{"episode_spec_graph_node_ids"};
  std::string reward_policy{
      "post_execution_ledger_log_growth_drawdown_cost_turnover_invalid"};
  std::string projection_validation{
      "projected_log_return_vs_realized_asset_base_return"};
  std::string realized_return_truth{"direct_edge_realized_return_truth_v1"};
  std::string policy_surface{"policy_adapter"};
  std::string action_policy_identity{"policy_adapter_must_match_action"};
  std::string initial_policy_kind{"deterministic_allocator_or_baseline"};
  std::string experiment_task_identity{"bundle_policy_task_indices"};
  std::string experiment_run_identity{"single_runtime_environment_run"};
  std::string step_artifact_identity{"episode_run_policy_cursor"};
  std::string experiment_report_count_policy{"counts_match_evidence"};
  std::string artifact_schema{"cajtucu_ready_replay_artifacts"};
  std::string lattice_fact_family{"replay_environment"};
  std::string lattice_target{"replay_environment_artifact_ready"};
  bool require_resolved_cursor{true};
  bool require_no_future_leakage{true};
  bool require_projection_validation{true};
  bool replay_executor{false};
  bool allocation_authority{false};
  bool execution_authority{false};
  bool live_capital_allowed{false};
  std::int64_t default_max_parallel_jobs{1};
};

inline void
validate_replay_environment_spec(const replay_environment_spec_t &spec) {
  if (spec.version_token != "kikijyeba.environment.replay.v1") {
    throw std::runtime_error("[replay_environment_spec] unsupported VERSION");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error(
        "[replay_environment_spec] COMPONENT_ASSEMBLY_ID is required");
  }
  if (spec.world_mode != "historical_replay") {
    throw std::runtime_error(
        "[replay_environment_spec] V1 is historical_replay only");
  }
  if (spec.api_contract != "rl_compatible_reset_step") {
    throw std::runtime_error("[replay_environment_spec] API_CONTRACT must be "
                             "rl_compatible_reset_step");
  }
  if (spec.spawn_model != "episode_parallel_step_sequential") {
    throw std::runtime_error(
        "[replay_environment_spec] SPAWN_MODEL must keep episodes parallel and "
        "steps sequential");
  }
  if (spec.range_source != "ujcamei_component_stream_cursor" ||
      spec.range_resolution != "runtime_resolved_cursor_identity") {
    throw std::runtime_error(
        "[replay_environment_spec] replay ranges must come from resolved "
        "Ujcamei/component_stream cursor identity");
  }
  if (spec.source_range_policy != "anchor_index_or_source_key") {
    throw std::runtime_error(
        "[replay_environment_spec] SOURCE_RANGE_POLICY must be "
        "anchor_index_or_source_key");
  }
  if (spec.source_order_policy != "sequential") {
    throw std::runtime_error(
        "[replay_environment_spec] SOURCE_ORDER_POLICY must be sequential for "
        "V1 replay");
  }
  if (spec.observation_time_law != "time_t_only" ||
      spec.realization_reveal != "after_action_execution") {
    throw std::runtime_error(
        "[replay_environment_spec] observation must expose only time-t "
        "knowledge and reveal realization after action/execution");
  }
  if (spec.realization_key_policy != "shared_key_per_frame") {
    throw std::runtime_error(
        "[replay_environment_spec] V1 requires one shared realization key per "
        "replay frame");
  }
  if (spec.action_kind != "target_node_weights_with_base_reserve" ||
      spec.reserve_node_policy != "graph_node_from_base_policy") {
    throw std::runtime_error(
        "[replay_environment_spec] action must target risky graph-node weights "
        "plus a base reserve graph node");
  }
  if (spec.action_schema_id !=
      "kikijyeba.environment.action.target_weights.v1") {
    throw std::runtime_error(
        "[replay_environment_spec] ACTION_SCHEMA_ID must be "
        "kikijyeba.environment.action.target_weights.v1");
  }
  if (spec.action_time_policy !=
      "decision_timestamp_after_knowledge_before_realization") {
    throw std::runtime_error(
        "[replay_environment_spec] ACTION_TIME_POLICY must bind policy actions "
        "after time-t knowledge and before the next realization is available");
  }
  if (spec.graph_node_universe_policy != "episode_spec_graph_node_ids") {
    throw std::runtime_error(
        "[replay_environment_spec] GRAPH_NODE_UNIVERSE_POLICY must require "
        "EpisodeSpec graph_node_ids");
  }
  if (spec.reward_policy !=
      "post_execution_ledger_log_growth_drawdown_cost_turnover_invalid") {
    throw std::runtime_error(
        "[replay_environment_spec] reward must be computed after execution and "
        "ledger update");
  }
  if (spec.projection_validation !=
      "projected_log_return_vs_realized_asset_base_return") {
    throw std::runtime_error(
        "[replay_environment_spec] projection validation contract mismatch");
  }
  if (spec.realized_return_truth != "direct_edge_realized_return_truth_v1") {
    throw std::runtime_error(
        "[replay_environment_spec] REALIZED_RETURN_TRUTH must be "
        "direct_edge_realized_return_truth_v1");
  }
  if (spec.policy_surface != "policy_adapter") {
    throw std::runtime_error(
        "[replay_environment_spec] POLICY_SURFACE must be policy_adapter");
  }
  if (spec.action_policy_identity != "policy_adapter_must_match_action") {
    throw std::runtime_error(
        "[replay_environment_spec] ACTION_POLICY_IDENTITY must require action "
        "identity to match the producing policy adapter");
  }
  if (spec.initial_policy_kind != "deterministic_allocator_or_baseline") {
    throw std::runtime_error(
        "[replay_environment_spec] INITIAL_POLICY_KIND must keep PPO delayed");
  }
  if (spec.experiment_task_identity != "bundle_policy_task_indices") {
    throw std::runtime_error(
        "[replay_environment_spec] experiment reports must carry explicit "
        "bundle/policy task identity");
  }
  if (spec.experiment_run_identity != "single_runtime_environment_run") {
    throw std::runtime_error(
        "[replay_environment_spec] EXPERIMENT_RUN_IDENTITY must bind every "
        "experiment report to one runtime_run_id and environment_run_id");
  }
  if (spec.step_artifact_identity != "episode_run_policy_cursor") {
    throw std::runtime_error(
        "[replay_environment_spec] STEP_ARTIFACT_IDENTITY must require "
        "episode/run/policy/cursor step identity");
  }
  if (spec.experiment_report_count_policy != "counts_match_evidence") {
    throw std::runtime_error(
        "[replay_environment_spec] EXPERIMENT_REPORT_COUNT_POLICY must require "
        "aggregate counts to match report evidence");
  }
  if (spec.artifact_schema != "cajtucu_ready_replay_artifacts") {
    throw std::runtime_error(
        "[replay_environment_spec] ARTIFACT_SCHEMA must be "
        "cajtucu_ready_replay_artifacts");
  }
  if (spec.lattice_fact_family != "replay_environment") {
    throw std::runtime_error(
        "[replay_environment_spec] LATTICE_FACT_FAMILY must be "
        "replay_environment");
  }
  if (spec.lattice_target != "replay_environment_artifact_ready") {
    throw std::runtime_error("[replay_environment_spec] LATTICE_TARGET must be "
                             "replay_environment_artifact_ready");
  }
  if (!spec.require_resolved_cursor || !spec.require_no_future_leakage ||
      !spec.require_projection_validation) {
    throw std::runtime_error(
        "[replay_environment_spec] V1 requires cursor, no-leakage, and "
        "projection-validation guards");
  }
  if (spec.replay_executor || spec.allocation_authority ||
      spec.execution_authority || spec.live_capital_allowed) {
    throw std::runtime_error(
        "[replay_environment_spec] replay V1 is audit-only and cannot execute "
        "or authorize live capital");
  }
  if (spec.default_max_parallel_jobs <= 0) {
    throw std::runtime_error(
        "[replay_environment_spec] DEFAULT_MAX_PARALLEL_JOBS must be "
        "positive");
  }
}

[[nodiscard]] inline replay_environment_spec_t
decode_replay_environment_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "REPLAY_ENVIRONMENT");
  replay_environment_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.world_mode = kv::required(block, "WORLD_MODE");
  spec.api_contract = kv::required(block, "API_CONTRACT");
  spec.spawn_model = kv::required(block, "SPAWN_MODEL");
  spec.range_source = kv::required(block, "RANGE_SOURCE");
  spec.source_range_policy = kv::required(block, "SOURCE_RANGE_POLICY");
  spec.source_order_policy = kv::required(block, "SOURCE_ORDER_POLICY");
  spec.range_resolution = kv::required(block, "RANGE_RESOLUTION");
  spec.observation_time_law = kv::required(block, "OBSERVATION_TIME_LAW");
  spec.realization_reveal = kv::required(block, "REALIZATION_REVEAL");
  spec.realization_key_policy = kv::required(block, "REALIZATION_KEY_POLICY");
  spec.action_kind = kv::required(block, "ACTION_KIND");
  spec.action_schema_id = kv::required(block, "ACTION_SCHEMA_ID");
  spec.action_time_policy = kv::required(block, "ACTION_TIME_POLICY");
  spec.reserve_node_policy = kv::required(block, "RESERVE_NODE_POLICY");
  spec.graph_node_universe_policy =
      kv::required(block, "GRAPH_NODE_UNIVERSE_POLICY");
  spec.reward_policy = kv::required(block, "REWARD_POLICY");
  spec.projection_validation = kv::required(block, "PROJECTION_VALIDATION");
  spec.realized_return_truth = kv::required(block, "REALIZED_RETURN_TRUTH");
  spec.policy_surface = kv::required(block, "POLICY_SURFACE");
  spec.action_policy_identity = kv::required(block, "ACTION_POLICY_IDENTITY");
  spec.initial_policy_kind = kv::required(block, "INITIAL_POLICY_KIND");
  spec.experiment_task_identity =
      kv::required(block, "EXPERIMENT_TASK_IDENTITY");
  spec.experiment_run_identity = kv::required(block, "EXPERIMENT_RUN_IDENTITY");
  spec.step_artifact_identity = kv::required(block, "STEP_ARTIFACT_IDENTITY");
  spec.experiment_report_count_policy =
      kv::required(block, "EXPERIMENT_REPORT_COUNT_POLICY");
  spec.artifact_schema = kv::required(block, "ARTIFACT_SCHEMA");
  spec.lattice_fact_family = kv::required(block, "LATTICE_FACT_FAMILY");
  spec.lattice_target = kv::required(block, "LATTICE_TARGET");
  spec.require_resolved_cursor =
      kv::parse_bool(kv::required(block, "REQUIRE_RESOLVED_CURSOR"));
  spec.require_no_future_leakage =
      kv::parse_bool(kv::required(block, "REQUIRE_NO_FUTURE_LEAKAGE"));
  spec.require_projection_validation =
      kv::parse_bool(kv::required(block, "REQUIRE_PROJECTION_VALIDATION"));
  spec.replay_executor = kv::parse_bool(kv::required(block, "REPLAY_EXECUTOR"));
  spec.allocation_authority =
      kv::parse_bool(kv::required(block, "ALLOCATION_AUTHORITY"));
  spec.execution_authority =
      kv::parse_bool(kv::required(block, "EXECUTION_AUTHORITY"));
  spec.live_capital_allowed =
      kv::parse_bool(kv::required(block, "LIVE_CAPITAL_ALLOWED"));
  spec.default_max_parallel_jobs =
      kv::parse_i64(kv::required(block, "DEFAULT_MAX_PARALLEL_JOBS"));
  validate_replay_environment_spec(spec);
  return spec;
}

} // namespace cuwacunu::kikijyeba::environment
