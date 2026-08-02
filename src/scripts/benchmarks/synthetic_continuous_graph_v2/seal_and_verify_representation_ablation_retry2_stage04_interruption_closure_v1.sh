#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly source_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"
readonly closure_schema_id="${source_schema_id}_stage04_interruption_closure_v1"
readonly expected_container_hostname="8ba42d04128d"

readonly expected_amendment_sha256="c0310d27fea46c97ee9517362b809c6f53c8d848a3ea9023a3d7aaf1c3a347f6"
readonly expected_runner_sha256="91915b7d32f0c1679d69e9077bbf8eb88777e367f590b34c2832a11fdcc26768"
readonly expected_content_inventory_sha256="622d8bf4e087d324abc4171dbfea45fed78febf1817cdc0f353b40541cb82143"
readonly expected_regular_file_count=60
readonly expected_directory_count=28
readonly expected_entry_count_below_root=87
readonly expected_regular_file_bytes=58518408
readonly expected_device=66
readonly expected_proc_maps_device="00:42"

readonly expected_runner_inode=168884986026428276
readonly expected_runner_bytes=414662
readonly expected_runtime_inode=110901140824013045
readonly expected_runtime_bytes=4096
readonly expected_lock_inode=23643898044239335

readonly expected_stage00_attempt_sha256="ed75d13c10f3381023b9bd648eca4a25dc4eb1d5956ef28340049ef27d07fa69"
readonly expected_stage00_completion_sha256="3cccdeae5b2765cbcc2c7c03095562b4a3963538ce7618e439a62ad703635433"
readonly expected_stage01_attempt_sha256="629bec6dd7ec10465c1c11bf51c9710af2598bd4aa25cba3ca29b72c82c883c3"
readonly expected_stage01_completion_sha256="5f35515e76287c647e2bdd09a6b466b548c98393c8b40b3705f986def02ef741"
readonly expected_stage02_attempt_sha256="901c1d9e0501cdf23c2754fd1c18872dd05c905755aa3fa009d4d290a3356243"
readonly expected_stage02_completion_sha256="54371e6aa019d4b2af3be819c13162812a9dd13c80624c7efd7340dd166900f8"
readonly expected_stage03_attempt_sha256="ec7c471d3c4fafb959734d1ad8e7b716ab5535027951d55db0c812bb7fee6f1c"
readonly expected_stage03_completion_sha256="d0ba0e40b8489a660196e23ccb1c63bfc198dfa22a2d3b40115e48b12fd60693"
readonly expected_stage04_attempt_sha256="19a7597dbe5a94f97908de3103cfa62d4e144c7648a5c91fbe82398d6cb82ae2"

readonly expected_stage00_attempt_inode=54043195529314981
readonly expected_stage00_completion_inode=18858823440482675
readonly expected_stage01_attempt_inode=8162774325153602
readonly expected_stage01_completion_inode=22236523161012467
readonly expected_stage02_attempt_inode=60235645016624941
readonly expected_stage02_completion_inode=36591746973255941
readonly expected_stage03_attempt_inode=26458647811667718
readonly expected_stage03_completion_inode=10414574139165542
readonly expected_stage04_attempt_inode=125537839612953903

readonly expected_no_tf_capture_config_sha256="b8ce96ced46131cf9d487c39931b041359d2312ea98744e7ed39d42463be6427"
readonly expected_no_tf_policy_sha256="1503407ad50dd86a5ba855c7247e0efdb4b78c11a22c97d359a8b2e64b518d37"
readonly expected_no_tf_net_sha256="df4398835b7eff3496ac8c20e7713b2d3d3a245754916c81b77271c696a08cda"
readonly expected_no_tf_train_config_sha256="4ff677252a8e9093b2e2dd65f2b8668160071727467a0865844e4bb9a601f5f0"
readonly expected_no_tf_log_sha256="b5d42b1e6d528e1d8933c48739380f79c7aac6ed650de0e42cbbd2a6e6a74237"
readonly expected_no_tf_spawn_sha256="5b5ae2c19b84dacfd1952fa4cde3d37747d877ecd427c5760412b1e8b3b788c1"
readonly expected_no_tf_report_sha256="b6229eca88e5aa4ab0879488f78b80e8682a55f2bac2bc66af68c20861b1c077"
readonly expected_no_tf_checkpoint_sha256="47ed5e41853d8c7c653624af8acd902f17296d7fb95811b9b7da1c8f037c4b20"
readonly expected_no_tf_manifest_sha256="6089ac29134465d79b0784208cf15156d594f3135f00d7f531d955b1b5ab9dc8"
readonly expected_no_tf_events_sha256="fc25fd28ea60b77ddfabbeb9a75f2ddf35553b2fbe8d020950c2521d0180f417"
readonly expected_no_tf_registry_sha256="e03a91247a6f4bb110cfaf6b4ff5c3d892f5bff48150de84ca20aba2c856181e"
readonly expected_no_tf_layout_sha256="a2a4bbd45074b32739d32667d2260bc537a9a60c85339989f9a247a584d5741f"

readonly expected_optimizer_steps=3000
readonly observed_optimizer_steps=2160
readonly observed_checkpoint_step=2150
readonly observed_event_records=12530
readonly observed_last_event_unix_ms=1785548394041
readonly observed_host_exit_decimal=1073807364
readonly observed_host_exit_hex="0x40010004"
readonly observed_host_exit_symbol="DBG_TERMINATE_PROCESS"

fail() {
  echo "retry2 stage04 interruption closure: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
}

assert_absent() {
  [[ ! -e "$1" && ! -L "$1" ]] || fail "$2 unexpectedly exists: $1"
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
    [[ ! -L "${current}" ]] || fail "symlink path component: ${current}"
  done
}

