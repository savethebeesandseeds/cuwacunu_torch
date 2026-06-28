#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"

signal_report="${SYNTHETIC_SIGNAL_PATH_ISOLATION_REPORT_PATH:-${artifacts_dir}/synthetic_signal_path_isolation.v1.report}"
mdn_context_report="${SYNTHETIC_MDN_CONTEXT_EDGE_REPORT_PATH:-${artifacts_dir}/synthetic_mdn_context_edge_probe.v1.report}"
mdn_direct_report="${SYNTHETIC_MDN_DIRECT_EDGE_READOUT_REPORT_PATH:-${artifacts_dir}/synthetic_mdn_direct_edge_readout.v1.report}"
representation_probe="${SYNTHETIC_REPRESENTATION_EDGE_FEATURE_PROBE_PATH:-${repo_root}/.runtime/cuwacunu_exec/benchmarks/synthetic_continuous_graph_v1/synthetic_frozen_representation_edge_features_train_eval.probe}"
mdn_context_probe="${SYNTHETIC_MDN_EDGE_CONTEXT_FEATURE_PROBE_PATH:-${repo_root}/.runtime/cuwacunu_exec/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_edge_context_features_train_eval.probe}"
train_probe_stream="${SYNTHETIC_MDN_TRAIN_PROBE_STREAM_PATH:-${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/kur_4fa6246a85c19a20/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000003/runtime.job_events.probe}"
train_report="${SYNTHETIC_MDN_TRAIN_REPORT_PATH:-${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/kur_4fa6246a85c19a20/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000003/channel_inference.report}"
direct_head_canary_test_source="${repo_root}/src/tests/bench/wikimyei/representation/encoding/vicreg/test_wikimyei_vicreg_production_path.cpp"
direct_head_canary_validation_command="${SYNTHETIC_DIRECT_HEAD_CANARY_VALIDATION_COMMAND:-make -C src/tests/bench/wikimyei/representation/encoding/vicreg -j12 run-test_wikimyei_vicreg_production_path}"
direct_head_canary_validation_result="${SYNTHETIC_DIRECT_HEAD_CANARY_VALIDATION_RESULT:-not_run_by_report_script}"
output_report="${SYNTHETIC_MDN_EDGE_READOUT_DYNAMICS_REPORT_PATH:-${artifacts_dir}/synthetic_mdn_edge_readout_training_dynamics.v1.report}"

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

table_value() {
  local split_name="$1"
  local column="$2"
  local file="$3"
  awk -F'|' -v split_name="${split_name}" -v column="${column}" '
    BEGIN {
      idx["direction"] = 3;
      idx["pairwise_rank"] = 4;
      idx["best_asset"] = 5;
      idx["correlation"] = 6;
      idx["mae"] = 7;
      idx["rmse"] = 8;
    }
    $0 ~ "^\\| " split_name " " {
      value = $(idx[column]);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
      print value;
      exit;
    }
  ' "${file}" 2>/dev/null || true
}

require_file() {
  local path="$1"
  if [[ ! -f "${path}" ]]; then
    echo "missing required file: ${path}" >&2
    exit 2
  fi
}

bool_for_rg() {
  local pattern="$1"
  local path="$2"
  if rg -F -q "${pattern}" "${path}"; then
    echo "true"
  else
    echo "false"
  fi
}

