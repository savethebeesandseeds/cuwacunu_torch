#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_mdn_train_isolated_v2.interruption.v1"
readonly interrupted_schema_id="synthetic_v2_mdn_train_isolated_v2"
readonly retry_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1"
readonly retry_result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"

readonly expected_amendment_sha256="a5b5a5dcda52c9e89d37f33db15678c5ceb67904f2c3ae187701123208573f8e"
readonly expected_runner_sha256="62defc3a15478c4aec1106c3a84105ef12aa1a65ec9bbafbc0669e96a62c2349"
readonly expected_inputs_sha256="dd766bec5f89d8013e3df14637f73af83cf8b1fd2676b195eab5995d2bacaf24"
readonly expected_config_sha256="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
readonly expected_objective_sha256="33b40ce2f6a76f0c9dddc67b9e3b162d1a171199b6d50b174dafaf854b135d5e"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_cursor_erratum_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly expected_representation_result_sha256="981971679e4c37ba23919aae549eab1bc1ea1c1452f1978c004802f4807dbd07"
readonly expected_representation_checkpoint_sha256="70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d"

readonly expected_optimizer_steps_observed=620
readonly expected_last_checkpoint_optimizer_step=600
readonly expected_requested_optimizer_steps=3500
readonly expected_accepted_anchor_count=3261
readonly expected_payload_directory_count=7
readonly expected_payload_file_count=15

fail() {
  echo "synthetic v2 interrupted MDN closure: $*" >&2
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
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${runtime_parent}/${interrupted_schema_id}"
job_dir="${runtime_root}/job"

amendment_path="${benchmark_root}/MDN_ENGINE_INTERRUPTION_RECOVERY_AMENDMENT.md"
runner_path="${script_dir}/run_mdn_train_isolated_v2.sh"
inputs_path="${runtime_root}/inputs.status"
config_path="${runtime_root}/synthetic_benchmark.train_core_channel_mdn.isolated.config"
objective_path="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
runtime_exec_path="${repo_root}/.build/exec/cuwacunu_exec"
source_closure_path="${runtime_parent}/synthetic_v2_isolated_development_source_v1/development_source_closure.status"
cursor_erratum_path="${runtime_parent}/synthetic_v2_isolated_development_source_v1/cursor_alignment_erratum.status"
representation_result_path="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/result.status"
representation_checkpoint_path="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
manifest_path="${job_dir}/job.manifest"
report_path="${job_dir}/channel_inference.report"
checkpoint_path="${job_dir}/channel_inference.report.channel_mdn.pt"
log_path="${runtime_root}/mdn_train.log"
execution_lock_path="${runtime_root}/.execution.lock"
event_probe_path="${job_dir}/runtime.job_events.probe"
representation_probe_path="${job_dir}/representation_edge_features.probe"
mdn_lls_path="${job_dir}/channel_inference.report.mdn.lls"
nodelift_lls_path="${job_dir}/channel_inference.report.nodelift.lls"
representation_lls_path="${job_dir}/channel_inference.report.representation.lls"
runtime_layout_path="${runtime_root}/system/runtime_layout.v1.lls"
spawn_registry_path="${runtime_root}/system/component_spawn_registry.v1.lls"
spawn_ref_path="${runtime_root}/components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"
receipt_path="${runtime_root}/interruption.status"
staging_path="${runtime_parent}/.${interrupted_schema_id}.interruption.status.candidate"
retry_runtime_root="${runtime_parent}/${retry_schema_id}"

declare -Ar expected_directories=(
  ["."]=1
  ["components"]=1
  ["components/wikimyei.inference.expected_value.mdn"]=1
  ["components/wikimyei.inference.expected_value.mdn/spawns"]=1
  ["components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb"]=1
  ["job"]=1
  ["system"]=1
)

declare -Ar expected_payload_hashes=(
  [".execution.lock"]="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  ["components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"]="3d0a65d5bc6c6e1b2b1782f77fe370e6f67af906a35d538e8b8da77d8d411543"
  ["inputs.status"]="dd766bec5f89d8013e3df14637f73af83cf8b1fd2676b195eab5995d2bacaf24"
  ["job/channel_inference.report"]="148bfe89c24d6ca8009ea6a7e30b9d1504b0bc44a0f24c2ababff725fb2a89e7"
  ["job/channel_inference.report.channel_mdn.pt"]="ab67674cefd9749b11b8302829489ad5cec8f6242abe3f0cb3e5b6bb4983bdc5"
  ["job/channel_inference.report.mdn.lls"]="74a4dcb2a3556919f6b952db2b035ee26fe779bc59af31688d1221778904b0ad"
  ["job/channel_inference.report.nodelift.lls"]="935f29ed8841465a09527aa4bf6d7ef35ec312559c49b6acda967712765adf35"
  ["job/channel_inference.report.representation.lls"]="a35189051c2deaeae20508f6d2fa62c225df8a117213b55c036e4c6160f949c9"
  ["job/job.manifest"]="f35e4835d14c6dc47fafd0ad4fc5eed63b838eea47fd3949b467b1ad2ac11479"
  ["job/representation_edge_features.probe"]="0772d0116a7547eae0a3adf8c149fc64eecc9f03236b267d4a3af3cceb59b981"
  ["job/runtime.job_events.probe"]="5966bf5053c7367ecb719855939bd5ef78b16c0646e18dcb3ee1b157af1d90ca"
  ["mdn_train.log"]="30d1e5663fb5026948d9bf12e9ad033c81494ab6924b3381250f3656f6adf1a3"
  ["synthetic_benchmark.train_core_channel_mdn.isolated.config"]="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
  ["system/component_spawn_registry.v1.lls"]="1213defa5898446e03de4458d0ebec8cf102744a133de2a3cbc4a3ec1fe16197"
  ["system/runtime_layout.v1.lls"]="c9647d3e919a1a6c801afefdb15f138e05a4e7e3adda1bde15ab056551621197"
)

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--seal|--verify]"
case "${mode}" in
--plan | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
interrupted_runtime_root=${runtime_root}
interruption_receipt_path=${receipt_path}
payload_directory_count=${expected_payload_directory_count}
payload_file_count_before_receipt=${expected_payload_file_count}
optimizer_steps_observed=${expected_optimizer_steps_observed}
last_checkpoint_optimizer_step=${expected_last_checkpoint_optimizer_step}
external_orchestrator_observation=docker_engine_exit_255
process_exit_status_claim=none
scientific_evidence=false
resume_authorized=false
partial_artifact_reuse_authorized=false
retry_schema_id=${retry_schema_id}
retry_runtime_root=${retry_runtime_root}
retry_restart_from_step_zero=true
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

