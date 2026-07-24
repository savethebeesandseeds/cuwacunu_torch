#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_channel_graph_first_conditioned_affine_shadow_eval_v1"
runtime_base="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
runtime_dir="${runtime_base}/${schema_id}"
lock_dir="${runtime_base}/${schema_id}.lock"
installed_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report"

runner_name="run_channel_graph_first_conditioned_affine_shadow_eval.sh"
harness_name="test_jkimyei_channel_graph_first_conditioned_affine_shadow_eval"
harness_source_name="${harness_name}.cpp"
test_dir="${repo_root}/src/tests/bench/jkimyei/training/channel_graph_first_launchers"
harness_source="${test_dir}/${harness_source_name}"
test_makefile="${test_dir}/Makefile"
makefile_config="${repo_root}/src/Makefile.config"
harness_live_binary="${repo_root}/.build/tests/${harness_name}"

base_config="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"
conditioned_artifact="${runtime_base}/synthetic_mdn_frozen_affine_deployment_bridge_v1/main/synthetic_mdn_frozen_affine_deployment_bridge_v1.pt"

expected_base_config_sha="7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
expected_representation_checkpoint_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_checkpoint_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_conditioned_artifact_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"

raw_relative_paths=(
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNALPHASYNUSD/1w/SYNALPHASYNUSD-1w-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNALPHASYNUSD/3d/SYNALPHASYNUSD-3d-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNBETASYNUSD/1w/SYNBETASYNUSD-1w-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNBETASYNUSD/3d/SYNBETASYNUSD-3d-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNGAMMASYNUSD/1w/SYNGAMMASYNUSD-1w-all-years.csv"
  "src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNGAMMASYNUSD/3d/SYNGAMMASYNUSD-3d-all-years.csv"
)
raw_expected_shas=(
  "8d266d72ba9b2a419a82078d60a1f3a3d5984d3bbe444ce377246aafe68fbb96"
  "d2cf91591bb554a77e6e9c3af4e50a587793a4f24f599d26c93b3ba7b0a9f020"
  "2691ad279d841229eb5980918d7837d6e0c24bb9067781bff2e4da428f9d13ad"
  "5d0a0c5581810cfaba7e783731398c0d80dbaa13672bf4868cb2519ca501055b"
  "8fbab5da910748df8eaec534e1877a2295a43e3f4415d08ba97bd4986cf6b9b2"
  "6e4e691cc95d3572d2d8ff59e642acaeaf6992b3c73cf243ba85d8f8496d3d4b"
  "81f7f7d5ef074360faf8514b0df5c204a3698f7cfaf0abf2a071a4526ca8f698"
  "2ed806271d1a88a56e3ded4051b81ea9f385db0ae46b4c215137af98f50f0872"
  "267a29cfba20027d219a61968d0439f08c66c1284cc6d00a56f0d9d224d6a6f0"
)

makefile_relative_paths=(
  "src/Makefile.config"
  "src/tests/bench/jkimyei/training/channel_graph_first_launchers/Makefile"
  "src/impl/piaabo/Makefile"
  "src/impl/piaabo/parse/bnf/Makefile"
  "src/impl/piaabo/math/Makefile"
  "src/impl/ujcamei/source/registry/types/Makefile"
  "src/impl/ujcamei/source/contract/Makefile"
  "src/impl/ujcamei/source/retrieval/storage/memory_mapped/Makefile"
  "src/impl/wikimyei/inference/expected_value/mdn/Makefile"
)

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

config_closure_relative_paths() {
  local installed_relative
  installed_relative="$(realpath -m --relative-to="${repo_root}" \
    "${installed_report}")"
  (
    cd "${repo_root}"
    find src/config -type f \
      ! -path 'src/config/.runtime/*' \
      ! -path "${installed_relative}" -print | LC_ALL=C sort
  )
}

reject_final_holdout_path() {
  local label="$1"
  local path="$2"
  local lowered="${path,,}"
  local forbidden_begin="$((1000 + 88))"
  local forbidden_end="$((1100 + 70))"
  [[ "${lowered}" != *final_holdout* &&
    "${path}" != *"${forbidden_begin}"* &&
    "${path}" != *"${forbidden_end}"* ]] ||
    fail "${label} refers to the forbidden final-holdout path/range: ${path}"
}

