#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_conditioned_affine_shadow_readout_v1"
final_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"
scratch_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}.scratch"

helper_name="mdn_frozen_conditioned_affine_shadow_readout.cpp"
runner_name="run_mdn_frozen_conditioned_affine_shadow_readout.sh"
objective_helper_name="mdn_frozen_affine_objective_ladder.cpp"
helper_src="${script_dir}/${helper_name}"

conditioning_schema="synthetic_mdn_frozen_affine_conditioning_parity_v1"
conditioning_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${conditioning_schema}"
objective_helper_src="${conditioning_dir}/frozen_sources/${objective_helper_name}"

development_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
development_input="${development_dir}/mdn_edge_context_features.probe"
capture_report="${development_dir}/channel_inference.report"

representation_training_report="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report"
representation_checkpoint="${representation_training_report}.mtf_jepa_mae_vicreg.pt"
mdn_training_report="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report"
mdn_checkpoint="${mdn_training_report}.channel_mdn.pt"

deployment_schema="synthetic_mdn_frozen_affine_deployment_bridge_v1"
deployment_runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${deployment_schema}"
deployment_dir="${deployment_runtime_dir}/main"
v2_artifact="${deployment_dir}/${deployment_schema}.pt"
deployment_probe="${deployment_dir}/${deployment_schema}.probe"
deployment_master="${deployment_runtime_dir}/master.sha256"
deployment_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${deployment_schema}.report"
deployment_report_and_master="${deployment_runtime_dir}/report_and_master.sha256"

expected_development_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_capture_report_sha="a584e1326c264429eb4c01c24e1794c8d6f4dbe54bc5db1a627cb42ffe211441"
expected_representation_training_report_sha="a1ef97f5850137e42d7e5623e3c44d0fffe4d55bf796429bdb289859183ff4f9"
expected_mdn_training_report_sha="59db5a98b7f74ce343aab755e7a3d102a48dfdd0e8498028c5b89995ffb36b6f"
expected_representation_checkpoint_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_checkpoint_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_v2_artifact_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"
expected_deployment_probe_sha="00a382d060d20d7a8277d9f120fdc6578c572072264847157424c8a09207d3fb"
expected_deployment_master_sha="467c69c9a05035c9b58f26d90e29df127fdc11dec36f4e7deed8c7b905b23a91"
expected_deployment_report_sha="a49b802f8dc7e21eb6c5c4874aa57b65d7b6c60fec87932fca8cc5c032f8764c"
expected_deployment_report_and_master_sha="133c723bea8f79b7df9531a05673bd0e25798e76d00032bdfcb2e2a98d579292"

expected_graph_order_fingerprint="d334e38b1887ae16"
expected_readout_digest="1f0594e4aecfa1727f0231563b9e5032357a2d8dada9eeb30cd1b4a309b44362"
expected_v2_semantic_digest="84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38"
expected_v2_cpu_prediction_digest="c2321514fa55fcc9e3fe17c3954105310b44f7c792f7a8e88f5252242cc52c7f"
expected_v2_cuda_prediction_digest="1f4e343b99bd2ce7d64584100596fea54e474ec2a5fb718d6b873bbbe7c0c12e"
expected_conditioning_probe_sha="1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1"
expected_conditioning_transform_sha="3509385d6ed3c35678a511d29f441ea0ca9d77512a96b510aaa7a9757da0570c"

# The helper and result contract were established by two byte-identical fresh
# scratch runs. Canonical execution is still independently hard-disabled below
# until the runner itself receives a parent audit.
expected_helper_source_sha="cdf508e5127b1cbc0467f6b9529967f871ee660e338399c7bbec5c7a6e3ac7ec"
expected_result_probe_sha="d36a9cb6ad2fcfd4760aa18028555833f0a6410e10d215ae0e6781ca9cf44a7f"
expected_result_ordered_keyset_sha="ee88499a35e2773800c01eb2a7362e1cfa086933e473aac96614a202d8a22b2f"
expected_result_sorted_keyset_sha="9449daf71dd9882303158a529710e85af472454bdc5f3396226d68407705444e"
expected_result_key_count="239"
canonical_gate="ENABLED_AFTER_PARENT_AUDIT"

expected_objective_helper_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"

channel_context_rel="wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
channel_train_rel="wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
backbone_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_backbone.h"
head_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
loss_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_loss.h"
types_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
utils_rel="wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
conditioned_head_rel="wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"
affine_head_rel="wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"
digest_rel="piaabo/digest/sha256.h"

channel_context_header="${repo_root}/src/include/${channel_context_rel}"
channel_train_header="${repo_root}/src/include/${channel_train_rel}"
backbone_header="${repo_root}/src/include/${backbone_rel}"
head_header="${repo_root}/src/include/${head_rel}"
loss_header="${repo_root}/src/include/${loss_rel}"
types_header="${repo_root}/src/include/${types_rel}"
utils_header="${repo_root}/src/include/${utils_rel}"
conditioned_head_header="${repo_root}/src/include/${conditioned_head_rel}"
affine_head_header="${conditioning_dir}/frozen_include/${affine_head_rel}"
digest_header="${conditioning_dir}/frozen_include/${digest_rel}"

