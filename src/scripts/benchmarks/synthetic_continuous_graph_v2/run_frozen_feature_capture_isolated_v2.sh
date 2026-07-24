#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_frozen_feature_capture_isolated_v2"
readonly source_schema_id="synthetic_v2_isolated_development_source_v1"
readonly cursor_erratum_schema_id="synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1"
readonly representation_result_schema_id="synthetic_v2_representation_train_isolated_v2.result.v1"
readonly mdn_result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"
readonly mdn_completion_closure_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.completion_concurrency_closure.v1"
readonly affine_development_schema_id="synthetic_v2_frozen_affine_development_isolated_v2"
readonly affine_trigger_schema_id="synthetic_v2_frozen_affine_route_trigger_isolated_v2"
readonly post384_report_file_id="synthetic_v2_frozen_representation_affine_development_isolated_v2"
readonly raw96_report_file_id="synthetic_v2_frozen_encoder_affine_development_isolated_v2"
readonly untrained_raw96_report_file_id="synthetic_v2_untrained_encoder_affine_control_isolated_v2"
readonly post384_report_schema_id="synthetic_v2_frozen_representation_affine_development_v1"
readonly raw96_report_schema_id="synthetic_v2_frozen_encoder_affine_development_v1"
readonly untrained_raw96_report_schema_id="synthetic_v2_untrained_encoder_affine_control_v1"

readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_fresh_preregistration_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly expected_diagnostic_preregistration_sha256="de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8"
readonly expected_protocol_amendment_sha256="3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5"
readonly expected_localization_addendum_sha256="2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4"
readonly expected_conditional_amendment_sha256="30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67"
readonly expected_ablation_preregistration_sha256="6a4175f431347387f33c250b747f1f34c29099aaf4b3c94a75ea2e4960cef6cd"
readonly expected_source_isolation_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_isolated_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_alignment_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
readonly expected_cursor_erratum_verifier_sha256="e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99"
readonly expected_cursor_erratum_receipt_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly expected_source_verifier_sha256="dca034ec2440c7ab9caa936dee965879fe4cbd48ca29fdd6432e62f73af1cf05"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_mdn_execution_runner_sha256="93c477a6e4de3ddfbded94ca8f22db14cd7954800a319713c0a7961ccf2bb799"
readonly expected_mdn_completion_closure_wrapper_sha256="68d08dab8d219bb9d59fc1a73c62becbc45ecff03ef203265bd2725059e6cc8c"
readonly expected_mdn_completion_closure_receipt_sha256="9ee4e5c78809ee622a9979608248f3db3309b50d3de53e91c2ba86a2187540cf"
readonly expected_mdn_completion_erratum_sha256="c00b5f842237b1aada5490783ee80023de2ac1c5f3d7a0224f46caf295c8cad9"
readonly expected_mdn_final_sealer_sha256="88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0"
readonly expected_mdn_completion_correction_sha256="ab4ceb9a7d1e6d55c6830b3263abfbfe60225399283ef63673176c11d4ebc5d9"
readonly expected_mdn_result_sha256="d9eeddb89be7f2313083f4ea375bbf8c7f4168c95d15c4dbc216eadd009c1d93"
readonly expected_mdn_checkpoint_sha256="a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f"
readonly expected_mdn_train_config_sha256="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
readonly expected_mdn_objective_sha256="33b40ce2f6a76f0c9dddc67b9e3b162d1a171199b6d50b174dafaf854b135d5e"
readonly expected_mdn_payload_inventory_master_sha256="43b36342443cd37d78d2639618ea87f8c8c6f43dce24e0ef4a73cdc9a6399f50"
readonly expected_mdn_content_inventory_sha256="84a401659e73f2a2c6cabf66f204f0cb50654128339d1b5024548ed0ad36339a"
readonly expected_mdn_completion_log_sha256="6dae5f7f4f1dc7e6b41756abd07171cb055300f131d1b958061b51b416f20013"
readonly expected_empty_sha256="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
readonly expected_mdn_run_execution_chain="ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:run"
readonly expected_mdn_stream_plan="source(ujcamei.source.retrieval.graph_anchor:graph_anchor_edge_dataset_v1) -> nodelift(wikimyei.expression.nodelift.srl:nodelift_srl_v1) -> channel_representation(wikimyei.representation.encoding.mtf_jepa_mae_vicreg:mtf_jepa_mae_vicreg_v1) -> channel_inference(wikimyei.inference.expected_value.mdn:mdn_v1)"

readonly accepted_anchor_count=3261
readonly maximum_anchor_index=3260
readonly train_begin=0
readonly train_end=2496
readonly validation_begin=2560
readonly validation_end=2816
readonly certified_begin=2880
readonly certified_end=3261
readonly forbidden_final_begin=3328
readonly train_rows=$(((train_end - train_begin) * 9))
readonly validation_rows=$(((validation_end - validation_begin) * 9))
readonly certified_rows=$(((certified_end - certified_begin) * 9))

fail() {
  echo "v2 isolated frozen feature capture: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'USAGE'
usage: run_frozen_feature_capture_isolated_v2.sh
       [--plan|--run-development|--verify-development|--run-certified|--verify]

  --run-development     Capture trained and untrained train/validation only.
  --verify-development  Read-only verification; certified artifacts must be absent.
  --run-certified       One-shot canonical certified capture, only after its trigger.
  --verify              Read-only verification of a completed canonical capture.
USAGE
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
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
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv_value "${path}" "${key}")"
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
    fail "${path}: exact keyset mismatch"
}