verify_hash() {
  local path="$1"
  local expected="$2"
  local label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
}

verify_external_provenance() {
  verify_hash "${amendment_path}" "${expected_amendment_sha256}" \
    "interruption amendment"
  verify_hash "${runner_path}" "${expected_runner_sha256}" \
    "original MDN runner"
  verify_hash "${objective_path}" "${expected_objective_sha256}" \
    "MDN objective"
  verify_hash "${runtime_exec_path}" "${expected_runtime_exec_sha256}" \
    "runtime executable"
  verify_hash "${source_closure_path}" "${expected_source_closure_sha256}" \
    "isolated-source closure"
  verify_hash "${cursor_erratum_path}" "${expected_cursor_erratum_sha256}" \
    "cursor-alignment erratum receipt"
  verify_hash "${representation_result_path}" \
    "${expected_representation_result_sha256}" "representation result"
  verify_hash "${representation_checkpoint_path}" \
    "${expected_representation_checkpoint_sha256}" \
    "representation checkpoint"

  expect_kv "${source_closure_path}" schema_id \
    synthetic_v2_isolated_development_source_v1
  expect_kv "${source_closure_path}" status complete
  expect_kv "${source_closure_path}" accepted_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${source_closure_path}" candidate_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${source_closure_path}" skipped_outside_common_range 0
  expect_kv "${source_closure_path}" skipped_missing_edge_coverage 0
  expect_kv "${source_closure_path}" skipped_failed_fetch_probe 0
  expect_kv "${source_closure_path}" duplicate_anchor_count 0
  expect_kv "${source_closure_path}" canonical_data_raw_access false
  expect_kv "${source_closure_path}" final_holdout_available false

  expect_kv "${cursor_erratum_path}" schema_id \
    synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1
  expect_kv "${cursor_erratum_path}" status complete
  expect_kv "${cursor_erratum_path}" accepted_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${cursor_erratum_path}" candidate_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${cursor_erratum_path}" maximum_anchor_index 3260
  expect_kv "${cursor_erratum_path}" canonical_data_raw_access false
  expect_kv "${cursor_erratum_path}" independent_final_evidence false

  expect_kv "${representation_result_path}" schema_id \
    synthetic_v2_representation_train_isolated_v2.result.v1
  expect_kv "${representation_result_path}" status complete
  expect_kv "${representation_result_path}" checkpoint_path \
    "${representation_checkpoint_path}"
  expect_kv "${representation_result_path}" checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${representation_result_path}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${representation_result_path}" accepted_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${representation_result_path}" restart_from_step_zero true
  expect_kv "${representation_result_path}" quarantined_checkpoint_access false
  expect_kv "${representation_result_path}" canonical_data_raw_access false
  expect_kv "${representation_result_path}" final_holdout_available false
  expect_kv "${representation_result_path}" policy_access false
}

