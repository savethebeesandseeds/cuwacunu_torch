#!/usr/bin/env bash
set -euo pipefail
shopt -s inherit_errexit
umask 077
export LC_ALL=C

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"
readonly closure_schema_id="${schema_id}_bootstrap_publication_failure_closure_v1"
readonly expected_old_runner_sha256="84ce29197961a232887290f045050fa06316652cde31651f6e930b302aec69ba"
readonly expected_old_amendment_sha256="414211345e95965f52d8a0ceb672b5efff74b2c495d67619ca2b3ac788060591"
readonly expected_erratum_sha256="b71b9a953175a3dc1f510e5b6bb8ffe72411a9f21c70e31f96a960be9dd9acb6"
readonly expected_observation_sha256="720b782bc4e5027589e18cbd7428df4527e2b8ed2fc1de07977a30ef8716de64"
readonly expected_empty_sha256="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
readonly expected_device="66"
readonly expected_scratch_inode="287948901175250063"
readonly expected_candidate_inode="7318349394570920"
readonly expected_lock_inode="9851624184966826"

fail() {
  echo "retry2 bootstrap publication failure quarantine: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local key="$1" path="$2" count value
  count="$(awk -v key="${key}" '
    { eq=index($0,"="); if (eq && substr($0,1,eq-1)==key) count += 1 }
    END { print count + 0 }
  ' "${path}")" || fail "could not count ${key}= in ${path}"
  [[ "${count}" == 1 ]] || fail "expected one ${key}= in ${path}, found ${count}"
  value="$(awk -v key="${key}" '
    { eq=index($0,"="); if (eq && substr($0,1,eq-1)==key) print substr($0,eq+1) }
  ' "${path}")" || fail "could not read ${key}= from ${path}"
  printf '%s' "${value}"
}

expect_kv() {
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv "${key}" "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

require_absent() {
  [[ ! -e "$1" && ! -L "$1" ]] || fail "path must be absent: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
  [[ "$(realpath -e -- "$1")" == "$1" ]] || fail "directory is not canonical: $1"
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked regular file: $1"
  [[ "$(realpath -e -- "$1")" == "$1" ]] || fail "file is not canonical: $1"
}

require_pinned_file() {
  local path="$1" expected_sha="$2" expected_mode="$3" label="$4"
  require_file "${path}"
  [[ "$(stat -c '%a:%u:%h' -- "${path}")" == \
    "${expected_mode}:${process_uid}:1" ]] ||
    fail "${label} metadata drifted: ${path}"
  [[ "$(sha256_of "${path}")" == "${expected_sha}" ]] ||
    fail "${label} hash drifted: ${path}"
}

assert_script_identity() {
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == "${process_start_script_sha256}" ]] ||
    fail "quarantine sealer changed after process start"
  [[ "$(stat -c '%i:%d:%s:%u:%h:%a' -- "${script_path}")" == \
    "${process_start_script_inode}:${process_start_script_device}:${process_start_script_bytes}:${process_uid}:1:555" ]] ||
    fail "quarantine sealer identity or metadata drifted"
}

assert_old_runner_lock() {
  local fd_identity path_identity
  [[ "${old_runner_lock_fd}" =~ ^[0-9]+$ ]] || fail "old runner lock FD is invalid"
  fd_identity="$(stat -Lc '%i:%d' -- "/proc/$$/fd/${old_runner_lock_fd}")" ||
    fail "could not read old runner lock descriptor identity"
  path_identity="$(stat -c '%i:%d' -- "${old_runner}")" ||
    fail "could not read old runner lock path identity"
  [[ "${fd_identity}" =~ ^[0-9]+:[0-9]+$ && "${fd_identity}" == "${path_identity}" ]] ||
    fail "old runner lock descriptor/path identity drifted"
  require_pinned_file "${old_runner}" "${expected_old_runner_sha256}" 555 \
    "old retry2 runner"
}

assert_runtime_parent_identity() {
  local identity
  identity="$(stat -c '%i:%d' -- "${runtime_parent}")" ||
    fail "could not read runtime parent identity"
  [[ "${identity}" == "${runtime_parent_identity}" ]] ||
    fail "runtime parent identity drifted"
  [[ "$(stat -c '%d' -- "${runtime_parent}")" == "${expected_device}" ]] ||
    fail "runtime parent device drifted"
}

