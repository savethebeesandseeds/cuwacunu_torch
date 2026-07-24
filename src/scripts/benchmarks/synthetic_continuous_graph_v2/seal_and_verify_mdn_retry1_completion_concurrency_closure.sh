#!/usr/bin/env bash
set -euo pipefail
umask 077
export LC_ALL=C

readonly closure_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.completion_concurrency_closure.v1"
readonly closure_runtime_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"
readonly retry_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1"
readonly retry_result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"

readonly expected_initial_loaded_publisher_sha256="3ccefd6952c0e2d1e283334f95d42cb8ee47f92412e635a152bd8631c1054c1c"
readonly expected_final_sealer_sha256="88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0"
readonly expected_erratum_sha256="c00b5f842237b1aada5490783ee80023de2ac1c5f3d7a0224f46caf295c8cad9"
readonly expected_completion_correction_sha256="ab4ceb9a7d1e6d55c6830b3263abfbfe60225399283ef63673176c11d4ebc5d9"
readonly expected_result_sha256="d9eeddb89be7f2313083f4ea375bbf8c7f4168c95d15c4dbc216eadd009c1d93"
readonly expected_payload_inventory_master_sha256="43b36342443cd37d78d2639618ea87f8c8c6f43dce24e0ef4a73cdc9a6399f50"
readonly expected_checkpoint_sha256="a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f"
readonly expected_execution_runner_sha256="93c477a6e4de3ddfbded94ca8f22db14cd7954800a319713c0a7961ccf2bb799"
readonly expected_empty_sha256="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
readonly expected_result_bytes=13863
readonly expected_final_sealer_bytes=75702
readonly expected_retry_directory_count=7
readonly expected_retry_file_count=24
readonly expected_closure_directory_count=1
readonly expected_closure_file_count=10