verify_exact_tree() {
  local expect_receipt="$1"
  reject_symlink_components "${runtime_root}"
  require_dir "${runtime_root}"
  [[ "$(realpath -e -- "${runtime_root}")" == "${runtime_root}" ]] ||
    fail "interrupted runtime root is not canonical"

  local entry rel dir_count=0 file_count=0
  while IFS= read -r -d '' entry; do
    if [[ "${entry}" == "${runtime_root}" ]]; then
      rel="."
    else
      rel="${entry#${runtime_root}/}"
    fi
    [[ ! -L "${entry}" ]] || fail "runtime contains symlink: ${rel}"
    if [[ -d "${entry}" ]]; then
      [[ -n "${expected_directories[${rel}]+present}" ]] ||
        fail "runtime contains unknown directory: ${rel}"
      ((dir_count += 1))
    elif [[ -f "${entry}" ]]; then
      if [[ -n "${expected_payload_hashes[${rel}]+present}" ]]; then
        ((file_count += 1))
      elif [[ "${expect_receipt}" == true && \
        "${entry}" == "${receipt_path}" ]]; then
        ((file_count += 1))
      else
        fail "runtime contains unknown file: ${rel}"
      fi
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "runtime file has an external hard link: ${rel}"
    else
      fail "runtime contains a special entry: ${rel}"
    fi
  done < <(find "${runtime_root}" -mindepth 0 -print0)

  [[ "${dir_count}" == "${expected_payload_directory_count}" ]] ||
    fail "expected ${expected_payload_directory_count} directories, found ${dir_count}"
  local expected_total_files="${expected_payload_file_count}"
  [[ "${expect_receipt}" == true ]] && ((expected_total_files += 1))
  [[ "${file_count}" == "${expected_total_files}" ]] ||
    fail "expected ${expected_total_files} files, found ${file_count}"
}

verify_payload_hashes() {
  local rel path actual
  for rel in "${!expected_payload_hashes[@]}"; do
    path="${runtime_root}/${rel}"
    reject_symlink_components "${path}"
    require_file "${path}"
    actual="$(sha256_of "${path}")"
    [[ "${actual}" == "${expected_payload_hashes[${rel}]}" ]] ||
      fail "interrupted payload hash drifted: ${rel}"
  done
}

verify_input_receipt() {
  verify_hash "${inputs_path}" "${expected_inputs_sha256}" \
    "original input receipt"
  expect_kv "${inputs_path}" schema_id \
    synthetic_v2_mdn_train_isolated_v2.inputs.v1
  expect_kv "${inputs_path}" runner_path "${runner_path}"
  expect_kv "${inputs_path}" runner_sha256 "${expected_runner_sha256}"
  expect_kv "${inputs_path}" isolated_source_closure_path \
    "${source_closure_path}"
  expect_kv "${inputs_path}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${inputs_path}" cursor_alignment_erratum_receipt_path \
    "${cursor_erratum_path}"
  expect_kv "${inputs_path}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_sha256}"
  expect_kv "${inputs_path}" config_snapshot_path "${config_path}"
  expect_kv "${inputs_path}" config_snapshot_sha256 \
    "${expected_config_sha256}"
  expect_kv "${inputs_path}" runtime_exec_path "${runtime_exec_path}"
  expect_kv "${inputs_path}" runtime_exec_sha256 \
    "${expected_runtime_exec_sha256}"
  expect_kv "${inputs_path}" representation_result_path \
    "${representation_result_path}"
  expect_kv "${inputs_path}" representation_result_sha256 \
    "${expected_representation_result_sha256}"
  expect_kv "${inputs_path}" representation_checkpoint_path \
    "${representation_checkpoint_path}"
  expect_kv "${inputs_path}" representation_checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${inputs_path}" train_anchor_begin 0
  expect_kv "${inputs_path}" train_anchor_end_exclusive 2496
  expect_kv "${inputs_path}" expected_optimizer_steps \
    "${expected_requested_optimizer_steps}"
  expect_kv "${inputs_path}" expected_accepted_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${inputs_path}" expected_candidate_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${inputs_path}" expected_maximum_available_anchor_index 3260
  expect_kv "${inputs_path}" restart_from_step_zero true
  expect_kv "${inputs_path}" quarantined_checkpoint_access false
  expect_kv "${inputs_path}" canonical_data_raw_access false
  expect_kv "${inputs_path}" final_holdout_available false
  expect_kv "${inputs_path}" policy_access false
}

