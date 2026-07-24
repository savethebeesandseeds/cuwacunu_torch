#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1_endpoint_bundle_for_retry2_v1"
readonly source_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1"
readonly interruption_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1"
readonly retry2_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"

readonly expected_amendment_sha256="c94c282d93844563f83abf3e1826111e14d640370d38e778bee04070aa1303ad"
readonly expected_interruption_sealer_sha256="95478dbd60734116171c9bd2bc40890c8abd1ea59b0d60c1dccc4dcbdb1241a3"
readonly expected_interruption_amendment_sha256="e7d99698ee4b62254a274ebbf3d699b9e00ced0ff727279767e1d4976d594002"
readonly expected_interruption_receipt_sha256="e6c845233f3f434a9c46bead1b9fb825217492a5da7ae0a95174fc10b15e1117"
readonly expected_interruption_regular_inventory_sha256="c7cce9005bee5efaa5f85924624839afad57655b4bfdb3d0d4a774fd4bf60926"
readonly expected_interruption_directory_inventory_sha256="40c7b2cf3846f3c439c6ebfe8d86b2839c969996fe6ce211be3577e2d11613ee"
readonly expected_retry1_runner_sha256="bebd812c48ba318ec632ed490841188daef9cdd68e02a53616ca9feba809ae43"
readonly expected_retry1_attempt_sha256="0ad5bbaba0183a710d4fd784cfc1bb5c3013fb937e4c5821b7df6183e231b359"
readonly expected_retry1_inputs_sha256="709350e637225eefcb854eaa6412f41a57cb3d89655c62ee4ebe59adacb1fa9b"

readonly expected_retry1_regular_file_count=49
readonly expected_retry1_directory_count=25
readonly expected_retry1_regular_file_bytes=64939302
readonly expected_retry1_content_inventory_sha256="6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05"

readonly expected_endpoint_file_count=21
readonly expected_endpoint_directory_count=9
readonly expected_endpoint_descendant_directory_count=8
readonly expected_endpoint_regular_file_bytes=32731999
readonly expected_endpoint_content_inventory_sha256="bd2f8d55b4e3e3a3a06bf14749b28ea0bec01ea9c07aaa1c1628e9ed4f59e13f"
readonly expected_checkpoint_sha256="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
readonly expected_checkpoint_internal_digest="14398b461a259bc5"
readonly expected_optimizer_steps=3000
readonly expected_seed=17
readonly expected_anchor_begin=0
readonly expected_anchor_end=2496
readonly expected_anchor_count=3261
readonly expected_event_record_count=17608
readonly expected_event_artifact_published_count=8
readonly expected_event_artifact_digest_count=6
readonly expected_event_artifact_schema_count=6

readonly expected_job_id="train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg"
readonly expected_component_family="wikimyei.representation.encoding.mtf_jepa_mae_vicreg"
readonly expected_cursor_token="version=ujcamei.graph_anchor_cursor_report.v1|graph=4133db527907a8e4|reference_edge=SYN2ALPHASYN2USD|Hx=30|Hf=1|edges=3|accepted=3261|candidates=3261|skipped=0|first=1896047999999|last=2177711999999"
readonly expected_execution_chain="ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train"

readonly -a endpoint_directories=(
  "."
  "config"
  "training"
  "training/components"
  "training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg"
  "training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns"
  "training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd"
  "training/job"
  "training/system"
)

readonly -a endpoint_relpaths=(
  "config/capture.config"
  "config/representation.jkimyei"
  "config/representation.net"
  "config/train.config"
  "training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd/component_spawn.ref"
  "training/job/channel_representation.report"
  "training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
  "training/job/job.manifest"
  "training/job/job.state"
  "training/job/lattice.checkpoint.fact"
  "training/job/lattice.exposure.fact"
  "training/job/lattice.source_analytics.fact"
  "training/job/runtime.checkpoint_io.fact"
  "training/job/runtime.component_training_update.fact"
  "training/job/runtime.health_measurement.fact"
  "training/job/runtime.job_events.probe"
  "training/job/runtime.result.fact"
  "training/system/component_spawn_registry.v1.lls"
  "training/system/runtime_layout.v1.lls"
  "training.log"
  "training.status"
)

declare -Ar expected_endpoint_hashes=(
  ["config/capture.config"]="63e042f47bbdbc2970cd8afbfcc639fcec5a7a980aacbf7a59d8db592f97f821"
  ["config/representation.jkimyei"]="c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8"
  ["config/representation.net"]="42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e"
  ["config/train.config"]="c517ef409c1829413d18536851aecc48bb94e7f7ef2ed1386106511ee1e3ef28"
  ["training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd/component_spawn.ref"]="b53337085984b9e1cf37378097d68e600c20a817f4fbbc013904cee5f033c284"
  ["training/job/channel_representation.report"]="99e370677d4dd1932aa4fd3af66b954e2969a3afc55b1be2436b8875e8c64740"
  ["training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"]="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
  ["training/job/job.manifest"]="f1e514f84f05ae898b8616204e75cfe4034f2b23ff972c525f62635349905c7c"
  ["training/job/job.state"]="8b3d7a4c268777aaa3443eec3fef804e15a988219c4adcb3a34c7cbcc2c94769"
  ["training/job/lattice.checkpoint.fact"]="5ecea7b518730e1f75e6c15e5ede3bf45a5350c10f837a6fed7fb6ad2313ee45"
  ["training/job/lattice.exposure.fact"]="ccc3f7c5e365f39848c85828d7c35e060e03c0578a53595c31d9ab55d5b3b186"
  ["training/job/lattice.source_analytics.fact"]="e31c2556cdf876bc10953ca96e00f594b7805038c9ea6e3e30d2f5c46867b6ca"
  ["training/job/runtime.checkpoint_io.fact"]="6df2bf5efdbfc6cc77b58c585ef0ed9d0968225e3d9a119ec3b614859d8ddf5d"
  ["training/job/runtime.component_training_update.fact"]="5c33365fcc2211a58a8a0570175a7c841872a526a436720ef5c74d2f23d87f28"
  ["training/job/runtime.health_measurement.fact"]="23081ff36898cca9934ec652497c13536e70e5cf3f75271a0e335023a1afa704"
  ["training/job/runtime.job_events.probe"]="cfb9ad80c8496c70666476575da929b913622647baa2188d236119ef44b2688d"
  ["training/job/runtime.result.fact"]="7d3275bc2bdd8d647f1f06f41c1d11097acff56cc4f5e8cbb7d2497978ce182c"
  ["training/system/component_spawn_registry.v1.lls"]="b30f3d69a58be02bc7e29f5f308fe6f1e4c02a335127c48715abfd6f8f63e8b7"
  ["training/system/runtime_layout.v1.lls"]="28ebbb59d7e486e9469ee3673a4db557e50e5f5ae60689be4dfc384f8f8ce287"
  ["training.log"]="7346a96afd6c226a772a22de6d903f2a361c70b5f72843816eff80c8c481f31c"
  ["training.status"]="b0fa364d31f32471cf0ff3d69b2836203e4dc47fd79f4c0507a7d7367b2f5ed5"
)

declare -Ar expected_endpoint_sizes=(
  ["config/capture.config"]="5604"
  ["config/representation.jkimyei"]="2119"
  ["config/representation.net"]="602"
  ["config/train.config"]="5601"
  ["training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd/component_spawn.ref"]="435"
  ["training/job/channel_representation.report"]="6805"
  ["training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"]="853931"
  ["training/job/job.manifest"]="7458"
  ["training/job/job.state"]="4860"
  ["training/job/lattice.checkpoint.fact"]="3297"
  ["training/job/lattice.exposure.fact"]="8683"
  ["training/job/lattice.source_analytics.fact"]="3858"
  ["training/job/runtime.checkpoint_io.fact"]="1996"
  ["training/job/runtime.component_training_update.fact"]="1185"
  ["training/job/runtime.health_measurement.fact"]="1744"
  ["training/job/runtime.job_events.probe"]="31801520"
  ["training/job/runtime.result.fact"]="3562"
  ["training/system/component_spawn_registry.v1.lls"]="778"
  ["training/system/runtime_layout.v1.lls"]="400"
  ["training.log"]="12281"
  ["training.status"]="5280"
)

