#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_mdn_train_isolated_v2_retry1"
readonly result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"
readonly representation_result_schema_id="synthetic_v2_representation_train_isolated_v2.result.v1"
readonly source_schema_id="synthetic_v2_isolated_development_source_v1"
readonly interruption_schema_id="synthetic_v2_mdn_train_isolated_v2.interruption.v1"
readonly expected_original_runner_sha256="62defc3a15478c4aec1106c3a84105ef12aa1a65ec9bbafbc0669e96a62c2349"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_source_verifier_sha256="e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_cursor_erratum_receipt_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly expected_representation_runner_sha256="cb8cfcb926d40b7172fb744246db346d4da768c546c63b976d19a5e4801d8295"
readonly expected_representation_result_sha256="981971679e4c37ba23919aae549eab1bc1ea1c1452f1978c004802f4807dbd07"
readonly expected_representation_checkpoint_sha256="70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d"
readonly expected_interrupted_checkpoint_sha256="ab67674cefd9749b11b8302829489ad5cec8f6242abe3f0cb3e5b6bb4983bdc5"
readonly expected_mdn_objective_sha256="33b40ce2f6a76f0c9dddc67b9e3b162d1a171199b6d50b174dafaf854b135d5e"
readonly expected_source_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_alignment_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
# The receipt value deliberately prevents an official run until the independently
# authored recovery closure is published and its final hash replaces it.
readonly expected_recovery_amendment_sha256="a5b5a5dcda52c9e89d37f33db15678c5ceb67904f2c3ae187701123208573f8e"
readonly expected_interruption_sealer_sha256="824b6174f88d560210febb29f9b9db584a29de20da1d28c62fdde82a10269392"
readonly expected_interruption_receipt_sha256="57e73e038d61f9d3c60e79344e05995a8739f7b539dc604949b4ccb712aec1ea"
readonly expected_config_snapshot_sha256="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
readonly expected_steps=3500
readonly expected_seed=31
readonly train_begin=0
readonly train_end=2496
readonly accepted_anchor_count=3261
readonly expected_execution_chain="ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:train"
readonly expected_stream_plan="source(ujcamei.source.retrieval.graph_anchor:graph_anchor_edge_dataset_v1) -> nodelift(wikimyei.expression.nodelift.srl:nodelift_srl_v1) -> channel_representation(wikimyei.representation.encoding.mtf_jepa_mae_vicreg:mtf_jepa_mae_vicreg_v1) -> channel_inference(wikimyei.inference.expected_value.mdn:mdn_v1)"
readonly expected_objective_vector="learning_rate=0.001|max_steps=3500|batch=64|grad_clip=5|checkpoint_every=50|report_every=10|seed=31|mixtures=3|hidden=128|residual_depth=2|edge_aux=0|edge_aux_direction=0|edge_aux_rank=0|edge_aux_huber=0.01|edge_aux_logit_scale=50|direct_enabled=true|direct=100|direct_direction=5|direct_rank=5|direct_huber=0.5|direct_logit_scale=1|direct_target_scale=36|warmup_steps=800|warmup_nll=0|post_warmup_nll=1|warmup_direct_only=true|identity=edge_embedding_per_edge|base_edges=3|identity_dim=16|adapter_hidden=128"

fail() {
  echo "v2 isolated-source MDN retry1: $*" >&2
  exit 1
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

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_final_sha256() {
  local label="$1" value="$2"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "${label} is not finalized: ${value}"
}

kv() {
  local key="$1" path="$2" count value
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
  actual="$(kv "${key}" "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

verify_exact_kv_keys() {
  local path="$1"
  shift
  local expected actual
  expected="$(printf '%s\n' "$@" | LC_ALL=C sort)"
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
  ' "${path}" | LC_ALL=C sort)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: exact receipt keyset mismatch"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
source_closure="${source_runtime}/development_source_closure.status"
source_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
source_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_alignment_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
cursor_alignment_erratum_receipt="${source_runtime}/cursor_alignment_erratum.status"
mdn_objective="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
representation_runner="${script_dir}/run_representation_train_isolated_v2.sh"
representation_result="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/result.status"
representation_checkpoint="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
original_runner="${script_dir}/run_mdn_train_isolated_v2.sh"
interrupted_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2"
interrupted_input_receipt="${interrupted_runtime}/inputs.status"
interrupted_job="${interrupted_runtime}/job"
interrupted_manifest="${interrupted_job}/job.manifest"
interrupted_report="${interrupted_job}/channel_inference.report"
interrupted_log="${interrupted_runtime}/mdn_train.log"
interrupted_checkpoint="${interrupted_job}/channel_inference.report.channel_mdn.pt"
interrupted_runtime_result="${interrupted_job}/runtime.result.fact"
interrupted_completion_result="${interrupted_runtime}/result.status"
recovery_amendment="${benchmark_root}/MDN_ENGINE_INTERRUPTION_RECOVERY_AMENDMENT.md"
interruption_sealer="${script_dir}/seal_and_verify_interrupted_mdn_attempt_v2.sh"
interruption_receipt="${interrupted_runtime}/interruption.status"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
runtime_root="${runtime_parent}/${schema_id}"
config_snapshot="${runtime_root}/synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
job_dir="${runtime_root}/job"
log_path="${runtime_root}/mdn_train.retry1.log"
input_receipt="${runtime_root}/inputs.status"
result_receipt="${runtime_root}/result.status"

verify_fixed_directory() {
  local path="$1"
  reject_symlink_components "${path}"
  require_dir "${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "directory is not canonical: ${path}"
}

verify_source_hierarchy() {
  verify_fixed_directory "${runtime_parent}"
  verify_fixed_directory "${source_runtime}"
  verify_fixed_directory "${interrupted_runtime}"
  reject_symlink_components "${source_closure}"
  reject_symlink_components "${representation_result}"
  reject_symlink_components "${representation_checkpoint}"
  reject_symlink_components "${interruption_receipt}"
}

prepare_runtime_root() {
  local path
  verify_source_hierarchy
  reject_symlink_components "${runtime_root}"
  [[ ! -e "${runtime_root}" && ! -L "${runtime_root}" ]] ||
    fail "retry1 runtime already exists; overwrite or in-place recovery is forbidden"
  mkdir -- "${runtime_root}"
  verify_fixed_directory "${runtime_root}"
  for path in "${config_snapshot}" "${job_dir}" "${log_path}" \
    "${input_receipt}" "${result_receipt}" \
    "${runtime_root}/.execution.lock"; do
    reject_symlink_components "${path}"
  done
}

verify_runtime_hierarchy() {
  local path
  verify_source_hierarchy
  verify_fixed_directory "${runtime_root}"
  for path in "${config_snapshot}" "${job_dir}" "${log_path}" \
    "${input_receipt}" "${result_receipt}" \
    "${runtime_root}/.execution.lock"; do
    reject_symlink_components "${path}"
  done
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--run|--verify]"
case "${mode}" in
--plan | --run | --verify) ;;
*) fail "usage: $0 [--plan|--run|--verify]" ;;
esac

if [[ "${mode}" == --plan ]]; then
  cat <<PLAN
schema_id=${schema_id}.plan
source_closure_schema=${source_schema_id}
interruption_receipt_schema=${interruption_schema_id}
input_representation=synthetic_v2_representation_train_isolated_v2
input_representation_checkpoint_sha256=${expected_representation_checkpoint_sha256}
input_mdn_checkpoint=
forbidden_interrupted_checkpoint_sha256=${expected_interrupted_checkpoint_sha256}
accepted_anchor_count=${accepted_anchor_count}
maximum_available_anchor=3260
train_anchor_range=[${train_begin},${train_end})
optimizer_steps=${expected_steps}
seed=${expected_seed}
restart_from_step_zero=true
recovery_evidence_hashes_finalized=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
PLAN
  exit 0
fi

verify_source_receipts() {
  local manifest="$1"
  local isolated_source_root receipts receipt source instrument interval
  local edge_field instrument_field interval_field record_field source_field extra
  local count=0 key expected_source expected_receipt
  local -A seen=()
  isolated_source_root="$(kv isolated_source_root_path "${source_closure}")"
  isolated_source_root="$(realpath -e -- "${isolated_source_root}")"
  [[ "${isolated_source_root}" == "${source_runtime}/source" ]] ||
    fail "unexpected isolated source root: ${isolated_source_root}"
  receipts="$(kv source_file_receipts "${manifest}")"
  [[ -n "${receipts}" ]] || fail "job manifest omitted source_file_receipts"
  [[ "${receipts}" != *"/data/raw/"* ]] ||
    fail "job manifest references canonical data/raw"
  while IFS= read -r receipt; do
    [[ -n "${receipt}" ]] || continue
    IFS='|' read -r edge_field instrument_field interval_field record_field \
      source_field extra <<<"${receipt}"
    [[ -z "${extra:-}" && "${edge_field}" == edge=* && \
      "${instrument_field}" == instrument=* && \
      "${interval_field}" == interval=* && \
      "${record_field}" == record_type=kline && \
      "${source_field}" == source=* ]] ||
      fail "malformed source receipt: ${receipt}"
    instrument="${instrument_field#instrument=}"
    interval="${interval_field#interval=}"
    [[ "${edge_field#edge=}" == "${instrument}" ]] ||
      fail "source receipt edge/instrument mismatch: ${receipt}"
    case "${instrument}" in
    SYN2ALPHASYN2USD | SYN2BETASYN2USD | SYN2GAMMASYN2USD) ;;
    *) fail "unexpected source instrument: ${instrument}" ;;
    esac
    case "${interval}" in
    1d | 3d | 1w) ;;
    *) fail "unexpected source interval: ${interval}" ;;
    esac
    source="${source_field#source=}"
    expected_source="${isolated_source_root}/${instrument}/${interval}/${instrument}-${interval}-all-years.csv"
    expected_receipt="edge=${instrument}|instrument=${instrument}|interval=${interval}|record_type=kline|source=${expected_source}"
    [[ "${receipt}" == "${expected_receipt}" ]] ||
      fail "source receipt differs from exact isolated descriptor: ${receipt}"
    reject_symlink_components "${source}"
    require_file "${source}"
    [[ "$(realpath -e -- "${source}")" == "${source}" ]] ||
      fail "source receipt is not canonical: ${source}"
    key="${instrument}/${interval}"
    [[ -z "${seen[${key}]:-}" ]] || fail "duplicate source receipt: ${key}"
    seen["${key}"]=1
    ((count += 1))
  done < <(printf '%s\n' "${receipts}" | sed 's/;;/\n/g')
  [[ "${count}" == 9 ]] ||
    fail "expected nine isolated source receipts, found ${count}"
  for instrument in SYN2ALPHASYN2USD SYN2BETASYN2USD SYN2GAMMASYN2USD; do
    for interval in 1d 3d 1w; do
      [[ "${seen[${instrument}/${interval}]:-}" == 1 ]] ||
        fail "missing exact source receipt: ${instrument}/${interval}"
    done
  done
}

