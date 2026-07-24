#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_isolated_development_source_v1"
readonly expected_old_closure_sha256="36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831"
readonly expected_isolation_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_isolated_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_fresh_preregistration_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly accepted_anchor_count=3261
readonly maximum_anchor_index=3260
readonly source_cursor_first_master_day_index=29
readonly source_cursor_last_master_day_index=3290
readonly source_cursor_first_key=1896047999999
readonly source_cursor_last_key=2177711999999
readonly train_begin=0
readonly train_end=2496
readonly prefix_source_count=9
readonly mirror_csv_count=9
readonly mirror_cache_count=18

fail() {
  echo "synthetic v2 isolated development source: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'USAGE'
usage: prepare_and_seal_isolated_development_source_v2.sh [--plan|--prepare|--verify]

  --plan     Print the fixed isolated-source contract without writing.
  --prepare  Copy only sealed development-prefix CSVs, build mirror-local
             caches through a zero-step Runtime dry run, and publish closure.
  --verify   Verify the completed isolated closure without writing.
USAGE
}

trim() {
  local value="$1"
  value="${value#"${value%%[![:space:]]*}"}"
  value="${value%"${value##*[![:space:]]}"}"
  printf '%s' "${value}"
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_regular_file() {
  local path="$1"
  [[ -f "${path}" ]] || fail "required regular file is absent: ${path}"
  [[ ! -L "${path}" ]] || fail "symbolic links are forbidden: ${path}"
}

require_directory() {
  local path="$1"
  [[ -d "${path}" ]] || fail "required directory is absent: ${path}"
  [[ ! -L "${path}" ]] || fail "symbolic links are forbidden: ${path}"
}

require_executable() {
  require_regular_file "$1"
  [[ -x "$1" ]] || fail "required executable is not executable: $1"
}

reject_data_raw_path() {
  local path="$1"
  case "${path}" in
  */data/raw | */data/raw/*)
    fail "canonical data/raw path is forbidden: ${path}"
    ;;
  esac
}

reject_symlink_components() {
  local path="$1"
  [[ "${path}" == /* ]] || fail "path is not absolute: ${path}"
  local current="/" rest component
  rest="${path#/}"
  while [[ -n "${rest}" ]]; do
    if [[ "${rest}" == */* ]]; then
      component="${rest%%/*}"
      rest="${rest#*/}"
    else
      component="${rest}"
      rest=""
    fi
    [[ -n "${component}" ]] || continue
    if [[ "${current}" == "/" ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains a symbolic-link component: ${current}"
  done
}

kv_value() {
  local path="$1"
  local key="$2"
  local count value
  count="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) count += 1;
    }
    END { print count + 0 }
  ' "${path}")"
  [[ "${count}" == 1 ]] ||
    fail "${path} must contain exactly one ${key}= entry (found ${count})"
  value="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) {
        value = substr($0, eq + 1);
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
        print value;
      }
    }
  ' "${path}")"
  printf '%s' "${value}"
}

expect_kv() {
  local path="$1"
  local key="$2"
  local expected="$3"
  local actual
  actual="$(kv_value "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="$(realpath -e -- \
  "${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2")"
prefix_root="${benchmark_root}/data/development_prefix"
old_closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
canonical_registry_source="${benchmark_root}/ujcamei.source.registry.dsl"
base_config_source="${benchmark_root}/synthetic_benchmark.config"
fresh_preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
isolation_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
isolated_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
cache_guard_source="${repo_root}/src/scripts/benchmarks/synthetic_continuous_graph_v1/check_frozen_cache_freshness.cpp"
cache_guard_header="${repo_root}/src/include/ujcamei/source/retrieval/storage/memory_mapped/cache_freshness.h"

runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${runtime_parent}/${schema_id}"
source_root="${runtime_root}/source"
config_dir="${runtime_root}/config"
isolated_registry="${config_dir}/ujcamei.source.registry.development_prefix.dsl"
isolated_base_config="${config_dir}/synthetic_benchmark.isolated_development.config"
dry_run_config="${config_dir}/synthetic_benchmark.isolated_development.cache_build.config"
bin_dir="${runtime_root}/bin"
cache_guard_binary="${bin_dir}/check_frozen_cache_freshness"
build_dir="${runtime_root}/build"
cache_guard_build_manifest="${build_dir}/cache_guard.build.status"
source_manifest="${runtime_root}/source_manifest.status"
job_parent="${runtime_root}/job"
job_dir="${job_parent}/isolated_development_cache_build_dry_run"
job_manifest="${job_dir}/job.manifest"
runtime_result="${job_dir}/runtime.result.fact"
logs_dir="${runtime_root}/logs"
runtime_log="${logs_dir}/isolated_development_cache_build_dry_run.log"
closure_receipt="${runtime_root}/development_source_closure.status"

mode="${1:---plan}"
[[ "$#" -le 1 ]] || {
  usage
  exit 2
}
case "${mode}" in
--plan | --prepare | --verify) ;;
*)
  usage
  exit 2
  ;;
esac

declare -a instruments=(
  SYN2ALPHASYN2USD
  SYN2BETASYN2USD
  SYN2GAMMASYN2USD
)
declare -a intervals=(1d 3d 1w)
declare -a csv_relatives=()
declare -a raw_cache_relatives=()
declare -a normalized_cache_relatives=()
declare -a prefix_csv_paths=()
declare -a mirror_csv_paths=()
declare -a mirror_cache_paths=()
declare -a cache_guard_args=()
declare -A expected_prefix_sha=()
declare -A expected_prefix_size=()
declare -A old_entry_kind=()
declare -A old_entry_path=()
declare -A old_entry_sha=()
declare -A old_entry_size=()
declare -A old_bound_sha=()
declare -A old_bound_size=()
old_metadata_loaded=false

populate_fixed_paths() {
  local instrument interval stem relative
  csv_relatives=()
  raw_cache_relatives=()
  normalized_cache_relatives=()
  prefix_csv_paths=()
  mirror_csv_paths=()
  mirror_cache_paths=()
  cache_guard_args=()
  for instrument in "${instruments[@]}"; do
    for interval in "${intervals[@]}"; do
      stem="${instrument}-${interval}-all-years"
      relative="${instrument}/${interval}/${stem}"
      csv_relatives+=("${relative}.csv")
      raw_cache_relatives+=("${relative}.cache.bin")
      normalized_cache_relatives+=(
        "${relative}.cache.norm.log_returns.bin")
      prefix_csv_paths+=("${prefix_root}/${relative}.csv")
      mirror_csv_paths+=("${source_root}/${relative}.csv")
      mirror_cache_paths+=(
        "${source_root}/${relative}.cache.bin"
        "${source_root}/${relative}.cache.norm.log_returns.bin")
      cache_guard_args+=(
        "${source_root}/${relative}.csv"
        "${source_root}/${relative}.cache.bin"
        "${source_root}/${relative}.cache.norm.log_returns.bin")
    done
  done
}

populate_fixed_paths