preflight_inputs() {
  require_sha256 "${base_config}" "${expected_base_config_sha}"
  require_sha256 "${representation_checkpoint}" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_checkpoint_sha}"
  require_sha256 "${conditioned_artifact}" \
    "${expected_conditioned_artifact_sha}"
  local index
  for index in "${!raw_relative_paths[@]}"; do
    require_sha256 "${repo_root}/${raw_relative_paths[index]}" \
      "${raw_expected_shas[index]}"
  done
}

preflight_sources() {
  require_file "${script_dir}/${runner_name}"
  require_file "${harness_source}"
  require_file "${test_makefile}"
  require_file "${makefile_config}"
  local relative_path
  for relative_path in "${makefile_relative_paths[@]}"; do
    require_file "${repo_root}/${relative_path}"
  done
  while IFS= read -r relative_path; do
    require_file "${repo_root}/${relative_path}"
  done < <(config_closure_relative_paths)
  [[ -d "${repo_root}/src/include" ]] || fail "missing public include tree"
  if find "${repo_root}/src/include" -type l -print -quit | grep -q .; then
    fail "public include tree contains a symlink"
  fi
  if find "${repo_root}/src/config" -type l \
    ! -path "${repo_root}/src/config/.runtime/*" -print -quit |
    grep -q .; then
    fail "config closure contains a symlink"
  fi
  grep -Fq "${harness_name}" "${test_makefile}" ||
    fail "test Makefile does not declare ${harness_name}"
  bash -n "${script_dir}/${runner_name}"
  clang-format --dry-run --Werror "${harness_source}"
}

preflight_environment() {
  command -v make >/dev/null || fail "make is unavailable"
  command -v python3 >/dev/null || fail "python3 is unavailable"
  command -v sha256sum >/dev/null || fail "sha256sum is unavailable"
  command -v nvidia-smi >/dev/null || fail "nvidia-smi is unavailable"
  nvidia-smi --query-gpu=name --format=csv,noheader | head -1 | grep -q . ||
    fail "CUDA GPU identity is unavailable"
}

preflight_boundaries() {
  reject_final_holdout_path "base config" "${base_config}"
  reject_final_holdout_path "runtime directory" "${runtime_dir}"
  reject_final_holdout_path "installed report" "${installed_report}"
  grep -Fq '584' "${harness_source}" ||
    fail "harness source is missing validation anchor 584"
  grep -Fq '730' "${harness_source}" ||
    fail "harness source is missing validation anchor 730"
}

preflight() {
  mkdir -p "${runtime_base}"
  preflight_inputs
  preflight_sources
  preflight_environment
  preflight_boundaries
  echo "preflight passed: ${schema_id}"
}

set_runtime_paths() {
  local root="$1"
  frozen_sources="${root}/frozen_sources"
  frozen_include="${root}/frozen_include"
  frozen_makefiles="${root}/frozen_makefiles"
  frozen_repo="${root}/frozen_repo"
  frozen_inputs="${root}/frozen_inputs"
  frozen_binary="${root}/bin/${harness_name}"
  main_dir="${root}/main"
  replay_dir="${root}/replay"
  main_work_dir="${main_dir}/work"
  replay_work_dir="${replay_dir}/work"
  main_fact="${main_dir}/${schema_id}.fact"
  replay_fact="${replay_dir}/${schema_id}.fact"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
}

copy_relative_file() {
  local relative_path="$1"
  local destination_root="$2"
  mkdir -p "${destination_root}/$(dirname "${relative_path}")"
  cp "${repo_root}/${relative_path}" \
    "${destination_root}/${relative_path}"
}

