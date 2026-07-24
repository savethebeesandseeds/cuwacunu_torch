#!/usr/bin/env bash
set -euo pipefail
umask 077

# This runner is the sole authority allowed to consume the preregistered
# synthetic final interval. Keep --preflight source/build/hash-only. The
# capture call exists only in run_initial(), after the external ledger link.

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_conditioned_affine_final_holdout_v1"
runtime_base="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
runtime_dir="${runtime_base}/${schema_id}"
capture_vault="${runtime_base}/${schema_id}.capture_vault"
lock_dir="${runtime_base}/${schema_id}.lock"
ledger_path="${runtime_base}/${schema_id}.consumption.ledger"
installed_report="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report"

helper_name="mdn_frozen_conditioned_affine_final_holdout.cpp"
runner_name="run_mdn_frozen_conditioned_affine_final_holdout.sh"
capture_runner_name="run_mdn_frozen_feature_capture.sh"
objective_name="mdn_frozen_affine_objective_ladder.cpp"
helper_src="${script_dir}/${helper_name}"
runner_src="${script_dir}/${runner_name}"
capture_runner="${script_dir}/${capture_runner_name}"
objective_src="${script_dir}/${objective_name}"

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
preregistration="${benchmark_root}/FINAL_HOLDOUT_PREREGISTRATION.md"
base_config="${benchmark_root}/synthetic_benchmark.config"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
representation_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
mdn_checkpoint="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"
conditioned_artifact="${runtime_base}/synthetic_mdn_frozen_affine_deployment_bridge_v1/main/synthetic_mdn_frozen_affine_deployment_bridge_v1.pt"
launcher_ab_fact="${benchmark_root}/artifacts/synthetic_mdn_channel_graph_first_conditioned_affine_shadow_eval_v1.report"

expected_capture_runner_sha="115042fce12b569f3bac5429949898e65aae755e3673527b67a269d1a27dd82b"
expected_objective_source_sha="157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289"
expected_base_config_sha="7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
expected_representation_checkpoint_sha="8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
expected_mdn_checkpoint_sha="eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
expected_conditioned_artifact_sha="7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739"
expected_launcher_ab_fact_sha="efee030e84a6868afef579086c88355951283b69023d55096a195f31f8fbf173"
expected_conditioned_semantic_digest="84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38"
expected_source_cursor_token="version=ujcamei.graph_anchor_cursor_report.v1|graph=d334e38b1887ae16|reference_edge=SYNALPHASYNUSD|Hx=30|Hf=1|edges=3|accepted=1170|candidates=1170|skipped=0|first=1580428799999|last=1681430399999"

declare -ar raw_relative_paths=(
  "data/raw/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
  "data/raw/SYNALPHASYNUSD/1w/SYNALPHASYNUSD-1w-all-years.csv"
  "data/raw/SYNALPHASYNUSD/3d/SYNALPHASYNUSD-3d-all-years.csv"
  "data/raw/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
  "data/raw/SYNBETASYNUSD/1w/SYNBETASYNUSD-1w-all-years.csv"
  "data/raw/SYNBETASYNUSD/3d/SYNBETASYNUSD-3d-all-years.csv"
  "data/raw/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
  "data/raw/SYNGAMMASYNUSD/1w/SYNGAMMASYNUSD-1w-all-years.csv"
  "data/raw/SYNGAMMASYNUSD/3d/SYNGAMMASYNUSD-3d-all-years.csv"
)
declare -ar raw_expected_shas=(
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

lock_owned=false
consumption_committed=false
vault_created=false
preflight_tmp=""
publish_scratch=""

fail() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
}

sha256_of() {
  sha256sum "$1" | awk '{print $1}'
}

