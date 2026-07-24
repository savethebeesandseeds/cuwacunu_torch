#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_affine_objective_ladder_v1"
capture_authority="verified_legacy_content_manifest_not_original_stage_seal"
graph_order_fingerprint="d334e38b1887ae16"
final_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"

sidecar_schema="synthetic_mdn_frozen_affine_calibration_sidecar_v1"
sidecar_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${sidecar_schema}"
helper_src="${script_dir}/mdn_frozen_affine_objective_ladder.cpp"
calibration_header_rel="wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
digest_header_rel="piaabo/digest/sha256.h"
calibration_header="${sidecar_dir}/frozen_include/${calibration_header_rel}"
digest_header="${sidecar_dir}/frozen_include/${digest_header_rel}"

split_dsl="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.splits.dsl"
canonical_ridge_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_train_eval_ridge_gate.v1.report"

train_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
train_probe="${train_dir}/mdn_edge_context_features.probe"
train_manifest="${train_dir}/job.manifest"
train_report="${train_dir}/channel_inference.report"
historical_dir="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/run/cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001"
historical_probe="${historical_dir}/mdn_edge_context_features.probe"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

oracle_archive="${sidecar_dir}/main/${sidecar_schema}.main.pt"
oracle_metadata="${sidecar_dir}/main/${sidecar_schema}.metadata"
oracle_master="${sidecar_dir}/master.sha256"
oracle_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${sidecar_schema}.report"
oracle_report_and_master="${sidecar_dir}/report_and_master.sha256"

expected_train_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_representation_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_oracle_archive_sha="e83392e50d315e3889b2d1a0f082e2328d7447b72669853a24badd34e810ba88"
expected_oracle_metadata_sha="ca8eab16a79774acd3d490e916fe07f3d572ebcf516a85709cbe6a3b24a3f4ee"
expected_oracle_master_sha="4a7e525c2bb0da0ab4bac94539ec7640781fcba26edd0a631d212ce78795a4aa"
expected_oracle_report_sha="ecd184594931f2687e9e946f364845422c0e84a057b0d225d92d739b5ce345b7"
expected_oracle_report_and_master_sha="0ab0c00594787c2cfac9651c96a9df38158f73fe1d0e73dad9c2f7767cb2a372"
expected_oracle_semantic_digest="b0f3f00760d92d3026ed8675c9d6f572dd1c0b5de6f1c45bbd8b9253f99fd709"
expected_calibration_header_sha="44a366fd3e044f5e5b80eebdf7e461f78bc1c3d4eb03d647dfb2343876ca7d55"
expected_digest_header_sha="3efdf247d7ffa2a3cc89bdf541caef82dee6059ea78c55f32adbde0cdeb110d3"
expected_canonical_ridge_report_sha="cdb51c9d3abde7ddad437699f35929ab6f2edb061eeb0e47630d9d76e671546a"
expected_result_probe_sha="1b43e31eae93a8a88ebe07b7ac9c7ab23b4881bf4b101f60566694286f8b4c7f"
expected_result_keyset_sha="150f10610de149024bf30936b2ca37558d211e2951e9257dc172cff9c7b88bd2"
expected_result_key_count="1109"

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  helper_bin="${build_dir}/mdn_frozen_affine_objective_ladder"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
  main_pair_manifest="${main_dir}/pair.sha256"
  replay_pair_manifest="${replay_dir}/pair.sha256"
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

expect_contains() {
  local path="$1"
  local key="$2"
  local needle="$3"
  local actual
  actual="$(probe_value "${path}" "${key}")"
  [[ "${actual}" == *"${needle}"* ]] ||
    fail "${key} in ${path} does not contain ${needle}"
}

require_source_token() {
  local token="$1"
  grep -Fq -- "${token}" "${helper_src}" ||
    fail "helper is missing required token: ${token}"
}

