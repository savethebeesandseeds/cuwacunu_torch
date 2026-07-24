#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1"
readonly source_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1"
readonly retry2_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"

readonly expected_amendment_sha256="e7d99698ee4b62254a274ebbf3d699b9e00ced0ff727279767e1d4976d594002"
readonly expected_runner_sha256="bebd812c48ba318ec632ed490841188daef9cdd68e02a53616ca9feba809ae43"
readonly expected_attempt_sha256="0ad5bbaba0183a710d4fd784cfc1bb5c3013fb937e4c5821b7df6183e231b359"
readonly expected_inputs_sha256="709350e637225eefcb854eaa6412f41a57cb3d89655c62ee4ebe59adacb1fa9b"
readonly expected_config_inputs_sha256="51ddd8b886965dd106f635127c29f2cc4418edddf19a44df3754731d5947a6cf"
readonly expected_effective_grammar_sha256="ceef6dc79533f8e72c231677a969680d9690327f091e53d4c35e08785d2f71a4"
readonly expected_canonical_import_sha256="89221a7e77067d0d69ddc11b79cda0ed6f81d7855df69c687570a4d4e811017e"
readonly expected_endpoint_status_sha256="b0fa364d31f32471cf0ff3d69b2836203e4dc47fd79f4c0507a7d7367b2f5ed5"
readonly expected_endpoint_result_sha256="7d3275bc2bdd8d647f1f06f41c1d11097acff56cc4f5e8cbb7d2497978ce182c"
readonly expected_endpoint_report_sha256="99e370677d4dd1932aa4fd3af66b954e2969a3afc55b1be2436b8875e8c64740"
readonly expected_endpoint_checkpoint_sha256="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
readonly expected_endpoint_checkpoint_internal_digest="14398b461a259bc5"
readonly expected_endpoint_manifest_sha256="f1e514f84f05ae898b8616204e75cfe4034f2b23ff972c525f62635349905c7c"
readonly expected_endpoint_log_sha256="7346a96afd6c226a772a22de6d903f2a361c70b5f72843816eff80c8c481f31c"
readonly expected_time_report_sha256="e4dde2289353241dc9dc501e8486f4d14c99028b4d17a579cf3b9968b691d19b"
readonly expected_time_checkpoint_sha256="33335faecacfc34d222389da326594f323243c57e5fb1f456ac266c7e5db01fc"
readonly expected_time_checkpoint_internal_digest="45b88b310b344ec0"
readonly expected_time_manifest_sha256="bb9f238bf1b4bcb1dd632efc8c0cb2bba4e0c5155608045bef5b309be16b6d3a"
readonly expected_time_events_sha256="46daf72237a2a2f30f67d5968cee8a206800928e50b7f2a405051ad91e3a0083"
readonly expected_time_log_sha256="0dcad31cb1a33ffad89066d53a80a2f76ac655c10c19b91fa298dc08df0e89b8"

readonly expected_regular_file_count=49
readonly expected_directory_count=25
readonly expected_regular_file_bytes=64939302
readonly expected_content_inventory_sha256="6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05"
readonly expected_optimizer_steps=3000
readonly expected_time_optimizer_steps=2880
readonly expected_time_checkpoint_step=2850

