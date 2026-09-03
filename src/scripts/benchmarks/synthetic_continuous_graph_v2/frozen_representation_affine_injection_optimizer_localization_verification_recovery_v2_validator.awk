function fatal(message) {
  print "report validation failed: " message > "/dev/stderr"
  failed = 1
  exit 1
}

function absolute(x) { return x < 0 ? -x : x }
function maximum(a, b) { return a > b ? a : b }

function is_number(x) {
  return x ~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/
}

function rv(key) {
  if (!(key in report_seen)) fatal("missing report key " key)
  return report[key]
}

function pv(key) {
  if (!(key in phase_seen)) fatal("missing Phase2A key " key)
  return phase[key]
}

function exact(key, expected, actual) {
  actual = rv(key)
  if ((actual "") != (expected ""))
    fatal(key " expected " expected ", got " actual)
}

function require_number(key, lower, upper, lower_closed, upper_closed, integer,
                        value) {
  value = rv(key)
  if (!is_number(value)) fatal(key " is not a finite decimal number")
  value += 0
  if (lower != "" &&
      (lower_closed ? value < lower : value <= lower))
    fatal(key " violates lower bound")
  if (upper != "" &&
      (upper_closed ? value > upper : value >= upper))
    fatal(key " violates upper bound")
  if (integer && value != int(value)) fatal(key " is not integral")
  return value
}

function close_values(actual, expected, tolerance, label, delta) {
  if (!is_number(actual) || !is_number(expected))
    fatal(label " has a nonnumeric operand")
  delta = absolute((actual + 0) - (expected + 0))
  if (delta > tolerance) fatal(label " exceeds absolute tolerance")
}

function close_report(key, expected, tolerance) {
  close_values(rv(key), expected, tolerance, key)
}

function metric(prefix, expected_count, field, ratio) {
  exact(prefix ".count", expected_count)
  exact(prefix ".pairwise_rank_count", expected_count)
  require_number(prefix ".mae", 0, "", 1, 1, 0)
  require_number(prefix ".rmse", 0, "", 1, 1, 0)
  require_number(prefix ".prediction_rms", 0, "", 1, 1, 0)
  require_number(prefix ".rmse_target_rms_ratio", 0, "", 1, 1, 0)
  require_number(prefix ".target_rms", 0, "", 0, 1, 0)
  for (field = 1; field <= 3; ++field) {
    bounded_field = (field == 1 ? "directional_accuracy" :
                     (field == 2 ? "pairwise_rank_accuracy" :
                                   "best_asset_agreement"))
    require_number(prefix "." bounded_field, 0, 1, 1, 1, 0)
  }
  require_number(prefix ".correlation", -1, 1, 1, 1, 0)
  ratio = (rv(prefix ".rmse") + 0) / (rv(prefix ".target_rms") + 0)
  close_report(prefix ".rmse_target_rms_ratio", ratio, 1e-12)
}

function mixed_duplicate(execution_key, recomputed_key, a, b, scale, delta,
                         relative_delta, allowance, fraction) {
  a = require_number(execution_key, 0, "", 0, 1, 0)
  b = require_number(recomputed_key, 0, "", 0, 1, 0)
  scale = maximum(absolute(a), absolute(b))
  delta = absolute(a - b)
  relative_delta = (scale == 0 ? 0 : delta / scale)
  allowance = abs_tol + rel_tol * scale
  fraction = (allowance == 0 ? (delta == 0 ? 0 : 1e300) :
                               delta / allowance)
  duplicate_pair_count++
  if (delta > 1e-12) strict_failure_count++
  if (delta <= allowance)
    mixed_pass_count++
  else
    fatal(execution_key " exceeds mixed tolerance")
  if (delta > max_abs_discrepancy) max_abs_discrepancy = delta
  if (relative_delta > max_rel_discrepancy)
    max_rel_discrepancy = relative_delta
  if (fraction > max_tolerance_fraction) max_tolerance_fraction = fraction
}

function update_direct_metric_delta(oracle_key, direct_key, delta) {
  delta = absolute((rv(oracle_key) + 0) - (rv(direct_key) + 0))
  if (delta > direct_metric_max_delta) direct_metric_max_delta = delta
}

