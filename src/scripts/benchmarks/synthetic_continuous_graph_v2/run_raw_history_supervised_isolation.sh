#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_v2_raw_history_supervised_isolation_v1"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
final_runtime_dir="${runtime_parent}/${schema_id}"
lock_dir="${final_runtime_dir}.lock"

helper_name="raw_history_supervised_isolation.cpp"
runner_name="run_raw_history_supervised_isolation.sh"
helper_source="${script_dir}/${helper_name}"
runner_source="${script_dir}/${runner_name}"

expected_1d_rows=3294
expected_3d_rows=1098
expected_1w_rows=471
development_prefix_root="${SYNTHETIC_V2_DEVELOPMENT_PREFIX_ROOT:-}"
alpha_prefix="${SYNTHETIC_V2_ALPHA_1D_PREFIX:-}"
beta_prefix="${SYNTHETIC_V2_BETA_1D_PREFIX:-}"
gamma_prefix="${SYNTHETIC_V2_GAMMA_1D_PREFIX:-}"

libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
benchmark_root=""
closure_report=""
training_policy=""
training_policy_sha256=""
mode=""
lock_held=false
declare -a all_prefix_files=()

fail() {
  echo "error: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'EOF'
usage:
  run_raw_history_supervised_isolation.sh {--preflight|--run} \
    --development-prefix-root ABSOLUTE_PATH_ENDING_IN/data/development_prefix \
    --alpha-prefix ABSOLUTE_1D_PREFIX.csv \
    --beta-prefix ABSOLUTE_1D_PREFIX.csv \
    --gamma-prefix ABSOLUTE_1D_PREFIX.csv

  run_raw_history_supervised_isolation.sh --verify

The equivalent SYNTHETIC_V2_* environment variables are also accepted.
Set SYNTHETIC_V2_RAW_HISTORY_CPU=true to force the deterministic CPU path.
EOF
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --preflight | --run | --verify)
    [[ -z "${mode}" ]] || fail "more than one mode was supplied"
    mode="$1"
    shift
    ;;
  --development-prefix-root)
    [[ $# -ge 2 ]] || usage
    development_prefix_root="$2"
    shift 2
    ;;
  --alpha-prefix)
    [[ $# -ge 2 ]] || usage
    alpha_prefix="$2"
    shift 2
    ;;
  --beta-prefix)
    [[ $# -ge 2 ]] || usage
    beta_prefix="$2"
    shift 2
    ;;
  --gamma-prefix)
    [[ $# -ge 2 ]] || usage
    gamma_prefix="$2"
    shift 2
    ;;
  *)
    usage
    ;;
  esac
done

[[ -n "${mode}" ]] || usage

require_regular_file() {
  [[ -f "$1" && ! -L "$1" ]] ||
    fail "missing or symlinked regular file: $1"
}

require_nonempty_file() {
  require_regular_file "$1"
  [[ -s "$1" ]] || fail "required file is empty: $1"
}

require_directory() {
  [[ -d "$1" && ! -L "$1" ]] ||
    fail "missing or symlinked directory: $1"
}

require_absolute_clean_path() {
  local path="$1"
  local label="$2"
  [[ "${path}" == /* ]] || fail "${label} must be absolute: ${path}"
  [[ "${path}" != *"//"* && "${path}" != *"/./"* &&
     "${path}" != *"/../"* && "${path}" != */. &&
     "${path}" != */.. ]] || fail "${label} is not lexically clean: ${path}"
}

reject_symlink_components() {
  local path="$1"
  local label="$2"
  local current=""
  local component
  local -a components=()

  require_absolute_clean_path "${path}" "${label}"
  IFS='/' read -r -a components <<<"${path#/}"
  for component in "${components[@]}"; do
    [[ -n "${component}" ]] || continue
    current="${current}/${component}"
    [[ ! -L "${current}" ]] ||
      fail "${label} traverses symlink component: ${current}"
  done
}

canonical_existing_path() {
  local path="$1"
  realpath -e -- "${path}" 2>/dev/null || fail "path does not exist: ${path}"
}

validate_exact_prefix_rows() {
  local path="$1"
  local expected="$2"
  local rows
  rows="$(awk 'END { print NR + 0 }' "${path}")"
  [[ "${rows}" == "${expected}" ]] ||
    fail "development-prefix row count mismatch (${rows} != ${expected}): ${path}"
}

validate_prefix_paths() {
  local canonical_root
  local canonical_alpha
  local canonical_beta
  local canonical_gamma
  local expected_alpha
  local expected_beta
  local expected_gamma
  local instrument
  local interval
  local prefix
  local canonical_prefix
  local expected_rows
  local -a instruments=(
    SYN2ALPHASYN2USD
    SYN2BETASYN2USD
    SYN2GAMMASYN2USD
  )
  local -a intervals=(1d 3d 1w)

  [[ -n "${development_prefix_root}" ]] ||
    fail "--development-prefix-root is required"
  [[ -n "${alpha_prefix}" && -n "${beta_prefix}" &&
     -n "${gamma_prefix}" ]] ||
    fail "all three explicit daily prefix paths are required"

  reject_symlink_components "${development_prefix_root}" \
    "development-prefix root"
  require_directory "${development_prefix_root}"
  canonical_root="$(canonical_existing_path "${development_prefix_root}")"
  [[ "${canonical_root}" == */data/development_prefix ]] ||
    fail "root must end exactly in /data/development_prefix: ${canonical_root}"
  [[ "${canonical_root}" != *"/data/raw"* ]] ||
    fail "canonical data/raw paths are forbidden"

  for prefix in "${alpha_prefix}" "${beta_prefix}" "${gamma_prefix}"; do
    reject_symlink_components "${prefix}" "daily development prefix"
    require_regular_file "${prefix}"
  done
  canonical_alpha="$(canonical_existing_path "${alpha_prefix}")"
  canonical_beta="$(canonical_existing_path "${beta_prefix}")"
  canonical_gamma="$(canonical_existing_path "${gamma_prefix}")"
  expected_alpha="${canonical_root}/SYN2ALPHASYN2USD/1d/SYN2ALPHASYN2USD-1d-all-years.csv"
  expected_beta="${canonical_root}/SYN2BETASYN2USD/1d/SYN2BETASYN2USD-1d-all-years.csv"
  expected_gamma="${canonical_root}/SYN2GAMMASYN2USD/1d/SYN2GAMMASYN2USD-1d-all-years.csv"
  [[ "${canonical_alpha}" == "${expected_alpha}" ]] ||
    fail "alpha prefix is not at its exact canonical path: ${expected_alpha}"
  [[ "${canonical_beta}" == "${expected_beta}" ]] ||
    fail "beta prefix is not at its exact canonical path: ${expected_beta}"
  [[ "${canonical_gamma}" == "${expected_gamma}" ]] ||
    fail "gamma prefix is not at its exact canonical path: ${expected_gamma}"
  [[ ! "${canonical_alpha}" -ef "${canonical_beta}" &&
     ! "${canonical_alpha}" -ef "${canonical_gamma}" &&
     ! "${canonical_beta}" -ef "${canonical_gamma}" ]] ||
    fail "daily instrument prefixes must be distinct files"

  all_prefix_files=()
  for instrument in "${instruments[@]}"; do
    for interval in "${intervals[@]}"; do
      prefix="${canonical_root}/${instrument}/${interval}/${instrument}-${interval}-all-years.csv"
      reject_symlink_components "${prefix}" \
        "${instrument} ${interval} development prefix"
      require_regular_file "${prefix}"
      canonical_prefix="$(canonical_existing_path "${prefix}")"
      [[ "${canonical_prefix}" == "${prefix}" ]] ||
        fail "development prefix is not at its exact canonical path: ${prefix}"
      [[ "${canonical_prefix}" != *"/data/raw/"* ]] ||
        fail "canonical data/raw paths are forbidden: ${canonical_prefix}"
      case "${interval}" in
      1d) expected_rows="${expected_1d_rows}" ;;
      3d) expected_rows="${expected_3d_rows}" ;;
      1w) expected_rows="${expected_1w_rows}" ;;
      *) fail "unexpected interval: ${interval}" ;;
      esac
      validate_exact_prefix_rows "${canonical_prefix}" "${expected_rows}"
      all_prefix_files+=("${canonical_prefix}")
    done
  done

  benchmark_root="${canonical_root%/data/development_prefix}"
  [[ -n "${benchmark_root}" && "${benchmark_root}" != "${canonical_root}" ]] ||
    fail "could not derive benchmark root"
  closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
  reject_symlink_components "${closure_report}" "fresh-seed data closure"
  require_nonempty_file "${closure_report}"
  [[ "$(canonical_existing_path "${closure_report}")" == "${closure_report}" ]] ||
    fail "fresh-seed data closure is not at its exact canonical path"
  grep -Fqx 'schema_id=synthetic_fresh_seed_data_closure.v2' \
    "${closure_report}" || fail "unexpected fresh-seed closure schema"
  grep -Fqx 'accepted_anchor_count=4096' "${closure_report}" ||
    fail "unexpected accepted anchor count in fresh-seed closure"
  grep -Fqx 'closure_complete=true' "${closure_report}" ||
    fail "fresh-seed data closure is not complete"

  training_policy="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
  reject_symlink_components "${training_policy}" "v2 MDN training policy"
  require_nonempty_file "${training_policy}"
  [[ "$(canonical_existing_path "${training_policy}")" == "${training_policy}" ]] ||
    fail "v2 MDN training policy is not at its exact canonical path"
  training_policy_sha256="$(sha256sum "${training_policy}")"
  training_policy_sha256="${training_policy_sha256%% *}"
  grep -Fq ".path=${training_policy}" "${closure_report}" ||
    fail "completed closure does not bind the v2 MDN training policy path"
  grep -Fq ".sha256=${training_policy_sha256}" "${closure_report}" ||
    fail "completed closure does not bind the live v2 MDN policy hash"

  local -a required_policy_lines=(
    'LEARNING_RATE = 0.001;'
    'MAX_STEPS = 3500;'
    'BATCH_SIZE = 64;'
    'GRAD_CLIP_NORM = 5.0;'
    'SEED = 31;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED = true;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT = 100.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_DIRECTION_WEIGHT = 5.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_RANK_WEIGHT = 5.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_HUBER_BETA = 0.5;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_LOGIT_SCALE = 1.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_TARGET_SCALE = 36.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_STEPS = 800;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_NLL_WEIGHT = 0.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT = 1.0;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_WARMUP_DIRECT_HEAD_ONLY = true;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_MODE = edge_embedding_per_edge;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_BASE_EDGE_COUNT = 3;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_EMBEDDING_DIM = 16;'
    'MDN_DIRECT_EDGE_RETURN_READOUT_ADAPTER_HIDDEN_DIM = 128;'
  )
  local policy_line
  for policy_line in "${required_policy_lines[@]}"; do
    grep -Fqx "  ${policy_line}" "${training_policy}" ||
      fail "v2 MDN policy drifted from the embedded isolation contract: ${policy_line}"
  done

  development_prefix_root="${canonical_root}"
  alpha_prefix="${canonical_alpha}"
  beta_prefix="${canonical_beta}"
  gamma_prefix="${canonical_gamma}"
}

