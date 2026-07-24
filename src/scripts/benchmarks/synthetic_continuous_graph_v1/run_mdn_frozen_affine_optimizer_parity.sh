#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_affine_optimizer_parity_v1"
runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"
build_dir="${runtime_dir}/bin"
helper_src="${script_dir}/mdn_cached_feature_runtime_head_parity.cpp"
helper_bin="${build_dir}/mdn_cached_feature_runtime_head_parity"
production_header="${repo_root}/src/include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
split_dsl="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.splits.dsl"
canonical_ridge_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_train_eval_ridge_gate.v1.report"

train_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
train_probe="${train_dir}/mdn_edge_context_features.probe"
historical_dir="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001"
historical_probe="${historical_dir}/mdn_edge_context_features.probe"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

expected_train_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_historical_sha="477758c9a25e05138c32f70c7fb4ded1aac855aa7cd2beeff7f9060708866ac5"
expected_representation_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"

baseline_probe="${runtime_dir}/${schema_id}.baseline.probe"
baseline_replay_probe="${runtime_dir}/${schema_id}.baseline.replay.probe"
parity_probe="${runtime_dir}/${schema_id}.registered_copy.probe"
parity_replay_probe="${runtime_dir}/${schema_id}.registered_copy.replay.probe"

fail() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" ]] || fail "missing required file: $1"
}

require_sha256() {
  local path="$1"
  local expected="$2"
  local actual
  require_file "${path}"
  actual="$(sha256sum "${path}" | awk '{print $1}')"
  [[ "${actual}" == "${expected}" ]] ||
    fail "SHA-256 mismatch for ${path}: expected ${expected}, got ${actual}"
}

probe_value() {
  local path="$1"
  local key="$2"
  local matches
  matches="$(awk -F= -v key="${key}" '$1 == key { count += 1 } END { print count + 0 }' "${path}")"
  [[ "${matches}" == "1" ]] ||
    fail "expected exactly one ${key}= entry in ${path}, found ${matches}"
  awk -F= -v key="${key}" '$1 == key { sub(/^[^=]*=/, ""); print; exit }' "${path}"
}

expect_value() {
  local path="$1"
  local key="$2"
  local expected="$3"
  local actual
  actual="$(probe_value "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "unexpected ${key} in ${path}: expected ${expected}, got ${actual}"
}

expect_abs_le() {
  local path="$1"
  local key="$2"
  local tolerance="$3"
  local actual
  actual="$(probe_value "${path}" "${key}")"
  awk -v value="${actual}" -v tolerance="${tolerance}" \
    'BEGIN {
       lower = tolower(value)
       if (lower ~ /nan/ || lower ~ /inf/) exit 1
       numeric = value + 0
       if ((numeric < 0 ? -numeric : numeric) > tolerance) exit 1
     }' ||
    fail "${key}=${actual} exceeds absolute tolerance ${tolerance}"
}