fail() {
  echo "representation ablation retry1 interruption closure: $*" >&2
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

kv() {
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
    fail "${path}: expected exactly one ${key}= entry, found ${count}"
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
  actual="$(kv "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
source_runtime="${runtime_parent}/${source_schema_id}"
closure_root="${runtime_parent}/${schema_id}"
staging_root="${runtime_parent}/.${schema_id}.candidate"
retry2_runtime="${runtime_parent}/${retry2_schema_id}"

amendment_path="${script_dir}/REPRESENTATION_ABLATION_RETRY1_OPERATIONAL_INTERRUPTION_RECOVERY_AMENDMENT.md"
runner_path="${script_dir}/run_representation_ablation_v2_retry1.sh"
source_frozen_runner="${source_runtime}/frozen_sources/$(basename "${runner_path}")"
source_lock_path="${source_runtime}/.development.lock"
attempt_path="${source_runtime}/development.retry1.attempt.status"
inputs_path="${source_runtime}/inputs.status"
config_inputs_path="${source_runtime}/config_inputs.status"
effective_grammar_path="${source_runtime}/effective_grammar_closure.status"
canonical_import_path="${source_runtime}/arms/canonical/import.status"

endpoint_root="${source_runtime}/arms/endpoint_scale"
endpoint_training="${endpoint_root}/training"
endpoint_status="${endpoint_root}/training.status"
endpoint_log="${endpoint_root}/training.log"
endpoint_job="${endpoint_training}/job"
endpoint_manifest="${endpoint_job}/job.manifest"
endpoint_report="${endpoint_job}/channel_representation.report"
endpoint_checkpoint="${endpoint_job}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
endpoint_result="${endpoint_job}/runtime.result.fact"

time_root="${source_runtime}/arms/time_only"
time_training="${time_root}/training"
time_log="${time_root}/training.log"
time_job="${time_training}/job"
time_manifest="${time_job}/job.manifest"
time_report="${time_job}/channel_representation.report"
time_checkpoint="${time_job}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
time_events="${time_job}/runtime.job_events.probe"
time_status="${time_root}/training.status"
time_result="${time_job}/runtime.result.fact"

no_tf_root="${source_runtime}/arms/no_tf_alignment"

receipt_path="${closure_root}/interruption_closure.status"
regular_inventory_path="${closure_root}/regular_files.inventory.tsv"
directory_inventory_path="${closure_root}/directories.inventory.tsv"
frozen_root="${closure_root}/frozen_sources"
frozen_sealer_path="${frozen_root}/$(basename "${script_path}")"
frozen_amendment_path="${frozen_root}/$(basename "${amendment_path}")"

readonly process_start_sealer_sha256="$(sha256_of "${script_path}")"
readonly process_start_sealer_mode="$(stat -c '%a' -- "${script_path}")"
readonly process_start_sealer_links="$(stat -c '%h' -- "${script_path}")"
readonly process_start_sealer_uid="$(stat -c '%u' -- "${script_path}")"
readonly process_start_sealer_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_sealer_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_sealer_device="$(stat -c '%d' -- "${script_path}")"

declare -a temporary_files=()
candidate_created_by_process=false
source_lock_fd=""
source_lock_device=""
source_lock_inode=""

cleanup() {
  local path
  for path in "${temporary_files[@]:-}"; do
    [[ -n "${path}" ]] && rm -f -- "${path}" 2>/dev/null || true
  done
  if [[ "${candidate_created_by_process}" == true && \
    -d "${staging_root}" && ! -L "${staging_root}" ]]; then
    chmod u+w -- "${staging_root}" "${staging_root}/frozen_sources" \
      2>/dev/null || true
    rm -f -- "${staging_root}/interruption_closure.status" \
      "${staging_root}/regular_files.inventory.tsv" \
      "${staging_root}/directories.inventory.tsv" \
      "${staging_root}/frozen_sources/$(basename "${script_path}")" \
      "${staging_root}/frozen_sources/$(basename "${amendment_path}")" \
      2>/dev/null || true
    rmdir -- "${staging_root}/frozen_sources" "${staging_root}" \
      2>/dev/null || true
  fi
}
trap cleanup EXIT

new_temp_file() {
  local __result_var="$1"
  local template="$2"
  local created
  created="$(mktemp "${template}")"
  temporary_files+=("${created}")
  printf -v "${__result_var}" '%s' "${created}"
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] ||
  fail "usage: $0 [--plan|--audit|--seal|--verify]"
case "${mode}" in
--plan | --audit | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--audit|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
source_runtime_schema_id=${source_schema_id}
source_runtime_root=${source_runtime}
source_lock_path=${source_lock_path}
source_snapshot_count=2
source_regular_file_count=${expected_regular_file_count}
source_directory_count=${expected_directory_count}
source_regular_file_bytes=${expected_regular_file_bytes}
source_content_inventory_sha256=${expected_content_inventory_sha256}
closure_runtime_root=${closure_root}
closure_receipt_path=${receipt_path}
publication_method=atomic_no_clobber_directory_rename
source_runtime_mutation_authorized=false
attempt_status=operationally_interrupted
endpoint_status=complete
endpoint_optimizer_steps=${expected_optimizer_steps}
time_only_status=incomplete
time_only_optimizer_steps_observed=${expected_time_optimizer_steps}
time_only_last_checkpoint_optimizer_step_reported=${expected_time_checkpoint_step}
no_tf_alignment_training_started=false
external_operational_evidence=docker_vm_runaway_tmp_log_inode_161829_and_sdd_io_errors
external_operational_evidence_is_model_or_result_evidence=false
exact_process_stop_claim=none
exact_process_exit_status_claim=none
partial_artifact_reuse_authorized=false
retry1_reentry_authorized=false
direct_endpoint_use_authorized=false
endpoint_import_eligibility=requires_separate_retry2_import_verifier
endpoint_import_requires_exact_retry2_scientific_config_equivalence=true
endpoint_import_requires_byte_identical_copy=true
endpoint_import_requires_distinct_source_copy_identity=true
endpoint_import_hardlink_authorized=false
retry2_endpoint_consumption=retry2_local_import_copy_only
retry2_schema_id=${retry2_schema_id}
retry2_runtime_root=${retry2_runtime}
retry2_time_only_restart_optimizer_step=0
retry2_no_tf_alignment_restart_optimizer_step=0
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
policy_access=false
PLAN
}

verify_hash() {
  local path="$1"
  local expected="$2"
  local label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an unexpected hard-link count"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
}

assert_process_start_source_identity() {
  reject_symlink_components "${script_path}"
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == \
    "${process_start_sealer_sha256}" ]] ||
    fail "executing sealer bytes changed after process start"
  [[ "$(stat -c '%a' -- "${script_path}")" == \
    "${process_start_sealer_mode}" ]] ||
    fail "executing sealer mode changed after process start"
  [[ "$(stat -c '%h' -- "${script_path}")" == \
    "${process_start_sealer_links}" ]] ||
    fail "executing sealer link count changed after process start"
  [[ "$(stat -c '%u' -- "${script_path}")" == \
    "${process_start_sealer_uid}" ]] ||
    fail "executing sealer owner changed after process start"
  [[ "$(stat -c '%s' -- "${script_path}")" == \
    "${process_start_sealer_bytes}" ]] ||
    fail "executing sealer size changed after process start"
  [[ "$(stat -c '%i' -- "${script_path}")" == \
    "${process_start_sealer_inode}" ]] ||
    fail "executing sealer inode changed after process start"
  [[ "$(stat -c '%d' -- "${script_path}")" == \
    "${process_start_sealer_device}" ]] ||
    fail "executing sealer device changed after process start"
  [[ "${process_start_sealer_mode}" == 555 ]] ||
    fail "executing sealer must be frozen mode 0555"
  [[ "${process_start_sealer_links}" == 1 ]] ||
    fail "executing sealer must have link count one"
}

verify_external_authorities() {
  assert_process_start_source_identity
  verify_hash "${amendment_path}" "${expected_amendment_sha256}" \
    "retry1 interruption recovery amendment"
  [[ "$(stat -c '%a' -- "${amendment_path}")" == 444 ]] ||
    fail "retry1 interruption recovery amendment must be mode 0444"
  verify_hash "${runner_path}" "${expected_runner_sha256}" \
    "retry1 operational runner"
  [[ "$(stat -c '%a' -- "${runner_path}")" == 555 ]] ||
    fail "retry1 operational runner must remain mode 0555"
  verify_hash "${source_frozen_runner}" "${expected_runner_sha256}" \
    "retry1 frozen runner"
}

