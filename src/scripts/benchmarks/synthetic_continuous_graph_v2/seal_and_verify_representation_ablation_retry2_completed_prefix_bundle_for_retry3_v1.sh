#!/usr/bin/env bash
set -euo pipefail
shopt -s inherit_errexit
umask 077

# This sealer is intentionally fail-closed until the independently produced
# Retry2 stage-04 interruption closure is published and the UNSEALED pins below
# are replaced with its exact immutable hashes.  The completed-prefix bundle is
# a copy-only recovery authority.  It never repairs or resumes Retry2.

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2_completed_prefix_bundle_for_retry3_v1"
readonly source_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"
readonly recovery_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry3"
readonly interruption_closure_schema_id="${source_schema_id}_stage04_interruption_closure_v1"

readonly expected_interruption_closure_sealer_sha256="c7ea8d0ab52a8395da19aaca5e8d9136a1f0f4ed74c62c94df9a7b94eda0e05f"
readonly expected_interruption_closure_amendment_sha256="c0310d27fea46c97ee9517362b809c6f53c8d848a3ea9023a3d7aaf1c3a347f6"
readonly expected_interruption_closure_receipt_sha256="d3b5c587f135335d97ed27a20dd6aa17d9f02e67760378524f04f437bfe87903"
readonly expected_interruption_closure_regular_inventory_sha256="943faa9ee84a7d8f9a2bc50ab2b710be6061f19c57489139de39f8aefab5ab9f"
readonly expected_interruption_closure_directory_inventory_sha256="4b08c785ea4d3be07fa03d21dca4842149eed8b06da89ecbc914528ebf3aeef1"

readonly expected_retry2_runner_sha256="91915b7d32f0c1679d69e9077bbf8eb88777e367f590b34c2832a11fdcc26768"
readonly expected_retry2_runner_inode="168884986026428276"
readonly expected_retry2_runner_device="66"
readonly expected_retry2_runner_bytes="414662"
readonly expected_retry2_frozen_runner_inode="54887620459446969"
readonly expected_retry2_runtime_inode="110901140824013045"
readonly expected_retry2_runtime_lock_inode="23643898044239335"
readonly expected_retry2_runtime_device="66"
readonly expected_proc_maps_device="00:42"

readonly expected_inputs_sha256="6cde2608e4a4cb9f26f73b6ed7e6ab6fa339048270b9a8b8b79cc7ba4c95d411"
readonly expected_config_closure_sha256="3a688c7845d2f65afd84a2c5bb1c0bb0a7c17cc8923fa6654cc101d8627681ce"
readonly expected_grammar_closure_sha256="5ae2341bdc4d2bb0b7aae6cd3c60f725621d2819aa3bd30f9c4035b657b36e8b"
readonly expected_stage00_attempt_sha256="ed75d13c10f3381023b9bd648eca4a25dc4eb1d5956ef28340049ef27d07fa69"
readonly expected_stage00_completion_sha256="3cccdeae5b2765cbcc2c7c03095562b4a3963538ce7618e439a62ad703635433"
readonly expected_stage01_attempt_sha256="629bec6dd7ec10465c1c11bf51c9710af2598bd4aa25cba3ca29b72c82c883c3"
readonly expected_stage01_completion_sha256="5f35515e76287c647e2bdd09a6b466b548c98393c8b40b3705f986def02ef741"
readonly expected_stage02_attempt_sha256="901c1d9e0501cdf23c2754fd1c18872dd05c905755aa3fa009d4d290a3356243"
readonly expected_stage02_completion_sha256="54371e6aa019d4b2af3be819c13162812a9dd13c80624c7efd7340dd166900f8"
readonly expected_stage03_attempt_sha256="ec7c471d3c4fafb959734d1ad8e7b716ab5535027951d55db0c812bb7fee6f1c"
readonly expected_stage03_completion_sha256="d0ba0e40b8489a660196e23ccb1c63bfc198dfa22a2d3b40115e48b12fd60693"
readonly expected_stage04_attempt_sha256="19a7597dbe5a94f97908de3103cfa62d4e144c7648a5c91fbe82398d6cb82ae2"

readonly expected_canonical_import_sha256="d022a71a38327139330d86fb07e8027b4b75ca823424ef79a1926d5e49242b5d"
readonly expected_endpoint_import_sha256="b5e5d2b75c8c31af3534d69b7538de5b0565ace60bd3b753e3063f11df67d2fa"
readonly expected_time_only_status_sha256="2643e01ff5788665a82da62408c98ff543421b941aee882ad1c8a692f28557b9"
readonly expected_time_only_checkpoint_sha256="f30aef1d8ea1c69ce17b2817e287355cf0d38e77076deaae4acdd560218972ac"
readonly expected_time_only_manifest_sha256="fb6ea4be431ffb18221f450b00876f3d40c98cd8a9911a907a9babc72b070dd9"
readonly expected_time_only_result_sha256="0334acc68fbde37deea3a578d4f8e08c5e2028d2c3ac070b5df5cd50bfc5bebe"
readonly expected_time_only_report_sha256="1a8323434eddd890ccabde2d85bdfe3584410f86da5f8ca185a02d15a8e8f4d1"
readonly expected_time_only_log_sha256="3ed5a767ce37af82a1d6a649f17718006b9dd5f22a5cb5e17293a736053dfe7a"
readonly expected_endpoint_checkpoint_sha256="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
readonly expected_endpoint_bundle_receipt_sha256="ff675afc779b106f628f3ea65fe3409314bf6ea29a531100e73dfa1a3cca9f96"
readonly expected_canonical_report_sha256="e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e"

readonly expected_payload_file_count=51
readonly expected_payload_directory_count=21
readonly expected_bundle_file_count=55
readonly expected_bundle_directory_count=23

readonly -a runtime_relpaths=(
  "synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config"
  "effective_grammar_closure.status"
  "config_inputs.status"
  "inputs.status"
  "frozen_sources/run_representation_ablation_v2_retry2.sh"
  "frozen_sources/frozen_representation_affine_probe.cpp"
  "frozen_sources/frozen_representation_affine_probe"
  "arms/endpoint_scale/config/capture.config"
  "arms/endpoint_scale/config/representation.jkimyei"
  "arms/endpoint_scale/config/representation.net"
  "arms/endpoint_scale/config/train.config"
  "arms/time_only/config/capture.config"
  "arms/time_only/config/representation.jkimyei"
  "arms/time_only/config/representation.net"
  "arms/time_only/config/train.config"
  "arms/no_tf_alignment/config/capture.config"
  "arms/no_tf_alignment/config/representation.jkimyei"
  "arms/no_tf_alignment/config/representation.net"
  "arms/no_tf_alignment/config/train.config"
  "arms/canonical/affine/main.report"
  "arms/canonical/affine/replay.report"
  "arms/canonical/import.status"
  "imports/retry1_endpoint_v1/channel_representation.report.mtf_jepa_mae_vicreg.pt"
  "imports/retry1_endpoint_v1/endpoint_import.status"
  "imports/retry1_endpoint_v1/source_endpoint_import_bundle.status"
  "arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c/component_spawn.ref"
  "arms/time_only/training/job/channel_representation.report"
  "arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
  "arms/time_only/training/job/job.manifest"
  "arms/time_only/training/job/job.state"
  "arms/time_only/training/job/lattice.checkpoint.fact"
  "arms/time_only/training/job/lattice.exposure.fact"
  "arms/time_only/training/job/lattice.source_analytics.fact"
  "arms/time_only/training/job/runtime.checkpoint_io.fact"
  "arms/time_only/training/job/runtime.component_training_update.fact"
  "arms/time_only/training/job/runtime.health_measurement.fact"
  "arms/time_only/training/job/runtime.job_events.probe"
  "arms/time_only/training/job/runtime.result.fact"
  "arms/time_only/training/system/component_spawn_registry.v1.lls"
  "arms/time_only/training/system/runtime_layout.v1.lls"
  "arms/time_only/training.log"
  "arms/time_only/training.status"
  "stage.00.initialize.attempt.status"
  "stage.00.initialize.status"
  "stage.01.canonical_import.attempt.status"
  "stage.01.canonical_import.status"
  "stage.02.endpoint_import.attempt.status"
  "stage.02.endpoint_import.status"
  "stage.03.time_only_training.attempt.status"
  "stage.03.time_only_training.status"
)

declare -Ar expected_runtime_hashes=(
  ["synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config"]="e9db78f15fb3cd9a0475f51ff99e900bff53dc11d9daf13d1565c2dc3a43d4e7"
  ["effective_grammar_closure.status"]="${expected_grammar_closure_sha256}"
  ["config_inputs.status"]="${expected_config_closure_sha256}"
  ["inputs.status"]="${expected_inputs_sha256}"
  ["frozen_sources/run_representation_ablation_v2_retry2.sh"]="${expected_retry2_runner_sha256}"
  ["frozen_sources/frozen_representation_affine_probe.cpp"]="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
  ["frozen_sources/frozen_representation_affine_probe"]="733841623165e1be1dbf76e82264022292b5c16825211696800fd5876cddad3f"
  ["arms/endpoint_scale/config/capture.config"]="c1aa7486c73abffc0875a800fe8b0f0e1e7479c3dd1018d172fa2f168ddbc432"
  ["arms/endpoint_scale/config/representation.jkimyei"]="c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8"
  ["arms/endpoint_scale/config/representation.net"]="42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e"
  ["arms/endpoint_scale/config/train.config"]="b014eb92a58e718b33a5815b24829815be4ee517c71d04949b1743f314496e39"
  ["arms/time_only/config/capture.config"]="85f0728f84b359cb2fce93ddfffe250822853452112ebf32d1b6fccf1277858c"
  ["arms/time_only/config/representation.jkimyei"]="c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8"
  ["arms/time_only/config/representation.net"]="b60dbbf805a6abac25488a738fcfc9136d6a6de79ac688fea265b3d3963f280e"
  ["arms/time_only/config/train.config"]="d6025cb615a711f3669bfa7a1a307e1ad398f43923c9fe3adf0c860286ecf8fa"
  ["arms/no_tf_alignment/config/capture.config"]="b8ce96ced46131cf9d487c39931b041359d2312ea98744e7ed39d42463be6427"
  ["arms/no_tf_alignment/config/representation.jkimyei"]="1503407ad50dd86a5ba855c7247e0efdb4b78c11a22c97d359a8b2e64b518d37"
  ["arms/no_tf_alignment/config/representation.net"]="df4398835b7eff3496ac8c20e7713b2d3d3a245754916c81b77271c696a08cda"
  ["arms/no_tf_alignment/config/train.config"]="4ff677252a8e9093b2e2dd65f2b8668160071727467a0865844e4bb9a601f5f0"
  ["arms/canonical/affine/main.report"]="${expected_canonical_report_sha256}"
  ["arms/canonical/affine/replay.report"]="${expected_canonical_report_sha256}"
  ["arms/canonical/import.status"]="${expected_canonical_import_sha256}"
  ["imports/retry1_endpoint_v1/channel_representation.report.mtf_jepa_mae_vicreg.pt"]="${expected_endpoint_checkpoint_sha256}"
  ["imports/retry1_endpoint_v1/endpoint_import.status"]="${expected_endpoint_import_sha256}"
  ["imports/retry1_endpoint_v1/source_endpoint_import_bundle.status"]="${expected_endpoint_bundle_receipt_sha256}"
  ["arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c/component_spawn.ref"]="a0221024a2cf125a13ee88d278618950577d46815f2a76e47a065b69dd10c167"
  ["arms/time_only/training/job/channel_representation.report"]="${expected_time_only_report_sha256}"
  ["arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"]="${expected_time_only_checkpoint_sha256}"
  ["arms/time_only/training/job/job.manifest"]="${expected_time_only_manifest_sha256}"
  ["arms/time_only/training/job/job.state"]="b7a8e04f29c67bda4aa85ead1cc190505d711001bfdaef3d1738938c5e3d350f"
  ["arms/time_only/training/job/lattice.checkpoint.fact"]="b3ce304c017231229b6e0cfc67e252f63850c0da2e328ca08118cd4015718857"
  ["arms/time_only/training/job/lattice.exposure.fact"]="8a01e2e5a5e6d6b55545b076c3fd0b233149e9c56ab60285d4bc72c14db30221"
  ["arms/time_only/training/job/lattice.source_analytics.fact"]="a1771994676c2202670907811e82f14f1564ce86981ab83dfb80edbf383661de"
  ["arms/time_only/training/job/runtime.checkpoint_io.fact"]="471c7142272e3f52518a406d703b07a93c7987be9379fb0caaf0dc0c297ab3fb"
  ["arms/time_only/training/job/runtime.component_training_update.fact"]="5ff8789457344b002d8421cf1400a4e3811b5d7b0af3e6277ca4bb4a318e03fc"
  ["arms/time_only/training/job/runtime.health_measurement.fact"]="0cc355fbc27f4225f6601faeab61c923e826c6269fc8bd5434af34265c0a53cd"
  ["arms/time_only/training/job/runtime.job_events.probe"]="350150a578b09b3c636aeb8b6d7bdf60402ec38ba4dad7e78e0f631c105e70ac"
  ["arms/time_only/training/job/runtime.result.fact"]="${expected_time_only_result_sha256}"
  ["arms/time_only/training/system/component_spawn_registry.v1.lls"]="8e11c77225d76c1ed4dec0732f8a2de0d94bdfdb83f0caeac81d5f47f39ea56f"
  ["arms/time_only/training/system/runtime_layout.v1.lls"]="734306cee9206082d09071543e85f16cd35edb0038c88a6430adaa092ce78c2b"
  ["arms/time_only/training.log"]="${expected_time_only_log_sha256}"
  ["arms/time_only/training.status"]="${expected_time_only_status_sha256}"
  ["stage.00.initialize.attempt.status"]="${expected_stage00_attempt_sha256}"
  ["stage.00.initialize.status"]="${expected_stage00_completion_sha256}"
  ["stage.01.canonical_import.attempt.status"]="${expected_stage01_attempt_sha256}"
  ["stage.01.canonical_import.status"]="${expected_stage01_completion_sha256}"
  ["stage.02.endpoint_import.attempt.status"]="${expected_stage02_attempt_sha256}"
  ["stage.02.endpoint_import.status"]="${expected_stage02_completion_sha256}"
  ["stage.03.time_only_training.attempt.status"]="${expected_stage03_attempt_sha256}"
  ["stage.03.time_only_training.status"]="${expected_stage03_completion_sha256}"
)

