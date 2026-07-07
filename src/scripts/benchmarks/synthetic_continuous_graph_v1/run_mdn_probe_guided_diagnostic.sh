#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
config_path="${benchmark_root}/synthetic_benchmark.config"
artifacts_dir="${benchmark_root}/artifacts"
diagnostic_schema="${SYNTHETIC_MDN_DIAGNOSTIC_SCHEMA_ID:-synthetic_mdn_probe_guided_diagnostic_run.v1}"
raw_root="${SYNTHETIC_MDN_DIAGNOSTIC_RAW_ROOT:-${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${diagnostic_schema}}"
raw_dir="${raw_root}/raw"
output_probe="${SYNTHETIC_MDN_DIAGNOSTIC_OUTPUT_PROBE:-${artifacts_dir}/${diagnostic_schema}.probe}"
marshal_profile="${SYNTHETIC_MDN_DIAGNOSTIC_MARSHAL_PROFILE:-bounded_operator}"

runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
marshal_mcp="${repo_root}/.build/hero/hero_marshal.mcp"

mkdir -p "${artifacts_dir}" "${raw_dir}"

require_file() {
  local path="$1"
  if [[ ! -f "${path}" ]]; then
    echo "required file missing: ${path}" >&2
    exit 2
  fi
}

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

json_field_last() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":\"[^\"]*\"" "${file}" 2>/dev/null | tail -1 |
    sed -E "s/^\"${field}\":\"(.*)\"$/\\1/" || true
}

json_bool_last() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":\\(true\\|false\\)" "${file}" 2>/dev/null | tail -1 |
    sed -E "s/^\"${field}\"://" || true
}

current_runtime_wave_id() {
  awk -F= '
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      value = $2;
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
      print value;
      exit;
    }' "${config_path}"
}

set_runtime_wave_id() {
  local wave_id="$1"
  local tmp
  tmp="$(mktemp)"
  awk -v wave_id="${wave_id}" '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= " wave_id);
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${config_path}" >"${tmp}"
  mv "${tmp}" "${config_path}"
}

run_capture() {
  local label="$1"
  local outfile="$2"
  shift 2
  set +e
  "$@" >"${outfile}" 2>&1
  local code=$?
  set -e
  printf '%s_exit_code=%s\n' "${label}" "${code}" >"${outfile}.status"
  return "${code}"
}

run_marshal_required() {
  local label="$1"
  local outfile="$2"
  local target_id="$3"
  local tool="$4"
  local mode="$5"
  local start_epoch
  start_epoch="$(date +%s)"
  printf '%s_start_epoch=%s\n' "${label}" "${start_epoch}" >"${outfile}.meta"
  run_capture "${label}" "${outfile}" \
    "${marshal_mcp}" --global-config "${config_path}" --tool "${tool}" \
    --args-json "{\"target_id\":\"${target_id}\",\"mode\":\"${mode}\",\"profile\":\"${marshal_profile}\",\"include_machine_payload\":false}" || {
      tail -200 "${outfile}" >&2 || true
      return 1
    }
  printf '%s_end_epoch=%s\n' "${label}" "$(date +%s)" >>"${outfile}.meta"
}

latest_job_dir_after() {
  local component="$1"
  local job_leaf="$2"
  local start_epoch="$3"
  local root="${repo_root}/.runtime/cuwacunu_exec/components/${component}"
  find "${root}" -path "*/jobs/*/${job_leaf}.attempt_*" -type d \
    -newermt "@${start_epoch}" -printf '%T@ %p\n' 2>/dev/null |
    sort -n | tail -1 | cut -d' ' -f2-
}