relative_path_of() {
  local path="$1"
  if [[ "${path}" == "${source_runtime}" ]]; then
    printf '.'
  else
    printf '%s' "${path#${source_runtime}/}"
  fi
}

emit_current_regular_inventory() {
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tsha256\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && \
      "${rel}" != *'|'* ]] ||
      fail "source runtime has a non-canonical inventory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    [[ "${links_value}" == 1 ]] ||
      fail "source runtime regular file has hard-link drift: ${rel}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}" \
      "$(sha256_of "${path}")"
  done < <(find "${source_runtime}" -xdev -type f -print0 |
    LC_ALL=C sort -z)
}

emit_current_directory_inventory() {
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && \
      "${rel}" != *'|'* ]] ||
      fail "source runtime has a non-canonical directory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}"
  done < <(find "${source_runtime}" -xdev -type d -print0 |
    LC_ALL=C sort -z)
}

emit_expected_directory_paths() {
  cat <<'DIRECTORIES'
.
arms
arms/canonical
arms/canonical/affine
arms/endpoint_scale
arms/endpoint_scale/config
arms/endpoint_scale/training
arms/endpoint_scale/training/components
arms/endpoint_scale/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg
arms/endpoint_scale/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns
arms/endpoint_scale/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd
arms/endpoint_scale/training/job
arms/endpoint_scale/training/system
arms/no_tf_alignment
arms/no_tf_alignment/config
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
DIRECTORIES
}

compute_complete_content_inventory_sha256() {
  (
    cd "${source_runtime}"
    find . -xdev -type f -print0 | LC_ALL=C sort -z |
      xargs -0 sha256sum |
      sha256sum | awk '{print $1}'
  )
}

verify_source_tree_shape() {
  reject_symlink_components "${source_runtime}"
  require_dir "${source_runtime}"
  [[ "$(realpath -e -- "${source_runtime}")" == "${source_runtime}" ]] ||
    fail "retry1 source runtime root is not canonical"

  local entry source_device entry_device
  source_device="$(stat -c '%d' -- "${source_runtime}")"
  while IFS= read -r -d '' entry; do
    [[ ! -L "${entry}" ]] ||
      fail "retry1 source runtime contains a symlink: ${entry}"
    entry_device="$(stat -c '%d' -- "${entry}")"
    [[ "${entry_device}" == "${source_device}" ]] ||
      fail "retry1 source runtime contains a cross-device entry: ${entry}"
    if [[ -f "${entry}" ]]; then
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "retry1 source runtime file has hard-link drift: ${entry}"
    elif [[ ! -d "${entry}" ]]; then
      fail "retry1 source runtime contains a special entry: ${entry}"
    fi
  done < <(find "${source_runtime}" -xdev -mindepth 0 -print0)
}

verify_snapshot_aggregate() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  local regular_count directory_count regular_bytes
  local expected_paths actual_paths

  regular_count="$(($(wc -l <"${regular_inventory}") - 1))"
  directory_count="$(($(wc -l <"${directory_inventory}") - 1))"
  regular_bytes="$(awk -F '\t' 'NR > 1 { sum += $6 }
    END { printf "%.0f", sum + 0 }' "${regular_inventory}")"
  [[ "${regular_count}" == "${expected_regular_file_count}" ]] ||
    fail "retry1 regular-file count drifted: ${regular_count}"
  [[ "${directory_count}" == "${expected_directory_count}" ]] ||
    fail "retry1 directory count drifted: ${directory_count}"
  [[ "${regular_bytes}" == "${expected_regular_file_bytes}" ]] ||
    fail "retry1 regular-file byte count drifted: ${regular_bytes}"
  [[ "$(compute_complete_content_inventory_sha256)" == \
    "${expected_content_inventory_sha256}" ]] ||
    fail "retry1 complete content inventory digest drifted"

  new_temp_file expected_paths "/tmp/${schema_id}.dirs.expected.XXXXXX"
  new_temp_file actual_paths "/tmp/${schema_id}.dirs.actual.XXXXXX"
  emit_expected_directory_paths >"${expected_paths}"
  tail -n +2 "${directory_inventory}" | cut -f1 >"${actual_paths}"
  cmp -s -- "${expected_paths}" "${actual_paths}" ||
    fail "retry1 directory path set drifted"
}

take_source_snapshot() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  assert_source_lock_identity
  verify_source_tree_shape
  emit_current_regular_inventory >"${regular_inventory}"
  emit_current_directory_inventory >"${directory_inventory}"
  verify_snapshot_aggregate "${regular_inventory}" "${directory_inventory}"
  assert_source_lock_identity
}

assert_absent() {
  local path="$1"
  local label="$2"
  [[ ! -e "${path}" && ! -L "${path}" ]] ||
    fail "${label} unexpectedly exists: ${path}"
}

verify_manifest_boundary() {
  local manifest="$1"
  local config="$2"
  expect_kv "${manifest}" manifest_format kikijyeba.runtime.job_manifest.v1
  expect_kv "${manifest}" job_id \
    train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" job_attempt_policy explicit_job_dir_no_overwrite_v1
  expect_kv "${manifest}" config_path "${config}"
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin 0
  expect_kv "${manifest}" resolved_anchor_index_end 2496
  expect_kv "${manifest}" accepted_anchor_count 3261
  expect_kv "${manifest}" candidate_anchor_count 3261
  expect_kv "${manifest}" skipped_outside_common_range 0
  expect_kv "${manifest}" skipped_missing_edge_coverage 0
  expect_kv "${manifest}" skipped_failed_fetch_probe 0
  expect_kv "${manifest}" duplicate_anchor_count 0
  expect_kv "${manifest}" input_representation_checkpoint_path ''
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  [[ "$(kv "${manifest}" source_file_receipts)" != *"/data/raw/"* ]] ||
    fail "retry1 manifest references canonical data/raw: ${manifest}"
  [[ "$(kv "${manifest}" source_file_receipts)" == \
    *"/synthetic_v2_isolated_development_source_v1/source/"* ]] ||
    fail "retry1 manifest omits the isolated development source: ${manifest}"
  [[ "$(kv "${manifest}" execution_chain)" != *"policy"* ]] ||
    fail "retry1 manifest execution chain reached policy: ${manifest}"
}

