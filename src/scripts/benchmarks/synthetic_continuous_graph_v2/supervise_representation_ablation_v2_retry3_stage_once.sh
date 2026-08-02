#!/usr/bin/env bash
set -euo pipefail
shopt -s inherit_errexit
umask 077

# Operational one-shot launcher: it is deliberately outside the scientific
# runtime, reserves exactly one expected stage, and never retries it.
readonly supervisor_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry3.one_stage_supervisor.v1"
readonly scientific_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry3"
readonly runner_basename="run_representation_ablation_v2_retry3.sh"
readonly supervisor_basename="supervise_representation_ablation_v2_retry3_stage_once.sh"
readonly unsealed_runner_sha256="0000000000000000000000000000000000000000000000000000000000000000"

# Fail closed until the final immutable Retry3 runner is audited and pinned.
readonly expected_retry3_runner_sha256="1935b772dd8526c882c6bb42717d2d7bb7ea8d642cd619e286bca6242b796395"

readonly -a stage_names=(
  initialize_from_retry2_prefix
  canonical_import_from_retry2
  endpoint_import_from_retry2
  time_only_import_from_retry2
  no_tf_alignment_training_restart
  endpoint_scale_capture
  time_only_capture
  no_tf_alignment_capture
  endpoint_scale_affine
  time_only_affine
  no_tf_alignment_affine
  selection_and_development
)
readonly stage_count="${#stage_names[@]}"

fail() {
  printf 'retry3 one-stage supervisor: %s\n' "$*" >&2
  exit 1
}

