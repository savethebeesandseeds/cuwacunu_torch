#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_conditioned_affine_historical_confirmation_v1"
runtime_base="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
runtime_dir="${runtime_base}/${schema_id}"
lock_dir="${runtime_base}/${schema_id}.lock"
installed_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report"

helper_name="mdn_frozen_conditioned_affine_historical_confirmation.cpp"
objective_name="mdn_frozen_affine_objective_ladder.cpp"
runner_name="run_mdn_frozen_conditioned_affine_historical_confirmation.sh"
capture_runner_name="run_mdn_frozen_feature_capture.sh"
helper_src="${script_dir}/${helper_name}"
objective_src="${script_dir}/${objective_name}"
capture_runner="${script_dir}/${capture_runner_name}"

capture_dir="${runtime_base}/synthetic_mdn_frozen_feature_capture.historical_760_1088.v1/anchor_760_1088"
historical_probe="${capture_dir}/mdn_edge_context_features.probe"
representation_probe="${capture_dir}/representation_edge_features.probe"
capture_report="${capture_dir}/channel_inference.report"
job_manifest="${capture_dir}/job.manifest"
runtime_result_fact="${capture_dir}/runtime.result.fact"
base_config="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"
conditioned_artifact="${runtime_base}/synthetic_mdn_frozen_affine_deployment_bridge_v1/main/synthetic_mdn_frozen_affine_deployment_bridge_v1.pt"

expected_helper_source_sha="6b6bf7d812e174adf96f1bd423f2b7a38925a37b348f74cf78a9facefa5045d9"
expected_objective_source_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"
expected_capture_runner_sha="115042fce12b569f3bac5429949898e65aae755e3673527b67a269d1a27dd82b"
expected_historical_probe_sha="477758c9a25e05138c32f70c7fb4ded1aac855aa7cd2beeff7f9060708866ac5"
expected_representation_probe_sha="8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
expected_capture_report_sha="9b99039ca0f15c82256b5664e108725b66142e92da8eaf87be4de6fd54e5317c"
expected_job_manifest_sha="d48c94c2bd78b0f4c71441d9ac86ac816580c61592796092e713b1f0732f196d"
expected_runtime_result_fact_sha="be29d99a92f3af7aab94daeb4dcfe23c1ad5ebf089f2b27f65c3eb79532005d6"
expected_base_config_sha="7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
expected_representation_checkpoint_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_checkpoint_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_conditioned_artifact_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"

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

preflight_inputs() {
  require_sha256 "${historical_probe}" "${expected_historical_probe_sha}"
  require_sha256 "${representation_probe}" \
    "${expected_representation_probe_sha}"
  require_sha256 "${capture_report}" "${expected_capture_report_sha}"
  require_sha256 "${job_manifest}" "${expected_job_manifest_sha}"
  require_sha256 "${runtime_result_fact}" \
    "${expected_runtime_result_fact_sha}"
  require_sha256 "${base_config}" "${expected_base_config_sha}"
  require_sha256 "${representation_checkpoint}" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_checkpoint_sha}"
  require_sha256 "${conditioned_artifact}" \
    "${expected_conditioned_artifact_sha}"
}