kv() {
  local path="$1" key="$2" count value
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
    fail "${path}: expected one ${key}= entry, found ${count}"
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
  local actual
  actual="$(kv "$1" "$2")"
  [[ "${actual}" == "$3" ]] ||
    fail "$1: expected $2=$3, found ${actual}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
source_runtime="${runtime_parent}/${source_schema_id}"
closure_root="${runtime_parent}/${closure_schema_id}"
staging_root="${runtime_parent}/.${closure_schema_id}.candidate"
source_snapshot_final="${closure_root}/source_snapshot"

runner_path="${script_dir}/run_representation_ablation_v2_retry2.sh"
amendment_path="${script_dir}/REPRESENTATION_ABLATION_RETRY2_STAGE04_INTERRUPTION_RECOVERY_AMENDMENT.md"
source_lock_path="${source_runtime}/.development.lock"
source_frozen_runner="${source_runtime}/frozen_sources/$(basename "${runner_path}")"

receipt_path="${closure_root}/interruption_closure.status"
regular_inventory_path="${closure_root}/source_regular_files.inventory.tsv"
directory_inventory_path="${closure_root}/source_directories.inventory.tsv"
frozen_root="${closure_root}/frozen_sources"
frozen_sealer_path="${frozen_root}/$(basename "${script_path}")"
frozen_amendment_path="${frozen_root}/$(basename "${amendment_path}")"

stage00_attempt="${source_runtime}/stage.00.initialize.attempt.status"
stage00_completion="${source_runtime}/stage.00.initialize.status"
stage01_attempt="${source_runtime}/stage.01.canonical_import.attempt.status"
stage01_completion="${source_runtime}/stage.01.canonical_import.status"
stage02_attempt="${source_runtime}/stage.02.endpoint_import.attempt.status"
stage02_completion="${source_runtime}/stage.02.endpoint_import.status"
stage03_attempt="${source_runtime}/stage.03.time_only_training.attempt.status"
stage03_completion="${source_runtime}/stage.03.time_only_training.status"
stage04_attempt="${source_runtime}/stage.04.no_tf_alignment_training.attempt.status"
stage04_completion="${source_runtime}/stage.04.no_tf_alignment_training.status"

no_tf_root="${source_runtime}/arms/no_tf_alignment"
no_tf_config="${no_tf_root}/config"
no_tf_training="${no_tf_root}/training"
no_tf_job="${no_tf_training}/job"
no_tf_report="${no_tf_job}/channel_representation.report"
no_tf_checkpoint="${no_tf_job}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
no_tf_manifest="${no_tf_job}/job.manifest"
no_tf_events="${no_tf_job}/runtime.job_events.probe"
no_tf_log="${no_tf_root}/training.log"
no_tf_spawn="${no_tf_training}/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/zts_ecf8b8d26bc21d88/component_spawn.ref"
no_tf_registry="${no_tf_training}/system/component_spawn_registry.v1.lls"
no_tf_layout="${no_tf_training}/system/runtime_layout.v1.lls"

readonly process_start_sealer_sha256="$(sha256_of "${script_path}")"
readonly process_start_sealer_mode="$(stat -c '%a' -- "${script_path}")"
readonly process_start_sealer_links="$(stat -c '%h' -- "${script_path}")"
readonly process_start_sealer_uid="$(stat -c '%u' -- "${script_path}")"
readonly process_start_sealer_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_sealer_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_sealer_device="$(stat -c '%d' -- "${script_path}")"

declare -a temporary_files=()
runner_lock_fd=""
source_lock_fd=""

cleanup() {
  local path
  for path in "${temporary_files[@]:-}"; do
    [[ -n "${path}" ]] && rm -f -- "${path}" 2>/dev/null || true
  done
}
trap cleanup EXIT

new_temp_file() {
  local __result_var="$1" template="$2" created
  created="$(mktemp "${template}")"
  temporary_files+=("${created}")
  printf -v "${__result_var}" '%s' "${created}"
}

capture_find0() {
  local __result_var="$1" root="$2" output
  shift 2
  new_temp_file output "/tmp/${closure_schema_id}.find.raw.XXXXXX"
  find "${root}" -xdev "$@" -print0 >"${output}" ||
    fail "checked tree traversal failed: ${root}"
  printf -v "${__result_var}" '%s' "${output}"
}

capture_sorted_find0() {
  local __result_var="$1" root="$2" raw sorted
  shift 2
  capture_find0 raw "${root}" "$@"
  new_temp_file sorted "/tmp/${closure_schema_id}.find.sorted.XXXXXX"
  LC_ALL=C sort -z -- "${raw}" >"${sorted}" ||
    fail "checked tree sort failed: ${root}"
  printf -v "${__result_var}" '%s' "${sorted}"
}

capture_command_output() {
  local __result_var="$1" label="$2" output
  shift 2
  new_temp_file output "/tmp/${closure_schema_id}.command.output.XXXXXX"
  "$@" >"${output}" || fail "checked producer failed: ${label}"
  printf -v "${__result_var}" '%s' "${output}"
}

capture_proc_children0() {
  local __result_var="$1" proc="$2" child_dir="$3" output status
  new_temp_file output "/tmp/${closure_schema_id}.proc.children.XXXXXX"
  if find "${proc}/${child_dir}" -mindepth 1 -maxdepth 1 -print0 \
    >"${output}" 2>/dev/null; then
    printf -v "${__result_var}" '%s' "${output}"
    return 0
  else
    status=$?
  fi
  [[ ! -d "${proc}" ]] && return 1
  fail "cannot enumerate extant process ${proc#/proc/} ${child_dir} references (find status ${status})"
}

read_proc_reference() {
  local __target_var="$1" __identity_var="$2" proc="$3" ref="$4"
  local read_target read_identity status
  if read_target="$(readlink -- "${ref}" 2>/dev/null)"; then
    :
  else
    status=$?
    [[ ! -e "${ref}" && ! -L "${ref}" ]] && return 1
    [[ ! -d "${proc}" ]] && return 1
    fail "cannot read extant process reference ${proc#/proc/}:${ref#${proc}/} (readlink status ${status})"
  fi
  if read_identity="$(stat -L -c '%d:%i' -- "${ref}" 2>/dev/null)"; then
    :
  else
    status=$?
    [[ ! -e "${ref}" && ! -L "${ref}" ]] && return 1
    [[ ! -d "${proc}" ]] && return 1
    fail "cannot stat extant process reference ${proc#/proc/}:${ref#${proc}/} (stat status ${status})"
  fi
  printf -v "${__target_var}" '%s' "${read_target}"
  printf -v "${__identity_var}" '%s' "${read_identity}"
}

proc_file_contains_literal() {
  local proc="$1" path="$2" literal="$3" label="$4" status
  if grep -aFq -- "${literal}" "${path}" 2>/dev/null; then
    return 0
  else
    status=$?
  fi
  [[ "${status}" == 1 ]] && return 1
  [[ ! -d "${proc}" ]] && return 1
  fail "cannot scan extant process ${proc#/proc/} ${label} (grep status ${status})"
}

assert_proc_maps_no_protected_inode() {
  local proc="$1" protected_inodes="$2" label="$3" status
  if awk -v expected_device="${expected_proc_maps_device}" '
    NR == FNR {
      protected["inode:" $1] = 1
      next
    }
    tolower($4) == expected_device && protected["inode:" $5] { exit 42 }
  ' "${protected_inodes}" "${proc}/maps"; then
    return 0
  else
    status=$?
  fi
  [[ ! -d "${proc}" ]] && return 0
  [[ "${status}" != 42 ]] ||
    fail "live process mapping inode reaches ${label}: pid ${proc#/proc/}"
  fail "cannot scan extant process ${proc#/proc/} mapping inodes (awk status ${status})"
}

read_proc_link_target() {
  local __target_var="$1" proc="$2" ref="$3" read_target status
  if read_target="$(readlink -- "${ref}" 2>/dev/null)"; then
    printf -v "${__target_var}" '%s' "${read_target}"
    return 0
  else
    status=$?
  fi
  [[ ! -e "${ref}" && ! -L "${ref}" ]] && return 1
  [[ ! -d "${proc}" ]] && return 1
  fail "cannot read extant process link ${proc#/proc/}:${ref#${proc}/} (readlink status ${status})"
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--audit|--seal|--verify]"
case "${mode}" in
--plan | --audit | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--audit|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${closure_schema_id}.plan
source_schema_id=${source_schema_id}
source_runtime_root=${source_runtime}
source_runtime_mutation_authorized=false
old_runner_path=${runner_path}
lock_order=old_runner_then_retry2_development_lock
lock_open_mode=read_only
source_snapshot_count=2
source_regular_file_count=${expected_regular_file_count}
source_directory_count=${expected_directory_count}
source_regular_file_bytes=${expected_regular_file_bytes}
source_content_inventory_sha256=${expected_content_inventory_sha256}
closure_root=${closure_root}
closure_receipt=${receipt_path}
source_regular_inventory=${regular_inventory_path}
source_directory_inventory=${directory_inventory_path}
source_snapshot=${source_snapshot_final}
source_snapshot_copy_method=cp_archive_reflink_never
source_snapshot_regular_file_mode=0444
source_snapshot_directory_mode=0555
publication_method=atomic_no_clobber_sibling_directory_rename
stage04_scientific_attempt_consumed=true
stage04_completion_present=false
stage04_optimizer_steps_observed=${observed_optimizer_steps}
stage04_last_checkpoint_optimizer_step=${observed_checkpoint_step}
termination_classification=external_host_or_controller_termination_supported_not_actor_attributed
partial_payload_adoption_authorized=false
checkpoint_resume_authorized=false
same_runtime_reentry_authorized=false
certified_input_access=false
final_holdout_access=false
policy_access=false
PLAN
}

assert_container_boundary() {
  [[ -f /.dockerenv ]] || fail "must run inside Docker container unnamed_taoist"
  [[ "${repo_root}" == /cuwacunu ]] || fail "repository root is not /cuwacunu"
  [[ "$(id -u)" == 0 ]] || fail "sealer must run as container uid 0"
  [[ "$(hostname)" == "${expected_container_hostname}" ]] ||
    fail "container hostname is not the pinned unnamed_taoist identity"
}

verify_hash() {
  local path="$1" expected="$2" label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has non-unit link count"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
}

verify_file_exact() {
  local path="$1" mode_expected="$2" bytes_expected="$3"
  local inode_expected="$4" hash_expected="$5" label="$6"
  verify_hash "${path}" "${hash_expected}" "${label}"
  [[ "$(stat -c '%a' -- "${path}")" == "${mode_expected}" ]] ||
    fail "${label} mode drifted"
  [[ "$(stat -c '%u' -- "${path}")" == 0 &&
    "$(stat -c '%g' -- "${path}")" == 0 ]] || fail "${label} owner drifted"
  [[ "$(stat -c '%s' -- "${path}")" == "${bytes_expected}" ]] ||
    fail "${label} size drifted"
  [[ "$(stat -c '%i' -- "${path}")" == "${inode_expected}" ]] ||
    fail "${label} inode drifted"
  [[ "$(stat -c '%d' -- "${path}")" == "${expected_device}" ]] ||
    fail "${label} device drifted"
}

verify_dir_exact() {
  local path="$1" inode_expected="$2" label="$3"
  reject_symlink_components "${path}"
  require_dir "${path}"
  [[ "$(stat -c '%a:%u:%g:%d:%i' -- "${path}")" == \
    "700:0:0:${expected_device}:${inode_expected}" ]] ||
    fail "${label} identity or metadata drifted"
}

assert_process_start_sealer_identity() {
  verify_hash "${script_path}" "${process_start_sealer_sha256}" \
    "executing stage04 closure sealer"
  [[ "$(stat -c '%a' -- "${script_path}")" == \
    "${process_start_sealer_mode}" ]] || fail "executing sealer mode changed"
  [[ "$(stat -c '%u' -- "${script_path}")" == \
    "${process_start_sealer_uid}" ]] || fail "executing sealer owner changed"
  [[ "$(stat -c '%s' -- "${script_path}")" == \
    "${process_start_sealer_bytes}" ]] || fail "executing sealer size changed"
  [[ "$(stat -c '%i' -- "${script_path}")" == \
    "${process_start_sealer_inode}" ]] || fail "executing sealer inode changed"
  [[ "$(stat -c '%d' -- "${script_path}")" == \
    "${process_start_sealer_device}" ]] || fail "executing sealer device changed"
  [[ "${process_start_sealer_mode}" == 555 &&
    "${process_start_sealer_links}" == 1 &&
    "${process_start_sealer_uid}" == 0 ]] ||
    fail "executing sealer must be uid0 mode0555 link1"
}

verify_external_authorities() {
  assert_process_start_sealer_identity
  verify_file_exact "${runner_path}" 555 "${expected_runner_bytes}" \
    "${expected_runner_inode}" "${expected_runner_sha256}" \
    "operational Retry2 runner"
  verify_hash "${amendment_path}" "${expected_amendment_sha256}" \
    "stage04 interruption recovery amendment"
  [[ "$(stat -c '%a:%u' -- "${amendment_path}")" == 444:0 ]] ||
    fail "stage04 amendment must be uid0 mode0444"
  verify_hash "${source_frozen_runner}" "${expected_runner_sha256}" \
    "Retry2 frozen runner"
  [[ "$(stat -c '%a:%u:%s:%d:%i' -- "${source_frozen_runner}")" == \
    "444:0:${expected_runner_bytes}:${expected_device}:54887620459446969" ]] ||
    fail "Retry2 frozen runner identity drifted"
}

assert_runner_lock_identity() {
  [[ -n "${runner_lock_fd}" && -e "/proc/self/fd/${runner_lock_fd}" ]] ||
    fail "old-runner lock descriptor is not open"
  [[ "$(stat -L -c '%d:%i' -- "/proc/self/fd/${runner_lock_fd}")" == \
    "${expected_device}:${expected_runner_inode}" ]] ||
    fail "old-runner lock descriptor identity drifted"
  [[ "$(stat -c '%d:%i' -- "${runner_path}")" == \
    "${expected_device}:${expected_runner_inode}" ]] ||
    fail "old-runner path no longer names locked inode"
  verify_external_authorities
}