load_old_closure_metadata() {
  [[ "${old_metadata_loaded}" == false ]] || return 0
  local line key value index formatted field count
  old_entry_kind=()
  old_entry_path=()
  old_entry_sha=()
  old_entry_size=()
  old_bound_sha=()
  old_bound_size=()
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] || continue
    key="${line%%=*}"
    value="${line#*=}"
    if [[ "${key}" =~ ^entry\.([0-9][0-9][0-9])\.(kind|path|sha256|size_bytes)$ ]]; then
      index="${BASH_REMATCH[1]}"
      field="${BASH_REMATCH[2]}"
      case "${field}" in
      kind) old_entry_kind["${index}"]="${value}" ;;
      path) old_entry_path["${index}"]="${value}" ;;
      sha256) old_entry_sha["${index}"]="${value}" ;;
      size_bytes) old_entry_size["${index}"]="${value}" ;;
      esac
    fi
  done <"${old_closure_report}"
  count="$(kv_value "${old_closure_report}" entry_count)"
  [[ "${count}" =~ ^[0-9]+$ && "${count}" -gt 0 ]] ||
    fail "old closure report has invalid entry_count"
  for ((index = 0; index < count; ++index)); do
    formatted="$(printf '%03d' "${index}")"
    [[ -n "${old_entry_kind[${formatted}]:-}" &&
      -n "${old_entry_path[${formatted}]:-}" &&
      -n "${old_entry_sha[${formatted}]:-}" &&
      -n "${old_entry_size[${formatted}]:-}" ]] ||
      fail "old closure report has an incomplete entry: ${formatted}"
    [[ -z "${old_bound_sha[${old_entry_path[${formatted}]}]:-}" ]] ||
      fail "old closure report duplicates a bound path"
    old_bound_sha["${old_entry_path[${formatted}]}"]="${old_entry_sha[${formatted}]}"
    old_bound_size["${old_entry_path[${formatted}]}"]="${old_entry_size[${formatted}]}"
  done
  old_metadata_loaded=true
}

verify_safe_file_against_old_closure() {
  local path="$1"
  reject_data_raw_path "${path}"
  [[ -n "${old_bound_sha[${path}]:-}" ]] ||
    fail "old closure report does not bind required safe input: ${path}"
  [[ "$(sha256_of "${path}")" == "${old_bound_sha[${path}]}" ]] ||
    fail "safe input differs from its old-closure hash: ${path}"
  [[ "$(stat -c '%s' -- "${path}")" == "${old_bound_size[${path}]}" ]] ||
    fail "safe input differs from its old-closure size: ${path}"
}

verify_static_inputs() {
  local path
  for path in \
    "${old_closure_report}" \
    "${canonical_registry_source}" \
    "${base_config_source}" \
    "${fresh_preregistration}" \
    "${isolation_amendment}" \
    "${isolated_protocol}" \
    "${staged_hardening}" \
    "${cursor_alignment_correction}" \
    "${runtime_exec}" \
    "${cache_guard_source}" \
    "${cache_guard_header}"; do
    reject_data_raw_path "${path}"
    reject_symlink_components "${path}"
    require_regular_file "${path}"
  done
  [[ "$(sha256_of "${old_closure_report}")" == \
    "${expected_old_closure_sha256}" ]] ||
    fail "old closure report document hash drifted"
  [[ "$(sha256_of "${fresh_preregistration}")" == \
    "${expected_fresh_preregistration_sha256}" ]] ||
    fail "fresh-seed preregistration hash drifted"
  [[ "$(sha256_of "${isolation_amendment}")" == \
    "${expected_isolation_amendment_sha256}" ]] ||
    fail "development-source isolation amendment hash drifted"
  [[ "$(sha256_of "${isolated_protocol}")" == \
    "${expected_isolated_protocol_sha256}" ]] ||
    fail "isolated-source protocol hash drifted"
  [[ "$(sha256_of "${staged_hardening}")" == \
    "${expected_staged_hardening_sha256}" ]] ||
    fail "staged-evaluation hardening amendment hash drifted"
  [[ "$(sha256_of "${cursor_alignment_correction}")" == \
    "${expected_cursor_alignment_correction_sha256}" ]] ||
    fail "development-prefix cursor alignment correction hash drifted"
  [[ "$(sha256_of "${runtime_exec}")" == \
    "${expected_runtime_exec_sha256}" ]] ||
    fail "Runtime executable hash drifted"
  require_executable "${runtime_exec}"
  expect_kv "${old_closure_report}" schema_id \
    synthetic_fresh_seed_data_closure.v2
  expect_kv "${old_closure_report}" closure_complete true
  load_old_closure_metadata
  for path in \
    "${canonical_registry_source}" \
    "${base_config_source}" \
    "${fresh_preregistration}" \
    "${runtime_exec}" \
    "${cache_guard_source}" \
    "${cache_guard_header}"; do
    verify_safe_file_against_old_closure "${path}"
  done
}

load_prefix_metadata_from_old_closure() {
  local count index formatted kind path found=0
  expected_prefix_sha=()
  expected_prefix_size=()
  load_old_closure_metadata
  count="$(kv_value "${old_closure_report}" entry_count)"
  [[ "${count}" =~ ^[0-9]+$ && "${count}" -gt 0 ]] ||
    fail "old closure report has invalid entry_count"
  for ((index = 0; index < count; ++index)); do
    formatted="$(printf '%03d' "${index}")"
    path="${old_entry_path[${formatted}]}"
    if [[ "${path}" == "${prefix_root}/"* ]]; then
      kind="${old_entry_kind[${formatted}]}"
      [[ "${kind}" == source_csv ]] ||
        fail "old closure prefix entry is not source_csv: ${path}"
      [[ -z "${expected_prefix_sha[${path}]:-}" ]] ||
        fail "old closure duplicates prefix source: ${path}"
      expected_prefix_sha["${path}"]="${old_entry_sha[${formatted}]}"
      expected_prefix_size["${path}"]="${old_entry_size[${formatted}]}"
      found=$((found + 1))
    fi
  done
  [[ "${found}" == "${prefix_source_count}" ]] ||
    fail "old closure binds ${found} development-prefix CSVs instead of 9"
  for path in "${prefix_csv_paths[@]}"; do
    [[ -n "${expected_prefix_sha[${path}]:-}" ]] ||
      fail "old closure does not bind expected prefix source: ${path}"
  done
}

array_contains() {
  local needle="$1"
  shift
  local candidate
  for candidate in "$@"; do
    [[ "${candidate}" == "${needle}" ]] && return 0
  done
  return 1
}