reject_data_raw_path() {
  local path="$1"
  case "${path}" in
  */data/raw | */data/raw/* | */data/final | */data/final/*)
    fail "canonical raw/final data path is forbidden: ${path}"
    ;;
  esac
}

is_original_non_retry_mdn_runtime_path() {
  local path="$1"
  case "${path}/" in
  */synthetic_v2_mdn_train_isolated_v2/*) return 0 ;;
  *) return 1 ;;
  esac
}

reject_old_runtime_path() {
  local path="$1"
  case "${path}" in
  *synthetic_v2_representation_train_v1* | \
    *synthetic_v2_mdn_train_v1* | \
    *synthetic_v2_frozen_feature_capture_v1*)
    fail "quarantined pre-isolation runtime path is forbidden: ${path}"
    ;;
  esac
  if is_original_non_retry_mdn_runtime_path "${path}"; then
    fail "quarantined original non-retry MDN runtime path is forbidden: ${path}"
  fi
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
    if [[ "${current}" == / ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains a symbolic-link component: ${current}"
  done
}

require_file() {
  local path="$1"
  reject_data_raw_path "${path}"
  reject_old_runtime_path "${path}"
  reject_symlink_components "${path}"
  [[ -f "${path}" && ! -L "${path}" ]] ||
    fail "missing, non-regular, or symlinked file: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "file path is not canonical: ${path}"
}

require_immutable_file() {
  require_file "$1"
  [[ "$(stat -c '%A' -- "$1")" != *w* ]] ||
    fail "immutable file retains a write bit: $1"
}

require_directory() {
  local path="$1"
  reject_data_raw_path "${path}"
  reject_old_runtime_path "${path}"
  reject_symlink_components "${path}"
  [[ -d "${path}" && ! -L "${path}" ]] ||
    fail "missing or symlinked directory: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "directory path is not canonical: ${path}"
}

require_contained_path() {
  local path="$1" root="$2"
  [[ "${path}" == "${root}" || "${path}" == "${root}/"* ]] ||
    fail "path escapes fixed root ${root}: ${path}"
}

ensure_fixed_directory() {
  local path="$1" create_parents="${2:-false}"
  reject_data_raw_path "${path}"
  reject_old_runtime_path "${path}"
  reject_symlink_components "${path}"
  if [[ -e "${path}" || -L "${path}" ]]; then
    require_directory "${path}"
  elif [[ "${create_parents}" == true ]]; then
    mkdir -p -- "${path}"
  else
    mkdir -- "${path}"
  fi
  require_directory "${path}"
}

publish_immutable_candidate() {
  local candidate="$1" destination="$2" label="$3"
  require_contained_path "${destination}" "${runtime_root}"
  reject_symlink_components "${destination}"
  chmod 0444 -- "${candidate}"
  if [[ -e "${destination}" || -L "${destination}" ]]; then
    require_immutable_file "${destination}"
    cmp -s -- "${candidate}" "${destination}" ||
      fail "immutable ${label} drifted: ${destination}"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${destination}" ||
      fail "could not atomically publish ${label}: ${destination}"
    rm -f -- "${candidate}"
  fi
  require_immutable_file "${destination}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"

source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
source_closure="${source_runtime}/development_source_closure.status"
source_verifier="${script_dir}/prepare_and_seal_isolated_development_source_v2.sh"
cursor_erratum_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
cursor_erratum_receipt="${source_runtime}/cursor_alignment_erratum.status"
isolated_source_root="${source_runtime}/source"
isolated_registry="${source_runtime}/config/ujcamei.source.registry.development_prefix.dsl"

representation_runner="${script_dir}/run_representation_train_isolated_v2.sh"
representation_runtime="${runtime_parent}/synthetic_v2_representation_train_isolated_v2"
representation_result="${representation_runtime}/result.status"
representation_train_config="${representation_runtime}/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config"

mdn_execution_runner="${script_dir}/run_mdn_train_isolated_v2_retry1.sh"
mdn_completion_closure_wrapper="${script_dir}/seal_and_verify_mdn_retry1_completion_concurrency_closure.sh"
mdn_completion_erratum="${benchmark_root}/MDN_RETRY1_SEAL_CONCURRENCY_ERRATUM.md"
mdn_final_sealer="${script_dir}/seal_and_verify_mdn_retry1_completed_job.sh"
mdn_completion_correction="${benchmark_root}/MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md"
mdn_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2_retry1"
mdn_result="${mdn_runtime}/result.status"
mdn_train_config="${mdn_runtime}/synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
mdn_completion_closure_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"
mdn_completion_closure_receipt="${mdn_completion_closure_runtime}/completion_concurrency.status"
mdn_policy_source="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"

affine_helper="${script_dir}/frozen_representation_affine_probe.cpp"
affine_runner="${script_dir}/run_frozen_representation_affine_probe_isolated_v2.sh"
affine_development_runtime="${runtime_parent}/synthetic_v2_frozen_affine_development_isolated_v2"
affine_binary="${affine_development_runtime}/bin/frozen_representation_affine_probe"
affine_development_status="${affine_development_runtime}/development.status"
affine_master_manifest="${affine_development_runtime}/master.sha256"
post384_report="${affine_development_runtime}/main/${post384_report_file_id}.report"
raw96_report="${affine_development_runtime}/main/${raw96_report_file_id}.report"
untrained_raw96_report="${affine_development_runtime}/main/${untrained_raw96_report_file_id}.report"

fresh_preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
diagnostic_preregistration="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md"
protocol_amendment="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md"
localization_addendum="${benchmark_root}/REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md"
conditional_amendment="${benchmark_root}/REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md"
ablation_preregistration="${benchmark_root}/REPRESENTATION_ABLATION_PREREGISTRATION.md"
source_isolation_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
isolated_source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_alignment_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"

runtime_root="${runtime_parent}/${schema_id}"
capture_config="${runtime_root}/synthetic_benchmark.frozen_feature_capture.isolated.config"
untrained_mdn_policy="${runtime_root}/wikimyei.inference.expected_value.mdn.untrained_control.isolated.jkimyei"
untrained_capture_config="${runtime_root}/synthetic_benchmark.untrained_representation_capture.isolated.config"
job_root="${runtime_root}/jobs"
train_job="${job_root}/train"
validation_job="${job_root}/validation"
certified_job="${job_root}/certified"
untrained_job_root="${runtime_root}/untrained_jobs"
untrained_train_job="${untrained_job_root}/train"
untrained_validation_job="${untrained_job_root}/validation"
selection_source_root="${runtime_root}/frozen_selection_sources"
frozen_affine_helper="${selection_source_root}/frozen_representation_affine_probe.cpp"
frozen_affine_runner="${selection_source_root}/run_frozen_representation_affine_probe_isolated_v2.sh"
input_receipt="${runtime_root}/inputs.status"
development_receipt="${runtime_root}/development.status"
affine_route_trigger="${runtime_root}/affine_route_trigger.status"
certified_attempt_receipt="${runtime_root}/certified.attempt.status"
result_receipt="${runtime_root}/result.status"

readonly -a mdn_retry1_result_keys=(
  schema_id status runtime_root job_dir runner_path runner_sha256
  execution_runner_path execution_runner_sha256 completion_sealer_path
  completion_sealer_sha256 completion_correction_path
  completion_correction_sha256 validator_failure_reason
  training_reexecution input_receipt_path input_receipt_sha256
  original_runner_path original_runner_sha256 recovery_amendment_path
  recovery_amendment_sha256 interruption_sealer_path
  interruption_sealer_sha256 interruption_receipt_path
  interruption_receipt_sha256 interrupted_runtime_path
  isolated_source_verifier_path isolated_source_verifier_sha256
  isolated_source_closure_path isolated_source_closure_sha256
  cursor_alignment_erratum_receipt_path
  cursor_alignment_erratum_receipt_sha256 config_snapshot_path
  config_snapshot_sha256 runtime_exec_path runtime_exec_sha256
  representation_runner_path representation_runner_sha256
  representation_result_path representation_result_sha256
  representation_checkpoint_path representation_checkpoint_sha256
  mdn_objective_path mdn_objective_sha256 input_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_sha256 checkpoint_path
  checkpoint_sha256 job_manifest_path job_manifest_sha256
  runtime_result_path runtime_result_sha256 mdn_report_path mdn_report_sha256
  train_log_path train_log_sha256 execution_lock_path execution_lock_sha256
  payload_inventory_format payload_inventory_master_sha256
  payload_directory_count payload_file_count_before_receipt
  sealed_file_count_including_receipt payload_bytes_before_receipt
  job_file_count component_file_count system_file_count
  job_file_00_path job_file_00_sha256 job_file_00_bytes
  job_file_01_path job_file_01_sha256 job_file_01_bytes
  job_file_02_path job_file_02_sha256 job_file_02_bytes
  job_file_03_path job_file_03_sha256 job_file_03_bytes
  job_file_04_path job_file_04_sha256 job_file_04_bytes
  job_file_05_path job_file_05_sha256 job_file_05_bytes
  job_file_06_path job_file_06_sha256 job_file_06_bytes
  job_file_07_path job_file_07_sha256 job_file_07_bytes
  job_file_08_path job_file_08_sha256 job_file_08_bytes
  job_file_09_path job_file_09_sha256 job_file_09_bytes
  job_file_10_path job_file_10_sha256 job_file_10_bytes
  job_file_11_path job_file_11_sha256 job_file_11_bytes
  job_file_12_path job_file_12_sha256 job_file_12_bytes
  job_file_13_path job_file_13_sha256 job_file_13_bytes
  job_file_14_path job_file_14_sha256 job_file_14_bytes
  job_file_15_path job_file_15_sha256 job_file_15_bytes
  component_file_00_path component_file_00_sha256 component_file_00_bytes
  system_file_00_path system_file_00_sha256 system_file_00_bytes
  system_file_01_path system_file_01_sha256 system_file_01_bytes
  config_snapshot_bytes input_receipt_bytes train_log_bytes
  execution_lock_bytes representation_probe_row_count
  runtime_event_probe_record_count train_anchor_range accepted_anchor_count
  candidate_anchor_count maximum_available_anchor_index
  skipped_outside_common_range skipped_missing_edge_coverage
  skipped_failed_fetch_probe duplicate_anchor_count optimizer_steps seed
  execution_chain stream_plan objective_vector
  report_finite_parameter_check_token
  runtime_result_finite_parameter_check_token
  runtime_health_finite_parameter_check_token
  lattice_exposure_finite_parameter_check_token
  lattice_exposure_gradients_finite_token
  runtime_checkpoint_io_publication_defaults_preserved
  lattice_exposure_publication_defaults_preserved
  payload_sealed_before_receipt_publication restart_from_step_zero
  quarantined_checkpoint_access canonical_data_raw_access
  final_holdout_available independent_final_evidence policy_access
)

readonly -a mdn_completion_closure_keys=(
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
  closure_exact_tree external_staging_absent
  wrapper_lock_separate_from_retry_execution_lock
  retry_execution_lock_held_by_wrapper downstream_verifier_path
  downstream_verifier_process_start_sha256 downstream_required_mode
  downstream_must_bind_closure_receipt result_or_old_runner_alone_forbidden
  authoritative_mutation training_reexecution_authorized
  canonical_data_raw_access final_holdout_available
  independent_final_evidence policy_access
)

declare -a csv_relatives=(
  "SYN2ALPHASYN2USD/1d/SYN2ALPHASYN2USD-1d-all-years.csv"
  "SYN2ALPHASYN2USD/3d/SYN2ALPHASYN2USD-3d-all-years.csv"
  "SYN2ALPHASYN2USD/1w/SYN2ALPHASYN2USD-1w-all-years.csv"
  "SYN2BETASYN2USD/1d/SYN2BETASYN2USD-1d-all-years.csv"
  "SYN2BETASYN2USD/3d/SYN2BETASYN2USD-3d-all-years.csv"
  "SYN2BETASYN2USD/1w/SYN2BETASYN2USD-1w-all-years.csv"
  "SYN2GAMMASYN2USD/1d/SYN2GAMMASYN2USD-1d-all-years.csv"
  "SYN2GAMMASYN2USD/3d/SYN2GAMMASYN2USD-3d-all-years.csv"
  "SYN2GAMMASYN2USD/1w/SYN2GAMMASYN2USD-1w-all-years.csv"
)

mode="${1:---plan}"
[[ "$#" -le 1 ]] || {
  usage
  exit 2
}
case "${mode}" in
--plan | --run-development | --verify-development | --run-certified | --verify) ;;
--run)
  fail "unconditional --run is forbidden; development and certified stages are separate"
  ;;
*)
  usage
  exit 2
  ;;
esac

verify_runtime_rejection_contract() {
  is_original_non_retry_mdn_runtime_path \
    "${runtime_parent}/synthetic_v2_mdn_train_isolated_v2" ||
    fail "original non-retry MDN runtime is not rejected"
  ! is_original_non_retry_mdn_runtime_path "${mdn_runtime}" ||
    fail "retry1 MDN runtime is falsely rejected as the original runtime"
  ! is_original_non_retry_mdn_runtime_path \
    "${mdn_completion_closure_runtime}" ||
    fail "retry1 completion closure is falsely rejected as the original runtime"
}

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
runtime_root=${runtime_root}
isolated_source_schema=${source_schema_id}
isolated_source_closure=${source_closure}
cursor_alignment_erratum_receipt=${cursor_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_erratum_receipt_sha256}
cursor_alignment_erratum_verifier=${cursor_erratum_verifier}
mdn_result_schema_id=${mdn_result_schema_id}
mdn_completion_closure_schema_id=${mdn_completion_closure_schema_id}
mdn_retry_runtime=${mdn_runtime}
mdn_execution_runner_path=${mdn_execution_runner}
mdn_execution_runner_sha256=${expected_mdn_execution_runner_sha256}
mdn_completion_closure_wrapper_path=${mdn_completion_closure_wrapper}
mdn_completion_closure_wrapper_sha256=${expected_mdn_completion_closure_wrapper_sha256}
mdn_completion_closure_receipt_path=${mdn_completion_closure_receipt}
mdn_completion_closure_receipt_sha256=${expected_mdn_completion_closure_receipt_sha256}
mdn_completion_erratum_path=${mdn_completion_erratum}
mdn_completion_erratum_sha256=${expected_mdn_completion_erratum_sha256}
mdn_final_sealer_path=${mdn_final_sealer}
mdn_final_sealer_sha256=${expected_mdn_final_sealer_sha256}
mdn_completion_correction_path=${mdn_completion_correction}
mdn_completion_correction_sha256=${expected_mdn_completion_correction_sha256}
mdn_result_path=${mdn_result}
mdn_result_sha256=${expected_mdn_result_sha256}
mdn_checkpoint_path=${mdn_runtime}/job/channel_inference.report.channel_mdn.pt
mdn_checkpoint_sha256=${expected_mdn_checkpoint_sha256}
mdn_train_config_path=${mdn_train_config}
mdn_train_config_sha256=${expected_mdn_train_config_sha256}
original_non_retry_mdn_runtime_forbidden=${runtime_parent}/synthetic_v2_mdn_train_isolated_v2
mdn_result_alone_is_authority=false
broken_retry_runner_verify_invoked=false
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
train_expected_probe_rows=${train_rows}
validation_expected_probe_rows=${validation_rows}
certified_expected_probe_rows=${certified_rows}
development_first=true
conditional_route_trigger=${affine_route_trigger}
canonical_certification_route=canonical_certification
ablation_route_certified_capture=false
affine_development_runtime=${affine_development_runtime}
affine_binary=${affine_binary}
affine_development_status=${affine_development_status}
affine_master_manifest=${affine_master_manifest}
post384_validation_report=${post384_report}
raw96_validation_report=${raw96_report}
untrained_raw96_validation_report=${untrained_raw96_report}
clean_affine_runner=${affine_runner}
purge_anchors_captured=false
untrained_representation_certified_access=false
model_state_mutation=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  verify_runtime_rejection_contract
  print_plan
  exit 0
fi

emit_document_bindings() {
  cat <<DOCUMENTS
fresh_preregistration_path=${fresh_preregistration}
fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")
diagnostic_preregistration_path=${diagnostic_preregistration}
diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")
diagnostic_protocol_amendment_path=${protocol_amendment}
diagnostic_protocol_amendment_sha256=$(sha256_of "${protocol_amendment}")
localization_addendum_path=${localization_addendum}
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_certification_amendment_path=${conditional_amendment}
conditional_certification_amendment_sha256=$(sha256_of "${conditional_amendment}")
ablation_preregistration_path=${ablation_preregistration}
ablation_preregistration_sha256=$(sha256_of "${ablation_preregistration}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${cursor_alignment_metadata_erratum}")
DOCUMENTS
}

verify_document_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" fresh_preregistration_path "${fresh_preregistration}"
  expect_kv "${receipt}" fresh_preregistration_sha256 \
    "$(sha256_of "${fresh_preregistration}")"
  expect_kv "${receipt}" diagnostic_preregistration_path \
    "${diagnostic_preregistration}"
  expect_kv "${receipt}" diagnostic_preregistration_sha256 \
    "$(sha256_of "${diagnostic_preregistration}")"
  expect_kv "${receipt}" diagnostic_protocol_amendment_path \
    "${protocol_amendment}"
  expect_kv "${receipt}" diagnostic_protocol_amendment_sha256 \
    "$(sha256_of "${protocol_amendment}")"
  expect_kv "${receipt}" localization_addendum_path "${localization_addendum}"
  expect_kv "${receipt}" localization_addendum_sha256 \
    "$(sha256_of "${localization_addendum}")"
  expect_kv "${receipt}" conditional_certification_amendment_path \
    "${conditional_amendment}"
  expect_kv "${receipt}" conditional_certification_amendment_sha256 \
    "$(sha256_of "${conditional_amendment}")"
  expect_kv "${receipt}" ablation_preregistration_path \
    "${ablation_preregistration}"
  expect_kv "${receipt}" ablation_preregistration_sha256 \
    "$(sha256_of "${ablation_preregistration}")"
  expect_kv "${receipt}" source_isolation_amendment_path \
    "${source_isolation_amendment}"
  expect_kv "${receipt}" source_isolation_amendment_sha256 \
    "$(sha256_of "${source_isolation_amendment}")"
  expect_kv "${receipt}" isolated_source_protocol_path \
    "${isolated_source_protocol}"
  expect_kv "${receipt}" isolated_source_protocol_sha256 \
    "$(sha256_of "${isolated_source_protocol}")"
  expect_kv "${receipt}" staged_hardening_amendment_path \
    "${staged_hardening}"
  expect_kv "${receipt}" staged_hardening_amendment_sha256 \
    "$(sha256_of "${staged_hardening}")"
  expect_kv "${receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${receipt}" cursor_alignment_correction_sha256 \
    "$(sha256_of "${cursor_alignment_correction}")"
  expect_kv "${receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${receipt}" cursor_alignment_metadata_erratum_sha256 \
    "$(sha256_of "${cursor_alignment_metadata_erratum}")"
}

verify_expected_document_hashes() {
  local path
  for path in "${fresh_preregistration}" "${diagnostic_preregistration}" \
    "${protocol_amendment}" "${localization_addendum}" \
    "${conditional_amendment}" "${ablation_preregistration}" \
    "${source_isolation_amendment}" "${isolated_source_protocol}" \
    "${staged_hardening}" "${cursor_alignment_correction}" \
    "${cursor_alignment_metadata_erratum}"; do
    require_file "${path}"
  done
  [[ "$(sha256_of "${fresh_preregistration}")" == \
    "${expected_fresh_preregistration_sha256}" ]] ||
    fail "fresh-seed preregistration hash drifted"
  [[ "$(sha256_of "${diagnostic_preregistration}")" == \
    "${expected_diagnostic_preregistration_sha256}" ]] ||
    fail "diagnostic preregistration hash drifted"
  [[ "$(sha256_of "${protocol_amendment}")" == \
    "${expected_protocol_amendment_sha256}" ]] ||
    fail "diagnostic protocol amendment hash drifted"
  [[ "$(sha256_of "${localization_addendum}")" == \
    "${expected_localization_addendum_sha256}" ]] ||
    fail "localization addendum hash drifted"
  [[ "$(sha256_of "${conditional_amendment}")" == \
    "${expected_conditional_amendment_sha256}" ]] ||
    fail "conditional-certification amendment hash drifted"
  [[ "$(sha256_of "${ablation_preregistration}")" == \
    "${expected_ablation_preregistration_sha256}" ]] ||
    fail "ablation preregistration hash drifted"
  [[ "$(sha256_of "${source_isolation_amendment}")" == \
    "${expected_source_isolation_amendment_sha256}" ]] ||
    fail "source-isolation amendment hash drifted"
  [[ "$(sha256_of "${isolated_source_protocol}")" == \
    "${expected_isolated_source_protocol_sha256}" ]] ||
    fail "isolated-source protocol hash drifted"
  [[ "$(sha256_of "${staged_hardening}")" == \
    "${expected_staged_hardening_sha256}" ]] ||
    fail "staged-hardening amendment hash drifted"
  [[ "$(sha256_of "${cursor_alignment_correction}")" == \
    "${expected_cursor_alignment_correction_sha256}" ]] ||
    fail "cursor-alignment correction hash drifted"
  [[ "$(sha256_of "${cursor_alignment_metadata_erratum}")" == \
    "${expected_cursor_alignment_metadata_erratum_sha256}" ]] ||
    fail "cursor-alignment metadata erratum hash drifted"
}

emit_cursor_erratum_bindings() {
  cat <<ERRATUM
cursor_alignment_erratum_verifier_path=${cursor_erratum_verifier}
cursor_alignment_erratum_verifier_sha256=$(sha256_of "${cursor_erratum_verifier}")
cursor_alignment_erratum_receipt_path=${cursor_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_erratum_receipt}")
ERRATUM
}

verify_cursor_erratum_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" cursor_alignment_erratum_verifier_path \
    "${cursor_erratum_verifier}"
  expect_kv "${receipt}" cursor_alignment_erratum_verifier_sha256 \
    "$(sha256_of "${cursor_erratum_verifier}")"
  expect_kv "${receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_erratum_receipt}"
  expect_kv "${receipt}" cursor_alignment_erratum_receipt_sha256 \
    "$(sha256_of "${cursor_erratum_receipt}")"
}

verify_source_closure() {
  require_file "${source_verifier}"
  [[ -x "${source_verifier}" ]] || fail "isolated-source verifier is not executable"
  [[ "$(sha256_of "${source_verifier}")" == \
    "${expected_source_verifier_sha256}" ]] ||
    fail "isolated-source verifier hash drifted"
  require_file "${cursor_erratum_verifier}"
  [[ -x "${cursor_erratum_verifier}" ]] ||
    fail "cursor-alignment erratum verifier is not executable"
  [[ "$(sha256_of "${cursor_erratum_verifier}")" == \
    "${expected_cursor_erratum_verifier_sha256}" ]] ||
    fail "cursor-alignment erratum verifier hash drifted"
  require_immutable_file "${source_closure}"
  [[ "$(sha256_of "${source_closure}")" == \
    "${expected_source_closure_sha256}" ]] ||
    fail "isolated-source closure hash drifted"
  require_immutable_file "${cursor_erratum_receipt}"
  [[ "$(stat -c '%a' -- "${cursor_erratum_receipt}")" == 444 ]] ||
    fail "cursor-alignment erratum receipt mode is not exactly 0444"
  [[ "$(sha256_of "${cursor_erratum_receipt}")" == \
    "${expected_cursor_erratum_receipt_sha256}" ]] ||
    fail "cursor-alignment erratum receipt hash drifted"
  "${cursor_erratum_verifier}" --verify >/dev/null
  expect_kv "${cursor_erratum_receipt}" schema_id \
    "${cursor_erratum_schema_id}"
  expect_kv "${cursor_erratum_receipt}" status complete
  expect_kv "${cursor_erratum_receipt}" erratum_verifier_path \
    "${cursor_erratum_verifier}"
  expect_kv "${cursor_erratum_receipt}" erratum_verifier_sha256 \
    "$(sha256_of "${cursor_erratum_verifier}")"
  expect_kv "${cursor_erratum_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${cursor_erratum_receipt}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  expect_kv "${cursor_erratum_receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${cursor_erratum_receipt}" \
    cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${cursor_erratum_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" maximum_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${cursor_erratum_receipt}" certified_development_anchor_range \
    "[${certified_begin},${certified_end})"
  expect_kv "${cursor_erratum_receipt}" certified_development_probe_rows \
    "${certified_rows}"
  expect_kv "${cursor_erratum_receipt}" canonical_data_raw_access false
  expect_kv "${cursor_erratum_receipt}" final_holdout_available false
  expect_kv "${cursor_erratum_receipt}" independent_final_evidence false
  expect_kv "${source_closure}" schema_id "${source_schema_id}"
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" maximum_anchor_index "${maximum_anchor_index}"
  expect_kv "${source_closure}" isolated_source_root_path \
    "${isolated_source_root}"
  expect_kv "${source_closure}" isolated_registry_path "${isolated_registry}"
  expect_kv "${source_closure}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${source_closure}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false
  expect_kv "${source_closure}" strict_cache_freshness pass
  require_directory "${isolated_source_root}"
  require_immutable_file "${isolated_registry}"
}

checkpoint_from_representation_result() {
  local result="${representation_result}"
  local expected_schema="${representation_result_schema_id}"
  local expected_runner="${representation_runner}"
  local checkpoint
  require_immutable_file "${result}"
  expect_kv "${result}" schema_id "${expected_schema}"
  expect_kv "${result}" status complete
  expect_kv "${result}" runner_path "${expected_runner}"
  expect_kv "${result}" runner_sha256 "$(sha256_of "${expected_runner}")"
  expect_kv "${result}" isolated_source_closure_path "${source_closure}"
  expect_kv "${result}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  expect_kv "${result}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${result}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${result}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${result}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${result}" cursor_alignment_erratum_receipt_path \
    "${cursor_erratum_receipt}"
  expect_kv "${result}" cursor_alignment_erratum_receipt_sha256 \
    "$(sha256_of "${cursor_erratum_receipt}")"
  expect_kv "${result}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${result}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${result}" maximum_available_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${result}" restart_from_step_zero true
  expect_kv "${result}" quarantined_checkpoint_access false
  expect_kv "${result}" canonical_data_raw_access false
  expect_kv "${result}" final_holdout_available false
  expect_kv "${result}" policy_access false
  checkpoint="$(kv_value "${result}" checkpoint_path)"
  require_immutable_file "${checkpoint}"
  expect_kv "${result}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  printf '%s' "${checkpoint}"
}

verify_pinned_immutable_file() {
  local path="$1" expected_sha256="$2" expected_mode="$3" label="$4"
  require_immutable_file "${path}"
  [[ "$(stat -c '%a' -- "${path}")" == "${expected_mode}" ]] ||
    fail "${label} mode is not exactly 0${expected_mode}: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
  [[ "$(sha256_of "${path}")" == "${expected_sha256}" ]] ||
    fail "${label} hash drifted: ${path}"
}

verify_pinned_file_hash() {
  local path="$1" expected_sha256="$2" label="$3"
  require_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
  [[ "$(sha256_of "${path}")" == "${expected_sha256}" ]] ||
    fail "${label} hash drifted: ${path}"
}

verify_mdn_retry1_authority() {
  local checkpoint
  verify_pinned_file_hash "${mdn_execution_runner}" \
    "${expected_mdn_execution_runner_sha256}" "MDN retry1 execution runner"
  [[ -x "${mdn_execution_runner}" ]] ||
    fail "MDN retry1 execution runner is not executable"
  verify_pinned_immutable_file "${mdn_completion_closure_wrapper}" \
    "${expected_mdn_completion_closure_wrapper_sha256}" 555 \
    "MDN completion closure wrapper"
  [[ -x "${mdn_completion_closure_wrapper}" ]] ||
    fail "MDN completion closure wrapper is not executable"
  verify_pinned_immutable_file "${mdn_completion_closure_receipt}" \
    "${expected_mdn_completion_closure_receipt_sha256}" 444 \
    "MDN completion closure receipt"
  verify_pinned_immutable_file "${mdn_completion_erratum}" \
    "${expected_mdn_completion_erratum_sha256}" 444 \
    "MDN completion concurrency erratum"
  verify_pinned_immutable_file "${mdn_final_sealer}" \
    "${expected_mdn_final_sealer_sha256}" 555 "MDN final completion sealer"
  [[ -x "${mdn_final_sealer}" ]] ||
    fail "MDN final completion sealer is not executable"
  verify_pinned_immutable_file "${mdn_completion_correction}" \
    "${expected_mdn_completion_correction_sha256}" 444 \
    "MDN completion inventory correction"
  verify_pinned_immutable_file "${mdn_result}" \
    "${expected_mdn_result_sha256}" 444 "MDN retry1 result"
  verify_pinned_immutable_file "${mdn_train_config}" \
    "${expected_mdn_train_config_sha256}" 444 "MDN retry1 config"
  verify_pinned_file_hash "${mdn_policy_source}" \
    "${expected_mdn_objective_sha256}" "MDN objective"

  verify_exact_kv_keys "${mdn_result}" "${mdn_retry1_result_keys[@]}"
  expect_kv "${mdn_result}" schema_id "${mdn_result_schema_id}"
  expect_kv "${mdn_result}" status complete
  expect_kv "${mdn_result}" runtime_root "${mdn_runtime}"
  expect_kv "${mdn_result}" job_dir "${mdn_runtime}/job"
  expect_kv "${mdn_result}" runner_path "${mdn_execution_runner}"
  expect_kv "${mdn_result}" runner_sha256 \
    "${expected_mdn_execution_runner_sha256}"
  expect_kv "${mdn_result}" execution_runner_path "${mdn_execution_runner}"
  expect_kv "${mdn_result}" execution_runner_sha256 \
    "${expected_mdn_execution_runner_sha256}"
  expect_kv "${mdn_result}" completion_sealer_path "${mdn_final_sealer}"
  expect_kv "${mdn_result}" completion_sealer_sha256 \
    "${expected_mdn_final_sealer_sha256}"
  expect_kv "${mdn_result}" completion_correction_path \
    "${mdn_completion_correction}"
  expect_kv "${mdn_result}" completion_correction_sha256 \
    "${expected_mdn_completion_correction_sha256}"
  expect_kv "${mdn_result}" validator_failure_reason \
    job_allowlist_omitted_seven_standard_runtime_facts_and_finite_parameter_check_component_encoding_mismatch
  expect_kv "${mdn_result}" training_reexecution false
  expect_kv "${mdn_result}" isolated_source_closure_path "${source_closure}"
  expect_kv "${mdn_result}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${mdn_result}" cursor_alignment_erratum_receipt_path \
    "${cursor_erratum_receipt}"
  expect_kv "${mdn_result}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_receipt_sha256}"
  expect_kv "${mdn_result}" config_snapshot_path "${mdn_train_config}"
  expect_kv "${mdn_result}" config_snapshot_sha256 \
    "${expected_mdn_train_config_sha256}"
  expect_kv "${mdn_result}" runtime_exec_path "${runtime_exec}"
  expect_kv "${mdn_result}" runtime_exec_sha256 \
    "${expected_runtime_exec_sha256}"
  expect_kv "${mdn_result}" representation_runner_path \
    "${representation_runner}"
  expect_kv "${mdn_result}" representation_runner_sha256 \
    "$(sha256_of "${representation_runner}")"
  expect_kv "${mdn_result}" representation_result_path \
    "${representation_result}"
  expect_kv "${mdn_result}" representation_result_sha256 \
    "$(sha256_of "${representation_result}")"
  expect_kv "${mdn_result}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${mdn_result}" representation_checkpoint_sha256 \
    "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${mdn_result}" mdn_objective_path "${mdn_policy_source}"
  expect_kv "${mdn_result}" mdn_objective_sha256 \
    "${expected_mdn_objective_sha256}"
  expect_kv "${mdn_result}" input_mdn_checkpoint_path ''
  checkpoint="$(kv_value "${mdn_result}" checkpoint_path)"
  [[ "${checkpoint}" == \
    "${mdn_runtime}/job/channel_inference.report.channel_mdn.pt" ]] ||
    fail "MDN retry1 result points to an unexpected checkpoint: ${checkpoint}"
  verify_pinned_immutable_file "${checkpoint}" \
    "${expected_mdn_checkpoint_sha256}" 444 "MDN retry1 checkpoint"
  expect_kv "${mdn_result}" checkpoint_sha256 \
    "${expected_mdn_checkpoint_sha256}"
  expect_kv "${mdn_result}" payload_inventory_master_sha256 \
    "${expected_mdn_payload_inventory_master_sha256}"
  expect_kv "${mdn_result}" payload_directory_count 7
  expect_kv "${mdn_result}" payload_file_count_before_receipt 23
  expect_kv "${mdn_result}" sealed_file_count_including_receipt 24
  expect_kv "${mdn_result}" train_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${mdn_result}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${mdn_result}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${mdn_result}" maximum_available_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${mdn_result}" skipped_outside_common_range 0
  expect_kv "${mdn_result}" skipped_missing_edge_coverage 0
  expect_kv "${mdn_result}" skipped_failed_fetch_probe 0
  expect_kv "${mdn_result}" duplicate_anchor_count 0
  expect_kv "${mdn_result}" optimizer_steps 3500
  expect_kv "${mdn_result}" seed 31
  expect_kv "${mdn_result}" execution_chain \
    "ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:train"
  expect_kv "${mdn_result}" stream_plan \
    "source(ujcamei.source.retrieval.graph_anchor:graph_anchor_edge_dataset_v1) -> nodelift(wikimyei.expression.nodelift.srl:nodelift_srl_v1) -> channel_representation(wikimyei.representation.encoding.mtf_jepa_mae_vicreg:mtf_jepa_mae_vicreg_v1) -> channel_inference(wikimyei.inference.expected_value.mdn:mdn_v1)"
  expect_kv "${mdn_result}" objective_vector \
    "learning_rate=0.001|max_steps=3500|batch=64|grad_clip=5|checkpoint_every=50|report_every=10|seed=31|mixtures=3|hidden=128|residual_depth=2|edge_aux=0|edge_aux_direction=0|edge_aux_rank=0|edge_aux_huber=0.01|edge_aux_logit_scale=50|direct_enabled=true|direct=100|direct_direction=5|direct_rank=5|direct_huber=0.5|direct_logit_scale=1|direct_target_scale=36|warmup_steps=800|warmup_nll=0|post_warmup_nll=1|warmup_direct_only=true|identity=edge_embedding_per_edge|base_edges=3|identity_dim=16|adapter_hidden=128"
  expect_kv "${mdn_result}" report_finite_parameter_check_token 1
  expect_kv "${mdn_result}" runtime_result_finite_parameter_check_token 1
  expect_kv "${mdn_result}" runtime_health_finite_parameter_check_token 1
  expect_kv "${mdn_result}" lattice_exposure_finite_parameter_check_token true
  expect_kv "${mdn_result}" lattice_exposure_gradients_finite_token false
  expect_kv "${mdn_result}" \
    runtime_checkpoint_io_publication_defaults_preserved true
  expect_kv "${mdn_result}" \
    lattice_exposure_publication_defaults_preserved true
  expect_kv "${mdn_result}" payload_sealed_before_receipt_publication true
  expect_kv "${mdn_result}" restart_from_step_zero true
  expect_kv "${mdn_result}" quarantined_checkpoint_access false
  expect_kv "${mdn_result}" canonical_data_raw_access false
  expect_kv "${mdn_result}" final_holdout_available false
  expect_kv "${mdn_result}" independent_final_evidence false
  expect_kv "${mdn_result}" policy_access false

  verify_exact_kv_keys "${mdn_completion_closure_receipt}" \
    "${mdn_completion_closure_keys[@]}"
  expect_kv "${mdn_completion_closure_receipt}" schema_id \
    "${mdn_completion_closure_schema_id}"
  expect_kv "${mdn_completion_closure_receipt}" status complete
  expect_kv "${mdn_completion_closure_receipt}" closure_runtime_root \
    "${mdn_completion_closure_runtime}"
  expect_kv "${mdn_completion_closure_receipt}" closure_wrapper_path \
    "${mdn_completion_closure_wrapper}"
  expect_kv "${mdn_completion_closure_receipt}" \
    closure_wrapper_process_start_sha256 \
    "${expected_mdn_completion_closure_wrapper_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" closure_wrapper_mode 0555
  expect_kv "${mdn_completion_closure_receipt}" erratum_path \
    "${mdn_completion_erratum}"
  expect_kv "${mdn_completion_closure_receipt}" erratum_sha256 \
    "${expected_mdn_completion_erratum_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" completion_correction_path \
    "${mdn_completion_correction}"
  expect_kv "${mdn_completion_closure_receipt}" completion_correction_sha256 \
    "${expected_mdn_completion_correction_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" \
    initial_loaded_publisher_is_current_path_hash_pair false
  expect_kv "${mdn_completion_closure_receipt}" \
    reconstructed_publisher_executable_authority false
  expect_kv "${mdn_completion_closure_receipt}" \
    receipt_emission_disk_sealer_path "${mdn_final_sealer}"
  expect_kv "${mdn_completion_closure_receipt}" \
    receipt_emission_disk_sealer_sha256 "${expected_mdn_final_sealer_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" \
    authoritative_adoption_sealer_path "${mdn_final_sealer}"
  expect_kv "${mdn_completion_closure_receipt}" \
    authoritative_adoption_sealer_sha256 "${expected_mdn_final_sealer_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" completion_result_path \
    "${mdn_result}"
  expect_kv "${mdn_completion_closure_receipt}" completion_result_schema_id \
    "${mdn_result_schema_id}"
  expect_kv "${mdn_completion_closure_receipt}" completion_result_sha256 \
    "${expected_mdn_result_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" \
    completion_result_alone_is_authority false
  expect_kv "${mdn_completion_closure_receipt}" payload_inventory_master_sha256 \
    "${expected_mdn_payload_inventory_master_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" mdn_checkpoint_path \
    "${checkpoint}"
  expect_kv "${mdn_completion_closure_receipt}" mdn_checkpoint_sha256 \
    "${expected_mdn_checkpoint_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" execution_runner_path \
    "${mdn_execution_runner}"
  expect_kv "${mdn_completion_closure_receipt}" execution_runner_sha256 \
    "${expected_mdn_execution_runner_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" content_pre_sha256 \
    "${expected_mdn_content_inventory_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" content_post_sha256 \
    "${expected_mdn_content_inventory_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" pre_post_content_identity_equal true
  expect_kv "${mdn_completion_closure_receipt}" content_identity_fields \
    sha256_bytes_mode_links_relative_path
  expect_kv "${mdn_completion_closure_receipt}" content_identity_excludes_ctime true
  expect_kv "${mdn_completion_closure_receipt}" \
    content_bytes_paths_inodes_unchanged true
  expect_kv "${mdn_completion_closure_receipt}" adoption_attempt_number 1
  expect_kv "${mdn_completion_closure_receipt}" \
    authoritative_seal_child_count 1
  expect_kv "${mdn_completion_closure_receipt}" \
    authoritative_verify_child_count 1
  expect_kv "${mdn_completion_closure_receipt}" adoption_log_sha256 \
    "${expected_mdn_completion_log_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" verify_log_sha256 \
    "${expected_mdn_completion_log_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" retry_directory_count 7
  expect_kv "${mdn_completion_closure_receipt}" retry_file_count 24
  expect_kv "${mdn_completion_closure_receipt}" retry_directory_mode 0555
  expect_kv "${mdn_completion_closure_receipt}" retry_file_mode 0444
  expect_kv "${mdn_completion_closure_receipt}" retry_file_links 1
  expect_kv "${mdn_completion_closure_receipt}" closure_directory_count 1
  expect_kv "${mdn_completion_closure_receipt}" closure_file_count 10
  expect_kv "${mdn_completion_closure_receipt}" closure_directory_mode 0555
  expect_kv "${mdn_completion_closure_receipt}" closure_file_mode 0444
  expect_kv "${mdn_completion_closure_receipt}" closure_file_links 1
  expect_kv "${mdn_completion_closure_receipt}" closure_lock_path \
    "${mdn_completion_closure_runtime}/.closure.lock"
  expect_kv "${mdn_completion_closure_receipt}" closure_lock_sha256 \
    "${expected_empty_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" closure_exact_tree true
  expect_kv "${mdn_completion_closure_receipt}" external_staging_absent true
  expect_kv "${mdn_completion_closure_receipt}" \
    wrapper_lock_separate_from_retry_execution_lock true
  expect_kv "${mdn_completion_closure_receipt}" \
    retry_execution_lock_held_by_wrapper false
  expect_kv "${mdn_completion_closure_receipt}" downstream_verifier_path \
    "${mdn_completion_closure_wrapper}"
  expect_kv "${mdn_completion_closure_receipt}" \
    downstream_verifier_process_start_sha256 \
    "${expected_mdn_completion_closure_wrapper_sha256}"
  expect_kv "${mdn_completion_closure_receipt}" downstream_required_mode --verify
  expect_kv "${mdn_completion_closure_receipt}" \
    downstream_must_bind_closure_receipt true
  expect_kv "${mdn_completion_closure_receipt}" \
    result_or_old_runner_alone_forbidden true
  expect_kv "${mdn_completion_closure_receipt}" authoritative_mutation \
    idempotent_chmod_only_may_advance_ctime
  expect_kv "${mdn_completion_closure_receipt}" \
    training_reexecution_authorized false
  expect_kv "${mdn_completion_closure_receipt}" canonical_data_raw_access false
  expect_kv "${mdn_completion_closure_receipt}" final_holdout_available false
  expect_kv "${mdn_completion_closure_receipt}" independent_final_evidence false
  expect_kv "${mdn_completion_closure_receipt}" policy_access false

  "${mdn_completion_closure_wrapper}" --verify >/dev/null
  printf '%s' "${checkpoint}"
}

emit_mdn_retry1_authority_bindings() {
  cat <<MDN_AUTHORITY
mdn_result_schema_id=${mdn_result_schema_id}
mdn_completion_closure_schema_id=${mdn_completion_closure_schema_id}
mdn_execution_runner_path=${mdn_execution_runner}
mdn_execution_runner_sha256=${expected_mdn_execution_runner_sha256}
mdn_completion_closure_wrapper_path=${mdn_completion_closure_wrapper}
mdn_completion_closure_wrapper_sha256=${expected_mdn_completion_closure_wrapper_sha256}
mdn_completion_closure_receipt_path=${mdn_completion_closure_receipt}
mdn_completion_closure_receipt_sha256=${expected_mdn_completion_closure_receipt_sha256}
mdn_completion_erratum_path=${mdn_completion_erratum}
mdn_completion_erratum_sha256=${expected_mdn_completion_erratum_sha256}
mdn_final_sealer_path=${mdn_final_sealer}
mdn_final_sealer_sha256=${expected_mdn_final_sealer_sha256}
mdn_completion_correction_path=${mdn_completion_correction}
mdn_completion_correction_sha256=${expected_mdn_completion_correction_sha256}
mdn_result_path=${mdn_result}
mdn_result_sha256=${expected_mdn_result_sha256}
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=${expected_mdn_checkpoint_sha256}
mdn_train_config_path=${mdn_train_config}
mdn_train_config_sha256=${expected_mdn_train_config_sha256}
mdn_result_alone_is_authority=false
mdn_completion_closure_verified=true
MDN_AUTHORITY
}

verify_mdn_retry1_authority_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" mdn_result_schema_id "${mdn_result_schema_id}"
  expect_kv "${receipt}" mdn_completion_closure_schema_id \
    "${mdn_completion_closure_schema_id}"
  expect_kv "${receipt}" mdn_execution_runner_path "${mdn_execution_runner}"
  expect_kv "${receipt}" mdn_execution_runner_sha256 \
    "${expected_mdn_execution_runner_sha256}"
  expect_kv "${receipt}" mdn_completion_closure_wrapper_path \
    "${mdn_completion_closure_wrapper}"
  expect_kv "${receipt}" mdn_completion_closure_wrapper_sha256 \
    "${expected_mdn_completion_closure_wrapper_sha256}"
  expect_kv "${receipt}" mdn_completion_closure_receipt_path \
    "${mdn_completion_closure_receipt}"
  expect_kv "${receipt}" mdn_completion_closure_receipt_sha256 \
    "${expected_mdn_completion_closure_receipt_sha256}"
  expect_kv "${receipt}" mdn_completion_erratum_path \
    "${mdn_completion_erratum}"
  expect_kv "${receipt}" mdn_completion_erratum_sha256 \
    "${expected_mdn_completion_erratum_sha256}"
  expect_kv "${receipt}" mdn_final_sealer_path "${mdn_final_sealer}"
  expect_kv "${receipt}" mdn_final_sealer_sha256 \
    "${expected_mdn_final_sealer_sha256}"
  expect_kv "${receipt}" mdn_completion_correction_path \
    "${mdn_completion_correction}"
  expect_kv "${receipt}" mdn_completion_correction_sha256 \
    "${expected_mdn_completion_correction_sha256}"
  expect_kv "${receipt}" mdn_result_path "${mdn_result}"
  expect_kv "${receipt}" mdn_result_sha256 "${expected_mdn_result_sha256}"
  expect_kv "${receipt}" mdn_checkpoint_path "${mdn_checkpoint}"
  expect_kv "${receipt}" mdn_checkpoint_sha256 \
    "${expected_mdn_checkpoint_sha256}"
  expect_kv "${receipt}" mdn_train_config_path "${mdn_train_config}"
  expect_kv "${receipt}" mdn_train_config_sha256 \
    "${expected_mdn_train_config_sha256}"
  expect_kv "${receipt}" mdn_result_alone_is_authority false
  expect_kv "${receipt}" mdn_completion_closure_verified true
}

verify_clean_training_inputs() {
  require_file "${source_verifier}"
  require_file "${representation_runner}"
  require_file "${affine_helper}"
  require_file "${affine_runner}"
  bash -n "${affine_runner}" || fail "clean affine runner has invalid shell syntax"
  require_file "${runtime_exec}"
  [[ -x "${runtime_exec}" ]] || fail "Runtime executable is not executable"
  [[ "$(sha256_of "${runtime_exec}")" == \
    "${expected_runtime_exec_sha256}" ]] || fail "Runtime executable hash drifted"
  verify_expected_document_hashes
  verify_source_closure
  "${representation_runner}" --verify >/dev/null
  representation_checkpoint="$(checkpoint_from_representation_result)"
  mdn_checkpoint="$(verify_mdn_retry1_authority)"
  expect_kv "${representation_result}" optimizer_steps 3000
  expect_kv "${representation_result}" config_snapshot_path \
    "${representation_train_config}"
  require_immutable_file "${representation_train_config}"
}

prepare_output_hierarchy() {
  local path
  ensure_fixed_directory "${runtime_parent}" true
  for path in "${runtime_root}" "${capture_config}" \
    "${untrained_mdn_policy}" "${untrained_capture_config}" \
    "${job_root}" "${train_job}" "${validation_job}" "${certified_job}" \
    "${untrained_job_root}" "${untrained_train_job}" \
    "${untrained_validation_job}" "${selection_source_root}" \
    "${input_receipt}" "${development_receipt}" \
    "${affine_route_trigger}" "${certified_attempt_receipt}" \
    "${result_receipt}" "${runtime_root}/.execution.lock" \
    "${runtime_root}/train.log" "${runtime_root}/validation.log" \
    "${runtime_root}/untrained_train.log" \
    "${runtime_root}/untrained_validation.log" \
    "${runtime_root}/certified.log"; do
    reject_data_raw_path "${path}"
    reject_old_runtime_path "${path}"
    reject_symlink_components "${path}"
    require_contained_path "${path}" "${runtime_root}"
  done
  ensure_fixed_directory "${runtime_root}"
  ensure_fixed_directory "${job_root}"
  ensure_fixed_directory "${untrained_job_root}"
  for path in "${runtime_root}" "${job_root}" "${untrained_job_root}"; do
    require_directory "${path}"
  done
}

derive_capture_config() {
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= cwu_02v_certified_replay_eval_mdn");
      replaced += 1;
    }
    { print }
    END { if (replaced != 1) exit 42 }
  ' "${mdn_train_config}"
}

derive_untrained_policy() {
  awk '
    /ALLOW_UNTRAINED_REPRESENTATION/ { exit 43 }
    /^[[:space:]]*SEED[[:space:]]*=/ {
      print "  SEED = 17;";
      seeded += 1;
      next;
    }
    /^[[:space:]]*};[[:space:]]*$/ && !inserted {
      print "  ALLOW_UNTRAINED_REPRESENTATION = true;";
      inserted += 1;
    }
    { print }
    END { if (inserted != 1 || seeded != 1) exit 42 }
  ' "${mdn_policy_source}"
}

derive_untrained_config() {
  awk -v path="${untrained_mdn_policy}" '
    BEGIN { replaced = 0 }
    /^[[:space:]]*wikimyei_inference_expected_value_mdn_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " path);
      replaced += 1;
    }
    { print }
    END { if (replaced != 1) exit 42 }
  ' "${capture_config}"
}

verify_capture_configs() {
  local path
  require_immutable_file "${capture_config}"
  require_immutable_file "${untrained_mdn_policy}"
  require_immutable_file "${untrained_capture_config}"
  cmp -s -- <(derive_capture_config) "${capture_config}" ||
    fail "trained capture config is not the deterministic clean-MDN derivation"
  cmp -s -- <(derive_untrained_policy) "${untrained_mdn_policy}" ||
    fail "untrained MDN policy is not the deterministic seed-17 derivation"
  cmp -s -- <(derive_untrained_config) "${untrained_capture_config}" ||
    fail "untrained capture config is not the deterministic trained derivation"
  expect_kv "${capture_config}" runtime_wave_id \
    cwu_02v_certified_replay_eval_mdn
  expect_kv "${capture_config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
  expect_kv "${capture_config}" \
    wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${mdn_policy_source}"
  expect_kv "${untrained_capture_config}" runtime_wave_id \
    cwu_02v_certified_replay_eval_mdn
  expect_kv "${untrained_capture_config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
  expect_kv "${untrained_capture_config}" \
    wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${untrained_mdn_policy}"
  expect_kv "${untrained_mdn_policy}" ALLOW_UNTRAINED_REPRESENTATION 'true;'
  expect_kv "${untrained_mdn_policy}" SEED '17;'
  expect_kv "${isolated_registry}" SOURCE_ROOT "${isolated_source_root};"
  for path in "${capture_config}" "${untrained_capture_config}" \
    "${isolated_registry}"; do
    ! grep -Fq '/data/raw' "${path}" ||
      fail "capture configuration references canonical data/raw: ${path}"
    ! grep -Fq '/data/final' "${path}" ||
      fail "capture configuration references final-holdout data: ${path}"
    ! grep -Eq 'synthetic_v2_(representation_train_v1|mdn_train_v1|frozen_feature_capture_v1)' \
      "${path}" || fail "capture configuration references quarantined runtime: ${path}"
    ! grep -Eq '(^|/)synthetic_v2_mdn_train_isolated_v2(/|$)' "${path}" ||
      fail "capture configuration references quarantined original non-retry MDN runtime: ${path}"
  done
}

write_capture_configs() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.capture_config.XXXXXX")"
  derive_capture_config >"${candidate}" || fail "could not derive capture config"
  publish_immutable_candidate "${candidate}" "${capture_config}" \
    "trained capture config"
  candidate="$(mktemp "${runtime_root}/.untrained_policy.XXXXXX")"
  derive_untrained_policy >"${candidate}" ||
    fail "could not derive untrained representation policy"
  publish_immutable_candidate "${candidate}" "${untrained_mdn_policy}" \
    "untrained representation policy"
  candidate="$(mktemp "${runtime_root}/.untrained_config.XXXXXX")"
  derive_untrained_config >"${candidate}" ||
    fail "could not derive untrained capture config"
  publish_immutable_candidate "${candidate}" "${untrained_capture_config}" \
    "untrained capture config"
  verify_capture_configs
}

verify_selection_sources() {
  local unexpected
  require_directory "${selection_source_root}"
  require_immutable_file "${frozen_affine_helper}"
  require_immutable_file "${frozen_affine_runner}"
  cmp -s -- "${affine_helper}" "${frozen_affine_helper}" ||
    fail "live affine helper differs from capture-frozen helper"
  cmp -s -- "${affine_runner}" "${frozen_affine_runner}" ||
    fail "live clean affine runner differs from capture-frozen runner"
  [[ -z "$(find "${selection_source_root}" -perm /222 -print -quit)" ]] ||
    fail "capture-frozen selection source tree is writable"
  unexpected="$(find "${selection_source_root}" -mindepth 1 \
    ! -type f -print -quit)"
  [[ -z "${unexpected}" ]] ||
    fail "unexpected non-regular entry in frozen selection source tree: ${unexpected}"
  local file_count
  file_count="$(find "${selection_source_root}" -type f -printf '.' | \
    wc -c | tr -d '[:space:]')"
  [[ "${file_count}" == 2 ]] ||
    fail "frozen selection source tree contains ${file_count} files instead of 2"
}

freeze_selection_sources() {
  if [[ -e "${selection_source_root}" || -L "${selection_source_root}" ]]; then
    verify_selection_sources
    return
  fi
  local candidate
  candidate="$(mktemp -d "${runtime_root}/.selection_sources.XXXXXXXX")"
  cp -- "${affine_helper}" \
    "${candidate}/frozen_representation_affine_probe.cpp"
  cp -- "${affine_runner}" \
    "${candidate}/run_frozen_representation_affine_probe_isolated_v2.sh"
  chmod 0444 -- "${candidate}"/*
  chmod 0555 -- "${candidate}"
  mv -T -n -- "${candidate}" "${selection_source_root}" ||
    fail "could not atomically freeze affine selection sources"
  [[ ! -e "${candidate}" ]] ||
    fail "frozen-source destination appeared concurrently; candidate retained"
  verify_selection_sources
}

array_contains() {
  local needle="$1"
  shift
  local item
  for item in "$@"; do
    [[ "${item}" == "${needle}" ]] && return 0
  done
  return 1
}

verify_source_receipts() {
  local manifest="$1" receipts receipt source relative edge interval
  local expected_receipt count=0
  declare -A seen=()
  receipts="$(kv_value "${manifest}" source_file_receipts)"
  [[ -n "${receipts}" ]] || fail "job manifest omitted source_file_receipts"
  [[ "${receipts}" != ';;'* && "${receipts}" != *';;' ]] ||
    fail "job manifest has an empty leading/trailing source receipt"
  [[ "${receipts}" != *"/data/raw"* ]] ||
    fail "job manifest contains a canonical data/raw receipt"
  [[ "${receipts}" != *"/data/final"* ]] ||
    fail "job manifest contains a final-holdout source receipt"
  while [[ -n "${receipts}" ]]; do
    if [[ "${receipts}" == *";;"* ]]; then
      receipt="${receipts%%;;*}"
      receipts="${receipts#*;;}"
    else
      receipt="${receipts}"
      receipts=""
    fi
    [[ "${receipt}" == *"|source="* ]] ||
      fail "malformed source receipt: ${receipt}"
    source="${receipt##*|source=}"
    reject_data_raw_path "${source}"
    reject_old_runtime_path "${source}"
    [[ "${source}" == "${isolated_source_root}/"* ]] ||
      fail "source receipt escapes isolated mirror: ${source}"
    reject_symlink_components "${source}"
    require_file "${source}"
    relative="${source#"${isolated_source_root}/"}"
    array_contains "${relative}" "${csv_relatives[@]}" ||
      fail "source receipt names an unexpected mirror file: ${source}"
    edge="${relative%%/*}"
    interval="${relative#*/}"
    interval="${interval%%/*}"
    expected_receipt="edge=${edge}|instrument=${edge}|interval=${interval}|record_type=kline|source=${source}"
    [[ "${receipt}" == "${expected_receipt}" ]] ||
      fail "source receipt metadata/path tuple is not exact: ${receipt}"
    [[ -z "${seen[${relative}]:-}" ]] ||
      fail "duplicate source receipt: ${source}"
    seen["${relative}"]=1
    count=$((count + 1))
  done
  [[ "${count}" == 9 ]] ||
    fail "job manifest contains ${count} unique source receipts instead of 9"
  for relative in "${csv_relatives[@]}"; do
    [[ -n "${seen[${relative}]:-}" ]] ||
      fail "job manifest omits isolated source receipt: ${relative}"
  done
}

