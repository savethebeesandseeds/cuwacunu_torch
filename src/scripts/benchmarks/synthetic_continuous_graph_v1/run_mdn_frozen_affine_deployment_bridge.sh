#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_affine_deployment_bridge_v1"
capture_authority="verified_legacy_content_manifest_not_original_stage_seal"
graph_order_fingerprint="d334e38b1887ae16"
final_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"

helper_name="mdn_frozen_affine_deployment_bridge.cpp"
runner_name="run_mdn_frozen_affine_deployment_bridge.sh"
helper_src="${script_dir}/${helper_name}"

conditioning_schema="synthetic_mdn_frozen_affine_conditioning_parity_v1"
conditioning_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${conditioning_schema}"
conditioning_probe="${conditioning_dir}/main/${conditioning_schema}.probe"
conditioning_master="${conditioning_dir}/master.sha256"
conditioning_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${conditioning_schema}.report"
conditioning_report_and_master="${conditioning_dir}/report_and_master.sha256"

objective_helper_name="mdn_frozen_affine_objective_ladder.cpp"
objective_helper_src="${conditioning_dir}/frozen_sources/${objective_helper_name}"

deployment_header_rel="wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"
calibration_header_rel="wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
digest_header_rel="piaabo/digest/sha256.h"
deployment_header="${repo_root}/src/include/${deployment_header_rel}"
calibration_header="${conditioning_dir}/frozen_include/${calibration_header_rel}"
digest_header="${conditioning_dir}/frozen_include/${digest_header_rel}"

split_dsl="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.splits.dsl"
train_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
train_probe="${train_dir}/mdn_edge_context_features.probe"
train_manifest="${train_dir}/job.manifest"
train_report="${train_dir}/channel_inference.report"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

expected_train_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_representation_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_split_sha="cd189941903d18c7ddaa6a52cec8c34e384ac6de135cd93673a0569bc55cb68e"

expected_conditioning_probe_sha="1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1"
expected_conditioning_master_sha="414242616aa71e74aaa5812a05d620073099b26cb7c014a0bc3b75eeb480329e"
expected_conditioning_report_sha="d7834a27c07bade9546a4f4000a9fcfd8a3df53f4cc6b933c9d089730bd1687e"
expected_conditioning_report_and_master_sha="0b209c43ac1cc34ff618c71e7b60623288d6669a2bd9c10cb16074a1ef779d9b"
expected_objective_helper_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"
expected_calibration_header_sha="44a366fd3e044f5e5b80eebdf7e461f78bc1c3d4eb03d647dfb2343876ca7d55"
expected_digest_header_sha="3efdf247d7ffa2a3cc89bdf541caef82dee6059ea78c55f32adbde0cdeb110d3"

expected_conditioning_state_digest="e977bbd91f879ab03b36394369defdb7d5acad27a00503e903abf197aadbfde5"
expected_conditioning_prediction_digest="13213b1298134a0c9adfa419106e6b3be99f62fa27f02354f14a79f435c33392"
expected_conditioning_objective="2.70530270559889488e-04"
expected_trained_arm_prediction_delta="1.86264514923095703e-08"

# Source pins are patched only after the helper and v2 public header are final.
expected_helper_source_sha="03d264055115b0973475a9e4a1974f88dd9ed83a4a08c67e17641374344e2c71"
expected_deployment_header_sha="69d633d604318fba16813f48bd539465be7e58c13e5d517cfc32b04f8c1599de"