verify_checkpoint_step_metadata() {
  command -v python3 >/dev/null 2>&1 ||
    fail "python3 is required to verify checkpoint step metadata"
  python3 - "${checkpoint_path}" <<'PY' ||
import struct
import sys
import zipfile

checkpoint = sys.argv[1]
with zipfile.ZipFile(checkpoint, "r") as archive:
    names = archive.namelist()
    for leaf in ("data/200", "data/201"):
        matches = [name for name in names if name.endswith("/" + leaf)]
        if len(matches) != 1:
            raise SystemExit(
                f"checkpoint must contain exactly one {leaf}, found {len(matches)}"
            )
        payload = archive.read(matches[0])
        if len(payload) != 8 or struct.unpack("<q", payload)[0] != 600:
            raise SystemExit(f"checkpoint {leaf} is not int64 optimizer step 600")
PY
  {
    fail "partial MDN checkpoint step metadata disagrees with step 600"
  }
}

typed_uint_summary() {
  local path="$1"
  local field="$2"
  awk -v field="${field}" '
    index($0, field "[") == 1 {
      eq = index($0, " = ");
      if (eq == 0) exit 41;
      value = substr($0, eq + 3);
      if (value !~ /^[0-9]+$/) exit 42;
      numeric = value + 0;
      if (count == 0 || numeric > maximum) maximum = numeric;
      count += 1;
    }
    END {
      if (count == 0) exit 43;
      print count, maximum;
    }
  ' "${path}"
}

plain_uint_summary() {
  local path="$1"
  local field="$2"
  awk -v field="${field}" '
    index($0, field "=") == 1 {
      value = substr($0, length(field) + 2);
      if (value !~ /^[0-9]+$/) exit 42;
      numeric = value + 0;
      if (count == 0 || numeric < minimum) minimum = numeric;
      if (count == 0 || numeric > maximum) maximum = numeric;
      count += 1;
    }
    END {
      if (count == 0) exit 43;
      print count, minimum, maximum;
    }
  ' "${path}"
}

verify_step_sidecars() {
  local count minimum maximum
  read -r count maximum < <(typed_uint_summary "${mdn_lls_path}" optimizer_steps)
  [[ "${count}" == 1 && "${maximum}" == \
    "${expected_optimizer_steps_observed}" ]] ||
    fail "MDN LLS optimizer-step state is not exactly step 620"
  read -r count maximum < <(typed_uint_summary "${mdn_lls_path}" wave_pulse_index)
  [[ "${count}" == 1 && "${maximum}" == \
    "${expected_optimizer_steps_observed}" ]] ||
    fail "MDN LLS wave-pulse state is not exactly step 620"
  read -r count minimum maximum < <(plain_uint_summary "${event_probe_path}" step)
  [[ "${count}" == 7130 && "${minimum}" == 10 && "${maximum}" == \
    "${expected_optimizer_steps_observed}" ]] ||
    fail "runtime event probe step records are not count 7130 / range [10,620]"
}