assert_source_lock_identity() {
  [[ -n "${source_lock_fd}" && -e "/proc/self/fd/${source_lock_fd}" ]] ||
    fail "Retry2 development-lock descriptor is not open"
  [[ "$(stat -L -c '%d:%i' -- "/proc/self/fd/${source_lock_fd}")" == \
    "${expected_device}:${expected_lock_inode}" ]] ||
    fail "Retry2 development-lock descriptor identity drifted"
  [[ "$(stat -c '%d:%i' -- "${source_lock_path}")" == \
    "${expected_device}:${expected_lock_inode}" ]] ||
    fail "Retry2 development-lock path no longer names locked inode"
  assert_runner_lock_identity
}

acquire_locks_in_order() {
  verify_external_authorities
  exec {runner_lock_fd}<"${runner_path}"
  flock -n "${runner_lock_fd}" ||
    fail "operational Retry2 runner is locked or active"
  assert_runner_lock_identity

  reject_symlink_components "${source_lock_path}"
  require_file "${source_lock_path}"
  exec {source_lock_fd}<"${source_lock_path}"
  flock -n "${source_lock_fd}" ||
    fail "Retry2 development lock is held; refusing closure"
  assert_source_lock_identity
}

relative_path_at() {
  local root="$1" path="$2"
  if [[ "${path}" == "${root}" ]]; then printf '.'; else
    printf '%s' "${path#${root}/}"
  fi
}

validate_inventory_relative_path() {
  local rel="$1"
  [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* &&
    "${rel}" != *'|'* ]] || fail "non-canonical inventory path: ${rel}"
}

emit_expected_directory_paths() {
  cat <<'DIRECTORIES'
.
arms
arms/canonical
arms/canonical/affine
arms/endpoint_scale
arms/endpoint_scale/config
arms/no_tf_alignment
arms/no_tf_alignment/config
arms/no_tf_alignment/training
arms/no_tf_alignment/training/components
arms/no_tf_alignment/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg
arms/no_tf_alignment/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns
arms/no_tf_alignment/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/zts_ecf8b8d26bc21d88
arms/no_tf_alignment/training/job
arms/no_tf_alignment/training/system
arms/time_only
arms/time_only/config
arms/time_only/training
arms/time_only/training/components
arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg
arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns
arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c
arms/time_only/training/job
arms/time_only/training/system
frozen_sources
imports
imports/retry1_endpoint_v1
scratch
DIRECTORIES
}