validate_probe_file() {
  local probe="$1" begin="$2" end="$3" expected_rows="$4"
  local expected_schema="$5" expected_width="$6"
  require_file "${probe}"
  LC_ALL=C awk -F, -v begin="${begin}" -v end="${end}" \
    -v rows_expected="${expected_rows}" -v schema="${expected_schema}" \
    -v width="${expected_width}" '
    BEGIN {
      header = "record_schema,anchor_key,anchor_index,anchor_local_index," \
               "edge_index,edge_id,base_node_id,quote_node_id," \
               "channel_index,target_edge_close_return,feature_count," \
               "feature_values";
      integer = "^[0-9]+$";
      signed_integer = "^[+-]?[0-9]+$";
      number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
      edge_id[0] = "SYN2ALPHASYN2USD";
      edge_id[1] = "SYN2BETASYN2USD";
      edge_id[2] = "SYN2GAMMASYN2USD";
      base_id[0] = "SYN2ALPHA";
      base_id[1] = "SYN2BETA";
      base_id[2] = "SYN2GAMMA";
    }
    NR == 1 {
      if ($0 != header) exit 41;
      next;
    }
    {
      if (NF != 12 || $1 != schema || $11 !~ integer || $11 + 0 != width)
        exit 42;
      if ($2 !~ signed_integer || $3 !~ integer || $4 !~ integer ||
          $5 !~ integer || $9 !~ integer || $10 !~ number)
        exit 43;
      anchor = $3 + 0;
      anchor_key = $2 + 0;
      anchor_local = $4 + 0;
      edge = $5 + 0;
      channel = $9 + 0;
      if (anchor < begin || anchor >= end || edge < 0 || edge > 2 ||
          channel < 0 || channel > 2 || anchor_key <= 0 ||
          anchor_local < 0) exit 44;
      if ($6 != edge_id[edge] || $7 != base_id[edge] || $8 != "SYN2USD")
        exit 48;
      cell_count = split($12, cells, ";");
      if (cell_count != width) exit 49;
      for (cell = 1; cell <= cell_count; ++cell) {
        if (cells[cell] !~ number) exit 50;
      }
      key = anchor SUBSEP edge SUBSEP channel;
      if (++seen[key] != 1) exit 45;
      if (!(anchor in anchor_key_by_index)) {
        anchor_key_by_index[anchor] = anchor_key;
        anchor_local_by_index[anchor] = anchor_local;
        if (++anchor_key_seen[anchor_key] != 1) exit 51;
      } else if (anchor_key_by_index[anchor] != anchor_key ||
                 anchor_local_by_index[anchor] != anchor_local) {
        exit 52;
      }
      anchor_rows[anchor] += 1;
      rows += 1;
    }
    END {
      if (NR == 1 || rows != rows_expected) exit 46;
      for (anchor = begin; anchor < end; ++anchor) {
        if (anchor_rows[anchor] != 9) exit 47;
        if (anchor > begin &&
            anchor_key_by_index[anchor] <= anchor_key_by_index[anchor - 1])
          exit 53;
      }
    }
  ' "${probe}" || fail "probe schema/range/inventory mismatch: ${probe}"
}

