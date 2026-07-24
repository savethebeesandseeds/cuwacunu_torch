#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_conditioned_affine_mdn_forward_bridge_v1"
runtime_base="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
runtime_dir="${runtime_base}/${schema_id}"
lock_dir="${runtime_base}/${schema_id}.lock"
installed_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report"

helper_name="mdn_frozen_conditioned_affine_mdn_forward_bridge.cpp"
objective_name="mdn_frozen_affine_objective_ladder.cpp"
runner_name="run_mdn_frozen_conditioned_affine_mdn_forward_bridge.sh"
helper_src="${script_dir}/${helper_name}"
objective_src="${script_dir}/${objective_name}"

capture_dir="${runtime_base}/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
representation_probe="${capture_dir}/representation_edge_features.probe"
cached_mdn_probe="${capture_dir}/mdn_edge_context_features.probe"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"
conditioned_artifact="${runtime_base}/synthetic_mdn_frozen_affine_deployment_bridge_v1/main/synthetic_mdn_frozen_affine_deployment_bridge_v1.pt"
shadow_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_frozen_conditioned_affine_shadow_readout_v1.report"

expected_representation_sha="d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed"
expected_cached_mdn_sha="ac8456e707b550dd1500083c5a5fde604b7e410abfa7498db823c8d7f83e4851"
expected_mdn_checkpoint_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_conditioned_artifact_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"
expected_shadow_report_sha="c62aad32d77e0bef3ebb1b2a749624925149e2aeea46f2f1709dbbbf1615a839"
expected_objective_source_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"