preflight() {
  local command_name
  for command_name in awk cmp cp dirname g++ grep mktemp mv realpath \
    sha256sum; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  validate_prefix_paths
  require_regular_file "${helper_source}"
  require_regular_file "${runner_source}"
  require_directory "${libtorch_path}/include"
  require_directory "${libtorch_path}/include/torch/csrc/api/include"
  require_directory "${libtorch_path}/lib"

  printf 'preflight_ok=true\n'
  printf 'schema_id=%s\n' "${schema_id}"
  printf 'development_prefix_root=%s\n' "${development_prefix_root}"
  printf 'fresh_seed_data_closure=%s\n' "${closure_report}"
  printf 'v2_mdn_training_policy=%s\n' "${training_policy}"
  printf 'v2_mdn_training_policy_sha256=%s\n' "${training_policy_sha256}"
  printf 'declared_train_anchor_range=[0,2496)\n'
  printf 'effective_train_anchor_range=[1,2496)\n'
  printf 'validation_anchor_range=[2560,2816)\n'
  printf 'certified_eval_anchor_range=[2880,3264)\n'
  printf 'maximum_anchor_read=3263\n'
  printf 'maximum_daily_row_read=3293\n'
}

release_lock() {
  if [[ "${lock_held}" == "true" ]]; then
    rmdir "${lock_dir}" 2>/dev/null || true
    lock_held=false
  fi
}