require_sha256() {
  local path="$1"
  local expected="$2"
  local actual
  [[ "${expected}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "invalid expected SHA-256 for ${path}"
  require_file "${path}"
  actual="$(sha256_of "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "SHA-256 mismatch for ${path}: expected ${expected}, got ${actual}"
}

install_file_noreplace_atomic() {
  local temporary="$1"
  local destination="$2"
  require_file "${temporary}"
  [[ ! -e "${destination}" ]] ||
    fail "destination already exists: ${destination}"
  chmod a-w "${temporary}"
  sync -f "${temporary}"
  ln "${temporary}" "${destination}" ||
    fail "no-clobber atomic installation failed: ${destination}"
  sync -f "${destination}"
  sync -f "$(dirname "${destination}")"
  rm -f -- "${temporary}"
}

kv() {
  local key="$1"
  local file="$2"
  awk -v key="${key}" '
    index($0, key "=") == 1 { print substr($0, length(key) + 2); exit }
  ' "${file}" 2>/dev/null || true
}

require_kv() {
  local file="$1"
  local key="$2"
  local expected="$3"
  local actual
  require_file "${file}"
  actual="$(kv "${key}" "${file}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "unexpected ${key} in ${file}: expected '${expected}', got '${actual}'"
}

require_token() {
  local path="$1"
  local token="$2"
  grep -Fq -- "${token}" "${path}" ||
    fail "${path} is missing required token: ${token}"
}

preregistered_sha() {
  local key="$1"
  local value
  value="$(kv "${key}" "${preregistration}")"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "preregistration is missing sealed ${key}"
  printf '%s\n' "${value}"
}

cleanup() {
  local status=$?
  if [[ -e "${ledger_path}" ]]; then
    consumption_committed=true
  fi
  if [[ -n "${preflight_tmp:-}" &&
    "${preflight_tmp}" == "${runtime_base}/.${schema_id}.preflight."* ]]; then
    rm -rf -- "${preflight_tmp}"
  fi
  if [[ "${consumption_committed:-false}" != true &&
    ! -e "${ledger_path}" ]]; then
    if [[ -n "${publish_scratch:-}" &&
      "${publish_scratch}" == "${runtime_base}/${schema_id}.publish."* ]]; then
      rm -rf -- "${publish_scratch}"
    fi
    if [[ "${vault_created:-false}" == true &&
      "${capture_vault}" == "${runtime_base}/${schema_id}.capture_vault" ]]; then
      rm -rf -- "${capture_vault}"
    fi
  elif [[ "${status}" -ne 0 ]]; then
    echo "consumed-state preservation: ledger and capture vault retained" >&2
    [[ -z "${publish_scratch:-}" ]] ||
      echo "consumed-state preservation: publish scratch retained at ${publish_scratch}" >&2
  fi
  if [[ "${lock_owned:-false}" == true ]]; then
    rmdir "${lock_dir}" 2>/dev/null || true
  fi
  return "${status}"
}
trap cleanup EXIT INT TERM

assert_exact_reserved_paths_absent() {
  [[ ! -e "${runtime_dir}" ]] || fail "final runtime result already exists"
  [[ ! -e "${capture_vault}" ]] || fail "final capture vault already exists"
  [[ ! -e "${ledger_path}" ]] || fail "final consumption ledger already exists"
  [[ ! -e "${installed_report}" ]] || fail "final installed report already exists"
  [[ ! -e "${lock_dir}" ]] || fail "final canonical lock already exists"
}

validate_fixed_inputs() {
  require_sha256 "${capture_runner}" "${expected_capture_runner_sha}"
  require_sha256 "${objective_src}" "${expected_objective_source_sha}"
  require_sha256 "${base_config}" "${expected_base_config_sha}"
  require_sha256 "${representation_checkpoint}" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${mdn_checkpoint}" "${expected_mdn_checkpoint_sha}"
  require_sha256 "${conditioned_artifact}" \
    "${expected_conditioned_artifact_sha}"
  require_sha256 "${launcher_ab_fact}" "${expected_launcher_ab_fact_sha}"
  require_kv "${launcher_ab_fact}" status complete
  require_kv "${launcher_ab_fact}" scope validation_only
  require_kv "${launcher_ab_fact}" training_performed false
  require_kv "${launcher_ab_fact}" refit_performed false
  require_kv "${launcher_ab_fact}" policy_path_used false
  require_kv "${launcher_ab_fact}" final_holdout_opened false
  require_kv "${launcher_ab_fact}" boundary.maximum_anchor_loaded 729
  require_kv "${launcher_ab_fact}" parity.canonical_report.byte_equal true
  local tensor_name
  for tensor_name in log_pi mu sigma direct_edge_return; do
    require_kv "${launcher_ab_fact}" \
      "parity.${tensor_name}.torch_equal" true
    require_kv "${launcher_ab_fact}" \
      "parity.${tensor_name}.raw_byte_equal" true
    require_kv "${launcher_ab_fact}" \
      "parity.${tensor_name}.maximum_abs_delta" \
      0.00000000000000000e+00
  done
  require_kv "${launcher_ab_fact}" \
    parity.shadow_to_sealed_report.exact false
  require_kv "${launcher_ab_fact}" \
    parity.shadow_to_sealed_report.pass true
  require_kv "${launcher_ab_fact}" \
    quality.preregistered_absolute_gates.pass true
  require_kv "${launcher_ab_fact}" equivalence.pass true
  local index
  for index in "${!raw_relative_paths[@]}"; do
    require_sha256 "${benchmark_root}/${raw_relative_paths[${index}]}" \
      "${raw_expected_shas[${index}]}"
  done
}

validate_source_contract() {
  require_file "${helper_src}"
  require_file "${runner_src}"
  require_file "${preregistration}"
  require_file "${runtime_exec}"
  require_dir "${repo_root}/src/include"
  require_dir "${repo_root}/src/config"

  require_token "${preregistration}" \
    'Schema reserved for the eventual result:'
  require_token "${preregistration}" 'anchor_range=[1088,1170)'
  require_token "${preregistration}" 'expected_rows=738'
  require_token "${preregistration}" \
    'Publish the result whether the scientific gates pass or fail.'
  require_token "${capture_runner}" \
    'SYNTHETIC_MDN_FROZEN_CAPTURE_ALLOW_UNCONSUMED_HOLDOUT'
  require_token "${capture_runner}" '--no-replay-artifacts'
  require_token "${helper_src}" 'kFinalBegin = 1088'
  require_token "${helper_src}" 'kFinalEnd = 1170'
  require_token "${helper_src}" 'load_channel_mdn_checkpoint_file'
  require_token "${helper_src}" 'policy_path_used=false'
  require_token "${helper_src}" 'optimizer_steps=0'

  if find "${repo_root}/src/include" -type l -print -quit | grep -q .; then
    fail "public include tree contains a symlink"
  fi
  if find "${repo_root}/src/config" -type l -print -quit | grep -q .; then
    fail "transitive config tree contains a symlink"
  fi
  bash -n "${runner_src}"
  clang-format --dry-run --Werror "${helper_src}"
}

validate_environment() {
  local command_name
  for command_name in awk bash basename chmod clang-format cmp cp date diff \
    dirname find g++ grep head ldd ln mktemp mv nvidia-smi python3 readlink \
    rm seq sha256sum sort stat sync xargs; do
    command -v "${command_name}" >/dev/null ||
      fail "required command is unavailable: ${command_name}"
  done
  require_dir "${repo_root}/.external/libtorch-upd/include"
  require_file "${repo_root}/.external/libtorch-upd/lib/libtorch_cuda.so"
  [[ -d "/usr/local/cuda-12.4/include" ]] ||
    fail "CUDA 12.4 include directory is unavailable"
  nvidia-smi --query-gpu=name --format=csv,noheader | head -1 | grep -q . ||
    fail "CUDA GPU identity is unavailable"
}

write_include_manifest() {
  local source_root="$1"
  local output="$2"
  (
    cd "${source_root}"
    find . -type f -print0 | LC_ALL=C sort -z |
      xargs -0 -r sha256sum
  ) >"${output}"
}

write_config_manifest() {
  local source_root="$1"
  local output="$2"
  local prereg_rel="./benchmarks/synthetic_continuous_graph_v1/FINAL_HOLDOUT_PREREGISTRATION.md"
  local report_rel="./benchmarks/synthetic_continuous_graph_v1/artifacts/${schema_id}.report"
  (
    cd "${source_root}"
    find . -type f ! -path "${prereg_rel}" ! -path "${report_rel}" \
      -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum
  ) >"${output}"
}

write_makefile_manifest() {
  local source_root="$1"
  local output="$2"
  (
    cd "${source_root}"
    find . \
      \( -path './.git' -o -path './.runtime' -o -path './.build' -o \
      -path './.external' \) -prune -o \
      -type f \( -name Makefile -o -name Makefile.config -o -name '*.mk' \) \
      -print0 |
      LC_ALL=C sort -z | xargs -0 -r sha256sum
  ) >"${output}"
}

copy_makefiles() {
  local destination="$1"
  local path relative
  mkdir -p "${destination}"
  while IFS= read -r -d '' path; do
    relative="${path#${repo_root}/}"
    mkdir -p "${destination}/$(dirname "${relative}")"
    cp "${path}" "${destination}/${relative}"
  done < <(
    find "${repo_root}" \
      \( -path "${repo_root}/.git" -o -path "${repo_root}/.runtime" -o \
      -path "${repo_root}/.build" -o -path "${repo_root}/.external" \) \
      -prune -o -type f \( -name Makefile -o -name Makefile.config -o \
      -name '*.mk' \) -print0
  )
}

write_library_manifest() {
  local executable="$1"
  local output="$2"
  local library resolved
  ldd "${executable}" |
    awk '
      $2 == "=>" && $3 ~ /^\// { print $3 }
      $1 ~ /^\// { print $1 }
    ' | LC_ALL=C sort -u |
    while IFS= read -r library; do
      resolved="$(readlink -f -- "${library}")"
      [[ -n "${resolved}" ]] ||
        fail "failed to resolve linked library: ${library}"
      require_file "${resolved}"
      sha256sum "${resolved}"
    done >"${output}"
  [[ -s "${output}" ]] || fail "empty linked-library manifest for ${executable}"
}

write_exact_input_manifest() {
  local output="$1"
  local index
  {
    sha256sum "${helper_src}" "${runner_src}" "${capture_runner}" \
      "${objective_src}" "${preregistration}" \
      "${base_config}" "${runtime_exec}" "${representation_checkpoint}" \
      "${mdn_checkpoint}" "${conditioned_artifact}" "${launcher_ab_fact}"
    for index in "${!raw_relative_paths[@]}"; do
      sha256sum "${benchmark_root}/${raw_relative_paths[${index}]}"
    done
  } >"${output}"
}

make_expected_capture_config() {
  local input="$1"
  local output="$2"
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= cwu_02v_certified_replay_eval_mdn")
      replaced = 1
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${input}" >"${output}"
}

compile_helper() {
  local source_root="$1"
  local include_root="$2"
  local output="$3"
  local libtorch="${repo_root}/.external/libtorch-upd"
  local cuda="/usr/local/cuda-12.4"
  mkdir -p "$(dirname "${output}")"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -ffile-prefix-map="${source_root}=/sealed/sources" \
    -fmacro-prefix-map="${source_root}=/sealed/sources" \
    -ffile-prefix-map="${include_root}=/sealed/include" \
    -fmacro-prefix-map="${include_root}=/sealed/include" \
    -I"${source_root}" -I"${include_root}" \
    -isystem "${libtorch}/include" \
    -isystem "${libtorch}/include/torch/csrc/api/include" \
    -isystem "${cuda}/include" \
    "${source_root}/${helper_name}" \
    -L"${libtorch}/lib" -L"${cuda}/lib64" \
    -Wl,-rpath,"${libtorch}/lib" -Wl,-rpath,"${cuda}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda \
    -Wl,--as-needed -ltorch_cpu -ltorch -lc10 \
    -lcuda -lcudart -lnvToolsExt -lstdc++ -lpthread -lm \
    -o "${output}"
  local linked="${output}.linked_libraries.status"
  ldd "${output}" >"${linked}"
  ! grep -Fq 'not found' "${linked}" ||
    fail "compiled final helper has an unresolved shared library"
  local library
  for library in libtorch_cuda.so libc10_cuda.so libcudart.so; do
    grep -Fq "${library}" "${linked}" ||
      fail "compiled final helper does not resolve ${library}"
  done
}

write_seal_candidates() {
  local root="$1"
  local candidate_helper="${root}/bin/final_holdout_helper"
  mkdir -p "${root}/sources" "${root}/include"
  cp "${helper_src}" "${objective_src}" "${root}/sources/"
  cp -a "${repo_root}/src/include/." "${root}/include/"
  compile_helper "${root}/sources" "${root}/include" "${candidate_helper}"
  write_include_manifest "${repo_root}/src/include" \
    "${root}/public_headers.sha256"
  write_config_manifest "${repo_root}/src/config" \
    "${root}/transitive_config.sha256"
  write_makefile_manifest "${repo_root}" "${root}/makefiles.sha256"
  write_library_manifest "${runtime_exec}" \
    "${root}/runtime_libraries.sha256"
  write_library_manifest "${candidate_helper}" \
    "${root}/helper_libraries.sha256"
  printf 'final_helper_sha256=%s\n' "$(sha256_of "${helper_src}")"
  printf 'final_runner_sha256=%s\n' "$(sha256_of "${runner_src}")"
  printf 'runtime_executable_sha256=%s\n' "$(sha256_of "${runtime_exec}")"
  printf 'public_header_tree_manifest_sha256=%s\n' \
    "$(sha256_of "${root}/public_headers.sha256")"
  printf 'transitive_config_tree_manifest_sha256=%s\n' \
    "$(sha256_of "${root}/transitive_config.sha256")"
  printf 'makefile_tree_manifest_sha256=%s\n' \
    "$(sha256_of "${root}/makefiles.sha256")"
  printf 'final_helper_binary_sha256=%s\n' \
    "$(sha256_of "${candidate_helper}")"
  printf 'runtime_shared_libraries_manifest_sha256=%s\n' \
    "$(sha256_of "${root}/runtime_libraries.sha256")"
  printf 'helper_shared_libraries_manifest_sha256=%s\n' \
    "$(sha256_of "${root}/helper_libraries.sha256")"
}

preflight_common() {
  mkdir -p "${runtime_base}"
  validate_fixed_inputs
  validate_source_contract
  validate_environment
}

preflight() {
  preflight_common
  assert_exact_reserved_paths_absent
  preflight_tmp="$(mktemp -d \
    "${runtime_base}/.${schema_id}.preflight.XXXXXX")"
  echo "preflight passed; candidate executable-closure seals follow"
  write_seal_candidates "${preflight_tmp}"
}

set_vault_paths() {
  frozen_sources="${capture_vault}/frozen_sources"
  frozen_include="${capture_vault}/frozen_include"
  frozen_config="${capture_vault}/frozen_config"
  frozen_makefiles="${capture_vault}/frozen_makefiles"
  frozen_inputs="${capture_vault}/frozen_inputs"
  frozen_executables="${capture_vault}/frozen_executables"
  bin_dir="${capture_vault}/bin"
  helper_bin="${bin_dir}/mdn_frozen_conditioned_affine_final_holdout"
  capture_job="${capture_vault}/capture"
  capture_config="${frozen_config}/benchmarks/synthetic_continuous_graph_v1/.final_holdout_capture.config"
  capture_runtime_log="${capture_vault}/anchor_1088_1170.log"
  capture_invocation_log="${capture_vault}/capture_runner.stdout.log"
  representation_probe="${capture_job}/representation_edge_features.probe"
  mdn_probe="${capture_job}/mdn_edge_context_features.probe"
  capture_report="${capture_job}/channel_inference.report"
  job_manifest="${capture_job}/job.manifest"
  runtime_result_fact="${capture_job}/runtime.result.fact"
  local_ledger="${capture_vault}/consumption.ledger"
  capture_complete="${capture_vault}/capture.complete"
  selected_evaluation="${capture_vault}/selected_evaluation.status"
  vault_master="${capture_vault}/vault_master.sha256"
}

assert_read_only_tree() {
  local root="$1"
  require_dir "${root}"
  if find "${root}" -perm /222 -print -quit | grep -q .; then
    fail "sealed tree contains a writable path: ${root}"
  fi
}

assert_frozen_config_permissions() {
  local capture_config_parent
  capture_config_parent="$(dirname "${capture_config}")"
  if [[ -e "${capture_config}" ]]; then
    assert_read_only_tree "${frozen_config}"
    return
  fi
  local -a writable_paths=()
  mapfile -t writable_paths < <(find "${frozen_config}" -perm /222 -print)
  [[ "${#writable_paths[@]}" -eq 1 &&
    "${writable_paths[0]}" == "${capture_config_parent}" ]] ||
    fail "only the capture-config parent may be writable before capture"
  find "${capture_config_parent}" -maxdepth 0 -type d -perm /200 -print -quit |
    grep -q . || fail "capture-config parent is not owner-writable"
}

verify_tree_manifest() {
  local root="$1"
  local manifest="$2"
  require_file "${manifest}"
  (
    cd "${root}"
    sha256sum -c "${manifest}" >/dev/null
  )
}

freeze_closure() {
  set_vault_paths
  mkdir -p "${frozen_sources}" "${frozen_include}" "${frozen_config}" \
    "${frozen_makefiles}" "${frozen_inputs}" "${frozen_executables}" \
    "${bin_dir}"

  cp "${helper_src}" "${objective_src}" "${capture_runner}" "${runner_src}" \
    "${preregistration}" "${frozen_sources}/"
  cp -a "${repo_root}/src/include/." "${frozen_include}/"
  cp -a "${repo_root}/src/config/." "${frozen_config}/"
  copy_makefiles "${frozen_makefiles}"
  cp "${base_config}" "${frozen_inputs}/base.config"
  cp "${representation_checkpoint}" \
    "${frozen_inputs}/representation_checkpoint.pt"
  cp "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt"
  cp "${conditioned_artifact}" "${frozen_inputs}/conditioned_artifact.pt"
  cp "${launcher_ab_fact}" "${frozen_inputs}/launcher_shadow_ab.report"
  cp "${runtime_exec}" "${frozen_executables}/cuwacunu_exec"
  make_expected_capture_config "${frozen_inputs}/base.config" \
    "${frozen_inputs}/expected_capture.config"

  write_include_manifest "${frozen_include}" \
    "${capture_vault}/public_headers.sha256"
  write_config_manifest "${frozen_config}" \
    "${capture_vault}/transitive_config.sha256"
  write_makefile_manifest "${frozen_makefiles}" \
    "${capture_vault}/makefiles.sha256"
  write_exact_input_manifest "${capture_vault}/exact_inputs.before.sha256"
  write_library_manifest "${runtime_exec}" \
    "${capture_vault}/runtime_libraries.sha256"

  compile_helper "${frozen_sources}" "${frozen_include}" "${helper_bin}"
  write_library_manifest "${helper_bin}" \
    "${capture_vault}/helper_libraries.sha256"

  require_sha256 "${frozen_sources}/${capture_runner_name}" \
    "${expected_capture_runner_sha}"
  require_sha256 "${frozen_sources}/${objective_name}" \
    "${expected_objective_source_sha}"
  require_sha256 "${frozen_inputs}/base.config" "${expected_base_config_sha}"
  require_sha256 "${frozen_inputs}/representation_checkpoint.pt" \
    "${expected_representation_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/mdn_checkpoint.pt" \
    "${expected_mdn_checkpoint_sha}"
  require_sha256 "${frozen_inputs}/conditioned_artifact.pt" \
    "${expected_conditioned_artifact_sha}"
  require_sha256 "${frozen_inputs}/launcher_shadow_ab.report" \
    "${expected_launcher_ab_fact_sha}"
  cmp -s "${runtime_exec}" "${frozen_executables}/cuwacunu_exec" ||
    fail "frozen production executable differs from the live executable"

  g++ --version >"${capture_vault}/compiler.version"
  nvidia-smi --query-gpu=name,driver_version,compute_cap \
    --format=csv,noheader >"${capture_vault}/cuda_device.status"
  {
    echo "schema_id=${schema_id}"
    echo "scope=sealed_final_holdout_no_refit_evaluation"
    echo "capture_range=[1088,1170)"
    echo "capture_authority=production_launcher_exactly_once"
    echo "evaluation_authority=frozen_probe_replay_only"
    echo "training_performed=false"
    echo "refit_performed=false"
    echo "optimizer_steps=0"
    echo "policy_path_used=false"
    echo "candidate_count=1"
  } >"${capture_vault}/execution_scope.status"

  chmod -R a-w "${frozen_sources}" "${frozen_include}" "${frozen_config}" \
    "${frozen_makefiles}" "${frozen_inputs}" "${frozen_executables}" \
    "${bin_dir}"
  chmod u+w "$(dirname "${capture_config}")"
  chmod a-w "${helper_bin}" "${helper_bin}.linked_libraries.status" \
    "${capture_vault}/public_headers.sha256" \
    "${capture_vault}/transitive_config.sha256" \
    "${capture_vault}/makefiles.sha256" \
    "${capture_vault}/exact_inputs.before.sha256" \
    "${capture_vault}/runtime_libraries.sha256" \
    "${capture_vault}/helper_libraries.sha256"
  chmod a-w "${capture_vault}/compiler.version" \
    "${capture_vault}/cuda_device.status" \
    "${capture_vault}/execution_scope.status"
}

require_seal_value() {
  local key="$1"
  local actual="$2"
  local expected
  expected="$(preregistered_sha "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "executable-closure seal mismatch for ${key}: expected ${expected}, got ${actual}"
}

require_protocol_seal() {
  set_vault_paths
  require_seal_value final_helper_sha256 \
    "$(sha256_of "${frozen_sources}/${helper_name}")"
  require_seal_value final_runner_sha256 \
    "$(sha256_of "${frozen_sources}/${runner_name}")"
  require_seal_value runtime_executable_sha256 \
    "$(sha256_of "${frozen_executables}/cuwacunu_exec")"
  require_seal_value public_header_tree_manifest_sha256 \
    "$(sha256_of "${capture_vault}/public_headers.sha256")"
  require_seal_value transitive_config_tree_manifest_sha256 \
    "$(sha256_of "${capture_vault}/transitive_config.sha256")"
  require_seal_value makefile_tree_manifest_sha256 \
    "$(sha256_of "${capture_vault}/makefiles.sha256")"
  require_seal_value final_helper_binary_sha256 "$(sha256_of "${helper_bin}")"
  require_seal_value runtime_shared_libraries_manifest_sha256 \
    "$(sha256_of "${capture_vault}/runtime_libraries.sha256")"
  require_seal_value helper_shared_libraries_manifest_sha256 \
    "$(sha256_of "${capture_vault}/helper_libraries.sha256")"
}

verify_live_closure() {
  set_vault_paths
  local verify_tmp
  verify_tmp="$(mktemp -d "/tmp/${schema_id}.verify.XXXXXX")"

  cmp -s "${helper_src}" "${frozen_sources}/${helper_name}" ||
    fail "live final helper differs from the frozen helper"
  cmp -s "${objective_src}" "${frozen_sources}/${objective_name}" ||
    fail "live objective helper differs from the frozen helper"
  cmp -s "${capture_runner}" \
    "${frozen_sources}/${capture_runner_name}" ||
    fail "live capture runner differs from the frozen runner"
  cmp -s "${runner_src}" "${frozen_sources}/${runner_name}" ||
    fail "live final runner differs from the frozen runner"
  cmp -s "${preregistration}" \
    "${frozen_sources}/FINAL_HOLDOUT_PREREGISTRATION.md" ||
    fail "live preregistration differs from the frozen preregistration"
  cmp -s "${runtime_exec}" "${frozen_executables}/cuwacunu_exec" ||
    fail "live production executable differs from the frozen executable"
  cmp -s "${representation_checkpoint}" \
    "${frozen_inputs}/representation_checkpoint.pt" ||
    fail "live representation checkpoint differs from the frozen input"
  cmp -s "${mdn_checkpoint}" "${frozen_inputs}/mdn_checkpoint.pt" ||
    fail "live MDN checkpoint differs from the frozen input"
  cmp -s "${conditioned_artifact}" \
    "${frozen_inputs}/conditioned_artifact.pt" ||
    fail "live conditioned artifact differs from the frozen input"
  cmp -s "${launcher_ab_fact}" \
    "${frozen_inputs}/launcher_shadow_ab.report" ||
    fail "live launcher A/B prerequisite differs from the frozen input"

  write_include_manifest "${repo_root}/src/include" \
    "${verify_tmp}/public_headers.sha256"
  write_config_manifest "${repo_root}/src/config" \
    "${verify_tmp}/transitive_config.sha256"
  write_makefile_manifest "${repo_root}" "${verify_tmp}/makefiles.sha256"
  write_exact_input_manifest "${verify_tmp}/exact_inputs.sha256"
  write_library_manifest "${runtime_exec}" \
    "${verify_tmp}/runtime_libraries.sha256"
  write_library_manifest "${helper_bin}" \
    "${verify_tmp}/helper_libraries.sha256"
  cmp -s "${verify_tmp}/public_headers.sha256" \
    "${capture_vault}/public_headers.sha256" ||
    fail "live public-header tree differs from the frozen manifest"
  cmp -s "${verify_tmp}/transitive_config.sha256" \
    "${capture_vault}/transitive_config.sha256" ||
    fail "live transitive-config tree differs from the frozen manifest"
  cmp -s "${verify_tmp}/makefiles.sha256" \
    "${capture_vault}/makefiles.sha256" ||
    fail "live Makefile closure differs from the frozen manifest"
  cmp -s "${verify_tmp}/exact_inputs.sha256" \
    "${capture_vault}/exact_inputs.before.sha256" ||
    fail "live exact input hashes changed after closure freeze"
  cmp -s "${verify_tmp}/runtime_libraries.sha256" \
    "${capture_vault}/runtime_libraries.sha256" ||
    fail "production executable shared libraries changed"
  cmp -s "${verify_tmp}/helper_libraries.sha256" \
    "${capture_vault}/helper_libraries.sha256" ||
    fail "helper shared libraries changed"
  rm -rf -- "${verify_tmp}"

  verify_tree_manifest "${frozen_include}" \
    "${capture_vault}/public_headers.sha256"
  verify_tree_manifest "${frozen_config}" \
    "${capture_vault}/transitive_config.sha256"
  verify_tree_manifest "${frozen_makefiles}" \
    "${capture_vault}/makefiles.sha256"
  assert_read_only_tree "${frozen_sources}"
  assert_read_only_tree "${frozen_include}"
  assert_frozen_config_permissions
  assert_read_only_tree "${frozen_makefiles}"
  assert_read_only_tree "${frozen_inputs}"
  assert_read_only_tree "${frozen_executables}"
  assert_read_only_tree "${bin_dir}"
}

create_consumption_ledger() {
  set_vault_paths
  [[ ! -e "${local_ledger}" ]] || fail "local ledger already exists"
  [[ ! -e "${ledger_path}" ]] || fail "external ledger already exists"
  {
    echo "schema_id=${schema_id}"
    echo "ledger_schema=synthetic_final_holdout_consumption_ledger.v1"
    echo "status=capture_authorized_interval_conservatively_consumed"
    echo "anchor_range=[1088,1170)"
    echo "anchor_count=82"
    echo "expected_rows=738"
    echo "capture_policy=exactly_once"
    echo "capture_schema_id=$(basename "${capture_vault}")"
    echo "capture_vault=${capture_vault}"
    echo "capture_job=${capture_job}"
    echo "capture_config=${capture_config}"
    echo "capture_invocation_imminent=true"
    echo "recapture_permitted=false"
    echo "resume_policy=evaluate_from_capture_complete_only"
    echo "created_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "runner.sha256=$(sha256_of "${runner_src}")"
    echo "helper.sha256=$(sha256_of "${helper_src}")"
    echo "capture_runner.sha256=$(sha256_of "${capture_runner}")"
    echo "runtime_executable.sha256=$(sha256_of "${runtime_exec}")"
  } >"${local_ledger}"
  chmod a-w "${local_ledger}"
  ln "${local_ledger}" "${ledger_path}" ||
    fail "no-clobber external consumption-ledger publication failed"
  consumption_committed=true
  sync -f "${ledger_path}"
}

validate_ledger() {
  set_vault_paths
  require_file "${local_ledger}"
  require_file "${ledger_path}"
  [[ "${local_ledger}" -ef "${ledger_path}" ]] ||
    fail "local and external consumption ledgers are not the same inode"
  require_kv "${ledger_path}" schema_id "${schema_id}"
  require_kv "${ledger_path}" status \
    capture_authorized_interval_conservatively_consumed
  require_kv "${ledger_path}" anchor_range '[1088,1170)'
  require_kv "${ledger_path}" anchor_count 82
  require_kv "${ledger_path}" expected_rows 738
  require_kv "${ledger_path}" capture_policy exactly_once
  require_kv "${ledger_path}" recapture_permitted false
  require_kv "${ledger_path}" capture_vault "${capture_vault}"
}

validate_capture_metadata() {
  set_vault_paths
  require_file "${representation_probe}"
  require_file "${mdn_probe}"
  require_file "${capture_report}"
  require_file "${job_manifest}"
  require_file "${runtime_result_fact}"
  require_file "${capture_config}"
  require_file "${capture_runtime_log}"
  require_file "${capture_invocation_log}"
  cmp -s "${capture_config}" \
    "${frozen_inputs}/expected_capture.config" ||
    fail "generated capture config differs from the sealed transformation"

  local representation_rows mdn_rows
  representation_rows="$(awk 'END { print NR > 0 ? NR - 1 : -1 }' \
    "${representation_probe}")"
  mdn_rows="$(awk 'END { print NR > 0 ? NR - 1 : -1 }' "${mdn_probe}")"
  [[ "${representation_rows}" == 738 ]] ||
    fail "representation probe row count is ${representation_rows}, expected 738"
  [[ "${mdn_rows}" == 738 ]] ||
    fail "MDN probe row count is ${mdn_rows}, expected 738"

  require_kv "${runtime_result_fact}" status completed
  require_kv "${runtime_result_fact}" checkpoint_written false
  require_kv "${runtime_result_fact}" checkpoint_path ''
  require_kv "${runtime_result_fact}" optimizer_steps 0
  require_kv "${runtime_result_fact}" model_state_mutated false
  require_kv "${runtime_result_fact}" nonfinite_output_count 0
  require_kv "${runtime_result_fact}" wave_action run
  require_kv "${runtime_result_fact}" source_report_path "${capture_report}"
  require_kv "${job_manifest}" resolved_anchor_index_begin 1088
  require_kv "${job_manifest}" resolved_anchor_index_end 1170
  require_kv "${job_manifest}" accepted_anchor_count 1170
  require_kv "${job_manifest}" candidate_anchor_count 1170
  require_kv "${job_manifest}" skipped_outside_common_range 0
  require_kv "${job_manifest}" skipped_missing_edge_coverage 0
  require_kv "${job_manifest}" skipped_failed_fetch_probe 0
  require_kv "${job_manifest}" duplicate_anchor_count 0
  require_kv "${job_manifest}" source_cursor_id \
    split:certified_replay_expansion_eval
  require_kv "${job_manifest}" source_cursor_token \
    "${expected_source_cursor_token}"
  require_kv "${job_manifest}" graph_order_fingerprint d334e38b1887ae16
  require_kv "${job_manifest}" node_ids SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA
  require_kv "${job_manifest}" edge_ids \
    SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD
  require_kv "${job_manifest}" wave_id cwu_02v_certified_replay_eval_mdn
  require_kv "${job_manifest}" wave_action run
  require_kv "${job_manifest}" wave_mode 'run|debug'
  require_kv "${job_manifest}" config_path "${capture_config}"
  require_kv "${job_manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint}"
  require_kv "${job_manifest}" input_mdn_checkpoint_path "${mdn_checkpoint}"
  require_kv "${job_manifest}" probe_sidecar_enabled true

  require_kv "${capture_report}" requested_anchor_index_begin 1088
  require_kv "${capture_report}" requested_anchor_index_end 1170
  require_kv "${capture_report}" source_anchor_count 1170
  require_kv "${capture_report}" source_cursor_token \
    "${expected_source_cursor_token}"
  require_kv "${capture_report}" graph_order_fingerprint d334e38b1887ae16
  require_kv "${capture_report}" node_ids SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA
  require_kv "${capture_report}" edge_ids \
    SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD
  require_kv "${capture_report}" wave_streamed_anchor_count 82
  require_kv "${capture_report}" steps_attempted 2
  require_kv "${capture_report}" steps_completed 2
  require_kv "${capture_report}" skipped_batches 0
  require_kv "${capture_report}" wave_pulses_attempted 2
  require_kv "${capture_report}" wave_pulses_completed 2
  require_kv "${capture_report}" wave_pulses_skipped 0
  require_kv "${capture_report}" optimizer_steps 0
  require_kv "${capture_report}" effective_batch_size 64
  require_kv "${capture_report}" batch_size_source derived
  require_kv "${capture_report}" dtype float32
  require_kv "${capture_report}" device cuda
  require_kv "${capture_report}" target_action run
  require_kv "${capture_report}" representation_checkpoint_loaded true
  require_kv "${capture_report}" mdn_checkpoint_loaded true
  require_kv "${capture_report}" checkpoint_written false
  require_kv "${capture_report}" checkpoint_write_count 0
  require_kv "${capture_report}" checkpoint_path ''
  require_kv "${capture_report}" nonfinite_output_count 0
  require_kv "${capture_report}" finite_parameter_check 1
  require_kv "${capture_report}" representation_parameter_device_check true
  require_kv "${capture_report}" mdn_parameter_device_check true
  require_kv "${capture_report}" representation_edge_feature_probe_written true
  require_kv "${capture_report}" representation_edge_feature_probe_row_count 738
  require_kv "${capture_report}" representation_edge_feature_probe_path \
    "${representation_probe}"
  require_kv "${capture_report}" representation_edge_feature_probe_error ''
  require_kv "${capture_report}" mdn_edge_context_feature_probe_written true
  require_kv "${capture_report}" mdn_edge_context_feature_probe_row_count 738
  require_kv "${capture_report}" mdn_edge_context_feature_probe_path \
    "${mdn_probe}"
  require_kv "${capture_report}" mdn_edge_context_feature_probe_error ''
  require_kv "${capture_report}" report_written true
  require_kv "${capture_report}" report_write_count 1
  require_kv "${capture_report}" report_path "${capture_report}"
  require_kv "${capture_report}" runtime_lls_emitted true
  require_kv "${capture_report}" direct_edge_return_readout_loss_valid_count 738
  require_kv "${capture_report}" \
    direct_edge_return_readout_loss_pairwise_valid_count 738
}

write_capture_complete() {
  local completion_origin="$1"
  set_vault_paths
  [[ ! -e "${capture_complete}" ]] || fail "capture.complete already exists"
  validate_capture_metadata
  chmod -R a-w "${capture_job}"
  chmod a-w "${capture_config}" "$(dirname "${capture_config}")" \
    "${capture_runtime_log}" "${capture_invocation_log}"
  local exact_after="${capture_vault}/exact_inputs.after.sha256"
  if [[ ! -e "${exact_after}" ]]; then
    local exact_after_tmp="${capture_vault}/.exact_inputs.after.$$"
    write_exact_input_manifest "${exact_after_tmp}"
    install_file_noreplace_atomic "${exact_after_tmp}" "${exact_after}"
  fi
  chmod a-w "${exact_after}"
  cmp -s "${capture_vault}/exact_inputs.before.sha256" \
    "${exact_after}" ||
    fail "exact input hashes changed during production capture"
  verify_live_closure
  local capture_complete_tmp="${capture_vault}/.capture.complete.$$"
  {
    echo "schema_id=${schema_id}"
    echo "capture_schema=synthetic_final_holdout_capture_complete.v1"
    echo "status=complete"
    echo "anchor_range=[1088,1170)"
    echo "anchor_count=82"
    echo "batch_count=2"
    echo "batch_sizes=64,18"
    echo "skipped_batches=0"
    echo "row_count=738"
    echo "checkpoint_written=false"
    echo "checkpoint_write_count=0"
    echo "representation_probe.written=true"
    echo "representation_probe.row_count=738"
    echo "representation_probe.path=${representation_probe}"
    echo "mdn_probe.written=true"
    echo "mdn_probe.row_count=738"
    echo "mdn_probe.path=${mdn_probe}"
    echo "nonfinite_output_count=0"
    echo "source_cursor_token=${expected_source_cursor_token}"
    echo "graph_order_fingerprint=d334e38b1887ae16"
    echo "wave_pulses_attempted=2"
    echo "wave_pulses_completed=2"
    echo "wave_pulses_skipped=0"
    echo "completion_origin=${completion_origin}"
    echo "production_capture_invocation_count=1"
    echo "representation_probe.sha256=$(sha256_of "${representation_probe}")"
    echo "mdn_probe.sha256=$(sha256_of "${mdn_probe}")"
    echo "capture_report.sha256=$(sha256_of "${capture_report}")"
    echo "job_manifest.sha256=$(sha256_of "${job_manifest}")"
    echo "runtime_result_fact.sha256=$(sha256_of "${runtime_result_fact}")"
    echo "capture_config.sha256=$(sha256_of "${capture_config}")"
    echo "capture_runtime_log.sha256=$(sha256_of "${capture_runtime_log}")"
    echo "capture_invocation_log.sha256=$(sha256_of "${capture_invocation_log}")"
    echo "exact_inputs_before.sha256=$(sha256_of "${capture_vault}/exact_inputs.before.sha256")"
    echo "exact_inputs_after.sha256=$(sha256_of "${exact_after}")"
    echo "recapture_permitted=false"
  } >"${capture_complete_tmp}"
  install_file_noreplace_atomic "${capture_complete_tmp}" \
    "${capture_complete}"
}

validate_capture_complete() {
  set_vault_paths
  require_kv "${capture_complete}" schema_id "${schema_id}"
  require_kv "${capture_complete}" status complete
  require_kv "${capture_complete}" anchor_range '[1088,1170)'
  require_kv "${capture_complete}" anchor_count 82
  require_kv "${capture_complete}" batch_count 2
  require_kv "${capture_complete}" batch_sizes 64,18
  require_kv "${capture_complete}" skipped_batches 0
  require_kv "${capture_complete}" row_count 738
  require_kv "${capture_complete}" checkpoint_written false
  require_kv "${capture_complete}" checkpoint_write_count 0
  require_kv "${capture_complete}" representation_probe.written true
  require_kv "${capture_complete}" representation_probe.row_count 738
  require_kv "${capture_complete}" representation_probe.path \
    "${representation_probe}"
  require_kv "${capture_complete}" mdn_probe.written true
  require_kv "${capture_complete}" mdn_probe.row_count 738
  require_kv "${capture_complete}" mdn_probe.path "${mdn_probe}"
  require_kv "${capture_complete}" nonfinite_output_count 0
  require_kv "${capture_complete}" source_cursor_token \
    "${expected_source_cursor_token}"
  require_kv "${capture_complete}" graph_order_fingerprint d334e38b1887ae16
  require_kv "${capture_complete}" wave_pulses_attempted 2
  require_kv "${capture_complete}" wave_pulses_completed 2
  require_kv "${capture_complete}" wave_pulses_skipped 0
  local completion_origin
  completion_origin="$(kv completion_origin "${capture_complete}")"
  case "${completion_origin}" in
  initial_capture_return | recovered_completed_capture_without_recapture) ;;
  *) fail "unexpected capture.complete origin: ${completion_origin}" ;;
  esac
  require_kv "${capture_complete}" production_capture_invocation_count 1
  require_kv "${capture_complete}" recapture_permitted false
  require_sha256 "${representation_probe}" \
    "$(kv representation_probe.sha256 "${capture_complete}")"
  require_sha256 "${mdn_probe}" \
    "$(kv mdn_probe.sha256 "${capture_complete}")"
  require_sha256 "${capture_report}" \
    "$(kv capture_report.sha256 "${capture_complete}")"
  require_sha256 "${job_manifest}" \
    "$(kv job_manifest.sha256 "${capture_complete}")"
  require_sha256 "${runtime_result_fact}" \
    "$(kv runtime_result_fact.sha256 "${capture_complete}")"
  require_sha256 "${capture_config}" \
    "$(kv capture_config.sha256 "${capture_complete}")"
  require_sha256 "${capture_runtime_log}" \
    "$(kv capture_runtime_log.sha256 "${capture_complete}")"
  require_sha256 "${capture_invocation_log}" \
    "$(kv capture_invocation_log.sha256 "${capture_complete}")"
  require_sha256 "${capture_vault}/exact_inputs.before.sha256" \
    "$(kv exact_inputs_before.sha256 "${capture_complete}")"
  require_sha256 "${capture_vault}/exact_inputs.after.sha256" \
    "$(kv exact_inputs_after.sha256 "${capture_complete}")"
  cmp -s "${capture_vault}/exact_inputs.before.sha256" \
    "${capture_vault}/exact_inputs.after.sha256" ||
    fail "before/after exact input manifests differ"
  validate_capture_metadata
}