verify_attempt_and_setup() {
  verify_hash "${attempt_path}" "${expected_attempt_sha256}" \
    "retry1 development attempt sentinel"
  expect_kv "${attempt_path}" schema_id \
    "${source_schema_id}.development_retry_attempt.v1"
  expect_kv "${attempt_path}" status consumed
  expect_kv "${attempt_path}" attempt_ordinal 1
  expect_kv "${attempt_path}" operational_ablation_runner_sha256 \
    "${expected_runner_sha256}"
  expect_kv "${attempt_path}" published_before_first_runtime_call true
  expect_kv "${attempt_path}" optimizer_steps_at_publication 0
  expect_kv "${attempt_path}" additional_development_retries_authorized false
  expect_kv "${attempt_path}" canonical_data_raw_access false
  expect_kv "${attempt_path}" certified_input_access false
  expect_kv "${attempt_path}" final_holdout_access false
  expect_kv "${attempt_path}" policy_access false

  verify_hash "${inputs_path}" "${expected_inputs_sha256}" \
    "retry1 input receipt"
  verify_hash "${config_inputs_path}" "${expected_config_inputs_sha256}" \
    "retry1 config-input closure"
  verify_hash "${effective_grammar_path}" \
    "${expected_effective_grammar_sha256}" \
    "retry1 effective-grammar closure"
  verify_hash "${canonical_import_path}" \
    "${expected_canonical_import_sha256}" "retry1 canonical import"
  expect_kv "${canonical_import_path}" schema_id \
    "${source_schema_id}.canonical_import.v1"
  expect_kv "${canonical_import_path}" status complete
  expect_kv "${canonical_import_path}" maximum_anchor_read 2815
  expect_kv "${canonical_import_path}" certified_input_access false
  expect_kv "${canonical_import_path}" canonical_data_raw_access false
  expect_kv "${canonical_import_path}" final_holdout_access false
  expect_kv "${canonical_import_path}" policy_access false
}

verify_endpoint_complete() {
  verify_hash "${endpoint_status}" "${expected_endpoint_status_sha256}" \
    "endpoint-scale training status"
  verify_hash "${endpoint_result}" "${expected_endpoint_result_sha256}" \
    "endpoint-scale Runtime result"
  verify_hash "${endpoint_report}" "${expected_endpoint_report_sha256}" \
    "endpoint-scale representation report"
  verify_hash "${endpoint_checkpoint}" \
    "${expected_endpoint_checkpoint_sha256}" \
    "endpoint-scale completed checkpoint"
  verify_hash "${endpoint_manifest}" "${expected_endpoint_manifest_sha256}" \
    "endpoint-scale job manifest"
  verify_hash "${endpoint_log}" "${expected_endpoint_log_sha256}" \
    "endpoint-scale training log"

  expect_kv "${endpoint_status}" schema_id "${source_schema_id}.training.v1"
  expect_kv "${endpoint_status}" status complete
  expect_kv "${endpoint_status}" arm endpoint_scale
  expect_kv "${endpoint_status}" runner_sha256 "${expected_runner_sha256}"
  expect_kv "${endpoint_status}" checkpoint_path "${endpoint_checkpoint}"
  expect_kv "${endpoint_status}" checkpoint_sha256 \
    "${expected_endpoint_checkpoint_sha256}"
  expect_kv "${endpoint_status}" runtime_result_path "${endpoint_result}"
  expect_kv "${endpoint_status}" runtime_result_sha256 \
    "${expected_endpoint_result_sha256}"
  expect_kv "${endpoint_status}" representation_report_path \
    "${endpoint_report}"
  expect_kv "${endpoint_status}" representation_report_sha256 \
    "${expected_endpoint_report_sha256}"
  expect_kv "${endpoint_status}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_status}" seed 17
  expect_kv "${endpoint_status}" forecast_labels_used false
  expect_kv "${endpoint_status}" canonical_data_raw_access false
  expect_kv "${endpoint_status}" final_holdout_access false
  expect_kv "${endpoint_status}" policy_access false

  expect_kv "${endpoint_result}" status completed
  expect_kv "${endpoint_result}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_result}" finite_parameter_check true
  expect_kv "${endpoint_result}" checkpoint_written true
  expect_kv "${endpoint_result}" checkpoint_path "${endpoint_checkpoint}"
  expect_kv "${endpoint_result}" source_report_path "${endpoint_report}"

  expect_kv "${endpoint_report}" steps_attempted \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_report}" steps_completed \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_report}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_report}" wave_pulses_completed \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_report}" finite_parameter_check true
  expect_kv "${endpoint_report}" nonfinite_output_count 0
  expect_kv "${endpoint_report}" checkpoint_written true
  expect_kv "${endpoint_report}" last_checkpoint_optimizer_step \
    "${expected_optimizer_steps}"
  expect_kv "${endpoint_report}" checkpoint_path "${endpoint_checkpoint}"
  expect_kv "${endpoint_report}" checkpoint_digest_reported \
    "${expected_endpoint_checkpoint_internal_digest}"
  expect_kv "${endpoint_report}" checkpoint_artifact_status verified
  expect_kv "${endpoint_report}" last_report_attempted_step \
    "${expected_optimizer_steps}"

  verify_manifest_boundary "${endpoint_manifest}" \
    "${endpoint_root}/config/train.config"
}