fail() {
  echo "synthetic v2 MDN retry1 concurrency closure: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

sha256_stream() {
  sha256sum | awk '{print $1}'
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

verify_hash() {
  local path="$1" expected="$2" label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
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
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

verify_exact_kv_keys() {
  local path="$1"
  shift
  local expected actual
  expected="$(printf '%s\n' "$@" | sort)"
  actual="$(awk '
    NF == 0 { next }
    {
      eq = index($0, "=");
      if (eq == 0) {
        print "__MALFORMED_LINE__" NR;
        next;
      }
      key = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", key);
      if (key == "" || key ~ /[[:space:]]/) {
        print "__MALFORMED_KEY__" NR;
        next;
      }
      print key;
    }
  ' "${path}" | sort)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: exact keyset mismatch"
}

expect_mode_links() {
  local path="$1" mode="$2" links="$3" label="$4"
  [[ "$(stat -c '%a' -- "${path}")" == "${mode}" ]] ||
    fail "${label} is not mode 0${mode}: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == "${links}" ]] ||
    fail "${label} has unexpected link count: ${path}"
}

readonly script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
readonly script_dir="$(dirname "${script_path}")"
readonly repo_root="$(realpath -e -- "${script_dir}/../../../..")"
readonly benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
readonly runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
readonly retry_runtime_root="${runtime_parent}/${retry_schema_id}"
readonly closure_runtime_root="${runtime_parent}/${closure_runtime_schema_id}"

readonly final_sealer="${script_dir}/seal_and_verify_mdn_retry1_completed_job.sh"
readonly erratum="${benchmark_root}/MDN_RETRY1_SEAL_CONCURRENCY_ERRATUM.md"
readonly completion_correction="${benchmark_root}/MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md"
readonly execution_runner="${script_dir}/run_mdn_train_isolated_v2_retry1.sh"
readonly retry_result="${retry_runtime_root}/result.status"
readonly checkpoint="${retry_runtime_root}/job/channel_inference.report.channel_mdn.pt"

readonly closure_lock="${closure_runtime_root}/.closure.lock"
readonly forensic_snapshot="${closure_runtime_root}/pre_adoption_forensic.status"
readonly reconstructed_publisher="${closure_runtime_root}/reconstructed_initial_publisher.3cce.sh"
readonly content_pre="${closure_runtime_root}/content.pre.sha256"
readonly attempt_marker="${closure_runtime_root}/attempt.started"
readonly adoption_log="${closure_runtime_root}/final_sealer_adoption.log"
readonly verify_log="${closure_runtime_root}/final_sealer_verify.log"
readonly content_post="${closure_runtime_root}/content.post.sha256"
readonly success_status="${closure_runtime_root}/success.status"
readonly closure_receipt="${closure_runtime_root}/completion_concurrency.status"

readonly process_started_at_utc="$(date -u +'%Y-%m-%dT%H:%M:%S.%NZ')"
readonly process_start_wrapper_sha256="$(sha256_of "${script_path}")"
readonly process_start_wrapper_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_wrapper_device="$(stat -c '%d' -- "${script_path}")"
readonly process_start_wrapper_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_final_sealer_inode="$(stat -c '%i' -- "${final_sealer}")"
readonly process_start_final_sealer_device="$(stat -c '%d' -- "${final_sealer}")"
readonly process_start_result_inode="$(stat -c '%i' -- "${retry_result}")"
readonly process_start_result_device="$(stat -c '%d' -- "${retry_result}")"
readonly process_start_runtime_inode="$(stat -c '%i' -- "${retry_runtime_root}")"
readonly process_start_runtime_device="$(stat -c '%d' -- "${retry_runtime_root}")"

readonly -a retry_directories=(
  "."
  "components"
  "components/wikimyei.inference.expected_value.mdn"
  "components/wikimyei.inference.expected_value.mdn/spawns"
  "components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb"
  "job"
  "system"
)

readonly -a retry_files=(
  ".execution.lock"
  "components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"
  "inputs.status"
  "job/channel_inference.report"
  "job/channel_inference.report.channel_mdn.pt"
  "job/channel_inference.report.mdn.lls"
  "job/channel_inference.report.nodelift.lls"
  "job/channel_inference.report.representation.lls"
  "job/job.manifest"
  "job/job.state"
  "job/lattice.checkpoint.fact"
  "job/lattice.exposure.fact"
  "job/lattice.source_analytics.fact"
  "job/representation_edge_features.probe"
  "job/runtime.checkpoint_io.fact"
  "job/runtime.component_training_update.fact"
  "job/runtime.health_measurement.fact"
  "job/runtime.job_events.probe"
  "job/runtime.result.fact"
  "mdn_train.retry1.log"
  "result.status"
  "synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
  "system/component_spawn_registry.v1.lls"
  "system/runtime_layout.v1.lls"
)

readonly -a closure_files=(
  ".closure.lock"
  "attempt.started"
  "completion_concurrency.status"
  "content.post.sha256"
  "content.pre.sha256"
  "final_sealer_adoption.log"
  "final_sealer_verify.log"
  "pre_adoption_forensic.status"
  "reconstructed_initial_publisher.3cce.sh"
  "success.status"
)

artifact_stage_path() {
  local leaf="$1"
  printf '%s/.%s.%s.candidate' \
    "${runtime_parent}" "${closure_runtime_schema_id}" "${leaf}"
}

assert_staging_namespace_contains_only_known_names() {
  local entry rel known
  while IFS= read -r -d '' entry; do
    known=false
    for rel in "${closure_files[@]}"; do
      [[ "${rel}" == .closure.lock ]] && continue
      if [[ "${entry}" == "$(artifact_stage_path "${rel}")" ]]; then
        known=true
        break
      fi
    done
    [[ "${known}" == true ]] ||
      fail "unknown deterministic closure staging residue: ${entry}"
  done < <(find "${runtime_parent}" -mindepth 1 -maxdepth 1 \
    -name ".${closure_runtime_schema_id}.*.candidate" -print0)
}

assert_staging_namespace_empty() {
  local residue
  residue="$(find "${runtime_parent}" -mindepth 1 -maxdepth 1 \
    -name ".${closure_runtime_schema_id}.*.candidate" -print -quit)"
  [[ -z "${residue}" ]] ||
    fail "closure staging namespace is not empty: ${residue}"
}

assert_source_identity_guards() {
  reject_symlink_components "${script_path}"
  reject_symlink_components "${final_sealer}"
  reject_symlink_components "${erratum}"
  reject_symlink_components "${completion_correction}"
  reject_symlink_components "${execution_runner}"
  reject_symlink_components "${retry_result}"
  reject_symlink_components "${retry_runtime_root}"
  require_file "${script_path}"
  require_file "${final_sealer}"
  require_file "${erratum}"
  require_file "${completion_correction}"
  require_file "${execution_runner}"
  require_file "${retry_result}"
  require_dir "${retry_runtime_root}"

  [[ "$(sha256_of "${script_path}")" == "${process_start_wrapper_sha256}" ]] ||
    fail "closure wrapper changed after process start"
  [[ "$(stat -c '%i' -- "${script_path}")" == "${process_start_wrapper_inode}" ]] ||
    fail "closure wrapper inode changed after process start"
  [[ "$(stat -c '%d' -- "${script_path}")" == "${process_start_wrapper_device}" ]] ||
    fail "closure wrapper device changed after process start"
  [[ "$(stat -c '%s' -- "${script_path}")" == "${process_start_wrapper_bytes}" ]] ||
    fail "closure wrapper size changed after process start"
  expect_mode_links "${script_path}" 555 1 "closure wrapper"

  verify_hash "${final_sealer}" "${expected_final_sealer_sha256}" \
    "final completion sealer"
  [[ "$(stat -c '%i' -- "${final_sealer}")" == "${process_start_final_sealer_inode}" ]] ||
    fail "final completion sealer inode changed after process start"
  [[ "$(stat -c '%d' -- "${final_sealer}")" == "${process_start_final_sealer_device}" ]] ||
    fail "final completion sealer device changed after process start"
  [[ "$(stat -c '%s' -- "${final_sealer}")" == "${expected_final_sealer_bytes}" ]] ||
    fail "final completion sealer size drifted"
  expect_mode_links "${final_sealer}" 555 1 "final completion sealer"

  verify_hash "${erratum}" "${expected_erratum_sha256}" "concurrency erratum"
  expect_mode_links "${erratum}" 444 1 "concurrency erratum"
  verify_hash "${completion_correction}" \
    "${expected_completion_correction_sha256}" "completion correction"
  expect_mode_links "${completion_correction}" 444 1 "completion correction"
  verify_hash "${execution_runner}" "${expected_execution_runner_sha256}" \
    "retry execution runner"

  verify_hash "${retry_result}" "${expected_result_sha256}" \
    "retry completion result"
  [[ "$(stat -c '%i' -- "${retry_result}")" == "${process_start_result_inode}" ]] ||
    fail "retry completion result inode changed after process start"
  [[ "$(stat -c '%d' -- "${retry_result}")" == "${process_start_result_device}" ]] ||
    fail "retry completion result device changed after process start"
  [[ "$(stat -c '%s' -- "${retry_result}")" == "${expected_result_bytes}" ]] ||
    fail "retry completion result size drifted"
  expect_mode_links "${retry_result}" 444 1 "retry completion result"
  [[ "$(stat -c '%i' -- "${retry_runtime_root}")" == "${process_start_runtime_inode}" ]] ||
    fail "retry runtime root inode changed after process start"
  [[ "$(stat -c '%d' -- "${retry_runtime_root}")" == "${process_start_runtime_device}" ]] ||
    fail "retry runtime root device changed after process start"
  [[ "$(stat -c '%a' -- "${retry_runtime_root}")" == 555 ]] ||
    fail "retry runtime root is not mode 0555"
}

verify_retry_result_bindings() {
  expect_kv "${retry_result}" schema_id "${retry_result_schema_id}"
  expect_kv "${retry_result}" status complete
  expect_kv "${retry_result}" completion_sealer_path "${final_sealer}"
  expect_kv "${retry_result}" completion_sealer_sha256 \
    "${expected_final_sealer_sha256}"
  expect_kv "${retry_result}" completion_correction_path \
    "${completion_correction}"
  expect_kv "${retry_result}" completion_correction_sha256 \
    "${expected_completion_correction_sha256}"
  expect_kv "${retry_result}" payload_inventory_master_sha256 \
    "${expected_payload_inventory_master_sha256}"
  expect_kv "${retry_result}" checkpoint_path "${checkpoint}"
  expect_kv "${retry_result}" checkpoint_sha256 \
    "${expected_checkpoint_sha256}"
  expect_kv "${retry_result}" execution_runner_path "${execution_runner}"
  expect_kv "${retry_result}" execution_runner_sha256 \
    "${expected_execution_runner_sha256}"
  expect_kv "${retry_result}" canonical_data_raw_access false
  expect_kv "${retry_result}" final_holdout_available false
  expect_kv "${retry_result}" independent_final_evidence false
  expect_kv "${retry_result}" policy_access false
}

emit_expected_retry_directories() {
  printf '%s\n' "${retry_directories[@]}" | sort
}

emit_actual_retry_directories() {
  printf '.\n'
  find "${retry_runtime_root}" -mindepth 1 -type d -printf '%P\n' | sort
}

emit_expected_retry_files() {
  printf '%s\n' "${retry_files[@]}" | sort
}

emit_actual_retry_files() {
  find "${retry_runtime_root}" -type f -printf '%P\n' | sort
}

verify_exact_retry_tree() {
  local special writable rel path
  reject_symlink_components "${retry_runtime_root}"
  require_dir "${retry_runtime_root}"
  [[ "$(realpath -e -- "${retry_runtime_root}")" == "${retry_runtime_root}" ]] ||
    fail "retry runtime root is not canonical"
  [[ "$(emit_expected_retry_directories)" == "$(emit_actual_retry_directories)" ]] ||
    fail "retry runtime exact directory tree mismatch"
  [[ "$(emit_expected_retry_files)" == "$(emit_actual_retry_files)" ]] ||
    fail "retry runtime exact file tree mismatch"
  special="$(find "${retry_runtime_root}" ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "retry runtime contains a special entry: ${special}"
  writable="$(find "${retry_runtime_root}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] || fail "retry runtime contains a writable entry: ${writable}"

  [[ "${#retry_directories[@]}" == "${expected_retry_directory_count}" ]] ||
    fail "internal retry directory count is invalid"
  for rel in "${retry_directories[@]}"; do
    path="${retry_runtime_root}"
    [[ "${rel}" == . ]] || path="${retry_runtime_root}/${rel}"
    require_dir "${path}"
    [[ "$(stat -c '%a' -- "${path}")" == 555 ]] ||
      fail "retry directory is not mode 0555: ${rel}"
  done

  [[ "${#retry_files[@]}" == "${expected_retry_file_count}" ]] ||
    fail "internal retry file count is invalid"
  for rel in "${retry_files[@]}"; do
    path="${retry_runtime_root}/${rel}"
    require_file "${path}"
    expect_mode_links "${path}" 444 1 "retry file ${rel}"
  done
}

emit_retry_content_inventory() {
  local rel path
  for rel in "${retry_files[@]}"; do
    path="${retry_runtime_root}/${rel}"
    printf '%s %s %s %s %s\n' \
      "$(sha256_of "${path}")" \
      "$(stat -c '%s' -- "${path}")" \
      "$(stat -c '%a' -- "${path}")" \
      "$(stat -c '%h' -- "${path}")" \
      "${rel}"
  done | sort -k5,5
}

emit_retry_file_inode_identity() {
  local rel path
  for rel in "${retry_files[@]}"; do
    path="${retry_runtime_root}/${rel}"
    printf '%s %s %s\n' \
      "$(stat -c '%d' -- "${path}")" \
      "$(stat -c '%i' -- "${path}")" \
      "${rel}"
  done | sort -k3,3
}

emit_retry_directory_identity() {
  local rel path
  for rel in "${retry_directories[@]}"; do
    path="${retry_runtime_root}"
    [[ "${rel}" == . ]] || path="${retry_runtime_root}/${rel}"
    printf '%s %s %s %s %s\n' \
      "$(stat -c '%a' -- "${path}")" \
      "$(stat -c '%h' -- "${path}")" \
      "$(stat -c '%d' -- "${path}")" \
      "$(stat -c '%i' -- "${path}")" \
      "${rel}"
  done | sort -k5,5
}

verify_content_inventory_file() {
  local inventory="$1" current_check="${2:-true}" line_count payload_master
  require_file "${inventory}"
  expect_mode_links "${inventory}" 444 1 "retry content inventory"
  line_count="$(wc -l <"${inventory}")"
  [[ "${line_count}" == "${expected_retry_file_count}" ]] ||
    fail "${inventory}: expected ${expected_retry_file_count} inventory rows"
  awk '
    NF != 5 { exit 41 }
    length($1) != 64 || $1 !~ /^[0-9a-f]+$/ { exit 42 }
    $2 !~ /^[0-9]+$/ { exit 43 }
    $3 != "444" { exit 44 }
    $4 != "1" { exit 45 }
    seen[$5]++ { exit 46 }
  ' "${inventory}" || fail "${inventory}: malformed content inventory"
  [[ "$(awk '{print $5}' "${inventory}")" == "$(emit_expected_retry_files)" ]] ||
    fail "${inventory}: inventory path set/order mismatch"
  if [[ "${current_check}" == true ]]; then
    cmp -s -- <(emit_retry_content_inventory) "${inventory}" ||
      fail "${inventory}: current retry content identity drifted"
  fi
  payload_master="$(awk '$5 != "result.status" { print $1 " " $2 " " $5 }' \
    "${inventory}" | sha256_stream)"
  [[ "${payload_master}" == "${expected_payload_inventory_master_sha256}" ]] ||
    fail "${inventory}: 23-file payload inventory master drifted"
  [[ "$(awk '$5 == "result.status" {print $1}' "${inventory}")" == \
    "${expected_result_sha256}" ]] || fail "${inventory}: result hash drifted"
  [[ "$(awk '$5 == "result.status" {print $2}' "${inventory}")" == \
    "${expected_result_bytes}" ]] || fail "${inventory}: result size drifted"
  [[ "$(awk '$5 == "job/channel_inference.report.channel_mdn.pt" {print $1}' \
    "${inventory}")" == "${expected_checkpoint_sha256}" ]] ||
    fail "${inventory}: checkpoint hash drifted"
}

emit_reconstructed_initial_publisher() {
  awk '
    {
      if ($0 == "  rm -f -- \"${staging_path}\"") {
        first = $0;
        if ((getline second) <= 0 || (getline third) <= 0) exit 81;
        if (second == "  chmod 0555 -- \"${runtime_root}\"" &&
            third == "  verify_exact_receipt \"${result_receipt}\"") {
          matches += 1;
          print first;
          print third;
          print second;
          next;
        }
        print first;
        print second;
        print third;
        next;
      }
      print;
    }
    END {
      if (matches != 1) exit 82;
    }
  ' "${final_sealer}"
}

verify_reconstructed_publisher() {
  require_file "${reconstructed_publisher}"
  expect_mode_links "${reconstructed_publisher}" 444 1 \
    "reconstructed initial publisher snapshot"
  [[ "$(sha256_of "${reconstructed_publisher}")" == \
    "${expected_initial_loaded_publisher_sha256}" ]] ||
    fail "reconstructed initial publisher hash proof failed"
  cmp -s -- <(emit_reconstructed_initial_publisher) \
    "${reconstructed_publisher}" ||
    fail "reconstructed initial publisher bytes are not the unique line-order reversal"
}

readonly -a forensic_keys=(
  schema_id status snapshot_phase erratum_path erratum_sha256
  initial_loaded_publisher_logical_path initial_loaded_publisher_sha256
  initial_loaded_publisher_is_current_path_hash_pair
  reconstructed_publisher_snapshot_path reconstructed_publisher_expected_sha256
  reconstructed_publisher_executable_authority
  receipt_emission_disk_sealer_path receipt_emission_disk_sealer_sha256
  incident_final_sealer_inode incident_final_sealer_device
  incident_final_sealer_mode incident_final_sealer_links
  incident_final_sealer_bytes incident_final_sealer_mtime
  incident_final_sealer_ctime incident_result_inode incident_result_device
  incident_result_mode incident_result_links incident_result_bytes
  incident_result_mtime incident_result_ctime incident_runtime_root_inode
  incident_runtime_root_device incident_runtime_root_mode
  incident_runtime_root_mtime incident_runtime_root_ctime
  incident_checkpoint_inode incident_checkpoint_device
  incident_checkpoint_mode incident_checkpoint_links incident_checkpoint_bytes
  incident_checkpoint_mtime incident_checkpoint_ctime
  launch_process_table_stime_utc launch_time_precision
  supervising_tool_cell_terminated docker_exec_continued_after_tool_termination
  procfs_scan_at_utc procfs_scan_self_excluding
  procfs_scan_matching_processes process_evidence_scope
  current_final_sealer_path current_final_sealer_sha256
  current_final_sealer_inode current_final_sealer_device
  current_final_sealer_mode current_final_sealer_links
  current_final_sealer_bytes current_final_sealer_mtime
  current_final_sealer_ctime current_closure_wrapper_path
  current_closure_wrapper_process_start_sha256 current_closure_wrapper_inode
  current_closure_wrapper_device current_closure_wrapper_mode
  current_closure_wrapper_links current_closure_wrapper_bytes
  current_closure_wrapper_mtime current_closure_wrapper_ctime
  retry_result_path retry_result_sha256 retry_result_inode
  retry_result_device retry_result_mode retry_result_links retry_result_bytes
  retry_runtime_root retry_runtime_root_inode retry_runtime_root_device
  retry_runtime_root_mode retry_directory_count retry_file_count
  retry_directory_identity_sha256 retry_file_inode_identity_sha256
  content_identity_fields content_identity_excludes_ctime
  authorized_adoption_mutation content_bytes_paths_inodes_must_remain_unchanged
  training_reexecution_authorized canonical_data_raw_access
  independent_final_evidence policy_access
)

emit_pre_adoption_forensic() {
  local destination="$1"
  cat >"${destination}" <<FORENSIC
schema_id=${closure_schema_id}.pre_adoption_forensic.v1
status=frozen
snapshot_phase=before_authoritative_final_sealer_adoption
erratum_path=${erratum}
erratum_sha256=${expected_erratum_sha256}
initial_loaded_publisher_logical_path=${final_sealer}
initial_loaded_publisher_sha256=${expected_initial_loaded_publisher_sha256}
initial_loaded_publisher_is_current_path_hash_pair=false
reconstructed_publisher_snapshot_path=${reconstructed_publisher}
reconstructed_publisher_expected_sha256=${expected_initial_loaded_publisher_sha256}
reconstructed_publisher_executable_authority=false
receipt_emission_disk_sealer_path=${final_sealer}
receipt_emission_disk_sealer_sha256=${expected_final_sealer_sha256}
incident_final_sealer_inode=130885864170461293
incident_final_sealer_device=66
incident_final_sealer_mode=0755
incident_final_sealer_links=1
incident_final_sealer_bytes=75702
incident_final_sealer_mtime=2026-07-21 14:17:23.739143400 +0000
incident_final_sealer_ctime=2026-07-21 14:17:23.739143400 +0000
incident_result_inode=79094468456190506
incident_result_device=66
incident_result_mode=0444
incident_result_links=1
incident_result_bytes=13863
incident_result_mtime=2026-07-21 14:18:19.686483500 +0000
incident_result_ctime=2026-07-21 14:19:34.648242700 +0000
incident_runtime_root_inode=9851624185335778
incident_runtime_root_device=66
incident_runtime_root_mode=0555
incident_runtime_root_mtime=2026-07-21 14:19:34.643710800 +0000
incident_runtime_root_ctime=2026-07-21 14:19:34.693483500 +0000
incident_checkpoint_inode=78250043525577835
incident_checkpoint_device=66
incident_checkpoint_mode=0444
incident_checkpoint_links=1
incident_checkpoint_bytes=3228076
incident_checkpoint_mtime=2026-07-21 12:32:40.727424400 +0000
incident_checkpoint_ctime=2026-07-21 14:18:19.786856900 +0000
launch_process_table_stime_utc=14:15
launch_time_precision=minute
supervising_tool_cell_terminated=true
docker_exec_continued_after_tool_termination=true
procfs_scan_at_utc=2026-07-21T14:30:04.623927172Z
procfs_scan_self_excluding=true
procfs_scan_matching_processes=none
process_evidence_scope=external_operational_not_runtime_exit_or_model_evidence
current_final_sealer_path=${final_sealer}
current_final_sealer_sha256=${expected_final_sealer_sha256}
current_final_sealer_inode=$(stat -c '%i' -- "${final_sealer}")
current_final_sealer_device=$(stat -c '%d' -- "${final_sealer}")
current_final_sealer_mode=0$(stat -c '%a' -- "${final_sealer}")
current_final_sealer_links=$(stat -c '%h' -- "${final_sealer}")
current_final_sealer_bytes=$(stat -c '%s' -- "${final_sealer}")
current_final_sealer_mtime=$(stat -c '%y' -- "${final_sealer}")
current_final_sealer_ctime=$(stat -c '%z' -- "${final_sealer}")
current_closure_wrapper_path=${script_path}
current_closure_wrapper_process_start_sha256=${process_start_wrapper_sha256}
current_closure_wrapper_inode=${process_start_wrapper_inode}
current_closure_wrapper_device=${process_start_wrapper_device}
current_closure_wrapper_mode=0$(stat -c '%a' -- "${script_path}")
current_closure_wrapper_links=$(stat -c '%h' -- "${script_path}")
current_closure_wrapper_bytes=${process_start_wrapper_bytes}
current_closure_wrapper_mtime=$(stat -c '%y' -- "${script_path}")
current_closure_wrapper_ctime=$(stat -c '%z' -- "${script_path}")
retry_result_path=${retry_result}
retry_result_sha256=${expected_result_sha256}
retry_result_inode=${process_start_result_inode}
retry_result_device=${process_start_result_device}
retry_result_mode=0$(stat -c '%a' -- "${retry_result}")
retry_result_links=$(stat -c '%h' -- "${retry_result}")
retry_result_bytes=$(stat -c '%s' -- "${retry_result}")
retry_runtime_root=${retry_runtime_root}
retry_runtime_root_inode=${process_start_runtime_inode}
retry_runtime_root_device=${process_start_runtime_device}
retry_runtime_root_mode=0$(stat -c '%a' -- "${retry_runtime_root}")
retry_directory_count=${expected_retry_directory_count}
retry_file_count=${expected_retry_file_count}
retry_directory_identity_sha256=$(emit_retry_directory_identity | sha256_stream)
retry_file_inode_identity_sha256=$(emit_retry_file_inode_identity | sha256_stream)
content_identity_fields=sha256_bytes_mode_links_relative_path
content_identity_excludes_ctime=true
authorized_adoption_mutation=idempotent_chmod_only_may_advance_ctime
content_bytes_paths_inodes_must_remain_unchanged=true
training_reexecution_authorized=false
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
FORENSIC
}

verify_pre_adoption_forensic() {
  require_file "${forensic_snapshot}"
  expect_mode_links "${forensic_snapshot}" 444 1 "pre-adoption forensic snapshot"
  verify_exact_kv_keys "${forensic_snapshot}" "${forensic_keys[@]}"
  expect_kv "${forensic_snapshot}" schema_id \
    "${closure_schema_id}.pre_adoption_forensic.v1"
  expect_kv "${forensic_snapshot}" status frozen
  expect_kv "${forensic_snapshot}" snapshot_phase \
    before_authoritative_final_sealer_adoption
  expect_kv "${forensic_snapshot}" erratum_path "${erratum}"
  expect_kv "${forensic_snapshot}" erratum_sha256 "${expected_erratum_sha256}"
  expect_kv "${forensic_snapshot}" initial_loaded_publisher_logical_path \
    "${final_sealer}"
  expect_kv "${forensic_snapshot}" initial_loaded_publisher_sha256 \
    "${expected_initial_loaded_publisher_sha256}"
  expect_kv "${forensic_snapshot}" \
    initial_loaded_publisher_is_current_path_hash_pair false
  expect_kv "${forensic_snapshot}" reconstructed_publisher_snapshot_path \
    "${reconstructed_publisher}"
  expect_kv "${forensic_snapshot}" reconstructed_publisher_expected_sha256 \
    "${expected_initial_loaded_publisher_sha256}"
  expect_kv "${forensic_snapshot}" reconstructed_publisher_executable_authority false
  expect_kv "${forensic_snapshot}" receipt_emission_disk_sealer_path \
    "${final_sealer}"
  expect_kv "${forensic_snapshot}" receipt_emission_disk_sealer_sha256 \
    "${expected_final_sealer_sha256}"

  expect_kv "${forensic_snapshot}" incident_final_sealer_inode 130885864170461293
  expect_kv "${forensic_snapshot}" incident_final_sealer_device 66
  expect_kv "${forensic_snapshot}" incident_final_sealer_mode 0755
  expect_kv "${forensic_snapshot}" incident_final_sealer_links 1
  expect_kv "${forensic_snapshot}" incident_final_sealer_bytes 75702
  expect_kv "${forensic_snapshot}" incident_final_sealer_mtime \
    "2026-07-21 14:17:23.739143400 +0000"
  expect_kv "${forensic_snapshot}" incident_final_sealer_ctime \
    "2026-07-21 14:17:23.739143400 +0000"
  expect_kv "${forensic_snapshot}" incident_result_inode 79094468456190506
  expect_kv "${forensic_snapshot}" incident_result_device 66
  expect_kv "${forensic_snapshot}" incident_result_mode 0444
  expect_kv "${forensic_snapshot}" incident_result_links 1
  expect_kv "${forensic_snapshot}" incident_result_bytes 13863
  expect_kv "${forensic_snapshot}" incident_result_mtime \
    "2026-07-21 14:18:19.686483500 +0000"
  expect_kv "${forensic_snapshot}" incident_result_ctime \
    "2026-07-21 14:19:34.648242700 +0000"
  expect_kv "${forensic_snapshot}" incident_runtime_root_inode 9851624185335778
  expect_kv "${forensic_snapshot}" incident_runtime_root_device 66
  expect_kv "${forensic_snapshot}" incident_runtime_root_mode 0555
  expect_kv "${forensic_snapshot}" incident_runtime_root_mtime \
    "2026-07-21 14:19:34.643710800 +0000"
  expect_kv "${forensic_snapshot}" incident_runtime_root_ctime \
    "2026-07-21 14:19:34.693483500 +0000"
  expect_kv "${forensic_snapshot}" incident_checkpoint_inode 78250043525577835
  expect_kv "${forensic_snapshot}" incident_checkpoint_device 66
  expect_kv "${forensic_snapshot}" incident_checkpoint_mode 0444
  expect_kv "${forensic_snapshot}" incident_checkpoint_links 1
  expect_kv "${forensic_snapshot}" incident_checkpoint_bytes 3228076
  expect_kv "${forensic_snapshot}" incident_checkpoint_mtime \
    "2026-07-21 12:32:40.727424400 +0000"
  expect_kv "${forensic_snapshot}" incident_checkpoint_ctime \
    "2026-07-21 14:18:19.786856900 +0000"
  expect_kv "${forensic_snapshot}" launch_process_table_stime_utc 14:15
  expect_kv "${forensic_snapshot}" launch_time_precision minute
  expect_kv "${forensic_snapshot}" supervising_tool_cell_terminated true
  expect_kv "${forensic_snapshot}" docker_exec_continued_after_tool_termination true
  expect_kv "${forensic_snapshot}" procfs_scan_at_utc \
    2026-07-21T14:30:04.623927172Z
  expect_kv "${forensic_snapshot}" procfs_scan_self_excluding true
  expect_kv "${forensic_snapshot}" procfs_scan_matching_processes none
  expect_kv "${forensic_snapshot}" process_evidence_scope \
    external_operational_not_runtime_exit_or_model_evidence

  expect_kv "${forensic_snapshot}" current_final_sealer_path "${final_sealer}"
  expect_kv "${forensic_snapshot}" current_final_sealer_sha256 \
    "${expected_final_sealer_sha256}"
  expect_kv "${forensic_snapshot}" current_final_sealer_inode \
    "${process_start_final_sealer_inode}"
  expect_kv "${forensic_snapshot}" current_final_sealer_device \
    "${process_start_final_sealer_device}"
  expect_kv "${forensic_snapshot}" current_final_sealer_mode 0555
  expect_kv "${forensic_snapshot}" current_final_sealer_links 1
  expect_kv "${forensic_snapshot}" current_final_sealer_bytes \
    "${expected_final_sealer_bytes}"
  [[ -n "$(kv "${forensic_snapshot}" current_final_sealer_mtime)" ]] ||
    fail "forensic snapshot lacks current final sealer mtime"
  [[ -n "$(kv "${forensic_snapshot}" current_final_sealer_ctime)" ]] ||
    fail "forensic snapshot lacks current final sealer ctime"
  expect_kv "${forensic_snapshot}" current_closure_wrapper_path "${script_path}"
  expect_kv "${forensic_snapshot}" current_closure_wrapper_process_start_sha256 \
    "${process_start_wrapper_sha256}"
  expect_kv "${forensic_snapshot}" current_closure_wrapper_inode \
    "${process_start_wrapper_inode}"
  expect_kv "${forensic_snapshot}" current_closure_wrapper_device \
    "${process_start_wrapper_device}"
  expect_kv "${forensic_snapshot}" current_closure_wrapper_mode 0555
  expect_kv "${forensic_snapshot}" current_closure_wrapper_links 1
  expect_kv "${forensic_snapshot}" current_closure_wrapper_bytes \
    "${process_start_wrapper_bytes}"
  expect_kv "${forensic_snapshot}" retry_result_path "${retry_result}"
  expect_kv "${forensic_snapshot}" retry_result_sha256 "${expected_result_sha256}"
  expect_kv "${forensic_snapshot}" retry_result_inode "${process_start_result_inode}"
  expect_kv "${forensic_snapshot}" retry_result_device "${process_start_result_device}"
  expect_kv "${forensic_snapshot}" retry_result_mode 0444
  expect_kv "${forensic_snapshot}" retry_result_links 1
  expect_kv "${forensic_snapshot}" retry_result_bytes "${expected_result_bytes}"
  expect_kv "${forensic_snapshot}" retry_runtime_root "${retry_runtime_root}"
  expect_kv "${forensic_snapshot}" retry_runtime_root_inode \
    "${process_start_runtime_inode}"
  expect_kv "${forensic_snapshot}" retry_runtime_root_device \
    "${process_start_runtime_device}"
  expect_kv "${forensic_snapshot}" retry_runtime_root_mode 0555
  expect_kv "${forensic_snapshot}" retry_directory_count \
    "${expected_retry_directory_count}"
  expect_kv "${forensic_snapshot}" retry_file_count "${expected_retry_file_count}"
  [[ "$(kv "${forensic_snapshot}" retry_directory_identity_sha256)" =~ \
    ^[0-9a-f]{64}$ ]] || fail "forensic directory identity hash is malformed"
  [[ "$(kv "${forensic_snapshot}" retry_file_inode_identity_sha256)" =~ \
    ^[0-9a-f]{64}$ ]] || fail "forensic file inode identity hash is malformed"
  expect_kv "${forensic_snapshot}" content_identity_fields \
    sha256_bytes_mode_links_relative_path
  expect_kv "${forensic_snapshot}" content_identity_excludes_ctime true
  expect_kv "${forensic_snapshot}" authorized_adoption_mutation \
    idempotent_chmod_only_may_advance_ctime
  expect_kv "${forensic_snapshot}" content_bytes_paths_inodes_must_remain_unchanged true
  expect_kv "${forensic_snapshot}" training_reexecution_authorized false
  expect_kv "${forensic_snapshot}" canonical_data_raw_access false
  expect_kv "${forensic_snapshot}" independent_final_evidence false
  expect_kv "${forensic_snapshot}" policy_access false
}

write_reconstructed_publisher() {
  local destination="$1"
  emit_reconstructed_initial_publisher >"${destination}"
}

write_content_pre() {
  local destination="$1"
  emit_retry_content_inventory >"${destination}"
}

write_content_post() {
  local destination="$1"
  emit_retry_content_inventory >"${destination}"
}

candidate_matches_emitter() {
  local candidate="$1" emitter="$2"
  cmp -s -- <("${emitter}" /dev/stdout) "${candidate}"
}

verify_external_candidate() {
  local candidate="$1" emitter="$2" label="$3"
  reject_symlink_components "${candidate}"
  require_file "${candidate}"
  [[ "$(stat -c '%u' -- "${candidate}")" == "$(id -u)" ]] ||
    fail "${label} candidate has unexpected owner"
  expect_mode_links "${candidate}" 444 1 "${label} candidate"
  candidate_matches_emitter "${candidate}" "${emitter}" ||
    fail "${label} deterministic candidate content drifted"
}

publish_static_artifact() {
  local leaf="$1" emitter="$2" label="$3"
  local destination="${closure_runtime_root}/${leaf}"
  local candidate
  candidate="$(artifact_stage_path "${leaf}")"
  reject_symlink_components "${destination}"
  reject_symlink_components "${candidate}"

  if [[ -e "${destination}" || -L "${destination}" ]]; then
    require_file "${destination}"
    if [[ -e "${candidate}" || -L "${candidate}" ]]; then
      require_file "${candidate}"
      [[ "${candidate}" -ef "${destination}" ]] ||
        fail "${label} destination and residue are not the same inode"
      [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
        fail "${label} published residue has unexpected link count"
      candidate_matches_emitter "${candidate}" "${emitter}" ||
        fail "${label} published residue content drifted"
      rm -f -- "${candidate}"
    fi
    expect_mode_links "${destination}" 444 1 "${label}"
    candidate_matches_emitter "${destination}" "${emitter}" ||
      fail "${label} content drifted"
    return
  fi

  if [[ -e "${candidate}" || -L "${candidate}" ]]; then
    verify_external_candidate "${candidate}" "${emitter}" "${label}"
  else
    if ! (set -o noclobber; "${emitter}" "${candidate}"); then
      fail "could not create deterministic ${label} candidate"
    fi
    chmod 0444 -- "${candidate}"
    verify_external_candidate "${candidate}" "${emitter}" "${label}"
  fi

  if ! ln -- "${candidate}" "${destination}"; then
    fail "atomic no-clobber publication failed for ${label}"
  fi
  [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
    fail "${label} publication did not create the expected hard-link pair"
  rm -f -- "${candidate}"
  expect_mode_links "${destination}" 444 1 "${label}"
  candidate_matches_emitter "${destination}" "${emitter}" ||
    fail "${label} content drifted after publication"
}

publish_existing_candidate() {
  local leaf="$1" label="$2"
  local destination="${closure_runtime_root}/${leaf}"
  local candidate
  candidate="$(artifact_stage_path "${leaf}")"
  [[ ! -e "${destination}" && ! -L "${destination}" ]] ||
    fail "refusing to replace existing ${label}"
  reject_symlink_components "${candidate}"
  require_file "${candidate}"
  [[ "$(stat -c '%u' -- "${candidate}")" == "$(id -u)" ]] ||
    fail "${label} candidate has unexpected owner"
  expect_mode_links "${candidate}" 444 1 "${label} candidate"
  if ! ln -- "${candidate}" "${destination}"; then
    fail "atomic no-clobber publication failed for ${label}"
  fi
  [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
    fail "${label} publication did not create the expected hard-link pair"
  rm -f -- "${candidate}"
  expect_mode_links "${destination}" 444 1 "${label}"
}

assert_partial_closure_tree() {
  local entry leaf mode links
  reject_symlink_components "${closure_runtime_root}"
  require_dir "${closure_runtime_root}"
  mode="$(stat -c '%a' -- "${closure_runtime_root}")"
  [[ "${mode}" == 700 || "${mode}" == 555 ]] ||
    fail "closure runtime has unexpected partial-state mode: ${mode}"
  while IFS= read -r -d '' entry; do
    leaf="${entry##*/}"
    case "${leaf}" in
    .closure.lock | attempt.started | completion_concurrency.status | \
      content.post.sha256 | content.pre.sha256 | final_sealer_adoption.log | \
      final_sealer_verify.log | pre_adoption_forensic.status | \
      reconstructed_initial_publisher.3cce.sh | success.status) ;;
    *) fail "unknown entry in closure runtime: ${leaf}" ;;
    esac
    require_file "${entry}"
    links="$(stat -c '%h' -- "${entry}")"
    [[ "${links}" == 1 ]] ||
      fail "closure artifact has an external or unresolved staging link: ${leaf}"
    if [[ "${leaf}" == .closure.lock ]]; then
      mode="$(stat -c '%a' -- "${entry}")"
      [[ "${mode}" == 600 || "${mode}" == 444 ]] ||
        fail "closure lock has unexpected mode: ${mode}"
    else
      [[ "$(stat -c '%a' -- "${entry}")" == 444 ]] ||
        fail "closure artifact is not mode 0444: ${leaf}"
    fi
  done < <(find "${closure_runtime_root}" -mindepth 1 -maxdepth 1 -print0)
}

prepare_closure_lock() {
  reject_symlink_components "${runtime_parent}"
  require_dir "${runtime_parent}"
  [[ "$(realpath -e -- "${runtime_parent}")" == "${runtime_parent}" ]] ||
    fail "runtime parent is not canonical"
  if [[ ! -e "${closure_runtime_root}" && ! -L "${closure_runtime_root}" ]]; then
    mkdir -- "${closure_runtime_root}"
    chmod 0700 -- "${closure_runtime_root}"
  fi
  reject_symlink_components "${closure_runtime_root}"
  require_dir "${closure_runtime_root}"
  [[ "$(realpath -e -- "${closure_runtime_root}")" == \
    "${closure_runtime_root}" ]] || fail "closure runtime is not canonical"

  if [[ ! -e "${closure_lock}" && ! -L "${closure_lock}" ]]; then
    [[ "$(stat -c '%a' -- "${closure_runtime_root}")" == 700 ]] ||
      fail "cannot create missing closure lock in a non-initial closure runtime"
    if ! (set -o noclobber; : >"${closure_lock}"); then
      fail "could not create closure lock without clobber"
    fi
    chmod 0600 -- "${closure_lock}"
  fi
  reject_symlink_components "${closure_lock}"
  require_file "${closure_lock}"
  [[ "$(stat -c '%h' -- "${closure_lock}")" == 1 ]] ||
    fail "closure lock has an external hard link"
  [[ "$(stat -c '%s' -- "${closure_lock}")" == 0 ]] ||
    fail "closure lock is not empty"
  [[ "$(sha256_of "${closure_lock}")" == "${expected_empty_sha256}" ]] ||
    fail "closure lock content drifted"
  exec {closure_lock_fd}<"${closure_lock}"
  flock -n "${closure_lock_fd}" || fail "closure lock is held"
  assert_staging_namespace_contains_only_known_names
  recover_published_candidate_links
  assert_partial_closure_tree
}

readonly -a attempt_keys=(
  schema_id status adoption_attempt_number started_at_utc
  closure_wrapper_path closure_wrapper_process_start_sha256
  closure_wrapper_inode closure_wrapper_device closure_wrapper_mode
  final_sealer_path final_sealer_sha256 final_sealer_inode
  final_sealer_device final_sealer_mode retry_result_path retry_result_sha256
  retry_result_inode retry_result_device erratum_path erratum_sha256
  completion_correction_path completion_correction_sha256
  pre_adoption_forensic_path pre_adoption_forensic_sha256
  reconstructed_publisher_snapshot_path reconstructed_publisher_sha256
  reconstructed_publisher_executable_authority content_pre_path
  content_pre_sha256 retry_file_inode_identity_sha256
  retry_directory_identity_sha256 closure_lock_held
  retry_execution_lock_held_by_wrapper authoritative_seal_child_authorized
  authoritative_verify_child_authorized
)

emit_attempt_marker() {
  local destination="$1" started_at="$2"
  cat >"${destination}" <<ATTEMPT
schema_id=${closure_schema_id}.attempt.v1
status=started
adoption_attempt_number=1
started_at_utc=${started_at}
closure_wrapper_path=${script_path}
closure_wrapper_process_start_sha256=${process_start_wrapper_sha256}
closure_wrapper_inode=${process_start_wrapper_inode}
closure_wrapper_device=${process_start_wrapper_device}
closure_wrapper_mode=0555
final_sealer_path=${final_sealer}
final_sealer_sha256=${expected_final_sealer_sha256}
final_sealer_inode=${process_start_final_sealer_inode}
final_sealer_device=${process_start_final_sealer_device}
final_sealer_mode=0555
retry_result_path=${retry_result}
retry_result_sha256=${expected_result_sha256}
retry_result_inode=${process_start_result_inode}
retry_result_device=${process_start_result_device}
erratum_path=${erratum}
erratum_sha256=${expected_erratum_sha256}
completion_correction_path=${completion_correction}
completion_correction_sha256=${expected_completion_correction_sha256}
pre_adoption_forensic_path=${forensic_snapshot}
pre_adoption_forensic_sha256=$(sha256_of "${forensic_snapshot}")
reconstructed_publisher_snapshot_path=${reconstructed_publisher}
reconstructed_publisher_sha256=${expected_initial_loaded_publisher_sha256}
reconstructed_publisher_executable_authority=false
content_pre_path=${content_pre}
content_pre_sha256=$(sha256_of "${content_pre}")
retry_file_inode_identity_sha256=$(kv "${forensic_snapshot}" retry_file_inode_identity_sha256)
retry_directory_identity_sha256=$(kv "${forensic_snapshot}" retry_directory_identity_sha256)
closure_lock_held=true
retry_execution_lock_held_by_wrapper=false
authoritative_seal_child_authorized=true
authoritative_verify_child_authorized=true
ATTEMPT
}

verify_attempt_marker() {
  local started_at
  require_file "${attempt_marker}"
  expect_mode_links "${attempt_marker}" 444 1 "adoption attempt marker"
  verify_exact_kv_keys "${attempt_marker}" "${attempt_keys[@]}"
  expect_kv "${attempt_marker}" schema_id "${closure_schema_id}.attempt.v1"
  expect_kv "${attempt_marker}" status started
  expect_kv "${attempt_marker}" adoption_attempt_number 1
  started_at="$(kv "${attempt_marker}" started_at_utc)"
  [[ "${started_at}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{9}Z$ ]] ||
    fail "attempt marker has malformed UTC timestamp"
  expect_kv "${attempt_marker}" closure_wrapper_path "${script_path}"
  expect_kv "${attempt_marker}" closure_wrapper_process_start_sha256 \
    "${process_start_wrapper_sha256}"
  expect_kv "${attempt_marker}" closure_wrapper_inode \
    "${process_start_wrapper_inode}"
  expect_kv "${attempt_marker}" closure_wrapper_device \
    "${process_start_wrapper_device}"
  expect_kv "${attempt_marker}" closure_wrapper_mode 0555
  expect_kv "${attempt_marker}" final_sealer_path "${final_sealer}"
  expect_kv "${attempt_marker}" final_sealer_sha256 \
    "${expected_final_sealer_sha256}"
  expect_kv "${attempt_marker}" final_sealer_inode \
    "${process_start_final_sealer_inode}"
  expect_kv "${attempt_marker}" final_sealer_device \
    "${process_start_final_sealer_device}"
  expect_kv "${attempt_marker}" final_sealer_mode 0555
  expect_kv "${attempt_marker}" retry_result_path "${retry_result}"
  expect_kv "${attempt_marker}" retry_result_sha256 "${expected_result_sha256}"
  expect_kv "${attempt_marker}" retry_result_inode "${process_start_result_inode}"
  expect_kv "${attempt_marker}" retry_result_device "${process_start_result_device}"
  expect_kv "${attempt_marker}" erratum_path "${erratum}"
  expect_kv "${attempt_marker}" erratum_sha256 "${expected_erratum_sha256}"
  expect_kv "${attempt_marker}" completion_correction_path \
    "${completion_correction}"
  expect_kv "${attempt_marker}" completion_correction_sha256 \
    "${expected_completion_correction_sha256}"
  expect_kv "${attempt_marker}" pre_adoption_forensic_path \
    "${forensic_snapshot}"
  expect_kv "${attempt_marker}" pre_adoption_forensic_sha256 \
    "$(sha256_of "${forensic_snapshot}")"
  expect_kv "${attempt_marker}" reconstructed_publisher_snapshot_path \
    "${reconstructed_publisher}"
  expect_kv "${attempt_marker}" reconstructed_publisher_sha256 \
    "${expected_initial_loaded_publisher_sha256}"
  expect_kv "${attempt_marker}" reconstructed_publisher_executable_authority false
  expect_kv "${attempt_marker}" content_pre_path "${content_pre}"
  expect_kv "${attempt_marker}" content_pre_sha256 "$(sha256_of "${content_pre}")"
  expect_kv "${attempt_marker}" retry_file_inode_identity_sha256 \
    "$(kv "${forensic_snapshot}" retry_file_inode_identity_sha256)"
  expect_kv "${attempt_marker}" retry_directory_identity_sha256 \
    "$(kv "${forensic_snapshot}" retry_directory_identity_sha256)"
  expect_kv "${attempt_marker}" closure_lock_held true
  expect_kv "${attempt_marker}" retry_execution_lock_held_by_wrapper false
  expect_kv "${attempt_marker}" authoritative_seal_child_authorized true
  expect_kv "${attempt_marker}" authoritative_verify_child_authorized true
}

publish_attempt_marker() {
  local candidate staged_started_at
  candidate="$(artifact_stage_path attempt.started)"
  [[ ! -e "${attempt_marker}" && ! -L "${attempt_marker}" ]] ||
    fail "an adoption attempt marker already exists"
  reject_symlink_components "${candidate}"
  if [[ -e "${candidate}" || -L "${candidate}" ]]; then
    require_file "${candidate}"
    [[ "$(stat -c '%u' -- "${candidate}")" == "$(id -u)" ]] ||
      fail "attempt marker candidate has unexpected owner"
    expect_mode_links "${candidate}" 444 1 "attempt marker candidate"
    verify_exact_kv_keys "${candidate}" "${attempt_keys[@]}"
    staged_started_at="$(kv "${candidate}" started_at_utc)"
    [[ "${staged_started_at}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{9}Z$ ]] ||
      fail "attempt marker candidate has malformed UTC timestamp"
    cmp -s -- <(emit_attempt_marker /dev/stdout "${staged_started_at}") \
      "${candidate}" ||
      fail "refusing foreign or drifted attempt marker candidate residue"
    if ! ln -- "${candidate}" "${attempt_marker}"; then
      fail "atomic no-clobber publication failed for attempt marker residue"
    fi
    rm -f -- "${candidate}"
    verify_attempt_marker
    return
  fi
  if ! (set -o noclobber; emit_attempt_marker "${candidate}" \
    "${process_started_at_utc}"); then
    fail "could not create deterministic attempt marker candidate"
  fi
  chmod 0444 -- "${candidate}"
  [[ "$(stat -c '%u' -- "${candidate}")" == "$(id -u)" ]] ||
    fail "new attempt marker candidate has unexpected owner"
  expect_mode_links "${candidate}" 444 1 "new attempt marker candidate"
  cmp -s -- <(emit_attempt_marker /dev/stdout "${process_started_at_utc}") \
    "${candidate}" || fail "new attempt marker candidate content drifted"
  if ! ln -- "${candidate}" "${attempt_marker}"; then
    fail "atomic no-clobber publication failed for adoption attempt marker"
  fi
  [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
    fail "attempt marker publication did not create the expected hard-link pair"
  rm -f -- "${candidate}"
  verify_attempt_marker
}

emit_expected_child_log() {
  cat <<CHILD_LOG
mdn_checkpoint=${checkpoint}
mdn_checkpoint_sha256=${expected_checkpoint_sha256}
completion_result=${retry_result}
completion_result_sha256=${expected_result_sha256}
CHILD_LOG
}

verify_child_log() {
  local path="$1" label="$2"
  require_file "${path}"
  expect_mode_links "${path}" 444 1 "${label}"
  cmp -s -- <(emit_expected_child_log) "${path}" ||
    fail "${label} output is not the exact expected four-line result"
}

assert_retry_identity_matches_forensic() {
  [[ "$(emit_retry_file_inode_identity | sha256_stream)" == \
    "$(kv "${forensic_snapshot}" retry_file_inode_identity_sha256)" ]] ||
    fail "retry file inode identity changed from the pre-adoption snapshot"
  [[ "$(emit_retry_directory_identity | sha256_stream)" == \
    "$(kv "${forensic_snapshot}" retry_directory_identity_sha256)" ]] ||
    fail "retry directory identity changed from the pre-adoption snapshot"
}

assert_pre_adoption_evidence() {
  verify_pre_adoption_forensic
  verify_reconstructed_publisher
  verify_content_inventory_file "${content_pre}" true
  assert_retry_identity_matches_forensic
}

run_authoritative_child_once() {
  local child_mode="$1" leaf="$2" label="$3" destination candidate rc
  destination="${closure_runtime_root}/${leaf}"
  candidate="$(artifact_stage_path "${leaf}")"
  [[ ! -e "${destination}" && ! -L "${destination}" ]] ||
    fail "refusing to repeat ${label}; its frozen log already exists"
  [[ ! -e "${candidate}" && ! -L "${candidate}" ]] ||
    fail "ambiguous ${label} candidate residue exists; refusing child re-execution"

  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  assert_pre_adoption_evidence

  set +e
  (set -o noclobber; "${final_sealer}" "${child_mode}" >"${candidate}" 2>&1)
  rc=$?
  set -e
  [[ "${rc}" == 0 ]] ||
    fail "${label} child failed with exit ${rc}; attempt is permanently ambiguous"
  chmod 0444 -- "${candidate}"
  cmp -s -- <(emit_expected_child_log) "${candidate}" ||
    fail "${label} child output drifted; attempt is permanently ambiguous"

  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  assert_pre_adoption_evidence
  publish_existing_candidate "${leaf}" "${label}"
  verify_child_log "${destination}" "${label}"
}

readonly -a success_keys=(
  schema_id status adoption_attempt_number completed_at_utc attempt_path
  attempt_sha256 authoritative_seal_child_exit authoritative_verify_child_exit
  adoption_log_path adoption_log_sha256 verify_log_path verify_log_sha256
  content_pre_path content_pre_sha256 content_post_path content_post_sha256
  pre_post_content_identity_equal retry_file_inode_identity_sha256
  retry_directory_identity_sha256 content_bytes_paths_inodes_unchanged
  final_sealer_path final_sealer_sha256 closure_wrapper_path
  closure_wrapper_process_start_sha256 retry_result_path retry_result_sha256
  retry_execution_lock_held_by_wrapper training_reexecution_authorized
  canonical_data_raw_access independent_final_evidence policy_access
)

emit_success_status() {
  local destination="$1" completed_at="$2"
  cat >"${destination}" <<SUCCESS
schema_id=${closure_schema_id}.success.v1
status=complete
adoption_attempt_number=1
completed_at_utc=${completed_at}
attempt_path=${attempt_marker}
attempt_sha256=$(sha256_of "${attempt_marker}")
authoritative_seal_child_exit=0
authoritative_verify_child_exit=0
adoption_log_path=${adoption_log}
adoption_log_sha256=$(sha256_of "${adoption_log}")
verify_log_path=${verify_log}
verify_log_sha256=$(sha256_of "${verify_log}")
content_pre_path=${content_pre}
content_pre_sha256=$(sha256_of "${content_pre}")
content_post_path=${content_post}
content_post_sha256=$(sha256_of "${content_post}")
pre_post_content_identity_equal=true
retry_file_inode_identity_sha256=$(emit_retry_file_inode_identity | sha256_stream)
retry_directory_identity_sha256=$(emit_retry_directory_identity | sha256_stream)
content_bytes_paths_inodes_unchanged=true
final_sealer_path=${final_sealer}
final_sealer_sha256=${expected_final_sealer_sha256}
closure_wrapper_path=${script_path}
closure_wrapper_process_start_sha256=${process_start_wrapper_sha256}
retry_result_path=${retry_result}
retry_result_sha256=${expected_result_sha256}
retry_execution_lock_held_by_wrapper=false
training_reexecution_authorized=false
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
SUCCESS
}

verify_success_status() {
  local completed_at
  require_file "${success_status}"
  expect_mode_links "${success_status}" 444 1 "closure success status"
  verify_exact_kv_keys "${success_status}" "${success_keys[@]}"
  expect_kv "${success_status}" schema_id "${closure_schema_id}.success.v1"
  expect_kv "${success_status}" status complete
  expect_kv "${success_status}" adoption_attempt_number 1
  completed_at="$(kv "${success_status}" completed_at_utc)"
  [[ "${completed_at}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{9}Z$ ]] ||
    fail "success status has malformed UTC timestamp"
  expect_kv "${success_status}" attempt_path "${attempt_marker}"
  expect_kv "${success_status}" attempt_sha256 "$(sha256_of "${attempt_marker}")"
  expect_kv "${success_status}" authoritative_seal_child_exit 0
  expect_kv "${success_status}" authoritative_verify_child_exit 0
  expect_kv "${success_status}" adoption_log_path "${adoption_log}"
  expect_kv "${success_status}" adoption_log_sha256 "$(sha256_of "${adoption_log}")"
  expect_kv "${success_status}" verify_log_path "${verify_log}"
  expect_kv "${success_status}" verify_log_sha256 "$(sha256_of "${verify_log}")"
  expect_kv "${success_status}" content_pre_path "${content_pre}"
  expect_kv "${success_status}" content_pre_sha256 "$(sha256_of "${content_pre}")"
  expect_kv "${success_status}" content_post_path "${content_post}"
  expect_kv "${success_status}" content_post_sha256 "$(sha256_of "${content_post}")"
  expect_kv "${success_status}" pre_post_content_identity_equal true
  expect_kv "${success_status}" retry_file_inode_identity_sha256 \
    "$(kv "${forensic_snapshot}" retry_file_inode_identity_sha256)"
  expect_kv "${success_status}" retry_directory_identity_sha256 \
    "$(kv "${forensic_snapshot}" retry_directory_identity_sha256)"
  expect_kv "${success_status}" content_bytes_paths_inodes_unchanged true
  expect_kv "${success_status}" final_sealer_path "${final_sealer}"
  expect_kv "${success_status}" final_sealer_sha256 \
    "${expected_final_sealer_sha256}"
  expect_kv "${success_status}" closure_wrapper_path "${script_path}"
  expect_kv "${success_status}" closure_wrapper_process_start_sha256 \
    "${process_start_wrapper_sha256}"
  expect_kv "${success_status}" retry_result_path "${retry_result}"
  expect_kv "${success_status}" retry_result_sha256 "${expected_result_sha256}"
  expect_kv "${success_status}" retry_execution_lock_held_by_wrapper false
  expect_kv "${success_status}" training_reexecution_authorized false
  expect_kv "${success_status}" canonical_data_raw_access false
  expect_kv "${success_status}" independent_final_evidence false
  expect_kv "${success_status}" policy_access false
}

publish_success_status() {
  local candidate completed_at
  candidate="$(artifact_stage_path success.status)"
  completed_at="$(date -u +'%Y-%m-%dT%H:%M:%S.%NZ')"
  [[ ! -e "${success_status}" && ! -L "${success_status}" ]] ||
    fail "refusing to replace existing success status"
  [[ ! -e "${candidate}" && ! -L "${candidate}" ]] ||
    fail "ambiguous success candidate residue exists"
  if ! (set -o noclobber; emit_success_status "${candidate}" "${completed_at}"); then
    fail "could not create success status candidate"
  fi
  chmod 0444 -- "${candidate}"
  if ! ln -- "${candidate}" "${success_status}"; then
    fail "atomic no-clobber publication failed for success status"
  fi
  [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
    fail "success publication did not create the expected hard-link pair"
  rm -f -- "${candidate}"
  verify_success_status
}

readonly -a receipt_keys=(
  schema_id status closure_runtime_root closure_wrapper_path
  closure_wrapper_process_start_sha256 closure_wrapper_inode
  closure_wrapper_device closure_wrapper_mode erratum_path erratum_sha256
  completion_correction_path completion_correction_sha256
  initial_loaded_publisher_logical_path initial_loaded_publisher_sha256
  initial_loaded_publisher_is_current_path_hash_pair
  reconstructed_publisher_snapshot_path reconstructed_publisher_sha256
  reconstructed_publisher_executable_authority
  receipt_emission_disk_sealer_path receipt_emission_disk_sealer_sha256
  authoritative_adoption_sealer_path authoritative_adoption_sealer_sha256
  authoritative_adoption_sealer_inode completion_result_path
  completion_result_schema_id completion_result_sha256
  completion_result_alone_is_authority payload_inventory_master_sha256
  mdn_checkpoint_path mdn_checkpoint_sha256 execution_runner_path
  execution_runner_sha256 pre_adoption_forensic_path
  pre_adoption_forensic_sha256 content_pre_path content_pre_sha256
  content_post_path content_post_sha256 pre_post_content_identity_equal
  content_identity_fields content_identity_excludes_ctime
  retry_file_inode_identity_sha256 retry_directory_identity_sha256
  content_bytes_paths_inodes_unchanged attempt_path attempt_sha256
  attempt_started_at_utc adoption_attempt_number authoritative_seal_child_count
  authoritative_verify_child_count adoption_log_path adoption_log_sha256
  verify_log_path verify_log_sha256 success_status_path success_status_sha256
  success_completed_at_utc retry_directory_count retry_file_count
  retry_directory_mode retry_file_mode retry_file_links
  closure_directory_count closure_file_count closure_directory_mode
  closure_file_mode closure_file_links closure_lock_path closure_lock_sha256
  closure_exact_tree
  external_staging_absent wrapper_lock_separate_from_retry_execution_lock
  retry_execution_lock_held_by_wrapper downstream_verifier_path
  downstream_verifier_process_start_sha256 downstream_required_mode
  downstream_must_bind_closure_receipt result_or_old_runner_alone_forbidden
  authoritative_mutation training_reexecution_authorized
  canonical_data_raw_access final_holdout_available
  independent_final_evidence policy_access
)

emit_closure_receipt() {
  local destination="$1"
  cat >"${destination}" <<RECEIPT
schema_id=${closure_schema_id}
status=complete
closure_runtime_root=${closure_runtime_root}
closure_wrapper_path=${script_path}
closure_wrapper_process_start_sha256=${process_start_wrapper_sha256}
closure_wrapper_inode=${process_start_wrapper_inode}
closure_wrapper_device=${process_start_wrapper_device}
closure_wrapper_mode=0555
erratum_path=${erratum}
erratum_sha256=${expected_erratum_sha256}
completion_correction_path=${completion_correction}
completion_correction_sha256=${expected_completion_correction_sha256}
initial_loaded_publisher_logical_path=${final_sealer}
initial_loaded_publisher_sha256=${expected_initial_loaded_publisher_sha256}
initial_loaded_publisher_is_current_path_hash_pair=false
reconstructed_publisher_snapshot_path=${reconstructed_publisher}
reconstructed_publisher_sha256=${expected_initial_loaded_publisher_sha256}
reconstructed_publisher_executable_authority=false
receipt_emission_disk_sealer_path=${final_sealer}
receipt_emission_disk_sealer_sha256=${expected_final_sealer_sha256}
authoritative_adoption_sealer_path=${final_sealer}
authoritative_adoption_sealer_sha256=${expected_final_sealer_sha256}
authoritative_adoption_sealer_inode=${process_start_final_sealer_inode}
completion_result_path=${retry_result}
completion_result_schema_id=${retry_result_schema_id}
completion_result_sha256=${expected_result_sha256}
completion_result_alone_is_authority=false
payload_inventory_master_sha256=${expected_payload_inventory_master_sha256}
mdn_checkpoint_path=${checkpoint}
mdn_checkpoint_sha256=${expected_checkpoint_sha256}
execution_runner_path=${execution_runner}
execution_runner_sha256=${expected_execution_runner_sha256}
pre_adoption_forensic_path=${forensic_snapshot}
pre_adoption_forensic_sha256=$(sha256_of "${forensic_snapshot}")
content_pre_path=${content_pre}
content_pre_sha256=$(sha256_of "${content_pre}")
content_post_path=${content_post}
content_post_sha256=$(sha256_of "${content_post}")
pre_post_content_identity_equal=true
content_identity_fields=sha256_bytes_mode_links_relative_path
content_identity_excludes_ctime=true
retry_file_inode_identity_sha256=$(kv "${success_status}" retry_file_inode_identity_sha256)
retry_directory_identity_sha256=$(kv "${success_status}" retry_directory_identity_sha256)
content_bytes_paths_inodes_unchanged=true
attempt_path=${attempt_marker}
attempt_sha256=$(sha256_of "${attempt_marker}")
attempt_started_at_utc=$(kv "${attempt_marker}" started_at_utc)
adoption_attempt_number=1
authoritative_seal_child_count=1
authoritative_verify_child_count=1
adoption_log_path=${adoption_log}
adoption_log_sha256=$(sha256_of "${adoption_log}")
verify_log_path=${verify_log}
verify_log_sha256=$(sha256_of "${verify_log}")
success_status_path=${success_status}
success_status_sha256=$(sha256_of "${success_status}")
success_completed_at_utc=$(kv "${success_status}" completed_at_utc)
retry_directory_count=${expected_retry_directory_count}
retry_file_count=${expected_retry_file_count}
retry_directory_mode=0555
retry_file_mode=0444
retry_file_links=1
closure_directory_count=${expected_closure_directory_count}
closure_file_count=${expected_closure_file_count}
closure_directory_mode=0555
closure_file_mode=0444
closure_file_links=1
closure_lock_path=${closure_lock}
closure_lock_sha256=${expected_empty_sha256}
closure_exact_tree=true
external_staging_absent=true
wrapper_lock_separate_from_retry_execution_lock=true
retry_execution_lock_held_by_wrapper=false
downstream_verifier_path=${script_path}
downstream_verifier_process_start_sha256=${process_start_wrapper_sha256}
downstream_required_mode=--verify
downstream_must_bind_closure_receipt=true
result_or_old_runner_alone_forbidden=true
authoritative_mutation=idempotent_chmod_only_may_advance_ctime
training_reexecution_authorized=false
canonical_data_raw_access=false
final_holdout_available=false
independent_final_evidence=false
policy_access=false
RECEIPT
}

verify_known_candidate_bytes() {
  local leaf="$1" candidate="$2" timestamp
  case "${leaf}" in
  pre_adoption_forensic.status)
    candidate_matches_emitter "${candidate}" emit_pre_adoption_forensic ||
      fail "pre-adoption forensic candidate content drifted"
    ;;
  reconstructed_initial_publisher.3cce.sh)
    candidate_matches_emitter "${candidate}" write_reconstructed_publisher ||
      fail "reconstructed publisher candidate content drifted"
    ;;
  content.pre.sha256)
    candidate_matches_emitter "${candidate}" write_content_pre ||
      fail "pre-adoption content candidate drifted"
    ;;
  attempt.started)
    verify_exact_kv_keys "${candidate}" "${attempt_keys[@]}"
    timestamp="$(kv "${candidate}" started_at_utc)"
    [[ "${timestamp}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{9}Z$ ]] ||
      fail "attempt candidate has malformed UTC timestamp"
    cmp -s -- <(emit_attempt_marker /dev/stdout "${timestamp}") \
      "${candidate}" || fail "attempt candidate content drifted"
    ;;
  final_sealer_adoption.log | final_sealer_verify.log)
    cmp -s -- <(emit_expected_child_log) "${candidate}" ||
      fail "child log candidate content drifted: ${leaf}"
    ;;
  content.post.sha256)
    candidate_matches_emitter "${candidate}" write_content_post ||
      fail "post-adoption content candidate drifted"
    ;;
  success.status)
    verify_exact_kv_keys "${candidate}" "${success_keys[@]}"
    timestamp="$(kv "${candidate}" completed_at_utc)"
    [[ "${timestamp}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{9}Z$ ]] ||
      fail "success candidate has malformed UTC timestamp"
    cmp -s -- <(emit_success_status /dev/stdout "${timestamp}") \
      "${candidate}" || fail "success candidate content drifted"
    ;;
  completion_concurrency.status)
    candidate_matches_emitter "${candidate}" emit_closure_receipt ||
      fail "completion concurrency receipt candidate content drifted"
    ;;
  *) fail "no recovery validator exists for candidate leaf: ${leaf}" ;;
  esac
}

recover_published_candidate_links() {
  local rel candidate destination
  for rel in "${closure_files[@]}"; do
    [[ "${rel}" == .closure.lock ]] && continue
    candidate="$(artifact_stage_path "${rel}")"
    destination="${closure_runtime_root}/${rel}"
    if [[ ! -e "${candidate}" && ! -L "${candidate}" ]]; then
      continue
    fi
    reject_symlink_components "${candidate}"
    require_file "${candidate}"
    if [[ ! -e "${destination}" && ! -L "${destination}" ]]; then
      continue
    fi
    reject_symlink_components "${destination}"
    require_file "${destination}"
    [[ "${candidate}" -ef "${destination}" ]] ||
      fail "published ${rel} and its staging residue are not the same inode"
    [[ "$(stat -c '%h' -- "${candidate}")" == 2 ]] ||
      fail "published ${rel} staging pair has unexpected link count"
    [[ "$(stat -c '%u' -- "${candidate}")" == "$(id -u)" ]] ||
      fail "published ${rel} staging pair has unexpected owner"
    [[ "$(stat -c '%a' -- "${candidate}")" == 444 ]] ||
      fail "published ${rel} staging pair is not mode 0444"
    verify_known_candidate_bytes "${rel}" "${candidate}"
    rm -f -- "${candidate}"
    expect_mode_links "${destination}" 444 1 "recovered closure artifact ${rel}"
  done
}

verify_closure_receipt() {
  require_file "${closure_receipt}"
  expect_mode_links "${closure_receipt}" 444 1 "completion concurrency receipt"
  verify_exact_kv_keys "${closure_receipt}" "${receipt_keys[@]}"
  cmp -s -- <(emit_closure_receipt /dev/stdout) "${closure_receipt}" ||
    fail "completion concurrency receipt content drifted"
}

emit_expected_closure_files() {
  printf '%s\n' "${closure_files[@]}" | sort
}

verify_exact_closure_tree() {
  local actual_files actual_dirs special writable rel path candidate
  reject_symlink_components "${closure_runtime_root}"
  require_dir "${closure_runtime_root}"
  [[ "$(realpath -e -- "${closure_runtime_root}")" == \
    "${closure_runtime_root}" ]] || fail "closure runtime is not canonical"
  actual_dirs="$(find "${closure_runtime_root}" -type d -printf '%P\n')"
  [[ "${actual_dirs}" == "" ]] || fail "closure runtime contains a subdirectory"
  actual_files="$(find "${closure_runtime_root}" -type f -printf '%P\n' | sort)"
  [[ "${actual_files}" == "$(emit_expected_closure_files)" ]] ||
    fail "closure runtime exact file tree mismatch"
  special="$(find "${closure_runtime_root}" ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "closure runtime contains a special entry: ${special}"
  writable="$(find "${closure_runtime_root}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] || fail "closure runtime contains a writable entry: ${writable}"
  [[ "$(stat -c '%a' -- "${closure_runtime_root}")" == 555 ]] ||
    fail "closure runtime is not mode 0555"
  [[ "${#closure_files[@]}" == "${expected_closure_file_count}" ]] ||
    fail "internal closure file count is invalid"
  for rel in "${closure_files[@]}"; do
    path="${closure_runtime_root}/${rel}"
    require_file "${path}"
    expect_mode_links "${path}" 444 1 "closure artifact ${rel}"
    candidate="$(artifact_stage_path "${rel}")"
    [[ ! -e "${candidate}" && ! -L "${candidate}" ]] ||
      fail "external staging residue remains for closure artifact ${rel}"
  done
  [[ "$(sha256_of "${closure_lock}")" == "${expected_empty_sha256}" ]] ||
    fail "sealed closure lock content drifted"
  assert_staging_namespace_empty
}

verify_all_frozen_evidence() {
  verify_pre_adoption_forensic
  verify_reconstructed_publisher
  verify_attempt_marker
  verify_child_log "${adoption_log}" "authoritative adoption log"
  verify_child_log "${verify_log}" "authoritative verification log"
  verify_content_inventory_file "${content_pre}" true
  verify_content_inventory_file "${content_post}" true
  cmp -s -- "${content_pre}" "${content_post}" ||
    fail "pre/post retry content identities are not byte-identical"
  assert_retry_identity_matches_forensic
  verify_success_status
}

seal_closure_tree() {
  local rel
  for rel in "${closure_files[@]}"; do
    chmod 0444 -- "${closure_runtime_root}/${rel}"
  done
  chmod 0555 -- "${closure_runtime_root}"
}

finalize_closure_receipt() {
  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  verify_all_frozen_evidence
  assert_partial_closure_tree
  [[ "$(stat -c '%a' -- "${closure_runtime_root}")" == 700 ]] ||
    fail "closure runtime must remain mode 0700 until receipt publication"
  publish_static_artifact completion_concurrency.status emit_closure_receipt \
    "completion concurrency receipt"
  assert_source_identity_guards
  seal_closure_tree
  verify_exact_closure_tree
  verify_closure_receipt
}

acquire_existing_closure_lock() {
  reject_symlink_components "${closure_runtime_root}"
  reject_symlink_components "${closure_lock}"
  require_dir "${closure_runtime_root}"
  require_file "${closure_lock}"
  [[ "$(sha256_of "${closure_lock}")" == "${expected_empty_sha256}" ]] ||
    fail "closure lock content drifted"
  [[ "$(realpath -e -- "${closure_runtime_root}")" == \
    "${closure_runtime_root}" ]] || fail "closure runtime is not canonical"
  exec {closure_lock_fd}<"${closure_lock}"
  flock -n "${closure_lock_fd}" || fail "closure lock is held"
}

verify_authoritative_child_read_only() {
  local child_output
  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  child_output="$("${final_sealer}" --verify 2>&1)" ||
    fail "authoritative final sealer --verify failed"
  [[ "${child_output}" == "$(emit_expected_child_log)" ]] ||
    fail "authoritative final sealer --verify output drifted"
  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
}

verify_complete_closure() {
  verify_exact_closure_tree
  assert_source_identity_guards
  verify_retry_result_bindings
  verify_all_frozen_evidence
  verify_closure_receipt
  verify_authoritative_child_read_only
  verify_exact_closure_tree
  verify_closure_receipt
}

assert_no_post_attempt_residue_before_attempt() {
  local leaf candidate
  for leaf in final_sealer_adoption.log final_sealer_verify.log \
    content.post.sha256 success.status completion_concurrency.status; do
    [[ ! -e "${closure_runtime_root}/${leaf}" && \
      ! -L "${closure_runtime_root}/${leaf}" ]] ||
      fail "post-attempt artifact exists before attempt marker: ${leaf}"
    candidate="$(artifact_stage_path "${leaf}")"
    [[ ! -e "${candidate}" && ! -L "${candidate}" ]] ||
      fail "post-attempt staging residue exists before attempt marker: ${leaf}"
  done
}

start_authoritative_adoption() {
  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  [[ "$(stat -c '%a' -- "${closure_runtime_root}")" == 700 ]] ||
    fail "new closure runtime is not mode 0700"
  assert_no_post_attempt_residue_before_attempt

  publish_static_artifact pre_adoption_forensic.status \
    emit_pre_adoption_forensic "pre-adoption forensic snapshot"
  publish_static_artifact reconstructed_initial_publisher.3cce.sh \
    write_reconstructed_publisher "reconstructed initial publisher snapshot"
  publish_static_artifact content.pre.sha256 write_content_pre \
    "pre-adoption retry content inventory"
  assert_pre_adoption_evidence
  publish_attempt_marker
  verify_attempt_marker

  run_authoritative_child_once --seal final_sealer_adoption.log \
    "authoritative final-sealer adoption"
  run_authoritative_child_once --verify final_sealer_verify.log \
    "authoritative final-sealer verification"

  assert_source_identity_guards
  verify_exact_retry_tree
  verify_retry_result_bindings
  publish_static_artifact content.post.sha256 write_content_post \
    "post-adoption retry content inventory"
  verify_content_inventory_file "${content_post}" true
  cmp -s -- "${content_pre}" "${content_post}" ||
    fail "authoritative adoption changed retry content identity"
  assert_retry_identity_matches_forensic
  publish_success_status
  finalize_closure_receipt
}

print_plan() {
  cat <<PLAN
schema_id=${closure_schema_id}.plan
closure_runtime_root=${closure_runtime_root}
closure_receipt_path=${closure_receipt}
closure_wrapper_path=${script_path}
closure_wrapper_process_start_sha256=${process_start_wrapper_sha256}
closure_wrapper_current_mode=0$(stat -c '%a' -- "${script_path}")
erratum_path=${erratum}
erratum_expected_sha256=${expected_erratum_sha256}
initial_loaded_publisher_logical_path=${final_sealer}
initial_loaded_publisher_sha256=${expected_initial_loaded_publisher_sha256}
initial_loaded_publisher_is_current_path_hash_pair=false
reconstructed_publisher_snapshot_path=${reconstructed_publisher}
reconstructed_publisher_proof_sha256=$(emit_reconstructed_initial_publisher | sha256_stream)
reconstructed_publisher_expected_sha256=${expected_initial_loaded_publisher_sha256}
reconstruction_requires_exactly_one_line_order_match=true
reconstructed_publisher_executable_authority=false
final_sealer_path=${final_sealer}
final_sealer_expected_sha256=${expected_final_sealer_sha256}
final_sealer_current_mode=0$(stat -c '%a' -- "${final_sealer}")
completion_result_path=${retry_result}
completion_result_expected_sha256=${expected_result_sha256}
payload_inventory_master_sha256=${expected_payload_inventory_master_sha256}
mdn_checkpoint_sha256=${expected_checkpoint_sha256}
retry_directory_count=${expected_retry_directory_count}
retry_file_count=${expected_retry_file_count}
closure_directory_count=${expected_closure_directory_count}
closure_file_count=${expected_closure_file_count}
authoritative_adoption_child=${final_sealer} --seal
authoritative_verification_child=${final_sealer} --verify
adoption_attempt_limit=1
ambiguous_attempt_reexecution=false
content_identity_fields=sha256_bytes_mode_links_relative_path
content_identity_excludes_ctime=true
authorized_adoption_mutation=idempotent_chmod_only_may_advance_ctime
retry_execution_lock_held_by_wrapper=false
training_reexecution_authorized=false
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

preflight_plan_read_only() {
  verify_hash "${final_sealer}" "${expected_final_sealer_sha256}" \
    "final completion sealer"
  verify_hash "${erratum}" "${expected_erratum_sha256}" "concurrency erratum"
  verify_hash "${completion_correction}" \
    "${expected_completion_correction_sha256}" "completion correction"
  verify_hash "${execution_runner}" "${expected_execution_runner_sha256}" \
    "retry execution runner"
  verify_hash "${retry_result}" "${expected_result_sha256}" \
    "retry completion result"
  verify_hash "${checkpoint}" "${expected_checkpoint_sha256}" "MDN checkpoint"
  verify_exact_retry_tree
  verify_retry_result_bindings
  [[ "$(emit_reconstructed_initial_publisher | sha256_stream)" == \
    "${expected_initial_loaded_publisher_sha256}" ]] ||
    fail "read-only reconstruction proof does not hash to the initial publisher"
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--seal|--verify]"
case "${mode}" in
--plan | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--seal|--verify]" ;;
esac

if [[ "${mode}" == --plan ]]; then
  preflight_plan_read_only
  print_plan
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  assert_source_identity_guards
  acquire_existing_closure_lock
  verify_complete_closure
  echo "completion_concurrency_closure=${closure_receipt}"
  echo "completion_concurrency_closure_sha256=$(sha256_of "${closure_receipt}")"
  echo "completion_result=${retry_result}"
  echo "completion_result_sha256=${expected_result_sha256}"
  exit 0
fi

assert_source_identity_guards
prepare_closure_lock

if [[ -e "${closure_receipt}" || -L "${closure_receipt}" ]]; then
  [[ -e "${success_status}" && ! -L "${success_status}" ]] ||
    fail "closure receipt exists without a valid success status"
  if [[ "$(stat -c '%a' -- "${closure_runtime_root}")" == 700 ]]; then
    publish_static_artifact completion_concurrency.status emit_closure_receipt \
      "completion concurrency receipt"
    seal_closure_tree
  fi
  verify_complete_closure
  echo "completion_concurrency_closure=${closure_receipt}"
  echo "completion_concurrency_closure_sha256=$(sha256_of "${closure_receipt}")"
  echo "completion_result=${retry_result}"
  echo "completion_result_sha256=${expected_result_sha256}"
  exit 0
fi

if [[ -e "${attempt_marker}" || -L "${attempt_marker}" ]]; then
  verify_attempt_marker
  if [[ -e "${success_status}" && ! -L "${success_status}" ]]; then
    finalize_closure_receipt
    echo "completion_concurrency_closure=${closure_receipt}"
    echo "completion_concurrency_closure_sha256=$(sha256_of "${closure_receipt}")"
    echo "completion_result=${retry_result}"
    echo "completion_result_sha256=${expected_result_sha256}"
    exit 0
  fi
  fail "attempt.started exists without success.status; refusing ambiguous adoption re-execution"
fi

start_authoritative_adoption
echo "completion_concurrency_closure=${closure_receipt}"
echo "completion_concurrency_closure_sha256=$(sha256_of "${closure_receipt}")"
echo "completion_result=${retry_result}"
echo "completion_result_sha256=${expected_result_sha256}"