expected_source_file_mode() {
  case "$1" in
  frozen_sources/frozen_representation_affine_probe)
    printf 555
    ;;
  .development.lock | \
    arms/time_only/training/system/runtime_layout.v1.lls | \
    arms/time_only/training/system/component_spawn_registry.v1.lls | \
    arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c/component_spawn.ref | \
    arms/time_only/training/job/runtime.component_training_update.fact | \
    arms/time_only/training/job/runtime.health_measurement.fact | \
    arms/time_only/training/job/runtime.checkpoint_io.fact | \
    arms/time_only/training/job/runtime.job_events.probe | \
    arms/time_only/training/job/lattice.checkpoint.fact | \
    arms/time_only/training/job/lattice.source_analytics.fact | \
    arms/time_only/training/job/job.state | \
    arms/time_only/training/job/lattice.exposure.fact | \
    arms/no_tf_alignment/training.log | \
    arms/no_tf_alignment/training/*)
    printf 600
    ;;
  *) printf 444 ;;
  esac
}

compute_content_inventory_sha256_at() {
  local root="$1" __result_var="$2" digest_file digest
  new_temp_file digest_file "/tmp/${closure_schema_id}.content.digest.XXXXXX"
  (
    cd "${root}"
    find . -xdev -type f -print0 | LC_ALL=C sort -z |
      xargs -0 sha256sum | sha256sum | awk '{print $1}'
  ) >"${digest_file}" || fail "content-inventory pipeline failed: ${root}"
  IFS= read -r digest <"${digest_file}" ||
    fail "content-inventory digest is missing: ${root}"
  [[ "${digest}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "content-inventory digest is malformed: ${root}"
  printf -v "${__result_var}" '%s' "${digest}"
}

verify_source_tree_shape_and_modes() {
  assert_source_lock_identity
  assert_sensitive_outputs_absent_before_tree_read
  reject_symlink_components "${source_runtime}"
  require_dir "${source_runtime}"
  [[ "$(realpath -e -- "${source_runtime}")" == "${source_runtime}" ]] ||
    fail "Retry2 runtime path is not canonical"
  [[ "$(stat -c '%a:%u:%g:%s:%d:%i' -- "${source_runtime}")" == \
    "700:0:0:${expected_runtime_bytes}:${expected_device}:${expected_runtime_inode}" ]] ||
    fail "Retry2 runtime-root identity drifted"

  local entry rel expected_mode entry_count=0 file_count=0 dir_count=0
  local byte_count=0 entries content_digest
  capture_find0 entries "${source_runtime}" -mindepth 0
  while IFS= read -r -d '' entry; do
    ((entry_count += 1))
    [[ ! -L "${entry}" ]] || fail "Retry2 runtime contains symlink: ${entry}"
    [[ "$(stat -c '%d' -- "${entry}")" == "${expected_device}" ]] ||
      fail "Retry2 runtime contains cross-device entry: ${entry}"
    [[ "$(stat -c '%u:%g' -- "${entry}")" == 0:0 ]] ||
      fail "Retry2 runtime entry owner drifted: ${entry}"
    rel="$(relative_path_at "${source_runtime}" "${entry}")"
    validate_inventory_relative_path "${rel}"
    if [[ -f "${entry}" ]]; then
      ((file_count += 1))
      byte_count=$((byte_count + $(stat -c '%s' -- "${entry}")))
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "Retry2 runtime file has non-unit link count: ${rel}"
      expected_mode="$(expected_source_file_mode "${rel}")"
      [[ "$(stat -c '%a' -- "${entry}")" == "${expected_mode}" ]] ||
        fail "Retry2 runtime file mode drifted: ${rel}"
    elif [[ -d "${entry}" ]]; then
      ((dir_count += 1))
      [[ "$(stat -c '%a' -- "${entry}")" == 700 ]] ||
        fail "Retry2 runtime directory mode drifted: ${rel}"
    else
      fail "Retry2 runtime contains special entry: ${entry}"
    fi
  done <"${entries}"

  [[ "${file_count}" == "${expected_regular_file_count}" &&
    "${dir_count}" == "${expected_directory_count}" &&
    "$((entry_count - 1))" == "${expected_entry_count_below_root}" &&
    "${byte_count}" == "${expected_regular_file_bytes}" ]] ||
    fail "Retry2 runtime aggregate count or byte drift"
  assert_sensitive_outputs_absent_before_tree_read
  compute_content_inventory_sha256_at "${source_runtime}" content_digest
  [[ "${content_digest}" == "${expected_content_inventory_sha256}" ]] ||
    fail "Retry2 complete relative content inventory drifted"

  local expected_paths actual_paths directory_entries
  new_temp_file expected_paths "/tmp/${closure_schema_id}.dirs.expected.XXXXXX"
  new_temp_file actual_paths "/tmp/${closure_schema_id}.dirs.actual.XXXXXX"
  emit_expected_directory_paths >"${expected_paths}"
  capture_sorted_find0 directory_entries "${source_runtime}" -type d
  while IFS= read -r -d '' entry; do
    relative_path_at "${source_runtime}" "${entry}"
    printf '\n'
  done <"${directory_entries}" >"${actual_paths}"
  cmp -s -- "${expected_paths}" "${actual_paths}" ||
    fail "Retry2 directory path set drifted"
  assert_source_lock_identity
}

emit_source_regular_inventory() {
  local path rel entries
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tmtime\tsha256\n'
  capture_sorted_find0 entries "${source_runtime}" -type f
  while IFS= read -r -d '' path; do
    rel="$(relative_path_at "${source_runtime}" "${path}")"
    validate_inventory_relative_path "${rel}"
    printf '%s\t' "${rel}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\t' -- "${path}"
    printf '%s\n' "$(sha256_of "${path}")"
  done <"${entries}"
}

emit_source_directory_inventory() {
  local path rel entries
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tmtime\n'
  capture_sorted_find0 entries "${source_runtime}" -type d
  while IFS= read -r -d '' path; do
    rel="$(relative_path_at "${source_runtime}" "${path}")"
    validate_inventory_relative_path "${rel}"
    printf '%s\t' "${rel}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\n' -- "${path}"
  done <"${entries}"
}

take_source_snapshot() {
  local regular="$1" directories="$2"
  assert_sensitive_outputs_absent_before_tree_read
  assert_no_live_references
  verify_source_tree_shape_and_modes
  emit_source_regular_inventory >"${regular}"
  emit_source_directory_inventory >"${directories}"
  [[ "$(($(wc -l <"${regular}") - 1))" == \
    "${expected_regular_file_count}" ]] || fail "source regular inventory count drift"
  [[ "$(($(wc -l <"${directories}") - 1))" == \
    "${expected_directory_count}" ]] || fail "source directory inventory count drift"
  assert_no_live_references
}

verify_receipt_bound_artifact() {
  local receipt="$1" path_key="$2" hash_key="$3" label="$4"
  local path digest
  path="$(kv "${receipt}" "${path_key}")"
  digest="$(kv "${receipt}" "${hash_key}")"
  [[ "${path}" == "${source_runtime}/"* ]] ||
    fail "${label} path escapes Retry2 runtime"
  verify_hash "${path}" "${digest}" "${label}"
}

verify_stage_attempt() {
  local path="$1" ordinal="$2" name="$3" previous="$4" previous_hash="$5"
  local expected_hash="$6" expected_inode="$7" expected_bytes
  case "${ordinal}" in
  00) expected_bytes=7818 ;;
  01) expected_bytes=8022 ;;
  02) expected_bytes=8027 ;;
  03) expected_bytes=8029 ;;
  04) expected_bytes=8038 ;;
  *) fail "unknown exact attempt-byte contract for stage ${ordinal}" ;;
  esac
  verify_file_exact "${path}" 444 "${expected_bytes}" \
    "${expected_inode}" "${expected_hash}" "stage ${ordinal} attempt"
  expect_kv "${path}" schema_id "${source_schema_id}.development_stage_attempt.v1"
  expect_kv "${path}" status consumed
  expect_kv "${path}" stage_ordinal "${ordinal}"
  expect_kv "${path}" stage_name "${name}"
  expect_kv "${path}" previous_stage_completion_path "${previous}"
  expect_kv "${path}" previous_stage_completion_sha256 "${previous_hash}"
  expect_kv "${path}" operational_ablation_runner_path "${runner_path}"
  expect_kv "${path}" operational_ablation_runner_sha256 "${expected_runner_sha256}"
  expect_kv "${path}" canonical_data_raw_access false
  expect_kv "${path}" certified_input_access false
  expect_kv "${path}" final_holdout_access false
  expect_kv "${path}" policy_access false
}

verify_stage_completion() {
  local path="$1" ordinal="$2" name="$3" previous="$4" previous_hash="$5"
  local attempt="$6" attempt_hash="$7" expected_hash="$8" expected_inode="$9"
  local expected_bytes
  case "${ordinal}" in
  00) expected_bytes=7837 ;;
  01) expected_bytes=7258 ;;
  02) expected_bytes=7616 ;;
  03) expected_bytes=7521 ;;
  *) fail "unknown exact completion-byte contract for stage ${ordinal}" ;;
  esac
  verify_file_exact "${path}" 444 "${expected_bytes}" \
    "${expected_inode}" "${expected_hash}" "stage ${ordinal} completion"
  expect_kv "${path}" schema_id "${source_schema_id}.development_stage_completion.v1"
  expect_kv "${path}" status complete
  expect_kv "${path}" stage_ordinal "${ordinal}"
  expect_kv "${path}" stage_name "${name}"
  expect_kv "${path}" previous_stage_completion_path "${previous}"
  expect_kv "${path}" previous_stage_completion_sha256 "${previous_hash}"
  expect_kv "${path}" stage_attempt_path "${attempt}"
  expect_kv "${path}" stage_attempt_sha256 "${attempt_hash}"
  expect_kv "${path}" certified_input_access false
  expect_kv "${path}" final_holdout_access false
  expect_kv "${path}" policy_access false
  verify_receipt_bound_artifact "${path}" primary_artifact_path \
    primary_artifact_sha256 "stage ${ordinal} primary artifact"
}

verify_completed_prefix() {
  verify_stage_attempt "${stage00_attempt}" 00 initialize none none \
    "${expected_stage00_attempt_sha256}" "${expected_stage00_attempt_inode}"
  verify_stage_completion "${stage00_completion}" 00 initialize none none \
    "${stage00_attempt}" "${expected_stage00_attempt_sha256}" \
    "${expected_stage00_completion_sha256}" "${expected_stage00_completion_inode}"

  verify_stage_attempt "${stage01_attempt}" 01 canonical_import \
    "${stage00_completion}" "${expected_stage00_completion_sha256}" \
    "${expected_stage01_attempt_sha256}" "${expected_stage01_attempt_inode}"
  verify_stage_completion "${stage01_completion}" 01 canonical_import \
    "${stage00_completion}" "${expected_stage00_completion_sha256}" \
    "${stage01_attempt}" "${expected_stage01_attempt_sha256}" \
    "${expected_stage01_completion_sha256}" "${expected_stage01_completion_inode}"

  verify_stage_attempt "${stage02_attempt}" 02 endpoint_import \
    "${stage01_completion}" "${expected_stage01_completion_sha256}" \
    "${expected_stage02_attempt_sha256}" "${expected_stage02_attempt_inode}"
  verify_stage_completion "${stage02_completion}" 02 endpoint_import \
    "${stage01_completion}" "${expected_stage01_completion_sha256}" \
    "${stage02_attempt}" "${expected_stage02_attempt_sha256}" \
    "${expected_stage02_completion_sha256}" "${expected_stage02_completion_inode}"

  verify_stage_attempt "${stage03_attempt}" 03 time_only_training \
    "${stage02_completion}" "${expected_stage02_completion_sha256}" \
    "${expected_stage03_attempt_sha256}" "${expected_stage03_attempt_inode}"
  verify_stage_completion "${stage03_completion}" 03 time_only_training \
    "${stage02_completion}" "${expected_stage02_completion_sha256}" \
    "${stage03_attempt}" "${expected_stage03_attempt_sha256}" \
    "${expected_stage03_completion_sha256}" "${expected_stage03_completion_inode}"
  verify_receipt_bound_artifact "${stage03_completion}" training_log_path \
    training_log_sha256 "stage 03 training log"
}

verify_partial_file_identities() {
  verify_file_exact "${no_tf_config}/capture.config" 444 5606 \
    6473924465214169 "${expected_no_tf_capture_config_sha256}" "no-TF capture config"
  verify_file_exact "${no_tf_config}/representation.jkimyei" 444 2119 \
    8162774325478096 "${expected_no_tf_policy_sha256}" "no-TF training policy"
  verify_file_exact "${no_tf_config}/representation.net" 444 603 \
    6755399441924818 "${expected_no_tf_net_sha256}" "no-TF network config"
  verify_file_exact "${no_tf_config}/train.config" 444 5603 \
    8162774325478102 "${expected_no_tf_train_config_sha256}" "no-TF train config"
  verify_file_exact "${no_tf_log}" 600 11017 121878664915808574 \
    "${expected_no_tf_log_sha256}" "no-TF partial training log"
  verify_file_exact "${no_tf_spawn}" 600 435 7318349394754577 \
    "${expected_no_tf_spawn_sha256}" "no-TF component spawn"
  verify_file_exact "${no_tf_report}" 600 6801 11540474045414494 \
    "${expected_no_tf_report_sha256}" "no-TF partial report"
  verify_file_exact "${no_tf_checkpoint}" 600 853931 34621422135688421 \
    "${expected_no_tf_checkpoint_sha256}" "no-TF partial checkpoint"
  verify_file_exact "${no_tf_manifest}" 600 7459 6192449487911963 \
    "${expected_no_tf_manifest_sha256}" "no-TF job manifest"
  verify_file_exact "${no_tf_events}" 600 22639168 6473924464622683 \
    "${expected_no_tf_events_sha256}" "no-TF event probe"
  verify_file_exact "${no_tf_registry}" 600 779 17732923533048831 \
    "${expected_no_tf_registry_sha256}" "no-TF component registry"
  verify_file_exact "${no_tf_layout}" 600 401 10977524091919287 \
    "${expected_no_tf_layout_sha256}" "no-TF runtime layout"

  verify_dir_exact "${no_tf_root}" 9288674232320718 "no-TF arm root"
  verify_dir_exact "${no_tf_config}" 8444249302188751 "no-TF config root"
  verify_dir_exact "${no_tf_training}" 41376821576506000 "no-TF training root"
  verify_dir_exact "${no_tf_training}/system" 23362423067080192 "no-TF system root"
  verify_dir_exact "${no_tf_job}" 39125021763058676 "no-TF job root"
  verify_dir_exact "${no_tf_training}/components" 7599824371465217 "no-TF components root"
  verify_dir_exact "${no_tf_training}/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg" \
    8444249301597187 "no-TF component root"
  verify_dir_exact "${no_tf_training}/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns" \
    7881299348175880 "no-TF spawns root"
  verify_dir_exact "${no_tf_training}/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/zts_ecf8b8d26bc21d88" \
    6755399441333258 "no-TF spawn root"
}

verify_stage04_terminal_attempt() {
  verify_stage_attempt "${stage04_attempt}" 04 no_tf_alignment_training \
    "${stage03_completion}" "${expected_stage03_completion_sha256}" \
    "${expected_stage04_attempt_sha256}" "${expected_stage04_attempt_inode}"
  expect_kv "${stage04_attempt}" attempt_without_completion_terminal true
  expect_kv "${stage04_attempt}" partial_payload_adoption_authorized false
  expect_kv "${stage04_attempt}" checkpoint_resume_authorized false
  assert_absent "${stage04_completion}" "stage04 completion"

  local ordinal tag name
  for ordinal in 5 6 7 8 9 10 11; do
    printf -v tag '%02d' "${ordinal}"
    case "${ordinal}" in
    5) name=endpoint_scale_capture ;;
    6) name=time_only_capture ;;
    7) name=no_tf_alignment_capture ;;
    8) name=endpoint_scale_affine ;;
    9) name=time_only_affine ;;
    10) name=no_tf_alignment_affine ;;
    11) name=selection_and_development ;;
    esac
    assert_absent "${source_runtime}/stage.${tag}.${name}.attempt.status" \
      "later stage attempt"
    assert_absent "${source_runtime}/stage.${tag}.${name}.status" \
      "later stage completion"
  done

  verify_partial_file_identities
  [[ "$(grep -Fxc -- '  MAX_STEPS = 3000;' \
    "${no_tf_config}/representation.jkimyei")" == 1 ]] ||
    fail "no-TF requested step contract drifted"
  [[ "$(grep -Fxc -- '  LAMBDA_TF_ALIGN = 0.00;' \
    "${no_tf_config}/representation.jkimyei")" == 1 ]] ||
    fail "no-TF alignment ablation contract drifted"

  expect_kv "${no_tf_report}" steps_attempted "${observed_optimizer_steps}"
  expect_kv "${no_tf_report}" steps_completed "${observed_optimizer_steps}"
  expect_kv "${no_tf_report}" optimizer_steps "${observed_optimizer_steps}"
  expect_kv "${no_tf_report}" skipped_batches 0
  expect_kv "${no_tf_report}" finite_parameter_check true
  expect_kv "${no_tf_report}" gradients_finite true
  expect_kv "${no_tf_report}" nonfinite_output_count 0
  expect_kv "${no_tf_report}" checkpoint_written true
  expect_kv "${no_tf_report}" last_checkpoint_optimizer_step \
    "${observed_checkpoint_step}"
  expect_kv "${no_tf_report}" checkpoint_path "${no_tf_checkpoint}"
  expect_kv "${no_tf_report}" checkpoint_artifact_status verified
  expect_kv "${no_tf_report}" last_report_attempted_step \
    "${observed_optimizer_steps}"

  expect_kv "${no_tf_manifest}" config_path "${no_tf_config}/train.config"
  expect_kv "${no_tf_manifest}" job_attempt_policy explicit_job_dir_no_overwrite_v1
  expect_kv "${no_tf_manifest}" wave_action train
  expect_kv "${no_tf_manifest}" source_range_policy anchor_index
  expect_kv "${no_tf_manifest}" resolved_anchor_index_begin 0
  expect_kv "${no_tf_manifest}" resolved_anchor_index_end 2496
  expect_kv "${no_tf_manifest}" input_representation_checkpoint_path ''
  expect_kv "${no_tf_manifest}" input_mdn_checkpoint_path ''
  [[ "$(kv "${no_tf_manifest}" execution_chain)" != *policy* ]] ||
    fail "no-TF partial execution reached policy"
  [[ "$(kv "${no_tf_manifest}" source_file_receipts)" != *'/data/raw/'* ]] ||
    fail "no-TF partial attempt reached canonical data/raw"

  local event_count_file sequence_file event_time_file final_step_file
  local maximum_step_file severity_count_file observed_value log_scan_status
  capture_command_output event_count_file "event record count" awk '
    /^record_schema=/ { count += 1 }
    END { print count + 0 }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${event_count_file}" ||
    fail "no-TF event count output is missing"
  [[ "${observed_value}" == "${observed_event_records}" ]] ||
    fail "no-TF event count drifted"

  capture_command_output sequence_file "final event sequence" awk '
    /^sequence=/ { value = $0; found = 1 }
    END { if (!found) exit 41; print value }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${sequence_file}" ||
    fail "no-TF final sequence output is missing"
  [[ "${observed_value}" == "sequence=${observed_event_records}" ]] ||
    fail "no-TF final sequence drifted"

  capture_command_output event_time_file "final event timestamp" awk '
    /^event_time_unix_ms=/ { value = $0; found = 1 }
    END { if (!found) exit 41; print value }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${event_time_file}" ||
    fail "no-TF final event timestamp output is missing"
  [[ "${observed_value}" == \
    "event_time_unix_ms=${observed_last_event_unix_ms}" ]] ||
    fail "no-TF final event timestamp drifted"

  capture_command_output final_step_file "final event step" awk '
    /^step=/ { value = $0; found = 1 }
    END { if (!found) exit 41; print value }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${final_step_file}" ||
    fail "no-TF final event step output is missing"
  [[ "${observed_value}" == "step=${observed_optimizer_steps}" ]] ||
    fail "no-TF final event step drifted"

  capture_command_output maximum_step_file "maximum event step" awk '
    /^step=/ {
      raw = substr($0, 6)
      if (raw !~ /^[0-9]+$/) invalid = 1
      value = raw + 0
      if (!found || value > maximum) maximum = value
      found = 1
    }
    END {
      if (invalid) exit 42
      if (!found) exit 41
      print maximum
    }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${maximum_step_file}" ||
    fail "no-TF maximum event step output is missing"
  [[ "${observed_value}" == "${observed_optimizer_steps}" ]] ||
    fail "no-TF maximum event step drifted"

  capture_command_output severity_count_file "event severity count" awk '
    /^severity=info$/ { count += 1 }
    END { print count + 0 }
  ' "${no_tf_events}"
  IFS= read -r observed_value <"${severity_count_file}" ||
    fail "no-TF event severity output is missing"
  [[ "${observed_value}" == "${observed_event_records}" ]] ||
    fail "no-TF event severity drifted"

  if grep -aEqi 'error|fatal|exception|signal|terminat|killed|failed' \
    "${no_tf_log}"; then
    fail "no-TF log now contains a failure token"
  else
    log_scan_status=$?
    [[ "${log_scan_status}" == 1 ]] ||
      fail "no-TF log failure-token scan failed (grep status ${log_scan_status})"
  fi

  local path
  for path in "${no_tf_root}/training.status" \
    "${no_tf_job}/runtime.result.fact" "${no_tf_job}/job.state" \
    "${no_tf_job}/runtime.component_training_update.fact" \
    "${no_tf_job}/runtime.health_measurement.fact" \
    "${no_tf_job}/runtime.checkpoint_io.fact" \
    "${no_tf_job}/lattice.checkpoint.fact" \
    "${no_tf_job}/lattice.exposure.fact" \
    "${no_tf_job}/lattice.source_analytics.fact"; do
    assert_absent "${path}" "stage04 finalization artifact"
  done
}

verify_downstream_absence() {
  local path arm
  for path in "${source_runtime}/selection.status" \
    "${source_runtime}/development.status" \
    "${source_runtime}/certified.attempt.status" \
    "${source_runtime}/certified" "${source_runtime}/result.status"; do
    assert_absent "${path}" "Retry2 downstream artifact"
  done
  for arm in endpoint_scale time_only no_tf_alignment; do
    for path in "${source_runtime}/arms/${arm}/capture" \
      "${source_runtime}/arms/${arm}/capture.status" \
      "${source_runtime}/arms/${arm}/affine.status"; do
      assert_absent "${path}" "Retry2 challenger downstream artifact"
    done
  done
}

assert_sensitive_outputs_absent_before_tree_read() {
  local path
  for path in "${source_runtime}/.certified.lock" \
    "${source_runtime}/certified.attempt.status" \
    "${source_runtime}/certified" \
    "${source_runtime}/result.status" \
    "${source_runtime}/final" \
    "${source_runtime}/selection.status" \
    "${source_runtime}/development.status"; do
    assert_absent "${path}" "pre-read downstream or sensitive artifact"
  done
}

verify_no_oom_evidence() {
  require_file /sys/fs/cgroup/memory.events
  [[ "$(awk '$1 == "oom" { print $2 }' /sys/fs/cgroup/memory.events)" == 0 ]] ||
    fail "container cgroup records an OOM event"
  [[ "$(awk '$1 == "oom_kill" { print $2 }' /sys/fs/cgroup/memory.events)" == 0 ]] ||
    fail "container cgroup records an OOM kill"
  [[ "$(awk '$1 == "oom_group_kill" { print $2 }' /sys/fs/cgroup/memory.events)" == 0 ]] ||
    fail "container cgroup records an OOM group kill"
}

assert_no_live_references() {
  assert_source_lock_identity
  local protected_identities protected_entries unsorted_identities entry identity
  local protected_map_inodes unsorted_map_inodes
  new_temp_file protected_identities \
    "/tmp/${closure_schema_id}.protected-identities.XXXXXX"
  new_temp_file unsorted_identities \
    "/tmp/${closure_schema_id}.protected-identities.unsorted.XXXXXX"
  new_temp_file protected_map_inodes \
    "/tmp/${closure_schema_id}.protected-map-inodes.XXXXXX"
  new_temp_file unsorted_map_inodes \
    "/tmp/${closure_schema_id}.protected-map-inodes.unsorted.XXXXXX"
  capture_find0 protected_entries "${source_runtime}" -mindepth 0
  {
    stat -c '%d:%i' -- "${runner_path}" ||
      fail "cannot capture protected runner identity"
    while IFS= read -r -d '' entry; do
      stat -c '%d:%i' -- "${entry}" ||
        fail "cannot capture protected Retry2 entry identity: ${entry}"
    done <"${protected_entries}"
  } >"${unsorted_identities}"
  LC_ALL=C sort -u -- "${unsorted_identities}" >"${protected_identities}" ||
    fail "cannot sort protected Retry2 identities"
  {
    stat -c '%i' -- "${runner_path}" ||
      fail "cannot capture protected runner mapping inode"
    while IFS= read -r -d '' entry; do
      stat -c '%i' -- "${entry}" ||
        fail "cannot capture protected Retry2 mapping inode: ${entry}"
    done <"${protected_entries}"
  } >"${unsorted_map_inodes}"
  LC_ALL=C sort -u -- "${unsorted_map_inodes}" >"${protected_map_inodes}" ||
    fail "cannot sort protected Retry2 mapping inodes"
  local proc pid ref fd target normalized_target fd_entries map_entries
  for proc in /proc/[0-9]*; do
    pid="${proc#/proc/}"
    [[ -d "${proc}" ]] || continue
    if proc_file_contains_literal "${proc}" "${proc}/cmdline" \
      "${source_runtime}" cmdline ||
      proc_file_contains_literal "${proc}" "${proc}/cmdline" \
        "${runner_path}" cmdline; then
      [[ "${pid}" == "$$" ]] ||
        fail "live process command references Retry2 runtime or runner: pid ${pid}"
    fi

    if ! capture_proc_children0 fd_entries "${proc}" fd; then
      continue
    fi
    while IFS= read -r -d '' ref; do
      fd="${ref##*/}"
      if ! read_proc_reference target identity "${proc}" "${ref}"; then
        continue
      fi
      if [[ -n "${identity}" ]] &&
        grep -Fxq -- "${identity}" "${protected_identities}"; then
        if [[ "${pid}" == "$$" && "${fd}" == "${runner_lock_fd}" &&
          "${target}" == "${runner_path}" ]]; then
          continue
        fi
        if [[ "${pid}" == "$$" && "${fd}" == "${source_lock_fd}" &&
          "${target}" == "${source_lock_path}" ]]; then
          continue
        fi
        fail "live file descriptor references Retry2 runtime or runner: ${pid}:${fd}:${target}"
      fi
    done <"${fd_entries}"

    for ref in "${proc}/cwd" "${proc}/root" "${proc}/exe"; do
      if ! read_proc_reference target identity "${proc}" "${ref}"; then
        continue
      fi
      if [[ -n "${identity}" ]] &&
        grep -Fxq -- "${identity}" "${protected_identities}"; then
        fail "live process path reference reaches Retry2 runtime or runner: ${pid}:${ref##*/}"
      fi
    done

    if ! capture_proc_children0 map_entries "${proc}" map_files; then
      continue
    fi
    while IFS= read -r -d '' ref; do
      if ! read_proc_link_target target "${proc}" "${ref}"; then
        continue
      fi
      normalized_target="${target% (deleted)}"
      if [[ "${normalized_target}" == "${runner_path}" ||
        "${normalized_target}" == "${source_runtime}" ||
        "${normalized_target}" == "${source_runtime}/"* ]]; then
        fail "live process mapping path reaches Retry2 runtime or runner: ${pid}:${ref##*/}:${target}"
      fi
    done <"${map_entries}"

    if proc_file_contains_literal "${proc}" "${proc}/maps" \
      "${source_runtime}" maps ||
      proc_file_contains_literal "${proc}" "${proc}/maps" \
        "${runner_path}" maps; then
      fail "live process mapping reaches Retry2 runtime or runner: pid ${pid}"
    fi
    assert_proc_maps_no_protected_inode "${proc}" \
      "${protected_map_inodes}" "Retry2 runtime or runner"
  done
  assert_source_lock_identity
}