FILENAME == ARGV[1] {
  separator = index($0, "=")
  if (separator < 2 || substr($0, length($0), 1) == "\r")
    fatal("malformed Phase2A line")
  key = substr($0, 1, separator - 1)
  if (key in phase_seen) fatal("duplicate Phase2A key " key)
  phase_seen[key] = 1
  phase[key] = substr($0, separator + 1)
  phase_lines++
  next
}

FILENAME == ARGV[2] {
  separator = index($0, "=")
  if (separator < 2 || substr($0, length($0), 1) == "\r")
    fatal("malformed report line")
  key = substr($0, 1, separator - 1)
  if (key in report_seen) fatal("duplicate report key " key)
  report_seen[key] = 1
  report[key] = substr($0, separator + 1)
  report_lines++
  next
}

END {
  if (syntax_self_test) exit 0
  if (failed) exit 1
  if (phase_lines != 234) fatal("Phase2A report line count is not 234")
  if (report_lines != 490) fatal("source report line count is not 490")

  exact("schema_id", "synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1")
  exact("status", "complete")
  exact("benchmark_id", "synthetic_continuous_graph_v2")
  exact("diagnostic_phase", "affine_injection_optimizer_localization")
  exact("diagnostic_authority", "development_only")
  exact("benchmark_acceptance_authority", "false")
  exact("train_input", "/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe")
  exact("validation_input", "/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe")
  exact("probe_kind", "representation")
  exact("probe_record_schema", "kikijyeba.synthetic.representation_edge_feature_probe.v1")
  exact("train_probe_rows", "22464")
  exact("validation_probe_rows", "2304")
  exact("certified_probe_rows", "0")
  exact("fit_anchor_range", "[0,2496)")
  exact("validation_anchor_range", "[2560,2816)")
  exact("maximum_anchor_read", "2815")
  exact("final_holdout_begin", "3328")
  close_report("fixed_ridge", 1e-12, 1e-24)
  exact("ridge_selection", "false")
  exact("oracle_phase2a_reference_validation", "external_runner_required")
  exact("head_index_formula", "channel*3+edge")
  exact("flat_row_order", "anchor,edge,channel")
  exact("direct_architecture", "Linear(96,9)+gather(channel*3+edge)")
  exact("injected_architecture", "Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)+gather(channel*3+edge)")
  exact("device", "cpu")
  exact("feature_dtype", "float32")
  exact("target_training_dtype", "float32")
  exact("metric_dtype", "float64")
  exact("oracle_solver_dtype", "float64")
  exact("deterministic_algorithms", "true")
  exact("deterministic_cudnn", "true")
  exact("deterministic_fill_uninitialized_memory", "true")
  exact("intraop_threads", "1")
  exact("interop_threads", "1")
  exact("float64_solver", "float64_centered_cholesky_ridge")
  exact("float32_feature_standardization", "train_core_all_edges_all_channels")
  exact("target_standardization", "train_core_per_edge_channel")
  exact("gelu_identity", "GELU(z)-GELU(-z)=z")
  exact("gelu_injected_hidden_units_per_layer", "18")
  exact("gelu_unused_hidden_units_per_layer", "110")
  exact("zero_optimizer_ladder_optimizer_steps", "0")
  require_number("feature_standardization_clamped_coordinate_count", 0, "", 1,
                 1, 1)
  require_number("target_standardization_clamped_coordinate_count", 0, "", 1,
                 1, 1)
  require_number("affine_maximum_normalized_residual", 0, "", 1, 1, 0)
  require_number("affine_coefficient_l2_norm", 0, "", 0, 1, 0)

  route_name[1] = "float64_oracle"
  route_name[2] = "direct_float32"
  route_name[3] = "paired_gelu_injected"
  route_name[4] = "direct_linear_adam_seed31"
  split_name[1] = "train"
  split_name[2] = "validation"
  for (route_index = 1; route_index <= 4; ++route_index) {
    for (split_index = 1; split_index <= 2; ++split_index) {
      split_id = split_name[split_index]
      count = (split_id == "train" ? 22464 : 2304)
      prefix = "route." route_name[route_index] "." split_id
      metric(prefix, count)
      for (channel = 0; channel <= 2; ++channel) {
        count = (split_id == "train" ? 7488 : 768)
        metric(prefix ".channel_" channel, count)
      }
    }
  }

  metric_field[1] = "mae"
  metric_field[2] = "rmse"
  metric_field[3] = "target_rms"
  metric_field[4] = "prediction_rms"
  metric_field[5] = "rmse_target_rms_ratio"
  metric_field[6] = "directional_accuracy"
  metric_field[7] = "pairwise_rank_accuracy"
  metric_field[8] = "best_asset_agreement"
  metric_field[9] = "correlation"
  for (split_index = 1; split_index <= 2; ++split_index) {
    split_id = split_name[split_index]
    for (channel_selector = -1; channel_selector <= 2; ++channel_selector) {
      suffix = (channel_selector < 0 ? "" : ".channel_" channel_selector)
      report_prefix = "route.float64_oracle." split_id suffix
      phase_prefix = "selected." split_id suffix
      exact(report_prefix ".count", pv(phase_prefix ".count"))
      exact(report_prefix ".pairwise_rank_count",
            pv(phase_prefix ".pairwise_rank_count"))
      for (field_index = 1; field_index <= 9; ++field_index) {
        close_report(report_prefix "." metric_field[field_index],
                     pv(phase_prefix "." metric_field[field_index]), 1e-12)
      }
    }
  }

  close_report("direct_float32_parity_tolerance_standardized_target_units",
               1e-3, 1e-15)
  close_report("paired_gelu_parity_tolerance_standardized_target_units", 1e-5,
               1e-17)
  delta_key[1] = "delta.float64_oracle_vs_direct_float32.train.standardized_target_units_max_abs"
  delta_key[2] = "delta.float64_oracle_vs_direct_float32.validation.standardized_target_units_max_abs"
  delta_key[3] = "delta.direct_float32_vs_paired_gelu.train.standardized_target_units_max_abs"
  delta_key[4] = "delta.direct_float32_vs_paired_gelu.validation.standardized_target_units_max_abs"
  delta_key[5] = "delta.float64_oracle_vs_direct_float32.train.original_units_max_abs"
  delta_key[6] = "delta.float64_oracle_vs_direct_float32.validation.original_units_max_abs"
  delta_key[7] = "delta.direct_float32_vs_paired_gelu.train.original_units_max_abs"
  delta_key[8] = "delta.direct_float32_vs_paired_gelu.validation.original_units_max_abs"
  delta_key[9] = "delta.float64_oracle_vs_paired_gelu.train.original_units_max_abs"
  delta_key[10] = "delta.float64_oracle_vs_paired_gelu.validation.original_units_max_abs"
  for (delta_index = 1; delta_index <= 10; ++delta_index)
    require_number(delta_key[delta_index], 0, "", 1, 1, 0)
  float_pass = ((rv(delta_key[1]) + 0) <= 0.001 &&
                (rv(delta_key[2]) + 0) <= 0.001 ? "true" : "false")
  gelu_pass = ((rv(delta_key[3]) + 0) <= 0.00001 &&
               (rv(delta_key[4]) + 0) <= 0.00001 ? "true" : "false")
  exact("direct_float32_parity_pass", float_pass)
  exact("paired_gelu_parity_pass", gelu_pass)

  oracle_mse = require_number("route.float64_oracle.train.standardized_mse", 0,
                              "", 0, 1, 0)
  adam_mse = require_number("route.direct_linear_adam_seed31.train.standardized_mse",
                            0, "", 0, 1, 0)
  aggregate_ratio = require_number("direct_linear_adam_to_oracle_train_standardized_mse_ratio",
                                   0, "", 1, 1, 0)
  close_values(aggregate_ratio, adam_mse / oracle_mse, 1e-12,
               "aggregate standardized MSE ratio")
  recovery = (aggregate_ratio <= 1.05)
  maximum_head_ratio = 0
  for (head = 0; head <= 8; ++head) {
    oracle_head_key = "route.float64_oracle.train.head_" head ".standardized_mse"
    adam_head_key = "route.direct_linear_adam_seed31.train.head_" head ".standardized_mse"
    oracle_head = require_number(oracle_head_key, 0, "", 0, 1, 0)
    adam_head = require_number(adam_head_key, 0, "", 0, 1, 0)
    head_ratio = adam_head / oracle_head
    close_report("direct_linear_adam_to_oracle_train.head_" head ".standardized_mse_ratio",
                 head_ratio, 1e-12)
    if (head_ratio > 1.10) recovery = 0
    if (head_ratio > maximum_head_ratio) maximum_head_ratio = head_ratio
    mixed_duplicate("direct_linear_adam.full_train_standardized_mse.head_" head ".edge_" (head % 3) ".channel_" int(head / 3),
                    adam_head_key)
  }
  close_report("direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio",
               maximum_head_ratio, 1e-12)
  if ((rv("route.direct_linear_adam_seed31.train.directional_accuracy") + 0) <
      (rv("route.float64_oracle.train.directional_accuracy") + 0) - 0.01)
    recovery = 0
  if ((rv("route.direct_linear_adam_seed31.train.pairwise_rank_accuracy") + 0) <
      (rv("route.float64_oracle.train.pairwise_rank_accuracy") + 0) - 0.01)
    recovery = 0
  if ((rv("route.direct_linear_adam_seed31.train.correlation") + 0) <
      (rv("route.float64_oracle.train.correlation") + 0) - 0.01)
    recovery = 0
  if ((rv("route.direct_linear_adam_seed31.train.rmse_target_rms_ratio") + 0) >
      (rv("route.float64_oracle.train.rmse_target_rms_ratio") + 0) + 0.05)
    recovery = 0
  clear_failure = (!recovery &&
                   (aggregate_ratio >= 1.25 || maximum_head_ratio >= 1.50))
  exact("direct_linear_adam_recovery_gate_pass",
        recovery ? "true" : "false")
  exact("direct_linear_adam_clear_failure_gate_pass",
        clear_failure ? "true" : "false")
  exact("direct_linear_adam_recovery_gate", "train_aggregate_mse_ratio<=1.05,each_train_head_mse_ratio<=1.10,train_direction>=oracle-0.01,train_rank>=oracle-0.01,train_correlation>=oracle-0.01,train_rmse_ratio<=oracle+0.05")
  exact("direct_linear_adam_clear_failure_gate", "!recovery_and_(train_aggregate_mse_ratio>=1.25_or_max_train_head_mse_ratio>=1.50)")
  if (float_pass == "false")
    expected_classification = "float32_conditioning_failure"
  else if (gelu_pass == "false")
    expected_classification = "paired_gelu_execution_failure"
  else if (recovery)
    expected_classification = "deep_parameterization_or_optimization_failure"
  else if (clear_failure)
    expected_classification = "direct_linear_adam_optimizer_failure"
  else
    expected_classification = "optimizer_localization_inconclusive"
  exact("classification", expected_classification)

  exact("seed", "31")
  exact("affine_oracle_grouped_fit_count", "1")
  exact("affine_oracle_head_solve_count", "9")
  exact("optimizer_fits_completed", "1")
  exact("total_train_fit_procedures", "2")
  exact("optimizer_steps", "3500")
  exact("steps_per_fit", "3500")
  exact("batch_size", "64")
  exact("optimizer", "Adam")
  exact("batch_sampling", "mt19937_64_uniform_with_replacement")
  exact("early_stopping", "false")
  exact("seed_selection", "false")
  exact("hyperparameter_search", "false")
  exact("retry", "false")
  exact("refit", "false")
  close_report("learning_rate", 0.001, 1e-15)
  close_report("adam_beta1", 0.9, 1e-15)
  close_report("adam_beta2", 0.999, 1e-15)
  close_report("adam_epsilon", 1e-8, 1e-20)
  close_report("weight_decay", 0, 0)
  close_report("gradient_clip_norm", 5, 0)
  exact("batch_schedule_fingerprint", "f2fa41d284a42d60")
  require_number("direct_linear_adam.initial_full_train_standardized_mse", 0,
                 "", 1, 1, 0)
  require_number("direct_linear_adam.final_full_train_standardized_mse", 0,
                 "", 1, 1, 0)
  require_number("direct_linear_adam.last_minibatch_loss", 0, "", 1, 1, 0)
  require_number("direct_linear_adam.maximum_preclip_gradient_norm", 0, "", 1,
                 1, 0)
  mixed_duplicate("direct_linear_adam.final_full_train_standardized_mse",
                  "route.direct_linear_adam_seed31.train.standardized_mse")
  require_number("direct_linear_adam.clipped_step_count", 0, 3500, 1, 1, 1)
  exact("validation_read_by_trainer", "false")
  exact("validation_driven_choice", "false")
  exact("representation_forward_executed", "false")
  exact("checkpoint_written", "false")
  exact("certified_input_access", "false")
  exact("final_holdout_access", "false")
  exact("policy_access", "false")
  if (duplicate_pair_count != 10 || mixed_pass_count != 10)
    fatal("duplicate MSE pair accounting mismatch")

  forecast_field[1] = "directional_accuracy"
  forecast_field[2] = "pairwise_rank_accuracy"
  forecast_field[3] = "correlation"
  forecast_field[4] = "rmse_target_rms_ratio"
  for (split_index = 1; split_index <= 2; ++split_index) {
    split_id = split_name[split_index]
    for (field_index = 1; field_index <= 4; ++field_index) {
      update_direct_metric_delta("route.float64_oracle." split_id "." forecast_field[field_index],
                                 "route.direct_float32." split_id "." forecast_field[field_index])
    }
  }

  print "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_worker_v1"
  print "status=verified"
  print "source_report_sha256=" source_report_sha
  print "phase2a_report_sha256=" phase2a_report_sha
  print "full_report_validation_pass=true"
  print "source_report_line_count=" report_lines
  print "phase2a_report_line_count=" phase_lines
  print "duplicate_mse_pair_count=" duplicate_pair_count
  print "duplicate_mse_strict_1e-12_failure_count=" strict_failure_count
  print "duplicate_mse_mixed_tolerance_pass_count=" mixed_pass_count
  print "duplicate_mse_absolute_tolerance=" abs_tol
  print "duplicate_mse_relative_tolerance=" rel_tol
  printf "duplicate_mse_maximum_absolute_discrepancy=%.17g\n", max_abs_discrepancy
  printf "duplicate_mse_maximum_relative_discrepancy=%.17g\n", max_rel_discrepancy
  printf "duplicate_mse_maximum_tolerance_fraction=%.17g\n", max_tolerance_fraction
  print "classification=" rv("classification")
  print "classification_order_preserved=true"
  print "direct_float32_parity_pass=" rv("direct_float32_parity_pass")
  print "paired_gelu_parity_pass=" rv("paired_gelu_parity_pass")
  print "direct_linear_adam_recovery_gate_pass=" rv("direct_linear_adam_recovery_gate_pass")
  print "direct_linear_adam_clear_failure_gate_pass=" rv("direct_linear_adam_clear_failure_gate_pass")
  print "direct_linear_adam_to_oracle_train_standardized_mse_ratio=" rv("direct_linear_adam_to_oracle_train_standardized_mse_ratio")
  print "direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio=" rv("direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio")
  printf "direct_float32_aggregate_forecast_metric_maximum_absolute_delta=%.17g\n", direct_metric_max_delta
  print "direct_float32_aggregate_forecast_metrics_within_0.001=" (direct_metric_max_delta <= 0.001 ? "true" : "false")
  for (split_index = 1; split_index <= 2; ++split_index) {
    split_id = split_name[split_index]
    for (field_index = 1; field_index <= 4; ++field_index) {
      field = forecast_field[field_index]
      print "float64_oracle." split_id "." field "=" rv("route.float64_oracle." split_id "." field)
      print "direct_float32." split_id "." field "=" rv("route.direct_float32." split_id "." field)
    }
  }
  print "probe_bytes_read_by_recovery=false"
  print "binary_executed_by_recovery=false"
  print "new_capture_invocations=0"
  print "new_representation_forward_invocations=0"
  print "new_evaluator_invocations=0"
  print "new_fits=0"
  print "new_optimizer_steps=0"
  print "certified_input_access=false"
  print "final_holdout_access=false"
  print "policy_access=false"
}