recover_capture_complete_without_recapture() {
  set_vault_paths
  [[ ! -e "${capture_complete}" ]] ||
    fail "capture.complete already exists; recovery is not applicable"
  validate_ledger
  validate_capture_metadata
  write_capture_complete recovered_completed_capture_without_recapture
  validate_capture_complete
}

run_capture_once() {
  set_vault_paths
  local capture_schema_id
  capture_schema_id="$(basename "${capture_vault}")"
  # Capture checkpoints retain their canonical origin paths because both the
  # production report and sealed helper require that complete identity.
  env \
    CUBLAS_WORKSPACE_CONFIG=:4096:8 \
    CUDA_DEVICE_ORDER=PCI_BUS_ID \
    SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_BEGIN=1088 \
    SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_END=1170 \
    SYNTHETIC_MDN_FROZEN_CAPTURE_ALLOW_UNCONSUMED_HOLDOUT=true \
    SYNTHETIC_MDN_FROZEN_CAPTURE_SCHEMA_ID="${capture_schema_id}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_JOB_DIR="${capture_job}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_CONFIG_PATH="${capture_config}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_BASE_CONFIG="${frozen_inputs}/base.config" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_REPRESENTATION_CHECKPOINT="${representation_checkpoint}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_MDN_CHECKPOINT="${mdn_checkpoint}" \
    "${capture_runner}" >"${capture_invocation_log}" 2>&1
  write_capture_complete initial_capture_return
}