expected_channel_context_sha="ea6f0e0914c0d325e2aead90ab58a8b60e95c9063093bf871c0a3266bd15de16"
expected_channel_train_sha="eaff3ae02bbcd5c134c758c43fc2eba41d18f1ee87c22a6db06a936ee54c2df2"
expected_backbone_sha="6e7d5d1fd85e7e1b73064e55c0720f7f0f007c953f7c9545b0cafb2db081a47c"
expected_head_sha="62e6bc63cdeefd7cd80a8242268338a4b79cc828be34f56b0baba6964e5811bf"
expected_loss_sha="d23806f5804adb9e8ac131332997bcdd1e597dd253a569aeb4d39125ccb9b36a"
expected_types_sha="5e097a8726b7c6fb17ea1152876dbaede29916b6cdfb8edfec06ba9fd8e9aeef"
expected_utils_sha="4c17af66f8f673db4d7fb79fc6756793622569c2a7e5b8b7cf535e0678242e4a"
expected_conditioned_head_sha="69d633d604318fba16813f48bd539465be7e58c13e5d517cfc32b04f8c1599de"
expected_affine_head_sha="44a366fd3e044f5e5b80eebdf7e461f78bc1c3d4eb03d647dfb2343876ca7d55"
expected_digest_header_sha="3efdf247d7ffa2a3cc89bdf541caef82dee6059ea78c55f32adbde0cdeb110d3"

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  helper_bin="${build_dir}/mdn_frozen_conditioned_affine_shadow_readout"
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
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked required file: $1"
}

