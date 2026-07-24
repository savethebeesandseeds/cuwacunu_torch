#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_affine_calibration_sidecar_v1"
source_capture_schema="synthetic_mdn_frozen_feature_capture.v1"
capture_authority="verified_legacy_content_manifest_not_original_stage_seal"
graph_order_fingerprint="d334e38b1887ae16"

final_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"

helper_src="${script_dir}/mdn_cached_feature_runtime_head_parity.cpp"
production_header_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
production_types_header_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
production_utils_header_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
calibration_header_rel="wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
digest_header_rel="piaabo/digest/sha256.h"
production_header="${repo_root}/src/include/${production_header_rel}"
production_types_header="${repo_root}/src/include/${production_types_header_rel}"
production_utils_header="${repo_root}/src/include/${production_utils_header_rel}"
calibration_header="${repo_root}/src/include/${calibration_header_rel}"
digest_header="${repo_root}/src/include/${digest_header_rel}"
split_dsl="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.splits.dsl"
canonical_ridge_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_train_eval_ridge_gate.v1.report"

train_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
train_probe="${train_dir}/mdn_edge_context_features.probe"
train_manifest="${train_dir}/job.manifest"
train_report="${train_dir}/channel_inference.report"
historical_dir="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001"
historical_probe="${historical_dir}/mdn_edge_context_features.probe"
historical_manifest="${historical_dir}/job.manifest"
historical_report="${historical_dir}/channel_inference.report"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

expected_train_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_historical_sha="477758c9a25e05138c32f70c7fb4ded1aac855aa7cd2beeff7f9060708866ac5"
expected_representation_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  helper_bin="${build_dir}/mdn_cached_feature_runtime_head_parity"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  # Distinct basenames intentionally expose filename-derived archive bytes.
  # Archive byte identity is observed, never an acceptance condition.
  main_archive="${main_dir}/${schema_id}.main.pt"
  replay_archive="${replay_dir}/${schema_id}.replay.pt"
  main_metadata="${main_dir}/${schema_id}.metadata"
  replay_metadata="${replay_dir}/${schema_id}.metadata"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
  main_pair_manifest="${main_dir}/pair.sha256"
  replay_pair_manifest="${replay_dir}/pair.sha256"
  archive_observation="${runtime_dir}/archive_byte_identity.status"
}

set_runtime_paths "${final_runtime_dir}"

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
  matches="$(awk -F= -v key="${key}" \
    '$1 == key { count += 1 } END { print count + 0 }' "${path}")"
  [[ "${matches}" == "1" ]] ||
    fail "expected exactly one ${key}= entry in ${path}, found ${matches}"
  awk -F= -v key="${key}" \
    '$1 == key { sub(/^[^=]*=/, ""); print; exit }' "${path}"
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

expect_all() {
  local path="$1"
  shift
  local item
  for item in "$@"; do
    expect_value "${path}" "${item%%=*}" "${item#*=}"
  done
}

expect_contains() {
  local path="$1"
  local key="$2"
  local fragment="$3"
  local actual
  actual="$(probe_value "${path}" "${key}")"
  [[ "${actual}" == *"${fragment}"* ]] ||
    fail "${key} in ${path} does not contain ${fragment}"
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
       if (value !~ /^[-+]?(([0-9]+([.][0-9]*)?)|([.][0-9]+))([eE][-+]?[0-9]+)?$/) {
         exit 1
       }
       numeric = value + 0
       if ((numeric < 0 ? -numeric : numeric) > tolerance) exit 1
     }' ||
    fail "${key}=${actual} exceeds absolute tolerance ${tolerance}"
}

expect_sha256_value() {
  local value
  value="$(probe_value "$1" "$2")"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "$2 in $1 is not a lowercase SHA-256"
}

expect_same_value() {
  local lhs
  local rhs
  lhs="$(probe_value "$1" "$3")"
  rhs="$(probe_value "$2" "$3")"
  [[ "${lhs}" == "${rhs}" ]] ||
    fail "$3 differs between $1 and $2"
}

expect_keys_equal() {
  local lhs
  local rhs
  lhs="$(probe_value "$1" "$2")"
  rhs="$(probe_value "$1" "$3")"
  [[ "${lhs}" == "${rhs}" ]] ||
    fail "$2 and $3 differ in $1"
}

require_source_token() {
  grep -Fq -- "$1" "${helper_src}" ||
    fail "comparator calibration contract is not ready: missing $1"
}