validate_capture_probe() {
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
expected_record_schema = "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1"
expected_edge_identities = {
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
        if row[0] != expected_record_schema:
            raise SystemExit(f"unexpected record schema at row {row_count}: {row[0]}")
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
        if tuple(row[index] for index in (5, 6, 7)) != expected_edge_identities[edge]:
            raise SystemExit(f"unexpected edge/base/quote identity at anchor {anchor}, edge {edge}")
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

verify_oracle_seal() {
  require_sha256 "${oracle_archive}" "${expected_oracle_archive_sha}"
  require_sha256 "${oracle_metadata}" "${expected_oracle_metadata_sha}"
  require_sha256 "${oracle_master}" "${expected_oracle_master_sha}"
  require_sha256 "${oracle_report}" "${expected_oracle_report_sha}"
  require_sha256 "${oracle_report_and_master}" "${expected_oracle_report_and_master_sha}"
  grep -Fxq "semantic_tensor_digest=64:${expected_oracle_semantic_digest}" \
    "${oracle_metadata}" ||
    fail "oracle metadata semantic digest does not match the declared oracle"
  (
    cd "${repo_root}"
    sha256sum -c "${oracle_report_and_master}" >/dev/null
  )
  (
    cd "${sidecar_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
}

preflight_inputs() {
  require_sha256 "${train_probe}" "${expected_train_sha}"
  require_sha256 "${representation_checkpoint}" "${expected_representation_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_sha}"
  require_file "${train_manifest}"
  require_file "${train_report}"
  require_file "${split_dsl}"
  require_sha256 "${canonical_ridge_report}" "${expected_canonical_ridge_report_sha}"
  [[ ! -e "${train_dir}/stage.sha256" ]] ||
    fail "legacy train capture unexpectedly acquired a stage seal"
  expect_value "${train_manifest}" graph_order_fingerprint "${graph_order_fingerprint}"
  expect_contains "${train_manifest}" source_cursor_token "accepted=1170"
  validate_capture_probe "${train_probe}" 0 730 6570
  validate_split_contract
  expect_value "${canonical_ridge_report}" solver.selection_fit_anchor_range "[0,554)"
  expect_value "${canonical_ridge_report}" solver.selection_purge_anchor_range "[554,584)"
  expect_value "${canonical_ridge_report}" solver.selection_validation_anchor_range "[584,730)"
  expect_value "${canonical_ridge_report}" selection.diagnostic_fallback_alpha "1e-10"
  expect_value "${canonical_ridge_report}" candidate.alpha_1e-10.validation.rmse "0.020836575958"
  expect_value "${canonical_ridge_report}" candidate.alpha_1e-10.validation.directional_accuracy "0.805936073059"
  expect_value "${canonical_ridge_report}" candidate.alpha_1e-10.validation.pairwise_rank_accuracy "0.793759512938"
  verify_oracle_seal
}

preflight_sources() {
  require_file "${helper_src}"
  require_sha256 "${calibration_header}" "${expected_calibration_header_sha}"
  require_sha256 "${digest_header}" "${expected_digest_header_sha}"
  for token in "--input" "--evaluation-input" "--oracle-archive" "--output" "--schema-id" "PerEdgeAffineReturnHead"; do
    require_source_token "${token}"
  done
  bash -n "${BASH_SOURCE[0]}"
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
  cp -- "${helper_src}" "${frozen_source_dir}/mdn_frozen_affine_objective_ladder.cpp"
  cp -- "${BASH_SOURCE[0]}" "${frozen_source_dir}/run_mdn_frozen_affine_objective_ladder.sh"
  copy_frozen_header "${calibration_header}" "${calibration_header_rel}"
  copy_frozen_header "${digest_header}" "${digest_header_rel}"
  cmp -s "${helper_src}" "${frozen_source_dir}/mdn_frozen_affine_objective_ladder.cpp" ||
    fail "frozen helper copy drifted"
  cmp -s "${BASH_SOURCE[0]}" "${frozen_source_dir}/run_mdn_frozen_affine_objective_ladder.sh" ||
    fail "frozen runner copy drifted"
  (
    cd "${runtime_dir}"
    sha256sum frozen_sources/mdn_frozen_affine_objective_ladder.cpp frozen_sources/run_mdn_frozen_affine_objective_ladder.sh > frozen_sources.sha256
    sha256sum "frozen_include/${calibration_header_rel}" "frozen_include/${digest_header_rel}" > public_headers.sha256
  )
}

compile_helper() {
  local libtorch_path="${repo_root}/.external/libtorch-upd"
  local cuda_path="/usr/local/cuda-12.4"
  mkdir -p "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC -I"${frozen_include_root}" -isystem "${libtorch_path}/include" -isystem "${libtorch_path}/include/torch/csrc/api/include" -isystem "${cuda_path}/include" "${frozen_source_dir}/mdn_frozen_affine_objective_ladder.cpp" -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" -Wl,-rpath,"${libtorch_path}/lib" -Wl,-rpath,"${cuda_path}/lib64" -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt -lstdc++ -lpthread -lm -o "${helper_bin}"
}

write_provenance() {
  {
    echo "authority=${capture_authority}"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "train_anchor_range=[0,730)"
    echo "selection_fit_anchor_range=[0,554)"
    echo "selection_purge_anchor_range=[554,584)"
    echo "selection_validation_anchor_range=[584,730)"
    echo "outer_purge_anchor_range=[730,760)"
    echo "historical_confirmation_anchor_range=[760,1088)"
    echo "historical_confirmation_role=previously_consumed_diagnostic_confirmation"
    echo "historical_raw_probe_preflight_content_read=false"
    echo "historical_raw_probe_content_sealed=false"
    echo "historical_helper_evaluation_opened_expected=false"
    echo "historical_cli_path_role=conditional_predeclared_path_not_opened_without_selection"
    echo "historical_used_for_selection=false"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "target=raw_future_base_close_minus_future_quote_close"
    echo "probe_boundary=post_adapter_direct_edge_features"
    echo "runtime_adapter_reapplied=false"
    echo "oracle_schema=${sidecar_schema}"
    echo "oracle_semantic_tensor_digest=${expected_oracle_semantic_digest}"
  } > "${runtime_dir}/capture_provenance.status"

  : > "${runtime_dir}/execution_inputs.sha256"
  while IFS= read -r -d '' path; do
    sha256sum "${path}" >> "${runtime_dir}/execution_inputs.sha256"
  done < <(find "${train_dir}" -type f -print0 | sort -z)
  sha256sum "${representation_checkpoint}" "${mdn_checkpoint}" "${split_dsl}" "${canonical_ridge_report}" "${oracle_archive}" "${oracle_metadata}" "${oracle_master}" "${oracle_report}" "${oracle_report_and_master}" >> "${runtime_dir}/execution_inputs.sha256"
  if grep -Fq "${historical_dir}/" "${runtime_dir}/execution_inputs.sha256"; then
    fail "unused historical raw content leaked into the execution-input seal"
  fi

  local compiler_path
  compiler_path="$(readlink -f "$(command -v g++)")"
  {
    echo "path=${compiler_path}"
    echo "sha256=$(sha256sum "${compiler_path}" | awk '{print $1}')"
    echo "target=$(g++ -dumpmachine)"
  } > "${runtime_dir}/compiler.identity"
  g++ --version > "${runtime_dir}/compiler.version"
  sha256sum "${compiler_path}" > "${runtime_dir}/compiler.sha256"
  nvidia-smi \
    --query-gpu=index,name,uuid,driver_version,compute_cap,memory.total \
    --format=csv,noheader > "${runtime_dir}/gpu.identity"
  ldd "${helper_bin}" > "${runtime_dir}/linked_libraries.txt"
  for required_library in libtorch_cuda.so libc10_cuda.so libcudart.so; do
    grep -Fq "${required_library}" "${runtime_dir}/linked_libraries.txt" ||
      fail "compiled helper is not linked to ${required_library}"
  done
  readelf -d "${helper_bin}" > "${runtime_dir}/binary.dynamic.txt"
  : > "${runtime_dir}/resolved_libraries.sha256"
  while IFS= read -r library; do
    [[ -n "${library}" && -f "${library}" ]] || continue
    sha256sum "${library}" >> "${runtime_dir}/resolved_libraries.sha256"
  done < <(awk '/=> \// {print $3} /^[[:space:]]*\// {print $1}' "${runtime_dir}/linked_libraries.txt" | sort -u)
  (
    cd "${runtime_dir}"
    sha256sum bin/mdn_frozen_affine_objective_ladder > binary.sha256
  )

  {
    echo "schema_id=${schema_id}"
    echo "helper_source_sha256=$(sha256sum "${frozen_source_dir}/mdn_frozen_affine_objective_ladder.cpp" | awk '{print $1}')"
    echo "runner_sha256=$(sha256sum "${frozen_source_dir}/run_mdn_frozen_affine_objective_ladder.sh" | awk '{print $1}')"
    echo "helper_binary_sha256=$(sha256sum "${helper_bin}" | awk '{print $1}')"
    echo "train_probe_sha256=${expected_train_sha}"
    echo "historical_raw_probe_preflight_content_read=false"
    echo "historical_raw_probe_content_sealed=false"
    echo "historical_helper_evaluation_opened_expected=false"
    echo "historical_cli_path_role=conditional_predeclared_path_not_opened_without_selection"
    echo "oracle_archive_sha256=${expected_oracle_archive_sha}"
    echo "oracle_semantic_tensor_digest=${expected_oracle_semantic_digest}"
    echo "selection_fit_anchor_range=[0,554)"
    echo "selection_purge_anchor_range=[554,584)"
    echo "selection_validation_anchor_range=[584,730)"
    echo "refit_anchor_range=[0,730)"
    echo "historical_confirmation_anchor_range=[760,1088)"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "seed=31"
    echo "affine_batch_schedule_seed=2031"
    echo "selection_schedule_epochs_started=389"
    echo "selection_schedule_batch_size_histogram=42:388,64:3112"
    echo "selection_schedule_sampled_anchor_count=215464"
    echo "selection_schedule_visit_histogram=388:42,389:512"
    echo "refit_minibatch_schedule_epochs_started=292"
    echo "refit_minibatch_schedule_batch_size_histogram=26:291,64:3209"
    echo "refit_minibatch_schedule_sampled_anchor_count=212942"
    echo "refit_minibatch_schedule_visit_histogram=291:218,292:512"
    echo "adam_steps=3500"
    echo "adam_learning_rate=0.001"
    echo "adam_batch_size=64"
    echo "grad_clip_norm=5.0"
    echo "cublas_workspace_config=:4096:8"
    echo "cuda_device_order=PCI_BUS_ID"
    echo "arm_order=L0_raw_mse,L1_scaled_huber,L2_plus_direction,L3_plus_rank"
    echo "arm.L0_raw_mse=mean_square_error_target_scale_1_weight_1_direction_0_rank_0"
    echo "arm.L1_scaled_huber=smooth_l1_target_scale_36_beta_0.5_weight_100_direction_0_rank_0"
    echo "arm.L2_plus_direction=L1_plus_direction_weight_5_logit_scale_1"
    echo "arm.L3_plus_rank=L2_plus_pairwise_rank_weight_5_logit_scale_1"
    echo "recovery_order=B1_no_clip_if_applicable,B2_full_batch_adam,B3_whitened_minibatch_adam,B4_full_batch_lbfgs"
    echo "pca_scope=selection_fit_only_per_edge_after_shared_standardization"
    echo "pca_retention_threshold=max_1e-12_or_1e-10_times_lambda_max"
    echo "pca_cuda_float32_covariance_error_max=0.05"
    echo "pca_mapped_prediction_max_abs_delta=0.00002"
    echo "lbfgs_line_search=strong_wolfe"
    echo "lbfgs_max_iterations=500"
    echo "lbfgs_max_evaluations=625"
    echo "lbfgs_history_size=100"
    echo "lbfgs_tolerance_grad=1e-12"
    echo "lbfgs_tolerance_change=1e-15"
    echo "selection_oracle_scope=analytic_ridge_fit_0_554_validation_584_730"
    echo "selection_statistics_scope=selection_fit_0_554_only"
    echo "refit_sidecar_scope=refit_0_730_valid_from_730"
    echo "historical_used_for_selection=false"
    echo "gate_policy=one_sided_noninferiority"
    echo "validation_direction_shortfall_max=0.01"
    echo "validation_rank_shortfall_max=0.01"
    echo "validation_rmse_excess_max=0.0005"
  } > "${runtime_dir}/execution_contract.status"
}

run_probe() {
  local output="$1"
  local log="$2"
  CUDA_DEVICE_ORDER=PCI_BUS_ID CUBLAS_WORKSPACE_CONFIG=:4096:8 "${helper_bin}" \
    --input "${train_probe}" \
    --evaluation-input "${historical_probe}" \
    --oracle-archive "${oracle_archive}" \
    --output "${output}" \
    --schema-id "${schema_id}" > "${log}" 2>&1
}

validate_result_probe() {
  local path="$1"
  require_file "${path}"
  if grep -nEv '^$|^[A-Za-z0-9_.]+=' "${path}"; then
    fail "malformed objective-ladder result line"
  fi
  if cut -d= -f1 "${path}" | grep -v '^$' | sort | uniq -d | grep .; then
    fail "duplicate objective-ladder result key"
  fi
  if grep -Eq '(^|=)/|=[A-Za-z]:[\\/]' "${path}"; then
    fail "objective-ladder result leaks a physical path"
  fi
  expect_value "${path}" schema_id "${schema_id}"
  require_sha256 "${path}" "${expected_result_probe_sha}"
  python3 - "${path}" "${expected_result_key_count}" \
    "${expected_result_keyset_sha}" <<'PY'
import hashlib
import math
import re
import sys

path, expected_count_text, expected_keyset_sha = sys.argv[1:]
raw = open(path, "rb").read()
if not raw.endswith(b"\n"):
    raise SystemExit("objective-ladder probe lacks its final newline")
text = raw.decode("utf-8")
values = {}
for number, line in enumerate(text.splitlines(), 1):
    if not re.fullmatch(r"[A-Za-z0-9_.]+=[^\x00-\x1f\x7f]*", line):
        raise SystemExit(f"malformed objective-ladder line {number}")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit(f"duplicate objective-ladder key: {key}")
    if "/" in value or "\\" in value:
        raise SystemExit(f"physical path-like value leaked through {key}")
    values[key] = value

expected_count = int(expected_count_text)
if len(values) != expected_count:
    raise SystemExit(
        f"key count drift: expected {expected_count}, got {len(values)}"
    )
keyset = "".join(f"{key}\n" for key in sorted(values)).encode()
actual_keyset_sha = hashlib.sha256(keyset).hexdigest()
if actual_keyset_sha != expected_keyset_sha:
    raise SystemExit(
        f"key-set SHA drift: expected {expected_keyset_sha}, "
        f"got {actual_keyset_sha}"
    )

def exact(key, expected):
    actual = values.get(key)
    if actual != expected:
        raise SystemExit(f"{key}: expected {expected!r}, got {actual!r}")

def integer(key):
    value = values.get(key)
    if value is None or re.fullmatch(r"-?[0-9]+", value) is None:
        raise SystemExit(f"{key}: not an integer: {value!r}")
    return int(value)

def number(key):
    try:
        result = float(values[key])
    except (KeyError, ValueError):
        raise SystemExit(f"{key}: missing or non-numeric")
    if not math.isfinite(result):
        raise SystemExit(f"{key}: non-finite")
    return result

def boolean(key):
    value = values.get(key)
    if value not in {"true", "false"}:
        raise SystemExit(f"{key}: not a boolean: {value!r}")
    return value == "true"

def digest(key, expected=None):
    value = values.get(key)
    if value is None or re.fullmatch(r"[0-9a-f]{64}", value) is None:
        raise SystemExit(f"{key}: not a lowercase SHA-256")
    if expected is not None and value != expected:
        raise SystemExit(f"{key}: expected {expected}, got {value}")
    return value

def metric(prefix, count, pairs):
    if integer(prefix + ".count") != count:
        raise SystemExit(f"{prefix}: count drift")
    if integer(prefix + ".pair_count") != pairs:
        raise SystemExit(f"{prefix}: pair-count drift")
    result = (
        number(prefix + ".rmse"),
        number(prefix + ".directional_accuracy"),
        number(prefix + ".pairwise_rank_accuracy"),
    )
    mae = number(prefix + ".mae")
    correlation = number(prefix + ".correlation")
    if result[0] < 0 or mae < 0:
        raise SystemExit(f"{prefix}: negative error")
    if not 0 <= result[1] <= 1 or not 0 <= result[2] <= 1:
        raise SystemExit(f"{prefix}: accuracy outside [0,1]")
    if not -1.0000001 <= correlation <= 1.0000001:
        raise SystemExit(f"{prefix}: correlation outside [-1,1]")
    return result

def comparison(prefix):
    equal = boolean(prefix + ".torch_equal")
    maximum = number(prefix + ".maximum_abs_delta")
    mean = number(prefix + ".mean_abs_delta")
    if maximum < 0 or mean < 0 or mean > maximum + 1e-12:
        raise SystemExit(f"{prefix}: invalid comparison")
    return equal, maximum, mean

fixed = {
    "status": "complete",
    "runtime.torch_version": "2.6.0",
    "runtime.cuda_compile_version": "12040",
    "runtime.cuda_runtime_version": "12040",
    "runtime.cuda_driver_version": "12080",
    "runtime.cuda_device_count": "1",
    "runtime.cuda_device_index": "0",
    "runtime.cuda_device_name": "NVIDIA RTX A2000 8GB Laptop GPU",
    "runtime.cuda_compute_capability": "8.6",
    "runtime.cuda_total_global_memory_bytes": "8589410304",
    "runtime.cpu_thread_count": "1",
    "runtime.cpu_interop_thread_count": "1",
    "runtime.deterministic_algorithms": "true",
    "runtime.deterministic_warn_only": "false",
    "runtime.deterministic_cudnn": "true",
    "runtime.cudnn_benchmark": "false",
    "runtime.allow_tf32_cublas": "false",
    "runtime.allow_tf32_cudnn": "false",
    "runtime.cublas_workspace_config": ":4096:8",
    "runtime.cuda_device_order": "PCI_BUS_ID",
    "runtime.seed": "31",
    "input.development.sha256":
        "ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851",
    "input.oracle_archive.sha256":
        "e83392e50d315e3889b2d1a0f082e2328d7447b72669853a24badd34e810ba88",
    "input.historical.opened": "false",
    "data.record_schema":
        "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1",
    "data.development.row_count": "6570",
    "data.development.anchor_count": "730",
    "data.feature_dim": "128",
    "data.dynamic_feature_count": "384",
    "data.source_feature_count": "400",
    "data.edge_count": "3",
    "data.channel_count": "3",
    "data.quote_node_id": "SYNUSD",
    "data.edge.0.edge_id": "SYNALPHASYNUSD",
    "data.edge.0.base_node_id": "SYNALPHA",
    "data.edge.1.edge_id": "SYNBETASYNUSD",
    "data.edge.1.base_node_id": "SYNBETA",
    "data.edge.2.edge_id": "SYNGAMMASYNUSD",
    "data.edge.2.base_node_id": "SYNGAMMA",
    "data.cached_prefix_torch_equal": "true",
    "data.cached_prefix_max_abs_delta": "0.00000000000000000e+00",
    "split.selection_fit": "[0,554)",
    "split.selection_purge": "[554,584)",
    "split.selection_validation": "[584,730)",
    "split.refit": "[0,730)",
    "split.outer_purge": "[730,760)",
    "split.historical_confirmation": "[760,1088)",
    "split.unconsumed_holdout": "[1088,1170)",
    "data.unconsumed_holdout_opened": "false",
    "data.maximum_anchor_loaded": "729",
    "execution.selection_zero_state_digest":
        "5be04c52af548a0bd6703fb35139b7f5d52c8fe50d1843078e852fccdc0dd8c7",
    "execution.adam.steps": "3500",
    "execution.adam.minibatch_size": "64",
    "execution.schedule.kind": "production_random_sampler_epoch_reseeded",
    "execution.schedule.seed": "2031",
    "execution.gate.policy": "one_sided_noninferiority",
    "execution.gate.direction_shortfall_max": "0.01",
    "execution.gate.rank_shortfall_max": "0.01",
    "execution.gate.rmse_excess_max": "0.0005",
    "execution.pca.mapped_prediction_max_abs_delta": "0.00002",
    "execution.pca.mapped_prediction_policy":
        "selection_deployability_gate_nonfatal_to_harness",
}
for key, expected in fixed.items():
    exact(key, expected)

oracle_fixed = {
    "oracle.kind": "analytic_ridge_alpha_1e-10",
    "oracle.feature_surface":
        "context_float32_promoted_to_float64_then_base_quote_difference_recomputed",
    "oracle.thread_policy": "deterministic_at_num_threads_1",
    "oracle.gate_reference": "pinned_one_thread_observed_oracle",
    "oracle.fit_scope": "[0,554)",
    "oracle.validation_scope": "[584,730)",
    "oracle.pinned_one_thread_fixture_match": "true",
    "oracle.legacy_reference_exactly_reproduced": "false",
    "oracle.legacy_reference.rmse_drift_pass": "true",
}
for key, expected in oracle_fixed.items():
    exact(key, expected)
oracle_fit = metric("oracle.fit", 4986, 4986)
oracle_validation = metric("oracle.validation", 1314, 1314)
if abs(oracle_validation[0] - 0.020836577214) > 2e-12:
    raise SystemExit("one-thread oracle RMSE drift")
if abs(oracle_validation[1] - 1059 / 1314) > 2e-12:
    raise SystemExit("one-thread oracle direction drift")
if abs(oracle_validation[2] - 1043 / 1314) > 2e-12:
    raise SystemExit("one-thread oracle rank drift")
legacy_rmse = number("oracle.legacy_reference.validation.rmse")
signed_drift = number("oracle.legacy_reference.rmse_drift")
absolute_drift = number("oracle.legacy_reference.rmse_abs_drift")
drift_limit = number("oracle.legacy_reference.rmse_abs_drift_max")
if abs(legacy_rmse - 0.020836575958) > 2e-15:
    raise SystemExit("legacy oracle reference drift")
if abs(signed_drift - (oracle_validation[0] - legacy_rmse)) > 2e-15:
    raise SystemExit("legacy oracle signed drift is inconsistent")
if abs(absolute_drift - abs(signed_drift)) > 2e-15:
    raise SystemExit("legacy oracle absolute drift is inconsistent")
if absolute_drift > drift_limit or drift_limit > 2.0000001e-9:
    raise SystemExit("legacy oracle drift gate failed")
if number("oracle.maximum_normalized_residual") > 1e-7:
    raise SystemExit("oracle solve residual exceeds 1e-7")
digest(
    "oracle.state_digest",
    "79c13695d2ed379ce56b96c75a24c8a490b176d4700a02c3dbfe23ce495e92ab",
)
digest(
    "oracle.validation_prediction_digest",
    "230e89fa2f1af20030d1ec5b9b020a22b54cf681c6ff734023ed16918ba1a664",
)

p0 = "control.P0_sidecar_copy"
for key, expected in {
    p0 + ".applicable": "true",
    p0 + ".selection_eligible": "false",
    p0 + ".role": "development_representability_not_selection",
    p0 + ".parameters_frozen": "true",
    p0 + ".state_copy_exact": "true",
    p0 + ".copy_parameters_frozen": "true",
    p0 + ".baseline_device": "cpu",
    p0 + ".copy_device": "cuda_0",
    p0 + ".cross_dtype_device_delta_policy":
        "descriptive_no_numeric_acceptance_gate",
}.items():
    exact(key, expected)
exact(
    p0 + ".semantic_tensor_digest",
    "b0f3f00760d92d3026ed8675c9d6f572dd1c0b5de6f1c45bbd8b9253f99fd709",
)
p0_state = digest(
    p0 + ".state_digest",
    "26637fdbd464dd8eb8ca18b3fbc589e99e057b7c635f20e06ec8a5124c7fcc6b",
)
if digest(p0 + ".copy_state_digest") != p0_state:
    raise SystemExit("P0 state-copy digest mismatch")
digest(
    p0 + ".development_prediction_digest",
    "48ba6b65225e32ec300d97a9484998ee5034b917ac6226a3725a57812ff2ee7a",
)
if comparison(p0 + ".development_context_to_readout") != (True, 0.0, 0.0):
    raise SystemExit("P0 CPU context/readout parity failed")
if comparison(p0 + ".development_sidecar_to_copy") != comparison(
    p0 + ".development_readout_sidecar_to_copy"
):
    raise SystemExit("P0 descriptive CUDA-copy comparisons disagree")
metric(p0 + ".development", 6570, 6570)
metric(p0 + ".copy_development", 6570, 6570)

p1 = "control.P1_selection_oracle_cuda_copy"
for key, expected in {
    p1 + ".applicable": "true",
    p1 + ".selection_eligible": "false",
    p1 + ".role": "selection_surface_representability_control",
    p1 + ".fit_scope": "[0,554)",
    p1 + ".validation_scope": "[584,730)",
    p1 + ".historical_evaluated": "false",
    p1 + ".candidate_normalization": "cached_float32_selection_fit_only",
    p1 + ".cross_surface_delta_policy": "descriptive_nonfatal",
    p1 + ".state_copy_exact": "true",
    p1 + ".copy_parameters_frozen": "true",
    p1 + ".validation_parity_pass": "true",
}.items():
    exact(key, expected)
digest(
    p1 + ".state_digest",
    "f50c13a46aa9b1ed4372a35ca8ae7e55e4c6d2863dbf5c5a0bc38c9491d454a7",
)
digest(
    p1 + ".fit_prediction_digest",
    "8d1b92063fc3c472784264c4669c4ce333e90dc92caed9fb222fc327049ebbe5",
)
digest(
    p1 + ".validation_prediction_digest",
    "27f69559cf3327470b8f0b9d900cdb084cedc33407b756f58697df9d06060546",
)
comparison(p1 + ".fit_oracle_to_copy")
comparison(p1 + ".validation_oracle_to_copy")
metric(p1 + ".fit", 4986, 4986)
metric(p1 + ".validation", 1314, 1314)
for suffix in ("direction_shortfall", "rank_shortfall", "rmse_excess"):
    if number(p1 + ".gate." + suffix) != 0:
        raise SystemExit(f"P1 {suffix} is nonzero despite parity pass")

schedule = {
    "schedule.selection.kind": "production_random_sampler_epoch_reseeded",
    "schedule.selection.seed": "2031",
    "schedule.selection.batch_count": "3500",
    "schedule.selection.epoch_count": "389",
    "schedule.selection.sample_count": "215464",
    "schedule.selection.digest":
        "3833230b7b2cbd2772e4590ab7b0f7f3cdf6675a76bf1a76e15ff58881284dc8",
    "schedule.selection.visit_min": "388",
    "schedule.selection.visit_max": "389",
    "schedule.selection.batch_size_histogram": "42:388,64:3112",
    "schedule.selection.visit_histogram": "388:42,389:512",
}
for key, expected in schedule.items():
    exact(key, expected)
for name in ("0", "1", "last"):
    batch = [int(value) for value in values[
        "schedule.selection.batch." + name
    ].split(",")]
    if len(batch) != 64 or len(set(batch)) != 64:
        raise SystemExit(f"selection batch {name} is not 64 unique anchors")
    if min(batch) < 0 or max(batch) >= 554:
        raise SystemExit(f"selection batch {name} is out of range")

expected_ranks = {
    ("raw", 0): 26, ("raw", 1): 26, ("raw", 2): 26,
    ("shared_standardized", 0): 121,
    ("shared_standardized", 1): 119,
    ("shared_standardized", 2): 120,
}
for (surface, edge), expected_rank in expected_ranks.items():
    prefix = f"conditioning.{surface}.edge.{edge}"
    if integer(prefix + ".observations") != 1662:
        raise SystemExit(f"{prefix}: observation-count drift")
    if integer(prefix + ".dimension") != 384:
        raise SystemExit(f"{prefix}: dimension drift")
    rank = integer(prefix + ".retained_rank")
    if rank != expected_rank or integer(prefix + ".nullity") != 384 - rank:
        raise SystemExit(f"{prefix}: rank/nullity drift")
    eigen = [
        number(prefix + ".lambda_min"),
        number(prefix + ".lambda_q25"),
        number(prefix + ".lambda_median"),
        number(prefix + ".lambda_q75"),
        number(prefix + ".lambda_max"),
    ]
    if any(a > b + 1e-12 for a, b in zip(eigen, eigen[1:])):
        raise SystemExit(f"{prefix}: eigenvalue quantiles are unordered")
    if not boolean(prefix + ".psd_pass"):
        raise SystemExit(f"{prefix}: PSD diagnostic failed")

arm_config = {
    "arm.L0_raw_mse": ("true", "raw_mse", "adam", "false", "false", {0,10,100,500,1000,2000,3500}),
    "arm.L1_scaled_huber": ("false", "100_times_huber_36x_beta_0.5", "adam", "false", "false", {0,10,100,500,1000,2000,3500}),
    "arm.L2_plus_direction": ("false", "100_times_huber_36x_beta_0.5_plus_5_direction", "adam", "false", "false", {0,10,100,500,1000,2000,3500}),
    "arm.L3_plus_rank": ("false", "100_times_huber_36x_beta_0.5_plus_5_direction_plus_5_rank", "adam", "false", "false", {0,10,100,500,1000,2000,3500}),
    "recovery.B2_full_batch_adam": ("true", "raw_mse", "adam", "true", "false", {0,10,100,500,1000,2000,3500}),
    "recovery.B3_whitened_minibatch_adam": ("true", "raw_mse", "adam", "false", "true", {0,10,100,500,1000,2000,3500}),
    "recovery.B4_full_batch_lbfgs": ("true", "raw_mse", "lbfgs_strong_wolfe", "true", "false", {0,1}),
}
telemetry_fields = {
    "fit_total", "fit_raw_mse", "fit_scaled_huber",
    "fit_direction_loss", "fit_rank_loss", "validation_total",
    "validation_rmse", "validation_direction", "validation_rank",
    "parameter_l2_norm",
}
arm_validation = {}
for prefix, config in arm_config.items():
    eligible, objective, optimizer, full_batch, pca, expected_steps = config
    exact(prefix + ".applicable", "true")
    exact(prefix + ".selection_eligible", eligible)
    exact(prefix + ".objective", objective)
    exact(prefix + ".optimizer", optimizer)
    exact(prefix + ".full_batch", full_batch)
    exact(prefix + ".pca_whitened", pca)
    if digest(prefix + ".zero_state_digest") != values[
        "execution.selection_zero_state_digest"
    ]:
        raise SystemExit(f"{prefix}: zero-state digest drift")
    digest(prefix + ".final_state_digest")
    digest(prefix + ".validation_prediction_digest")
    metric(prefix + ".fit", 4986, 4986)
    validation = metric(prefix + ".validation", 1314, 1314)
    arm_validation[prefix] = validation
    shortfalls = (
        max(oracle_validation[1] - validation[1], 0.0),
        max(oracle_validation[2] - validation[2], 0.0),
        max(validation[0] - oracle_validation[0], 0.0),
    )
    for suffix, expected in zip(
        ("direction_shortfall", "rank_shortfall", "rmse_excess"),
        shortfalls,
    ):
        if abs(number(prefix + ".gate." + suffix) - expected) > 1e-14:
            raise SystemExit(f"{prefix}: inconsistent {suffix}")
    pca_pass = boolean(prefix + ".pca_validation_pass")
    mapped_pass = boolean(prefix + ".pca_mapped_prediction_pass")
    expected_pass = (
        shortfalls[0] <= 0.01 and shortfalls[1] <= 0.01
        and shortfalls[2] <= 0.0005 and pca_pass and mapped_pass
    )
    if boolean(prefix + ".validation_parity_pass") != expected_pass:
        raise SystemExit(f"{prefix}: inconsistent gate decision")
    seen = set()
    marker = prefix + ".telemetry."
    for key in values:
        if key.startswith(marker):
            step_text, field = key[len(marker):].split(".", 1)
            if field not in telemetry_fields:
                raise SystemExit(f"{prefix}: unexpected telemetry field")
            seen.add(int(step_text))
            number(key)
    if seen != expected_steps:
        raise SystemExit(f"{prefix}: telemetry-step drift")
    for step in seen:
        for field in telemetry_fields:
            number(f"{marker}{step}.{field}")

b3 = "recovery.B3_whitened_minibatch_adam"
exact(b3 + ".pca_validation_pass", "false")
exact(b3 + ".pca_mapped_prediction_pass", "false")
if number(b3 + ".pca_mapped_prediction_max_abs_delta") <= 0.00002:
    raise SystemExit("B3 mapping gate should have failed at the frozen threshold")
for edge, rank in enumerate((121, 119, 120)):
    prefix = f"{b3}.pca.edge.{edge}"
    if integer(prefix + ".rank") != rank:
        raise SystemExit(f"{prefix}: PCA rank drift")
    if number(prefix + ".cuda_float32_diagonal_max_abs_error") > 0.05:
        raise SystemExit(f"{prefix}: diagonal whitening error")
    if number(prefix + ".cuda_float32_off_diagonal_max_abs") > 0.05:
        raise SystemExit(f"{prefix}: off-diagonal whitening error")

for delta_prefix, before, after in (
    ("objective_delta.L0_to_L1.validation", "arm.L0_raw_mse", "arm.L1_scaled_huber"),
    ("objective_delta.L1_to_L2.validation", "arm.L1_scaled_huber", "arm.L2_plus_direction"),
    ("objective_delta.L2_to_L3.validation", "arm.L2_plus_direction", "arm.L3_plus_rank"),
):
    expected_delta = [
        arm_validation[after][index] - arm_validation[before][index]
        for index in range(3)
    ]
    for suffix, expected in zip(
        ("rmse", "directional_accuracy", "pairwise_rank_accuracy"),
        expected_delta,
    ):
        if abs(number(delta_prefix + "." + suffix) - expected) > 1e-14:
            raise SystemExit(f"{delta_prefix}: inconsistent delta")

exact("recovery.activated", "true")
exact("recovery.trigger", "l0_validation_parity_failure")
b1 = "recovery.B1_no_clip_if_applicable"
b1_keys = {key for key in values if key.startswith(b1 + ".")}
if b1_keys != {
    b1 + ".applicable", b1 + ".selection_eligible", b1 + ".reason"
}:
    raise SystemExit("B1 skipped-arm key surface drift")
exact(b1 + ".applicable", "false")
exact(b1 + ".selection_eligible", "true")
exact(b1 + ".reason", "l0_gradient_clip_count_zero")
if integer("arm.L0_raw_mse.clip_count") != 0:
    raise SystemExit("B1 skip contradicts L0 clip count")

eligible = [
    "arm.L0_raw_mse",
    "recovery.B2_full_batch_adam",
    "recovery.B3_whitened_minibatch_adam",
    "recovery.B4_full_batch_lbfgs",
]
passing = [
    prefix for prefix in eligible
    if boolean(prefix + ".validation_parity_pass")
]
if integer("selection.passing_candidate_count") != len(passing):
    raise SystemExit("selection passing count is inconsistent")
if boolean("selection.found") != bool(passing):
    raise SystemExit("selection.found is inconsistent")
expected_selected = passing[0].split(".", 1)[1] if passing else "none"
exact("selection.selected", expected_selected)
if passing:
    raise SystemExit("frozen no-selection branch unexpectedly changed")
exact("selection.passing_candidate_count", "0")
exact("selection.found", "false")
exact("refit.applicable", "false")
exact("refit.reason", "no_passing_validation_candidate")
exact("historical.evaluated", "false")
if "input.historical.sha256" in values:
    raise SystemExit("historical SHA emitted while unopened")
if any(key.startswith("schedule.refit.") for key in values):
    raise SystemExit("refit schedule emitted without selection")
if any(key.startswith("refit.selected.") for key in values):
    raise SystemExit("refit arm emitted without selection")
if {key for key in values if key.startswith("historical.")} != {
    "historical.evaluated"
}:
    raise SystemExit("historical key surface opened without selection")
if boolean("input.historical.opened") or boolean("historical.evaluated"):
    raise SystemExit("historical data opened without selection")
if integer("data.maximum_anchor_loaded") != 729:
    raise SystemExit("no-selection branch loaded beyond development")

print(
    "validated objective ladder: 1109 pinned keys; "
    "selection=none; historical=false; max_anchor=729"
)
PY
}

verify_replay_contract() {
  validate_result_probe "${main_probe}"
  validate_result_probe "${replay_probe}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay objective-ladder probes are not byte-identical"
  cmp -s "${main_log}" "${replay_log}" ||
    fail "main/replay objective-ladder stdout logs are not byte-identical"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful objective-ladder runs must not emit stdout or stderr"
}

seal_outputs() {
  (
    cd "${runtime_dir}"
    local shared=(
      bin/mdn_frozen_affine_objective_ladder
      frozen_sources.sha256
      public_headers.sha256
      binary.sha256
      compiler.identity
      compiler.version
      compiler.sha256
      gpu.identity
      linked_libraries.txt
      binary.dynamic.txt
      resolved_libraries.sha256
      capture_provenance.status
      execution_inputs.sha256
      execution_contract.status
    )
    sha256sum "${shared[@]}" "main/${schema_id}.probe" "main/${schema_id}.stdout.log" > main/pair.sha256
    sha256sum "${shared[@]}" "replay/${schema_id}.probe" "replay/${schema_id}.stdout.log" > replay/pair.sha256
    sha256sum main/pair.sha256 replay/pair.sha256 "main/${schema_id}.probe" "main/${schema_id}.stdout.log" "replay/${schema_id}.probe" "replay/${schema_id}.stdout.log" > outputs.sha256
    sha256sum frozen_sources/mdn_frozen_affine_objective_ladder.cpp frozen_sources/run_mdn_frozen_affine_objective_ladder.sh "frozen_include/${calibration_header_rel}" "frozen_include/${digest_header_rel}" bin/mdn_frozen_affine_objective_ladder frozen_sources.sha256 public_headers.sha256 binary.sha256 compiler.sha256 execution_inputs.sha256 outputs.sha256 main/pair.sha256 replay/pair.sha256 > frozen_evidence.sha256
    sha256sum frozen_evidence.sha256 capture_provenance.status execution_inputs.sha256 execution_contract.status compiler.identity compiler.version compiler.sha256 gpu.identity frozen_sources.sha256 public_headers.sha256 binary.sha256 linked_libraries.txt binary.dynamic.txt resolved_libraries.sha256 outputs.sha256 main/pair.sha256 replay/pair.sha256 "main/${schema_id}.probe" "main/${schema_id}.stdout.log" "replay/${schema_id}.probe" "replay/${schema_id}.stdout.log" > master.sha256
  )
}

verify_seals() {
  (
    cd "${runtime_dir}"
    sha256sum -c execution_inputs.sha256 >/dev/null
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
  set_runtime_paths "${final_runtime_dir}"
  require_file "${runtime_dir}/master.sha256"
  preflight_inputs
  verify_seals
  verify_replay_contract
  echo "${runtime_dir}"
}

run_all() {
  set_runtime_paths "${final_runtime_dir}"
  if [[ -f "${final_runtime_dir}/master.sha256" ]]; then
    verify_complete
    return
  fi
  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "refusing to overwrite partial evidence directory: ${final_runtime_dir}"
  preflight

  local lock_dir="${final_runtime_dir}.lock"
  local candidate_dir="${final_runtime_dir}.candidate.$$"
  mkdir "${lock_dir}" ||
    fail "another objective-ladder installation holds ${lock_dir}"
  trap 'rmdir "'"${lock_dir}"'" 2>/dev/null || true' EXIT
  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "final evidence appeared after lock acquisition"
  [[ ! -e "${candidate_dir}" && ! -L "${candidate_dir}" ]] ||
    fail "candidate evidence directory already exists"

  set_runtime_paths "${candidate_dir}"
  mkdir -p "${main_dir}" "${replay_dir}"
  freeze_sources
  compile_helper
  write_provenance
  run_probe "${main_probe}" "${main_log}"
  run_probe "${replay_probe}" "${replay_log}"
  verify_replay_contract
  seal_outputs
  verify_seals
  mv -T -n "${candidate_dir}" "${final_runtime_dir}" ||
    fail "evidence installation failed; candidate preserved"
  [[ ! -e "${candidate_dir}" && ! -L "${candidate_dir}" &&
     -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "no-clobber evidence installation postcondition failed"
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