fail() {
  echo "retry2 completed-prefix bundle for retry3: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_resolved_sha256_pin() {
  local value="$1" label="$2"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "${label} SHA-256 pin is unresolved: ${value}"
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
    if [[ "${current}" == / ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains symbolic-link component: ${current}"
  done
}

require_file() {
  reject_symlink_components "$1"
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
  [[ "$(realpath -e -- "$1")" == "$1" ]] ||
    fail "file path is not canonical: $1"
}

require_nonempty_file() {
  require_file "$1"
  [[ -s "$1" ]] || fail "empty required file: $1"
}

require_dir() {
  reject_symlink_components "$1"
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
  [[ "$(realpath -e -- "$1")" == "$1" ]] ||
    fail "directory path is not canonical: $1"
}

path_is_absent() {
  reject_symlink_components "$1"
  [[ ! -e "$1" && ! -L "$1" ]]
}

verify_hash() {
  local path="$1" expected="$2" label="$3"
  require_nonempty_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has nonunit hard-link count: ${path}"
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
    fail "${path}: expected exactly one ${key}= field, found ${count}"
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

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
live_runtime="${runtime_parent}/${source_schema_id}"
live_runtime_lock="${live_runtime}/.development.lock"
live_retry2_runner="${script_dir}/run_representation_ablation_v2_retry2.sh"
live_retry2_frozen_runner="${live_runtime}/frozen_sources/run_representation_ablation_v2_retry2.sh"

interruption_closure_root="${runtime_parent}/${interruption_closure_schema_id}"
interruption_closure_receipt="${interruption_closure_root}/interruption_closure.status"
interruption_closure_regular_inventory="${interruption_closure_root}/source_regular_files.inventory.tsv"
interruption_closure_directory_inventory="${interruption_closure_root}/source_directories.inventory.tsv"
interruption_closure_source_snapshot="${interruption_closure_root}/source_snapshot"
interruption_closure_sealer="${script_dir}/seal_and_verify_representation_ablation_retry2_stage04_interruption_closure_v1.sh"
interruption_closure_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY2_STAGE04_INTERRUPTION_RECOVERY_AMENDMENT.md"
interruption_closure_frozen_sealer="${interruption_closure_root}/frozen_sources/$(basename "${interruption_closure_sealer}")"
interruption_closure_frozen_amendment="${interruption_closure_root}/frozen_sources/$(basename "${interruption_closure_amendment}")"

bundle_root="${runtime_parent}/${schema_id}"
staging_root="${runtime_parent}/.${schema_id}.candidate"
completed_prefix_root="${bundle_root}/completed_prefix"
bundle_receipt="${bundle_root}/completed_prefix_bundle.status"
bundle_regular_inventory="${bundle_root}/regular_files.inventory.tsv"
bundle_directory_inventory="${bundle_root}/directories.inventory.tsv"
bundle_frozen_sealer="${bundle_root}/frozen_sources/$(basename "${script_path}")"

readonly process_owner_uid="$(id -u)"
readonly process_owner_gid="$(id -g)"
readonly process_start_sealer_sha256="$(sha256_of "${script_path}")"
readonly process_start_sealer_mode="$(stat -c '%a' -- "${script_path}")"
readonly process_start_sealer_links="$(stat -c '%h' -- "${script_path}")"
readonly process_start_sealer_uid="$(stat -c '%u' -- "${script_path}")"
readonly process_start_sealer_gid="$(stat -c '%g' -- "${script_path}")"
readonly process_start_sealer_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_sealer_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_sealer_device="$(stat -c '%d' -- "${script_path}")"

declare -a temporary_files=()
candidate_created_by_process=false
candidate_device=""
candidate_inode=""
runner_lock_fd=""
runner_lock_inode=""
runner_lock_device=""
runtime_lock_fd=""
runtime_lock_inode=""
runtime_lock_device=""

cleanup() {
  local path
  for path in "${temporary_files[@]:-}"; do
    [[ -n "${path}" ]] && rm -f -- "${path}" 2>/dev/null || true
  done
  if [[ "${candidate_created_by_process}" == true && \
    -n "${candidate_device}" && -n "${candidate_inode}" && \
    -d "${staging_root}" && ! -L "${staging_root}" && \
    "$(realpath -e -- "${staging_root}" 2>/dev/null)" == "${staging_root}" && \
    "$(stat -c '%d:%i' -- "${staging_root}" 2>/dev/null)" == \
      "${candidate_device}:${candidate_inode}" ]]; then
    chmod -R u+w -- "${staging_root}" 2>/dev/null || true
    find "${staging_root}" -xdev -depth -type f -delete 2>/dev/null || true
    find "${staging_root}" -xdev -depth -type d -empty -delete \
      2>/dev/null || true
  fi
}
trap cleanup EXIT

new_temp_file() {
  local __result_var="$1" leaf="$2" created
  created="$(mktemp "${runtime_parent}/.${schema_id}.${leaf}.XXXXXX")"
  temporary_files+=("${created}")
  printf -v "${__result_var}" '%s' "${created}"
}

capture_find0() {
  local __result_var="$1" root="$2" output
  shift 2
  new_temp_file output "find.raw"
  find "${root}" -xdev "$@" -print0 >"${output}" ||
    fail "checked tree traversal failed: ${root}"
  printf -v "${__result_var}" '%s' "${output}"
}

capture_sorted_find0() {
  local __result_var="$1" root="$2" raw sorted
  shift 2
  capture_find0 raw "${root}" "$@"
  new_temp_file sorted "find.sorted"
  LC_ALL=C sort -z -- "${raw}" >"${sorted}" ||
    fail "checked tree sort failed: ${root}"
  printf -v "${__result_var}" '%s' "${sorted}"
}

capture_command_output() {
  local __result_var="$1" label="$2" output
  shift 2
  new_temp_file output "command.output"
  "$@" >"${output}" || fail "checked producer failed: ${label}"
  printf -v "${__result_var}" '%s' "${output}"
}

capture_proc_children0() {
  local __result_var="$1" proc="$2" child_dir="$3" output status
  new_temp_file output "proc.children"
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

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--audit|--seal|--verify]"
case "${mode}" in
--plan | --audit | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--audit|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
source_runtime_schema_id=${source_schema_id}
source_runtime_root=${live_runtime}
source_runtime_lock=${live_runtime_lock}
source_live_runner=${live_retry2_runner}
source_frozen_runner=${live_retry2_frozen_runner}
lock_order=retry2_live_runner_then_retry2_development_lock
lock_descriptors_opened_read_only=true
interruption_closure_schema_id=${interruption_closure_schema_id}
interruption_closure_root=${interruption_closure_root}
interruption_closure_source_snapshot=${interruption_closure_source_snapshot}
interruption_closure_receipt_pin=${expected_interruption_closure_receipt_sha256}
interruption_closure_regular_inventory_pin=${expected_interruption_closure_regular_inventory_sha256}
interruption_closure_directory_inventory_pin=${expected_interruption_closure_directory_inventory_sha256}
unsealed_pins_fail_closed=true
primary_copy_source=interruption_closure_source_snapshot
live_runtime_equals_closure_snapshot_required=true
source_snapshot_count=2
bundle_root=${bundle_root}
completed_prefix_root=${completed_prefix_root}
receipt_path=${bundle_receipt}
regular_inventory_path=${bundle_regular_inventory}
directory_inventory_path=${bundle_directory_inventory}
payload_regular_file_count=${expected_payload_file_count}
payload_directory_count=${expected_payload_directory_count}
bundle_regular_file_count=${expected_bundle_file_count}
bundle_directory_count=${expected_bundle_directory_count}
copy_method=cp_--reflink=never
hardlinks_authorized=false
publication_method=atomic_no_clobber_directory_rename
completed_stage_count=4
stages_00_through_03_reuse_authorized=true
stage_04_attempt_included=false
stage_04_reuse_authorized=false
no_tf_alignment_configuration_included=true
no_tf_alignment_partial_payload_included=false
no_tf_alignment_restart_optimizer_step=0
certified_input_access=false
final_holdout_access=false
policy_access=false
PLAN
}

assert_process_start_identity() {
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == "${process_start_sealer_sha256}" ]] ||
    fail "executing completed-prefix sealer bytes changed after process start"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${script_path}")" == \
    "${process_start_sealer_mode}:${process_start_sealer_uid}:${process_start_sealer_links}:${process_start_sealer_bytes}:${process_start_sealer_device}:${process_start_sealer_inode}" ]] ||
    fail "executing completed-prefix sealer identity changed after process start"
  [[ "${process_start_sealer_links}" == 1 ]] ||
    fail "executing completed-prefix sealer must have link count one"
}

verify_live_retry2_runner_identity() {
  verify_hash "${live_retry2_runner}" "${expected_retry2_runner_sha256}" \
    "Retry2 live runner"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${live_retry2_runner}")" == \
    "555:${process_owner_uid}:1:${expected_retry2_runner_bytes}:${expected_retry2_runner_device}:${expected_retry2_runner_inode}" ]] ||
    fail "Retry2 live runner exact identity drifted"
}