validate_prefix_inventory() {
  local path relative actual_sha actual_size symlink special
  require_directory "${prefix_root}"
  reject_symlink_components "${prefix_root}"
  symlink="$(find "${prefix_root}" -type l -print -quit)"
  [[ -z "${symlink}" ]] ||
    fail "development prefix contains a symlink: ${symlink}"
  special="$(find "${prefix_root}" -mindepth 1 ! -type d ! -type f \
    -print -quit)"
  [[ -z "${special}" ]] ||
    fail "development prefix contains a special entry: ${special}"
  local file_count directory_count
  file_count="$(find "${prefix_root}" -type f -printf '.' | wc -c)"
  directory_count="$(find "${prefix_root}" -type d -printf '.' | wc -c)"
  [[ "${file_count}" == "${prefix_source_count}" ]] ||
    fail "development prefix has ${file_count} files instead of 9"
  [[ "${directory_count}" == 13 ]] ||
    fail "development prefix has an unexpected directory inventory"
  while IFS= read -r -d '' path; do
    relative="${path#"${prefix_root}/"}"
    array_contains "${relative}" "${csv_relatives[@]}" ||
      fail "unexpected development-prefix file: ${path}"
  done < <(find "${prefix_root}" -type f -print0)
  for path in "${prefix_csv_paths[@]}"; do
    reject_data_raw_path "${path}"
    reject_symlink_components "${path}"
    require_regular_file "${path}"
    [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
      fail "prefix source canonical path drifted: ${path}"
    actual_sha="$(sha256_of "${path}")"
    actual_size="$(stat -c '%s' -- "${path}")"
    [[ "${actual_sha}" == "${expected_prefix_sha[${path}]}" ]] ||
      fail "prefix source hash differs from old closure: ${path}"
    [[ "${actual_size}" == "${expected_prefix_size[${path}]}" ]] ||
      fail "prefix source size differs from old closure: ${path}"
  done
}

validate_source_tree() {
  local root="$1"
  local cache_policy="$2"
  local path relative prefix_path actual_sha actual_size symlink special
  local file_count directory_count cache_count=0 csv_count=0
  require_directory "${root}"
  reject_data_raw_path "${root}"
  reject_symlink_components "${root}"
  symlink="$(find "${root}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "isolated source contains a symlink: ${symlink}"
  special="$(find "${root}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "isolated source contains a special entry: ${special}"
  directory_count="$(find "${root}" -type d -printf '.' | wc -c)"
  [[ "${directory_count}" == 13 ]] ||
    fail "isolated source has an unexpected directory inventory"
  while IFS= read -r -d '' path; do
    reject_data_raw_path "${path}"
    relative="${path#"${root}/"}"
    if array_contains "${relative}" "${csv_relatives[@]}"; then
      csv_count=$((csv_count + 1))
    elif array_contains "${relative}" "${raw_cache_relatives[@]}" ||
      array_contains "${relative}" "${normalized_cache_relatives[@]}"; then
      cache_count=$((cache_count + 1))
    else
      fail "unexpected isolated-source file: ${path}"
    fi
  done < <(find "${root}" -type f -print0)
  file_count=$((csv_count + cache_count))
  [[ "${csv_count}" == "${mirror_csv_count}" ]] ||
    fail "isolated source has ${csv_count} CSVs instead of 9"
  case "${cache_policy}" in
  allow_partial)
    ((cache_count <= mirror_cache_count)) ||
      fail "isolated source has too many cache files"
    ;;
  require_complete)
    [[ "${cache_count}" == "${mirror_cache_count}" ]] ||
      fail "isolated source has ${cache_count} caches instead of 18"
    ;;
  *) fail "invalid internal cache inventory policy: ${cache_policy}" ;;
  esac
  ((file_count == mirror_csv_count + cache_count)) ||
    fail "isolated source file accounting failed"
  for relative in "${csv_relatives[@]}"; do
    path="${root}/${relative}"
    prefix_path="${prefix_root}/${relative}"
    require_regular_file "${path}"
    reject_symlink_components "${path}"
    actual_sha="$(sha256_of "${path}")"
    actual_size="$(stat -c '%s' -- "${path}")"
    [[ "${actual_sha}" == "${expected_prefix_sha[${prefix_path}]}" ]] ||
      fail "isolated mirror CSV hash mismatch: ${path}"
    [[ "${actual_size}" == "${expected_prefix_size[${prefix_path}]}" ]] ||
      fail "isolated mirror CSV size mismatch: ${path}"
  done
}

publish_immutable_file() {
  local candidate="$1"
  local destination="$2"
  local label="$3"
  chmod 0444 "${candidate}"
  if [[ -e "${destination}" ]]; then
    require_regular_file "${destination}"
    cmp -s -- "${candidate}" "${destination}" ||
      fail "immutable ${label} drifted: ${destination}"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${destination}" ||
      fail "could not publish immutable ${label}: ${destination}"
    rm -f -- "${candidate}"
  fi
}

ensure_fixed_directory() {
  local path="$1"
  local create_parents="${2:-false}"
  reject_data_raw_path "${path}"
  reject_symlink_components "${path}"
  if [[ -e "${path}" || -L "${path}" ]]; then
    require_directory "${path}"
  elif [[ "${create_parents}" == true ]]; then
    mkdir -p -- "${path}"
  else
    mkdir -- "${path}"
  fi
  reject_symlink_components "${path}"
  require_directory "${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "fixed output directory is not canonical: ${path}"
}

prepare_output_hierarchy() {
  local path
  ensure_fixed_directory "${runtime_parent}" true

  # Reject every pre-created redirect before the first lock, candidate, log,
  # compiler output, or Runtime job can be opened.
  for path in \
    "${runtime_root}" "${source_root}" "${config_dir}" \
    "${isolated_registry}" "${isolated_base_config}" \
    "${dry_run_config}" "${bin_dir}" "${cache_guard_binary}" \
    "${build_dir}" "${cache_guard_build_manifest}" \
    "${source_manifest}" "${job_parent}" "${job_dir}" \
    "${job_manifest}" "${runtime_result}" "${logs_dir}" \
    "${runtime_log}" "${closure_receipt}" \
    "${runtime_root}/.preparation.lock"; do
    reject_data_raw_path "${path}"
    reject_symlink_components "${path}"
  done

  ensure_fixed_directory "${runtime_root}"
  for path in "${config_dir}" "${bin_dir}" "${build_dir}" \
    "${job_parent}" "${logs_dir}"; do
    ensure_fixed_directory "${path}"
  done

  # Recheck all planned destinations after directory creation so a fixed
  # output can never silently resolve outside the sealed runtime root.
  for path in \
    "${runtime_root}" "${source_root}" "${config_dir}" \
    "${isolated_registry}" "${isolated_base_config}" \
    "${dry_run_config}" "${bin_dir}" "${cache_guard_binary}" \
    "${build_dir}" "${cache_guard_build_manifest}" \
    "${source_manifest}" "${job_parent}" "${job_dir}" \
    "${job_manifest}" "${runtime_result}" "${logs_dir}" \
    "${runtime_log}" "${closure_receipt}" \
    "${runtime_root}/.preparation.lock"; do
    reject_symlink_components "${path}"
  done
}

assert_no_stale_candidates() {
  local stale
  stale="$(find "${runtime_root}" -maxdepth 1 -mindepth 1 \
    \( -name '.source.candidate.*' -o -name '.registry.*' -o \
       -name '.base_config.*' -o -name '.dry_config.*' -o \
       -name '.cache_guard.*' -o -name '.cache_guard_build.*' -o \
       -name '.source_manifest.*' -o -name '.closure.*' \) \
    -print -quit 2>/dev/null || true)"
  [[ -z "${stale}" ]] ||
    fail "stale preparation candidate requires operator inspection: ${stale}"
}