freeze_sources() {
  local runtime="$1"
  local frozen_source_dir="${runtime}/frozen_sources"
  local frozen_include_root="${runtime}/frozen_include"
  local frozen_contract_dir="${runtime}/frozen_contracts"
  local header_name

  mkdir "${frozen_source_dir}" "${frozen_contract_dir}"
  mkdir -p \
    "${frozen_include_root}/piaabo/digest" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn"
  cp -- "${helper_source}" "${frozen_source_dir}/${helper_name}"
  cp -- "${runner_source}" "${frozen_source_dir}/${runner_name}"
  cp -- "${repo_root}/src/include/piaabo/digest/sha256.h" \
    "${frozen_include_root}/piaabo/digest/sha256.h"
  for header_name in \
    channel_context_mdn.h \
    channel_context_mdn_train_model.h \
    mixture_density_network_backbone.h \
    mixture_density_network_head.h \
    mixture_density_network_loss.h \
    mixture_density_network_types.h \
    mixture_density_network_utils.h; do
    cp -- \
      "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/${header_name}" \
      "${frozen_include_root}/wikimyei/inference/expected_value/mdn/${header_name}"
  done
  cp -- "${closure_report}" \
    "${frozen_contract_dir}/fresh_seed_data_closure.v2.report"
  cp -- "${training_policy}" \
    "${frozen_contract_dir}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
}