# Canonical execution stays disabled until two fresh scratch runs establish
# the exact probe contract and a stable semantic artifact digest. The archive
# file pin may later be a SHA-256 or NONDETERMINISTIC; semantic identity is
# always mandatory.
expected_result_probe_sha="00a382d060d20d7a8277d9f120fdc6578c572072264847157424c8a09207d3fb"
expected_result_keyset_sha="275b86d4ece31ec00bbb9591724d439335d9de5f9a72a99fe84d0fd1abc82f6b"
expected_result_key_count="504"
expected_artifact_semantic_digest="84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38"
expected_artifact_file_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  helper_bin="${build_dir}/mdn_frozen_affine_deployment_bridge"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_artifact="${main_dir}/${schema_id}.pt"
  replay_artifact="${replay_dir}/${schema_id}.pt"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
  artifact_observation="${runtime_dir}/artifact_identity.status"
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
  [[ "${expected}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "unresolved SHA-256 pin for ${path}: ${expected}"
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

reject_source_token() {
  local token="$1"
  if grep -Fq -- "${token}" "${helper_src}"; then
    fail "helper contains forbidden source-boundary token: ${token}"
  fi
}

validate_capture_probe() {
  local path="$1"
  python3 - "${path}" <<'PY'
import csv
import math
import sys
from collections import Counter

path = sys.argv[1]
begin, end, expected_rows = 0, 730, 6570
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
            raise SystemExit(f"unexpected record schema at row {row_count}")
        anchor = int(row[2])
        edge = int(row[4])
        channel = int(row[8])
        target = float(row[9])
        feature_count = int(row[10])
        features = row[11].split(";") if row[11] else []
        if not begin <= anchor < end:
            raise SystemExit(f"anchor {anchor} outside [{begin},{end})")
        if edge not in range(3) or channel not in range(3):
            raise SystemExit(f"unexpected edge/channel at anchor {anchor}")
        if tuple(row[index] for index in (5, 6, 7)) != expected_edge_identities[edge]:
            raise SystemExit(f"unexpected edge identity at anchor {anchor}")
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
print(f"validated development capture: anchors=[{begin},{end}) rows={row_count}")
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

verify_conditioning_seal() {
  require_sha256 "${conditioning_probe}" "${expected_conditioning_probe_sha}"
  require_sha256 "${conditioning_master}" "${expected_conditioning_master_sha}"
  require_sha256 "${conditioning_report}" "${expected_conditioning_report_sha}"
  require_sha256 \
    "${conditioning_report_and_master}" \
    "${expected_conditioning_report_and_master_sha}"
  require_sha256 "${objective_helper_src}" "${expected_objective_helper_sha}"
  require_sha256 "${calibration_header}" "${expected_calibration_header_sha}"
  require_sha256 "${digest_header}" "${expected_digest_header_sha}"
  (
    cd "${conditioning_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
  (
    cd "${repo_root}"
    sha256sum -c "${conditioning_report_and_master}" >/dev/null
  )
  expect_value "${conditioning_probe}" schema_id "${conditioning_schema}"
  expect_value "${conditioning_probe}" status complete
  expect_value "${conditioning_probe}" \
    input.development.sha256 "${expected_train_sha}"
  expect_value "${conditioning_probe}" boundary.maximum_anchor_loaded 729
  expect_value "${conditioning_probe}" data.maximum_anchor_loaded 729
  expect_value "${conditioning_probe}" split.fit '[0,554)'
  expect_value "${conditioning_probe}" split.purge '[554,584)'
  expect_value "${conditioning_probe}" split.validation '[584,730)'
  expect_value "${conditioning_probe}" split.holdout.opened false
  expect_value "${conditioning_probe}" boundary.unconsumed_holdout_opened false
  expect_value "${conditioning_probe}" \
    reference.R2_damped_materialized_f32_analytic.state_digest \
    "${expected_conditioning_state_digest}"
  expect_value "${conditioning_probe}" \
    reference.R2_damped_materialized_f32_analytic.prediction_digest \
    "${expected_conditioning_prediction_digest}"
  expect_value "${conditioning_probe}" \
    reference.R2_damped_materialized_f32_analytic.objective.total \
    "${expected_conditioning_objective}"
  expect_value "${conditioning_probe}" \
    reference.R2_damped_materialized_f32_analytic.deployment_parity.pass \
    false
  expect_value "${conditioning_probe}" arm.R2_damped_lbfgs.iterations 3
  expect_value "${conditioning_probe}" arm.R2_damped_lbfgs.closure_evaluations 3
  expect_value "${conditioning_probe}" arm.R2_damped_lbfgs.scientific_gate.pass true
  expect_value "${conditioning_probe}" \
    arm.R2_damped_lbfgs.scientific_gate.reference \
    reference.full_analytic
  expect_value "${conditioning_probe}" \
    arm.R2_damped_lbfgs.analytic_prediction_delta.reference \
    reference.R2_damped_materialized_f32_analytic
  expect_value "${conditioning_probe}" \
    arm.R2_damped_lbfgs.analytic_prediction_delta.maximum_abs_delta \
    "${expected_trained_arm_prediction_delta}"
}

preflight_inputs() {
  require_sha256 "${train_probe}" "${expected_train_sha}"
  require_sha256 "${representation_checkpoint}" "${expected_representation_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_sha}"
  require_sha256 "${split_dsl}" "${expected_split_sha}"
  require_file "${train_manifest}"
  require_file "${train_report}"
  [[ ! -e "${train_dir}/stage.sha256" ]] ||
    fail "legacy development capture unexpectedly acquired a stage seal"
  expect_value "${train_manifest}" graph_order_fingerprint "${graph_order_fingerprint}"
  expect_contains "${train_manifest}" source_cursor_token "accepted=1170"
  validate_capture_probe "${train_probe}"
  validate_split_contract
  verify_conditioning_seal
}

preflight_sources() {
  require_sha256 "${helper_src}" "${expected_helper_source_sha}"
  require_sha256 "${deployment_header}" "${expected_deployment_header_sha}"
  for token in \
    '--development-input' \
    '--conditioning-probe' \
    '--representation-checkpoint' \
    '--mdn-checkpoint' \
    '--graph-order-fingerprint' \
    '--artifact-output' \
    '--output' \
    '--schema-id' \
    '#include "mdn_frozen_affine_objective_ladder.cpp"' \
    '#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"'; do
    require_source_token "${token}"
  done
  reject_source_token '--evaluation''-input'
  reject_source_token '--histo''ry-input'
  reject_source_token '--re''fit-input'
  reject_source_token '--policy-input'
  reject_source_token '--holdout-input'
  reject_source_token '--oracle-archive'
  reject_source_token 'external_sidecar'
  reject_source_token '/jobs/''run/'
  bash -n "${BASH_SOURCE[0]}"
}

preflight() {
  preflight_inputs
  preflight_sources
}

pins_are_ready() {
  [[ "${expected_result_probe_sha}" =~ ^[0-9a-f]{64}$ &&
     "${expected_result_keyset_sha}" =~ ^[0-9a-f]{64}$ &&
     "${expected_result_key_count}" =~ ^[1-9][0-9]*$ &&
     "${expected_artifact_semantic_digest}" =~ ^[0-9a-f]{64}$ &&
     ( "${expected_artifact_file_sha}" =~ ^[0-9a-f]{64}$ ||
       "${expected_artifact_file_sha}" == "NONDETERMINISTIC" ) ]]
}

pins_are_unpinned() {
  [[ "${expected_result_probe_sha}" == "UNPINNED" &&
     "${expected_result_keyset_sha}" == "UNPINNED" &&
     "${expected_result_key_count}" == "UNPINNED" &&
     "${expected_artifact_semantic_digest}" == "UNPINNED" &&
     "${expected_artifact_file_sha}" == "UNPINNED" ]]
}

validate_pin_state() {
  pins_are_ready || pins_are_unpinned ||
    fail "result pins are partially patched; patch the probe and artifact pins together"
}

require_result_pins() {
  pins_are_ready ||
    fail "canonical execution is disabled until two scratch runs pin the probe and semantic artifact contract"
}

copy_frozen_header() {
  local destination="${frozen_include_root}/$2"
  mkdir -p "$(dirname "${destination}")"
  cp -- "$1" "${destination}"
}

freeze_sources() {
  mkdir -p "${frozen_source_dir}" "${frozen_include_root}"
  cp -- "${helper_src}" "${frozen_source_dir}/${helper_name}"
  cp -- "${objective_helper_src}" "${frozen_source_dir}/${objective_helper_name}"
  cp -- "${BASH_SOURCE[0]}" "${frozen_source_dir}/${runner_name}"
  copy_frozen_header "${deployment_header}" "${deployment_header_rel}"
  copy_frozen_header "${calibration_header}" "${calibration_header_rel}"
  copy_frozen_header "${digest_header}" "${digest_header_rel}"
  cmp -s "${helper_src}" "${frozen_source_dir}/${helper_name}" ||
    fail "frozen deployment helper copy drifted"
  cmp -s "${objective_helper_src}" "${frozen_source_dir}/${objective_helper_name}" ||
    fail "frozen objective helper copy drifted"
  cmp -s "${BASH_SOURCE[0]}" "${frozen_source_dir}/${runner_name}" ||
    fail "frozen runner copy drifted"
  require_sha256 "${frozen_source_dir}/${helper_name}" "${expected_helper_source_sha}"
  require_sha256 \
    "${frozen_source_dir}/${objective_helper_name}" \
    "${expected_objective_helper_sha}"
  require_sha256 \
    "${frozen_include_root}/${deployment_header_rel}" \
    "${expected_deployment_header_sha}"
  require_sha256 \
    "${frozen_include_root}/${calibration_header_rel}" \
    "${expected_calibration_header_sha}"
  require_sha256 \
    "${frozen_include_root}/${digest_header_rel}" \
    "${expected_digest_header_sha}"
  (
    cd "${runtime_dir}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${objective_helper_name}" \
      "frozen_sources/${runner_name}" \
      > frozen_sources.sha256
    sha256sum \
      "frozen_include/${deployment_header_rel}" \
      "frozen_include/${calibration_header_rel}" \
      "frozen_include/${digest_header_rel}" \
      > public_headers.sha256
  )
}

compile_helper() {
  local libtorch_path="${repo_root}/.external/libtorch-upd"
  local cuda_path="/usr/local/cuda-12.4"
  mkdir -p "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${frozen_source_dir}" \
    -I"${frozen_include_root}" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${frozen_source_dir}/${helper_name}" \
    -L"${libtorch_path}/lib" \
    -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda \
    -Wl,--as-needed -ltorch_cpu -ltorch -lc10 \
    -lcuda -lcudart -lnvToolsExt -lstdc++ -lpthread -lm \
    -o "${helper_bin}"
}

write_provenance() {
  {
    echo "authority=${capture_authority}"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "development_anchor_range=[0,730)"
    echo "fit_anchor_range=[0,554)"
    echo "purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "maximum_source_anchor=729"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "target=raw_future_base_close_minus_future_quote_close"
    echo "probe_boundary=post_adapter_direct_edge_features"
    echo "runtime_adapter_reapplied=false"
    echo "source_conditioning_schema=${conditioning_schema}"
    echo "source_conditioning_probe_sha256=${expected_conditioning_probe_sha}"
    echo "source_conditioning_reference=reference.R2_damped_materialized_f32_analytic"
    echo "source_conditioning_state_digest=${expected_conditioning_state_digest}"
    echo "source_conditioning_prediction_digest=${expected_conditioning_prediction_digest}"
    echo "source_conditioning_objective=${expected_conditioning_objective}"
    echo "source_conditioning_trained_arm_prediction_max_abs_delta=${expected_trained_arm_prediction_delta}"
    echo "source_conditioning_raw_affine_deployment_pass=false"
    echo "conditioning_transform_schema=synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1"
    echo "included_helper_source_sha256=${expected_objective_helper_sha}"
  } > "${runtime_dir}/capture_provenance.status"

  sha256sum \
    "${train_probe}" \
    "${train_manifest}" \
    "${train_report}" \
    "${representation_checkpoint}" \
    "${mdn_checkpoint}" \
    "${split_dsl}" \
    "${conditioning_probe}" \
    "${conditioning_master}" \
    "${conditioning_report}" \
    "${conditioning_report_and_master}" \
    > "${runtime_dir}/execution_inputs.sha256"
  if grep -Fq '/jobs/run/' "${runtime_dir}/execution_inputs.sha256"; then
    fail "non-development raw path leaked into the execution-input seal"
  fi
  python3 - "${runtime_dir}/execution_inputs.sha256" <<'PY'
import re
import sys

for number, line in enumerate(open(sys.argv[1], encoding="utf-8"), 1):
    line = line.rstrip("\n")
    match = re.fullmatch(r"[0-9a-f]{64}  (.+)", line)
    if match is None:
        raise SystemExit(f"malformed execution-input seal line {number}")
    path = match.group(1).lower()
    forbidden = (
        "history", "refit", "selection", "policy", "evaluation",
        "holdout",
    )
    if any(token in path for token in forbidden):
        raise SystemExit(
            f"forbidden raw path namespace in execution-input seal line {number}"
        )
print("validated deployment execution-input path boundary")
PY

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
  done < <(awk '/=> \// {print $3} /^[[:space:]]*\// {print $1}' \
    "${runtime_dir}/linked_libraries.txt" | sort -u)
  (
    cd "${runtime_dir}"
    sha256sum bin/mdn_frozen_affine_deployment_bridge > binary.sha256
  )

  {
    echo "schema_id=${schema_id}"
    echo "helper_source_sha256=$(sha256sum "${frozen_source_dir}/${helper_name}" | awk '{print $1}')"
    echo "included_helper_source_sha256=${expected_objective_helper_sha}"
    echo "deployment_header_sha256=${expected_deployment_header_sha}"
    echo "runner_sha256=$(sha256sum "${frozen_source_dir}/${runner_name}" | awk '{print $1}')"
    echo "helper_binary_sha256=$(sha256sum "${helper_bin}" | awk '{print $1}')"
    echo "development_probe_sha256=${expected_train_sha}"
    echo "conditioning_probe_sha256=${expected_conditioning_probe_sha}"
    echo "conditioning_reference=reference.R2_damped_materialized_f32_analytic"
    echo "conditioning_state_digest=${expected_conditioning_state_digest}"
    echo "conditioning_prediction_digest=${expected_conditioning_prediction_digest}"
    echo "development_anchor_range=[0,730)"
    echo "fit_anchor_range=[0,554)"
    echo "purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "maximum_source_anchor=729"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "deployment_gate_prediction_max_abs_delta=2e-5"
    echo "chunk_size=37"
    echo "operation_order=readout_float32_prefix_384_then_subtract_mean_float32_then_multiply_inv_std_float32_then_promote_float64_same_device_then_subtract_edge_center_float64_then_dot_mapped_weights_sum_float64_then_add_centered_bias_float64_then_cast_output_float32"
    echo "dtype_roles=input_float32_normalization_float32_coefficients_float64_accumulation_float64_output_float32"
    echo "suffix_policy=ignore_indices_384_through_399"
    echo "conditioning_transform_schema=synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1"
    echo "cuda_device_order=PCI_BUS_ID"
    echo "cublas_workspace_config=:4096:8"
  } > "${runtime_dir}/execution_contract.status"
}

run_probe() {
  local artifact="$1"
  local probe="$2"
  local log="$3"
  CUDA_DEVICE_ORDER=PCI_BUS_ID CUBLAS_WORKSPACE_CONFIG=:4096:8 \
    "${helper_bin}" \
    --development-input "${train_probe}" \
    --conditioning-probe "${conditioning_probe}" \
    --representation-checkpoint "${representation_checkpoint}" \
    --mdn-checkpoint "${mdn_checkpoint}" \
    --graph-order-fingerprint "${graph_order_fingerprint}" \
    --artifact-output "${artifact}" \
    --output "${probe}" \
    --schema-id "${schema_id}" \
    > "${log}" 2>&1
}

validate_result_probe() {
  local artifact="$1"
  local path="$2"
  local allow_unpinned="$3"
  require_file "${artifact}"
  require_file "${path}"
  [[ -s "${artifact}" ]] || fail "deployment artifact is empty: ${artifact}"
  if ! pins_are_ready && [[ "${allow_unpinned}" != "true" ]]; then
    require_result_pins
  fi
  python3 - \
    "${artifact}" \
    "${path}" \
    "${schema_id}" \
    "${expected_train_sha}" \
    "${expected_conditioning_probe_sha}" \
    "${expected_representation_sha}" \
    "${expected_mdn_sha}" \
    "${graph_order_fingerprint}" \
    "${expected_conditioning_state_digest}" \
    "${expected_conditioning_prediction_digest}" \
    "${expected_conditioning_objective}" \
    "${expected_trained_arm_prediction_delta}" \
    "${expected_result_probe_sha}" \
    "${expected_result_keyset_sha}" \
    "${expected_result_key_count}" \
    "${expected_artifact_semantic_digest}" \
    "${expected_artifact_file_sha}" <<'PY'
import hashlib
import math
import re
import sys

(
    artifact_path,
    path,
    schema_id,
    expected_development_sha,
    expected_conditioning_probe_sha,
    expected_representation_sha,
    expected_mdn_sha,
    expected_graph_fingerprint,
    expected_conditioning_state_digest,
    expected_conditioning_prediction_digest,
    expected_conditioning_objective_text,
    expected_trained_arm_delta_text,
    expected_probe_sha,
    expected_keyset_sha,
    expected_key_count_text,
    expected_artifact_semantic_digest,
    expected_artifact_file_sha,
) = sys.argv[1:]

raw = open(path, "rb").read()
artifact_raw = open(artifact_path, "rb").read()
if not raw.endswith(b"\n"):
    raise SystemExit("deployment probe lacks its final newline")
if b"\r" in raw or b"\x00" in raw:
    raise SystemExit("deployment probe contains a forbidden byte")
try:
    text = raw.decode("utf-8")
except UnicodeDecodeError as error:
    raise SystemExit(f"deployment probe is not UTF-8: {error}")

values = {}
for number, line in enumerate(text.splitlines(), 1):
    if not re.fullmatch(r"[A-Za-z0-9_.]+=[^\x00-\x1f\x7f]*", line):
        raise SystemExit(f"malformed deployment line {number}")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit(f"duplicate deployment key: {key}")
    if "/" in value or "\\" in value:
        raise SystemExit(f"physical path-like value leaked through {key}")
    components = key.lower().split(".")
    forbidden_namespaces = {
        "history", "historical", "refit", "selection", "policy",
        "evaluation",
    }
    if any(component in forbidden_namespaces for component in components):
        raise SystemExit(f"forbidden namespace in deployment probe: {key}")
    timing_components = {
        "elapsed", "duration", "timing", "wall_seconds", "milliseconds",
        "microseconds", "nanoseconds",
    }
    if any(component in timing_components for component in components):
        raise SystemExit(f"nondeterministic timing key in deployment probe: {key}")
    values[key] = value

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

def require_sha(key, expected=None):
    value = values.get(key)
    if value is None or re.fullmatch(r"[0-9a-f]{64}", value) is None:
        raise SystemExit(f"{key}: not a lowercase SHA-256")
    if expected is not None and value != expected:
        raise SystemExit(f"{key}: expected {expected}, got {value}")
    return value

def require_close(key, expected, rel_tol=1.0e-12, abs_tol=5.0e-15):
    actual = number(key)
    if not math.isclose(actual, expected, rel_tol=rel_tol, abs_tol=abs_tol):
        raise SystemExit(f"{key}: expected recomputed {expected}, got {actual}")

exact("schema_id", schema_id)
exact("status", "complete")
exact("input.development.sha256", expected_development_sha)
exact("input.conditioning_probe.sha256", expected_conditioning_probe_sha)
exact("input.representation_checkpoint.sha256", expected_representation_sha)
exact("input.mdn_checkpoint.sha256", expected_mdn_sha)
exact("input.graph_order_fingerprint", expected_graph_fingerprint)
exact("data.record_schema", "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1")
exact("data.readout_shape", "[730,3,3,400]")
require_sha("data.live_readout_digest")
require_sha("data.dynamic_prefix_digest")
if integer("data.maximum_anchor_loaded") != 729:
    raise SystemExit("deployment helper loaded outside development anchors")
exact("boundary.development_only", "true")
exact("boundary.maximum_anchor_loaded", "729")
exact("boundary.holdout.opened", "false")
exact("split.fit", "[0,554)")
exact("split.purge", "[554,584)")
exact("split.validation", "[584,730)")
exact("split.holdout.opened", "false")

exact("runtime.cpu_thread_count", "1")
exact("runtime.cpu_interop_thread_count", "1")
exact("runtime.deterministic_algorithms", "true")
exact("runtime.allow_tf32_cublas", "false")
exact("runtime.allow_tf32_cudnn", "false")
exact("runtime.seed", "31")
for key in (
    "runtime.torch_version",
    "runtime.cuda_compile_version",
    "runtime.cuda_runtime_version",
    "runtime.cuda_driver_version",
    "runtime.cuda_device_name",
):
    if not values.get(key):
        raise SystemExit(f"{key}: missing or empty")

deployment_threshold = number("execution.deployment_threshold")
if number("execution.alpha") != 1.0e-10:
    raise SystemExit("deployment ridge alpha drift")
if deployment_threshold != 2.0e-5:
    raise SystemExit("deployment threshold drift")
if integer("execution.chunk_size") != 37:
    raise SystemExit("deployment chunk-size drift")
exact(
    "execution.operation_order",
    "readout_float32_prefix_384_then_subtract_mean_float32_then_multiply_inv_std_float32_then_promote_float64_same_device_then_subtract_edge_center_float64_then_dot_mapped_weights_sum_float64_then_add_centered_bias_float64_then_cast_output_float32",
)
exact(
    "execution.dtype_roles",
    "input_float32_normalization_float32_coefficients_float64_accumulation_float64_output_float32",
)
exact("execution.suffix_policy", "ignore_indices_384_through_399")
exact(
    "execution.suffix_perturbation",
    "alternating_quiet_nan_and_positive_infinity_indices_384_through_399",
)
exact("execution.fit_scope", "[0,554)")
exact("execution.validation_scope", "[584,730)")
exact("execution.fit_computation_uses_validation", "false")

require_sha(
    "source.conditioning_reference.state_digest",
    expected_conditioning_state_digest,
)
require_sha(
    "source.conditioning_reference.prediction_digest",
    expected_conditioning_prediction_digest,
)
data_mse = number("source.conditioning_reference.objective.data_mse")
ridge_penalty = number("source.conditioning_reference.objective.ridge_penalty")
objective = number("source.conditioning_reference.objective.total")
if data_mse < 0 or ridge_penalty < 0:
    raise SystemExit("conditioning-reference objective component is negative")
require_close(
    "source.conditioning_reference.objective.total",
    data_mse + ridge_penalty,
)
if not math.isclose(
    objective,
    float(expected_conditioning_objective_text),
    rel_tol=1.0e-12,
    abs_tol=5.0e-15,
):
    raise SystemExit("conditioning-reference objective does not match seal")
exact("source.conditioning_reference.reconstructed_state_match", "true")
exact("source.conditioning_reference.reconstructed_prediction_match", "true")
exact("source.conditioning_reference.reconstructed_objective_match", "true")
require_close(
    "source.conditioning_reference.trained_arm_to_analytic.maximum_abs_delta",
    float(expected_trained_arm_delta_text),
)
require_close(
    "source.conditioning_reference.trained_arm_to_analytic.threshold",
    deployment_threshold,
)
expected_trained_pass = (
    float(expected_trained_arm_delta_text) <= deployment_threshold
)
if boolean("source.conditioning_reference.trained_arm_to_analytic.pass") != expected_trained_pass:
    raise SystemExit("trained-arm applicability pass does not recompute")

for key in (
    "state.normalization_mean.digest",
    "state.normalization_inv_std.digest",
    "state.edge_center.digest",
    "state.damped_transform.digest",
    "state.theta.digest",
    "state.mapped_weights.digest",
    "state.centered_bias.digest",
    "state.uncentered_standardized_bias.digest",
):
    require_sha(key)

exact(
    "source.conditioning_transform.schema_id",
    "synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1",
)
conditioning_transform_digest = require_sha(
    "source.conditioning_transform.sha256"
)
if conditioning_transform_digest == expected_conditioning_state_digest:
    raise SystemExit(
        "conditioning transform contract was incorrectly aliased to state digest"
    )

for edge in range(3):
    prefix = f"conditioner.edge.{edge}"
    if integer(prefix + ".rank") != 384:
        raise SystemExit(f"{prefix}: rank is not the full 384-mode surface")
    if integer(prefix + ".mode_count") != 384:
        raise SystemExit(f"{prefix}: mode count is not 384")
    require_sha(prefix + ".transform_digest")
    require_sha(prefix + ".theta_digest")

exact(
    "reference.canonical_materialized_damped_f64.operation_order",
    "materialized_damped_coordinates_float32_promoted_float64_then_theta_float64_dot_plus_centered_bias_float64_then_cast_float32",
)
require_sha(
    "reference.canonical_materialized_damped_f64.prediction_digest",
    expected_conditioning_prediction_digest,
)
for key in (
    "reference.materialized_direct_f32.cpu.prediction_digest",
    "reference.materialized_direct_f32.cuda.prediction_digest",
    "reference.live_centered_mapped_f64.cpu.prediction_digest",
    "reference.live_centered_mapped_f64.cuda.prediction_digest",
):
    require_sha(key)

arm_operation_orders = {
    "arm.unsafe_uncentered_raw_f32":
        "readout_prefix_float32_normalize_float32_then_uncentered_mapped_weight_dot_float32_plus_uncentered_standardized_bias_float32",
    "arm.live_centered_mapped_f32":
        "readout_prefix_float32_normalize_float32_then_subtract_edge_center_float32_then_mapped_weight_dot_float32_plus_centered_bias_float32",
    "arm.live_explicit_damped_f32":
        "readout_prefix_float32_normalize_float32_then_subtract_edge_center_float32_then_damped_transform_float32_then_theta_dot_float32_plus_centered_bias_float32",
}
for arm, operation_order in arm_operation_orders.items():
    exact(arm + ".operation_order", operation_order)
    require_sha(arm + ".prediction_digest")

artifact_digest = require_sha("artifact.semantic_digest")
for key in (
    "artifact.source.semantic_digest",
    "artifact.reloaded.semantic_digest",
    "artifact.reloaded_cpu.semantic_digest",
    "artifact.reloaded_cuda.semantic_digest",
):
    require_sha(key, artifact_digest)
for key in (
    "artifact.source.conditioning_transform.schema_id",
    "artifact.reloaded.conditioning_transform.schema_id",
):
    exact(
        key,
        "synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1",
    )
for key in (
    "artifact.source.conditioning_transform.sha256",
    "artifact.reloaded.conditioning_transform.sha256",
):
    require_sha(key, conditioning_transform_digest)
if expected_artifact_semantic_digest != "UNPINNED":
    if artifact_digest != expected_artifact_semantic_digest:
        raise SystemExit("artifact semantic digest drift")
exact(
    "artifact.family",
    "wikimyei.inference.expected_value.mdn.per_edge_conditioned_affine_return_head.v2",
)
if integer("artifact.schema_version") != 2:
    raise SystemExit("artifact schema version drift")
exact("artifact.run_only", "true")
for key in (
    "artifact.in_memory.cpu.prediction_digest",
    "artifact.in_memory.cuda.prediction_digest",
    "artifact.reloaded_cpu.prediction_digest",
    "artifact.reloaded_cuda.prediction_digest",
):
    require_sha(key)

comparison_pairs = {
    "parity.canonical_to_materialized_direct_f32_cpu": (
        "reference.canonical_materialized_damped_f64",
        "reference.materialized_direct_f32.cpu",
    ),
    "parity.canonical_to_materialized_direct_f32_cuda": (
        "reference.canonical_materialized_damped_f64",
        "reference.materialized_direct_f32.cuda",
    ),
    "parity.canonical_to_live_centered_f64_cuda": (
        "reference.canonical_materialized_damped_f64",
        "reference.live_centered_mapped_f64.cuda",
    ),
    "parity.live_centered_f64_cuda_full_to_chunked": (
        "reference.live_centered_mapped_f64.cuda.full",
        "reference.live_centered_mapped_f64.cuda.chunked",
    ),
    "arm.unsafe_uncentered_raw_f32.to_canonical_reference": (
        "arm.unsafe_uncentered_raw_f32",
        "reference.canonical_materialized_damped_f64",
    ),
    "arm.unsafe_uncentered_raw_f32.to_centered_f64_reference": (
        "arm.unsafe_uncentered_raw_f32",
        "reference.live_centered_mapped_f64",
    ),
    "arm.live_centered_mapped_f32.to_canonical_reference": (
        "arm.live_centered_mapped_f32",
        "reference.canonical_materialized_damped_f64",
    ),
    "arm.live_centered_mapped_f32.to_centered_f64_reference": (
        "arm.live_centered_mapped_f32",
        "reference.live_centered_mapped_f64",
    ),
    "arm.live_explicit_damped_f32.to_canonical_reference": (
        "arm.live_explicit_damped_f32",
        "reference.canonical_materialized_damped_f64",
    ),
    "arm.live_explicit_damped_f32.to_centered_f64_reference": (
        "arm.live_explicit_damped_f32",
        "reference.live_centered_mapped_f64",
    ),
    "parity.cpu_reference_to_in_memory": (
        "reference.canonical_materialized_damped_f64",
        "artifact.in_memory.cpu",
    ),
    "parity.cpu_to_cuda": (
        "artifact.in_memory.cpu",
        "artifact.in_memory.cuda",
    ),
    "parity.cpu_full_to_chunked": (
        "artifact.in_memory.cpu.full",
        "artifact.in_memory.cpu.chunked",
    ),
    "parity.cuda_full_to_chunked": (
        "artifact.in_memory.cuda.full",
        "artifact.in_memory.cuda.chunked",
    ),
    "parity.in_memory_to_archive_reload": (
        "artifact.in_memory.cuda",
        "artifact.reloaded.cuda",
    ),
    "parity.cpu_in_memory_to_archive_reload": (
        "artifact.in_memory.cpu",
        "artifact.reloaded.cpu",
    ),
    "parity.archive_cpu_full_to_chunked": (
        "artifact.reloaded.cpu.full",
        "artifact.reloaded.cpu.chunked",
    ),
    "parity.archive_cuda_full_to_chunked": (
        "artifact.reloaded.cuda.full",
        "artifact.reloaded.cuda.chunked",
    ),
    "parity.live_centered_f64_to_in_memory_cpu": (
        "reference.live_centered_mapped_f64.cpu",
        "artifact.in_memory.cpu",
    ),
    "parity.live_centered_f64_to_in_memory_cuda": (
        "reference.live_centered_mapped_f64.cuda",
        "artifact.in_memory.cuda",
    ),
    "parity.archive_cpu_suffix_ignored": (
        "artifact.reloaded.cpu.original_suffix",
        "artifact.reloaded.cpu.changed_suffix",
    ),
    "parity.archive_cuda_suffix_ignored": (
        "artifact.reloaded.cuda.original_suffix",
        "artifact.reloaded.cuda.changed_suffix",
    ),
}
comparison_fields = {
    "lhs", "rhs", "scope", "torch_equal", "maximum_abs_delta",
    "mean_abs_delta", "rmse_delta", "threshold", "pass",
}

def validate_comparison(prefix, lhs, rhs, scope):
    exact(prefix + ".lhs", lhs)
    exact(prefix + ".rhs", rhs)
    exact(prefix + ".scope", scope)
    maximum = number(prefix + ".maximum_abs_delta")
    mean = number(prefix + ".mean_abs_delta")
    rmse = number(prefix + ".rmse_delta")
    threshold = number(prefix + ".threshold")
    if min(maximum, mean, rmse) < 0:
        raise SystemExit(f"{prefix}: negative comparison diagnostic")
    if mean > maximum + 1.0e-15 or rmse > maximum + 1.0e-15:
        raise SystemExit(f"{prefix}: aggregate delta exceeds maximum")
    if threshold != deployment_threshold:
        raise SystemExit(f"{prefix}: threshold drift")
    equal = boolean(prefix + ".torch_equal")
    if equal != (maximum == 0.0):
        raise SystemExit(f"{prefix}: equality flag disagrees with maximum")
    expected_pass = maximum <= deployment_threshold
    if boolean(prefix + ".pass") != expected_pass:
        raise SystemExit(f"{prefix}: pass does not recompute")
    return expected_pass, equal

comparison_results = {}
for prefix, (lhs, rhs) in comparison_pairs.items():
    comparison_results[prefix] = validate_comparison(
        prefix, lhs, rhs, "[0,730)"
    )
    validation_prefix = prefix + ".validation"
    comparison_results[validation_prefix] = validate_comparison(
        validation_prefix, lhs, rhs, "[584,730)"
    )

for prefix in (
    "parity.archive_cpu_suffix_ignored",
    "parity.archive_cuda_suffix_ignored",
):
    if not comparison_results[prefix][1]:
        raise SystemExit(f"{prefix}: suffix invariance is not torch-exact")

if boolean("conclusion.unsafe_uncentered_raw_f32.pass") != comparison_results[
    "arm.unsafe_uncentered_raw_f32.to_canonical_reference"
][0]:
    raise SystemExit("unsafe-raw conclusion does not recompute")
if boolean("conclusion.live_centered_mapped_f32.pass") != comparison_results[
    "arm.live_centered_mapped_f32.to_canonical_reference"
][0]:
    raise SystemExit("centered-f32 conclusion does not recompute")
if boolean("conclusion.live_explicit_damped_f32.pass") != comparison_results[
    "arm.live_explicit_damped_f32.to_canonical_reference"
][0]:
    raise SystemExit("explicit-damped-f32 conclusion does not recompute")
if boolean("conclusion.live_centered_mapped_f64.pass") != comparison_results[
    "parity.canonical_to_live_centered_f64_cuda"
][0]:
    raise SystemExit("centered-f64 conclusion does not recompute")
semantic_match = all(
    values[key] == artifact_digest
    for key in (
        "artifact.source.semantic_digest",
        "artifact.reloaded.semantic_digest",
        "artifact.reloaded_cpu.semantic_digest",
        "artifact.reloaded_cuda.semantic_digest",
    )
)
if boolean("conclusion.artifact_semantic_match") != semantic_match:
    raise SystemExit("artifact semantic conclusion does not recompute")
core_bridge_prefixes = (
    "parity.cpu_reference_to_in_memory",
    "parity.cpu_to_cuda",
)
exact_identity_prefixes = (
    "parity.cpu_full_to_chunked",
    "parity.cuda_full_to_chunked",
    "parity.in_memory_to_archive_reload",
    "parity.cpu_in_memory_to_archive_reload",
    "parity.archive_cpu_full_to_chunked",
    "parity.archive_cuda_full_to_chunked",
    "parity.live_centered_f64_to_in_memory_cpu",
    "parity.live_centered_f64_to_in_memory_cuda",
    "parity.archive_cpu_suffix_ignored",
    "parity.archive_cuda_suffix_ignored",
)
bridge_pass = (
    not comparison_results[
        "arm.unsafe_uncentered_raw_f32.to_canonical_reference"
    ][0]
    and expected_trained_pass
    and comparison_results[
        "parity.canonical_to_live_centered_f64_cuda"
    ][0]
    and all(comparison_results[prefix][0] for prefix in core_bridge_prefixes)
    and all(comparison_results[prefix][1] for prefix in exact_identity_prefixes)
    and semantic_match
)
if boolean("conclusion.deployment_bridge_pass") != bridge_pass:
    raise SystemExit("deployment-bridge conclusion does not recompute")
if not bridge_pass:
    raise SystemExit("deployment bridge did not pass its operational contract")
exact(
    "conclusion.result_role",
    "development_only_deployment_arithmetic_diagnostic",
)

fixed_keys = {
    "schema_id", "status",
    "runtime.torch_version", "runtime.cuda_compile_version",
    "runtime.cuda_runtime_version", "runtime.cuda_driver_version",
    "runtime.cuda_device_name", "runtime.cpu_thread_count",
    "runtime.cpu_interop_thread_count", "runtime.deterministic_algorithms",
    "runtime.allow_tf32_cublas", "runtime.allow_tf32_cudnn", "runtime.seed",
    "input.development.sha256", "input.conditioning_probe.sha256",
    "input.representation_checkpoint.sha256", "input.mdn_checkpoint.sha256",
    "input.graph_order_fingerprint", "data.record_schema",
    "data.readout_shape", "data.live_readout_digest",
    "data.dynamic_prefix_digest", "data.maximum_anchor_loaded",
    "boundary.development_only", "boundary.maximum_anchor_loaded",
    "boundary.holdout.opened", "split.fit", "split.purge",
    "split.validation", "split.holdout.opened", "execution.alpha",
    "execution.deployment_threshold", "execution.chunk_size",
    "execution.operation_order", "execution.dtype_roles",
    "execution.suffix_policy", "execution.suffix_perturbation",
    "execution.fit_scope",
    "execution.validation_scope", "execution.fit_computation_uses_validation",
    "source.conditioning_reference.state_digest",
    "source.conditioning_reference.prediction_digest",
    "source.conditioning_reference.objective.data_mse",
    "source.conditioning_reference.objective.ridge_penalty",
    "source.conditioning_reference.objective.total",
    "source.conditioning_reference.reconstructed_state_match",
    "source.conditioning_reference.reconstructed_prediction_match",
    "source.conditioning_reference.reconstructed_objective_match",
    "source.conditioning_reference.trained_arm_to_analytic.maximum_abs_delta",
    "source.conditioning_reference.trained_arm_to_analytic.threshold",
    "source.conditioning_reference.trained_arm_to_analytic.pass",
    "state.normalization_mean.digest", "state.normalization_inv_std.digest",
    "state.edge_center.digest", "state.damped_transform.digest",
    "state.theta.digest", "state.mapped_weights.digest",
    "state.centered_bias.digest", "state.uncentered_standardized_bias.digest",
    "source.conditioning_transform.schema_id",
    "source.conditioning_transform.sha256",
    "reference.canonical_materialized_damped_f64.operation_order",
    "reference.canonical_materialized_damped_f64.prediction_digest",
    "reference.materialized_direct_f32.cpu.prediction_digest",
    "reference.materialized_direct_f32.cuda.prediction_digest",
    "reference.live_centered_mapped_f64.cpu.prediction_digest",
    "reference.live_centered_mapped_f64.cuda.prediction_digest",
    "artifact.family", "artifact.schema_version", "artifact.run_only",
    "artifact.semantic_digest", "artifact.source.semantic_digest",
    "artifact.reloaded.semantic_digest",
    "artifact.reloaded_cpu.semantic_digest",
    "artifact.reloaded_cuda.semantic_digest",
    "artifact.source.conditioning_transform.schema_id",
    "artifact.source.conditioning_transform.sha256",
    "artifact.reloaded.conditioning_transform.schema_id",
    "artifact.reloaded.conditioning_transform.sha256",
    "artifact.in_memory.cpu.prediction_digest",
    "artifact.in_memory.cuda.prediction_digest",
    "artifact.reloaded_cpu.prediction_digest",
    "artifact.reloaded_cuda.prediction_digest",
    "conclusion.unsafe_uncentered_raw_f32.pass",
    "conclusion.live_centered_mapped_f32.pass",
    "conclusion.live_explicit_damped_f32.pass",
    "conclusion.live_centered_mapped_f64.pass",
    "conclusion.artifact_semantic_match",
    "conclusion.deployment_bridge_pass", "conclusion.result_role",
}
for edge in range(3):
    fixed_keys.update({
        f"conditioner.edge.{edge}.rank",
        f"conditioner.edge.{edge}.mode_count",
        f"conditioner.edge.{edge}.transform_digest",
        f"conditioner.edge.{edge}.theta_digest",
    })
for arm in arm_operation_orders:
    fixed_keys.update({arm + ".operation_order", arm + ".prediction_digest"})
for prefix in comparison_pairs:
    fixed_keys.update(prefix + "." + field for field in comparison_fields)
    fixed_keys.update(
        prefix + ".validation." + field for field in comparison_fields
    )
if set(values) != fixed_keys:
    missing = sorted(fixed_keys - set(values))
    extra = sorted(set(values) - fixed_keys)
    raise SystemExit(
        f"deployment probe key contract mismatch: missing={missing}, extra={extra}"
    )

keyset = "".join(f"{key}\n" for key in sorted(values)).encode()
actual_probe_sha = hashlib.sha256(raw).hexdigest()
actual_keyset_sha = hashlib.sha256(keyset).hexdigest()
actual_key_count = len(values)
actual_artifact_sha = hashlib.sha256(artifact_raw).hexdigest()
if expected_probe_sha != "UNPINNED":
    if actual_probe_sha != expected_probe_sha:
        raise SystemExit(
            f"probe SHA drift: expected {expected_probe_sha}, got {actual_probe_sha}"
        )
    if actual_keyset_sha != expected_keyset_sha:
        raise SystemExit(
            f"key-set SHA drift: expected {expected_keyset_sha}, got {actual_keyset_sha}"
        )
    if actual_key_count != int(expected_key_count_text):
        raise SystemExit(
            f"key count drift: expected {expected_key_count_text}, got {actual_key_count}"
        )
if re.fullmatch(r"[0-9a-f]{64}", expected_artifact_file_sha):
    if actual_artifact_sha != expected_artifact_file_sha:
        raise SystemExit(
            "deterministic artifact file SHA drift: "
            f"expected {expected_artifact_file_sha}, got {actual_artifact_sha}"
        )

print(
    "validated deployment bridge: "
    f"keys={actual_key_count} sha256={actual_probe_sha} "
    f"keyset_sha256={actual_keyset_sha} "
    f"artifact_semantic_digest={artifact_digest} max_anchor=729"
)
PY
}

validate_scratch_pair() {
  local first_artifact="$1"
  local first_probe="$2"
  local first_log="$3"
  local second_artifact="$4"
  local second_probe="$5"
  local second_log="$6"
  pins_are_unpinned ||
    fail "scratch validation mode requires all result pins to be UNPINNED"
  require_file "${first_log}"
  require_file "${second_log}"
  validate_result_probe "${first_artifact}" "${first_probe}" true
  validate_result_probe "${second_artifact}" "${second_probe}" true
  cmp -s "${first_probe}" "${second_probe}" ||
    fail "scratch deployment probes are not byte-identical"
  cmp -s "${first_log}" "${second_log}" ||
    fail "scratch deployment stdout logs are not byte-identical"
  [[ ! -s "${first_log}" && ! -s "${second_log}" ]] ||
    fail "successful scratch runs must not emit stdout or stderr"
  local first_semantic second_semantic
  first_semantic="$(probe_value "${first_probe}" artifact.semantic_digest)"
  second_semantic="$(probe_value "${second_probe}" artifact.semantic_digest)"
  [[ "${first_semantic}" == "${second_semantic}" ]] ||
    fail "scratch artifact semantic digests differ"
  python3 - \
    "${first_artifact}" \
    "${first_probe}" \
    "${second_artifact}" \
    "${first_semantic}" <<'PY'
import hashlib
import re
import sys

first_artifact, probe_path, second_artifact, semantic_digest = sys.argv[1:]
raw = open(probe_path, "rb").read()
keys = []
for line in raw.decode("utf-8").splitlines():
    if not re.fullmatch(r"[A-Za-z0-9_.]+=.*", line):
        raise SystemExit("cannot fingerprint malformed scratch probe")
    keys.append(line.split("=", 1)[0])
keyset = "".join(f"{key}\n" for key in sorted(keys)).encode()
first_raw = open(first_artifact, "rb").read()
second_raw = open(second_artifact, "rb").read()
print(f"expected_result_probe_sha={hashlib.sha256(raw).hexdigest()}")
print(f"expected_result_keyset_sha={hashlib.sha256(keyset).hexdigest()}")
print(f"expected_result_key_count={len(keys)}")
print(f"expected_artifact_semantic_digest={semantic_digest}")
if first_raw == second_raw:
    print(f"expected_artifact_file_sha={hashlib.sha256(first_raw).hexdigest()}")
else:
    print("expected_artifact_file_sha=NONDETERMINISTIC")
PY
}

verify_replay_contract() {
  validate_result_probe "${main_artifact}" "${main_probe}" false
  validate_result_probe "${replay_artifact}" "${replay_probe}" false
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay deployment probes are not byte-identical"
  cmp -s "${main_log}" "${replay_log}" ||
    fail "main/replay deployment stdout logs are not byte-identical"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful deployment runs must not emit stdout or stderr"
  [[ "$(probe_value "${main_probe}" artifact.semantic_digest)" == \
     "$(probe_value "${replay_probe}" artifact.semantic_digest)" ]] ||
    fail "main/replay artifact semantic digests differ"
  if [[ "${expected_artifact_file_sha}" =~ ^[0-9a-f]{64}$ ]]; then
    require_sha256 "${main_artifact}" "${expected_artifact_file_sha}"
    require_sha256 "${replay_artifact}" "${expected_artifact_file_sha}"
    cmp -s "${main_artifact}" "${replay_artifact}" ||
      fail "pinned deterministic main/replay artifacts are not byte-identical"
  fi
}

write_artifact_observation() {
  local byte_identity="false"
  local byte_identity_required="false"
  if cmp -s "${main_artifact}" "${replay_artifact}"; then
    byte_identity="true"
  fi
  if [[ "${expected_artifact_file_sha}" =~ ^[0-9a-f]{64}$ ]]; then
    byte_identity_required="true"
  fi
  {
    echo "artifact_byte_identity_required=${byte_identity_required}"
    echo "artifact_byte_identity_observed=${byte_identity}"
    echo "main_artifact_sha256=$(sha256sum "${main_artifact}" | awk '{print $1}')"
    echo "replay_artifact_sha256=$(sha256sum "${replay_artifact}" | awk '{print $1}')"
    echo "main_artifact_semantic_digest=$(probe_value "${main_probe}" artifact.semantic_digest)"
    echo "replay_artifact_semantic_digest=$(probe_value "${replay_probe}" artifact.semantic_digest)"
    echo "semantic_identity_required=true"
    echo "semantic_identity_pass=true"
    echo "generation_reload_validation_required=true"
    echo "generation_reload_validation_pass=true"
  } > "${artifact_observation}"
}

validate_artifact_observation() {
  require_file "${artifact_observation}"
  local byte_identity="false"
  local byte_identity_required="false"
  if cmp -s "${main_artifact}" "${replay_artifact}"; then
    byte_identity="true"
  fi
  if [[ "${expected_artifact_file_sha}" =~ ^[0-9a-f]{64}$ ]]; then
    byte_identity_required="true"
  fi
  expect_value "${artifact_observation}" \
    artifact_byte_identity_required "${byte_identity_required}"
  expect_value "${artifact_observation}" \
    artifact_byte_identity_observed "${byte_identity}"
  expect_value "${artifact_observation}" \
    main_artifact_sha256 "$(sha256sum "${main_artifact}" | awk '{print $1}')"
  expect_value "${artifact_observation}" \
    replay_artifact_sha256 "$(sha256sum "${replay_artifact}" | awk '{print $1}')"
  expect_value "${artifact_observation}" \
    main_artifact_semantic_digest \
    "$(probe_value "${main_probe}" artifact.semantic_digest)"
  expect_value "${artifact_observation}" \
    replay_artifact_semantic_digest \
    "$(probe_value "${replay_probe}" artifact.semantic_digest)"
  expect_value "${artifact_observation}" semantic_identity_required true
  expect_value "${artifact_observation}" semantic_identity_pass true
  expect_value "${artifact_observation}" \
    generation_reload_validation_required true
  expect_value "${artifact_observation}" \
    generation_reload_validation_pass true
  if [[ "${byte_identity_required}" == "true" &&
        "${byte_identity}" != "true" ]]; then
    fail "deterministic artifact pin requires byte-identical main/replay files"
  fi
}

seal_outputs() {
  (
    cd "${runtime_dir}"
    local shared=(
      bin/mdn_frozen_affine_deployment_bridge
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
      artifact_identity.status
    )
    sha256sum "${shared[@]}" \
      "main/${schema_id}.pt" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      > main/pair.sha256
    sha256sum "${shared[@]}" \
      "replay/${schema_id}.pt" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > replay/pair.sha256
    sha256sum \
      main/pair.sha256 \
      replay/pair.sha256 \
      "main/${schema_id}.pt" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.pt" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > outputs.sha256
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${objective_helper_name}" \
      "frozen_sources/${runner_name}" \
      "frozen_include/${deployment_header_rel}" \
      "frozen_include/${calibration_header_rel}" \
      "frozen_include/${digest_header_rel}" \
      bin/mdn_frozen_affine_deployment_bridge \
      frozen_sources.sha256 \
      public_headers.sha256 \
      binary.sha256 \
      compiler.sha256 \
      execution_inputs.sha256 \
      artifact_identity.status \
      outputs.sha256 \
      main/pair.sha256 \
      replay/pair.sha256 \
      > frozen_evidence.sha256
    sha256sum \
      frozen_evidence.sha256 \
      capture_provenance.status \
      execution_inputs.sha256 \
      execution_contract.status \
      artifact_identity.status \
      compiler.identity \
      compiler.version \
      compiler.sha256 \
      gpu.identity \
      frozen_sources.sha256 \
      public_headers.sha256 \
      binary.sha256 \
      linked_libraries.txt \
      binary.dynamic.txt \
      resolved_libraries.sha256 \
      outputs.sha256 \
      main/pair.sha256 \
      replay/pair.sha256 \
      "main/${schema_id}.pt" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.pt" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > master.sha256
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

verify_frozen_source_pins() {
  require_sha256 \
    "${frozen_source_dir}/${helper_name}" \
    "${expected_helper_source_sha}"
  require_sha256 \
    "${frozen_source_dir}/${objective_helper_name}" \
    "${expected_objective_helper_sha}"
  require_sha256 \
    "${frozen_include_root}/${deployment_header_rel}" \
    "${expected_deployment_header_sha}"
  require_sha256 \
    "${frozen_include_root}/${calibration_header_rel}" \
    "${expected_calibration_header_sha}"
  require_sha256 \
    "${frozen_include_root}/${digest_header_rel}" \
    "${expected_digest_header_sha}"
}

verify_complete() {
  require_result_pins
  set_runtime_paths "${final_runtime_dir}"
  require_file "${runtime_dir}/master.sha256"
  verify_frozen_source_pins
  preflight_inputs
  verify_seals
  verify_replay_contract
  validate_artifact_observation
  echo "${runtime_dir}"
}

run_all() {
  require_result_pins
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
    fail "another deployment installation holds ${lock_dir}"
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
  run_probe "${main_artifact}" "${main_probe}" "${main_log}"
  run_probe "${replay_artifact}" "${replay_probe}" "${replay_log}"
  verify_replay_contract
  write_artifact_observation
  validate_artifact_observation
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
    [[ "$#" == 1 ]] || fail "--preflight takes no arguments"
    validate_pin_state
    preflight
    ;;
  --validate-scratch)
    [[ "$#" == 7 ]] ||
      fail "usage: $0 --validate-scratch MAIN_ARTIFACT MAIN_PROBE MAIN_LOG REPLAY_ARTIFACT REPLAY_PROBE REPLAY_LOG"
    validate_scratch_pair "$2" "$3" "$4" "$5" "$6" "$7"
    ;;
  --run)
    [[ "$#" == 1 ]] || fail "--run takes no arguments"
    run_all
    ;;
  --verify)
    [[ "$#" == 1 ]] || fail "--verify takes no arguments"
    verify_complete
    ;;
  *)
    fail "usage: $0 [--preflight|--run|--verify|--validate-scratch MAIN_ARTIFACT MAIN_PROBE MAIN_LOG REPLAY_ARTIFACT REPLAY_PROBE REPLAY_LOG]"
    ;;
esac