interruption_receipt_keys=(
  schema_id closure_status attempt_status evidence_class scientific_evidence
  metric_evidence_authorized interruption_sealer_path interruption_sealer_sha256
  interruption_amendment_path interruption_amendment_sha256
  interrupted_runtime_schema_id interrupted_runtime_root original_runner_path
  original_runner_sha256 input_receipt_path input_receipt_sha256
  config_snapshot_path config_snapshot_sha256 mdn_objective_path
  mdn_objective_sha256 runtime_exec_path runtime_exec_sha256
  isolated_source_closure_path isolated_source_closure_sha256
  cursor_alignment_erratum_receipt_path cursor_alignment_erratum_receipt_sha256
  representation_result_path representation_result_sha256
  representation_checkpoint_path representation_checkpoint_sha256
  execution_lock_path execution_lock_sha256 job_manifest_path
  job_manifest_sha256 mdn_report_path mdn_report_sha256
  partial_mdn_checkpoint_path partial_mdn_checkpoint_sha256 train_log_path
  train_log_sha256 runtime_event_probe_path runtime_event_probe_sha256
  representation_edge_probe_path representation_edge_probe_sha256
  mdn_lls_path mdn_lls_sha256 nodelift_lls_path nodelift_lls_sha256
  representation_lls_path representation_lls_sha256 runtime_layout_path
  runtime_layout_sha256 component_spawn_registry_path
  component_spawn_registry_sha256 component_spawn_ref_path
  component_spawn_ref_sha256 payload_directory_count
  payload_file_count_before_receipt sealed_file_count_including_receipt
  train_anchor_range accepted_anchor_count candidate_anchor_count
  maximum_available_anchor_index skipped_outside_common_range
  skipped_missing_edge_coverage skipped_failed_fetch_probe
  duplicate_anchor_count requested_optimizer_steps optimizer_steps_observed
  last_report_attempted_step report_write_count
  last_checkpoint_optimizer_step checkpoint_write_count finite_parameter_check
  nonfinite_output_count runtime_result_present completion_result_present
  original_runner_lock_acquired_by_sealer process_exit_status_claim
  process_liveness_claim external_orchestrator_observation
  external_observation_provenance external_observation_is_model_evidence
  resume_authorized partial_artifact_reuse_authorized retry_required
  retry_schema_id retry_result_schema_id retry_runtime_root retry_runner_path
  retry_restart_from_step_zero retry_expected_optimizer_steps
  scientific_configuration_changed canonical_data_raw_access
  final_holdout_available independent_final_evidence policy_access
)