compile_helper() {
  local runtime="$1"
  local frozen_source_dir="${runtime}/frozen_sources"
  local frozen_include_root="${runtime}/frozen_include"
  local bin_dir="${runtime}/bin"
  mkdir "${bin_dir}"
  g++ -std=c++20 -O1 -g0 -Wall -Wextra -fPIC \
    -I"${frozen_include_root}" \
    -I"${repo_root}/src" \
    -I"${repo_root}/src/include" \
    -I"${repo_root}/src/include/torch_compat" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${frozen_source_dir}/${helper_name}" \
    -L"${libtorch_path}/lib" \
    -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${bin_dir}/raw_history_supervised_isolation"
}

run_probe() {
  local runtime="$1"
  local report="$2"
  local log="$3"
  local -a device_args=()

  export CUBLAS_WORKSPACE_CONFIG=:4096:8
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  if [[ "${SYNTHETIC_V2_RAW_HISTORY_CPU:-false}" == "true" ]]; then
    device_args+=(--cpu)
  fi
  "${runtime}/bin/raw_history_supervised_isolation" \
    --alpha-prefix "${alpha_prefix}" \
    --beta-prefix "${beta_prefix}" \
    --gamma-prefix "${gamma_prefix}" \
    --closure "${closure_report}" \
    --output "${report}" \
    --schema-id "${schema_id}" \
    "${device_args[@]}" \
    >"${log}" 2>&1
  require_nonempty_file "${report}"
  require_nonempty_file "${log}"
}

write_manifests() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "${all_prefix_files[@]}" \
      "${closure_report}" \
      "${training_policy}" \
      execution_contract.status >input_manifest.sha256
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${runner_name}" \
      frozen_include/piaabo/digest/sha256.h \
      frozen_include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h \
      frozen_include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_backbone.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_loss.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_types.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h \
      >source_manifest.sha256
    sha256sum \
      frozen_contracts/fresh_seed_data_closure.v2.report \
      frozen_contracts/wikimyei.inference.expected_value.mdn.v2.jkimyei \
      >contract_manifest.sha256
    sha256sum \
      "main/${schema_id}.report" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.report" \
      "replay/${schema_id}.stdout.log" \
      >output_manifest.sha256
    sha256sum \
      bin/raw_history_supervised_isolation \
      input_manifest.sha256 source_manifest.sha256 \
      contract_manifest.sha256 output_manifest.sha256 \
      >master.sha256
  )
}

