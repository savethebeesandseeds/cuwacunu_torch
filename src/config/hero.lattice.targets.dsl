/*
  hero.lattice.targets.dsl
  =============================

  Active Lattice target catalog. Keep this file focused on declarations; the
  authoring and reference text lives in src/config/README.md,
  src/config/man/hero.lattice.man, and
  src/include/hero/lattice_hero/lattice/README.md.
*/
LATTICE_PROFILE {
  PROFILE_ID = vicreg_training_readiness;
  TARGET_KIND = vicreg_ready;
  MIN_OPTIMIZER_STEPS = 1;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_PROFILE {
  PROFILE_ID = mtf_representation_training_readiness;
  TARGET_KIND = mtf_representation_ready;
  MIN_OPTIMIZER_STEPS = 1;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_PROFILE {
  PROFILE_ID = channel_mdn_training_readiness;
  TARGET_KIND = channel_mdn_ready;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_representation_ready;
  TARGET_KIND = vicreg_ready;
  SOURCE_RANGE = all;
  MIN_OPTIMIZER_STEPS = 1;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_ready;
  TARGET_KIND = channel_mdn_ready;
  SOURCE_RANGE = all;
  UPSTREAM_TARGET_ID = vicreg_representation_ready;
  MIN_OPTIMIZER_STEPS = 1;
  MIN_VALID_TARGET_FRACTION = 0.05;
  WAVE_MODE = train|debug;
  PLAN_MAX_ATTEMPTS = 3;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_train_core_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  USE_PROFILE = mtf_representation_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  USE_PROFILE = vicreg_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  OVER_SPLIT = train_core;
};

/*
  Protocol-scoped train-core readiness targets.

  The unscoped train-core targets above remain public operator targets used by
  Marshal reports, MCP examples, and readiness summaries. Protocol-scoped
  variants are materialized from a compact family when a proof needs to bind a
  target to a concrete protocol.
*/
LATTICE_TARGET_FAMILY {
  FAMILY_ID = protocol_train_core_readiness;
  FAMILY_KIND = protocol_train_core_readiness;
  PROTOCOL_IDS = cwu_01v, cwu_02v;
  REPRESENTATION_PROFILE_IDS = vicreg_training_readiness, mtf_representation_training_readiness;
  MDN_PROFILE_ID = channel_mdn_training_readiness;
  OVER_SPLIT = train_core;
};

LATTICE_TARGET_FAMILY {
  FAMILY_ID = cwu_02v_channel_mdn_leakage_guard_chain;
  FAMILY_KIND = protocol_channel_mdn_leakage_guard_chain;
  PROTOCOL_IDS = cwu_02v;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_no_validation_leakage;
  TARGET_KIND = channel_mdn_ready;
  LEAKAGE_GUARD_SCOPE = channel_mdn_validation;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_ready;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_train_core_no_test_leakage;
  TARGET_KIND = channel_mdn_ready;
  LEAKAGE_GUARD_SCOPE = channel_mdn_test;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_train_core_no_validation_leakage;
};

LATTICE_TARGET_FAMILY {
  FAMILY_ID = cwu_02v_channel_mdn_evaluation_readiness;
  FAMILY_KIND = protocol_channel_mdn_evaluation_readiness;
  PROTOCOL_IDS = cwu_02v;
  EVALUATION_SPLITS = validation_holdout, certified_replay_expansion_eval;
};

/*
  Roadmap evidence catalog proofs.

  These cwu_02v validation-scope targets prove artifact/fact integrity only.
  Most quality diagnostics over the same facts should surface first as warnings
  or summaries, not as new TARGET_KIND values or policy decisions.
*/
LATTICE_PROFILE {
  PROFILE_ID = cwu_02v_validation_mdn_artifact_readiness;
  TARGET_CLASS = artifact_readiness;
  ARTIFACT_SCOPE = cwu_02v_validation;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
};

LATTICE_PROFILE {
  PROFILE_ID = cwu_02v_validation_policy_training_artifact_readiness;
  TARGET_CLASS = artifact_readiness;
  ARTIFACT_SCOPE = cwu_02v_validation;
  SUBJECT_COMPONENT = wikimyei.policy.portfolio.graph_node_allocation;
};

LATTICE_PROFILE {
  PROFILE_ID = cwu_02v_validation_tsodao_settings_artifact_readiness;
  TARGET_CLASS = artifact_readiness;
  ARTIFACT_SCOPE = cwu_02v_validation;
  SUBJECT_COMPONENT = tsodao.settings_protection;
};

LATTICE_PROFILE {
  PROFILE_ID = cwu_02v_validation_tsodao_policy_artifact_readiness;
  TARGET_CLASS = artifact_readiness;
  ARTIFACT_SCOPE = cwu_02v_validation;
  SUBJECT_COMPONENT = tsodao.policy_acceptance;
};

LATTICE_PROFILE {
  PROFILE_ID = cwu_02v_validation_paper_online_artifact_readiness;
  TARGET_CLASS = artifact_readiness;
  ARTIFACT_SCOPE = cwu_02v_validation;
  SUBJECT_COMPONENT = kikijyeba.paper_online.readiness;
};

LATTICE_TARGET_FAMILY {
  FAMILY_ID = cwu_02v_validation_mdn_artifact_readiness_targets;
  FAMILY_KIND = profile_artifact_readiness_targets;
  PROTOCOL_IDS = cwu_02v;
  USE_PROFILE = cwu_02v_validation_mdn_artifact_readiness;
  TARGET_IDS = target_transform_contract_ready, forecast_baseline_artifact_ready, forecast_eval_artifact_ready, observer_belief_artifact_ready, allocation_artifact_ready, replay_environment_artifact_ready;
  SUBJECT_FACT_FAMILIES = target_transform, forecast_baseline, forecast_eval, observer_belief, allocation_engine, replay_environment;
};

LATTICE_WARN_SET {
  TARGET_ID = forecast_baseline_artifact_ready;
  WARNING_IDS = forecast_baseline_valid_support_low_visibility_only, forecast_baseline_kind_coverage_low_visibility_only, forecast_baseline_metric_coverage_low_visibility_only;
  METRICS = valid_count, baseline_kind_count, computed_metric_fact_count;
  BELOW_VALUES = 50, 4, 1;
};

LATTICE_WARN_SET {
  TARGET_ID = forecast_eval_artifact_ready;
  WARNING_IDS = forecast_eval_calibration_coverage_low_visibility_only, forecast_eval_skill_vs_baseline_negative_visibility_only, forecast_eval_horizon_support_low_visibility_only;
  METRICS = calibration_coverage, skill_vs_baseline, weakest_horizon_support_rows;
  BELOW_VALUES = 0.95, 0.0, 10;
};

LATTICE_TARGET {
  TARGET_ID = synthetic_forecast_oracle_accuracy_ready;
  TARGET_CLASS = synthetic_forecast_oracle_gate;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SUBJECT_FACT_FAMILY = forecast_eval;
  PROOF_KIND = synthetic_forecast_oracle_accuracy_bound;
  UPSTREAM_TARGET_ID = channel_mdn_certified_replay_expansion_eval_ready;
  OVER_SPLIT = certified_replay_expansion_eval;
  MAX_ORACLE_EV_MAE = 0.05;
  MAX_ORACLE_EV_RMSE = 0.075;
  MIN_ORACLE_DIRECTIONAL_ACCURACY = 0.95;
  MAX_ORACLE_PRICE_EV_MAE = 0.02;
  MAX_ORACLE_PRICE_EV_RMSE = 0.025;
  MAX_ORACLE_ACTIVITY_EV_MAE = 0.07;
  MAX_ORACLE_ACTIVITY_EV_RMSE = 0.10;
  MIN_ORACLE_CLOSE_DIRECTIONAL_ACCURACY = 0.95;
  WAVE_MODE = none;
  PLAN_MAX_ATTEMPTS = 0;
};

LATTICE_TARGET {
  TARGET_ID = synthetic_edge_return_projection_oracle_ready;
  TARGET_CLASS = synthetic_edge_return_projection_oracle_gate;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.inference.expected_value.mdn;
  SUBJECT_FACT_FAMILY = forecast_eval;
  PROOF_KIND = synthetic_edge_return_projection_oracle_bound;
  UPSTREAM_TARGET_ID = channel_mdn_certified_replay_expansion_eval_ready;
  OVER_SPLIT = certified_replay_expansion_eval;
  MAX_ORACLE_EDGE_RETURN_EV_MAE = 0.05;
  MAX_ORACLE_EDGE_RETURN_EV_RMSE = 0.075;
  MIN_ORACLE_EDGE_RETURN_DIRECTIONAL_ACCURACY = 0.95;
  MIN_ORACLE_EDGE_RETURN_PAIRWISE_RANK_ACCURACY = 0.95;
  MIN_ORACLE_EDGE_RETURN_BEST_ASSET_AGREEMENT = 0.60;
  MIN_ORACLE_EDGE_RETURN_CORRELATION = 0.25;
  WAVE_MODE = none;
  PLAN_MAX_ATTEMPTS = 0;
};

LATTICE_POLICY_GATE {
  POLICY_ID = forecast_quality_acceptance_reserved;
  POLICY_KIND = forecast_quality_acceptance;
  TARGET_ID = forecast_eval_artifact_ready;
  METRIC = skill_vs_baseline;
  METRIC_DEFINITION = skill_vs_baseline_same_transform_horizon_support;
  BASELINE = forecast_baseline_artifact_ready;
  BASELINE_DEFINITION = same_protocol_split_transform_horizon_baseline_fact;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_until_support_and_calibration_policy_exists;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = required_selection_leakage_and_negative_skill_tests;
  CALIBRATION_REQUIREMENTS = pit_interval_and_sigma_scale_visibility_required;
  HOLDOUT_DECLARATION = validation_holdout_out_of_sample;
  THRESHOLD_SELECTION_AUDIT = disabled_no_threshold_tuning_audit;
  ENABLED = false;
};

LATTICE_WARN_SET {
  TARGET_ID = observer_belief_artifact_ready;
  WARNING_IDS = observer_belief_confidence_low_visibility_only, observer_belief_data_quality_low_visibility_only, observer_belief_liquidity_low_visibility_only;
  METRICS = confidence, data_quality, liquidity;
  BELOW_VALUES = 0.70, 0.70, 0.70;
};

LATTICE_TARGET_FAMILY {
  FAMILY_ID = cwu_02v_validation_specialized_artifact_readiness_targets;
  FAMILY_KIND = profile_artifact_readiness_targets;
  PROTOCOL_IDS = cwu_02v;
  USE_PROFILE_IDS = cwu_02v_validation_policy_training_artifact_readiness, cwu_02v_validation_tsodao_settings_artifact_readiness, cwu_02v_validation_tsodao_policy_artifact_readiness, cwu_02v_validation_paper_online_artifact_readiness;
  TARGET_IDS = policy_training_artifact_ready, tsodao_settings_protection_ready, policy_acceptance_contract_ready, paper_online_readiness_contract_ready;
  SUBJECT_FACT_FAMILIES = policy_training, tsodao_settings_protection, policy_acceptance, paper_online_readiness;
};

LATTICE_TARGET {
  TARGET_ID = policy_execution_input_handoff_ready;
  TARGET_CLASS = policy_execution_input_handoff;
  PROTOCOL_ID = cwu_02v;
  COMPONENT = wikimyei.policy.portfolio.graph_node_allocation;
  SUBJECT_FACT_FAMILY = replay_environment;
  PROOF_KIND = policy_execution_input_handoff_bound;
  OVER_SPLIT = certified_replay_expansion_eval;
  CHECKPOINT_SOURCE = none;
  WAVE_MODE = train;
  PLAN_MAX_ATTEMPTS = 1;
  REQUIRE_COMPONENT_MATCH = false;
  REQUIRE_CHECKPOINT_EXISTS = false;
  REQUIRE_FINITE_LOSS = false;
};

LATTICE_WARN {
  TARGET_ID = allocation_artifact_ready;
  WARNING_ID = allocation_turnover_high_visibility_only;
  METRIC = turnover;
  ABOVE = 0.10;
};

LATTICE_WARN {
  TARGET_ID = allocation_artifact_ready;
  WARNING_ID = allocation_cap_diagnostics_missing_visibility_only;
  METRIC = cap_diagnostics_bound;
  BELOW = 1.0;
};

LATTICE_WARN_SET {
  TARGET_ID = allocation_artifact_ready;
  WARNING_IDS = allocation_scenario_growth_floor_attention_visibility_only, allocation_fallback_active_visibility_only, allocation_derisk_active_visibility_only;
  METRICS = scenario_growth_floor_attention, fallback_active, derisk_active;
  ABOVE_VALUES = 0.0, 0.0, 0.0;
};

LATTICE_POLICY_GATE {
  POLICY_ID = allocation_acceptance_reserved;
  POLICY_KIND = allocation_acceptance;
  TARGET_ID = allocation_artifact_ready;
  METRIC = risk_adjusted_growth_after_cost;
  METRIC_DEFINITION = replay_or_policy_declared_risk_adjusted_growth_after_cost;
  BASELINE = base_policy;
  BASELINE_DEFINITION = base_policy_accounting_numeraire_graph_node_reference;
  THRESHOLD = 0.0;
  UNCERTAINTY_POLICY = disabled;
  UNCERTAINTY_MODEL = disabled_until_replay_uncertainty_policy_exists;
  SUPPORT_MINIMUM = 0;
  SELECTOR_SPLIT = validation_holdout;
  ANTI_LEAKAGE_POLICY = required;
  TIE_POLICY = reject_ties;
  NEGATIVE_TESTS = required_cost_cvar_fallback_and_derisk_negative_tests;
  CALIBRATION_REQUIREMENTS = observer_confidence_data_quality_liquidity_required;
  HOLDOUT_DECLARATION = validation_holdout_out_of_sample;
  THRESHOLD_SELECTION_AUDIT = disabled_no_threshold_tuning_audit;
  ENABLED = false;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  USE_PROFILE = channel_mdn_training_readiness;
  OVER_SPLIT = acceptance_smoke;
};

LATTICE_TARGET {
  TARGET_ID = channel_mdn_acceptance_no_validation_leakage;
  TARGET_KIND = channel_mdn_ready;
  LEAKAGE_GUARD_SCOPE = channel_mdn_validation;
  CHECKPOINT_SOURCE = latest_satisfying:channel_mdn_acceptance_smoke_ready;
};

LATTICE_REQUIRES_SET {
  TARGET_IDS = vicreg_train_core_ready, mtf_jepa_mae_vicreg_train_core_ready, cwu_01v_representation_train_core_ready, cwu_02v_representation_train_core_ready;
  REQUIREMENT_IDS = channel_observed_input_train_core_coverage, mtf_observed_input_train_core_coverage, cwu_01v_observed_input_train_core_coverage, cwu_02v_observed_input_train_core_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  PLAN_ID = train_mtf_jepa_mae_vicreg_train_core;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_representation_effective_rank_fraction_low;
  KIND = representation_health;
  USE = observed_input;
  METRIC = representation_effective_rank_fraction;
  BELOW = 0.25;
};

LATTICE_WARN_SET {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_IDS = mtf_representation_condition_number_high, mtf_context_starvation_seen, mtf_targets_reduced_for_context_seen;
  KIND = representation_health;
  USE = observed_input;
  METRICS = representation_condition_number, context_starved_sample_count, reduced_targets_for_context_count;
  ABOVE_VALUES = 1000.0, 0.0, 0.0;
};

LATTICE_WARN_SET {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_IDS = mtf_tf_pair_support_missing, mtf_vicreg_global_rows_missing, mtf_vicreg_channel_rows_missing;
  KIND = representation_health;
  USE = observed_input;
  METRICS = tf_pair_valid_count, vicreg_global_valid_rows, vicreg_channel_valid_rows;
  BELOW_VALUES = 1.0, 1.0, 1.0;
};

LATTICE_WARN {
  TARGET_ID = mtf_jepa_mae_vicreg_train_core_ready;
  WARNING_ID = mtf_source_missingness_high_visibility_only;
  KIND = source_analytics;
  USE = observed_input;
  METRIC = missingness_fraction;
  ABOVE = 0.10;
};

LATTICE_REQUIRES {
  TARGET_ID = vicreg_acceptance_smoke_ready;
  REQUIREMENT_ID = channel_observed_input_acceptance_coverage;
  KIND = exposure_coverage;
  USE = observed_input;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_DEPENDS {
  TARGET_ID = channel_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = vicreg_train_core_ready;
};

LATTICE_REQUIRES_SET {
  TARGET_IDS = channel_mdn_train_core_ready, cwu_01v_mdn_train_core_ready, cwu_02v_mdn_train_core_ready;
  REQUIREMENT_IDS = channel_target_supervision_train_core_coverage, cwu_01v_channel_target_supervision_train_core_coverage, cwu_02v_channel_target_supervision_train_core_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_train_core_ready;
  PLAN_ID = train_channel_mdn_train_core;
};

LATTICE_DEPENDS {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = cwu_01v_representation_train_core_ready;
};

LATTICE_PLAN {
  TARGET_ID = cwu_01v_mdn_train_core_ready;
  PLAN_ID = train_cwu_01v_channel_mdn_train_core;
};

LATTICE_DEPENDS {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  UPSTREAM_TARGET_ID = cwu_02v_representation_train_core_ready;
};

LATTICE_PLAN {
  TARGET_ID = cwu_02v_mdn_train_core_ready;
  PLAN_ID = train_cwu_02v_channel_mdn_train_core;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_target_supervision_train_core_load;
  KIND = exposure_load;
  USE = target_supervision;
  CURSOR_EPOCHS_ABOVE = 3.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_optimizer_effort_density;
  KIND = effort_density;
  USE = target_supervision;
  ABOVE = 10.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_mean_nll;
  KIND = mdn_distribution_calibration;
  USE = target_supervision;
  METRIC = mean_nll;
  ABOVE = 2.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_finite_parameter_check_failed;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = finite_parameter_check;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_nonfinite_output_count;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = nonfinite_output_count;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_checkpoint_missing;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = checkpoint_written;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_train_representation_checkpoint_missing;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = representation_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = high_channel_mdn_pre_clip_grad_norm;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = grad_norm_max_pre_clip;
  ABOVE = 1000.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_train_core_ready;
  WARNING_ID = channel_mdn_valid_target_fraction_low;
  KIND = runtime_health;
  USE = target_supervision;
  METRIC = valid_target_fraction;
  BELOW = 0.05;
};

LATTICE_DEPENDS {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  UPSTREAM_TARGET_ID = vicreg_acceptance_smoke_ready;
};

LATTICE_REQUIRES {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  REQUIREMENT_ID = channel_target_supervision_acceptance_coverage;
  KIND = exposure_coverage;
  USE = target_supervision;
  CURSOR_EPOCHS = 1.0;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_acceptance_smoke_ready;
  PLAN_ID = train_channel_mdn_acceptance_smoke;
};

LATTICE_REQUIRES_SET {
  TARGET_IDS = channel_mdn_validation_eval_ready, channel_mdn_certified_replay_expansion_eval_ready;
  REQUIREMENT_IDS = channel_validation_evaluation_metric_coverage, channel_certified_replay_expansion_evaluation_metric_coverage;
  KIND = exposure_coverage;
  USE = evaluation_metric;
  EFFECT = none;
  CURSOR_EPOCHS = 0.95;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  PLAN_ID = run_channel_mdn_validation_eval;
};

LATTICE_PLAN {
  TARGET_ID = channel_mdn_certified_replay_expansion_eval_ready;
  PLAN_ID = run_channel_mdn_certified_replay_expansion_eval;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_mutated_model_state;
  KIND = runtime_health;
  USE = evaluation_metric;
  METRIC = model_state_mutated;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_checkpoint_written;
  KIND = runtime_health;
  USE = evaluation_metric;
  METRIC = checkpoint_written;
  ABOVE = 0.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_representation_checkpoint_missing;
  KIND = runtime_health;
  USE = evaluation_metric;
  METRIC = representation_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_mdn_checkpoint_missing;
  KIND = runtime_health;
  USE = evaluation_metric;
  METRIC = mdn_checkpoint_loaded;
  BELOW = 1.0;
};

LATTICE_WARN {
  TARGET_ID = channel_mdn_validation_eval_ready;
  WARNING_ID = channel_mdn_validation_eval_nonfinite_output_count;
  KIND = runtime_health;
  USE = evaluation_metric;
  METRIC = nonfinite_output_count;
  ABOVE = 0.0;
};