fail() {
  echo "representation ablation retry1 endpoint bundle: $*" >&2
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

expect_line_count() {
  local path="$1"
  local line="$2"
  local expected="$3"
  local actual
  actual="$(grep -Fxc -- "${line}" "${path}" || true)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${expected} copies of line: ${line}; found ${actual}"
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

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
source_runtime="${runtime_parent}/${source_schema_id}"
source_endpoint="${source_runtime}/arms/endpoint_scale"
source_lock_path="${source_runtime}/.development.lock"
bundle_root="${runtime_parent}/${schema_id}"
staging_root="${runtime_parent}/.${schema_id}.candidate"
retry2_runtime="${runtime_parent}/${retry2_schema_id}"

amendment_path="${script_dir}/REPRESENTATION_ABLATION_RETRY1_ENDPOINT_BUNDLE_FOR_RETRY2_AMENDMENT.md"
retry1_runner="${script_dir}/run_representation_ablation_v2_retry1.sh"
retry1_frozen_runner="${source_runtime}/frozen_sources/$(basename "${retry1_runner}")"
retry1_attempt="${source_runtime}/development.retry1.attempt.status"
retry1_inputs="${source_runtime}/inputs.status"

interruption_root="${runtime_parent}/${interruption_schema_id}"
interruption_sealer="${script_dir}/seal_and_verify_representation_ablation_retry1_interruption_closure_v1.sh"
interruption_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY1_OPERATIONAL_INTERRUPTION_RECOVERY_AMENDMENT.md"
interruption_receipt="${interruption_root}/interruption_closure.status"
interruption_regular_inventory="${interruption_root}/regular_files.inventory.tsv"
interruption_directory_inventory="${interruption_root}/directories.inventory.tsv"
interruption_frozen_sealer="${interruption_root}/frozen_sources/$(basename "${interruption_sealer}")"
interruption_frozen_amendment="${interruption_root}/frozen_sources/$(basename "${interruption_amendment}")"

source_policy="${source_endpoint}/config/representation.jkimyei"
source_net="${source_endpoint}/config/representation.net"
source_train_config="${source_endpoint}/config/train.config"
source_capture_config="${source_endpoint}/config/capture.config"
source_status="${source_endpoint}/training.status"
source_log="${source_endpoint}/training.log"
source_job="${source_endpoint}/training/job"
source_report="${source_job}/channel_representation.report"
source_checkpoint="${source_job}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
source_manifest="${source_job}/job.manifest"
source_job_state="${source_job}/job.state"
source_lattice_checkpoint="${source_job}/lattice.checkpoint.fact"
source_lattice_exposure="${source_job}/lattice.exposure.fact"
source_lattice_source="${source_job}/lattice.source_analytics.fact"
source_checkpoint_io="${source_job}/runtime.checkpoint_io.fact"
source_component_update="${source_job}/runtime.component_training_update.fact"
source_health="${source_job}/runtime.health_measurement.fact"
source_events="${source_job}/runtime.job_events.probe"
source_result="${source_job}/runtime.result.fact"
source_spawn_ref="${source_endpoint}/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/syv_c3cfe3b2ed5adcdd/component_spawn.ref"
source_spawn_registry="${source_endpoint}/training/system/component_spawn_registry.v1.lls"
source_runtime_layout="${source_endpoint}/training/system/runtime_layout.v1.lls"

bundle_snapshot="${bundle_root}/source_snapshot"
bundle_receipt="${bundle_root}/endpoint_bundle.status"
bundle_regular_inventory="${bundle_root}/source_regular_files.inventory.tsv"
bundle_directory_inventory="${bundle_root}/source_directories.inventory.tsv"
bundle_frozen_root="${bundle_root}/frozen_sources"
bundle_frozen_sealer="${bundle_frozen_root}/$(basename "${script_path}")"
bundle_frozen_amendment="${bundle_frozen_root}/$(basename "${amendment_path}")"

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
    chmod -R u+w -- "${staging_root}" 2>/dev/null || true
    find "${staging_root}" -xdev -depth -type f -delete 2>/dev/null || true
    find "${staging_root}" -xdev -depth -type d -empty -delete \
      2>/dev/null || true
  fi
}
trap cleanup EXIT

new_temp_file() {
  local __result_var="$1"
  local leaf="$2"
  local created
  created="$(mktemp "${runtime_parent}/.${schema_id}.${leaf}.XXXXXX")"
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
source_endpoint_root=${source_endpoint}
source_lock_path=${source_lock_path}
interruption_closure_schema_id=${interruption_schema_id}
interruption_closure_receipt_sha256=${expected_interruption_receipt_sha256}
interruption_closure_regular_inventory_sha256=${expected_interruption_regular_inventory_sha256}
interruption_closure_directory_inventory_sha256=${expected_interruption_directory_inventory_sha256}
interruption_closure_verify_before_source_lock=true
source_lock_held_through_snapshots_and_copy=true
source_snapshot_count=2
endpoint_regular_file_count=${expected_endpoint_file_count}
endpoint_directory_count_including_root=${expected_endpoint_directory_count}
endpoint_descendant_directory_count=${expected_endpoint_descendant_directory_count}
endpoint_regular_file_bytes=${expected_endpoint_regular_file_bytes}
endpoint_content_inventory_sha256=${expected_endpoint_content_inventory_sha256}
endpoint_event_artifact_published_count=${expected_event_artifact_published_count}
endpoint_event_artifact_digest_count=${expected_event_artifact_digest_count}
endpoint_event_artifact_schema_count=${expected_event_artifact_schema_count}
bundle_runtime_root=${bundle_root}
bundle_snapshot_root=${bundle_snapshot}
bundle_receipt_path=${bundle_receipt}
copy_method=cp_reflink_never
source_destination_inode_tuple_distinct=true
source_destination_link_count=1
publication_method=atomic_no_clobber_directory_rename
scratch_parent=${runtime_parent}
endpoint_bundle_owned_tmp_usage=false
endpoint_bundle_payload_under_tmp=false
prerequisite_interruption_verifier_invoked=true
prerequisite_interruption_verifier_may_use_sealed_small_tmp_snapshots=true
prerequisite_interruption_verifier_cleans_tmp_snapshots=true
large_payload_tmp_usage=false
historical_evidence_only=true
direct_retry1_use_authorized=false
direct_bundle_use_authorized=false
retry2_local_consumption_authorized=false
separate_retry2_stage_verifier_required=true
retry2_exact_scientific_config_equivalence_required=true
retry2_second_local_copy_required=true
retry2_runtime_root=${retry2_runtime}
time_only_artifact_included=false
no_tf_alignment_artifact_included=false
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
forecast_access=false
policy_access=false
PLAN
}

assert_process_start_identity() {
  reject_symlink_components "${script_path}"
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == \
    "${process_start_sealer_sha256}" ]] ||
    fail "executing endpoint-bundle sealer bytes changed after process start"
  [[ "$(stat -c '%a' -- "${script_path}")" == \
    "${process_start_sealer_mode}" ]] ||
    fail "executing endpoint-bundle sealer mode changed after process start"
  [[ "$(stat -c '%h' -- "${script_path}")" == \
    "${process_start_sealer_links}" ]] ||
    fail "executing endpoint-bundle sealer link count changed after process start"
  [[ "$(stat -c '%u' -- "${script_path}")" == \
    "${process_start_sealer_uid}" ]] ||
    fail "executing endpoint-bundle sealer owner changed after process start"
  [[ "$(stat -c '%s' -- "${script_path}")" == \
    "${process_start_sealer_bytes}" ]] ||
    fail "executing endpoint-bundle sealer size changed after process start"
  [[ "$(stat -c '%i' -- "${script_path}")" == \
    "${process_start_sealer_inode}" ]] ||
    fail "executing endpoint-bundle sealer inode changed after process start"
  [[ "$(stat -c '%d' -- "${script_path}")" == \
    "${process_start_sealer_device}" ]] ||
    fail "executing endpoint-bundle sealer device changed after process start"
  [[ "${process_start_sealer_links}" == 1 ]] ||
    fail "executing endpoint-bundle sealer must have link count one"
}

verify_live_bundle_authorities() {
  assert_process_start_identity
  verify_hash "${amendment_path}" "${expected_amendment_sha256}" \
    "endpoint-bundle amendment"
  if [[ "${mode}" == --seal || "${mode}" == --verify ]]; then
    [[ "${process_start_sealer_mode}" == 555 ]] ||
      fail "endpoint-bundle sealer must be frozen mode 0555"
    [[ "$(stat -c '%a' -- "${amendment_path}")" == 444 ]] ||
      fail "endpoint-bundle amendment must be frozen mode 0444"
  fi
}

verify_interruption_static_authority() {
  verify_hash "${interruption_sealer}" \
    "${expected_interruption_sealer_sha256}" \
    "live interruption-closure sealer"
  verify_hash "${interruption_amendment}" \
    "${expected_interruption_amendment_sha256}" \
    "live interruption amendment"
  verify_hash "${interruption_receipt}" \
    "${expected_interruption_receipt_sha256}" \
    "interruption-closure receipt"
  verify_hash "${interruption_regular_inventory}" \
    "${expected_interruption_regular_inventory_sha256}" \
    "interruption-closure regular inventory"
  verify_hash "${interruption_directory_inventory}" \
    "${expected_interruption_directory_inventory_sha256}" \
    "interruption-closure directory inventory"
  verify_hash "${interruption_frozen_sealer}" \
    "${expected_interruption_sealer_sha256}" \
    "frozen interruption-closure sealer"
  verify_hash "${interruption_frozen_amendment}" \
    "${expected_interruption_amendment_sha256}" \
    "frozen interruption amendment"

  expect_kv "${interruption_receipt}" schema_id "${interruption_schema_id}"
  expect_kv "${interruption_receipt}" status complete
  expect_kv "${interruption_receipt}" source_runtime_root "${source_runtime}"
  expect_kv "${interruption_receipt}" content_inventory_sha256 \
    "${expected_retry1_content_inventory_sha256}"
  expect_kv "${interruption_receipt}" endpoint_arm_status complete
  expect_kv "${interruption_receipt}" endpoint_checkpoint_sha256 \
    "${expected_checkpoint_sha256}"
  expect_kv "${interruption_receipt}" \
    endpoint_checkpoint_internal_digest_reported \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${interruption_receipt}" endpoint_direct_use_authorized false
  expect_kv "${interruption_receipt}" endpoint_import_eligibility \
    requires_separate_retry2_import_verifier
  expect_kv "${interruption_receipt}" \
    endpoint_import_requires_exact_retry2_scientific_config_equivalence true
  expect_kv "${interruption_receipt}" time_only_partial_artifact_reuse_authorized \
    false
  expect_kv "${interruption_receipt}" retry1_reentry_authorized false
}

verify_interruption_authority_prelock() {
  verify_live_bundle_authorities
  verify_interruption_static_authority
  "${interruption_sealer}" --verify >/dev/null
  verify_interruption_static_authority
  assert_process_start_identity
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
    fail "retry1 lock file-descriptor identity drifted"
  [[ "${path_device}" == "${source_lock_device}" && \
    "${path_inode}" == "${source_lock_inode}" ]] ||
    fail "retry1 lock path no longer names the locked inode"
}

acquire_source_lock() {
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

relative_path_of() {
  local root="$1"
  local path="$2"
  if [[ "${path}" == "${root}" ]]; then
    printf '.'
  else
    printf '%s' "${path#${root}/}"
  fi
}

emit_regular_inventory() {
  local root="$1"
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tsha256\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${root}" "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && \
      "${rel}" != *'|'* ]] ||
      fail "non-canonical inventory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    [[ "${links_value}" == 1 ]] ||
      fail "regular file has hard-link drift: ${path}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}" \
      "$(sha256_of "${path}")"
  done < <(find "${root}" -xdev -type f -print0 | LC_ALL=C sort -z)
}