freeze_closure() {
  local root="$1"
  local relative_path
  set_runtime_paths "${root}"
  mkdir -p "${frozen_sources}" "${frozen_include}" \
    "${frozen_makefiles}" "${frozen_repo}" "${frozen_inputs}" \
    "${root}/bin" "${main_dir}" "${replay_dir}"

  cp "${harness_source}" "${frozen_sources}/${harness_source_name}"
  cp "${script_dir}/${runner_name}" "${frozen_sources}/${runner_name}"
  cp -a "${repo_root}/src/include/." "${frozen_include}/"
  for relative_path in "${makefile_relative_paths[@]}"; do
    copy_relative_file "${relative_path}" "${frozen_makefiles}"
  done
  while IFS= read -r relative_path; do
    copy_relative_file "${relative_path}" "${frozen_repo}"
  done < <(config_closure_relative_paths)
  for relative_path in "${raw_relative_paths[@]}"; do
    copy_relative_file "${relative_path}" "${frozen_repo}"
  done
  cp "${representation_checkpoint}" \
    "${frozen_inputs}/representation_checkpoint.pt"
  cp "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt"
  cp "${conditioned_artifact}" \
    "${frozen_inputs}/conditioned_artifact.pt"

  require_sha256 \
    "${frozen_repo}/src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config" \
    "${expected_base_config_sha}"
  require_sha256 "${frozen_inputs}/representation_checkpoint.pt" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/mdn_checkpoint.pt" \
    "${expected_mdn_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/conditioned_artifact.pt" \
    "${expected_conditioned_artifact_sha}"
  local index
  for index in "${!raw_relative_paths[@]}"; do
    require_sha256 "${frozen_repo}/${raw_relative_paths[index]}" \
      "${raw_expected_shas[index]}"
  done

  chmod -R a-w "${frozen_sources}" "${frozen_include}" \
    "${frozen_makefiles}" "${frozen_repo}" "${frozen_inputs}"
  (
    cd "${root}"
    find frozen_sources -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_sources.sha256
    find frozen_include -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_include.sha256
    find frozen_makefiles -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_makefiles.sha256
    find frozen_repo -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_repo.sha256
    find frozen_inputs -type f -print0 | sort -z | xargs -0 sha256sum \
      >frozen_inputs.sha256
  )
}

build_harness() {
  local root="$1"
  set_runtime_paths "${root}"
  make -C "${test_dir}" -j12 conditioned-affine-shadow-eval \
    >"${root}/build.stdout.log" 2>&1
  require_file "${harness_live_binary}"
  cp "${harness_live_binary}" "${frozen_binary}"
  chmod a-w "${frozen_binary}"
  ldd "${frozen_binary}" >"${root}/linked_libraries.status"
  ! grep -Fq 'not found' "${root}/linked_libraries.status" ||
    fail "frozen harness has an unresolved shared library"
  for library in libtorch_cuda.so libc10_cuda.so libcudart.so; do
    grep -Fq "${library}" "${root}/linked_libraries.status" ||
      fail "frozen harness does not resolve ${library}"
  done
  (
    cd "${root}"
    sha256sum "bin/${harness_name}" >frozen_binary.sha256
  )
}

write_execution_input_manifest() {
  local destination="$1"
  local paths=(
    "${base_config}"
    "${representation_checkpoint}"
    "${mdn_checkpoint}"
    "${conditioned_artifact}"
  )
  local relative_path
  for relative_path in "${raw_relative_paths[@]}"; do
    paths+=("${repo_root}/${relative_path}")
  done
  sha256sum "${paths[@]}" >"${destination}"
}

run_harness_once() {
  local root="$1"
  local work_dir="$2"
  local output="$3"
  local log="$4"
  set_runtime_paths "${root}"
  reject_final_holdout_path "harness work directory" "${work_dir}"
  reject_final_holdout_path "harness output" "${output}"
  [[ ! -e "${work_dir}" && -d "$(dirname "${work_dir}")" ]] ||
    fail "harness work directory is not fresh and absent: ${work_dir}"
  [[ ! -e "${output}" ]] || fail "harness output already exists: ${output}"
  (
    cd "${repo_root}"
    env \
      CUBLAS_WORKSPACE_CONFIG=:4096:8 \
      CUDA_DEVICE_ORDER=PCI_BUS_ID \
      OMP_NUM_THREADS=1 \
      MKL_NUM_THREADS=1 \
      "${frozen_binary}" \
      --config "${base_config}" \
      --representation-checkpoint "${representation_checkpoint}" \
      --mdn-checkpoint "${mdn_checkpoint}" \
      --artifact "${conditioned_artifact}" \
      --work-dir "${work_dir}" \
      --output "${output}"
  ) >"${log}" 2>&1
}

validate_fact() {
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
        raise SystemExit(f"malformed fact line {line_number}")
    key, value = raw.split("=", 1)
    if not key or key in values:
        raise SystemExit(f"duplicate or empty fact key at line {line_number}")
    values[key] = value

expected = {
    "schema_id": schema,
    "status": "complete",
    "scope": "validation_only",
    "execution.control.completed": "true",
    "execution.treatment.completed": "true",
    "training_performed": "false",
    "refit_performed": "false",
    "policy_path_used": "false",
    "final_holdout_opened": "false",
    "boundary.validation": "[584,730)",
    "boundary.maximum_anchor_loaded": "729",
    "execution.control.steps_attempted": "3",
    "execution.control.steps_completed": "3",
    "execution.control.streamed_anchor_count": "146",
    "execution.control.optimizer_steps": "0",
    "execution.control.checkpoint_written": "false",
    "execution.treatment.steps_attempted": "3",
    "execution.treatment.steps_completed": "3",
    "execution.treatment.streamed_anchor_count": "146",
    "execution.treatment.optimizer_steps": "0",
    "execution.treatment.checkpoint_written": "false",
    "parity.canonical_report.byte_equal": "true",
    "shadow.count": "1314",
    "shadow.pair_count": "1314",
    "parity.shadow_to_sealed_report.count.exact": "true",
    "parity.shadow_to_sealed_report.pair_count.exact": "true",
    "parity.shadow_to_sealed_report.exact": "false",
    "parity.shadow_to_sealed_report.tolerance_basis": "count_aware_live_batch_partition_operational_equivalence",
    "parity.shadow_to_sealed_report.pass": "true",
    "quality.preregistered_absolute_gates.pass": "true",
    "input.config.sha256": "7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6",
    "input.representation_checkpoint.sha256": "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de",
    "input.mdn_checkpoint.sha256": "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e",
    "input.artifact.sha256": "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739",
    "input.artifact.semantic_tensor_digest": "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38",
    "equivalence.pass": "true",
}
for key, expected_value in expected.items():
    actual = values.get(key)
    if actual != expected_value:
        raise SystemExit(f"unexpected {key}: {actual!r}, expected {expected_value!r}")

for tensor in ("log_pi", "mu", "sigma", "direct_edge_return"):
    prefix = f"parity.{tensor}"
    if values.get(f"{prefix}.batch_count") != "3":
        raise SystemExit(f"unexpected {tensor} batch count")
    if values.get(f"{prefix}.torch_equal") != "true":
        raise SystemExit(f"{tensor} is not torch-equal")
    if values.get(f"{prefix}.raw_byte_equal") != "true":
        raise SystemExit(f"{tensor} is not raw-byte equal")
    delta = float(values.get(f"{prefix}.maximum_abs_delta", "nan"))
    if not math.isfinite(delta) or delta != 0.0:
        raise SystemExit(f"nonzero {tensor} maximum delta")

expected_metrics = {
    "shadow.mae": (0.016155784230196004, 1.0e-12),
    "shadow.rmse": (0.020866339515652064, 1.0e-12),
    "shadow.directional_accuracy": (0.80669710806697104, 1.0e-12),
    "shadow.pairwise_rank_accuracy": (0.79223744292237441, 1.0e-12),
    "shadow.correlation": (0.68640422567195447, 1.0e-12),
    "shadow.prediction_population_std": (0.024317800238387269, 1.0e-12),
}
for key, (expected_value, tolerance) in expected_metrics.items():
    actual = float(values.get(key, "nan"))
    if not math.isfinite(actual) or abs(actual - expected_value) > tolerance:
        raise SystemExit(
            f"unexpected {key}: {actual!r}, expected {expected_value} +/- {tolerance}"
        )

continuous_tolerance = float(
    values.get("parity.shadow_to_sealed_report.continuous_metric_tolerance", "nan")
)
direction_count_tolerance = int(
    values.get("parity.shadow_to_sealed_report.direction_decision_count_tolerance", "-1")
)
rank_count_tolerance = int(
    values.get("parity.shadow_to_sealed_report.rank_decision_count_tolerance", "-1")
)
if continuous_tolerance != 1.0e-3:
    raise SystemExit("unexpected continuous-metric operational tolerance")
if direction_count_tolerance != 1 or rank_count_tolerance != 2:
    raise SystemExit("unexpected decision-count operational tolerances")
delta_tolerances = {
    "mae": continuous_tolerance,
    "rmse": continuous_tolerance,
    "correlation": continuous_tolerance,
    "prediction_std": continuous_tolerance,
}
for metric, tolerance in delta_tolerances.items():
    key = f"parity.shadow_to_sealed_report.{metric}.maximum_abs_delta"
    delta = float(values.get(key, "nan"))
    if not math.isfinite(delta) or delta < 0.0 or delta > tolerance:
        raise SystemExit(f"invalid sealed-report delta: {key}={delta!r}")
maximum_delta = float(
    values.get("parity.shadow_to_sealed_report.maximum_abs_delta", "nan")
)
maximum_continuous_delta = float(
    values.get(
        "parity.shadow_to_sealed_report.maximum_continuous_metric_abs_delta", "nan"
    )
)
if (
    not math.isfinite(maximum_continuous_delta)
    or maximum_continuous_delta < 0.0
    or maximum_continuous_delta > continuous_tolerance
):
    raise SystemExit("invalid maximum continuous sealed-report delta")
if not math.isfinite(maximum_delta) or maximum_delta < 0.0 or maximum_delta > (2.0 / 1314.0 + 1.0e-12):
    raise SystemExit("invalid maximum sealed-report delta")
direction_count_delta = int(
    values.get("parity.shadow_to_sealed_report.direction.decision_count_delta", "-1")
)
rank_count_delta = int(
    values.get("parity.shadow_to_sealed_report.rank.decision_count_delta", "-1")
)
if direction_count_delta < 0 or direction_count_delta > direction_count_tolerance:
    raise SystemExit("invalid directional decision-count delta")
if rank_count_delta < 0 or rank_count_delta > rank_count_tolerance:
    raise SystemExit("invalid rank decision-count delta")
direction_delta = float(
    values.get("parity.shadow_to_sealed_report.direction.maximum_abs_delta", "nan")
)
rank_delta = float(
    values.get("parity.shadow_to_sealed_report.rank.maximum_abs_delta", "nan")
)
if not math.isfinite(direction_delta) or direction_delta > (1.0 / 1314.0 + 1.0e-12):
    raise SystemExit("invalid directional-accuracy delta")
if not math.isfinite(rank_delta) or rank_delta > (2.0 / 1314.0 + 1.0e-12):
    raise SystemExit("invalid rank-accuracy delta")

for key, value in values.items():
    if key.endswith(".path") or key in {"work_dir", "output"}:
        raise SystemExit(f"volatile path key is forbidden in deterministic fact: {key}")
    if value.startswith("/") or "\\" in value:
        raise SystemExit(f"volatile path value is forbidden in deterministic fact: {key}")
PY
}

validate_shadow_report() {
  local path="$1"
  require_file "${path}"
  python3 - "${path}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${conditioned_artifact}" <<'PY'
import math
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
representation = str(pathlib.Path(sys.argv[2]).resolve())
mdn = str(pathlib.Path(sys.argv[3]).resolve())
artifact = str(pathlib.Path(sys.argv[4]).resolve())
values = {}
for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
    if not raw or "=" not in raw:
        raise SystemExit(f"malformed shadow report line {line_number}")
    key, value = raw.split("=", 1)
    if not key or key in values:
        raise SystemExit(f"duplicate or empty shadow key at line {line_number}")
    values[key] = value

expected = {
    "schema_id": "wikimyei.inference.expected_value.mdn.conditioned_affine_shadow_eval.v1",
    "status": "complete",
    "diagnostic_only": "true",
    "run_only": "true",
    "policy_eligible": "false",
    "canonical_output_mutated": "false",
    "inference_batch_observer_exposed": "false",
    "replay_or_policy_output_exposed": "false",
    "artifact.path": artifact,
    "artifact.file_sha256": "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739",
    "artifact.semantic_tensor_digest": "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38",
    "representation_checkpoint.path": representation,
    "representation_checkpoint.sha256": "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de",
    "mdn_checkpoint.path": mdn,
    "mdn_checkpoint.sha256": "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e",
    "artifact.fit_begin": "0",
    "artifact.fit_end": "554",
    "artifact.purge_begin": "554",
    "artifact.purge_end": "584",
    "artifact.validation_begin": "584",
    "artifact.validation_end": "730",
    "artifact.max_anchor": "729",
    "execution.observed_batch_count": "3",
    "execution.observed_anchor_count": "146",
    "execution.canonical_steps_attempted": "3",
    "execution.canonical_steps_completed": "3",
    "metrics.valid_count": "1314",
    "metrics.pairwise_rank_valid_count": "1314",
    "metrics.valid_count_per_edge": "438,438,438",
    "metrics.valid_count_per_channel": "438,438,438",
}
for key, expected_value in expected.items():
    actual = values.get(key)
    if actual != expected_value:
        raise SystemExit(f"unexpected shadow {key}: {actual!r}, expected {expected_value!r}")

expected_metrics = {
    "metrics.mae": (0.016155784230196004, 1.0e-12),
    "metrics.rmse": (0.020866339515652064, 1.0e-12),
    "metrics.directional_accuracy": (0.80669710806697104, 1.0e-12),
    "metrics.pairwise_rank_accuracy": (0.79223744292237441, 1.0e-12),
    "metrics.correlation": (0.68640422567195447, 1.0e-12),
    "metrics.prediction_std": (0.024317800238387269, 1.0e-12),
}
for key, (expected_value, tolerance) in expected_metrics.items():
    actual = float(values.get(key, "nan"))
    if not math.isfinite(actual) or abs(actual - expected_value) > tolerance:
        raise SystemExit(f"unexpected shadow metric {key}: {actual!r}")
PY
}

validate_success_log() {
  local path="$1"
  require_file "${path}"
  python3 - "${path}" <<'PY'
import pathlib
import re
import sys

raw = pathlib.Path(sys.argv[1]).read_bytes()
try:
    text = raw.decode("utf-8")
except UnicodeDecodeError as error:
    raise SystemExit(f"success log is not UTF-8: {error}")
plain = re.sub(r"\x1b\[[0-9;]*m", "", text)
expected = re.compile(
    r"^\[0x[0-9A-Fa-f]+\]: DEBUG: \[source_runtime_t\] initializing "
    r"static-global source snapshot \(single mutable cache updated by explicit "
    r"runtime call\)\n$"
)
if expected.fullmatch(plain) is None:
    raise SystemExit("successful launcher log is not the single expected runtime-init line")
PY
}

assert_final_holdout_artifacts_absent() {
  local matches
  matches="$({
    find "${runtime_base}" -mindepth 1 -maxdepth 4 \
      \( -iname '*final*holdout*' -o -iname '*1088*1170*' \) -print
    find "$(dirname "${installed_report}")" -mindepth 1 -maxdepth 1 \
      \( -iname '*final*holdout*' -o -iname '*1088*1170*' \) -print
  } 2>/dev/null || true)"
  [[ -z "${matches}" ]] ||
    fail "a final-holdout runtime, ledger, or report path already exists: ${matches}"
}

