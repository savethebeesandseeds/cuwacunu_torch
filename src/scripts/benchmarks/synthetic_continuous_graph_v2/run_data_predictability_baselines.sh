#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_v2_data_predictability_baselines_v1"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
final_runtime_dir="${runtime_parent}/${schema_id}"
lock_dir="${final_runtime_dir}.lock"

helper_name="data_predictability_baselines.cpp"
runner_name="run_data_predictability_baselines.sh"
helper_source="${script_dir}/${helper_name}"
runner_source="${script_dir}/${runner_name}"

# Audited multi-timescale development-prefix invariants. This raw baseline
# reads only the three daily files, but the complete prefix contract is
# recorded here so callers cannot silently substitute a different mapping.
expected_1d_rows="${SYNTHETIC_V2_EXPECTED_1D_PREFIX_ROWS:-3294}"
expected_3d_rows="${SYNTHETIC_V2_EXPECTED_3D_PREFIX_ROWS:-1098}"
expected_1w_rows="${SYNTHETIC_V2_EXPECTED_1W_PREFIX_ROWS:-471}"

development_prefix_root="${SYNTHETIC_V2_DEVELOPMENT_PREFIX_ROOT:-}"
alpha_prefix="${SYNTHETIC_V2_ALPHA_1D_PREFIX:-}"
beta_prefix="${SYNTHETIC_V2_BETA_1D_PREFIX:-}"
gamma_prefix="${SYNTHETIC_V2_GAMMA_1D_PREFIX:-}"
ar_order="${SYNTHETIC_V2_AR_ORDER:-24}"
ridge_lambda="${SYNTHETIC_V2_RIDGE_LAMBDA:-0.00000001}"
authored_periods="${SYNTHETIC_V2_AUTHORED_PERIODS:-}"
benchmark_root=""
closure_report=""
declare -a all_prefix_files=()
mode=""
lock_held=false

fail() {
  echo "error: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'EOF'
usage:
  run_data_predictability_baselines.sh {--preflight|--run} \
    --development-prefix-root ABSOLUTE_PATH_ENDING_IN/data/development_prefix \
    --alpha-prefix ABSOLUTE_1D_PREFIX.csv \
    --beta-prefix ABSOLUTE_1D_PREFIX.csv \
    --gamma-prefix ABSOLUTE_1D_PREFIX.csv \
    [--ar-order 24] [--ridge-lambda 0.00000001] \
    [--authored-periods P_ALPHA,P_BETA,P_GAMMA]

  run_data_predictability_baselines.sh --verify

The equivalent SYNTHETIC_V2_* environment variables are also accepted.
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
    development_prefix_root="${2:-}"
    shift 2
    ;;
  --alpha-prefix)
    alpha_prefix="${2:-}"
    shift 2
    ;;
  --beta-prefix)
    beta_prefix="${2:-}"
    shift 2
    ;;
  --gamma-prefix)
    gamma_prefix="${2:-}"
    shift 2
    ;;
  --expected-1d-prefix-rows)
    expected_1d_rows="${2:-}"
    shift 2
    ;;
  --expected-3d-prefix-rows)
    expected_3d_rows="${2:-}"
    shift 2
    ;;
  --expected-1w-prefix-rows)
    expected_1w_rows="${2:-}"
    shift 2
    ;;
  --ar-order)
    ar_order="${2:-}"
    shift 2
    ;;
  --ridge-lambda)
    ridge_lambda="${2:-}"
    shift 2
    ;;
  --authored-periods)
    authored_periods="${2:-}"
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
  realpath -e -- "${path}" 2>/dev/null ||
    fail "path does not exist: ${path}"
}