emit_directory_inventory() {
  local root="$1"
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${root}" "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && \
      "${rel}" != *'|'* ]] ||
      fail "non-canonical directory inventory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}"
  done < <(find "${root}" -xdev -type d -print0 | LC_ALL=C sort -z)
}

compute_content_inventory_sha256() {
  local root="$1"
  (
    cd "${root}"
    find . -xdev -type f -print0 | LC_ALL=C sort -z |
      xargs -0 sha256sum |
      sha256sum | awk '{print $1}'
  )
}

verify_tree_safety() {
  local root="$1"
  reject_symlink_components "${root}"
  require_dir "${root}"
  [[ "$(realpath -e -- "${root}")" == "${root}" ]] ||
    fail "tree root is not canonical: ${root}"
  local root_device entry entry_device
  root_device="$(stat -c '%d' -- "${root}")"
  while IFS= read -r -d '' entry; do
    [[ ! -L "${entry}" ]] || fail "tree contains symlink: ${entry}"
    entry_device="$(stat -c '%d' -- "${entry}")"
    [[ "${entry_device}" == "${root_device}" ]] ||
      fail "tree contains cross-device entry: ${entry}"
    if [[ -f "${entry}" ]]; then
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "tree contains file with nonunit link count: ${entry}"
    elif [[ ! -d "${entry}" ]]; then
      fail "tree contains special entry: ${entry}"
    fi
  done < <(find "${root}" -xdev -mindepth 0 -print0)
}

verify_retry1_snapshot_aggregate() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  local regular_count directory_count regular_bytes
  regular_count="$(($(wc -l <"${regular_inventory}") - 1))"
  directory_count="$(($(wc -l <"${directory_inventory}") - 1))"
  regular_bytes="$(awk -F '\t' 'NR > 1 { sum += $6 }
    END { printf "%.0f", sum + 0 }' "${regular_inventory}")"
  [[ "${regular_count}" == "${expected_retry1_regular_file_count}" ]] ||
    fail "retry1 regular-file count drifted: ${regular_count}"
  [[ "${directory_count}" == "${expected_retry1_directory_count}" ]] ||
    fail "retry1 directory count drifted: ${directory_count}"
  [[ "${regular_bytes}" == "${expected_retry1_regular_file_bytes}" ]] ||
    fail "retry1 regular-file byte total drifted: ${regular_bytes}"
  [[ "$(compute_content_inventory_sha256 "${source_runtime}")" == \
    "${expected_retry1_content_inventory_sha256}" ]] ||
    fail "retry1 complete content inventory digest drifted"
  cmp -s -- "${regular_inventory}" "${interruption_regular_inventory}" ||
    fail "retry1 regular inventory differs from published interruption closure"
  cmp -s -- "${directory_inventory}" "${interruption_directory_inventory}" ||
    fail "retry1 directory inventory differs from published interruption closure"
}

verify_endpoint_snapshot_aggregate() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  local regular_count directory_count regular_bytes expected_paths actual_paths
  regular_count="$(($(wc -l <"${regular_inventory}") - 1))"
  directory_count="$(($(wc -l <"${directory_inventory}") - 1))"
  regular_bytes="$(awk -F '\t' 'NR > 1 { sum += $6 }
    END { printf "%.0f", sum + 0 }' "${regular_inventory}")"
  [[ "${regular_count}" == "${expected_endpoint_file_count}" ]] ||
    fail "endpoint regular-file count drifted: ${regular_count}"
  [[ "${directory_count}" == "${expected_endpoint_directory_count}" ]] ||
    fail "endpoint directory count drifted: ${directory_count}"
  [[ "$((directory_count - 1))" == \
    "${expected_endpoint_descendant_directory_count}" ]] ||
    fail "endpoint descendant-directory count drifted"
  [[ "${regular_bytes}" == "${expected_endpoint_regular_file_bytes}" ]] ||
    fail "endpoint regular-file byte total drifted: ${regular_bytes}"
  [[ "$(compute_content_inventory_sha256 "${source_endpoint}")" == \
    "${expected_endpoint_content_inventory_sha256}" ]] ||
    fail "endpoint complete content inventory digest drifted"

  new_temp_file expected_paths "endpoint.dirs.expected"
  new_temp_file actual_paths "endpoint.dirs.actual"
  printf '%s\n' "${endpoint_directories[@]}" >"${expected_paths}"
  tail -n +2 "${directory_inventory}" | cut -f1 >"${actual_paths}"
  cmp -s -- "${expected_paths}" "${actual_paths}" ||
    fail "endpoint directory path set drifted"
}

take_source_snapshots() {
  local retry1_regular="$1"
  local retry1_directories="$2"
  local endpoint_regular="$3"
  local endpoint_directories_path="$4"
  assert_source_lock_identity
  verify_interruption_static_authority
  verify_tree_safety "${source_runtime}"
  verify_tree_safety "${source_endpoint}"
  emit_regular_inventory "${source_runtime}" >"${retry1_regular}"
  emit_directory_inventory "${source_runtime}" >"${retry1_directories}"
  verify_retry1_snapshot_aggregate "${retry1_regular}" \
    "${retry1_directories}"
  emit_regular_inventory "${source_endpoint}" >"${endpoint_regular}"
  emit_directory_inventory "${source_endpoint}" >"${endpoint_directories_path}"
  verify_endpoint_snapshot_aggregate "${endpoint_regular}" \
    "${endpoint_directories_path}"
  assert_source_lock_identity
}

verify_endpoint_exact_tree() {
  verify_tree_safety "${source_endpoint}"
  local entry rel dir_count=0 file_count=0
  local -A allowed_dirs=() allowed_files=()
  for rel in "${endpoint_directories[@]}"; do
    allowed_dirs["${rel}"]=1
  done
  for rel in "${endpoint_relpaths[@]}"; do
    allowed_files["${rel}"]=1
  done
  while IFS= read -r -d '' entry; do
    rel="$(relative_path_of "${source_endpoint}" "${entry}")"
    if [[ -d "${entry}" ]]; then
      [[ -n "${allowed_dirs[${rel}]+present}" ]] ||
        fail "endpoint source contains unknown directory: ${rel}"
      ((dir_count += 1))
    elif [[ -f "${entry}" ]]; then
      [[ -n "${allowed_files[${rel}]+present}" ]] ||
        fail "endpoint source contains unknown file: ${rel}"
      ((file_count += 1))
    else
      fail "endpoint source contains special entry: ${rel}"
    fi
  done < <(find "${source_endpoint}" -xdev -mindepth 0 -print0)
  [[ "${dir_count}" == "${expected_endpoint_directory_count}" ]] ||
    fail "endpoint source directory count drifted"
  [[ "${file_count}" == "${expected_endpoint_file_count}" ]] ||
    fail "endpoint source file count drifted"
}

verify_endpoint_hashes_and_sizes_at() {
  local root="$1"
  local rel path total_bytes=0
  for rel in "${endpoint_relpaths[@]}"; do
    path="${root}/${rel}"
    verify_hash "${path}" "${expected_endpoint_hashes[${rel}]}" \
      "endpoint payload ${rel}"
    [[ "$(stat -c '%s' -- "${path}")" == \
      "${expected_endpoint_sizes[${rel}]}" ]] ||
      fail "endpoint payload size drifted: ${rel}"
    total_bytes=$((total_bytes + expected_endpoint_sizes[${rel}]))
  done
  [[ "${total_bytes}" == "${expected_endpoint_regular_file_bytes}" ]] ||
    fail "internal endpoint byte total drifted: ${total_bytes}"
  [[ "$(compute_content_inventory_sha256 "${root}")" == \
    "${expected_endpoint_content_inventory_sha256}" ]] ||
    fail "endpoint payload content inventory drifted at ${root}"
}

verify_source_receipts() {
  local receipts instrument interval expected_fragment
  receipts="$(kv "${source_manifest}" source_file_receipts)"
  [[ "${receipts}" != *"/data/raw/"* ]] ||
    fail "endpoint manifest references canonical data/raw"
  [[ "${receipts}" != *"/data/final/"* ]] ||
    fail "endpoint manifest references final data"
  [[ "$(grep -oF 'record_type=kline' <<<"${receipts}" | wc -l)" == 9 ]] ||
    fail "endpoint manifest does not contain exactly nine source receipts"
  for instrument in SYN2ALPHASYN2USD SYN2BETASYN2USD SYN2GAMMASYN2USD; do
    for interval in 1d 3d 1w; do
      expected_fragment="edge=${instrument}|instrument=${instrument}|interval=${interval}|record_type=kline|source=${runtime_parent}/synthetic_v2_isolated_development_source_v1/source/${instrument}/${interval}/${instrument}-${interval}-all-years.csv"
      [[ "$(grep -oF "${expected_fragment}" <<<"${receipts}" | wc -l)" == 1 ]] ||
        fail "endpoint manifest source receipt drifted: ${instrument}/${interval}"
    done
  done
}

verify_configuration_semantics() {
  expect_kv "${source_train_config}" runtime_wave_id \
    train_core_mtf_jepa_mae_vicreg
  expect_kv "${source_capture_config}" runtime_wave_id \
    cwu_02v_certified_replay_eval_mdn
  expect_kv "${source_train_config}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path \
    "${source_policy}"
  expect_kv "${source_train_config}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_net_path "${source_net}"
  expect_kv "${source_train_config}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path \
    "${repo_root}/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei.bnf"
  expect_kv "${source_train_config}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path \
    "${repo_root}/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf"
  expect_line_count "${source_policy}" \
    '  TRAINING_ID = synthetic_continuous_graph_v2_mtf_jepa_mae_vicreg_train_core_v1;' 1
  expect_line_count "${source_policy}" '  MAX_STEPS = 3000;' 1
  expect_line_count "${source_policy}" '  BATCH_SIZE = 32;' 1
  expect_line_count "${source_policy}" '  SEED = 17;' 1
  expect_line_count "${source_policy}" \
    '  AUGMENTATION_PROFILE = light_phase_safe_v2;' 1
  expect_line_count "${source_policy}" '  LAMBDA_TF_ALIGN = 0.10;' 1
  expect_line_count "${source_policy}" '  USE_TF_ALIGN_LOSS = true;' 1
  expect_line_count "${source_policy}" \
    '  USE_RAW_RECONSTRUCTION_TARGETS = true;' 1
  expect_line_count "${source_net}" '  D_MODEL = 32;' 1
  expect_line_count "${source_net}" '  LATENT_DIM = 32;' 1
  expect_line_count "${source_net}" '  USE_FREQUENCY_TOKENS = true;' 1
}