verify_live_retry2_root_and_lock_identity() {
  require_dir "${live_runtime}"
  require_file "${live_runtime_lock}"
  [[ "$(stat -c '%a:%u:%h:%d:%i' -- "${live_runtime}")" == \
    "700:${process_owner_uid}:1:${expected_retry2_runtime_device}:${expected_retry2_runtime_inode}" ]] ||
    fail "Retry2 runtime-root exact identity drifted"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- "${live_runtime_lock}")" == \
    "600:${process_owner_uid}:1:0:${expected_retry2_runtime_device}:${expected_retry2_runtime_lock_inode}" ]] ||
    fail "Retry2 development-lock exact identity drifted"
}

assert_runner_lock_identity() {
  [[ -n "${runner_lock_fd}" && -n "${runner_lock_inode}" && \
    -n "${runner_lock_device}" ]] || fail "Retry2 runner lock is uninitialized"
  [[ -e "/proc/self/fd/${runner_lock_fd}" ]] ||
    fail "Retry2 runner lock descriptor is closed"
  verify_live_retry2_runner_identity
  [[ "$(stat -L -c '%d:%i' -- "/proc/self/fd/${runner_lock_fd}")" == \
    "${runner_lock_device}:${runner_lock_inode}" ]] ||
    fail "Retry2 runner lock descriptor identity drifted"
  [[ "$(stat -c '%d:%i' -- "${live_retry2_runner}")" == \
    "${runner_lock_device}:${runner_lock_inode}" ]] ||
    fail "Retry2 runner path no longer names the locked inode"
}

assert_runtime_lock_identity() {
  assert_runner_lock_identity
  [[ -n "${runtime_lock_fd}" && -n "${runtime_lock_inode}" && \
    -n "${runtime_lock_device}" ]] || fail "Retry2 runtime lock is uninitialized"
  [[ -e "/proc/self/fd/${runtime_lock_fd}" ]] ||
    fail "Retry2 runtime lock descriptor is closed"
  verify_live_retry2_root_and_lock_identity
  [[ "$(stat -L -c '%d:%i' -- "/proc/self/fd/${runtime_lock_fd}")" == \
    "${runtime_lock_device}:${runtime_lock_inode}" ]] ||
    fail "Retry2 runtime lock descriptor identity drifted"
  [[ "$(stat -c '%d:%i' -- "${live_runtime_lock}")" == \
    "${runtime_lock_device}:${runtime_lock_inode}" ]] ||
    fail "Retry2 runtime lock path no longer names the locked inode"
}

acquire_locks_in_order() {
  verify_live_retry2_runner_identity
  exec {runner_lock_fd}<"${live_retry2_runner}"
  flock -n "${runner_lock_fd}" ||
    fail "Retry2 live runner is locked by another process"
  runner_lock_device="$(stat -L -c '%d' -- "/proc/self/fd/${runner_lock_fd}")"
  runner_lock_inode="$(stat -L -c '%i' -- "/proc/self/fd/${runner_lock_fd}")"
  assert_runner_lock_identity

  verify_live_retry2_root_and_lock_identity
  exec {runtime_lock_fd}<"${live_runtime_lock}"
  flock -n "${runtime_lock_fd}" ||
    fail "Retry2 development lock is held by another process"
  runtime_lock_device="$(stat -L -c '%d' -- "/proc/self/fd/${runtime_lock_fd}")"
  runtime_lock_inode="$(stat -L -c '%i' -- "/proc/self/fd/${runtime_lock_fd}")"
  assert_runtime_lock_identity
}

verify_tree_safety() {
  local root="$1" label="$2" root_device entry entry_device entries
  require_dir "${root}"
  root_device="$(stat -c '%d' -- "${root}")"
  capture_find0 entries "${root}" -mindepth 0
  while IFS= read -r -d '' entry; do
    [[ ! -L "${entry}" ]] || fail "${label} contains a symlink: ${entry}"
    entry_device="$(stat -c '%d' -- "${entry}")"
    [[ "${entry_device}" == "${root_device}" ]] ||
      fail "${label} contains a cross-device entry: ${entry}"
    if [[ -f "${entry}" ]]; then
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "${label} contains a nonunit-linked file: ${entry}"
    elif [[ ! -d "${entry}" ]]; then
      fail "${label} contains a special entry: ${entry}"
    fi
  done <"${entries}"
}

assert_tree_has_no_live_references() {
  local root="$1" label="$2" protected_identities protected_inodes entry
  require_dir "${root}"
  [[ "$(stat -c '%d' -- "${root}")" == \
    "${expected_retry2_runtime_device}" ]] ||
    fail "${label} is not on the pinned 00:42 publication mount"
  new_temp_file protected_identities "publication.identities"
  new_temp_file protected_inodes "publication.inodes"
  find "${root}" -xdev -mindepth 0 -printf '%D:%i\n' \
    >"${protected_identities}" ||
    fail "could not inventory ${label} identities"
  LC_ALL=C sort -u -o "${protected_identities}" \
    "${protected_identities}" || fail "could not sort ${label} identities"
  find "${root}" -xdev -mindepth 0 -printf '%i\n' \
    >"${protected_inodes}" || fail "could not inventory ${label} mapping inodes"
  LC_ALL=C sort -u -o "${protected_inodes}" "${protected_inodes}" ||
    fail "could not sort ${label} mapping inodes"

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
      "${protected_inodes}" "${label}"
  done
}

relative_path_of() {
  local root="$1" path="$2"
  if [[ "${path}" == "${root}" ]]; then
    printf '.'
  else
    printf '%s' "${path#${root}/}"
  fi
}

emit_normalized_tree_content() {
  local root="$1" path rel entries
  verify_tree_safety "${root}" "normalized tree"
  capture_sorted_find0 entries "${root}" -mindepth 0
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${root}" "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && \
      "${rel}" != *'|'* ]] || fail "non-canonical tree path: ${rel}"
    if [[ -d "${path}" ]]; then
      printf 'd\t%s\n' "${rel}"
    else
      printf 'f\t%s\t%s\t%s\n' "${rel}" \
        "$(stat -c '%s' -- "${path}")" "$(sha256_of "${path}")"
    fi
  done <"${entries}"
}

verify_closure_authority() {
  require_resolved_sha256_pin "${expected_interruption_closure_sealer_sha256}" \
    "interruption closure sealer"
  require_resolved_sha256_pin "${expected_interruption_closure_amendment_sha256}" \
    "interruption closure amendment"
  require_resolved_sha256_pin "${expected_interruption_closure_receipt_sha256}" \
    "interruption closure receipt"
  require_resolved_sha256_pin \
    "${expected_interruption_closure_regular_inventory_sha256}" \
    "interruption closure regular inventory"
  require_resolved_sha256_pin \
    "${expected_interruption_closure_directory_inventory_sha256}" \
    "interruption closure directory inventory"

  verify_hash "${interruption_closure_sealer}" \
    "${expected_interruption_closure_sealer_sha256}" \
    "live interruption closure sealer"
  verify_hash "${interruption_closure_frozen_sealer}" \
    "${expected_interruption_closure_sealer_sha256}" \
    "frozen interruption closure sealer"
  verify_hash "${interruption_closure_amendment}" \
    "${expected_interruption_closure_amendment_sha256}" \
    "live interruption closure amendment"
  verify_hash "${interruption_closure_frozen_amendment}" \
    "${expected_interruption_closure_amendment_sha256}" \
    "frozen interruption closure amendment"
  verify_hash "${interruption_closure_receipt}" \
    "${expected_interruption_closure_receipt_sha256}" \
    "interruption closure receipt"
  verify_hash "${interruption_closure_regular_inventory}" \
    "${expected_interruption_closure_regular_inventory_sha256}" \
    "interruption closure regular inventory"
  verify_hash "${interruption_closure_directory_inventory}" \
    "${expected_interruption_closure_directory_inventory_sha256}" \
    "interruption closure directory inventory"

  [[ "$(stat -c '%a:%u:%h' -- "${interruption_closure_sealer}")" == \
    "555:${process_owner_uid}:1" ]] ||
    fail "live interruption closure sealer is not sealed 0555"
  local immutable
  for immutable in "${interruption_closure_frozen_sealer}" \
    "${interruption_closure_amendment}" \
    "${interruption_closure_frozen_amendment}" \
    "${interruption_closure_receipt}" \
    "${interruption_closure_regular_inventory}" \
    "${interruption_closure_directory_inventory}"; do
    [[ "$(stat -c '%a:%u:%h' -- "${immutable}")" == \
      "444:${process_owner_uid}:1" ]] ||
      fail "interruption closure authority is not sealed 0444: ${immutable}"
  done
  require_dir "${interruption_closure_source_snapshot}"
  [[ "$(stat -c '%a:%u:%h' -- "${interruption_closure_source_snapshot}")" == \
    "555:${process_owner_uid}:1" ]] ||
    fail "interruption closure source snapshot is not sealed 0555"
  expect_kv "${interruption_closure_receipt}" schema_id \
    "${interruption_closure_schema_id}"
  expect_kv "${interruption_closure_receipt}" status complete
  expect_kv "${interruption_closure_receipt}" \
    source_regular_file_inventory_path \
    "${interruption_closure_regular_inventory}"
  expect_kv "${interruption_closure_receipt}" \
    source_regular_file_inventory_sha256 \
    "${expected_interruption_closure_regular_inventory_sha256}"
  expect_kv "${interruption_closure_receipt}" \
    source_directory_inventory_path \
    "${interruption_closure_directory_inventory}"
  expect_kv "${interruption_closure_receipt}" \
    source_directory_inventory_sha256 \
    "${expected_interruption_closure_directory_inventory_sha256}"
  expect_kv "${interruption_closure_receipt}" source_snapshot_path \
    "${interruption_closure_source_snapshot}"
  expect_kv "${interruption_closure_receipt}" source_snapshot_copy_method \
    cp_archive_reflink_never
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_regular_file_count 60
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_directory_count 28
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_regular_file_bytes 58518408
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_regular_files_byte_identical true
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_regular_files_distinct_inodes true
  expect_kv "${interruption_closure_receipt}" \
    source_snapshot_regular_files_single_link true
  grep -Fq -- "${live_runtime}" "${interruption_closure_receipt}" ||
    fail "interruption closure receipt does not bind the live Retry2 runtime"
  grep -Fq -- "${interruption_closure_source_snapshot}" \
    "${interruption_closure_receipt}" ||
    fail "interruption closure receipt does not bind its source snapshot"
  grep -Fq -- "${interruption_closure_regular_inventory}" \
    "${interruption_closure_receipt}" ||
    fail "interruption closure receipt does not bind its regular inventory"
  grep -Fq -- "${interruption_closure_directory_inventory}" \
    "${interruption_closure_receipt}" ||
    fail "interruption closure receipt does not bind its directory inventory"
  verify_tree_safety "${interruption_closure_source_snapshot}" \
    "interruption closure source snapshot"
}