verify_time_only_incomplete() {
  verify_hash "${time_report}" "${expected_time_report_sha256}" \
    "time-only retained report"
  verify_hash "${time_checkpoint}" "${expected_time_checkpoint_sha256}" \
    "time-only retained partial checkpoint"
  verify_hash "${time_manifest}" "${expected_time_manifest_sha256}" \
    "time-only job manifest"
  verify_hash "${time_events}" "${expected_time_events_sha256}" \
    "time-only Runtime event probe"
  verify_hash "${time_log}" "${expected_time_log_sha256}" \
    "time-only training log"

  [[ "$(grep -Fxc -- '  MAX_STEPS = 3000;' \
    "${time_root}/config/representation.jkimyei")" == 1 ]] ||
    fail "time-only requested step contract is not exactly 3000"
  expect_kv "${time_report}" use_frequency_tokens false
  expect_kv "${time_report}" steps_attempted \
    "${expected_time_optimizer_steps}"
  expect_kv "${time_report}" steps_completed \
    "${expected_time_optimizer_steps}"
  expect_kv "${time_report}" optimizer_steps \
    "${expected_time_optimizer_steps}"
  expect_kv "${time_report}" wave_pulses_completed \
    "${expected_time_optimizer_steps}"
  expect_kv "${time_report}" finite_parameter_check true
  expect_kv "${time_report}" nonfinite_output_count 0
  expect_kv "${time_report}" checkpoint_written true
  expect_kv "${time_report}" last_checkpoint_optimizer_step \
    "${expected_time_checkpoint_step}"
  expect_kv "${time_report}" checkpoint_path "${time_checkpoint}"
  expect_kv "${time_report}" checkpoint_digest_reported \
    "${expected_time_checkpoint_internal_digest}"
  expect_kv "${time_report}" checkpoint_artifact_status verified
  expect_kv "${time_report}" last_report_attempted_step \
    "${expected_time_optimizer_steps}"
  verify_manifest_boundary "${time_manifest}" \
    "${time_root}/config/train.config"

  assert_absent "${time_status}" "time-only completion status"
  assert_absent "${time_result}" "time-only Runtime result"
}

verify_no_tf_and_downstream_absent() {
  local path
  for path in "${no_tf_root}/training" "${no_tf_root}/training.log" \
    "${no_tf_root}/training.status"; do
    assert_absent "${path}" "no-TF-alignment training artifact"
  done

  for path in "${source_runtime}/development.status" \
    "${source_runtime}/selection.status" \
    "${source_runtime}/certified.attempt.status" \
    "${source_runtime}/certified" \
    "${source_runtime}/result.status"; do
    assert_absent "${path}" "retry1 downstream artifact"
  done

  local arm
  for arm in endpoint_scale time_only no_tf_alignment; do
    for path in "${source_runtime}/arms/${arm}/capture" \
      "${source_runtime}/arms/${arm}/capture.log" \
      "${source_runtime}/arms/${arm}/capture.status" \
      "${source_runtime}/arms/${arm}/affine.status"; do
      assert_absent "${path}" "retry1 challenger downstream artifact"
    done
  done

  path="$(find "${source_runtime}" -xdev -type f \
    \( -name representation_edge_features.probe -o \
    -name capture.status -o -name affine.status -o \
    -name development.status -o -name selection.status -o \
    -name result.status \) -print -quit)"
  [[ -z "${path}" ]] ||
    fail "retry1 contains a forbidden downstream file: ${path}"
}

verify_retry1_boundary() {
  assert_source_lock_identity
  verify_external_authorities
  verify_attempt_and_setup
  verify_endpoint_complete
  verify_time_only_incomplete
  verify_no_tf_and_downstream_absent
  assert_process_start_source_identity
  assert_source_lock_identity
}