verify_manifest_and_report() {
  verify_hash "${config_path}" "${expected_config_sha256}" \
    "original configuration snapshot"
  verify_hash "${manifest_path}" \
    "${expected_payload_hashes[job/job.manifest]}" "original job manifest"
  verify_hash "${report_path}" \
    "${expected_payload_hashes[job/channel_inference.report]}" \
    "original MDN report"
  verify_hash "${checkpoint_path}" \
    "${expected_payload_hashes[job/channel_inference.report.channel_mdn.pt]}" \
    "partial MDN checkpoint"
  verify_hash "${log_path}" "${expected_payload_hashes[mdn_train.log]}" \
    "original MDN log"

  expect_kv "${config_path}" runtime_wave_id train_core_channel_mdn
  expect_kv "${config_path}" ujcamei_source_registry_dsl_path \
    "${runtime_parent}/synthetic_v2_isolated_development_source_v1/config/ujcamei.source.registry.development_prefix.dsl"
  expect_kv "${config_path}" wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${objective_path}"

  expect_kv "${manifest_path}" manifest_format \
    kikijyeba.runtime.job_manifest.v1
  expect_kv "${manifest_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${manifest_path}" config_bundle_id 4aade55ae756faa6
  expect_kv "${manifest_path}" config_receipt_id fc09e9181f37e44e
  expect_kv "${manifest_path}" config_path "${config_path}"
  expect_kv "${manifest_path}" wave_id train_core_channel_mdn
  expect_kv "${manifest_path}" wave_action train
  expect_kv "${manifest_path}" source_range_policy anchor_index
  expect_kv "${manifest_path}" resolved_anchor_index_begin 0
  expect_kv "${manifest_path}" resolved_anchor_index_end 2496
  expect_kv "${manifest_path}" accepted_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${manifest_path}" candidate_anchor_count \
    "${expected_accepted_anchor_count}"
  expect_kv "${manifest_path}" skipped_outside_common_range 0
  expect_kv "${manifest_path}" skipped_missing_edge_coverage 0
  expect_kv "${manifest_path}" skipped_failed_fetch_probe 0
  expect_kv "${manifest_path}" duplicate_anchor_count 0
  expect_kv "${manifest_path}" input_representation_checkpoint_path \
    "${representation_checkpoint_path}"
  expect_kv "${manifest_path}" input_mdn_checkpoint_path ''
  expect_kv "${manifest_path}" policy_training_contract_schema ''
  expect_kv "${manifest_path}" policy_training_artifact_schema ''
  [[ "$(kv "${manifest_path}" source_file_receipts)" != *"/data/raw/"* ]] ||
    fail "original manifest references canonical data/raw"
  [[ "$(kv "${manifest_path}" source_file_receipts)" == \
    *"/synthetic_v2_isolated_development_source_v1/source/"* ]] ||
    fail "original manifest omitted the isolated source mirror"
  [[ "$(kv "${manifest_path}" execution_chain)" != *"policy"* ]] ||
    fail "original execution chain reached policy"

  expect_kv "${report_path}" training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${report_path}" mixture_count 3
  expect_kv "${report_path}" hidden_width 128
  expect_kv "${report_path}" residual_depth 2
  expect_kv "${report_path}" feature_embedding_dim 8
  expect_kv "${report_path}" channel_adapter_rank 16
  expect_kv "${report_path}" loss_reduction balanced_channel_feature_mean
  expect_kv "${report_path}" edge_return_auxiliary_loss_weight 0
  expect_kv "${report_path}" direct_edge_return_readout_loss_weight 100
  expect_kv "${report_path}" direct_edge_return_readout_direction_weight 5
  expect_kv "${report_path}" direct_edge_return_readout_rank_weight 5
  expect_kv "${report_path}" direct_edge_return_readout_huber_beta 0.5
  expect_kv "${report_path}" direct_edge_return_readout_logit_scale 1
  expect_kv "${report_path}" direct_edge_return_readout_target_scale 36
  expect_kv "${report_path}" direct_edge_return_readout_warmup_steps 800
  expect_kv "${report_path}" direct_edge_return_readout_warmup_nll_weight 0
  expect_kv "${report_path}" \
    direct_edge_return_readout_post_warmup_nll_weight 1
  expect_kv "${report_path}" \
    direct_edge_return_readout_warmup_direct_head_only true
  expect_kv "${report_path}" direct_edge_return_readout_identity_mode \
    edge_embedding_per_edge
  expect_kv "${report_path}" effective_batch_size 64
  expect_kv "${report_path}" seed 31
  expect_kv "${report_path}" wave_id train_core_channel_mdn
  expect_kv "${report_path}" target_action train
  expect_kv "${report_path}" representation_checkpoint_path \
    "${representation_checkpoint_path}"
  expect_kv "${report_path}" representation_checkpoint_loaded true
  expect_kv "${report_path}" mdn_checkpoint_path ''
  expect_kv "${report_path}" mdn_checkpoint_loaded false
  expect_kv "${report_path}" optimizer_steps \
    "${expected_optimizer_steps_observed}"
  expect_kv "${report_path}" direct_edge_return_readout_warmup_step_count \
    "${expected_optimizer_steps_observed}"
  expect_kv "${report_path}" \
    direct_edge_return_readout_direct_head_only_warmup_step_count \
    "${expected_optimizer_steps_observed}"
  expect_kv "${report_path}" finite_parameter_check 1
  expect_kv "${report_path}" nonfinite_output_count 0
  expect_kv "${report_path}" checkpoint_written true
  expect_kv "${report_path}" checkpoint_write_count 12
  expect_kv "${report_path}" last_checkpoint_optimizer_step \
    "${expected_last_checkpoint_optimizer_step}"
  expect_kv "${report_path}" checkpoint_path "${checkpoint_path}"
  expect_kv "${report_path}" report_written true
  expect_kv "${report_path}" report_write_count 62
  expect_kv "${report_path}" last_report_attempted_step \
    "${expected_optimizer_steps_observed}"
  verify_checkpoint_step_metadata
  verify_step_sidecars
}