assert_no_visible_residue_reference() {
  local proc_entry ref ref_path link_identity identity fd_ref fd_listing
  for proc_entry in /proc/[0-9]*; do
    [[ -d "${proc_entry}" ]] || continue
    for ref in cwd root exe; do
      ref_path="${proc_entry}/${ref}"
      if ! link_identity="$(stat -c '%d:%i' -- "${ref_path}" 2>/dev/null)"; then
        if [[ ! -d "${proc_entry}" ]]; then
          continue
        fi
        fail "could not enumerate container-visible process reference: ${ref_path}"
      fi
      [[ "${link_identity}" =~ ^[0-9]+:[0-9]+$ ]] ||
        fail "container-visible process reference identity is malformed: ${ref_path}"
      if [[ ! -L "${ref_path}" ]]; then
        if [[ ! -d "${proc_entry}" ]]; then
          continue
        fi
        fail "container-visible process reference is malformed: ${ref_path}"
      fi
      if identity="$(stat -Lc '%d:%i' -- "${ref_path}" 2>/dev/null)"; then
        case "${identity}" in
        "${expected_device}:${expected_scratch_inode}" | \
        "${expected_device}:${expected_candidate_inode}" | \
        "${expected_device}:${expected_lock_inode}")
          fail "container-visible process reference holds a bootstrap-residue inode: ${ref_path}"
          ;;
        esac
      elif [[ ! -d "${proc_entry}" || ( ! -e "${ref_path}" && ! -L "${ref_path}" ) ]]; then
        continue
      else
        fail "could not inspect container-visible process reference: ${ref_path}"
      fi
    done
    if ! fd_listing="$(find "${proc_entry}/fd" -mindepth 1 -maxdepth 1 \
      -printf '%p\n' 2>/dev/null)"; then
      if [[ ! -d "${proc_entry}" ]]; then
        continue
      fi
      fail "could not enumerate container-visible file descriptors: ${proc_entry}/fd"
    fi
    while IFS= read -r fd_ref; do
      [[ -n "${fd_ref}" ]] || continue
      if identity="$(stat -Lc '%d:%i' -- "${fd_ref}" 2>/dev/null)"; then
        case "${identity}" in
        "${expected_device}:${expected_scratch_inode}" | \
        "${expected_device}:${expected_candidate_inode}" | \
        "${expected_device}:${expected_lock_inode}")
          fail "container-visible file descriptor holds a bootstrap-residue inode: ${fd_ref}"
          ;;
        esac
      elif [[ ! -d "${proc_entry}" || ( ! -e "${fd_ref}" && ! -L "${fd_ref}" ) ]]; then
        continue
      else
        fail "could not inspect container-visible file descriptor: ${fd_ref}"
      fi
    done <<<"${fd_listing}"
  done
}

verify_residue() {
  local closure_candidate_state="${1:-absent}"
  local special file_count dir_count entry_count first_entry
  [[ "${closure_candidate_state}" == absent || \
    "${closure_candidate_state}" == assembling ]] ||
    fail "invalid closure-candidate state for residue verification"
  require_dir "${bootstrap_scratch}"
  require_dir "${candidate_root}"
  require_file "${candidate_lock}"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${bootstrap_scratch}")" == \
    "700:${process_uid}:1:4096:${expected_device}:${expected_scratch_inode}" ]] ||
    fail "bootstrap scratch metadata differs from the observed incident"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${candidate_root}")" == \
    "700:${process_uid}:1:4096:${expected_device}:${expected_candidate_inode}" ]] ||
    fail "runtime-root candidate metadata differs from the observed incident"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${candidate_lock}")" == \
    "600:${process_uid}:1:0:${expected_device}:${expected_lock_inode}" ]] ||
    fail "candidate lock metadata differs from the observed incident"
  [[ "$(sha256_of "${candidate_lock}")" == "${expected_empty_sha256}" ]] ||
    fail "candidate lock is not byte-empty"
  first_entry="$(find "${bootstrap_scratch}" -mindepth 1 -maxdepth 1 -print -quit)" ||
    fail "could not enumerate bootstrap scratch"
  [[ "${first_entry}" == "${candidate_root}" ]] ||
    fail "bootstrap scratch contains an unexpected top-level entry: ${first_entry}"
  special="$(find "${bootstrap_scratch}" -xdev -mindepth 0 ! -type f ! -type d \
    -print -quit)" || fail "could not scan residue entry types"
  [[ -z "${special}" ]] || fail "residue contains a symlink or special entry: ${special}"
  file_count="$(find "${bootstrap_scratch}" -xdev -type f -printf '.' | wc -c)" ||
    fail "could not count residue regular files"
  dir_count="$(find "${bootstrap_scratch}" -xdev -type d -printf '.' | wc -c)" ||
    fail "could not count residue directories"
  entry_count="$(find "${bootstrap_scratch}" -xdev -mindepth 0 -printf '.' | wc -c)" ||
    fail "could not count residue entries"
  [[ "${file_count}:${dir_count}:${entry_count}" == "1:2:3" ]] ||
    fail "residue tree cardinality drifted: ${file_count}:${dir_count}:${entry_count}"
  [[ "$(stat -c '%d' -- "${runtime_parent}")" == "${expected_device}" ]] ||
    fail "runtime parent device drifted"
  require_absent "${runtime_root}"
  require_absent "${stage00_attempt}"
  require_absent "${stage00_completion}"
  require_absent "${closure_root}"
  if [[ "${closure_candidate_state}" == absent ]]; then
    require_absent "${closure_candidate}"
  else
    require_dir "${closure_candidate}"
  fi
}