verify_runtime() {
  require_directory "${final_runtime_dir}"
  require_nonempty_file "${final_runtime_dir}/input_manifest.sha256"
  require_nonempty_file "${final_runtime_dir}/source_manifest.sha256"
  require_nonempty_file "${final_runtime_dir}/contract_manifest.sha256"
  require_nonempty_file "${final_runtime_dir}/output_manifest.sha256"
  require_nonempty_file "${final_runtime_dir}/master.sha256"
  require_nonempty_file "${final_runtime_dir}/main/${schema_id}.report"
  require_nonempty_file "${final_runtime_dir}/main/${schema_id}.stdout.log"
  require_nonempty_file "${final_runtime_dir}/replay/${schema_id}.report"
  require_nonempty_file "${final_runtime_dir}/replay/${schema_id}.stdout.log"
  (
    cd "${final_runtime_dir}"
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
    sha256sum -c contract_manifest.sha256 >/dev/null
    sha256sum -c output_manifest.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
  cmp -s "${final_runtime_dir}/main/${schema_id}.report" \
    "${final_runtime_dir}/replay/${schema_id}.report" ||
    fail "main/replay reports are not byte-identical"
  cmp -s "${final_runtime_dir}/main/${schema_id}.stdout.log" \
    "${final_runtime_dir}/replay/${schema_id}.stdout.log" ||
    fail "main/replay logs are not byte-identical"
  printf 'verified_runtime=%s\n' "${final_runtime_dir}"
  printf 'verified_report=%s\n' \
    "${final_runtime_dir}/main/${schema_id}.report"
}

verify_live_sources_match_runtime() {
  cmp -s "${helper_source}" \
    "${final_runtime_dir}/frozen_sources/${helper_name}" ||
    fail "live helper differs from sealed runtime; use a new schema"
  cmp -s "${runner_source}" \
    "${final_runtime_dir}/frozen_sources/${runner_name}" ||
    fail "live runner differs from sealed runtime; use a new schema"
}

validate_report_contract() {
  local report="$1"
  local expected_closure_sha
  expected_closure_sha="$(sha256sum "${closure_report}")"
  expected_closure_sha="${expected_closure_sha%% *}"
  grep -Fqx 'max_anchor_read=3263' "${report}" ||
    fail "report does not bind max_anchor_read=3263"
  grep -Fqx 'max_daily_row_read=3293' "${report}" ||
    fail "report does not bind max_daily_row_read=3293"
  grep -Fqx 'test_input_used=false' "${report}" ||
    fail "report does not attest that final input was unused"
  grep -Fqx 'test_boundary_assertion_passed=true' "${report}" ||
    fail "report does not prove the audited final boundary"
  grep -Fqx 'provenance.representation_forward_executed=false' "${report}" ||
    fail "report does not attest representation bypass"
  grep -Fqx 'provenance.production_mdn_checkpoint_used=false' "${report}" ||
    fail "report does not attest checkpoint isolation"
  grep -Fqx 'provenance.production_channel_context_mdn_executed=true' \
    "${report}" || fail "production ChannelContextMdn was not attested"
  grep -Fqx 'execution.k1_objective=nll_only' "${report}" ||
    fail "K=1 NLL-only arm is not attested"
  grep -Fqx 'execution.k3_objective=current_direct_readout' "${report}" ||
    fail "K=3 current direct-readout arm is not attested"
  grep -Fqx 'execution.k3.direct_identity_mode=edge_embedding_per_edge' \
    "${report}" || fail "K=3 direct identity mode is not attested"
  grep -Fqx 'execution.k3.direct_warmup_steps=800' "${report}" ||
    fail "K=3 direct warmup is not attested"
  grep -Fqx "data.closure_sha256=${expected_closure_sha}" "${report}" ||
    fail "report is not bound to the completed fresh-seed closure"
  grep -Eq '^raw_history_supervised_gate_passed=(true|false)$' "${report}" ||
    fail "report is missing the non-fatal supervised capability decision"
}

run_all() {
  local scratch_runtime
  local main_dir
  local replay_dir
  local main_report
  local replay_report

  preflight
  if [[ -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" &&
        -f "${final_runtime_dir}/master.sha256" ]]; then
    verify_live_sources_match_runtime
    verify_runtime
    return
  fi

  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "refusing to overwrite an incomplete runtime: ${final_runtime_dir}"
  mkdir -p "${runtime_parent}"
  require_directory "${runtime_parent}"
  mkdir "${lock_dir}" || fail "another run holds ${lock_dir}"
  lock_held=true
  trap release_lock EXIT

  scratch_runtime="$(mktemp -d "${runtime_parent}/${schema_id}.scratch.XXXXXXXX")"
  main_dir="${scratch_runtime}/main"
  replay_dir="${scratch_runtime}/replay"
  main_report="${main_dir}/${schema_id}.report"
  replay_report="${replay_dir}/${schema_id}.report"
  mkdir "${main_dir}" "${replay_dir}"

  freeze_sources "${scratch_runtime}"
  compile_helper "${scratch_runtime}"
  {
    echo "schema_id=${schema_id}"
    echo "development_prefix_root=${development_prefix_root}"
    echo "benchmark_root=${benchmark_root}"
    echo "fresh_seed_data_closure=${closure_report}"
    echo "v2_mdn_training_policy=${training_policy}"
    echo "v2_mdn_training_policy_sha256=${training_policy_sha256}"
    echo "alpha_prefix=${alpha_prefix}"
    echo "beta_prefix=${beta_prefix}"
    echo "gamma_prefix=${gamma_prefix}"
    echo "expected_1d_prefix_rows=${expected_1d_rows}"
    echo "expected_3d_prefix_rows=${expected_3d_rows}"
    echo "expected_1w_prefix_rows=${expected_1w_rows}"
    echo "declared_train_anchor_range=[0,2496)"
    echo "effective_train_anchor_range=[1,2496)"
    echo "validation_anchor_range=[2560,2816)"
    echo "certified_eval_anchor_range=[2880,3264)"
    echo "maximum_anchor_read=3263"
    echo "maximum_daily_row_read=3293"
    echo "forbidden_final_holdout_begin=3328"
    echo "cpu_forced=${SYNTHETIC_V2_RAW_HISTORY_CPU:-false}"
  } >"${scratch_runtime}/execution_contract.status"

  run_probe "${scratch_runtime}" "${main_report}" \
    "${main_dir}/${schema_id}.stdout.log"
  run_probe "${scratch_runtime}" "${replay_report}" \
    "${replay_dir}/${schema_id}.stdout.log"
  cmp -s "${main_report}" "${replay_report}" ||
    fail "main/replay reports are not byte-identical"
  cmp -s "${main_dir}/${schema_id}.stdout.log" \
    "${replay_dir}/${schema_id}.stdout.log" ||
    fail "main/replay logs are not byte-identical"
  validate_report_contract "${main_report}"
  write_manifests "${scratch_runtime}"
  (
    cd "${scratch_runtime}"
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
    sha256sum -c contract_manifest.sha256 >/dev/null
    sha256sum -c output_manifest.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )

  mv -T -n "${scratch_runtime}" "${final_runtime_dir}" ||
    fail "atomic runtime installation failed; scratch runtime retained"
  [[ ! -e "${scratch_runtime}" && -d "${final_runtime_dir}" &&
     ! -L "${final_runtime_dir}" ]] ||
    fail "runtime no-clobber installation postcondition failed"
  release_lock
  trap - EXIT
  verify_runtime
}

case "${mode}" in
--preflight)
  preflight
  ;;
--run)
  run_all
  ;;
--verify)
  verify_runtime
  ;;
*)
  usage
  ;;
esac