verify_absent_completion() {
  local path
  for path in "${runtime_root}/result.status" \
    "${job_dir}/runtime.result.fact"; do
    [[ ! -e "${path}" && ! -L "${path}" ]] ||
      fail "interrupted runtime unexpectedly has a completion artifact: ${path}"
  done
}

verify_interrupted_payload() {
  local expect_receipt="$1"
  verify_exact_tree "${expect_receipt}"
  verify_payload_hashes
  verify_external_provenance
  verify_input_receipt
  verify_manifest_and_report
  verify_absent_completion
}

emit_receipt() {
  local destination="$1"
  cat >"${destination}" <<RECEIPT
schema_id=${schema_id}
closure_status=complete
attempt_status=interrupted
evidence_class=operational_interruption_record
scientific_evidence=false
metric_evidence_authorized=false
interruption_sealer_path=${script_path}
interruption_sealer_sha256=$(sha256_of "${script_path}")
interruption_amendment_path=${amendment_path}
interruption_amendment_sha256=${expected_amendment_sha256}
interrupted_runtime_schema_id=${interrupted_schema_id}
interrupted_runtime_root=${runtime_root}
original_runner_path=${runner_path}
original_runner_sha256=${expected_runner_sha256}
input_receipt_path=${inputs_path}
input_receipt_sha256=${expected_inputs_sha256}
config_snapshot_path=${config_path}
config_snapshot_sha256=${expected_config_sha256}
mdn_objective_path=${objective_path}
mdn_objective_sha256=${expected_objective_sha256}
runtime_exec_path=${runtime_exec_path}
runtime_exec_sha256=${expected_runtime_exec_sha256}
isolated_source_closure_path=${source_closure_path}
isolated_source_closure_sha256=${expected_source_closure_sha256}
cursor_alignment_erratum_receipt_path=${cursor_erratum_path}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_erratum_sha256}
representation_result_path=${representation_result_path}
representation_result_sha256=${expected_representation_result_sha256}
representation_checkpoint_path=${representation_checkpoint_path}
representation_checkpoint_sha256=${expected_representation_checkpoint_sha256}
execution_lock_path=${execution_lock_path}
execution_lock_sha256=${expected_payload_hashes[.execution.lock]}
job_manifest_path=${manifest_path}
job_manifest_sha256=${expected_payload_hashes[job/job.manifest]}
mdn_report_path=${report_path}
mdn_report_sha256=${expected_payload_hashes[job/channel_inference.report]}
partial_mdn_checkpoint_path=${checkpoint_path}
partial_mdn_checkpoint_sha256=${expected_payload_hashes[job/channel_inference.report.channel_mdn.pt]}
train_log_path=${log_path}
train_log_sha256=${expected_payload_hashes[mdn_train.log]}
runtime_event_probe_path=${event_probe_path}
runtime_event_probe_sha256=${expected_payload_hashes[job/runtime.job_events.probe]}
representation_edge_probe_path=${representation_probe_path}
representation_edge_probe_sha256=${expected_payload_hashes[job/representation_edge_features.probe]}
mdn_lls_path=${mdn_lls_path}
mdn_lls_sha256=${expected_payload_hashes[job/channel_inference.report.mdn.lls]}
nodelift_lls_path=${nodelift_lls_path}
nodelift_lls_sha256=${expected_payload_hashes[job/channel_inference.report.nodelift.lls]}
representation_lls_path=${representation_lls_path}
representation_lls_sha256=${expected_payload_hashes[job/channel_inference.report.representation.lls]}
runtime_layout_path=${runtime_layout_path}
runtime_layout_sha256=${expected_payload_hashes[system/runtime_layout.v1.lls]}
component_spawn_registry_path=${spawn_registry_path}
component_spawn_registry_sha256=${expected_payload_hashes[system/component_spawn_registry.v1.lls]}
component_spawn_ref_path=${spawn_ref_path}
component_spawn_ref_sha256=${expected_payload_hashes[components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref]}
payload_directory_count=${expected_payload_directory_count}
payload_file_count_before_receipt=${expected_payload_file_count}
sealed_file_count_including_receipt=16
train_anchor_range=[0,2496)
accepted_anchor_count=${expected_accepted_anchor_count}
candidate_anchor_count=${expected_accepted_anchor_count}
maximum_available_anchor_index=3260
skipped_outside_common_range=0
skipped_missing_edge_coverage=0
skipped_failed_fetch_probe=0
duplicate_anchor_count=0
requested_optimizer_steps=${expected_requested_optimizer_steps}
optimizer_steps_observed=${expected_optimizer_steps_observed}
last_report_attempted_step=${expected_optimizer_steps_observed}
report_write_count=62
last_checkpoint_optimizer_step=${expected_last_checkpoint_optimizer_step}
checkpoint_write_count=12
finite_parameter_check=1
nonfinite_output_count=0
runtime_result_present=false
completion_result_present=false
original_runner_lock_acquired_by_sealer=true
process_exit_status_claim=none
process_liveness_claim=none
external_orchestrator_observation=docker_engine_exit_255
external_observation_provenance=operator_observation_not_runtime_artifact
external_observation_is_model_evidence=false
resume_authorized=false
partial_artifact_reuse_authorized=false
retry_required=true
retry_schema_id=${retry_schema_id}
retry_result_schema_id=${retry_result_schema_id}
retry_runtime_root=${retry_runtime_root}
retry_runner_path=${script_dir}/run_mdn_train_isolated_v2_retry1.sh
retry_restart_from_step_zero=true
retry_expected_optimizer_steps=${expected_requested_optimizer_steps}
scientific_configuration_changed=false
canonical_data_raw_access=false
final_holdout_available=false
independent_final_evidence=false
policy_access=false
RECEIPT
}