validate_probe_pair_identity() {
  local context_probe="$1" representation_probe="$2"
  LC_ALL=C awk -F, '
    NR == FNR {
      if (FNR == 1) next;
      key = $3 SUBSEP $5 SUBSEP $9;
      signature = $2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10;
      context[key] = signature;
      next;
    }
    FNR == 1 { next }
    {
      key = $3 SUBSEP $5 SUBSEP $9;
      signature = $2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10;
      if (!(key in context) || context[key] != signature) exit 41;
      delete context[key];
    }
    END {
      for (key in context) exit 42;
    }
  ' "${context_probe}" "${representation_probe}" ||
    fail "context/representation probe coordinates or targets disagree"
}

verify_mdn_run_manifest_isolation() {
  local manifest="$1" key
  expect_kv "${manifest}" manifest_format kikijyeba.runtime.job_manifest.v1
  expect_kv "${manifest}" job_kind channel_inference_mdn
  expect_kv "${manifest}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest}" protocol_id cwu_02v
  expect_kv "${manifest}" protocol_kind channel_graph_first
  expect_kv "${manifest}" protocol_status active
  expect_kv "${manifest}" successor_protocol ''
  expect_kv "${manifest}" target_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest}" wave_mode 'run|debug'
  expect_kv "${manifest}" execution_chain \
    "${expected_mdn_run_execution_chain}"
  expect_kv "${manifest}" mutated_components ''
  expect_kv "${manifest}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  expect_kv "${manifest}" source_order_policy sequential
  expect_kv "${manifest}" source_order_policy_explicit false
  expect_kv "${manifest}" source_order_warning_level none
  expect_kv "${manifest}" source_order_warnings ''
  expect_kv "${manifest}" stream_plan "${expected_mdn_stream_plan}"
  expect_kv "${manifest}" representation_training_id \
    synthetic_continuous_graph_v2_mtf_jepa_mae_vicreg_train_core_v1
  expect_kv "${manifest}" inference_training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${manifest}" probe_sidecar_enabled true
  expect_kv "${manifest}" probe_record_schema \
    kikijyeba.runtime.job_events.probe_record.v1
  expect_kv "${manifest}" probe_stream_leaf runtime.job_events.probe

  local -a empty_downstream_policy_fields=(
    runtime_handoff_id runtime_handoff_digest marshal_target_driver_run_id
    policy_training_contract_schema policy_training_contract_digest
    policy_training_artifact_schema policy_operator_surface_contract_id
    policy_operator_surface_digest policy_family_id policy_dsl_digest
    policy_net_digest policy_input_feature_manifest_digest
    policy_jkimyei_digest target_node_universe_digest
    action_distribution_id action_distribution_config_digest
    reward_contract_id execution_profile_digest causal_schedule_digest
  )
  for key in "${empty_downstream_policy_fields[@]}"; do
    expect_kv "${manifest}" "${key}" ''
  done
}