emit_receipt() {
  local destination="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  local frozen_sealer="$4"
  local frozen_amendment="$5"
  cat >"${destination}" <<RECEIPT
schema_id=${schema_id}
status=complete
closure_kind=external_operational_interruption
attempt_status=interrupted
evidence_class=operational_interruption_boundary
scientific_evidence=false
metric_evidence_authorized=false
closure_runtime_root=${closure_root}
closure_receipt_path=${receipt_path}
publication_method=atomic_no_clobber_directory_rename
source_runtime_schema_id=${source_schema_id}
source_runtime_root=${source_runtime}
source_runtime_mutated=false
source_runtime_receipt_published_in_place=false
source_lock_path=${source_lock_path}
source_lock_acquired_read_only=true
source_lock_held_through_two_snapshots=true
source_snapshot_count=2
source_snapshots_identical=true
regular_file_inventory_path=${regular_inventory_path}
regular_file_inventory_sha256=$(sha256_of "${regular_inventory}")
source_snapshot_1_regular_inventory_sha256=$(sha256_of "${regular_inventory}")
source_snapshot_2_regular_inventory_sha256=$(sha256_of "${regular_inventory}")
source_regular_file_count=${expected_regular_file_count}
source_regular_file_bytes=${expected_regular_file_bytes}
directory_inventory_path=${directory_inventory_path}
directory_inventory_sha256=$(sha256_of "${directory_inventory}")
source_snapshot_1_directory_inventory_sha256=$(sha256_of "${directory_inventory}")
source_snapshot_2_directory_inventory_sha256=$(sha256_of "${directory_inventory}")
source_directory_count=${expected_directory_count}
content_inventory_algorithm=cd_source_root_find_dot_xdev_regular_files_c_sort_z_sha256sum_then_sha256sum
content_inventory_sha256=${expected_content_inventory_sha256}
source_symlink_count=0
source_special_entry_count=0
source_regular_file_nonunit_link_count=0
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
retry1_runner_path=${runner_path}
retry1_runner_sha256=${expected_runner_sha256}
retry1_frozen_runner_path=${source_frozen_runner}
retry1_frozen_runner_sha256=${expected_runner_sha256}
retry1_attempt_path=${attempt_path}
retry1_attempt_sha256=${expected_attempt_sha256}
retry1_attempt_sentinel_status=consumed
retry1_reentry_authorized=false
retry1_partial_artifact_reuse_authorized=false
input_receipt_path=${inputs_path}
input_receipt_sha256=${expected_inputs_sha256}
config_input_closure_path=${config_inputs_path}
config_input_closure_sha256=${expected_config_inputs_sha256}
effective_grammar_closure_path=${effective_grammar_path}
effective_grammar_closure_sha256=${expected_effective_grammar_sha256}
canonical_import_path=${canonical_import_path}
canonical_import_sha256=${expected_canonical_import_sha256}
canonical_import_status=complete
endpoint_arm_status=complete
endpoint_training_status_path=${endpoint_status}
endpoint_training_status_sha256=${expected_endpoint_status_sha256}
endpoint_runtime_result_path=${endpoint_result}
endpoint_runtime_result_sha256=${expected_endpoint_result_sha256}
endpoint_runtime_result_status=completed
endpoint_report_path=${endpoint_report}
endpoint_report_sha256=${expected_endpoint_report_sha256}
endpoint_manifest_path=${endpoint_manifest}
endpoint_manifest_sha256=${expected_endpoint_manifest_sha256}
endpoint_log_path=${endpoint_log}
endpoint_log_sha256=${expected_endpoint_log_sha256}
endpoint_checkpoint_path=${endpoint_checkpoint}
endpoint_checkpoint_sha256=${expected_endpoint_checkpoint_sha256}
endpoint_checkpoint_internal_digest_reported=${expected_endpoint_checkpoint_internal_digest}
endpoint_checkpoint_file_sha256_distinct_from_internal_digest=true
endpoint_requested_optimizer_steps=${expected_optimizer_steps}
endpoint_completed_optimizer_steps=${expected_optimizer_steps}
endpoint_last_checkpoint_optimizer_step=${expected_optimizer_steps}
endpoint_direct_use_authorized=false
direct_endpoint_use_authorized=false
endpoint_import_eligibility=requires_separate_retry2_import_verifier
endpoint_import_receipt_present=false
endpoint_import_requires_exact_retry2_scientific_config_equivalence=true
endpoint_import_requires_byte_identical_copy=true
endpoint_import_requires_distinct_source_copy_identity=true
endpoint_import_hardlink_authorized=false
endpoint_import_consumption_scope=retry2_local_import_copy_only
time_only_arm_status=incomplete
time_only_report_path=${time_report}
time_only_report_sha256=${expected_time_report_sha256}
time_only_manifest_path=${time_manifest}
time_only_manifest_sha256=${expected_time_manifest_sha256}
time_only_log_path=${time_log}
time_only_log_sha256=${expected_time_log_sha256}
time_only_event_probe_path=${time_events}
time_only_event_probe_sha256=${expected_time_events_sha256}
time_only_partial_checkpoint_path=${time_checkpoint}
time_only_partial_checkpoint_sha256=${expected_time_checkpoint_sha256}
time_only_checkpoint_internal_digest_reported=${expected_time_checkpoint_internal_digest}
time_only_checkpoint_file_sha256_distinct_from_internal_digest=true
time_only_requested_optimizer_steps=${expected_optimizer_steps}
time_only_steps_attempted_observed=${expected_time_optimizer_steps}
time_only_steps_completed_observed=${expected_time_optimizer_steps}
time_only_optimizer_steps_observed=${expected_time_optimizer_steps}
time_only_last_checkpoint_optimizer_step_reported=${expected_time_checkpoint_step}
time_only_training_status_present=false
time_only_runtime_result_present=false
time_only_partial_artifact_reuse_authorized=false
no_tf_alignment_arm_status=not_started
no_tf_alignment_training_directory_present=false
no_tf_alignment_training_log_present=false
no_tf_alignment_training_status_present=false
no_tf_alignment_runtime_result_present=false
challenger_capture_artifact_present=false
challenger_affine_artifact_present=false
cross_arm_selection_present=false
development_completion_present=false
certified_artifact_present=false
final_result_present=false
external_operational_evidence=docker_vm_runaway_tmp_log_inode_161829_and_sdd_io_errors
external_operational_evidence_provenance=operator_observation_outside_retry1_runtime
external_operational_evidence_is_runtime_emitted=false
external_operational_evidence_is_model_or_result_evidence=false
external_operational_evidence_is_causal_attribution=false
exact_process_stop_claim=none
exact_process_exit_status_claim=none
process_liveness_history_claim=none
partial_artifact_reuse_authorized=false
resume_authorized=false
retry_required=true
retry2_schema_id=${retry2_schema_id}
retry2_runtime_root=${retry2_runtime}
retry2_time_only_restart_from_step_zero=true
retry2_time_only_restart_optimizer_step=0
retry2_no_tf_alignment_restart_from_step_zero=true
retry2_no_tf_alignment_restart_optimizer_step=0
retry2_endpoint_import_requires_separate_verifier=true
retry2_endpoint_import_requires_exact_scientific_config_equivalence=true
retry2_endpoint_import_requires_byte_identical_copy=true
retry2_endpoint_import_requires_distinct_source_copy_identity=true
retry2_endpoint_import_hardlink_authorized=false
retry2_endpoint_consumption=retry2_local_import_copy_only
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
policy_access=false
RECEIPT
}

verify_receipt_key_shape() {
  local inspected_receipt="$1"
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || seen[key]++) exit 42;
    }
  ' "${inspected_receipt}" ||
    fail "interruption closure receipt has a malformed or duplicate key"
}

verify_receipt_exact() {
  local inspected_receipt="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  local frozen_sealer="$4"
  local frozen_amendment="$5"
  reject_symlink_components "${inspected_receipt}"
  require_file "${inspected_receipt}"
  [[ "$(stat -c '%a' -- "${inspected_receipt}")" == 444 ]] ||
    fail "interruption closure receipt must be mode 0444"
  [[ "$(stat -c '%h' -- "${inspected_receipt}")" == 1 ]] ||
    fail "interruption closure receipt has an unexpected hard-link count"

  local expected_receipt
  new_temp_file expected_receipt "/tmp/${schema_id}.receipt.expected.XXXXXX"
  emit_receipt "${expected_receipt}" "${regular_inventory}" \
    "${directory_inventory}" "${frozen_sealer}" "${frozen_amendment}"
  verify_receipt_key_shape "${expected_receipt}"
  verify_receipt_key_shape "${inspected_receipt}"
  cmp -s -- "${expected_receipt}" "${inspected_receipt}" ||
    fail "interruption closure receipt content drifted"
}