verify_exact_receipt() {
  local inspected_path="${1:-${receipt_path}}"
  reject_symlink_components "${inspected_path}"
  require_file "${inspected_path}"
  [[ "$(stat -c '%h' -- "${inspected_path}")" == 1 ]] ||
    fail "interruption receipt candidate has an external hard link"

  local expected keys_expected keys_actual
  expected="$(mktemp /tmp/synthetic_v2_mdn_interruption.expected.XXXXXX)"
  keys_expected="$(mktemp /tmp/synthetic_v2_mdn_interruption.keys.expected.XXXXXX)"
  keys_actual="$(mktemp /tmp/synthetic_v2_mdn_interruption.keys.actual.XXXXXX)"
  emit_receipt "${expected}"
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || seen[key]++) exit 42;
      print key;
    }
  ' "${expected}" >"${keys_expected}" || {
    rm -f -- "${expected}" "${keys_expected}" "${keys_actual}"
    fail "internal expected interruption receipt keyset is invalid"
  }
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || seen[key]++) exit 42;
      print key;
    }
  ' "${inspected_path}" >"${keys_actual}" || {
    rm -f -- "${expected}" "${keys_expected}" "${keys_actual}"
    fail "interruption receipt contains a malformed or duplicate key"
  }
  cmp -s -- "${keys_expected}" "${keys_actual}" || {
    rm -f -- "${expected}" "${keys_expected}" "${keys_actual}"
    fail "interruption receipt keyset or key order drifted"
  }
  cmp -s -- "${expected}" "${inspected_path}" || {
    rm -f -- "${expected}" "${keys_expected}" "${keys_actual}"
    fail "interruption receipt content drifted"
  }
  rm -f -- "${expected}" "${keys_expected}" "${keys_actual}"
}

receipt_matches_expected() {
  local inspected_path="$1"
  local expected
  expected="$(mktemp /tmp/synthetic_v2_mdn_interruption.match.XXXXXX)"
  emit_receipt "${expected}"
  if cmp -s -- "${expected}" "${inspected_path}"; then
    rm -f -- "${expected}"
    return 0
  fi
  rm -f -- "${expected}"
  return 1
}