validate_job() {
  local job="$1" begin="$2" end="$3" expected_rows="$4"
  local representation_checkpoint_arg="$5" mdn_checkpoint_arg="$6"
  local expected_config="$7"
  ((begin >= 0 && end <= accepted_anchor_count && begin < end)) ||
    fail "invalid authorized job range [${begin},${end})"
  ((end <= forbidden_final_begin)) || fail "job crosses final boundary"
  require_directory "${job}"
  local result="${job}/runtime.result.fact"
  local manifest="${job}/job.manifest"
  local report="${job}/channel_inference.report"
  local context_probe="${job}/mdn_edge_context_features.probe"
  local representation_probe="${job}/representation_edge_features.probe"
  require_file "${result}"
  require_file "${manifest}"
  require_file "${report}"
  require_file "${context_probe}"
  require_file "${representation_probe}"
  ! grep -Fq '/data/raw' "${manifest}" ||
    fail "job manifest references canonical data/raw"
  ! grep -Fq '/data/final' "${manifest}" ||
    fail "job manifest references final-holdout data"
  expect_kv "${result}" status completed
  expect_kv "${result}" optimizer_steps 0
  expect_kv "${result}" checkpoint_written false
  expect_kv "${result}" checkpoint_path ''
  expect_kv "${result}" model_state_mutated false
  expect_kv "${manifest}" config_path "${expected_config}"
  expect_kv "${manifest}" wave_id cwu_02v_certified_replay_eval_mdn
  expect_kv "${manifest}" wave_action run
  verify_mdn_run_manifest_isolation "${manifest}"
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin "${begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${end}"
  expect_kv "${manifest}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" skipped_outside_common_range 0
  expect_kv "${manifest}" skipped_missing_edge_coverage 0
  expect_kv "${manifest}" skipped_failed_fetch_probe 0
  expect_kv "${manifest}" duplicate_anchor_count 0
  expect_kv "${manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint_arg}"
  expect_kv "${manifest}" input_mdn_checkpoint_path "${mdn_checkpoint_arg}"
  verify_source_receipts "${manifest}"
  if [[ -n "${representation_checkpoint_arg}" ]]; then
    expect_kv "${report}" representation_checkpoint_loaded true
    expect_kv "${report}" representation_checkpoint_path \
      "${representation_checkpoint_arg}"
    expect_kv "${report}" seed 31
    expect_kv "${report}" allow_untrained_representation false
  else
    expect_kv "${report}" representation_checkpoint_loaded false
    expect_kv "${report}" representation_checkpoint_path ''
    expect_kv "${report}" seed 17
    expect_kv "${report}" allow_untrained_representation true
  fi
  expect_kv "${report}" mdn_checkpoint_loaded true
  expect_kv "${report}" mdn_checkpoint_path "${mdn_checkpoint_arg}"
  expect_kv "${report}" requested_anchor_index_begin "${begin}"
  expect_kv "${report}" requested_anchor_index_end "${end}"
  expect_kv "${report}" wave_streamed_anchor_count "$((end - begin))"
  expect_kv "${report}" mdn_edge_context_feature_probe_written true
  expect_kv "${report}" mdn_edge_context_feature_probe_row_count \
    "${expected_rows}"
  expect_kv "${report}" mdn_edge_context_feature_probe_path "${context_probe}"
  expect_kv "${report}" representation_edge_feature_probe_written true
  expect_kv "${report}" representation_edge_feature_probe_row_count \
    "${expected_rows}"
  expect_kv "${report}" representation_edge_feature_probe_path \
    "${representation_probe}"
  expect_kv "${report}" representation_edge_feature_probe_error ''
  expect_kv "${report}" edge_return_projection_valid_count "${expected_rows}"
  expect_kv "${report}" direct_edge_return_readout_valid_count "${expected_rows}"
  validate_probe_file "${context_probe}" "${begin}" "${end}" \
    "${expected_rows}" kikijyeba.synthetic.mdn_edge_context_feature_probe.v1 400
  validate_probe_file "${representation_probe}" "${begin}" "${end}" \
    "${expected_rows}" \
    kikijyeba.synthetic.representation_edge_feature_probe.v1 96
  validate_probe_pair_identity "${context_probe}" "${representation_probe}"
  if find "${job}" -mindepth 1 -iname '*replay*' -print -quit | grep -q .; then
    fail "capture job contains a replay artifact: ${job}"
  fi
}