verify_allowlist_shape() {
  [[ "${#runtime_relpaths[@]}" == 50 ]] ||
    fail "Retry2 runtime allowlist file count drifted"
  [[ "${#expected_runtime_hashes[@]}" == 50 ]] ||
    fail "Retry2 runtime expected-hash map count drifted"
  local rel
  local -A seen=()
  for rel in "${runtime_relpaths[@]}"; do
    [[ "${rel}" != /* && "${rel}" != . && "${rel}" != ../* && \
      "${rel}" != */../* && "${rel}" != */.. ]] ||
      fail "non-canonical runtime allowlist path: ${rel}"
    [[ -z "${seen[${rel}]+present}" ]] ||
      fail "duplicate runtime allowlist path: ${rel}"
    seen["${rel}"]=1
    [[ -n "${expected_runtime_hashes[${rel}]+present}" ]] ||
      fail "runtime allowlist path lacks an expected hash: ${rel}"
    [[ "${rel}" != stage.04.* && \
      "${rel}" != arms/no_tf_alignment/training && \
      "${rel}" != arms/no_tf_alignment/training/* && \
      "${rel}" != arms/no_tf_alignment/training.log && \
      "${rel}" != arms/no_tf_alignment/training.status ]] ||
      fail "forbidden stage-04/no-TF partial path entered allowlist: ${rel}"
  done
  for rel in "${!expected_runtime_hashes[@]}"; do
    [[ -n "${seen[${rel}]+present}" ]] ||
      fail "expected-hash map contains a non-allowlisted path: ${rel}"
  done
}

entry_count() {
  printf '%s' "$((1 + ${#runtime_relpaths[@]}))"
}

entry_source_relative_path() {
  local index="$1"
  if ((index == 0)); then
    printf '@operational_retry2_runner'
  else
    printf '%s' "${runtime_relpaths[$((index - 1))]}"
  fi
}

entry_source_path() {
  local index="$1"
  if ((index == 0)); then
    printf '%s' "${live_retry2_runner}"
  else
    printf '%s/%s' "${interruption_closure_source_snapshot}" \
      "${runtime_relpaths[$((index - 1))]}"
  fi
}

entry_live_runtime_path() {
  local index="$1"
  ((index > 0)) || fail "operational runner has no runtime-relative path"
  printf '%s/%s' "${live_runtime}" "${runtime_relpaths[$((index - 1))]}"
}

entry_destination_relative_path() {
  local index="$1"
  if ((index == 0)); then
    printf 'operational_authority/run_representation_ablation_v2_retry2.sh'
  else
    printf '%s' "${runtime_relpaths[$((index - 1))]}"
  fi
}

entry_expected_sha256() {
  local index="$1"
  if ((index == 0)); then
    printf '%s' "${expected_retry2_runner_sha256}"
  else
    printf '%s' \
      "${expected_runtime_hashes[${runtime_relpaths[$((index - 1))]}]}"
  fi
}

emit_expected_completed_prefix_directories() {
  local count index rel dir
  count="$(entry_count)"
  printf '.\n'
  for ((index = 0; index < count; ++index)); do
    rel="$(entry_destination_relative_path "${index}")"
    dir="$(dirname -- "${rel}")"
    while [[ "${dir}" != . ]]; do
      printf '%s\n' "${dir}"
      dir="$(dirname -- "${dir}")"
    done
  done | LC_ALL=C sort -u
}

emit_expected_source_runtime_directories() {
  local rel dir
  printf '.\n'
  for rel in "${runtime_relpaths[@]}"; do
    dir="$(dirname -- "${rel}")"
    while [[ "${dir}" != . ]]; do
      printf '%s\n' "${dir}"
      dir="$(dirname -- "${dir}")"
    done
  done | LC_ALL=C sort -u
}

verify_source_files() {
  assert_runtime_lock_identity
  verify_closure_authority
  verify_allowlist_shape
  local count index source live expected rel
  count="$(entry_count)"
  [[ "${count}" == "${expected_payload_file_count}" ]] ||
    fail "completed-prefix payload file count drifted"
  for ((index = 0; index < count; ++index)); do
    source="$(entry_source_path "${index}")"
    expected="$(entry_expected_sha256 "${index}")"
    verify_hash "${source}" "${expected}" "completed-prefix source entry ${index}"
    [[ "$(stat -c '%u:%g:%h' -- "${source}")" == \
      "${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "completed-prefix source ownership/link drifted: ${source}"
    if ((index == 0)); then
      [[ "$(stat -c '%a:%s:%d:%i' -- "${source}")" == \
        "555:${expected_retry2_runner_bytes}:${expected_retry2_runner_device}:${expected_retry2_runner_inode}" ]] ||
        fail "operational Retry2 runner identity drifted in source verification"
    else
      [[ "$(stat -c '%a' -- "${source}")" == 444 ]] ||
        fail "closure snapshot source file is not mode 0444: ${source}"
      live="$(entry_live_runtime_path "${index}")"
      rel="$(entry_source_relative_path "${index}")"
      verify_hash "${live}" "${expected}" "live Retry2 allowlist entry ${rel}"
      cmp -s -- "${live}" "${source}" ||
        fail "live Retry2 and closure snapshot bytes differ: ${rel}"
    fi
  done
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- \
    "${live_retry2_frozen_runner}")" == \
    "444:${process_owner_uid}:1:${expected_retry2_runner_bytes}:${expected_retry2_runner_device}:${expected_retry2_frozen_runner_inode}" ]] ||
    fail "live Retry2 frozen runner exact identity drifted"
  cmp -s -- "${live_retry2_runner}" "${live_retry2_frozen_runner}" ||
    fail "live and runtime-frozen Retry2 runners differ"
  cmp -s -- "${live_retry2_runner}" \
    "${interruption_closure_source_snapshot}/frozen_sources/run_representation_ablation_v2_retry2.sh" ||
    fail "live and closure-frozen Retry2 runners differ"
  assert_runtime_lock_identity
}

emit_source_file_snapshot() {
  local count index source rel destination expected stat_fields
  local mode_value uid_value gid_value links_value bytes_value inode_value
  local device_value
  printf 'entry_index\tsource_relative_path\tsource_path\tdestination_relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tsha256\n'
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    source="$(entry_source_path "${index}")"
    rel="$(entry_source_relative_path "${index}")"
    destination="$(entry_destination_relative_path "${index}")"
    expected="$(entry_expected_sha256 "${index}")"
    verify_hash "${source}" "${expected}" "source snapshot entry ${index}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${source}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%02d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${index}" "${rel}" "${source}" "${destination}" "${mode_value}" \
      "${uid_value}" "${gid_value}" "${links_value}" "${bytes_value}" \
      "${inode_value}" "${device_value}" "${expected}"
  done
}

emit_source_directory_snapshot() {
  local rel path stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value expected_directories
  printf 'relative_path\tpath\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\n'
  capture_command_output expected_directories \
    "expected source-runtime directories" \
    emit_expected_source_runtime_directories
  while IFS= read -r rel; do
    path="${interruption_closure_source_snapshot}"
    [[ "${rel}" == . ]] || path="${path}/${rel}"
    require_dir "${path}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${rel}" \
      "${path}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}"
  done <"${expected_directories}"
}

take_source_snapshot() {
  local file_inventory="$1" directory_inventory="$2"
  local live_content="$3" closure_content="$4"
  assert_runtime_lock_identity
  verify_closure_authority
  emit_normalized_tree_content "${live_runtime}" >"${live_content}"
  emit_normalized_tree_content "${interruption_closure_source_snapshot}" \
    >"${closure_content}"
  cmp -s -- "${live_content}" "${closure_content}" ||
    fail "live Retry2 tree differs from the immutable closure source snapshot"
  verify_source_files
  emit_source_file_snapshot >"${file_inventory}"
  emit_source_directory_snapshot >"${directory_inventory}"
  assert_runtime_lock_identity
}

closure_file() {
  printf '%s/%s' "${interruption_closure_source_snapshot}" "$1"
}

live_file() {
  printf '%s/%s' "${live_runtime}" "$1"
}

verify_historical_runner_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" operational_ablation_runner_path \
    "${live_retry2_runner}"
  expect_kv "${receipt}" operational_ablation_runner_sha256 \
    "${expected_retry2_runner_sha256}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_sha256 \
    "${expected_retry2_runner_sha256}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_inode \
    "${expected_retry2_runner_inode}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_device \
    "${expected_retry2_runner_device}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_bytes \
    "${expected_retry2_runner_bytes}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" operational_ablation_runner_mode 0555
  expect_kv "${receipt}" operational_ablation_runner_links 1
}

verify_stage_attempt() {
  local ordinal="$1" name="$2" rel="$3" expected_hash="$4"
  local previous_rel="$5" previous_hash="$6" receipt
  receipt="$(closure_file "${rel}")"
  verify_hash "${receipt}" "${expected_hash}" "Retry2 stage-${ordinal} attempt"
  expect_kv "${receipt}" schema_id \
    "${source_schema_id}.development_stage_attempt.v1"
  expect_kv "${receipt}" status consumed
  expect_kv "${receipt}" stage_ordinal "${ordinal}"
  expect_kv "${receipt}" stage_name "${name}"
  if [[ "${previous_rel}" == none ]]; then
    expect_kv "${receipt}" previous_stage_completion_path none
    expect_kv "${receipt}" previous_stage_completion_sha256 none
  else
    expect_kv "${receipt}" previous_stage_completion_path \
      "$(live_file "${previous_rel}")"
    expect_kv "${receipt}" previous_stage_completion_sha256 \
      "${previous_hash}"
  fi
  verify_historical_runner_bindings "${receipt}"
  expect_kv "${receipt}" runtime_root_path "${live_runtime}"
  expect_kv "${receipt}" runtime_root_inode "${expected_retry2_runtime_inode}"
  expect_kv "${receipt}" runtime_root_device "${expected_retry2_runtime_device}"
  expect_kv "${receipt}" development_lock_path "${live_runtime_lock}"
  expect_kv "${receipt}" development_lock_inode \
    "${expected_retry2_runtime_lock_inode}"
  expect_kv "${receipt}" development_lock_device \
    "${expected_retry2_runtime_device}"
  expect_kv "${receipt}" attempt_without_completion_terminal true
  expect_kv "${receipt}" partial_payload_adoption_authorized false
  expect_kv "${receipt}" checkpoint_resume_authorized false
  expect_kv "${receipt}" certified_input_access false
  expect_kv "${receipt}" final_holdout_access false
  expect_kv "${receipt}" policy_access false
}

verify_stage_completion() {
  local ordinal="$1" name="$2" rel="$3" expected_hash="$4"
  local previous_rel="$5" previous_hash="$6" attempt_rel="$7"
  local attempt_hash="$8" primary_kind="$9" primary_rel="${10}"
  local primary_hash="${11}" receipt
  receipt="$(closure_file "${rel}")"
  verify_hash "${receipt}" "${expected_hash}" \
    "Retry2 stage-${ordinal} completion"
  expect_kv "${receipt}" schema_id \
    "${source_schema_id}.development_stage_completion.v1"
  expect_kv "${receipt}" status complete
  expect_kv "${receipt}" stage_ordinal "${ordinal}"
  expect_kv "${receipt}" stage_name "${name}"
  if [[ "${previous_rel}" == none ]]; then
    expect_kv "${receipt}" previous_stage_completion_path none
    expect_kv "${receipt}" previous_stage_completion_sha256 none
  else
    expect_kv "${receipt}" previous_stage_completion_path \
      "$(live_file "${previous_rel}")"
    expect_kv "${receipt}" previous_stage_completion_sha256 \
      "${previous_hash}"
  fi
  expect_kv "${receipt}" stage_attempt_path "$(live_file "${attempt_rel}")"
  expect_kv "${receipt}" stage_attempt_sha256 "${attempt_hash}"
  expect_kv "${receipt}" primary_artifact_kind "${primary_kind}"
  expect_kv "${receipt}" primary_artifact_path "$(live_file "${primary_rel}")"
  expect_kv "${receipt}" primary_artifact_sha256 "${primary_hash}"
  verify_historical_runner_bindings "${receipt}"
  expect_kv "${receipt}" one_fixed_stage_this_invocation true
  expect_kv "${receipt}" pre_completion_semantic_verification_pass true
  expect_kv "${receipt}" post_stage_resource_gate_pass true
  expect_kv "${receipt}" certified_input_access false
  expect_kv "${receipt}" final_holdout_access false
  expect_kv "${receipt}" policy_access false
}

verify_completed_stage_chain() {
  verify_stage_attempt 00 initialize \
    stage.00.initialize.attempt.status "${expected_stage00_attempt_sha256}" \
    none none
  verify_stage_completion 00 initialize stage.00.initialize.status \
    "${expected_stage00_completion_sha256}" none none \
    stage.00.initialize.attempt.status "${expected_stage00_attempt_sha256}" \
    initialization_inputs inputs.status "${expected_inputs_sha256}"

  verify_stage_attempt 01 canonical_import \
    stage.01.canonical_import.attempt.status \
    "${expected_stage01_attempt_sha256}" stage.00.initialize.status \
    "${expected_stage00_completion_sha256}"
  verify_stage_completion 01 canonical_import \
    stage.01.canonical_import.status "${expected_stage01_completion_sha256}" \
    stage.00.initialize.status "${expected_stage00_completion_sha256}" \
    stage.01.canonical_import.attempt.status \
    "${expected_stage01_attempt_sha256}" canonical_import \
    arms/canonical/import.status "${expected_canonical_import_sha256}"

  verify_stage_attempt 02 endpoint_import \
    stage.02.endpoint_import.attempt.status \
    "${expected_stage02_attempt_sha256}" stage.01.canonical_import.status \
    "${expected_stage01_completion_sha256}"
  verify_stage_completion 02 endpoint_import \
    stage.02.endpoint_import.status "${expected_stage02_completion_sha256}" \
    stage.01.canonical_import.status "${expected_stage01_completion_sha256}" \
    stage.02.endpoint_import.attempt.status \
    "${expected_stage02_attempt_sha256}" retry1_endpoint_import \
    imports/retry1_endpoint_v1/endpoint_import.status \
    "${expected_endpoint_import_sha256}"

  verify_stage_attempt 03 time_only_training \
    stage.03.time_only_training.attempt.status \
    "${expected_stage03_attempt_sha256}" stage.02.endpoint_import.status \
    "${expected_stage02_completion_sha256}"
  verify_stage_completion 03 time_only_training \
    stage.03.time_only_training.status "${expected_stage03_completion_sha256}" \
    stage.02.endpoint_import.status "${expected_stage02_completion_sha256}" \
    stage.03.time_only_training.attempt.status \
    "${expected_stage03_attempt_sha256}" fresh_retry2_training \
    arms/time_only/training.status "${expected_time_only_status_sha256}"
}

verify_initialization_authority() {
  local inputs config grammar frozen
  inputs="$(closure_file inputs.status)"
  config="$(closure_file config_inputs.status)"
  grammar="$(closure_file effective_grammar_closure.status)"
  frozen="$(closure_file frozen_sources/run_representation_ablation_v2_retry2.sh)"
  verify_hash "${inputs}" "${expected_inputs_sha256}" "Retry2 inputs receipt"
  verify_hash "${config}" "${expected_config_closure_sha256}" \
    "Retry2 configuration closure"
  verify_hash "${grammar}" "${expected_grammar_closure_sha256}" \
    "Retry2 effective-grammar closure"
  verify_hash "${frozen}" "${expected_retry2_runner_sha256}" \
    "Retry2 closure-frozen runner"
  expect_kv "${inputs}" schema_id "${source_schema_id}.inputs.v1"
  expect_kv "${inputs}" status complete
  verify_historical_runner_bindings "${inputs}"
  expect_kv "${inputs}" runner_path "${live_retry2_runner}"
  expect_kv "${inputs}" runner_sha256 "${expected_retry2_runner_sha256}"
  expect_kv "${inputs}" frozen_runner_path "${live_retry2_frozen_runner}"
  expect_kv "${inputs}" frozen_runner_sha256 "${expected_retry2_runner_sha256}"
  expect_kv "${inputs}" config_closure_path "$(live_file config_inputs.status)"
  expect_kv "${inputs}" config_closure_sha256 \
    "${expected_config_closure_sha256}"
  expect_kv "${inputs}" effective_grammar_closure_path \
    "$(live_file effective_grammar_closure.status)"
  expect_kv "${inputs}" effective_grammar_closure_sha256 \
    "${expected_grammar_closure_sha256}"
  expect_kv "${inputs}" challenger_count 3
  expect_kv "${inputs}" challenger_seed 17
  expect_kv "${inputs}" challenger_optimizer_steps 3000
  expect_kv "${inputs}" train_anchor_range '[0,2496)'
  expect_kv "${inputs}" validation_anchor_range '[2560,2816)'
  expect_kv "${inputs}" certified_input_access false
  expect_kv "${inputs}" final_holdout_access false
  expect_kv "${inputs}" policy_access false
  expect_kv "${config}" schema_id "${source_schema_id}.config_inputs.v1"
  expect_kv "${config}" status complete
  expect_kv "${grammar}" schema_id \
    "${source_schema_id}.effective_grammar_closure.v1"
  expect_kv "${grammar}" status complete
  expect_kv "${grammar}" config_count 6
  expect_kv "${grammar}" effective_grammar_tuple_count 84
}

verify_canonical_authority() {
  local status main replay
  status="$(closure_file arms/canonical/import.status)"
  main="$(closure_file arms/canonical/affine/main.report)"
  replay="$(closure_file arms/canonical/affine/replay.report)"
  verify_hash "${status}" "${expected_canonical_import_sha256}" \
    "Retry2 canonical import receipt"
  verify_hash "${main}" "${expected_canonical_report_sha256}" \
    "Retry2 canonical main report"
  verify_hash "${replay}" "${expected_canonical_report_sha256}" \
    "Retry2 canonical replay report"
  cmp -s -- "${main}" "${replay}" ||
    fail "Retry2 canonical main/replay reports differ"
  expect_kv "${status}" schema_id "${source_schema_id}.canonical_import.v1"
  expect_kv "${status}" status complete
  expect_kv "${status}" arm canonical
  expect_kv "${status}" runner_path "${live_retry2_frozen_runner}"
  expect_kv "${status}" runner_sha256 "${expected_retry2_runner_sha256}"
  expect_kv "${status}" retry_attempt_sentinel_sha256 \
    "${expected_stage00_completion_sha256}"
  expect_kv "${status}" imported_main_report_sha256 \
    "${expected_canonical_report_sha256}"
  expect_kv "${status}" imported_replay_report_sha256 \
    "${expected_canonical_report_sha256}"
  expect_kv "${status}" maximum_anchor_read 2815
  expect_kv "${status}" certified_input_access false
  expect_kv "${status}" final_holdout_access false
  expect_kv "${status}" policy_access false
}

verify_endpoint_authority() {
  local status checkpoint local_bundle
  status="$(closure_file imports/retry1_endpoint_v1/endpoint_import.status)"
  checkpoint="$(closure_file imports/retry1_endpoint_v1/channel_representation.report.mtf_jepa_mae_vicreg.pt)"
  local_bundle="$(closure_file imports/retry1_endpoint_v1/source_endpoint_import_bundle.status)"
  verify_hash "${status}" "${expected_endpoint_import_sha256}" \
    "Retry2 endpoint import receipt"
  verify_hash "${checkpoint}" "${expected_endpoint_checkpoint_sha256}" \
    "Retry2 endpoint imported checkpoint"
  verify_hash "${local_bundle}" "${expected_endpoint_bundle_receipt_sha256}" \
    "Retry2 local endpoint bundle receipt"
  expect_kv "${status}" schema_id \
    "${source_schema_id}.retry1_endpoint_import.v1"
  expect_kv "${status}" status complete
  expect_kv "${status}" arm endpoint_scale
  expect_kv "${status}" local_source_bundle_receipt_sha256 \
    "${expected_endpoint_bundle_receipt_sha256}"
  expect_kv "${status}" imported_checkpoint_sha256 \
    "${expected_endpoint_checkpoint_sha256}"
  expect_kv "${status}" copy_command cp_--reflink=never
  expect_kv "${status}" exact_second_copy_verified true
  expect_kv "${status}" byte_identical_copy_verified true
  expect_kv "${status}" hardlink_authorized false
  expect_kv "${status}" historical_source_optimizer_steps 3000
  expect_kv "${status}" retry2_import_optimizer_steps 0
  expect_kv "${status}" retry2_checkpoint_resume false
  expect_kv "${status}" certified_input_access false
  expect_kv "${status}" final_holdout_access false
  expect_kv "${status}" policy_access false
}

verify_time_only_authority() {
  local status manifest result report checkpoint log
  status="$(closure_file arms/time_only/training.status)"
  manifest="$(closure_file arms/time_only/training/job/job.manifest)"
  result="$(closure_file arms/time_only/training/job/runtime.result.fact)"
  report="$(closure_file arms/time_only/training/job/channel_representation.report)"
  checkpoint="$(closure_file arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt)"
  log="$(closure_file arms/time_only/training.log)"
  verify_hash "${status}" "${expected_time_only_status_sha256}" \
    "Retry2 time-only training receipt"
  verify_hash "${manifest}" "${expected_time_only_manifest_sha256}" \
    "Retry2 time-only manifest"
  verify_hash "${result}" "${expected_time_only_result_sha256}" \
    "Retry2 time-only Runtime result"
  verify_hash "${report}" "${expected_time_only_report_sha256}" \
    "Retry2 time-only representation report"
  verify_hash "${checkpoint}" "${expected_time_only_checkpoint_sha256}" \
    "Retry2 time-only checkpoint"
  verify_hash "${log}" "${expected_time_only_log_sha256}" \
    "Retry2 time-only training log"
  expect_kv "${status}" schema_id "${source_schema_id}.training.v1"
  expect_kv "${status}" status complete
  expect_kv "${status}" arm time_only
  expect_kv "${status}" runner_path "${live_retry2_frozen_runner}"
  expect_kv "${status}" runner_sha256 "${expected_retry2_runner_sha256}"
  expect_kv "${status}" input_receipt_sha256 "${expected_inputs_sha256}"
  expect_kv "${status}" retry_attempt_sentinel_sha256 \
    "${expected_stage00_completion_sha256}"
  expect_kv "${status}" checkpoint_sha256 \
    "${expected_time_only_checkpoint_sha256}"
  expect_kv "${status}" job_manifest_sha256 \
    "${expected_time_only_manifest_sha256}"
  expect_kv "${status}" runtime_result_sha256 \
    "${expected_time_only_result_sha256}"
  expect_kv "${status}" representation_report_sha256 \
    "${expected_time_only_report_sha256}"
  expect_kv "${status}" train_anchor_range '[0,2496)'
  expect_kv "${status}" optimizer_steps 3000
  expect_kv "${status}" seed 17
  expect_kv "${status}" forecast_labels_used false
  expect_kv "${status}" final_holdout_access false
  expect_kv "${status}" policy_access false

  expect_kv "${manifest}" job_id \
    train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin 0
  expect_kv "${manifest}" resolved_anchor_index_end 2496
  expect_kv "${manifest}" input_representation_checkpoint_path ''
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  expect_kv "${report}" training_id \
    synthetic_continuous_graph_v2_mtf_jepa_mae_vicreg_train_core_v1
  expect_kv "${report}" augmentation_profile light_phase_safe_v2
  expect_kv "${report}" use_frequency_tokens false
  expect_kv "${report}" lambda_tf_align 0.1
  expect_kv "${report}" seed 17
  expect_kv "${report}" requested_anchor_index_begin 0
  expect_kv "${report}" requested_anchor_index_end 2496
  expect_kv "${report}" steps_completed 3000
  expect_kv "${report}" optimizer_steps 3000
  expect_kv "${report}" finite_parameter_check true
  expect_kv "${report}" nonfinite_output_count 0
  expect_kv "${report}" checkpoint_written true
  expect_kv "${report}" checkpoint_path \
    "$(live_file arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt)"
  expect_kv "${result}" status completed
  expect_kv "${result}" optimizer_steps 3000
  expect_kv "${result}" checkpoint_written true
  expect_kv "${result}" model_state_mutated true
  expect_kv "${result}" finite_parameter_check true
  expect_kv "${result}" nonfinite_output_count 0
  grep -Fq 'status=completed' "${log}" ||
    fail "Retry2 time-only training log lacks completion evidence"
}

verify_stage04_exclusion_boundary() {
  local attempt later_markers
  attempt="${interruption_closure_source_snapshot}/stage.04.no_tf_alignment_training.attempt.status"
  verify_hash "${attempt}" "${expected_stage04_attempt_sha256}" \
    "excluded Retry2 stage-04 attempt"
  expect_kv "${attempt}" schema_id \
    "${source_schema_id}.development_stage_attempt.v1"
  expect_kv "${attempt}" status consumed
  expect_kv "${attempt}" stage_ordinal 04
  expect_kv "${attempt}" stage_name no_tf_alignment_training
  expect_kv "${attempt}" previous_stage_completion_sha256 \
    "${expected_stage03_completion_sha256}"
  verify_historical_runner_bindings "${attempt}"
  expect_kv "${attempt}" attempt_without_completion_terminal true
  expect_kv "${attempt}" partial_payload_adoption_authorized false
  expect_kv "${attempt}" checkpoint_resume_authorized false
  expect_kv "${attempt}" certified_input_access false
  expect_kv "${attempt}" final_holdout_access false
  expect_kv "${attempt}" policy_access false

  path_is_absent \
    "${live_runtime}/stage.04.no_tf_alignment_training.status" ||
    fail "Retry2 stage-04 completion unexpectedly exists"
  path_is_absent \
    "${interruption_closure_source_snapshot}/stage.04.no_tf_alignment_training.status" ||
    fail "closure snapshot contains a stage-04 completion"
  path_is_absent "${live_runtime}/arms/no_tf_alignment/training.status" ||
    fail "Retry2 no-TF partial unexpectedly has a training status"
  path_is_absent \
    "${live_runtime}/arms/no_tf_alignment/training/job/runtime.result.fact" ||
    fail "Retry2 no-TF partial unexpectedly has a Runtime result"
  new_temp_file later_markers "later-stage.markers"
  local later
  for later in 05 06 07 08 09 10 11; do
    find "${live_runtime}" -maxdepth 1 -type f \
      -name "stage.${later}.*" -print -quit >"${later_markers}" ||
      fail "could not scan Retry2 stage-${later} markers"
    if [[ -s "${later_markers}" ]]; then
      fail "Retry2 contains a stage-${later} marker after terminal stage-04"
    fi
  done
}

verify_completed_prefix_semantics() {
  assert_runtime_lock_identity
  verify_initialization_authority
  verify_completed_stage_chain
  verify_canonical_authority
  verify_endpoint_authority
  verify_time_only_authority
  verify_stage04_exclusion_boundary
  assert_runtime_lock_identity
}

destination_path_at() {
  local root="$1" index="$2"
  printf '%s/completed_prefix/%s' "${root}" \
    "$(entry_destination_relative_path "${index}")"
}

final_destination_path() {
  local index="$1"
  destination_path_at "${bundle_root}" "${index}"
}

verify_source_destination_copy() {
  local root="$1" count index source destination expected
  local source_tuple destination_tuple
  assert_runtime_lock_identity
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    source="$(entry_source_path "${index}")"
    destination="$(destination_path_at "${root}" "${index}")"
    expected="$(entry_expected_sha256 "${index}")"
    verify_hash "${source}" "${expected}" \
      "completed-prefix copy source ${index}"
    verify_hash "${destination}" "${expected}" \
      "completed-prefix copy destination ${index}"
    cmp -s -- "${source}" "${destination}" ||
      fail "completed-prefix source/destination bytes differ: ${index}"
    [[ "$(stat -c '%a:%u:%g:%h' -- "${destination}")" == \
      "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "completed-prefix destination metadata drifted: ${destination}"
    [[ "$(stat -c '%s' -- "${source}")" == \
      "$(stat -c '%s' -- "${destination}")" ]] ||
      fail "completed-prefix source/destination sizes differ: ${index}"
    source_tuple="$(stat -c '%d:%i' -- "${source}")"
    destination_tuple="$(stat -c '%d:%i' -- "${destination}")"
    [[ "${source_tuple}" != "${destination_tuple}" ]] ||
      fail "completed-prefix destination aliases source inode: ${index}"
  done
  assert_runtime_lock_identity
}

emit_regular_pair_inventory() {
  local actual_root="$1" reported_root="$2"
  local count index source_relative source destination_relative
  local destination_reported expected source_stat destination_stat
  local source_mode source_uid source_gid source_links source_bytes
  local source_inode source_device destination_mode destination_uid
  local destination_gid destination_links destination_bytes
  local destination_inode destination_device
  verify_source_destination_copy "${actual_root}"
  printf 'entry_index\tsource_relative_path\tsource_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_sha256\tdestination_relative_path\tdestination_path\tdestination_mode\tdestination_uid\tdestination_gid\tdestination_links\tdestination_bytes\tdestination_inode\tdestination_device\tdestination_sha256\tbyte_identical\tdistinct_inode\n'
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    source_relative="$(entry_source_relative_path "${index}")"
    source="$(entry_source_path "${index}")"
    destination_relative="$(entry_destination_relative_path "${index}")"
    destination="$(destination_path_at "${actual_root}" "${index}")"
    destination_reported="${reported_root}/completed_prefix/${destination_relative}"
    expected="$(entry_expected_sha256 "${index}")"
    source_stat="$(stat -c '%a %u %g %h %s %i %d' -- "${source}")"
    read -r source_mode source_uid source_gid source_links source_bytes \
      source_inode source_device <<<"${source_stat}"
    destination_stat="$(stat -c '%a %u %g %h %s %i %d' -- \
      "${destination}")"
    read -r destination_mode destination_uid destination_gid \
      destination_links destination_bytes destination_inode \
      destination_device <<<"${destination_stat}"
    [[ "${source_device}:${source_inode}" != \
      "${destination_device}:${destination_inode}" ]] ||
      fail "pair inventory encountered aliased inode: ${index}"
    printf '%02d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\ttrue\ttrue\n' \
      "${index}" "${source_relative}" "${source}" "${source_mode}" \
      "${source_uid}" "${source_gid}" "${source_links}" \
      "${source_bytes}" "${source_inode}" "${source_device}" \
      "${expected}" "${destination_relative}" "${destination_reported}" \
      "${destination_mode}" "${destination_uid}" "${destination_gid}" \
      "${destination_links}" "${destination_bytes}" \
      "${destination_inode}" "${destination_device}" "${expected}"
  done
  assert_runtime_lock_identity
}

emit_completed_prefix_directory_inventory() {
  local actual_root="$1" rel path stat_fields mode_value uid_value
  local gid_value links_value bytes_value inode_value device_value count=0
  local expected_directories
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\n'
  capture_command_output expected_directories \
    "expected completed-prefix directories" \
    emit_expected_completed_prefix_directories
  while IFS= read -r rel; do
    path="${actual_root}/completed_prefix"
    [[ "${rel}" == . ]] || path="${path}/${rel}"
    require_dir "${path}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value \
      inode_value device_value <<<"${stat_fields}"
    [[ "${mode_value}:${uid_value}:${gid_value}:${links_value}" == \
      "555:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "completed-prefix directory metadata drifted: ${path}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${rel}" \
      "${mode_value}" "${uid_value}" "${gid_value}" "${links_value}" \
      "${bytes_value}" "${inode_value}" "${device_value}"
    ((count += 1))
  done <"${expected_directories}"
  [[ "${count}" == "${expected_payload_directory_count}" ]] ||
    fail "completed-prefix directory allowlist count drifted: ${count}"
}

compute_payload_content_inventory_sha256() {
  local regular_inventory="$1"
  awk -F '\t' 'NR > 1 { print $12 "\t" $21 }' \
    "${regular_inventory}" | sha256sum | awk '{print $1}'
}

compute_payload_metadata_inventory_sha256() {
  local regular_inventory="$1" directory_inventory="$2"
  {
    printf 'regular_files.inventory.tsv\t%s\n' \
      "$(sha256_of "${regular_inventory}")"
    printf 'directories.inventory.tsv\t%s\n' \
      "$(sha256_of "${directory_inventory}")"
  } | sha256sum | awk '{print $1}'
}

payload_regular_file_bytes() {
  local regular_inventory="$1"
  awk -F '\t' 'NR > 1 { total += $18 }
    END { printf "%.0f", total + 0 }' "${regular_inventory}"
}

emit_bundle_receipt() {
  local destination="$1" regular_inventory="$2" directory_inventory="$3"
  local regular_sha directory_sha content_sha metadata_sha count index
  local source_relative source source_sha destination_relative
  local destination_reported closure_content_sha
  regular_sha="$(sha256_of "${regular_inventory}")"
  directory_sha="$(sha256_of "${directory_inventory}")"
  content_sha="$(compute_payload_content_inventory_sha256 \
    "${regular_inventory}")"
  metadata_sha="$(compute_payload_metadata_inventory_sha256 \
    "${regular_inventory}" "${directory_inventory}")"
  closure_content_sha="$(kv "${interruption_closure_receipt}" \
    source_snapshot_content_inventory_sha256)"
  cat >"${destination}" <<RECEIPT_HEAD
schema_id=${schema_id}
status=complete
bundle_kind=immutable_retry2_completed_prefix_authority_for_retry3
historical_completed_prefix_only=true
bundle_root=${bundle_root}
completed_prefix_root=${completed_prefix_root}
bundle_receipt_path=${bundle_receipt}
regular_inventory_path=${bundle_regular_inventory}
regular_inventory_sha256=${regular_sha}
directory_inventory_path=${bundle_directory_inventory}
directory_inventory_sha256=${directory_sha}
completed_prefix_content_inventory_algorithm=sha256_of_tab_separated_destination_relative_path_and_destination_sha256_lines
completed_prefix_content_inventory_sha256=${content_sha}
completed_prefix_metadata_inventory_algorithm=sha256_of_labeled_regular_and_directory_inventory_sha256_lines
completed_prefix_metadata_inventory_sha256=${metadata_sha}
completed_prefix_regular_file_count=${expected_payload_file_count}
completed_prefix_directory_count=${expected_payload_directory_count}
completed_prefix_regular_file_bytes=$(payload_regular_file_bytes "${regular_inventory}")
bundle_regular_file_count=${expected_bundle_file_count}
bundle_directory_count=${expected_bundle_directory_count}
publication_method=atomic_no_clobber_directory_rename
copy_method=cp_--reflink=never
copy_source=interruption_closure_source_snapshot_except_operational_runner
copy_byte_identity_verified_with_cmp=true
copy_sha256_identity_verified=true
copy_source_destination_inode_tuple_distinct=true
copy_source_link_count=1
copy_destination_link_count=1
hardlinks_authorized=false
source_runtime_schema_id=${source_schema_id}
source_runtime_root=${live_runtime}
source_runtime_lock=${live_runtime_lock}
source_runtime_mutated=false
source_live_runtime_equals_interruption_closure_snapshot=true
source_snapshot_count=2
source_snapshots_identical=true
source_live_runner_lock_acquired_read_only=true
source_development_lock_acquired_read_only=true
source_lock_order=retry2_live_runner_then_retry2_development_lock
source_locks_held_through_two_snapshots_copy_and_publication=true
source_lock_fd_path_identity_rechecked=true
interruption_closure_schema_id=${interruption_closure_schema_id}
interruption_closure_root=${interruption_closure_root}
interruption_closure_receipt_path=${interruption_closure_receipt}
interruption_closure_receipt_sha256=${expected_interruption_closure_receipt_sha256}
interruption_closure_regular_inventory_path=${interruption_closure_regular_inventory}
interruption_closure_regular_inventory_sha256=${expected_interruption_closure_regular_inventory_sha256}
interruption_closure_directory_inventory_path=${interruption_closure_directory_inventory}
interruption_closure_directory_inventory_sha256=${expected_interruption_closure_directory_inventory_sha256}
interruption_closure_source_snapshot_root=${interruption_closure_source_snapshot}
interruption_closure_source_snapshot_content_inventory_sha256=${closure_content_sha}
interruption_closure_source_snapshot_regular_file_count=60
interruption_closure_source_snapshot_directory_count=28
interruption_closure_source_snapshot_regular_file_bytes=58518408
interruption_closure_sealer_path=${interruption_closure_sealer}
interruption_closure_sealer_sha256=${expected_interruption_closure_sealer_sha256}
interruption_closure_frozen_sealer_path=${interruption_closure_frozen_sealer}
interruption_closure_frozen_sealer_sha256=${expected_interruption_closure_sealer_sha256}
interruption_closure_amendment_path=${interruption_closure_amendment}
interruption_closure_amendment_sha256=${expected_interruption_closure_amendment_sha256}
interruption_closure_frozen_amendment_path=${interruption_closure_frozen_amendment}
interruption_closure_frozen_amendment_sha256=${expected_interruption_closure_amendment_sha256}
completed_prefix_bundle_sealer_path=${script_path}
completed_prefix_bundle_sealer_process_start_sha256=${process_start_sealer_sha256}
completed_prefix_bundle_sealer_process_start_mode=0${process_start_sealer_mode}
completed_prefix_bundle_sealer_process_start_links=${process_start_sealer_links}
completed_prefix_bundle_sealer_process_start_owner_uid=${process_start_sealer_uid}
completed_prefix_bundle_sealer_process_start_owner_gid=${process_start_sealer_gid}
completed_prefix_bundle_sealer_process_start_bytes=${process_start_sealer_bytes}
completed_prefix_bundle_sealer_process_start_inode=${process_start_sealer_inode}
completed_prefix_bundle_sealer_process_start_device=${process_start_sealer_device}
frozen_completed_prefix_bundle_sealer_path=${bundle_frozen_sealer}
frozen_completed_prefix_bundle_sealer_sha256=${process_start_sealer_sha256}
retry2_live_runner_path=${live_retry2_runner}
retry2_live_runner_sha256=${expected_retry2_runner_sha256}
retry2_live_runner_inode=${expected_retry2_runner_inode}
retry2_live_runner_device=${expected_retry2_runner_device}
retry2_live_runner_bytes=${expected_retry2_runner_bytes}
retry2_live_runner_mode=0555
retry2_live_runner_links=1
retry2_frozen_runner_source_path=${live_retry2_frozen_runner}
retry2_frozen_runner_source_sha256=${expected_retry2_runner_sha256}
retry2_live_and_frozen_runner_byte_identical=true
retry2_stage_00_attempt_sha256=${expected_stage00_attempt_sha256}
retry2_stage_00_completion_sha256=${expected_stage00_completion_sha256}
retry2_stage_01_attempt_sha256=${expected_stage01_attempt_sha256}
retry2_stage_01_completion_sha256=${expected_stage01_completion_sha256}
retry2_stage_02_attempt_sha256=${expected_stage02_attempt_sha256}
retry2_stage_02_completion_sha256=${expected_stage02_completion_sha256}
retry2_stage_03_attempt_sha256=${expected_stage03_attempt_sha256}
retry2_stage_03_completion_sha256=${expected_stage03_completion_sha256}
retry2_stage_04_attempt_sha256=${expected_stage04_attempt_sha256}
retry2_stage_04_attempt_verified_in_interruption_closure=true
retry2_stage_04_attempt_included=false
completed_stage_count=4
stages_00_through_03_reuse_authorized=true
stage_04_reuse_authorized=false
no_tf_alignment_configuration_included=true
no_tf_alignment_partial_payload_included=false
no_tf_alignment_restart_optimizer_step=0
retry3_runtime_schema_id=${recovery_schema_id}
retry3_runtime_root=${runtime_parent}/${recovery_schema_id}
retry3_reuse_scope=retry2_stages_00_through_03_only
retry3_must_restart_no_tf_alignment_from_optimizer_step=0
retry3_direct_retry2_runtime_consumption_authorized=false
retry3_completed_prefix_bundle_consumption_authorized=true
stage_04_completion_present=false
stage_04_partial_payload_adoption_authorized=false
stage_04_checkpoint_resume_authorized=false
certified_input_access=false
final_holdout_access=false
forecast_access=false
policy_access=false
RECEIPT_HEAD
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    source_relative="$(entry_source_relative_path "${index}")"
    source="$(entry_source_path "${index}")"
    source_sha="$(entry_expected_sha256 "${index}")"
    destination_relative="$(entry_destination_relative_path "${index}")"
    destination_reported="$(final_destination_path "${index}")"
    printf 'payload_file_%02d_source_relative_path=%s\n' \
      "${index}" "${source_relative}" >>"${destination}"
    printf 'payload_file_%02d_source_path=%s\n' \
      "${index}" "${source}" >>"${destination}"
    printf 'payload_file_%02d_source_sha256=%s\n' \
      "${index}" "${source_sha}" >>"${destination}"
    printf 'payload_file_%02d_destination_relative_path=%s\n' \
      "${index}" "${destination_relative}" >>"${destination}"
    printf 'payload_file_%02d_destination_path=%s\n' \
      "${index}" "${destination_reported}" >>"${destination}"
    printf 'payload_file_%02d_destination_sha256=%s\n' \
      "${index}" "${source_sha}" >>"${destination}"
    printf 'payload_file_%02d_byte_identical=true\n' \
      "${index}" >>"${destination}"
    printf 'payload_file_%02d_distinct_inode=true\n' \
      "${index}" >>"${destination}"
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
    fail "completed-prefix receipt has a malformed or duplicate key"
}

verify_receipt_exact() {
  local inspected_receipt="$1" regular_inventory="$2"
  local directory_inventory="$3" expected_receipt
  require_file "${inspected_receipt}"
  [[ "$(stat -c '%a:%u:%g:%h' -- "${inspected_receipt}")" == \
    "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
    fail "completed-prefix receipt metadata drifted"
  new_temp_file expected_receipt "receipt.expected"
  emit_bundle_receipt "${expected_receipt}" "${regular_inventory}" \
    "${directory_inventory}"
  verify_receipt_key_shape "${expected_receipt}"
  verify_receipt_key_shape "${inspected_receipt}"
  cmp -s -- "${expected_receipt}" "${inspected_receipt}" ||
    fail "completed-prefix receipt content drifted"
}

verify_pair_inventory_exact() {
  local root="$1" inspected_inventory="$2" expected_inventory
  require_file "${inspected_inventory}"
  [[ "$(stat -c '%a:%u:%g:%h' -- "${inspected_inventory}")" == \
    "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
    fail "completed-prefix regular inventory metadata drifted"
  [[ "$(($(wc -l <"${inspected_inventory}") - 1))" == \
    "${expected_payload_file_count}" ]] ||
    fail "completed-prefix regular inventory row count drifted"
  new_temp_file expected_inventory "regular.expected"
  emit_regular_pair_inventory "${root}" "${bundle_root}" \
    >"${expected_inventory}"
  cmp -s -- "${expected_inventory}" "${inspected_inventory}" ||
    fail "completed-prefix regular pair inventory drifted"
}

verify_directory_inventory_exact() {
  local root="$1" inspected_inventory="$2" expected_inventory
  require_file "${inspected_inventory}"
  [[ "$(stat -c '%a:%u:%g:%h' -- "${inspected_inventory}")" == \
    "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
    fail "completed-prefix directory inventory metadata drifted"
  [[ "$(($(wc -l <"${inspected_inventory}") - 1))" == \
    "${expected_payload_directory_count}" ]] ||
    fail "completed-prefix directory inventory row count drifted"
  new_temp_file expected_inventory "directories.expected"
  emit_completed_prefix_directory_inventory "${root}" \
    >"${expected_inventory}"
  cmp -s -- "${expected_inventory}" "${inspected_inventory}" ||
    fail "completed-prefix directory inventory drifted"
}

verify_bundle_exclusion_boundary() {
  local root="$1" payload
  payload="${root}/completed_prefix"
  path_is_absent \
    "${payload}/stage.04.no_tf_alignment_training.attempt.status" ||
    fail "completed-prefix bundle includes the Retry2 stage-04 attempt"
  path_is_absent \
    "${payload}/stage.04.no_tf_alignment_training.status" ||
    fail "completed-prefix bundle includes a Retry2 stage-04 completion"
  path_is_absent "${payload}/arms/no_tf_alignment/training" ||
    fail "completed-prefix bundle includes no-TF partial training bytes"
  path_is_absent "${payload}/arms/no_tf_alignment/training.log" ||
    fail "completed-prefix bundle includes no-TF partial training log"
  path_is_absent "${payload}/arms/no_tf_alignment/training.status" ||
    fail "completed-prefix bundle includes no-TF partial training status"
}

verify_bundle_tree_at() {
  local root="$1" receipt regular_inventory directory_inventory frozen_sealer
  receipt="${root}/completed_prefix_bundle.status"
  regular_inventory="${root}/regular_files.inventory.tsv"
  directory_inventory="${root}/directories.inventory.tsv"
  frozen_sealer="${root}/frozen_sources/$(basename "${script_path}")"
  local entry rel file_count=0 directory_count=0 writable
  local expected_directories entries writable_entries
  local -A allowed_files=() allowed_dirs=()
  assert_process_start_identity
  verify_tree_safety "${root}" "completed-prefix bundle"
  allowed_dirs[.]=1
  allowed_dirs[frozen_sources]=1
  capture_command_output expected_directories \
    "expected completed-prefix bundle directories" \
    emit_expected_completed_prefix_directories
  while IFS= read -r rel; do
    if [[ "${rel}" == . ]]; then
      allowed_dirs[completed_prefix]=1
    else
      allowed_dirs["completed_prefix/${rel}"]=1
    fi
  done <"${expected_directories}"
  allowed_files[completed_prefix_bundle.status]=1
  allowed_files[regular_files.inventory.tsv]=1
  allowed_files[directories.inventory.tsv]=1
  allowed_files["frozen_sources/$(basename "${script_path}")"]=1
  local count index destination_relative
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    destination_relative="$(entry_destination_relative_path "${index}")"
    allowed_files["completed_prefix/${destination_relative}"]=1
  done
  capture_find0 entries "${root}" -mindepth 0
  while IFS= read -r -d '' entry; do
    rel="$(relative_path_of "${root}" "${entry}")"
    if [[ -d "${entry}" ]]; then
      [[ -n "${allowed_dirs[${rel}]+present}" ]] ||
        fail "completed-prefix bundle contains unknown directory: ${rel}"
      [[ "$(stat -c '%a:%u:%g:%h' -- "${entry}")" == \
        "555:${process_owner_uid}:${process_owner_gid}:1" ]] ||
        fail "completed-prefix bundle directory metadata drifted: ${rel}"
      ((directory_count += 1))
    elif [[ -f "${entry}" ]]; then
      [[ -n "${allowed_files[${rel}]+present}" ]] ||
        fail "completed-prefix bundle contains unknown file: ${rel}"
      [[ "$(stat -c '%a:%u:%g:%h' -- "${entry}")" == \
        "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
        fail "completed-prefix bundle file metadata drifted: ${rel}"
      ((file_count += 1))
    else
      fail "completed-prefix bundle contains special entry: ${rel}"
    fi
  done <"${entries}"
  [[ "${file_count}" == "${expected_bundle_file_count}" ]] ||
    fail "completed-prefix bundle file count drifted: ${file_count}"
  [[ "${directory_count}" == "${expected_bundle_directory_count}" ]] ||
    fail "completed-prefix bundle directory count drifted: ${directory_count}"
  verify_hash "${frozen_sealer}" "${process_start_sealer_sha256}" \
    "frozen completed-prefix bundle sealer"
  cmp -s -- "${script_path}" "${frozen_sealer}" ||
    fail "live and frozen completed-prefix bundle sealers differ"
  [[ "$(stat -c '%d:%i' -- "${script_path}")" != \
    "$(stat -c '%d:%i' -- "${frozen_sealer}")" ]] ||
    fail "frozen completed-prefix sealer aliases the live sealer inode"
  verify_source_destination_copy "${root}"
  verify_pair_inventory_exact "${root}" "${regular_inventory}"
  verify_directory_inventory_exact "${root}" "${directory_inventory}"
  verify_receipt_exact "${receipt}" "${regular_inventory}" \
    "${directory_inventory}"
  verify_bundle_exclusion_boundary "${root}"
  new_temp_file writable_entries "writable.entries"
  find "${root}" -xdev -perm /222 -print -quit >"${writable_entries}" ||
    fail "could not scan completed-prefix bundle writability"
  writable="$(head -n 1 -- "${writable_entries}")"
  [[ -z "${writable}" ]] ||
    fail "completed-prefix bundle contains a writable entry: ${writable}"
  assert_runtime_lock_identity
}

build_staging_bundle() {
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "completed-prefix staging path already exists: ${staging_root}"
  mkdir -- "${staging_root}"
  candidate_device="$(stat -c '%d' -- "${staging_root}")"
  candidate_inode="$(stat -c '%i' -- "${staging_root}")"
  candidate_created_by_process=true
  mkdir -- "${staging_root}/frozen_sources"
  mkdir -- "${staging_root}/completed_prefix"
  local rel count index source destination expected_directories
  capture_command_output expected_directories \
    "expected staging completed-prefix directories" \
    emit_expected_completed_prefix_directories
  while IFS= read -r rel; do
    [[ "${rel}" == . ]] && continue
    mkdir -p -- "${staging_root}/completed_prefix/${rel}"
  done <"${expected_directories}"
  count="$(entry_count)"
  for ((index = 0; index < count; ++index)); do
    source="$(entry_source_path "${index}")"
    destination="$(destination_path_at "${staging_root}" "${index}")"
    cp --reflink=never -- "${source}" "${destination}"
  done
  cp --reflink=never -- "${script_path}" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")"
  find "${staging_root}/completed_prefix" -xdev -type f \
    -exec chmod 0444 -- {} +
  find "${staging_root}/completed_prefix" -xdev -depth -type d \
    -exec chmod 0555 -- {} +
  verify_source_destination_copy "${staging_root}"
  emit_regular_pair_inventory "${staging_root}" "${bundle_root}" \
    >"${staging_root}/regular_files.inventory.tsv"
  emit_completed_prefix_directory_inventory "${staging_root}" \
    >"${staging_root}/directories.inventory.tsv"
  emit_bundle_receipt "${staging_root}/completed_prefix_bundle.status" \
    "${staging_root}/regular_files.inventory.tsv" \
    "${staging_root}/directories.inventory.tsv"
  find "${staging_root}" -xdev -type f -exec chmod 0444 -- {} +
  find "${staging_root}" -xdev -depth -type d -exec chmod 0555 -- {} +
  assert_process_start_identity
  assert_runtime_lock_identity
}

publish_staging_bundle() {
  local staging_device staging_inode destination_device destination_inode
  local parent_device parent_inode
  [[ ! -e "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "refusing to overwrite an existing completed-prefix bundle"
  require_dir "${staging_root}"
  staging_device="$(stat -c '%d' -- "${staging_root}")"
  staging_inode="$(stat -c '%i' -- "${staging_root}")"
  parent_device="$(stat -c '%d' -- "${runtime_parent}")"
  parent_inode="$(stat -c '%i' -- "${runtime_parent}")"
  [[ "${staging_device}" == "${parent_device}" ]] ||
    fail "completed-prefix staging root is on the wrong publication device"
  [[ "${staging_device}" == "${expected_retry2_runtime_device}" ]] ||
    fail "completed-prefix staging root is not on the pinned 00:42 publication mount"
  assert_tree_has_no_live_references "${staging_root}" \
    "completed-prefix staging tree"
  assert_runtime_lock_identity
  [[ "$(stat -c '%d:%i' -- "${runtime_parent}")" == \
    "${parent_device}:${parent_inode}" ]] ||
    fail "completed-prefix publication parent identity drifted"
  [[ "$(stat -c '%d:%i' -- "${staging_root}")" == \
    "${staging_device}:${staging_inode}" ]] ||
    fail "completed-prefix staging path identity drifted before publication"
  [[ ! -e "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "completed-prefix destination appeared before publication"
  mv -T -n -- "${staging_root}" "${bundle_root}"
  if [[ -e "${staging_root}" || -L "${staging_root}" ]]; then
    fail "atomic no-clobber completed-prefix publication did not consume staging"
  fi
  [[ -d "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "completed-prefix publication did not create the final bundle"
  destination_device="$(stat -c '%d' -- "${bundle_root}")"
  destination_inode="$(stat -c '%i' -- "${bundle_root}")"
  [[ "${destination_device}:${destination_inode}" == \
    "${staging_device}:${staging_inode}" ]] ||
    fail "completed-prefix publication did not preserve candidate inode identity"
  candidate_created_by_process=false
}

assert_audit_publication_absent() {
  [[ ! -e "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "audit requires the completed-prefix bundle to remain unpublished"
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "audit requires the completed-prefix staging path to remain absent"
}

compare_source_snapshots() {
  local files_1="$1" directories_1="$2" live_content_1="$3"
  local closure_content_1="$4" files_2="$5" directories_2="$6"
  local live_content_2="$7" closure_content_2="$8"
  cmp -s -- "${files_1}" "${files_2}" ||
    fail "completed-prefix source file snapshots differ"
  cmp -s -- "${directories_1}" "${directories_2}" ||
    fail "completed-prefix source directory snapshots differ"
  cmp -s -- "${live_content_1}" "${live_content_2}" ||
    fail "live Retry2 normalized-content snapshots differ"
  cmp -s -- "${closure_content_1}" "${closure_content_2}" ||
    fail "closure normalized-content snapshots differ"
  cmp -s -- "${live_content_2}" "${closure_content_2}" ||
    fail "second live Retry2 and closure normalized-content snapshots differ"
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

assert_process_start_identity
[[ "${process_start_sealer_mode}:${process_start_sealer_uid}:${process_start_sealer_gid}:${process_start_sealer_links}" == \
  "555:${process_owner_uid}:${process_owner_gid}:1" ]] ||
  fail "completed-prefix bundle sealer must be sealed mode 0555, owned by the process owner, with link count one"
reject_symlink_components "${runtime_parent}"
require_dir "${runtime_parent}"
[[ "$(realpath -e -- "${runtime_parent}")" == "${runtime_parent}" ]] ||
  fail "runtime parent is not canonical"
verify_allowlist_shape
[[ "$(emit_expected_completed_prefix_directories | wc -l)" == \
  "${expected_payload_directory_count}" ]] ||
  fail "completed-prefix directory allowlist count drifted"
if [[ "${mode}" == --audit ]]; then
  assert_audit_publication_absent
fi
acquire_locks_in_order

source_files_1=""
source_directories_1=""
live_content_1=""
closure_content_1=""
source_files_2=""
source_directories_2=""
live_content_2=""
closure_content_2=""
new_temp_file source_files_1 "source.files.snapshot1"
new_temp_file source_directories_1 "source.directories.snapshot1"
new_temp_file live_content_1 "live.content.snapshot1"
new_temp_file closure_content_1 "closure.content.snapshot1"
new_temp_file source_files_2 "source.files.snapshot2"
new_temp_file source_directories_2 "source.directories.snapshot2"
new_temp_file live_content_2 "live.content.snapshot2"
new_temp_file closure_content_2 "closure.content.snapshot2"

take_source_snapshot "${source_files_1}" "${source_directories_1}" \
  "${live_content_1}" "${closure_content_1}"
verify_completed_prefix_semantics

if [[ "${mode}" == --audit ]]; then
  take_source_snapshot "${source_files_2}" "${source_directories_2}" \
    "${live_content_2}" "${closure_content_2}"
  compare_source_snapshots "${source_files_1}" "${source_directories_1}" \
    "${live_content_1}" "${closure_content_1}" "${source_files_2}" \
    "${source_directories_2}" "${live_content_2}" "${closure_content_2}"
  verify_completed_prefix_semantics
  assert_audit_publication_absent
  echo "retry2_completed_prefix_bundle_audit=complete"
  echo "source_snapshots_identical=true"
  echo "live_runtime_equals_interruption_closure_snapshot=true"
  echo "stages_00_through_03_reuse_authorized=true"
  echo "stage_04_reuse_authorized=false"
  echo "bundle_published=false"
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  [[ -d "${bundle_root}" && ! -L "${bundle_root}" ]] ||
    fail "completed-prefix bundle is not published: ${bundle_root}"
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "completed-prefix staging path remains present during verification"
  verify_bundle_tree_at "${bundle_root}"
  verify_completed_prefix_semantics
  take_source_snapshot "${source_files_2}" "${source_directories_2}" \
    "${live_content_2}" "${closure_content_2}"
  compare_source_snapshots "${source_files_1}" "${source_directories_1}" \
    "${live_content_1}" "${closure_content_1}" "${source_files_2}" \
    "${source_directories_2}" "${live_content_2}" "${closure_content_2}"
  verify_bundle_tree_at "${bundle_root}"
  echo "completed_prefix_bundle_receipt=${bundle_receipt}"
  echo "completed_prefix_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
  exit 0
fi

if [[ -e "${bundle_root}" || -L "${bundle_root}" ]]; then
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "completed-prefix staging path remains beside published bundle"
  verify_bundle_tree_at "${bundle_root}"
  verify_completed_prefix_semantics
  take_source_snapshot "${source_files_2}" "${source_directories_2}" \
    "${live_content_2}" "${closure_content_2}"
  compare_source_snapshots "${source_files_1}" "${source_directories_1}" \
    "${live_content_1}" "${closure_content_1}" "${source_files_2}" \
    "${source_directories_2}" "${live_content_2}" "${closure_content_2}"
  verify_bundle_tree_at "${bundle_root}"
  echo "completed_prefix_bundle_receipt=${bundle_receipt}"
  echo "completed_prefix_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
  exit 0
fi

build_staging_bundle
verify_bundle_tree_at "${staging_root}"
verify_completed_prefix_semantics
take_source_snapshot "${source_files_2}" "${source_directories_2}" \
  "${live_content_2}" "${closure_content_2}"
compare_source_snapshots "${source_files_1}" "${source_directories_1}" \
  "${live_content_1}" "${closure_content_1}" "${source_files_2}" \
  "${source_directories_2}" "${live_content_2}" "${closure_content_2}"
verify_bundle_tree_at "${staging_root}"
publish_staging_bundle
verify_bundle_tree_at "${bundle_root}"
assert_runtime_lock_identity
echo "completed_prefix_bundle_receipt=${bundle_receipt}"
echo "completed_prefix_bundle_receipt_sha256=$(sha256_of "${bundle_receipt}")"