verify_status_manifest_report_state() {
  expect_kv "${source_status}" schema_id "${source_schema_id}.training.v1"
  expect_kv "${source_status}" status complete
  expect_kv "${source_status}" arm endpoint_scale
  expect_kv "${source_status}" runner_sha256 \
    "${expected_retry1_runner_sha256}"
  expect_kv "${source_status}" input_receipt_sha256 \
    "${expected_retry1_inputs_sha256}"
  expect_kv "${source_status}" retry_attempt_sentinel_sha256 \
    "${expected_retry1_attempt_sha256}"
  expect_kv "${source_status}" policy_path "${source_policy}"
  expect_kv "${source_status}" policy_sha256 \
    "${expected_endpoint_hashes[config/representation.jkimyei]}"
  expect_kv "${source_status}" net_path "${source_net}"
  expect_kv "${source_status}" net_sha256 \
    "${expected_endpoint_hashes[config/representation.net]}"
  expect_kv "${source_status}" config_path "${source_train_config}"
  expect_kv "${source_status}" config_sha256 \
    "${expected_endpoint_hashes[config/train.config]}"
  expect_kv "${source_status}" checkpoint_path "${source_checkpoint}"
  expect_kv "${source_status}" checkpoint_sha256 "${expected_checkpoint_sha256}"
  expect_kv "${source_status}" job_manifest_path "${source_manifest}"
  expect_kv "${source_status}" job_manifest_sha256 \
    "${expected_endpoint_hashes[training/job/job.manifest]}"
  expect_kv "${source_status}" runtime_result_path "${source_result}"
  expect_kv "${source_status}" runtime_result_sha256 \
    "${expected_endpoint_hashes[training/job/runtime.result.fact]}"
  expect_kv "${source_status}" representation_report_path "${source_report}"
  expect_kv "${source_status}" representation_report_sha256 \
    "${expected_endpoint_hashes[training/job/channel_representation.report]}"
  expect_kv "${source_status}" train_anchor_range \
    "[${expected_anchor_begin},${expected_anchor_end})"
  expect_kv "${source_status}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${source_status}" seed "${expected_seed}"
  expect_kv "${source_status}" forecast_labels_used false
  expect_kv "${source_status}" canonical_data_raw_access false
  expect_kv "${source_status}" final_holdout_access false
  expect_kv "${source_status}" policy_access false

  expect_kv "${source_manifest}" manifest_format \
    kikijyeba.runtime.job_manifest.v1
  expect_kv "${source_manifest}" job_id "${expected_job_id}"
  expect_kv "${source_manifest}" job_attempt_policy \
    explicit_job_dir_no_overwrite_v1
  expect_kv "${source_manifest}" job_kind \
    channel_representation_mtf_jepa_mae_vicreg
  expect_kv "${source_manifest}" config_path "${source_train_config}"
  expect_kv "${source_manifest}" component_family_id \
    "${expected_component_family}"
  expect_kv "${source_manifest}" component_spawn_fingerprint \
    c3cfe3b2ed5adcdd
  expect_kv "${source_manifest}" component_operator_surface_digest \
    b5ff3f49dfef83cf
  expect_kv "${source_manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${source_manifest}" wave_action train
  expect_kv "${source_manifest}" execution_chain \
    "${expected_execution_chain}"
  expect_kv "${source_manifest}" mutated_components \
    "${expected_component_family}"
  expect_kv "${source_manifest}" frozen_components ''
  expect_kv "${source_manifest}" source_range_policy anchor_index
  expect_kv "${source_manifest}" source_order_policy random_per_epoch
  expect_kv "${source_manifest}" source_order_policy_explicit true
  expect_kv "${source_manifest}" resolved_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_manifest}" resolved_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${source_manifest}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${source_manifest}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${source_manifest}" skipped_outside_common_range 0
  expect_kv "${source_manifest}" skipped_missing_edge_coverage 0
  expect_kv "${source_manifest}" skipped_failed_fetch_probe 0
  expect_kv "${source_manifest}" duplicate_anchor_count 0
  expect_kv "${source_manifest}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${source_manifest}" protocol_contract_fingerprint \
    afcb1bf10ba11fc3
  expect_kv "${source_manifest}" graph_order_fingerprint 4133db527907a8e4
  expect_kv "${source_manifest}" \
    mtf_jepa_mae_vicreg_assembly_fingerprint f950757262787454
  expect_kv "${source_manifest}" input_representation_checkpoint_path ''
  expect_kv "${source_manifest}" input_mdn_checkpoint_path ''
  expect_kv "${source_manifest}" runtime_handoff_id ''
  expect_kv "${source_manifest}" runtime_handoff_digest ''
  expect_kv "${source_manifest}" policy_training_contract_schema ''
  expect_kv "${source_manifest}" policy_family_id ''
  verify_source_receipts

  expect_kv "${source_report}" report_schema_id \
    wikimyei.representation.mtf_jepa_mae_vicreg.report.v1
  expect_kv "${source_report}" training_id \
    synthetic_continuous_graph_v2_mtf_jepa_mae_vicreg_train_core_v1
  expect_kv "${source_report}" config_path "${source_train_config}"
  expect_kv "${source_report}" component_assembly_id mtf_jepa_mae_vicreg_v1
  expect_kv "${source_report}" graph_order_fingerprint 4133db527907a8e4
  expect_kv "${source_report}" d_model 32
  expect_kv "${source_report}" encoding_dim 32
  expect_kv "${source_report}" augmentation_profile light_phase_safe_v2
  expect_kv "${source_report}" use_frequency_tokens true
  expect_kv "${source_report}" lambda_tf_align 0.1
  expect_kv "${source_report}" use_tf_align_loss true
  expect_kv "${source_report}" effective_batch_size 32
  expect_kv "${source_report}" seed "${expected_seed}"
  expect_kv "${source_report}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${source_report}" requested_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_report}" requested_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${source_report}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${source_report}" steps_attempted "${expected_optimizer_steps}"
  expect_kv "${source_report}" steps_completed "${expected_optimizer_steps}"
  expect_kv "${source_report}" skipped_batches 0
  expect_kv "${source_report}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${source_report}" wave_pulses_completed \
    "${expected_optimizer_steps}"
  expect_kv "${source_report}" finite_parameter_check true
  expect_kv "${source_report}" gradients_finite true
  expect_kv "${source_report}" nonfinite_output_count 0
  expect_kv "${source_report}" checkpoint_written true
  expect_kv "${source_report}" checkpoint_write_count 60
  expect_kv "${source_report}" last_checkpoint_optimizer_step \
    "${expected_optimizer_steps}"
  expect_kv "${source_report}" checkpoint_path "${source_checkpoint}"
  expect_kv "${source_report}" checkpoint_path_reported "${source_checkpoint}"
  expect_kv "${source_report}" checkpoint_digest_reported \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_report}" checkpoint_digest_verified true
  expect_kv "${source_report}" checkpoint_file_exists true
  expect_kv "${source_report}" checkpoint_bytes 853931
  expect_kv "${source_report}" checkpoint_artifact_status verified
  expect_kv "${source_report}" report_written true
  expect_kv "${source_report}" report_write_count 300
  expect_kv "${source_report}" last_report_attempted_step \
    "${expected_optimizer_steps}"

  expect_kv "${source_job_state}" state_format \
    kikijyeba.runtime.job_state.v1
  expect_kv "${source_job_state}" job_id "${expected_job_id}"
  expect_kv "${source_job_state}" status completed
  expect_kv "${source_job_state}" error_message ''
  expect_kv "${source_job_state}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${source_job_state}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${source_job_state}" resolved_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_job_state}" resolved_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${source_job_state}" steps_attempted "${expected_optimizer_steps}"
  expect_kv "${source_job_state}" steps_completed "${expected_optimizer_steps}"
  expect_kv "${source_job_state}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${source_job_state}" checkpoint_written true
  expect_kv "${source_job_state}" checkpoint_write_count 60
  expect_kv "${source_job_state}" checkpoint_path "${source_checkpoint}"
  expect_kv "${source_job_state}" report_written true
  expect_kv "${source_job_state}" report_write_count 300
  expect_kv "${source_job_state}" report_path "${source_report}"
  expect_kv "${source_job_state}" lattice_checkpoint_fact_written true
  expect_kv "${source_job_state}" lattice_checkpoint_fact_path \
    "${source_lattice_checkpoint}"
  expect_kv "${source_job_state}" lattice_exposure_fact_written true
  expect_kv "${source_job_state}" lattice_exposure_fact_path \
    "${source_lattice_exposure}"
  expect_kv "${source_job_state}" lattice_source_analytics_fact_written true
  expect_kv "${source_job_state}" lattice_source_analytics_fact_path \
    "${source_lattice_source}"
  expect_kv "${source_job_state}" runtime_result_fact_written true
  expect_kv "${source_job_state}" runtime_result_fact_path "${source_result}"
  expect_kv "${source_job_state}" runtime_checkpoint_io_fact_written true
  expect_kv "${source_job_state}" runtime_checkpoint_io_fact_path \
    "${source_checkpoint_io}"
  expect_kv "${source_job_state}" runtime_health_measurement_fact_written true
  expect_kv "${source_job_state}" runtime_health_measurement_fact_path \
    "${source_health}"
  expect_kv "${source_job_state}" runtime_terminal_fact_error ''
  expect_kv "${source_job_state}" replay_artifacts_written false
  expect_kv "${source_job_state}" representation_edge_feature_probe_written \
    false
  expect_kv "${source_job_state}" probe_sidecar_enabled true
  expect_kv "${source_job_state}" probe_records_written true
  expect_kv "${source_job_state}" probe_stream_path "${source_events}"
  expect_kv "${source_job_state}" probe_record_count \
    "${expected_event_record_count}"
}