seal_job_artifacts() {
  local job="$1" log="$2" symlink special
  require_directory "${job}"
  require_file "${log}"
  symlink="$(find "${job}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "capture job contains symlink: ${symlink}"
  special="$(find "${job}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "capture job contains special entry: ${special}"
  find "${job}" -type f -exec chmod 0444 -- {} +
  find "${job}" -depth -type d -exec chmod 0555 -- {} +
  chmod 0444 -- "${log}"
}

assert_job_artifacts_sealed() {
  local job="$1" log="$2" writable symlink special
  require_directory "${job}"
  require_immutable_file "${log}"
  symlink="$(find "${job}" -type l -print -quit)"
  [[ -z "${symlink}" ]] ||
    fail "sealed capture job contains symlink: ${symlink}"
  special="$(find "${job}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "sealed capture job contains special entry: ${special}"
  writable="$(find "${job}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "sealed capture job retains write bit: ${writable}"
}

run_job_once() {
  local job="$1" log="$2" begin="$3" end="$4"
  local representation_checkpoint_arg="$5" mdn_checkpoint_arg="$6"
  local config="$7"
  if [[ -e "${job}" || -L "${job}" ]]; then
    return
  fi
  [[ ! -e "${log}" && ! -L "${log}" ]] ||
    fail "capture log exists without a reusable complete job: ${log}"
  local -a command=(
    "${runtime_exec}"
    --config "${config}"
    --job-dir "${job}"
    --source-range anchor_index
    --anchor-index-begin "${begin}"
    --anchor-index-end "${end}"
  )
  if [[ -n "${representation_checkpoint_arg}" ]]; then
    command+=(--input-representation-checkpoint \
      "${representation_checkpoint_arg}")
  fi
  command+=(--input-mdn-checkpoint "${mdn_checkpoint_arg}" \
    --no-replay-artifacts)
  "${command[@]}" >"${log}" 2>&1 ||
    fail "capture failed irrecoverably for [${begin},${end}); see ${log}"
}

emit_input_receipt() {
  cat <<INPUTS
schema_id=${schema_id}.inputs.v1
status=complete
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
isolated_source_verifier_path=${source_verifier}
isolated_source_verifier_sha256=$(sha256_of "${source_verifier}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
isolated_source_root_path=${isolated_source_root}
isolated_registry_path=${isolated_registry}
representation_runner_path=${representation_runner}
representation_runner_sha256=$(sha256_of "${representation_runner}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
$(emit_mdn_retry1_authority_bindings)
mdn_policy_source_path=${mdn_policy_source}
mdn_policy_source_sha256=$(sha256_of "${mdn_policy_source}")
capture_config_path=${capture_config}
capture_config_sha256=$(sha256_of "${capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
affine_helper_source_path=${affine_helper}
affine_helper_source_sha256=$(sha256_of "${affine_helper}")
affine_runner_source_path=${affine_runner}
affine_runner_source_sha256=$(sha256_of "${affine_runner}")
frozen_affine_helper_path=${frozen_affine_helper}
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_path=${frozen_affine_runner}
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
affine_development_runtime=${affine_development_runtime}
affine_binary_expected_path=${affine_binary}
affine_development_status_expected_path=${affine_development_status}
affine_master_manifest_expected_path=${affine_master_manifest}
post384_validation_report_expected_path=${post384_report}
raw96_validation_report_expected_path=${raw96_report}
untrained_raw96_validation_report_expected_path=${untrained_raw96_report}
affine_route_trigger_path=${affine_route_trigger}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
train_expected_probe_rows=${train_rows}
validation_expected_probe_rows=${validation_rows}
certified_expected_probe_rows=${certified_rows}
purge_anchors_captured=false
certified_scoring_attempt_count=1
untrained_representation_certified_access=false
forbidden_final_anchor_begin=${forbidden_final_begin}
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
INPUTS
}

write_input_receipt() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
  emit_input_receipt >"${candidate}"
  publish_immutable_candidate "${candidate}" "${input_receipt}" \
    "capture input receipt"
}

verify_input_receipt() {
  require_immutable_file "${input_receipt}"
  cmp -s -- <(emit_input_receipt) "${input_receipt}" ||
    fail "capture input receipt differs from the complete fixed contract"
  expect_kv "${input_receipt}" schema_id "${schema_id}.inputs.v1"
  expect_kv "${input_receipt}" status complete
  verify_cursor_erratum_bindings "${input_receipt}"
  verify_mdn_retry1_authority_bindings "${input_receipt}"
  verify_document_bindings "${input_receipt}"
}

emit_job_bindings() {
  local prefix="$1" job="$2"
  cat <<JOB
${prefix}_job_dir=${job}
${prefix}_manifest_path=${job}/job.manifest
${prefix}_manifest_sha256=$(sha256_of "${job}/job.manifest")
${prefix}_runtime_result_path=${job}/runtime.result.fact
${prefix}_runtime_result_sha256=$(sha256_of "${job}/runtime.result.fact")
${prefix}_report_path=${job}/channel_inference.report
${prefix}_report_sha256=$(sha256_of "${job}/channel_inference.report")
${prefix}_context_probe_path=${job}/mdn_edge_context_features.probe
${prefix}_context_probe_sha256=$(sha256_of "${job}/mdn_edge_context_features.probe")
${prefix}_representation_probe_path=${job}/representation_edge_features.probe
${prefix}_representation_probe_sha256=$(sha256_of "${job}/representation_edge_features.probe")
JOB
}

verify_job_bindings() {
  local receipt="$1" prefix="$2" job="$3"
  local suffix path_key sha_key expected
  expect_kv "${receipt}" "${prefix}_job_dir" "${job}"
  for suffix in manifest runtime_result report context_probe representation_probe; do
    path_key="${prefix}_${suffix}_path"
    sha_key="${prefix}_${suffix}_sha256"
    case "${suffix}" in
    manifest) expected="${job}/job.manifest" ;;
    runtime_result) expected="${job}/runtime.result.fact" ;;
    report) expected="${job}/channel_inference.report" ;;
    context_probe) expected="${job}/mdn_edge_context_features.probe" ;;
    representation_probe) expected="${job}/representation_edge_features.probe" ;;
    esac
    expect_kv "${receipt}" "${path_key}" "${expected}"
    require_immutable_file "${expected}"
    expect_kv "${receipt}" "${sha_key}" "$(sha256_of "${expected}")"
  done
}

assert_certified_artifacts_absent() {
  local path
  for path in "${certified_job}" "${certified_attempt_receipt}" \
    "${result_receipt}" "${runtime_root}/certified.log"; do
    [[ ! -e "${path}" && ! -L "${path}" ]] ||
      fail "certified artifact is forbidden in development-only state: ${path}"
  done
}

