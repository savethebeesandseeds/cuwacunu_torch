#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_affine_conditioning_parity_v1"
capture_authority="verified_legacy_content_manifest_not_original_stage_seal"
graph_order_fingerprint="d334e38b1887ae16"
final_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"

helper_src="${script_dir}/mdn_frozen_affine_conditioning_parity.cpp"
included_helper_name="mdn_frozen_affine_objective_ladder.cpp"

objective_schema="synthetic_mdn_frozen_affine_objective_ladder_v1"
objective_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${objective_schema}"
included_helper_src="${objective_dir}/frozen_sources/${included_helper_name}"
objective_master="${objective_dir}/master.sha256"
objective_sources_manifest="${objective_dir}/frozen_sources.sha256"
objective_headers_manifest="${objective_dir}/public_headers.sha256"

calibration_header_rel="wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
digest_header_rel="piaabo/digest/sha256.h"
calibration_header="${objective_dir}/frozen_include/${calibration_header_rel}"
digest_header="${objective_dir}/frozen_include/${digest_header_rel}"

split_dsl="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/ujcamei.source.splits.dsl"
train_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
train_probe="${train_dir}/mdn_edge_context_features.probe"
train_manifest="${train_dir}/job.manifest"
train_report="${train_dir}/channel_inference.report"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

oracle_schema="synthetic_mdn_frozen_affine_calibration_sidecar_v1"
oracle_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${oracle_schema}"
oracle_archive="${oracle_dir}/main/${oracle_schema}.main.pt"
oracle_metadata="${oracle_dir}/main/${oracle_schema}.metadata"
oracle_master="${oracle_dir}/master.sha256"
oracle_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${oracle_schema}.report"
oracle_report_and_master="${oracle_dir}/report_and_master.sha256"

expected_train_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_representation_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_split_sha="cd189941903d18c7ddaa6a52cec8c34e384ac6de135cd93673a0569bc55cb68e"
expected_objective_master_sha="c3bf0375a4cf16b7e690c43452f6701f25fc86378a3d2c61b198d48c6f51fb64"
expected_helper_source_sha="ede7f9088725e1a0d41d2697f8d435aa01c5328c7c87d79716a58110e14c349b"
expected_included_helper_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"
expected_calibration_header_sha="44a366fd3e044f5e5b80eebdf7e461f78bc1c3d4eb03d647dfb2343876ca7d55"
expected_digest_header_sha="3efdf247d7ffa2a3cc89bdf541caef82dee6059ea78c55f32adbde0cdeb110d3"
expected_oracle_archive_sha="e83392e50d315e3889b2d1a0f082e2328d7447b72669853a24badd34e810ba88"
expected_oracle_metadata_sha="ca8eab16a79774acd3d490e916fe07f3d572ebcf516a85709cbe6a3b24a3f4ee"
expected_oracle_master_sha="4a7e525c2bb0da0ab4bac94539ec7640781fcba26edd0a631d212ce78795a4aa"
expected_oracle_report_sha="ecd184594931f2687e9e946f364845422c0e84a057b0d225d92d739b5ce345b7"
expected_oracle_report_and_master_sha="0ab0c00594787c2cfac9651c96a9df38158f73fe1d0e73dad9c2f7767cb2a372"
expected_oracle_semantic_digest="b0f3f00760d92d3026ed8675c9d6f572dd1c0b5de6f1c45bbd8b9253f99fd709"

# These three values deliberately block canonical installation until two
# independent scratch probes have passed --validate-scratch byte-for-byte.
expected_result_probe_sha="1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1"
expected_result_keyset_sha="c03215a394c3e2adc7281fb00a7bfbcfcb5e3ebc68952bbdee27510426c125d9"
expected_result_key_count="608"

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  helper_bin="${build_dir}/mdn_frozen_affine_conditioning_parity"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
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