assert_sealed_modes() {
  local rel path
  for rel in "${!expected_payload_hashes[@]}"; do
    path="${runtime_root}/${rel}"
    [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
      fail "interrupted payload file is not mode 0444: ${rel}"
  done
  [[ "$(stat -c '%a' -- "${receipt_path}")" == 444 ]] ||
    fail "interruption receipt is not mode 0444"
  for rel in "${!expected_directories[@]}"; do
    if [[ "${rel}" == "." ]]; then
      path="${runtime_root}"
    else
      path="${runtime_root}/${rel}"
    fi
    [[ "$(stat -c '%a' -- "${path}")" == 555 ]] ||
      fail "interrupted payload directory is not mode 0555: ${rel}"
  done
}

seal_tree() {
  local rel
  for rel in "${!expected_payload_hashes[@]}"; do
    chmod 0444 -- "${runtime_root}/${rel}"
  done
  chmod 0444 -- "${receipt_path}"
  find "${runtime_root}" -depth -type d -exec chmod 0555 -- {} +
}

prepare_external_staging() {
  reject_symlink_components "${runtime_parent}"
  require_dir "${runtime_parent}"
  [[ "$(realpath -e -- "${runtime_parent}")" == "${runtime_parent}" ]] ||
    fail "runtime parent is not canonical"
  reject_symlink_components "${staging_path}"

  if [[ -e "${staging_path}" || -L "${staging_path}" ]]; then
    require_file "${staging_path}"
    [[ "$(stat -c '%h' -- "${staging_path}")" == 1 ]] ||
      fail "external interruption staging file has unexpected hard links"
    [[ "$(stat -c '%u' -- "${staging_path}")" == "$(id -u)" ]] ||
      fail "external interruption staging file has unexpected ownership"
    if receipt_matches_expected "${staging_path}"; then
      chmod 0444 -- "${staging_path}"
      verify_exact_receipt "${staging_path}"
      return
    fi
    chmod u+w -- "${staging_path}"
    rm -f -- "${staging_path}"
  fi

  if ! (set -o noclobber; emit_receipt "${staging_path}"); then
    if [[ -f "${staging_path}" && ! -L "${staging_path}" && \
      "$(stat -c '%h' -- "${staging_path}")" == 1 && \
      "$(stat -c '%u' -- "${staging_path}")" == "$(id -u)" ]]; then
      rm -f -- "${staging_path}"
    fi
    fail "could not create deterministic external interruption staging file"
  fi
  chmod 0444 -- "${staging_path}"
  verify_exact_receipt "${staging_path}"
}

recover_published_staging() {
  if [[ ! -e "${staging_path}" && ! -L "${staging_path}" ]]; then
    return
  fi
  reject_symlink_components "${staging_path}"
  require_file "${staging_path}"
  if [[ ! -e "${receipt_path}" && ! -L "${receipt_path}" ]]; then
    return
  fi
  reject_symlink_components "${receipt_path}"
  require_file "${receipt_path}"
  [[ "${staging_path}" -ef "${receipt_path}" ]] ||
    fail "published receipt and deterministic staging are not the same inode"
  receipt_matches_expected "${staging_path}" ||
    fail "published interruption staging content drifted"
  [[ "$(stat -c '%h' -- "${staging_path}")" == 2 ]] ||
    fail "published interruption staging has an unexpected link count"
  rm -f -- "${staging_path}"
}

publish_receipt() {
  [[ ! -e "${receipt_path}" && ! -L "${receipt_path}" ]] ||
    fail "refusing to overwrite existing interruption receipt"
  prepare_external_staging
  if ! ln -- "${staging_path}" "${receipt_path}"; then
    fail "atomic no-clobber interruption receipt publication failed"
  fi
  rm -f -- "${staging_path}"
  verify_exact_receipt "${receipt_path}"
}

acquire_original_runner_lock() {
  reject_symlink_components "${execution_lock_path}"
  require_file "${execution_lock_path}"
  exec {runner_lock_fd}<"${execution_lock_path}"
  flock -n "${runner_lock_fd}" ||
    fail "original MDN runner lock is held; refusing to seal or verify"
}

verify_sealed() {
  verify_interrupted_payload true
  verify_exact_receipt
  assert_sealed_modes
  echo "interruption_receipt=${receipt_path}"
  echo "interruption_receipt_sha256=$(sha256_of "${receipt_path}")"
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

acquire_original_runner_lock

if [[ "${mode}" == --verify ]]; then
  verify_sealed
  exit 0
fi

recover_published_staging

if [[ -e "${receipt_path}" || -L "${receipt_path}" ]]; then
  verify_interrupted_payload true
  verify_exact_receipt "${receipt_path}"
  seal_tree
  verify_sealed
  exit 0
fi

verify_interrupted_payload false
publish_receipt
seal_tree
verify_sealed