verify_lattice_and_runtime_facts() {
  expect_kv "${source_lattice_checkpoint}" schema \
    kikijyeba.lattice.checkpoint.v1
  expect_kv "${source_lattice_checkpoint}" fact_type checkpoint_identity
  expect_kv "${source_lattice_checkpoint}" checkpoint_path \
    "${source_checkpoint}"
  expect_kv "${source_lattice_checkpoint}" checkpoint_file_digest \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_lattice_checkpoint}" generation_id 396a8345c81eaf39
  expect_kv "${source_lattice_checkpoint}" read_checkpoint_digests ''
  expect_kv "${source_lattice_checkpoint}" \
    producer_component_update_fact_digest b141c8c63f054bab
  expect_kv "${source_lattice_checkpoint}" fit_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_checkpoint}" fit_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_checkpoint}" valid_from_anchor \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_checkpoint}" influence_complete true
  expect_kv "${source_lattice_checkpoint}" influence_coverage_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_checkpoint}" influence_coverage_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_checkpoint}" \
    influence_label_or_reward_availability_end_exclusive_max \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_checkpoint}" component \
    "${expected_component_family}"
  expect_kv "${source_lattice_checkpoint}" graph_order_fingerprint \
    4133db527907a8e4
  expect_kv "${source_lattice_checkpoint}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${source_lattice_checkpoint}" created_by_job_id \
    "${expected_job_id}"
  expect_kv "${source_lattice_checkpoint}" input_checkpoints ''
  expect_kv "${source_lattice_checkpoint}" direct_exposure_digest \
    57834b8041b41f01
  expect_kv "${source_lattice_checkpoint}" fact_digest 65afee081363d346

  expect_kv "${source_lattice_exposure}" schema \
    kikijyeba.lattice.exposure.v1
  expect_kv "${source_lattice_exposure}" fact_type exposure
  expect_kv "${source_lattice_exposure}" target_component_family_id \
    "${expected_component_family}"
  expect_kv "${source_lattice_exposure}" job_id "${expected_job_id}"
  expect_kv "${source_lattice_exposure}" job_status completed
  expect_kv "${source_lattice_exposure}" wave_action train
  expect_kv "${source_lattice_exposure}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${source_lattice_exposure}" anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_exposure}" anchor_end "${expected_anchor_end}"
  expect_kv "${source_lattice_exposure}" completed_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_exposure}" completed_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_exposure}" use_target_supervision false
  expect_kv "${source_lattice_exposure}" use_selection_signal false
  expect_kv "${source_lattice_exposure}" mutated_component true
  expect_kv "${source_lattice_exposure}" batches_seen \
    "${expected_optimizer_steps}"
  expect_kv "${source_lattice_exposure}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${source_lattice_exposure}" output_checkpoint \
    "${source_checkpoint}"
  expect_kv "${source_lattice_exposure}" checkpoint_digest_reported \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_lattice_exposure}" checkpoint_digest_verified true
  expect_kv "${source_lattice_exposure}" checkpoint_bytes 853931
  expect_kv "${source_lattice_exposure}" checkpoint_artifact_status verified
  expect_kv "${source_lattice_exposure}" input_checkpoints ''
  expect_kv "${source_lattice_exposure}" finite_loss true
  expect_kv "${source_lattice_exposure}" nonfinite_output_count 0
  expect_kv "${source_lattice_exposure}" finite_parameter_check true
  expect_kv "${source_lattice_exposure}" representation_health_available true
  expect_kv "${source_lattice_exposure}" fact_digest 57834b8041b41f01

  expect_kv "${source_lattice_source}" schema \
    kikijyeba.lattice.source_analytics.v1
  expect_kv "${source_lattice_source}" fact_type source_analytics
  expect_kv "${source_lattice_source}" parent_exposure_fact_digest \
    57834b8041b41f01
  expect_kv "${source_lattice_source}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${source_lattice_source}" job_id "${expected_job_id}"
  expect_kv "${source_lattice_source}" job_status completed
  expect_kv "${source_lattice_source}" anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_source}" anchor_end "${expected_anchor_end}"
  expect_kv "${source_lattice_source}" completed_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_lattice_source}" completed_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${source_lattice_source}" source_receipt_fact_count 9
  expect_kv "${source_lattice_source}" sample_validity_fraction 1
  expect_kv "${source_lattice_source}" missingness_fraction 0
  expect_kv "${source_lattice_source}" duplicate_sample_count 0
  expect_kv "${source_lattice_source}" source_health_level ok
  expect_kv "${source_lattice_source}" visibility_only true
  expect_kv "${source_lattice_source}" readiness_authority false
  expect_kv "${source_lattice_source}" coverage_authority false
  expect_kv "${source_lattice_source}" leakage_authority false
  expect_kv "${source_lattice_source}" fact_digest db6398566980cc5e

  expect_kv "${source_component_update}" schema \
    runtime.component_training_update.v1
  expect_kv "${source_component_update}" component_id \
    "${expected_component_family}"
  expect_kv "${source_component_update}" component_role representation
  expect_kv "${source_component_update}" job_id "${expected_job_id}"
  expect_kv "${source_component_update}" update_kind train
  expect_kv "${source_component_update}" fit_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${source_component_update}" fit_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${source_component_update}" read_checkpoint_digests ''
  expect_kv "${source_component_update}" write_checkpoint_digest \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_component_update}" write_generation_id \
    396a8345c81eaf39
  expect_kv "${source_component_update}" optimizer_update_count \
    "${expected_optimizer_steps}"
  expect_kv "${source_component_update}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${source_component_update}" valid_from_anchor \
    "${expected_anchor_end}"
  expect_kv "${source_component_update}" \
    label_or_reward_availability_end_exclusive_max "${expected_anchor_end}"
  expect_kv "${source_component_update}" fact_digest b141c8c63f054bab

  expect_kv "${source_checkpoint_io}" fact_type runtime.checkpoint_io.fact
  expect_kv "${source_checkpoint_io}" job_id "${expected_job_id}"
  expect_kv "${source_checkpoint_io}" component_family_id \
    "${expected_component_family}"
  expect_kv "${source_checkpoint_io}" checkpoint_path "${source_checkpoint}"
  expect_kv "${source_checkpoint_io}" checkpoint_path_reported \
    "${source_checkpoint}"
  expect_kv "${source_checkpoint_io}" checkpoint_written true
  expect_kv "${source_checkpoint_io}" checkpoint_write_count 60
  expect_kv "${source_checkpoint_io}" checkpoint_artifact_digest \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_checkpoint_io}" checkpoint_digest_reported \
    "${expected_checkpoint_internal_digest}"
  expect_kv "${source_checkpoint_io}" checkpoint_digest_verified true
  expect_kv "${source_checkpoint_io}" checkpoint_file_exists true
  expect_kv "${source_checkpoint_io}" checkpoint_bytes 853931
  expect_kv "${source_checkpoint_io}" checkpoint_artifact_status verified
  expect_kv "${source_checkpoint_io}" model_state_mutated true
  expect_kv "${source_checkpoint_io}" source_report_path "${source_report}"
  expect_kv "${source_checkpoint_io}" source_report_digest b1fe7f08ae30058c
  expect_kv "${source_checkpoint_io}" fact_digest 78d087ee0ba1be3e

  expect_kv "${source_health}" fact_type runtime.health_measurement.fact
  expect_kv "${source_health}" job_id "${expected_job_id}"
  expect_kv "${source_health}" component_family_id \
    "${expected_component_family}"
  expect_kv "${source_health}" finite_parameter_check true
  expect_kv "${source_health}" nonfinite_output_count 0
  expect_kv "${source_health}" source_report_path "${source_report}"
  expect_kv "${source_health}" source_report_digest b1fe7f08ae30058c
  expect_kv "${source_health}" representation_effective_rank_fraction \
    0.721795
  expect_kv "${source_health}" fact_digest 9ad733f61c472387

  expect_kv "${source_result}" fact_type runtime.result.fact
  expect_kv "${source_result}" status completed
  expect_kv "${source_result}" error_message ''
  expect_kv "${source_result}" job_id "${expected_job_id}"
  expect_kv "${source_result}" target_component_family_id \
    "${expected_component_family}"
  expect_kv "${source_result}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${source_result}" wave_action train
  expect_kv "${source_result}" source_report_path "${source_report}"
  expect_kv "${source_result}" source_report_digest b1fe7f08ae30058c
  expect_kv "${source_result}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${source_result}" checkpoint_path "${source_checkpoint}"
  expect_kv "${source_result}" checkpoint_written true
  expect_kv "${source_result}" model_state_mutated true
  expect_kv "${source_result}" finite_parameter_check true
  expect_kv "${source_result}" nonfinite_output_count 0
  expect_kv "${source_result}" fact_digest b74e985285f9e41b
}