reject_source_token() {
  local token="$1"
  if grep -Fq -- "${token}" "${helper_src}"; then
    fail "helper contains a forbidden development-boundary token"
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

verify_objective_source_seal() {
  require_sha256 "${objective_master}" "${expected_objective_master_sha}"
  require_sha256 "${included_helper_src}" "${expected_included_helper_sha}"
  require_sha256 "${calibration_header}" "${expected_calibration_header_sha}"
  require_sha256 "${digest_header}" "${expected_digest_header_sha}"
  require_file "${objective_sources_manifest}"
  require_file "${objective_headers_manifest}"
  (
    cd "${objective_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
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
    cd "${oracle_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
  (
    cd "${repo_root}"
    sha256sum -c "${oracle_report_and_master}" >/dev/null
  )
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
  verify_objective_source_seal
  verify_oracle_seal
}

preflight_sources() {
  require_sha256 "${helper_src}" "${expected_helper_source_sha}"
  for token in \
    '--development-input' \
    '--oracle-archive' \
    '--output' \
    '--schema-id' \
    '#include "mdn_frozen_affine_objective_ladder.cpp"'; do
    require_source_token "${token}"
  done
  reject_source_token '--evaluation''-input'
  reject_source_token 'input.''histo''rical'
  reject_source_token 'split.''histo''rical'
  reject_source_token 'split.''re''fit'
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
     "${expected_result_key_count}" =~ ^[1-9][0-9]*$ ]]
}

pins_are_unpinned() {
  [[ "${expected_result_probe_sha}" == "UNPINNED" &&
     "${expected_result_keyset_sha}" == "UNPINNED" &&
     "${expected_result_key_count}" == "UNPINNED" ]]
}

validate_pin_state() {
  pins_are_ready || pins_are_unpinned ||
    fail "result pins are partially patched; pin SHA, key-set SHA, and key count together"
}

require_result_pins() {
  pins_are_ready ||
    fail "canonical execution is disabled until scratch probe SHA, key-set SHA, and key count are pinned"
}

copy_frozen_header() {
  local destination="${frozen_include_root}/$2"
  mkdir -p "$(dirname "${destination}")"
  cp -- "$1" "${destination}"
}

freeze_sources() {
  mkdir -p "${frozen_source_dir}" "${frozen_include_root}"
  cp -- "${helper_src}" "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp"
  cp -- "${included_helper_src}" "${frozen_source_dir}/${included_helper_name}"
  cp -- "${BASH_SOURCE[0]}" "${frozen_source_dir}/run_mdn_frozen_affine_conditioning_parity.sh"
  copy_frozen_header "${calibration_header}" "${calibration_header_rel}"
  copy_frozen_header "${digest_header}" "${digest_header_rel}"
  cmp -s "${helper_src}" "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp" ||
    fail "frozen conditioning helper copy drifted"
  cmp -s "${included_helper_src}" "${frozen_source_dir}/${included_helper_name}" ||
    fail "frozen included helper copy drifted"
  cmp -s "${BASH_SOURCE[0]}" "${frozen_source_dir}/run_mdn_frozen_affine_conditioning_parity.sh" ||
    fail "frozen runner copy drifted"
  require_sha256 \
    "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp" \
    "${expected_helper_source_sha}"
  require_sha256 \
    "${frozen_source_dir}/${included_helper_name}" \
    "${expected_included_helper_sha}"
  require_sha256 \
    "${frozen_include_root}/${calibration_header_rel}" \
    "${expected_calibration_header_sha}"
  require_sha256 \
    "${frozen_include_root}/${digest_header_rel}" \
    "${expected_digest_header_sha}"
  (
    cd "${runtime_dir}"
    sha256sum \
      frozen_sources/mdn_frozen_affine_conditioning_parity.cpp \
      "frozen_sources/${included_helper_name}" \
      frozen_sources/run_mdn_frozen_affine_conditioning_parity.sh \
      > frozen_sources.sha256
    sha256sum \
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
    "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp" \
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
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "target=raw_future_base_close_minus_future_quote_close"
    echo "probe_boundary=post_adapter_direct_edge_features"
    echo "runtime_adapter_reapplied=false"
    echo "external_sidecar_schema=${oracle_schema}"
    echo "external_sidecar_fit_anchor_range=[0,730)"
    echo "external_sidecar_role=development_0_730_fitted_descriptive_not_validation_reference"
    echo "external_sidecar_used_as_gate_reference=false"
    echo "external_sidecar_semantic_tensor_digest=${expected_oracle_semantic_digest}"
    echo "included_helper_schema=${objective_schema}"
    echo "included_helper_source_sha256=${expected_included_helper_sha}"
  } > "${runtime_dir}/capture_provenance.status"

  : > "${runtime_dir}/execution_inputs.sha256"
  while IFS= read -r -d '' path; do
    sha256sum "${path}" >> "${runtime_dir}/execution_inputs.sha256"
  done < <(find "${train_dir}" -type f -print0 | sort -z)
  sha256sum \
    "${representation_checkpoint}" \
    "${mdn_checkpoint}" \
    "${split_dsl}" \
    "${oracle_archive}" \
    "${oracle_metadata}" \
    "${oracle_master}" \
    "${oracle_report}" \
    "${oracle_report_and_master}" \
    "${objective_master}" \
    "${objective_sources_manifest}" \
    "${objective_headers_manifest}" \
    >> "${runtime_dir}/execution_inputs.sha256"
  if grep -Fq '/jobs/run/' "${runtime_dir}/execution_inputs.sha256"; then
    fail "non-development capture path leaked into the execution-input seal"
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
  done < <(awk '/=> \// {print $3} /^[[:space:]]*\// {print $1}' \
    "${runtime_dir}/linked_libraries.txt" | sort -u)
  (
    cd "${runtime_dir}"
    sha256sum bin/mdn_frozen_affine_conditioning_parity > binary.sha256
  )

  {
    echo "schema_id=${schema_id}"
    echo "helper_source_sha256=$(sha256sum "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp" | awk '{print $1}')"
    echo "included_helper_source_sha256=${expected_included_helper_sha}"
    echo "runner_sha256=$(sha256sum "${frozen_source_dir}/run_mdn_frozen_affine_conditioning_parity.sh" | awk '{print $1}')"
    echo "helper_binary_sha256=$(sha256sum "${helper_bin}" | awk '{print $1}')"
    echo "development_probe_sha256=${expected_train_sha}"
    echo "external_sidecar_archive_sha256=${expected_oracle_archive_sha}"
    echo "external_sidecar_fit_anchor_range=[0,730)"
    echo "external_sidecar_role=development_0_730_fitted_descriptive_not_validation_reference"
    echo "external_sidecar_used_as_gate_reference=false"
    echo "external_sidecar_semantic_tensor_digest=${expected_oracle_semantic_digest}"
    echo "development_anchor_range=[0,730)"
    echo "fit_anchor_range=[0,554)"
    echo "purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
    echo "cuda_device_order=PCI_BUS_ID"
    echo "cublas_workspace_config=:4096:8"
    echo "method_order=reference.context_recomputed_fit_analytic,reference.full_analytic,reference.R0_retained_analytic,reference.R1_materialized_f32_analytic,reference.R2_ordinary_materialized_f32_analytic,reference.R2_damped_materialized_f32_analytic,arm.R1_adam,arm.R1_lbfgs,arm.R2_ordinary_lbfgs,arm.R2_damped_lbfgs"
  } > "${runtime_dir}/execution_contract.status"
}

