#!/usr/bin/env bash
set -euo pipefail

readonly expected_prereg_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly closure_schema="synthetic_fresh_seed_data_closure.v2"
readonly dataset_manifest_name="synthetic_quasiperiodic_chart_manifest.v2.report"
readonly closure_report_name="fresh_seed_data_closure.v2.report"
readonly train_begin=0
readonly train_end=2496
readonly accepted_anchor_count=4096

fail() {
  echo "synthetic v2 preparation: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'USAGE'
usage: prepare_and_seal_fresh_seed.sh [--plan|--prepare|--verify]

  --plan     Print the fail-closed preparation contract without writing.
  --prepare  Generate/resume pre-seal data preparation, build caches with a
             zero-step dry run, verify the closure, and seal the data tree.
  --verify   Read only the benchmark/repository state and verify an existing
             completed closure. Temporary validator binaries are rebuilt
             under /tmp/runtime.

SYNTHETIC_V2_BENCHMARK_ROOT may select an existing benchmark root. The root
is canonicalized before use and must contain the fixed preregistration and
v2 configuration. An override receives a root-specific isolated preparation
directory so it cannot reuse canonical preparation artifacts.
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
  [[ -f "${path}" ]] || fail "required file is absent: ${path}"
  [[ ! -L "${path}" ]] || fail "required file is a symbolic link: ${path}"
}