validate_probe() {
  local path="$1"
  local expected_begin="$2"
  local expected_end="$3"
  local expected_rows="$4"
  python3 - "${path}" "${expected_begin}" "${expected_end}" \
    "${expected_rows}" <<'PY'
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
expected_edges = {
    0: ("SYNALPHASYNUSD", "SYNALPHA", "SYNUSD"),
    1: ("SYNBETASYNUSD", "SYNBETA", "SYNUSD"),
    2: ("SYNGAMMASYNUSD", "SYNGAMMA", "SYNUSD"),
}
counts = Counter()
keys = set()
row_count = 0
with open(path, newline="", encoding="utf-8") as handle:
    reader = csv.reader(handle)
    if next(reader, None) != expected_header:
        raise SystemExit(f"unexpected probe header in {path}")
    for row in reader:
        row_count += 1
        if len(row) != 12:
            raise SystemExit(f"row {row_count} has {len(row)} columns")
        if row[0] != "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1":
            raise SystemExit(f"unexpected record schema at row {row_count}")
        anchor = int(row[2])
        edge = int(row[4])
        channel = int(row[8])
        target = float(row[9])
        feature_count = int(row[10])
        features = row[11].split(";") if row[11] else []
        if anchor >= 1088:
            raise SystemExit(f"sealed final holdout anchor opened: {anchor}")
        if not begin <= anchor < end:
            raise SystemExit(f"anchor {anchor} outside [{begin},{end})")
        if edge not in expected_edges or channel not in range(3):
            raise SystemExit(f"unexpected edge/channel at anchor {anchor}")
        if tuple(row[5:8]) != expected_edges[edge]:
            raise SystemExit(f"unexpected graph identity at anchor {anchor}")
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

validate_split_contract() {
  python3 - "${split_dsl}" <<'PY'
import re
import sys

text = open(sys.argv[1], encoding="utf-8").read()
blocks = re.findall(r"UJCAMEI_SOURCE_SPLIT\s*\{([^{}]*)\};", text, re.DOTALL)
matches = [
    block for block in blocks
    if re.search(r"SPLIT_ID\s*=\s*test_holdout\s*;", block)
]
if len(matches) != 1:
    raise SystemExit(f"expected one test_holdout split, found {len(matches)}")
body = matches[0]
for expected in (
    r"ROLE\s*=\s*test\s*;",
    r"SELECTOR\s*=\s*fraction_range\s*;",
    r"BEGIN_FRACTION\s*=\s*93/100\s*;",
    r"END_FRACTION\s*=\s*100/100\s*;",
):
    if not re.search(expected, body):
        raise SystemExit(f"test_holdout does not satisfy {expected}")
print("validated sealed test_holdout: [93/100,100/100) = [1088,1170)")
PY
}

preflight_inputs() {
  require_sha256 "${train_probe}" "${expected_train_sha}"
  require_sha256 "${historical_probe}" "${expected_historical_sha}"
  require_sha256 "${representation_checkpoint}" "${expected_representation_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_sha}"
  require_file "${train_manifest}"
  require_file "${historical_manifest}"
  require_file "${train_report}"
  require_file "${historical_report}"
  require_file "${split_dsl}"
  require_file "${canonical_ridge_report}"
  [[ ! -e "${train_dir}/stage.sha256" ]] ||
    fail "legacy train capture unexpectedly acquired a stage seal"
  [[ ! -e "${historical_dir}/stage.sha256" ]] ||
    fail "legacy historical capture unexpectedly acquired a stage seal"

  local train_temp_config
  train_temp_config="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/.mdn_frozen_capture.cWUmyd.config"
  expect_value "${train_manifest}" config_path "${train_temp_config}"
  [[ ! -e "${train_temp_config}" ]] ||
    fail "legacy temporary train config unexpectedly exists"

  expect_value "${train_manifest}" graph_order_fingerprint \
    "${graph_order_fingerprint}"
  expect_value "${historical_manifest}" graph_order_fingerprint \
    "${graph_order_fingerprint}"
  expect_value "${train_report}" graph_order_fingerprint \
    "${graph_order_fingerprint}"
  expect_value "${historical_report}" graph_order_fingerprint \
    "${graph_order_fingerprint}"
  expect_contains "${train_manifest}" source_cursor_token "accepted=1170"
  expect_contains "${historical_manifest}" source_cursor_token "accepted=1170"
  validate_probe "${train_probe}" 0 730 6570
  validate_probe "${historical_probe}" 760 1088 2952
  validate_split_contract
}

preflight_sources() {
  require_file "${helper_src}"
  require_file "${production_header}"
  require_file "${production_types_header}"
  require_file "${production_utils_header}"
  require_file "${calibration_header}"
  require_file "${digest_header}"
  for token in \
    "${calibration_header_rel}" \
    "--affine-calibration-output" \
    "--affine-calibration-metadata-output" \
    "--affine-calibration-source-schema" \
    "--affine-calibration-fit-probe-sha256" \
    "--affine-calibration-representation-checkpoint-sha256" \
    "--affine-calibration-mdn-checkpoint-sha256" \
    "--affine-calibration-graph-fingerprint" \
    "--affine-calibration-capture-authority" \
    "affine_calibration.semantic_tensor_digest" \
    "affine_calibration.fit_probe_sha256_verified" \
    "affine_calibration.feature_prefix_layout_torch_equal" \
    "affine_calibration.context_feature_prefix_digest" \
    "affine_calibration.cached_feature_prefix_digest" \
    "affine_calibration.state_roundtrip_torch_equal" \
    "affine_calibration.full_reloaded_context_prediction_digest"; do
    require_source_token "${token}"
  done
}

preflight() {
  preflight_inputs
  preflight_sources
}

copy_frozen_header() {
  local destination="${frozen_include_root}/$2"
  mkdir -p "$(dirname "${destination}")"
  cp -- "$1" "${destination}"
}

freeze_sources() {
  mkdir -p "${frozen_source_dir}" "${frozen_include_root}"
  cp -- "${helper_src}" \
    "${frozen_source_dir}/mdn_cached_feature_runtime_head_parity.cpp"
  cp -- "${BASH_SOURCE[0]}" \
    "${frozen_source_dir}/run_mdn_frozen_affine_calibration_sidecar.sh"
  copy_frozen_header "${production_header}" "${production_header_rel}"
  copy_frozen_header "${production_types_header}" \
    "${production_types_header_rel}"
  copy_frozen_header "${production_utils_header}" \
    "${production_utils_header_rel}"
  copy_frozen_header "${calibration_header}" "${calibration_header_rel}"
  copy_frozen_header "${digest_header}" "${digest_header_rel}"
  cmp -s "${helper_src}" \
    "${frozen_source_dir}/mdn_cached_feature_runtime_head_parity.cpp" ||
    fail "frozen comparator copy drifted during capture"
  cmp -s "${BASH_SOURCE[0]}" \
    "${frozen_source_dir}/run_mdn_frozen_affine_calibration_sidecar.sh" ||
    fail "frozen runner copy drifted during capture"
}

compile_helper() {
  local libtorch_path="${repo_root}/.external/libtorch-upd"
  local cuda_path="/usr/local/cuda-12.4"
  mkdir -p "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${frozen_include_root}" \
    -I"${repo_root}/src" \
    -I"${repo_root}/src/include" \
    -I"${repo_root}/src/include/torch_compat" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${frozen_source_dir}/mdn_cached_feature_runtime_head_parity.cpp" \
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
  {
    echo "authority=${capture_authority}"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "source_capture_schema=${source_capture_schema}"
    echo "train_anchor_range=[0,730)"
    echo "train_role=development_fit"
    echo "historical_confirmation_anchor_range=[760,1088)"
    echo "historical_confirmation_role=previously_consumed_diagnostic_confirmation"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "target=raw_future_base_close_minus_future_quote_close"
    echo "probe_boundary=post_adapter_direct_edge_features"
    echo "runtime_adapter_reapplied=false"
    echo "cached_feature_prefix_layout=base_quote_base_minus_quote_prefix.v1"
    echo "comparator_generic_holdout_label_scope=legacy_internal_name_for_previously_consumed_diagnostic_confirmation"
    echo "train_original_stage_seal_present=false"
    echo "historical_original_stage_seal_present=false"
    echo "train_original_temporary_config_present=false"
  } > "${runtime_dir}/capture_provenance.status"

  : > "${runtime_dir}/legacy_capture_inputs.sha256"
  while IFS= read -r -d '' path; do
    sha256sum "${path}" >> "${runtime_dir}/legacy_capture_inputs.sha256"
  done < <(find "${train_dir}" "${historical_dir}" -maxdepth 1 -type f \
    -print0 | sort -z)
  sha256sum "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${split_dsl}" "${canonical_ridge_report}" \
    >> "${runtime_dir}/legacy_capture_inputs.sha256"

  local compiler_path
  compiler_path="$(readlink -f "$(command -v g++)")"
  {
    echo "compiler_command=g++"
    echo "compiler_resolved_path=${compiler_path}"
  } > "${runtime_dir}/compiler.identity"
  g++ --version > "${runtime_dir}/compiler.version"
  sha256sum "${compiler_path}" > "${runtime_dir}/compiler.sha256"
  (
    cd "${runtime_dir}"
    sha256sum bin/mdn_cached_feature_runtime_head_parity > binary.sha256
    sha256sum \
      frozen_sources/mdn_cached_feature_runtime_head_parity.cpp \
      frozen_sources/run_mdn_frozen_affine_calibration_sidecar.sh \
      > frozen_sources.sha256
    sha256sum \
      "frozen_include/${production_header_rel}" \
      "frozen_include/${production_types_header_rel}" \
      "frozen_include/${production_utils_header_rel}" \
      "frozen_include/${calibration_header_rel}" \
      "frozen_include/${digest_header_rel}" \
      > public_headers.sha256
  )

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
    echo "source_capture_schema=${source_capture_schema}"
    echo "capture_authority=${capture_authority}"
    echo "graph_order_fingerprint=${graph_order_fingerprint}"
    echo "input_train_probe_sha256=${expected_train_sha}"
    echo "input_historical_probe_sha256=${expected_historical_sha}"
    echo "representation_checkpoint_sha256=${expected_representation_sha}"
    echo "mdn_checkpoint_sha256=${expected_mdn_sha}"
    echo "identity_modes=edge_embedding_per_edge"
    echo "feature_dim=128"
    echo "readout_feature_dim=400"
    echo "dynamic_feature_dim=384"
    echo "identity_embedding_dim=16"
    echo "edge_count=3"
    echo "channel_count=3"
    echo "selected_alpha=1e-10"
    echo "selection_train_anchor_range=[0,554)"
    echo "validation_purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "refit_anchor_range=[0,730)"
    echo "valid_from_anchor=730"
    echo "diagnostic_confirmation_anchor_range=[760,1088)"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
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
    echo "main_archive_basename=$(basename "${main_archive}")"
    echo "replay_archive_basename=$(basename "${replay_archive}")"
    echo "stdout_log_path_normalization=candidate_runtime_prefix_to_final_runtime_prefix"
    echo "result_probe_byte_identity_required=true"
    echo "metadata_byte_identity_required=true"
    echo "archive_byte_identity_required=false"
    echo "fit_probe_sha256_verification_required=true"
    echo "context_cached_feature_prefix_identity_required=true"
    echo "semantic_tensor_digest_identity_required=true"
    echo "exact_loaded_state_required=true"
    echo "exact_loaded_predictions_required=true"
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
  "${helper_bin}" "${common_args[@]}" \
    --output "$1" \
    --schema-id "${schema_id}" \
    --affine-parity-only \
    --affine-calibration-output "$2" \
    --affine-calibration-metadata-output "$3" \
    --affine-calibration-source-schema "${source_capture_schema}" \
    --affine-calibration-fit-probe-sha256 "${expected_train_sha}" \
    --affine-calibration-representation-checkpoint-sha256 \
      "${expected_representation_sha}" \
    --affine-calibration-mdn-checkpoint-sha256 "${expected_mdn_sha}" \
    --affine-calibration-graph-fingerprint "${graph_order_fingerprint}" \
    --affine-calibration-capture-authority "${capture_authority}" \
    2>&1 |
    python3 -c \
      'import sys; old, new = sys.argv[1:]; sys.stdout.write(sys.stdin.read().replace(old, new))' \
      "${runtime_dir}" "${final_runtime_dir}" > "$4"
}

validate_common_output() {
  local path="$1"
  expect_all "${path}" \
    "schema_id=${schema_id}" \
    "anchor_count=1058" \
    "primary_anchor_count=730" \
    "anchor_min=0" \
    "anchor_max=1087" \
    "fit_anchor_count=730" \
    "holdout_anchor_count=328" \
    "fit_anchor_max=729" \
    "holdout_anchor_min=760" \
    "edge_count=3" \
    "channel_count=3" \
    "feature_dim=128" \
    "source_feature_count=400" \
    "dynamic_feature_count=384" \
    "ignored_cached_identity_feature_count=16" \
    "reconstruction_max_abs_error=0.000000000000" \
    "quote_consistency_max_abs_error=0.000000000000" \
    "ridge.selection_train_anchor_range=[0,554)" \
    "ridge.validation_purge_anchor_range=[554,584)" \
    "ridge.validation_anchor_range=[584,730)" \
    "ridge.diagnostic_confirmation_anchor_range=[760,1088)" \
    "ridge.selected_alpha=0.000000000100" \
    "ridge.selected.validation.rmse=0.020836575958" \
    "ridge.selected.validation.directional_accuracy=0.805936073059" \
    "ridge.selected.validation.pairwise_rank_accuracy=0.793759512938" \
    "ridge.refit.development.rmse=0.016640222725" \
    "ridge.refit.development.directional_accuracy=0.841704718417" \
    "ridge.refit.development.pairwise_rank_accuracy=0.824961948250" \
    "ridge.refit.diagnostic_confirmation.rmse=0.018692893967" \
    "ridge.refit.diagnostic_confirmation.directional_accuracy=0.810975609756" \
    "ridge.refit.diagnostic_confirmation.pairwise_rank_accuracy=0.785907859079"
}

validate_parity_output() {
  local path="$1"
  validate_common_output "${path}"
  expect_all "${path}" \
    "device=cpu" \
    "affine_parity_only=true" \
    "mechanical_production_adam_training_skipped=true" \
    "affine_parity.enabled=true" \
    "affine_parity.compute_device=cpu" \
    "affine_parity.compute_dtype=float64" \
    "affine_parity.output_dtype=float32" \
    "affine_parity.development.prediction_count=6570" \
    "affine_parity.diagnostic_confirmation.prediction_count=2952" \
    "affine_parity.development.pass=true" \
    "affine_parity.diagnostic_confirmation.pass=true" \
    "affine_parity.pass=true"
  local key
  for key in \
    affine_parity.development.max_abs_delta \
    affine_parity.diagnostic_confirmation.max_abs_delta; do
    expect_abs_le "${path}" "${key}" 0.000001
  done
  for key in \
    affine_parity.development.directional_accuracy_delta \
    affine_parity.development.pairwise_rank_accuracy_delta \
    affine_parity.development.rmse_delta \
    affine_parity.diagnostic_confirmation.directional_accuracy_delta \
    affine_parity.diagnostic_confirmation.pairwise_rank_accuracy_delta \
    affine_parity.diagnostic_confirmation.rmse_delta; do
    expect_abs_le "${path}" "${key}" 0.000000001
  done
  if grep -q '^variant\.' "${path}" ||
     grep -q '^control\.standardized_per_edge_linear\.' "${path}" ||
     grep -q '^mechanical_overfit\.' "${path}"; then
    fail "affine-parity-only output contains skipped training results"
  fi
}

validate_calibration_output() {
  local path="$1"
  expect_all "${path}" \
    "affine_calibration.enabled=true" \
    "affine_calibration.artifact_family=wikimyei.inference.expected_value.mdn.per_edge_affine_return_head.v1" \
    "affine_calibration.schema_version=1" \
    "affine_calibration.diagnostic_only=true" \
    "affine_calibration.feature_layout=base_quote_base_minus_quote_prefix.v1" \
    "affine_calibration.suffix_policy=ignored_after_3h" \
    "affine_calibration.archive_byte_identity_required=false" \
    "affine_calibration.statistics_scope=development_fit_only" \
    "affine_calibration.selection_train_anchor_range=[0,554)" \
    "affine_calibration.validation_purge_anchor_range=[554,584)" \
    "affine_calibration.validation_anchor_range=[584,730)" \
    "affine_calibration.refit_anchor_range=[0,730)" \
    "affine_calibration.valid_from_anchor=730" \
    "affine_calibration.fit_probe_schema_id=${source_capture_schema}" \
    "affine_calibration.fit_probe_sha256=${expected_train_sha}" \
    "affine_calibration.fit_probe_sha256_verified=true" \
    "affine_calibration.representation_checkpoint_sha256=${expected_representation_sha}" \
    "affine_calibration.mdn_checkpoint_sha256=${expected_mdn_sha}" \
    "affine_calibration.graph_order_fingerprint=${graph_order_fingerprint}" \
    "affine_calibration.legacy_capture_authority=${capture_authority}" \
    "affine_calibration.feature_prefix_layout_torch_equal=true" \
    "affine_calibration.source_state_copy_exact=true" \
    "affine_calibration.state_roundtrip_torch_equal=true" \
    "affine_calibration.metadata_roundtrip_exact=true" \
    "affine_calibration.metadata_file_roundtrip_exact=true" \
    "affine_calibration.semantic_digest_roundtrip_exact=true" \
    "affine_calibration.in_memory_parameters_frozen=true" \
    "affine_calibration.reloaded_parameters_frozen=true" \
    "affine_calibration.development.pass=true" \
    "affine_calibration.diagnostic_confirmation.pass=true" \
    "affine_calibration.pass=true"

  expect_sha256_value "${path}" affine_calibration.semantic_tensor_digest
  expect_abs_le "${path}" \
    affine_calibration.feature_prefix_layout_max_abs_delta 0
  local digest_key
  for digest_key in \
    affine_calibration.context_feature_prefix_digest \
    affine_calibration.cached_feature_prefix_digest \
    affine_calibration.full_reloaded_context_prediction_digest \
    affine_calibration.full_reloaded_readout_prediction_digest \
    affine_calibration.development.reloaded_context_prediction_digest \
    affine_calibration.development.reloaded_readout_prediction_digest \
    affine_calibration.diagnostic_confirmation.reloaded_context_prediction_digest \
    affine_calibration.diagnostic_confirmation.reloaded_readout_prediction_digest; do
    expect_sha256_value "${path}" "${digest_key}"
  done
  expect_keys_equal "${path}" \
    affine_calibration.context_feature_prefix_digest \
    affine_calibration.cached_feature_prefix_digest

  local exact_prefix
  for exact_prefix in \
    affine_calibration.full_in_memory_context_vs_exact_readout \
    affine_calibration.full_in_memory_vs_reloaded_context \
    affine_calibration.full_in_memory_vs_reloaded_readout \
    affine_calibration.full_reloaded_context_vs_readout \
    affine_calibration.development.in_memory_context_vs_exact_readout \
    affine_calibration.development.in_memory_vs_reloaded_context \
    affine_calibration.development.in_memory_vs_reloaded_readout \
    affine_calibration.development.reloaded_context_vs_readout \
    affine_calibration.diagnostic_confirmation.in_memory_context_vs_exact_readout \
    affine_calibration.diagnostic_confirmation.in_memory_vs_reloaded_context \
    affine_calibration.diagnostic_confirmation.in_memory_vs_reloaded_readout \
    affine_calibration.diagnostic_confirmation.reloaded_context_vs_readout; do
    expect_value "${path}" "${exact_prefix}.torch_equal" true
    expect_abs_le "${path}" "${exact_prefix}.max_abs_delta" 0
  done

  local split
  for split in development diagnostic_confirmation; do
    expect_abs_le "${path}" \
      "affine_calibration.${split}.analytic_vs_reloaded_context.max_abs_delta" \
      0.000001
    expect_abs_le "${path}" \
      "affine_calibration.${split}.directional_accuracy_delta" 0.000000001
    expect_abs_le "${path}" \
      "affine_calibration.${split}.pairwise_rank_accuracy_delta" 0.000000001
    expect_abs_le "${path}" \
      "affine_calibration.${split}.rmse_delta" 0.000000001
  done
}

validate_metadata() {
  python3 - "$1" "${source_capture_schema}" "${capture_authority}" \
    "${graph_order_fingerprint}" "${expected_train_sha}" \
    "${expected_representation_sha}" "${expected_mdn_sha}" <<'PY'
import math
import re
import sys

(
    path,
    source_schema,
    capture_authority,
    graph_fingerprint,
    train_sha,
    representation_sha,
    mdn_sha,
) = sys.argv[1:]
lines = open(path, encoding="utf-8").read().splitlines()
if not lines or lines[0] != "per_edge_affine_return_head_metadata.v1":
    raise SystemExit("unexpected canonical metadata preamble")
values = {}
for line in lines[1:]:
    if "=" not in line:
        raise SystemExit(f"malformed metadata line: {line}")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit(f"duplicate metadata key: {key}")
    values[key] = value

def string_value(key):
    raw = values[key]
    if ":" not in raw:
        raise SystemExit(f"{key} is not length-prefixed")
    length_text, value = raw.split(":", 1)
    if not length_text.isdigit() or len(value.encode("utf-8")) != int(length_text):
        raise SystemExit(f"{key} has an invalid length prefix")
    return value

expected_scalars = {
    "schema_version": "1",
    "diagnostic_only": "1",
    "feature_dim": "128",
    "edge_count": "3",
    "readout_feature_dim": "400",
    "dynamic_feature_dim": "384",
    "channel_count": "3",
    "quote_node_index": "0",
    "selection_train_begin": "0",
    "selection_train_end": "554",
    "validation_purge_begin": "554",
    "validation_purge_end": "584",
    "validation_begin": "584",
    "validation_end": "730",
    "refit_begin": "0",
    "refit_end": "730",
    "valid_from_anchor": "730",
    "edge_ids.count": "3",
    "node_ids.count": "4",
}
for key, expected in expected_scalars.items():
    if values.get(key) != expected:
        raise SystemExit(f"unexpected {key}: {values.get(key)!r}")
if not math.isclose(float(values["selected_alpha"]), 1.0e-10,
                    rel_tol=0.0, abs_tol=0.0):
    raise SystemExit("unexpected selected_alpha")

expected_strings = {
    "artifact_family":
        "wikimyei.inference.expected_value.mdn.per_edge_affine_return_head.v1",
    "feature_layout": "base_quote_base_minus_quote_prefix.v1",
    "suffix_policy": "ignored_after_3h",
    "normalization_layout": "shared_featurewise_population.v1",
    "coefficient_layout": "standardized_per_edge_linear.v1",
    "coefficient_source": "analytic_ridge_refit",
    "statistics_scope": "development_fit_only",
    "compute_device": "cpu",
    "compute_dtype": "float64",
    "output_dtype_policy": "match_input",
    "graph_order_fingerprint": graph_fingerprint,
    "edge_ids.0": "SYNALPHASYNUSD",
    "edge_ids.1": "SYNBETASYNUSD",
    "edge_ids.2": "SYNGAMMASYNUSD",
    "node_ids.0": "SYNUSD",
    "node_ids.1": "SYNALPHA",
    "node_ids.2": "SYNBETA",
    "node_ids.3": "SYNGAMMA",
    "fit_probe_schema_id": source_schema,
    "fit_probe_sha256": train_sha,
    "representation_checkpoint_sha256": representation_sha,
    "mdn_checkpoint_sha256": mdn_sha,
    "legacy_capture_authority": capture_authority,
}
for key, expected in expected_strings.items():
    actual = string_value(key)
    if actual != expected:
        raise SystemExit(f"unexpected {key}: {actual!r}")
digest = string_value("semantic_tensor_digest")
if not re.fullmatch(r"[0-9a-f]{64}", digest):
    raise SystemExit("semantic_tensor_digest is not a lowercase SHA-256")
expected_keys = (
    set(expected_scalars)
    | set(expected_strings)
    | {"selected_alpha", "semantic_tensor_digest"}
)
if set(values) != expected_keys:
    missing = sorted(expected_keys - set(values))
    extra = sorted(set(values) - expected_keys)
    raise SystemExit(f"metadata key mismatch: missing={missing}, extra={extra}")
print(f"validated canonical calibration metadata: {path}")
PY
}

metadata_semantic_digest() {
  local raw
  raw="$(probe_value "$1" semantic_tensor_digest)"
  [[ "${raw}" =~ ^64:([0-9a-f]{64})$ ]] ||
    fail "invalid semantic_tensor_digest encoding in $1"
  printf '%s\n' "${BASH_REMATCH[1]}"
}

verify_replay_contract() {
  validate_parity_output "${main_probe}"
  validate_parity_output "${replay_probe}"
  validate_calibration_output "${main_probe}"
  validate_calibration_output "${replay_probe}"
  validate_metadata "${main_metadata}"
  validate_metadata "${replay_metadata}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay result probes are not byte-identical"
  cmp -s "${main_metadata}" "${replay_metadata}" ||
    fail "main/replay canonical metadata are not byte-identical"

  local key
  for key in \
    affine_calibration.semantic_tensor_digest \
    affine_calibration.context_feature_prefix_digest \
    affine_calibration.cached_feature_prefix_digest \
    affine_calibration.full_reloaded_context_prediction_digest \
    affine_calibration.full_reloaded_readout_prediction_digest \
    affine_calibration.development.reloaded_context_prediction_digest \
    affine_calibration.development.reloaded_readout_prediction_digest \
    affine_calibration.diagnostic_confirmation.reloaded_context_prediction_digest \
    affine_calibration.diagnostic_confirmation.reloaded_readout_prediction_digest; do
    expect_same_value "${main_probe}" "${replay_probe}" "${key}"
  done

  local main_digest
  local replay_digest
  local main_metadata_digest
  local replay_metadata_digest
  main_digest="$(probe_value "${main_probe}" \
    affine_calibration.semantic_tensor_digest)"
  replay_digest="$(probe_value "${replay_probe}" \
    affine_calibration.semantic_tensor_digest)"
  main_metadata_digest="$(metadata_semantic_digest "${main_metadata}")"
  replay_metadata_digest="$(metadata_semantic_digest "${replay_metadata}")"
  [[ "${main_digest}" == "${replay_digest}" &&
     "${main_digest}" == "${main_metadata_digest}" &&
     "${main_digest}" == "${replay_metadata_digest}" ]] ||
    fail "semantic tensor digest mismatch across result/archive metadata replay"
}

write_archive_observation() {
  local observed="false"
  if cmp -s "${main_archive}" "${replay_archive}"; then
    observed="true"
  fi
  {
    echo "archive_byte_identity_authoritative=false"
    echo "archive_byte_identity_required=false"
    echo "archive_byte_identity_observed=${observed}"
    echo "main_archive_basename=$(basename "${main_archive}")"
    echo "replay_archive_basename=$(basename "${replay_archive}")"
    echo "main_archive_sha256=$(sha256sum "${main_archive}" | awk '{print $1}')"
    echo "replay_archive_sha256=$(sha256sum "${replay_archive}" | awk '{print $1}')"
    echo "semantic_acceptance=verified_fit_probe_and_feature_prefix_plus_semantic_digest_plus_exact_loaded_state_plus_exact_loaded_predictions"
    echo "semantic_acceptance_pass=true"
  } > "${archive_observation}"
}

validate_archive_observation() {
  local observed="false"
  if cmp -s "${main_archive}" "${replay_archive}"; then
    observed="true"
  fi
  expect_all "${archive_observation}" \
    "archive_byte_identity_authoritative=false" \
    "archive_byte_identity_required=false" \
    "archive_byte_identity_observed=${observed}" \
    "main_archive_basename=$(basename "${main_archive}")" \
    "replay_archive_basename=$(basename "${replay_archive}")" \
    "main_archive_sha256=$(sha256sum "${main_archive}" | awk '{print $1}')" \
    "replay_archive_sha256=$(sha256sum "${replay_archive}" | awk '{print $1}')" \
    "semantic_acceptance_pass=true"
}

seal_outputs() {
  write_archive_observation
  (
    cd "${runtime_dir}"
    local shared_pair_evidence=(
      bin/mdn_cached_feature_runtime_head_parity
      frozen_sources.sha256
      public_headers.sha256
      binary.sha256
      compiler.identity
      compiler.version
      compiler.sha256
      linked_libraries.txt
      binary.dynamic.txt
      resolved_libraries.sha256
      capture_provenance.status
      legacy_capture_inputs.sha256
      execution_inputs.status
    )
    sha256sum "${shared_pair_evidence[@]}" \
      "main/${schema_id}.main.pt" \
      "main/${schema_id}.metadata" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      > main/pair.sha256
    sha256sum "${shared_pair_evidence[@]}" \
      "replay/${schema_id}.replay.pt" \
      "replay/${schema_id}.metadata" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > replay/pair.sha256

    sha256sum \
      main/pair.sha256 replay/pair.sha256 \
      "main/${schema_id}.main.pt" \
      "main/${schema_id}.metadata" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.replay.pt" \
      "replay/${schema_id}.metadata" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      archive_byte_identity.status \
      > outputs.sha256

    sha256sum \
      frozen_sources/mdn_cached_feature_runtime_head_parity.cpp \
      frozen_sources/run_mdn_frozen_affine_calibration_sidecar.sh \
      "frozen_include/${production_header_rel}" \
      "frozen_include/${production_types_header_rel}" \
      "frozen_include/${production_utils_header_rel}" \
      "frozen_include/${calibration_header_rel}" \
      "frozen_include/${digest_header_rel}" \
      bin/mdn_cached_feature_runtime_head_parity \
      frozen_sources.sha256 public_headers.sha256 binary.sha256 \
      compiler.sha256 legacy_capture_inputs.sha256 outputs.sha256 \
      main/pair.sha256 replay/pair.sha256 \
      > frozen_evidence.sha256

    sha256sum \
      frozen_evidence.sha256 \
      capture_provenance.status legacy_capture_inputs.sha256 \
      compiler.identity compiler.version compiler.sha256 \
      frozen_sources.sha256 public_headers.sha256 binary.sha256 \
      linked_libraries.txt binary.dynamic.txt resolved_libraries.sha256 \
      execution_inputs.status outputs.sha256 \
      main/pair.sha256 replay/pair.sha256 \
      "main/${schema_id}.main.pt" \
      "main/${schema_id}.metadata" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.replay.pt" \
      "replay/${schema_id}.metadata" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      archive_byte_identity.status \
      > master.sha256
  )
}

verify_seals() {
  (
    cd "${runtime_dir}"
    sha256sum -c legacy_capture_inputs.sha256 >/dev/null
    sha256sum -c compiler.sha256 >/dev/null
    sha256sum -c frozen_sources.sha256 >/dev/null
    sha256sum -c public_headers.sha256 >/dev/null
    sha256sum -c binary.sha256 >/dev/null
    sha256sum -c resolved_libraries.sha256 >/dev/null
    sha256sum -c main/pair.sha256 >/dev/null
    sha256sum -c replay/pair.sha256 >/dev/null
    sha256sum -c outputs.sha256 >/dev/null
    sha256sum -c frozen_evidence.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
}

verify_complete() {
  require_file "${runtime_dir}/master.sha256"
  preflight_inputs
  verify_seals
  verify_replay_contract
  validate_archive_observation
  echo "${runtime_dir}"
}

run_all() {
  set_runtime_paths "${final_runtime_dir}"
  if [[ -f "${final_runtime_dir}/master.sha256" ]]; then
    verify_complete
    return
  fi
  [[ ! -e "${final_runtime_dir}" ]] ||
    fail "refusing to overwrite partial evidence directory: ${final_runtime_dir}"
  preflight

  local lock_dir="${final_runtime_dir}.lock"
  local candidate_dir="${final_runtime_dir}.candidate.$$"
  mkdir "${lock_dir}" ||
    fail "another calibration-sidecar installation holds ${lock_dir}"
  trap 'rmdir "${lock_dir}" 2>/dev/null || true' EXIT
  [[ ! -e "${final_runtime_dir}" ]] ||
    fail "final evidence appeared after lock acquisition: ${final_runtime_dir}"
  [[ ! -e "${candidate_dir}" ]] ||
    fail "candidate evidence directory already exists: ${candidate_dir}"

  set_runtime_paths "${candidate_dir}"
  mkdir -p "${main_dir}" "${replay_dir}"
  freeze_sources
  compile_helper
  write_provenance
  run_probe "${main_probe}" "${main_archive}" "${main_metadata}" \
    "${main_log}"
  run_probe "${replay_probe}" "${replay_archive}" "${replay_metadata}" \
    "${replay_log}"
  verify_replay_contract
  seal_outputs
  verify_seals
  validate_archive_observation
  mv -T "${candidate_dir}" "${final_runtime_dir}" ||
    fail "atomic evidence installation failed; candidate preserved at ${candidate_dir}"
  set_runtime_paths "${final_runtime_dir}"
  rmdir "${lock_dir}"
  trap - EXIT
  verify_complete
}

mode="${1:---run}"
case "${mode}" in
  --preflight)
    preflight
    ;;
  --run)
    run_all
    ;;
  --verify)
    verify_complete
    ;;
  *)
    fail "usage: $0 [--preflight|--run|--verify]"
    ;;
esac