run_probe() {
  local output="$1"
  local log="$2"
  CUDA_DEVICE_ORDER=PCI_BUS_ID CUBLAS_WORKSPACE_CONFIG=:4096:8 \
    "${helper_bin}" \
    --development-input "${train_probe}" \
    --oracle-archive "${oracle_archive}" \
    --output "${output}" \
    --schema-id "${schema_id}" \
    > "${log}" 2>&1
}

validate_result_probe() {
  local path="$1"
  local allow_unpinned="$2"
  require_file "${path}"
  if ! pins_are_ready && [[ "${allow_unpinned}" != "true" ]]; then
    require_result_pins
  fi
  python3 - \
    "${path}" \
    "${schema_id}" \
    "${expected_train_sha}" \
    "${expected_oracle_archive_sha}" \
    "${expected_oracle_semantic_digest}" \
    "${expected_result_probe_sha}" \
    "${expected_result_keyset_sha}" \
    "${expected_result_key_count}" <<'PY'
import hashlib
import math
import re
import sys

(
    path,
    schema_id,
    expected_development_sha,
    expected_oracle_sha,
    expected_oracle_semantic_digest,
    expected_probe_sha,
    expected_keyset_sha,
    expected_key_count_text,
) = sys.argv[1:]
raw = open(path, "rb").read()
if not raw.endswith(b"\n"):
    raise SystemExit("conditioning probe lacks its final newline")
try:
    text = raw.decode("utf-8")
except UnicodeDecodeError as error:
    raise SystemExit(f"conditioning probe is not UTF-8: {error}")

values = {}
for number, line in enumerate(text.splitlines(), 1):
    if not re.fullmatch(r"[A-Za-z0-9_.]+=[^\x00-\x1f\x7f]*", line):
        raise SystemExit(f"malformed conditioning line {number}")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit(f"duplicate conditioning key: {key}")
    if "/" in value or "\\" in value:
        raise SystemExit(f"physical path-like value leaked through {key}")
    values[key] = value

forbidden = ("histo" + "rical", "re" + "fit", "selec" + "tion")
for key in values:
    lowered = key.lower()
    if any(token in lowered for token in forbidden):
        raise SystemExit(f"forbidden namespace in conditioning probe: {key}")

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

exact("schema_id", schema_id)
exact("status", "complete")
exact("input.development.sha256", expected_development_sha)
exact("input.external_sidecar_archive.sha256", expected_oracle_sha)
exact("data.record_schema", "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1")
exact(
    "data.canonical_design",
    "cached_384_float32_dynamic_features_exact_fit_only_shared_float32_normalization_materialized_float32_then_promoted_to_cpu_float64",
)
exact("boundary.development_only", "true")
exact("boundary.maximum_anchor_loaded", "729")
exact("boundary.unconsumed_holdout_opened", "false")
exact("split.fit", "[0,554)")
exact("split.purge", "[554,584)")
exact("split.validation", "[584,730)")
exact("split.holdout.opened", "false")
if integer("data.maximum_anchor_loaded") != 729:
    raise SystemExit("conditioning helper loaded outside development anchors")
exact("runtime.cpu_thread_count", "1")
exact("runtime.cpu_interop_thread_count", "1")
exact("runtime.deterministic_algorithms", "true")
exact("runtime.allow_tf32_cublas", "false")
exact("runtime.allow_tf32_cudnn", "false")
exact("runtime.seed", "31")
exact(
    "execution.ridge_objective",
    "data_mse_plus_alpha_div_edge_count_times_mapped_weight_squared_norm",
)
exact("execution.bias_penalized", "false")
exact("execution.adam.steps", "3500")
exact("execution.lbfgs.max_iterations", "500")
exact("execution.lbfgs.max_evaluations", "625")
exact("execution.lbfgs.line_search", "strong_wolfe")
exact("execution.deployment_gate.separate_from_scientific_gate", "true")
exact("data.normalization.scope", "[0,554)")
exact("data.normalization.accumulation", "float32")
exact(
    "execution.full_canonical_surface_kkt.role",
    "full_canonical_standardized_coordinate_gradient_residual_including_omitted_modes_and_materialization_error",
)
exact(
    "execution.optimizer_surface_kkt.role",
    "direct_parameter_coordinate_gradient_of_data_mse_plus_alpha_div_edge_count_mapped_weight_penalty",
)
exact("execution.deployment_gate.scope", "[0,730)")
exact("external_sidecar.semantic_tensor_digest", expected_oracle_semantic_digest)
exact("external_sidecar.parameters_frozen", "true")
exact("external_sidecar.fit_scope", "[0,730)")
exact(
    "external_sidecar.role",
    "development_0_730_fitted_descriptive_not_validation_reference",
)
exact("external_sidecar.used_as_gate_reference", "false")
exact("conclusion.result_role", "development_conditioning_diagnostic")
if number("execution.alpha") != 1.0e-10:
    raise SystemExit("ridge alpha drift")
if number("execution.adam.learning_rate") != 1.0e-3:
    raise SystemExit("Adam learning-rate drift")

references = (
    "reference.full_analytic",
    "reference.R0_retained_analytic",
    "reference.R1_materialized_f32_analytic",
    "reference.R2_ordinary_materialized_f32_analytic",
    "reference.R2_damped_materialized_f32_analytic",
)
arms = (
    "arm.R1_adam",
    "arm.R1_lbfgs",
    "arm.R2_ordinary_lbfgs",
    "arm.R2_damped_lbfgs",
)

def validate_metric(prefix, expected_count):
    if integer(prefix + ".count") != expected_count:
        raise SystemExit(f"{prefix}: count drift")
    if integer(prefix + ".pair_count") != expected_count:
        raise SystemExit(f"{prefix}: pair-count drift")
    rmse = number(prefix + ".rmse")
    mae = number(prefix + ".mae")
    direction = number(prefix + ".directional_accuracy")
    rank = number(prefix + ".pairwise_rank_accuracy")
    correlation = number(prefix + ".correlation")
    if rmse < 0 or mae < 0:
        raise SystemExit(f"{prefix}: negative error metric")
    if not 0 <= direction <= 1 or not 0 <= rank <= 1:
        raise SystemExit(f"{prefix}: accuracy outside [0,1]")
    if not -1 <= correlation <= 1:
        raise SystemExit(f"{prefix}: correlation outside [-1,1]")

for method in references + arms:
    validate_metric(method + ".fit", 4986)
    validate_metric(method + ".validation", 1314)
for reference in references:
    exact(reference + ".fit_scope", "[0,554)")
    exact(reference + ".validation_scope", "[584,730)")
validate_metric("external_sidecar.validation", 1314)

context_reference = "reference.context_recomputed_fit_analytic"
exact(context_reference + ".fit_scope", "[0,554)")
exact(context_reference + ".validation_scope", "[584,730)")
exact(
    context_reference + ".feature_surface",
    "context_float32_promoted_to_float64_then_base_quote_difference_recomputed",
)
if number(context_reference + ".maximum_normalized_residual") < 0:
    raise SystemExit("context analytic residual is negative")
if number(context_reference + ".coefficient_l2_norm") < 0:
    raise SystemExit("context analytic coefficient norm is negative")
validate_metric(context_reference + ".fit", 4986)
validate_metric(context_reference + ".validation", 1314)
validate_comparison_key = (
    "reference.full_analytic_to_context_recomputed_fit_analytic.validation"
)

direction_tolerance = number(
    "execution.scientific_gate.direction_shortfall_max"
)
rank_tolerance = number("execution.scientific_gate.rank_shortfall_max")
rmse_tolerance = number("execution.scientific_gate.rmse_excess_max")
deployment_tolerance = number(
    "execution.deployment_gate.prediction_max_abs_delta"
)
if direction_tolerance != 1.0e-2 or rank_tolerance != 1.0e-2:
    raise SystemExit("unexpected accuracy shortfall tolerance")
if rmse_tolerance != 5.0e-4 or deployment_tolerance != 2.0e-5:
    raise SystemExit("unexpected RMSE or deployment tolerance")

gate_references = {
    "arm.R1_adam": "reference.R0_retained_analytic",
    "arm.R1_lbfgs": "reference.R0_retained_analytic",
    "arm.R2_ordinary_lbfgs": "reference.full_analytic",
    "arm.R2_damped_lbfgs": "reference.full_analytic",
}
optimizer_references = {
    "arm.R1_adam": "reference.R1_materialized_f32_analytic",
    "arm.R1_lbfgs": "reference.R1_materialized_f32_analytic",
    "arm.R2_ordinary_lbfgs": "reference.R2_ordinary_materialized_f32_analytic",
    "arm.R2_damped_lbfgs": "reference.R2_damped_materialized_f32_analytic",
}

def require_close(key, expected, rel_tol=1.0e-12, abs_tol=5.0e-15):
    actual = number(key)
    if not math.isclose(actual, expected, rel_tol=rel_tol, abs_tol=abs_tol):
        raise SystemExit(f"{key}: expected recomputed {expected}, got {actual}")

def validate_comparison(prefix):
    maximum = number(prefix + ".maximum_abs_delta")
    mean = number(prefix + ".mean_abs_delta")
    equal = boolean(prefix + ".torch_equal")
    if maximum < 0 or mean < 0 or mean > maximum + 1.0e-15:
        raise SystemExit(f"{prefix}: invalid pointwise comparison")
    if equal != (maximum == 0.0):
        raise SystemExit(f"{prefix}: equality flag disagrees with max delta")
    return maximum

def validate_gate(candidate, reference, prefix):
    direction_shortfall = max(
        number(reference + ".validation.directional_accuracy")
        - number(candidate + ".validation.directional_accuracy"),
        0.0,
    )
    rank_shortfall = max(
        number(reference + ".validation.pairwise_rank_accuracy")
        - number(candidate + ".validation.pairwise_rank_accuracy"),
        0.0,
    )
    rmse_excess = max(
        number(candidate + ".validation.rmse")
        - number(reference + ".validation.rmse"),
        0.0,
    )
    require_close(prefix + ".direction_shortfall", direction_shortfall)
    require_close(prefix + ".rank_shortfall", rank_shortfall)
    require_close(prefix + ".rmse_excess", rmse_excess)
    expected_scientific_pass = (
        direction_shortfall <= direction_tolerance
        and rank_shortfall <= rank_tolerance
        and rmse_excess <= rmse_tolerance
    )
    if boolean(prefix + ".pass") != expected_scientific_pass:
        raise SystemExit(f"{prefix}: gate pass does not recompute")

def validate_deployment(prefix, delta_prefix):
    tolerance = number(prefix + ".max_abs_tolerance")
    if tolerance != deployment_tolerance:
        raise SystemExit(f"{prefix}: deployment tolerance drift")
    maximum = validate_comparison(delta_prefix)
    if boolean(prefix + ".pass") != (maximum <= deployment_tolerance):
        raise SystemExit(f"{prefix}: deployment pass does not recompute")

for reference in references:
    data_mse = number(reference + ".objective.data_mse")
    ridge_penalty = number(reference + ".objective.ridge_penalty")
    if data_mse < 0 or ridge_penalty < 0:
        raise SystemExit(f"{reference}: negative objective component")
    require_close(reference + ".objective.total", data_mse + ridge_penalty)
    exact(reference + ".objective.evaluation_dtype", "float64")
    exact(reference + ".runtime_prediction.dtype", "float32")
    for suffix in (
        ".runtime_prediction.quantization_max_abs",
        ".runtime_prediction.quantization_mean_abs",
        ".optimizer_surface_kkt.l2_norm",
        ".optimizer_surface_kkt.max_abs",
        ".full_canonical_surface_kkt.l2_norm",
        ".full_canonical_surface_kkt.max_abs",
        ".mapped_weight_l2_norm",
    ):
        if number(reference + suffix) < 0:
            raise SystemExit(f"{reference + suffix}: negative diagnostic")
    validate_deployment(
        reference + ".deployment_parity",
        reference + ".deployment_parity.direct_coordinate_to_mapped_delta",
    )

exact(
    "reference.R0_retained_analytic.operational_gate.reference",
    "reference.full_analytic",
)
validate_gate(
    "reference.R0_retained_analytic",
    "reference.full_analytic",
    "reference.R0_retained_analytic.operational_gate",
)
validate_comparison(
    "reference.R0_retained_analytic.operational_gate.validation_prediction_delta"
)

for arm in arms:
    gate_reference = gate_references[arm]
    optimizer_reference = optimizer_references[arm]
    exact(arm + ".scientific_gate.reference", gate_reference)
    exact(
        arm + ".optimizer_surface_relative_objective_gap.reference",
        optimizer_reference,
    )
    exact(arm + ".analytic_prediction_delta.reference", optimizer_reference)
    validate_gate(arm, gate_reference, arm + ".scientific_gate")

    optimizer = values.get(arm + ".optimizer")
    configured_steps = integer(arm + ".configured_steps")
    iterations = integer(arm + ".iterations")
    closures = integer(arm + ".closure_evaluations")
    if arm == "arm.R1_adam":
        if optimizer != "adam" or configured_steps != 3500:
            raise SystemExit("R1 Adam optimizer contract drift")
        if iterations != 3500 or closures != 0:
            raise SystemExit("R1 Adam execution counts drift")
    else:
        if optimizer != "lbfgs_strong_wolfe" or configured_steps != 1:
            raise SystemExit(f"{arm}: LBFGS optimizer contract drift")
        if not 0 <= iterations <= 500 or not 1 <= closures <= 625:
            raise SystemExit(f"{arm}: LBFGS execution counts outside contract")
    if number(arm + ".objective.initial") < 0:
        raise SystemExit(f"{arm}: negative initial objective")

    cuda_data = number(arm + ".objective.cuda_f32.data_mse")
    cuda_ridge = number(arm + ".objective.cuda_f32.ridge_penalty")
    cuda_total = number(arm + ".objective.cuda_f32.total")
    cpu_data = number(arm + ".objective.cpu_f64_promoted_parameters.data_mse")
    cpu_ridge = number(
        arm + ".objective.cpu_f64_promoted_parameters.ridge_penalty"
    )
    cpu_total = number(arm + ".objective.cpu_f64_promoted_parameters.total")
    if min(cuda_data, cuda_ridge, cpu_data, cpu_ridge) < 0:
        raise SystemExit(f"{arm}: negative objective component")
    require_close(
        arm + ".objective.cuda_f32.total",
        cuda_data + cuda_ridge,
        rel_tol=2.0e-6,
        abs_tol=2.0e-9,
    )
    require_close(
        arm + ".objective.cpu_f64_promoted_parameters.total",
        cpu_data + cpu_ridge,
    )
    require_close(
        arm + ".objective.cuda_f32_to_cpu_f64_promoted_delta",
        cuda_total - cpu_total,
    )
    optimizer_objective = number(optimizer_reference + ".objective.total")
    expected_gap = (cpu_total - optimizer_objective) / max(
        abs(optimizer_objective), 1.0e-30
    )
    require_close(arm + ".optimizer_surface_relative_objective_gap", expected_gap)

    for suffix in (
        ".optimizer_surface_kkt.initial_cuda_f32_autograd_l2_norm",
        ".optimizer_surface_kkt.final_cuda_f32_autograd_l2_norm",
        ".full_canonical_surface_kkt.l2_norm",
        ".full_canonical_surface_kkt.max_abs",
        ".parameter_update_l2_norm",
        ".mapped_weight_l2_norm",
        ".mapped_intercept_cancellation_max_abs",
        ".mapped_intercept_cancellation_ratio_max",
    ):
        if number(arm + suffix) < 0:
            raise SystemExit(f"{arm + suffix}: negative diagnostic")
    validate_comparison(arm + ".analytic_prediction_delta")
    validate_comparison(arm + ".R0_prediction_delta")
    validate_deployment(
        arm + ".deployment_parity",
        arm + ".deployment_parity.prediction_delta",
    )

validate_comparison(validate_comparison_key)

reference_delta_pairs = {
    "reference_delta.R1_materialized_to_R0": (
        "reference.R1_materialized_f32_analytic",
        "reference.R0_retained_analytic",
    ),
    "reference_delta.R2_ordinary_materialized_to_full": (
        "reference.R2_ordinary_materialized_f32_analytic",
        "reference.full_analytic",
    ),
    "reference_delta.R2_damped_materialized_to_full": (
        "reference.R2_damped_materialized_f32_analytic",
        "reference.full_analytic",
    ),
}
for prefix, (lhs, rhs) in reference_delta_pairs.items():
    for scope_name, scope_value in (
        ("development", "[0,730)"),
        ("validation", "[584,730)"),
    ):
        scoped = prefix + "." + scope_name
        exact(scoped + ".lhs", lhs)
        exact(scoped + ".rhs", rhs)
        exact(scoped + ".scope", scope_value)
        maximum = number(scoped + ".maximum_abs_delta")
        mean = number(scoped + ".mean_abs_delta")
        rmse = number(scoped + ".rmse_delta")
        equal = boolean(scoped + ".torch_equal")
        if min(maximum, mean, rmse) < 0 or mean > maximum or rmse > maximum:
            raise SystemExit(f"{scoped}: invalid materialization delta")
        if equal != (maximum == 0.0):
            raise SystemExit(f"{scoped}: equality flag disagrees with max delta")

for edge in range(3):
    prefix = f"conditioning.edge.{edge}"
    lambda_max = number(prefix + ".lambda_max")
    tau = number(prefix + ".tau")
    if lambda_max < 0:
        raise SystemExit(f"{prefix}: negative lambda_max")
    expected_tau = max(1.0e-12, 1.0e-10 * lambda_max)
    if not math.isclose(tau, expected_tau, rel_tol=1.0e-12, abs_tol=5.0e-15):
        raise SystemExit(f"{prefix}: tau does not recompute")
    retained_rank = integer(prefix + ".retained_rank")
    numerical_rank = integer(prefix + ".numerical_rank")
    counts = [
        integer(prefix + ".eigenband.le_alpha.count"),
        integer(prefix + ".eigenband.alpha_to_tau.count"),
        integer(prefix + ".eigenband.gt_tau.count"),
    ]
    if sum(counts) != 384 or counts[2] != retained_rank:
        raise SystemExit(f"{prefix}: eigenband partition drift")
    if not 0 <= retained_rank <= 384 or not 0 <= numerical_rank <= 384:
        raise SystemExit(f"{prefix}: rank outside [0,384]")
    for suffix in (
        ".svd_rank_tolerance",
        ".basis_orthogonality_max_abs_error",
        ".retained_whitened_covariance_max_abs_error",
        ".transform_reconstruction_max_abs_error",
        ".preconditioned_hessian_max_abs_error",
    ):
        if number(prefix + suffix) < 0:
            raise SystemExit(f"{prefix + suffix}: negative diagnostic")
    for band in ("le_alpha", "alpha_to_tau", "gt_tau"):
        for suffix in (
            ".effective_dof",
            ".target_coupling_energy",
            ".objective_reduction_contribution",
        ):
            if number(prefix + ".eigenband." + band + suffix) < 0:
                raise SystemExit(f"{prefix}: negative eigenband diagnostic")

for key, value in values.items():
    if key.endswith(".sha256") and re.fullmatch(r"[0-9a-f]{64}", value) is None:
        raise SystemExit(f"{key}: not a lowercase SHA-256")
    if key.endswith(".pass") and value not in {"true", "false"}:
        raise SystemExit(f"{key}: pass field is not boolean")
    if key.endswith("digest") and key != "external_sidecar.semantic_tensor_digest":
        if re.fullmatch(r"[0-9a-f]{64}", value) is None:
            raise SystemExit(f"{key}: not a lowercase 64-hex digest")

keyset = "".join(f"{key}\n" for key in sorted(values)).encode()
actual_probe_sha = hashlib.sha256(raw).hexdigest()
actual_keyset_sha = hashlib.sha256(keyset).hexdigest()
actual_key_count = len(values)
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

print(
    "validated conditioning probe: "
    f"keys={actual_key_count} sha256={actual_probe_sha} "
    f"keyset_sha256={actual_keyset_sha} max_anchor=729"
)
PY
}