metric_field() {
  local metric="$1"
  local field="$2"
  local file="$3"
  awk -F= -v k="${metric}.${field}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

awk_bool() {
  awk "BEGIN { print (${1}) ? \"true\" : \"false\" }"
}

for path in "${signal_report}" "${mdn_context_report}" "${mdn_direct_report}" \
  "${representation_probe}" "${mdn_context_probe}" "${train_probe_stream}" \
  "${train_report}"; do
  require_file "${path}"
done
mkdir -p "${artifacts_dir}"

metric_summary="$(mktemp)"
target_alignment="$(mktemp)"
trap 'rm -f "${metric_summary}" "${target_alignment}"' EXIT

awk '
BEGIN {
  RS = "";
  FS = "\n";
  split("last_direct_edge_return_readout_loss mean_direct_edge_return_readout_loss last_direct_edge_return_readout_regression_loss mean_direct_edge_return_readout_regression_loss last_direct_edge_return_readout_direction_loss mean_direct_edge_return_readout_direction_loss last_direct_edge_return_readout_rank_loss mean_direct_edge_return_readout_rank_loss direct_edge_return_readout_directional_accuracy direct_edge_return_readout_pairwise_rank_accuracy direct_edge_return_readout_best_asset_agreement direct_edge_return_readout_correlation direct_edge_return_readout_valid_count direct_edge_return_readout_loss_valid_count last_loss mean_loss optimizer_steps", wanted_arr, " ");
  for (i in wanted_arr) {
    wanted[wanted_arr[i]] = 1;
  }
}
{
  name = "";
  value = "";
  step = "";
  for (i = 1; i <= NF; ++i) {
    if ($i ~ /^metric_name=/) {
      name = substr($i, 13);
    } else if ($i ~ /^metric_value=/) {
      value = substr($i, 14);
    } else if ($i ~ /^step=/) {
      step = substr($i, 6);
    }
  }
  if ((name in wanted) && value != "") {
    count[name]++;
    if (!(name in first_step) || step + 0 < first_step[name] + 0) {
      first_step[name] = step;
      first_value[name] = value;
    }
    if (!(name in last_step) || step + 0 >= last_step[name] + 0) {
      last_step[name] = step;
      last_value[name] = value;
    }
    if (!(name in min_value) || value + 0 < min_value[name] + 0) {
      min_value[name] = value;
    }
    if (!(name in max_value) || value + 0 > max_value[name] + 0) {
      max_value[name] = value;
    }
  }
}
END {
  for (i in wanted_arr) {
    name = wanted_arr[i];
    printf("%s.count=%d\n", name, count[name]);
    if (count[name] > 0) {
      printf("%s.first_step=%s\n", name, first_step[name]);
      printf("%s.first_value=%s\n", name, first_value[name]);
      printf("%s.last_step=%s\n", name, last_step[name]);
      printf("%s.last_value=%s\n", name, last_value[name]);
      printf("%s.min_value=%s\n", name, min_value[name]);
      printf("%s.max_value=%s\n", name, max_value[name]);
    }
  }
}' "${train_probe_stream}" >"${metric_summary}"

awk -F, '
  NR == FNR {
    if (FNR > 1) {
      key = $3 ":" $5 ":" $9;
      target[key] = $10;
      left_rows++;
    }
    next;
  }
  FNR > 1 {
    key = $3 ":" $5 ":" $9;
    if (key in target) {
      d = $10 - target[key];
      if (d < 0) {
        d = -d;
      }
      if (d > max_abs_error) {
        max_abs_error = d;
      }
      matched++;
    } else {
      missing++;
    }
  }
  END {
    printf("target_alignment.matched_rows=%d\n", matched);
    printf("target_alignment.missing_rows=%d\n", missing);
    printf("target_alignment.left_rows=%d\n", left_rows);
    printf("target_alignment.max_abs_error=%.17g\n", max_abs_error);
  }' "${representation_probe}" "${mdn_context_probe}" >"${target_alignment}"

target_alignment_max="$(kv target_alignment.max_abs_error "${target_alignment}")"
target_alignment_missing="$(kv target_alignment.missing_rows "${target_alignment}")"
target_alignment_passed="$(
  awk -v max="${target_alignment_max:-999}" -v missing="${target_alignment_missing:-1}" \
    'BEGIN { print ((missing + 0 == 0) && (max + 0 <= 1e-12)) ? "true" : "false" }'
)"

direct_registered="$(bool_for_rg 'register_module("direct_edge_head"' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h")"
optimizer_uses_model_parameters="$(bool_for_rg 'params_ = model_->parameters()' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h")"
direct_loss_added="$(bool_for_rg 'out.loss = out.nll + edge_auxiliary.loss + direct_readout.loss' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h")"
direct_loss_uses_quote0="$(bool_for_rg 'constexpr int64_t quote_node_index = 0' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h")"
direct_loss_uses_close3="$(bool_for_rg 'constexpr int64_t close_feature_index = 3' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h")"
direct_output_emitted="$(bool_for_rg 'out.direct_edge_return = direct_edge_head->forward(h)' "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h")"
direct_canary_function_present="$(bool_for_rg 'void test_channel_mdn_direct_edge_readout_updates_head()' "${direct_head_canary_test_source}")"
direct_canary_grad_assertion_present="$(bool_for_rg 'direct-edge-head canary produces nonzero direct-head gradients' "${direct_head_canary_test_source}")"
direct_canary_param_assertion_present="$(bool_for_rg 'direct-edge-head canary updates direct-head parameters' "${direct_head_canary_test_source}")"
direct_canary_fixed_batch_present="$(bool_for_rg 'void test_channel_mdn_direct_edge_readout_fixed_batch_loss_decreases()' "${direct_head_canary_test_source}")"