emit_expected_regular_inventory() {
  printf 'relative_path\tmode\tuid\tlinks\tbytes\tsha256\tdevice\tinode\n'
  printf 'residue/%s/.development.lock\t600\t%s\t1\t0\t%s\t%s\t%s\n' \
    "${candidate_name}" "${process_uid}" "${expected_empty_sha256}" \
    "${expected_device}" "${expected_lock_inode}"
}

emit_expected_directory_inventory() {
  printf 'relative_path\tmode\tuid\tlinks\tbytes\tdevice\tinode\n'
  printf 'residue\t700\t%s\t1\t4096\t%s\t%s\n' \
    "${process_uid}" "${expected_device}" "${expected_scratch_inode}"
  printf 'residue/%s\t700\t%s\t1\t4096\t%s\t%s\n' \
    "${candidate_name}" "${process_uid}" "${expected_device}" \
    "${expected_candidate_inode}"
}

write_inventories() {
  emit_expected_regular_inventory >"${regular_inventory}" ||
    fail "could not write regular-file inventory"
  emit_expected_directory_inventory >"${directory_inventory}" ||
    fail "could not write directory inventory"
}

copy_frozen() {
  local source="$1" destination="$2" expected_sha="$3" label="$4"
  cp --reflink=never -- "${source}" "${destination}" || fail "could not freeze ${label}"
  require_file "${destination}"
  [[ "$(stat -c '%u:%h' -- "${destination}")" == "${process_uid}:1" ]] ||
    fail "frozen ${label} metadata drifted"
  [[ "$(sha256_of "${destination}")" == "${expected_sha}" ]] ||
    fail "frozen ${label} differs from its source"
  chmod 0444 -- "${destination}"
}

emit_failure_status_content() {
  cat <<STATUS
schema_id=${closure_schema_id}
status=complete
closure_kind=pre_attempt_bootstrap_publication_failure_quarantine
scientific_attempt_consumed=false
optimizer_steps=0
candidate_adopted=false
canonical_runtime_published=false
stage00_attempt_published=false
stage00_completion_published=false
scientific_payload_created=false
quarantine_scope=container_cooperative_bootstrap_lock
host_wide_handle_exclusion_claimed=false
observed_failure_exit_code=1
observed_failure_operation=mv_-T_-n
observed_failure_result=permission_denied
old_runner_path=${old_runner}
old_runner_sha256=${expected_old_runner_sha256}
old_runner_device=${old_runner_device}
old_runner_inode=${old_runner_inode}
old_amendment_path=${old_amendment}
old_amendment_sha256=${expected_old_amendment_sha256}
windows_erratum_path=${windows_erratum}
windows_erratum_sha256=${expected_erratum_sha256}
failure_observation_path=${failure_observation}
failure_observation_sha256=${expected_observation_sha256}
quarantine_sealer_path=${script_path}
quarantine_sealer_sha256=${process_start_script_sha256}
quarantine_sealer_mode=555
quarantine_sealer_uid=${process_uid}
quarantine_sealer_links=1
quarantine_sealer_bytes=${process_start_script_bytes}
quarantine_sealer_device=${process_start_script_device}
quarantine_sealer_inode=${process_start_script_inode}
frozen_old_runner_path=${closure_root}/frozen/old_runner.sh
frozen_old_runner_sha256=${expected_old_runner_sha256}
frozen_old_amendment_path=${closure_root}/frozen/old_amendment.md
frozen_old_amendment_sha256=${expected_old_amendment_sha256}
frozen_windows_erratum_path=${closure_root}/frozen/windows_erratum.md
frozen_windows_erratum_sha256=${expected_erratum_sha256}
frozen_failure_observation_path=${closure_root}/frozen/failure_observation.txt
frozen_failure_observation_sha256=${expected_observation_sha256}
frozen_quarantine_sealer_path=${closure_root}/frozen/quarantine_sealer.sh
frozen_quarantine_sealer_sha256=${process_start_script_sha256}
regular_file_inventory_path=${closure_root}/residue_regular_files.inventory.tsv
regular_file_inventory_sha256=${regular_inventory_sha256}
directory_inventory_path=${closure_root}/residue_directories.inventory.tsv
directory_inventory_sha256=${directory_inventory_sha256}
residue_inventory_phase=observed_pre_quarantine_pre_seal
original_bootstrap_scratch_path=${bootstrap_scratch}
original_bootstrap_scratch_device=${expected_device}
original_bootstrap_scratch_inode=${expected_scratch_inode}
original_candidate_root_path=${candidate_root}
original_candidate_root_device=${expected_device}
original_candidate_root_inode=${expected_candidate_inode}
original_candidate_lock_path=${candidate_lock}
original_candidate_lock_device=${expected_device}
original_candidate_lock_inode=${expected_lock_inode}
original_candidate_lock_sha256=${expected_empty_sha256}
quarantined_residue_path=${closure_root}/residue
quarantined_candidate_root_path=${closure_root}/residue/${candidate_name}
quarantined_candidate_lock_path=${closure_root}/residue/${candidate_name}/.development.lock
residue_regular_file_count=1
residue_directory_count=2
residue_total_entry_count=3
residue_source_absent_after_quarantine=true
residue_inode_device_continuity_verified=true
runtime_parent_path=${runtime_parent}
runtime_parent_device=${expected_device}
runtime_parent_inode=${runtime_parent_inode}
closure_root_path=${closure_root}
closure_root_device=${expected_device}
closure_root_inode=${closure_root_inode}
closure_regular_file_count=9
closure_directory_count=4
closure_total_entry_count=13
closure_regular_file_mode=0444
closure_directory_mode=0555
closure_publication_same_device=true
closure_publication_no_clobber=true
canonical_runtime_path=${runtime_root}
canonical_runtime_absent_after_quarantine=true
retry2_bootstrap_failure_is_scientific_input=false
final_holdout_access=false
policy_access=false
STATUS
}