fail() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_sha256() {
  local path="$1"
  local expected="$2"
  local actual
  [[ "${expected}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "invalid expected SHA-256 for ${path}"
  require_file "${path}"
  actual="$(sha256sum "${path}" | awk '{print $1}')"
  [[ "${actual}" == "${expected}" ]] ||
    fail "SHA-256 mismatch for ${path}: expected ${expected}, got ${actual}"
}

require_source_token() {
  grep -Fq -- "$1" "${helper_src}" ||
    fail "helper is missing required boundary token: $1"
}

probe_value() {
  local path="$1"
  local key="$2"
  local count
  count="$(awk -F= -v key="${key}" '$1 == key {n += 1} END {print n + 0}' "${path}")"
  [[ "${count}" == "1" ]] ||
    fail "expected one ${key}= entry in ${path}, found ${count}"
  awk -F= -v key="${key}" '$1 == key {sub(/^[^=]*=/, ""); print; exit}' "${path}"
}

expect_value() {
  local actual
  actual="$(probe_value "$1" "$2")"
  [[ "${actual}" == "$3" ]] ||
    fail "unexpected $2 in $1: expected $3, got ${actual}"
}

preflight_inputs() {
  require_sha256 "${representation_probe}" "${expected_representation_sha}"
  require_sha256 "${cached_mdn_probe}" "${expected_cached_mdn_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_checkpoint_sha}"
  require_sha256 "${conditioned_artifact}" \
    "${expected_conditioned_artifact_sha}"
  require_sha256 "${shadow_report}" "${expected_shadow_report_sha}"
}

preflight_sources() {
  require_file "${helper_src}"
  require_sha256 "${objective_src}" "${expected_objective_source_sha}"
  require_file "${script_dir}/${runner_name}"
  [[ -d "${repo_root}/src/include" ]] || fail "missing public include tree"
  if find "${repo_root}/src/include" -type l -print -quit | grep -q .; then
    fail "public include tree contains a symlink"
  fi
  require_source_token \
    'production_launcher::load_channel_mdn_checkpoint_file'
  require_source_token 'channel_mdn_checkpoint_identity_t identity'
  require_source_token 'model->direct_edge_context_features(context, mask)'
  require_source_token 'begin += 64'
  require_source_token \
    'load_per_edge_conditioned_affine_return_head'
  require_source_token 'compute_direct_edge_return_readout_loss'
  require_source_token 'boundary.anchor_730_or_later_opened=false'
  require_source_token 'execution.policy_path_used=false'
  require_source_token \
    'execution.mdn_backbone_adapters_direct_feature_forward_'
  require_source_token 'execution.representation_forward_executed=false'
  require_source_token 'execution.mdn_distribution_forward_executed=false'
  require_source_token 'execution.end_to_end_forecast_path_executed=false'
  bash -n "${script_dir}/${runner_name}"
  clang-format --dry-run --Werror "${helper_src}"
}

preflight_environment() {
  command -v g++ >/dev/null || fail "g++ is unavailable"
  command -v python3 >/dev/null || fail "python3 is unavailable"
  command -v sha256sum >/dev/null || fail "sha256sum is unavailable"
  command -v nvidia-smi >/dev/null || fail "nvidia-smi is unavailable"
  [[ -d "${repo_root}/.external/libtorch-upd/include" ]] ||
    fail "LibTorch include tree is unavailable"
  [[ -f "${repo_root}/.external/libtorch-upd/lib/libtorch_cuda.so" ]] ||
    fail "LibTorch CUDA library is unavailable"
  [[ -d /usr/local/cuda-12.4/include ]] || fail "CUDA 12.4 is unavailable"
  nvidia-smi --query-gpu=name --format=csv,noheader | head -1 | grep -q . ||
    fail "CUDA GPU identity is unavailable"
}

preflight() {
  mkdir -p "${runtime_base}"
  preflight_inputs
  preflight_sources
  preflight_environment
  echo "preflight passed: ${schema_id}"
}

set_runtime_paths() {
  local root="$1"
  frozen_sources="${root}/frozen_sources"
  frozen_include="${root}/frozen_include"
  frozen_inputs="${root}/frozen_inputs"
  build_dir="${root}/bin"
  helper_bin="${build_dir}/mdn_frozen_conditioned_affine_mdn_forward_bridge"
  main_dir="${root}/main"
  replay_dir="${root}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
}

freeze_closure() {
  local root="$1"
  set_runtime_paths "${root}"
  mkdir -p "${frozen_sources}" "${frozen_include}" "${frozen_inputs}" \
    "${build_dir}" "${main_dir}" "${replay_dir}"
  cp "${helper_src}" "${frozen_sources}/${helper_name}"
  cp "${objective_src}" "${frozen_sources}/${objective_name}"
  cp "${script_dir}/${runner_name}" "${frozen_sources}/${runner_name}"
  cp -a "${repo_root}/src/include/." "${frozen_include}/"
  cp "${representation_probe}" "${frozen_inputs}/representation.probe"
  cp "${cached_mdn_probe}" "${frozen_inputs}/cached_mdn.probe"
  cp "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt"
  cp "${conditioned_artifact}" "${frozen_inputs}/conditioned_artifact.pt"
  cp "${shadow_report}" "${frozen_inputs}/shadow.report"
  require_sha256 "${frozen_sources}/${objective_name}" \
    "${expected_objective_source_sha}"
  require_sha256 "${frozen_inputs}/representation.probe" \
    "${expected_representation_sha}"
  require_sha256 "${frozen_inputs}/cached_mdn.probe" \
    "${expected_cached_mdn_sha}"
  require_sha256 "${frozen_inputs}/mdn_checkpoint.pt" \
    "${expected_mdn_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/conditioned_artifact.pt" \
    "${expected_conditioned_artifact_sha}"
  require_sha256 "${frozen_inputs}/shadow.report" \
    "${expected_shadow_report_sha}"
  chmod -R a-w "${frozen_sources}" "${frozen_include}" "${frozen_inputs}"
  (
    cd "${root}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${objective_name}" \
      "frozen_sources/${runner_name}" > frozen_sources.sha256
    find frozen_include -type f -print0 | sort -z | xargs -0 sha256sum \
      > frozen_include.sha256
    sha256sum \
      frozen_inputs/representation.probe \
      frozen_inputs/cached_mdn.probe \
      frozen_inputs/mdn_checkpoint.pt \
      frozen_inputs/conditioned_artifact.pt \
      frozen_inputs/shadow.report > frozen_inputs.sha256
  )
}

compile_helper() {
  local root="$1"
  local libtorch="${repo_root}/.external/libtorch-upd"
  local cuda="/usr/local/cuda-12.4"
  set_runtime_paths "${root}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${frozen_sources}" \
    -I"${frozen_include}" \
    -isystem "${libtorch}/include" \
    -isystem "${libtorch}/include/torch/csrc/api/include" \
    -isystem "${cuda}/include" \
    "${frozen_sources}/${helper_name}" \
    -L"${libtorch}/lib" \
    -L"${cuda}/lib64" \
    -Wl,-rpath,"${libtorch}/lib" \
    -Wl,-rpath,"${cuda}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda \
    -Wl,--as-needed -ltorch_cpu -ltorch -lc10 \
    -lcuda -lcudart -lnvToolsExt -lstdc++ -lpthread -lm \
    -o "${helper_bin}"
  ldd "${helper_bin}" >"${root}/linked_libraries.status"
  ! grep -Fq 'not found' "${root}/linked_libraries.status" ||
    fail "compiled helper has an unresolved shared library"
  for library in libtorch_cuda.so libc10_cuda.so libcudart.so; do
    grep -Fq "${library}" "${root}/linked_libraries.status" ||
      fail "compiled helper does not resolve ${library}"
  done
}

run_helper_once() {
  local root="$1"
  local destination="$2"
  local log="$3"
  set_runtime_paths "${root}"
  env \
    CUBLAS_WORKSPACE_CONFIG=:4096:8 \
    CUDA_DEVICE_ORDER=PCI_BUS_ID \
    "${helper_bin}" \
    --representation-probe "${frozen_inputs}/representation.probe" \
    --cached-probe "${frozen_inputs}/cached_mdn.probe" \
    --mdn-checkpoint "${frozen_inputs}/mdn_checkpoint.pt" \
    --v2-artifact "${frozen_inputs}/conditioned_artifact.pt" \
    --shadow-report "${frozen_inputs}/shadow.report" \
    --schema-id "${schema_id}" \
    --output "${destination}" >"${log}" 2>&1
}

validate_probe() {
  local path="$1"
  require_file "${path}"
  python3 - "${path}" "${schema_id}" <<'PY'
import math
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
schema = sys.argv[2]
values = {}
for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
    if not raw or "=" not in raw:
        raise SystemExit(f"malformed probe line {number}")
    key, value = raw.split("=", 1)
    if not key or key in values:
        raise SystemExit(f"duplicate or empty probe key at line {number}")
    values[key] = value

expected = {
    "schema_id": schema,
    "status": "complete",
    "diagnostic_authority": "diagnostic_only",
    "benchmark_acceptance_authority": "false",
    "execution.production_checkpoint_loader_executed": "true",
    "execution.mdn_backbone_adapters_direct_feature_forward_executed": "true",
    "execution.representation_forward_executed": "false",
    "execution.mdn_distribution_forward_executed": "false",
    "execution.end_to_end_forecast_path_executed": "false",
    "execution.conditioned_artifact_loaded": "true",
    "execution.training_performed": "false",
    "execution.policy_path_used": "false",
    "training_performed": "false",
    "policy_path_used": "false",
    "history_input_used": "false",
    "holdout_input_used": "false",
    "data.max_anchor": "729",
    "boundary.anchor_730_or_later_opened": "false",
    "boundary.unconsumed_holdout_opened": "false",
    "parity.mdn_features.pass": "true",
    "parity.conditioned_predictions.pass": "true",
    "parity.cached_shadow_metrics.pass": "true",
    "parity.recomputed_shadow_metrics.pass": "true",
    "parity.production_direct_objectives.pass": "true",
    "conclusion.mdn_backbone_adapters_direct_feature_forward_executed": "true",
    "conclusion.representation_forward_executed": "false",
    "conclusion.mdn_distribution_forward_executed": "false",
    "conclusion.end_to_end_forecast_path_executed": "false",
    "conclusion.live_mdn_direct_feature_forward_bridge_valid": "true",
}
for key, wanted in expected.items():
    actual = values.get(key)
    if actual != wanted:
        raise SystemExit(f"unexpected {key}: expected {wanted!r}, got {actual!r}")

for key, value in values.items():
    if key.endswith(".pass") and value != "true":
        raise SystemExit(f"failed gated assertion: {key}={value}")

for key, value in values.items():
    if key.endswith((".maximum_abs_delta", ".mean_abs_delta")):
        parsed = float(value)
        if not math.isfinite(parsed) or parsed < 0.0:
            raise SystemExit(f"invalid finite delta: {key}={value}")

def finite_nonnegative(key):
    try:
        value = float(values[key])
    except KeyError:
        raise SystemExit(f"missing numeric gate: {key}")
    if not math.isfinite(value) or value < 0.0:
        raise SystemExit(f"invalid numeric gate: {key}={value!r}")
    return value

def require_bounded(value_key, threshold_key, maximum_threshold):
    value = finite_nonnegative(value_key)
    threshold = finite_nonnegative(threshold_key)
    if threshold > maximum_threshold or value > threshold:
        raise SystemExit(
            f"numeric gate failed: {value_key}={value} "
            f"{threshold_key}={threshold} allowed<={maximum_threshold}"
        )

require_bounded(
    "parity.recomputed_to_cached.readout.maximum_abs_delta",
    "parity.recomputed_to_cached.readout.threshold",
    1.0e-6,
)
require_bounded(
    "parity.recomputed_to_cached.prediction.maximum_abs_delta",
    "parity.recomputed_to_cached.prediction.threshold",
    1.0e-6,
)
for split in ("fit", "validation"):
    require_bounded(
        f"parity.cached_{split}_to_shadow_report.maximum_abs_delta",
        "parity.cached_shadow_metric.threshold",
        1.0e-12,
    )
    require_bounded(
        f"parity.recomputed_{split}_to_shadow_report.maximum_abs_delta",
        "parity.recomputed_shadow_metric.threshold",
        1.0e-6,
    )
    require_bounded(
        f"parity.cached_{split}_objective_to_shadow_report.maximum_abs_delta",
        "parity.cached_shadow_objective.threshold",
        1.0e-5,
    )
    require_bounded(
        f"parity.recomputed_{split}_objective_to_shadow_report.maximum_abs_delta",
        "parity.recomputed_shadow_objective.threshold",
        1.0e-3,
    )
PY
}

write_provenance() {
  local root="$1"
  set_runtime_paths "${root}"
  {
    echo "schema_id=${schema_id}"
    echo "scope=development_only_mdn_direct_feature_forward_bridge"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "training_performed=false"
    echo "policy_path_used=false"
    echo "history_input_used=false"
    echo "holdout_input_used=false"
    echo "development_anchor_range=[0,730)"
    echo "maximum_anchor_loaded=729"
    echo "unconsumed_holdout_anchor_range=[1088,1170)"
    echo "unconsumed_holdout_opened=false"
  } >"${root}/execution_scope.status"
  g++ --version >"${root}/compiler.version"
  nvidia-smi --query-gpu=name,driver_version,compute_cap \
    --format=csv,noheader >"${root}/cuda_device.status"
  (
    cd "${root}"
    sha256sum bin/mdn_frozen_conditioned_affine_mdn_forward_bridge \
      > helper_binary.sha256
  )
}

write_master_manifest() {
  local root="$1"
  (
    cd "${root}"
    find . -type f ! -name master.sha256 ! -name report_and_master.sha256 \
      -print0 | sort -z | xargs -0 sha256sum > master.sha256
  )
}

publish_directory_noreplace() {
  local source="$1"
  local destination="$2"
  [[ ! -e "${destination}" ]] ||
    fail "canonical runtime destination already exists"
  mv -T -n -- "${source}" "${destination}"
  [[ ! -e "${source}" && -d "${destination}" ]] ||
    fail "no-clobber canonical runtime publication failed"
}

verify_live_sources() {
  local root="$1"
  cmp -s "${helper_src}" "${root}/frozen_sources/${helper_name}" ||
    fail "live helper differs from sealed helper"
  cmp -s "${objective_src}" "${root}/frozen_sources/${objective_name}" ||
    fail "live objective helper differs from sealed objective helper"
  cmp -s "${script_dir}/${runner_name}" \
    "${root}/frozen_sources/${runner_name}" ||
    fail "live runner differs from sealed runner"
  diff -qr "${repo_root}/src/include" "${root}/frozen_include" >/dev/null ||
    fail "live include tree differs from sealed include tree"
}

verify_runtime() {
  require_file "${runtime_dir}/master.sha256"
  require_file "${runtime_dir}/report_and_master.sha256"
  (
    cd "${runtime_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
  (
    cd "${repo_root}"
    sha256sum -c "${runtime_dir}/report_and_master.sha256" >/dev/null
  )
  preflight_inputs
  verify_live_sources "${runtime_dir}"
  set_runtime_paths "${runtime_dir}"
  require_file "${main_probe}"
  require_file "${replay_probe}"
  require_file "${main_log}"
  require_file "${replay_log}"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful primary/replay logs must be empty"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "primary and replay probes differ"
  require_file "${installed_report}"
  cmp -s "${main_probe}" "${installed_report}" ||
    fail "installed report differs from sealed primary probe"
  validate_probe "${main_probe}"
  echo "verification passed: ${schema_id}"
}

run_canonical() {
  preflight
  if [[ -d "${runtime_dir}" || -f "${installed_report}" ]]; then
    [[ -d "${runtime_dir}" && -f "${installed_report}" ]] ||
      fail "partial canonical installation exists"
    verify_runtime
    echo "reused sealed canonical result: ${schema_id}"
    return 0
  fi
  mkdir "${lock_dir}" 2>/dev/null || fail "canonical runner lock is held"
  local scratch="${runtime_base}/${schema_id}.scratch.$$"
  local report_tmp="${installed_report}.tmp.$$"
  cleanup_scratch="${scratch}"
  cleanup_report_tmp="${report_tmp}"
  cleanup_runtime_moved=false
  cleanup_report_installed=false
  cleanup_publication_complete=false
  [[ "${scratch}" == "${runtime_base}/${schema_id}.scratch."* ]] ||
    fail "scratch path escaped the benchmark runtime root"
  [[ ! -e "${scratch}" ]] || fail "scratch path already exists: ${scratch}"
  cleanup() {
    if [[ "${cleanup_scratch:-}" == \
      "${runtime_base}/${schema_id}.scratch."* ]]; then
      rm -rf -- "${cleanup_scratch}"
    fi
    if [[ "${cleanup_report_tmp:-}" == "${installed_report}.tmp."* ]]; then
      rm -f -- "${cleanup_report_tmp}"
    fi
    if [[ "${cleanup_publication_complete:-false}" != true ]]; then
      if [[ "${cleanup_report_installed:-false}" == true ]]; then
        rm -f -- "${installed_report}"
      fi
      if [[ "${cleanup_runtime_moved:-false}" == true &&
        "${runtime_dir}" == "${runtime_base}/${schema_id}" ]]; then
        rm -rf -- "${runtime_dir}"
      fi
    fi
    rmdir "${lock_dir}" 2>/dev/null || true
  }
  trap cleanup EXIT INT TERM

  mkdir "${scratch}"
  freeze_closure "${scratch}"
  compile_helper "${scratch}"
  set_runtime_paths "${scratch}"
  run_helper_once "${scratch}" "${main_probe}" "${main_log}"
  run_helper_once "${scratch}" "${replay_probe}" "${replay_log}"
  validate_probe "${main_probe}"
  validate_probe "${replay_probe}"
  [[ ! -s "${main_log}" && ! -s "${replay_log}" ]] ||
    fail "successful primary/replay logs must be empty"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "primary and replay probes are not byte-identical"
  write_provenance "${scratch}"
  write_master_manifest "${scratch}"

  publish_directory_noreplace "${scratch}" "${runtime_dir}"
  cleanup_runtime_moved=true

  cp "${runtime_dir}/main/${schema_id}.probe" "${report_tmp}"
  [[ ! -e "${installed_report}" ]] || fail "installed report appeared during run"
  ln "${report_tmp}" "${installed_report}" ||
    fail "no-clobber report publication failed"
  cleanup_report_installed=true
  rm -f -- "${report_tmp}"
  (
    cd "${repo_root}"
    sha256sum \
      "src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report" \
      ".runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}/master.sha256" \
      >"${runtime_dir}/report_and_master.sha256"
  )
  verify_runtime
  cleanup_publication_complete=true
  trap - EXIT INT TERM
  rmdir "${lock_dir}" 2>/dev/null || true
  unset cleanup_scratch cleanup_report_tmp cleanup_runtime_moved
  unset cleanup_report_installed cleanup_publication_complete
  echo "canonical run complete: ${schema_id}"
}

usage() {
  echo "usage: $0 --preflight|--run|--verify" >&2
  exit 2
}

[[ $# -eq 1 ]] || usage
case "$1" in
--preflight)
  preflight
  ;;
--run)
  run_canonical
  ;;
--verify)
  verify_runtime
  ;;
*)
  usage
  ;;
esac