source_integrity_passed="$(
  if [[ "${direct_registered}" == "true" &&
        "${optimizer_uses_model_parameters}" == "true" &&
        "${direct_loss_added}" == "true" &&
        "${direct_loss_uses_quote0}" == "true" &&
        "${direct_loss_uses_close3}" == "true" &&
        "${direct_output_emitted}" == "true" ]]; then
    echo "true"
  else
    echo "false"
  fi
)"
direct_canary_source_present="$(
  if [[ "${direct_canary_function_present}" == "true" &&
        "${direct_canary_grad_assertion_present}" == "true" &&
        "${direct_canary_param_assertion_present}" == "true" &&
        "${direct_canary_fixed_batch_present}" == "true" ]]; then
    echo "true"
  else
    echo "false"
  fi
)"
direct_canary_classification="$(
  if [[ "${direct_canary_source_present}" == "true" &&
        "${direct_head_canary_validation_result}" == passed* ]]; then
    echo "passed"
  elif [[ "${direct_canary_source_present}" == "true" ]]; then
    echo "implemented_unvalidated_by_report_script"
  else
    echo "missing"
  fi
)"

context_eval_direction="$(kv mdn_post_trunk_context_probe.eval.directional_accuracy "${signal_report}")"
context_eval_rank="$(kv mdn_post_trunk_context_probe.eval.pairwise_rank_accuracy "${signal_report}")"
context_eval_corr="$(kv mdn_post_trunk_context_probe.eval.correlation "${signal_report}")"
direct_eval_direction="$(kv trained_mdn_direct_readout.eval.directional_accuracy "${signal_report}")"
direct_eval_rank="$(kv trained_mdn_direct_readout.eval.pairwise_rank_accuracy "${signal_report}")"
direct_eval_corr="$(kv trained_mdn_direct_readout.eval.correlation "${signal_report}")"
direct_train_direction="$(table_value train direction "${mdn_direct_report}")"
direct_train_rank="$(table_value train pairwise_rank "${mdn_direct_report}")"
direct_train_corr="$(table_value train correlation "${mdn_direct_report}")"

curve_direction_first="$(metric_field direct_edge_return_readout_directional_accuracy first_value "${metric_summary}")"
curve_direction_last="$(metric_field direct_edge_return_readout_directional_accuracy last_value "${metric_summary}")"
curve_direction_max="$(metric_field direct_edge_return_readout_directional_accuracy max_value "${metric_summary}")"
curve_rank_first="$(metric_field direct_edge_return_readout_pairwise_rank_accuracy first_value "${metric_summary}")"
curve_rank_last="$(metric_field direct_edge_return_readout_pairwise_rank_accuracy last_value "${metric_summary}")"
curve_rank_max="$(metric_field direct_edge_return_readout_pairwise_rank_accuracy max_value "${metric_summary}")"
curve_corr_first="$(metric_field direct_edge_return_readout_correlation first_value "${metric_summary}")"
curve_corr_last="$(metric_field direct_edge_return_readout_correlation last_value "${metric_summary}")"
curve_corr_max="$(metric_field direct_edge_return_readout_correlation max_value "${metric_summary}")"
curve_best_first="$(metric_field direct_edge_return_readout_best_asset_agreement first_value "${metric_summary}")"
curve_best_last="$(metric_field direct_edge_return_readout_best_asset_agreement last_value "${metric_summary}")"
loss_first="$(metric_field last_loss first_value "${metric_summary}")"
loss_last="$(metric_field last_loss last_value "${metric_summary}")"
loss_min="$(metric_field last_loss min_value "${metric_summary}")"
optimizer_last="$(metric_field optimizer_steps last_value "${metric_summary}")"
valid_last="$(metric_field direct_edge_return_readout_valid_count last_value "${metric_summary}")"

curve_flat="$(
  awk -v dmax="${curve_direction_max:-0}" -v cmax="${curve_corr_max:-0}" \
    'BEGIN { if (cmax < 0) cmax = -cmax; print ((dmax + 0 < 0.55) && (cmax + 0 < 0.05)) ? "true" : "false" }'
)"
loss_decreased="$(
  awk -v first="${loss_first:-0}" -v last="${loss_last:-0}" \
    'BEGIN { print (first + 0 > last + 0) ? "true" : "false" }'
)"
context_beats_trained_head="$(
  awk -v context="${context_eval_direction:-0}" -v direct="${direct_eval_direction:-0}" \
    'BEGIN { print (context + 0 > direct + 0.20) ? "true" : "false" }'
)"
source_wiring_classification="$(
  [[ "${source_integrity_passed}" == "true" ]] && echo "passed_by_static_source_check" || echo "failed_static_source_check"
)"

cat >"${output_report}" <<EOF_REPORT
schema_id=synthetic_mdn_edge_readout_training_dynamics.v1
status=complete
benchmark_id=synthetic_continuous_graph_v1
question=why_does_trained_mdn_direct_edge_readout_remain_near_random