validate_final_report() {
  local report="$1"
  local representation_sha mdn_sha capture_report_sha job_manifest_sha
  representation_sha="$(kv representation_probe.sha256 "${capture_complete}")"
  mdn_sha="$(kv mdn_probe.sha256 "${capture_complete}")"
  capture_report_sha="$(kv capture_report.sha256 "${capture_complete}")"
  job_manifest_sha="$(kv job_manifest.sha256 "${capture_complete}")"
  require_file "${report}"
  python3 - "${report}" "${schema_id}" "${representation_sha}" "${mdn_sha}" \
    "${capture_report_sha}" "${job_manifest_sha}" <<'PY'
import math
import pathlib
import re
import sys

path = pathlib.Path(sys.argv[1])
schema = sys.argv[2]
representation_sha, mdn_sha, capture_report_sha, job_manifest_sha = sys.argv[3:7]
values = {}
for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
    if not raw or "=" not in raw:
        raise SystemExit(f"malformed final report line {line_number}")
    key, value = raw.split("=", 1)
    if not key or key in values:
        raise SystemExit(f"duplicate or empty final report key at line {line_number}")
    values[key] = value

expected = {
    "schema_id": schema,
    "status": "complete",
    "result_role": "sealed_final_holdout_no_refit_evaluation",
    "scientific_result_published_regardless_of_gate_outcome": "true",
    "candidate.count": "1",
    "candidate.semantic_tensor_digest":
        "84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38",
    "candidate.fit_range": "[0,554)",
    "candidate.purge_range": "[554,584)",
    "candidate.selection_range": "[584,730)",
    "candidate.maximum_fit_or_selection_anchor": "729",
    "candidate.run_only": "true",
    "candidate.policy_eligible": "false",
    "boundary.final_holdout": "[1088,1170)",
    "boundary.minimum_anchor_loaded": "1088",
    "boundary.maximum_anchor_loaded": "1169",
    "data.anchor_count": "82",
    "data.batch_count": "2",
    "data.batch_sizes": "64,18",
    "data.skipped_batches": "0",
    "data.row_count": "738",
    "data.representation_shape": "[82,4,3,32]",
    "data.feature_shape": "[82,3,3,400]",
    "data.target_shape": "[82,3,3]",
    "data.target_torch_equal_across_probes": "true",
    "data.target_raw_bytes_equal_across_probes": "true",
    "data.dynamic_prefix_torch_equal": "true",
    "data.dynamic_prefix_raw_bytes_equal": "true",
    "input.representation_probe.sha256": representation_sha,
    "input.mdn_probe.sha256": mdn_sha,
    "input.capture_report.sha256": capture_report_sha,
    "input.job_manifest.sha256": job_manifest_sha,
    "input.representation_checkpoint.sha256":
        "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de",
    "input.mdn_checkpoint.sha256":
        "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e",
    "input.v2_artifact.sha256":
        "7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739",
    "input.hashes_identical_before_after": "true",
    "execution.production_checkpoint_loader_executed": "true",
    "execution.checkpoint_expected_identity_nonnull": "true",
    "execution.checkpoint_full_expected_identity_populated": "true",
    "execution.mdn_direct_feature_forward_executed": "true",
    "execution.mdn_distribution_forward_executed": "false",
    "execution.checkpoint_head_forward_executed": "true",
    "execution.conditioned_head_forward_executed": "true",
    "execution.training_performed": "false",
    "execution.refit_performed": "false",
    "execution.optimizer_steps": "0",
    "execution.checkpoint_written": "false",
    "execution.policy_path_used": "false",
    "execution.no_grad": "true",
    "execution.model_eval": "true",
    "execution.checkpoint_head_eval": "true",
    "execution.conditioned_head_eval": "true",
    "execution.conditioned_parameters_frozen": "true",
    "execution.device": "cuda",
    "execution.live_input_dtype": "float32",
    "parity.conditioned_suffix_ignored.torch_equal": "true",
    "parity.conditioned_suffix_ignored.raw_bytes_equal": "true",
    "parity.suffix_input_genuinely_changed": "true",
    "replay.internal_payload_byte_identical": "true",
    "replay.checkpoint_prediction_torch_equal": "true",
    "replay.conditioned_prediction_torch_equal": "true",
    "replay.suffix_prediction_torch_equal": "true",
    "validity.external_runner_closure_manifest.required": "true",
    "validity.external_empty_success_logs.required": "true",
    "validity.external_primary_replay_report_identity.required": "true",
    "interpretation.requires_runner_closure_pass": "true",
    "scope.validates_mdn_distribution_output": "false",
    "scope.validates_policy": "false",
    "scope.validates_forecast_to_action_path": "false",
    "scope.validates_other_synthetic_seeds": "false",
    "scope.validates_real_market_data": "false",
    "conclusion.final_interval_consumed": "true",
    "conclusion.candidate_may_not_be_tuned_on_this_interval": "true",
}
for key, expected_value in expected.items():
    actual = values.get(key)
    if actual != expected_value:
        raise SystemExit(f"unexpected {key}: {actual!r}, expected {expected_value!r}")

validity_keys = (
    "anchor_range", "anchor_count", "batch_shape", "zero_skipped_batches",
    "target_and_rank_counts", "per_edge_and_channel_counts",
    "dynamic_prefix_bit_exact", "conditioned_suffix_ignored",
    "production_checkpoint_identity", "eval_no_grad_cuda_float32",
    "no_training_refit_optimizer", "no_checkpoint_or_policy",
    "input_hash_stability", "predictions_and_metrics_finite",
    "internal_primary_replay", "helper_level",
)
for suffix in validity_keys:
    key = f"validity.{suffix}.pass"
    if values.get(key) != "true":
        raise SystemExit(f"helper validity did not pass: {key}")

metric_fields = (
    "mae", "rmse", "directional_accuracy", "pairwise_rank_accuracy",
    "correlation", "prediction_population_std",
)
numeric_keys = [
    "data.representation_base_quote_difference.maximum_abs_delta",
    "data.representation_quote_cross_edge.maximum_abs_delta",
    "data.dynamic_prefix_maximum_abs_delta",
    "data.full_recomputed_to_cached_maximum_abs_delta",
    "data.full_recomputed_to_cached_descriptive_tolerance",
    "parity.conditioned_suffix_ignored.maximum_abs_delta",
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
]
for arm in ("checkpoint", "conditioned"):
    if values.get(f"final.{arm}.count") != "738":
        raise SystemExit(f"unexpected final {arm} count")
    if values.get(f"final.{arm}.pair_count") != "738":
        raise SystemExit(f"unexpected final {arm} pair count")
    numeric_keys.extend(f"final.{arm}.{field}" for field in metric_fields)
    for group in ("per_edge", "per_channel"):
        for index in range(3):
            prefix = f"final.{arm}.{group}.{index}"
            if values.get(f"{prefix}.count") != "246":
                raise SystemExit(f"unexpected count: {prefix}")
            expected_pair_count = "0" if group == "per_edge" else "246"
            if values.get(f"{prefix}.pair_count") != expected_pair_count:
                raise SystemExit(f"unexpected pair count: {prefix}")
            numeric_keys.extend(f"{prefix}.{field}" for field in metric_fields)
    objective = f"production_direct_objective.{arm}"
    if values.get(f"{objective}.valid_count") != "738":
        raise SystemExit(f"unexpected objective valid count: {arm}")
    if values.get(f"{objective}.pairwise_valid_count") != "738":
        raise SystemExit(f"unexpected objective pair count: {arm}")
    numeric_keys.extend(
        f"{objective}.{field}" for field in ("total", "regression", "direction", "rank")
    )
    descriptive = f"final.{arm}.descriptive"
    numeric_keys.extend(
        f"{descriptive}.{field}" for field in (
            "signed_error", "target_population_std",
            "prediction_to_target_std_ratio", "best_asset_agreement",
            "margin_epsilon", "margin_directional_accuracy",
            "rank_margin_accuracy",
        )
    )

numbers = {}
for key in numeric_keys:
    if key not in values:
        raise SystemExit(f"missing required numeric field: {key}")
    number = float(values[key])
    if not math.isfinite(number):
        raise SystemExit(f"non-finite required field: {key}={values[key]!r}")
    numbers[key] = number

pinned_thresholds = {
    "scientific_gate.threshold.minimum_direction": 0.65,
    "scientific_gate.threshold.minimum_rank": 0.65,
    "scientific_gate.threshold.minimum_correlation": 0.50,
    "scientific_gate.threshold.maximum_rmse": 0.025,
    "scientific_gate.threshold.minimum_direction_improvement": 0.10,
    "scientific_gate.threshold.minimum_rank_improvement": 0.10,
    "scientific_gate.threshold.maximum_rmse_ratio": 0.90,
}
for key, pinned in pinned_thresholds.items():
    if numbers[key] != pinned:
        raise SystemExit(
            f"scientific threshold drift: {key}={numbers[key]!r}, expected {pinned!r}"
        )

if numbers["data.dynamic_prefix_maximum_abs_delta"] != 0.0:
    raise SystemExit("dynamic prefix maximum delta is nonzero")
if numbers["parity.conditioned_suffix_ignored.maximum_abs_delta"] != 0.0:
    raise SystemExit("conditioned suffix perturbation changed predictions")
if numbers["data.full_recomputed_to_cached_maximum_abs_delta"] > numbers[
    "data.full_recomputed_to_cached_descriptive_tolerance"
]:
    raise SystemExit("full recomputed feature drift exceeds descriptive tolerance")

for key in (
    "prediction.checkpoint.digest", "prediction.conditioned.digest",
    "prediction.conditioned_suffix_perturbed.digest",
    "replay.internal_evaluation_payload.sha256",
):
    if not re.fullmatch(r"[0-9a-f]{64}", values.get(key, "")):
        raise SystemExit(f"invalid prediction/replay digest: {key}")
if values["prediction.conditioned.digest"] != values[
    "prediction.conditioned_suffix_perturbed.digest"
]:
    raise SystemExit("conditioned and suffix-perturbed digests differ")

direction_delta = (
    numbers["final.conditioned.directional_accuracy"]
    - numbers["final.checkpoint.directional_accuracy"]
)
rank_delta = (
    numbers["final.conditioned.pairwise_rank_accuracy"]
    - numbers["final.checkpoint.pairwise_rank_accuracy"]
)
checkpoint_rmse = numbers["final.checkpoint.rmse"]
checkpoint_rmse_positive = checkpoint_rmse > 0.0 and math.isfinite(checkpoint_rmse)
rmse_ratio = (
    numbers["final.conditioned.rmse"] / checkpoint_rmse
    if checkpoint_rmse_positive
    else sys.float_info.max
)
for actual, reported, label in (
    (direction_delta, numbers["improvement.directional_accuracy"], "direction"),
    (rank_delta, numbers["improvement.pairwise_rank_accuracy"], "rank"),
    (rmse_ratio, numbers["improvement.rmse_ratio"], "rmse ratio"),
):
    if actual != reported and abs(actual - reported) > 1.0e-12:
        raise SystemExit(f"inconsistent {label} improvement")

gate_truth = {
    "scientific_gate.direction.pass":
        numbers["final.conditioned.directional_accuracy"]
        >= numbers["scientific_gate.threshold.minimum_direction"],
    "scientific_gate.rank.pass":
        numbers["final.conditioned.pairwise_rank_accuracy"]
        >= numbers["scientific_gate.threshold.minimum_rank"],
    "scientific_gate.correlation.pass":
        numbers["final.conditioned.correlation"]
        >= numbers["scientific_gate.threshold.minimum_correlation"],
    "scientific_gate.rmse.pass":
        numbers["final.conditioned.rmse"]
        <= numbers["scientific_gate.threshold.maximum_rmse"],
    "scientific_gate.direction_improvement.pass":
        direction_delta
        >= numbers["scientific_gate.threshold.minimum_direction_improvement"],
    "scientific_gate.rank_improvement.pass":
        rank_delta
        >= numbers["scientific_gate.threshold.minimum_rank_improvement"],
    "scientific_gate.rmse_improvement.pass":
        checkpoint_rmse_positive
        and rmse_ratio <= numbers["scientific_gate.threshold.maximum_rmse_ratio"],
}
for key, truth in gate_truth.items():
    if values.get(key) != ("true" if truth else "false"):
        raise SystemExit(f"inconsistent scientific gate: {key}")
absolute = all(gate_truth[key] for key in (
    "scientific_gate.direction.pass", "scientific_gate.rank.pass",
    "scientific_gate.correlation.pass", "scientific_gate.rmse.pass",
))
improvement = all(gate_truth[key] for key in (
    "scientific_gate.direction_improvement.pass",
    "scientific_gate.rank_improvement.pass",
    "scientific_gate.rmse_improvement.pass",
))
overall = absolute and improvement
for key, truth in (
    ("scientific_gate.absolute.pass", absolute),
    ("scientific_gate.improvement.pass", improvement),
    ("scientific_gate.pass", overall),
    ("interpretation.full_acceptance", overall),
):
    if values.get(key) != ("true" if truth else "false"):
        raise SystemExit(f"inconsistent aggregate scientific gate: {key}")
if not values.get("interpretation.outcome"):
    raise SystemExit("missing interpretation outcome")
PY
}