write_provenance() {
  local root="$1"
  set_runtime_paths "${root}"
  {
    echo "schema_id=${schema_id}"
    echo "scope=validation_only"
    echo "validation_anchor_range=[584,730)"
    echo "maximum_anchor_loaded=729"
    echo "anchor_count=146"
    echo "pulse_count=3"
    echo "target_count=1314"
    echo "training_performed=false"
    echo "refit_performed=false"
    echo "optimizer_steps=0"
    echo "checkpoint_written=false"
    echo "policy_path_used=false"
    echo "final_holdout_opened=false"
    echo "stable_canonical_input_paths_used=true"
    echo "build_target=conditioned-affine-shadow-eval"
    echo "primary_replay_work_dirs_isolated=true"
    echo "success_logs_empty=false"
    echo "success_logs_expected_runtime_init_only=true"
  } >"${root}/execution_scope.status"
  {
    echo "before=false"
    echo "after=false"
    echo "final_holdout_path_or_ledger_created=false"
  } >"${root}/final_holdout_artifacts.status"
  g++ --version >"${root}/compiler.version"
  nvidia-smi --query-gpu=name,driver_version,compute_cap \
    --format=csv,noheader >"${root}/cuda_device.status"
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

validate_run_pair() {
  local root="$1"
  set_runtime_paths "${root}"
  require_file "${main_fact}"
  require_file "${replay_fact}"
  require_file "${main_log}"
  require_file "${replay_log}"
  validate_success_log "${main_log}"
  validate_success_log "${replay_log}"
  cmp -s "${main_fact}" "${replay_fact}" ||
    fail "primary and replay facts are not byte-identical"
  validate_fact "${main_fact}"
  validate_fact "${replay_fact}"

  local main_canonical="${main_work_dir}/canonical.report"
  local replay_canonical="${replay_work_dir}/canonical.report"
  local main_shadow="${main_work_dir}/conditioned_affine_shadow.report"
  local replay_shadow="${replay_work_dir}/conditioned_affine_shadow.report"
  require_file "${main_canonical}"
  require_file "${replay_canonical}"
  require_file "${main_shadow}"
  require_file "${replay_shadow}"
  cmp -s "${main_canonical}" "${replay_canonical}" ||
    fail "primary and replay canonical reports are not byte-identical"
  cmp -s "${main_shadow}" "${replay_shadow}" ||
    fail "primary and replay shadow reports are not byte-identical"
  validate_shadow_report "${main_shadow}"
  validate_shadow_report "${replay_shadow}"
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
  preflight_boundaries
  verify_live_closure "${runtime_dir}"
  require_file "${runtime_dir}/execution_inputs.before.sha256"
  require_file "${runtime_dir}/execution_inputs.after.sha256"
  cmp -s "${runtime_dir}/execution_inputs.before.sha256" \
    "${runtime_dir}/execution_inputs.after.sha256" ||
    fail "execution input hashes changed during primary/replay"
  write_execution_input_manifest "${runtime_dir}/execution_inputs.verify.tmp"
  cmp -s "${runtime_dir}/execution_inputs.before.sha256" \
    "${runtime_dir}/execution_inputs.verify.tmp" || {
    rm -f -- "${runtime_dir}/execution_inputs.verify.tmp"
    fail "live execution input hashes differ from the seal"
  }
  rm -f -- "${runtime_dir}/execution_inputs.verify.tmp"
  validate_run_pair "${runtime_dir}"
  require_file "${installed_report}"
  set_runtime_paths "${runtime_dir}"
  cmp -s "${main_fact}" "${installed_report}" ||
    fail "installed report differs from the sealed primary fact"
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

  assert_final_holdout_artifacts_absent
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
  write_execution_input_manifest \
    "${scratch}/execution_inputs.before.sha256"
  verify_live_closure "${scratch}"
  build_harness "${scratch}"
  verify_live_closure "${scratch}"
  set_runtime_paths "${scratch}"
  run_harness_once "${scratch}" "${main_work_dir}" "${main_fact}" \
    "${main_log}"
  run_harness_once "${scratch}" "${replay_work_dir}" "${replay_fact}" \
    "${replay_log}"
  write_execution_input_manifest \
    "${scratch}/execution_inputs.after.sha256"
  cmp -s "${scratch}/execution_inputs.before.sha256" \
    "${scratch}/execution_inputs.after.sha256" ||
    fail "execution inputs changed during primary/replay"
  verify_live_closure "${scratch}"
  assert_final_holdout_artifacts_absent
  validate_run_pair "${scratch}"
  write_provenance "${scratch}"
  write_master_manifest "${scratch}"

  publish_directory_noreplace "${scratch}" "${runtime_dir}"
  cleanup_runtime_moved=true
  cp "${runtime_dir}/main/${schema_id}.fact" "${report_tmp}"
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

verify_live_closure() {
  local root="$1"
  local relative_path
  set_runtime_paths "${root}"
  cmp -s "${harness_source}" \
    "${frozen_sources}/${harness_source_name}" ||
    fail "live harness source differs from the sealed source"
  cmp -s "${script_dir}/${runner_name}" \
    "${frozen_sources}/${runner_name}" ||
    fail "live runner differs from the sealed runner"
  diff -qr "${repo_root}/src/include" "${frozen_include}" >/dev/null ||
    fail "live public include tree differs from the sealed tree"
  for relative_path in "${makefile_relative_paths[@]}"; do
    cmp -s "${repo_root}/${relative_path}" \
      "${frozen_makefiles}/${relative_path}" ||
      fail "live Makefile closure differs at ${relative_path}"
  done
  diff -u <(config_closure_relative_paths) <(
    cd "${frozen_repo}"
    find src/config -type f -print | LC_ALL=C sort
  ) >/dev/null || fail "live config closure file set differs from the seal"
  while IFS= read -r relative_path; do
    cmp -s "${repo_root}/${relative_path}" \
      "${frozen_repo}/${relative_path}" ||
      fail "live config closure differs at ${relative_path}"
  done < <(config_closure_relative_paths)
  for relative_path in "${raw_relative_paths[@]}"; do
    cmp -s "${repo_root}/${relative_path}" \
      "${frozen_repo}/${relative_path}" ||
      fail "live raw input differs at ${relative_path}"
  done
  cmp -s "${representation_checkpoint}" \
    "${frozen_inputs}/representation_checkpoint.pt" ||
    fail "live representation checkpoint differs from the sealed input"
  cmp -s "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt" ||
    fail "live MDN checkpoint differs from the sealed input"
  cmp -s "${conditioned_artifact}" \
    "${frozen_inputs}/conditioned_artifact.pt" ||
    fail "live conditioned artifact differs from the sealed input"
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