emit_development_receipt() {
  cat <<DEVELOPMENT
schema_id=${schema_id}.development.v1
status=complete
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
isolated_source_root_path=${isolated_source_root}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
capture_config_path=${capture_config}
capture_config_sha256=$(sha256_of "${capture_config}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
$(emit_mdn_retry1_authority_bindings)
frozen_affine_helper_path=${frozen_affine_helper}
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_path=${frozen_affine_runner}
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
affine_development_runtime=${affine_development_runtime}
affine_binary_expected_path=${affine_binary}
affine_development_status_expected_path=${affine_development_status}
affine_master_manifest_expected_path=${affine_master_manifest}
post384_validation_report_expected_path=${post384_report}
raw96_validation_report_expected_path=${raw96_report}
untrained_raw96_validation_report_expected_path=${untrained_raw96_report}
affine_route_trigger_path=${affine_route_trigger}
$(emit_job_bindings trained_train "${train_job}")
$(emit_job_bindings trained_validation "${validation_job}")
$(emit_job_bindings untrained_train "${untrained_train_job}")
$(emit_job_bindings untrained_validation "${untrained_validation_job}")
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
maximum_anchor_read=$((validation_end - 1))
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_capture_created=false
certified_attempt_created=false
certified_result_created=false
purge_anchors_captured=false
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
DEVELOPMENT
}

write_development_receipt() {
  assert_certified_artifacts_absent
  local candidate
  candidate="$(mktemp "${runtime_root}/.development.XXXXXX")"
  emit_development_receipt >"${candidate}"
  publish_immutable_candidate "${candidate}" "${development_receipt}" \
    "development capture receipt"
}

verify_development_receipt() {
  require_immutable_file "${development_receipt}"
  cmp -s -- <(emit_development_receipt) "${development_receipt}" ||
    fail "development receipt differs from the complete fixed contract"
  expect_kv "${development_receipt}" schema_id "${schema_id}.development.v1"
  expect_kv "${development_receipt}" status complete
  expect_kv "${development_receipt}" maximum_anchor_read \
    "$((validation_end - 1))"
  expect_kv "${development_receipt}" certified_capture_created false
  expect_kv "${development_receipt}" certified_attempt_created false
  expect_kv "${development_receipt}" certified_result_created false
  expect_kv "${development_receipt}" canonical_data_raw_access false
  expect_kv "${development_receipt}" final_holdout_access false
  expect_kv "${development_receipt}" policy_access false
  verify_cursor_erratum_bindings "${development_receipt}"
  verify_mdn_retry1_authority_bindings "${development_receipt}"
  verify_document_bindings "${development_receipt}"
  verify_job_bindings "${development_receipt}" trained_train "${train_job}"
  verify_job_bindings "${development_receipt}" trained_validation \
    "${validation_job}"
  verify_job_bindings "${development_receipt}" untrained_train \
    "${untrained_train_job}"
  verify_job_bindings "${development_receipt}" untrained_validation \
    "${untrained_validation_job}"
}

verify_development_core() {
  verify_clean_training_inputs
  verify_capture_configs
  verify_selection_sources
  verify_input_receipt
  validate_job "${train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${capture_config}"
  validate_job "${validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${representation_checkpoint}" \
    "${mdn_checkpoint}" "${capture_config}"
  validate_job "${untrained_train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" '' "${mdn_checkpoint}" "${untrained_capture_config}"
  validate_job "${untrained_validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" '' "${mdn_checkpoint}" \
    "${untrained_capture_config}"
  validate_probe_pair_identity \
    "${train_job}/representation_edge_features.probe" \
    "${untrained_train_job}/representation_edge_features.probe"
  validate_probe_pair_identity \
    "${validation_job}/representation_edge_features.probe" \
    "${untrained_validation_job}/representation_edge_features.probe"
  assert_job_artifacts_sealed "${train_job}" "${runtime_root}/train.log"
  assert_job_artifacts_sealed "${validation_job}" \
    "${runtime_root}/validation.log"
  assert_job_artifacts_sealed "${untrained_train_job}" \
    "${runtime_root}/untrained_train.log"
  assert_job_artifacts_sealed "${untrained_validation_job}" \
    "${runtime_root}/untrained_validation.log"
  verify_development_receipt
}

verify_sha256_manifest() {
  local manifest="$1" root="$2" line path resolved count=0
  declare -A seen=()
  require_immutable_file "${manifest}"
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" =~ ^[0-9a-f]{64}[[:space:]][\ \*](.+)$ ]] ||
      fail "malformed SHA-256 manifest line in ${manifest}"
    path="${BASH_REMATCH[1]}"
    [[ -n "${path}" && "${path}" != *$'\n'* ]] ||
      fail "empty manifest path in ${manifest}"
    [[ -z "${seen[${path}]:-}" ]] ||
      fail "duplicate path in SHA-256 manifest ${manifest}: ${path}"
    seen["${path}"]=1
    reject_data_raw_path "${path}"
    reject_old_runtime_path "${path}"
    [[ "${path}" != *"/../"* && "${path}" != ../* && \
      "${path}" != */.. ]] || fail "parent traversal in ${manifest}: ${path}"
    if [[ "${path}" == /* ]]; then
      resolved="${path}"
      case "${resolved}" in
      "${runtime_root}"/* | "${affine_development_runtime}"/* | \
        "${source_closure}" | "${fresh_preregistration}" | \
        "${diagnostic_preregistration}" | "${protocol_amendment}" | \
        "${localization_addendum}" | "${conditional_amendment}" | \
        "${ablation_preregistration}" | "${source_isolation_amendment}" | \
        "${isolated_source_protocol}" | "${staged_hardening}" | \
        "${cursor_alignment_correction}" | "${affine_helper}" | \
        "${cursor_alignment_metadata_erratum}" | \
        "${cursor_erratum_receipt}" | "${cursor_erratum_verifier}" | \
        "${source_verifier}" | "${script_path}" | \
        "${affine_runner}" | "${runtime_exec}") ;;
      *) fail "affine manifest absolute path is outside its fixed provenance set: ${resolved}" ;;
      esac
    else
      resolved="${root}/${path}"
    fi
    require_file "${resolved}"
    count=$((count + 1))
  done <"${manifest}"
  ((count > 0)) || fail "empty SHA-256 manifest: ${manifest}"
  (
    cd "${root}"
    sha256sum -c -- "${manifest}" >/dev/null
  ) || fail "SHA-256 manifest verification failed: ${manifest}"
}

verify_affine_development_inventory() {
  local relative branch file_id suffix actual_count directory_count
  local -a expected=(
    bin/frozen_representation_affine_probe
    compiler.identity
    linked_libraries.txt
    execution_contract.status
    input_manifest.sha256
    source_manifest.sha256
    binary.sha256
    output_manifest.sha256
    master.sha256
    development.status
    frozen_sources/frozen_representation_affine_probe.cpp
    frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh
  )
  for branch in main replay; do
    for file_id in "${post384_report_file_id}" "${raw96_report_file_id}" \
      "${untrained_raw96_report_file_id}"; do
      for suffix in report stdout.log; do
        expected+=("${branch}/${file_id}.${suffix}")
      done
    done
  done
  actual_count="$(find "${affine_development_runtime}" -type f -printf . | \
    wc -c | tr -d '[:space:]')"
  [[ "${actual_count}" == "${#expected[@]}" ]] ||
    fail "affine development runtime has ${actual_count} files instead of ${#expected[@]}"
  for relative in "${expected[@]}"; do
    require_immutable_file "${affine_development_runtime}/${relative}"
  done
  directory_count="$(find "${affine_development_runtime}" -type d -printf . | \
    wc -c | tr -d '[:space:]')"
  [[ "${directory_count}" == 5 ]] ||
    fail "affine development runtime has ${directory_count} directories instead of 5"
  for relative in bin frozen_sources main replay; do
    require_directory "${affine_development_runtime}/${relative}"
  done
}

verify_affine_master_manifest() {
  require_directory "${affine_development_runtime}"
  verify_affine_development_inventory
  local manifest line path symlink special count=0
  declare -A master_seen=()
  for manifest in input_manifest.sha256 source_manifest.sha256 binary.sha256 \
    output_manifest.sha256 master.sha256; do
    verify_sha256_manifest "${affine_development_runtime}/${manifest}" \
      "${affine_development_runtime}"
  done
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" =~ ^[0-9a-f]{64}[[:space:]][\ \*](.+)$ ]] ||
      fail "malformed affine master manifest"
    path="${BASH_REMATCH[1]}"
    case "${path}" in
    input_manifest.sha256 | source_manifest.sha256 | binary.sha256 | \
      output_manifest.sha256 | compiler.identity | linked_libraries.txt) ;;
    *) fail "unexpected affine master-manifest entry: ${path}" ;;
    esac
    [[ -z "${master_seen[${path}]:-}" ]] ||
      fail "duplicate affine master-manifest entry: ${path}"
    master_seen["${path}"]=1
    count=$((count + 1))
  done <"${affine_master_manifest}"
  [[ "${count}" == 6 ]] ||
    fail "affine master manifest has ${count} entries instead of 6"
  symlink="$(find "${affine_development_runtime}" -type l -print -quit)"
  [[ -z "${symlink}" ]] ||
    fail "affine development runtime contains symlink: ${symlink}"
  special="$(find "${affine_development_runtime}" -mindepth 1 \
    ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] ||
    fail "affine development runtime contains special entry: ${special}"
  [[ -z "$(find "${affine_development_runtime}" -perm /222 -print -quit)" ]] ||
    fail "affine development runtime is not immutable"
}

validate_development_affine_report() {
  local report="$1" expected_schema="$2" expected_probe_kind="$3"
  local expected_record_schema expected_probe_width expected_affine_width
  local expected_excluded expected_classification
  case "${expected_probe_kind}" in
  mdn_context)
    expected_record_schema="kikijyeba.synthetic.mdn_edge_context_feature_probe.v1"
    expected_probe_width=400
    expected_affine_width=384
    expected_excluded=16
    ;;
  representation)
    expected_record_schema="kikijyeba.synthetic.representation_edge_feature_probe.v1"
    expected_probe_width=96
    expected_affine_width=96
    expected_excluded=0
    ;;
  *) fail "unsupported affine development probe kind: ${expected_probe_kind}" ;;
  esac
  if [[ "${expected_schema}" == "${untrained_raw96_report_schema_id}" ]]; then
    expected_classification=untrained_representation_validation_control
  else
    expected_classification=development_selection_complete
  fi
  require_immutable_file "${report}"
  expect_kv "${report}" schema_id "${expected_schema}"
  expect_kv "${report}" status complete
  expect_kv "${report}" benchmark_id synthetic_continuous_graph_v2
  expect_kv "${report}" probe_kind "${expected_probe_kind}"
  expect_kv "${report}" probe_record_schema "${expected_record_schema}"
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows 0
  expect_kv "${report}" probe_rows_total "$((train_rows + validation_rows))"
  expect_kv "${report}" probe_ranges_disjoint true
  expect_kv "${report}" fit_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${report}" validation_anchor_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${report}" certified_anchor_range not_opened
  expect_kv "${report}" purge_anchors_used false
  expect_kv "${report}" certified_candidates_scored 0
  expect_kv "${report}" maximum_anchor_read "$((validation_end - 1))"
  expect_kv "${report}" final_holdout_begin "${forbidden_final_begin}"
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" refit_after_selection false
  expect_kv "${report}" probe_feature_width "${expected_probe_width}"
  expect_kv "${report}" affine_feature_width "${expected_affine_width}"
  expect_kv "${report}" edge_identity_feature_width_excluded \
    "${expected_excluded}"
  expect_kv "${report}" certified_strong_gate_pass not_evaluated
  expect_kv "${report}" certified_partial_gate_pass not_evaluated
  expect_kv "${report}" classification "${expected_classification}"
}

verify_affine_development_status() {
  require_immutable_file "${affine_development_status}"
  require_immutable_file "${affine_binary}"
  [[ -x "${affine_binary}" ]] || fail "affine probe binary is not executable"
  verify_affine_master_manifest
  validate_development_affine_report "${post384_report}" \
    "${post384_report_schema_id}" mdn_context
  validate_development_affine_report "${raw96_report}" \
    "${raw96_report_schema_id}" representation
  validate_development_affine_report "${untrained_raw96_report}" \
    "${untrained_raw96_report_schema_id}" representation
  expect_kv "${affine_development_status}" schema_id \
    "${affine_development_schema_id}"
  expect_kv "${affine_development_status}" status complete
  expect_kv "${affine_development_status}" capture_development_path \
    "${development_receipt}"
  expect_kv "${affine_development_status}" capture_development_sha256 \
    "$(sha256_of "${development_receipt}")"
  expect_kv "${affine_development_status}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${affine_development_status}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  verify_cursor_erratum_bindings "${affine_development_status}"
  expect_kv "${affine_development_status}" capture_runner_path "${script_path}"
  expect_kv "${affine_development_status}" capture_runner_sha256 \
    "$(sha256_of "${script_path}")"
  expect_kv "${affine_development_status}" affine_runner_path \
    "${frozen_affine_runner}"
  expect_kv "${affine_development_status}" affine_runner_sha256 \
    "$(sha256_of "${frozen_affine_runner}")"
  expect_kv "${affine_development_status}" affine_helper_path \
    "${frozen_affine_helper}"
  expect_kv "${affine_development_status}" affine_helper_sha256 \
    "$(sha256_of "${frozen_affine_helper}")"
  expect_kv "${affine_development_status}" affine_binary_path "${affine_binary}"
  expect_kv "${affine_development_status}" affine_binary_sha256 \
    "$(sha256_of "${affine_binary}")"
  expect_kv "${affine_development_status}" affine_master_manifest_path \
    "${affine_master_manifest}"
  expect_kv "${affine_development_status}" affine_master_manifest_sha256 \
    "$(sha256_of "${affine_master_manifest}")"
  expect_kv "${affine_development_status}" post384_validation_report_path \
    "${post384_report}"
  expect_kv "${affine_development_status}" post384_validation_report_sha256 \
    "$(sha256_of "${post384_report}")"
  expect_kv "${affine_development_status}" raw96_validation_report_path \
    "${raw96_report}"
  expect_kv "${affine_development_status}" raw96_validation_report_sha256 \
    "$(sha256_of "${raw96_report}")"
  expect_kv "${affine_development_status}" \
    untrained_raw96_validation_report_path "${untrained_raw96_report}"
  expect_kv "${affine_development_status}" \
    untrained_raw96_validation_report_sha256 \
    "$(sha256_of "${untrained_raw96_report}")"
  expect_kv "${affine_development_status}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${affine_development_status}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${affine_development_status}" maximum_available_anchor \
    "${maximum_anchor_index}"
  expect_kv "${affine_development_status}" train_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${affine_development_status}" validation_anchor_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${affine_development_status}" certified_development_anchor_range \
    "[${certified_begin},${certified_end})"
  expect_kv "${affine_development_status}" train_probe_rows "${train_rows}"
  expect_kv "${affine_development_status}" validation_probe_rows \
    "${validation_rows}"
  expect_kv "${affine_development_status}" certified_development_probe_rows \
    "${certified_rows}"
  expect_kv "${affine_development_status}" maximum_anchor_read \
    "$((validation_end - 1))"
  expect_kv "${affine_development_status}" certified_input_access false
  expect_kv "${affine_development_status}" certified_candidates_scored 0
  expect_kv "${affine_development_status}" \
    untrained_representation_certified_access false
  expect_kv "${affine_development_status}" canonical_data_raw_access false
  expect_kv "${affine_development_status}" final_holdout_access false
  expect_kv "${affine_development_status}" final_holdout_scored false
  expect_kv "${affine_development_status}" independent_final_evidence false
  expect_kv "${affine_development_status}" policy_access false
  verify_document_bindings "${affine_development_status}"
}

numeric_gate_boolean() {
  local value="$1" comparison="$2" threshold="$3"
  LC_ALL=C awk -v value="${value}" -v comparison="${comparison}" \
    -v threshold="${threshold}" '
    BEGIN {
      number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
      if (value !~ number) exit 42;
      if (comparison == "ge")
        print (value + 0 >= threshold + 0) ? "true" : "false";
      else if (comparison == "le")
        print (value + 0 <= threshold + 0) ? "true" : "false";
      else exit 43;
    }
  ' || fail "invalid numeric gate input: ${value} ${comparison} ${threshold}"
}

load_raw96_route() {
  direction_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_report}" selected.validation.directional_accuracy)" \
    ge 0.95)"
  rank_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_report}" selected.validation.pairwise_rank_accuracy)" \
    ge 0.95)"
  correlation_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_report}" selected.validation.correlation)" ge 0.95)"
  rmse_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_report}" selected.validation.rmse_target_rms_ratio)" \
    le 0.25)"
  strong_gate=false
  if [[ "${direction_gate}" == true && "${rank_gate}" == true && \
    "${correlation_gate}" == true && "${rmse_gate}" == true ]]; then
    strong_gate=true
  fi
  expect_kv "${raw96_report}" validation_strong_gate_pass "${strong_gate}"
  if [[ "${strong_gate}" == true ]]; then
    expected_route=canonical_certification
  else
    expected_route=representation_ablation_screen
  fi
}

verify_affine_route_trigger() {
  verify_affine_development_status
  load_raw96_route
  require_immutable_file "${affine_route_trigger}"
  expect_kv "${affine_route_trigger}" schema_id "${affine_trigger_schema_id}"
  expect_kv "${affine_route_trigger}" status complete
  expect_kv "${affine_route_trigger}" route "${expected_route}"
  expect_kv "${affine_route_trigger}" maximum_anchor_read \
    "$((validation_end - 1))"
  expect_kv "${affine_route_trigger}" capture_development_path \
    "${development_receipt}"
  expect_kv "${affine_route_trigger}" capture_development_sha256 \
    "$(sha256_of "${development_receipt}")"
  expect_kv "${affine_route_trigger}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${affine_route_trigger}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  verify_cursor_erratum_bindings "${affine_route_trigger}"
  expect_kv "${affine_route_trigger}" capture_runner_path "${script_path}"
  expect_kv "${affine_route_trigger}" capture_runner_sha256 \
    "$(sha256_of "${script_path}")"
  expect_kv "${affine_route_trigger}" affine_runner_path \
    "${frozen_affine_runner}"
  expect_kv "${affine_route_trigger}" affine_runner_sha256 \
    "$(sha256_of "${frozen_affine_runner}")"
  expect_kv "${affine_route_trigger}" affine_helper_path \
    "${frozen_affine_helper}"
  expect_kv "${affine_route_trigger}" affine_helper_sha256 \
    "$(sha256_of "${frozen_affine_helper}")"
  expect_kv "${affine_route_trigger}" affine_binary_path "${affine_binary}"
  expect_kv "${affine_route_trigger}" affine_binary_sha256 \
    "$(sha256_of "${affine_binary}")"
  expect_kv "${affine_route_trigger}" affine_development_receipt_path \
    "${affine_development_status}"
  expect_kv "${affine_route_trigger}" affine_development_receipt_sha256 \
    "$(sha256_of "${affine_development_status}")"
  expect_kv "${affine_route_trigger}" affine_master_manifest_path \
    "${affine_master_manifest}"
  expect_kv "${affine_route_trigger}" affine_master_manifest_sha256 \
    "$(sha256_of "${affine_master_manifest}")"
  expect_kv "${affine_route_trigger}" post384_validation_report_path \
    "${post384_report}"
  expect_kv "${affine_route_trigger}" post384_validation_report_sha256 \
    "$(sha256_of "${post384_report}")"
  expect_kv "${affine_route_trigger}" raw96_validation_report_path \
    "${raw96_report}"
  expect_kv "${affine_route_trigger}" raw96_validation_report_sha256 \
    "$(sha256_of "${raw96_report}")"
  expect_kv "${affine_route_trigger}" \
    untrained_raw96_validation_report_path "${untrained_raw96_report}"
  expect_kv "${affine_route_trigger}" \
    untrained_raw96_validation_report_sha256 \
    "$(sha256_of "${untrained_raw96_report}")"
  expect_kv "${affine_route_trigger}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${affine_route_trigger}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${affine_route_trigger}" maximum_available_anchor \
    "${maximum_anchor_index}"
  expect_kv "${affine_route_trigger}" \
    raw96_validation_direction_gate_pass "${direction_gate}"
  expect_kv "${affine_route_trigger}" \
    raw96_validation_pairwise_rank_gate_pass "${rank_gate}"
  expect_kv "${affine_route_trigger}" \
    raw96_validation_correlation_gate_pass "${correlation_gate}"
  expect_kv "${affine_route_trigger}" \
    raw96_validation_rmse_ratio_gate_pass "${rmse_gate}"
  expect_kv "${affine_route_trigger}" raw96_validation_strong_gate_pass \
    "${strong_gate}"
  expect_kv "${affine_route_trigger}" certified_capture_opened false
  expect_kv "${affine_route_trigger}" canonical_data_raw_access false
  expect_kv "${affine_route_trigger}" final_holdout_access false
  expect_kv "${affine_route_trigger}" final_holdout_scored false
  expect_kv "${affine_route_trigger}" independent_final_evidence false
  expect_kv "${affine_route_trigger}" policy_access false
  verify_document_bindings "${affine_route_trigger}"
}

require_canonical_certification_route() {
  verify_affine_route_trigger
  if [[ "${expected_route}" == representation_ablation_screen ]]; then
    assert_certified_artifacts_absent
    fail "route=representation_ablation_screen permanently forbids canonical certified capture"
  fi
  [[ "${expected_route}" == canonical_certification ]] ||
    fail "canonical certification route was not authorized"
}

emit_certified_attempt_receipt() {
  cat <<ATTEMPT
schema_id=${schema_id}.certified_attempt.v1
status=authorized_and_started
immutable_mode=0444
attempt_ordinal=1
certified_anchor_begin=${certified_begin}
certified_anchor_end_exclusive=${certified_end}
certified_anchor_range=[${certified_begin},${certified_end})
certified_expected_probe_rows=${certified_rows}
job_dir=${certified_job}
route=canonical_certification
capture_runner_path=${script_path}
capture_runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
capture_development_path=${development_receipt}
capture_development_sha256=$(sha256_of "${development_receipt}")
affine_route_trigger_path=${affine_route_trigger}
affine_route_trigger_sha256=$(sha256_of "${affine_route_trigger}")
capture_config_path=${capture_config}
capture_config_sha256=$(sha256_of "${capture_config}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
$(emit_mdn_retry1_authority_bindings)
affine_binary_path=${affine_binary}
affine_binary_sha256=$(sha256_of "${affine_binary}")
affine_helper_source_path=${frozen_affine_helper}
affine_helper_source_sha256=$(sha256_of "${frozen_affine_helper}")
affine_runner_source_path=${frozen_affine_runner}
affine_runner_source_sha256=$(sha256_of "${frozen_affine_runner}")
affine_development_status_path=${affine_development_status}
affine_development_status_sha256=$(sha256_of "${affine_development_status}")
affine_master_manifest_path=${affine_master_manifest}
affine_master_manifest_sha256=$(sha256_of "${affine_master_manifest}")
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
purge_anchors_captured=false
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
ATTEMPT
}

publish_certified_attempt() {
  [[ ! -e "${certified_attempt_receipt}" && \
    ! -L "${certified_attempt_receipt}" ]] ||
    fail "certified attempt has already been consumed"
  local candidate
  candidate="$(mktemp "${runtime_root}/.certified_attempt.XXXXXX")"
  emit_certified_attempt_receipt >"${candidate}"
  publish_immutable_candidate "${candidate}" "${certified_attempt_receipt}" \
    "certified-attempt receipt"
  sync -f "${certified_attempt_receipt}"
  sync -f "${runtime_root}"
}

verify_certified_attempt() {
  require_immutable_file "${certified_attempt_receipt}"
  [[ "$(stat -c '%a' -- "${certified_attempt_receipt}")" == 444 ]] ||
    fail "certified-attempt receipt mode is not exactly 0444"
  cmp -s -- <(emit_certified_attempt_receipt) \
    "${certified_attempt_receipt}" ||
    fail "certified-attempt receipt differs from its complete fixed contract"
  expect_kv "${certified_attempt_receipt}" schema_id \
    "${schema_id}.certified_attempt.v1"
  expect_kv "${certified_attempt_receipt}" status authorized_and_started
  expect_kv "${certified_attempt_receipt}" immutable_mode 0444
  expect_kv "${certified_attempt_receipt}" attempt_ordinal 1
  expect_kv "${certified_attempt_receipt}" certified_anchor_begin \
    "${certified_begin}"
  expect_kv "${certified_attempt_receipt}" certified_anchor_end_exclusive \
    "${certified_end}"
  expect_kv "${certified_attempt_receipt}" job_dir "${certified_job}"
  expect_kv "${certified_attempt_receipt}" route canonical_certification
  expect_kv "${certified_attempt_receipt}" canonical_data_raw_access false
  expect_kv "${certified_attempt_receipt}" final_holdout_access false
  expect_kv "${certified_attempt_receipt}" policy_access false
  verify_cursor_erratum_bindings "${certified_attempt_receipt}"
  verify_mdn_retry1_authority_bindings "${certified_attempt_receipt}"
  verify_document_bindings "${certified_attempt_receipt}"
}

run_certified_job_once() {
  local log="${runtime_root}/certified.log"
  require_canonical_certification_route
  if [[ -e "${certified_attempt_receipt}" || \
    -L "${certified_attempt_receipt}" ]]; then
    verify_certified_attempt
    [[ -e "${certified_job}" && -e "${log}" ]] ||
      fail "certified attempt is consumed without a complete reusable job; retry forbidden"
    return
  fi
  [[ ! -e "${certified_job}" && ! -L "${certified_job}" && \
    ! -e "${log}" && ! -L "${log}" ]] ||
    fail "certified job/log exists without its attempt receipt; capture forbidden"
  publish_certified_attempt
  run_job_once "${certified_job}" "${log}" "${certified_begin}" \
    "${certified_end}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${capture_config}"
}

emit_result_receipt() {
  cat <<RESULT
schema_id=${schema_id}.result.v1
status=complete
route=canonical_certification
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
development_receipt_path=${development_receipt}
development_receipt_sha256=$(sha256_of "${development_receipt}")
affine_route_trigger_path=${affine_route_trigger}
affine_route_trigger_sha256=$(sha256_of "${affine_route_trigger}")
certified_attempt_receipt_path=${certified_attempt_receipt}
certified_attempt_receipt_sha256=$(sha256_of "${certified_attempt_receipt}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
isolated_source_root_path=${isolated_source_root}
capture_config_path=${capture_config}
capture_config_sha256=$(sha256_of "${capture_config}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
$(emit_mdn_retry1_authority_bindings)
affine_binary_path=${affine_binary}
affine_binary_sha256=$(sha256_of "${affine_binary}")
affine_helper_source_path=${frozen_affine_helper}
affine_helper_source_sha256=$(sha256_of "${frozen_affine_helper}")
affine_runner_source_path=${frozen_affine_runner}
affine_runner_source_sha256=$(sha256_of "${frozen_affine_runner}")
affine_development_status_path=${affine_development_status}
affine_development_status_sha256=$(sha256_of "${affine_development_status}")
affine_master_manifest_path=${affine_master_manifest}
affine_master_manifest_sha256=$(sha256_of "${affine_master_manifest}")
$(emit_job_bindings trained_train "${train_job}")
$(emit_job_bindings trained_validation "${validation_job}")
$(emit_job_bindings untrained_train "${untrained_train_job}")
$(emit_job_bindings untrained_validation "${untrained_validation_job}")
$(emit_job_bindings certified "${certified_job}")
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_probe_rows=${certified_rows}
maximum_anchor_scored=$((certified_end - 1))
purge_anchors_captured=false
certified_scoring_attempt_count=1
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
RESULT
}

write_result_receipt() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  emit_result_receipt >"${candidate}"
  publish_immutable_candidate "${candidate}" "${result_receipt}" \
    "completed capture receipt"
}

verify_result_receipt() {
  require_immutable_file "${result_receipt}"
  cmp -s -- <(emit_result_receipt) "${result_receipt}" ||
    fail "result receipt differs from the complete fixed contract"
  expect_kv "${result_receipt}" schema_id "${schema_id}.result.v1"
  expect_kv "${result_receipt}" status complete
  expect_kv "${result_receipt}" route canonical_certification
  expect_kv "${result_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${result_receipt}" maximum_anchor_scored \
    "$((certified_end - 1))"
  expect_kv "${result_receipt}" certified_scoring_attempt_count 1
  expect_kv "${result_receipt}" canonical_data_raw_access false
  expect_kv "${result_receipt}" final_holdout_access false
  expect_kv "${result_receipt}" policy_access false
  verify_cursor_erratum_bindings "${result_receipt}"
  verify_mdn_retry1_authority_bindings "${result_receipt}"
  verify_document_bindings "${result_receipt}"
  verify_job_bindings "${result_receipt}" trained_train "${train_job}"
  verify_job_bindings "${result_receipt}" trained_validation \
    "${validation_job}"
  verify_job_bindings "${result_receipt}" untrained_train \
    "${untrained_train_job}"
  verify_job_bindings "${result_receipt}" untrained_validation \
    "${untrained_validation_job}"
  verify_job_bindings "${result_receipt}" certified "${certified_job}"
}

verify_development_completed() {
  assert_certified_artifacts_absent
  verify_development_core
  if [[ -e "${affine_route_trigger}" || -L "${affine_route_trigger}" ]]; then
    verify_affine_route_trigger
    if [[ "${expected_route}" == representation_ablation_screen ]]; then
      assert_certified_artifacts_absent
    fi
  fi
  echo "development_receipt_path=${development_receipt}"
  echo "maximum_anchor_read=$((validation_end - 1))"
  echo "certified_capture_created=false"
}

verify_completed() {
  verify_development_core
  require_canonical_certification_route
  verify_certified_attempt
  validate_job "${certified_job}" "${certified_begin}" "${certified_end}" \
    "${certified_rows}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${capture_config}"
  assert_job_artifacts_sealed "${certified_job}" \
    "${runtime_root}/certified.log"
  verify_result_receipt
  "${cursor_erratum_verifier}" --verify >/dev/null
  echo "certified_representation_probe_path=${certified_job}/representation_edge_features.probe"
  echo "certified_context_probe_path=${certified_job}/mdn_edge_context_features.probe"
  echo "maximum_anchor_scored=$((certified_end - 1))"
}

if [[ "${mode}" == --verify-development ]]; then
  verify_development_completed
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  verify_completed
  exit 0
fi

((certified_end <= accepted_anchor_count)) ||
  fail "certified range exceeds isolated source domain"
((certified_end <= forbidden_final_begin)) || fail "capture crosses final boundary"
verify_clean_training_inputs
prepare_output_hierarchy
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || fail "another isolated capture holds the execution lock"

if [[ "${mode}" == --run-development ]]; then
  assert_certified_artifacts_absent
  if [[ -e "${development_receipt}" || -L "${development_receipt}" ]]; then
    verify_development_completed
    exit 0
  fi
  write_capture_configs
  freeze_selection_sources
  write_input_receipt

  run_job_once "${untrained_train_job}" \
    "${runtime_root}/untrained_train.log" "${train_begin}" "${train_end}" '' \
    "${mdn_checkpoint}" "${untrained_capture_config}"
  validate_job "${untrained_train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" '' "${mdn_checkpoint}" "${untrained_capture_config}"
  seal_job_artifacts "${untrained_train_job}" \
    "${runtime_root}/untrained_train.log"

  run_job_once "${untrained_validation_job}" \
    "${runtime_root}/untrained_validation.log" "${validation_begin}" \
    "${validation_end}" '' "${mdn_checkpoint}" "${untrained_capture_config}"
  validate_job "${untrained_validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" '' "${mdn_checkpoint}" \
    "${untrained_capture_config}"
  seal_job_artifacts "${untrained_validation_job}" \
    "${runtime_root}/untrained_validation.log"

  run_job_once "${train_job}" "${runtime_root}/train.log" "${train_begin}" \
    "${train_end}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${capture_config}"
  validate_job "${train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${capture_config}"
  seal_job_artifacts "${train_job}" "${runtime_root}/train.log"

  run_job_once "${validation_job}" "${runtime_root}/validation.log" \
    "${validation_begin}" "${validation_end}" "${representation_checkpoint}" \
    "${mdn_checkpoint}" "${capture_config}"
  validate_job "${validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${representation_checkpoint}" \
    "${mdn_checkpoint}" "${capture_config}"
  seal_job_artifacts "${validation_job}" "${runtime_root}/validation.log"

  write_development_receipt
  verify_development_completed
  exit 0
fi

[[ "${mode}" == --run-certified ]] || fail "unreachable mode: ${mode}"
if [[ -e "${result_receipt}" || -L "${result_receipt}" ]]; then
  verify_completed
  exit 0
fi
verify_development_core
require_canonical_certification_route
run_certified_job_once
validate_job "${certified_job}" "${certified_begin}" "${certified_end}" \
  "${certified_rows}" "${representation_checkpoint}" "${mdn_checkpoint}" \
  "${capture_config}"
seal_job_artifacts "${certified_job}" "${runtime_root}/certified.log"
verify_certified_attempt
write_result_receipt
verify_completed