assert_tree_has_no_live_references() {
  local root="$1" label="$2" protected_identities protected_entries
  local unsorted_identities protected_map_inodes unsorted_map_inodes entry
  reject_symlink_components "${root}"
  require_dir "${root}"
  new_temp_file protected_identities \
    "/tmp/${closure_schema_id}.publication-identities.XXXXXX"
  new_temp_file unsorted_identities \
    "/tmp/${closure_schema_id}.publication-identities.unsorted.XXXXXX"
  new_temp_file protected_map_inodes \
    "/tmp/${closure_schema_id}.publication-map-inodes.XXXXXX"
  new_temp_file unsorted_map_inodes \
    "/tmp/${closure_schema_id}.publication-map-inodes.unsorted.XXXXXX"
  capture_find0 protected_entries "${root}" -mindepth 0
  while IFS= read -r -d '' entry; do
    stat -c '%d:%i' -- "${entry}" ||
      fail "cannot capture ${label} entry identity: ${entry}"
  done <"${protected_entries}" >"${unsorted_identities}"
  LC_ALL=C sort -u -- "${unsorted_identities}" >"${protected_identities}" ||
    fail "cannot sort ${label} identities"
  while IFS= read -r -d '' entry; do
    stat -c '%i' -- "${entry}" ||
      fail "cannot capture ${label} mapping inode: ${entry}"
  done <"${protected_entries}" >"${unsorted_map_inodes}"
  LC_ALL=C sort -u -- "${unsorted_map_inodes}" >"${protected_map_inodes}" ||
    fail "cannot sort ${label} mapping inodes"
  local proc pid ref target normalized_target identity fd_entries map_entries
  for proc in /proc/[0-9]*; do
    pid="${proc#/proc/}"
    [[ -d "${proc}" ]] || continue
    proc_file_contains_literal "${proc}" "${proc}/cmdline" \
      "${root}" cmdline &&
      fail "live process command references ${label}: pid ${pid}"

    if ! capture_proc_children0 fd_entries "${proc}" fd; then
      continue
    fi
    while IFS= read -r -d '' ref; do
      if ! read_proc_reference target identity "${proc}" "${ref}"; then
        continue
      fi
      if [[ -n "${identity}" ]] &&
        grep -Fxq -- "${identity}" "${protected_identities}"; then
        fail "live process reference reaches ${label}: ${pid}:${ref#${proc}/}"
      fi
    done <"${fd_entries}"

    for ref in "${proc}/cwd" "${proc}/root" "${proc}/exe"; do
      if ! read_proc_reference target identity "${proc}" "${ref}"; then
        continue
      fi
      if [[ -n "${identity}" ]] &&
        grep -Fxq -- "${identity}" "${protected_identities}"; then
        fail "live process reference reaches ${label}: ${pid}:${ref#${proc}/}"
      fi
    done

    if ! capture_proc_children0 map_entries "${proc}" map_files; then
      continue
    fi
    while IFS= read -r -d '' ref; do
      if ! read_proc_link_target target "${proc}" "${ref}"; then
        continue
      fi
      normalized_target="${target% (deleted)}"
      if [[ "${normalized_target}" == "${root}" ||
        "${normalized_target}" == "${root}/"* ]]; then
        fail "live process mapping path reaches ${label}: ${pid}:${ref##*/}:${target}"
      fi
    done <"${map_entries}"

    if proc_file_contains_literal "${proc}" "${proc}/maps" \
      "${root}" maps; then
      fail "live process mapping reaches ${label}: pid ${pid}"
    fi
    assert_proc_maps_no_protected_inode "${proc}" \
      "${protected_map_inodes}" "${label}"
  done
}