install_source_mirror() {
  validate_prefix_inventory
  if [[ -e "${source_root}" ]]; then
    validate_source_tree "${source_root}" allow_partial
    validate_prefix_inventory
    return
  fi
  local stale candidate relative destination
  stale="$(find "${runtime_root}" -maxdepth 1 -type d \
    -name '.source.candidate.*' -print -quit 2>/dev/null || true)"
  [[ -z "${stale}" ]] ||
    fail "partial atomic source candidate exists; refusing ambiguity: ${stale}"
  candidate="$(mktemp -d "${runtime_root}/.source.candidate.XXXXXXXX")"
  for relative in "${csv_relatives[@]}"; do
    destination="${candidate}/${relative}"
    mkdir -p "$(dirname "${destination}")"
    cp --no-dereference --preserve=timestamps -- \
      "${prefix_root}/${relative}" "${destination}"
  done
  validate_source_tree "${candidate}" allow_partial
  validate_prefix_inventory
  mv -T -- "${candidate}" "${source_root}" ||
    fail "atomic isolated-source installation failed; candidate retained"
  validate_source_tree "${source_root}" allow_partial
}

write_isolated_registry() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.registry.XXXXXX")"
  awk -v root="${source_root}" '
    /^[[:space:]]*SOURCE_ROOT[[:space:]]*=/ {
      if (++replaced > 1) exit 42;
      indent = $0;
      sub(/[^[:space:]].*$/, "", indent);
      print indent "SOURCE_ROOT = " root ";";
      next;
    }
    { print }
    END { if (replaced != 1) exit 43 }
  ' "${canonical_registry_source}" >"${candidate}" ||
    fail "could not derive isolated source registry"
  publish_immutable_file "${candidate}" "${isolated_registry}" \
    "isolated registry"
}