verify_interruption_receipt() {
  require_file "${interruption_receipt}"
  [[ "$(stat -c '%A' -- "${interruption_receipt}")" != *w* ]] ||
    fail "interruption receipt is writable"
  [[ "$(sha256_of "${interruption_receipt}")" == \
    "${expected_interruption_receipt_sha256}" ]] ||
    fail "interruption receipt hash drifted"
  verify_exact_kv_keys "${interruption_receipt}" \
    "${interruption_receipt_keys[@]}"
  expect_kv "${interruption_receipt}" schema_id "${interruption_schema_id}"
  expect_kv "${interruption_receipt}" closure_status complete
  expect_kv "${interruption_receipt}" attempt_status interrupted
  expect_kv "${interruption_receipt}" evidence_class \
    operational_interruption_record
  expect_kv "${interruption_receipt}" scientific_evidence false
  expect_kv "${interruption_receipt}" metric_evidence_authorized false
  expect_kv "${interruption_receipt}" interruption_sealer_path \
    "${interruption_sealer}"
  expect_kv "${interruption_receipt}" interruption_sealer_sha256 \
    "${expected_interruption_sealer_sha256}"
  expect_kv "${interruption_receipt}" interruption_amendment_path \
    "${recovery_amendment}"
  expect_kv "${interruption_receipt}" interruption_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${interruption_receipt}" interrupted_runtime_schema_id \
    synthetic_v2_mdn_train_isolated_v2
  expect_kv "${interruption_receipt}" original_runner_path "${original_runner}"
  expect_kv "${interruption_receipt}" original_runner_sha256 \
    "${expected_original_runner_sha256}"
  expect_kv "${interruption_receipt}" interrupted_runtime_root \
    "${interrupted_runtime}"
  expect_kv "${interruption_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${interruption_receipt}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${interruption_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${interruption_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_receipt_sha256}"
  expect_kv "${interruption_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${interruption_receipt}" representation_result_sha256 \
    "${expected_representation_result_sha256}"
  expect_kv "${interruption_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${interruption_receipt}" partial_mdn_checkpoint_path \
    "${interrupted_checkpoint}"
  expect_kv "${interruption_receipt}" partial_mdn_checkpoint_sha256 \
    "${expected_interrupted_checkpoint_sha256}"
  expect_kv "${interruption_receipt}" representation_checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${interruption_receipt}" mdn_objective_path "${mdn_objective}"
  expect_kv "${interruption_receipt}" mdn_objective_sha256 \
    "${expected_mdn_objective_sha256}"
  expect_kv "${interruption_receipt}" train_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${interruption_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${interruption_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${interruption_receipt}" maximum_available_anchor_index 3260
  expect_kv "${interruption_receipt}" requested_optimizer_steps \
    "${expected_steps}"
  expect_kv "${interruption_receipt}" optimizer_steps_observed 620
  expect_kv "${interruption_receipt}" last_checkpoint_optimizer_step 600
  expect_kv "${interruption_receipt}" finite_parameter_check 1
  expect_kv "${interruption_receipt}" nonfinite_output_count 0
  expect_kv "${interruption_receipt}" runtime_result_present false
  expect_kv "${interruption_receipt}" completion_result_present false
  expect_kv "${interruption_receipt}" resume_authorized false
  expect_kv "${interruption_receipt}" partial_artifact_reuse_authorized false
  expect_kv "${interruption_receipt}" retry_required true
  expect_kv "${interruption_receipt}" retry_schema_id "${schema_id}"
  expect_kv "${interruption_receipt}" retry_result_schema_id \
    "${result_schema_id}"
  expect_kv "${interruption_receipt}" retry_runtime_root "${runtime_root}"
  expect_kv "${interruption_receipt}" retry_runner_path "${script_path}"
  expect_kv "${interruption_receipt}" retry_restart_from_step_zero true
  expect_kv "${interruption_receipt}" retry_expected_optimizer_steps \
    "${expected_steps}"
  expect_kv "${interruption_receipt}" scientific_configuration_changed false
  expect_kv "${interruption_receipt}" canonical_data_raw_access false
  expect_kv "${interruption_receipt}" final_holdout_available false
  expect_kv "${interruption_receipt}" independent_final_evidence false
  expect_kv "${interruption_receipt}" policy_access false
}

verify_static_inputs() {
  local path expected
  require_final_sha256 recovery_amendment \
    "${expected_recovery_amendment_sha256}"
  require_final_sha256 interruption_sealer \
    "${expected_interruption_sealer_sha256}"
  require_final_sha256 interruption_receipt \
    "${expected_interruption_receipt_sha256}"
  verify_source_hierarchy
  for path in "${source_closure}" "${source_verifier}" "${source_amendment}" \
    "${source_protocol}" "${staged_hardening}" \
    "${cursor_alignment_correction}" \
    "${cursor_alignment_metadata_erratum}" \
    "${cursor_alignment_erratum_receipt}" "${representation_runner}" \
    "${representation_result}" "${representation_checkpoint}" \
    "${mdn_objective}" \
    "${original_runner}" "${recovery_amendment}" \
    "${interruption_sealer}" "${interruption_receipt}" \
    "${interrupted_input_receipt}" "${interrupted_manifest}" \
    "${interrupted_report}" "${interrupted_log}" \
    "${interrupted_checkpoint}" "${runtime_exec}"; do
    require_file "${path}"
  done
  while IFS='|' read -r path expected; do
    [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
      fail "fixed input hash drifted: ${path}"
  done <<HASHES
${runtime_exec}|${expected_runtime_exec_sha256}
${source_verifier}|${expected_source_verifier_sha256}
${source_closure}|${expected_source_closure_sha256}
${source_amendment}|${expected_source_amendment_sha256}
${source_protocol}|${expected_source_protocol_sha256}
${staged_hardening}|${expected_staged_hardening_sha256}
${cursor_alignment_correction}|${expected_cursor_alignment_correction_sha256}
${cursor_alignment_metadata_erratum}|${expected_cursor_alignment_metadata_erratum_sha256}
${cursor_alignment_erratum_receipt}|${expected_cursor_erratum_receipt_sha256}
${representation_runner}|${expected_representation_runner_sha256}
${representation_result}|${expected_representation_result_sha256}
${representation_checkpoint}|${expected_representation_checkpoint_sha256}
${mdn_objective}|${expected_mdn_objective_sha256}
${original_runner}|${expected_original_runner_sha256}
${recovery_amendment}|${expected_recovery_amendment_sha256}
${interruption_sealer}|${expected_interruption_sealer_sha256}
${interruption_receipt}|${expected_interruption_receipt_sha256}
${interrupted_checkpoint}|${expected_interrupted_checkpoint_sha256}
HASHES
  "${source_verifier}" --verify >/dev/null
  "${representation_runner}" --verify >/dev/null
  "${interruption_sealer}" --verify >/dev/null
  verify_interruption_receipt
  [[ ! -e "${interrupted_runtime_result}" && \
    ! -L "${interrupted_runtime_result}" ]] ||
    fail "interrupted attempt unexpectedly has a runtime completion result"
  [[ ! -e "${interrupted_completion_result}" && \
    ! -L "${interrupted_completion_result}" ]] ||
    fail "interrupted attempt unexpectedly has a published completion result"
  expect_kv "${source_closure}" schema_id "${source_schema_id}"
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_closure}" maximum_anchor_index 3260
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false
  expect_kv "${cursor_alignment_erratum_receipt}" schema_id \
    synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1
  expect_kv "${cursor_alignment_erratum_receipt}" status complete
  expect_kv "${cursor_alignment_erratum_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${cursor_alignment_erratum_receipt}" maximum_anchor_index 3260
  [[ "$(stat -c '%A' -- "${interrupted_checkpoint}")" != *w* ]] ||
    fail "forbidden partial MDN checkpoint remains writable"
}

isolated_base_config_path() {
  local config registry active_root_count
  config="$(kv isolated_base_config_path "${source_closure}")"
  reject_symlink_components "${config}"
  require_file "${config}"
  expect_kv "${source_closure}" isolated_base_config_sha256 \
    "$(sha256_of "${config}")"
  [[ "${config}" == \
    "${source_runtime}/config/synthetic_benchmark.isolated_development.config" ]] ||
    fail "unexpected isolated base config: ${config}"
  registry="$(kv ujcamei_source_registry_dsl_path "${config}")"
  reject_symlink_components "${registry}"
  expect_kv "${source_closure}" isolated_registry_path "${registry}"
  require_file "${registry}"
  expect_kv "${source_closure}" isolated_registry_sha256 \
    "$(sha256_of "${registry}")"
  expect_kv "${registry}" SOURCE_ROOT "${source_runtime}/source;"
  active_root_count="$(awk '
    /^[[:space:]]*SOURCE_ROOT[[:space:]]*=/ { ++count }
    END { print count + 0 }
  ' "${registry}")"
  [[ "${active_root_count}" == 1 ]] ||
    fail "isolated registry must contain exactly one active SOURCE_ROOT"
  expect_kv "${config}" runtime_wave_id policy_training_ppo_v0
  expect_kv "${config}" wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${mdn_objective}"
  printf '%s' "${config}"
}

verify_representation_input() {
  require_file "${representation_result}"
  require_file "${representation_checkpoint}"
  [[ "$(stat -c '%A' -- "${representation_result}")" != *w* ]] ||
    fail "clean representation result receipt is writable"
  [[ "$(stat -c '%A' -- "${representation_checkpoint}")" != *w* ]] ||
    fail "clean representation checkpoint is writable"
  expect_kv "${representation_result}" schema_id \
    "${representation_result_schema_id}"
  expect_kv "${representation_result}" status complete
  expect_kv "${representation_result}" checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${representation_result}" checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${representation_result}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${representation_result}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${representation_result}" restart_from_step_zero true
  expect_kv "${representation_result}" quarantined_checkpoint_access false
  expect_kv "${representation_result}" canonical_data_raw_access false
  [[ "$(sha256_of "${representation_checkpoint}")" == \
    "${expected_representation_checkpoint_sha256}" ]] ||
    fail "clean representation checkpoint hash drifted"
  [[ "${representation_checkpoint}" != "${interrupted_checkpoint}" ]] ||
    fail "representation input aliases the forbidden partial MDN checkpoint"
}

derive_config_snapshot() {
  local base_config="$1"
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= train_core_channel_mdn");
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${base_config}"
}

verify_config_snapshot() {
  local base_config="$1"
  reject_symlink_components "${config_snapshot}"
  require_file "${config_snapshot}"
  [[ "$(stat -c '%A' -- "${config_snapshot}")" != *w* ]] ||
    fail "isolated retry1 MDN config is writable"
  cmp -s -- <(derive_config_snapshot "${base_config}") "${config_snapshot}" ||
    fail "isolated retry1 MDN config is not the fixed base derivation"
  [[ "$(sha256_of "${config_snapshot}")" == \
    "${expected_config_snapshot_sha256}" ]] ||
    fail "isolated retry1 MDN config hash drifted"
  expect_kv "${config_snapshot}" runtime_wave_id train_core_channel_mdn
  expect_kv "${config_snapshot}" ujcamei_source_registry_dsl_path \
    "${source_runtime}/config/ujcamei.source.registry.development_prefix.dsl"
}

write_config_snapshot() {
  local base_config="$1" candidate
  candidate="$(mktemp "${runtime_root}/.config.XXXXXX")"
  derive_config_snapshot "${base_config}" >"${candidate}" ||
    fail "could not derive isolated retry1 MDN config"
  chmod 0444 "${candidate}"
  if [[ -e "${config_snapshot}" ]]; then
    cmp -s -- "${candidate}" "${config_snapshot}" ||
      fail "immutable isolated retry1 MDN config drifted"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${config_snapshot}" ||
      fail "isolated retry1 MDN config publication failed"
    rm -f -- "${candidate}"
  fi
  verify_config_snapshot "${base_config}"
}

emit_inputs() {
  local destination="$1"
  cat >"${destination}" <<INPUTS
schema_id=${schema_id}.inputs.v1
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
original_runner_path=${original_runner}
original_runner_sha256=${expected_original_runner_sha256}
recovery_amendment_path=${recovery_amendment}
recovery_amendment_sha256=${expected_recovery_amendment_sha256}
interruption_sealer_path=${interruption_sealer}
interruption_sealer_sha256=${expected_interruption_sealer_sha256}
interruption_receipt_path=${interruption_receipt}
interruption_receipt_sha256=${expected_interruption_receipt_sha256}
interrupted_runtime_path=${interrupted_runtime}
isolated_source_verifier_path=${source_verifier}
isolated_source_verifier_sha256=${expected_source_verifier_sha256}
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=${expected_source_closure_sha256}
source_isolation_amendment_sha256=${expected_source_amendment_sha256}
isolated_source_protocol_sha256=${expected_source_protocol_sha256}
staged_hardening_amendment_sha256=${expected_staged_hardening_sha256}
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=${expected_cursor_alignment_correction_sha256}
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=${expected_cursor_alignment_metadata_erratum_sha256}
cursor_alignment_erratum_receipt_path=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_erratum_receipt_sha256}
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=${expected_config_snapshot_sha256}
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=${expected_runtime_exec_sha256}
representation_runner_path=${representation_runner}
representation_runner_sha256=${expected_representation_runner_sha256}
representation_result_path=${representation_result}
representation_result_sha256=${expected_representation_result_sha256}
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=${expected_representation_checkpoint_sha256}
mdn_objective_path=${mdn_objective}
mdn_objective_sha256=${expected_mdn_objective_sha256}
input_mdn_checkpoint_path=
forbidden_interrupted_mdn_checkpoint_path=${interrupted_checkpoint}
forbidden_interrupted_mdn_checkpoint_sha256=${expected_interrupted_checkpoint_sha256}
train_anchor_begin=${train_begin}
train_anchor_end_exclusive=${train_end}
expected_optimizer_steps=${expected_steps}
expected_seed=${expected_seed}
expected_accepted_anchor_count=${accepted_anchor_count}
expected_candidate_anchor_count=${accepted_anchor_count}
expected_maximum_available_anchor_index=3260
expected_execution_chain=${expected_execution_chain}
expected_stream_plan=${expected_stream_plan}
expected_objective_vector=${expected_objective_vector}
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
final_holdout_available=false
policy_access=false
INPUTS
}

input_receipt_keys=(
  schema_id runner_path runner_sha256 original_runner_path original_runner_sha256
  recovery_amendment_path recovery_amendment_sha256 interruption_sealer_path
  interruption_sealer_sha256 interruption_receipt_path
  interruption_receipt_sha256 interrupted_runtime_path
  isolated_source_verifier_path isolated_source_verifier_sha256
  isolated_source_closure_path isolated_source_closure_sha256
  source_isolation_amendment_sha256 isolated_source_protocol_sha256
  staged_hardening_amendment_sha256 cursor_alignment_correction_path
  cursor_alignment_correction_sha256 cursor_alignment_metadata_erratum_path
  cursor_alignment_metadata_erratum_sha256 cursor_alignment_erratum_receipt_path
  cursor_alignment_erratum_receipt_sha256 config_snapshot_path
  config_snapshot_sha256 runtime_exec_path runtime_exec_sha256
  representation_runner_path representation_runner_sha256
  representation_result_path representation_result_sha256
  representation_checkpoint_path representation_checkpoint_sha256
  mdn_objective_path mdn_objective_sha256
  input_mdn_checkpoint_path forbidden_interrupted_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_sha256 train_anchor_begin
  train_anchor_end_exclusive expected_optimizer_steps expected_seed
  expected_accepted_anchor_count expected_candidate_anchor_count
  expected_maximum_available_anchor_index expected_execution_chain
  expected_stream_plan expected_objective_vector restart_from_step_zero
  quarantined_checkpoint_access canonical_data_raw_access
  final_holdout_available policy_access
)

verify_input_receipt() {
  require_file "${input_receipt}"
  [[ "$(stat -c '%A' -- "${input_receipt}")" != *w* ]] ||
    fail "input receipt is writable"
  verify_exact_kv_keys "${input_receipt}" "${input_receipt_keys[@]}"
  expect_kv "${input_receipt}" schema_id "${schema_id}.inputs.v1"
  expect_kv "${input_receipt}" runner_path "${script_path}"
  expect_kv "${input_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${input_receipt}" original_runner_path "${original_runner}"
  expect_kv "${input_receipt}" original_runner_sha256 \
    "${expected_original_runner_sha256}"
  expect_kv "${input_receipt}" recovery_amendment_path \
    "${recovery_amendment}"
  expect_kv "${input_receipt}" recovery_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${input_receipt}" interruption_sealer_path \
    "${interruption_sealer}"
  expect_kv "${input_receipt}" interruption_sealer_sha256 \
    "${expected_interruption_sealer_sha256}"
  expect_kv "${input_receipt}" interruption_receipt_path \
    "${interruption_receipt}"
  expect_kv "${input_receipt}" interruption_receipt_sha256 \
    "${expected_interruption_receipt_sha256}"
  expect_kv "${input_receipt}" interrupted_runtime_path \
    "${interrupted_runtime}"
  expect_kv "${input_receipt}" isolated_source_verifier_path \
    "${source_verifier}"
  expect_kv "${input_receipt}" isolated_source_verifier_sha256 \
    "${expected_source_verifier_sha256}"
  expect_kv "${input_receipt}" isolated_source_closure_path "${source_closure}"
  expect_kv "${input_receipt}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${input_receipt}" source_isolation_amendment_sha256 \
    "${expected_source_amendment_sha256}"
  expect_kv "${input_receipt}" isolated_source_protocol_sha256 \
    "${expected_source_protocol_sha256}"
  expect_kv "${input_receipt}" staged_hardening_amendment_sha256 \
    "${expected_staged_hardening_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${input_receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_receipt_sha256}"
  expect_kv "${input_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${input_receipt}" config_snapshot_sha256 \
    "${expected_config_snapshot_sha256}"
  expect_kv "${input_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${input_receipt}" runtime_exec_sha256 \
    "${expected_runtime_exec_sha256}"
  expect_kv "${input_receipt}" representation_runner_path \
    "${representation_runner}"
  expect_kv "${input_receipt}" representation_runner_sha256 \
    "${expected_representation_runner_sha256}"
  expect_kv "${input_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${input_receipt}" representation_result_sha256 \
    "${expected_representation_result_sha256}"
  expect_kv "${input_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${input_receipt}" representation_checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${input_receipt}" mdn_objective_path "${mdn_objective}"
  expect_kv "${input_receipt}" mdn_objective_sha256 \
    "${expected_mdn_objective_sha256}"
  expect_kv "${input_receipt}" input_mdn_checkpoint_path ''
  expect_kv "${input_receipt}" forbidden_interrupted_mdn_checkpoint_path \
    "${interrupted_checkpoint}"
  expect_kv "${input_receipt}" forbidden_interrupted_mdn_checkpoint_sha256 \
    "${expected_interrupted_checkpoint_sha256}"
  expect_kv "${input_receipt}" train_anchor_begin "${train_begin}"
  expect_kv "${input_receipt}" train_anchor_end_exclusive "${train_end}"
  expect_kv "${input_receipt}" expected_optimizer_steps "${expected_steps}"
  expect_kv "${input_receipt}" expected_seed "${expected_seed}"
  expect_kv "${input_receipt}" expected_accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${input_receipt}" expected_candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${input_receipt}" expected_maximum_available_anchor_index 3260
  expect_kv "${input_receipt}" expected_execution_chain \
    "${expected_execution_chain}"
  expect_kv "${input_receipt}" expected_stream_plan "${expected_stream_plan}"
  expect_kv "${input_receipt}" expected_objective_vector \
    "${expected_objective_vector}"
  expect_kv "${input_receipt}" restart_from_step_zero true
  expect_kv "${input_receipt}" quarantined_checkpoint_access false
  expect_kv "${input_receipt}" canonical_data_raw_access false
  expect_kv "${input_receipt}" final_holdout_available false
  expect_kv "${input_receipt}" policy_access false
}

verify_objective_report() {
  local report="$1"
  expect_kv "${report}" effective_batch_size 64
  expect_kv "${report}" batch_size_source derived
  expect_kv "${report}" checkpoint_every 50
  expect_kv "${report}" report_every 10
  expect_kv "${report}" validation_every 0
  expect_kv "${report}" mixture_count 3
  expect_kv "${report}" hidden_width 128
  expect_kv "${report}" residual_depth 2
  expect_kv "${report}" edge_return_auxiliary_loss_weight 0
  expect_kv "${report}" edge_return_auxiliary_direction_weight 0
  expect_kv "${report}" edge_return_auxiliary_rank_weight 0
  expect_kv "${report}" edge_return_auxiliary_huber_beta 0.01
  expect_kv "${report}" edge_return_auxiliary_logit_scale 50
  expect_kv "${report}" direct_edge_return_readout_enabled true
  expect_kv "${report}" direct_edge_return_readout_loss_weight 100
  expect_kv "${report}" direct_edge_return_readout_direction_weight 5
  expect_kv "${report}" direct_edge_return_readout_rank_weight 5
  expect_kv "${report}" direct_edge_return_readout_huber_beta 0.5
  expect_kv "${report}" direct_edge_return_readout_logit_scale 1
  expect_kv "${report}" direct_edge_return_readout_target_scale 36
  expect_kv "${report}" direct_edge_return_readout_warmup_steps 800
  expect_kv "${report}" direct_edge_return_readout_warmup_nll_weight 0
  expect_kv "${report}" direct_edge_return_readout_post_warmup_nll_weight 1
  expect_kv "${report}" direct_edge_return_readout_warmup_direct_head_only true
  expect_kv "${report}" direct_edge_return_readout_identity_mode \
    edge_embedding_per_edge
  expect_kv "${report}" direct_edge_return_readout_base_edge_count 3
  expect_kv "${report}" direct_edge_return_readout_identity_embedding_dim 16
  expect_kv "${report}" direct_edge_return_readout_adapter_hidden_dim 128
}

assert_exact_job_tree() {
  local actual expected
  actual="$(find "${job_dir}" -mindepth 1 -maxdepth 1 -printf '%f\n' | \
    LC_ALL=C sort)"
  expected="$(printf '%s\n' \
    channel_inference.report \
    channel_inference.report.channel_mdn.pt \
    channel_inference.report.mdn.lls \
    channel_inference.report.nodelift.lls \
    channel_inference.report.representation.lls \
    job.manifest \
    representation_edge_features.probe \
    runtime.job_events.probe \
    runtime.result.fact | LC_ALL=C sort)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "retry1 job has an unexpected output entry"
}

validate_job() {
  local result="${job_dir}/runtime.result.fact"
  local manifest="${job_dir}/job.manifest"
  local report="${job_dir}/channel_inference.report"
  local checkpoint="${job_dir}/channel_inference.report.channel_mdn.pt"
  local discovered_checkpoint checkpoint_count path
  verify_fixed_directory "${job_dir}"
  for path in "${result}" "${manifest}" "${report}" "${checkpoint}" \
    "${log_path}"; do
    reject_symlink_components "${path}"
    require_file "${path}"
  done
  assert_exact_job_tree
  expect_kv "${result}" status completed
  expect_kv "${result}" wave_id train_core_channel_mdn
  expect_kv "${result}" wave_action train
  expect_kv "${result}" job_id train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${result}" job_kind channel_inference_mdn
  expect_kv "${result}" target_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${result}" source_report_path "${report}"
  expect_kv "${result}" optimizer_steps "${expected_steps}"
  expect_kv "${result}" checkpoint_path "${checkpoint}"
  expect_kv "${result}" checkpoint_written true
  expect_kv "${result}" model_state_mutated true
  expect_kv "${result}" finite_parameter_check true
  expect_kv "${result}" nonfinite_output_count 0
  expect_kv "${manifest}" config_path "${config_snapshot}"
  expect_kv "${manifest}" job_id train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${manifest}" job_kind channel_inference_mdn
  expect_kv "${manifest}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest}" component_spawn_fingerprint 5ba58d2de0fb7dcb
  expect_kv "${manifest}" component_operator_surface_digest 6a65a1724dce0110
  expect_kv "${manifest}" mdn_assembly_fingerprint fe0bb67764d8ab24
  expect_kv "${manifest}" wave_id train_core_channel_mdn
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" execution_chain "${expected_execution_chain}"
  expect_kv "${manifest}" mutated_components \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" source_order_policy random_per_epoch
  expect_kv "${manifest}" source_order_policy_explicit true
  expect_kv "${manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${train_end}"
  expect_kv "${manifest}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" skipped_outside_common_range 0
  expect_kv "${manifest}" skipped_missing_edge_coverage 0
  expect_kv "${manifest}" skipped_failed_fetch_probe 0
  expect_kv "${manifest}" duplicate_anchor_count 0
  expect_kv "${manifest}" stream_plan "${expected_stream_plan}"
  expect_kv "${manifest}" inference_training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  expect_kv "${manifest}" runtime_handoff_id ''
  expect_kv "${manifest}" runtime_handoff_digest ''
  expect_kv "${manifest}" policy_training_contract_schema ''
  expect_kv "${manifest}" policy_training_contract_digest ''
  expect_kv "${manifest}" policy_training_artifact_schema ''
  expect_kv "${manifest}" policy_operator_surface_contract_id ''
  expect_kv "${manifest}" policy_operator_surface_digest ''
  expect_kv "${manifest}" policy_family_id ''
  expect_kv "${manifest}" policy_dsl_digest ''
  expect_kv "${manifest}" policy_net_digest ''
  expect_kv "${manifest}" policy_input_feature_manifest_digest ''
  expect_kv "${manifest}" policy_jkimyei_digest ''
  expect_kv "${manifest}" target_node_universe_digest ''
  expect_kv "${manifest}" action_distribution_id ''
  expect_kv "${manifest}" action_distribution_config_digest ''
  expect_kv "${manifest}" reward_contract_id ''
  expect_kv "${manifest}" execution_profile_digest ''
  expect_kv "${manifest}" causal_schedule_digest ''
  expect_kv "${report}" training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${report}" config_path "${config_snapshot}"
  expect_kv "${report}" component_assembly_id mdn_v1
  expect_kv "${report}" input_representation_assembly_id \
    mtf_jepa_mae_vicreg_v1
  expect_kv "${report}" context_contract \
    graph_order.channel_node_representation.v1
  expect_kv "${report}" output_contract \
    graph_order.channel_node_future_distribution.v1
  expect_kv "${report}" channel_count 3
  expect_kv "${report}" context_dim 32
  expect_kv "${report}" future_horizon 1
  expect_kv "${report}" context_mode channel_context_strict
  expect_kv "${report}" target_domain channel_node_future
  expect_kv "${report}" target_mask_policy per_target_feature_valid
  expect_kv "${report}" activity_target node_feature_support_mean
  expect_kv "${report}" feature_embedding_dim 8
  expect_kv "${report}" channel_adapter_rank 16
  expect_kv "${report}" mdn_architecture \
    shared_slot_trunk.channel_adapter.shared_feature_head.direct_edge_readout.edge_embedding_per_edge.v4
  expect_kv "${report}" loss_reduction balanced_channel_feature_mean
  expect_kv "${report}" shared_trunk true
  expect_kv "${report}" channel_adapters_enabled true
  expect_kv "${report}" shared_feature_head true
  expect_kv "${report}" feature_embedding_enabled true
  expect_kv "${report}" node_id_embedding false
  expect_kv "${report}" cross_node_attention false
  expect_kv "${report}" cross_channel_attention false
  expect_kv "${report}" sigma_min 0.001
  expect_kv "${report}" sigma_max 0
  expect_kv "${report}" eps 1e-06
  expect_kv "${report}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${report}" representation_checkpoint_loaded true
  expect_kv "${report}" mdn_checkpoint_path ''
  expect_kv "${report}" mdn_checkpoint_loaded false
  expect_kv "${report}" stream_plan "${expected_stream_plan}"
  expect_kv "${report}" seed "${expected_seed}"
  expect_kv "${report}" seed_scope torch_manual_seed_cuda_manual_seed_all
  expect_kv "${report}" allow_untrained_representation false
  expect_kv "${report}" requested_anchor_index_begin "${train_begin}"
  expect_kv "${report}" requested_anchor_index_end "${train_end}"
  expect_kv "${report}" steps_attempted "${expected_steps}"
  expect_kv "${report}" steps_completed "${expected_steps}"
  expect_kv "${report}" skipped_batches 0
  expect_kv "${report}" optimizer_steps "${expected_steps}"
  expect_kv "${report}" finite_parameter_check 1
  expect_kv "${report}" nonfinite_output_count 0
  expect_kv "${report}" checkpoint_written true
  expect_kv "${report}" checkpoint_path "${checkpoint}"
  verify_objective_report "${report}"
  verify_source_receipts "${manifest}"
  [[ "$(realpath -e -- "${checkpoint}")" == "${checkpoint}" ]] ||
    fail "retry1 MDN checkpoint is not canonical"
  [[ "${checkpoint}" != "${interrupted_checkpoint}" ]] ||
    fail "retry1 output aliases the forbidden partial MDN checkpoint"
  [[ "$(sha256_of "${checkpoint}")" != \
    "${expected_interrupted_checkpoint_sha256}" ]] ||
    fail "retry1 output equals the forbidden partial MDN checkpoint"
  checkpoint_count="$(find "${job_dir}" -type f -name '*.pt' -printf '%p\n' | wc -l)"
  [[ "${checkpoint_count}" == 1 ]] ||
    fail "retry1 job must contain exactly one checkpoint, found ${checkpoint_count}"
  discovered_checkpoint="$(find "${job_dir}" -type f -name '*.pt' -print -quit)"
  [[ "${discovered_checkpoint}" == "${checkpoint}" ]] ||
    fail "retry1 job checkpoint escaped exact output containment"
  printf '%s' "${checkpoint}"
}

seal_job_artifacts() {
  local symlink special hardlinked
  verify_fixed_directory "${job_dir}"
  symlink="$(find "${job_dir}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "training job contains a symlink: ${symlink}"
  special="$(find "${job_dir}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "training job contains a special entry: ${special}"
  hardlinked="$(find "${job_dir}" -type f ! -links 1 -print -quit)"
  [[ -z "${hardlinked}" ]] ||
    fail "training job contains an externally mutable hard link: ${hardlinked}"
  find "${job_dir}" -type f -exec chmod 0444 -- {} +
  find "${job_dir}" -depth -type d -exec chmod 0555 -- {} +
  chmod 0444 -- "${log_path}"
}

assert_job_artifacts_sealed() {
  local writable hardlinked
  writable="$(find "${job_dir}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "training job artifact remains writable: ${writable}"
  hardlinked="$(find "${job_dir}" -type f ! -links 1 -print -quit)"
  [[ -z "${hardlinked}" ]] ||
    fail "sealed training job has an external hard link: ${hardlinked}"
  [[ "$(stat -c '%A' -- "${log_path}")" != *w* ]] ||
    fail "retry1 MDN training log remains writable"
}

write_result_receipt() {
  local checkpoint="$1" candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  cat >"${candidate}" <<RESULT
schema_id=${result_schema_id}
status=complete
job_dir=${job_dir}
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
original_runner_path=${original_runner}
original_runner_sha256=${expected_original_runner_sha256}
recovery_amendment_path=${recovery_amendment}
recovery_amendment_sha256=${expected_recovery_amendment_sha256}
interruption_sealer_path=${interruption_sealer}
interruption_sealer_sha256=${expected_interruption_sealer_sha256}
interruption_receipt_path=${interruption_receipt}
interruption_receipt_sha256=${expected_interruption_receipt_sha256}
interrupted_runtime_path=${interrupted_runtime}
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=${expected_source_closure_sha256}
cursor_alignment_erratum_receipt_path=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_erratum_receipt_sha256}
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=${expected_config_snapshot_sha256}
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=${expected_runtime_exec_sha256}
representation_result_path=${representation_result}
representation_result_sha256=${expected_representation_result_sha256}
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=${expected_representation_checkpoint_sha256}
mdn_objective_path=${mdn_objective}
mdn_objective_sha256=${expected_mdn_objective_sha256}
input_mdn_checkpoint_path=
forbidden_interrupted_mdn_checkpoint_path=${interrupted_checkpoint}
forbidden_interrupted_mdn_checkpoint_sha256=${expected_interrupted_checkpoint_sha256}
checkpoint_path=${checkpoint}
checkpoint_sha256=$(sha256_of "${checkpoint}")
job_manifest_path=${job_dir}/job.manifest
job_manifest_sha256=$(sha256_of "${job_dir}/job.manifest")
runtime_result_path=${job_dir}/runtime.result.fact
runtime_result_sha256=$(sha256_of "${job_dir}/runtime.result.fact")
mdn_report_path=${job_dir}/channel_inference.report
mdn_report_sha256=$(sha256_of "${job_dir}/channel_inference.report")
train_log_path=${log_path}
train_log_sha256=$(sha256_of "${log_path}")
train_anchor_range=[${train_begin},${train_end})
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor_index=3260
optimizer_steps=${expected_steps}
seed=${expected_seed}
execution_chain=${expected_execution_chain}
stream_plan=${expected_stream_plan}
objective_vector=${expected_objective_vector}
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
final_holdout_available=false
policy_access=false
RESULT
  chmod 0444 "${candidate}"
  if [[ -e "${result_receipt}" ]]; then
    cmp -s -- "${candidate}" "${result_receipt}" ||
      fail "immutable retry1 result receipt drifted"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${result_receipt}" ||
      fail "retry1 result receipt publication failed"
    rm -f -- "${candidate}"
  fi
}