validate_probe() {
  local path="$1"
  local expected_begin="$2"
  local expected_end="$3"
  local expected_rows="$4"
  python3 - "${path}" "${expected_begin}" "${expected_end}" "${expected_rows}" <<'PY'
import csv
import math
import sys
from collections import Counter

path, begin_text, end_text, rows_text = sys.argv[1:]
begin = int(begin_text)
end = int(end_text)
expected_rows = int(rows_text)
expected_header = [
    "record_schema", "anchor_key", "anchor_index", "anchor_local_index",
    "edge_index", "edge_id", "base_node_id", "quote_node_id",
    "channel_index", "target_edge_close_return", "feature_count",
    "feature_values",
]
counts = Counter()
keys = set()
row_count = 0
with open(path, newline="", encoding="utf-8") as handle:
    reader = csv.reader(handle)
    header = next(reader, None)
    if header != expected_header:
        raise SystemExit(f"unexpected probe header in {path}")
    for row in reader:
        row_count += 1
        if len(row) != 12:
            raise SystemExit(f"row {row_count} has {len(row)} columns")
        anchor = int(row[2])
        edge = int(row[4])
        channel = int(row[8])
        target = float(row[9])
        feature_count = int(row[10])
        features = row[11].split(";") if row[11] else []
        if anchor >= 1088:
            raise SystemExit(f"sealed holdout anchor opened: {anchor}")
        if not begin <= anchor < end:
            raise SystemExit(f"anchor {anchor} outside [{begin},{end})")
        if edge not in range(3) or channel not in range(3):
            raise SystemExit(f"unexpected edge/channel at anchor {anchor}")
        if not math.isfinite(target):
            raise SystemExit(f"non-finite target at anchor {anchor}")
        if feature_count != 400 or len(features) != 400:
            raise SystemExit(f"unexpected feature width at anchor {anchor}")
        if any(not math.isfinite(float(value)) for value in features):
            raise SystemExit(f"non-finite feature at anchor {anchor}")
        key = (anchor, edge, channel)
        if key in keys:
            raise SystemExit(f"duplicate key {key}")
        keys.add(key)
        counts[anchor] += 1
if row_count != expected_rows:
    raise SystemExit(f"expected {expected_rows} rows, got {row_count}")
if set(counts) != set(range(begin, end)):
    raise SystemExit("anchor set is not the exact expected contiguous range")
if any(counts[anchor] != 9 for anchor in range(begin, end)):
    raise SystemExit("expected exactly nine edge/channel rows per anchor")
print(f"validated {path}: anchors=[{begin},{end}) rows={row_count} width=400")
PY
}

preflight() {
  require_sha256 "${train_probe}" "${expected_train_sha}"
  require_sha256 "${historical_probe}" "${expected_historical_sha}"
  require_sha256 "${representation_checkpoint}" "${expected_representation_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_sha}"
  require_file "${helper_src}"
  require_file "${production_header}"
  require_file "${split_dsl}"
  require_file "${canonical_ridge_report}"
  [[ ! -e "${train_dir}/stage.sha256" ]] ||
    fail "legacy train capture unexpectedly acquired a stage seal; re-audit provenance"
  [[ ! -e "${historical_dir}/stage.sha256" ]] ||
    fail "legacy historical capture unexpectedly acquired a stage seal; re-audit provenance"
  validate_probe "${train_probe}" 0 730 6570
  validate_probe "${historical_probe}" 760 1088 2952
}

compile_helper() {
  local libtorch_path="${repo_root}/.external/libtorch-upd"
  local cuda_path="/usr/local/cuda-12.4"
  mkdir -p "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${repo_root}/src" \
    -I"${repo_root}/src/include" \
    -I"${repo_root}/src/include/torch_compat" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${helper_src}" \
    -L"${libtorch_path}/lib" \
    -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${helper_bin}"
}