write_isolated_base_config() {
  local candidate line lhs rhs key value indent absolute
  candidate="$(mktemp "${runtime_root}/.base_config.XXXXXX")"
  while IFS= read -r line || [[ -n "${line}" ]]; do
    if [[ "${line}" != *"="* ]]; then
      printf '%s\n' "${line}" >>"${candidate}"
      continue
    fi
    lhs="${line%%=*}"
    rhs="${line#*=}"
    key="$(trim "${lhs}")"
    value="$(trim "${rhs}")"
    indent="${lhs%%[![:space:]]*}"
    if [[ "${key}" == ujcamei_source_registry_dsl_path ]]; then
      printf '%s%s = %s\n' "${indent}" "${key}" "${isolated_registry}" \
        >>"${candidate}"
    elif [[ "${key}" == *_path && "${value}" != /* ]]; then
      absolute="$(realpath -m -- "${benchmark_root}/${value}")"
      printf '%s%s = %s\n' "${indent}" "${key}" "${absolute}" \
        >>"${candidate}"
    else
      printf '%s\n' "${line}" >>"${candidate}"
    fi
  done <"${base_config_source}"
  publish_immutable_file "${candidate}" "${isolated_base_config}" \
    "isolated base config"
}

write_dry_run_config() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.dry_config.XXXXXX")"
  awk '
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      if (++replaced > 1) exit 42;
      indent = $0;
      sub(/[^[:space:]].*$/, "", indent);
      print indent "runtime_wave_id = train_core_mtf_jepa_mae_vicreg";
      next;
    }
    { print }
    END { if (replaced != 1) exit 43 }
  ' "${isolated_base_config}" >"${candidate}" ||
    fail "could not derive isolated cache-build config"
  publish_immutable_file "${candidate}" "${dry_run_config}" \
    "isolated dry-run config"
}

validate_config_file() {
  local config="$1"
  local expected_wave="$2"
  local line lhs rhs key value canonical
  require_regular_file "${config}"
  reject_data_raw_path "${config}"
  expect_kv "${config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
  expect_kv "${config}" runtime_wave_id "${expected_wave}"
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] || continue
    lhs="${line%%=*}"
    rhs="${line#*=}"
    key="$(trim "${lhs}")"
    value="$(trim "${rhs}")"
    [[ "${key}" == *_path ]] || continue
    [[ "${value}" == /* ]] ||
      fail "isolated config contains non-absolute path: ${key}=${value}"
    reject_data_raw_path "${value}"
    reject_symlink_components "${value}"
    canonical="$(realpath -e -- "${value}")"
    [[ "${canonical}" == "${value}" ]] ||
      fail "isolated config path is not canonical: ${key}=${value}"
    require_regular_file "${canonical}"
  done <"${config}"
}

validate_registry_and_configs() {
  require_regular_file "${isolated_registry}"
  expect_kv "${isolated_registry}" SOURCE_ROOT "${source_root};"
  local active_root_count
  active_root_count="$(awk '
    /^[[:space:]]*SOURCE_ROOT[[:space:]]*=/ { ++count }
    END { print count + 0 }
  ' "${isolated_registry}")"
  [[ "${active_root_count}" == 1 ]] ||
    fail "isolated registry must contain exactly one SOURCE_ROOT"
  validate_config_file "${isolated_base_config}" policy_training_ppo_v0
  validate_config_file "${dry_run_config}" train_core_mtf_jepa_mae_vicreg
}

compile_cache_guard() {
  local candidate compiler
  mkdir -p "${bin_dir}" "${build_dir}"
  compiler="$(realpath -e -- "$(command -v g++)")"
  candidate="$(mktemp "${runtime_root}/.cache_guard.XXXXXX")"
  (
    cd "${repo_root}"
    "${compiler}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
      -Isrc/include "${cache_guard_source}" -o "${candidate}"
  )
  chmod 0555 "${candidate}"
  if [[ -e "${cache_guard_binary}" ]]; then
    require_executable "${cache_guard_binary}"
    cmp -s -- "${candidate}" "${cache_guard_binary}" ||
      fail "cache guard binary differs from the existing immutable build"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${cache_guard_binary}" ||
      fail "could not publish isolated cache guard binary"
    rm -f -- "${candidate}"
  fi
  candidate="$(mktemp "${runtime_root}/.cache_guard_build.XXXXXX")"
  cat >"${candidate}" <<BUILD
schema_id=${schema_id}.cache_guard_build.v1
status=complete
compiler_path=${compiler}
compiler_sha256=$(sha256_of "${compiler}")
compiler_target=$("${compiler}" -dumpmachine)
cache_guard_source_path=${cache_guard_source}
cache_guard_source_sha256=$(sha256_of "${cache_guard_source}")
cache_guard_header_path=${cache_guard_header}
cache_guard_header_sha256=$(sha256_of "${cache_guard_header}")
cache_guard_binary_path=${cache_guard_binary}
cache_guard_binary_sha256=$(sha256_of "${cache_guard_binary}")
compile_flags=-std=c++20,-O2,-Wall,-Wextra,-Wpedantic,-Werror,-Isrc/include
BUILD
  publish_immutable_file "${candidate}" "${cache_guard_build_manifest}" \
    "cache-guard build manifest"
}

validate_source_receipts() {
  local manifest="$1"
  local receipts receipt source count=0 relative
  declare -A seen=()
  receipts="$(kv_value "${manifest}" source_file_receipts)"
  [[ "${receipts}" != *"/data/raw/"* ]] ||
    fail "dry-run manifest contains canonical data/raw receipt"
  while [[ -n "${receipts}" ]]; do
    if [[ "${receipts}" == *";;"* ]]; then
      receipt="${receipts%%;;*}"
      receipts="${receipts#*;;}"
    else
      receipt="${receipts}"
      receipts=""
    fi
    [[ "${receipt}" == *"|source="* ]] ||
      fail "malformed source receipt: ${receipt}"
    source="${receipt##*|source=}"
    reject_data_raw_path "${source}"
    [[ "${source}" == "${source_root}/"* ]] ||
      fail "source receipt escapes isolated mirror: ${source}"
    relative="${source#"${source_root}/"}"
    array_contains "${relative}" "${csv_relatives[@]}" ||
      fail "source receipt names an unexpected file: ${source}"
    [[ -z "${seen[${source}]:-}" ]] ||
      fail "duplicate source receipt: ${source}"
    seen["${source}"]=1
    count=$((count + 1))
  done
  [[ "${count}" == "${prefix_source_count}" ]] ||
    fail "dry-run manifest contains ${count} source receipts instead of 9"
  for source in "${mirror_csv_paths[@]}"; do
    [[ -n "${seen[${source}]:-}" ]] ||
      fail "dry-run manifest omits isolated source: ${source}"
  done
}

validate_dry_run_artifacts() {
  require_regular_file "${job_manifest}"
  require_regular_file "${runtime_result}"
  expect_kv "${runtime_result}" status dry_run
  expect_kv "${runtime_result}" optimizer_steps 0
  expect_kv "${runtime_result}" checkpoint_written false
  expect_kv "${runtime_result}" checkpoint_path ''
  expect_kv "${runtime_result}" model_state_mutated false
  expect_kv "${runtime_result}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${runtime_result}" wave_action train
  expect_kv "${job_manifest}" config_path "${dry_run_config}"
  expect_kv "${job_manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${job_manifest}" wave_action train
  expect_kv "${job_manifest}" source_cursor_id split:train_core
  expect_kv "${job_manifest}" source_range_policy anchor_index
  expect_kv "${job_manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${job_manifest}" resolved_anchor_index_end "${train_end}"
  expect_kv "${job_manifest}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${job_manifest}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${job_manifest}" skipped_outside_common_range 0
  expect_kv "${job_manifest}" skipped_missing_edge_coverage 0
  expect_kv "${job_manifest}" skipped_failed_fetch_probe 0
  expect_kv "${job_manifest}" duplicate_anchor_count 0
  local cursor_token
  cursor_token="$(kv_value "${job_manifest}" source_cursor_token)"
  [[ "${cursor_token}" == *"|accepted=${accepted_anchor_count}|"* &&
    "${cursor_token}" == *"|candidates=${accepted_anchor_count}|"* &&
    "${cursor_token}" == *"|first=${source_cursor_first_key}|"* &&
    "${cursor_token}" == *"|last=${source_cursor_last_key}"* ]] ||
    fail "dry-run source cursor does not prove the corrected isolated domain"
  validate_source_receipts "${job_manifest}"
  if find "${job_dir}" -mindepth 1 -iname '*replay*' -print -quit |
    grep -q .; then
    fail "isolated dry-run job contains a replay artifact"
  fi
}

run_or_resume_dry_run() {
  if [[ -e "${job_dir}" ]]; then
    require_directory "${job_dir}"
    [[ -f "${job_manifest}" && -f "${runtime_result}" ]] ||
      fail "partial dry-run job exists; refusing an ambiguous retry"
    validate_dry_run_artifacts
    return
  fi
  [[ "$(find "${source_root}" -perm /222 -print -quit)" != "" ]] ||
    fail "isolated source is sealed before cache construction"
  "${runtime_exec}" \
    --config "${dry_run_config}" \
    --job-dir "${job_dir}" \
    --dry-run \
    --force-rebuild-cache \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --no-replay-artifacts >"${runtime_log}" 2>&1 ||
    fail "isolated cache-building dry run failed; see ${runtime_log}"
  validate_dry_run_artifacts
}

run_cache_freshness_guard() {
  require_executable "${cache_guard_binary}"
  validate_source_tree "${source_root}" require_complete
  "${cache_guard_binary}" "${cache_guard_args[@]}" >/dev/null ||
    fail "isolated source cache freshness guard failed"
}

write_source_manifest() {
  local candidate index relative prefix mirror raw normalized formatted
  candidate="$(mktemp "${runtime_root}/.source_manifest.XXXXXX")"
  {
    echo "schema_id=${schema_id}.source_manifest.v1"
    echo "status=complete"
    echo "prefix_source_root=${prefix_root}"
    echo "isolated_source_root=${source_root}"
    echo "prefix_source_count=${prefix_source_count}"
    echo "mirror_csv_count=${mirror_csv_count}"
    echo "mirror_cache_count=${mirror_cache_count}"
    for index in "${!csv_relatives[@]}"; do
      formatted="$(printf '%02d' "${index}")"
      relative="${csv_relatives[${index}]}"
      prefix="${prefix_root}/${relative}"
      mirror="${source_root}/${relative}"
      raw="${source_root}/${raw_cache_relatives[${index}]}"
      normalized="${source_root}/${normalized_cache_relatives[${index}]}"
      echo "source.${formatted}.relative_path=${relative}"
      echo "source.${formatted}.prefix_path=${prefix}"
      echo "source.${formatted}.prefix_sha256=$(sha256_of "${prefix}")"
      echo "source.${formatted}.prefix_size_bytes=$(stat -c '%s' -- "${prefix}")"
      echo "source.${formatted}.mirror_path=${mirror}"
      echo "source.${formatted}.mirror_sha256=$(sha256_of "${mirror}")"
      echo "source.${formatted}.mirror_size_bytes=$(stat -c '%s' -- "${mirror}")"
      echo "source.${formatted}.raw_cache_path=${raw}"
      echo "source.${formatted}.raw_cache_sha256=$(sha256_of "${raw}")"
      echo "source.${formatted}.raw_cache_size_bytes=$(stat -c '%s' -- "${raw}")"
      echo "source.${formatted}.normalized_cache_path=${normalized}"
      echo "source.${formatted}.normalized_cache_sha256=$(sha256_of "${normalized}")"
      echo "source.${formatted}.normalized_cache_size_bytes=$(stat -c '%s' -- "${normalized}")"
    done
    echo "canonical_data_raw_access=false"
    echo "accepted_anchor_count=${accepted_anchor_count}"
    echo "candidate_anchor_count=${accepted_anchor_count}"
    echo "maximum_anchor_index=${maximum_anchor_index}"
    echo "source_cursor_first_master_day_index=${source_cursor_first_master_day_index}"
    echo "source_cursor_last_master_day_index=${source_cursor_last_master_day_index}"
    echo "final_holdout_available=false"
  } >"${candidate}"
  publish_immutable_file "${candidate}" "${source_manifest}" \
    "isolated source manifest"
}

validate_source_manifest() {
  require_regular_file "${source_manifest}"
  expect_kv "${source_manifest}" schema_id \
    "${schema_id}.source_manifest.v1"
  expect_kv "${source_manifest}" status complete
  expect_kv "${source_manifest}" prefix_source_count \
    "${prefix_source_count}"
  expect_kv "${source_manifest}" mirror_csv_count "${mirror_csv_count}"
  expect_kv "${source_manifest}" mirror_cache_count "${mirror_cache_count}"
  expect_kv "${source_manifest}" canonical_data_raw_access false
  expect_kv "${source_manifest}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_manifest}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_manifest}" maximum_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${source_manifest}" source_cursor_first_master_day_index \
    "${source_cursor_first_master_day_index}"
  expect_kv "${source_manifest}" source_cursor_last_master_day_index \
    "${source_cursor_last_master_day_index}"
  expect_kv "${source_manifest}" final_holdout_available false
  local index formatted relative prefix mirror raw normalized
  for index in "${!csv_relatives[@]}"; do
    formatted="$(printf '%02d' "${index}")"
    relative="${csv_relatives[${index}]}"
    prefix="${prefix_root}/${relative}"
    mirror="${source_root}/${relative}"
    raw="${source_root}/${raw_cache_relatives[${index}]}"
    normalized="${source_root}/${normalized_cache_relatives[${index}]}"
    expect_kv "${source_manifest}" "source.${formatted}.relative_path" \
      "${relative}"
    expect_kv "${source_manifest}" "source.${formatted}.prefix_path" \
      "${prefix}"
    expect_kv "${source_manifest}" "source.${formatted}.prefix_sha256" \
      "$(sha256_of "${prefix}")"
    expect_kv "${source_manifest}" "source.${formatted}.prefix_size_bytes" \
      "$(stat -c '%s' -- "${prefix}")"
    expect_kv "${source_manifest}" "source.${formatted}.mirror_path" \
      "${mirror}"
    expect_kv "${source_manifest}" "source.${formatted}.mirror_sha256" \
      "$(sha256_of "${mirror}")"
    expect_kv "${source_manifest}" "source.${formatted}.mirror_size_bytes" \
      "$(stat -c '%s' -- "${mirror}")"
    expect_kv "${source_manifest}" "source.${formatted}.raw_cache_path" \
      "${raw}"
    expect_kv "${source_manifest}" "source.${formatted}.raw_cache_sha256" \
      "$(sha256_of "${raw}")"
    expect_kv "${source_manifest}" "source.${formatted}.raw_cache_size_bytes" \
      "$(stat -c '%s' -- "${raw}")"
    expect_kv "${source_manifest}" \
      "source.${formatted}.normalized_cache_path" "${normalized}"
    expect_kv "${source_manifest}" \
      "source.${formatted}.normalized_cache_sha256" \
      "$(sha256_of "${normalized}")"
    expect_kv "${source_manifest}" \
      "source.${formatted}.normalized_cache_size_bytes" \
      "$(stat -c '%s' -- "${normalized}")"
  done
}

declare -a receipt_kinds=()
declare -a receipt_paths=()
declare -A receipt_seen=()

add_receipt_entry() {
  local kind="$1"
  local path="$2"
  reject_data_raw_path "${path}"
  reject_symlink_components "${path}"
  path="$(realpath -e -- "${path}")"
  reject_data_raw_path "${path}"
  require_regular_file "${path}"
  if [[ -n "${receipt_seen[${path}]:-}" ]]; then
    return
  fi
  receipt_seen["${path}"]=1
  receipt_kinds+=("${kind}")
  receipt_paths+=("${path}")
}

collect_config_dependencies() {
  local config="$1"
  local line lhs rhs key value
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] || continue
    lhs="${line%%=*}"
    rhs="${line#*=}"
    key="$(trim "${lhs}")"
    value="$(trim "${rhs}")"
    [[ "${key}" == *_path ]] || continue
    reject_data_raw_path "${value}"
    add_receipt_entry config_dependency "${value}"
  done <"${config}"
}

collect_receipt_entries() {
  local path compiler
  receipt_kinds=()
  receipt_paths=()
  receipt_seen=()
  add_receipt_entry preparation_runner "${script_path}"
  add_receipt_entry old_closure_document "${old_closure_report}"
  add_receipt_entry fresh_preregistration "${fresh_preregistration}"
  add_receipt_entry source_isolation_amendment "${isolation_amendment}"
  add_receipt_entry isolated_source_protocol "${isolated_protocol}"
  add_receipt_entry staged_hardening_amendment "${staged_hardening}"
  add_receipt_entry cursor_alignment_correction \
    "${cursor_alignment_correction}"
  add_receipt_entry canonical_registry_source "${canonical_registry_source}"
  add_receipt_entry canonical_base_config_source "${base_config_source}"
  add_receipt_entry isolated_registry "${isolated_registry}"
  add_receipt_entry isolated_base_config "${isolated_base_config}"
  add_receipt_entry isolated_dry_run_config "${dry_run_config}"
  collect_config_dependencies "${isolated_base_config}"
  collect_config_dependencies "${dry_run_config}"
  add_receipt_entry runtime_executable "${runtime_exec}"
  add_receipt_entry cache_guard_source "${cache_guard_source}"
  add_receipt_entry cache_guard_header "${cache_guard_header}"
  add_receipt_entry cache_guard_binary "${cache_guard_binary}"
  add_receipt_entry cache_guard_build_manifest \
    "${cache_guard_build_manifest}"
  compiler="$(kv_value "${cache_guard_build_manifest}" compiler_path)"
  add_receipt_entry cache_guard_compiler "${compiler}"
  add_receipt_entry source_manifest "${source_manifest}"
  add_receipt_entry dry_run_job_manifest "${job_manifest}"
  add_receipt_entry dry_run_runtime_result "${runtime_result}"
  for path in "${prefix_csv_paths[@]}"; do
    add_receipt_entry prefix_source_csv "${path}"
  done
  for path in "${mirror_csv_paths[@]}"; do
    add_receipt_entry mirror_source_csv "${path}"
  done
  for path in "${mirror_cache_paths[@]}"; do
    add_receipt_entry mirror_source_cache "${path}"
  done
}

emit_closure_candidate() {
  local destination="$1"
  local index formatted path kind
  collect_receipt_entries
  {
    echo "schema_id=${schema_id}"
    echo "status=complete"
    echo "accepted_anchor_count=${accepted_anchor_count}"
    echo "candidate_anchor_count=${accepted_anchor_count}"
    echo "maximum_anchor_index=${maximum_anchor_index}"
    echo "source_cursor_first_master_day_index=${source_cursor_first_master_day_index}"
    echo "source_cursor_last_master_day_index=${source_cursor_last_master_day_index}"
    echo "skipped_outside_common_range=0"
    echo "skipped_missing_edge_coverage=0"
    echo "skipped_failed_fetch_probe=0"
    echo "duplicate_anchor_count=0"
    echo "isolated_source_root_path=${source_root}"
    echo "isolated_registry_path=${isolated_registry}"
    echo "isolated_registry_sha256=$(sha256_of "${isolated_registry}")"
    echo "isolated_base_config_path=${isolated_base_config}"
    echo "isolated_base_config_sha256=$(sha256_of "${isolated_base_config}")"
    echo "runtime_exec_path=${runtime_exec}"
    echo "runtime_exec_sha256=$(sha256_of "${runtime_exec}")"
    echo "source_manifest_path=${source_manifest}"
    echo "source_manifest_sha256=$(sha256_of "${source_manifest}")"
    echo "prefix_source_count=${prefix_source_count}"
    echo "mirror_csv_count=${mirror_csv_count}"
    echo "mirror_cache_count=${mirror_cache_count}"
    echo "canonical_data_raw_access=false"
    echo "final_holdout_available=false"
    echo "old_closure_report_path=${old_closure_report}"
    echo "old_closure_report_sha256=$(sha256_of "${old_closure_report}")"
    echo "isolation_amendment_path=${isolation_amendment}"
    echo "isolation_amendment_sha256=$(sha256_of "${isolation_amendment}")"
    echo "isolated_protocol_path=${isolated_protocol}"
    echo "isolated_protocol_sha256=$(sha256_of "${isolated_protocol}")"
    echo "staged_hardening_path=${staged_hardening}"
    echo "staged_hardening_sha256=$(sha256_of "${staged_hardening}")"
    echo "cursor_alignment_correction_path=${cursor_alignment_correction}"
    echo "cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")"
    echo "dry_run_config_path=${dry_run_config}"
    echo "dry_run_config_sha256=$(sha256_of "${dry_run_config}")"
    echo "cache_guard_binary_path=${cache_guard_binary}"
    echo "cache_guard_binary_sha256=$(sha256_of "${cache_guard_binary}")"
    echo "cache_guard_build_manifest_path=${cache_guard_build_manifest}"
    echo "cache_guard_build_manifest_sha256=$(sha256_of "${cache_guard_build_manifest}")"
    echo "dry_run_job_manifest_path=${job_manifest}"
    echo "dry_run_job_manifest_sha256=$(sha256_of "${job_manifest}")"
    echo "dry_run_runtime_result_path=${runtime_result}"
    echo "dry_run_runtime_result_sha256=$(sha256_of "${runtime_result}")"
    echo "train_anchor_range=[${train_begin},${train_end})"
    echo "optimizer_steps=0"
    echo "checkpoint_written=false"
    echo "model_state_mutated=false"
    echo "replay_artifacts=false"
    echo "strict_cache_freshness=pass"
    echo "source_tree_read_only=true"
    echo "config_read_only=true"
    echo "independent_final_evidence=false"
    echo "entry_count=${#receipt_paths[@]}"
    for index in "${!receipt_paths[@]}"; do
      formatted="$(printf '%03d' "${index}")"
      path="${receipt_paths[${index}]}"
      kind="${receipt_kinds[${index}]}"
      echo "entry.${formatted}.kind=${kind}"
      echo "entry.${formatted}.path=${path}"
      echo "entry.${formatted}.sha256=$(sha256_of "${path}")"
      echo "entry.${formatted}.size_bytes=$(stat -c '%s' -- "${path}")"
      echo "entry.${formatted}.mtime=$(stat -c '%y' -- "${path}")"
    done
  } >"${destination}"
}

verify_closure_entries() {
  local receipt="$1"
  local count index formatted expected_kind expected_path actual_kind actual_path
  local expected_sha expected_size expected_mtime
  collect_receipt_entries
  count="$(kv_value "${receipt}" entry_count)"
  [[ "${count}" == "${#receipt_paths[@]}" ]] ||
    fail "closure entry_count differs from the fixed provenance set"
  for index in "${!receipt_paths[@]}"; do
    formatted="$(printf '%03d' "${index}")"
    expected_kind="${receipt_kinds[${index}]}"
    expected_path="${receipt_paths[${index}]}"
    actual_kind="$(kv_value "${receipt}" "entry.${formatted}.kind")"
    actual_path="$(kv_value "${receipt}" "entry.${formatted}.path")"
    reject_data_raw_path "${actual_path}"
    [[ "${actual_kind}" == "${expected_kind}" ]] ||
      fail "closure entry kind drift at ${formatted}"
    [[ "${actual_path}" == "${expected_path}" ]] ||
      fail "closure entry path drift at ${formatted}"
    expected_sha="$(kv_value "${receipt}" "entry.${formatted}.sha256")"
    expected_size="$(kv_value "${receipt}" "entry.${formatted}.size_bytes")"
    expected_mtime="$(kv_value "${receipt}" "entry.${formatted}.mtime")"
    require_regular_file "${actual_path}"
    [[ "$(sha256_of "${actual_path}")" == "${expected_sha}" ]] ||
      fail "closure entry hash drift: ${actual_path}"
    [[ "$(stat -c '%s' -- "${actual_path}")" == "${expected_size}" ]] ||
      fail "closure entry size drift: ${actual_path}"
    [[ "$(stat -c '%y' -- "${actual_path}")" == "${expected_mtime}" ]] ||
      fail "closure entry mtime drift: ${actual_path}"
  done
}

validate_closure_candidate() {
  local receipt="$1"
  require_regular_file "${receipt}"
  expect_kv "${receipt}" schema_id "${schema_id}"
  expect_kv "${receipt}" status complete
  expect_kv "${receipt}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${receipt}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${receipt}" maximum_anchor_index "${maximum_anchor_index}"
  expect_kv "${receipt}" source_cursor_first_master_day_index \
    "${source_cursor_first_master_day_index}"
  expect_kv "${receipt}" source_cursor_last_master_day_index \
    "${source_cursor_last_master_day_index}"
  expect_kv "${receipt}" skipped_outside_common_range 0
  expect_kv "${receipt}" skipped_missing_edge_coverage 0
  expect_kv "${receipt}" skipped_failed_fetch_probe 0
  expect_kv "${receipt}" duplicate_anchor_count 0
  expect_kv "${receipt}" isolated_source_root_path "${source_root}"
  expect_kv "${receipt}" isolated_registry_path "${isolated_registry}"
  expect_kv "${receipt}" isolated_registry_sha256 \
    "$(sha256_of "${isolated_registry}")"
  expect_kv "${receipt}" isolated_base_config_path \
    "${isolated_base_config}"
  expect_kv "${receipt}" isolated_base_config_sha256 \
    "$(sha256_of "${isolated_base_config}")"
  expect_kv "${receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${receipt}" runtime_exec_sha256 \
    "$(sha256_of "${runtime_exec}")"
  expect_kv "${receipt}" source_manifest_path "${source_manifest}"
  expect_kv "${receipt}" source_manifest_sha256 \
    "$(sha256_of "${source_manifest}")"
  expect_kv "${receipt}" prefix_source_count "${prefix_source_count}"
  expect_kv "${receipt}" mirror_csv_count "${mirror_csv_count}"
  expect_kv "${receipt}" mirror_cache_count "${mirror_cache_count}"
  expect_kv "${receipt}" canonical_data_raw_access false
  expect_kv "${receipt}" final_holdout_available false
  expect_kv "${receipt}" strict_cache_freshness pass
  expect_kv "${receipt}" source_tree_read_only true
  expect_kv "${receipt}" config_read_only true
  expect_kv "${receipt}" isolation_amendment_sha256 \
    "${expected_isolation_amendment_sha256}"
  expect_kv "${receipt}" isolated_protocol_sha256 \
    "${expected_isolated_protocol_sha256}"
  expect_kv "${receipt}" staged_hardening_sha256 \
    "${expected_staged_hardening_sha256}"
  expect_kv "${receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  verify_closure_entries "${receipt}"
}

assert_read_only_seal() {
  local writable path
  writable="$(find "${source_root}" "${config_dir}" -perm /222 \
    -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "sealed isolated source/config retains a write bit: ${writable}"
  [[ "$(stat -c '%A' -- "${cache_guard_binary}")" != *w* ]] ||
    fail "cache guard binary remains writable"
  [[ "$(stat -c '%A' -- "${source_manifest}")" != *w* ]] ||
    fail "source manifest remains writable"
  for path in "${cache_guard_build_manifest}" "${job_manifest}" \
    "${runtime_result}"; do
    [[ "$(stat -c '%A' -- "${path}")" != *w* ]] ||
      fail "sealed provenance artifact remains writable: ${path}"
  done
  if [[ -e "${runtime_log}" ]]; then
    [[ "$(stat -c '%A' -- "${runtime_log}")" != *w* ]] ||
      fail "sealed Runtime log remains writable: ${runtime_log}"
  fi
}

seal_preclosure_artifacts() {
  find "${source_root}" -type f -exec chmod a-w -- {} +
  find "${source_root}" -depth -type d -exec chmod a-w -- {} +
  find "${config_dir}" -type f -exec chmod a-w -- {} +
  find "${config_dir}" -depth -type d -exec chmod a-w -- {} +
  chmod 0555 "${cache_guard_binary}"
  chmod 0444 "${cache_guard_build_manifest}" "${source_manifest}" \
    "${job_manifest}" "${runtime_result}"
  [[ ! -e "${runtime_log}" ]] || chmod 0444 "${runtime_log}"
  assert_read_only_seal
}

verify_completed_closure() {
  verify_static_inputs
  load_prefix_metadata_from_old_closure
  validate_prefix_inventory
  require_regular_file "${closure_receipt}"
  [[ "$(stat -c '%A' -- "${closure_receipt}")" != *w* ]] ||
    fail "isolated closure receipt is writable"
  expect_kv "${closure_receipt}" schema_id "${schema_id}"
  expect_kv "${closure_receipt}" status complete
  expect_kv "${closure_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${closure_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${closure_receipt}" maximum_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${closure_receipt}" source_cursor_first_master_day_index \
    "${source_cursor_first_master_day_index}"
  expect_kv "${closure_receipt}" source_cursor_last_master_day_index \
    "${source_cursor_last_master_day_index}"
  expect_kv "${closure_receipt}" skipped_outside_common_range 0
  expect_kv "${closure_receipt}" skipped_missing_edge_coverage 0
  expect_kv "${closure_receipt}" skipped_failed_fetch_probe 0
  expect_kv "${closure_receipt}" duplicate_anchor_count 0
  expect_kv "${closure_receipt}" isolated_source_root_path \
    "${source_root}"
  expect_kv "${closure_receipt}" isolated_registry_path \
    "${isolated_registry}"
  expect_kv "${closure_receipt}" isolated_registry_sha256 \
    "$(sha256_of "${isolated_registry}")"
  expect_kv "${closure_receipt}" isolated_base_config_path \
    "${isolated_base_config}"
  expect_kv "${closure_receipt}" isolated_base_config_sha256 \
    "$(sha256_of "${isolated_base_config}")"
  expect_kv "${closure_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${closure_receipt}" runtime_exec_sha256 \
    "$(sha256_of "${runtime_exec}")"
  expect_kv "${closure_receipt}" source_manifest_path "${source_manifest}"
  expect_kv "${closure_receipt}" source_manifest_sha256 \
    "$(sha256_of "${source_manifest}")"
  expect_kv "${closure_receipt}" prefix_source_count \
    "${prefix_source_count}"
  expect_kv "${closure_receipt}" mirror_csv_count "${mirror_csv_count}"
  expect_kv "${closure_receipt}" mirror_cache_count "${mirror_cache_count}"
  expect_kv "${closure_receipt}" canonical_data_raw_access false
  expect_kv "${closure_receipt}" final_holdout_available false
  expect_kv "${closure_receipt}" strict_cache_freshness pass
  expect_kv "${closure_receipt}" source_tree_read_only true
  expect_kv "${closure_receipt}" config_read_only true
  expect_kv "${closure_receipt}" isolation_amendment_sha256 \
    "${expected_isolation_amendment_sha256}"
  expect_kv "${closure_receipt}" isolated_protocol_sha256 \
    "${expected_isolated_protocol_sha256}"
  expect_kv "${closure_receipt}" staged_hardening_sha256 \
    "${expected_staged_hardening_sha256}"
  expect_kv "${closure_receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${closure_receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  validate_source_tree "${source_root}" require_complete
  validate_registry_and_configs
  validate_dry_run_artifacts
  validate_source_manifest
  validate_closure_candidate "${closure_receipt}"
  assert_read_only_seal
  run_cache_freshness_guard
  validate_prefix_inventory
  echo "verified isolated development source closure: ${closure_receipt}"
}

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
runtime_root=${runtime_root}
isolated_source_root_path=${source_root}
isolated_registry_path=${isolated_registry}
isolated_base_config_path=${isolated_base_config}
closure_receipt_path=${closure_receipt}
prefix_source_root=${prefix_root}
prefix_source_count=${prefix_source_count}
mirror_csv_count=${mirror_csv_count}
mirror_cache_count=${mirror_cache_count}
accepted_anchor_count=${accepted_anchor_count}
maximum_anchor_index=${maximum_anchor_index}
dry_run_anchor_range=[${train_begin},${train_end})
optimizer_steps=0
checkpoint_written=false
force_rebuild_cache=true
strict_cache_freshness=true
canonical_data_raw_access=false
old_closure_verifier_invoked=false
source_tree_read_only_before_closure=true
final_holdout_available=false
independent_final_evidence=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  verify_static_inputs
  print_plan
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  verify_completed_closure
  exit 0
fi

verify_static_inputs
load_prefix_metadata_from_old_closure
validate_prefix_inventory
prepare_output_hierarchy
exec 9>"${runtime_root}/.preparation.lock"
flock -n 9 || fail "another isolated-source preparation holds the lock"
[[ ! -e "${closure_receipt}" ]] ||
  fail "isolated closure already exists; use --verify"

assert_no_stale_candidates
install_source_mirror
write_isolated_registry
write_isolated_base_config
write_dry_run_config
validate_registry_and_configs
compile_cache_guard
run_or_resume_dry_run
validate_prefix_inventory
validate_source_tree "${source_root}" require_complete
run_cache_freshness_guard
write_source_manifest
validate_source_manifest
seal_preclosure_artifacts
run_cache_freshness_guard
validate_prefix_inventory

closure_candidate="$(mktemp "${runtime_root}/.closure.XXXXXX")"
emit_closure_candidate "${closure_candidate}"
chmod 0444 "${closure_candidate}"
validate_closure_candidate "${closure_candidate}"
ln -- "${closure_candidate}" "${closure_receipt}" ||
  fail "could not atomically publish isolated development-source closure"
rm -f -- "${closure_candidate}"
sync -f "${closure_receipt}"
verify_completed_closure