validate_scratch_pair() {
  local first_probe="$1"
  local first_log="$2"
  local second_probe="$3"
  local second_log="$4"
  pins_are_unpinned ||
    fail "scratch validation mode requires all result pins to be UNPINNED"
  require_file "${first_log}"
  require_file "${second_log}"
  validate_result_probe "${first_probe}" true
  validate_result_probe "${second_probe}" true
  cmp -s "${first_probe}" "${second_probe}" ||
    fail "scratch conditioning probes are not byte-identical"
  cmp -s "${first_log}" "${second_log}" ||
    fail "scratch conditioning stdout logs are not byte-identical"
  [[ ! -s "${first_log}" && ! -s "${second_log}" ]] ||
    fail "successful scratch runs must not emit stdout or stderr"
  python3 - "${first_probe}" <<'PY'
import hashlib
import re
import sys

raw = open(sys.argv[1], "rb").read()
keys = []
for line in raw.decode("utf-8").splitlines():
    if not re.fullmatch(r"[A-Za-z0-9_.]+=.*", line):
        raise SystemExit("cannot fingerprint malformed scratch probe")
    keys.append(line.split("=", 1)[0])
keyset = "".join(f"{key}\n" for key in sorted(keys)).encode()
print(f"expected_result_probe_sha={hashlib.sha256(raw).hexdigest()}")
print(f"expected_result_keyset_sha={hashlib.sha256(keyset).hexdigest()}")
print(f"expected_result_key_count={len(keys)}")
PY
}