verify_registry_and_event_evidence() {
  expect_kv "${source_spawn_ref}" schema \
    kikijyeba.runtime.component_spawn_ref.v1
  expect_kv "${source_spawn_ref}" component_family_id \
    "${expected_component_family}"
  expect_kv "${source_spawn_ref}" component_spawn_id syv
  expect_kv "${source_spawn_ref}" component_spawn_fingerprint \
    c3cfe3b2ed5adcdd
  expect_kv "${source_spawn_ref}" component_spawn_registry_id \
    711393906c184fa9
  expect_kv "${source_spawn_ref}" config_bundle_id 78cfd10aece06832
  expect_kv "${source_spawn_ref}" config_receipt_id f1df9b17e981d6ba
  expect_kv "${source_spawn_ref}" registry_path \
    system/component_spawn_registry.v1.lls

  expect_kv "${source_spawn_registry}" schema \
    kikijyeba.component_spawn_registry.v1
  expect_kv "${source_spawn_registry}" component_spawn_registry_id \
    711393906c184fa9
  expect_kv "${source_spawn_registry}" entry_count 1
  expect_kv "${source_spawn_registry}" entry_0_component_family_id \
    "${expected_component_family}"
  expect_kv "${source_spawn_registry}" \
    entry_0_component_spawn_fingerprint c3cfe3b2ed5adcdd
  expect_kv "${source_spawn_registry}" entry_0_config_bundle_id \
    78cfd10aece06832
  expect_kv "${source_spawn_registry}" entry_0_config_receipt_id \
    f1df9b17e981d6ba
  expect_kv "${source_spawn_registry}" entry_0_first_seen_runtime_root \
    "${source_endpoint}/training"

  expect_kv "${source_runtime_layout}" schema kikijyeba.runtime.layout.v1
  expect_kv "${source_runtime_layout}" runtime_root \
    "${source_endpoint}/training"
  expect_kv "${source_runtime_layout}" system_dir system
  expect_kv "${source_runtime_layout}" components_dir components

  awk -F= -v expected="${expected_event_record_count}" '
    $1 == "record_schema" {
      records += 1;
      if ($2 != "kikijyeba.runtime.job_events.probe_record.v1") bad = 1;
    }
    $1 == "sequence" {
      sequences += 1;
      if (($2 + 0) != sequences) bad = 1;
    }
    END {
      if (bad || records != expected || sequences != expected) exit 1;
    }
  ' "${source_events}" ||
    fail "endpoint Runtime event stream record sequence drifted"
  expect_line_count "${source_events}" "event_kind=job.lifecycle.started" 1
  expect_line_count "${source_events}" "event_kind=job.lifecycle.completed" 1
  expect_line_count "${source_events}" "status=completed" 2
  expect_line_count "${source_events}" "sequence=17404" 1
  expect_line_count "${source_events}" "event_kind=job.artifact.published" \
    "${expected_event_artifact_published_count}"
  expect_line_count "${source_events}" "artifact_kind=delegated_report" 1
  expect_line_count "${source_events}" "artifact_kind=checkpoint" 1
  expect_line_count "${source_events}" "artifact_kind=runtime_result_fact" 1
  expect_line_count "${source_events}" \
    "artifact_kind=runtime_checkpoint_io_fact" 1
  expect_line_count "${source_events}" \
    "artifact_kind=runtime_health_measurement_fact" 1
  expect_line_count "${source_events}" "artifact_kind=lattice_exposure_fact" 1
  expect_line_count "${source_events}" \
    "artifact_kind=lattice_source_analytics_fact" 1
  expect_line_count "${source_events}" \
    "artifact_kind=lattice_checkpoint_fact" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_report}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_checkpoint}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_result}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_checkpoint_io}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_health}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_lattice_exposure}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_lattice_source}" 1
  expect_line_count "${source_events}" \
    "artifact_path=${source_lattice_checkpoint}" 1
  expect_line_count "${source_events}" "artifact_digest=b74e985285f9e41b" 1
  expect_line_count "${source_events}" "artifact_digest=9ad733f61c472387" 1
  expect_line_count "${source_events}" "artifact_digest=78d087ee0ba1be3e" 1
  expect_line_count "${source_events}" "artifact_digest=57834b8041b41f01" 1
  expect_line_count "${source_events}" "artifact_digest=db6398566980cc5e" 1
  expect_line_count "${source_events}" "artifact_digest=65afee081363d346" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.runtime.result.fact.v1" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.runtime.checkpoint_io.fact.v1" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.runtime.health_measurement.fact.v1" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.lattice.exposure.v1" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.lattice.source_analytics.v1" 1
  expect_line_count "${source_events}" \
    "artifact_schema=kikijyeba.lattice.checkpoint.v1" 1
  [[ "$(grep -c '^artifact_digest=' "${source_events}")" == \
    "${expected_event_artifact_digest_count}" ]] ||
    fail "endpoint Runtime event artifact digest count drifted"
  [[ "$(grep -c '^artifact_schema=' "${source_events}")" == \
    "${expected_event_artifact_schema_count}" ]] ||
    fail "endpoint Runtime event artifact schema count drifted"
  if grep -Fq 'severity=error' "${source_events}" ||
    grep -Fq 'status=failed' "${source_events}"; then
    fail "endpoint Runtime event stream contains terminal error evidence"
  fi
  grep -Fq 'status=completed' "${source_log}" ||
    fail "endpoint training log omits completion marker"
  grep -Fq 'resolved_anchor_count=2496' "${source_log}" ||
    fail "endpoint training log omits resolved anchor count"
}

verify_endpoint_semantics() {
  assert_source_lock_identity
  verify_hash "${retry1_runner}" "${expected_retry1_runner_sha256}" \
    "retry1 runner"
  verify_hash "${retry1_frozen_runner}" "${expected_retry1_runner_sha256}" \
    "retry1 frozen runner"
  verify_hash "${retry1_attempt}" "${expected_retry1_attempt_sha256}" \
    "retry1 attempt sentinel"
  verify_hash "${retry1_inputs}" "${expected_retry1_inputs_sha256}" \
    "retry1 input receipt"
  verify_endpoint_exact_tree
  verify_endpoint_hashes_and_sizes_at "${source_endpoint}"
  verify_configuration_semantics
  verify_status_manifest_report_state
  verify_lattice_and_runtime_facts
  verify_registry_and_event_evidence
  [[ "${expected_checkpoint_sha256}" != \
    "${expected_checkpoint_internal_digest}" ]] ||
    fail "checkpoint file SHA-256 and internal digest were conflated"
  [[ ! -e "${source_job}/channel_inference.report" ]] ||
    fail "endpoint representation job contains a forecast report"
  [[ ! -e "${source_job}/channel_policy.report" ]] ||
    fail "endpoint representation job contains a policy report"
  if find "${source_endpoint}" -xdev \
    \( -path '*time_only*' -o -path '*no_tf_alignment*' \) \
    -print -quit | grep -q .; then
    fail "endpoint source contains another-arm path"
  fi
  if grep -R -I -Fq '/arms/time_only/' "${source_endpoint}" ||
    grep -R -I -Fq '/arms/no_tf_alignment/' "${source_endpoint}"; then
    fail "endpoint source text references another retry1 arm"
  fi
  assert_source_lock_identity
}

