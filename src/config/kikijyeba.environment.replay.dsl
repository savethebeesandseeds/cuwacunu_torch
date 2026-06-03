/*
  kikijyeba.environment.replay.dsl
  =================================
  Contract for the first Kikijyeba operating environment.

  This is not PPO and not a live trading executor. V1 is a historical replay
  world that keeps the same reset/step grammar needed by future RL policies,
  while validating the deterministic AllocationBelief/action/accounting loop.
*/
REPLAY_ENVIRONMENT {
  VERSION = kikijyeba.environment.replay.v1;
  COMPONENT_ASSEMBLY_ID = replay_environment_v1;
  WORLD_MODE = historical_replay;
  API_CONTRACT = rl_compatible_reset_step;
  SPAWN_MODEL = episode_parallel_step_sequential;
  RANGE_SOURCE = ujcamei_component_stream_cursor;
  SOURCE_RANGE_POLICY = anchor_index_or_source_key;
  SOURCE_ORDER_POLICY = sequential;
  RANGE_RESOLUTION = runtime_resolved_cursor_identity;
  OBSERVATION_TIME_LAW = time_t_only;
  REALIZATION_REVEAL = after_action_execution;
  REALIZATION_KEY_POLICY = shared_key_per_frame;
  ACTION_KIND = target_node_weights_with_base_reserve;
  ACTION_SCHEMA_ID = kikijyeba.environment.action.target_weights.v1;
  ACTION_TIME_POLICY = decision_timestamp_after_knowledge_before_realization;
  RESERVE_NODE_POLICY = graph_node_from_base_policy;
  GRAPH_NODE_UNIVERSE_POLICY = episode_spec_graph_node_ids;
  REWARD_POLICY = post_execution_ledger_log_growth_drawdown_cost_turnover_invalid;
  PROJECTION_VALIDATION = projected_log_return_vs_realized_asset_base_return;
  REALIZED_RETURN_TRUTH = direct_edge_realized_return_truth_v1;
  POLICY_SURFACE = policy_adapter;
  ACTION_POLICY_IDENTITY = policy_adapter_must_match_action;
  INITIAL_POLICY_KIND = deterministic_allocator_or_baseline;
  EXPERIMENT_TASK_IDENTITY = bundle_policy_task_indices;
  EXPERIMENT_RUN_IDENTITY = single_runtime_environment_run;
  STEP_ARTIFACT_IDENTITY = episode_run_policy_cursor;
  EXPERIMENT_REPORT_COUNT_POLICY = counts_match_evidence;
  ARTIFACT_SCHEMA = replay_audit_artifacts;
  REQUIRE_RESOLVED_CURSOR = true;
  REQUIRE_NO_FUTURE_LEAKAGE = true;
  REQUIRE_PROJECTION_VALIDATION = true;
  REPLAY_EXECUTOR = false;
  ALLOCATION_AUTHORITY = false;
  EXECUTION_AUTHORITY = false;
  LIVE_CAPITAL_ALLOWED = false;
  DEFAULT_MAX_PARALLEL_JOBS = 1;
};