metric_field() {
  local metric="$1"
  local field="$2"
  local file="$3"
  awk -F= -v k="${metric}.${field}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

class_bool_gt() {
  local value="$1"
  local threshold="$2"
  awk -v v="${value:-nan}" -v t="${threshold}" \
    'BEGIN { print (v == "" || v == "nan") ? "unknown" : ((v + 0 > t + 0) ? "true" : "false") }'
}

summarize_probe_stream() {
  local probe_stream="$1"
  local summary_path="$2"
  awk '
  BEGIN {
    RS = "";
    FS = "\n";
    split("last_loss mean_loss last_nll_loss mean_nll_loss optimizer_steps " \
          "last_direct_edge_return_readout_loss mean_direct_edge_return_readout_loss " \
          "last_direct_edge_return_readout_regression_loss mean_direct_edge_return_readout_regression_loss " \
          "last_direct_edge_return_readout_direction_loss mean_direct_edge_return_readout_direction_loss " \
          "last_direct_edge_return_readout_rank_loss mean_direct_edge_return_readout_rank_loss " \
          "last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio " \
          "last_direct_edge_return_readout_loss_abs_to_total_abs_ratio " \
          "last_direct_edge_return_readout_head_grad_norm " \
          "max_direct_edge_return_readout_head_grad_norm " \
          "last_direct_edge_return_readout_head_parameter_norm " \
          "last_direct_edge_return_readout_head_parameter_update_norm " \
          "max_direct_edge_return_readout_head_parameter_update_norm " \
          "last_direct_edge_return_readout_scheduled_nll_weight " \
          "direct_edge_return_readout_warmup_step_count " \
          "direct_edge_return_readout_direct_head_only_warmup_step_count " \
          "direct_edge_return_readout_pred_mean direct_edge_return_readout_realized_mean " \
          "direct_edge_return_readout_pred_std direct_edge_return_readout_realized_std " \
          "direct_edge_return_readout_pred_to_realized_std_ratio " \
          "direct_edge_return_readout_pred_min direct_edge_return_readout_pred_max " \
          "direct_edge_return_readout_realized_min direct_edge_return_readout_realized_max " \
          "direct_edge_return_readout_near_zero_target_fraction " \
          "direct_edge_return_readout_margin_eps direct_edge_return_readout_margin_valid_count " \
          "direct_edge_return_readout_margin_directional_accuracy " \
          "direct_edge_return_readout_rank_margin_eps " \
          "direct_edge_return_readout_margin_pairwise_rank_valid_count " \
          "direct_edge_return_readout_margin_pairwise_rank_accuracy " \
          "direct_edge_return_readout_directional_accuracy " \
          "direct_edge_return_readout_pairwise_rank_accuracy " \
          "direct_edge_return_readout_best_asset_agreement " \
          "direct_edge_return_readout_correlation " \
          "direct_edge_return_readout_valid_count " \
          "direct_edge_return_readout_loss_valid_count " \
          "direct_edge_return_readout_directional_accuracy_per_edge_min " \
          "direct_edge_return_readout_directional_accuracy_per_edge_max " \
          "direct_edge_return_readout_directional_accuracy_per_edge_spread " \
          "direct_edge_return_readout_directional_accuracy_per_channel_min " \
          "direct_edge_return_readout_directional_accuracy_per_channel_max " \
          "direct_edge_return_readout_directional_accuracy_per_channel_spread", wanted_arr, " ");
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
  }' "${probe_stream}" >"${summary_path}"
}

require_file "${config_path}"
require_file "${runtime_exec}"
require_file "${marshal_mcp}"

if ! rg -a -q 'last_direct_edge_return_readout_head_grad_norm' "${runtime_exec}"; then
  echo "cuwacunu_exec does not contain the required MDN probe metrics; rebuild it first" >&2
  exit 3
fi
if ! rg -a -q 'direct_edge_return_readout_target_scale' "${runtime_exec}"; then
  echo "cuwacunu_exec does not contain the direct-readout intervention fields; rebuild it first" >&2
  exit 3
fi
if ! rg -a -q 'direct_edge_return_readout_post_warmup_nll_weight' "${runtime_exec}"; then
  echo "cuwacunu_exec does not contain the post-warmup NLL objective field; rebuild it first" >&2
  exit 3
fi
if ! rg -a -q 'direct_edge_return_readout_identity_mode' "${runtime_exec}" ||
  ! rg -a -q 'edge_embedding_per_edge' "${runtime_exec}"; then
  echo "cuwacunu_exec does not contain the identity-conditioned direct-readout fields; rebuild it first" >&2
  exit 3
fi

original_wave_id="$(current_runtime_wave_id)"
restore_wave() {
  if [[ -n "${original_wave_id}" ]]; then
    set_runtime_wave_id "${original_wave_id}" || true
  fi
}
trap restore_wave EXIT

"${script_dir}/generate_synthetic_klines.sh" >"${raw_dir}/generate_synthetic_klines.log" 2>&1

set_runtime_wave_id "train_core_mtf_jepa_mae_vicreg"
rep_out="${raw_dir}/marshal_representation_execute.json"
run_marshal_required "representation_execute" "${rep_out}" \
  "cwu_02v_representation_train_core_ready" "hero.marshal.prepare.train" \
  "execute"
rep_start_epoch="$(kv representation_execute_start_epoch "${rep_out}.meta")"
rep_job_dir="$(latest_job_dir_after \
  "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" \
  "train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg" \
  "${rep_start_epoch}")"
if [[ -z "${rep_job_dir}" ]]; then
  echo "failed to locate fresh representation job dir" >&2
  exit 4
fi
rep_result="${rep_job_dir}/runtime.result.fact"
require_file "${rep_result}"
if [[ "$(kv status "${rep_result}")" != "completed" ]]; then
  echo "fresh representation job did not complete: ${rep_job_dir}" >&2
  exit 5
fi
rep_checkpoint="$(kv checkpoint_path "${rep_result}")"
require_file "${rep_checkpoint}"

set_runtime_wave_id "train_core_channel_mdn"
mdn_out="${raw_dir}/marshal_mdn_execute.json"
run_marshal_required "mdn_execute" "${mdn_out}" \
  "cwu_02v_mdn_train_core_ready" "hero.marshal.prepare.train" "execute"
mdn_start_epoch="$(kv mdn_execute_start_epoch "${mdn_out}.meta")"
mdn_job_dir="$(latest_job_dir_after \
  "wikimyei.inference.expected_value.mdn" \
  "train_core_channel_mdn.train.channel_inference_mdn" \
  "${mdn_start_epoch}")"
if [[ -z "${mdn_job_dir}" ]]; then
  echo "failed to locate fresh MDN job dir" >&2
  exit 6
fi
mdn_result="${mdn_job_dir}/runtime.result.fact"
mdn_probe_stream="${mdn_job_dir}/runtime.job_events.probe"
mdn_report="${mdn_job_dir}/channel_inference.report"
require_file "${mdn_result}"
require_file "${mdn_probe_stream}"
require_file "${mdn_report}"
if [[ "$(kv status "${mdn_result}")" != "completed" ]]; then
  echo "fresh MDN job did not complete: ${mdn_job_dir}" >&2
  exit 7
fi
if ! rg -q 'last_direct_edge_return_readout_head_grad_norm|direct_edge_return_readout_pred_to_realized_std_ratio' \
  "${mdn_probe_stream}"; then
  echo "fresh MDN probe stream is missing required diagnostic metrics: ${mdn_probe_stream}" >&2
  exit 8
fi
mdn_checkpoint="$(kv checkpoint_path "${mdn_result}")"
require_file "${mdn_checkpoint}"

eval_out="${raw_dir}/marshal_mdn_certified_replay_eval_execute.json"
eval_job_dir=""
eval_status="not_attempted"
set_runtime_wave_id "cwu_02v_certified_replay_eval_mdn"
eval_start_epoch="$(date +%s)"
printf 'eval_execute_start_epoch=%s\n' "${eval_start_epoch}" >"${eval_out}.meta"
if run_capture "eval_execute" "${eval_out}" \
  "${marshal_mcp}" --global-config "${config_path}" \
  --tool "hero.marshal.prepare.evaluate" \
  --args-json "{\"target_id\":\"channel_mdn_certified_replay_expansion_eval_ready\",\"mode\":\"execute\",\"profile\":\"${marshal_profile}\",\"include_machine_payload\":false}"; then
  eval_job_dir="$(latest_job_dir_after \
    "wikimyei.inference.expected_value.mdn" \
    "cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn" \
    "${eval_start_epoch}")"
  if [[ -n "${eval_job_dir}" && -f "${eval_job_dir}/runtime.result.fact" ]]; then
    eval_status="$(kv status "${eval_job_dir}/runtime.result.fact")"
  else
    eval_status="no_runtime_job_observed"
  fi
else
  eval_status="marshal_evaluate_call_failed"
fi
printf 'eval_execute_end_epoch=%s\n' "$(date +%s)" >>"${eval_out}.meta"

metric_summary="${raw_dir}/mdn_train_metric_summary.probe"
summarize_probe_stream "${mdn_probe_stream}" "${metric_summary}"

grad_max="$(metric_field last_direct_edge_return_readout_head_grad_norm max_value "${metric_summary}")"
update_max="$(metric_field last_direct_edge_return_readout_head_parameter_update_norm max_value "${metric_summary}")"
loss_ratio_nll_last="$(metric_field last_direct_edge_return_readout_loss_abs_to_nll_abs_ratio last_value "${metric_summary}")"
loss_ratio_total_last="$(metric_field last_direct_edge_return_readout_loss_abs_to_total_abs_ratio last_value "${metric_summary}")"
std_ratio_last="$(metric_field direct_edge_return_readout_pred_to_realized_std_ratio last_value "${metric_summary}")"
target_scale="$(kv direct_edge_return_readout_target_scale "${mdn_report}")"
warmup_steps_config="$(kv direct_edge_return_readout_warmup_steps "${mdn_report}")"
warmup_nll_weight_config="$(kv direct_edge_return_readout_warmup_nll_weight "${mdn_report}")"
post_warmup_nll_weight_config="$(kv direct_edge_return_readout_post_warmup_nll_weight "${mdn_report}")"
warmup_direct_head_only_config="$(kv direct_edge_return_readout_warmup_direct_head_only "${mdn_report}")"
identity_mode="$(kv direct_edge_return_readout_identity_mode "${mdn_report}")"
base_edge_count="$(kv direct_edge_return_readout_base_edge_count "${mdn_report}")"
identity_embedding_dim="$(kv direct_edge_return_readout_identity_embedding_dim "${mdn_report}")"
adapter_hidden_dim="$(kv direct_edge_return_readout_adapter_hidden_dim "${mdn_report}")"
warmup_steps_observed="$(metric_field direct_edge_return_readout_warmup_step_count last_value "${metric_summary}")"
direct_head_only_warmup_steps_observed="$(metric_field direct_edge_return_readout_direct_head_only_warmup_step_count last_value "${metric_summary}")"
scheduled_nll_weight_min="$(metric_field last_direct_edge_return_readout_scheduled_nll_weight min_value "${metric_summary}")"
scheduled_nll_weight_last="$(metric_field last_direct_edge_return_readout_scheduled_nll_weight last_value "${metric_summary}")"
margin_direction_last="$(metric_field direct_edge_return_readout_margin_directional_accuracy last_value "${metric_summary}")"
margin_rank_last="$(metric_field direct_edge_return_readout_margin_pairwise_rank_accuracy last_value "${metric_summary}")"
direction_last="$(metric_field direct_edge_return_readout_directional_accuracy last_value "${metric_summary}")"
rank_last="$(metric_field direct_edge_return_readout_pairwise_rank_accuracy last_value "${metric_summary}")"
correlation_last="$(metric_field direct_edge_return_readout_correlation last_value "${metric_summary}")"
edge_spread_last="$(metric_field direct_edge_return_readout_directional_accuracy_per_edge_spread last_value "${metric_summary}")"
channel_spread_last="$(metric_field direct_edge_return_readout_directional_accuracy_per_channel_spread last_value "${metric_summary}")"

gradients_present="$(class_bool_gt "${grad_max}" "0.000000000001")"
parameters_move="$(class_bool_gt "${update_max}" "0.000000000001")"

loss_scale_class="$(
  awk -v r="${loss_ratio_nll_last:-nan}" 'BEGIN {
    if (r == "" || r == "nan") print "unknown";
    else if (r + 0 < 0.01) print "too_small_to_compete_with_nll";
    else if (r + 0 > 100.0) print "dominates_nll";
    else print "comparable_to_nll";
  }'
)"
prediction_class="$(
  awk -v r="${std_ratio_last:-nan}" 'BEGIN {
    if (r == "" || r == "nan") print "unknown";
    else if (r + 0 < 0.05) print "collapsed";
    else if (r + 0 < 0.25) print "weak_variance";
    else print "variance_present";
  }'
)"
margin_class="$(
  awk -v d="${margin_direction_last:-nan}" -v r="${margin_rank_last:-nan}" 'BEGIN {
    if (d == "" || r == "" || d == "nan" || r == "nan") print "unknown";
    else if (d + 0 >= 0.45 && d + 0 <= 0.55 && r + 0 >= 0.45 && r + 0 <= 0.55) print "near_random";
    else print "non_random_or_asymmetric";
  }'
)"
localized_class="$(
  awk -v e="${edge_spread_last:-0}" -v c="${channel_spread_last:-0}" 'BEGIN {
    print ((e + 0 > 0.20) || (c + 0 > 0.20)) ? "localized_edge_or_channel_gap" : "not_obviously_localized";
  }'
)"
next_goal="$(
  if [[ "${gradients_present}" != "true" ]]; then
    echo "synthetic_mdn_direct_head_gradient_path_bug.v1"
  elif [[ "${parameters_move}" != "true" ]]; then
    echo "synthetic_mdn_direct_head_optimizer_binding_bug.v1"
  elif [[ "${loss_scale_class}" == "too_small_to_compete_with_nll" ]]; then
    echo "synthetic_mdn_direct_readout_loss_scale_schedule_ablation.v1"
  elif [[ "${prediction_class}" == "collapsed" ||
          "${prediction_class}" == "weak_variance" ]]; then
    echo "synthetic_mdn_direct_readout_prediction_collapse.v1"
  elif [[ "${localized_class}" == "localized_edge_or_channel_gap" ]]; then
    echo "synthetic_mdn_edge_channel_identity_scaffolding.v1"
  else
    echo "synthetic_mdn_objective_readout_alignment_ablation.v1"
  fi
)"

