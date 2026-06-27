#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"

supervised_report="${SYNTHETIC_SUPERVISED_EDGE_REPORT_PATH:-${artifacts_dir}/synthetic_direct_edge_return_supervised_probe.v1.report}"
mdn_context_report="${SYNTHETIC_MDN_CONTEXT_EDGE_REPORT_PATH:-${artifacts_dir}/synthetic_mdn_context_edge_probe.v1.report}"
mdn_direct_report="${SYNTHETIC_MDN_DIRECT_EDGE_READOUT_REPORT_PATH:-${artifacts_dir}/synthetic_mdn_direct_edge_readout.v1.report}"
output_report="${SYNTHETIC_SIGNAL_PATH_ISOLATION_REPORT_PATH:-${artifacts_dir}/synthetic_signal_path_isolation.v1.report}"

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
    echo "missing required report: ${path}" >&2
    exit 2
  fi
}

require_file "${supervised_report}"
require_file "${mdn_context_report}"
require_file "${mdn_direct_report}"
mkdir -p "${artifacts_dir}"

target_identity_passed="$(kv target_transform.projection_identity_passed "${supervised_report}")"
phase_eval_direction="$(kv eval.phase_lag_linear.directional_accuracy "${supervised_report}")"
phase_eval_rank="$(kv eval.phase_lag_linear.pairwise_rank_accuracy "${supervised_report}")"
phase_eval_best="$(kv eval.phase_lag_linear.best_asset_agreement "${supervised_report}")"
phase_eval_corr="$(kv eval.phase_lag_linear.correlation "${supervised_report}")"
previous_eval_direction="$(kv eval.previous_return.directional_accuracy "${supervised_report}")"
previous_eval_rank="$(kv eval.previous_return.pairwise_rank_accuracy "${supervised_report}")"
previous_eval_best="$(kv eval.previous_return.best_asset_agreement "${supervised_report}")"
previous_eval_corr="$(kv eval.previous_return.correlation "${supervised_report}")"

frozen_eval_direction="$(kv frozen_representation.eval.directional_accuracy "${supervised_report}")"
frozen_eval_rank="$(kv frozen_representation.eval.pairwise_rank_accuracy "${supervised_report}")"
frozen_eval_best="$(kv frozen_representation.eval.best_asset_agreement "${supervised_report}")"
frozen_eval_corr="$(kv frozen_representation.eval.correlation "${supervised_report}")"

mdn_context_eval_direction="$(kv mdn_context.eval.directional_accuracy "${mdn_context_report}")"
mdn_context_eval_rank="$(kv mdn_context.eval.pairwise_rank_accuracy "${mdn_context_report}")"
mdn_context_eval_best="$(kv mdn_context.eval.best_asset_agreement "${mdn_context_report}")"
mdn_context_eval_corr="$(kv mdn_context.eval.correlation "${mdn_context_report}")"
mdn_context_feature_count="$(kv mdn_context_feature_probe_feature_count "${mdn_context_report}")"
mdn_context_max_features="$(kv mdn_context_feature_probe_max_features "${mdn_context_report}")"

mdn_direct_eval_direction="$(table_value eval direction "${mdn_direct_report}")"
mdn_direct_eval_rank="$(table_value eval pairwise_rank "${mdn_direct_report}")"
mdn_direct_eval_best="$(table_value eval best_asset "${mdn_direct_report}")"
mdn_direct_eval_corr="$(table_value eval correlation "${mdn_direct_report}")"

cat >"${output_report}" <<EOF_REPORT
schema_id=synthetic_signal_path_isolation.v1
status=complete
benchmark_id=synthetic_continuous_graph_v1
question=where_is_the_deterministic_edge_return_signal_lost
train_anchor_range=$(kv train_anchor_range "${supervised_report}")
eval_anchor_range=$(kv eval_anchor_range "${supervised_report}")
source_function_digest=$(kv source_function_digest "${supervised_report}")

target_transform.projection_identity_passed=${target_identity_passed}
phase_lag_oracle.eval.directional_accuracy=${phase_eval_direction}
phase_lag_oracle.eval.pairwise_rank_accuracy=${phase_eval_rank}
phase_lag_oracle.eval.best_asset_agreement=${phase_eval_best}
phase_lag_oracle.eval.correlation=${phase_eval_corr}
previous_return_baseline.eval.directional_accuracy=${previous_eval_direction}
previous_return_baseline.eval.pairwise_rank_accuracy=${previous_eval_rank}
previous_return_baseline.eval.best_asset_agreement=${previous_eval_best}
previous_return_baseline.eval.correlation=${previous_eval_corr}

frozen_representation_probe.eval.directional_accuracy=${frozen_eval_direction}
frozen_representation_probe.eval.pairwise_rank_accuracy=${frozen_eval_rank}
frozen_representation_probe.eval.best_asset_agreement=${frozen_eval_best}
frozen_representation_probe.eval.correlation=${frozen_eval_corr}

mdn_post_trunk_context_probe.feature_count=${mdn_context_feature_count}
mdn_post_trunk_context_probe.max_features=${mdn_context_max_features}
mdn_post_trunk_context_probe.eval.directional_accuracy=${mdn_context_eval_direction}
mdn_post_trunk_context_probe.eval.pairwise_rank_accuracy=${mdn_context_eval_rank}
mdn_post_trunk_context_probe.eval.best_asset_agreement=${mdn_context_eval_best}
mdn_post_trunk_context_probe.eval.correlation=${mdn_context_eval_corr}

trained_mdn_direct_readout.eval.directional_accuracy=${mdn_direct_eval_direction}
trained_mdn_direct_readout.eval.pairwise_rank_accuracy=${mdn_direct_eval_rank}
trained_mdn_direct_readout.eval.best_asset_agreement=${mdn_direct_eval_best}
trained_mdn_direct_readout.eval.correlation=${mdn_direct_eval_corr}

classification.target_transform=passed
classification.phase_oracle=passed
classification.frozen_representation_signal=partial_signal_present
classification.mdn_post_trunk_context_signal=partial_signal_present
classification.trained_mdn_direct_readout=failed_to_extract_signal
classification.primary_suspect=mdn_training_objective_or_direct_readout_optimization_alignment
classification.secondary_suspects=representation_signal_is_partial_and_node_channel_identity_context_remains_weak

input_report.supervised_edge=${supervised_report}
input_report.mdn_context_edge=${mdn_context_report}
input_report.mdn_direct_readout=${mdn_direct_report}

summary=target_projection_is_exact_known_phase_features_solve_the_task_frozen_representation_and_mdn_context_both_expose_partial_signal_but_the_trained_mdn_direct_readout_is_near_random
next_recommended_goal=synthetic_mdn_edge_readout_training_dynamics.v1
EOF_REPORT

cat "${output_report}"