require_nonempty_file() {
  require_file "$1"
  [[ -s "$1" ]] || fail "required file is empty: $1"
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

require_source_token() {
  grep -Fq -- "$1" "${helper_src}" ||
    fail "helper is missing required source token: $1"
}

reject_source_token() {
  if grep -Fq -- "$1" "${helper_src}"; then
    fail "helper contains forbidden source-boundary token: $1"
  fi
}

probe_value() {
  local path="$1"
  local key="$2"
  local count
  count="$(awk -F= -v key="${key}" '$1 == key {n += 1} END {print n + 0}' "${path}")"
  [[ "${count}" == "1" ]] ||
    fail "expected exactly one ${key}= entry in ${path}, found ${count}"
  awk -F= -v key="${key}" '$1 == key {sub(/^[^=]*=/, ""); print; exit}' "${path}"
}

expect_value() {
  local actual
  actual="$(probe_value "$1" "$2")"
  [[ "${actual}" == "$3" ]] ||
    fail "unexpected $2 in $1: expected $3, got ${actual}"
}

result_pins_ready() {
  [[ "${expected_result_probe_sha}" =~ ^[0-9a-f]{64}$ &&
     "${expected_result_ordered_keyset_sha}" =~ ^[0-9a-f]{64}$ &&
     "${expected_result_sorted_keyset_sha}" =~ ^[0-9a-f]{64}$ &&
     "${expected_result_key_count}" =~ ^[1-9][0-9]*$ ]]
}

require_result_pins() {
  result_pins_ready || fail "result contract pins are unresolved"
}

require_canonical_gate() {
  require_result_pins
  [[ "${expected_helper_source_sha}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "helper source pin is unresolved"
  [[ "${canonical_gate}" == "ENABLED_AFTER_PARENT_AUDIT" ]] ||
    fail "canonical execution is hard-disabled pending parent audit"
}

preflight_inputs() {
  require_sha256 "${development_input}" "${expected_development_sha}"
  require_sha256 "${capture_report}" "${expected_capture_report_sha}"
  require_sha256 "${representation_training_report}" \
    "${expected_representation_training_report_sha}"
  require_sha256 "${mdn_training_report}" \
    "${expected_mdn_training_report_sha}"
  require_sha256 "${representation_checkpoint}" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_checkpoint_sha}"
  require_sha256 "${v2_artifact}" "${expected_v2_artifact_sha}"
  require_sha256 "${deployment_probe}" "${expected_deployment_probe_sha}"
  require_sha256 "${deployment_master}" "${expected_deployment_master_sha}"
  require_sha256 "${deployment_report}" "${expected_deployment_report_sha}"
  require_sha256 "${deployment_report_and_master}" \
    "${expected_deployment_report_and_master_sha}"
  (
    cd "${deployment_runtime_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
  (
    cd "${repo_root}"
    sha256sum -c "${deployment_report_and_master}" >/dev/null
  )
}

preflight_source_closure() {
  python3 - \
    "${helper_src}" \
    "${objective_helper_src}" \
    "${channel_context_header}" \
    "${channel_train_header}" \
    "${backbone_header}" \
    "${head_header}" \
    "${loss_header}" \
    "${types_header}" \
    "${utils_header}" \
    "${conditioned_head_header}" \
    "${affine_head_header}" \
    "${digest_header}" <<'PY'
import re
import sys

expected = {
    "mdn_frozen_affine_objective_ladder.cpp",
    "wikimyei/inference/expected_value/mdn/channel_context_mdn.h",
    "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h",
    "wikimyei/inference/expected_value/mdn/mixture_density_network_backbone.h",
    "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h",
    "wikimyei/inference/expected_value/mdn/mixture_density_network_loss.h",
    "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h",
    "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h",
    "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h",
    "wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h",
    "piaabo/digest/sha256.h",
}
actual = set()
for path in sys.argv[1:]:
    text = open(path, encoding="utf-8").read()
    actual.update(re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.MULTILINE))
if actual != expected:
    raise SystemExit(
        "quoted include closure drift: "
        f"missing={sorted(expected - actual)} extra={sorted(actual - expected)}"
    )
PY
}

preflight_sources() {
  require_sha256 "${helper_src}" "${expected_helper_source_sha}"
  require_sha256 "${objective_helper_src}" "${expected_objective_helper_sha}"
  require_sha256 "${channel_context_header}" "${expected_channel_context_sha}"
  require_sha256 "${channel_train_header}" "${expected_channel_train_sha}"
  require_sha256 "${backbone_header}" "${expected_backbone_sha}"
  require_sha256 "${head_header}" "${expected_head_sha}"
  require_sha256 "${loss_header}" "${expected_loss_sha}"
  require_sha256 "${types_header}" "${expected_types_sha}"
  require_sha256 "${utils_header}" "${expected_utils_sha}"
  require_sha256 "${conditioned_head_header}" "${expected_conditioned_head_sha}"
  require_sha256 "${affine_head_header}" "${expected_affine_head_sha}"
  require_sha256 "${digest_header}" "${expected_digest_header_sha}"
  for token in \
    '--development-input' \
    '--capture-report' \
    '--representation-training-report' \
    '--mdn-training-report' \
    '--representation-checkpoint' \
    '--mdn-checkpoint' \
    '--v2-artifact' \
    '--deployment-probe' \
    '--output' \
    '--schema-id' \
    '--evaluation-input' \
    '--historical-input' \
    '--history-input' \
    '--holdout-input' \
    '--policy-input' \
    '--refit-input' \
    'forbidden non-development shadow-readout input:' \
    '#include "mdn_frozen_affine_objective_ladder.cpp"' \
    '#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"'; do
    require_source_token "${token}"
  done
  reject_source_token '/jobs/run/'
  preflight_source_closure
  bash -n "${BASH_SOURCE[0]}"
}

preflight() {
  preflight_inputs
  preflight_sources
}

copy_frozen_header() {
  local source="$1"
  local relative="$2"
  local destination="${frozen_include_root}/${relative}"
  mkdir -p "$(dirname "${destination}")"
  cp -- "${source}" "${destination}"
  cmp -s "${source}" "${destination}" ||
    fail "frozen header copy drifted: ${relative}"
}

freeze_sources() {
  mkdir -p "${frozen_source_dir}" "${frozen_include_root}"
  cp -- "${helper_src}" "${frozen_source_dir}/${helper_name}"
  cp -- "${objective_helper_src}" "${frozen_source_dir}/${objective_helper_name}"
  cp -- "${BASH_SOURCE[0]}" "${frozen_source_dir}/${runner_name}"
  copy_frozen_header "${channel_context_header}" "${channel_context_rel}"
  copy_frozen_header "${channel_train_header}" "${channel_train_rel}"
  copy_frozen_header "${backbone_header}" "${backbone_rel}"
  copy_frozen_header "${head_header}" "${head_rel}"
  copy_frozen_header "${loss_header}" "${loss_rel}"
  copy_frozen_header "${types_header}" "${types_rel}"
  copy_frozen_header "${utils_header}" "${utils_rel}"
  copy_frozen_header "${conditioned_head_header}" "${conditioned_head_rel}"
  copy_frozen_header "${affine_head_header}" "${affine_head_rel}"
  copy_frozen_header "${digest_header}" "${digest_rel}"
  cmp -s "${helper_src}" "${frozen_source_dir}/${helper_name}" ||
    fail "frozen helper copy drifted"
  cmp -s "${objective_helper_src}" \
    "${frozen_source_dir}/${objective_helper_name}" ||
    fail "frozen objective-helper copy drifted"
  cmp -s "${BASH_SOURCE[0]}" "${frozen_source_dir}/${runner_name}" ||
    fail "frozen runner copy drifted"

  require_sha256 "${frozen_source_dir}/${helper_name}" \
    "${expected_helper_source_sha}"
  require_sha256 "${frozen_source_dir}/${objective_helper_name}" \
    "${expected_objective_helper_sha}"
  require_sha256 "${frozen_include_root}/${channel_context_rel}" \
    "${expected_channel_context_sha}"
  require_sha256 "${frozen_include_root}/${channel_train_rel}" \
    "${expected_channel_train_sha}"
  require_sha256 "${frozen_include_root}/${backbone_rel}" "${expected_backbone_sha}"
  require_sha256 "${frozen_include_root}/${head_rel}" "${expected_head_sha}"
  require_sha256 "${frozen_include_root}/${loss_rel}" "${expected_loss_sha}"
  require_sha256 "${frozen_include_root}/${types_rel}" "${expected_types_sha}"
  require_sha256 "${frozen_include_root}/${utils_rel}" "${expected_utils_sha}"
  require_sha256 "${frozen_include_root}/${conditioned_head_rel}" \
    "${expected_conditioned_head_sha}"
  require_sha256 "${frozen_include_root}/${affine_head_rel}" \
    "${expected_affine_head_sha}"
  require_sha256 "${frozen_include_root}/${digest_rel}" \
    "${expected_digest_header_sha}"

  (
    cd "${runtime_dir}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${objective_helper_name}" \
      "frozen_sources/${runner_name}" \
      > frozen_sources.sha256
    sha256sum \
      "frozen_include/${channel_context_rel}" \
      "frozen_include/${channel_train_rel}" \
      "frozen_include/${backbone_rel}" \
      "frozen_include/${head_rel}" \
      "frozen_include/${loss_rel}" \
      "frozen_include/${types_rel}" \
      "frozen_include/${utils_rel}" \
      "frozen_include/${conditioned_head_rel}" \
      "frozen_include/${affine_head_rel}" \
      "frozen_include/${digest_rel}" \
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
  local execution_scope="$1"
  local compiler_path
  compiler_path="$(readlink -f "$(command -v g++)")"

  {
    echo "schema_id=${schema_id}"
    echo "scope=${execution_scope}"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "policy_path_used=false"
    echo "fit_performed=false"
    echo "refit_performed=false"
    echo "history_input_used=false"
    echo "holdout_input_used=false"
    echo "development_anchor_range=[0,730)"
    echo "fit_anchor_range=[0,554)"
    echo "purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
  } > "${runtime_dir}/execution_scope.status"

  sha256sum \
    "${development_input}" \
    "${capture_report}" \
    "${representation_training_report}" \
    "${mdn_training_report}" \
    "${representation_checkpoint}" \
    "${mdn_checkpoint}" \
    "${v2_artifact}" \
    "${deployment_probe}" \
    > "${runtime_dir}/execution_inputs.sha256"
  [[ "$(wc -l < "${runtime_dir}/execution_inputs.sha256")" == "8" ]] ||
    fail "execution-input manifest must contain exactly eight files"
  sha256sum \
    "${deployment_master}" \
    "${deployment_report}" \
    "${deployment_report_and_master}" \
    > "${runtime_dir}/upstream_seals.sha256"
  [[ "$(wc -l < "${runtime_dir}/upstream_seals.sha256")" == "3" ]] ||
    fail "upstream-seal manifest must contain exactly three files"

  {
    echo "development.sha256=${expected_development_sha}"
    echo "capture_report.sha256=${expected_capture_report_sha}"
    echo "representation_training_report.sha256=${expected_representation_training_report_sha}"
    echo "mdn_training_report.sha256=${expected_mdn_training_report_sha}"
    echo "representation_checkpoint.sha256=${expected_representation_checkpoint_sha}"
    echo "mdn_checkpoint.sha256=${expected_mdn_checkpoint_sha}"
    echo "v2_artifact.sha256=${expected_v2_artifact_sha}"
    echo "deployment_probe.sha256=${expected_deployment_probe_sha}"
  } > "${runtime_dir}/input_contract.status"

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
  require_nonempty_file "${runtime_dir}/gpu.identity"

  ldd "${helper_bin}" > "${runtime_dir}/linked_libraries.txt"
  if grep -Fq 'not found' "${runtime_dir}/linked_libraries.txt"; then
    fail "compiled helper has unresolved linked libraries"
  fi
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
  require_nonempty_file "${runtime_dir}/resolved_libraries.sha256"
  (
    cd "${runtime_dir}"
    sha256sum bin/mdn_frozen_conditioned_affine_shadow_readout > binary.sha256
  )

  {
    echo "schema_id=${schema_id}"
    echo "scope=${execution_scope}"
    echo "helper_source_sha256=${expected_helper_source_sha}"
    echo "objective_helper_source_sha256=${expected_objective_helper_sha}"
    echo "runner_sha256=$(sha256sum "${frozen_source_dir}/${runner_name}" | awk '{print $1}')"
    echo "helper_binary_sha256=$(sha256sum "${helper_bin}" | awk '{print $1}')"
    echo "readout_surface=[730,3,3,400]"
    echo "dynamic_feature_prefix=[0,384)"
    echo "identity_suffix=[384,400)"
    echo "chunk_size=37"
    echo "operational_tolerance=2e-5"
    echo "cuda_device_order=PCI_BUS_ID"
    echo "cublas_workspace_config=:4096:8"
    echo "main_replay_probe_byte_identity_required=true"
    echo "main_replay_stdout_empty_required=true"
    echo "policy_path_used=false"
    echo "fit_performed=false"
    echo "refit_performed=false"
    echo "history_input_used=false"
    echo "holdout_input_used=false"
  } > "${runtime_dir}/execution_contract.status"
}

run_probe() {
  local output="$1"
  local log="$2"
  [[ ! -e "${output}" && ! -L "${output}" ]] ||
    fail "refusing to overwrite probe: ${output}"
  [[ ! -e "${log}" && ! -L "${log}" ]] ||
    fail "refusing to overwrite log: ${log}"
  CUDA_DEVICE_ORDER=PCI_BUS_ID \
  CUBLAS_WORKSPACE_CONFIG=:4096:8 \
  OMP_NUM_THREADS=1 \
  MKL_NUM_THREADS=1 \
    "${helper_bin}" \
    --development-input "${development_input}" \
    --capture-report "${capture_report}" \
    --representation-training-report "${representation_training_report}" \
    --mdn-training-report "${mdn_training_report}" \
    --representation-checkpoint "${representation_checkpoint}" \
    --mdn-checkpoint "${mdn_checkpoint}" \
    --v2-artifact "${v2_artifact}" \
    --deployment-probe "${deployment_probe}" \
    --output "${output}" \
    --schema-id "${schema_id}" \
    > "${log}" 2>&1
}

validate_result_probe() {
  local path="$1"
  local enforce_pins="$2"
  require_nonempty_file "${path}"
  if [[ "${enforce_pins}" == "true" ]]; then
    require_result_pins
  fi
  python3 - \
    "${path}" \
    "${schema_id}" \
    "${expected_development_sha}" \
    "${expected_capture_report_sha}" \
    "${expected_representation_training_report_sha}" \
    "${expected_mdn_training_report_sha}" \
    "${expected_representation_checkpoint_sha}" \
    "${expected_mdn_checkpoint_sha}" \
    "${expected_v2_artifact_sha}" \
    "${expected_deployment_probe_sha}" \
    "${expected_graph_order_fingerprint}" \
    "${expected_readout_digest}" \
    "${expected_v2_semantic_digest}" \
    "${expected_v2_cpu_prediction_digest}" \
    "${expected_v2_cuda_prediction_digest}" \
    "${expected_conditioning_probe_sha}" \
    "${expected_conditioning_transform_sha}" \
    "${expected_result_probe_sha}" \
    "${expected_result_ordered_keyset_sha}" \
    "${expected_result_sorted_keyset_sha}" \
    "${expected_result_key_count}" \
    "${enforce_pins}" <<'PY'
import hashlib
import math
import re
import struct
import sys

(
    path, schema_id, development_sha, capture_sha,
    representation_report_sha, mdn_report_sha,
    representation_checkpoint_sha, mdn_checkpoint_sha,
    v2_artifact_sha, deployment_probe_sha, graph_fingerprint,
    readout_digest, v2_semantic_digest, v2_cpu_digest, v2_cuda_digest,
    conditioning_probe_sha, conditioning_transform_sha,
    expected_probe_sha, expected_ordered_keyset_sha,
    expected_sorted_keyset_sha, expected_key_count, enforce_pins,
) = sys.argv[1:]

raw = open(path, "rb").read()
if not raw.endswith(b"\n"):
    raise SystemExit("shadow-readout probe lacks its final newline")
if b"\r" in raw or b"\x00" in raw:
    raise SystemExit("shadow-readout probe contains a forbidden byte")
try:
    text = raw.decode("utf-8")
except UnicodeDecodeError as error:
    raise SystemExit(f"shadow-readout probe is not UTF-8: {error}")

values = {}
ordered_keys = []
for number, line in enumerate(text.splitlines(), 1):
    if not re.fullmatch(r"[A-Za-z0-9_.]+=[^\x00-\x1f\x7f]*", line):
        raise SystemExit(f"malformed shadow-readout line {number}")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit(f"duplicate shadow-readout key: {key}")
    if "/" in value or "\\" in value:
        raise SystemExit(f"physical path-like value leaked through {key}")
    timing = {"elapsed", "duration", "timing", "wall_seconds",
              "milliseconds", "microseconds", "nanoseconds"}
    if any(component in timing for component in key.lower().split(".")):
        raise SystemExit(f"nondeterministic timing key in probe: {key}")
    values[key] = value
    ordered_keys.append(key)

def exact(key, expected):
    actual = values.get(key)
    if actual != expected:
        raise SystemExit(f"{key}: expected {expected!r}, got {actual!r}")

def integer(key):
    value = values.get(key)
    if value is None or re.fullmatch(r"-?[0-9]+", value) is None:
        raise SystemExit(f"{key}: missing or non-integer: {value!r}")
    return int(value)

def number(key):
    try:
        value = float(values[key])
    except (KeyError, ValueError):
        raise SystemExit(f"{key}: missing or non-numeric")
    if not math.isfinite(value):
        raise SystemExit(f"{key}: non-finite")
    return value

def sha(key, expected=None):
    value = values.get(key)
    if value is None or re.fullmatch(r"[0-9a-f]{64}", value) is None:
        raise SystemExit(f"{key}: not a lowercase SHA-256")
    if expected is not None and value != expected:
        raise SystemExit(f"{key}: expected {expected}, got {value}")
    return value

exact("schema_id", schema_id)
exact("status", "complete")
exact("result_role", "development_only_conditioned_affine_shadow_readout")
exact("diagnostic_authority", "diagnostic_only")
for key in (
    "benchmark_acceptance_authority", "policy_path_used", "fit_performed",
    "refit_performed", "history_input_used", "holdout_input_used",
):
    exact(key, "false")

for key, expected in (
    ("input.development.sha256", development_sha),
    ("input.capture_report.sha256", capture_sha),
    ("input.representation_training_report.sha256", representation_report_sha),
    ("input.mdn_training_report.sha256", mdn_report_sha),
    ("input.representation_checkpoint.sha256", representation_checkpoint_sha),
    ("input.mdn_checkpoint.sha256", mdn_checkpoint_sha),
    ("input.v2_artifact.sha256", v2_artifact_sha),
    ("input.deployment_probe.sha256", deployment_probe_sha),
):
    sha(key, expected)

exact("data.record_schema", "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1")
exact("data.readout_shape", "[730,3,3,400]")
exact("data.target_shape", "[730,3,3]")
exact("data.row_count", "6570")
sha("data.readout_digest", readout_digest)
exact("data.graph_order_fingerprint", graph_fingerprint)
exact("data.maximum_anchor_loaded", "729")
exact("data.identity_suffix_matches_checkpoint.torch_equal", "true")
if number("data.identity_suffix_matches_checkpoint.maximum_abs_delta") != 0.0:
    raise SystemExit("checkpoint identity suffix is not exactly equal")
exact("boundary.development", "[0,730)")
exact("boundary.fit", "[0,554)")
exact("boundary.purge", "[554,584)")
exact("boundary.validation", "[584,730)")
exact("boundary.unconsumed_holdout", "[1088,1170)")
exact("boundary.unconsumed_holdout_opened", "false")

exact("provenance.representation_training.anchor_range", "[0,730)")
exact("provenance.mdn_training.anchor_range", "[0,730)")
exact("provenance.representation_training.target_action", "train")
exact("provenance.representation_training.steps_completed", "3000")
exact("provenance.representation_training.optimizer_steps", "3000")
exact("provenance.representation_training.checkpoint_written", "true")
exact("provenance.mdn_training.target_action", "train")
exact("provenance.mdn_training.steps_completed", "3500")
exact("provenance.mdn_training.optimizer_steps", "3500")
exact("provenance.mdn_training.checkpoint_written", "true")
exact("provenance.report_path_bindings_match", "true")
exact("provenance.capture_report_path_bindings_match", "true")
exact("provenance.checkpoint_path_bindings_match", "true")
exact("representation_checkpoint.input_hash_verified", "true")
exact("representation_checkpoint.path_bound_to_reports", "true")
exact("representation_checkpoint.loaded_by_shadow", "false")

for key in (
    "caveat.cached_upstream_representation_trained_on_development_0_730",
    "caveat.cached_upstream_mdn_trained_on_development_0_730",
    "caveat.validation_584_730_withheld_from_v2_affine_fit",
    "caveat.results_are_descriptive", "caveat.cached_readout_only",
):
    exact(key, "true")
for key in (
    "caveat.unseen_future_generalization_measured",
    "caveat.validation_584_730_withheld_from_upstream_training",
    "caveat.policy_performance_measured",
    "caveat.representation_forward_executed",
    "caveat.mdn_backbone_and_distribution_forward_executed",
    "caveat.mdn_distribution_nll_measured",
    "caveat.end_to_end_forecast_path_executed",
):
    exact(key, "false")

exact("runtime.deterministic_algorithms", "true")
for key in ("runtime.torch_version", "runtime.cuda_runtime_version",
            "runtime.cuda_driver_version", "runtime.cuda_device_name"):
    if not values.get(key):
        raise SystemExit(f"{key}: missing or empty")
if integer("runtime.cpu_thread_count") != 1:
    raise SystemExit("runtime.cpu_thread_count must be one")
if integer("runtime.cpu_interop_thread_count") != 1:
    raise SystemExit("runtime.cpu_interop_thread_count must be one")
exact("runtime.seed", "31")
exact("execution.chunk_size", "37")
if not math.isclose(number("execution.operational_tolerance"), 2.0e-5):
    raise SystemExit("operational tolerance drift")

exact("v2.schema_version", "2")
exact("v2.run_only", "true")
exact("v2.policy_eligible", "false")
sha("v2.semantic_digest", v2_semantic_digest)
sha("v2.conditioning_probe.sha256", conditioning_probe_sha)
sha("v2.conditioning_transform.sha256", conditioning_transform_sha)
exact("v2.sealed_canonical.pass", "true")
sealed_threshold = number("v2.sealed_canonical.threshold")
sealed_delta = number("v2.sealed_canonical.maximum_abs_delta")
if sealed_threshold != 2.0e-5:
    raise SystemExit("v2 sealed-canonical threshold drift")
if sealed_delta > sealed_threshold:
    raise SystemExit("v2 sealed-canonical delta exceeds threshold")

for key in (
    "conditioned_v2.cpu.full.prediction_digest",
    "conditioned_v2.cpu.chunked.prediction_digest",
    "conditioned_v2.cpu.suffix_perturbed.prediction_digest",
):
    sha(key, v2_cpu_digest)
for key in (
    "conditioned_v2.cuda.full.prediction_digest",
    "conditioned_v2.cuda.chunked.prediction_digest",
    "conditioned_v2.cuda.suffix_perturbed.prediction_digest",
):
    sha(key, v2_cuda_digest)
for key, value in values.items():
    if key.endswith(".pass") and value != "true":
        raise SystemExit(f"required operational pass is false: {key}")
    if key.endswith((".maximum_abs_delta", ".mean_abs_delta", ".rmse_delta",
                     ".threshold")):
        number(key)

for prefix in (
    "conditioned_v2.parity.cpu_full_to_chunked",
    "conditioned_v2.parity.cuda_full_to_chunked",
    "conditioned_v2.parity.cpu_suffix_ignored",
    "conditioned_v2.parity.cuda_suffix_ignored",
):
    exact(prefix + ".torch_equal", "true")

for prefix, expected_count in (
    ("checkpoint_mlp.fit", 4986),
    ("checkpoint_mlp.validation", 1314),
    ("conditioned_v2.fit", 4986),
    ("conditioned_v2.validation", 1314),
):
    if integer(prefix + ".metric.count") != expected_count:
        raise SystemExit(f"{prefix}.metric.count drift")
    if integer(prefix + ".metric.pair_count") != expected_count:
        raise SystemExit(f"{prefix}.metric.pair_count drift")
    if integer(prefix + ".production_objective.valid_count") != expected_count:
        raise SystemExit(f"{prefix}.production_objective.valid_count drift")
    if integer(prefix + ".production_objective.pairwise_valid_count") != expected_count:
        raise SystemExit(f"{prefix}.production_objective.pairwise_valid_count drift")
    for suffix in (
        ".metric.mae", ".metric.rmse", ".metric.directional_accuracy",
        ".metric.pairwise_rank_accuracy", ".metric.correlation",
        ".metric.prediction_population_std", ".production_objective.total",
        ".production_objective.regression", ".production_objective.direction",
        ".production_objective.rank",
    ):
        number(prefix + suffix)
    regression = number(prefix + ".production_objective.regression")
    direction = number(prefix + ".production_objective.direction")
    rank = number(prefix + ".production_objective.rank")
    total = number(prefix + ".production_objective.total")

    def f32(value):
        return struct.unpack("=f", struct.pack("=f", value))[0]

    sequential_total = f32(0.0)
    sequential_total = f32(sequential_total + f32(100.0 * regression))
    sequential_total = f32(sequential_total + f32(5.0 * direction))
    sequential_total = f32(sequential_total + f32(5.0 * rank))
    if f32(total) != sequential_total:
        raise SystemExit(
            f"{prefix}.production_objective.total does not match "
            "sequential float32 weighting"
        )

exact("comparison.role", "descriptive_no_beats_mlp_gate")
for key in (
    "conclusion.provenance_pass",
    "conclusion.checkpoint_mlp_operational_parity_pass",
    "conclusion.conditioned_v2_operational_parity_pass",
    "conclusion.unconsumed_holdout_remains_sealed",
    "conclusion.shadow_replay_complete",
):
    exact(key, "true")
for key in (
    "conclusion.fit_performed", "conclusion.selection_gate_applied",
    "conclusion.beats_mlp_gate_applied",
    "conclusion.policy_performance_measured",
):
    exact(key, "false")

# Policy/history/holdout/refit/evaluation disclosures are permitted only at the
# exact boundary keys validated above and only with their expected false/true
# sealed-status values. Any additional namespace is rejected.
allowed_sensitive = {
    "policy_path_used", "history_input_used", "holdout_input_used",
    "refit_performed", "v2.policy_eligible",
    "caveat.policy_performance_measured",
    "boundary.unconsumed_holdout", "boundary.unconsumed_holdout_opened",
    "conclusion.policy_performance_measured",
    "conclusion.unconsumed_holdout_remains_sealed",
}
for key in values:
    components = set(re.split(r"[._]", key.lower()))
    if components & {"policy", "history", "holdout", "refit", "evaluation"}:
        if key not in allowed_sensitive:
            raise SystemExit(f"unexpected sensitive namespace in probe: {key}")

ordered_blob = "".join(f"{key}\n" for key in ordered_keys).encode()
sorted_blob = "".join(f"{key}\n" for key in sorted(values)).encode()
actual_probe_sha = hashlib.sha256(raw).hexdigest()
actual_ordered_sha = hashlib.sha256(ordered_blob).hexdigest()
actual_sorted_sha = hashlib.sha256(sorted_blob).hexdigest()
if enforce_pins == "true":
    if actual_probe_sha != expected_probe_sha:
        raise SystemExit(
            f"probe SHA drift: expected {expected_probe_sha}, got {actual_probe_sha}"
        )
    if actual_ordered_sha != expected_ordered_keyset_sha:
        raise SystemExit("ordered key-set SHA drift")
    if actual_sorted_sha != expected_sorted_keyset_sha:
        raise SystemExit("sorted key-set SHA drift")
    if len(values) != int(expected_key_count):
        raise SystemExit(
            f"key-count drift: expected {expected_key_count}, got {len(values)}"
        )
print(
    f"validated shadow readout: keys={len(values)} sha256={actual_probe_sha} "
    f"ordered_keyset_sha256={actual_ordered_sha} "
    f"sorted_keyset_sha256={actual_sorted_sha}"
)
PY
}

verify_replay_contract() {
  local enforce_pins="$1"
  require_file "${main_log}"
  require_file "${replay_log}"
  validate_result_probe "${main_probe}" "${enforce_pins}"
  validate_result_probe "${replay_probe}" "${enforce_pins}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay probes are not byte-identical"
  cmp -s "${main_log}" "${replay_log}" ||
    fail "main/replay stdout logs are not byte-identical"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful main/replay runs must emit no stdout or stderr"
}

emit_result_pins() {
  python3 - "${main_probe}" <<'PY'
import hashlib
import re
import sys

raw = open(sys.argv[1], "rb").read()
keys = []
for line in raw.decode("utf-8").splitlines():
    if not re.fullmatch(r"[A-Za-z0-9_.]+=.*", line):
        raise SystemExit("cannot fingerprint malformed probe")
    keys.append(line.split("=", 1)[0])
ordered = "".join(f"{key}\n" for key in keys).encode()
sorted_keys = "".join(f"{key}\n" for key in sorted(keys)).encode()
print(f"expected_result_probe_sha={hashlib.sha256(raw).hexdigest()}")
print(f"expected_result_ordered_keyset_sha={hashlib.sha256(ordered).hexdigest()}")
print(f"expected_result_sorted_keyset_sha={hashlib.sha256(sorted_keys).hexdigest()}")
print(f"expected_result_key_count={len(keys)}")
PY
}

seal_outputs() {
  (
    cd "${runtime_dir}"
    shared=(
      bin/mdn_frozen_conditioned_affine_shadow_readout
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
      execution_scope.status
      execution_inputs.sha256
      upstream_seals.sha256
      input_contract.status
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
      frozen_sources.sha256 \
      public_headers.sha256 \
      binary.sha256 \
      execution_inputs.sha256 \
      outputs.sha256 \
      main/pair.sha256 \
      replay/pair.sha256 \
      > frozen_evidence.sha256
    sha256sum \
      frozen_evidence.sha256 \
      execution_scope.status \
      execution_inputs.sha256 \
      upstream_seals.sha256 \
      input_contract.status \
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

seal_build_only() {
  (
    cd "${runtime_dir}"
    sha256sum \
      bin/mdn_frozen_conditioned_affine_shadow_readout \
      frozen_sources.sha256 \
      public_headers.sha256 \
      binary.sha256 \
      compiler.identity \
      compiler.version \
      compiler.sha256 \
      gpu.identity \
      linked_libraries.txt \
      binary.dynamic.txt \
      resolved_libraries.sha256 \
      execution_scope.status \
      execution_inputs.sha256 \
      upstream_seals.sha256 \
      input_contract.status \
      execution_contract.status \
      > build_master.sha256
  )
}

verify_common_seals() {
  (
    cd "${runtime_dir}"
    sha256sum -c execution_inputs.sha256 >/dev/null
    sha256sum -c upstream_seals.sha256 >/dev/null
    sha256sum -c compiler.sha256 >/dev/null
    sha256sum -c frozen_sources.sha256 >/dev/null
    sha256sum -c public_headers.sha256 >/dev/null
    sha256sum -c binary.sha256 >/dev/null
    sha256sum -c resolved_libraries.sha256 >/dev/null
  )
}

verify_output_seals() {
  verify_common_seals
  (
    cd "${runtime_dir}"
    sha256sum -c main/pair.sha256 >/dev/null
    sha256sum -c replay/pair.sha256 >/dev/null
    sha256sum -c outputs.sha256 >/dev/null
    sha256sum -c frozen_evidence.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
}

verify_frozen_source_pins() {
  require_sha256 "${frozen_source_dir}/${helper_name}" \
    "${expected_helper_source_sha}"
  require_sha256 "${frozen_source_dir}/${objective_helper_name}" \
    "${expected_objective_helper_sha}"
  require_sha256 "${frozen_include_root}/${channel_context_rel}" \
    "${expected_channel_context_sha}"
  require_sha256 "${frozen_include_root}/${channel_train_rel}" \
    "${expected_channel_train_sha}"
  require_sha256 "${frozen_include_root}/${backbone_rel}" "${expected_backbone_sha}"
  require_sha256 "${frozen_include_root}/${head_rel}" "${expected_head_sha}"
  require_sha256 "${frozen_include_root}/${loss_rel}" "${expected_loss_sha}"
  require_sha256 "${frozen_include_root}/${types_rel}" "${expected_types_sha}"
  require_sha256 "${frozen_include_root}/${utils_rel}" "${expected_utils_sha}"
  require_sha256 "${frozen_include_root}/${conditioned_head_rel}" \
    "${expected_conditioned_head_sha}"
  require_sha256 "${frozen_include_root}/${affine_head_rel}" \
    "${expected_affine_head_sha}"
  require_sha256 "${frozen_include_root}/${digest_rel}" \
    "${expected_digest_header_sha}"
}

allocate_scratch_dir() {
  local requested="$1"
  local name
  [[ ! -L "${scratch_root}" ]] || fail "scratch root is a symlink"
  mkdir -p "${scratch_root}"
  name="$(basename -- "${requested}")"
  [[ "${requested}" == "${name}" ]] ||
    fail "scratch name must be one safe path component"
  [[ "${name}" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] ||
    fail "invalid scratch name: ${name}"
  scratch_dir="${scratch_root}/${name}"
  [[ ! -e "${scratch_dir}" && ! -L "${scratch_dir}" ]] ||
    fail "refusing to overwrite scratch directory: ${scratch_dir}"
  mkdir "${scratch_dir}"
}

build_scratch() {
  preflight
  allocate_scratch_dir "$1"
  set_runtime_paths "${scratch_dir}"
  freeze_sources
  compile_helper
  write_provenance build_scratch
  seal_build_only
  verify_common_seals
  (
    cd "${runtime_dir}"
    sha256sum -c build_master.sha256 >/dev/null
  )
  echo "${runtime_dir}"
}

run_scratch() {
  preflight
  allocate_scratch_dir "$1"
  set_runtime_paths "${scratch_dir}"
  mkdir -p "${main_dir}" "${replay_dir}"
  freeze_sources
  compile_helper
  write_provenance scratch
  run_probe "${main_probe}" "${main_log}"
  run_probe "${replay_probe}" "${replay_log}"
  verify_replay_contract true
  seal_outputs
  verify_output_seals
  emit_result_pins
  echo "${runtime_dir}"
}

validate_scratch_pair() {
  local first_probe="$1"
  local first_log="$2"
  local second_probe="$3"
  local second_log="$4"
  require_file "${first_log}"
  require_file "${second_log}"
  validate_result_probe "${first_probe}" true
  validate_result_probe "${second_probe}" true
  cmp -s "${first_probe}" "${second_probe}" ||
    fail "scratch probes are not byte-identical"
  cmp -s "${first_log}" "${second_log}" ||
    fail "scratch stdout logs are not byte-identical"
  [[ ! -s "${first_log}" && ! -s "${second_log}" ]] ||
    fail "successful scratch runs must emit no stdout or stderr"
}

verify_complete() {
  require_canonical_gate
  set_runtime_paths "${final_runtime_dir}"
  require_file "${runtime_dir}/master.sha256"
  expect_value "${runtime_dir}/execution_scope.status" scope canonical
  verify_frozen_source_pins
  preflight_inputs
  verify_output_seals
  verify_replay_contract true
  echo "${runtime_dir}"
}

run_canonical() {
  require_canonical_gate
  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "refusing to overwrite canonical evidence directory: ${final_runtime_dir}"
  preflight

  local lock_dir="${final_runtime_dir}.lock"
  local candidate_dir="${final_runtime_dir}.candidate.$$"
  mkdir "${lock_dir}" || fail "another installation holds ${lock_dir}"
  trap 'rmdir "'"${lock_dir}"'" 2>/dev/null || true' EXIT
  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "canonical evidence appeared after lock acquisition"
  [[ ! -e "${candidate_dir}" && ! -L "${candidate_dir}" ]] ||
    fail "candidate evidence directory already exists: ${candidate_dir}"

  set_runtime_paths "${candidate_dir}"
  mkdir -p "${main_dir}" "${replay_dir}"
  freeze_sources
  compile_helper
  write_provenance canonical
  run_probe "${main_probe}" "${main_log}"
  run_probe "${replay_probe}" "${replay_log}"
  verify_replay_contract true
  seal_outputs
  verify_output_seals
  mv -T -n "${candidate_dir}" "${final_runtime_dir}" ||
    fail "canonical evidence installation failed; candidate preserved"
  [[ ! -e "${candidate_dir}" && ! -L "${candidate_dir}" &&
     -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "no-clobber canonical installation postcondition failed"
  rmdir "${lock_dir}"
  trap - EXIT
  verify_complete
}

mode="${1:-}"
case "${mode}" in
  --preflight)
    [[ "$#" == 1 ]] || fail "--preflight takes no arguments"
    preflight
    echo "helper_source_sha256=${expected_helper_source_sha}"
    echo "result_probe_sha256=${expected_result_probe_sha}"
    echo "result_key_count=${expected_result_key_count}"
    echo "canonical_gate=${canonical_gate}"
    ;;
  --build-scratch)
    [[ "$#" == 2 ]] || fail "usage: $0 --build-scratch SCRATCH_NAME"
    build_scratch "$2"
    ;;
  --scratch)
    [[ "$#" == 2 ]] || fail "usage: $0 --scratch SCRATCH_NAME"
    run_scratch "$2"
    ;;
  --validate-scratch)
    [[ "$#" == 5 ]] ||
      fail "usage: $0 --validate-scratch MAIN_PROBE MAIN_LOG REPLAY_PROBE REPLAY_LOG"
    validate_scratch_pair "$2" "$3" "$4" "$5"
    ;;
  --run)
    [[ "$#" == 1 ]] || fail "--run takes no arguments"
    run_canonical
    ;;
  --verify)
    [[ "$#" == 1 ]] || fail "--verify takes no arguments"
    verify_complete
    ;;
  *)
    fail "usage: $0 {--preflight|--build-scratch NAME|--scratch NAME|--validate-scratch MAIN_PROBE MAIN_LOG REPLAY_PROBE REPLAY_LOG|--run|--verify}"
    ;;
esac