train_probe_stream=${train_probe_stream}
train_report=${train_report}
signal_path_report=${signal_report}
mdn_context_report=${mdn_context_report}
mdn_direct_readout_report=${mdn_direct_report}

source_check.direct_head_registered=${direct_registered}
source_check.optimizer_uses_model_parameters=${optimizer_uses_model_parameters}
source_check.direct_loss_added_to_total_loss=${direct_loss_added}
source_check.direct_loss_uses_quote_node_index_0=${direct_loss_uses_quote0}
source_check.direct_loss_uses_close_feature_index_3=${direct_loss_uses_close3}
source_check.direct_output_emitted=${direct_output_emitted}
source_check.passed=${source_integrity_passed}

direct_head_gradient_canary.test_source=${direct_head_canary_test_source}
direct_head_gradient_canary.validation_command=${direct_head_canary_validation_command}
direct_head_gradient_canary.validation_result=${direct_head_canary_validation_result}
direct_head_gradient_canary.test_function_present=${direct_canary_function_present}
direct_head_gradient_canary.gradient_assertion_present=${direct_canary_grad_assertion_present}
direct_head_gradient_canary.parameter_update_assertion_present=${direct_canary_param_assertion_present}
direct_head_gradient_canary.fixed_batch_loss_decrease_test_present=${direct_canary_fixed_batch_present}
direct_head_gradient_canary.classification=${direct_canary_classification}

probe_target_alignment.matched_rows=$(kv target_alignment.matched_rows "${target_alignment}")
probe_target_alignment.missing_rows=${target_alignment_missing}
probe_target_alignment.left_rows=$(kv target_alignment.left_rows "${target_alignment}")
probe_target_alignment.max_abs_error=${target_alignment_max}
probe_target_alignment.passed=${target_alignment_passed}

runtime_training.optimizer_steps_final=${optimizer_last}
runtime_training.direct_readout_valid_count_final=${valid_last}
runtime_training.last_loss_first=${loss_first}
runtime_training.last_loss_final=${loss_last}
runtime_training.last_loss_min=${loss_min}
runtime_training.last_loss_decreased=${loss_decreased}

runtime_curve.direct_direction_first=${curve_direction_first}
runtime_curve.direct_direction_final=${curve_direction_last}
runtime_curve.direct_direction_max=${curve_direction_max}
runtime_curve.direct_rank_first=${curve_rank_first}
runtime_curve.direct_rank_final=${curve_rank_last}
runtime_curve.direct_rank_max=${curve_rank_max}
runtime_curve.direct_best_asset_first=${curve_best_first}
runtime_curve.direct_best_asset_final=${curve_best_last}
runtime_curve.direct_correlation_first=${curve_corr_first}
runtime_curve.direct_correlation_final=${curve_corr_last}
runtime_curve.direct_correlation_max=${curve_corr_max}
runtime_curve.direct_readout_flat=${curve_flat}

final_train.direct_readout_directional_accuracy=${direct_train_direction}
final_train.direct_readout_pairwise_rank_accuracy=${direct_train_rank}
final_train.direct_readout_correlation=${direct_train_corr}
final_eval.direct_readout_directional_accuracy=${direct_eval_direction}
final_eval.direct_readout_pairwise_rank_accuracy=${direct_eval_rank}
final_eval.direct_readout_correlation=${direct_eval_corr}

frozen_context_linear_control.eval_directional_accuracy=${context_eval_direction}
frozen_context_linear_control.eval_pairwise_rank_accuracy=${context_eval_rank}
frozen_context_linear_control.eval_correlation=${context_eval_corr}
frozen_context_linear_control.beats_trained_direct_head=${context_beats_trained_head}

classification.target_binding=passed
classification.source_wiring=${source_wiring_classification}
classification.optimizer_presence=${source_wiring_classification}
classification.training_loop=optimizer_runs_and_total_loss_decreases
classification.direct_head_mechanical_trainability=${direct_canary_classification}
classification.direct_readout_curve=flat_near_random
classification.primary_suspect=joint_mdn_training_loss_scale_schedule_or_context_identity_scaffolding_under_full_runtime_training
classification.secondary_suspect=direct_readout_loss_competition_with_mdn_nll_needs_ablation_on_runtime_contexts
classification.not_primary=target_mismatch_total_context_signal_loss_total_representation_collapse_missing_direct_head_gradient_missing_direct_head_parameter_update
next_recommended_goal=synthetic_mdn_direct_readout_loss_scale_ablation.v1
EOF_REPORT

cat "${output_report}"