emit_receipt() {
  local destination="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  local index rel
  cat >"${destination}" <<RECEIPT_HEAD
schema_id=${schema_id}
status=complete
bundle_kind=immutable_historical_endpoint_evidence
historical_evidence_only=true
bundle_runtime_root=${bundle_root}
bundle_snapshot_root=${bundle_snapshot}
bundle_receipt_path=${bundle_receipt}
publication_method=atomic_no_clobber_directory_rename
scratch_parent=${runtime_parent}
endpoint_bundle_owned_tmp_usage=false
endpoint_bundle_payload_under_tmp=false
prerequisite_interruption_verifier_invoked=true
prerequisite_interruption_verifier_may_use_sealed_small_tmp_snapshots=true
prerequisite_interruption_verifier_cleans_tmp_snapshots=true
large_payload_tmp_usage=false
source_runtime_schema_id=${source_schema_id}
source_runtime_root=${source_runtime}
source_endpoint_root=${source_endpoint}
source_runtime_mutated=false
source_lock_path=${source_lock_path}
source_lock_acquired_read_only=true
source_lock_held_through_two_snapshots_and_copy=true
source_lock_fd_path_identity_rechecked=true
source_snapshot_count=2
source_snapshots_identical=true
source_regular_inventory_path=${bundle_regular_inventory}
source_regular_inventory_sha256=$(sha256_of "${regular_inventory}")
source_directory_inventory_path=${bundle_directory_inventory}
source_directory_inventory_sha256=$(sha256_of "${directory_inventory}")
endpoint_regular_file_count=${expected_endpoint_file_count}
endpoint_directory_count_including_root=${expected_endpoint_directory_count}
endpoint_descendant_directory_count=${expected_endpoint_descendant_directory_count}
endpoint_regular_file_bytes=${expected_endpoint_regular_file_bytes}
endpoint_content_inventory_algorithm=cd_endpoint_root_find_dot_xdev_regular_files_c_sort_z_sha256sum_then_sha256sum
endpoint_content_inventory_sha256=${expected_endpoint_content_inventory_sha256}
endpoint_symlink_count=0
endpoint_special_entry_count=0
endpoint_cross_device_entry_count=0
endpoint_regular_file_nonunit_link_count=0
copy_method=cp_reflink_never
copy_byte_identity_verified_with_cmp=true
copy_sha256_identity_verified=true
copy_source_destination_inode_tuple_distinct=true
copy_source_link_count=1
copy_destination_link_count=1
interruption_closure_schema_id=${interruption_schema_id}
interruption_closure_root=${interruption_root}
interruption_closure_receipt_path=${interruption_receipt}
interruption_closure_receipt_sha256=${expected_interruption_receipt_sha256}
interruption_closure_regular_inventory_path=${interruption_regular_inventory}
interruption_closure_regular_inventory_sha256=${expected_interruption_regular_inventory_sha256}
interruption_closure_directory_inventory_path=${interruption_directory_inventory}
interruption_closure_directory_inventory_sha256=${expected_interruption_directory_inventory_sha256}
interruption_closure_sealer_path=${interruption_sealer}
interruption_closure_sealer_sha256=${expected_interruption_sealer_sha256}
interruption_closure_frozen_sealer_path=${interruption_frozen_sealer}
interruption_closure_frozen_sealer_sha256=${expected_interruption_sealer_sha256}
interruption_amendment_path=${interruption_amendment}
interruption_amendment_sha256=${expected_interruption_amendment_sha256}
interruption_frozen_amendment_path=${interruption_frozen_amendment}
interruption_frozen_amendment_sha256=${expected_interruption_amendment_sha256}
retry1_complete_regular_file_count=${expected_retry1_regular_file_count}
retry1_complete_directory_count=${expected_retry1_directory_count}
retry1_complete_regular_file_bytes=${expected_retry1_regular_file_bytes}
retry1_complete_content_inventory_sha256=${expected_retry1_content_inventory_sha256}
endpoint_bundle_sealer_path=${script_path}
endpoint_bundle_sealer_process_start_sha256=${process_start_sealer_sha256}
endpoint_bundle_sealer_process_start_mode=0${process_start_sealer_mode}
endpoint_bundle_sealer_process_start_links=${process_start_sealer_links}
endpoint_bundle_sealer_process_start_owner_uid=${process_start_sealer_uid}
endpoint_bundle_sealer_process_start_bytes=${process_start_sealer_bytes}
endpoint_bundle_sealer_process_start_inode=${process_start_sealer_inode}
endpoint_bundle_sealer_process_start_device=${process_start_sealer_device}
frozen_endpoint_bundle_sealer_path=${bundle_frozen_sealer}
frozen_endpoint_bundle_sealer_sha256=${process_start_sealer_sha256}
endpoint_bundle_amendment_path=${amendment_path}
endpoint_bundle_amendment_sha256=${expected_amendment_sha256}
frozen_endpoint_bundle_amendment_path=${bundle_frozen_amendment}
frozen_endpoint_bundle_amendment_sha256=${expected_amendment_sha256}
retry1_runner_path=${retry1_runner}
retry1_runner_sha256=${expected_retry1_runner_sha256}
retry1_attempt_path=${retry1_attempt}
retry1_attempt_sha256=${expected_retry1_attempt_sha256}
retry1_input_receipt_path=${retry1_inputs}
retry1_input_receipt_sha256=${expected_retry1_inputs_sha256}
endpoint_training_status=complete
endpoint_optimizer_steps=${expected_optimizer_steps}
endpoint_seed=${expected_seed}
endpoint_train_anchor_range=[${expected_anchor_begin},${expected_anchor_end})
endpoint_accepted_anchor_count=${expected_anchor_count}
endpoint_candidate_anchor_count=${expected_anchor_count}
endpoint_checkpoint_source_path=${source_checkpoint}
endpoint_checkpoint_bundle_path=${bundle_snapshot}/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt
endpoint_checkpoint_file_sha256=${expected_checkpoint_sha256}
endpoint_checkpoint_internal_digest_reported=${expected_checkpoint_internal_digest}
endpoint_checkpoint_file_sha256_distinct_from_internal_digest=true
endpoint_job_state_status=completed
endpoint_runtime_result_status=completed
endpoint_finite_parameter_check=true
endpoint_nonfinite_output_count=0
endpoint_input_checkpoint_present=false
endpoint_forecast_artifact_present=false
endpoint_policy_artifact_present=false
endpoint_event_record_count=${expected_event_record_count}
endpoint_event_lifecycle_started_count=1
endpoint_event_lifecycle_completed_count=1
endpoint_event_artifact_published_count=${expected_event_artifact_published_count}
endpoint_event_artifact_digest_count=${expected_event_artifact_digest_count}
endpoint_event_artifact_schema_count=${expected_event_artifact_schema_count}
time_only_path_included=false
time_only_artifact_reuse_authorized=false
no_tf_alignment_path_included=false
no_tf_alignment_artifact_reuse_authorized=false
direct_retry1_use_authorized=false
direct_bundle_use_authorized=false
retry2_local_consumption_authorized=false
endpoint_import_authorized_by_this_bundle=false
separate_retry2_stage_verifier_required=true
retry2_exact_scientific_config_equivalence_required=true
retry2_second_local_copy_required=true
retry2_second_copy_requires_reflink_never=true
retry2_second_copy_requires_distinct_inode_tuple=true
retry2_second_copy_requires_link_count_one=true
retry2_runtime_schema_id=${retry2_schema_id}
retry2_runtime_root=${retry2_runtime}
retry2_endpoint_consumption=retry2_local_second_copy_only_after_stage_verifier
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
forecast_access=false
policy_access=false
RECEIPT_HEAD
  for index in "${!endpoint_relpaths[@]}"; do
    rel="${endpoint_relpaths[${index}]}"
    printf 'endpoint_file_%02d_relative_path=%s\n' "${index}" "${rel}" \
      >>"${destination}"
    printf 'endpoint_file_%02d_sha256=%s\n' "${index}" \
      "${expected_endpoint_hashes[${rel}]}" >>"${destination}"
    printf 'endpoint_file_%02d_bytes=%s\n' "${index}" \
      "${expected_endpoint_sizes[${rel}]}" >>"${destination}"
  done
}

verify_receipt_key_shape() {
  local inspected_receipt="$1"
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || key ~ /[[:space:]]/ || seen[key]++) exit 42;
    }
  ' "${inspected_receipt}" ||
    fail "endpoint-bundle receipt has a malformed or duplicate key"
}

verify_receipt_exact() {
  local inspected_receipt="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  reject_symlink_components "${inspected_receipt}"
  require_file "${inspected_receipt}"
  [[ "$(stat -c '%a' -- "${inspected_receipt}")" == 444 ]] ||
    fail "endpoint-bundle receipt must be mode 0444"
  [[ "$(stat -c '%h' -- "${inspected_receipt}")" == 1 ]] ||
    fail "endpoint-bundle receipt has an unexpected hard-link count"
  local expected_receipt
  new_temp_file expected_receipt "receipt.expected"
  emit_receipt "${expected_receipt}" "${regular_inventory}" \
    "${directory_inventory}"
  verify_receipt_key_shape "${expected_receipt}"
  verify_receipt_key_shape "${inspected_receipt}"
  cmp -s -- "${expected_receipt}" "${inspected_receipt}" ||
    fail "endpoint-bundle receipt content drifted"
}

verify_source_destination_copy() {
  local root="$1"
  local rel source_path destination_path source_tuple destination_tuple
  assert_source_lock_identity
  for rel in "${endpoint_relpaths[@]}"; do
    source_path="${source_endpoint}/${rel}"
    destination_path="${root}/source_snapshot/${rel}"
    verify_hash "${source_path}" "${expected_endpoint_hashes[${rel}]}" \
      "endpoint source ${rel}"
    verify_hash "${destination_path}" "${expected_endpoint_hashes[${rel}]}" \
      "endpoint bundle copy ${rel}"
    [[ "$(stat -c '%s' -- "${source_path}")" == \
      "${expected_endpoint_sizes[${rel}]}" ]] ||
      fail "endpoint source size drifted during copy validation: ${rel}"
    [[ "$(stat -c '%s' -- "${destination_path}")" == \
      "${expected_endpoint_sizes[${rel}]}" ]] ||
      fail "endpoint destination size drifted during copy validation: ${rel}"
    cmp -s -- "${source_path}" "${destination_path}" ||
      fail "endpoint source and copied bytes differ: ${rel}"
    source_tuple="$(stat -c '%d:%i' -- "${source_path}")"
    destination_tuple="$(stat -c '%d:%i' -- "${destination_path}")"
    [[ "${source_tuple}" != "${destination_tuple}" ]] ||
      fail "endpoint destination aliases source inode: ${rel}"
    [[ "$(stat -c '%h' -- "${source_path}")" == 1 ]] ||
      fail "endpoint source link count drifted: ${rel}"
    [[ "$(stat -c '%h' -- "${destination_path}")" == 1 ]] ||
      fail "endpoint destination link count drifted: ${rel}"
  done
  assert_source_lock_identity
}

verify_bundle_tree_at() {
  local root="$1"
  local expected_regular_inventory="$2"
  local expected_directory_inventory="$3"
  local snapshot="${root}/source_snapshot"
  local receipt="${root}/endpoint_bundle.status"
  local regular_inventory="${root}/source_regular_files.inventory.tsv"
  local directory_inventory="${root}/source_directories.inventory.tsv"
  local frozen_root="${root}/frozen_sources"
  local frozen_sealer="${frozen_root}/$(basename "${script_path}")"
  local frozen_amendment="${frozen_root}/$(basename "${amendment_path}")"

  verify_tree_safety "${root}"
  local entry rel file_count=0 directory_count=0 writable
  local -A allowed_dirs=() allowed_files=()
  allowed_dirs["."]=1
  allowed_dirs["frozen_sources"]=1
  for rel in "${endpoint_directories[@]}"; do
    if [[ "${rel}" == "." ]]; then
      allowed_dirs["source_snapshot"]=1
    else
      allowed_dirs["source_snapshot/${rel}"]=1
    fi
  done
  allowed_files["endpoint_bundle.status"]=1
  allowed_files["source_regular_files.inventory.tsv"]=1
  allowed_files["source_directories.inventory.tsv"]=1
  allowed_files["frozen_sources/$(basename "${script_path}")"]=1
  allowed_files["frozen_sources/$(basename "${amendment_path}")"]=1
  for rel in "${endpoint_relpaths[@]}"; do
    allowed_files["source_snapshot/${rel}"]=1
  done

  while IFS= read -r -d '' entry; do
    rel="$(relative_path_of "${root}" "${entry}")"
    if [[ -d "${entry}" ]]; then
      [[ -n "${allowed_dirs[${rel}]+present}" ]] ||
        fail "endpoint bundle contains unknown directory: ${rel}"
      [[ "$(stat -c '%a' -- "${entry}")" == 555 ]] ||
        fail "endpoint-bundle directory is not mode 0555: ${rel}"
      ((directory_count += 1))
    elif [[ -f "${entry}" ]]; then
      [[ -n "${allowed_files[${rel}]+present}" ]] ||
        fail "endpoint bundle contains unknown file: ${rel}"
      [[ "$(stat -c '%a' -- "${entry}")" == 444 ]] ||
        fail "endpoint-bundle file is not mode 0444: ${rel}"
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "endpoint-bundle file has nonunit link count: ${rel}"
      ((file_count += 1))
    else
      fail "endpoint bundle contains special entry: ${rel}"
    fi
  done < <(find "${root}" -xdev -mindepth 0 -print0)
  [[ "${directory_count}" == 11 ]] ||
    fail "endpoint bundle expected 11 directories, found ${directory_count}"
  [[ "${file_count}" == 26 ]] ||
    fail "endpoint bundle expected 26 files, found ${file_count}"

  cmp -s -- "${expected_regular_inventory}" "${regular_inventory}" ||
    fail "published endpoint source regular inventory drifted"
  cmp -s -- "${expected_directory_inventory}" "${directory_inventory}" ||
    fail "published endpoint source directory inventory drifted"
  [[ "$(sha256_of "${frozen_sealer}")" == \
    "${process_start_sealer_sha256}" ]] ||
    fail "frozen endpoint-bundle sealer hash drifted"
  cmp -s -- "${script_path}" "${frozen_sealer}" ||
    fail "live and frozen endpoint-bundle sealers differ"
  [[ "$(sha256_of "${frozen_amendment}")" == \
    "${expected_amendment_sha256}" ]] ||
    fail "frozen endpoint-bundle amendment hash drifted"
  cmp -s -- "${amendment_path}" "${frozen_amendment}" ||
    fail "live and frozen endpoint-bundle amendments differ"
  verify_endpoint_hashes_and_sizes_at "${snapshot}"
  verify_source_destination_copy "${root}"
  verify_receipt_exact "${receipt}" "${regular_inventory}" \
    "${directory_inventory}"
  writable="$(find "${root}" -xdev -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "endpoint bundle contains a writable entry: ${writable}"
}