require_executable() {
  require_regular_file "$1"
  [[ -x "$1" ]] || fail "required executable is not executable: $1"
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
default_benchmark_root="$(realpath -e -- \
  "${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2")"
requested_benchmark_root="${SYNTHETIC_V2_BENCHMARK_ROOT:-${default_benchmark_root}}"

[[ "${requested_benchmark_root}" != *$'\n'* ]] ||
  fail "benchmark root must not contain a newline"
[[ -d "${requested_benchmark_root}" ]] ||
  fail "benchmark root must already exist: ${requested_benchmark_root}"
[[ ! -L "${requested_benchmark_root}" ]] ||
  fail "benchmark root must not be a symbolic link: ${requested_benchmark_root}"
benchmark_root="$(realpath -e -- "${requested_benchmark_root}")"
[[ "${benchmark_root}" != "/" && "${benchmark_root}" != "${repo_root}" ]] ||
  fail "unsafe benchmark root: ${benchmark_root}"

runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/preparation"
if [[ "${benchmark_root}" != "${default_benchmark_root}" ]]; then
  root_scope="$(printf '%s' "${benchmark_root}" | sha256sum | cut -c1-16)"
  runtime_root="${runtime_root}.override_${root_scope}"
fi

preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
base_config="${benchmark_root}/synthetic_benchmark.config"
data_root="${benchmark_root}/data"
raw_root="${data_root}/raw"
prefix_root="${data_root}/development_prefix"
artifacts_root="${benchmark_root}/artifacts"
dataset_manifest="${artifacts_root}/${dataset_manifest_name}"
closure_report="${artifacts_root}/${closure_report_name}"

generator_runner="${script_dir}/generate_synthetic_klines.sh"
generator_source="${script_dir}/generate_synthetic_klines.cpp"
validator_source="${script_dir}/validate_synthetic_dataset.cpp"
cache_guard_source="${repo_root}/src/scripts/benchmarks/synthetic_continuous_graph_v1/check_frozen_cache_freshness.cpp"
cache_guard_header="${repo_root}/src/include/ujcamei/source/retrieval/storage/memory_mapped/cache_freshness.h"
sha256_header="${repo_root}/src/include/piaabo/digest/sha256.h"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"

config_dir="${runtime_root}/config"
job_parent="${runtime_root}/job"
job_dir="${job_parent}/train_core_representation_cache_build_dry_run"
logs_dir="${runtime_root}/logs"
runtime_log="${logs_dir}/train_core_representation_cache_build_dry_run.log"
config_snapshot="${config_dir}/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.config"
binding_receipt="${runtime_root}/fresh_seed_data_closure.v2.bindings"

tmp_runtime_root="/tmp/runtime/synthetic_continuous_graph_v2"
validator_binary="${tmp_runtime_root}/validate_synthetic_dataset"
cache_guard_binary="${tmp_runtime_root}/check_frozen_cache_freshness"

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

require_regular_file "${preregistration}"
actual_prereg_sha256="$(sha256_of "${preregistration}")"
[[ "${actual_prereg_sha256}" == "${expected_prereg_sha256}" ]] ||
  fail "preregistration SHA-256 drift: expected ${expected_prereg_sha256}, found ${actual_prereg_sha256}"

print_plan() {
  cat <<PLAN
schema_id=${closure_schema}.plan
benchmark_root=${benchmark_root}
preparation_runtime_root=${runtime_root}
preregistration_sha256=${actual_prereg_sha256}
train_anchor_range=[${train_begin},${train_end})
authoritative_accepted_anchor_count=${accepted_anchor_count}
preparation_wave=train_core_mtf_jepa_mae_vicreg
runtime_action=dry_run
force_rebuild_cache=true
optimizer_steps=0
checkpoint_written=false
replay_artifacts=false
raw_cache_chain_count=9
closure_csv_count=18
closure_cache_count=18
data_tree_sealed_read_only=true
final_holdout_scoring=false
final_consumption_ledger=false
prepare_resume_policy=idempotent_before_closure_and_fail_closed_after_closure
verify_policy=benchmark_and_repository_read_only
PLAN
}

if [[ "${mode}" == "--plan" ]]; then
  print_plan
  exit 0
fi

declare -a instruments=(
  SYN2ALPHASYN2USD
  SYN2BETASYN2USD
  SYN2GAMMASYN2USD
)
declare -a intervals=(1w 3d 1d)
declare -a csv_paths=()
declare -a raw_csv_paths=()
declare -a cache_paths=()
declare -a cache_guard_args=()

populate_expected_data_paths() {
  local tree instrument interval stem csv raw_cache normalized_cache
  csv_paths=()
  raw_csv_paths=()
  cache_paths=()
  cache_guard_args=()
  for tree in raw development_prefix; do
    for instrument in "${instruments[@]}"; do
      for interval in "${intervals[@]}"; do
        stem="${instrument}-${interval}-all-years"
        csv="${data_root}/${tree}/${instrument}/${interval}/${stem}.csv"
        csv_paths+=("${csv}")
        if [[ "${tree}" == raw ]]; then
          raw_cache="${data_root}/${tree}/${instrument}/${interval}/${stem}.cache.bin"
          normalized_cache="${data_root}/${tree}/${instrument}/${interval}/${stem}.cache.norm.log_returns.bin"
          raw_csv_paths+=("${csv}")
          cache_paths+=("${raw_cache}" "${normalized_cache}")
          cache_guard_args+=("${csv}" "${raw_cache}" "${normalized_cache}")
        fi
      done
    done
  done
}

populate_expected_data_paths

validate_exact_data_inventory() {
  local path actual_csv_count actual_cache_count symlink
  require_regular_file "${dataset_manifest}"
  [[ -d "${raw_root}" ]] || fail "raw data directory is absent: ${raw_root}"
  [[ -d "${prefix_root}" ]] ||
    fail "development-prefix directory is absent: ${prefix_root}"

  symlink="$(find "${data_root}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "data tree contains a symbolic link: ${symlink}"

  for path in "${csv_paths[@]}" "${cache_paths[@]}"; do
    require_regular_file "${path}"
  done

  actual_csv_count="$(find "${raw_root}" "${prefix_root}" -type f -name '*.csv' -printf '.' | wc -c)"
  [[ "${actual_csv_count}" == 18 ]] ||
    fail "expected exactly 18 source CSVs, found ${actual_csv_count}"
  actual_cache_count="$(find "${raw_root}" -type f \
    \( -name '*.cache.bin' -o -name '*.cache.norm.log_returns.bin' \) \
    -printf '.' | wc -c)"
  [[ "${actual_cache_count}" == 18 ]] ||
    fail "expected exactly 18 raw/normalized caches, found ${actual_cache_count}"
}

assert_data_tree_read_only() {
  local writable
  writable="$(find "${data_root}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "sealed data path retains a write bit: ${writable}"
}

compile_validation_tools() {
  require_regular_file "${validator_source}"
  require_regular_file "${cache_guard_source}"
  require_regular_file "${cache_guard_header}"
  mkdir -p "${tmp_runtime_root}"
  (
    cd "${repo_root}"
    g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc/include \
      "${validator_source}" -o "${validator_binary}.tmp.$$"
    mv -f -- "${validator_binary}.tmp.$$" "${validator_binary}"
    g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc/include \
      "${cache_guard_source}" -o "${cache_guard_binary}.tmp.$$"
    mv -f -- "${cache_guard_binary}.tmp.$$" "${cache_guard_binary}"
  )
}

run_dataset_validator() {
  require_executable "${validator_binary}"
  "${validator_binary}" --benchmark-root "${benchmark_root}"
}

run_cache_freshness_guard() {
  require_executable "${cache_guard_binary}"
  validate_exact_data_inventory
  "${cache_guard_binary}" "${cache_guard_args[@]}"
}

write_config_snapshot() {
  require_regular_file "${base_config}"
  local candidate line lhs rhs key value indent absolute
  candidate="$(mktemp "${config_dir}/.config.XXXXXX")"
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
    if [[ "${key}" == runtime_wave_id ]]; then
      printf '%s%s = train_core_mtf_jepa_mae_vicreg\n' \
        "${indent}" "${key}" >>"${candidate}"
    elif [[ "${key}" == *_path && "${value}" != /* ]]; then
      absolute="$(realpath -m -- "${benchmark_root}/${value}")"
      printf '%s%s = %s\n' "${indent}" "${key}" "${absolute}" \
        >>"${candidate}"
    else
      printf '%s\n' "${line}" >>"${candidate}"
    fi
  done <"${base_config}"

  if [[ -e "${config_snapshot}" ]]; then
    require_regular_file "${config_snapshot}"
    if ! cmp -s -- "${candidate}" "${config_snapshot}"; then
      rm -f -- "${candidate}"
      fail "immutable config snapshot drifted: ${config_snapshot}"
    fi
    rm -f -- "${candidate}"
  else
    chmod 0444 "${candidate}"
    ln "${candidate}" "${config_snapshot}" || {
      rm -f -- "${candidate}"
      fail "no-clobber config snapshot publication failed"
    }
    rm -f -- "${candidate}"
  fi
}

validate_config_snapshot() {
  require_regular_file "${config_snapshot}"
  expect_kv "${config_snapshot}" runtime_wave_id \
    train_core_mtf_jepa_mae_vicreg
  local line lhs rhs key value
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] || continue
    lhs="${line%%=*}"
    rhs="${line#*=}"
    key="$(trim "${lhs}")"
    value="$(trim "${rhs}")"
    [[ "${key}" == *_path ]] || continue
    [[ "${value}" == /* ]] ||
      fail "config snapshot contains a non-absolute path: ${key}=${value}"
    require_regular_file "$(realpath -m -- "${value}")"
  done <"${config_snapshot}"
}

validate_dry_run_artifacts() {
  local result="${job_dir}/runtime.result.fact"
  local manifest="${job_dir}/job.manifest"
  require_regular_file "${result}"
  require_regular_file "${manifest}"

  expect_kv "${result}" status dry_run
  expect_kv "${result}" optimizer_steps 0
  expect_kv "${result}" checkpoint_written false
  expect_kv "${result}" checkpoint_path ''
  expect_kv "${result}" model_state_mutated false
  expect_kv "${result}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${result}" wave_action train

  expect_kv "${manifest}" config_path "${config_snapshot}"
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" source_cursor_id split:train_core
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${train_end}"
  expect_kv "${manifest}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" skipped_outside_common_range 0
  expect_kv "${manifest}" skipped_missing_edge_coverage 0
  expect_kv "${manifest}" skipped_failed_fetch_probe 0
  expect_kv "${manifest}" duplicate_anchor_count 0

  # Runtime is allowed to emit checkpoint-I/O fact sidecars proving zero
  # writes. checkpoint_written=false and an empty checkpoint_path above are
  # the authoritative payload assertions; only replay-named artifacts are
  # categorically forbidden here.
  if find "${job_dir}" -mindepth 1 -iname '*replay*' -print -quit |
      grep -q .; then
    fail "dry-run job contains a replay artifact: ${job_dir}"
  fi
}

run_or_resume_dry_run() {
  require_executable "${runtime_exec}"
  if [[ -e "${job_dir}" ]]; then
    [[ -d "${job_dir}" ]] || fail "dry-run job path is not a directory"
    [[ -f "${job_dir}/runtime.result.fact" &&
      -f "${job_dir}/job.manifest" ]] ||
      fail "existing dry-run job is partial; refusing an ambiguous retry: ${job_dir}"
    validate_dry_run_artifacts
    return
  fi

  "${runtime_exec}" \
    --config "${config_snapshot}" \
    --job-dir "${job_dir}" \
    --dry-run \
    --force-rebuild-cache \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --no-replay-artifacts >"${runtime_log}" 2>&1 ||
    fail "cache-building dry run failed; see ${runtime_log}"
  validate_dry_run_artifacts
}

declare -a receipt_kinds=()
declare -a receipt_paths=()
declare -A receipt_seen=()

add_receipt_entry() {
  local kind="$1"
  local path="$2"
  path="$(realpath -e -- "${path}")"
  require_regular_file "${path}"
  if [[ -n "${receipt_seen[${path}]:-}" ]]; then
    return
  fi
  receipt_seen["${path}"]=1
  receipt_kinds+=("${kind}")
  receipt_paths+=("${path}")
}

collect_config_inputs() {
  local line lhs rhs key value canonical
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] || continue
    lhs="${line%%=*}"
    rhs="${line#*=}"
    key="$(trim "${lhs}")"
    value="$(trim "${rhs}")"
    [[ "${key}" == *_path ]] || continue
    [[ "${value}" == /* ]] ||
      fail "closure config input is not absolute: ${key}=${value}"
    canonical="$(realpath -e -- "${value}")"
    add_receipt_entry execution_config_input "${canonical}"
  done <"${config_snapshot}"
}

collect_receipt_entries() {
  local path
  receipt_kinds=()
  receipt_paths=()
  receipt_seen=()

  add_receipt_entry preregistration "${preregistration}"
  add_receipt_entry dataset_manifest "${dataset_manifest}"
  add_receipt_entry execution_config_snapshot "${config_snapshot}"
  add_receipt_entry execution_config_base "${base_config}"
  collect_config_inputs
  add_receipt_entry generator_runner "${generator_runner}"
  add_receipt_entry generator_source "${generator_source}"
  add_receipt_entry validator_source "${validator_source}"
  add_receipt_entry validator_dependency "${sha256_header}"
  add_receipt_entry cache_guard_source "${cache_guard_source}"
  add_receipt_entry cache_guard_dependency "${cache_guard_header}"
  add_receipt_entry closure_runner "${script_path}"
  add_receipt_entry runtime_executable "${runtime_exec}"
  add_receipt_entry preparation_job_manifest "${job_dir}/job.manifest"
  add_receipt_entry preparation_runtime_result "${job_dir}/runtime.result.fact"
  for path in "${csv_paths[@]}"; do
    add_receipt_entry source_csv "${path}"
  done
  for path in "${cache_paths[@]}"; do
    add_receipt_entry source_cache "${path}"
  done
}

emit_binding_receipt() {
  local destination="$1"
  local index path kind formatted sha size mtime
  {
    echo "schema_id=${closure_schema}"
    echo "benchmark_root=${benchmark_root}"
    echo "preregistration_sha256=${expected_prereg_sha256}"
    echo "dataset_manifest_name=${dataset_manifest_name}"
    echo "accepted_anchor_count=${accepted_anchor_count}"
    echo "train_anchor_begin=${train_begin}"
    echo "train_anchor_end_exclusive=${train_end}"
    echo "preparation_wave_id=train_core_mtf_jepa_mae_vicreg"
    echo "preparation_status=dry_run"
    echo "preparation_optimizer_steps=0"
    echo "preparation_checkpoint_written=false"
    echo "preparation_replay_artifacts=false"
    echo "csv_entry_count=18"
    echo "cache_entry_count=18"
    echo "cache_chain_count=9"
    echo "entry_count=${#receipt_paths[@]}"
    for index in "${!receipt_paths[@]}"; do
      path="${receipt_paths[${index}]}"
      kind="${receipt_kinds[${index}]}"
      formatted="$(printf '%03d' "${index}")"
      sha="$(sha256_of "${path}")"
      size="$(stat -c '%s' -- "${path}")"
      mtime="$(stat -c '%y' -- "${path}")"
      echo "entry.${formatted}.kind=${kind}"
      echo "entry.${formatted}.path=${path}"
      echo "entry.${formatted}.sha256=${sha}"
      echo "entry.${formatted}.size_bytes=${size}"
      echo "entry.${formatted}.mtime=${mtime}"
    done
  } >"${destination}"
}

verify_receipt_entries() {
  local receipt="$1"
  local count index formatted kind path expected actual expected_size actual_size
  local expected_mtime actual_mtime csv_count=0 cache_count=0
  require_regular_file "${receipt}"
  expect_kv "${receipt}" schema_id "${closure_schema}"
  expect_kv "${receipt}" benchmark_root "${benchmark_root}"
  expect_kv "${receipt}" preregistration_sha256 "${expected_prereg_sha256}"
  expect_kv "${receipt}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${receipt}" train_anchor_begin "${train_begin}"
  expect_kv "${receipt}" train_anchor_end_exclusive "${train_end}"
  expect_kv "${receipt}" preparation_status dry_run
  expect_kv "${receipt}" preparation_optimizer_steps 0
  expect_kv "${receipt}" preparation_checkpoint_written false
  expect_kv "${receipt}" preparation_replay_artifacts false
  expect_kv "${receipt}" csv_entry_count 18
  expect_kv "${receipt}" cache_entry_count 18
  expect_kv "${receipt}" cache_chain_count 9

  count="$(kv_value "${receipt}" entry_count)"
  [[ "${count}" =~ ^[0-9]+$ && "${count}" -gt 0 ]] ||
    fail "invalid closure entry_count: ${count}"
  for ((index = 0; index < count; ++index)); do
    formatted="$(printf '%03d' "${index}")"
    kind="$(kv_value "${receipt}" "entry.${formatted}.kind")"
    path="$(kv_value "${receipt}" "entry.${formatted}.path")"
    expected="$(kv_value "${receipt}" "entry.${formatted}.sha256")"
    expected_size="$(kv_value "${receipt}" "entry.${formatted}.size_bytes")"
    expected_mtime="$(kv_value "${receipt}" "entry.${formatted}.mtime")"
    [[ "${path}" == /* ]] || fail "closure entry is not absolute: ${path}"
    require_regular_file "${path}"
    actual="$(sha256_of "${path}")"
    [[ "${actual}" == "${expected}" ]] ||
      fail "closure hash drift for ${path}: expected ${expected}, found ${actual}"
    actual_size="$(stat -c '%s' -- "${path}")"
    [[ "${actual_size}" == "${expected_size}" ]] ||
      fail "closure size drift for ${path}"
    actual_mtime="$(stat -c '%y' -- "${path}")"
    [[ "${actual_mtime}" == "${expected_mtime}" ]] ||
      fail "closure mtime drift for ${path}"
    [[ "${kind}" == source_csv ]] && csv_count=$((csv_count + 1))
    [[ "${kind}" == source_cache ]] && cache_count=$((cache_count + 1))
  done
  [[ "${csv_count}" == 18 ]] ||
    fail "closure binds ${csv_count} CSV entries instead of 18"
  [[ "${cache_count}" == 18 ]] ||
    fail "closure binds ${cache_count} cache entries instead of 18"
}

verify_completed_closure() {
  local expected_binding_sha actual_binding_sha binding_lines
  require_regular_file "${closure_report}"
  require_regular_file "${binding_receipt}"
  expect_kv "${closure_report}" closure_complete true
  expect_kv "${closure_report}" data_tree_read_only true
  expect_kv "${closure_report}" cache_freshness_after_read_only pass
  expect_kv "${closure_report}" byte_hash_verification_after_read_only pass
  expected_binding_sha="$(kv_value "${closure_report}" binding_receipt_sha256)"
  actual_binding_sha="$(sha256_of "${binding_receipt}")"
  [[ "${actual_binding_sha}" == "${expected_binding_sha}" ]] ||
    fail "binding receipt hash does not match the completed closure"
  binding_lines="$(wc -l <"${binding_receipt}")"
  cmp -s -- <(head -n "${binding_lines}" "${closure_report}") \
    "${binding_receipt}" ||
    fail "completed closure does not preserve the no-clobber binding payload"
  [[ "$(stat -c '%A' -- "${closure_report}")" != *w* ]] ||
    fail "completed closure report is writable: ${closure_report}"
  [[ "$(stat -c '%A' -- "${binding_receipt}")" != *w* ]] ||
    fail "binding receipt is writable: ${binding_receipt}"
  verify_receipt_entries "${closure_report}"
  validate_exact_data_inventory
  assert_data_tree_read_only
  compile_validation_tools
  run_dataset_validator
  run_cache_freshness_guard
  validate_dry_run_artifacts
  echo "verified completed synthetic v2 fresh-seed closure: ${closure_report}"
}

if [[ "${mode}" == "--verify" ]]; then
  [[ -e "${closure_report}" ]] ||
    fail "completed closure does not exist: ${closure_report}"
  verify_completed_closure
  exit 0
fi

[[ ! -e "${closure_report}" ]] ||
  fail "closure is already complete; --prepare is disabled after closure (use --verify)"

require_executable "${generator_runner}"
require_regular_file "${generator_source}"
require_regular_file "${validator_source}"
require_regular_file "${cache_guard_source}"
require_regular_file "${cache_guard_header}"
require_regular_file "${sha256_header}"
require_executable "${runtime_exec}"

mkdir -p "${config_dir}" "${job_parent}" "${logs_dir}"
exec 9>"${runtime_root}/.preparation.lock"
flock -n 9 || fail "another process holds ${runtime_root}/.preparation.lock"

data_present=false
manifest_present=false
[[ -e "${data_root}" ]] && data_present=true
[[ -e "${dataset_manifest}" ]] && manifest_present=true
if [[ "${data_present}" != "${manifest_present}" ]]; then
  fail "partial generated state: data_present=${data_present}, manifest_present=${manifest_present}"
fi
if [[ "${data_present}" == false ]]; then
  SYNTHETIC_V2_BENCHMARK_ROOT="${benchmark_root}" "${generator_runner}"
fi
[[ -d "${data_root}" && -f "${dataset_manifest}" ]] ||
  fail "generator did not publish a complete data-plus-manifest state"

compile_validation_tools
run_dataset_validator
write_config_snapshot
validate_config_snapshot
run_or_resume_dry_run
run_dataset_validator
run_cache_freshness_guard

collect_receipt_entries
receipt_candidate="$(mktemp "${runtime_root}/.closure-bindings.XXXXXX")"
emit_binding_receipt "${receipt_candidate}"
if [[ -e "${binding_receipt}" ]]; then
  require_regular_file "${binding_receipt}"
  if ! cmp -s -- "${receipt_candidate}" "${binding_receipt}"; then
    rm -f -- "${receipt_candidate}"
    fail "pre-seal no-clobber binding receipt drifted: ${binding_receipt}"
  fi
  rm -f -- "${receipt_candidate}"
else
  chmod 0444 "${receipt_candidate}"
  ln "${receipt_candidate}" "${binding_receipt}" || {
    rm -f -- "${receipt_candidate}"
    fail "no-clobber binding receipt publication failed"
  }
  rm -f -- "${receipt_candidate}"
fi

# Sealing changes permissions only. File contents and mtimes are bound above,
# then rechecked below before the completed report becomes visible.
find "${data_root}" -type f -exec chmod a-w -- {} +
find "${data_root}" -depth -type d -exec chmod a-w -- {} +
assert_data_tree_read_only
verify_receipt_entries "${binding_receipt}"
run_dataset_validator
run_cache_freshness_guard

mkdir -p "${artifacts_root}"
final_candidate="$(mktemp "${artifacts_root}/.${closure_report_name}.XXXXXX")"
cat "${binding_receipt}" >"${final_candidate}"
{
  echo "binding_receipt_sha256=$(sha256_of "${binding_receipt}")"
  echo "data_tree_read_only=true"
  echo "cache_freshness_after_read_only=pass"
  echo "byte_hash_verification_after_read_only=pass"
  echo "closure_complete=true"
  echo "sealed_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >>"${final_candidate}"
chmod 0444 "${final_candidate}"
ln "${final_candidate}" "${closure_report}" || {
  rm -f -- "${final_candidate}"
  fail "atomic no-clobber completed-closure publication failed"
}
rm -f -- "${final_candidate}"
sync -f "${closure_report}"

verify_completed_closure
echo "sealed synthetic v2 fresh seed without scoring or a final ledger"