verify_scientific_boundary() {
  assert_no_live_references
  verify_external_authorities
  verify_completed_prefix
  verify_stage04_terminal_attempt
  verify_downstream_absence
  verify_no_oom_evidence
  verify_source_tree_shape_and_modes
  assert_no_live_references
}

copy_and_freeze_source_snapshot() {
  local snapshot_root="$1"
  assert_sensitive_outputs_absent_before_tree_read
  require_dir "$(dirname "${snapshot_root}")"
  assert_absent "${snapshot_root}" "candidate source snapshot"
  mkdir -- "${snapshot_root}"
  cp -a --reflink=never -- "${source_runtime}/." "${snapshot_root}/"
  find "${snapshot_root}" -xdev -type f -exec chmod 0444 -- {} +
  find "${snapshot_root}" -xdev -depth -type d -exec chmod 0555 -- {} +
}

emit_paired_regular_inventory() {
  local snapshot_root="$1" source path rel snapshot source_entries
  local source_inode snapshot_inode source_hash snapshot_hash
  assert_sensitive_outputs_absent_before_tree_read
  printf 'relative_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_mtime\tsource_sha256\tsnapshot_mode\tsnapshot_uid\tsnapshot_gid\tsnapshot_links\tsnapshot_bytes\tsnapshot_inode\tsnapshot_device\tsnapshot_mtime\tsnapshot_sha256\tinodes_distinct\tbytes_identical\n'
  capture_sorted_find0 source_entries "${source_runtime}" -type f
  while IFS= read -r -d '' source; do
    rel="$(relative_path_at "${source_runtime}" "${source}")"
    snapshot="${snapshot_root}/${rel}"
    reject_symlink_components "${snapshot}"
    require_file "${snapshot}"
    source_inode="$(stat -c '%i' -- "${source}")"
    snapshot_inode="$(stat -c '%i' -- "${snapshot}")"
    source_hash="$(sha256_of "${source}")"
    snapshot_hash="$(sha256_of "${snapshot}")"
    [[ "${source_inode}" != "${snapshot_inode}" ]] ||
      fail "snapshot file reuses source inode: ${rel}"
    [[ "$(stat -c '%h' -- "${source}")" == 1 &&
      "$(stat -c '%h' -- "${snapshot}")" == 1 ]] ||
      fail "source/snapshot file has non-unit link count: ${rel}"
    [[ "$(stat -c '%s' -- "${source}")" == "$(stat -c '%s' -- "${snapshot}")" &&
      "${source_hash}" == "${snapshot_hash}" ]] ||
      fail "source/snapshot bytes differ: ${rel}"
    [[ "$(stat -c '%a:%u:%g:%d' -- "${snapshot}")" == \
      "444:0:0:${expected_device}" ]] ||
      fail "snapshot regular-file metadata drifted: ${rel}"
    printf '%s\t' "${rel}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\t' -- "${source}"
    printf '%s\t' "${source_hash}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\t' -- "${snapshot}"
    printf '%s\ttrue\ttrue\n' "${snapshot_hash}"
  done <"${source_entries}"
}

emit_paired_directory_inventory() {
  local snapshot_root="$1" source rel snapshot source_entries
  assert_sensitive_outputs_absent_before_tree_read
  printf 'relative_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_mtime\tsnapshot_mode\tsnapshot_uid\tsnapshot_gid\tsnapshot_links\tsnapshot_bytes\tsnapshot_inode\tsnapshot_device\tsnapshot_mtime\tinodes_distinct\n'
  capture_sorted_find0 source_entries "${source_runtime}" -type d
  while IFS= read -r -d '' source; do
    rel="$(relative_path_at "${source_runtime}" "${source}")"
    if [[ "${rel}" == . ]]; then snapshot="${snapshot_root}"; else
      snapshot="${snapshot_root}/${rel}"
    fi
    reject_symlink_components "${snapshot}"
    require_dir "${snapshot}"
    [[ "$(stat -c '%i' -- "${source}")" != "$(stat -c '%i' -- "${snapshot}")" ]] ||
      fail "snapshot directory reuses source inode: ${rel}"
    [[ "$(stat -c '%a:%u:%g:%d' -- "${snapshot}")" == \
      "555:0:0:${expected_device}" ]] ||
      fail "snapshot directory metadata drifted: ${rel}"
    printf '%s\t' "${rel}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\t' -- "${source}"
    stat --printf '%a\t%u\t%g\t%h\t%s\t%i\t%d\t%y\ttrue\n' -- "${snapshot}"
  done <"${source_entries}"
}