result_receipt_keys=(
  schema_id status job_dir runner_path runner_sha256 input_receipt_path
  input_receipt_sha256 original_runner_path original_runner_sha256
  recovery_amendment_path recovery_amendment_sha256 interruption_sealer_path
  interruption_sealer_sha256 interruption_receipt_path
  interruption_receipt_sha256 interrupted_runtime_path
  isolated_source_closure_path isolated_source_closure_sha256
  cursor_alignment_erratum_receipt_path cursor_alignment_erratum_receipt_sha256
  config_snapshot_path config_snapshot_sha256 runtime_exec_path
  runtime_exec_sha256 representation_result_path representation_result_sha256
  representation_checkpoint_path representation_checkpoint_sha256
  mdn_objective_path mdn_objective_sha256
  input_mdn_checkpoint_path forbidden_interrupted_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_sha256 checkpoint_path checkpoint_sha256
  job_manifest_path job_manifest_sha256 runtime_result_path
  runtime_result_sha256 mdn_report_path mdn_report_sha256 train_log_path
  train_log_sha256 train_anchor_range accepted_anchor_count
  candidate_anchor_count maximum_available_anchor_index optimizer_steps seed
  execution_chain stream_plan objective_vector restart_from_step_zero
  quarantined_checkpoint_access canonical_data_raw_access
  final_holdout_available policy_access
)