validate_numeric_contract() {
  [[ "${expected_1d_rows}" == "3294" ]] ||
    fail "audited 1d development prefix length is exactly 3294"
  [[ "${expected_3d_rows}" == "1098" ]] ||
    fail "audited 3d development prefix length is exactly 1098"
  [[ "${expected_1w_rows}" == "471" ]] ||
    fail "audited 1w development prefix length is exactly 471"
  [[ "${ar_order}" =~ ^[0-9]+$ ]] || fail "AR order must be an integer"
  ((ar_order >= 1 && ar_order <= 29)) ||
    fail "AR order must be in [1,29]"
  awk -v value="${ridge_lambda}" \
    'BEGIN { exit !(value ~ /^([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/ && value + 0.0 >= 0.0) }' ||
    fail "ridge lambda must be a finite nonnegative decimal"
  if [[ -n "${authored_periods}" ]]; then
    [[ "${authored_periods}" =~ ^[0-9]+,[0-9]+,[0-9]+$ ]] ||
      fail "authored periods must be three comma-separated integers"
    IFS=',' read -r period_alpha period_beta period_gamma \
      <<<"${authored_periods}"
    for period in "${period_alpha}" "${period_beta}" "${period_gamma}"; do
      ((period >= 1 && period <= 29)) ||
        fail "authored periods must be in [1,29]"
    done
  fi
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
    reject_symlink_components "${prefix}" "daily prefix"
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
      all_prefix_files+=("${canonical_prefix}")
    done
  done

  benchmark_root="${canonical_root%/data/development_prefix}"
  [[ -n "${benchmark_root}" && "${benchmark_root}" != "${canonical_root}" ]] ||
    fail "could not derive benchmark root from development-prefix root"
  closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
  reject_symlink_components "${closure_report}" "fresh-seed data closure"
  require_regular_file "${closure_report}"
  grep -Fqx 'closure_complete=true' "${closure_report}" ||
    fail "fresh-seed data closure is not complete: ${closure_report}"

  development_prefix_root="${canonical_root}"
  alpha_prefix="${canonical_alpha}"
  beta_prefix="${canonical_beta}"
  gamma_prefix="${canonical_gamma}"
}

validate_exact_prefix_rows() {
  local path="$1"
  local expected="$2"
  local rows
  rows="$(awk 'END { print NR + 0 }' "${path}")"
  [[ "${rows}" == "${expected}" ]] ||
    fail "development-prefix row count mismatch (${rows} != ${expected}): ${path}"
}