run_helper_once() {
  local destination="$1"
  local log="$2"
  set_vault_paths
  [[ ! -e "${destination}" && ! -e "${log}" ]] ||
    fail "refusing to replace an evaluation output or log"
  env \
    CUBLAS_WORKSPACE_CONFIG=:4096:8 \
    CUDA_DEVICE_ORDER=PCI_BUS_ID \
    "${helper_bin}" \
    --representation-probe "${representation_probe}" \
    --mdn-probe "${mdn_probe}" \
    --capture-report "${capture_report}" \
    --job-manifest "${job_manifest}" \
    --representation-checkpoint "${frozen_inputs}/representation_checkpoint.pt" \
    --mdn-checkpoint "${frozen_inputs}/mdn_checkpoint.pt" \
    --v2-artifact "${frozen_inputs}/conditioned_artifact.pt" \
    --representation-probe-sha256 \
    "$(kv representation_probe.sha256 "${capture_complete}")" \
    --mdn-probe-sha256 "$(kv mdn_probe.sha256 "${capture_complete}")" \
    --capture-report-sha256 \
    "$(kv capture_report.sha256 "${capture_complete}")" \
    --job-manifest-sha256 \
    "$(kv job_manifest.sha256 "${capture_complete}")" \
    --schema-id "${schema_id}" \
    --output "${destination}" >"${log}" 2>&1
}