write_provenance() {
  mkdir -p "${runtime_dir}"
  {
    echo "authority=verified_legacy_content_manifest_not_original_stage_seal"
    echo "diagnostic_authority=diagnostic_only"
    echo "train_anchor_range=[0,730)"
    echo "historical_confirmation_anchor_range=[760,1088)"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "target=raw_future_base_close_minus_future_quote_close"
    echo "train_original_stage_seal_present=false"
    echo "historical_original_stage_seal_present=false"
  } > "${runtime_dir}/capture_provenance.status"

  : > "${runtime_dir}/legacy_capture_inputs.sha256"
  while IFS= read -r -d '' path; do
    sha256sum "${path}" >> "${runtime_dir}/legacy_capture_inputs.sha256"
  done < <(find "${train_dir}" "${historical_dir}" -maxdepth 1 -type f -print0 | sort -z)
  sha256sum "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${split_dsl}" "${canonical_ridge_report}" \
    >> "${runtime_dir}/legacy_capture_inputs.sha256"

  g++ --version > "${runtime_dir}/compiler.version"
  ldd "${helper_bin}" > "${runtime_dir}/linked_libraries.txt"
  readelf -d "${helper_bin}" > "${runtime_dir}/binary.dynamic.txt"
  : > "${runtime_dir}/resolved_libraries.sha256"
  while IFS= read -r library; do
    [[ -n "${library}" && -f "${library}" ]] || continue
    sha256sum "${library}" >> "${runtime_dir}/resolved_libraries.sha256"
  done < <(awk '/=> \// {print $3} /^[[:space:]]*\// {print $1}' \
    "${runtime_dir}/linked_libraries.txt" | sort -u)

  {
    echo "schema_id=${schema_id}"
    echo "helper_source_sha256=$(sha256sum "${helper_src}" | awk '{print $1}')"
    echo "production_header_sha256=$(sha256sum "${production_header}" | awk '{print $1}')"
    echo "helper_binary_sha256=$(sha256sum "${helper_bin}" | awk '{print $1}')"
    echo "input_train_probe_sha256=${expected_train_sha}"
    echo "input_historical_probe_sha256=${expected_historical_sha}"
    echo "identity_modes=edge_embedding_per_edge"
    echo "feature_dim=128"
    echo "identity_embedding_dim=16"
    echo "steps=3500"
    echo "batch_size=64"
    echo "overfit_anchors=16"
    echo "overfit_steps=2000"
    echo "standardized_linear_steps=3500"
    echo "seed=31"
    echo "ridge_validation_fraction=0.20"
    echo "ridge_validation_gap=30"
    echo "ridge_direction_gate=0.95"
    echo "ridge_rank_gate=0.95"
    echo "learning_rate=0.001"
    echo "grad_clip_norm=5.0"
    echo "objective_weights=100.0,5.0,5.0"
    echo "target_scale=36.0"
  } > "${runtime_dir}/execution_inputs.status"
}

common_args=(
  --input "${train_probe}"
  --evaluation-input "${historical_probe}"
  --identity-modes edge_embedding_per_edge
  --feature-dim 128
  --identity-embedding-dim 16
  --steps 3500
  --batch-size 64
  --overfit-anchors 16
  --overfit-steps 2000
  --standardized-linear-steps 3500
  --seed 31
  --fit-fraction 0.70
  --ridge-alphas 1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1,10
  --ridge-validation-fraction 0.20
  --ridge-validation-gap 30
  --ridge-direction-gate 0.95
  --ridge-rank-gate 0.95
  --learning-rate 0.001
  --grad-clip-norm 5.0
  --regression-weight 100.0
  --direction-weight 5.0
  --rank-weight 5.0
  --huber-beta 0.5
  --logit-scale 1.0
  --target-scale 36.0
  --margin-eps 0.001
  --rank-margin-eps 0.001
)

run_probe() {
  local output="$1"
  local run_schema="$2"
  local log="$3"
  shift 3
  "${helper_bin}" "${common_args[@]}" \
    --output "${output}" \
    --schema-id "${run_schema}" \
    "$@" > "${log}"
}