cat >"${output_probe}" <<EOF_PROBE
schema_id=${diagnostic_schema}
status=complete
benchmark_id=synthetic_continuous_graph_v1
generated_at_unix=$(date +%s)

config_path=${config_path}
original_runtime_wave_id=${original_wave_id}
runtime_exec=${runtime_exec}
marshal_mcp=${marshal_mcp}
marshal_profile=${marshal_profile}

representation_job_dir=${rep_job_dir}
representation_checkpoint=${rep_checkpoint}
representation_status=$(kv status "${rep_result}")

mdn_job_dir=${mdn_job_dir}
mdn_checkpoint=${mdn_checkpoint}
mdn_status=$(kv status "${mdn_result}")
mdn_report=${mdn_report}
mdn_probe_stream=${mdn_probe_stream}
mdn_metric_summary=${metric_summary}

eval_attempt_status=${eval_status}
eval_job_dir=${eval_job_dir}
eval_result_fact=${eval_job_dir:+${eval_job_dir}/runtime.result.fact}

diagnostic.direct_head_receives_gradients=${gradients_present}
diagnostic.direct_head_grad_norm_max=${grad_max}
diagnostic.direct_head_parameters_move=${parameters_move}
diagnostic.direct_head_parameter_update_norm_max=${update_max}
diagnostic.direct_loss_ratio_to_nll_last=${loss_ratio_nll_last}
diagnostic.direct_loss_ratio_to_total_last=${loss_ratio_total_last}
diagnostic.direct_loss_scale_class=${loss_scale_class}
diagnostic.direct_readout_target_scale=${target_scale}
diagnostic.direct_readout_warmup_steps_config=${warmup_steps_config}
diagnostic.direct_readout_warmup_nll_weight_config=${warmup_nll_weight_config}
diagnostic.direct_readout_post_warmup_nll_weight_config=${post_warmup_nll_weight_config}
diagnostic.direct_readout_warmup_direct_head_only_config=${warmup_direct_head_only_config}
diagnostic.direct_readout_identity_mode=${identity_mode}
diagnostic.direct_readout_base_edge_count=${base_edge_count}
diagnostic.direct_readout_identity_embedding_dim=${identity_embedding_dim}
diagnostic.direct_readout_adapter_hidden_dim=${adapter_hidden_dim}
diagnostic.direct_readout_warmup_steps_observed=${warmup_steps_observed}
diagnostic.direct_readout_direct_head_only_warmup_steps_observed=${direct_head_only_warmup_steps_observed}
diagnostic.direct_readout_scheduled_nll_weight_min=${scheduled_nll_weight_min}
diagnostic.direct_readout_scheduled_nll_weight_last=${scheduled_nll_weight_last}
diagnostic.pred_to_realized_std_ratio_last=${std_ratio_last}
diagnostic.prediction_variance_class=${prediction_class}
diagnostic.margin_directional_accuracy_last=${margin_direction_last}
diagnostic.margin_pairwise_rank_accuracy_last=${margin_rank_last}
diagnostic.margin_signal_class=${margin_class}
diagnostic.directional_accuracy_last=${direction_last}
diagnostic.pairwise_rank_accuracy_last=${rank_last}
diagnostic.correlation_last=${correlation_last}
diagnostic.edge_directional_accuracy_spread_last=${edge_spread_last}
diagnostic.channel_directional_accuracy_spread_last=${channel_spread_last}
diagnostic.locality_class=${localized_class}

answer.direct_head_receives_meaningful_gradients=${gradients_present}
answer.parameters_are_moving=${parameters_move}
answer.direct_readout_loss_scale=${loss_scale_class}
answer.identity_conditioned_direct_readout_mode=${identity_mode}
answer.predictions_collapsed_vs_targets=${prediction_class}
answer.margin_filtered_direction_rank=${margin_class}
answer.failure_uniformity=${localized_class}
answer.next_recommended_goal=${next_goal}

raw.representation_marshal_output=${rep_out}
raw.mdn_marshal_output=${mdn_out}
raw.eval_marshal_output=${eval_out}
EOF_PROBE

{
  echo
  echo "# metric_summary"
  cat "${metric_summary}"
} >>"${output_probe}"

cat "${output_probe}"