validate_evaluation_attempt() {
  local attempt="$1"
  local primary_report="${attempt}/primary/${schema_id}.report"
  local replay_report="${attempt}/replay/${schema_id}.report"
  local primary_log="${attempt}/primary/${schema_id}.stdout.log"
  local replay_log="${attempt}/replay/${schema_id}.stdout.log"
  require_file "${primary_report}"
  require_file "${replay_report}"
  require_file "${primary_log}"
  require_file "${replay_log}"
  [[ ! -s "${primary_log}" && ! -s "${replay_log}" ]] ||
    fail "successful primary/replay helper logs must be empty"
  cmp -s "${primary_report}" "${replay_report}" ||
    fail "external primary/replay reports are not byte-identical"
  validate_final_report "${primary_report}"
  validate_final_report "${replay_report}"
}

selected_attempt_path() {
  set_vault_paths
  if [[ -f "${selected_evaluation}" ]]; then
    local relative
    relative="$(kv attempt_relative_path "${selected_evaluation}")"
    [[ "${relative}" == evaluation/attempt_[0-9][0-9][0-9][0-9][0-9][0-9] &&
      "${relative}" != *..* ]] ||
      fail "selected evaluation path is malformed"
    local selected="${capture_vault}/${relative}"
    validate_evaluation_attempt "${selected}"
    printf '%s\n' "${selected}"
    return
  fi

  local index attempt=""
  for index in $(seq 1 999); do
    attempt="${capture_vault}/evaluation/attempt_$(printf '%06d' "${index}")"
    [[ -e "${attempt}" ]] || break
    attempt=""
  done
  [[ -n "${attempt}" ]] || fail "no unused evaluation attempt remains"
  mkdir -p "${attempt}/primary" "${attempt}/replay"
  run_helper_once "${attempt}/primary/${schema_id}.report" \
    "${attempt}/primary/${schema_id}.stdout.log"
  run_helper_once "${attempt}/replay/${schema_id}.report" \
    "${attempt}/replay/${schema_id}.stdout.log"
  validate_evaluation_attempt "${attempt}"
  local selected_evaluation_tmp="${capture_vault}/.selected_evaluation.$$"
  {
    echo "schema_id=${schema_id}"
    echo "status=complete"
    echo "attempt_relative_path=${attempt#${capture_vault}/}"
    echo "primary_report.sha256=$(sha256_of "${attempt}/primary/${schema_id}.report")"
    echo "replay_report.sha256=$(sha256_of "${attempt}/replay/${schema_id}.report")"
    echo "reports.byte_identical=true"
    echo "success_logs.empty=true"
    echo "scientific_gate.pass=$(kv scientific_gate.pass "${attempt}/primary/${schema_id}.report")"
  } >"${selected_evaluation_tmp}"
  chmod -R a-w "${attempt}"
  install_file_noreplace_atomic "${selected_evaluation_tmp}" \
    "${selected_evaluation}"
  printf '%s\n' "${attempt}"
}