validate_common_output() {
  local path="$1"
  expect_value "${path}" anchor_count 1058
  expect_value "${path}" primary_anchor_count 730
  expect_value "${path}" anchor_min 0
  expect_value "${path}" anchor_max 1087
  expect_value "${path}" fit_anchor_count 730
  expect_value "${path}" holdout_anchor_count 328
  expect_value "${path}" fit_anchor_max 729
  expect_value "${path}" holdout_anchor_min 760
  expect_value "${path}" edge_count 3
  expect_value "${path}" channel_count 3
  expect_value "${path}" feature_dim 128
  expect_value "${path}" source_feature_count 400
  expect_value "${path}" dynamic_feature_count 384
  expect_value "${path}" ignored_cached_identity_feature_count 16
  expect_value "${path}" reconstruction_max_abs_error 0.000000000000
  expect_value "${path}" quote_consistency_max_abs_error 0.000000000000
  expect_value "${path}" ridge.selection_train_anchor_range '[0,554)'
  expect_value "${path}" ridge.validation_purge_anchor_range '[554,584)'
  expect_value "${path}" ridge.validation_anchor_range '[584,730)'
  expect_value "${path}" ridge.diagnostic_confirmation_anchor_range '[760,1088)'
  expect_value "${path}" ridge.selected_alpha 0.000000000100
  expect_value "${path}" ridge.selected.validation.rmse 0.020836575958
  expect_value "${path}" ridge.selected.validation.directional_accuracy 0.805936073059
  expect_value "${path}" ridge.selected.validation.pairwise_rank_accuracy 0.793759512938
  expect_value "${path}" ridge.refit.development.rmse 0.016640222725
  expect_value "${path}" ridge.refit.development.directional_accuracy 0.841704718417
  expect_value "${path}" ridge.refit.development.pairwise_rank_accuracy 0.824961948250
  expect_value "${path}" ridge.refit.diagnostic_confirmation.rmse 0.018692893967
  expect_value "${path}" ridge.refit.diagnostic_confirmation.directional_accuracy 0.810975609756
  expect_value "${path}" ridge.refit.diagnostic_confirmation.pairwise_rank_accuracy 0.785907859079
}

validate_baseline_output() {
  local path="$1"
  validate_common_output "${path}"
  expect_value "${path}" device cuda
  expect_value "${path}" affine_parity_only false
  expect_value "${path}" mechanical_production_adam_training_skipped false
  expect_value "${path}" mechanical_overfit.pass false
  expect_value "${path}" variant.edge_embedding_per_edge.fit.rmse 0.028358779025
  expect_value "${path}" variant.edge_embedding_per_edge.fit.directional_accuracy 0.483257229833
  expect_value "${path}" variant.edge_embedding_per_edge.fit.pairwise_rank_accuracy 0.512937595129
  expect_value "${path}" variant.edge_embedding_per_edge.holdout.rmse 0.028395306240
  expect_value "${path}" variant.edge_embedding_per_edge.holdout.directional_accuracy 0.481029810298
  expect_value "${path}" variant.edge_embedding_per_edge.holdout.pairwise_rank_accuracy 0.508130081301
  expect_value "${path}" control.standardized_per_edge_linear.fit.rmse 0.035385793004
  expect_value "${path}" control.standardized_per_edge_linear.fit.directional_accuracy 0.543835616438
  expect_value "${path}" control.standardized_per_edge_linear.fit.pairwise_rank_accuracy 0.553424657534
  expect_value "${path}" control.standardized_per_edge_linear.holdout.rmse 0.035622412621
  expect_value "${path}" control.standardized_per_edge_linear.holdout.directional_accuracy 0.537940379404
  expect_value "${path}" control.standardized_per_edge_linear.holdout.pairwise_rank_accuracy 0.550813008130
  if grep -q '^affine_parity\.enabled=' "${path}"; then
    fail "baseline unexpectedly emitted affine-copy parity results"
  fi
}