verify_snapshot_copy_at() {
  local snapshot_root="$1" regular_inventory="$2" directory_inventory="$3"
  require_dir "${snapshot_root}"
  local entry file_count=0 dir_count=0 byte_count=0 entries content_digest
  capture_find0 entries "${snapshot_root}" -mindepth 0
  while IFS= read -r -d '' entry; do
    [[ ! -L "${entry}" ]] || fail "source snapshot contains symlink: ${entry}"
    [[ "$(stat -c '%d:%u:%g' -- "${entry}")" == \
      "${expected_device}:0:0" ]] || fail "source snapshot metadata drifted: ${entry}"
    if [[ -f "${entry}" ]]; then
      ((file_count += 1))
      byte_count=$((byte_count + $(stat -c '%s' -- "${entry}")))
      [[ "$(stat -c '%a:%h' -- "${entry}")" == 444:1 ]] ||
        fail "source snapshot file is not immutable link1: ${entry}"
    elif [[ -d "${entry}" ]]; then
      ((dir_count += 1))
      [[ "$(stat -c '%a' -- "${entry}")" == 555 ]] ||
        fail "source snapshot directory is not immutable: ${entry}"
    else
      fail "source snapshot contains special entry: ${entry}"
    fi
  done <"${entries}"
  [[ "${file_count}" == "${expected_regular_file_count}" &&
    "${dir_count}" == "${expected_directory_count}" &&
    "${byte_count}" == "${expected_regular_file_bytes}" ]] ||
    fail "source snapshot count or byte aggregate drifted"
  compute_content_inventory_sha256_at "${snapshot_root}" content_digest
  [[ "${content_digest}" == "${expected_content_inventory_sha256}" ]] ||
    fail "source snapshot relative content inventory drifted"

  local expected_regular expected_directories
  new_temp_file expected_regular "/tmp/${closure_schema_id}.paired.regular.XXXXXX"
  new_temp_file expected_directories "/tmp/${closure_schema_id}.paired.directories.XXXXXX"
  emit_paired_regular_inventory "${snapshot_root}" >"${expected_regular}"
  emit_paired_directory_inventory "${snapshot_root}" >"${expected_directories}"
  cmp -s -- "${expected_regular}" "${regular_inventory}" ||
    fail "paired regular-file inventory drifted"
  cmp -s -- "${expected_directories}" "${directory_inventory}" ||
    fail "paired directory inventory drifted"
}

emit_receipt() {
  local destination="$1" regular_inventory="$2" directory_inventory="$3"
  local snapshot_root="$4" frozen_sealer="$5" frozen_amendment="$6"
  local source_regular_snapshot_sha="$7" source_directory_snapshot_sha="$8"
  cat >"${destination}" <<RECEIPT
schema_id=${closure_schema_id}
status=complete
closure_kind=retry2_stage04_external_operational_interruption
termination_classification=external_host_or_controller_termination_supported_not_actor_attributed
actor_attributed=false
cause_attributed=false
host_controller_status_decimal=${observed_host_exit_decimal}
host_controller_status_hex=${observed_host_exit_hex}
host_controller_status_symbol=${observed_host_exit_symbol}
host_controller_status_provenance=operator_observation_outside_linux_runtime
linux_child_exit_status_claim=none
container_name_required=unnamed_taoist
container_hostname_pinned=${expected_container_hostname}
closure_root=${closure_root}
closure_receipt_path=${receipt_path}
publication_method=atomic_no_clobber_sibling_directory_rename
source_schema_id=${source_schema_id}
source_runtime_root=${source_runtime}
source_runtime_device=${expected_device}
source_runtime_inode=${expected_runtime_inode}
source_runtime_mutated=false
source_runtime_receipt_published_in_place=false
old_runner_path=${runner_path}
old_runner_sha256=${expected_runner_sha256}
old_runner_device=${expected_device}
old_runner_inode=${expected_runner_inode}
old_runner_lock_acquired_first=true
old_runner_lock_open_mode=read_only
source_lock_path=${source_lock_path}
source_lock_device=${expected_device}
source_lock_inode=${expected_lock_inode}
source_lock_acquired_second=true
source_lock_open_mode=read_only
locks_held_through_copy_and_publication=true
live_process_reference_count=0
live_fd_reference_count_excluding_own_locks=0
live_cwd_root_exe_reference_count=0
live_mapping_reference_count=0
cgroup_oom_count=0
cgroup_oom_kill_count=0
source_snapshot_count=2
source_snapshots_identical=true
source_snapshot_1_regular_inventory_sha256=${source_regular_snapshot_sha}
source_snapshot_2_regular_inventory_sha256=${source_regular_snapshot_sha}
source_snapshot_1_directory_inventory_sha256=${source_directory_snapshot_sha}
source_snapshot_2_directory_inventory_sha256=${source_directory_snapshot_sha}
source_regular_file_count=${expected_regular_file_count}
source_directory_count=${expected_directory_count}
source_entry_count_below_root=${expected_entry_count_below_root}
source_regular_file_bytes=${expected_regular_file_bytes}
source_content_inventory_sha256=${expected_content_inventory_sha256}
source_regular_file_inventory_path=${regular_inventory_path}
source_regular_file_inventory_sha256=$(sha256_of "${regular_inventory}")
source_directory_inventory_path=${directory_inventory_path}
source_directory_inventory_sha256=$(sha256_of "${directory_inventory}")
source_snapshot_path=${source_snapshot_final}
source_snapshot_copy_method=cp_archive_reflink_never
source_snapshot_device=$(stat -c '%d' -- "${snapshot_root}")
source_snapshot_inode=$(stat -c '%i' -- "${snapshot_root}")
source_snapshot_content_inventory_sha256=${expected_content_inventory_sha256}
source_snapshot_regular_file_count=${expected_regular_file_count}
source_snapshot_directory_count=${expected_directory_count}
source_snapshot_regular_file_bytes=${expected_regular_file_bytes}
source_snapshot_regular_files_byte_identical=true
source_snapshot_regular_files_distinct_inodes=true
source_snapshot_regular_files_single_link=true
source_snapshot_regular_files_mode=0444
source_snapshot_directories_mode=0555
source_snapshot_symlink_count=0
source_snapshot_special_entry_count=0
interruption_sealer_path=${script_path}
interruption_sealer_process_start_sha256=${process_start_sealer_sha256}
interruption_sealer_process_start_mode=0${process_start_sealer_mode}
interruption_sealer_process_start_links=${process_start_sealer_links}
interruption_sealer_process_start_owner_uid=${process_start_sealer_uid}
interruption_sealer_process_start_bytes=${process_start_sealer_bytes}
interruption_sealer_process_start_inode=${process_start_sealer_inode}
interruption_sealer_process_start_device=${process_start_sealer_device}
frozen_interruption_sealer_path=${frozen_sealer_path}
frozen_interruption_sealer_sha256=$(sha256_of "${frozen_sealer}")
interruption_amendment_path=${amendment_path}
interruption_amendment_sha256=${expected_amendment_sha256}
frozen_interruption_amendment_path=${frozen_amendment_path}
frozen_interruption_amendment_sha256=$(sha256_of "${frozen_amendment}")
stage_00_attempt_path=${stage00_attempt}
stage_00_attempt_sha256=${expected_stage00_attempt_sha256}
stage_00_completion_path=${stage00_completion}
stage_00_completion_sha256=${expected_stage00_completion_sha256}
stage_01_attempt_path=${stage01_attempt}
stage_01_attempt_sha256=${expected_stage01_attempt_sha256}
stage_01_completion_path=${stage01_completion}
stage_01_completion_sha256=${expected_stage01_completion_sha256}
stage_02_attempt_path=${stage02_attempt}
stage_02_attempt_sha256=${expected_stage02_attempt_sha256}
stage_02_completion_path=${stage02_completion}
stage_02_completion_sha256=${expected_stage02_completion_sha256}
stage_03_attempt_path=${stage03_attempt}
stage_03_attempt_sha256=${expected_stage03_attempt_sha256}
stage_03_completion_path=${stage03_completion}
stage_03_completion_sha256=${expected_stage03_completion_sha256}
completed_stage_prefix=00,01,02,03
stage_04_attempt_path=${stage04_attempt}
stage_04_attempt_sha256=${expected_stage04_attempt_sha256}
stage_04_attempt_status=consumed
stage_04_scientific_attempt_consumed=true
stage_04_completion_present=false
stage_04_attempt_without_completion_terminal=true
stage_04_requested_optimizer_steps=${expected_optimizer_steps}
stage_04_steps_attempted_observed=${observed_optimizer_steps}
stage_04_steps_completed_observed=${observed_optimizer_steps}
stage_04_optimizer_steps_observed=${observed_optimizer_steps}
stage_04_last_checkpoint_optimizer_step=${observed_checkpoint_step}
stage_04_event_record_count=${observed_event_records}
stage_04_last_event_unix_ms=${observed_last_event_unix_ms}
stage_04_parameters_finite=true
stage_04_gradients_finite=true
stage_04_nonfinite_output_count=0
stage_04_training_status_present=false
stage_04_runtime_result_present=false
stage_04_partial_checkpoint_path=${no_tf_checkpoint}
stage_04_partial_checkpoint_sha256=${expected_no_tf_checkpoint_sha256}
stage_05_or_later_marker_present=false
partial_payload_adoption_authorized=false
checkpoint_resume_authorized=false
same_runtime_reentry_authorized=false
completed_prefix_import_authorized_by_this_closure=false
recovery_requires_new_schema=true
no_tf_alignment_restart_optimizer_step=0
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
policy_access=false
RECEIPT
}

verify_receipt_key_shape() {
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || seen[key]++) exit 42;
    }
  ' "$1" || fail "closure receipt has malformed or duplicate key"
}

verify_receipt_exact() {
  local receipt="$1" regular="$2" directories="$3" snapshot="$4"
  local frozen_sealer="$5" frozen_amendment="$6"
  local source_regular_snapshot_sha="$7" source_directory_snapshot_sha="$8"
  require_file "${receipt}"
  [[ "$(stat -c '%a:%u:%h' -- "${receipt}")" == 444:0:1 ]] ||
    fail "closure receipt metadata drifted"
  local expected
  new_temp_file expected "/tmp/${closure_schema_id}.receipt.expected.XXXXXX"
  emit_receipt "${expected}" "${regular}" "${directories}" "${snapshot}" \
    "${frozen_sealer}" "${frozen_amendment}" \
    "${source_regular_snapshot_sha}" "${source_directory_snapshot_sha}"
  verify_receipt_key_shape "${expected}"
  verify_receipt_key_shape "${receipt}"
  cmp -s -- "${expected}" "${receipt}" || fail "closure receipt content drifted"
}