preflight() {
  local command_name
  local prefix
  local expected_rows
  for command_name in awk cmp cp dirname g++ grep mktemp mv \
    realpath sha256sum; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  validate_numeric_contract
  validate_prefix_paths
  require_regular_file "${helper_source}"
  require_regular_file "${runner_source}"
  for prefix in "${all_prefix_files[@]}"; do
    case "${prefix}" in
    */1d/*) expected_rows="${expected_1d_rows}" ;;
    */3d/*) expected_rows="${expected_3d_rows}" ;;
    */1w/*) expected_rows="${expected_1w_rows}" ;;
    *) fail "unexpected development-prefix interval path: ${prefix}" ;;
    esac
    validate_exact_prefix_rows "${prefix}" "${expected_rows}"
  done

  printf 'preflight_ok=true\n'
  printf 'schema_id=%s\n' "${schema_id}"
  printf 'development_prefix_root=%s\n' "${development_prefix_root}"
  printf 'expected_1d_prefix_rows=%s\n' "${expected_1d_rows}"
  printf 'expected_3d_prefix_rows=%s\n' "${expected_3d_rows}"
  printf 'expected_1w_prefix_rows=%s\n' "${expected_1w_rows}"
  printf 'maximum_visible_anchor_exclusive=3264\n'
  printf 'maximum_daily_row_read=3293\n'
  printf 'fresh_seed_data_closure=%s\n' "${closure_report}"
}

release_lock() {
  if [[ "${lock_held}" == "true" ]]; then
    rmdir "${lock_dir}" 2>/dev/null || true
    lock_held=false
  fi
}

verify_runtime() {
  require_directory "${final_runtime_dir}"
  require_regular_file "${final_runtime_dir}/input_manifest.sha256"
  require_regular_file "${final_runtime_dir}/source_manifest.sha256"
  require_regular_file "${final_runtime_dir}/output_manifest.sha256"
  require_regular_file "${final_runtime_dir}/master.sha256"
  require_regular_file \
    "${final_runtime_dir}/main/${schema_id}.report"
  require_regular_file \
    "${final_runtime_dir}/replay/${schema_id}.report"
  (
    cd "${final_runtime_dir}"
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
    sha256sum -c output_manifest.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
  cmp -s "${final_runtime_dir}/main/${schema_id}.report" \
    "${final_runtime_dir}/replay/${schema_id}.report" ||
    fail "main/replay reports are not byte-identical"
  printf 'verified_runtime=%s\n' "${final_runtime_dir}"
  printf 'verified_report=%s\n' \
    "${final_runtime_dir}/main/${schema_id}.report"
}

run_all() {
  local scratch_runtime
  local frozen_source_dir
  local bin_dir
  local helper_binary
  local main_dir
  local replay_dir
  local main_report
  local replay_report
  local -a authored_args=()

  preflight
  if [[ -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" &&
        -f "${final_runtime_dir}/master.sha256" ]]; then
    cmp -s "${helper_source}" \
      "${final_runtime_dir}/frozen_sources/${helper_name}" ||
      fail "live helper differs from sealed runtime; use a new schema"
    cmp -s "${runner_source}" \
      "${final_runtime_dir}/frozen_sources/${runner_name}" ||
      fail "live runner differs from sealed runtime; use a new schema"
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
  frozen_source_dir="${scratch_runtime}/frozen_sources"
  bin_dir="${scratch_runtime}/bin"
  helper_binary="${bin_dir}/data_predictability_baselines"
  main_dir="${scratch_runtime}/main"
  replay_dir="${scratch_runtime}/replay"
  main_report="${main_dir}/${schema_id}.report"
  replay_report="${replay_dir}/${schema_id}.report"
  mkdir "${frozen_source_dir}" "${bin_dir}" "${main_dir}" "${replay_dir}"

  cp -- "${helper_source}" "${frozen_source_dir}/${helper_name}"
  cp -- "${runner_source}" "${frozen_source_dir}/${runner_name}"
  g++ -std=c++20 -O2 -Wall -Wextra -Werror -pedantic \
    "${frozen_source_dir}/${helper_name}" -o "${helper_binary}"

  {
    echo "schema_id=${schema_id}"
    echo "development_prefix_root=${development_prefix_root}"
    echo "benchmark_root=${benchmark_root}"
    echo "fresh_seed_data_closure=${closure_report}"
    echo "alpha_prefix=${alpha_prefix}"
    echo "beta_prefix=${beta_prefix}"
    echo "gamma_prefix=${gamma_prefix}"
    echo "expected_1d_prefix_rows=${expected_1d_rows}"
    echo "expected_3d_prefix_rows=${expected_3d_rows}"
    echo "expected_1w_prefix_rows=${expected_1w_rows}"
    echo "ar_order=${ar_order}"
    echo "ridge_lambda=${ridge_lambda}"
    echo "authored_periods=${authored_periods}"
    echo "maximum_visible_anchor_exclusive=3264"
    echo "forbidden_test_anchor_begin=3328"
  } >"${scratch_runtime}/execution_contract.status"

  if [[ -n "${authored_periods}" ]]; then
    authored_args+=(--authored-periods "${authored_periods}")
  fi

  "${helper_binary}" \
    --alpha-prefix "${alpha_prefix}" \
    --beta-prefix "${beta_prefix}" \
    --gamma-prefix "${gamma_prefix}" \
    --expected-prefix-rows "${expected_1d_rows}" \
    --ar-order "${ar_order}" \
    --ridge-lambda "${ridge_lambda}" \
    --output "${main_report}" \
    "${authored_args[@]}" \
    >"${main_dir}/${schema_id}.stdout.log" 2>&1
  "${helper_binary}" \
    --alpha-prefix "${alpha_prefix}" \
    --beta-prefix "${beta_prefix}" \
    --gamma-prefix "${gamma_prefix}" \
    --expected-prefix-rows "${expected_1d_rows}" \
    --ar-order "${ar_order}" \
    --ridge-lambda "${ridge_lambda}" \
    --output "${replay_report}" \
    "${authored_args[@]}" \
    >"${replay_dir}/${schema_id}.stdout.log" 2>&1

  require_regular_file "${main_report}"
  require_regular_file "${replay_report}"
  cmp -s "${main_report}" "${replay_report}" ||
    fail "main/replay reports are not byte-identical"
  cmp -s "${main_dir}/${schema_id}.stdout.log" \
    "${replay_dir}/${schema_id}.stdout.log" ||
    fail "main/replay logs are not byte-identical"
  grep -Fqx 'max_anchor_read=3263' "${main_report}" ||
    fail "report does not bind max_anchor_read=3263"
  grep -Fqx 'max_daily_row_read=3293' "${main_report}" ||
    fail "report does not bind max_daily_row_read=3293"
  grep -Fqx 'test_input_used=false' "${main_report}" ||
    fail "report does not attest that test input was unused"
  grep -Fqx 'test_boundary_assertion_passed=true' "${main_report}" ||
    fail "report does not prove the audited test boundary"
  grep -Eq '^raw_predictability_gate_passed=(true|false)$' "${main_report}" ||
    fail "report is missing the non-fatal raw predictability decision"

  (
    cd "${scratch_runtime}"
    sha256sum \
      "${all_prefix_files[@]}" \
      "${closure_report}" \
      execution_contract.status >input_manifest.sha256
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${runner_name}" >source_manifest.sha256
    sha256sum \
      "main/${schema_id}.report" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.report" \
      "replay/${schema_id}.stdout.log" >output_manifest.sha256
    sha256sum \
      bin/data_predictability_baselines \
      input_manifest.sha256 source_manifest.sha256 output_manifest.sha256 \
      >master.sha256
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
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