selected_existing_attempt() {
  set_vault_paths
  require_file "${selected_evaluation}"
  local relative
  relative="$(kv attempt_relative_path "${selected_evaluation}")"
  [[ "${relative}" == evaluation/attempt_[0-9][0-9][0-9][0-9][0-9][0-9] &&
    "${relative}" != *..* ]] ||
    fail "selected evaluation path is malformed"
  local selected="${capture_vault}/${relative}"
  validate_evaluation_attempt "${selected}"
  printf '%s\n' "${selected}"
}

ensure_vault_master() {
  set_vault_paths
  if [[ -f "${vault_master}" ]]; then
    (
      cd "${capture_vault}"
      sha256sum -c "${vault_master}" >/dev/null
    )
    return
  fi
  require_file "${selected_evaluation}"
  local vault_master_tmp="${capture_vault}/.vault_master.$$"
  (
    cd "${capture_vault}"
    find . -type f ! -name vault_master.sha256 ! -name '.vault_master.*' \
      -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum \
      >"${vault_master_tmp}"
  )
  install_file_noreplace_atomic "${vault_master_tmp}" "${vault_master}"
  (
    cd "${capture_vault}"
    sha256sum -c "${vault_master}" >/dev/null
  )
  find "${capture_vault}" -type f -exec chmod a-w {} +
}

verify_vault_master() {
  set_vault_paths
  require_file "${vault_master}"
  (
    cd "${capture_vault}"
    sha256sum -c "${vault_master}" >/dev/null
  )
}

publish_runtime_package() {
  local attempt="$1"
  set_vault_paths
  [[ ! -e "${runtime_dir}" ]] || fail "final runtime package already exists"
  publish_scratch="${runtime_base}/${schema_id}.publish.$$"
  [[ ! -e "${publish_scratch}" ]] ||
    fail "publish scratch already exists: ${publish_scratch}"
  mkdir "${publish_scratch}"
  cp "${attempt}/primary/${schema_id}.report" \
    "${publish_scratch}/${schema_id}.report"
  cp "${capture_complete}" "${publish_scratch}/capture.complete"
  cp "${local_ledger}" "${publish_scratch}/consumption.ledger"
  cp "${selected_evaluation}" \
    "${publish_scratch}/selected_evaluation.status"
  cp "${capture_vault}/execution_scope.status" \
    "${publish_scratch}/execution_scope.status"
  {
    echo "schema_id=${schema_id}"
    echo "status=complete"
    echo "capture_vault=${capture_vault}"
    echo "capture_vault_master.sha256=$(sha256_of "${vault_master}")"
    echo "capture_complete.sha256=$(sha256_of "${capture_complete}")"
    echo "consumption_ledger.sha256=$(sha256_of "${local_ledger}")"
    echo "selected_evaluation.sha256=$(sha256_of "${selected_evaluation}")"
    echo "scientific_gate.pass=$(kv scientific_gate.pass "${attempt}/primary/${schema_id}.report")"
    echo "scientific_result_published_regardless_of_gate_outcome=true"
  } >"${publish_scratch}/vault_reference.status"
  (
    cd "${publish_scratch}"
    find . -type f ! -name master.sha256 -print0 |
      LC_ALL=C sort -z | xargs -0 -r sha256sum >master.sha256
  )
  find "${publish_scratch}" -type f -exec chmod a-w {} +
  mv -T -n -- "${publish_scratch}" "${runtime_dir}"
  [[ ! -e "${publish_scratch}" && -d "${runtime_dir}" ]] ||
    fail "no-clobber final runtime package publication failed"
  publish_scratch=""
}