verify_closure_tree_at() {
  local root="$1"
  local expected_regular_inventory="$2"
  local expected_directory_inventory="$3"
  local regular_inventory="${root}/regular_files.inventory.tsv"
  local directory_inventory="${root}/directories.inventory.tsv"
  local receipt="${root}/interruption_closure.status"
  local frozen_dir="${root}/frozen_sources"
  local frozen_sealer="${frozen_dir}/$(basename "${script_path}")"
  local frozen_amendment="${frozen_dir}/$(basename "${amendment_path}")"

  reject_symlink_components "${root}"
  require_dir "${root}"
  require_dir "${frozen_dir}"

  local entry rel entry_count=0
  while IFS= read -r -d '' entry; do
    ((entry_count += 1))
    [[ ! -L "${entry}" ]] || fail "closure contains a symlink: ${entry}"
    if [[ "${entry}" == "${root}" ]]; then
      rel="."
    else
      rel="${entry#${root}/}"
    fi
    case "${rel}" in
    . | frozen_sources)
      [[ -d "${entry}" ]] ||
        fail "closure path is not a directory: ${rel}"
      ;;
    interruption_closure.status | regular_files.inventory.tsv | \
      directories.inventory.tsv | \
      "frozen_sources/$(basename "${script_path}")" | \
      "frozen_sources/$(basename "${amendment_path}")")
      [[ -f "${entry}" ]] ||
        fail "closure path is not a regular file: ${rel}"
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "closure file has an unexpected hard-link count: ${rel}"
      ;;
    *) fail "closure contains an unknown entry: ${rel}" ;;
    esac
  done < <(find "${root}" -mindepth 0 -print0)
  [[ "${entry_count}" == 7 ]] ||
    fail "closure expected seven total entries, found ${entry_count}"

  local path
  for path in "${receipt}" "${regular_inventory}" "${directory_inventory}" \
    "${frozen_sealer}" "${frozen_amendment}"; do
    require_file "${path}"
    [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
      fail "closure file is not mode 0444: ${path}"
    [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
      fail "closure file has an unexpected hard-link count: ${path}"
  done
  [[ "$(stat -c '%a' -- "${root}")" == 555 ]] ||
    fail "closure root is not mode 0555"
  [[ "$(stat -c '%a' -- "${frozen_dir}")" == 555 ]] ||
    fail "closure frozen-source directory is not mode 0555"

  cmp -s -- "${expected_regular_inventory}" "${regular_inventory}" ||
    fail "published retry1 regular-file inventory drifted"
  cmp -s -- "${expected_directory_inventory}" "${directory_inventory}" ||
    fail "published retry1 directory inventory drifted"
  [[ "$(sha256_of "${frozen_sealer}")" == \
    "${process_start_sealer_sha256}" ]] ||
    fail "frozen interruption sealer hash drifted"
  cmp -s -- "${script_path}" "${frozen_sealer}" ||
    fail "live and frozen interruption sealers differ"
  [[ "$(sha256_of "${frozen_amendment}")" == \
    "${expected_amendment_sha256}" ]] ||
    fail "frozen interruption amendment hash drifted"
  cmp -s -- "${amendment_path}" "${frozen_amendment}" ||
    fail "live and frozen interruption amendments differ"
  verify_receipt_exact "${receipt}" "${regular_inventory}" \
    "${directory_inventory}" "${frozen_sealer}" "${frozen_amendment}"
}

build_staging_closure() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "external closure staging already exists: ${staging_root}"
  mkdir -- "${staging_root}"
  candidate_created_by_process=true
  mkdir -- "${staging_root}/frozen_sources"
  cp -- "${regular_inventory}" \
    "${staging_root}/regular_files.inventory.tsv"
  cp -- "${directory_inventory}" \
    "${staging_root}/directories.inventory.tsv"
  cp -- "${script_path}" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")"
  cp -- "${amendment_path}" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  assert_process_start_source_identity
  emit_receipt "${staging_root}/interruption_closure.status" \
    "${staging_root}/regular_files.inventory.tsv" \
    "${staging_root}/directories.inventory.tsv" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  chmod 0444 -- "${staging_root}/interruption_closure.status" \
    "${staging_root}/regular_files.inventory.tsv" \
    "${staging_root}/directories.inventory.tsv" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  chmod 0555 -- "${staging_root}/frozen_sources" "${staging_root}"
}

publish_staging_closure() {
  [[ ! -e "${closure_root}" && ! -L "${closure_root}" ]] ||
    fail "refusing to overwrite existing interruption closure"
  mv -T -n -- "${staging_root}" "${closure_root}"
  if [[ -e "${staging_root}" || -L "${staging_root}" ]]; then
    fail "atomic no-clobber closure publication did not consume staging"
  fi
  candidate_created_by_process=false
}

assert_source_lock_identity() {
  [[ -n "${source_lock_fd}" && -n "${source_lock_device}" && \
    -n "${source_lock_inode}" ]] ||
    fail "retry1 development lock identity is not initialized"
  [[ -e "/proc/self/fd/${source_lock_fd}" ]] ||
    fail "retry1 development lock file descriptor is closed"
  reject_symlink_components "${source_lock_path}"
  require_file "${source_lock_path}"
  [[ "$(stat -c '%h' -- "${source_lock_path}")" == 1 ]] ||
    fail "retry1 development lock has an unexpected hard-link count"

  local fd_device fd_inode path_device path_inode
  fd_device="$(stat -L -c '%d' -- "/proc/self/fd/${source_lock_fd}")"
  fd_inode="$(stat -L -c '%i' -- "/proc/self/fd/${source_lock_fd}")"
  path_device="$(stat -c '%d' -- "${source_lock_path}")"
  path_inode="$(stat -c '%i' -- "${source_lock_path}")"
  [[ "${fd_device}" == "${source_lock_device}" && \
    "${fd_inode}" == "${source_lock_inode}" ]] ||
    fail "retry1 development lock file-descriptor identity drifted"
  [[ "${path_device}" == "${source_lock_device}" && \
    "${path_inode}" == "${source_lock_inode}" ]] ||
    fail "retry1 development lock path no longer names the locked inode"
}