validate_parity_output() {
  local path="$1"
  validate_common_output "${path}"
  expect_value "${path}" device cpu
  expect_value "${path}" affine_parity_only true
  expect_value "${path}" mechanical_production_adam_training_skipped true
  expect_value "${path}" affine_parity.enabled true
  expect_value "${path}" affine_parity.compute_device cpu
  expect_value "${path}" affine_parity.compute_dtype float64
  expect_value "${path}" affine_parity.output_dtype float32
  expect_value "${path}" affine_parity.development.prediction_count 6570
  expect_value "${path}" affine_parity.diagnostic_confirmation.prediction_count 2952
  expect_value "${path}" affine_parity.development.pass true
  expect_value "${path}" affine_parity.diagnostic_confirmation.pass true
  expect_value "${path}" affine_parity.pass true
  expect_abs_le "${path}" affine_parity.development.max_abs_delta 0.000001
  expect_abs_le "${path}" affine_parity.development.directional_accuracy_delta 0.000000001
  expect_abs_le "${path}" affine_parity.development.pairwise_rank_accuracy_delta 0.000000001
  expect_abs_le "${path}" affine_parity.development.rmse_delta 0.000000001
  expect_abs_le "${path}" affine_parity.diagnostic_confirmation.max_abs_delta 0.000001
  expect_abs_le "${path}" affine_parity.diagnostic_confirmation.directional_accuracy_delta 0.000000001
  expect_abs_le "${path}" affine_parity.diagnostic_confirmation.pairwise_rank_accuracy_delta 0.000000001
  expect_abs_le "${path}" affine_parity.diagnostic_confirmation.rmse_delta 0.000000001
  if grep -q '^variant\.' "${path}" ||
     grep -q '^control\.standardized_per_edge_linear\.' "${path}" ||
     grep -q '^mechanical_overfit\.' "${path}"; then
    fail "affine-parity-only output contains skipped training results"
  fi
}

seal_outputs() {
  sha256sum "${baseline_probe}" "${baseline_replay_probe}" \
    "${parity_probe}" "${parity_replay_probe}" \
    > "${runtime_dir}/outputs.sha256"

  sha256sum \
    "${BASH_SOURCE[0]}" \
    "${helper_src}" \
    "${production_header}" \
    "${helper_bin}" \
    "${runtime_dir}/capture_provenance.status" \
    "${runtime_dir}/legacy_capture_inputs.sha256" \
    "${runtime_dir}/compiler.version" \
    "${runtime_dir}/linked_libraries.txt" \
    "${runtime_dir}/binary.dynamic.txt" \
    "${runtime_dir}/resolved_libraries.sha256" \
    "${runtime_dir}/execution_inputs.status" \
    "${runtime_dir}/outputs.sha256" \
    "${baseline_probe}" "${baseline_replay_probe}" \
    "${parity_probe}" "${parity_replay_probe}" \
    "${runtime_dir}/baseline.stdout.log" \
    "${runtime_dir}/baseline.replay.stdout.log" \
    "${runtime_dir}/registered_copy.stdout.log" \
    "${runtime_dir}/registered_copy.replay.stdout.log" \
    > "${runtime_dir}/master.sha256"
}

run_all() {
  mkdir -p "${runtime_dir}"
  preflight
  compile_helper
  write_provenance

  run_probe "${baseline_probe}" "${schema_id}.baseline" \
    "${runtime_dir}/baseline.stdout.log"
  run_probe "${baseline_replay_probe}" "${schema_id}.baseline" \
    "${runtime_dir}/baseline.replay.stdout.log"
  cmp -s "${baseline_probe}" "${baseline_replay_probe}" ||
    fail "baseline replay is not byte-identical"
  validate_baseline_output "${baseline_probe}"

  run_probe "${parity_probe}" "${schema_id}.registered_copy" \
    "${runtime_dir}/registered_copy.stdout.log" --affine-parity-only
  run_probe "${parity_replay_probe}" "${schema_id}.registered_copy" \
    "${runtime_dir}/registered_copy.replay.stdout.log" --affine-parity-only
  cmp -s "${parity_probe}" "${parity_replay_probe}" ||
    fail "registered-copy replay is not byte-identical"
  validate_parity_output "${parity_probe}"

  seal_outputs
  sha256sum -c "${runtime_dir}/legacy_capture_inputs.sha256" >/dev/null
  sha256sum -c "${runtime_dir}/outputs.sha256" >/dev/null
  sha256sum -c "${runtime_dir}/master.sha256" >/dev/null
  echo "${runtime_dir}"
}

mode="${1:---run}"
case "${mode}" in
  --preflight)
    preflight
    ;;
  --run)
    run_all
    ;;
  *)
    fail "usage: $0 [--preflight|--run]"
    ;;
esac