build_staging_bundle() {
  local regular_inventory="$1"
  local directory_inventory="$2"
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "endpoint-bundle staging already exists: ${staging_root}"
  mkdir -- "${staging_root}"
  candidate_created_by_process=true
  mkdir -- "${staging_root}/frozen_sources"
  mkdir -- "${staging_root}/source_snapshot"
  local rel
  for rel in "${endpoint_directories[@]}"; do
    [[ "${rel}" == "." ]] && continue
    mkdir -p -- "${staging_root}/source_snapshot/${rel}"
  done
  for rel in "${endpoint_relpaths[@]}"; do
    cp --reflink=never -- "${source_endpoint}/${rel}" \
      "${staging_root}/source_snapshot/${rel}"
  done
  verify_source_destination_copy "${staging_root}"
  cp --reflink=never -- "${regular_inventory}" \
    "${staging_root}/source_regular_files.inventory.tsv"
  cp --reflink=never -- "${directory_inventory}" \
    "${staging_root}/source_directories.inventory.tsv"
  cp --reflink=never -- "${script_path}" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")"
  cp --reflink=never -- "${amendment_path}" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  emit_receipt "${staging_root}/endpoint_bundle.status" \
    "${staging_root}/source_regular_files.inventory.tsv" \
    "${staging_root}/source_directories.inventory.tsv"
  find "${staging_root}" -xdev -type f -exec chmod 0444 -- {} +
  find "${staging_root}" -xdev -depth -type d -exec chmod 0555 -- {} +
}

publish_staging_bundle() {
  [[ ! -e "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "refusing to overwrite existing endpoint bundle"
  mv -T -n -- "${staging_root}" "${bundle_root}"
  if [[ -e "${staging_root}" || -L "${staging_root}" ]]; then
    fail "atomic no-clobber endpoint-bundle publication did not consume staging"
  fi
  candidate_created_by_process=false
}

assert_audit_publication_absent() {
  [[ ! -e "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "audit requires the endpoint bundle to remain unpublished"
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "audit requires the endpoint-bundle staging path to remain absent"
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

reject_symlink_components "${runtime_parent}"
require_dir "${runtime_parent}"
[[ "$(realpath -e -- "${runtime_parent}")" == "${runtime_parent}" ]] ||
  fail "runtime parent is not canonical"
if [[ "${mode}" == --audit ]]; then
  assert_audit_publication_absent
fi
verify_interruption_authority_prelock
acquire_source_lock

retry1_regular_1=""
retry1_directories_1=""
endpoint_regular_1=""
endpoint_directories_1=""
retry1_regular_2=""
retry1_directories_2=""
endpoint_regular_2=""
endpoint_directories_2=""
new_temp_file retry1_regular_1 "retry1.regular.snapshot1"
new_temp_file retry1_directories_1 "retry1.directories.snapshot1"
new_temp_file endpoint_regular_1 "endpoint.regular.snapshot1"
new_temp_file endpoint_directories_1 "endpoint.directories.snapshot1"
new_temp_file retry1_regular_2 "retry1.regular.snapshot2"
new_temp_file retry1_directories_2 "retry1.directories.snapshot2"
new_temp_file endpoint_regular_2 "endpoint.regular.snapshot2"
new_temp_file endpoint_directories_2 "endpoint.directories.snapshot2"

take_source_snapshots "${retry1_regular_1}" "${retry1_directories_1}" \
  "${endpoint_regular_1}" "${endpoint_directories_1}"
verify_endpoint_semantics

if [[ "${mode}" == --audit ]]; then
  take_source_snapshots "${retry1_regular_2}" "${retry1_directories_2}" \
    "${endpoint_regular_2}" "${endpoint_directories_2}"
  cmp -s -- "${retry1_regular_1}" "${retry1_regular_2}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${retry1_directories_1}" "${retry1_directories_2}" ||
    fail "retry1 directory snapshots differ"
  cmp -s -- "${endpoint_regular_1}" "${endpoint_regular_2}" ||
    fail "endpoint regular-file snapshots differ"
  cmp -s -- "${endpoint_directories_1}" "${endpoint_directories_2}" ||
    fail "endpoint directory snapshots differ"
  prospective_receipt_1=""
  prospective_receipt_2=""
  new_temp_file prospective_receipt_1 "receipt.audit1"
  new_temp_file prospective_receipt_2 "receipt.audit2"
  emit_receipt "${prospective_receipt_1}" "${endpoint_regular_1}" \
    "${endpoint_directories_1}"
  emit_receipt "${prospective_receipt_2}" "${endpoint_regular_2}" \
    "${endpoint_directories_2}"
  verify_receipt_key_shape "${prospective_receipt_1}"
  verify_receipt_key_shape "${prospective_receipt_2}"
  cmp -s -- "${prospective_receipt_1}" "${prospective_receipt_2}" ||
    fail "prospective endpoint-bundle receipts differ"
  assert_source_lock_identity
  assert_audit_publication_absent
  echo "retry1_endpoint_bundle_audit=complete"
  echo "endpoint_content_inventory_sha256=${expected_endpoint_content_inventory_sha256}"
  echo "source_snapshots_identical=true"
  echo "prospective_receipt_shape=valid_unique_keys"
  echo "bundle_published=false"
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  [[ -d "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "endpoint bundle is not published: ${bundle_root}"
  verify_bundle_tree_at "${bundle_root}" "${endpoint_regular_1}" \
    "${endpoint_directories_1}"
  verify_endpoint_semantics
  take_source_snapshots "${retry1_regular_2}" "${retry1_directories_2}" \
    "${endpoint_regular_2}" "${endpoint_directories_2}"
  cmp -s -- "${retry1_regular_1}" "${retry1_regular_2}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${retry1_directories_1}" "${retry1_directories_2}" ||
    fail "retry1 directory snapshots differ"
  cmp -s -- "${endpoint_regular_1}" "${endpoint_regular_2}" ||
    fail "endpoint regular-file snapshots differ"
  cmp -s -- "${endpoint_directories_1}" "${endpoint_directories_2}" ||
    fail "endpoint directory snapshots differ"
  verify_bundle_tree_at "${bundle_root}" "${endpoint_regular_2}" \
    "${endpoint_directories_2}"
  echo "endpoint_bundle_receipt=${bundle_receipt}"
  echo "endpoint_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
  exit 0
fi

if [[ -e "${bundle_root}" || -L "${bundle_root}" ]]; then
  verify_bundle_tree_at "${bundle_root}" "${endpoint_regular_1}" \
    "${endpoint_directories_1}"
  verify_endpoint_semantics
  take_source_snapshots "${retry1_regular_2}" "${retry1_directories_2}" \
    "${endpoint_regular_2}" "${endpoint_directories_2}"
  cmp -s -- "${retry1_regular_1}" "${retry1_regular_2}" ||
    fail "retry1 regular-file snapshots differ"
  cmp -s -- "${retry1_directories_1}" "${retry1_directories_2}" ||
    fail "retry1 directory snapshots differ"
  cmp -s -- "${endpoint_regular_1}" "${endpoint_regular_2}" ||
    fail "endpoint regular-file snapshots differ"
  cmp -s -- "${endpoint_directories_1}" "${endpoint_directories_2}" ||
    fail "endpoint directory snapshots differ"
  verify_bundle_tree_at "${bundle_root}" "${endpoint_regular_2}" \
    "${endpoint_directories_2}"
  echo "endpoint_bundle_receipt=${bundle_receipt}"
  echo "endpoint_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
  exit 0
fi

build_staging_bundle "${endpoint_regular_1}" "${endpoint_directories_1}"
verify_bundle_tree_at "${staging_root}" "${endpoint_regular_1}" \
  "${endpoint_directories_1}"
verify_endpoint_semantics
take_source_snapshots "${retry1_regular_2}" "${retry1_directories_2}" \
  "${endpoint_regular_2}" "${endpoint_directories_2}"
cmp -s -- "${retry1_regular_1}" "${retry1_regular_2}" ||
  fail "retry1 regular-file snapshots differ"
cmp -s -- "${retry1_directories_1}" "${retry1_directories_2}" ||
  fail "retry1 directory snapshots differ"
cmp -s -- "${endpoint_regular_1}" "${endpoint_regular_2}" ||
  fail "endpoint regular-file snapshots differ"
cmp -s -- "${endpoint_directories_1}" "${endpoint_directories_2}" ||
  fail "endpoint directory snapshots differ"
verify_bundle_tree_at "${staging_root}" "${endpoint_regular_2}" \
  "${endpoint_directories_2}"
publish_staging_bundle
verify_bundle_tree_at "${bundle_root}" "${endpoint_regular_2}" \
  "${endpoint_directories_2}"
assert_source_lock_identity
echo "endpoint_bundle_receipt=${bundle_receipt}"
echo "endpoint_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