usage() {
  cat <<'USAGE'
Usage:
  supervise_representation_ablation_v2_retry3_stage_once.sh --expected-stage NN

NN must be the exact two-digit next stage index (00 through 11). A successful
call reserves that stage permanently, starts one detached worker, and returns.
Inspect the printed operational directory for launch, log, and exit evidence.
USAGE
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

safe_sha256_of() {
  local output digest
  output="$(sha256sum -- "$1" 2>/dev/null)" || return 1
  digest="${output%% *}"
  [[ "${digest}" =~ ^[0-9a-f]{64}$ ]] || return 1
  printf '%s' "${digest}"
}

require_hash() {
  local value="$1" label="$2"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "${label} is not a lowercase SHA-256"
  [[ "${value}" != "${unsealed_runner_sha256}" ]] ||
    fail "${label} is still the fail-closed unsealed sentinel"
}

require_absent() {
  [[ ! -e "$1" && ! -L "$1" ]] || fail "$2 already exists: $1"
}

require_owned_file() {
  local path="$1" mode="$2" label="$3"
  [[ -f "${path}" && ! -L "${path}" ]] ||
    fail "${label} is not a regular non-symlink file: ${path}"
  [[ "$(stat -c '%a' -- "${path}")" == "${mode}" ]] ||
    fail "${label} mode is not exactly 0${mode}: ${path}"
  [[ "$(stat -c '%u' -- "${path}")" == "$(id -u)" ]] ||
    fail "${label} is not owned by the executing uid: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
}

require_owned_directory() {
  local path="$1" mode="$2" label="$3"
  [[ -d "${path}" && ! -L "${path}" ]] ||
    fail "${label} is not a non-symlink directory: ${path}"
  [[ "$(stat -c '%a' -- "${path}")" == "${mode}" ]] ||
    fail "${label} mode is not exactly 0${mode}: ${path}"
  [[ "$(stat -c '%u' -- "${path}")" == "$(id -u)" ]] ||
    fail "${label} is not owned by the executing uid: ${path}"
}

reject_symlink_components() {
  local path="$1" current="/" rest component
  [[ "${path}" == /* ]] || fail "path is not absolute: ${path}"
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

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runner_path="${script_dir}/${runner_basename}"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${runtime_parent}/${scientific_schema_id}"
operational_parent="${repo_root}/.runtime/operator_invocations"
operational_root="${operational_parent}/${scientific_schema_id}"
supervisor_lock="${operational_root}/one_stage_supervisor.lock"
process_start_supervisor_sha256="$(sha256_of "${script_path}")"
readonly script_path script_dir repo_root runner_path runtime_parent runtime_root
readonly operational_parent operational_root supervisor_lock
readonly process_start_supervisor_sha256

assert_supervisor_identity() {
  [[ "${script_path}" == "${script_dir}/${supervisor_basename}" ]] ||
    fail "supervisor is not running from its fixed operational path"
  require_owned_file "${script_path}" 555 "operational supervisor"
  [[ "$(sha256_of "${script_path}")" == "${process_start_supervisor_sha256}" ]] ||
    fail "operational supervisor changed after process start"
}

assert_runner_identity() {
  require_hash "${expected_retry3_runner_sha256}" \
    "expected Retry3 operational runner SHA-256"
  require_owned_file "${runner_path}" 555 "Retry3 operational runner"
  [[ "$(sha256_of "${runner_path}")" == "${expected_retry3_runner_sha256}" ]] ||
    fail "Retry3 operational runner does not match its fixed supervisor pin"
}

stage_tag() {
  printf '%02d' "$1"
}

stage_attempt_path() {
  local index="$1" tag
  tag="$(stage_tag "${index}")"
  printf '%s/stage.%s.%s.attempt.status' \
    "${runtime_root}" "${tag}" "${stage_names[${index}]}"
}

stage_completion_path() {
  local index="$1" tag
  tag="$(stage_tag "${index}")"
  printf '%s/stage.%s.%s.status' \
    "${runtime_root}" "${tag}" "${stage_names[${index}]}"
}

require_stage_marker() {
  require_owned_file "$1" 444 "$2"
}

assert_no_marker() {
  [[ ! -e "$1" && ! -L "$1" ]] || fail "unexpected $2: $1"
}

observed_next_stage=-1
inspect_next_stage() {
  local index later attempt completion
  observed_next_stage=0
  if [[ ! -e "${runtime_root}" && ! -L "${runtime_root}" ]]; then
    return 0
  fi
  [[ -d "${runtime_root}" && ! -L "${runtime_root}" ]] ||
    fail "Retry3 scientific runtime is not a non-symlink directory"
  reject_symlink_components "${runtime_root}"

  for ((index = 0; index < stage_count; index++)); do
    attempt="$(stage_attempt_path "${index}")"
    completion="$(stage_completion_path "${index}")"
    if [[ -e "${completion}" || -L "${completion}" ]]; then
      require_stage_marker "${completion}" "stage completion"
      require_stage_marker "${attempt}" "stage attempt paired with completion"
      observed_next_stage="$((index + 1))"
      continue
    fi
    if [[ -e "${attempt}" || -L "${attempt}" ]]; then
      require_stage_marker "${attempt}" "terminal stage attempt"
      fail "stage $(stage_tag "${index}") has an attempt without completion; automatic retry is forbidden"
    fi
    observed_next_stage="${index}"
    for ((later = index + 1; later < stage_count; later++)); do
      assert_no_marker "$(stage_attempt_path "${later}")" "later stage attempt"
      assert_no_marker "$(stage_completion_path "${later}")" "later stage completion"
    done
    return 0
  done
}

normalize_expected_stage() {
  [[ "$1" =~ ^(0[0-9]|1[01])$ ]] ||
    fail "expected stage must be exactly 00 through 11"
  printf '%s' "$1"
}

assert_expected_next_stage() {
  local expected_tag="$1" expected_index
  expected_index="$((10#${expected_tag}))"
  inspect_next_stage
  ((observed_next_stage < stage_count)) ||
    fail "all Retry3 development stages are already complete"
  [[ "${observed_next_stage}" == "${expected_index}" ]] ||
    fail "expected stage ${expected_tag}, but the durable next stage is $(stage_tag "${observed_next_stage}")"
}

ensure_operational_root() {
  reject_symlink_components "${repo_root}/.runtime"
  if [[ ! -e "${operational_parent}" && ! -L "${operational_parent}" ]]; then
    mkdir -- "${operational_parent}"
    chmod 0700 -- "${operational_parent}"
  fi
  require_owned_directory "${operational_parent}" 700 \
    "operational invocation parent"
  if [[ ! -e "${operational_root}" && ! -L "${operational_root}" ]]; then
    mkdir -- "${operational_root}"
    chmod 0700 -- "${operational_root}"
  fi
  require_owned_directory "${operational_root}" 700 \
    "Retry3 operational invocation root"
  reject_symlink_components "${operational_root}"
}

acquire_supervisor_lock() {
  if [[ ! -e "${supervisor_lock}" && ! -L "${supervisor_lock}" ]]; then
    : >"${supervisor_lock}"
    chmod 0600 -- "${supervisor_lock}"
  fi
  require_owned_file "${supervisor_lock}" 600 "one-stage supervisor lock"
  exec 9>>"${supervisor_lock}"
  flock -n 9 || fail "another Retry3 one-stage supervisor is active"
}

assert_inherited_supervisor_lock() {
  local fd_target lock_target
  [[ "${RETRY3_STAGE_ONCE_WORKER:-}" == authorized ]] ||
    fail "worker mode lacks private launcher authority"
  [[ -e "/proc/$$/fd/9" ]] ||
    fail "detached worker did not inherit the supervisor lock"
  fd_target="$(realpath -e -- "/proc/$$/fd/9")"
  lock_target="$(realpath -e -- "${supervisor_lock}")"
  [[ "${fd_target}" == "${lock_target}" ]] ||
    fail "detached worker inherited the wrong lock"
  flock -n 9 ||
    fail "detached worker does not hold the inherited supervisor lock"
}

last_candidate_dev=""
last_candidate_inode=""
active_candidate_path=""
active_candidate_dev=""
active_candidate_inode=""
active_candidate_preopen_absent=false
last_published_receipt_dev=""
last_published_receipt_inode=""
last_published_receipt_sha256=""
last_publication_error=""

cleanup_exact_candidate() {
  local candidate="$1" expected_dev="$2" expected_inode="$3"
  local metadata dev inode mode uid links executing_uid
  [[ -n "${expected_dev}" && -n "${expected_inode}" ]] || return 1
  [[ -f "${candidate}" && ! -L "${candidate}" ]] || return 1
  metadata="$(stat -c '%d %i %a %u %h' -- "${candidate}" \
    2>/dev/null)" || return 1
  read -r dev inode mode uid links <<<"${metadata}"
  executing_uid="$(id -u 2>/dev/null)" || return 1
  [[ "${dev}" == "${expected_dev}" && \
        "${inode}" == "${expected_inode}" && \
        "${uid}" == "${executing_uid}" && "${links}" == 1 ]] ||
    return 1
  rm -- "${candidate}" || return 1
  [[ ! -e "${candidate}" && ! -L "${candidate}" ]]
}

clear_active_candidate() {
  active_candidate_path=""
  active_candidate_dev=""
  active_candidate_inode=""
  active_candidate_preopen_absent=false
}

cleanup_active_candidate() {
  [[ -n "${active_candidate_path}" ]] || return 0
  if [[ ! -e "${active_candidate_path}" && \
        ! -L "${active_candidate_path}" ]]; then
    clear_active_candidate
    return 0
  fi
  cleanup_exact_candidate "${active_candidate_path}" \
    "${active_candidate_dev}" "${active_candidate_inode}" || return 1
  clear_active_candidate
}

create_operational_candidate_nonfatal() {
  local candidate="$1" emitter="$2" metadata fd_identity
  local dev="" inode="" mode="" uid="" links="" executing_uid=""
  local emitter_rc=0
  local noclobber_was_set=false
  shift 2
  [[ -z "${active_candidate_path}" ]] || return 1
  last_candidate_dev=""
  last_candidate_inode=""
  if [[ -e "${candidate}" || -L "${candidate}" ]]; then
    return 1
  fi
  [[ "$-" == *C* ]] && noclobber_was_set=true
  set -C
  if ! exec 7>"${candidate}"; then
    [[ "${noclobber_was_set}" == true ]] || set +C
    return 1
  fi
  [[ "${noclobber_was_set}" == true ]] || set +C
  # Noclobber succeeded after the explicit absence check, so this path is a
  # breadcrumb for the inode opened on FD7 even if an identity syscall fails.
  active_candidate_path="${candidate}"
  active_candidate_dev=""
  active_candidate_inode=""
  active_candidate_preopen_absent=true
  fd_identity="$(stat -L -c '%d %i' -- /proc/self/fd/7 \
    2>/dev/null)" || fd_identity=""
  if [[ -n "${fd_identity}" ]]; then
    read -r last_candidate_dev last_candidate_inode <<<"${fd_identity}"
    active_candidate_dev="${last_candidate_dev}"
    active_candidate_inode="${last_candidate_inode}"
  fi
  metadata="$(stat -c '%d %i %a %u %h' -- "${candidate}" \
    2>/dev/null)" || metadata=""
  executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
  if [[ -n "${metadata}" ]]; then
    read -r dev inode mode uid links <<<"${metadata}"
  fi
  if [[ -z "${fd_identity}" && -n "${metadata}" && \
        "${active_candidate_preopen_absent}" == true && \
        "${candidate}" -ef /proc/self/fd/7 ]]; then
    last_candidate_dev="${dev}"
    last_candidate_inode="${inode}"
    active_candidate_dev="${dev}"
    active_candidate_inode="${inode}"
    fd_identity="${dev} ${inode}"
  fi
  if [[ -z "${fd_identity}" || -z "${metadata}" || \
        "${dev} ${inode}" != "${last_candidate_dev} ${last_candidate_inode}" || \
        "${mode}" != 600 || \
        "${uid}" != "${executing_uid}" || "${links}" != 1 ]]; then
    exec 7>&- || true
    cleanup_active_candidate || true
    return 1
  fi
  if "${emitter}" "$@" >&7; then
    emitter_rc=0
  else
    emitter_rc=$?
  fi
  exec 7>&- || emitter_rc=1
  if ((emitter_rc != 0)); then
    cleanup_active_candidate || true
    return 1
  fi
  metadata="$(stat -c '%d %i %a %u %h' -- "${candidate}" \
    2>/dev/null)" || metadata=""
  if [[ "${metadata}" != \
        "${last_candidate_dev} ${last_candidate_inode} 600 ${executing_uid} 1" || \
        ! -f "${candidate}" || -L "${candidate}" ]]; then
    cleanup_active_candidate || true
    return 1
  fi
  return 0
}

create_operational_candidate() {
  local candidate="$1" label="$2" emitter="$3"
  shift 3
  create_operational_candidate_nonfatal "${candidate}" "${emitter}" "$@" ||
    fail "could not create ${label}: ${candidate}"
}

record_expected_receipt_publication() {
  local destination="$1" dev="$2" inode="$3" digest="$4"
  if [[ -n "${worker_started_receipt:-}" && \
        "${destination}" == "${worker_started_receipt}" ]]; then
    worker_started_receipt_expected_dev="${dev}"
    worker_started_receipt_expected_inode="${inode}"
    worker_started_receipt_expected_sha256="${digest}"
  fi
  if [[ -n "${worker_exit_receipt:-}" && \
        "${destination}" == "${worker_exit_receipt}" ]]; then
    worker_exit_receipt_expected_dev="${dev}"
    worker_exit_receipt_expected_inode="${inode}"
    worker_exit_receipt_expected_sha256="${digest}"
  fi
}

record_completed_receipt_publication() {
  local destination="$1" dev="$2" inode="$3" digest="$4"
  if [[ -n "${worker_started_receipt:-}" && \
        "${destination}" == "${worker_started_receipt}" ]]; then
    worker_started_receipt_published=true
  fi
  if [[ -n "${worker_exit_receipt:-}" && \
        "${destination}" == "${worker_exit_receipt}" ]]; then
    worker_exit_receipt_published=true
  fi
  last_published_receipt_dev="${dev}"
  last_published_receipt_inode="${inode}"
  last_published_receipt_sha256="${digest}"
}

publish_operational_receipt_nonfatal() {
  local candidate="$1" destination="$2" candidate_parent destination_parent
  local parent_before parent_after candidate_before candidate_frozen
  local destination_after digest_before digest_after executing_uid
  local candidate_dev candidate_inode candidate_mode candidate_uid candidate_links
  local parent_dev parent_inode parent_mode parent_uid
  last_publication_error=""
  last_published_receipt_dev=""
  last_published_receipt_inode=""
  last_published_receipt_sha256=""
  candidate_parent="${candidate%/*}"
  destination_parent="${destination%/*}"
  if [[ -z "${candidate_parent}" || \
        "${candidate_parent}" != "${destination_parent}" ]]; then
    last_publication_error="candidate and destination are not in the same directory"
    cleanup_exact_candidate "${candidate}" "${last_candidate_dev}" \
      "${last_candidate_inode}" || true
    return 1
  fi
  if [[ ! -d "${candidate_parent}" || -L "${candidate_parent}" ]]; then
    last_publication_error="receipt parent is not a non-symlink directory"
    cleanup_exact_candidate "${candidate}" "${last_candidate_dev}" \
      "${last_candidate_inode}" || true
    return 1
  fi
  parent_before="$(stat -c '%d %i %a %u' -- "${candidate_parent}" \
    2>/dev/null)" || parent_before=""
  candidate_before="$(stat -c '%d %i %a %u %h' -- "${candidate}" \
    2>/dev/null)" || candidate_before=""
  executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
  if [[ -z "${candidate_before}" ]]; then
    last_publication_error="receipt candidate identity is unavailable"
    cleanup_exact_candidate "${candidate}" "${last_candidate_dev}" \
      "${last_candidate_inode}" || true
    return 1
  fi
  read -r candidate_dev candidate_inode candidate_mode candidate_uid \
    candidate_links <<<"${candidate_before}"
  if [[ -n "${parent_before}" ]]; then
    read -r parent_dev parent_inode parent_mode parent_uid \
      <<<"${parent_before}"
  fi
  if [[ "${active_candidate_path}" != "${candidate}" || \
        "${active_candidate_dev}" != "${candidate_dev}" || \
        "${active_candidate_inode}" != "${candidate_inode}" ]]; then
    last_publication_error="receipt candidate no longer matches the exact created inode"
    cleanup_active_candidate || true
    return 1
  fi
  if [[ ! -f "${candidate}" || -L "${candidate}" || \
        "${candidate_mode}" != 600 || \
        "${candidate_uid}" != "${executing_uid}" || \
        "${candidate_links}" != 1 ]]; then
    last_publication_error="receipt candidate metadata is invalid"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  if [[ -z "${parent_before}" || "${parent_mode}" != 700 || \
        "${parent_uid}" != "${executing_uid}" ]]; then
    last_publication_error="receipt parent metadata is invalid"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  if [[ -e "${destination}" || -L "${destination}" ]]; then
    last_publication_error="receipt destination already exists"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  if ! chmod 0444 -- "${candidate}"; then
    last_publication_error="could not freeze receipt candidate"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  candidate_frozen="$(stat -c '%d %i %a %u %h' -- "${candidate}" \
    2>/dev/null)" || candidate_frozen=""
  if [[ "${candidate_frozen}" != \
        "${candidate_dev} ${candidate_inode} 444 ${executing_uid} 1" || \
        ! -f "${candidate}" || -L "${candidate}" ]]; then
    last_publication_error="frozen receipt candidate identity changed"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  if ! digest_before="$(safe_sha256_of "${candidate}")"; then
    last_publication_error="could not digest frozen receipt candidate"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  record_expected_receipt_publication "${destination}" "${candidate_dev}" \
    "${candidate_inode}" "${digest_before}"
  parent_after="$(stat -c '%d %i %a %u' -- "${candidate_parent}" \
    2>/dev/null)" || parent_after=""
  if [[ "${parent_after}" != "${parent_before}" || \
        -e "${destination}" || -L "${destination}" ]]; then
    last_publication_error="receipt parent changed or destination appeared"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  if ! mv -T --no-clobber -- "${candidate}" "${destination}"; then
    last_publication_error="atomic no-clobber receipt rename failed"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  parent_after="$(stat -c '%d %i %a %u' -- "${destination_parent}" \
    2>/dev/null)" || parent_after=""
  destination_after="$(stat -c '%d %i %a %u %h' -- "${destination}" \
    2>/dev/null)" || destination_after=""
  digest_after="$(safe_sha256_of "${destination}" 2>/dev/null)" ||
    digest_after=""
  if [[ "${parent_after}" != "${parent_before}" || \
        -e "${candidate}" || -L "${candidate}" || \
        "${destination_after}" != \
          "${candidate_dev} ${candidate_inode} 444 ${executing_uid} 1" || \
        "${digest_after}" != "${digest_before}" || \
        ! -f "${destination}" || -L "${destination}" ]]; then
    last_publication_error="published receipt failed identity verification"
    cleanup_exact_candidate "${candidate}" "${candidate_dev}" \
      "${candidate_inode}" || true
    return 1
  fi
  record_completed_receipt_publication "${destination}" "${candidate_dev}" \
    "${candidate_inode}" "${digest_after}"
  if [[ "${active_candidate_path}" == "${candidate}" && \
        "${active_candidate_dev}" == "${candidate_dev}" && \
        "${active_candidate_inode}" == "${candidate_inode}" ]]; then
    clear_active_candidate
  fi
  return 0
}

publish_operational_receipt() {
  local candidate="$1" destination="$2"
  publish_operational_receipt_nonfatal "${candidate}" "${destination}" ||
    fail "could not publish operational receipt (${last_publication_error}): ${destination}"
}

kv() {
  local key="$1" path="$2" count value
  count="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      if (substr($0, 1, eq - 1) == key) count += 1;
    }
    END { print count + 0 }
  ' "${path}")"
  [[ "${count}" == 1 ]] ||
    fail "${path}: expected exactly one ${key}= field"
  value="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      if (substr($0, 1, eq - 1) == key) print substr($0, eq + 1);
    }
  ' "${path}")"
  printf '%s' "${value}"
}

expect_kv() {
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv "${key}" "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

emit_launch_receipt() {
  local expected_tag="$1" invocation_dir="$2" expected_index
  expected_index="$((10#${expected_tag}))"
  cat <<EOF
schema_id=${supervisor_schema_id}.launch.v1
status=detached_worker_pending
scientific_schema_id=${scientific_schema_id}
scientific_runtime_root=${runtime_root}
expected_stage_index=${expected_tag}
expected_stage_name=${stage_names[${expected_index}]}
runner_path=${runner_path}
runner_sha256=${expected_retry3_runner_sha256}
runner_argument=--advance-development
maximum_runner_invocations=1
supervisor_path=${script_path}
supervisor_process_start_sha256=${process_start_supervisor_sha256}
operational_invocation_root=${invocation_dir}
detached=true
launcher_pid=$$
EOF
}

verify_launch_receipt() {
  local receipt="$1" expected_tag="$2" invocation_dir="$3" expected_sha="$4"
  local expected_index
  expected_index="$((10#${expected_tag}))"
  require_owned_file "${receipt}" 444 "supervisor launch receipt"
  [[ "$(sha256_of "${receipt}")" == "${expected_sha}" ]] ||
    fail "supervisor launch receipt does not match worker authority"
  expect_kv "${receipt}" schema_id "${supervisor_schema_id}.launch.v1"
  expect_kv "${receipt}" status detached_worker_pending
  expect_kv "${receipt}" scientific_schema_id "${scientific_schema_id}"
  expect_kv "${receipt}" scientific_runtime_root "${runtime_root}"
  expect_kv "${receipt}" expected_stage_index "${expected_tag}"
  expect_kv "${receipt}" expected_stage_name "${stage_names[${expected_index}]}"
  expect_kv "${receipt}" runner_path "${runner_path}"
  expect_kv "${receipt}" runner_sha256 "${expected_retry3_runner_sha256}"
  expect_kv "${receipt}" runner_argument --advance-development
  expect_kv "${receipt}" maximum_runner_invocations 1
  expect_kv "${receipt}" supervisor_path "${script_path}"
  expect_kv "${receipt}" supervisor_process_start_sha256 \
    "${process_start_supervisor_sha256}"
  expect_kv "${receipt}" operational_invocation_root "${invocation_dir}"
  expect_kv "${receipt}" detached true
}

emit_worker_started_receipt() {
  local expected_tag="$1" invocation_dir="$2" launch_receipt="$3"
  local expected_index
  expected_index="$((10#${expected_tag}))"
  cat <<EOF
schema_id=${supervisor_schema_id}.worker_started.v1
status=running
expected_stage_index=${expected_tag}
expected_stage_name=${stage_names[${expected_index}]}
launch_receipt_path=${launch_receipt}
launch_receipt_sha256=$(sha256_of "${launch_receipt}")
runner_path=${runner_path}
runner_sha256=${expected_retry3_runner_sha256}
advance_call_limit=1
worker_pid=$$
operational_invocation_root=${invocation_dir}
EOF
}

post_attempt_present=false
post_attempt_valid=false
post_completion_present=false
post_completion_valid=false
post_attempt_sha256="${unsealed_runner_sha256}"
post_completion_sha256="${unsealed_runner_sha256}"
inspect_post_run_stage() {
  local expected_tag="$1" expected_index index later attempt completion
  post_attempt_present=false
  post_attempt_valid=false
  post_completion_present=false
  post_completion_valid=false
  post_attempt_sha256="${unsealed_runner_sha256}"
  post_completion_sha256="${unsealed_runner_sha256}"
  expected_index="$((10#${expected_tag}))"
  for ((index = 0; index < expected_index; index++)); do
    require_stage_marker "$(stage_attempt_path "${index}")" \
      "completed-prefix stage attempt"
    require_stage_marker "$(stage_completion_path "${index}")" \
      "completed-prefix stage completion"
  done
  attempt="$(stage_attempt_path "${expected_index}")"
  completion="$(stage_completion_path "${expected_index}")"
  if [[ -e "${attempt}" || -L "${attempt}" ]]; then
    require_stage_marker "${attempt}" "expected stage attempt"
    post_attempt_present=true
    post_attempt_valid=true
    post_attempt_sha256="$(sha256_of "${attempt}")"
  fi
  if [[ -e "${completion}" || -L "${completion}" ]]; then
    require_stage_marker "${completion}" "expected stage completion"
    [[ "${post_attempt_present}" == true ]] ||
      fail "expected completion exists without its attempt receipt"
    post_completion_present=true
    post_completion_valid=true
    post_completion_sha256="$(sha256_of "${completion}")"
  fi
  for ((later = expected_index + 1; later < stage_count; later++)); do
    assert_no_marker "$(stage_attempt_path "${later}")" \
      "later stage attempt after one-shot advance"
    assert_no_marker "$(stage_completion_path "${later}")" \
      "later stage completion after one-shot advance"
  done
}

# Once worker.started.status has been published, the EXIT guard is the terminal
# evidence backstop for every set -e path. It never replaces an exit receipt.
worker_exit_guard_armed=false
worker_started_receipt_published=false
worker_exit_receipt_published=false
worker_started_receipt_expected_dev=""
worker_started_receipt_expected_inode=""
worker_started_receipt_expected_sha256=""
worker_exit_receipt_expected_dev=""
worker_exit_receipt_expected_inode=""
worker_exit_receipt_expected_sha256=""
worker_expected_tag=""
worker_expected_index=-1
worker_invocation_dir=""
worker_launch_receipt=""
worker_launch_sha256="${unsealed_runner_sha256}"
worker_started_receipt=""
worker_exit_receipt=""
worker_runner_log=""
worker_bootstrap_log=""
worker_expected_attempt_path=""
worker_expected_completion_path=""
worker_runner_call_count=0
worker_runner_call_reserved=false
worker_runner_call_returned=false
worker_runner_invocation_state=not_reserved
worker_runner_return_captured=false
worker_runner_rc=not_invoked
worker_runner_observed_sha256="${unsealed_runner_sha256}"
worker_runner_identity_state=not_checked
worker_runner_log_present=false
worker_runner_log_valid=false
worker_runner_log_state=not_created
worker_runner_log_sha256="${unsealed_runner_sha256}"
worker_runner_log_reference_scan_state=not_run
worker_runner_log_reference_count=0
worker_terminal_context=not_armed

snapshot_expected_marker_relaxed() {
  local kind="$1" path="$2" present=false valid=false
  local digest="${unsealed_runner_sha256}" metadata_before metadata_after
  local dev inode size mtime ctime mode uid links executing_uid
  if [[ -e "${path}" || -L "${path}" ]]; then
    present=true
  fi
  if [[ -f "${path}" && ! -L "${path}" ]]; then
    metadata_before="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
      "${path}" 2>/dev/null)" || metadata_before=""
    if digest="$(safe_sha256_of "${path}")"; then
      metadata_after="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
        "${path}" 2>/dev/null)" || metadata_after=""
      executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
      if [[ -n "${metadata_before}" && \
            "${metadata_before}" == "${metadata_after}" ]]; then
        read -r dev inode size mtime ctime mode uid links \
          <<<"${metadata_after}"
        if [[ "${mode}" == 444 && "${uid}" == "${executing_uid}" && \
              "${links}" == 1 ]]; then
          valid=true
        fi
      fi
    else
      digest="${unsealed_runner_sha256}"
    fi
  fi
  case "${kind}" in
  attempt)
    post_attempt_present="${present}"
    post_attempt_valid="${valid}"
    post_attempt_sha256="${digest}"
    ;;
  completion)
    post_completion_present="${present}"
    post_completion_valid="${valid}"
    post_completion_sha256="${digest}"
    ;;
  *)
    return 1
    ;;
  esac
}

snapshot_expected_stage_relaxed() {
  snapshot_expected_marker_relaxed attempt \
    "${worker_expected_attempt_path}" || true
  snapshot_expected_marker_relaxed completion \
    "${worker_expected_completion_path}" || true
}

snapshot_runner_identity_relaxed() {
  local digest="${unsealed_runner_sha256}" metadata_before metadata_after
  local dev inode size mtime ctime mode uid links executing_uid
  worker_runner_observed_sha256="${unsealed_runner_sha256}"
  worker_runner_identity_state=missing
  if [[ -L "${runner_path}" ]]; then
    worker_runner_identity_state=symlink
    return 0
  fi
  if [[ ! -f "${runner_path}" ]]; then
    [[ -e "${runner_path}" ]] && worker_runner_identity_state=not_regular
    return 0
  fi
  metadata_before="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
    "${runner_path}" 2>/dev/null)" || metadata_before=""
  if ! digest="$(safe_sha256_of "${runner_path}")"; then
    worker_runner_identity_state=hash_unavailable
    return 0
  fi
  worker_runner_observed_sha256="${digest}"
  metadata_after="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
    "${runner_path}" 2>/dev/null)" || metadata_after=""
  if [[ -z "${metadata_before}" || \
        "${metadata_before}" != "${metadata_after}" ]]; then
    worker_runner_identity_state=changed_during_snapshot
    return 0
  fi
  read -r dev inode size mtime ctime mode uid links <<<"${metadata_after}"
  executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
  if [[ "${mode}" != 555 || "${uid}" != "${executing_uid}" || \
        "${links}" != 1 ]]; then
    worker_runner_identity_state=metadata_mismatch
  elif [[ "${digest}" != "${expected_retry3_runner_sha256}" ]]; then
    worker_runner_identity_state=sha256_mismatch
  else
    worker_runner_identity_state=verified
  fi
}

create_proc_scan_temp_relaxed() {
  local __path_var="$1" __dev_var="$2" __inode_var="$3"
  local temp metadata executing_uid
  local dev="" inode="" mode="" uid="" links=""
  temp="$(mktemp "/tmp/${supervisor_schema_id}.proc_scan.XXXXXX")" ||
    return 1
  printf -v "${__path_var}" '%s' "${temp}"
  executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
  metadata="$(stat -c '%d %i %a %u %h' -- "${temp}" 2>/dev/null)" ||
    metadata=""
  if [[ -n "${metadata}" ]]; then
    read -r dev inode mode uid links <<<"${metadata}"
  fi
  if [[ ! -f "${temp}" || -L "${temp}" || \
        "${mode}" != 600 || "${uid}" != "${executing_uid}" || \
        "${links}" != 1 ]]; then
    if [[ -f "${temp}" && ! -L "${temp}" && \
          "${uid}" == "${executing_uid}" && "${links}" == 1 ]]; then
      rm -- "${temp}" 2>/dev/null || true
    fi
    return 1
  fi
  printf -v "${__dev_var}" '%s' "${dev}"
  printf -v "${__inode_var}" '%s' "${inode}"
}

verify_proc_scan_temp_relaxed() {
  local temp="$1" expected_dev="$2" expected_inode="$3"
  local metadata executing_uid
  [[ -f "${temp}" && ! -L "${temp}" ]] || return 1
  executing_uid="$(id -u 2>/dev/null)" || return 1
  metadata="$(stat -c '%d %i %a %u %h' -- "${temp}" 2>/dev/null)" ||
    return 1
  [[ "${metadata}" == \
    "${expected_dev} ${expected_inode} 600 ${executing_uid} 1" ]]
}

remove_proc_scan_temp_relaxed() {
  local temp="$1" expected_dev="$2" expected_inode="$3"
  verify_proc_scan_temp_relaxed "${temp}" "${expected_dev}" \
    "${expected_inode}" || return 1
  rm -- "${temp}" || return 1
  [[ ! -e "${temp}" && ! -L "${temp}" ]]
}

scan_visible_proc_fd_references_relaxed() {
  local target="$1" target_before target_after target_dev target_inode rest
  local process_dir fd_dir fd_entry fd_identity producer_status
  local process_snapshot="" fd_snapshot="" self_pid_seen=false
  local process_snapshot_dev="" process_snapshot_inode=""
  local fd_snapshot_dev="" fd_snapshot_inode=""
  local self_fd_snapshot_seen=false process_snapshot_produced=false
  local scan_failure_state="" cleanup_failed=false
  worker_runner_log_reference_scan_state=target_unavailable
  worker_runner_log_reference_count=0

  # Every producer below may fork. Prove first that none can inherit the
  # supervisor's writable runner-log descriptor.
  if [[ -e /proc/self/fd/8 || -L /proc/self/fd/8 ]]; then
    worker_runner_log_reference_scan_state=supervisor_fd8_still_open
    return 0
  fi
  target_before="$(stat -c '%d %i %s %Y %Z' -- "${target}" \
    2>/dev/null)" || return 0
  read -r target_dev target_inode rest <<<"${target_before}"

  if ! create_proc_scan_temp_relaxed process_snapshot \
    process_snapshot_dev process_snapshot_inode; then
    if [[ -n "${process_snapshot}" && -n "${process_snapshot_dev}" ]]; then
      remove_proc_scan_temp_relaxed "${process_snapshot}" \
        "${process_snapshot_dev}" "${process_snapshot_inode}" || true
    fi
    worker_runner_log_reference_scan_state=process_snapshot_temp_failed
    return 0
  fi
  if ! create_proc_scan_temp_relaxed fd_snapshot fd_snapshot_dev \
    fd_snapshot_inode; then
    remove_proc_scan_temp_relaxed "${process_snapshot}" \
      "${process_snapshot_dev}" "${process_snapshot_inode}" || true
    worker_runner_log_reference_scan_state=fd_snapshot_temp_failed
    return 0
  fi

  if ! verify_proc_scan_temp_relaxed "${process_snapshot}" \
    "${process_snapshot_dev}" "${process_snapshot_inode}"; then
    producer_status=125
  elif find /proc -mindepth 1 -maxdepth 1 -name '[0-9]*' -print0 \
    >"${process_snapshot}" 2>/dev/null; then
    producer_status=0
    process_snapshot_produced=true
  else
    producer_status=$?
  fi
  if ! verify_proc_scan_temp_relaxed "${process_snapshot}" \
    "${process_snapshot_dev}" "${process_snapshot_inode}"; then
    scan_failure_state=process_snapshot_identity_changed
  elif ((producer_status != 0)); then
    scan_failure_state="process_enumeration_failed_status_${producer_status}"
  else
    while IFS= read -r -d '' process_dir; do
      if [[ "${process_dir}" == "/proc/$$" ]]; then
        self_pid_seen=true
      fi
      if [[ ! "${process_dir}" =~ ^/proc/[0-9]+$ ]]; then
        scan_failure_state=invalid_process_snapshot_entry
        continue
      fi
      if [[ ! -e "${process_dir}" && ! -L "${process_dir}" ]]; then
        continue
      fi
      if [[ ! -d "${process_dir}" || -L "${process_dir}" ]]; then
        scan_failure_state=invalid_extant_process_entry
        continue
      fi
      fd_dir="${process_dir}/fd"
      if [[ ! -d "${fd_dir}" || -L "${fd_dir}" ]]; then
        if [[ ! -e "${process_dir}" && ! -L "${process_dir}" ]]; then
          continue
        fi
        scan_failure_state=invalid_extant_fd_directory
        continue
      fi
      if ! verify_proc_scan_temp_relaxed "${fd_snapshot}" \
        "${fd_snapshot_dev}" "${fd_snapshot_inode}"; then
        scan_failure_state=fd_snapshot_identity_changed
        break
      fi
      if ! : >"${fd_snapshot}"; then
        scan_failure_state=fd_snapshot_reset_failed
        break
      fi
      if find "${fd_dir}" -mindepth 1 -maxdepth 1 -print0 \
        >"${fd_snapshot}" 2>/dev/null; then
        producer_status=0
      else
        producer_status=$?
      fi
      if ! verify_proc_scan_temp_relaxed "${fd_snapshot}" \
        "${fd_snapshot_dev}" "${fd_snapshot_inode}"; then
        scan_failure_state=fd_snapshot_identity_changed
        break
      elif ((producer_status != 0)); then
        if [[ ! -e "${process_dir}" && ! -L "${process_dir}" ]]; then
          continue
        fi
        scan_failure_state="fd_enumeration_failed_status_${producer_status}"
        continue
      fi
      if [[ "${process_dir}" == "/proc/$$" ]]; then
        self_fd_snapshot_seen=true
      fi
      while IFS= read -r -d '' fd_entry; do
        if [[ "${fd_entry%/*}" != "${fd_dir}" ]]; then
          scan_failure_state=invalid_fd_snapshot_entry
          continue
        fi
        if fd_identity="$(stat -Lc '%d:%i' -- "${fd_entry}" \
          2>/dev/null)"; then
          if [[ "${fd_identity}" == "${target_dev}:${target_inode}" ]]; then
            worker_runner_log_reference_count="$((
              worker_runner_log_reference_count + 1
            ))"
          fi
          continue
        fi
        if [[ ! -e "${fd_entry}" && ! -L "${fd_entry}" ]]; then
          continue
        fi
        if [[ ! -e "${process_dir}" && ! -L "${process_dir}" ]]; then
          continue
        fi
        scan_failure_state=unreadable_extant_fd_reference
      done <"${fd_snapshot}"
    done <"${process_snapshot}"
  fi

  if [[ "${process_snapshot_produced}" == true && \
        "${self_pid_seen}" != true ]]; then
    scan_failure_state=self_pid_missing_from_process_snapshot
  elif [[ "${process_snapshot_produced}" == true && \
          "${self_fd_snapshot_seen}" != true ]]; then
    scan_failure_state=self_fd_snapshot_not_enumerated
  fi
  target_after="$(stat -c '%d %i %s %Y %Z' -- "${target}" \
    2>/dev/null)" || target_after=""
  remove_proc_scan_temp_relaxed "${fd_snapshot}" "${fd_snapshot_dev}" \
    "${fd_snapshot_inode}" || cleanup_failed=true
  remove_proc_scan_temp_relaxed "${process_snapshot}" \
    "${process_snapshot_dev}" "${process_snapshot_inode}" ||
    cleanup_failed=true

  if [[ "${cleanup_failed}" == true ]]; then
    worker_runner_log_reference_scan_state=proc_snapshot_cleanup_failed
  elif [[ -z "${target_after}" || \
        "${target_after}" != "${target_before}" ]]; then
    worker_runner_log_reference_scan_state=target_changed_during_scan
  elif [[ -n "${scan_failure_state}" ]]; then
    worker_runner_log_reference_scan_state="${scan_failure_state}"
  elif ((worker_runner_log_reference_count > 0)); then
    worker_runner_log_reference_scan_state=open_reference_found
  else
    worker_runner_log_reference_scan_state=clear
  fi
}

snapshot_runner_log_relaxed() {
  local digest="${unsealed_runner_sha256}" metadata_before metadata_after
  local dev inode size mtime ctime mode uid links executing_uid
  worker_runner_log_present=false
  worker_runner_log_valid=false
  worker_runner_log_state=missing
  worker_runner_log_sha256="${unsealed_runner_sha256}"
  worker_runner_log_reference_scan_state=not_run
  worker_runner_log_reference_count=0
  if [[ -e /proc/self/fd/8 || -L /proc/self/fd/8 ]]; then
    worker_runner_log_reference_scan_state=supervisor_fd8_still_open
    worker_runner_log_state=reference_scan_supervisor_fd8_still_open
    return 0
  fi
  if [[ -e "${worker_runner_log}" || -L "${worker_runner_log}" ]]; then
    worker_runner_log_present=true
  else
    return 0
  fi
  if [[ -L "${worker_runner_log}" ]]; then
    worker_runner_log_state=symlink
    return 0
  fi
  if [[ ! -f "${worker_runner_log}" ]]; then
    worker_runner_log_state=not_regular
    return 0
  fi
  chmod 0444 -- "${worker_runner_log}" 2>/dev/null || true
  metadata_before="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
    "${worker_runner_log}" 2>/dev/null)" || metadata_before=""
  executing_uid="$(id -u 2>/dev/null)" || executing_uid=""
  if [[ -z "${metadata_before}" ]]; then
    worker_runner_log_state=metadata_unavailable
    return 0
  fi
  read -r dev inode size mtime ctime mode uid links <<<"${metadata_before}"
  if [[ "${mode}" != 444 || "${uid}" != "${executing_uid}" || \
        "${links}" != 1 ]]; then
    worker_runner_log_state=metadata_mismatch
    return 0
  fi
  scan_visible_proc_fd_references_relaxed "${worker_runner_log}"
  if [[ "${worker_runner_log_reference_scan_state}" != clear ]]; then
    worker_runner_log_state="reference_scan_${worker_runner_log_reference_scan_state}"
    return 0
  fi
  if ! digest="$(safe_sha256_of "${worker_runner_log}")"; then
    worker_runner_log_state=hash_unavailable
    return 0
  fi
  metadata_after="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
    "${worker_runner_log}" 2>/dev/null)" || metadata_after=""
  if [[ -z "${metadata_after}" || \
        "${metadata_before}" != "${metadata_after}" ]]; then
    worker_runner_log_state=changed_during_digest
    return 0
  fi
  scan_visible_proc_fd_references_relaxed "${worker_runner_log}"
  if [[ "${worker_runner_log_reference_scan_state}" != clear ]]; then
    worker_runner_log_state="post_digest_reference_scan_${worker_runner_log_reference_scan_state}"
    return 0
  fi
  metadata_after="$(stat -c '%d %i %s %Y %Z %a %u %h' -- \
    "${worker_runner_log}" 2>/dev/null)" || metadata_after=""
  if [[ "${metadata_before}" != "${metadata_after}" ]]; then
    worker_runner_log_state=changed_after_digest
    return 0
  fi
  worker_runner_log_sha256="${digest}"
  worker_runner_log_valid=true
  worker_runner_log_state=sealed_no_open_references
}

validate_recorded_receipt() {
  local path="$1" expected_dev="$2" expected_inode="$3"
  local expected_digest="$4" metadata_before metadata_after digest executing_uid
  [[ -n "${expected_dev}" && -n "${expected_inode}" && \
        "${expected_digest}" =~ ^[0-9a-f]{64}$ ]] || return 1
  [[ -f "${path}" && ! -L "${path}" ]] || return 1
  executing_uid="$(id -u 2>/dev/null)" || return 1
  metadata_before="$(stat -c '%d %i %a %u %h' -- "${path}" \
    2>/dev/null)" || return 1
  [[ "${metadata_before}" == \
    "${expected_dev} ${expected_inode} 444 ${executing_uid} 1" ]] ||
    return 1
  digest="$(safe_sha256_of "${path}")" || return 1
  metadata_after="$(stat -c '%d %i %a %u %h' -- "${path}" \
    2>/dev/null)" || return 1
  [[ "${metadata_after}" == "${metadata_before}" && \
        "${digest}" == "${expected_digest}" ]]
}

validate_worker_started_receipt() {
  validate_recorded_receipt "${worker_started_receipt}" \
    "${worker_started_receipt_expected_dev}" \
    "${worker_started_receipt_expected_inode}" \
    "${worker_started_receipt_expected_sha256}" || return 1
  worker_started_receipt_published=true
}

validate_worker_exit_receipt() {
  validate_recorded_receipt "${worker_exit_receipt}" \
    "${worker_exit_receipt_expected_dev}" \
    "${worker_exit_receipt_expected_inode}" \
    "${worker_exit_receipt_expected_sha256}" || return 1
  worker_exit_receipt_published=true
}

freeze_worker_bootstrap_log_after_detach() {
  local metadata executing_uid
  exec 1>/dev/null 2>/dev/null || return 1
  [[ -f "${worker_bootstrap_log}" && ! -L "${worker_bootstrap_log}" ]] ||
    return 0
  executing_uid="$(id -u 2>/dev/null)" || return 0
  metadata="$(stat -c '%a %u %h' -- "${worker_bootstrap_log}" \
    2>/dev/null)" || return 0
  [[ "${metadata}" == "600 ${executing_uid} 1" || \
        "${metadata}" == "444 ${executing_uid} 1" ]] || return 1
  chmod 0444 -- "${worker_bootstrap_log}" 2>/dev/null || return 1
  metadata="$(stat -c '%a %u %h' -- "${worker_bootstrap_log}" \
    2>/dev/null)" || return 1
  [[ "${metadata}" == "444 ${executing_uid} 1" ]]
}

emit_exit_receipt() {
  local outcome="$1" worker_exit_code="$2" receipt_mode="$3"
  local terminal_context="$4" runner_exit_code
  if [[ "${worker_runner_call_returned}" == true && \
        "${worker_runner_return_captured}" == true ]]; then
    runner_exit_code="${worker_runner_rc}"
  elif [[ "${worker_runner_call_reserved}" == true ]]; then
    runner_exit_code=reserved_call_did_not_return
  else
    runner_exit_code=not_reserved
  fi
  cat <<EOF
schema_id=${supervisor_schema_id}.exit.v1
status=complete
outcome=${outcome}
receipt_mode=${receipt_mode}
terminal_context=${terminal_context}
worker_exit_code=${worker_exit_code}
expected_stage_index=${worker_expected_tag}
expected_stage_name=${stage_names[${worker_expected_index}]}
runner_path=${runner_path}
runner_sha256=${expected_retry3_runner_sha256}
runner_expected_sha256=${expected_retry3_runner_sha256}
runner_observed_sha256=${worker_runner_observed_sha256}
runner_identity_state=${worker_runner_identity_state}
runner_argument=--advance-development
advance_call_count=${worker_runner_call_count}
advance_call_reserved=${worker_runner_call_reserved}
advance_call_returned=${worker_runner_call_returned}
runner_invocation_state=${worker_runner_invocation_state}
runner_exit_code=${runner_exit_code}
expected_attempt_path=${worker_expected_attempt_path}
expected_attempt_present=${post_attempt_present}
expected_attempt_valid=${post_attempt_valid}
expected_attempt_sha256=${post_attempt_sha256}
expected_completion_path=${worker_expected_completion_path}
expected_completion_present=${post_completion_present}
expected_completion_valid=${post_completion_valid}
expected_completion_sha256=${post_completion_sha256}
runner_log_path=${worker_runner_log}
runner_log_present=${worker_runner_log_present}
runner_log_valid=${worker_runner_log_valid}
runner_log_state=${worker_runner_log_state}
runner_log_reference_scan_state=${worker_runner_log_reference_scan_state}
runner_log_open_reference_count=${worker_runner_log_reference_count}
runner_log_sha256=${worker_runner_log_sha256}
launch_receipt_path=${worker_launch_receipt}
launch_receipt_sha256=${worker_launch_sha256}
operational_invocation_root=${worker_invocation_dir}
EOF
}

publish_failsafe_exit_receipt() {
  local trapped_rc="$1" outcome candidate
  if [[ "${worker_runner_call_returned}" == true ]]; then
    outcome=worker_failed_after_runner_return
  elif [[ "${worker_runner_call_reserved}" == true ]]; then
    outcome=worker_failed_with_reserved_runner_call_without_return
  else
    outcome=worker_failed_before_runner_invocation
  fi
  snapshot_runner_identity_relaxed
  snapshot_expected_stage_relaxed
  snapshot_runner_log_relaxed

  candidate="${worker_invocation_dir}/.failsafe.exit.status.${BASHPID}.${RANDOM}"
  if ! create_operational_candidate_nonfatal "${candidate}" \
    emit_exit_receipt "${outcome}" "${trapped_rc}" failsafe \
    "${worker_terminal_context}"; then
    printf 'retry3 one-stage supervisor: could not create fail-safe exit receipt candidate: %s\n' \
      "${candidate}" >&2
    return 1
  fi
  if ! publish_operational_receipt_nonfatal "${candidate}" \
    "${worker_exit_receipt}"; then
    if validate_worker_exit_receipt; then
      return 0
    fi
    printf 'retry3 one-stage supervisor: could not publish fail-safe exit receipt (%s): %s\n' \
      "${last_publication_error}" "${worker_exit_receipt}" >&2
    return 1
  fi
  if validate_worker_exit_receipt; then
    return 0
  fi
  printf 'retry3 one-stage supervisor: published fail-safe exit receipt is invalid: %s\n' \
    "${worker_exit_receipt}" >&2
  return 1
}

worker_exit_guard() {
  local trapped_rc="$1" started_was_published=false
  trap - EXIT
  set +e
  set +u
  set +o pipefail
  exec 8>&- || true
  if ! cleanup_active_candidate; then
    printf 'retry3 one-stage supervisor: refused to clean an unverified hidden candidate: %s\n' \
      "${active_candidate_path}" >&2
  fi
  if [[ "${worker_exit_guard_armed}" == true ]]; then
    if [[ "${worker_started_receipt_published}" == true ]]; then
      started_was_published=true
      if ! validate_worker_started_receipt; then
        printf 'retry3 one-stage supervisor: previously validated worker-started receipt is now invalid: %s\n' \
          "${worker_started_receipt}" >&2
      fi
    elif validate_worker_started_receipt; then
      started_was_published=true
    fi
    if [[ "${started_was_published}" == true ]]; then
      if ! validate_worker_exit_receipt; then
        publish_failsafe_exit_receipt "${trapped_rc}" || true
      fi
    else
      printf 'retry3 one-stage supervisor: EXIT guard found no valid published worker-started receipt: %s\n' \
        "${worker_started_receipt}" >&2
    fi
  fi
  if ! cleanup_active_candidate; then
    printf 'retry3 one-stage supervisor: fail-safe publication left an unverified hidden candidate untouched: %s\n' \
      "${active_candidate_path}" >&2
  fi
  freeze_worker_bootstrap_log_after_detach || true
  exit "${trapped_rc}"
}

run_detached_worker() {
  local expected_tag="$1" launch_sha="$2" expected_index invocation_dir
  local launch_receipt started_receipt exit_receipt runner_log bootstrap_log
  local candidate runner_rc outcome final_rc
  expected_tag="$(normalize_expected_stage "${expected_tag}")"
  require_hash "${launch_sha}" "launch receipt SHA-256"
  expected_index="$((10#${expected_tag}))"
  invocation_dir="${operational_root}/stage.${expected_tag}.${stage_names[${expected_index}]}"
  launch_receipt="${invocation_dir}/launch.status"
  started_receipt="${invocation_dir}/worker.started.status"
  exit_receipt="${invocation_dir}/exit.status"
  runner_log="${invocation_dir}/advance-development.log"
  bootstrap_log="${invocation_dir}/worker.bootstrap.log"

  worker_expected_tag="${expected_tag}"
  worker_expected_index="${expected_index}"
  worker_invocation_dir="${invocation_dir}"
  worker_launch_receipt="${launch_receipt}"
  worker_launch_sha256="${launch_sha}"
  worker_started_receipt="${started_receipt}"
  worker_exit_receipt="${exit_receipt}"
  worker_runner_log="${runner_log}"
  worker_bootstrap_log="${bootstrap_log}"
  worker_expected_attempt_path="$(stage_attempt_path "${expected_index}")"
  worker_expected_completion_path="$(stage_completion_path \
    "${expected_index}")"
  worker_terminal_context=worker_preflight
  trap 'worker_exit_guard "$?"' EXIT

  assert_supervisor_identity
  assert_runner_identity
  ensure_operational_root
  require_owned_directory "${invocation_dir}" 700 \
    "reserved operational stage invocation"
  assert_inherited_supervisor_lock
  verify_launch_receipt "${launch_receipt}" "${expected_tag}" \
    "${invocation_dir}" "${launch_sha}"
  require_absent "${started_receipt}" "worker-started receipt"
  require_absent "${exit_receipt}" "worker exit receipt"
  require_absent "${runner_log}" "advance-development log"
  assert_expected_next_stage "${expected_tag}"

  candidate="${invocation_dir}/.worker.started.status.$$"
  create_operational_candidate "${candidate}" \
    "worker-started receipt candidate" emit_worker_started_receipt \
    "${expected_tag}" "${invocation_dir}" "${launch_receipt}"
  worker_terminal_context=worker_started_receipt_publication
  worker_exit_guard_armed=true
  publish_operational_receipt "${candidate}" "${started_receipt}"
  validate_worker_started_receipt ||
    fail "published worker-started receipt failed validation"

  # The only scientific-runner invocation in this supervisor.
  worker_terminal_context=pre_run_supervisor_identity_validation
  assert_supervisor_identity
  worker_terminal_context=pre_run_runner_identity_validation
  assert_runner_identity
  snapshot_runner_identity_relaxed
  [[ "${worker_runner_identity_state}" == verified ]] ||
    fail "Retry3 operational runner identity snapshot is not verified"
  worker_terminal_context=runner_log_reservation
  exec 8>"${runner_log}"
  chmod 0600 -- "${runner_log}"
  require_owned_file "${runner_log}" 600 \
    "reserved advance-development log"
  worker_runner_call_reserved=true worker_runner_invocation_state=reserved
  worker_terminal_context=runner_invocation
  if "${runner_path}" --advance-development >&8 2>&1; then
    runner_rc=0
  else
    runner_rc=$?
  fi
  worker_runner_rc="${runner_rc}" worker_runner_return_captured=true \
    worker_runner_call_returned=true worker_runner_call_count=1 \
    worker_runner_invocation_state=returned
  worker_terminal_context=runner_log_finalization
  exec 8>&-
  chmod 0444 -- "${runner_log}"
  require_owned_file "${runner_log}" 444 "advance-development log"
  snapshot_runner_log_relaxed
  [[ "${worker_runner_log_valid}" == true && \
        "${worker_runner_log_state}" == sealed_no_open_references && \
        "${worker_runner_log_reference_scan_state}" == clear && \
        "${worker_runner_log_reference_count}" == 0 ]] ||
    fail "advance-development log snapshot is not valid and sealed"
  require_hash "${worker_runner_log_sha256}" \
    "advance-development log SHA-256"

  worker_terminal_context=post_run_supervisor_identity_validation
  assert_supervisor_identity
  worker_terminal_context=post_run_runner_identity_validation
  assert_runner_identity
  snapshot_runner_identity_relaxed
  [[ "${worker_runner_identity_state}" == verified ]] ||
    fail "Retry3 operational runner identity snapshot is not verified"
  worker_terminal_context=post_run_stage_inspection
  inspect_post_run_stage "${expected_tag}"
  worker_terminal_context=outcome_classification
  if [[ "${runner_rc}" == 0 && "${post_completion_present}" == true ]]; then
    outcome=stage_completed
    final_rc=0
  elif [[ "${runner_rc}" == 0 ]]; then
    outcome=runner_returned_success_without_stage_completion
    final_rc=70
  elif [[ "${post_completion_present}" == true ]]; then
    outcome=runner_failed_after_stage_completion
    final_rc="${runner_rc}"
  elif [[ "${post_attempt_present}" == true ]]; then
    outcome=scientific_attempt_failed
    final_rc="${runner_rc}"
  else
    outcome=pre_attempt_failure
    final_rc="${runner_rc}"
  fi

  candidate="${invocation_dir}/.exit.status.$$"
  worker_terminal_context=exit_receipt_publication
  create_operational_candidate "${candidate}" \
    "worker exit receipt candidate" emit_exit_receipt "${outcome}" \
    "${final_rc}" normal normal_completion
  publish_operational_receipt "${candidate}" "${exit_receipt}"
  validate_worker_exit_receipt ||
    fail "published worker exit receipt failed validation"
  worker_terminal_context=normal_completion

  freeze_worker_bootstrap_log_after_detach || true
  exit "${final_rc}"
}

launch_one_stage() {
  local expected_tag="$1" expected_index invocation_dir launch_receipt
  local bootstrap_log candidate launch_sha worker_pid
  expected_tag="$(normalize_expected_stage "${expected_tag}")"
  expected_index="$((10#${expected_tag}))"

  assert_supervisor_identity
  assert_runner_identity
  ensure_operational_root
  acquire_supervisor_lock
  assert_expected_next_stage "${expected_tag}"

  invocation_dir="${operational_root}/stage.${expected_tag}.${stage_names[${expected_index}]}"
  require_absent "${invocation_dir}" \
    "one-shot operational reservation for stage ${expected_tag}"
  mkdir -- "${invocation_dir}"
  chmod 0700 -- "${invocation_dir}"
  require_owned_directory "${invocation_dir}" 700 \
    "reserved operational stage invocation"

  launch_receipt="${invocation_dir}/launch.status"
  bootstrap_log="${invocation_dir}/worker.bootstrap.log"
  candidate="${invocation_dir}/.launch.status.$$"
  create_operational_candidate "${candidate}" \
    "launch receipt candidate" emit_launch_receipt "${expected_tag}" \
    "${invocation_dir}"
  publish_operational_receipt "${candidate}" "${launch_receipt}"
  launch_sha="$(sha256_of "${launch_receipt}")"

  RETRY3_STAGE_ONCE_WORKER=authorized \
    nohup setsid "${script_path}" --worker "${expected_tag}" "${launch_sha}" \
    >"${bootstrap_log}" 2>&1 </dev/null &
  worker_pid=$!
  disown "${worker_pid}" 2>/dev/null || true

  printf 'reserved_stage=%s\n' "${expected_tag}"
  printf 'reserved_stage_name=%s\n' "${stage_names[${expected_index}]}"
  printf 'detached_worker_pid=%s\n' "${worker_pid}"
  printf 'operational_invocation_root=%s\n' "${invocation_dir}"
  printf 'launch_receipt_path=%s\n' "${launch_receipt}"
  printf 'runner_log_path=%s\n' "${invocation_dir}/advance-development.log"
  printf 'exit_receipt_path=%s\n' "${invocation_dir}/exit.status"
}

case "${1:-}" in
--expected-stage)
  [[ "$#" == 2 ]] || {
    usage >&2
    exit 2
  }
  launch_one_stage "$2"
  ;;
--worker)
  [[ "$#" == 3 ]] || fail "malformed private worker invocation"
  run_detached_worker "$2" "$3"
  ;;
--help | -h)
  usage
  ;;
*)
  usage >&2
  exit 2
  ;;
esac