preflight_sources() {
  require_sha256 "${helper_src}" "${expected_helper_source_sha}"
  require_sha256 "${objective_src}" "${expected_objective_source_sha}"
  require_sha256 "${capture_runner}" "${expected_capture_runner_sha}"
  require_file "${script_dir}/${runner_name}"
  [[ -d "${repo_root}/src/include" ]] || fail "missing public include tree"
  if find "${repo_root}/src/include" -type l -print -quit | grep -q .; then
    fail "public include tree contains a symlink"
  fi
  require_source_token 'read_probe(options.historical_probe, kHistoricalBegin,'
  require_source_token 'load_channel_mdn_checkpoint_file'
  require_source_token 'forward_readout_features(features)'
  require_source_token 'execution.refit_performed=false'
  require_source_token 'execution.policy_path_used=false'
  require_source_token 'boundary.final_holdout_opened=false'
  require_source_token 'scientific_gate.pass='
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
  helper_bin="${build_dir}/mdn_frozen_conditioned_affine_historical_confirmation"
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
  cp "${capture_runner}" "${frozen_sources}/${capture_runner_name}"
  cp -a "${repo_root}/src/include/." "${frozen_include}/"
  cp "${historical_probe}" "${frozen_inputs}/historical_mdn.probe"
  cp "${representation_probe}" "${frozen_inputs}/historical_representation.probe"
  cp "${capture_report}" "${frozen_inputs}/capture.report"
  cp "${job_manifest}" "${frozen_inputs}/job.manifest"
  cp "${runtime_result_fact}" "${frozen_inputs}/runtime.result.fact"
  cp "${base_config}" "${frozen_inputs}/base.config"
  cp "${representation_checkpoint}" \
    "${frozen_inputs}/representation_checkpoint.pt"
  cp "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt"
  cp "${conditioned_artifact}" "${frozen_inputs}/conditioned_artifact.pt"

  require_sha256 "${frozen_sources}/${helper_name}" \
    "${expected_helper_source_sha}"
  require_sha256 "${frozen_sources}/${objective_name}" \
    "${expected_objective_source_sha}"
  require_sha256 "${frozen_sources}/${capture_runner_name}" \
    "${expected_capture_runner_sha}"
  require_sha256 "${frozen_inputs}/historical_mdn.probe" \
    "${expected_historical_probe_sha}"
  require_sha256 "${frozen_inputs}/historical_representation.probe" \
    "${expected_representation_probe_sha}"
  require_sha256 "${frozen_inputs}/capture.report" \
    "${expected_capture_report_sha}"
  require_sha256 "${frozen_inputs}/job.manifest" \
    "${expected_job_manifest_sha}"
  require_sha256 "${frozen_inputs}/runtime.result.fact" \
    "${expected_runtime_result_fact_sha}"
  require_sha256 "${frozen_inputs}/base.config" "${expected_base_config_sha}"
  require_sha256 "${frozen_inputs}/representation_checkpoint.pt" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/mdn_checkpoint.pt" \
    "${expected_mdn_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/conditioned_artifact.pt" \
    "${expected_conditioned_artifact_sha}"
  chmod -R a-w "${frozen_sources}" "${frozen_include}" "${frozen_inputs}"
  (
    cd "${root}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${objective_name}" \
      "frozen_sources/${runner_name}" \
      "frozen_sources/${capture_runner_name}" >frozen_sources.sha256
    find frozen_include -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_include.sha256
    find frozen_inputs -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_inputs.sha256
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
    --historical-probe "${frozen_inputs}/historical_mdn.probe" \
    --capture-report "${frozen_inputs}/capture.report" \
    --job-manifest "${frozen_inputs}/job.manifest" \
    --representation-checkpoint \
    "${frozen_inputs}/representation_checkpoint.pt" \
    --mdn-checkpoint "${frozen_inputs}/mdn_checkpoint.pt" \
    --v2-artifact "${frozen_inputs}/conditioned_artifact.pt" \
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
for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
    if not raw or "=" not in raw:
        raise SystemExit(f"malformed probe line {line_number}")
    key, value = raw.split("=", 1)
    if not key or key in values:
        raise SystemExit(f"duplicate or empty probe key at line {line_number}")
    values[key] = value

expected = {
    "schema_id": schema,
    "status": "complete",
    "result_role": "unseen_historical_no_refit_confirmation",
    "diagnostic_authority": "diagnostic_only",
    "benchmark_acceptance_authority": "false",
    "execution.provenance_pass": "true",
    "execution.production_checkpoint_loader_executed": "true",
    "execution.checkpoint_head_forward_executed": "true",
    "execution.conditioned_head_forward_executed": "true",
    "execution.training_performed": "false",
    "execution.refit_performed": "false",
    "execution.optimizer_steps": "0",
    "execution.policy_path_used": "false",
    "execution.checkpoint_written": "false",
    "execution.no_grad": "true",
    "execution.eval": "true",
    "execution.device": "cuda",
    "boundary.development": "[0,730)",
    "boundary.unseen_historical": "[760,1088)",
    "boundary.maximum_anchor_loaded": "1087",
    "boundary.final_holdout": "[1088,1170)",
    "boundary.final_holdout_opened": "false",
    "data.anchor_count": "328",
    "data.row_count": "2952",
    "data.feature_shape": "[328,3,3,400]",
    "data.dynamic_prefix_torch_equal": "true",
    "artifact.fit_range": "[0,554)",
    "artifact.purge_range": "[554,584)",
    "artifact.validation_range": "[584,730)",
    "artifact.maximum_anchor": "729",
    "artifact.run_only": "true",
    "artifact.policy_eligible": "false",
    "historical.checkpoint.count": "2952",
    "historical.checkpoint.pair_count": "2952",
    "historical.conditioned.count": "2952",
    "historical.conditioned.pair_count": "2952",
    "production_direct_objective.checkpoint.valid_count": "2952",
    "production_direct_objective.checkpoint.pairwise_valid_count": "2952",
    "production_direct_objective.conditioned.valid_count": "2952",
    "production_direct_objective.conditioned.pairwise_valid_count": "2952",
    "conclusion.final_holdout_remains_sealed": "true",
    "conclusion.no_refit_historical_confirmation_complete": "true",
    "input.historical_probe.sha256": "477758c9a25e05138c32f70c7fb4ded1aac855aa7cd2beeff7f9060708866ac5",
    "input.capture_report.sha256": "9b99039ca0f15c82256b5664e108725b66142e92da8eaf87be4de6fd54e5317c",
    "input.job_manifest.sha256": "d48c94c2bd78b0f4c71441d9ac86ac816580c61592796092e713b1f0732f196d",
    "input.representation_checkpoint.sha256": "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de",
    "input.mdn_checkpoint.sha256": "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e",
    "input.v2_artifact.sha256": "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739",
    "artifact.semantic_tensor_digest": "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38",
}
for key, expected_value in expected.items():
    actual = values.get(key)
    if actual != expected_value:
        raise SystemExit(f"unexpected {key}: {actual!r}, expected {expected_value!r}")

metric_fields = (
    "mae", "rmse", "directional_accuracy", "pairwise_rank_accuracy",
    "correlation", "prediction_population_std",
)
numeric_keys = ["data.dynamic_prefix_maximum_abs_delta"]
for arm in ("checkpoint", "conditioned"):
    numeric_keys.extend(f"historical.{arm}.{field}" for field in metric_fields)
    numeric_keys.extend(
        f"production_direct_objective.{arm}.{field}"
        for field in ("total", "regression", "direction", "rank")
    )
numeric_keys.extend((
    "improvement.directional_accuracy",
    "improvement.pairwise_rank_accuracy",
    "improvement.rmse_ratio",
    "scientific_gate.threshold.minimum_direction",
    "scientific_gate.threshold.minimum_rank",
    "scientific_gate.threshold.minimum_correlation",
    "scientific_gate.threshold.maximum_rmse",
    "scientific_gate.threshold.minimum_direction_improvement",
    "scientific_gate.threshold.minimum_rank_improvement",
    "scientific_gate.threshold.maximum_rmse_ratio",
))
numbers = {}
for key in numeric_keys:
    if key not in values:
        raise SystemExit(f"missing numeric field: {key}")
    number = float(values[key])
    if not math.isfinite(number):
        raise SystemExit(f"non-finite numeric field: {key}={values[key]!r}")
    numbers[key] = number

if numbers["data.dynamic_prefix_maximum_abs_delta"] != 0.0:
    raise SystemExit("historical dynamic feature prefix is not exact")

direction_delta = (
    numbers["historical.conditioned.directional_accuracy"]
    - numbers["historical.checkpoint.directional_accuracy"]
)
rank_delta = (
    numbers["historical.conditioned.pairwise_rank_accuracy"]
    - numbers["historical.checkpoint.pairwise_rank_accuracy"]
)
rmse_ratio = (
    numbers["historical.conditioned.rmse"]
    / numbers["historical.checkpoint.rmse"]
)
for actual, reported, name in (
    (direction_delta, numbers["improvement.directional_accuracy"], "direction"),
    (rank_delta, numbers["improvement.pairwise_rank_accuracy"], "rank"),
    (rmse_ratio, numbers["improvement.rmse_ratio"], "rmse ratio"),
):
    if abs(actual - reported) > 1.0e-12:
        raise SystemExit(f"inconsistent {name} improvement")

gate_results = {
    "scientific_gate.direction.pass":
        numbers["historical.conditioned.directional_accuracy"]
        >= numbers["scientific_gate.threshold.minimum_direction"],
    "scientific_gate.rank.pass":
        numbers["historical.conditioned.pairwise_rank_accuracy"]
        >= numbers["scientific_gate.threshold.minimum_rank"],
    "scientific_gate.correlation.pass":
        numbers["historical.conditioned.correlation"]
        >= numbers["scientific_gate.threshold.minimum_correlation"],
    "scientific_gate.rmse.pass":
        numbers["historical.conditioned.rmse"]
        <= numbers["scientific_gate.threshold.maximum_rmse"],
    "scientific_gate.direction_improvement.pass":
        direction_delta
        >= numbers["scientific_gate.threshold.minimum_direction_improvement"],
    "scientific_gate.rank_improvement.pass":
        rank_delta
        >= numbers["scientific_gate.threshold.minimum_rank_improvement"],
    "scientific_gate.rmse_improvement.pass":
        rmse_ratio
        <= numbers["scientific_gate.threshold.maximum_rmse_ratio"],
}
for key, truth in gate_results.items():
    expected_text = "true" if truth else "false"
    if values.get(key) != expected_text:
        raise SystemExit(f"inconsistent scientific gate: {key}")
overall = all(gate_results.values())
if values.get("scientific_gate.pass") != ("true" if overall else "false"):
    raise SystemExit("inconsistent overall scientific gate")
PY
}

write_provenance() {
  local root="$1"
  set_runtime_paths "${root}"
  {
    echo "schema_id=${schema_id}"
    echo "scope=unseen_historical_no_refit_confirmation"
    echo "diagnostic_authority=diagnostic_only"
    echo "benchmark_acceptance_authority=false"
    echo "training_performed=false"
    echo "refit_performed=false"
    echo "policy_path_used=false"
    echo "historical_anchor_range=[760,1088)"
    echo "maximum_anchor_loaded=1087"
    echo "final_holdout_anchor_range=[1088,1170)"
    echo "final_holdout_opened=false"
  } >"${root}/execution_scope.status"
  g++ --version >"${root}/compiler.version"
  nvidia-smi --query-gpu=name,driver_version,compute_cap \
    --format=csv,noheader >"${root}/cuda_device.status"
  (
    cd "${root}"
    sha256sum bin/mdn_frozen_conditioned_affine_historical_confirmation \
      >helper_binary.sha256
  )
}

write_master_manifest() {
  local root="$1"
  (
    cd "${root}"
    find . -type f ! -name master.sha256 ! -name report_and_master.sha256 \
      -print0 | sort -z | xargs -0 sha256sum >master.sha256
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

verify_live_closure() {
  local root="$1"
  cmp -s "${helper_src}" "${root}/frozen_sources/${helper_name}" ||
    fail "live helper differs from sealed helper"
  cmp -s "${objective_src}" "${root}/frozen_sources/${objective_name}" ||
    fail "live objective helper differs from sealed objective helper"
  cmp -s "${capture_runner}" \
    "${root}/frozen_sources/${capture_runner_name}" ||
    fail "live capture runner differs from sealed capture runner"
  cmp -s "${script_dir}/${runner_name}" \
    "${root}/frozen_sources/${runner_name}" ||
    fail "live runner differs from sealed runner"
  diff -qr "${repo_root}/src/include" "${root}/frozen_include" >/dev/null ||
    fail "live include tree differs from sealed include tree"
  cmp -s "${historical_probe}" \
    "${root}/frozen_inputs/historical_mdn.probe" ||
    fail "live historical MDN probe differs from sealed input"
  cmp -s "${representation_probe}" \
    "${root}/frozen_inputs/historical_representation.probe" ||
    fail "live representation probe differs from sealed input"
  cmp -s "${capture_report}" "${root}/frozen_inputs/capture.report" ||
    fail "live capture report differs from sealed input"
  cmp -s "${job_manifest}" "${root}/frozen_inputs/job.manifest" ||
    fail "live job manifest differs from sealed input"
  cmp -s "${runtime_result_fact}" \
    "${root}/frozen_inputs/runtime.result.fact" ||
    fail "live runtime result fact differs from sealed input"
  cmp -s "${base_config}" "${root}/frozen_inputs/base.config" ||
    fail "live base config differs from sealed input"
  cmp -s "${representation_checkpoint}" \
    "${root}/frozen_inputs/representation_checkpoint.pt" ||
    fail "live representation checkpoint differs from sealed input"
  cmp -s "${mdn_checkpoint}" "${root}/frozen_inputs/mdn_checkpoint.pt" ||
    fail "live MDN checkpoint differs from sealed input"
  cmp -s "${conditioned_artifact}" \
    "${root}/frozen_inputs/conditioned_artifact.pt" ||
    fail "live conditioned artifact differs from sealed input"
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
  preflight_sources
  preflight_environment
  verify_live_closure "${runtime_dir}"
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
  [[ ! -e "${installed_report}" ]] ||
    fail "installed report appeared during run"
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