verify_runtime_package() {
  set_vault_paths
  require_dir "${runtime_dir}"
  require_file "${runtime_dir}/master.sha256"
  require_file "${runtime_dir}/${schema_id}.report"
  (
    cd "${runtime_dir}"
    sha256sum -c master.sha256 >/dev/null
  )
  require_kv "${runtime_dir}/vault_reference.status" schema_id "${schema_id}"
  require_kv "${runtime_dir}/vault_reference.status" status complete
  require_kv "${runtime_dir}/vault_reference.status" capture_vault \
    "${capture_vault}"
  require_kv "${runtime_dir}/vault_reference.status" \
    scientific_result_published_regardless_of_gate_outcome true
  require_kv "${runtime_dir}/vault_reference.status" scientific_gate.pass \
    "$(kv scientific_gate.pass "${runtime_dir}/${schema_id}.report")"
  require_sha256 "${vault_master}" \
    "$(kv capture_vault_master.sha256 "${runtime_dir}/vault_reference.status")"
  require_sha256 "${capture_complete}" \
    "$(kv capture_complete.sha256 "${runtime_dir}/vault_reference.status")"
  require_sha256 "${local_ledger}" \
    "$(kv consumption_ledger.sha256 "${runtime_dir}/vault_reference.status")"
  require_sha256 "${selected_evaluation}" \
    "$(kv selected_evaluation.sha256 "${runtime_dir}/vault_reference.status")"
  cmp -s "${runtime_dir}/capture.complete" "${capture_complete}" ||
    fail "runtime package capture fact differs from the vault"
  cmp -s "${runtime_dir}/consumption.ledger" "${local_ledger}" ||
    fail "runtime package ledger differs from the immutable ledger"
  cmp -s "${runtime_dir}/selected_evaluation.status" \
    "${selected_evaluation}" ||
    fail "runtime package selected evaluation differs from the vault"
  local attempt
  attempt="$(selected_existing_attempt)"
  cmp -s "${runtime_dir}/${schema_id}.report" \
    "${attempt}/primary/${schema_id}.report" ||
    fail "runtime report differs from the selected primary report"
  validate_final_report "${runtime_dir}/${schema_id}.report"
}

install_manifest_noreplace() {
  local temporary="$1"
  local destination="$2"
  install_file_noreplace_atomic "${temporary}" "${destination}"
}

finalize_publication() {
  set_vault_paths
  verify_runtime_package
  local runtime_report="${runtime_dir}/${schema_id}.report"
  if [[ -e "${installed_report}" ]]; then
    require_file "${installed_report}"
    cmp -s "${runtime_report}" "${installed_report}" ||
      fail "installed final report differs from the runtime report"
  else
    ln "${runtime_report}" "${installed_report}" ||
      fail "no-clobber installed-report publication failed"
  fi

  local report_manifest="${runtime_dir}/report_and_master.sha256"
  if [[ ! -e "${report_manifest}" ]]; then
    local report_manifest_tmp="${runtime_dir}/.report_and_master.$$"
    (
      cd "${repo_root}"
      sha256sum \
        "${installed_report#${repo_root}/}" \
        "${runtime_dir#${repo_root}/}/master.sha256" \
        "${capture_vault#${repo_root}/}/vault_master.sha256" \
        "${ledger_path#${repo_root}/}" \
        "${capture_complete#${repo_root}/}" >"${report_manifest_tmp}"
    )
    install_manifest_noreplace "${report_manifest_tmp}" "${report_manifest}"
  fi
  (
    cd "${repo_root}"
    sha256sum -c "${report_manifest}" >/dev/null
  )

  local publication="${runtime_dir}/publication.complete"
  if [[ ! -e "${publication}" ]]; then
    local publication_tmp="${runtime_dir}/.publication.complete.$$"
    {
      echo "schema_id=${schema_id}"
      echo "status=complete"
      echo "validity.external_runner_closure_manifest.pass=true"
      echo "validity.external_empty_success_logs.pass=true"
      echo "validity.external_primary_replay_report_identity.pass=true"
      echo "scientific_gate.pass=$(kv scientific_gate.pass "${runtime_report}")"
      echo "scientific_result_published_regardless_of_gate_outcome=true"
      echo "capture_replayed=false"
      echo "final_interval_consumed=true"
    } >"${publication_tmp}"
    install_manifest_noreplace "${publication_tmp}" "${publication}"
  fi
  require_kv "${publication}" schema_id "${schema_id}"
  require_kv "${publication}" status complete
  require_kv "${publication}" \
    validity.external_runner_closure_manifest.pass true
  require_kv "${publication}" validity.external_empty_success_logs.pass true
  require_kv "${publication}" \
    validity.external_primary_replay_report_identity.pass true
  require_kv "${publication}" \
    scientific_result_published_regardless_of_gate_outcome true
  require_kv "${publication}" scientific_gate.pass \
    "$(kv scientific_gate.pass "${runtime_report}")"
  require_kv "${publication}" capture_replayed false
  require_kv "${publication}" final_interval_consumed true
  cmp -s "${runtime_report}" "${installed_report}" ||
    fail "installed and runtime reports are not byte-identical"

  local final_manifest="${runtime_dir}/final_publication.sha256"
  if [[ ! -e "${final_manifest}" ]]; then
    local final_manifest_tmp="${runtime_dir}/.final_publication.$$"
    (
      cd "${repo_root}"
      sha256sum \
        "${installed_report#${repo_root}/}" \
        "${runtime_dir#${repo_root}/}/master.sha256" \
        "${runtime_dir#${repo_root}/}/report_and_master.sha256" \
        "${runtime_dir#${repo_root}/}/publication.complete" \
        "${capture_vault#${repo_root}/}/vault_master.sha256" \
        "${ledger_path#${repo_root}/}" \
        "${capture_complete#${repo_root}/}" >"${final_manifest_tmp}"
    )
    install_manifest_noreplace "${final_manifest_tmp}" "${final_manifest}"
  fi
  (
    cd "${repo_root}"
    sha256sum -c "${final_manifest}" >/dev/null
  )
  find "${runtime_dir}" -type f -exec chmod a-w {} +
  sync -f "${installed_report}"
}

verify_publication() {
  set_vault_paths
  verify_runtime_package
  require_file "${installed_report}"
  cmp -s "${runtime_dir}/${schema_id}.report" "${installed_report}" ||
    fail "installed report differs from the runtime report"
  require_file "${runtime_dir}/report_and_master.sha256"
  (
    cd "${repo_root}"
    sha256sum -c "${runtime_dir}/report_and_master.sha256" >/dev/null
  )
  require_kv "${runtime_dir}/publication.complete" schema_id "${schema_id}"
  require_kv "${runtime_dir}/publication.complete" status complete
  require_kv "${runtime_dir}/publication.complete" \
    validity.external_runner_closure_manifest.pass true
  require_kv "${runtime_dir}/publication.complete" \
    validity.external_empty_success_logs.pass true
  require_kv "${runtime_dir}/publication.complete" \
    validity.external_primary_replay_report_identity.pass true
  require_kv "${runtime_dir}/publication.complete" \
    scientific_result_published_regardless_of_gate_outcome true
  require_kv "${runtime_dir}/publication.complete" scientific_gate.pass \
    "$(kv scientific_gate.pass "${runtime_dir}/${schema_id}.report")"
  require_kv "${runtime_dir}/publication.complete" capture_replayed false
  require_kv "${runtime_dir}/publication.complete" final_interval_consumed true
  require_file "${runtime_dir}/final_publication.sha256"
  (
    cd "${repo_root}"
    sha256sum -c "${runtime_dir}/final_publication.sha256" >/dev/null
  )
}

acquire_lock() {
  mkdir "${lock_dir}" 2>/dev/null || fail "canonical final-holdout lock is held"
  lock_owned=true
}

assert_initial_payload_absent() {
  [[ ! -e "${runtime_dir}" ]] || fail "final runtime result already exists"
  [[ ! -e "${capture_vault}" ]] || fail "final capture vault already exists"
  [[ ! -e "${ledger_path}" ]] || fail "final consumption ledger already exists"
  [[ ! -e "${installed_report}" ]] || fail "final installed report already exists"
}

complete_evaluation_and_publication() {
  set_vault_paths
  local attempt
  if [[ -d "${runtime_dir}" ]]; then
    verify_runtime_package
  else
    attempt="$(selected_attempt_path)"
    ensure_vault_master
    publish_runtime_package "${attempt}"
  fi
  finalize_publication
  verify_publication
}

run_initial() {
  preflight_common
  assert_exact_reserved_paths_absent
  acquire_lock
  assert_initial_payload_absent
  mkdir "${capture_vault}"
  vault_created=true
  freeze_closure
  require_protocol_seal
  verify_live_closure
  set_vault_paths
  [[ ! -e "${capture_job}" && ! -e "${capture_config}" &&
    ! -e "${capture_complete}" ]] ||
    fail "capture vault was not sealed in an unopened state"

  # Nothing may be inserted between this durable ledger link and the sole
  # production capture call. Any failure after the link preserves the vault.
  create_consumption_ledger
  run_capture_once

  validate_ledger
  validate_capture_complete
  complete_evaluation_and_publication
  echo "canonical final-holdout run complete: ${schema_id}"
}

resume_evaluate_only() {
  preflight_common
  [[ ! -e "${lock_dir}" ]] || fail "canonical final-holdout lock is held"
  acquire_lock
  consumption_committed=true
  set_vault_paths
  require_dir "${capture_vault}"
  require_file "${ledger_path}"
  [[ ! -e "${installed_report}" || -d "${runtime_dir}" ]] ||
    fail "installed report exists without its runtime package"
  validate_ledger
  require_protocol_seal
  if [[ -e "${capture_complete}" ]]; then
    validate_capture_complete
  else
    recover_capture_complete_without_recapture
  fi
  verify_live_closure
  complete_evaluation_and_publication
  echo "evaluate-only resume complete; production source was not recaptured"
}

verify_runtime() {
  [[ ! -e "${lock_dir}" ]] || fail "cannot verify while canonical lock is held"
  consumption_committed=true
  preflight_common
  set_vault_paths
  require_dir "${capture_vault}"
  require_dir "${runtime_dir}"
  require_file "${ledger_path}"
  require_file "${installed_report}"
  validate_ledger
  validate_capture_complete
  require_protocol_seal
  verify_live_closure
  verify_vault_master
  verify_publication
  echo "verification passed: ${schema_id}"
}

usage() {
  echo "usage: $0 --preflight|--run|--resume-evaluate-only|--verify" >&2
  exit 2
}

[[ $# -eq 1 ]] || usage
case "$1" in
--preflight)
  preflight
  ;;
--run)
  run_initial
  ;;
--resume-evaluate-only)
  resume_evaluate_only
  ;;
--verify)
  verify_runtime
  ;;
*)
  usage
  ;;
esac