emit_failure_status() {
  local destination="$1"
  emit_failure_status_content >"${destination}" ||
    fail "could not write failure closure receipt"
}

validate_failure_status() {
  local path="$1"
  local regular_path="${2:-${regular_inventory}}"
  local directory_path="${3:-${directory_inventory}}"
  local expected_status_sha256 actual_status_sha256
  require_file "${path}"
  [[ "$(sha256_of "${regular_path}")" == "${regular_inventory_sha256}" ]] ||
    fail "regular-file inventory differs from its exact expected bytes"
  [[ "$(sha256_of "${directory_path}")" == "${directory_inventory_sha256}" ]] ||
    fail "directory inventory differs from its exact expected bytes"
  expected_status_sha256="$(emit_failure_status_content | sha256sum | awk '{print $1}')" ||
    fail "could not derive the exact expected failure receipt hash"
  [[ "${expected_status_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "derived failure receipt hash is malformed"
  actual_status_sha256="$(sha256_of "${path}")" ||
    fail "could not hash the failure closure receipt"
  [[ "${actual_status_sha256}" == "${expected_status_sha256}" ]] ||
    fail "failure closure receipt differs from its exact expected bytes"
  LC_ALL=C awk '
    /^[[:space:]]*$/ { next }
    {
      eq=index($0,"=");
      if (eq <= 1 || eq == length($0)) invalid=1;
      key=substr($0,1,eq-1); value=substr($0,eq+1);
      if (key ~ /sha256$/ && (length(value) != 64 || value !~ /^[0-9a-f]+$/)) invalid=1;
    }
    END { if (invalid) exit 42 }
  ' "${path}" || fail "failure closure receipt has an empty or malformed field"
  expect_kv "${path}" schema_id "${closure_schema_id}"
  expect_kv "${path}" status complete
  expect_kv "${path}" scientific_attempt_consumed false
  expect_kv "${path}" optimizer_steps 0
  expect_kv "${path}" candidate_adopted false
  expect_kv "${path}" old_runner_sha256 "${expected_old_runner_sha256}"
  expect_kv "${path}" old_runner_device "${old_runner_device}"
  expect_kv "${path}" old_runner_inode "${old_runner_inode}"
  expect_kv "${path}" old_amendment_sha256 "${expected_old_amendment_sha256}"
  expect_kv "${path}" windows_erratum_sha256 "${expected_erratum_sha256}"
  expect_kv "${path}" failure_observation_sha256 "${expected_observation_sha256}"
  expect_kv "${path}" quarantine_sealer_sha256 "${process_start_script_sha256}"
  expect_kv "${path}" closure_root_device "${expected_device}"
  expect_kv "${path}" closure_root_inode "${closure_root_inode}"
  expect_kv "${path}" regular_file_inventory_sha256 "${regular_inventory_sha256}"
  expect_kv "${path}" directory_inventory_sha256 "${directory_inventory_sha256}"
  expect_kv "${path}" residue_inventory_phase observed_pre_quarantine_pre_seal
  expect_kv "${path}" residue_source_absent_after_quarantine true
  expect_kv "${path}" residue_inode_device_continuity_verified true
  expect_kv "${path}" retry2_bootstrap_failure_is_scientific_input false
}

assert_prepared_failure_status_identity() {
  require_file "${prepared_failure_status}"
  [[ "$(stat -c '%i:%d' -- "${prepared_failure_status}")" == \
    "${prepared_failure_status_identity}" ]] ||
    fail "prepared failure closure receipt identity drifted"
  [[ "$(sha256_of "${prepared_failure_status}")" == \
    "${prepared_failure_status_sha256}" ]] ||
    fail "prepared failure closure receipt content drifted"
  [[ "$(stat -c '%a:%u:%h:%d' -- "${prepared_failure_status}")" == \
    "444:${process_uid}:1:${expected_device}" ]] ||
    fail "prepared failure closure receipt metadata drifted"
}

verify_prepared_closure_assembly() {
  local root="${closure_candidate}" frozen="${closure_candidate}/frozen"
  local file_count dir_count entry_count special path metadata
  local -a expected_files
  require_dir "${root}"
  require_dir "${frozen}"
  require_absent "${quarantined_residue}"
  require_absent "${failure_status}"
  [[ "$(stat -c '%i:%d' -- "${root}")" == "${closure_candidate_identity}" ]] ||
    fail "prepared closure root identity drifted"
  for path in "${root}" "${frozen}"; do
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not read prepared closure directory metadata: ${path}"
    [[ "${metadata}" == "700:${process_uid}:1:${expected_device}" ]] ||
      fail "prepared closure directory metadata drifted: ${path}"
  done
  expected_files=(
    "${prepared_failure_status}"
    "${regular_inventory}"
    "${directory_inventory}"
    "${frozen}/old_runner.sh"
    "${frozen}/old_amendment.md"
    "${frozen}/windows_erratum.md"
    "${frozen}/failure_observation.txt"
    "${frozen}/quarantine_sealer.sh"
  )
  for path in "${expected_files[@]}"; do
    require_file "${path}"
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not read prepared closure file metadata: ${path}"
    [[ "${metadata}" == "444:${process_uid}:1:${expected_device}" ]] ||
      fail "prepared closure file metadata drifted: ${path}"
  done
  [[ "$(sha256_of "${frozen}/old_runner.sh")" == "${expected_old_runner_sha256}" ]] ||
    fail "prepared frozen old runner drifted"
  [[ "$(sha256_of "${frozen}/old_amendment.md")" == "${expected_old_amendment_sha256}" ]] ||
    fail "prepared frozen old amendment drifted"
  [[ "$(sha256_of "${frozen}/windows_erratum.md")" == "${expected_erratum_sha256}" ]] ||
    fail "prepared frozen erratum drifted"
  [[ "$(sha256_of "${frozen}/failure_observation.txt")" == \
    "${expected_observation_sha256}" ]] || fail "prepared frozen observation drifted"
  [[ "$(sha256_of "${frozen}/quarantine_sealer.sh")" == \
    "${process_start_script_sha256}" ]] || fail "prepared frozen sealer drifted"
  [[ "$(sha256_of "${regular_inventory}")" == "${regular_inventory_sha256}" ]] ||
    fail "prepared regular-file inventory drifted"
  [[ "$(sha256_of "${directory_inventory}")" == "${directory_inventory_sha256}" ]] ||
    fail "prepared directory inventory drifted"
  assert_prepared_failure_status_identity
  validate_failure_status "${prepared_failure_status}" "${regular_inventory}" \
    "${directory_inventory}"
  special="$(find "${root}" -xdev -mindepth 0 ! -type f ! -type d -print -quit)" ||
    fail "could not scan prepared closure entry types"
  [[ -z "${special}" ]] || fail "prepared closure contains a special entry: ${special}"
  file_count="$(find "${root}" -xdev -type f -printf '.' | wc -c)" ||
    fail "could not count prepared closure files"
  dir_count="$(find "${root}" -xdev -type d -printf '.' | wc -c)" ||
    fail "could not count prepared closure directories"
  entry_count="$(find "${root}" -xdev -mindepth 0 -printf '.' | wc -c)" ||
    fail "could not count prepared closure entries"
  [[ "${file_count}:${dir_count}:${entry_count}" == "8:2:10" ]] ||
    fail "prepared closure cardinality drifted: ${file_count}:${dir_count}:${entry_count}"
}

verify_sealed_closure_tree() {
  local root="$1" expected_root_identity="$2" label="$3"
  local residue candidate lock status regular dirs frozen
  local file_count dir_count entry_count special path metadata
  local -a expected_dirs expected_files
  residue="${root}/residue"
  candidate="${residue}/${candidate_name}"
  lock="${candidate}/.development.lock"
  status="${root}/failure.status"
  regular="${root}/residue_regular_files.inventory.tsv"
  dirs="${root}/residue_directories.inventory.tsv"
  frozen="${root}/frozen"
  require_dir "${root}"
  require_dir "${residue}"
  require_dir "${candidate}"
  require_dir "${frozen}"
  require_file "${lock}"
  require_file "${status}"
  require_file "${regular}"
  require_file "${dirs}"
  require_absent "${root}/.failure.status.prepared"
  [[ "$(stat -c '%i:%d' -- "${root}")" == "${expected_root_identity}" ]] ||
    fail "${label} closure root identity drifted"
  [[ "$(stat -c '%d:%i' -- "${residue}")" == \
    "${expected_device}:${expected_scratch_inode}" ]] || fail "${label} residue identity drifted"
  [[ "$(stat -c '%d:%i' -- "${candidate}")" == \
    "${expected_device}:${expected_candidate_inode}" ]] || fail "${label} candidate identity drifted"
  [[ "$(stat -c '%d:%i' -- "${lock}")" == \
    "${expected_device}:${expected_lock_inode}" ]] || fail "${label} lock identity drifted"
  expected_dirs=("${root}" "${residue}" "${candidate}" "${frozen}")
  expected_files=(
    "${status}"
    "${regular}"
    "${dirs}"
    "${lock}"
    "${frozen}/old_runner.sh"
    "${frozen}/old_amendment.md"
    "${frozen}/windows_erratum.md"
    "${frozen}/failure_observation.txt"
    "${frozen}/quarantine_sealer.sh"
  )
  for path in "${expected_dirs[@]}"; do
    require_dir "${path}"
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not read ${label} closure directory metadata: ${path}"
    [[ "${metadata}" == "555:${process_uid}:1:${expected_device}" ]] ||
      fail "${label} closure directory metadata drifted: ${path}"
  done
  for path in "${expected_files[@]}"; do
    require_file "${path}"
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not read ${label} closure file metadata: ${path}"
    [[ "${metadata}" == "444:${process_uid}:1:${expected_device}" ]] ||
      fail "${label} closure file metadata drifted: ${path}"
  done
  [[ "$(stat -c '%s' -- "${lock}")" == 0 ]] ||
    fail "${label} quarantined lock is not empty"
  [[ "$(sha256_of "${lock}")" == "${expected_empty_sha256}" ]] ||
    fail "${label} quarantined lock content drifted"
  [[ "$(sha256_of "${frozen}/old_runner.sh")" == "${expected_old_runner_sha256}" ]] ||
    fail "${label} frozen old runner drifted"
  [[ "$(sha256_of "${frozen}/old_amendment.md")" == "${expected_old_amendment_sha256}" ]] ||
    fail "${label} frozen old amendment drifted"
  [[ "$(sha256_of "${frozen}/windows_erratum.md")" == "${expected_erratum_sha256}" ]] ||
    fail "${label} frozen erratum drifted"
  [[ "$(sha256_of "${frozen}/failure_observation.txt")" == \
    "${expected_observation_sha256}" ]] || fail "${label} frozen observation drifted"
  [[ "$(sha256_of "${frozen}/quarantine_sealer.sh")" == \
    "${process_start_script_sha256}" ]] || fail "${label} frozen sealer drifted"
  [[ "$(sha256_of "${regular}")" == "${regular_inventory_sha256}" ]] ||
    fail "${label} regular-file inventory drifted"
  [[ "$(sha256_of "${dirs}")" == "${directory_inventory_sha256}" ]] ||
    fail "${label} directory inventory drifted"
  validate_failure_status "${status}" "${regular}" "${dirs}"
  special="$(find "${root}" -xdev -mindepth 0 ! -type f ! -type d -print -quit)" ||
    fail "could not scan ${label} closure entry types"
  [[ -z "${special}" ]] || fail "${label} closure contains a special entry: ${special}"
  file_count="$(find "${root}" -xdev -type f -printf '.' | wc -c)" ||
    fail "could not count ${label} closure files"
  dir_count="$(find "${root}" -xdev -type d -printf '.' | wc -c)" ||
    fail "could not count ${label} closure directories"
  entry_count="$(find "${root}" -xdev -mindepth 0 -printf '.' | wc -c)" ||
    fail "could not count ${label} closure entries"
  [[ "${file_count}:${dir_count}:${entry_count}" == "9:4:13" ]] ||
    fail "${label} closure cardinality drifted: ${file_count}:${dir_count}:${entry_count}"
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--seal]"
case "${mode}" in
--plan | --seal) ;;
*) fail "usage: $0 [--plan|--seal]" ;;
esac

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
process_uid="$(id -u)"
process_start_script_sha256="$(sha256_of "${script_path}")"
process_start_script_inode="$(stat -c '%i' -- "${script_path}")"
process_start_script_device="$(stat -c '%d' -- "${script_path}")"
process_start_script_bytes="$(stat -c '%s' -- "${script_path}")"
[[ "${process_uid}" == 0 && "${process_start_script_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
  fail "invalid sealer process identity"
readonly script_path script_dir repo_root process_uid
readonly process_start_script_sha256 process_start_script_inode
readonly process_start_script_device process_start_script_bytes

runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
old_runner="${script_dir}/run_representation_ablation_v2_retry2.sh"
old_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY2_STAGED_RECOVERY_AMENDMENT.md"
windows_erratum="${script_dir}/REPRESENTATION_ABLATION_RETRY2_BOOTSTRAP_PUBLICATION_WINDOWS_ERRATUM.md"
failure_observation="${script_dir}/REPRESENTATION_ABLATION_RETRY2_BOOTSTRAP_PUBLICATION_FAILURE_OBSERVATION.txt"
bootstrap_scratch="${runtime_parent}/.${schema_id}.preflight_scratch"
candidate_name="${schema_id}.runtime_root.candidate"
candidate_root="${bootstrap_scratch}/${candidate_name}"
candidate_lock="${candidate_root}/.development.lock"
runtime_root="${runtime_parent}/${schema_id}"
stage00_attempt="${runtime_root}/stage.00.initialize.attempt.status"
stage00_completion="${runtime_root}/stage.00.initialize.status"
closure_root="${runtime_parent}/${closure_schema_id}"
closure_candidate="${runtime_parent}/.${closure_schema_id}.candidate"
quarantined_residue="${closure_candidate}/residue"
quarantined_candidate="${quarantined_residue}/${candidate_name}"
quarantined_lock="${quarantined_candidate}/.development.lock"
regular_inventory="${closure_candidate}/residue_regular_files.inventory.tsv"
directory_inventory="${closure_candidate}/residue_directories.inventory.tsv"
prepared_failure_status="${closure_candidate}/.failure.status.prepared"
failure_status="${closure_candidate}/failure.status"
readonly runtime_parent old_runner old_amendment windows_erratum failure_observation
readonly bootstrap_scratch candidate_name candidate_root candidate_lock runtime_root
readonly stage00_attempt stage00_completion closure_root closure_candidate
readonly quarantined_residue quarantined_candidate quarantined_lock
readonly regular_inventory directory_inventory prepared_failure_status failure_status

if [[ "${mode}" == --plan ]]; then
  echo "schema_id=${closure_schema_id}.plan"
  echo "old_runner_sha256=${expected_old_runner_sha256}"
  echo "old_amendment_sha256=${expected_old_amendment_sha256}"
  echo "windows_erratum_sha256=${expected_erratum_sha256}"
  echo "failure_observation_sha256=${expected_observation_sha256}"
  echo "quarantine_source=${bootstrap_scratch}"
  echo "quarantine_destination=${closure_root}/residue"
  echo "candidate_adopted=false"
  echo "scientific_attempt_consumed=false"
  exit 0
fi

assert_script_identity
require_dir "${runtime_parent}"
runtime_parent_identity="$(stat -c '%i:%d' -- "${runtime_parent}")" ||
  fail "could not capture runtime parent identity"
[[ "${runtime_parent_identity}" =~ ^[0-9]+:${expected_device}$ ]] ||
  fail "runtime parent identity or device is malformed"
runtime_parent_inode="${runtime_parent_identity%%:*}"
readonly runtime_parent_identity runtime_parent_inode
assert_runtime_parent_identity
require_pinned_file "${old_runner}" "${expected_old_runner_sha256}" 555 "old retry2 runner"
require_pinned_file "${old_amendment}" "${expected_old_amendment_sha256}" 444 \
  "old retry2 amendment"
require_pinned_file "${windows_erratum}" "${expected_erratum_sha256}" 444 \
  "Windows publication erratum"
require_pinned_file "${failure_observation}" "${expected_observation_sha256}" 444 \
  "bootstrap failure observation"

exec {old_runner_lock_fd}<"${old_runner}"
flock -n "${old_runner_lock_fd}" || fail "old retry2 runner is active"
assert_old_runner_lock
old_runner_identity="$(stat -c '%i:%d' -- "${old_runner}")" ||
  fail "could not capture old runner identity"
[[ "${old_runner_identity}" =~ ^[0-9]+:[0-9]+$ ]] ||
  fail "old runner identity is malformed"
old_runner_inode="${old_runner_identity%%:*}"
old_runner_device="${old_runner_identity##*:}"
readonly old_runner_identity old_runner_inode old_runner_device
verify_residue
assert_no_visible_residue_reference
verify_residue
assert_old_runner_lock
assert_script_identity

mkdir -- "${closure_candidate}"
chmod 0700 -- "${closure_candidate}"
assert_runtime_parent_identity
closure_candidate_identity="$(stat -c '%i:%d' -- "${closure_candidate}")" ||
  fail "could not capture closure candidate identity"
[[ "${closure_candidate_identity}" =~ ^[0-9]+:${expected_device}$ ]] ||
  fail "closure candidate identity or device is malformed"
closure_root_inode="${closure_candidate_identity%%:*}"
readonly closure_candidate_identity closure_root_inode
mkdir -- "${closure_candidate}/frozen"
chmod 0700 -- "${closure_candidate}/frozen"
write_inventories
regular_inventory_sha256="$(emit_expected_regular_inventory | sha256sum | awk '{print $1}')" ||
  fail "could not derive the exact regular-file inventory hash"
directory_inventory_sha256="$(emit_expected_directory_inventory | sha256sum | awk '{print $1}')" ||
  fail "could not derive the exact directory inventory hash"
[[ "${regular_inventory_sha256}" =~ ^[0-9a-f]{64}$ && \
  "${directory_inventory_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
  fail "derived inventory hash is malformed"
readonly regular_inventory_sha256 directory_inventory_sha256
[[ "$(sha256_of "${regular_inventory}")" == "${regular_inventory_sha256}" ]] ||
  fail "written regular-file inventory differs from its expected bytes"
[[ "$(sha256_of "${directory_inventory}")" == "${directory_inventory_sha256}" ]] ||
  fail "written directory inventory differs from its expected bytes"
copy_frozen "${old_runner}" "${closure_candidate}/frozen/old_runner.sh" \
  "${expected_old_runner_sha256}" "old retry2 runner"
copy_frozen "${old_amendment}" "${closure_candidate}/frozen/old_amendment.md" \
  "${expected_old_amendment_sha256}" "old retry2 amendment"
copy_frozen "${windows_erratum}" "${closure_candidate}/frozen/windows_erratum.md" \
  "${expected_erratum_sha256}" "Windows publication erratum"
copy_frozen "${failure_observation}" \
  "${closure_candidate}/frozen/failure_observation.txt" \
  "${expected_observation_sha256}" "failure observation"
copy_frozen "${script_path}" "${closure_candidate}/frozen/quarantine_sealer.sh" \
  "${process_start_script_sha256}" "quarantine sealer"
emit_failure_status "${prepared_failure_status}"
validate_failure_status "${prepared_failure_status}"
chmod 0444 -- "${regular_inventory}" "${directory_inventory}" \
  "${prepared_failure_status}"
prepared_failure_status_identity="$(stat -c '%i:%d' -- \
  "${prepared_failure_status}")" ||
  fail "could not capture prepared failure receipt identity"
[[ "${prepared_failure_status_identity}" =~ ^[0-9]+:${expected_device}$ ]] ||
  fail "prepared failure receipt identity or device is malformed"
prepared_failure_status_sha256="$(sha256_of "${prepared_failure_status}")" ||
  fail "could not capture prepared failure receipt hash"
[[ "${prepared_failure_status_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
  fail "prepared failure receipt hash is malformed"
readonly prepared_failure_status_identity prepared_failure_status_sha256
verify_prepared_closure_assembly

assert_old_runner_lock
assert_script_identity
assert_runtime_parent_identity
verify_residue assembling
verify_prepared_closure_assembly
assert_no_visible_residue_reference
verify_residue assembling
verify_prepared_closure_assembly
assert_old_runner_lock
assert_script_identity
assert_runtime_parent_identity
mv -T -n -- "${bootstrap_scratch}" "${quarantined_residue}" ||
  fail "could not quarantine the exact bootstrap residue"
require_absent "${bootstrap_scratch}"
require_absent "${runtime_root}"
require_dir "${quarantined_residue}"
require_dir "${quarantined_candidate}"
require_file "${quarantined_lock}"
[[ "$(stat -c '%d:%i' -- "${quarantined_residue}")" == \
  "${expected_device}:${expected_scratch_inode}" ]] || fail "quarantine move changed scratch identity"
[[ "$(stat -c '%d:%i' -- "${quarantined_candidate}")" == \
  "${expected_device}:${expected_candidate_inode}" ]] || fail "quarantine move changed candidate identity"
[[ "$(stat -c '%d:%i' -- "${quarantined_lock}")" == \
  "${expected_device}:${expected_lock_inode}" ]] || fail "quarantine move changed lock identity"
assert_prepared_failure_status_identity
require_absent "${failure_status}"
mv -T -n -- "${prepared_failure_status}" "${failure_status}" ||
  fail "could not promote the prepared failure closure receipt"
require_absent "${prepared_failure_status}"
require_file "${failure_status}"
[[ "$(stat -c '%i:%d' -- "${failure_status}")" == \
  "${prepared_failure_status_identity}" ]] ||
  fail "failure closure receipt promotion changed inode or device"
[[ "$(sha256_of "${failure_status}")" == \
  "${prepared_failure_status_sha256}" ]] ||
  fail "failure closure receipt promotion changed content"
[[ "$(stat -c '%a:%u:%h:%d' -- "${failure_status}")" == \
  "444:${process_uid}:1:${expected_device}" ]] ||
  fail "promoted failure closure receipt metadata drifted"
validate_failure_status "${failure_status}"

chmod 0444 -- "${quarantined_lock}"
chmod 0555 -- "${quarantined_candidate}" "${quarantined_residue}"
chmod 0555 -- "${closure_candidate}/frozen"
chmod 0555 -- "${closure_candidate}"
verify_sealed_closure_tree "${closure_candidate}" "${closure_candidate_identity}" \
  "pre-publication"
assert_no_visible_residue_reference
verify_sealed_closure_tree "${closure_candidate}" "${closure_candidate_identity}" \
  "pre-publication-after-reference-scan"
assert_old_runner_lock
assert_script_identity
assert_runtime_parent_identity
require_absent "${runtime_root}"
require_absent "${closure_root}"
mv -T -n -- "${closure_candidate}" "${closure_root}" ||
  fail "could not publish the quarantine closure"
require_absent "${closure_candidate}"
verify_sealed_closure_tree "${closure_root}" "${closure_candidate_identity}" \
  "published"
assert_no_visible_residue_reference
verify_sealed_closure_tree "${closure_root}" "${closure_candidate_identity}" \
  "published-after-reference-scan"
require_absent "${runtime_root}"
require_absent "${stage00_attempt}"
require_absent "${stage00_completion}"
assert_old_runner_lock
assert_script_identity
assert_runtime_parent_identity
final_receipt_sha256="$(sha256_of "${closure_root}/failure.status")" ||
  fail "could not hash the published failure closure receipt"
[[ "${final_receipt_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
  fail "published failure closure receipt hash is malformed"
echo "closure_receipt=${closure_root}/failure.status"
echo "closure_receipt_sha256=${final_receipt_sha256}"