verify_result_receipt() {
  local checkpoint="$1"
  require_file "${result_receipt}"
  [[ "$(stat -c '%A' -- "${result_receipt}")" != *w* ]] ||
    fail "result receipt is writable"
  verify_exact_kv_keys "${result_receipt}" "${result_receipt_keys[@]}"
  expect_kv "${result_receipt}" schema_id "${result_schema_id}"
  expect_kv "${result_receipt}" status complete
  expect_kv "${result_receipt}" job_dir "${job_dir}"
  expect_kv "${result_receipt}" runner_path "${script_path}"
  expect_kv "${result_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${result_receipt}" input_receipt_path "${input_receipt}"
  expect_kv "${result_receipt}" input_receipt_sha256 \
    "$(sha256_of "${input_receipt}")"
  expect_kv "${result_receipt}" original_runner_path "${original_runner}"
  expect_kv "${result_receipt}" original_runner_sha256 \
    "${expected_original_runner_sha256}"
  expect_kv "${result_receipt}" recovery_amendment_path \
    "${recovery_amendment}"
  expect_kv "${result_receipt}" recovery_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${result_receipt}" interruption_sealer_path \
    "${interruption_sealer}"
  expect_kv "${result_receipt}" interruption_sealer_sha256 \
    "${expected_interruption_sealer_sha256}"
  expect_kv "${result_receipt}" interruption_receipt_path \
    "${interruption_receipt}"
  expect_kv "${result_receipt}" interruption_receipt_sha256 \
    "${expected_interruption_receipt_sha256}"
  expect_kv "${result_receipt}" interrupted_runtime_path \
    "${interrupted_runtime}"
  expect_kv "${result_receipt}" isolated_source_closure_path "${source_closure}"
  expect_kv "${result_receipt}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${result_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${result_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_receipt_sha256}"
  expect_kv "${result_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${result_receipt}" config_snapshot_sha256 \
    "${expected_config_snapshot_sha256}"
  expect_kv "${result_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${result_receipt}" runtime_exec_sha256 \
    "${expected_runtime_exec_sha256}"
  expect_kv "${result_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${result_receipt}" representation_result_sha256 \
    "${expected_representation_result_sha256}"
  expect_kv "${result_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${result_receipt}" representation_checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${result_receipt}" mdn_objective_path "${mdn_objective}"
  expect_kv "${result_receipt}" mdn_objective_sha256 \
    "${expected_mdn_objective_sha256}"
  expect_kv "${result_receipt}" input_mdn_checkpoint_path ''
  expect_kv "${result_receipt}" forbidden_interrupted_mdn_checkpoint_path \
    "${interrupted_checkpoint}"
  expect_kv "${result_receipt}" forbidden_interrupted_mdn_checkpoint_sha256 \
    "${expected_interrupted_checkpoint_sha256}"
  expect_kv "${result_receipt}" checkpoint_path "${checkpoint}"
  expect_kv "${result_receipt}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  expect_kv "${result_receipt}" job_manifest_path "${job_dir}/job.manifest"
  expect_kv "${result_receipt}" job_manifest_sha256 \
    "$(sha256_of "${job_dir}/job.manifest")"
  expect_kv "${result_receipt}" runtime_result_path \
    "${job_dir}/runtime.result.fact"
  expect_kv "${result_receipt}" runtime_result_sha256 \
    "$(sha256_of "${job_dir}/runtime.result.fact")"
  expect_kv "${result_receipt}" mdn_report_path \
    "${job_dir}/channel_inference.report"
  expect_kv "${result_receipt}" mdn_report_sha256 \
    "$(sha256_of "${job_dir}/channel_inference.report")"
  expect_kv "${result_receipt}" train_log_path "${log_path}"
  expect_kv "${result_receipt}" train_log_sha256 "$(sha256_of "${log_path}")"
  expect_kv "${result_receipt}" train_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${result_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${result_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${result_receipt}" maximum_available_anchor_index 3260
  expect_kv "${result_receipt}" optimizer_steps "${expected_steps}"
  expect_kv "${result_receipt}" seed "${expected_seed}"
  expect_kv "${result_receipt}" execution_chain "${expected_execution_chain}"
  expect_kv "${result_receipt}" stream_plan "${expected_stream_plan}"
  expect_kv "${result_receipt}" objective_vector "${expected_objective_vector}"
  expect_kv "${result_receipt}" restart_from_step_zero true
  expect_kv "${result_receipt}" quarantined_checkpoint_access false
  expect_kv "${result_receipt}" canonical_data_raw_access false
  expect_kv "${result_receipt}" final_holdout_available false
  expect_kv "${result_receipt}" policy_access false
}

seal_runtime_artifacts() {
  local path symlink special hardlinked
  for path in "${runtime_root}/components" "${runtime_root}/system"; do
    verify_fixed_directory "${path}"
  done
  symlink="$(find "${runtime_root}" -type l -print -quit)"
  [[ -z "${symlink}" ]] ||
    fail "retry1 runtime contains a symlink: ${symlink}"
  special="$(find "${runtime_root}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "retry1 runtime contains a special entry: ${special}"
  hardlinked="$(find "${runtime_root}" -type f ! -links 1 -print -quit)"
  [[ -z "${hardlinked}" ]] ||
    fail "retry1 runtime contains an external hard link: ${hardlinked}"
  find "${runtime_root}/components" "${runtime_root}/system" \
    -type f -exec chmod 0444 -- {} +
  find "${runtime_root}/components" "${runtime_root}/system" \
    -depth -type d -exec chmod 0555 -- {} +
  chmod 0444 -- "${config_snapshot}" "${input_receipt}" "${result_receipt}" \
    "${log_path}" "${runtime_root}/.execution.lock"
  chmod 0555 -- "${runtime_root}"
}

assert_exact_runtime_tree() {
  local actual expected
  actual="$(find "${runtime_root}" -mindepth 1 -maxdepth 1 -printf '%f\n' | \
    LC_ALL=C sort)"
  expected="$(printf '%s\n' \
    .execution.lock \
    components \
    inputs.status \
    job \
    mdn_train.retry1.log \
    result.status \
    system \
    synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config | \
    LC_ALL=C sort)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "completed retry1 runtime has an unexpected top-level entry"
}

assert_runtime_artifacts_sealed() {
  local path writable symlink special hardlinked
  assert_exact_runtime_tree
  for path in "${runtime_root}" "${job_dir}"; do
    [[ "$(stat -c '%A' -- "${path}")" != *w* ]] ||
      fail "completed runtime directory is writable: ${path}"
  done
  for path in "${config_snapshot}" "${input_receipt}" "${result_receipt}" \
    "${log_path}" "${runtime_root}/.execution.lock"; do
    [[ "$(stat -c '%A' -- "${path}")" != *w* ]] ||
      fail "completed runtime artifact is writable: ${path}"
    [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
      fail "completed runtime artifact has an external hard link: ${path}"
  done
  writable="$(find "${runtime_root}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "completed runtime tree contains a writable entry: ${writable}"
  symlink="$(find "${runtime_root}" -type l -print -quit)"
  [[ -z "${symlink}" ]] ||
    fail "completed runtime tree contains a symlink: ${symlink}"
  special="$(find "${runtime_root}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "completed runtime tree contains a special entry: ${special}"
  hardlinked="$(find "${runtime_root}" -type f ! -links 1 -print -quit)"
  [[ -z "${hardlinked}" ]] ||
    fail "completed runtime tree contains an external hard link: ${hardlinked}"
}

verify_completed() {
  local base_config checkpoint
  verify_static_inputs
  verify_runtime_hierarchy
  base_config="$(isolated_base_config_path)"
  verify_config_snapshot "${base_config}"
  verify_representation_input
  verify_input_receipt
  checkpoint="$(validate_job)"
  assert_job_artifacts_sealed
  verify_result_receipt "${checkpoint}"
  assert_runtime_artifacts_sealed
  echo "mdn_checkpoint=${checkpoint}"
  echo "mdn_checkpoint_sha256=$(sha256_of "${checkpoint}")"
}

if [[ "${mode}" == --verify ]]; then
  verify_completed
  exit 0
fi

verify_static_inputs
prepare_runtime_root
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || fail "another isolated MDN retry1 run holds the lock"
base_config="$(isolated_base_config_path)"
write_config_snapshot "${base_config}"
verify_representation_input

candidate_inputs="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
emit_inputs "${candidate_inputs}"
chmod 0444 "${candidate_inputs}"
if [[ -e "${input_receipt}" ]]; then
  cmp -s -- "${candidate_inputs}" "${input_receipt}" ||
    fail "immutable retry1 input receipt drifted"
  rm -f -- "${candidate_inputs}"
else
  ln -- "${candidate_inputs}" "${input_receipt}" ||
    fail "retry1 input receipt publication failed"
  rm -f -- "${candidate_inputs}"
fi
verify_input_receipt

[[ ! -e "${job_dir}" && ! -L "${job_dir}" ]] ||
  fail "retry1 job path already exists; overwrite/resume is forbidden"
"${runtime_exec}" \
  --config "${config_snapshot}" \
  --job-dir "${job_dir}" \
  --source-range anchor_index \
  --anchor-index-begin "${train_begin}" \
  --anchor-index-end "${train_end}" \
  --input-representation-checkpoint "${representation_checkpoint}" \
  --no-replay-artifacts >"${log_path}" 2>&1 ||
  fail "isolated MDN retry1 training failed; see ${log_path}"

checkpoint="$(validate_job)"
seal_job_artifacts
checkpoint="$(validate_job)"
assert_job_artifacts_sealed
"${source_verifier}" --verify >/dev/null
"${interruption_sealer}" --verify >/dev/null
write_result_receipt "${checkpoint}"
verify_result_receipt "${checkpoint}"
seal_runtime_artifacts
verify_completed