verify_replay_contract() {
  validate_result_probe "${main_probe}" false
  validate_result_probe "${replay_probe}" false
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay conditioning probes are not byte-identical"
  cmp -s "${main_log}" "${replay_log}" ||
    fail "main/replay conditioning stdout logs are not byte-identical"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful conditioning runs must not emit stdout or stderr"
}

seal_outputs() {
  (
    cd "${runtime_dir}"
    local shared=(
      bin/mdn_frozen_affine_conditioning_parity
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
    sha256sum "${shared[@]}" \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      > main/pair.sha256
    sha256sum "${shared[@]}" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > replay/pair.sha256
    sha256sum \
      main/pair.sha256 \
      replay/pair.sha256 \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      > outputs.sha256
    sha256sum \
      frozen_sources/mdn_frozen_affine_conditioning_parity.cpp \
      "frozen_sources/${included_helper_name}" \
      frozen_sources/run_mdn_frozen_affine_conditioning_parity.sh \
      "frozen_include/${calibration_header_rel}" \
      "frozen_include/${digest_header_rel}" \
      bin/mdn_frozen_affine_conditioning_parity \
      frozen_sources.sha256 \
      public_headers.sha256 \
      binary.sha256 \
      compiler.sha256 \
      execution_inputs.sha256 \
      outputs.sha256 \
      main/pair.sha256 \
      replay/pair.sha256 \
      > frozen_evidence.sha256
    sha256sum \
      frozen_evidence.sha256 \
      capture_provenance.status \
      execution_inputs.sha256 \
      execution_contract.status \
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
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
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
    "${frozen_source_dir}/mdn_frozen_affine_conditioning_parity.cpp" \
    "${expected_helper_source_sha}"
  require_sha256 \
    "${frozen_source_dir}/${included_helper_name}" \
    "${expected_included_helper_sha}"
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
    fail "another conditioning installation holds ${lock_dir}"
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
    [[ "$#" == 1 ]] || fail "--preflight takes no arguments"
    validate_pin_state
    preflight
    ;;
  --validate-scratch)
    [[ "$#" == 5 ]] ||
      fail "usage: $0 --validate-scratch MAIN_PROBE MAIN_LOG REPLAY_PROBE REPLAY_LOG"
    validate_scratch_pair "$2" "$3" "$4" "$5"
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
    fail "usage: $0 [--preflight|--run|--verify|--validate-scratch MAIN_PROBE MAIN_LOG REPLAY_PROBE REPLAY_LOG]"
    ;;
esac