verify_closure_tree_at() {
  local root="$1" source_regular_snapshot_sha="$2"
  local source_directory_snapshot_sha="$3"
  local regular="${root}/source_regular_files.inventory.tsv"
  local directories="${root}/source_directories.inventory.tsv"
  local snapshot="${root}/source_snapshot"
  local frozen="${root}/frozen_sources"
  local frozen_sealer="${frozen}/$(basename "${script_path}")"
  local frozen_amendment="${frozen}/$(basename "${amendment_path}")"
  local receipt="${root}/interruption_closure.status"

  reject_symlink_components "${root}"
  require_dir "${root}"
  require_dir "${frozen}"
  require_dir "${snapshot}"
  local path top_entries expected_top_entries frozen_entries
  local expected_frozen_entries all_entries entry_count=0
  for path in "${root}" "${frozen}"; do
    [[ "$(stat -c '%a:%u:%g:%d' -- "${path}")" == \
      "555:0:0:${expected_device}" ]] || fail "closure directory metadata drifted: ${path}"
  done
  for path in "${regular}" "${directories}" "${receipt}" \
    "${frozen_sealer}" "${frozen_amendment}"; do
    require_file "${path}"
    [[ "$(stat -c '%a:%u:%g:%h:%d' -- "${path}")" == \
      "444:0:0:1:${expected_device}" ]] || fail "closure file metadata drifted: ${path}"
  done
  capture_sorted_find0 top_entries "${root}" -mindepth 1 -maxdepth 1
  new_temp_file expected_top_entries \
    "/tmp/${closure_schema_id}.closure.top-level.expected.XXXXXX"
  printf '%s\0' "${root}/frozen_sources" \
    "${root}/interruption_closure.status" \
    "${root}/source_directories.inventory.tsv" \
    "${root}/source_regular_files.inventory.tsv" \
    "${root}/source_snapshot" >"${expected_top_entries}"
  cmp -s -- "${expected_top_entries}" "${top_entries}" ||
    fail "closure top-level path set drifted"

  capture_sorted_find0 frozen_entries "${frozen}" -mindepth 1 -maxdepth 1
  new_temp_file expected_frozen_entries \
    "/tmp/${closure_schema_id}.closure.frozen.expected.XXXXXX"
  printf '%s\0' \
    "${frozen}/REPRESENTATION_ABLATION_RETRY2_STAGE04_INTERRUPTION_RECOVERY_AMENDMENT.md" \
    "${frozen}/seal_and_verify_representation_ablation_retry2_stage04_interruption_closure_v1.sh" \
    >"${expected_frozen_entries}"
  cmp -s -- "${expected_frozen_entries}" "${frozen_entries}" ||
    fail "closure frozen-authority path set drifted"

  capture_find0 all_entries "${root}" -mindepth 0
  while IFS= read -r -d '' path; do
    ((entry_count += 1))
  done <"${all_entries}"
  [[ "${entry_count}" == 95 ]] || fail "closure total entry count drifted"

  [[ "$(sha256_of "${frozen_sealer}")" == "${process_start_sealer_sha256}" ]] ||
    fail "frozen sealer hash drifted"
  cmp -s -- "${script_path}" "${frozen_sealer}" ||
    fail "live and frozen sealers differ"
  [[ "$(sha256_of "${frozen_amendment}")" == "${expected_amendment_sha256}" ]] ||
    fail "frozen amendment hash drifted"
  cmp -s -- "${amendment_path}" "${frozen_amendment}" ||
    fail "live and frozen amendments differ"

  verify_snapshot_copy_at "${snapshot}" "${regular}" "${directories}"
  verify_receipt_exact "${receipt}" "${regular}" "${directories}" "${snapshot}" \
    "${frozen_sealer}" "${frozen_amendment}" \
    "${source_regular_snapshot_sha}" "${source_directory_snapshot_sha}"
}

build_staging_closure() {
  local source_regular_snapshot_sha="$1" source_directory_snapshot_sha="$2"
  assert_absent "${staging_root}" "closure staging root"
  mkdir -- "${staging_root}"
  mkdir -- "${staging_root}/frozen_sources"
  copy_and_freeze_source_snapshot "${staging_root}/source_snapshot"

  emit_paired_regular_inventory "${staging_root}/source_snapshot" \
    >"${staging_root}/source_regular_files.inventory.tsv"
  emit_paired_directory_inventory "${staging_root}/source_snapshot" \
    >"${staging_root}/source_directories.inventory.tsv"
  cp --reflink=never -- "${script_path}" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")"
  cp --reflink=never -- "${amendment_path}" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  emit_receipt "${staging_root}/interruption_closure.status" \
    "${staging_root}/source_regular_files.inventory.tsv" \
    "${staging_root}/source_directories.inventory.tsv" \
    "${staging_root}/source_snapshot" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")" \
    "${source_regular_snapshot_sha}" "${source_directory_snapshot_sha}"

  chmod 0444 -- "${staging_root}/interruption_closure.status" \
    "${staging_root}/source_regular_files.inventory.tsv" \
    "${staging_root}/source_directories.inventory.tsv" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  chmod 0555 -- "${staging_root}/frozen_sources" "${staging_root}"
}

publish_staging_closure() {
  local staging_device staging_inode destination_device destination_inode
  local parent_device parent_inode
  assert_absent "${closure_root}" "published closure root"
  reject_symlink_components "${runtime_parent}"
  require_dir "${runtime_parent}"
  reject_symlink_components "${staging_root}"
  require_dir "${staging_root}"
  parent_device="$(stat -c '%d' -- "${runtime_parent}")"
  parent_inode="$(stat -c '%i' -- "${runtime_parent}")"
  staging_device="$(stat -c '%d' -- "${staging_root}")"
  staging_inode="$(stat -c '%i' -- "${staging_root}")"
  [[ "${staging_device}" == "${parent_device}" ]] ||
    fail "closure staging root is on the wrong publication device"
  assert_tree_has_no_live_references "${staging_root}" \
    "closure staging tree"
  assert_source_lock_identity
  [[ "$(stat -c '%d:%i' -- "${runtime_parent}")" == \
    "${parent_device}:${parent_inode}" ]] ||
    fail "closure publication parent identity drifted before rename"
  [[ "$(stat -c '%d:%i' -- "${staging_root}")" == \
    "${staging_device}:${staging_inode}" ]] ||
    fail "closure staging identity drifted before rename"
  assert_absent "${closure_root}" "published closure root before rename"
  mv -T -n -- "${staging_root}" "${closure_root}"
  assert_absent "${staging_root}" "consumed closure staging root"
  require_dir "${closure_root}"
  destination_device="$(stat -c '%d' -- "${closure_root}")"
  destination_inode="$(stat -c '%i' -- "${closure_root}")"
  [[ "${destination_device}:${destination_inode}" == \
    "${staging_device}:${staging_inode}" ]] ||
    fail "closure publication did not preserve candidate inode identity"
}

assert_container_boundary

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

acquire_locks_in_order
assert_sensitive_outputs_absent_before_tree_read
assert_absent "${staging_root}" "closure staging root"

snapshot1_regular=""
snapshot1_directories=""
snapshot2_regular=""
snapshot2_directories=""
new_temp_file snapshot1_regular "/tmp/${closure_schema_id}.source.regular.1.XXXXXX"
new_temp_file snapshot1_directories "/tmp/${closure_schema_id}.source.directories.1.XXXXXX"
new_temp_file snapshot2_regular "/tmp/${closure_schema_id}.source.regular.2.XXXXXX"
new_temp_file snapshot2_directories "/tmp/${closure_schema_id}.source.directories.2.XXXXXX"

take_source_snapshot "${snapshot1_regular}" "${snapshot1_directories}"
verify_scientific_boundary

if [[ "${mode}" == --audit ]]; then
  take_source_snapshot "${snapshot2_regular}" "${snapshot2_directories}"
  cmp -s -- "${snapshot1_regular}" "${snapshot2_regular}" ||
    fail "two source regular-file snapshots differ"
  cmp -s -- "${snapshot1_directories}" "${snapshot2_directories}" ||
    fail "two source directory snapshots differ"
  echo "retry2_stage04_interruption_audit=complete"
  echo "source_regular_snapshot_sha256=$(sha256_of "${snapshot1_regular}")"
  echo "source_directory_snapshot_sha256=$(sha256_of "${snapshot1_directories}")"
  echo "source_snapshots_identical=true"
  echo "source_runtime_mutated=false"
  exit 0
fi

if [[ "${mode}" == --verify || -e "${closure_root}" || -L "${closure_root}" ]]; then
  [[ "${mode}" != --seal || -d "${closure_root}" ]] ||
    fail "closure path exists but is not a directory"
  require_dir "${closure_root}"
  take_source_snapshot "${snapshot2_regular}" "${snapshot2_directories}"
  cmp -s -- "${snapshot1_regular}" "${snapshot2_regular}" ||
    fail "two source regular-file snapshots differ"
  cmp -s -- "${snapshot1_directories}" "${snapshot2_directories}" ||
    fail "two source directory snapshots differ"
  verify_closure_tree_at "${closure_root}" \
    "$(sha256_of "${snapshot1_regular}")" \
    "$(sha256_of "${snapshot1_directories}")"
  verify_scientific_boundary
  echo "interruption_closure_receipt=${receipt_path}"
  echo "interruption_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
  exit 0
fi

[[ "${mode}" == --seal ]] || fail "closure is absent; use --seal"
build_staging_closure "$(sha256_of "${snapshot1_regular}")" \
  "$(sha256_of "${snapshot1_directories}")"
verify_scientific_boundary
take_source_snapshot "${snapshot2_regular}" "${snapshot2_directories}"
cmp -s -- "${snapshot1_regular}" "${snapshot2_regular}" ||
  fail "two source regular-file snapshots differ"
cmp -s -- "${snapshot1_directories}" "${snapshot2_directories}" ||
  fail "two source directory snapshots differ"
verify_closure_tree_at "${staging_root}" \
  "$(sha256_of "${snapshot2_regular}")" \
  "$(sha256_of "${snapshot2_directories}")"
assert_no_live_references
publish_staging_closure
verify_closure_tree_at "${closure_root}" \
  "$(sha256_of "${snapshot2_regular}")" \
  "$(sha256_of "${snapshot2_directories}")"
verify_scientific_boundary
echo "interruption_closure_receipt=${receipt_path}"
echo "interruption_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