acquire_source_runtime_lock() {
  reject_symlink_components "${source_lock_path}"
  require_file "${source_lock_path}"
  [[ "$(stat -c '%h' -- "${source_lock_path}")" == 1 ]] ||
    fail "retry1 development lock has an unexpected hard-link count"
  exec {source_lock_fd}<"${source_lock_path}"
  flock -n "${source_lock_fd}" ||
    fail "retry1 development lock is held; refusing to inspect"
  source_lock_device="$(stat -L -c '%d' -- \
    "/proc/self/fd/${source_lock_fd}")"
  source_lock_inode="$(stat -L -c '%i' -- \
    "/proc/self/fd/${source_lock_fd}")"
  assert_source_lock_identity
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

acquire_source_runtime_lock

snapshot_1_regular=""
snapshot_1_directories=""
snapshot_2_regular=""
snapshot_2_directories=""
new_temp_file snapshot_1_regular "/tmp/${schema_id}.regular.snapshot1.XXXXXX"
new_temp_file snapshot_1_directories \
  "/tmp/${schema_id}.directories.snapshot1.XXXXXX"
new_temp_file snapshot_2_regular "/tmp/${schema_id}.regular.snapshot2.XXXXXX"
new_temp_file snapshot_2_directories \
  "/tmp/${schema_id}.directories.snapshot2.XXXXXX"

take_source_snapshot "${snapshot_1_regular}" "${snapshot_1_directories}"
verify_retry1_boundary

if [[ "${mode}" == --audit ]]; then
  take_source_snapshot "${snapshot_2_regular}" "${snapshot_2_directories}"
  cmp -s -- "${snapshot_1_regular}" "${snapshot_2_regular}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${snapshot_1_directories}" "${snapshot_2_directories}" ||
    fail "retry1 directory snapshots differ"
  prospective_receipt_1=""
  prospective_receipt_2=""
  new_temp_file prospective_receipt_1 \
    "/tmp/${schema_id}.receipt.audit1.XXXXXX"
  new_temp_file prospective_receipt_2 \
    "/tmp/${schema_id}.receipt.audit2.XXXXXX"
  emit_receipt "${prospective_receipt_1}" "${snapshot_1_regular}" \
    "${snapshot_1_directories}" "${script_path}" "${amendment_path}"
  emit_receipt "${prospective_receipt_2}" "${snapshot_2_regular}" \
    "${snapshot_2_directories}" "${script_path}" "${amendment_path}"
  verify_receipt_key_shape "${prospective_receipt_1}"
  verify_receipt_key_shape "${prospective_receipt_2}"
  cmp -s -- "${prospective_receipt_1}" "${prospective_receipt_2}" ||
    fail "prospective interruption closure receipts differ"
  echo "retry1_interruption_audit=complete"
  echo "source_content_inventory_sha256=${expected_content_inventory_sha256}"
  echo "source_snapshots_identical=true"
  echo "prospective_receipt_shape=valid_unique_keys"
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  [[ -d "${closure_root}" && ! -L "${closure_root}" ]] ||
    fail "interruption closure is not published: ${closure_root}"
  verify_closure_tree_at "${closure_root}" \
    "${snapshot_1_regular}" "${snapshot_1_directories}"
  verify_retry1_boundary
  take_source_snapshot "${snapshot_2_regular}" "${snapshot_2_directories}"
  cmp -s -- "${snapshot_1_regular}" "${snapshot_2_regular}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${snapshot_1_directories}" "${snapshot_2_directories}" ||
    fail "retry1 directory snapshots differ"
  verify_closure_tree_at "${closure_root}" \
    "${snapshot_2_regular}" "${snapshot_2_directories}"
  echo "interruption_closure_receipt=${receipt_path}"
  echo "interruption_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
  exit 0
fi

if [[ -e "${closure_root}" || -L "${closure_root}" ]]; then
  verify_closure_tree_at "${closure_root}" \
    "${snapshot_1_regular}" "${snapshot_1_directories}"
  verify_retry1_boundary
  take_source_snapshot "${snapshot_2_regular}" "${snapshot_2_directories}"
  cmp -s -- "${snapshot_1_regular}" "${snapshot_2_regular}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${snapshot_1_directories}" "${snapshot_2_directories}" ||
    fail "retry1 directory snapshots differ"
  verify_closure_tree_at "${closure_root}" \
    "${snapshot_2_regular}" "${snapshot_2_directories}"
  echo "interruption_closure_receipt=${receipt_path}"
  echo "interruption_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
  exit 0
fi

build_staging_closure "${snapshot_1_regular}" "${snapshot_1_directories}"
verify_retry1_boundary
take_source_snapshot "${snapshot_2_regular}" "${snapshot_2_directories}"
cmp -s -- "${snapshot_1_regular}" "${snapshot_2_regular}" ||
  fail "retry1 regular-file snapshots differ"
cmp -s -- "${snapshot_1_directories}" "${snapshot_2_directories}" ||
  fail "retry1 directory snapshots differ"
verify_closure_tree_at "${staging_root}" \
  "${snapshot_2_regular}" "${snapshot_2_directories}"
publish_staging_closure
verify_closure_tree_at "${closure_root}" \
  "${snapshot_2_regular}" "${snapshot_2_directories}"
echo "interruption_closure_receipt=${receipt_path}"
echo "interruption_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
