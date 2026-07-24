#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1"
readonly prior_failed_schema_id="synthetic_v2_representation_ablation_isolated_v2"
readonly failure_closure_schema_id="synthetic_v2_representation_ablation_isolated_v2_prejob_failure_closure_v1"
readonly expected_recovery_amendment_sha256="7e2e71579c444c5190d824f5963d6cef3f66dc6203b0edd2a3bdd9f9c3cd9088"
readonly expected_failure_closure_verifier_sha256="236555266b85643aa297f399e8e2fd89434e49c14ac9903837e2d690fa1b050e"
readonly expected_failure_closure_receipt_sha256="a57c006eb7b9e627bb6459da0ee79f5f15345ac74c2feb40dfdc3ce4cd5cec50"
readonly expected_canonical_mtf_net_bnf_sha256="26f1d105ec04945024ac91806fc4206e21d81429c3a190782b7159af69d2e0a3"
readonly expected_preregistration_sha256="6a4175f431347387f33c250b747f1f34c29099aaf4b3c94a75ea2e4960cef6cd"
readonly expected_conditional_amendment_sha256="30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67"
readonly expected_source_isolation_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_isolated_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_alignment_erratum_verifier_sha256="e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99"
readonly expected_cursor_alignment_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
readonly expected_cursor_alignment_erratum_receipt_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly cursor_alignment_erratum_schema_id="synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1"
readonly expected_source_verifier_sha256="dca034ec2440c7ab9caa936dee965879fe4cbd48ca29fdd6432e62f73af1cf05"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_fresh_preregistration_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly expected_diagnostic_preregistration_sha256="de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8"
readonly expected_diagnostic_amendment_sha256="3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5"
readonly expected_localization_addendum_sha256="2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4"
readonly expected_data_closure_sha256="36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_capture_runner_sha256="bb5f8fe728d81a71d6a8f603ef85686bdb70ae6c52c4ea6890f466d466a1cb32"
readonly expected_capture_development_sha256="fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6"
readonly expected_scientific_affine_runner_sha256="ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173"
readonly expected_operational_affine_runner_sha256="008e45996da402a61a4aea8765a6922997cb35de605cd21d9fc255afefa5345d"
readonly expected_affine_helper_sha256="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly expected_cuda_correction_sha256="d9c88f5c37771678016799afb157ec7661e3016eb58cdb4321da67b3329358ce"
readonly expected_route_trigger_sha256="bbaa1e5f6e81741569fc905f3e4601b7495c7b9e281581de5bba7d617b7a1860"
readonly expected_affine_development_status_sha256="bf90ba9ef353c6b93aced12a98636351e2380b509454486517ab47f5c8372c06"
readonly expected_affine_master_manifest_sha256="2619295b834762cb3b914e3ba8f06b9b423fae5be0a09dcd95bad31505563a2b"
readonly expected_affine_binary_sha256="733841623165e1be1dbf76e82264022292b5c16825211696800fd5876cddad3f"
readonly expected_affine_execution_contract_sha256="e3fa7542d637cb012b89d6170ac5ef79d498be4ac0eee5ab1b137e70c79e4486"
readonly expected_raw96_report_sha256="e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e"
readonly expected_post384_report_sha256="0fa614b8a2407fce11de1bb7dd1c083f5485e7bb7b2a0af5341831a60d230cea"
readonly expected_untrained_report_sha256="cd3c0a028e0609369b4b71669f70bd96c29df662a9014dc0ba99d05e0c7d4cd1"
readonly mdn_result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"
readonly mdn_completion_closure_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.completion_concurrency_closure.v1"
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
readonly expected_operational_affine_runner_inode="32369622321839121"
readonly expected_operational_affine_runner_device="66"
readonly expected_operational_affine_runner_bytes="102742"
readonly expected_operational_affine_runner_owner="0"
readonly expected_steps=3000
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
readonly tie_tolerance="1e-12"
readonly canonical_training_id="synthetic_continuous_graph_v2_mtf_jepa_mae_vicreg_train_core_v1"

readonly -a all_arms=(canonical endpoint_scale time_only no_tf_alignment)
readonly -a challenger_arms=(endpoint_scale time_only no_tf_alignment)
readonly -a effective_grammar_data_keys=(
  wikimyei_expression_nodelift_srl_dsl_path
  wikimyei_representation_vicreg_dsl_path
  wikimyei_representation_vicreg_net_path
  wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path
  wikimyei_representation_mtf_jepa_mae_vicreg_net_path
  wikimyei_inference_expected_value_mdn_dsl_path
  wikimyei_inference_expected_value_mdn_net_path
  wikimyei_observer_belief_dsl_path
  wikimyei_policy_portfolio_spot_distributional_utility_dsl_path
  wikimyei_policy_portfolio_graph_node_allocation_dsl_path
  wikimyei_policy_portfolio_graph_node_allocation_net_path
  wikimyei_policy_portfolio_graph_node_allocation_features_path
  ujcamei_source_cursor_dsl_path
  kikijyeba_environment_replay_dsl_path
)

fail() {
  echo "v2 representation ablation retry1: $*" >&2
  exit 1
}

reject_forbidden_path() {
  local path="$1"
  case "${path}" in
  */data/raw | */data/raw/* | */data/final | */data/final/*)
    fail "canonical raw/final data path is forbidden: ${path}"
    ;;
  *synthetic_v2_representation_train_v1* | \
    *synthetic_v2_mdn_train_v1* | \
    *synthetic_v2_frozen_feature_capture_v1* | \
    *synthetic_v2_frozen_affine_development_v1* | \
    */synthetic_v2_mdn_train_isolated_v2/*)
    fail "quarantined pre-isolation or non-retry path is forbidden: ${path}"
    ;;
  */synthetic_v2_representation_ablation_isolated_v2 | \
    */synthetic_v2_representation_ablation_isolated_v2/*)
    fail "failed pre-job ablation runtime is forbidden as a scientific input: ${path}"
    ;;
  esac
}

require_file() {
  local path="$1"
  reject_forbidden_path "${path}"
  reject_symlink_components "${path}"
  [[ -f "${path}" && ! -L "${path}" ]] ||
    fail "missing, non-regular, or symlinked file: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "file path is not canonical: ${path}"
}

require_nonempty_file() {
  require_file "$1"
  [[ -s "$1" ]] || fail "empty required file: $1"
}

require_immutable_file() {
  require_nonempty_file "$1"
  [[ "$(stat -c '%A' -- "$1")" != *w* ]] ||
    fail "required immutable file is writable: $1"
}

require_dir() {
  local path="$1"
  reject_forbidden_path "${path}"
  reject_symlink_components "${path}"
  [[ -d "${path}" && ! -L "${path}" ]] ||
    fail "missing or symlinked directory: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "directory path is not canonical: ${path}"
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

# Snapshot the operational source before any non-trivial work. Plan mode may
# describe a mutable candidate; every non-plan mode must bind to this fixed
# identity and must run only from the final single-linked 0555 source.
script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
readonly process_owner_uid="$(id -u)"
readonly process_start_runner_sha256="$(sha256_of "${script_path}")"
readonly process_start_runner_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_runner_device="$(stat -c '%d' -- "${script_path}")"
readonly process_start_runner_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_runner_owner="$(stat -c '%u' -- "${script_path}")"

expect_mode_owner_links() {
  local path="$1" expected_mode="$2" label="$3"
  require_file "${path}"
  [[ "$(stat -c '%a' -- "${path}")" == "${expected_mode}" ]] ||
    fail "${label} mode is not exactly 0${expected_mode}: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
  [[ "$(stat -c '%u' -- "${path}")" == "${process_owner_uid}" ]] ||
    fail "${label} is not owned by the executing uid: ${path}"
}

assert_operational_runner_identity() {
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == "${process_start_runner_sha256}" ]] ||
    fail "operational ablation runner changed after process start"
  [[ "$(stat -c '%i' -- "${script_path}")" == "${process_start_runner_inode}" ]] ||
    fail "operational ablation runner inode changed after process start"
  [[ "$(stat -c '%d' -- "${script_path}")" == "${process_start_runner_device}" ]] ||
    fail "operational ablation runner device changed after process start"
  [[ "$(stat -c '%s' -- "${script_path}")" == "${process_start_runner_bytes}" ]] ||
    fail "operational ablation runner size changed after process start"
  [[ "$(stat -c '%u' -- "${script_path}")" == "${process_start_runner_owner}" ]] ||
    fail "operational ablation runner owner changed after process start"
  [[ "${process_start_runner_owner}" == "${process_owner_uid}" ]] ||
    fail "operational ablation runner was not owned by the executing uid at process start"
  expect_mode_owner_links "${script_path}" 555 "operational ablation runner"
}

run_guarded_child() {
  local child_status
  assert_operational_runner_identity
  if "$@"; then
    child_status=0
  else
    child_status=$?
  fi
  assert_operational_runner_identity
  return "${child_status}"
}

kv() {
  local key="$1"
  local path="$2"
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
  actual="$(kv "${key}" "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

bound_file() {
  local receipt="$1"
  local path_key="$2"
  local sha_key="$3"
  local path
  path="$(kv "${path_key}" "${receipt}")"
  require_nonempty_file "${path}"
  expect_kv "${receipt}" "${sha_key}" "$(sha256_of "${path}")"
  printf '%s' "${path}"
}

bound_exact_file() {
  local receipt="$1"
  local path_key="$2"
  local sha_key="$3"
  local expected_path="$4"
  expect_kv "${receipt}" "${path_key}" "${expected_path}"
  require_nonempty_file "${expected_path}"
  expect_kv "${receipt}" "${sha_key}" "$(sha256_of "${expected_path}")"
}

verify_document_binding() {
  local receipt="$1"
  local key_prefix="$2"
  local expected_path="$3"
  bound_exact_file "${receipt}" "${key_prefix}_path" \
    "${key_prefix}_sha256" "${expected_path}"
}

require_contained_path() {
  local path="$1" root="$2"
  [[ "${path}" == "${root}" || "${path}" == "${root}/"* ]] ||
    fail "path escapes fixed runtime root ${root}: ${path}"
}

publish_immutable() {
  local candidate="$1"
  local destination="$2"
  assert_operational_runner_identity
  require_contained_path "${candidate}" "${runtime_root}"
  require_contained_path "${destination}" "${runtime_root}"
  require_file "${candidate}"
  reject_symlink_components "${destination}"
  chmod 0444 -- "${candidate}"
  if [[ -e "${destination}" || -L "${destination}" ]]; then
    require_immutable_file "${destination}"
    expect_mode_owner_links "${destination}" 444 \
      "immutable published artifact"
    cmp -s -- "${candidate}" "${destination}" ||
      fail "immutable artifact drifted: ${destination}"
    rm -f -- "${candidate}"
  else
    assert_operational_runner_identity
    ln -- "${candidate}" "${destination}" ||
      fail "could not publish immutable artifact: ${destination}"
    assert_operational_runner_identity
    rm -f -- "${candidate}"
  fi
  require_immutable_file "${destination}"
  expect_mode_owner_links "${destination}" 444 \
    "immutable published artifact"
  assert_operational_runner_identity
}

numeric_gate() {
  local value="$1"
  local comparison="$2"
  local threshold="$3"
  LC_ALL=C awk -v value="${value}" -v comparison="${comparison}" \
    -v threshold="${threshold}" '
    BEGIN {
      number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
      if (value !~ number) exit 42;
      if (comparison == "ge") {
        print (value + 0 >= threshold + 0) ? "true" : "false";
      } else if (comparison == "le") {
        print (value + 0 <= threshold + 0) ? "true" : "false";
      } else {
        exit 43;
      }
    }
  ' || fail "invalid numeric gate: value=${value}, comparison=${comparison}"
}

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${runtime_parent}/${schema_id}"
prior_failed_runtime="${runtime_parent}/${prior_failed_schema_id}"
failure_closure_runtime="${runtime_parent}/${failure_closure_schema_id}"

preregistration="${benchmark_root}/REPRESENTATION_ABLATION_PREREGISTRATION.md"
recovery_amendment="${script_dir}/REPRESENTATION_ABLATION_PREJOB_CONFIG_PATH_RECOVERY_AMENDMENT.md"
failure_closure_verifier="${script_dir}/seal_and_verify_representation_ablation_prejob_failure_closure_v1.sh"
failure_closure_receipt="${failure_closure_runtime}/failure_closure.status"
failure_regular_files_inventory="${failure_closure_runtime}/regular_files.inventory.tsv"
failure_directories_inventory="${failure_closure_runtime}/directories.inventory.tsv"
conditional_amendment="${benchmark_root}/REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md"
source_isolation_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
isolated_source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_alignment_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
fresh_preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
diagnostic_preregistration="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md"
diagnostic_amendment="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md"
localization_addendum="${benchmark_root}/REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md"
closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
isolated_source_verifier="${script_dir}/prepare_and_seal_isolated_development_source_v2.sh"
cursor_alignment_erratum_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
capture_runner="${script_dir}/run_frozen_feature_capture_isolated_v2.sh"
scientific_affine_runner="${script_dir}/run_frozen_representation_affine_probe_isolated_v2.sh"
operational_affine_runner="${script_dir}/run_frozen_representation_affine_probe_isolated_v2_cuda_canonical.sh"
affine_runner="${scientific_affine_runner}"
cuda_correction="${benchmark_root}/AFFINE_CUDA_CANONICAL_PATH_CORRECTION.md"
representation_runner="${script_dir}/run_representation_train_isolated_v2.sh"
helper_source="${script_dir}/frozen_representation_affine_probe.cpp"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"

isolated_source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
isolated_source_closure="${isolated_source_runtime}/development_source_closure.status"
cursor_alignment_erratum_receipt="${isolated_source_runtime}/cursor_alignment_erratum.status"
isolated_source_root="${isolated_source_runtime}/source"
isolated_registry="${isolated_source_runtime}/config/ujcamei.source.registry.development_prefix.dsl"
isolated_base_config="${isolated_source_runtime}/config/synthetic_benchmark.isolated_development.config"
canonical_capture_runtime="${runtime_parent}/synthetic_v2_frozen_feature_capture_isolated_v2"
canonical_capture_development="${canonical_capture_runtime}/development.status"
route_trigger="${canonical_capture_runtime}/affine_route_trigger.status"
canonical_capture_result="${canonical_capture_runtime}/result.status"
canonical_capture_config="${canonical_capture_runtime}/synthetic_benchmark.frozen_feature_capture.isolated.config"
canonical_untrained_capture_config="${canonical_capture_runtime}/synthetic_benchmark.untrained_representation_capture.isolated.config"
canonical_untrained_mdn_policy="${canonical_capture_runtime}/wikimyei.inference.expected_value.mdn.untrained_control.isolated.jkimyei"
canonical_capture_train_job="${canonical_capture_runtime}/jobs/train"
canonical_capture_validation_job="${canonical_capture_runtime}/jobs/validation"
canonical_untrained_train_job="${canonical_capture_runtime}/untrained_jobs/train"
canonical_untrained_validation_job="${canonical_capture_runtime}/untrained_jobs/validation"
canonical_frozen_helper="${canonical_capture_runtime}/frozen_selection_sources/frozen_representation_affine_probe.cpp"
canonical_frozen_affine_runner="${canonical_capture_runtime}/frozen_selection_sources/run_frozen_representation_affine_probe_isolated_v2.sh"
canonical_affine_development="${runtime_parent}/synthetic_v2_frozen_affine_development_isolated_v2"
canonical_affine_development_status="${canonical_affine_development}/development.status"
canonical_affine_master_manifest="${canonical_affine_development}/master.sha256"
canonical_affine_binary="${canonical_affine_development}/bin/frozen_representation_affine_probe"
canonical_affine_execution_contract="${canonical_affine_development}/execution_contract.status"
canonical_raw96_report="${canonical_affine_development}/main/synthetic_v2_frozen_encoder_affine_development_isolated_v2.report"
canonical_raw96_replay_report="${canonical_affine_development}/replay/synthetic_v2_frozen_encoder_affine_development_isolated_v2.report"
canonical_post384_report="${canonical_affine_development}/main/synthetic_v2_frozen_representation_affine_development_isolated_v2.report"
canonical_post384_replay_report="${canonical_affine_development}/replay/synthetic_v2_frozen_representation_affine_development_isolated_v2.report"
canonical_untrained_report="${canonical_affine_development}/main/synthetic_v2_untrained_encoder_affine_control_isolated_v2.report"
canonical_untrained_replay_report="${canonical_affine_development}/replay/synthetic_v2_untrained_encoder_affine_control_isolated_v2.report"
canonical_affine_final="${runtime_parent}/synthetic_v2_frozen_representation_affine_probe_isolated_v2/result.status"
canonical_representation_result="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/result.status"
mdn_execution_runner="${script_dir}/run_mdn_train_isolated_v2_retry1.sh"
mdn_completion_closure_wrapper="${script_dir}/seal_and_verify_mdn_retry1_completion_concurrency_closure.sh"
mdn_completion_erratum="${benchmark_root}/MDN_RETRY1_SEAL_CONCURRENCY_ERRATUM.md"
mdn_final_sealer="${script_dir}/seal_and_verify_mdn_retry1_completed_job.sh"
mdn_completion_correction="${benchmark_root}/MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md"
mdn_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2_retry1"
canonical_mdn_result="${mdn_runtime}/result.status"
mdn_checkpoint="${mdn_runtime}/job/channel_inference.report.channel_mdn.pt"
mdn_train_config="${mdn_runtime}/synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
mdn_completion_closure_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"
mdn_completion_closure_receipt="${mdn_completion_closure_runtime}/completion_concurrency.status"
mdn_policy_source="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
base_training_config="${runtime_root}/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config"
canonical_policy="${benchmark_root}/wikimyei.representation.mtf_jepa_mae_vicreg.v2.jkimyei"
canonical_net="${repo_root}/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.net"
canonical_mtf_net_bnf="${repo_root}/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf"

frozen_root="${runtime_root}/frozen_sources"
frozen_runner="${frozen_root}/run_representation_ablation_v2_retry1.sh"
frozen_helper="${frozen_root}/frozen_representation_affine_probe.cpp"
frozen_binary="${frozen_root}/frozen_representation_affine_probe"
arms_root="${runtime_root}/arms"
config_closure="${runtime_root}/config_inputs.status"
effective_grammar_closure="${runtime_root}/effective_grammar_closure.status"
input_receipt="${runtime_root}/inputs.status"
retry_attempt_sentinel="${runtime_root}/development.retry1.attempt.status"
development_receipt="${runtime_root}/development.status"
selection_receipt="${runtime_root}/selection.status"
certified_attempt="${runtime_root}/certified.attempt.status"
certified_job="${runtime_root}/certified/job"
certified_capture_status="${runtime_root}/certified/capture.status"
certified_capture_log="${runtime_root}/certified/capture.log"
certified_main_report="${runtime_root}/certified/main/synthetic_v2_frozen_encoder_affine_probe_isolated_v2.report"
certified_replay_report="${runtime_root}/certified/replay/synthetic_v2_frozen_encoder_affine_probe_isolated_v2.report"
certified_main_log="${runtime_root}/certified/main/synthetic_v2_frozen_encoder_affine_probe_isolated_v2.stdout.log"
certified_replay_log="${runtime_root}/certified/replay/synthetic_v2_frozen_encoder_affine_probe_isolated_v2.stdout.log"
result_receipt="${runtime_root}/result.status"
canonical_import_receipt="${arms_root}/canonical/import.status"

mode="${1:---plan}"
[[ "$#" -le 1 ]] ||
  fail "usage: $0 [--plan|--preflight|--run-development|--verify-development|--run-certified|--verify]"
case "${mode}" in
--plan | --preflight | --run-development | --verify-development | --run-certified | --verify) ;;
--run)
  fail "unconditional --run is disabled; use --run-development and only then --run-certified"
  ;;
*)
  fail "usage: $0 [--plan|--preflight|--run-development|--verify-development|--run-certified|--verify]"
  ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
operational_ablation_runner_path=${script_path}
operational_ablation_runner_sha256=${process_start_runner_sha256}
operational_ablation_runner_process_start_inode=${process_start_runner_inode}
operational_ablation_runner_process_start_device=${process_start_runner_device}
operational_ablation_runner_process_start_bytes=${process_start_runner_bytes}
operational_ablation_runner_process_start_owner_uid=${process_start_runner_owner}
operational_ablation_runner_required_mode=0555
operational_ablation_runner_required_links=1
activation_trigger=${route_trigger}
activation_trigger_sha256=${expected_route_trigger_sha256}
required_route=representation_ablation_screen
capture_runner=${capture_runner}
capture_runner_sha256=${expected_capture_runner_sha256}
capture_development=${canonical_capture_development}
capture_development_sha256=${expected_capture_development_sha256}
scientific_affine_runner=${scientific_affine_runner}
scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}
operational_affine_runner=${operational_affine_runner}
operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}
cuda_canonical_path_correction=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
affine_development_status_sha256=${expected_affine_development_status_sha256}
affine_master_manifest_sha256=${expected_affine_master_manifest_sha256}
affine_binary_sha256=${expected_affine_binary_sha256}
affine_execution_contract=${canonical_affine_execution_contract}
affine_execution_contract_sha256=${expected_affine_execution_contract_sha256}
raw96_validation_report_sha256=${expected_raw96_report_sha256}
post384_validation_report_sha256=${expected_post384_report_sha256}
untrained_raw96_validation_report_sha256=${expected_untrained_report_sha256}
mdn_retry1_result=${canonical_mdn_result}
mdn_retry1_result_sha256=${expected_mdn_result_sha256}
mdn_retry1_execution_runner=${mdn_execution_runner}
mdn_retry1_execution_runner_sha256=${expected_mdn_execution_runner_sha256}
mdn_completion_closure_wrapper=${mdn_completion_closure_wrapper}
mdn_completion_closure_wrapper_sha256=${expected_mdn_completion_closure_wrapper_sha256}
mdn_completion_closure_receipt=${mdn_completion_closure_receipt}
mdn_completion_closure_receipt_sha256=${expected_mdn_completion_closure_receipt_sha256}
isolated_source_closure=${isolated_source_closure}
isolated_source_closure_sha256=${expected_source_closure_sha256}
isolated_source_verifier=${isolated_source_verifier}
isolated_source_verifier_sha256=${expected_source_verifier_sha256}
isolated_source_root=${isolated_source_root}
isolated_base_config=${isolated_base_config}
base_training_config=${base_training_config}
base_training_config_derivation=isolated_base_config_with_exact_runtime_wave_substitution
recovery_amendment=${recovery_amendment}
recovery_amendment_sha256=${expected_recovery_amendment_sha256}
prejob_failure_closure_schema_id=${failure_closure_schema_id}
prejob_failure_closure_verifier=${failure_closure_verifier}
prejob_failure_closure_verifier_sha256=${expected_failure_closure_verifier_sha256}
prejob_failure_closure_receipt=${failure_closure_receipt}
prejob_failure_closure_receipt_sha256=${expected_failure_closure_receipt_sha256}
prior_failed_runtime=${prior_failed_runtime}
prior_failed_runtime_scientific_input=false
canonical_mtf_net_bnf_path=${canonical_mtf_net_bnf}
canonical_mtf_net_bnf_sha256=${expected_canonical_mtf_net_bnf_sha256}
effective_grammar_config_count=6
effective_grammar_key_count_per_config=14
effective_grammar_tuple_count=84
runtime_dry_run_preflight_jobs=0
retry_one_shot_sentinel=${retry_attempt_sentinel}
runtime_wave_id=train_core_mtf_jepa_mae_vicreg
cursor_alignment_correction=${cursor_alignment_correction}
cursor_alignment_erratum_verifier=${cursor_alignment_erratum_verifier}
cursor_alignment_erratum_verifier_sha256=${expected_cursor_alignment_erratum_verifier_sha256}
cursor_alignment_metadata_erratum=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=${expected_cursor_alignment_metadata_erratum_sha256}
cursor_alignment_erratum_receipt=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_alignment_erratum_receipt_sha256}
cursor_alignment_erratum_schema_id=${cursor_alignment_erratum_schema_id}
authoritative_accepted_anchor_count=3261
authoritative_candidate_anchor_count=3261
authoritative_maximum_anchor_index=3260
canonical_data_raw_access=false
canonical_arm_source=existing_immutable_development_checkpoint_and_probes
challenger_arms=endpoint_scale,time_only,no_tf_alignment
challenger_seed=17
challenger_optimizer_steps=${expected_steps}
endpoint_scale_only_diff=TIME_SCALES:8,16,32,64->8,16,32,1
time_only_only_diff=USE_FREQUENCY_TOKENS:true->false
no_tf_alignment_only_diff=LAMBDA_TF_ALIGN:0.10->0.00
reverse_substitution_cmp_required=true
development_capture_ranges=[${train_begin},${train_end}),[${validation_begin},${validation_end})
development_certified_access=false
development_affine_mode=development-only
development_main_replay_byte_identical=true
cross_arm_selection_order=direction,rank,correlation,rmse
cross_arm_tie_tolerance=${tie_tolerance}
cross_arm_tie_preference=canonical,endpoint_scale,time_only,no_tf_alignment
immutable_selection_before_certified=true
certified_evaluator_selection_lock_required=true
certified_selection_lock_verified_before_probe_open=true
certified_capture_range=[${certified_begin},${certified_end})
certified_probe_rows=${certified_rows}
selected_arm_certified_attempts=1
runner_up_certified_retries=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

assert_operational_runner_identity

verify_pinned_file() {
  local path="$1" expected_sha256="$2" label="$3"
  require_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
  [[ "$(stat -c '%u' -- "${path}")" == "${process_owner_uid}" ]] ||
    fail "${label} is not owned by the executing uid: ${path}"
  [[ "$(sha256_of "${path}")" == "${expected_sha256}" ]] ||
    fail "${label} hash drifted: ${path}"
}

verify_pinned_mode_file() {
  local path="$1" expected_sha256="$2" expected_mode="$3" label="$4"
  verify_pinned_file "${path}" "${expected_sha256}" "${label}"
  [[ "$(stat -c '%a' -- "${path}")" == "${expected_mode}" ]] ||
    fail "${label} mode is not exactly 0${expected_mode}: ${path}"
}

require_resolved_sha256_pin() {
  local value="$1" label="$2"
  [[ "${value}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "${label} SHA-256 pin is unresolved: ${value}"
}

emit_recovery_authority_bindings() {
  cat <<RECOVERY_AUTHORITY
recovery_amendment_path=${recovery_amendment}
recovery_amendment_sha256=${expected_recovery_amendment_sha256}
prejob_failure_closure_schema_id=${failure_closure_schema_id}
prejob_failure_closure_verifier_path=${failure_closure_verifier}
prejob_failure_closure_verifier_sha256=${expected_failure_closure_verifier_sha256}
prejob_failure_closure_receipt_path=${failure_closure_receipt}
prejob_failure_closure_receipt_sha256=${expected_failure_closure_receipt_sha256}
prejob_failure_regular_files_inventory_path=${failure_regular_files_inventory}
prejob_failure_regular_files_inventory_sha256=$(sha256_of "${failure_regular_files_inventory}")
prejob_failure_directories_inventory_path=${failure_directories_inventory}
prejob_failure_directories_inventory_sha256=$(sha256_of "${failure_directories_inventory}")
prior_failed_runtime_path=${prior_failed_runtime}
prior_failed_runtime_scientific_input=false
prejob_failure_job_created=false
prejob_failure_optimizer_steps=0
prejob_failure_checkpoint_created=false
prejob_failure_source_data_rows_read=false
prejob_failure_model_metric_exposed=false
recovery_scope=config_path_only
RECOVERY_AUTHORITY
}

verify_recovery_authority_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" recovery_amendment_path "${recovery_amendment}"
  expect_kv "${receipt}" recovery_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${receipt}" prejob_failure_closure_schema_id \
    "${failure_closure_schema_id}"
  expect_kv "${receipt}" prejob_failure_closure_verifier_path \
    "${failure_closure_verifier}"
  expect_kv "${receipt}" prejob_failure_closure_verifier_sha256 \
    "${expected_failure_closure_verifier_sha256}"
  expect_kv "${receipt}" prejob_failure_closure_receipt_path \
    "${failure_closure_receipt}"
  expect_kv "${receipt}" prejob_failure_closure_receipt_sha256 \
    "${expected_failure_closure_receipt_sha256}"
  expect_kv "${receipt}" prejob_failure_regular_files_inventory_path \
    "${failure_regular_files_inventory}"
  expect_kv "${receipt}" prejob_failure_regular_files_inventory_sha256 \
    "$(sha256_of "${failure_regular_files_inventory}")"
  expect_kv "${receipt}" prejob_failure_directories_inventory_path \
    "${failure_directories_inventory}"
  expect_kv "${receipt}" prejob_failure_directories_inventory_sha256 \
    "$(sha256_of "${failure_directories_inventory}")"
  expect_kv "${receipt}" prior_failed_runtime_path "${prior_failed_runtime}"
  expect_kv "${receipt}" prior_failed_runtime_scientific_input false
  expect_kv "${receipt}" prejob_failure_job_created false
  expect_kv "${receipt}" prejob_failure_optimizer_steps 0
  expect_kv "${receipt}" prejob_failure_checkpoint_created false
  expect_kv "${receipt}" prejob_failure_source_data_rows_read false
  expect_kv "${receipt}" prejob_failure_model_metric_exposed false
  expect_kv "${receipt}" recovery_scope config_path_only
}

verify_recovery_authority() {
  require_resolved_sha256_pin "${expected_recovery_amendment_sha256}" \
    "recovery amendment"
  require_resolved_sha256_pin "${expected_failure_closure_verifier_sha256}" \
    "pre-job failure closure verifier"
  require_resolved_sha256_pin "${expected_failure_closure_receipt_sha256}" \
    "pre-job failure closure receipt"
  verify_pinned_mode_file "${recovery_amendment}" \
    "${expected_recovery_amendment_sha256}" 444 "recovery amendment"
  verify_pinned_mode_file "${failure_closure_verifier}" \
    "${expected_failure_closure_verifier_sha256}" 555 \
    "pre-job failure closure verifier"
  verify_pinned_mode_file "${failure_closure_receipt}" \
    "${expected_failure_closure_receipt_sha256}" 444 \
    "pre-job failure closure receipt"
  require_immutable_file "${failure_regular_files_inventory}"
  require_immutable_file "${failure_directories_inventory}"
  expect_kv "${failure_closure_receipt}" schema_id \
    "${failure_closure_schema_id}"
  expect_kv "${failure_closure_receipt}" status complete
  expect_kv "${failure_closure_receipt}" recovery_amendment_path \
    "${recovery_amendment}"
  expect_kv "${failure_closure_receipt}" recovery_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${failure_closure_receipt}" failed_runtime_root \
    "${prior_failed_runtime}"
  expect_kv "${failure_closure_receipt}" failed_runtime_mutated false
  expect_kv "${failure_closure_receipt}" regular_file_inventory_path \
    "${failure_regular_files_inventory}"
  expect_kv "${failure_closure_receipt}" regular_file_inventory_sha256 \
    "$(sha256_of "${failure_regular_files_inventory}")"
  expect_kv "${failure_closure_receipt}" directory_inventory_path \
    "${failure_directories_inventory}"
  expect_kv "${failure_closure_receipt}" directory_inventory_sha256 \
    "$(sha256_of "${failure_directories_inventory}")"
  expect_kv "${failure_closure_receipt}" observed_endpoint_training_descendant_count 0
  expect_kv "${failure_closure_receipt}" observed_job_manifest_count 0
  expect_kv "${failure_closure_receipt}" observed_runtime_result_count 0
  expect_kv "${failure_closure_receipt}" observed_checkpoint_count 0
  expect_kv "${failure_closure_receipt}" observed_probe_count 0
  expect_kv "${failure_closure_receipt}" observed_selection_artifact_count 0
  expect_kv "${failure_closure_receipt}" observed_certified_artifact_count 0
  expect_kv "${failure_closure_receipt}" inferred_graph_first_bundle_loaded false
  expect_kv "${failure_closure_receipt}" inferred_runtime_job_creation_reached false
  expect_kv "${failure_closure_receipt}" inferred_optimizer_construction_reached false
  expect_kv "${failure_closure_receipt}" inferred_optimizer_steps 0
  expect_kv "${failure_closure_receipt}" partial_artifact_reuse_authorized false
  expect_kv "${failure_closure_receipt}" retry_schema_id "${schema_id}"
  expect_kv "${failure_closure_receipt}" retry_runtime_root "${runtime_root}"
  expect_kv "${failure_closure_receipt}" retry_restart_from_step_zero true
  expect_kv "${failure_closure_receipt}" \
    retry_requires_explicit_mtf_net_bnf_path true
  expect_kv "${failure_closure_receipt}" \
    retry_requires_effective_grammar_closure true
  run_guarded_child "${failure_closure_verifier}" --verify >/dev/null
}

emit_ablation_runner_bindings() {
  assert_operational_runner_identity
  cat <<RUNNER_BINDING
operational_ablation_runner_path=${script_path}
operational_ablation_runner_sha256=${process_start_runner_sha256}
operational_ablation_runner_process_start_sha256=${process_start_runner_sha256}
operational_ablation_runner_process_start_inode=${process_start_runner_inode}
operational_ablation_runner_process_start_device=${process_start_runner_device}
operational_ablation_runner_process_start_bytes=${process_start_runner_bytes}
operational_ablation_runner_process_start_owner_uid=${process_start_runner_owner}
operational_ablation_runner_mode=0555
operational_ablation_runner_links=1
RUNNER_BINDING
}

verify_ablation_runner_bindings() {
  local receipt="$1"
  assert_operational_runner_identity
  expect_kv "${receipt}" operational_ablation_runner_path "${script_path}"
  expect_kv "${receipt}" operational_ablation_runner_sha256 \
    "${process_start_runner_sha256}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_sha256 \
    "${process_start_runner_sha256}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_inode \
    "${process_start_runner_inode}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_device \
    "${process_start_runner_device}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_bytes \
    "${process_start_runner_bytes}"
  expect_kv "${receipt}" operational_ablation_runner_process_start_owner_uid \
    "${process_start_runner_owner}"
  expect_kv "${receipt}" operational_ablation_runner_mode 0555
  expect_kv "${receipt}" operational_ablation_runner_links 1
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
mdn_result_path=${canonical_mdn_result}
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
  expect_kv "${receipt}" mdn_completion_erratum_path "${mdn_completion_erratum}"
  expect_kv "${receipt}" mdn_completion_erratum_sha256 \
    "${expected_mdn_completion_erratum_sha256}"
  expect_kv "${receipt}" mdn_final_sealer_path "${mdn_final_sealer}"
  expect_kv "${receipt}" mdn_final_sealer_sha256 \
    "${expected_mdn_final_sealer_sha256}"
  expect_kv "${receipt}" mdn_completion_correction_path \
    "${mdn_completion_correction}"
  expect_kv "${receipt}" mdn_completion_correction_sha256 \
    "${expected_mdn_completion_correction_sha256}"
  expect_kv "${receipt}" mdn_result_path "${canonical_mdn_result}"
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

verify_mdn_retry1_authority() {
  verify_pinned_mode_file "${mdn_execution_runner}" \
    "${expected_mdn_execution_runner_sha256}" 755 \
    "MDN retry1 execution runner"
  verify_pinned_mode_file "${mdn_completion_closure_wrapper}" \
    "${expected_mdn_completion_closure_wrapper_sha256}" 555 \
    "MDN retry1 completion closure wrapper"
  verify_pinned_mode_file "${mdn_completion_closure_receipt}" \
    "${expected_mdn_completion_closure_receipt_sha256}" 444 \
    "MDN retry1 completion closure receipt"
  verify_pinned_mode_file "${mdn_completion_erratum}" \
    "${expected_mdn_completion_erratum_sha256}" 444 \
    "MDN retry1 completion concurrency erratum"
  verify_pinned_mode_file "${mdn_final_sealer}" \
    "${expected_mdn_final_sealer_sha256}" 555 \
    "MDN retry1 final completion sealer"
  verify_pinned_mode_file "${mdn_completion_correction}" \
    "${expected_mdn_completion_correction_sha256}" 444 \
    "MDN retry1 completion inventory correction"
  verify_pinned_mode_file "${canonical_mdn_result}" \
    "${expected_mdn_result_sha256}" 444 "MDN retry1 result"
  verify_pinned_mode_file "${mdn_checkpoint}" \
    "${expected_mdn_checkpoint_sha256}" 444 "MDN retry1 checkpoint"
  verify_pinned_mode_file "${mdn_train_config}" \
    "${expected_mdn_train_config_sha256}" 444 "MDN retry1 config"
  verify_pinned_file "${mdn_policy_source}" \
    "${expected_mdn_objective_sha256}" "MDN retry1 objective"

  expect_kv "${canonical_mdn_result}" schema_id "${mdn_result_schema_id}"
  expect_kv "${canonical_mdn_result}" status complete
  expect_kv "${canonical_mdn_result}" runner_path "${mdn_execution_runner}"
  expect_kv "${canonical_mdn_result}" runner_sha256 \
    "${expected_mdn_execution_runner_sha256}"
  expect_kv "${canonical_mdn_result}" checkpoint_path "${mdn_checkpoint}"
  expect_kv "${canonical_mdn_result}" checkpoint_sha256 \
    "${expected_mdn_checkpoint_sha256}"
  expect_kv "${canonical_mdn_result}" config_snapshot_path \
    "${mdn_train_config}"
  expect_kv "${canonical_mdn_result}" config_snapshot_sha256 \
    "${expected_mdn_train_config_sha256}"
  expect_kv "${canonical_mdn_result}" mdn_objective_path \
    "${mdn_policy_source}"
  expect_kv "${canonical_mdn_result}" mdn_objective_sha256 \
    "${expected_mdn_objective_sha256}"
  expect_kv "${canonical_mdn_result}" accepted_anchor_count 3261
  expect_kv "${canonical_mdn_result}" candidate_anchor_count 3261
  expect_kv "${canonical_mdn_result}" maximum_available_anchor_index 3260
  expect_kv "${canonical_mdn_result}" canonical_data_raw_access false
  expect_kv "${canonical_mdn_result}" final_holdout_available false
  expect_kv "${canonical_mdn_result}" policy_access false
  expect_kv "${mdn_completion_closure_receipt}" schema_id \
    "${mdn_completion_closure_schema_id}"
  expect_kv "${mdn_completion_closure_receipt}" status complete

  run_guarded_child "${mdn_completion_closure_wrapper}" --verify >/dev/null
}

verify_affine_operational_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" scientific_affine_runner_path \
    "${scientific_affine_runner}"
  expect_kv "${receipt}" scientific_affine_runner_sha256 \
    "${expected_scientific_affine_runner_sha256}"
  expect_kv "${receipt}" scientific_affine_runner_mode 0755
  expect_kv "${receipt}" scientific_affine_runner_links 1
  expect_kv "${receipt}" scientific_affine_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" capture_frozen_affine_runner_path \
    "${canonical_frozen_affine_runner}"
  expect_kv "${receipt}" capture_frozen_affine_runner_sha256 \
    "${expected_scientific_affine_runner_sha256}"
  expect_kv "${receipt}" capture_frozen_affine_runner_mode 0444
  expect_kv "${receipt}" capture_frozen_affine_runner_links 1
  expect_kv "${receipt}" capture_frozen_affine_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" operational_affine_runner_path \
    "${operational_affine_runner}"
  expect_kv "${receipt}" operational_affine_runner_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${receipt}" operational_affine_runner_process_start_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${receipt}" operational_affine_runner_process_start_inode \
    "${expected_operational_affine_runner_inode}"
  expect_kv "${receipt}" operational_affine_runner_process_start_device \
    "${expected_operational_affine_runner_device}"
  expect_kv "${receipt}" operational_affine_runner_process_start_bytes \
    "${expected_operational_affine_runner_bytes}"
  expect_kv "${receipt}" operational_affine_runner_process_start_owner_uid \
    "${expected_operational_affine_runner_owner}"
  expect_kv "${receipt}" operational_affine_runner_mode 0555
  expect_kv "${receipt}" operational_affine_runner_links 1
  expect_kv "${receipt}" cuda_canonical_path_correction_path \
    "${cuda_correction}"
  expect_kv "${receipt}" cuda_canonical_path_correction_sha256 \
    "${expected_cuda_correction_sha256}"
  expect_kv "${receipt}" cuda_canonical_path_correction_mode 0444
  expect_kv "${receipt}" cuda_canonical_path_correction_links 1
  expect_kv "${receipt}" cuda_canonical_path_correction_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" frozen_feature_capture_runner_path "${capture_runner}"
  expect_kv "${receipt}" frozen_feature_capture_runner_sha256 \
    "${expected_capture_runner_sha256}"
  expect_kv "${receipt}" frozen_feature_capture_runner_mode 0555
  expect_kv "${receipt}" frozen_feature_capture_runner_links 1
  expect_kv "${receipt}" frozen_feature_capture_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" frozen_capture_development_path \
    "${canonical_capture_development}"
  expect_kv "${receipt}" frozen_capture_development_sha256 \
    "${expected_capture_development_sha256}"
  expect_kv "${receipt}" frozen_capture_development_mode 0444
  expect_kv "${receipt}" frozen_capture_development_links 1
  expect_kv "${receipt}" frozen_capture_development_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" scientific_affine_helper_path "${helper_source}"
  expect_kv "${receipt}" scientific_affine_helper_sha256 \
    "${expected_affine_helper_sha256}"
  expect_kv "${receipt}" scientific_affine_helper_mode 0644
  expect_kv "${receipt}" scientific_affine_helper_links 1
  expect_kv "${receipt}" scientific_affine_helper_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" capture_frozen_affine_helper_path \
    "${canonical_frozen_helper}"
  expect_kv "${receipt}" capture_frozen_affine_helper_sha256 \
    "${expected_affine_helper_sha256}"
  expect_kv "${receipt}" capture_frozen_affine_helper_mode 0444
  expect_kv "${receipt}" capture_frozen_affine_helper_links 1
  expect_kv "${receipt}" capture_frozen_affine_helper_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" cuda_include_alias_path /usr/local/cuda-12.4/include
  expect_kv "${receipt}" cuda_include_alias_readlink targets/x86_64-linux/include
  expect_kv "${receipt}" cuda_include_alias_realpath \
    /usr/local/cuda-12.4/targets/x86_64-linux/include
  expect_kv "${receipt}" cuda_canonical_include_path \
    /usr/local/cuda-12.4/targets/x86_64-linux/include
  expect_kv "${receipt}" cuda_library_alias_path /usr/local/cuda-12.4/lib64
  expect_kv "${receipt}" cuda_library_alias_readlink targets/x86_64-linux/lib
  expect_kv "${receipt}" cuda_library_alias_realpath \
    /usr/local/cuda-12.4/targets/x86_64-linux/lib
  expect_kv "${receipt}" cuda_canonical_library_path \
    /usr/local/cuda-12.4/targets/x86_64-linux/lib
  expect_kv "${receipt}" cuda_original_failure_path \
    /usr/local/cuda-12.4/include
  expect_kv "${receipt}" cuda_original_failure_reason \
    path_contains_symbolic_link_component
  expect_kv "${receipt}" cuda_compile_include_argument \
    /usr/local/cuda-12.4/include
  expect_kv "${receipt}" cuda_link_library_argument \
    /usr/local/cuda-12.4/lib64
  expect_kv "${receipt}" cuda_runtime_rpath_argument \
    /usr/local/cuda-12.4/lib64
  expect_kv "${receipt}" cuda_alias_contract_verified true
  expect_kv "${receipt}" cuda_alias_exception_scope \
    two_exact_compatibility_symlinks_only
  expect_kv "${receipt}" global_symlink_policy_relaxed false
  expect_kv "${receipt}" compile_helper_changed false
  expect_kv "${receipt}" scientific_contract_changed false

  verify_pinned_mode_file "${scientific_affine_runner}" \
    "${expected_scientific_affine_runner_sha256}" 755 \
    "scientific affine runner"
  verify_pinned_mode_file "${canonical_frozen_affine_runner}" \
    "${expected_scientific_affine_runner_sha256}" 444 \
    "capture-frozen scientific affine runner"
  verify_pinned_mode_file "${operational_affine_runner}" \
    "${expected_operational_affine_runner_sha256}" 555 \
    "operational affine runner"
  [[ "$(stat -c '%i' -- "${operational_affine_runner}")" == \
    "${expected_operational_affine_runner_inode}" ]] ||
    fail "operational affine runner inode drifted"
  [[ "$(stat -c '%d' -- "${operational_affine_runner}")" == \
    "${expected_operational_affine_runner_device}" ]] ||
    fail "operational affine runner device drifted"
  [[ "$(stat -c '%s' -- "${operational_affine_runner}")" == \
    "${expected_operational_affine_runner_bytes}" ]] ||
    fail "operational affine runner size drifted"
  [[ "$(stat -c '%u' -- "${operational_affine_runner}")" == \
    "${expected_operational_affine_runner_owner}" ]] ||
    fail "operational affine runner owner drifted"
}

arm_root() { printf '%s/%s' "${arms_root}" "$1"; }
arm_policy() { printf '%s/config/representation.jkimyei' "$(arm_root "$1")"; }
arm_net() { printf '%s/config/representation.net' "$(arm_root "$1")"; }
arm_config() { printf '%s/config/train.config' "$(arm_root "$1")"; }
arm_capture_config() { printf '%s/config/capture.config' "$(arm_root "$1")"; }
arm_train_job() { printf '%s/training/job' "$(arm_root "$1")"; }
arm_training_status() { printf '%s/training.status' "$(arm_root "$1")"; }
arm_capture_job() { printf '%s/capture/%s' "$(arm_root "$1")" "$2"; }
arm_capture_status() { printf '%s/capture.status' "$(arm_root "$1")"; }
arm_main_report() { printf '%s/affine/main.report' "$(arm_root "$1")"; }
arm_replay_report() { printf '%s/affine/replay.report' "$(arm_root "$1")"; }
arm_main_log() { printf '%s/affine/main.stdout.log' "$(arm_root "$1")"; }
arm_replay_log() { printf '%s/affine/replay.stdout.log' "$(arm_root "$1")"; }
arm_affine_status() { printf '%s/affine.status' "$(arm_root "$1")"; }

emit_cursor_alignment_erratum_binding() {
  cat <<BINDING
cursor_alignment_erratum_verifier_path=${cursor_alignment_erratum_verifier}
cursor_alignment_erratum_verifier_sha256=$(sha256_of "${cursor_alignment_erratum_verifier}")
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${cursor_alignment_metadata_erratum}")
cursor_alignment_erratum_receipt_path=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_alignment_erratum_receipt}")
cursor_alignment_erratum_schema_id=${cursor_alignment_erratum_schema_id}
BINDING
}

verify_cursor_alignment_erratum_chain() {
  require_nonempty_file "${cursor_alignment_erratum_verifier}"
  [[ -x "${cursor_alignment_erratum_verifier}" ]] ||
    fail "cursor-alignment erratum verifier is not executable"
  require_immutable_file "${cursor_alignment_metadata_erratum}"
  require_immutable_file "${cursor_alignment_erratum_receipt}"
  [[ "$(sha256_of "${cursor_alignment_erratum_verifier}")" == \
    "${expected_cursor_alignment_erratum_verifier_sha256}" ]] ||
    fail "cursor-alignment erratum verifier hash drifted"
  [[ "$(sha256_of "${cursor_alignment_metadata_erratum}")" == \
    "${expected_cursor_alignment_metadata_erratum_sha256}" ]] ||
    fail "cursor-alignment metadata erratum hash drifted"
  [[ "$(sha256_of "${cursor_alignment_erratum_receipt}")" == \
    "${expected_cursor_alignment_erratum_receipt_sha256}" ]] ||
    fail "cursor-alignment erratum receipt hash drifted"
  expect_kv "${cursor_alignment_erratum_receipt}" schema_id \
    "${cursor_alignment_erratum_schema_id}"
  expect_kv "${cursor_alignment_erratum_receipt}" status complete
  bound_exact_file "${cursor_alignment_erratum_receipt}" \
    erratum_verifier_path erratum_verifier_sha256 \
    "${cursor_alignment_erratum_verifier}"
  bound_exact_file "${cursor_alignment_erratum_receipt}" \
    isolated_source_verifier_path isolated_source_verifier_sha256 \
    "${isolated_source_verifier}"
  bound_exact_file "${cursor_alignment_erratum_receipt}" \
    isolated_source_closure_path isolated_source_closure_sha256 \
    "${isolated_source_closure}"
  bound_exact_file "${cursor_alignment_erratum_receipt}" \
    original_cursor_correction_path original_cursor_correction_sha256 \
    "${cursor_alignment_correction}"
  bound_exact_file "${cursor_alignment_erratum_receipt}" \
    cursor_alignment_metadata_erratum_path \
    cursor_alignment_metadata_erratum_sha256 \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${cursor_alignment_erratum_receipt}" \
    source_cursor_first_anchor_master_day_index 29
  expect_kv "${cursor_alignment_erratum_receipt}" \
    source_cursor_last_anchor_master_day_index 3289
  expect_kv "${cursor_alignment_erratum_receipt}" \
    source_cursor_last_required_coarse_boundary_master_day_index 3290
  expect_kv "${cursor_alignment_erratum_receipt}" \
    source_cursor_first_anchor_key 1896047999999
  expect_kv "${cursor_alignment_erratum_receipt}" \
    source_cursor_last_anchor_key 2177711999999
  expect_kv "${cursor_alignment_erratum_receipt}" accepted_anchor_count 3261
  expect_kv "${cursor_alignment_erratum_receipt}" candidate_anchor_count 3261
  expect_kv "${cursor_alignment_erratum_receipt}" maximum_anchor_index 3260
  expect_kv "${cursor_alignment_erratum_receipt}" \
    certified_development_anchor_range '[2880,3261)'
  expect_kv "${cursor_alignment_erratum_receipt}" \
    certified_development_probe_rows "${certified_rows}"
  expect_kv "${cursor_alignment_erratum_receipt}" \
    canonical_data_raw_access false
  expect_kv "${cursor_alignment_erratum_receipt}" final_holdout_available false
  expect_kv "${cursor_alignment_erratum_receipt}" \
    independent_final_evidence false
  run_guarded_child "${cursor_alignment_erratum_verifier}" --verify \
    >/dev/null
}

verify_static_inputs() {
  local command_name
  [[ "${train_rows}" == 22464 && "${validation_rows}" == 2304 && \
    "${certified_rows}" == 3429 ]] ||
    fail "fixed probe-row arithmetic drifted"
  for command_name in awk basename bash cat chmod cmp cp dirname env find flock grep id ln \
    mkdir mktemp mv readlink realpath rm sed sha256sum sort stat sync wc; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  require_nonempty_file "${script_path}"
  require_nonempty_file "${preregistration}"
  require_nonempty_file "${conditional_amendment}"
  require_nonempty_file "${source_isolation_amendment}"
  require_nonempty_file "${isolated_source_protocol}"
  require_nonempty_file "${staged_hardening}"
  require_nonempty_file "${cursor_alignment_correction}"
  require_immutable_file "${cursor_alignment_correction}"
  require_nonempty_file "${cursor_alignment_metadata_erratum}"
  require_nonempty_file "${cursor_alignment_erratum_verifier}"
  require_nonempty_file "${cursor_alignment_erratum_receipt}"
  require_nonempty_file "${fresh_preregistration}"
  require_nonempty_file "${diagnostic_preregistration}"
  require_nonempty_file "${diagnostic_amendment}"
  require_nonempty_file "${localization_addendum}"
  require_nonempty_file "${closure_report}"
  require_nonempty_file "${isolated_source_verifier}"
  require_nonempty_file "${isolated_source_closure}"
  require_dir "${isolated_source_root}"
  [[ "$(stat -c '%A' -- "${isolated_source_root}")" != *w* ]] ||
    fail "isolated source root is writable"
  require_nonempty_file "${isolated_registry}"
  require_nonempty_file "${isolated_base_config}"
  require_immutable_file "${isolated_source_closure}"
  require_immutable_file "${isolated_registry}"
  require_immutable_file "${isolated_base_config}"
  require_nonempty_file "${capture_runner}"
  require_nonempty_file "${affine_runner}"
  require_nonempty_file "${operational_affine_runner}"
  require_nonempty_file "${cuda_correction}"
  require_nonempty_file "${representation_runner}"
  require_nonempty_file "${helper_source}"
  require_nonempty_file "${runtime_exec}"
  require_nonempty_file "${canonical_policy}"
  require_nonempty_file "${canonical_net}"
  [[ -x "${runtime_exec}" ]] || fail "Runtime executable is not executable"
  [[ "$(sha256_of "${preregistration}")" == \
    "${expected_preregistration_sha256}" ]] ||
    fail "representation ablation preregistration hash drifted"
  [[ "$(sha256_of "${conditional_amendment}")" == \
    "${expected_conditional_amendment_sha256}" ]] ||
    fail "conditional-certification amendment hash drifted"
  [[ "$(sha256_of "${source_isolation_amendment}")" == \
    "${expected_source_isolation_amendment_sha256}" ]] ||
    fail "development-source isolation amendment hash drifted"
  [[ "$(sha256_of "${isolated_source_protocol}")" == \
    "${expected_isolated_source_protocol_sha256}" ]] ||
    fail "isolated-source protocol hash drifted"
  [[ "$(sha256_of "${staged_hardening}")" == \
    "${expected_staged_hardening_sha256}" ]] ||
    fail "staged-evaluation hardening amendment hash drifted"
  [[ "$(sha256_of "${cursor_alignment_correction}")" == \
    "${expected_cursor_alignment_correction_sha256}" ]] ||
    fail "development-prefix cursor-alignment correction hash drifted"
  [[ "$(sha256_of "${fresh_preregistration}")" == \
    "${expected_fresh_preregistration_sha256}" ]] ||
    fail "fresh-seed preregistration hash drifted"
  [[ "$(sha256_of "${diagnostic_preregistration}")" == \
    "${expected_diagnostic_preregistration_sha256}" ]] ||
    fail "representation diagnostic preregistration hash drifted"
  [[ "$(sha256_of "${diagnostic_amendment}")" == \
    "${expected_diagnostic_amendment_sha256}" ]] ||
    fail "representation diagnostic amendment hash drifted"
  [[ "$(sha256_of "${localization_addendum}")" == \
    "${expected_localization_addendum_sha256}" ]] ||
    fail "representation localization addendum hash drifted"
  [[ "$(sha256_of "${closure_report}")" == \
    "${expected_data_closure_sha256}" ]] || fail "data closure hash drifted"
  [[ "$(sha256_of "${runtime_exec}")" == \
    "${expected_runtime_exec_sha256}" ]] || fail "Runtime hash drifted"
  expect_kv "${isolated_base_config}" runtime_wave_id policy_training_ppo_v0
  expect_kv "${canonical_policy}" SEED '17;'
  expect_kv "${canonical_policy}" MAX_STEPS '3000;'
  expect_kv "${canonical_policy}" LAMBDA_TF_ALIGN '0.10;'
  expect_kv "${canonical_net}" TIME_SCALES '8,16,32,64;'
  expect_kv "${canonical_net}" SCALE_STRIDES '4,8,16,32;'
  expect_kv "${canonical_net}" USE_FREQUENCY_TOKENS 'true;'
  expect_kv "${isolated_source_closure}" schema_id \
    synthetic_v2_isolated_development_source_v1
  expect_kv "${isolated_source_closure}" status complete
  expect_kv "${isolated_source_closure}" accepted_anchor_count 3261
  expect_kv "${isolated_source_closure}" candidate_anchor_count 3261
  expect_kv "${isolated_source_closure}" maximum_anchor_index 3260
  expect_kv "${isolated_source_closure}" source_cursor_first_master_day_index 29
  expect_kv "${isolated_source_closure}" source_cursor_last_master_day_index 3290
  expect_kv "${isolated_source_closure}" skipped_outside_common_range 0
  expect_kv "${isolated_source_closure}" skipped_missing_edge_coverage 0
  expect_kv "${isolated_source_closure}" skipped_failed_fetch_probe 0
  expect_kv "${isolated_source_closure}" duplicate_anchor_count 0
  expect_kv "${isolated_source_closure}" prefix_source_count 9
  expect_kv "${isolated_source_closure}" mirror_csv_count 9
  expect_kv "${isolated_source_closure}" mirror_cache_count 18
  expect_kv "${isolated_source_closure}" canonical_data_raw_access false
  expect_kv "${isolated_source_closure}" final_holdout_available false
  expect_kv "${isolated_source_closure}" isolated_source_root_path \
    "${isolated_source_root}"
  expect_kv "${isolated_source_closure}" isolated_registry_path \
    "${isolated_registry}"
  expect_kv "${isolated_source_closure}" isolated_registry_sha256 \
    "$(sha256_of "${isolated_registry}")"
  expect_kv "${isolated_source_closure}" isolated_base_config_path \
    "${isolated_base_config}"
  expect_kv "${isolated_source_closure}" isolated_base_config_sha256 \
    "$(sha256_of "${isolated_base_config}")"
  expect_kv "${isolated_source_closure}" runtime_exec_path "${runtime_exec}"
  expect_kv "${isolated_source_closure}" runtime_exec_sha256 \
    "$(sha256_of "${runtime_exec}")"
  bound_file "${isolated_source_closure}" source_manifest_path \
    source_manifest_sha256 >/dev/null
  bound_exact_file "${isolated_source_closure}" \
    cursor_alignment_correction_path cursor_alignment_correction_sha256 \
    "${cursor_alignment_correction}"
  verify_cursor_alignment_erratum_chain
  validate_isolated_registry
  reject_data_raw_file "${isolated_base_config}"
  expect_kv "${isolated_base_config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
}

verify_read_only_preflight_inputs() {
  verify_static_inputs
  verify_pinned_file "${isolated_source_verifier}" \
    "${expected_source_verifier_sha256}" "isolated source verifier"
  verify_pinned_mode_file "${isolated_source_closure}" \
    "${expected_source_closure_sha256}" 444 "isolated source closure"
  verify_pinned_mode_file "${cursor_alignment_correction}" \
    "${expected_cursor_alignment_correction_sha256}" 444 \
    "cursor-alignment correction"
  verify_pinned_mode_file "${cursor_alignment_erratum_verifier}" \
    "${expected_cursor_alignment_erratum_verifier_sha256}" 755 \
    "cursor-alignment erratum verifier"
  verify_pinned_mode_file "${cursor_alignment_metadata_erratum}" \
    "${expected_cursor_alignment_metadata_erratum_sha256}" 444 \
    "cursor-alignment metadata erratum"
  verify_pinned_mode_file "${cursor_alignment_erratum_receipt}" \
    "${expected_cursor_alignment_erratum_receipt_sha256}" 444 \
    "cursor-alignment erratum receipt"
  verify_pinned_mode_file "${capture_runner}" \
    "${expected_capture_runner_sha256}" 555 "frozen feature capture runner"
  verify_pinned_mode_file "${scientific_affine_runner}" \
    "${expected_scientific_affine_runner_sha256}" 755 \
    "scientific affine runner"
  verify_pinned_mode_file "${operational_affine_runner}" \
    "${expected_operational_affine_runner_sha256}" 555 \
    "operational affine runner"
  verify_pinned_mode_file "${helper_source}" \
    "${expected_affine_helper_sha256}" 644 "scientific affine helper"
  verify_pinned_mode_file "${cuda_correction}" \
    "${expected_cuda_correction_sha256}" 444 \
    "CUDA canonical-path correction"
  verify_pinned_mode_file "${canonical_capture_development}" \
    "${expected_capture_development_sha256}" 444 \
    "capture development receipt"
  verify_pinned_mode_file "${route_trigger}" \
    "${expected_route_trigger_sha256}" 444 "affine route trigger"
  verify_pinned_mode_file "${canonical_affine_development_status}" \
    "${expected_affine_development_status_sha256}" 444 \
    "affine development receipt"
  verify_pinned_mode_file "${canonical_affine_master_manifest}" \
    "${expected_affine_master_manifest_sha256}" 444 \
    "affine master manifest"
  verify_pinned_mode_file "${canonical_affine_binary}" \
    "${expected_affine_binary_sha256}" 555 "affine binary"
  verify_pinned_mode_file "${canonical_affine_execution_contract}" \
    "${expected_affine_execution_contract_sha256}" 444 \
    "affine execution contract"
  verify_pinned_mode_file "${canonical_raw96_report}" \
    "${expected_raw96_report_sha256}" 444 "raw96 affine report"
  verify_pinned_mode_file "${canonical_raw96_replay_report}" \
    "${expected_raw96_report_sha256}" 444 "raw96 affine replay report"
  verify_pinned_mode_file "${canonical_post384_report}" \
    "${expected_post384_report_sha256}" 444 "post384 affine report"
  verify_pinned_mode_file "${canonical_post384_replay_report}" \
    "${expected_post384_report_sha256}" 444 \
    "post384 affine replay report"
  verify_pinned_mode_file "${canonical_untrained_report}" \
    "${expected_untrained_report_sha256}" 444 \
    "untrained raw96 affine report"
  verify_pinned_mode_file "${canonical_untrained_replay_report}" \
    "${expected_untrained_report_sha256}" 444 \
    "untrained raw96 affine replay report"
}

reject_data_raw_file() {
  local path="$1"
  require_nonempty_file "${path}"
  if LC_ALL=C grep -Fq 'data/raw' "${path}"; then
    fail "canonical data/raw path is forbidden: ${path}"
  fi
}

validate_isolated_registry() {
  local canonical_root
  reject_symlink_components "${isolated_registry}"
  require_immutable_file "${isolated_registry}"
  reject_symlink_components "${isolated_source_root}"
  require_dir "${isolated_source_root}"
  canonical_root="$(realpath -e -- "${isolated_source_root}")"
  [[ "${canonical_root}" == "${isolated_source_root}" ]] ||
    fail "isolated source root is not canonical: ${isolated_source_root}"
  LC_ALL=C awk -v expected_root="${isolated_source_root}" '
    function trim(value) {
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
      return value;
    }
    function bad(message) {
      print "registry semantic error: " message > "/dev/stderr";
      failed = 1;
      exit 42;
    }
    function strip_comments(text, output, position, pair, cell) {
      output = "";
      position = 1;
      while (position <= length(text)) {
        pair = substr(text, position, 2);
        cell = substr(text, position, 1);
        if (in_comment) {
          if (pair == "*/") {
            in_comment = 0;
            position += 2;
          } else {
            position += 1;
          }
        } else if (pair == "/*") {
          in_comment = 1;
          position += 2;
        } else if (pair == "//" || cell == "#") {
          break;
        } else {
          output = output cell;
          position += 1;
        }
      }
      return output;
    }
    function allowed_block(name) {
      return name == "CSV_POLICY" || name == "DATA_ANALYTICS_POLICY" ||
             name == "SOURCE_DEFAULTS" || name == "KLINE_SOURCE_SET";
    }
    function allowed_key(name, key) {
      if (name == "CSV_POLICY") {
        return key == "CSV_BOOTSTRAP_DELTAS" ||
               key == "CSV_STEP_ABS_TOL" || key == "CSV_STEP_REL_TOL";
      }
      if (name == "DATA_ANALYTICS_POLICY") {
        return key == "MAX_SAMPLES" || key == "MAX_FEATURES" ||
               key == "MASK_EPSILON" || key == "STANDARDIZE_EPSILON";
      }
      if (name == "SOURCE_DEFAULTS") {
        return key == "SOURCE_ROOT" || key == "KLINE_INTERVALS";
      }
      if (name == "KLINE_SOURCE_SET") {
        return key == "INSTRUMENT" || key == "MARKET_TYPE" ||
               key == "VENUE" || key == "BASE_ASSET" ||
               key == "QUOTE_ASSET" || key == "SOURCE_KIND";
      }
      return 0;
    }
    function frame_line(text, left, right, body) {
      if (length(text) < 3 || substr(text, 1, 1) != left ||
          substr(text, length(text), 1) != right) return 0;
      body = substr(text, 2, length(text) - 2);
      return body ~ /^-+$/;
    }
    function require_field(identifier, key, expected, location) {
      location = identifier SUBSEP key;
      if (!(location in fields)) bad(active_block " omitted " key);
      if (fields[location] != expected) {
        bad(active_block " has " key "=" fields[location] ", expected " expected);
      }
    }
    function validate_block(name, identifier, instrument, expected_base) {
      if (name == "CSV_POLICY") {
        if (field_count[identifier] != 3) bad("CSV_POLICY field count drifted");
        require_field(identifier, "CSV_BOOTSTRAP_DELTAS", "128");
        require_field(identifier, "CSV_STEP_ABS_TOL", "1e-7");
        require_field(identifier, "CSV_STEP_REL_TOL", "1e-9");
      } else if (name == "DATA_ANALYTICS_POLICY") {
        if (field_count[identifier] != 4) bad("DATA_ANALYTICS_POLICY field count drifted");
        require_field(identifier, "MAX_SAMPLES", "4096");
        require_field(identifier, "MAX_FEATURES", "2048");
        require_field(identifier, "MASK_EPSILON", "1e-12");
        require_field(identifier, "STANDARDIZE_EPSILON", "1e-8");
      } else if (name == "SOURCE_DEFAULTS") {
        if (field_count[identifier] != 2) bad("SOURCE_DEFAULTS field count drifted");
        require_field(identifier, "SOURCE_ROOT", expected_root);
        require_field(identifier, "KLINE_INTERVALS", "1w,3d,1d");
      } else if (name == "KLINE_SOURCE_SET") {
        if (field_count[identifier] != 6) bad("KLINE_SOURCE_SET field count drifted");
        instrument = fields[identifier SUBSEP "INSTRUMENT"];
        if (instrument == "SYN2ALPHASYN2USD") {
          expected_base = "SYN2ALPHA";
        } else if (instrument == "SYN2BETASYN2USD") {
          expected_base = "SYN2BETA";
        } else if (instrument == "SYN2GAMMASYN2USD") {
          expected_base = "SYN2GAMMA";
        } else {
          bad("unexpected KLINE_SOURCE_SET instrument: " instrument);
        }
        if (++instrument_seen[instrument] != 1) {
          bad("duplicate KLINE_SOURCE_SET descriptor: " instrument);
        }
        require_field(identifier, "MARKET_TYPE", "synthetic");
        require_field(identifier, "VENUE", "local");
        require_field(identifier, "BASE_ASSET", expected_base);
        require_field(identifier, "QUOTE_ASSET", "SYN2USD");
        require_field(identifier, "SOURCE_KIND", "synthetic");
      }
    }
    {
      text = trim(strip_comments($0));
      if (text == "") next;
      if (active_block == "") {
        if (text ~ /^[A-Z_]+[[:space:]]*\{$/) {
          if (table_state != 0) bad("active block follows instrument table");
          name = text;
          sub(/[[:space:]]*\{$/, "", name);
          if (!allowed_block(name)) bad("unknown active block: " name);
          active_block = name;
          active_identifier = ++block_identifier;
          block_count[name] += 1;
          next;
        }
        normalized = text;
        gsub(/[[:space:]]+/, "", normalized);
        expected_header = "|instrument|interval|record_type|market_type|venue|base_asset|quote_asset|source_kind|source|";
        if (table_state == 0 && frame_line(text, "/", "\\")) {
          table_state = 1;
          next;
        }
        if (table_state == 1 && normalized == expected_header) {
          table_state = 2;
          next;
        }
        if (table_state == 2 && frame_line(text, "|", "|")) {
          table_state = 3;
          next;
        }
        if (table_state == 3 && frame_line(text, "\\", "/")) {
          table_state = 4;
          next;
        }
        bad("unexpected active statement outside a block: " text);
      }
      if (text == "};") {
        validate_block(active_block, active_identifier);
        active_block = "";
        active_identifier = 0;
        next;
      }
      if (text ~ /[{}]/) bad("nested or malformed registry block");
      separator = index(text, "=");
      if (separator == 0 || substr(text, length(text), 1) != ";") {
        bad("malformed active assignment: " text);
      }
      key = trim(substr(text, 1, separator - 1));
      value = trim(substr(text, separator + 1));
      sub(/;$/, "", value);
      value = trim(value);
      if (key !~ /^[A-Z_]+$/ || !allowed_key(active_block, key)) {
        bad("unknown active key in " active_block ": " key);
      }
      location = active_identifier SUBSEP key;
      if (location in fields) {
        bad("duplicate active descriptor in " active_block ": " key);
      }
      if (index(tolower(value), "data/raw") != 0) {
        bad("active value references canonical data/raw: " key "=" value);
      }
      fields[location] = value;
      field_count[active_identifier] += 1;
      if (key == "SOURCE_ROOT") root_count += 1;
    }
    END {
      if (failed) exit 42;
      if (in_comment) bad("unterminated block comment");
      if (active_block != "") bad("unterminated active block: " active_block);
      if (block_count["CSV_POLICY"] != 1 ||
          block_count["DATA_ANALYTICS_POLICY"] != 1 ||
          block_count["SOURCE_DEFAULTS"] != 1 ||
          block_count["KLINE_SOURCE_SET"] != 3) {
        bad("active block inventory drifted");
      }
      if (root_count != 1) bad("expected exactly one active SOURCE_ROOT");
      if (instrument_seen["SYN2ALPHASYN2USD"] != 1 ||
          instrument_seen["SYN2BETASYN2USD"] != 1 ||
          instrument_seen["SYN2GAMMASYN2USD"] != 1) {
        bad("isolated instrument descriptor inventory drifted");
      }
      if (table_state != 4) bad("instrument table framing drifted");
    }
  ' "${isolated_registry}" ||
    fail "isolated registry semantic validation failed"
}

derive_base_training_config() {
  LC_ALL=C awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      if ($0 !~ /=[[:space:]]*policy_training_ppo_v0[[:space:]]*$/) exit 42;
      sub(/=.*/, "= train_core_mtf_jepa_mae_vicreg");
      replaced += 1;
    }
    { print }
    END { if (replaced != 1) exit 43 }
  ' "${isolated_base_config}"
}

validate_base_training_config_content() {
  local config="$1"
  expect_kv "${config}" runtime_wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
  validate_isolated_config "${config}"
}

verify_base_training_derivation() {
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.base_training_derivation.XXXXXX)"
  derive_base_training_config >"${candidate}" || {
    rm -f -- "${candidate}"
    fail "could not derive fixed train-core base config"
  }
  validate_base_training_config_content "${candidate}"
  rm -f -- "${candidate}"
}

verify_base_training_config() {
  local candidate
  reject_symlink_components "${base_training_config}"
  require_immutable_file "${base_training_config}"
  candidate="$(mktemp /tmp/${schema_id}.base_training_verify.XXXXXX)"
  derive_base_training_config >"${candidate}" || {
    rm -f -- "${candidate}"
    fail "could not reproduce fixed train-core base config"
  }
  cmp -s -- "${candidate}" "${base_training_config}" || {
    rm -f -- "${candidate}"
    fail "immutable train-core base config is not the fixed derivation"
  }
  rm -f -- "${candidate}"
  validate_base_training_config_content "${base_training_config}"
}

write_base_training_config() {
  assert_operational_runner_identity
  local candidate
  reject_symlink_components "${runtime_root}"
  require_dir "${runtime_root}"
  reject_symlink_components "${base_training_config}"
  candidate="$(mktemp "${runtime_root}/.base_training_config.XXXXXX")"
  derive_base_training_config >"${candidate}" || {
    rm -f -- "${candidate}"
    fail "could not derive fixed train-core base config"
  }
  publish_immutable "${candidate}" "${base_training_config}"
  verify_base_training_config
  assert_operational_runner_identity
}

validate_isolated_config() {
  local config="$1"
  reject_symlink_components "${config}"
  require_nonempty_file "${config}"
  reject_data_raw_file "${config}"
  expect_kv "${config}" ujcamei_source_registry_dsl_path \
    "${isolated_registry}"
}

validate_isolated_job_manifest() {
  local manifest="$1"
  local begin="$2"
  local end="$3"
  local receipts receipt source instrument interval key expected_source
  local expected_receipt edge_field instrument_field interval_field
  local record_field source_field extra count=0
  local -A seen=()
  require_nonempty_file "${manifest}"
  reject_data_raw_file "${manifest}"
  expect_kv "${manifest}" accepted_anchor_count 3261
  expect_kv "${manifest}" candidate_anchor_count 3261
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin "${begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${end}"
  ((end <= 3261)) || fail "job manifest opens an anchor beyond 3260"
  receipts="$(kv source_file_receipts "${manifest}")"
  [[ -n "${receipts}" ]] || fail "job manifest lacks source_file_receipts"
  [[ "${receipts}" != *'/data/raw/'* ]] ||
    fail "job manifest includes canonical data/raw"
  while IFS= read -r receipt; do
    [[ -n "${receipt}" ]] || continue
    IFS='|' read -r edge_field instrument_field interval_field record_field \
      source_field extra <<<"${receipt}"
    [[ -z "${extra:-}" && "${edge_field}" == edge=* && \
      "${instrument_field}" == instrument=* && \
      "${interval_field}" == interval=* && \
      "${record_field}" == record_type=kline && \
      "${source_field}" == source=* ]] ||
      fail "malformed isolated source descriptor: ${receipt}"
    instrument="${instrument_field#instrument=}"
    interval="${interval_field#interval=}"
    [[ "${edge_field#edge=}" == "${instrument}" ]] ||
      fail "source descriptor edge/instrument mismatch: ${receipt}"
    case "${instrument}" in
    SYN2ALPHASYN2USD | SYN2BETASYN2USD | SYN2GAMMASYN2USD) ;;
    *) fail "unexpected source descriptor instrument: ${instrument}" ;;
    esac
    case "${interval}" in
    1d | 3d | 1w) ;;
    *) fail "unexpected source descriptor interval: ${interval}" ;;
    esac
    source="${source_field#source=}"
    expected_source="${isolated_source_root}/${instrument}/${interval}/${instrument}-${interval}-all-years.csv"
    expected_receipt="edge=${instrument}|instrument=${instrument}|interval=${interval}|record_type=kline|source=${expected_source}"
    [[ "${receipt}" == "${expected_receipt}" ]] ||
      fail "source descriptor differs from isolated contract: ${receipt}"
    reject_symlink_components "${source}"
    require_nonempty_file "${source}"
    [[ "$(realpath -e -- "${source}")" == "${source}" ]] ||
      fail "source descriptor is not canonical: ${source}"
    key="${instrument}/${interval}"
    [[ -z "${seen[${key}]:-}" ]] ||
      fail "duplicate source descriptor: ${key}"
    seen["${key}"]=1
    count=$((count + 1))
  done < <(printf '%s\n' "${receipts}" | sed 's/;;/\n/g')
  [[ "${count}" == 9 ]] ||
    fail "job manifest expected nine source receipts, found ${count}"
  for instrument in SYN2ALPHASYN2USD SYN2BETASYN2USD SYN2GAMMASYN2USD; do
    for interval in 1d 3d 1w; do
      [[ "${seen[${instrument}/${interval}]:-}" == 1 ]] ||
        fail "job manifest omitted source descriptor: ${instrument}/${interval}"
    done
  done
}

verify_affine_master_manifest() {
  local manifest
  require_dir "${canonical_affine_development}"
  [[ "$(stat -c '%A' -- "${canonical_affine_development}")" != *w* ]] ||
    fail "canonical affine development runtime is writable"
  for manifest in input_manifest.sha256 source_manifest.sha256 binary.sha256 \
    output_manifest.sha256 master.sha256; do
    require_immutable_file "${canonical_affine_development}/${manifest}"
    (
      cd "${canonical_affine_development}"
      sha256sum -c -- "${manifest}" >/dev/null
    ) || fail "canonical affine manifest verification failed: ${manifest}"
  done
}

verify_clean_capture_development() {
  local clean_representation_checkpoint clean_mdn_checkpoint
  verify_pinned_mode_file "${canonical_capture_development}" \
    "${expected_capture_development_sha256}" 444 \
    "capture development receipt"
  expect_kv "${canonical_capture_development}" schema_id \
    synthetic_v2_frozen_feature_capture_isolated_v2.development.v1
  expect_kv "${canonical_capture_development}" status complete
  bound_exact_file "${canonical_capture_development}" runner_path runner_sha256 \
    "${capture_runner}"
  expect_kv "${canonical_capture_development}" runner_sha256 \
    "${expected_capture_runner_sha256}"
  bound_exact_file "${canonical_capture_development}" \
    isolated_source_closure_path isolated_source_closure_sha256 \
    "${isolated_source_closure}"
  expect_kv "${canonical_capture_development}" isolated_source_root_path \
    "${isolated_source_root}"
  bound_exact_file "${canonical_capture_development}" capture_config_path \
    capture_config_sha256 "${canonical_capture_config}"
  bound_exact_file "${canonical_capture_development}" \
    untrained_capture_config_path untrained_capture_config_sha256 \
    "${canonical_untrained_capture_config}"
  bound_exact_file "${canonical_capture_development}" \
    untrained_mdn_policy_path untrained_mdn_policy_sha256 \
    "${canonical_untrained_mdn_policy}"
  bound_exact_file "${canonical_capture_development}" representation_result_path \
    representation_result_sha256 "${canonical_representation_result}"
  verify_mdn_retry1_authority_bindings "${canonical_capture_development}"
  verify_mdn_retry1_authority
  clean_representation_checkpoint="$(bound_file \
    "${canonical_capture_development}" representation_checkpoint_path \
    representation_checkpoint_sha256)"
  clean_mdn_checkpoint="$(bound_file "${canonical_capture_development}" \
    mdn_checkpoint_path mdn_checkpoint_sha256)"
  [[ "${clean_mdn_checkpoint}" == "${mdn_checkpoint}" ]] ||
    fail "capture receipt points to an unexpected MDN retry1 checkpoint"
  expect_kv "${canonical_capture_development}" mdn_checkpoint_sha256 \
    "${expected_mdn_checkpoint_sha256}"
  require_immutable_file "${clean_representation_checkpoint}"
  require_immutable_file "${clean_mdn_checkpoint}"
  bound_exact_file "${canonical_capture_development}" frozen_affine_helper_path \
    frozen_affine_helper_sha256 "${canonical_frozen_helper}"
  bound_exact_file "${canonical_capture_development}" frozen_affine_runner_path \
    frozen_affine_runner_sha256 "${canonical_frozen_affine_runner}"
  expect_kv "${canonical_capture_development}" accepted_anchor_count 3261
  expect_kv "${canonical_capture_development}" candidate_anchor_count 3261
  expect_kv "${canonical_capture_development}" maximum_available_anchor 3260
  expect_kv "${canonical_capture_development}" train_capture_range \
    "[${train_begin},${train_end})"
  expect_kv "${canonical_capture_development}" validation_capture_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${canonical_capture_development}" train_probe_rows "${train_rows}"
  expect_kv "${canonical_capture_development}" validation_probe_rows \
    "${validation_rows}"
  expect_kv "${canonical_capture_development}" maximum_anchor_read 2815
  expect_kv "${canonical_capture_development}" certified_capture_created false
  expect_kv "${canonical_capture_development}" certified_attempt_created false
  expect_kv "${canonical_capture_development}" certified_result_created false
  expect_kv "${canonical_capture_development}" canonical_data_raw_access false
  expect_kv "${canonical_capture_development}" final_holdout_scored false
  expect_kv "${canonical_capture_development}" policy_access false

  local key job
  for key in fresh_preregistration diagnostic_preregistration \
    diagnostic_protocol_amendment localization_addendum \
    conditional_certification_amendment \
    ablation_preregistration source_isolation_amendment \
    isolated_source_protocol staged_hardening_amendment \
    cursor_alignment_correction; do
    case "${key}" in
    fresh_preregistration) verify_document_binding "${canonical_capture_development}" "${key}" "${fresh_preregistration}" ;;
    diagnostic_preregistration) verify_document_binding "${canonical_capture_development}" "${key}" "${diagnostic_preregistration}" ;;
    diagnostic_protocol_amendment) verify_document_binding "${canonical_capture_development}" "${key}" "${diagnostic_amendment}" ;;
    localization_addendum) verify_document_binding "${canonical_capture_development}" "${key}" "${localization_addendum}" ;;
    conditional_certification_amendment) verify_document_binding "${canonical_capture_development}" "${key}" "${conditional_amendment}" ;;
    ablation_preregistration) verify_document_binding "${canonical_capture_development}" "${key}" "${preregistration}" ;;
    source_isolation_amendment) verify_document_binding "${canonical_capture_development}" "${key}" "${source_isolation_amendment}" ;;
    isolated_source_protocol) verify_document_binding "${canonical_capture_development}" "${key}" "${isolated_source_protocol}" ;;
    staged_hardening_amendment) verify_document_binding "${canonical_capture_development}" "${key}" "${staged_hardening}" ;;
    cursor_alignment_correction) verify_document_binding "${canonical_capture_development}" "${key}" "${cursor_alignment_correction}" ;;
    esac
  done

  for job in train validation; do
    local job_root begin end prefix
    if [[ "${job}" == train ]]; then
      job_root="${canonical_capture_train_job}"
      begin="${train_begin}"
      end="${train_end}"
      prefix=trained_train
    else
      job_root="${canonical_capture_validation_job}"
      begin="${validation_begin}"
      end="${validation_end}"
      prefix=trained_validation
    fi
    expect_kv "${canonical_capture_development}" "${prefix}_job_dir" \
      "${job_root}"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_context_probe_path" "${prefix}_context_probe_sha256" \
      "${job_root}/mdn_edge_context_features.probe"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_representation_probe_path" \
      "${prefix}_representation_probe_sha256" \
      "${job_root}/representation_edge_features.probe"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_report_path" "${prefix}_report_sha256" \
      "${job_root}/channel_inference.report"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_manifest_path" "${prefix}_manifest_sha256" \
      "${job_root}/job.manifest"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_runtime_result_path" "${prefix}_runtime_result_sha256" \
      "${job_root}/runtime.result.fact"
    validate_isolated_job_manifest "${job_root}/job.manifest" "${begin}" "${end}"
    expect_kv "${job_root}/job.manifest" config_path \
      "${canonical_capture_config}"
    expect_kv "${job_root}/job.manifest" wave_id \
      cwu_02v_certified_replay_eval_mdn
    expect_kv "${job_root}/job.manifest" wave_action run
    expect_kv "${job_root}/job.manifest" input_representation_checkpoint_path \
      "${clean_representation_checkpoint}"
    expect_kv "${job_root}/job.manifest" input_mdn_checkpoint_path \
      "${clean_mdn_checkpoint}"
    [[ ! -e "${job_root}/channel_policy.report" ]] ||
      fail "clean canonical capture accessed a policy: ${job_root}"
    validate_probe_file "${job_root}/representation_edge_features.probe" \
      "${begin}" "${end}" "$(((end - begin) * 9))"
    validate_probe_file "${job_root}/mdn_edge_context_features.probe" \
      "${begin}" "${end}" "$(((end - begin) * 9))" \
      kikijyeba.synthetic.mdn_edge_context_feature_probe.v1 400
    verify_capture_job_immutable "${job_root}"
  done

  for job in train validation; do
    local job_root begin end prefix
    if [[ "${job}" == train ]]; then
      job_root="${canonical_untrained_train_job}"
      begin="${train_begin}"
      end="${train_end}"
      prefix=untrained_train
    else
      job_root="${canonical_untrained_validation_job}"
      begin="${validation_begin}"
      end="${validation_end}"
      prefix=untrained_validation
    fi
    expect_kv "${canonical_capture_development}" "${prefix}_job_dir" \
      "${job_root}"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_representation_probe_path" \
      "${prefix}_representation_probe_sha256" \
      "${job_root}/representation_edge_features.probe"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_context_probe_path" "${prefix}_context_probe_sha256" \
      "${job_root}/mdn_edge_context_features.probe"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_report_path" "${prefix}_report_sha256" \
      "${job_root}/channel_inference.report"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_manifest_path" "${prefix}_manifest_sha256" \
      "${job_root}/job.manifest"
    bound_exact_file "${canonical_capture_development}" \
      "${prefix}_runtime_result_path" "${prefix}_runtime_result_sha256" \
      "${job_root}/runtime.result.fact"
    validate_isolated_job_manifest "${job_root}/job.manifest" "${begin}" "${end}"
    expect_kv "${job_root}/job.manifest" config_path \
      "${canonical_untrained_capture_config}"
    expect_kv "${job_root}/job.manifest" wave_id \
      cwu_02v_certified_replay_eval_mdn
    expect_kv "${job_root}/job.manifest" wave_action run
    expect_kv "${job_root}/job.manifest" input_representation_checkpoint_path ''
    expect_kv "${job_root}/job.manifest" input_mdn_checkpoint_path \
      "${clean_mdn_checkpoint}"
    [[ ! -e "${job_root}/channel_policy.report" ]] ||
      fail "clean untrained capture accessed a policy: ${job_root}"
    validate_probe_file "${job_root}/representation_edge_features.probe" \
      "${begin}" "${end}" "$(((end - begin) * 9))"
    validate_probe_file "${job_root}/mdn_edge_context_features.probe" \
      "${begin}" "${end}" "$(((end - begin) * 9))" \
      kikijyeba.synthetic.mdn_edge_context_feature_probe.v1 400
    verify_capture_job_immutable "${job_root}"
  done
  require_immutable_file "${canonical_capture_config}"
  require_immutable_file "${canonical_untrained_capture_config}"
  require_immutable_file "${canonical_untrained_mdn_policy}"
  validate_isolated_config "${canonical_capture_config}"
  validate_isolated_config "${canonical_untrained_capture_config}"
  expect_kv "${canonical_untrained_capture_config}" \
    wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${canonical_untrained_mdn_policy}"
  expect_kv "${canonical_untrained_mdn_policy}" \
    ALLOW_UNTRAINED_REPRESENTATION 'true;'
  expect_kv "${canonical_untrained_mdn_policy}" SEED '17;'
  verify_pinned_mode_file "${canonical_frozen_helper}" \
    "${expected_affine_helper_sha256}" 444 \
    "capture-frozen affine helper"
  verify_pinned_mode_file "${canonical_frozen_affine_runner}" \
    "${expected_scientific_affine_runner_sha256}" 444 \
    "capture-frozen scientific affine runner"
}

verify_route_trigger() {
  verify_pinned_mode_file "${route_trigger}" \
    "${expected_route_trigger_sha256}" 444 "affine route trigger"
  verify_clean_capture_development
  expect_kv "${route_trigger}" schema_id \
    synthetic_v2_frozen_affine_route_trigger_isolated_v2
  expect_kv "${route_trigger}" status complete
  expect_kv "${route_trigger}" route representation_ablation_screen
  expect_kv "${route_trigger}" accepted_anchor_count 3261
  expect_kv "${route_trigger}" candidate_anchor_count 3261
  expect_kv "${route_trigger}" maximum_available_anchor 3260
  expect_kv "${route_trigger}" maximum_anchor_read 2815
  expect_kv "${route_trigger}" raw96_validation_strong_gate_pass false
  expect_kv "${route_trigger}" certified_capture_opened false
  expect_kv "${route_trigger}" canonical_data_raw_access false
  expect_kv "${route_trigger}" final_holdout_access false
  expect_kv "${route_trigger}" policy_access false
  bound_exact_file "${route_trigger}" capture_development_path \
    capture_development_sha256 "${canonical_capture_development}"
  bound_exact_file "${route_trigger}" capture_runner_path capture_runner_sha256 \
    "${capture_runner}"
  bound_exact_file "${route_trigger}" isolated_source_closure_path \
    isolated_source_closure_sha256 "${isolated_source_closure}"
  bound_exact_file "${route_trigger}" affine_runner_path affine_runner_sha256 \
    "${canonical_frozen_affine_runner}"
  bound_exact_file "${route_trigger}" affine_helper_path affine_helper_sha256 \
    "${canonical_frozen_helper}"
  bound_exact_file "${route_trigger}" affine_binary_path affine_binary_sha256 \
    "${canonical_affine_binary}"
  bound_exact_file "${route_trigger}" affine_development_receipt_path \
    affine_development_receipt_sha256 "${canonical_affine_development_status}"
  bound_exact_file "${route_trigger}" affine_master_manifest_path \
    affine_master_manifest_sha256 "${canonical_affine_master_manifest}"
  bound_exact_file "${route_trigger}" raw96_validation_report_path \
    raw96_validation_report_sha256 "${canonical_raw96_report}"
  bound_exact_file "${route_trigger}" post384_validation_report_path \
    post384_validation_report_sha256 "${canonical_post384_report}"
  bound_exact_file "${route_trigger}" untrained_raw96_validation_report_path \
    untrained_raw96_validation_report_sha256 "${canonical_untrained_report}"
  verify_pinned_mode_file "${canonical_affine_binary}" \
    "${expected_affine_binary_sha256}" 555 "affine binary"
  verify_pinned_mode_file "${canonical_raw96_report}" \
    "${expected_raw96_report_sha256}" 444 "raw96 affine report"
  verify_pinned_mode_file "${canonical_post384_report}" \
    "${expected_post384_report_sha256}" 444 "post384 affine report"
  verify_pinned_mode_file "${canonical_untrained_report}" \
    "${expected_untrained_report_sha256}" 444 \
    "untrained raw96 affine report"
  verify_pinned_mode_file "${canonical_raw96_replay_report}" \
    "${expected_raw96_report_sha256}" 444 "raw96 affine replay report"
  verify_pinned_mode_file "${canonical_post384_replay_report}" \
    "${expected_post384_report_sha256}" 444 \
    "post384 affine replay report"
  verify_pinned_mode_file "${canonical_untrained_replay_report}" \
    "${expected_untrained_report_sha256}" 444 \
    "untrained raw96 affine replay report"
  cmp -s -- "${canonical_raw96_report}" "${canonical_raw96_replay_report}" ||
    fail "canonical raw96 main/replay reports differ"
  cmp -s -- "${canonical_post384_report}" \
    "${canonical_post384_replay_report}" ||
    fail "canonical post384 main/replay reports differ"
  cmp -s -- "${canonical_untrained_report}" \
    "${canonical_untrained_replay_report}" ||
    fail "canonical untrained main/replay reports differ"

  verify_document_binding "${route_trigger}" fresh_preregistration \
    "${fresh_preregistration}"
  verify_document_binding "${route_trigger}" diagnostic_preregistration \
    "${diagnostic_preregistration}"
  verify_document_binding "${route_trigger}" diagnostic_protocol_amendment \
    "${diagnostic_amendment}"
  verify_document_binding "${route_trigger}" localization_addendum \
    "${localization_addendum}"
  verify_document_binding "${route_trigger}" \
    conditional_certification_amendment \
    "${conditional_amendment}"
  verify_document_binding "${route_trigger}" ablation_preregistration \
    "${preregistration}"
  verify_document_binding "${route_trigger}" source_isolation_amendment \
    "${source_isolation_amendment}"
  verify_document_binding "${route_trigger}" isolated_source_protocol \
    "${isolated_source_protocol}"
  verify_document_binding "${route_trigger}" staged_hardening_amendment \
    "${staged_hardening}"
  verify_document_binding "${route_trigger}" cursor_alignment_correction \
    "${cursor_alignment_correction}"

  verify_pinned_mode_file "${canonical_affine_development_status}" \
    "${expected_affine_development_status_sha256}" 444 \
    "affine development receipt"
  expect_kv "${canonical_affine_development_status}" schema_id \
    synthetic_v2_frozen_affine_development_isolated_v2
  expect_kv "${canonical_affine_development_status}" status complete
  expect_kv "${canonical_affine_development_status}" maximum_anchor_read 2815
  expect_kv "${canonical_affine_development_status}" accepted_anchor_count 3261
  expect_kv "${canonical_affine_development_status}" candidate_anchor_count 3261
  expect_kv "${canonical_affine_development_status}" \
    maximum_available_anchor 3260
  expect_kv "${canonical_affine_development_status}" certified_input_access false
  expect_kv "${canonical_affine_development_status}" canonical_data_raw_access false
  expect_kv "${canonical_affine_development_status}" final_holdout_access false
  bound_exact_file "${canonical_affine_development_status}" \
    capture_development_path capture_development_sha256 \
    "${canonical_capture_development}"
  bound_exact_file "${canonical_affine_development_status}" \
    isolated_source_closure_path isolated_source_closure_sha256 \
    "${isolated_source_closure}"
  bound_exact_file "${canonical_affine_development_status}" affine_runner_path \
    affine_runner_sha256 "${canonical_frozen_affine_runner}"
  bound_exact_file "${canonical_affine_development_status}" affine_helper_path \
    affine_helper_sha256 "${canonical_frozen_helper}"
  bound_exact_file "${canonical_affine_development_status}" affine_binary_path \
    affine_binary_sha256 "${canonical_affine_binary}"
  bound_exact_file "${canonical_affine_development_status}" \
    raw96_validation_report_path raw96_validation_report_sha256 \
    "${canonical_raw96_report}"
  bound_exact_file "${canonical_affine_development_status}" \
    post384_validation_report_path post384_validation_report_sha256 \
    "${canonical_post384_report}"
  bound_exact_file "${canonical_affine_development_status}" \
    untrained_raw96_validation_report_path \
    untrained_raw96_validation_report_sha256 "${canonical_untrained_report}"
  verify_document_binding "${canonical_affine_development_status}" \
    fresh_preregistration "${fresh_preregistration}"
  verify_document_binding "${canonical_affine_development_status}" \
    diagnostic_preregistration "${diagnostic_preregistration}"
  verify_document_binding "${canonical_affine_development_status}" \
    diagnostic_protocol_amendment "${diagnostic_amendment}"
  verify_document_binding "${canonical_affine_development_status}" \
    localization_addendum "${localization_addendum}"
  verify_document_binding "${canonical_affine_development_status}" \
    conditional_certification_amendment "${conditional_amendment}"
  verify_document_binding "${canonical_affine_development_status}" \
    ablation_preregistration "${preregistration}"
  verify_document_binding "${canonical_affine_development_status}" \
    source_isolation_amendment "${source_isolation_amendment}"
  verify_document_binding "${canonical_affine_development_status}" \
    isolated_source_protocol "${isolated_source_protocol}"
  verify_document_binding "${canonical_affine_development_status}" \
    staged_hardening_amendment "${staged_hardening}"
  verify_document_binding "${canonical_affine_development_status}" \
    cursor_alignment_correction "${cursor_alignment_correction}"
  [[ -x "${canonical_affine_binary}" ]] ||
    fail "canonical affine binary is not executable"
  verify_pinned_mode_file "${canonical_affine_master_manifest}" \
    "${expected_affine_master_manifest_sha256}" 444 \
    "affine master manifest"
  verify_pinned_mode_file "${canonical_affine_execution_contract}" \
    "${expected_affine_execution_contract_sha256}" 444 \
    "affine execution contract"
  verify_affine_operational_bindings "${route_trigger}"
  verify_affine_operational_bindings \
    "${canonical_affine_development_status}"
  verify_affine_operational_bindings \
    "${canonical_affine_execution_contract}"
  verify_affine_master_manifest

  local report direction rank correlation rmse strong conjunction=false
  report="${canonical_raw96_report}"
  expect_kv "${report}" schema_id \
    synthetic_v2_frozen_encoder_affine_development_v1
  expect_kv "${report}" status complete
  expect_kv "${report}" probe_kind representation
  expect_kv "${report}" certified_probe_rows 0
  expect_kv "${report}" certified_anchor_range not_opened
  expect_kv "${report}" maximum_anchor_read 2815
  expect_kv "${canonical_post384_report}" schema_id \
    synthetic_v2_frozen_representation_affine_development_v1
  expect_kv "${canonical_post384_report}" status complete
  expect_kv "${canonical_post384_report}" probe_kind mdn_context
  expect_kv "${canonical_post384_report}" certified_probe_rows 0
  expect_kv "${canonical_post384_report}" certified_anchor_range not_opened
  expect_kv "${canonical_post384_report}" maximum_anchor_read 2815
  expect_kv "${canonical_untrained_report}" schema_id \
    synthetic_v2_untrained_encoder_affine_control_v1
  expect_kv "${canonical_untrained_report}" status complete
  expect_kv "${canonical_untrained_report}" probe_kind representation
  expect_kv "${canonical_untrained_report}" certified_probe_rows 0
  expect_kv "${canonical_untrained_report}" certified_anchor_range not_opened
  expect_kv "${canonical_untrained_report}" maximum_anchor_read 2815
  expect_kv "${canonical_untrained_report}" classification \
    untrained_representation_validation_control
  direction="$(numeric_gate \
    "$(kv selected.validation.directional_accuracy "${report}")" ge 0.95)"
  rank="$(numeric_gate \
    "$(kv selected.validation.pairwise_rank_accuracy "${report}")" ge 0.95)"
  correlation="$(numeric_gate \
    "$(kv selected.validation.correlation "${report}")" ge 0.95)"
  rmse="$(numeric_gate \
    "$(kv selected.validation.rmse_target_rms_ratio "${report}")" le 0.25)"
  if [[ "${direction}" == true && "${rank}" == true && \
    "${correlation}" == true && "${rmse}" == true ]]; then
    conjunction=true
  fi
  strong="$(kv validation_strong_gate_pass "${report}")"
  [[ "${strong}" == false && "${conjunction}" == false ]] ||
    fail "ablation route is not supported by the canonical raw96 report"
  expect_kv "${route_trigger}" raw96_validation_direction_gate_pass \
    "${direction}"
  expect_kv "${route_trigger}" raw96_validation_pairwise_rank_gate_pass \
    "${rank}"
  expect_kv "${route_trigger}" raw96_validation_correlation_gate_pass \
    "${correlation}"
  expect_kv "${route_trigger}" raw96_validation_rmse_ratio_gate_pass \
    "${rmse}"
  [[ ! -e "${canonical_capture_result}" ]] ||
    fail "canonical certified capture exists despite the ablation route"
  [[ ! -e "${canonical_affine_final}" ]] ||
    fail "canonical certified affine result exists despite the ablation route"
}

canonical_inputs() {
  verify_route_trigger
  require_nonempty_file "${canonical_capture_development}"
  canonical_checkpoint="$(bound_file "${canonical_capture_development}" \
    representation_checkpoint_path representation_checkpoint_sha256)"
  mdn_checkpoint="$(bound_file "${canonical_capture_development}" \
    mdn_checkpoint_path mdn_checkpoint_sha256)"
  capture_config="$(bound_file "${canonical_capture_development}" \
    capture_config_path capture_config_sha256)"
  canonical_train_probe="$(bound_file "${canonical_capture_development}" \
    trained_train_representation_probe_path \
    trained_train_representation_probe_sha256)"
  canonical_validation_probe="$(bound_file "${canonical_capture_development}" \
    trained_validation_representation_probe_path \
    trained_validation_representation_probe_sha256)"
  canonical_report="$(bound_file "${route_trigger}" \
    raw96_validation_report_path raw96_validation_report_sha256)"
  affine_binary_source="$(bound_file "${route_trigger}" affine_binary_path \
    affine_binary_sha256)"
  affine_helper_source="$(bound_file "${route_trigger}" affine_helper_path \
    affine_helper_sha256)"
  affine_runner_source="$(bound_file "${route_trigger}" affine_runner_path \
    affine_runner_sha256)"
  [[ "${capture_config}" == "${canonical_capture_config}" ]] ||
    fail "canonical capture config path drifted"
  [[ "${canonical_train_probe}" == \
    "${canonical_capture_train_job}/representation_edge_features.probe" ]] ||
    fail "canonical train representation probe path drifted"
  [[ "${canonical_validation_probe}" == \
    "${canonical_capture_validation_job}/representation_edge_features.probe" ]] ||
    fail "canonical validation representation probe path drifted"
  [[ "${canonical_report}" == "${canonical_raw96_report}" ]] ||
    fail "canonical validation report path drifted"
  [[ "${affine_binary_source}" == "${canonical_affine_binary}" ]] ||
    fail "canonical affine binary path drifted"
  [[ "${affine_helper_source}" == "${canonical_frozen_helper}" ]] ||
    fail "canonical affine helper path drifted"
  [[ "${affine_runner_source}" == "${canonical_frozen_affine_runner}" ]] ||
    fail "canonical affine runner path drifted"
  cmp -s -- "${helper_source}" "${affine_helper_source}" ||
    fail "live affine helper differs from the trigger-frozen helper"
  cmp -s -- "${affine_runner}" "${affine_runner_source}" ||
    fail "live affine runner differs from the trigger-frozen runner"
  canonical_replay_report="${canonical_raw96_replay_report}"
  require_immutable_file "${canonical_replay_report}"
  cmp -s -- "${canonical_report}" "${canonical_replay_report}" ||
    fail "canonical raw96 main/replay reports differ"
  validate_probe_file "${canonical_train_probe}" "${train_begin}" \
    "${train_end}" "${train_rows}"
  validate_probe_file "${canonical_validation_probe}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}"
  require_immutable_file "${canonical_checkpoint}"
  require_immutable_file "${mdn_checkpoint}"
  require_immutable_file "${capture_config}"
  require_immutable_file "${canonical_train_probe}"
  require_immutable_file "${canonical_validation_probe}"
  validate_isolated_config "${capture_config}"
  local canonical_train_manifest canonical_validation_manifest
  canonical_train_manifest="$(bound_file "${canonical_capture_development}" \
    trained_train_manifest_path trained_train_manifest_sha256)"
  canonical_validation_manifest="$(bound_file \
    "${canonical_capture_development}" trained_validation_manifest_path \
    trained_validation_manifest_sha256)"
  validate_isolated_job_manifest "${canonical_train_manifest}" \
    "${train_begin}" "${train_end}"
  validate_isolated_job_manifest "${canonical_validation_manifest}" \
    "${validation_begin}" "${validation_end}"
  require_nonempty_file "${canonical_representation_result}"
  require_nonempty_file "${canonical_mdn_result}"
  require_immutable_file "${canonical_representation_result}"
  require_immutable_file "${canonical_mdn_result}"
  expect_kv "${canonical_representation_result}" schema_id \
    synthetic_v2_representation_train_isolated_v2.result.v1
  expect_kv "${canonical_representation_result}" status complete
  expect_kv "${canonical_representation_result}" accepted_anchor_count 3261
  expect_kv "${canonical_representation_result}" candidate_anchor_count 3261
  expect_kv "${canonical_representation_result}" maximum_available_anchor_index 3260
  expect_kv "${canonical_representation_result}" canonical_data_raw_access false
  bound_exact_file "${canonical_representation_result}" \
    cursor_alignment_correction_path cursor_alignment_correction_sha256 \
    "${cursor_alignment_correction}"
  expect_kv "${canonical_representation_result}" isolated_source_closure_path \
    "${isolated_source_closure}"
  expect_kv "${canonical_representation_result}" isolated_source_closure_sha256 \
    "$(sha256_of "${isolated_source_closure}")"
  expect_kv "${canonical_representation_result}" checkpoint_path \
    "${canonical_checkpoint}"
  expect_kv "${canonical_representation_result}" checkpoint_sha256 \
    "$(sha256_of "${canonical_checkpoint}")"
  verify_mdn_retry1_authority
  verify_mdn_retry1_authority_bindings "${canonical_capture_development}"
  expect_kv "${canonical_mdn_result}" status complete
  expect_kv "${canonical_mdn_result}" schema_id "${mdn_result_schema_id}"
  expect_kv "${canonical_mdn_result}" accepted_anchor_count 3261
  expect_kv "${canonical_mdn_result}" candidate_anchor_count 3261
  expect_kv "${canonical_mdn_result}" maximum_available_anchor_index 3260
  expect_kv "${canonical_mdn_result}" canonical_data_raw_access false
  bound_exact_file "${canonical_mdn_result}" \
    cursor_alignment_erratum_receipt_path \
    cursor_alignment_erratum_receipt_sha256 \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${canonical_mdn_result}" isolated_source_closure_path \
    "${isolated_source_closure}"
  expect_kv "${canonical_mdn_result}" isolated_source_closure_sha256 \
    "$(sha256_of "${isolated_source_closure}")"
  expect_kv "${canonical_mdn_result}" checkpoint_path "${mdn_checkpoint}"
  expect_kv "${canonical_mdn_result}" checkpoint_sha256 \
    "$(sha256_of "${mdn_checkpoint}")"
}

preflight_read_only() {
  assert_operational_runner_identity
  verify_recovery_authority
  verify_read_only_preflight_inputs
  verify_mdn_retry1_authority

  run_guarded_child "${capture_runner}" --verify-development >/dev/null

  run_guarded_child "${operational_affine_runner}" --verify-development \
    >/dev/null

  # canonical_inputs transitively verifies the exact capture receipt and the
  # sealed representation_ablation_screen route without creating artifacts.
  canonical_inputs
  assert_operational_runner_identity
}

freeze_sources() {
  assert_operational_runner_identity
  canonical_inputs
  assert_operational_runner_identity
  if [[ -e "${frozen_root}" ]]; then
    verify_frozen_sources
    assert_operational_runner_identity
    return
  fi
  local candidate
  assert_operational_runner_identity
  candidate="$(mktemp -d "${runtime_root}/.frozen_sources.XXXXXXXX")"
  assert_operational_runner_identity
  cp -- "${script_path}" \
    "${candidate}/run_representation_ablation_v2_retry1.sh"
  cp -- "${affine_helper_source}" \
    "${candidate}/frozen_representation_affine_probe.cpp"
  cp -- "${affine_binary_source}" \
    "${candidate}/frozen_representation_affine_probe"
  assert_operational_runner_identity
  chmod 0444 "${candidate}/run_representation_ablation_v2_retry1.sh" \
    "${candidate}/frozen_representation_affine_probe.cpp"
  chmod 0555 "${candidate}/frozen_representation_affine_probe"
  mv -T -n "${candidate}" "${frozen_root}" ||
    fail "could not atomically freeze ablation sources"
  assert_operational_runner_identity
  verify_frozen_sources
  assert_operational_runner_identity
}

verify_frozen_sources() {
  require_dir "${frozen_root}"
  require_nonempty_file "${frozen_runner}"
  require_nonempty_file "${frozen_helper}"
  require_nonempty_file "${frozen_binary}"
  [[ -x "${frozen_binary}" ]] || fail "frozen affine binary is not executable"
  [[ "$(sha256_of "${frozen_runner}")" == \
    "${process_start_runner_sha256}" ]] ||
    fail "frozen ablation runner does not match the process-start source"
  expect_mode_owner_links "${frozen_runner}" 444 "frozen ablation runner"
  [[ "$(sha256_of "${frozen_helper}")" == \
    "${expected_affine_helper_sha256}" ]] ||
    fail "frozen affine helper hash drifted"
  expect_mode_owner_links "${frozen_helper}" 444 "frozen affine helper"
  [[ "$(sha256_of "${frozen_binary}")" == \
    "${expected_affine_binary_sha256}" ]] ||
    fail "frozen affine binary hash drifted"
  expect_mode_owner_links "${frozen_binary}" 555 "frozen affine binary"
  cmp -s -- "${script_path}" "${frozen_runner}" ||
    fail "live ablation runner differs from its frozen source"
  cmp -s -- "${affine_helper_source}" "${frozen_helper}" ||
    fail "trigger-bound affine helper differs from the frozen helper"
  cmp -s -- "${affine_binary_source}" "${frozen_binary}" ||
    fail "trigger-bound affine binary differs from the frozen binary"
}

generate_arm_files() {
  local arm="$1"
  local root policy net config capture candidate
  root="$(arm_root "${arm}")"
  policy="$(arm_policy "${arm}")"
  net="$(arm_net "${arm}")"
  config="$(arm_config "${arm}")"
  capture="$(arm_capture_config "${arm}")"
  mkdir -p "${root}/config"

  candidate="$(mktemp "${root}/config/.policy.XXXXXX")"
  case "${arm}" in
  endpoint_scale | time_only)
    cp -- "${canonical_policy}" "${candidate}"
    ;;
  no_tf_alignment)
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*LAMBDA_TF_ALIGN[[:space:]]*=[[:space:]]*0[.]10;[[:space:]]*$/ {
        sub(/0[.]10;/, "0.00;");
        changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${canonical_policy}" >"${candidate}" ||
      fail "could not derive no_tf_alignment policy"
    ;;
  *) fail "unknown challenger arm: ${arm}" ;;
  esac
  publish_immutable "${candidate}" "${policy}"

  candidate="$(mktemp "${root}/config/.net.XXXXXX")"
  case "${arm}" in
  endpoint_scale)
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*TIME_SCALES[[:space:]]*=[[:space:]]*8,16,32,64;[[:space:]]*$/ {
        sub(/8,16,32,64;/, "8,16,32,1;");
        changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${canonical_net}" >"${candidate}" ||
      fail "could not derive endpoint_scale net"
    ;;
  time_only)
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*USE_FREQUENCY_TOKENS[[:space:]]*=[[:space:]]*true;[[:space:]]*$/ {
        sub(/true;/, "false;");
        changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${canonical_net}" >"${candidate}" ||
      fail "could not derive time_only net"
    ;;
  no_tf_alignment)
    cp -- "${canonical_net}" "${candidate}"
    ;;
  *) fail "unknown challenger arm: ${arm}" ;;
  esac
  publish_immutable "${candidate}" "${net}"

  candidate="$(mktemp "${root}/config/.train_config.XXXXXX")"
  awk -v policy="${policy}" -v net="${net}" \
    -v bnf="${canonical_mtf_net_bnf}" '
    BEGIN {
      policy_changed = 0;
      net_changed = 0;
      bnf_inserted = 0;
      bnf_preexisting = 0;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " policy);
      policy_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path[[:space:]]*=/ {
      bnf_preexisting += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_path[[:space:]]*=/ {
      sub(/=.*/, "= " net);
      net_changed += 1;
      print;
      print "    wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path = " bnf;
      bnf_inserted += 1;
      next;
    }
    { print }
    END {
      if (policy_changed != 1 || net_changed != 1 ||
          bnf_inserted != 1 || bnf_preexisting != 0) exit 42;
    }
  ' "${base_training_config}" >"${candidate}" ||
    fail "could not derive training config for ${arm}"
  publish_immutable "${candidate}" "${config}"

  candidate="$(mktemp "${root}/config/.capture_config.XXXXXX")"
  awk -v policy="${policy}" -v net="${net}" \
    -v bnf="${canonical_mtf_net_bnf}" '
    BEGIN {
      policy_changed = 0;
      net_changed = 0;
      bnf_inserted = 0;
      bnf_preexisting = 0;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " policy);
      policy_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path[[:space:]]*=/ {
      bnf_preexisting += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_path[[:space:]]*=/ {
      sub(/=.*/, "= " net);
      net_changed += 1;
      print;
      print "    wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path = " bnf;
      bnf_inserted += 1;
      next;
    }
    { print }
    END {
      if (policy_changed != 1 || net_changed != 1 ||
          bnf_inserted != 1 || bnf_preexisting != 0) exit 42;
    }
  ' "${capture_config}" >"${candidate}" ||
    fail "could not derive capture config for ${arm}"
  publish_immutable "${candidate}" "${capture}"
  verify_arm_files "${arm}"
}

verify_arm_files() {
  local arm="$1"
  local policy net config capture reverse
  policy="$(arm_policy "${arm}")"
  net="$(arm_net "${arm}")"
  config="$(arm_config "${arm}")"
  capture="$(arm_capture_config "${arm}")"
  require_nonempty_file "${policy}"
  require_nonempty_file "${net}"
  require_nonempty_file "${config}"
  require_nonempty_file "${capture}"
  expect_kv "${policy}" TRAINING_ID "${canonical_training_id};"
  expect_kv "${policy}" SEED '17;'
  expect_kv "${policy}" MAX_STEPS '3000;'
  expect_kv "${net}" SCALE_STRIDES '4,8,16,32;'
  expect_kv "${config}" runtime_wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${config}" wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path \
    "${policy}"
  expect_kv "${config}" wikimyei_representation_mtf_jepa_mae_vicreg_net_path \
    "${net}"
  expect_kv "${config}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path \
    "${canonical_mtf_net_bnf}"
  expect_kv "${capture}" runtime_wave_id cwu_02v_certified_replay_eval_mdn
  expect_kv "${capture}" wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path \
    "${policy}"
  expect_kv "${capture}" wikimyei_representation_mtf_jepa_mae_vicreg_net_path \
    "${net}"
  expect_kv "${capture}" \
    wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path \
    "${canonical_mtf_net_bnf}"
  validate_isolated_config "${config}"
  validate_isolated_config "${capture}"

  reverse="$(mktemp /tmp/${schema_id}.reverse.XXXXXX)"
  case "${arm}" in
  endpoint_scale)
    cmp -s -- "${policy}" "${canonical_policy}" ||
      fail "endpoint_scale policy differs from canonical"
    expect_kv "${net}" TIME_SCALES '8,16,32,1;'
    expect_kv "${net}" USE_FREQUENCY_TOKENS 'true;'
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*TIME_SCALES[[:space:]]*=[[:space:]]*8,16,32,1;[[:space:]]*$/ {
        sub(/8,16,32,1;/, "8,16,32,64;"); changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${net}" >"${reverse}" || fail "endpoint_scale reverse substitution failed"
    cmp -s -- "${reverse}" "${canonical_net}" ||
      fail "endpoint_scale changes more than TIME_SCALES"
    ;;
  time_only)
    cmp -s -- "${policy}" "${canonical_policy}" ||
      fail "time_only policy differs from canonical"
    expect_kv "${net}" TIME_SCALES '8,16,32,64;'
    expect_kv "${net}" USE_FREQUENCY_TOKENS 'false;'
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*USE_FREQUENCY_TOKENS[[:space:]]*=[[:space:]]*false;[[:space:]]*$/ {
        sub(/false;/, "true;"); changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${net}" >"${reverse}" || fail "time_only reverse substitution failed"
    cmp -s -- "${reverse}" "${canonical_net}" ||
      fail "time_only changes more than USE_FREQUENCY_TOKENS"
    ;;
  no_tf_alignment)
    cmp -s -- "${net}" "${canonical_net}" ||
      fail "no_tf_alignment net differs from canonical"
    expect_kv "${policy}" LAMBDA_TF_ALIGN '0.00;'
    awk '
      BEGIN { changed = 0 }
      /^[[:space:]]*LAMBDA_TF_ALIGN[[:space:]]*=[[:space:]]*0[.]00;[[:space:]]*$/ {
        sub(/0[.]00;/, "0.10;"); changed += 1;
      }
      { print }
      END { if (changed != 1) exit 42 }
    ' "${policy}" >"${reverse}" ||
      fail "no_tf_alignment reverse substitution failed"
    cmp -s -- "${reverse}" "${canonical_policy}" ||
      fail "no_tf_alignment changes more than LAMBDA_TF_ALIGN"
    ;;
  *) rm -f -- "${reverse}"; fail "unknown challenger arm: ${arm}" ;;
  esac
  rm -f -- "${reverse}"

  reverse="$(mktemp /tmp/${schema_id}.config_reverse.XXXXXX)"
  awk -v policy="${canonical_policy}" -v net="${canonical_net}" \
    -v bnf="${canonical_mtf_net_bnf}" '
    BEGIN { policy_changed = 0; net_changed = 0; bnf_removed = 0; bad = 0 }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " policy); policy_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_path[[:space:]]*=/ {
      sub(/=.*/, "= " net); net_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path[[:space:]]*=/ {
      value = $0;
      sub(/^[^=]*=[[:space:]]*/, "", value);
      gsub(/[[:space:]]+$/, "", value);
      if (value != bnf) bad = 1;
      bnf_removed += 1;
      next;
    }
    { print }
    END {
      if (policy_changed != 1 || net_changed != 1 ||
          bnf_removed != 1 || bad != 0) exit 42;
    }
  ' "${config}" >"${reverse}" || fail "config reverse substitution failed"
  cmp -s -- "${reverse}" "${base_training_config}" ||
    fail "${arm} training config exceeds its two local paths and canonical grammar binding"
  rm -f -- "${reverse}"

  reverse="$(mktemp /tmp/${schema_id}.capture_config_reverse.XXXXXX)"
  awk -v policy="${canonical_policy}" -v net="${canonical_net}" \
    -v bnf="${canonical_mtf_net_bnf}" '
    BEGIN { policy_changed = 0; net_changed = 0; bnf_removed = 0; bad = 0 }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " policy); policy_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_path[[:space:]]*=/ {
      sub(/=.*/, "= " net); net_changed += 1;
    }
    /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path[[:space:]]*=/ {
      value = $0;
      sub(/^[^=]*=[[:space:]]*/, "", value);
      gsub(/[[:space:]]+$/, "", value);
      if (value != bnf) bad = 1;
      bnf_removed += 1;
      next;
    }
    { print }
    END {
      if (policy_changed != 1 || net_changed != 1 ||
          bnf_removed != 1 || bad != 0) exit 42;
    }
  ' "${capture}" >"${reverse}" ||
    fail "capture config reverse substitution failed"
  cmp -s -- "${reverse}" "${capture_config}" ||
    fail "${arm} capture config exceeds its two local paths and canonical grammar binding"
  rm -f -- "${reverse}"
}

generate_all_arm_files() {
  assert_operational_runner_identity
  local arm
  verify_base_training_config
  for arm in "${challenger_arms[@]}"; do
    generate_arm_files "${arm}"
  done
  assert_operational_runner_identity
}

config_optional_kv() {
  local config="$1" key="$2" count
  count="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) count += 1;
    }
    END { print count + 0 }
  ' "${config}")"
  [[ "${count}" =~ ^[01]$ ]] ||
    fail "${config}: expected at most one ${key}= entry, found ${count}"
  if [[ "${count}" == 0 ]]; then
    printf ''
  else
    kv "${key}" "${config}"
  fi
}

resolve_effective_config_path() {
  local config="$1" raw="$2" resolved
  [[ -n "${raw}" ]] || fail "empty authored configuration path in ${config}"
  if [[ "${raw}" == /* ]]; then
    resolved="$(realpath -m -- "${raw}")"
  else
    resolved="$(realpath -m -- "$(dirname "${config}")/${raw}")"
  fi
  require_nonempty_file "${resolved}"
  printf '%s' "${resolved}"
}

default_effective_grammar_path() {
  local data_path="$1" data_filename grammar_filename
  data_filename="$(basename -- "${data_path}")"
  [[ -n "${data_filename}" ]] ||
    fail "cannot derive grammar for empty data filename: ${data_path}"
  if [[ "${data_filename}" == kikijyeba.protocol.*.dsl && \
    "${data_filename}" != kikijyeba.protocol.dsl ]]; then
    grammar_filename="kikijyeba.protocol.dsl.bnf"
  else
    grammar_filename="${data_filename}.bnf"
  fi
  realpath -m -- "$(dirname "${data_path}")/grammar/${grammar_filename}"
}

emit_effective_grammar_config() {
  local config_index="$1" label="$2" config="$3"
  local config_tag key_index key_tag data_key data_raw data_path
  local grammar_key grammar_raw grammar_path grammar_origin
  config_tag="$(printf '%02d' "${config_index}")"
  require_nonempty_file "${config}"
  echo "config.${config_tag}.label=${label}"
  echo "config.${config_tag}.path=${config}"
  echo "config.${config_tag}.sha256=$(sha256_of "${config}")"
  echo "config.${config_tag}.effective_grammar_count=${#effective_grammar_data_keys[@]}"
  key_index=0
  for data_key in "${effective_grammar_data_keys[@]}"; do
    key_tag="$(printf '%02d' "${key_index}")"
    data_raw="$(kv "${data_key}" "${config}")"
    data_path="$(resolve_effective_config_path "${config}" "${data_raw}")"
    grammar_key="${data_key%_path}_bnf_path"
    grammar_raw="$(config_optional_kv "${config}" "${grammar_key}")"
    if [[ -n "${grammar_raw}" ]]; then
      grammar_origin=authored
      grammar_path="$(resolve_effective_config_path "${config}" "${grammar_raw}")"
    else
      grammar_origin=derived
      grammar_path="$(default_effective_grammar_path "${data_path}")"
      require_nonempty_file "${grammar_path}"
    fi
    if [[ "${data_key}" == \
      wikimyei_representation_mtf_jepa_mae_vicreg_net_path ]]; then
      [[ "${grammar_origin}" == authored ]] ||
        fail "MTF net grammar must be explicitly authored in ${config}"
      [[ "${grammar_path}" == "${canonical_mtf_net_bnf}" ]] ||
        fail "MTF net grammar is not canonical in ${config}: ${grammar_path}"
      [[ "$(sha256_of "${grammar_path}")" == \
        "${expected_canonical_mtf_net_bnf_sha256}" ]] ||
        fail "canonical MTF net grammar hash drifted"
    else
      [[ "${grammar_origin}" == derived ]] ||
        fail "unexpected authored grammar override ${grammar_key} in ${config}"
    fi
    echo "config.${config_tag}.grammar.${key_tag}.data_key=${data_key}"
    echo "config.${config_tag}.grammar.${key_tag}.data_path=${data_path}"
    echo "config.${config_tag}.grammar.${key_tag}.data_sha256=$(sha256_of "${data_path}")"
    echo "config.${config_tag}.grammar.${key_tag}.grammar_key=${grammar_key}"
    echo "config.${config_tag}.grammar.${key_tag}.origin=${grammar_origin}"
    echo "config.${config_tag}.grammar.${key_tag}.path=${grammar_path}"
    echo "config.${config_tag}.grammar.${key_tag}.sha256=$(sha256_of "${grammar_path}")"
    key_index=$((key_index + 1))
  done
  [[ "${key_index}" == 14 ]] ||
    fail "effective grammar key count drifted for ${config}: ${key_index}"
}

emit_effective_grammar_closure() {
  local destination="$1"
  {
    echo "schema_id=${schema_id}.effective_grammar_closure.v1"
    echo "status=complete"
    echo "resolver_contract=hero.config_derivation.default_grammar_path_for_data_path.v1"
    echo "relative_paths_resolve_against=config_parent"
    echo "kikijyeba_protocol_variant_grammar_filename=kikijyeba.protocol.dsl.bnf"
    echo "config_count=6"
    echo "effective_grammar_count_per_config=14"
    echo "effective_grammar_tuple_count=84"
    echo "authored_grammar_count_per_config=1"
    echo "derived_grammar_count_per_config=13"
    echo "runtime_dry_run_job_count=0"
    echo "canonical_mtf_net_bnf_path=${canonical_mtf_net_bnf}"
    echo "canonical_mtf_net_bnf_sha256=${expected_canonical_mtf_net_bnf_sha256}"
    emit_effective_grammar_config 0 endpoint_scale.train \
      "$(arm_config endpoint_scale)"
    emit_effective_grammar_config 1 endpoint_scale.capture \
      "$(arm_capture_config endpoint_scale)"
    emit_effective_grammar_config 2 time_only.train \
      "$(arm_config time_only)"
    emit_effective_grammar_config 3 time_only.capture \
      "$(arm_capture_config time_only)"
    emit_effective_grammar_config 4 no_tf_alignment.train \
      "$(arm_config no_tf_alignment)"
    emit_effective_grammar_config 5 no_tf_alignment.capture \
      "$(arm_capture_config no_tf_alignment)"
  } >"${destination}"
}

write_effective_grammar_closure() {
  assert_operational_runner_identity
  [[ "${#effective_grammar_data_keys[@]}" == 14 ]] ||
    fail "effective grammar key-set size drifted"
  local candidate
  candidate="$(mktemp "${runtime_root}/.effective_grammar.XXXXXX")"
  emit_effective_grammar_closure "${candidate}"
  publish_immutable "${candidate}" "${effective_grammar_closure}"
  assert_operational_runner_identity
}

verify_effective_grammar_closure() {
  require_immutable_file "${effective_grammar_closure}"
  expect_kv "${effective_grammar_closure}" schema_id \
    "${schema_id}.effective_grammar_closure.v1"
  expect_kv "${effective_grammar_closure}" status complete
  expect_kv "${effective_grammar_closure}" config_count 6
  expect_kv "${effective_grammar_closure}" \
    effective_grammar_count_per_config 14
  expect_kv "${effective_grammar_closure}" effective_grammar_tuple_count 84
  expect_kv "${effective_grammar_closure}" authored_grammar_count_per_config 1
  expect_kv "${effective_grammar_closure}" derived_grammar_count_per_config 13
  expect_kv "${effective_grammar_closure}" runtime_dry_run_job_count 0
  expect_kv "${effective_grammar_closure}" canonical_mtf_net_bnf_path \
    "${canonical_mtf_net_bnf}"
  expect_kv "${effective_grammar_closure}" canonical_mtf_net_bnf_sha256 \
    "${expected_canonical_mtf_net_bnf_sha256}"
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.effective_grammar_verify.XXXXXX)"
  emit_effective_grammar_closure "${candidate}"
  cmp -s -- "${candidate}" "${effective_grammar_closure}" || {
    rm -f -- "${candidate}"
    fail "effective grammar closure drifted"
  }
  rm -f -- "${candidate}"
}

emit_config_closure() {
  local destination="$1"
  local paths_file path count=0 index=0 formatted config arm
  paths_file="$(mktemp /tmp/${schema_id}.config_paths.XXXXXX)"
  {
    printf '%s\n' "${base_training_config}" "${capture_config}" \
      "${canonical_untrained_capture_config}" \
      "${canonical_untrained_mdn_policy}" \
      "${canonical_policy}" "${canonical_net}" "${canonical_mtf_net_bnf}" \
      "${effective_grammar_closure}" "${recovery_amendment}" \
      "${failure_closure_verifier}" "${failure_closure_receipt}" \
      "${failure_regular_files_inventory}" \
      "${failure_directories_inventory}" "${isolated_source_closure}" \
      "${isolated_registry}" "${isolated_source_verifier}" \
      "${cursor_alignment_erratum_verifier}" \
      "${representation_runner}" "${capture_runner}" \
      "${affine_runner}" "${operational_affine_runner}" \
      "${helper_source}" "${affine_runner_source}" "${cuda_correction}" \
      "${canonical_capture_development}" "${route_trigger}" \
      "${canonical_affine_development_status}" \
      "${canonical_affine_master_manifest}" \
      "${canonical_affine_execution_contract}" \
      "${canonical_affine_binary}" "${canonical_raw96_report}" \
      "${canonical_raw96_replay_report}" "${canonical_post384_report}" \
      "${canonical_post384_replay_report}" \
      "${canonical_untrained_report}" \
      "${canonical_untrained_replay_report}" \
      "${mdn_execution_runner}" "${mdn_completion_closure_wrapper}" \
      "${mdn_completion_closure_receipt}" "${mdn_completion_erratum}" \
      "${mdn_final_sealer}" "${mdn_completion_correction}" \
      "${canonical_mdn_result}" "${mdn_checkpoint}" \
      "${mdn_train_config}" "${mdn_policy_source}" \
      "${source_isolation_amendment}" "${isolated_source_protocol}" \
      "${staged_hardening}" "${cursor_alignment_correction}" \
      "${cursor_alignment_metadata_erratum}" \
      "${cursor_alignment_erratum_receipt}" \
      "${fresh_preregistration}" \
      "${diagnostic_preregistration}" "${diagnostic_amendment}" \
      "${localization_addendum}" "${conditional_amendment}" \
      "${preregistration}"
    for arm in "${challenger_arms[@]}"; do
      printf '%s\n' "$(arm_policy "${arm}")" "$(arm_net "${arm}")" \
        "$(arm_config "${arm}")" "$(arm_capture_config "${arm}")"
    done
    for config in "${base_training_config}" "${capture_config}" \
      "${canonical_untrained_capture_config}" \
      "$(arm_config endpoint_scale)" "$(arm_config time_only)" \
      "$(arm_config no_tf_alignment)" \
      "$(arm_capture_config endpoint_scale)" \
      "$(arm_capture_config time_only)" \
      "$(arm_capture_config no_tf_alignment)"; do
      awk '
        {
          eq = index($0, "=");
          if (eq == 0) next;
          key = substr($0, 1, eq - 1);
          value = substr($0, eq + 1);
          gsub(/^[[:space:]]+|[[:space:]]+$/, "", key);
          gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
          if (key ~ /_path$/ && value ~ /^\//) print value;
        }
      ' "${config}"
    done
  } | LC_ALL=C sort -u >"${paths_file}"
  count="$(wc -l <"${paths_file}")"
  {
    echo "schema_id=${schema_id}.config_inputs.v1"
    echo "status=complete"
    echo "entry_count=${count}"
    while IFS= read -r path; do
      require_nonempty_file "${path}"
      [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
        fail "configuration input is not canonical: ${path}"
      formatted="$(printf '%03d' "${index}")"
      echo "entry.${formatted}.path=${path}"
      echo "entry.${formatted}.sha256=$(sha256_of "${path}")"
      index=$((index + 1))
    done <"${paths_file}"
  } >"${destination}"
  rm -f -- "${paths_file}"
}

write_config_closure() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/.config_inputs.XXXXXX")"
  emit_config_closure "${candidate}"
  publish_immutable "${candidate}" "${config_closure}"
  assert_operational_runner_identity
}

verify_config_closure() {
  require_nonempty_file "${config_closure}"
  expect_kv "${config_closure}" schema_id "${schema_id}.config_inputs.v1"
  expect_kv "${config_closure}" status complete
  local candidate count index formatted path
  candidate="$(mktemp /tmp/${schema_id}.config_verify.XXXXXX)"
  emit_config_closure "${candidate}"
  cmp -s -- "${candidate}" "${config_closure}" || {
    rm -f -- "${candidate}"
    fail "configuration input closure drifted"
  }
  rm -f -- "${candidate}"
  count="$(kv entry_count "${config_closure}")"
  [[ "${count}" =~ ^[0-9]+$ ]] || fail "invalid config closure entry count"
  for ((index = 0; index < count; ++index)); do
    formatted="$(printf '%03d' "${index}")"
    path="$(kv "entry.${formatted}.path" "${config_closure}")"
    require_nonempty_file "${path}"
    expect_kv "${config_closure}" "entry.${formatted}.sha256" \
      "$(sha256_of "${path}")"
  done
}

emit_inputs() {
  local destination="$1"
  local arm
  {
    echo "schema_id=${schema_id}.inputs.v1"
    echo "status=complete"
    emit_ablation_runner_bindings
    echo "runner_path=${script_path}"
    echo "runner_sha256=${process_start_runner_sha256}"
    echo "frozen_runner_path=${frozen_runner}"
    echo "frozen_runner_sha256=$(sha256_of "${frozen_runner}")"
    emit_recovery_authority_bindings
    echo "preregistration_path=${preregistration}"
    echo "preregistration_sha256=$(sha256_of "${preregistration}")"
    echo "conditional_amendment_path=${conditional_amendment}"
    echo "conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")"
    echo "source_isolation_amendment_path=${source_isolation_amendment}"
    echo "source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")"
    echo "isolated_source_protocol_path=${isolated_source_protocol}"
    echo "isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")"
    echo "staged_hardening_amendment_path=${staged_hardening}"
    echo "staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")"
    echo "cursor_alignment_correction_path=${cursor_alignment_correction}"
    echo "cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")"
    emit_cursor_alignment_erratum_binding
    echo "fresh_preregistration_path=${fresh_preregistration}"
    echo "fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")"
    echo "diagnostic_preregistration_path=${diagnostic_preregistration}"
    echo "diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")"
    echo "diagnostic_amendment_path=${diagnostic_amendment}"
    echo "diagnostic_amendment_sha256=$(sha256_of "${diagnostic_amendment}")"
    echo "localization_addendum_path=${localization_addendum}"
    echo "localization_addendum_sha256=$(sha256_of "${localization_addendum}")"
    echo "data_closure_path=${closure_report}"
    echo "data_closure_sha256=$(sha256_of "${closure_report}")"
    echo "isolated_source_closure_path=${isolated_source_closure}"
    echo "isolated_source_closure_sha256=${expected_source_closure_sha256}"
    echo "isolated_source_verifier_path=${isolated_source_verifier}"
    echo "isolated_source_verifier_sha256=${expected_source_verifier_sha256}"
    echo "isolated_source_root=${isolated_source_root}"
    echo "isolated_registry_path=${isolated_registry}"
    echo "isolated_registry_sha256=$(sha256_of "${isolated_registry}")"
    echo "authoritative_accepted_anchor_count=3261"
    echo "authoritative_candidate_anchor_count=3261"
    echo "authoritative_maximum_anchor_index=3260"
    echo "route_trigger_path=${route_trigger}"
    echo "route_trigger_sha256=${expected_route_trigger_sha256}"
    echo "required_route=representation_ablation_screen"
    echo "canonical_capture_development_path=${canonical_capture_development}"
    echo "canonical_capture_development_sha256=${expected_capture_development_sha256}"
    echo "canonical_affine_development_path=${canonical_affine_development_status}"
    echo "canonical_affine_development_sha256=${expected_affine_development_status_sha256}"
    echo "canonical_affine_master_manifest_path=${canonical_affine_master_manifest}"
    echo "canonical_affine_master_manifest_sha256=${expected_affine_master_manifest_sha256}"
    echo "canonical_affine_execution_contract_path=${canonical_affine_execution_contract}"
    echo "canonical_affine_execution_contract_sha256=${expected_affine_execution_contract_sha256}"
    echo "capture_runner_path=${capture_runner}"
    echo "capture_runner_sha256=${expected_capture_runner_sha256}"
    echo "representation_runner_path=${representation_runner}"
    echo "representation_runner_sha256=$(sha256_of "${representation_runner}")"
    echo "affine_runner_path=${affine_runner_source}"
    echo "affine_runner_sha256=${expected_scientific_affine_runner_sha256}"
    echo "scientific_affine_runner_path=${scientific_affine_runner}"
    echo "scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}"
    echo "operational_affine_runner_path=${operational_affine_runner}"
    echo "operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}"
    echo "cuda_canonical_path_correction_path=${cuda_correction}"
    echo "cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}"
    echo "live_affine_helper_source_path=${helper_source}"
    echo "live_affine_helper_source_sha256=$(sha256_of "${helper_source}")"
    echo "affine_helper_path=${frozen_helper}"
    echo "affine_helper_sha256=$(sha256_of "${frozen_helper}")"
    echo "affine_binary_path=${frozen_binary}"
    echo "affine_binary_sha256=$(sha256_of "${frozen_binary}")"
    echo "sealed_affine_binary_path=${canonical_affine_binary}"
    echo "sealed_affine_binary_sha256=${expected_affine_binary_sha256}"
    echo "sealed_raw96_report_path=${canonical_raw96_report}"
    echo "sealed_raw96_report_sha256=${expected_raw96_report_sha256}"
    echo "sealed_post384_report_path=${canonical_post384_report}"
    echo "sealed_post384_report_sha256=${expected_post384_report_sha256}"
    echo "sealed_untrained_report_path=${canonical_untrained_report}"
    echo "sealed_untrained_report_sha256=${expected_untrained_report_sha256}"
    echo "runtime_exec_path=${runtime_exec}"
    echo "runtime_exec_sha256=$(sha256_of "${runtime_exec}")"
    echo "config_closure_path=${config_closure}"
    echo "config_closure_sha256=$(sha256_of "${config_closure}")"
    echo "effective_grammar_closure_path=${effective_grammar_closure}"
    echo "effective_grammar_closure_sha256=$(sha256_of "${effective_grammar_closure}")"
    echo "canonical_mtf_net_bnf_path=${canonical_mtf_net_bnf}"
    echo "canonical_mtf_net_bnf_sha256=${expected_canonical_mtf_net_bnf_sha256}"
    echo "base_training_config_path=${base_training_config}"
    echo "base_training_config_sha256=$(sha256_of "${base_training_config}")"
    echo "capture_config_path=${capture_config}"
    echo "capture_config_sha256=$(sha256_of "${capture_config}")"
    echo "canonical_untrained_capture_config_path=${canonical_untrained_capture_config}"
    echo "canonical_untrained_capture_config_sha256=$(sha256_of "${canonical_untrained_capture_config}")"
    echo "canonical_untrained_mdn_policy_path=${canonical_untrained_mdn_policy}"
    echo "canonical_untrained_mdn_policy_sha256=$(sha256_of "${canonical_untrained_mdn_policy}")"
    echo "canonical_representation_result_path=${canonical_representation_result}"
    echo "canonical_representation_result_sha256=$(sha256_of "${canonical_representation_result}")"
    echo "canonical_mdn_result_path=${canonical_mdn_result}"
    echo "canonical_mdn_result_sha256=${expected_mdn_result_sha256}"
    emit_mdn_retry1_authority_bindings
    echo "canonical_checkpoint_path=${canonical_checkpoint}"
    echo "canonical_checkpoint_sha256=$(sha256_of "${canonical_checkpoint}")"
    echo "canonical_train_probe_path=${canonical_train_probe}"
    echo "canonical_train_probe_sha256=$(sha256_of "${canonical_train_probe}")"
    echo "canonical_validation_probe_path=${canonical_validation_probe}"
    echo "canonical_validation_probe_sha256=$(sha256_of "${canonical_validation_probe}")"
    echo "canonical_validation_report_path=${canonical_report}"
    echo "canonical_validation_report_sha256=$(sha256_of "${canonical_report}")"
    for arm in "${challenger_arms[@]}"; do
      echo "arm.${arm}.policy_path=$(arm_policy "${arm}")"
      echo "arm.${arm}.policy_sha256=$(sha256_of "$(arm_policy "${arm}")")"
      echo "arm.${arm}.net_path=$(arm_net "${arm}")"
      echo "arm.${arm}.net_sha256=$(sha256_of "$(arm_net "${arm}")")"
      echo "arm.${arm}.config_path=$(arm_config "${arm}")"
      echo "arm.${arm}.config_sha256=$(sha256_of "$(arm_config "${arm}")")"
      echo "arm.${arm}.capture_config_path=$(arm_capture_config "${arm}")"
      echo "arm.${arm}.capture_config_sha256=$(sha256_of "$(arm_capture_config "${arm}")")"
    done
    echo "challenger_count=3"
    echo "challenger_seed=17"
    echo "challenger_optimizer_steps=${expected_steps}"
    echo "train_anchor_range=[${train_begin},${train_end})"
    echo "validation_anchor_range=[${validation_begin},${validation_end})"
    echo "maximum_development_anchor_read=$((validation_end - 1))"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

write_inputs() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
  emit_inputs "${candidate}"
  publish_immutable "${candidate}" "${input_receipt}"
  assert_operational_runner_identity
}

verify_inputs() {
  require_nonempty_file "${input_receipt}"
  verify_ablation_runner_bindings "${input_receipt}"
  verify_recovery_authority_bindings "${input_receipt}"
  verify_mdn_retry1_authority_bindings "${input_receipt}"
  expect_kv "${input_receipt}" effective_grammar_closure_path \
    "${effective_grammar_closure}"
  expect_kv "${input_receipt}" effective_grammar_closure_sha256 \
    "$(sha256_of "${effective_grammar_closure}")"
  expect_kv "${input_receipt}" canonical_mtf_net_bnf_path \
    "${canonical_mtf_net_bnf}"
  expect_kv "${input_receipt}" canonical_mtf_net_bnf_sha256 \
    "${expected_canonical_mtf_net_bnf_sha256}"
  expect_kv "${input_receipt}" route_trigger_sha256 \
    "${expected_route_trigger_sha256}"
  expect_kv "${input_receipt}" canonical_capture_development_sha256 \
    "${expected_capture_development_sha256}"
  expect_kv "${input_receipt}" canonical_affine_development_sha256 \
    "${expected_affine_development_status_sha256}"
  expect_kv "${input_receipt}" canonical_affine_master_manifest_sha256 \
    "${expected_affine_master_manifest_sha256}"
  expect_kv "${input_receipt}" \
    canonical_affine_execution_contract_sha256 \
    "${expected_affine_execution_contract_sha256}"
  expect_kv "${input_receipt}" operational_affine_runner_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${input_receipt}" cuda_canonical_path_correction_sha256 \
    "${expected_cuda_correction_sha256}"
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.inputs_verify.XXXXXX)"
  emit_inputs "${candidate}"
  cmp -s -- "${candidate}" "${input_receipt}" || {
    rm -f -- "${candidate}"
    fail "ablation input receipt drifted"
  }
  rm -f -- "${candidate}"
  expect_kv "${input_receipt}" required_route representation_ablation_screen
  expect_kv "${input_receipt}" challenger_count 3
  expect_kv "${input_receipt}" challenger_seed 17
  expect_kv "${input_receipt}" challenger_optimizer_steps "${expected_steps}"
  expect_kv "${input_receipt}" authoritative_accepted_anchor_count 3261
  expect_kv "${input_receipt}" authoritative_candidate_anchor_count 3261
  expect_kv "${input_receipt}" authoritative_maximum_anchor_index 3260
  expect_kv "${input_receipt}" canonical_data_raw_access false
  expect_kv "${input_receipt}" certified_input_access false
  expect_kv "${input_receipt}" final_holdout_access false
}

emit_retry_attempt_sentinel() {
  local destination="$1"
  {
    echo "schema_id=${schema_id}.development_retry_attempt.v1"
    echo "status=consumed"
    echo "immutable_mode=0444"
    echo "attempt_ordinal=1"
    emit_ablation_runner_bindings
    emit_recovery_authority_bindings
    echo "config_closure_path=${config_closure}"
    echo "config_closure_sha256=$(sha256_of "${config_closure}")"
    echo "effective_grammar_closure_path=${effective_grammar_closure}"
    echo "effective_grammar_closure_sha256=$(sha256_of "${effective_grammar_closure}")"
    echo "input_receipt_path=${input_receipt}"
    echo "input_receipt_sha256=$(sha256_of "${input_receipt}")"
    echo "canonical_mtf_net_bnf_path=${canonical_mtf_net_bnf}"
    echo "canonical_mtf_net_bnf_sha256=${expected_canonical_mtf_net_bnf_sha256}"
    echo "published_after_config_closure_verification=true"
    echo "published_after_effective_grammar_verification=true"
    echo "published_before_canonical_import=true"
    echo "published_before_first_runtime_call=true"
    echo "canonical_import_created_at_publication=false"
    echo "runtime_job_created_at_publication=false"
    echo "optimizer_steps_at_publication=0"
    echo "retry_scope=prejob_config_path_recovery"
    echo "additional_development_retries_authorized=false"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_retry_attempt_sentinel() {
  require_immutable_file "${retry_attempt_sentinel}"
  verify_ablation_runner_bindings "${retry_attempt_sentinel}"
  verify_recovery_authority_bindings "${retry_attempt_sentinel}"
  expect_kv "${retry_attempt_sentinel}" schema_id \
    "${schema_id}.development_retry_attempt.v1"
  expect_kv "${retry_attempt_sentinel}" status consumed
  expect_kv "${retry_attempt_sentinel}" immutable_mode 0444
  expect_kv "${retry_attempt_sentinel}" attempt_ordinal 1
  expect_kv "${retry_attempt_sentinel}" config_closure_path \
    "${config_closure}"
  expect_kv "${retry_attempt_sentinel}" config_closure_sha256 \
    "$(sha256_of "${config_closure}")"
  expect_kv "${retry_attempt_sentinel}" effective_grammar_closure_path \
    "${effective_grammar_closure}"
  expect_kv "${retry_attempt_sentinel}" effective_grammar_closure_sha256 \
    "$(sha256_of "${effective_grammar_closure}")"
  expect_kv "${retry_attempt_sentinel}" input_receipt_path "${input_receipt}"
  expect_kv "${retry_attempt_sentinel}" input_receipt_sha256 \
    "$(sha256_of "${input_receipt}")"
  expect_kv "${retry_attempt_sentinel}" \
    published_after_config_closure_verification true
  expect_kv "${retry_attempt_sentinel}" \
    published_after_effective_grammar_verification true
  expect_kv "${retry_attempt_sentinel}" published_before_canonical_import true
  expect_kv "${retry_attempt_sentinel}" published_before_first_runtime_call true
  expect_kv "${retry_attempt_sentinel}" additional_development_retries_authorized false
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.attempt_verify.XXXXXX)"
  emit_retry_attempt_sentinel "${candidate}"
  cmp -s -- "${candidate}" "${retry_attempt_sentinel}" || {
    rm -f -- "${candidate}"
    fail "immutable retry1 development attempt sentinel drifted"
  }
  rm -f -- "${candidate}"
}

publish_retry_attempt_sentinel_once() {
  assert_operational_runner_identity
  [[ ! -e "${retry_attempt_sentinel}" ]] ||
    fail "retry1 development attempt is already consumed"
  [[ ! -e "${canonical_import_receipt}" ]] ||
    fail "canonical import predates the retry1 attempt sentinel"
  if find "${arms_root}" -type f -name job.manifest -print -quit 2>/dev/null |
    grep -q .; then
    fail "Runtime job predates the retry1 attempt sentinel"
  fi
  verify_effective_grammar_closure
  verify_config_closure
  verify_inputs
  local candidate
  candidate="$(mktemp "${runtime_root}/.development_retry_attempt.XXXXXX")"
  emit_retry_attempt_sentinel "${candidate}"
  publish_immutable "${candidate}" "${retry_attempt_sentinel}"
  verify_retry_attempt_sentinel
  assert_operational_runner_identity
}

validate_training_job() {
  local arm="$1"
  local job result manifest report checkpoint expected_checkpoint
  job="$(arm_train_job "${arm}")"
  result="${job}/runtime.result.fact"
  manifest="${job}/job.manifest"
  report="${job}/channel_representation.report"
  require_nonempty_file "${result}"
  require_nonempty_file "${manifest}"
  require_nonempty_file "${report}"
  expect_kv "${result}" status completed
  expect_kv "${result}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${result}" wave_action train
  expect_kv "${result}" job_id \
    train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg
  expect_kv "${result}" source_report_path "${report}"
  expect_kv "${result}" optimizer_steps "${expected_steps}"
  expect_kv "${result}" checkpoint_written true
  expect_kv "${result}" model_state_mutated true
  expect_kv "${result}" finite_parameter_check true
  expect_kv "${result}" nonfinite_output_count 0
  expect_kv "${manifest}" config_path "$(arm_config "${arm}")"
  expect_kv "${manifest}" job_id \
    train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" execution_chain \
    'ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train'
  expect_kv "${manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${train_end}"
  validate_isolated_config "$(arm_config "${arm}")"
  validate_isolated_job_manifest "${manifest}" "${train_begin}" "${train_end}"
  expect_kv "${manifest}" input_representation_checkpoint_path ''
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  [[ ! -e "${job}/channel_inference.report" ]] ||
    fail "representation-only training produced a forecast report for ${arm}"
  [[ ! -e "${job}/channel_policy.report" ]] ||
    fail "representation-only training accessed a policy for ${arm}"
  expect_kv "${report}" training_id "${canonical_training_id}"
  expect_kv "${report}" wave_id train_core_mtf_jepa_mae_vicreg
  expect_kv "${report}" seed 17
  expect_kv "${report}" augmentation_profile light_phase_safe_v2
  expect_kv "${report}" requested_anchor_index_begin "${train_begin}"
  expect_kv "${report}" requested_anchor_index_end "${train_end}"
  expect_kv "${report}" optimizer_steps "${expected_steps}"
  expect_kv "${report}" finite_parameter_check true
  expect_kv "${report}" nonfinite_output_count 0
  case "${arm}" in
  endpoint_scale)
    expect_kv "${report}" use_frequency_tokens true
    expect_kv "${report}" lambda_tf_align 0.1
    ;;
  time_only)
    expect_kv "${report}" use_frequency_tokens false
    expect_kv "${report}" lambda_tf_align 0.1
    ;;
  no_tf_alignment)
    expect_kv "${report}" use_frequency_tokens true
    expect_kv "${report}" lambda_tf_align 0
    ;;
  *) fail "unknown challenger arm in training validation: ${arm}" ;;
  esac
  checkpoint="$(kv checkpoint_path "${result}")"
  expected_checkpoint="${job}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
  [[ "${checkpoint}" == "${expected_checkpoint}" ]] ||
    fail "unexpected representation checkpoint path for ${arm}: ${checkpoint}"
  require_nonempty_file "${checkpoint}"
  printf '%s' "${checkpoint}"
}

emit_training_status() {
  local arm="$1"
  local checkpoint="$2"
  local destination="$3"
  local job
  job="$(arm_train_job "${arm}")"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.training.v1
status=complete
arm=${arm}
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
retry_attempt_sentinel_path=${retry_attempt_sentinel}
retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
policy_path=$(arm_policy "${arm}")
policy_sha256=$(sha256_of "$(arm_policy "${arm}")")
net_path=$(arm_net "${arm}")
net_sha256=$(sha256_of "$(arm_net "${arm}")")
config_path=$(arm_config "${arm}")
config_sha256=$(sha256_of "$(arm_config "${arm}")")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
checkpoint_path=${checkpoint}
checkpoint_sha256=$(sha256_of "${checkpoint}")
job_manifest_path=${job}/job.manifest
job_manifest_sha256=$(sha256_of "${job}/job.manifest")
runtime_result_path=${job}/runtime.result.fact
runtime_result_sha256=$(sha256_of "${job}/runtime.result.fact")
representation_report_path=${job}/channel_representation.report
representation_report_sha256=$(sha256_of "${job}/channel_representation.report")
train_anchor_range=[${train_begin},${train_end})
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
optimizer_steps=${expected_steps}
seed=17
forecast_labels_used=false
canonical_data_raw_access=false
final_holdout_access=false
policy_access=false
STATUS
}

write_training_status() {
  assert_operational_runner_identity
  local arm="$1"
  local checkpoint="$2"
  local candidate
  candidate="$(mktemp "$(arm_root "${arm}")/.training_status.XXXXXX")"
  emit_training_status "${arm}" "${checkpoint}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_training_status "${arm}")"
  assert_operational_runner_identity
}

verify_training_status() {
  local arm="$1"
  local status checkpoint candidate
  status="$(arm_training_status "${arm}")"
  require_nonempty_file "${status}"
  expect_kv "${status}" retry_attempt_sentinel_path \
    "${retry_attempt_sentinel}"
  expect_kv "${status}" retry_attempt_sentinel_sha256 \
    "$(sha256_of "${retry_attempt_sentinel}")"
  checkpoint="$(validate_training_job "${arm}")"
  require_immutable_file "${checkpoint}"
  require_immutable_file "$(arm_train_job "${arm}")/job.manifest"
  require_immutable_file "$(arm_train_job "${arm}")/runtime.result.fact"
  require_immutable_file "$(arm_train_job "${arm}")/channel_representation.report"
  candidate="$(mktemp /tmp/${schema_id}.${arm}.training_verify.XXXXXX)"
  emit_training_status "${arm}" "${checkpoint}" "${candidate}"
  cmp -s -- "${candidate}" "${status}" || {
    rm -f -- "${candidate}"
    fail "training status drifted for ${arm}"
  }
  rm -f -- "${candidate}"
  printf '%s' "${checkpoint}"
}

run_training_arm() {
  assert_operational_runner_identity
  local arm="$1"
  local job checkpoint log
  job="$(arm_train_job "${arm}")"
  log="$(arm_root "${arm}")/training.log"
  if [[ -e "${job}" ]]; then
    [[ -e "$(arm_training_status "${arm}")" ]] ||
      fail "partial training job exists without status for ${arm}"
    verify_training_status "${arm}" >/dev/null
    assert_operational_runner_identity
    return
  fi
  [[ ! -e "$(arm_training_status "${arm}")" ]] ||
    fail "training status exists without a job for ${arm}"
  mkdir -p "$(arm_root "${arm}")/training"
  run_guarded_child "${runtime_exec}" \
    --config "$(arm_config "${arm}")" \
    --job-dir "${job}" \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "${arm} training failed; partial job is terminal; see ${log}"
  checkpoint="$(validate_training_job "${arm}")"
  chmod 0444 "${checkpoint}" "${job}/job.manifest" \
    "${job}/runtime.result.fact" "${job}/channel_representation.report"
  write_training_status "${arm}" "${checkpoint}"
  assert_operational_runner_identity
}

validate_probe_file() {
  local probe="$1"
  local begin="$2"
  local end="$3"
  local expected_rows="$4"
  local expected_schema="${5:-kikijyeba.synthetic.representation_edge_feature_probe.v1}"
  local expected_width="${6:-96}"
  local rows min_anchor max_anchor
  require_nonempty_file "${probe}"
  awk -F, -v schema="${expected_schema}" -v width="${expected_width}" '
    NR == 1 {
      expected = "record_schema,anchor_key,anchor_index,anchor_local_index," \
                 "edge_index,edge_id,base_node_id,quote_node_id," \
                 "channel_index,target_edge_close_return,feature_count," \
                 "feature_values";
      if ($0 != expected) exit 41;
      next;
    }
    NF != 12 || $1 != schema || $11 + 0 != width {
      exit 42;
    }
  ' "${probe}" || fail "raw96 probe schema mismatch: ${probe}"
  rows="$(awk 'NR > 1 { rows += 1 } END { print rows + 0 }' "${probe}")"
  [[ "${rows}" == "${expected_rows}" ]] ||
    fail "probe row mismatch: ${probe}: ${rows} != ${expected_rows}"
  min_anchor="$(awk -F, 'NR == 2 { min = $3 + 0 } NR > 1 && $3 + 0 < min { min = $3 + 0 } END { print min + 0 }' "${probe}")"
  max_anchor="$(awk -F, 'NR > 1 && $3 + 0 > max { max = $3 + 0 } END { print max + 0 }' "${probe}")"
  [[ "${min_anchor}" == "${begin}" && "${max_anchor}" == "$((end - 1))" ]] ||
    fail "probe anchor range mismatch: ${probe}"
}

validate_capture_job() {
  local job="$1"
  local begin="$2"
  local end="$3"
  local expected_rows="$4"
  local checkpoint="$5"
  local expected_config="$6"
  local result manifest report probe mdn_probe
  result="${job}/runtime.result.fact"
  manifest="${job}/job.manifest"
  report="${job}/channel_inference.report"
  probe="${job}/representation_edge_features.probe"
  mdn_probe="${job}/mdn_edge_context_features.probe"
  require_nonempty_file "${result}"
  require_nonempty_file "${manifest}"
  require_nonempty_file "${report}"
  require_nonempty_file "${probe}"
  require_nonempty_file "${mdn_probe}"
  expect_kv "${result}" status completed
  expect_kv "${result}" optimizer_steps 0
  expect_kv "${result}" checkpoint_written false
  expect_kv "${result}" model_state_mutated false
  expect_kv "${manifest}" config_path "${expected_config}"
  expect_kv "${manifest}" wave_id cwu_02v_certified_replay_eval_mdn
  expect_kv "${manifest}" wave_action run
  expect_kv "${manifest}" resolved_anchor_index_begin "${begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${end}"
  validate_isolated_config "${expected_config}"
  validate_isolated_job_manifest "${manifest}" "${begin}" "${end}"
  expect_kv "${manifest}" input_representation_checkpoint_path "${checkpoint}"
  expect_kv "${manifest}" input_mdn_checkpoint_path "${mdn_checkpoint}"
  [[ ! -e "${job}/channel_policy.report" ]] ||
    fail "feature capture accessed a policy: ${job}"
  expect_kv "${report}" representation_checkpoint_loaded true
  expect_kv "${report}" representation_checkpoint_path "${checkpoint}"
  expect_kv "${report}" mdn_checkpoint_loaded true
  expect_kv "${report}" mdn_checkpoint_path "${mdn_checkpoint}"
  expect_kv "${report}" requested_anchor_index_begin "${begin}"
  expect_kv "${report}" requested_anchor_index_end "${end}"
  expect_kv "${report}" wave_streamed_anchor_count "$((end - begin))"
  expect_kv "${report}" representation_edge_feature_probe_written true
  expect_kv "${report}" representation_edge_feature_probe_row_count \
    "${expected_rows}"
  expect_kv "${report}" representation_edge_feature_probe_path "${probe}"
  expect_kv "${report}" mdn_edge_context_feature_probe_written true
  expect_kv "${report}" mdn_edge_context_feature_probe_row_count \
    "${expected_rows}"
  expect_kv "${report}" mdn_edge_context_feature_probe_path "${mdn_probe}"
  validate_probe_file "${probe}" "${begin}" "${end}" "${expected_rows}"
  validate_probe_file "${mdn_probe}" "${begin}" "${end}" "${expected_rows}" \
    kikijyeba.synthetic.mdn_edge_context_feature_probe.v1 400
  printf '%s' "${probe}"
}

seal_capture_job_files() {
  local job="$1"
  local path
  for path in "${job}/job.manifest" "${job}/runtime.result.fact" \
    "${job}/channel_inference.report" \
    "${job}/representation_edge_features.probe" \
    "${job}/mdn_edge_context_features.probe"; do
    require_nonempty_file "${path}"
  done
  chmod 0444 "${job}/job.manifest" "${job}/runtime.result.fact" \
    "${job}/channel_inference.report" \
    "${job}/representation_edge_features.probe" \
    "${job}/mdn_edge_context_features.probe"
}

verify_capture_job_immutable() {
  local job="$1"
  require_immutable_file "${job}/job.manifest"
  require_immutable_file "${job}/runtime.result.fact"
  require_immutable_file "${job}/channel_inference.report"
  require_immutable_file "${job}/representation_edge_features.probe"
  require_immutable_file "${job}/mdn_edge_context_features.probe"
}

emit_capture_status() {
  local arm="$1"
  local checkpoint="$2"
  local train_probe="$3"
  local validation_probe="$4"
  local destination="$5"
  local train_job validation_job config
  train_job="$(arm_capture_job "${arm}" train)"
  validation_job="$(arm_capture_job "${arm}" validation)"
  config="$(arm_capture_config "${arm}")"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.capture.v1
status=complete
arm=${arm}
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
retry_attempt_sentinel_path=${retry_attempt_sentinel}
retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
config_path=${config}
config_sha256=$(sha256_of "${config}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_checkpoint_path=${checkpoint}
representation_checkpoint_sha256=$(sha256_of "${checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
train_mdn_probe_path=${train_job}/mdn_edge_context_features.probe
train_mdn_probe_sha256=$(sha256_of "${train_job}/mdn_edge_context_features.probe")
train_manifest_path=${train_job}/job.manifest
train_manifest_sha256=$(sha256_of "${train_job}/job.manifest")
train_runtime_result_path=${train_job}/runtime.result.fact
train_runtime_result_sha256=$(sha256_of "${train_job}/runtime.result.fact")
train_report_path=${train_job}/channel_inference.report
train_report_sha256=$(sha256_of "${train_job}/channel_inference.report")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
validation_mdn_probe_path=${validation_job}/mdn_edge_context_features.probe
validation_mdn_probe_sha256=$(sha256_of "${validation_job}/mdn_edge_context_features.probe")
validation_manifest_path=${validation_job}/job.manifest
validation_manifest_sha256=$(sha256_of "${validation_job}/job.manifest")
validation_runtime_result_path=${validation_job}/runtime.result.fact
validation_runtime_result_sha256=$(sha256_of "${validation_job}/runtime.result.fact")
validation_report_path=${validation_job}/channel_inference.report
validation_report_sha256=$(sha256_of "${validation_job}/channel_inference.report")
train_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
maximum_anchor_read=$((validation_end - 1))
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
policy_access=false
STATUS
}

write_capture_status() {
  assert_operational_runner_identity
  local arm="$1"
  local checkpoint="$2"
  local train_probe="$3"
  local validation_probe="$4"
  local candidate
  candidate="$(mktemp "$(arm_root "${arm}")/.capture_status.XXXXXX")"
  emit_capture_status "${arm}" "${checkpoint}" "${train_probe}" \
    "${validation_probe}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_capture_status "${arm}")"
  assert_operational_runner_identity
}

verify_capture_status() {
  local arm="$1"
  local status checkpoint config train_probe validation_probe candidate
  status="$(arm_capture_status "${arm}")"
  require_immutable_file "${status}"
  expect_kv "${status}" retry_attempt_sentinel_path \
    "${retry_attempt_sentinel}"
  expect_kv "${status}" retry_attempt_sentinel_sha256 \
    "$(sha256_of "${retry_attempt_sentinel}")"
  checkpoint="$(verify_training_status "${arm}")"
  config="$(arm_capture_config "${arm}")"
  train_probe="$(validate_capture_job "$(arm_capture_job "${arm}" train)" \
    "${train_begin}" "${train_end}" "${train_rows}" "${checkpoint}" \
    "${config}")"
  validation_probe="$(validate_capture_job \
    "$(arm_capture_job "${arm}" validation)" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${checkpoint}" "${config}")"
  verify_capture_job_immutable "$(arm_capture_job "${arm}" train)"
  verify_capture_job_immutable "$(arm_capture_job "${arm}" validation)"
  candidate="$(mktemp /tmp/${schema_id}.${arm}.capture_verify.XXXXXX)"
  emit_capture_status "${arm}" "${checkpoint}" "${train_probe}" \
    "${validation_probe}" "${candidate}"
  cmp -s -- "${candidate}" "${status}" || {
    rm -f -- "${candidate}"
    fail "capture status drifted for ${arm}"
  }
  rm -f -- "${candidate}"
}

run_capture_arm() {
  assert_operational_runner_identity
  local arm="$1"
  local status checkpoint config train_job validation_job train_probe
  local validation_probe log
  status="$(arm_capture_status "${arm}")"
  train_job="$(arm_capture_job "${arm}" train)"
  validation_job="$(arm_capture_job "${arm}" validation)"
  if [[ -e "${status}" ]]; then
    verify_capture_status "${arm}"
    assert_operational_runner_identity
    return
  fi
  [[ ! -e "${train_job}" && ! -e "${validation_job}" ]] ||
    fail "partial capture jobs exist without immutable status for ${arm}"
  checkpoint="$(verify_training_status "${arm}")"
  config="$(arm_capture_config "${arm}")"
  mkdir -p "$(arm_root "${arm}")/capture"
  log="$(arm_root "${arm}")/capture/train.log"
  run_guarded_child "${runtime_exec}" \
    --config "${config}" \
    --job-dir "${train_job}" \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --input-representation-checkpoint "${checkpoint}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "${arm} train capture failed; its partial job is terminal; see ${log}"
  train_probe="$(validate_capture_job "${train_job}" "${train_begin}" \
    "${train_end}" "${train_rows}" "${checkpoint}" "${config}")"
  seal_capture_job_files "${train_job}"
  log="$(arm_root "${arm}")/capture/validation.log"
  run_guarded_child "${runtime_exec}" \
    --config "${config}" \
    --job-dir "${validation_job}" \
    --source-range anchor_index \
    --anchor-index-begin "${validation_begin}" \
    --anchor-index-end "${validation_end}" \
    --input-representation-checkpoint "${checkpoint}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "${arm} validation capture failed; its partial job is terminal; see ${log}"
  validation_probe="$(validate_capture_job "${validation_job}" \
    "${validation_begin}" "${validation_end}" "${validation_rows}" \
    "${checkpoint}" "${config}")"
  seal_capture_job_files "${validation_job}"
  write_capture_status "${arm}" "${checkpoint}" "${train_probe}" \
    "${validation_probe}"
  assert_operational_runner_identity
}

require_number() {
  local value="$1"
  local field="$2"
  LC_ALL=C awk -v value="${value}" '
    BEGIN {
      number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
      if (value !~ number) exit 42;
    }
  ' || fail "invalid numeric field ${field}=${value}"
}

validate_development_report() {
  local report="$1"
  local strong direction rank correlation rmse_ratio conjunction=false
  require_immutable_file "${report}"
  expect_kv "${report}" schema_id \
    synthetic_v2_frozen_encoder_affine_development_v1
  expect_kv "${report}" status complete
  expect_kv "${report}" benchmark_id synthetic_continuous_graph_v2
  expect_kv "${report}" probe_kind representation
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows 0
  expect_kv "${report}" fit_anchor_range "[${train_begin},${train_end})"
  expect_kv "${report}" validation_anchor_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${report}" certified_anchor_range not_opened
  expect_kv "${report}" maximum_anchor_read 2815
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" refit_after_selection false
  expect_kv "${report}" certified_candidates_scored 0
  expect_kv "${report}" probe_feature_width 96
  expect_kv "${report}" affine_feature_width 96
  local report_tolerance
  report_tolerance="$(kv selection_tie_tolerance "${report}")"
  require_number "${report_tolerance}" selection_tie_tolerance
  LC_ALL=C awk -v actual="${report_tolerance}" -v expected="${tie_tolerance}" \
    'BEGIN { exit((actual + 0 == expected + 0) ? 0 : 1) }' ||
    fail "affine selection tolerance drifted in ${report}"
  expect_kv "${report}" classification development_selection_complete
  [[ "$(kv selected_candidate_index "${report}")" =~ ^[0-5]$ ]] ||
    fail "invalid selected ridge candidate in ${report}"
  local field
  for field in selected_ridge selected_maximum_normalized_residual \
    selected_coefficient_l2_norm \
    selected.validation.directional_accuracy \
    selected.validation.pairwise_rank_accuracy \
    selected.validation.correlation selected.validation.rmse \
    selected.validation.rmse_target_rms_ratio; do
    require_number "$(kv "${field}" "${report}")" "${field}"
  done
  direction="$(numeric_gate \
    "$(kv selected.validation.directional_accuracy "${report}")" ge 0.95)"
  rank="$(numeric_gate \
    "$(kv selected.validation.pairwise_rank_accuracy "${report}")" ge 0.95)"
  correlation="$(numeric_gate \
    "$(kv selected.validation.correlation "${report}")" ge 0.95)"
  rmse_ratio="$(numeric_gate \
    "$(kv selected.validation.rmse_target_rms_ratio "${report}")" le 0.25)"
  if [[ "${direction}" == true && "${rank}" == true && \
    "${correlation}" == true && "${rmse_ratio}" == true ]]; then
    conjunction=true
  fi
  strong="$(kv validation_strong_gate_pass "${report}")"
  [[ "${strong}" == "${conjunction}" ]] ||
    fail "development report strong gate disagrees with its metrics: ${report}"
}

emit_canonical_import() {
  local destination="$1"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.canonical_import.v1
status=complete
arm=canonical
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
retry_attempt_sentinel_path=${retry_attempt_sentinel}
retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")
route_trigger_path=${route_trigger}
route_trigger_sha256=${expected_route_trigger_sha256}
canonical_capture_development_path=${canonical_capture_development}
canonical_capture_development_sha256=${expected_capture_development_sha256}
canonical_affine_development_path=${canonical_affine_development_status}
canonical_affine_development_sha256=${expected_affine_development_status_sha256}
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
representation_checkpoint_path=${canonical_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${canonical_checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
capture_config_path=${capture_config}
capture_config_sha256=$(sha256_of "${capture_config}")
train_probe_path=${canonical_train_probe}
train_probe_sha256=$(sha256_of "${canonical_train_probe}")
validation_probe_path=${canonical_validation_probe}
validation_probe_sha256=$(sha256_of "${canonical_validation_probe}")
source_main_report_path=${canonical_report}
source_main_report_sha256=$(sha256_of "${canonical_report}")
source_replay_report_path=${canonical_replay_report}
source_replay_report_sha256=$(sha256_of "${canonical_replay_report}")
imported_main_report_path=$(arm_main_report canonical)
imported_main_report_sha256=$(sha256_of "$(arm_main_report canonical)")
imported_replay_report_path=$(arm_replay_report canonical)
imported_replay_report_sha256=$(sha256_of "$(arm_replay_report canonical)")
maximum_anchor_read=2815
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
certified_input_access=false
canonical_data_raw_access=false
final_holdout_access=false
policy_access=false
STATUS
}

verify_canonical_import() {
  require_immutable_file "${canonical_import_receipt}"
  expect_kv "${canonical_import_receipt}" retry_attempt_sentinel_path \
    "${retry_attempt_sentinel}"
  expect_kv "${canonical_import_receipt}" retry_attempt_sentinel_sha256 \
    "$(sha256_of "${retry_attempt_sentinel}")"
  require_immutable_file "$(arm_main_report canonical)"
  require_immutable_file "$(arm_replay_report canonical)"
  cmp -s -- "${canonical_report}" "$(arm_main_report canonical)" ||
    fail "canonical imported main report differs from source"
  cmp -s -- "${canonical_replay_report}" "$(arm_replay_report canonical)" ||
    fail "canonical imported replay report differs from source"
  cmp -s -- "$(arm_main_report canonical)" "$(arm_replay_report canonical)" ||
    fail "canonical imported main/replay reports differ"
  validate_development_report "$(arm_main_report canonical)"
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.canonical_import_verify.XXXXXX)"
  emit_canonical_import "${candidate}"
  cmp -s -- "${candidate}" "${canonical_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "canonical import receipt drifted"
  }
  rm -f -- "${candidate}"
}

import_canonical_arm() {
  assert_operational_runner_identity
  if [[ -e "${canonical_import_receipt}" ]]; then
    verify_canonical_import
    assert_operational_runner_identity
    return
  fi
  [[ ! -e "$(arm_main_report canonical)" && \
    ! -e "$(arm_replay_report canonical)" ]] ||
    fail "partial canonical import exists without immutable receipt"
  mkdir -p "$(arm_root canonical)/affine"
  local candidate
  candidate="$(mktemp "$(arm_root canonical)/affine/.main.XXXXXX")"
  cp -- "${canonical_report}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_main_report canonical)"
  candidate="$(mktemp "$(arm_root canonical)/affine/.replay.XXXXXX")"
  cp -- "${canonical_replay_report}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_replay_report canonical)"
  candidate="$(mktemp "$(arm_root canonical)/.import.XXXXXX")"
  emit_canonical_import "${candidate}"
  publish_immutable "${candidate}" "${canonical_import_receipt}"
  verify_canonical_import
  assert_operational_runner_identity
}

emit_affine_status() {
  local arm="$1"
  local destination="$2"
  local capture_status train_probe validation_probe
  capture_status="$(arm_capture_status "${arm}")"
  train_probe="$(kv train_probe_path "${capture_status}")"
  validation_probe="$(kv validation_probe_path "${capture_status}")"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.affine_development.v1
status=complete
arm=${arm}
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
affine_runner_source_path=${affine_runner_source}
affine_runner_source_sha256=$(sha256_of "${affine_runner_source}")
affine_helper_source_path=${frozen_helper}
affine_helper_source_sha256=$(sha256_of "${frozen_helper}")
affine_binary_path=${frozen_binary}
affine_binary_sha256=$(sha256_of "${frozen_binary}")
capture_status_path=${capture_status}
capture_status_sha256=$(sha256_of "${capture_status}")
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
main_report_path=$(arm_main_report "${arm}")
main_report_sha256=$(sha256_of "$(arm_main_report "${arm}")")
replay_report_path=$(arm_replay_report "${arm}")
replay_report_sha256=$(sha256_of "$(arm_replay_report "${arm}")")
main_stdout_log_path=$(arm_main_log "${arm}")
main_stdout_log_sha256=$(sha256_of "$(arm_main_log "${arm}")")
replay_stdout_log_path=$(arm_replay_log "${arm}")
replay_stdout_log_sha256=$(sha256_of "$(arm_replay_log "${arm}")")
selected_candidate_index=$(kv selected_candidate_index "$(arm_main_report "${arm}")")
selected_ridge=$(kv selected_ridge "$(arm_main_report "${arm}")")
validation_directional_accuracy=$(kv selected.validation.directional_accuracy "$(arm_main_report "${arm}")")
validation_pairwise_rank_accuracy=$(kv selected.validation.pairwise_rank_accuracy "$(arm_main_report "${arm}")")
validation_correlation=$(kv selected.validation.correlation "$(arm_main_report "${arm}")")
validation_rmse=$(kv selected.validation.rmse "$(arm_main_report "${arm}")")
validation_rmse_target_rms_ratio=$(kv selected.validation.rmse_target_rms_ratio "$(arm_main_report "${arm}")")
validation_strong_gate_pass=$(kv validation_strong_gate_pass "$(arm_main_report "${arm}")")
development_only=true
main_replay_byte_identical=true
maximum_anchor_read=2815
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
certified_input_access=false
canonical_data_raw_access=false
final_holdout_access=false
policy_access=false
STATUS
}

verify_affine_status() {
  local arm="$1"
  local status candidate
  status="$(arm_affine_status "${arm}")"
  require_immutable_file "${status}"
  verify_capture_status "${arm}"
  require_immutable_file "$(arm_main_report "${arm}")"
  require_immutable_file "$(arm_replay_report "${arm}")"
  require_file "$(arm_main_log "${arm}")"
  require_file "$(arm_replay_log "${arm}")"
  [[ "$(stat -c '%A' -- "$(arm_main_log "${arm}")")" != *w* ]] ||
    fail "affine main stdout log is writable for ${arm}"
  [[ "$(stat -c '%A' -- "$(arm_replay_log "${arm}")")" != *w* ]] ||
    fail "affine replay stdout log is writable for ${arm}"
  cmp -s -- "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" ||
    fail "development affine main/replay reports differ for ${arm}"
  cmp -s -- "$(arm_main_log "${arm}")" "$(arm_replay_log "${arm}")" ||
    fail "development affine main/replay logs differ for ${arm}"
  validate_development_report "$(arm_main_report "${arm}")"
  candidate="$(mktemp /tmp/${schema_id}.${arm}.affine_verify.XXXXXX)"
  emit_affine_status "${arm}" "${candidate}"
  cmp -s -- "${candidate}" "${status}" || {
    rm -f -- "${candidate}"
    fail "development affine status drifted for ${arm}"
  }
  rm -f -- "${candidate}"
}

run_affine_arm() {
  assert_operational_runner_identity
  local arm="$1"
  local status train_probe validation_probe affine_dir artifact candidate
  status="$(arm_affine_status "${arm}")"
  if [[ -e "${status}" ]]; then
    verify_affine_status "${arm}"
    assert_operational_runner_identity
    return
  fi
  for artifact in "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" "$(arm_main_log "${arm}")" \
    "$(arm_replay_log "${arm}")"; do
    [[ ! -e "${artifact}" ]] ||
      fail "partial affine evaluation exists without status for ${arm}: ${artifact}"
  done
  verify_capture_status "${arm}"
  train_probe="$(kv train_probe_path "$(arm_capture_status "${arm}")")"
  validation_probe="$(kv validation_probe_path \
    "$(arm_capture_status "${arm}")")"
  affine_dir="$(arm_root "${arm}")/affine"
  mkdir -p "${affine_dir}"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --development-only \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --output "$(arm_main_report "${arm}")" \
    >"$(arm_main_log "${arm}")" 2>&1 ||
    fail "development affine main execution failed for ${arm}; attempt is terminal"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --development-only \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --output "$(arm_replay_report "${arm}")" \
    >"$(arm_replay_log "${arm}")" 2>&1 ||
    fail "development affine replay execution failed for ${arm}; attempt is terminal"
  chmod 0444 "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" "$(arm_main_log "${arm}")" \
    "$(arm_replay_log "${arm}")"
  validate_development_report "$(arm_main_report "${arm}")"
  cmp -s -- "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" ||
    fail "development affine main/replay reports differ for ${arm}"
  cmp -s -- "$(arm_main_log "${arm}")" "$(arm_replay_log "${arm}")" ||
    fail "development affine main/replay logs differ for ${arm}"
  candidate="$(mktemp "$(arm_root "${arm}")/.affine_status.XXXXXX")"
  emit_affine_status "${arm}" "${candidate}"
  publish_immutable "${candidate}" "${status}"
  assert_operational_runner_identity
}

arm_checkpoint_path() {
  local arm="$1"
  if [[ "${arm}" == canonical ]]; then
    printf '%s' "${canonical_checkpoint}"
  else
    bound_file "$(arm_training_status "${arm}")" checkpoint_path \
      checkpoint_sha256
  fi
}

arm_capture_config_path() {
  local arm="$1"
  if [[ "${arm}" == canonical ]]; then
    printf '%s' "${capture_config}"
  else
    printf '%s' "$(arm_capture_config "${arm}")"
  fi
}

arm_train_probe_path() {
  local arm="$1"
  if [[ "${arm}" == canonical ]]; then
    printf '%s' "${canonical_train_probe}"
  else
    bound_file "$(arm_capture_status "${arm}")" train_probe_path \
      train_probe_sha256
  fi
}

arm_validation_probe_path() {
  local arm="$1"
  if [[ "${arm}" == canonical ]]; then
    printf '%s' "${canonical_validation_probe}"
  else
    bound_file "$(arm_capture_status "${arm}")" validation_probe_path \
      validation_probe_sha256
  fi
}

report_is_better() {
  local candidate_report="$1"
  local incumbent_report="$2"
  LC_ALL=C awk -v tolerance="${tie_tolerance}" \
    -v candidate_direction="$(kv selected.validation.directional_accuracy "${candidate_report}")" \
    -v incumbent_direction="$(kv selected.validation.directional_accuracy "${incumbent_report}")" \
    -v candidate_rank="$(kv selected.validation.pairwise_rank_accuracy "${candidate_report}")" \
    -v incumbent_rank="$(kv selected.validation.pairwise_rank_accuracy "${incumbent_report}")" \
    -v candidate_correlation="$(kv selected.validation.correlation "${candidate_report}")" \
    -v incumbent_correlation="$(kv selected.validation.correlation "${incumbent_report}")" \
    -v candidate_rmse="$(kv selected.validation.rmse "${candidate_report}")" \
    -v incumbent_rmse="$(kv selected.validation.rmse "${incumbent_report}")" '
    function compare_high(left, right) {
      if (left > right + tolerance) return 1;
      if (right > left + tolerance) return -1;
      return 0;
    }
    function compare_low(left, right) {
      if (left + tolerance < right) return 1;
      if (right + tolerance < left) return -1;
      return 0;
    }
    BEGIN {
      result = compare_high(candidate_direction + 0, incumbent_direction + 0);
      if (result == 0) result = compare_high(candidate_rank + 0, incumbent_rank + 0);
      if (result == 0) result = compare_high(candidate_correlation + 0, incumbent_correlation + 0);
      if (result == 0) result = compare_low(candidate_rmse + 0, incumbent_rmse + 0);
      exit(result == 1 ? 0 : 1);
    }
  '
}

write_comparator_fixture() {
  local path="$1"
  local direction="$2"
  local rank="$3"
  local correlation="$4"
  local rmse="$5"
  cat >"${path}" <<FIXTURE
selected.validation.directional_accuracy=${direction}
selected.validation.pairwise_rank_accuracy=${rank}
selected.validation.correlation=${correlation}
selected.validation.rmse=${rmse}
FIXTURE
}

verify_selection_comparator_static() {
  local candidate incumbent
  candidate="$(mktemp /tmp/${schema_id}.comparator_candidate.XXXXXX)"
  incumbent="$(mktemp /tmp/${schema_id}.comparator_incumbent.XXXXXX)"
  write_comparator_fixture "${candidate}" 0.96 0.10 0.10 9.0
  write_comparator_fixture "${incumbent}" 0.95 0.99 0.99 0.1
  report_is_better "${candidate}" "${incumbent}" ||
    fail "cross-arm comparator does not prioritize direction"
  write_comparator_fixture "${candidate}" 0.9500000000005 0.96 0.10 9.0
  write_comparator_fixture "${incumbent}" 0.95 0.95 0.99 0.1
  report_is_better "${candidate}" "${incumbent}" ||
    fail "cross-arm comparator does not fall through tolerance to rank"
  write_comparator_fixture "${candidate}" 0.95 0.95 0.95 0.20
  write_comparator_fixture "${incumbent}" 0.95 0.95 0.95 0.21
  report_is_better "${candidate}" "${incumbent}" ||
    fail "cross-arm comparator does not minimize RMSE"
  write_comparator_fixture "${candidate}" 0.95 0.95 0.95 0.20
  write_comparator_fixture "${incumbent}" 0.95 0.95 0.95 0.20
  if report_is_better "${candidate}" "${incumbent}"; then
    fail "cross-arm comparator replaced the incumbent on an exact tie"
  fi
  rm -f -- "${candidate}" "${incumbent}"
}

compute_selected_arm() {
  local best=canonical arm
  for arm in "${all_arms[@]}"; do
    validate_development_report "$(arm_main_report "${arm}")"
  done
  for arm in "${challenger_arms[@]}"; do
    if report_is_better "$(arm_main_report "${arm}")" \
      "$(arm_main_report "${best}")"; then
      best="${arm}"
    fi
  done
  printf '%s' "${best}"
}

emit_selection() {
  local destination="$1"
  local selected arm report checkpoint train_probe validation_probe config
  selected="$(compute_selected_arm)"
  {
    echo "schema_id=${schema_id}.selection.v1"
    echo "status=complete"
    echo "runner_path=${frozen_runner}"
    echo "runner_sha256=$(sha256_of "${frozen_runner}")"
    echo "input_receipt_path=${input_receipt}"
    echo "input_receipt_sha256=$(sha256_of "${input_receipt}")"
    echo "retry_attempt_sentinel_path=${retry_attempt_sentinel}"
    echo "retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")"
    echo "route_trigger_path=${route_trigger}"
    echo "route_trigger_sha256=${expected_route_trigger_sha256}"
    echo "scientific_affine_runner_path=${scientific_affine_runner}"
    echo "scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}"
    echo "operational_affine_runner_path=${operational_affine_runner}"
    echo "operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}"
    echo "cuda_canonical_path_correction_path=${cuda_correction}"
    echo "cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}"
    echo "isolated_source_closure_path=${isolated_source_closure}"
    echo "isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")"
    echo "preregistration_path=${preregistration}"
    echo "preregistration_sha256=$(sha256_of "${preregistration}")"
    echo "conditional_amendment_path=${conditional_amendment}"
    echo "conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")"
    echo "source_isolation_amendment_path=${source_isolation_amendment}"
    echo "source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")"
    echo "isolated_source_protocol_path=${isolated_source_protocol}"
    echo "isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")"
    echo "staged_hardening_amendment_path=${staged_hardening}"
    echo "staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")"
    echo "cursor_alignment_correction_path=${cursor_alignment_correction}"
    echo "cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")"
    emit_cursor_alignment_erratum_binding
    echo "affine_helper_path=${frozen_helper}"
    echo "affine_helper_sha256=$(sha256_of "${frozen_helper}")"
    echo "affine_binary_path=${frozen_binary}"
    echo "affine_binary_sha256=$(sha256_of "${frozen_binary}")"
    echo "selection_order=validation_direction,validation_rank,validation_correlation,validation_rmse"
    echo "selection_tie_tolerance=${tie_tolerance}"
    echo "selection_tie_preference=canonical,endpoint_scale,time_only,no_tf_alignment"
    echo "arm_count=4"
    for arm in "${all_arms[@]}"; do
      report="$(arm_main_report "${arm}")"
      checkpoint="$(arm_checkpoint_path "${arm}")"
      train_probe="$(arm_train_probe_path "${arm}")"
      validation_probe="$(arm_validation_probe_path "${arm}")"
      config="$(arm_capture_config_path "${arm}")"
      echo "arm.${arm}.checkpoint_path=${checkpoint}"
      echo "arm.${arm}.checkpoint_sha256=$(sha256_of "${checkpoint}")"
      echo "arm.${arm}.capture_config_path=${config}"
      echo "arm.${arm}.capture_config_sha256=$(sha256_of "${config}")"
      echo "arm.${arm}.train_probe_path=${train_probe}"
      echo "arm.${arm}.train_probe_sha256=$(sha256_of "${train_probe}")"
      echo "arm.${arm}.validation_probe_path=${validation_probe}"
      echo "arm.${arm}.validation_probe_sha256=$(sha256_of "${validation_probe}")"
      echo "arm.${arm}.main_report_path=${report}"
      echo "arm.${arm}.main_report_sha256=$(sha256_of "${report}")"
      echo "arm.${arm}.replay_report_path=$(arm_replay_report "${arm}")"
      echo "arm.${arm}.replay_report_sha256=$(sha256_of "$(arm_replay_report "${arm}")")"
      echo "arm.${arm}.selected_candidate_index=$(kv selected_candidate_index "${report}")"
      echo "arm.${arm}.selected_ridge=$(kv selected_ridge "${report}")"
      echo "arm.${arm}.validation_directional_accuracy=$(kv selected.validation.directional_accuracy "${report}")"
      echo "arm.${arm}.validation_pairwise_rank_accuracy=$(kv selected.validation.pairwise_rank_accuracy "${report}")"
      echo "arm.${arm}.validation_correlation=$(kv selected.validation.correlation "${report}")"
      echo "arm.${arm}.validation_rmse=$(kv selected.validation.rmse "${report}")"
      echo "arm.${arm}.validation_rmse_target_rms_ratio=$(kv selected.validation.rmse_target_rms_ratio "${report}")"
      echo "arm.${arm}.validation_strong_gate_pass=$(kv validation_strong_gate_pass "${report}")"
    done
    echo "selected_arm=${selected}"
    echo "selected_checkpoint_path=$(arm_checkpoint_path "${selected}")"
    echo "selected_checkpoint_sha256=$(sha256_of "$(arm_checkpoint_path "${selected}")")"
    echo "selected_train_probe_path=$(arm_train_probe_path "${selected}")"
    echo "selected_train_probe_sha256=$(sha256_of "$(arm_train_probe_path "${selected}")")"
    echo "selected_validation_probe_path=$(arm_validation_probe_path "${selected}")"
    echo "selected_validation_probe_sha256=$(sha256_of "$(arm_validation_probe_path "${selected}")")"
    echo "selected_development_report_path=$(arm_main_report "${selected}")"
    echo "selected_development_report_sha256=$(sha256_of "$(arm_main_report "${selected}")")"
    echo "selection_locked_before_certified=true"
    echo "maximum_anchor_read=2815"
    echo "accepted_anchor_count=3261"
    echo "candidate_anchor_count=3261"
    echo "maximum_available_anchor_index=3260"
    echo "certified_input_access=false"
    echo "canonical_data_raw_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_selection() {
  require_immutable_file "${selection_receipt}"
  expect_kv "${selection_receipt}" retry_attempt_sentinel_path \
    "${retry_attempt_sentinel}"
  expect_kv "${selection_receipt}" retry_attempt_sentinel_sha256 \
    "$(sha256_of "${retry_attempt_sentinel}")"
  expect_kv "${selection_receipt}" schema_id "${schema_id}.selection.v1"
  expect_kv "${selection_receipt}" status complete
  expect_kv "${selection_receipt}" selection_locked_before_certified true
  expect_kv "${selection_receipt}" maximum_anchor_read 2815
  expect_kv "${selection_receipt}" certified_input_access false
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.selection_verify.XXXXXX)"
  emit_selection "${candidate}"
  cmp -s -- "${candidate}" "${selection_receipt}" || {
    rm -f -- "${candidate}"
    fail "immutable cross-arm selection drifted"
  }
  rm -f -- "${candidate}"
}

write_selection() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/.selection.XXXXXX")"
  emit_selection "${candidate}"
  publish_immutable "${candidate}" "${selection_receipt}"
  verify_selection
  assert_operational_runner_identity
}

emit_development_receipt() {
  local destination="$1"
  local arm
  {
    echo "schema_id=${schema_id}.development.v1"
    echo "status=complete"
    emit_ablation_runner_bindings
    echo "runner_path=${script_path}"
    echo "runner_sha256=${process_start_runner_sha256}"
    echo "frozen_runner_path=${frozen_runner}"
    echo "frozen_runner_sha256=$(sha256_of "${frozen_runner}")"
    echo "input_receipt_path=${input_receipt}"
    echo "input_receipt_sha256=$(sha256_of "${input_receipt}")"
    echo "retry_attempt_sentinel_path=${retry_attempt_sentinel}"
    echo "retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")"
    echo "config_closure_path=${config_closure}"
    echo "config_closure_sha256=$(sha256_of "${config_closure}")"
    echo "effective_grammar_closure_path=${effective_grammar_closure}"
    echo "effective_grammar_closure_sha256=$(sha256_of "${effective_grammar_closure}")"
    echo "isolated_source_closure_path=${isolated_source_closure}"
    echo "isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")"
    echo "route_trigger_path=${route_trigger}"
    echo "route_trigger_sha256=${expected_route_trigger_sha256}"
    echo "scientific_affine_runner_path=${scientific_affine_runner}"
    echo "scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}"
    echo "operational_affine_runner_path=${operational_affine_runner}"
    echo "operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}"
    echo "cuda_canonical_path_correction_path=${cuda_correction}"
    echo "cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}"
    echo "required_route=representation_ablation_screen"
    echo "canonical_import_path=${canonical_import_receipt}"
    echo "canonical_import_sha256=$(sha256_of "${canonical_import_receipt}")"
    echo "selection_path=${selection_receipt}"
    echo "selection_sha256=$(sha256_of "${selection_receipt}")"
    echo "selected_arm=$(kv selected_arm "${selection_receipt}")"
    echo "affine_runner_source_path=${affine_runner_source}"
    echo "affine_runner_source_sha256=$(sha256_of "${affine_runner_source}")"
    echo "affine_helper_path=${frozen_helper}"
    echo "affine_helper_sha256=$(sha256_of "${frozen_helper}")"
    echo "affine_binary_path=${frozen_binary}"
    echo "affine_binary_sha256=$(sha256_of "${frozen_binary}")"
    echo "runtime_exec_path=${runtime_exec}"
    echo "runtime_exec_sha256=$(sha256_of "${runtime_exec}")"
    echo "preregistration_path=${preregistration}"
    echo "preregistration_sha256=$(sha256_of "${preregistration}")"
    echo "conditional_amendment_path=${conditional_amendment}"
    echo "conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")"
    echo "source_isolation_amendment_path=${source_isolation_amendment}"
    echo "source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")"
    echo "isolated_source_protocol_path=${isolated_source_protocol}"
    echo "isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")"
    echo "staged_hardening_amendment_path=${staged_hardening}"
    echo "staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")"
    echo "cursor_alignment_correction_path=${cursor_alignment_correction}"
    echo "cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")"
    emit_cursor_alignment_erratum_binding
    for arm in "${challenger_arms[@]}"; do
      echo "arm.${arm}.training_status_path=$(arm_training_status "${arm}")"
      echo "arm.${arm}.training_status_sha256=$(sha256_of "$(arm_training_status "${arm}")")"
      echo "arm.${arm}.capture_status_path=$(arm_capture_status "${arm}")"
      echo "arm.${arm}.capture_status_sha256=$(sha256_of "$(arm_capture_status "${arm}")")"
      echo "arm.${arm}.affine_status_path=$(arm_affine_status "${arm}")"
      echo "arm.${arm}.affine_status_sha256=$(sha256_of "$(arm_affine_status "${arm}")")"
    done
    echo "challenger_count=3"
    echo "challenger_seed=17"
    echo "challenger_optimizer_steps=${expected_steps}"
    echo "train_anchor_range=[${train_begin},${train_end})"
    echo "validation_anchor_range=[${validation_begin},${validation_end})"
    echo "maximum_anchor_read=2815"
    echo "accepted_anchor_count=3261"
    echo "candidate_anchor_count=3261"
    echo "maximum_available_anchor_index=3260"
    echo "cross_arm_selection_complete=true"
    echo "selection_locked_before_certified=true"
    echo "certified_attempt_created=false"
    echo "certified_input_access=false"
    echo "canonical_data_raw_access=false"
    echo "final_holdout_access=false"
    echo "independent_final_evidence=false"
    echo "policy_access=false"
  } >"${destination}"
}

write_development_receipt() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/.development.XXXXXX")"
  emit_development_receipt "${candidate}"
  publish_immutable "${candidate}" "${development_receipt}"
  assert_operational_runner_identity
}

verify_development_receipt() {
  require_immutable_file "${development_receipt}"
  expect_kv "${development_receipt}" retry_attempt_sentinel_path \
    "${retry_attempt_sentinel}"
  expect_kv "${development_receipt}" retry_attempt_sentinel_sha256 \
    "$(sha256_of "${retry_attempt_sentinel}")"
  verify_ablation_runner_bindings "${development_receipt}"
  expect_kv "${development_receipt}" route_trigger_sha256 \
    "${expected_route_trigger_sha256}"
  expect_kv "${development_receipt}" operational_affine_runner_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${development_receipt}" \
    cuda_canonical_path_correction_sha256 "${expected_cuda_correction_sha256}"
  expect_kv "${development_receipt}" schema_id "${schema_id}.development.v1"
  expect_kv "${development_receipt}" status complete
  expect_kv "${development_receipt}" required_route \
    representation_ablation_screen
  expect_kv "${development_receipt}" selection_locked_before_certified true
  expect_kv "${development_receipt}" certified_input_access false
  expect_kv "${development_receipt}" canonical_data_raw_access false
  expect_kv "${development_receipt}" final_holdout_access false
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.development_verify.XXXXXX)"
  emit_development_receipt "${candidate}"
  cmp -s -- "${candidate}" "${development_receipt}" || {
    rm -f -- "${candidate}"
    fail "immutable development receipt drifted"
  }
  rm -f -- "${candidate}"
}

assert_no_local_certified_artifacts() {
  local path
  for path in "${certified_attempt}" "${runtime_root}/certified" \
    "${certified_job}" \
    "${certified_capture_status}" "${certified_capture_log}" \
    "${certified_main_report}" \
    "${certified_replay_report}" "${result_receipt}" \
    "${certified_main_log}" "${certified_replay_log}"; do
    [[ ! -e "${path}" ]] ||
      fail "development-only phase found certified artifact: ${path}"
  done
}

audit_development_job_set() {
  local manifests count=0 manifest end
  manifests="$(find "${arms_root}" -type f -name job.manifest -print 2>/dev/null || true)"
  while IFS= read -r manifest; do
    [[ -n "${manifest}" ]] || continue
    count=$((count + 1))
    end="$(kv resolved_anchor_index_end "${manifest}")"
    [[ "${end}" =~ ^[0-9]+$ && "${end}" -le 2816 ]] ||
      fail "development job escaped the development ranges: ${manifest}"
    reject_data_raw_file "${manifest}"
  done <<<"${manifests}"
  [[ "${count}" == 9 ]] ||
    fail "expected exactly nine challenger development jobs, found ${count}"
}

verify_development_core() {
  preflight_read_only
  verify_base_training_derivation
  verify_selection_comparator_static
  verify_base_training_config
  canonical_inputs
  verify_frozen_sources
  local arm
  for arm in "${challenger_arms[@]}"; do
    verify_arm_files "${arm}"
  done
  verify_effective_grammar_closure
  verify_config_closure
  verify_inputs
  verify_retry_attempt_sentinel
  verify_canonical_import
  for arm in "${challenger_arms[@]}"; do
    verify_training_status "${arm}" >/dev/null
    verify_capture_status "${arm}"
    verify_affine_status "${arm}"
  done
  verify_selection
  verify_development_receipt
  audit_development_job_set
  assert_operational_runner_identity
}

run_development() {
  preflight_read_only
  assert_operational_runner_identity
  verify_base_training_derivation
  verify_selection_comparator_static
  assert_operational_runner_identity
  mkdir -p "${runtime_root}"
  assert_operational_runner_identity
  exec 9>"${runtime_root}/.development.lock"
  flock -n 9 || fail "another ablation development process holds the lock"
  assert_operational_runner_identity
  [[ ! -e "${retry_attempt_sentinel}" ]] ||
    fail "retry1 development attempt is already consumed and cannot be resumed"
  write_base_training_config
  assert_no_local_certified_artifacts
  canonical_inputs
  freeze_sources
  generate_all_arm_files
  write_effective_grammar_closure
  verify_effective_grammar_closure
  write_config_closure
  write_inputs
  verify_config_closure
  verify_inputs
  publish_retry_attempt_sentinel_once
  import_canonical_arm
  local arm
  for arm in "${challenger_arms[@]}"; do
    run_training_arm "${arm}"
  done
  for arm in "${challenger_arms[@]}"; do
    run_capture_arm "${arm}"
  done
  for arm in "${challenger_arms[@]}"; do
    run_affine_arm "${arm}"
  done
  write_selection
  assert_no_local_certified_artifacts
  write_development_receipt
  assert_no_local_certified_artifacts
  verify_development_core
  assert_operational_runner_identity
}

emit_certified_attempt() {
  local destination="$1"
  local selected checkpoint config train_probe validation_probe lock_report
  selected="$(kv selected_arm "${selection_receipt}")"
  checkpoint="$(arm_checkpoint_path "${selected}")"
  config="$(arm_capture_config_path "${selected}")"
  train_probe="$(arm_train_probe_path "${selected}")"
  validation_probe="$(arm_validation_probe_path "${selected}")"
  lock_report="$(arm_main_report "${selected}")"
  cat >"${destination}" <<ATTEMPT
schema_id=${schema_id}.certified_attempt.v1
status=consumed
immutable_mode=0444
attempt_ordinal=1
route=representation_ablation_screen
selected_arm=${selected}
certified_job_path=${certified_job}
certified_anchor_range=[${certified_begin},${certified_end})
certified_probe_rows=${certified_rows}
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
$(emit_ablation_runner_bindings)
live_runner_path=${script_path}
live_runner_sha256=${process_start_runner_sha256}
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
development_receipt_path=${development_receipt}
development_receipt_sha256=$(sha256_of "${development_receipt}")
selection_receipt_path=${selection_receipt}
selection_receipt_sha256=$(sha256_of "${selection_receipt}")
route_trigger_path=${route_trigger}
route_trigger_sha256=${expected_route_trigger_sha256}
canonical_capture_development_path=${canonical_capture_development}
canonical_capture_development_sha256=${expected_capture_development_sha256}
scientific_affine_runner_path=${scientific_affine_runner}
scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}
operational_affine_runner_path=${operational_affine_runner}
operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}
cuda_canonical_path_correction_path=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
isolated_source_root_path=${isolated_source_root}
config_path=${config}
config_sha256=$(sha256_of "${config}")
representation_checkpoint_path=${checkpoint}
representation_checkpoint_sha256=$(sha256_of "${checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
selection_lock_path=${lock_report}
selection_lock_sha256=$(sha256_of "${lock_report}")
selection_lock_schema_id=synthetic_v2_frozen_encoder_affine_development_v1
selection_lock_selected_candidate_index=$(kv selected_candidate_index "${lock_report}")
selection_lock_selected_ridge=$(kv selected_ridge "${lock_report}")
affine_runner_source_path=${affine_runner_source}
affine_runner_source_sha256=$(sha256_of "${affine_runner_source}")
affine_helper_source_path=${frozen_helper}
affine_helper_source_sha256=$(sha256_of "${frozen_helper}")
affine_binary_path=${frozen_binary}
affine_binary_sha256=$(sha256_of "${frozen_binary}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
fresh_preregistration_path=${fresh_preregistration}
fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")
diagnostic_preregistration_path=${diagnostic_preregistration}
diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")
diagnostic_amendment_path=${diagnostic_amendment}
diagnostic_amendment_sha256=$(sha256_of "${diagnostic_amendment}")
localization_addendum_path=${localization_addendum}
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
ablation_preregistration_path=${preregistration}
ablation_preregistration_sha256=$(sha256_of "${preregistration}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
selection_locked_before_certified=true
published_before_certified_input=true
certified_job_complete_at_publication=false
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
runner_up_retry_authorized=false
canonical_data_raw_access=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
ATTEMPT
}

verify_certified_attempt() {
  require_immutable_file "${certified_attempt}"
  verify_ablation_runner_bindings "${certified_attempt}"
  expect_kv "${certified_attempt}" route_trigger_sha256 \
    "${expected_route_trigger_sha256}"
  expect_kv "${certified_attempt}" operational_affine_runner_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${certified_attempt}" cuda_canonical_path_correction_sha256 \
    "${expected_cuda_correction_sha256}"
  [[ "$(stat -c '%a' -- "${certified_attempt}")" == 444 ]] ||
    fail "certified attempt immutable mode is not 0444"
  expect_kv "${certified_attempt}" schema_id \
    "${schema_id}.certified_attempt.v1"
  expect_kv "${certified_attempt}" status consumed
  expect_kv "${certified_attempt}" immutable_mode 0444
  expect_kv "${certified_attempt}" attempt_ordinal 1
  expect_kv "${certified_attempt}" certified_job_path "${certified_job}"
  expect_kv "${certified_attempt}" route representation_ablation_screen
  expect_kv "${certified_attempt}" published_before_certified_input true
  expect_kv "${certified_attempt}" runner_up_retry_authorized false
  expect_kv "${certified_attempt}" canonical_data_raw_access false
  expect_kv "${certified_attempt}" final_holdout_access false
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.attempt_verify.XXXXXX)"
  emit_certified_attempt "${candidate}"
  cmp -s -- "${candidate}" "${certified_attempt}" || {
    rm -f -- "${candidate}"
    fail "certified attempt receipt is incomplete or drifted"
  }
  rm -f -- "${candidate}"
}

publish_certified_attempt_once() {
  assert_operational_runner_identity
  [[ ! -e "${certified_attempt}" ]] ||
    fail "certified attempt already exists and is consumed"
  local candidate
  candidate="$(mktemp "${runtime_root}/.certified_attempt.XXXXXX")"
  emit_certified_attempt "${candidate}"
  publish_immutable "${candidate}" "${certified_attempt}"
  verify_certified_attempt
  assert_operational_runner_identity
}

emit_certified_capture_status() {
  local destination="$1"
  local selected checkpoint config probe
  selected="$(kv selected_arm "${selection_receipt}")"
  checkpoint="$(arm_checkpoint_path "${selected}")"
  config="$(arm_capture_config_path "${selected}")"
  probe="${certified_job}/representation_edge_features.probe"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.certified_capture.v1
status=complete
attempt_ordinal=1
selected_arm=${selected}
certified_attempt_path=${certified_attempt}
certified_attempt_sha256=$(sha256_of "${certified_attempt}")
selection_receipt_path=${selection_receipt}
selection_receipt_sha256=$(sha256_of "${selection_receipt}")
route_trigger_path=${route_trigger}
route_trigger_sha256=${expected_route_trigger_sha256}
scientific_affine_runner_path=${scientific_affine_runner}
scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}
operational_affine_runner_path=${operational_affine_runner}
operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}
cuda_canonical_path_correction_path=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
config_path=${config}
config_sha256=$(sha256_of "${config}")
representation_checkpoint_path=${checkpoint}
representation_checkpoint_sha256=$(sha256_of "${checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
capture_log_path=${certified_capture_log}
capture_log_sha256=$(sha256_of "${certified_capture_log}")
job_path=${certified_job}
job_manifest_path=${certified_job}/job.manifest
job_manifest_sha256=$(sha256_of "${certified_job}/job.manifest")
runtime_result_path=${certified_job}/runtime.result.fact
runtime_result_sha256=$(sha256_of "${certified_job}/runtime.result.fact")
inference_report_path=${certified_job}/channel_inference.report
inference_report_sha256=$(sha256_of "${certified_job}/channel_inference.report")
certified_probe_path=${probe}
certified_probe_sha256=$(sha256_of "${probe}")
certified_mdn_probe_path=${certified_job}/mdn_edge_context_features.probe
certified_mdn_probe_sha256=$(sha256_of "${certified_job}/mdn_edge_context_features.probe")
certified_anchor_range=[${certified_begin},${certified_end})
certified_probe_rows=${certified_rows}
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
maximum_anchor_read=3260
selected_arm_attempt_count=1
runner_up_retry_authorized=false
canonical_data_raw_access=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
STATUS
}

verify_certified_capture_status() {
  require_immutable_file "${certified_capture_status}"
  require_file "${certified_capture_log}"
  [[ "$(stat -c '%A' -- "${certified_capture_log}")" != *w* ]] ||
    fail "certified capture log is writable"
  verify_certified_attempt
  local selected checkpoint config probe candidate
  selected="$(kv selected_arm "${selection_receipt}")"
  checkpoint="$(arm_checkpoint_path "${selected}")"
  config="$(arm_capture_config_path "${selected}")"
  probe="$(validate_capture_job "${certified_job}" "${certified_begin}" \
    "${certified_end}" "${certified_rows}" "${checkpoint}" "${config}")"
  verify_capture_job_immutable "${certified_job}"
  [[ "${probe}" == "${certified_job}/representation_edge_features.probe" ]] ||
    fail "certified probe path drifted"
  candidate="$(mktemp /tmp/${schema_id}.certified_capture_verify.XXXXXX)"
  emit_certified_capture_status "${candidate}"
  cmp -s -- "${candidate}" "${certified_capture_status}" || {
    rm -f -- "${candidate}"
    fail "certified capture status drifted"
  }
  rm -f -- "${candidate}"
}

write_certified_capture_status() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/certified/.capture_status.XXXXXX")"
  emit_certified_capture_status "${candidate}"
  publish_immutable "${candidate}" "${certified_capture_status}"
  assert_operational_runner_identity
}

selection_lines() {
  awk '
    /^candidate\.[0-9]+\.(ridge|numerically_valid|rejection_reason|maximum_normalized_residual|coefficient_l2_norm)=/ { print; next }
    /^candidate\.[0-9]+\.validation\./ { print; next }
    /^selected_candidate_index=/ { print; next }
    /^selected_ridge=/ { print; next }
    /^selected_maximum_normalized_residual=/ { print; next }
    /^selected_coefficient_l2_norm=/ { print; next }
    /^selected\.train\./ { print; next }
    /^selected\.validation\./ { print; next }
    /^validation_(strong|partial)_gate_pass=/ { print; next }
  ' "$1"
}

validate_certified_report() {
  local report="$1"
  local lock_report="$2"
  require_immutable_file "${report}"
  validate_development_report "${lock_report}"
  expect_kv "${report}" schema_id \
    synthetic_v2_frozen_encoder_affine_probe_v1
  expect_kv "${report}" status complete
  expect_kv "${report}" benchmark_id synthetic_continuous_graph_v2
  expect_kv "${report}" probe_kind representation
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows "${certified_rows}"
  expect_kv "${report}" fit_anchor_range "[${train_begin},${train_end})"
  expect_kv "${report}" validation_anchor_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${report}" certified_anchor_range \
    "[${certified_begin},${certified_end})"
  expect_kv "${report}" maximum_anchor_read 3260
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" refit_after_selection false
  expect_kv "${report}" certified_candidates_scored 1
  expect_kv "${report}" selection_lock_provided true
  expect_kv "${report}" selection_lock_verified true
  expect_kv "${report}" selection_lock_path "${lock_report}"
  expect_kv "${report}" selection_lock_schema_id \
    synthetic_v2_frozen_encoder_affine_development_v1
  expect_kv "${report}" selection_lock_probe_kind representation
  expect_kv "${report}" selection_lock_selected_candidate_index \
    "$(kv selected_candidate_index "${lock_report}")"
  expect_kv "${report}" selection_lock_selected_ridge \
    "$(kv selected_ridge "${lock_report}")"
  cmp -s <(selection_lines "${lock_report}") \
    <(selection_lines "${report}") ||
    fail "runner-side certified selection comparison failed"
  local field
  for field in selected.certified.directional_accuracy \
    selected.certified.pairwise_rank_accuracy \
    selected.certified.correlation selected.certified.rmse \
    selected.certified.rmse_target_rms_ratio; do
    require_number "$(kv "${field}" "${report}")" "${field}"
  done
  local gate classification
  for gate in validation_strong_gate_pass certified_strong_gate_pass \
    validation_partial_gate_pass certified_partial_gate_pass; do
    [[ "$(kv "${gate}" "${report}")" == true || \
      "$(kv "${gate}" "${report}")" == false ]] ||
      fail "invalid certified gate field ${gate} in ${report}"
  done
  classification="$(kv classification "${report}")"
  case "${classification}" in
  strong_information_preservation | partial_information_preservation | representation_or_exposed_interface_failure) ;;
  *) fail "invalid certified representation classification: ${classification}" ;;
  esac
}

run_certified_reports() {
  assert_operational_runner_identity
  local selected train_probe validation_probe certified_probe lock_report
  selected="$(kv selected_arm "${selection_receipt}")"
  train_probe="$(arm_train_probe_path "${selected}")"
  validation_probe="$(arm_validation_probe_path "${selected}")"
  certified_probe="${certified_job}/representation_edge_features.probe"
  lock_report="$(arm_main_report "${selected}")"
  [[ ! -e "${certified_main_report}" && \
    ! -e "${certified_replay_report}" && ! -e "${certified_main_log}" && \
    ! -e "${certified_replay_log}" ]] ||
    fail "partial certified affine reports already exist; attempt is consumed"
  mkdir -p "$(dirname "${certified_main_report}")" \
    "$(dirname "${certified_replay_report}")"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --certified-input "${certified_probe}" \
    --selection-lock "${lock_report}" \
    --output "${certified_main_report}" >"${certified_main_log}" 2>&1 ||
    fail "certified affine main execution failed; attempt is permanently consumed"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --certified-input "${certified_probe}" \
    --selection-lock "${lock_report}" \
    --output "${certified_replay_report}" >"${certified_replay_log}" 2>&1 ||
    fail "certified affine replay execution failed; attempt is permanently consumed"
  chmod 0444 "${certified_main_report}" "${certified_replay_report}" \
    "${certified_main_log}" "${certified_replay_log}"
  validate_certified_report "${certified_main_report}" "${lock_report}"
  validate_certified_report "${certified_replay_report}" "${lock_report}"
  cmp -s -- "${certified_main_report}" "${certified_replay_report}" ||
    fail "certified affine main/replay reports differ"
  cmp -s -- "${certified_main_log}" "${certified_replay_log}" ||
    fail "certified affine main/replay logs differ"
  assert_operational_runner_identity
}

emit_result_receipt() {
  local destination="$1"
  local selected checkpoint config train_probe validation_probe lock_report
  local certified_probe validation_strong certified_strong success=false
  selected="$(kv selected_arm "${selection_receipt}")"
  checkpoint="$(arm_checkpoint_path "${selected}")"
  config="$(arm_capture_config_path "${selected}")"
  train_probe="$(arm_train_probe_path "${selected}")"
  validation_probe="$(arm_validation_probe_path "${selected}")"
  lock_report="$(arm_main_report "${selected}")"
  certified_probe="${certified_job}/representation_edge_features.probe"
  validation_strong="$(kv validation_strong_gate_pass "${certified_main_report}")"
  certified_strong="$(kv certified_strong_gate_pass "${certified_main_report}")"
  if [[ "${validation_strong}" == true && "${certified_strong}" == true ]]; then
    success=true
  fi
  cat >"${destination}" <<RESULT
schema_id=${schema_id}.result.v1
status=complete
route=representation_ablation_screen
selected_arm=${selected}
runner_path=${frozen_runner}
runner_sha256=$(sha256_of "${frozen_runner}")
$(emit_ablation_runner_bindings)
live_runner_path=${script_path}
live_runner_sha256=${process_start_runner_sha256}
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
config_closure_path=${config_closure}
config_closure_sha256=$(sha256_of "${config_closure}")
development_receipt_path=${development_receipt}
development_receipt_sha256=$(sha256_of "${development_receipt}")
selection_receipt_path=${selection_receipt}
selection_receipt_sha256=$(sha256_of "${selection_receipt}")
certified_attempt_path=${certified_attempt}
certified_attempt_sha256=$(sha256_of "${certified_attempt}")
certified_capture_status_path=${certified_capture_status}
certified_capture_status_sha256=$(sha256_of "${certified_capture_status}")
route_trigger_path=${route_trigger}
route_trigger_sha256=${expected_route_trigger_sha256}
canonical_capture_development_path=${canonical_capture_development}
canonical_capture_development_sha256=${expected_capture_development_sha256}
scientific_affine_runner_path=${scientific_affine_runner}
scientific_affine_runner_sha256=${expected_scientific_affine_runner_sha256}
operational_affine_runner_path=${operational_affine_runner}
operational_affine_runner_sha256=${expected_operational_affine_runner_sha256}
cuda_canonical_path_correction_path=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
isolated_source_closure_path=${isolated_source_closure}
isolated_source_closure_sha256=$(sha256_of "${isolated_source_closure}")
config_path=${config}
config_sha256=$(sha256_of "${config}")
representation_checkpoint_path=${checkpoint}
representation_checkpoint_sha256=$(sha256_of "${checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
selection_lock_path=${lock_report}
selection_lock_sha256=$(sha256_of "${lock_report}")
certified_probe_path=${certified_probe}
certified_probe_sha256=$(sha256_of "${certified_probe}")
certified_mdn_probe_path=${certified_job}/mdn_edge_context_features.probe
certified_mdn_probe_sha256=$(sha256_of "${certified_job}/mdn_edge_context_features.probe")
certified_job_manifest_path=${certified_job}/job.manifest
certified_job_manifest_sha256=$(sha256_of "${certified_job}/job.manifest")
certified_runtime_result_path=${certified_job}/runtime.result.fact
certified_runtime_result_sha256=$(sha256_of "${certified_job}/runtime.result.fact")
certified_inference_report_path=${certified_job}/channel_inference.report
certified_inference_report_sha256=$(sha256_of "${certified_job}/channel_inference.report")
affine_runner_source_path=${affine_runner_source}
affine_runner_source_sha256=$(sha256_of "${affine_runner_source}")
affine_helper_source_path=${frozen_helper}
affine_helper_source_sha256=$(sha256_of "${frozen_helper}")
affine_binary_path=${frozen_binary}
affine_binary_sha256=$(sha256_of "${frozen_binary}")
certified_main_report_path=${certified_main_report}
certified_main_report_sha256=$(sha256_of "${certified_main_report}")
certified_replay_report_path=${certified_replay_report}
certified_replay_report_sha256=$(sha256_of "${certified_replay_report}")
certified_main_log_path=${certified_main_log}
certified_main_log_sha256=$(sha256_of "${certified_main_log}")
certified_replay_log_path=${certified_replay_log}
certified_replay_log_sha256=$(sha256_of "${certified_replay_log}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
preregistration_path=${preregistration}
preregistration_sha256=$(sha256_of "${preregistration}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
$(emit_cursor_alignment_erratum_binding)
validation_directional_accuracy=$(kv selected.validation.directional_accuracy "${certified_main_report}")
validation_pairwise_rank_accuracy=$(kv selected.validation.pairwise_rank_accuracy "${certified_main_report}")
validation_correlation=$(kv selected.validation.correlation "${certified_main_report}")
validation_rmse=$(kv selected.validation.rmse "${certified_main_report}")
validation_rmse_target_rms_ratio=$(kv selected.validation.rmse_target_rms_ratio "${certified_main_report}")
certified_directional_accuracy=$(kv selected.certified.directional_accuracy "${certified_main_report}")
certified_pairwise_rank_accuracy=$(kv selected.certified.pairwise_rank_accuracy "${certified_main_report}")
certified_correlation=$(kv selected.certified.correlation "${certified_main_report}")
certified_rmse=$(kv selected.certified.rmse "${certified_main_report}")
certified_rmse_target_rms_ratio=$(kv selected.certified.rmse_target_rms_ratio "${certified_main_report}")
validation_strong_gate_pass=${validation_strong}
certified_strong_gate_pass=${certified_strong}
validation_partial_gate_pass=$(kv validation_partial_gate_pass "${certified_main_report}")
certified_partial_gate_pass=$(kv certified_partial_gate_pass "${certified_main_report}")
classification=$(kv classification "${certified_main_report}")
representation_success=${success}
selection_lock_provided=true
selection_lock_verified=true
runner_side_selection_comparison=true
selected_arm_certified_attempt_count=1
total_certified_capture_job_count=1
runner_up_certified_retry=false
certified_anchor_range=[${certified_begin},${certified_end})
maximum_anchor_read=3260
accepted_anchor_count=3261
candidate_anchor_count=3261
maximum_available_anchor_index=3260
canonical_data_raw_access=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
RESULT
}

assert_only_selected_certified() {
  local arm selected
  selected="$(kv selected_arm "${selection_receipt}")"
  for arm in "${all_arms[@]}"; do
    [[ ! -e "$(arm_root "${arm}")/certified" ]] ||
      fail "arm-local certified artifact is forbidden for ${arm}"
  done
  expect_kv "${certified_attempt}" selected_arm "${selected}"
  expect_kv "${certified_capture_status}" selected_arm "${selected}"
}

audit_complete_job_set() {
  local manifests count=0 manifest end
  manifests="$(find "${runtime_root}" -type f -name job.manifest -print 2>/dev/null || true)"
  while IFS= read -r manifest; do
    [[ -n "${manifest}" ]] || continue
    count=$((count + 1))
    end="$(kv resolved_anchor_index_end "${manifest}")"
    [[ "${end}" =~ ^[0-9]+$ && "${end}" -le 3261 ]] ||
      fail "job manifest crossed the isolated development prefix: ${manifest}"
    reject_data_raw_file "${manifest}"
  done <<<"${manifests}"
  [[ "${count}" == 10 ]] ||
    fail "expected nine development jobs and one certified job, found ${count}"
}

verify_result_receipt() {
  require_immutable_file "${result_receipt}"
  verify_ablation_runner_bindings "${result_receipt}"
  expect_kv "${result_receipt}" route_trigger_sha256 \
    "${expected_route_trigger_sha256}"
  expect_kv "${result_receipt}" operational_affine_runner_sha256 \
    "${expected_operational_affine_runner_sha256}"
  expect_kv "${result_receipt}" cuda_canonical_path_correction_sha256 \
    "${expected_cuda_correction_sha256}"
  expect_kv "${result_receipt}" schema_id "${schema_id}.result.v1"
  expect_kv "${result_receipt}" status complete
  expect_kv "${result_receipt}" route representation_ablation_screen
  expect_kv "${result_receipt}" selection_lock_provided true
  expect_kv "${result_receipt}" selection_lock_verified true
  expect_kv "${result_receipt}" selected_arm_certified_attempt_count 1
  expect_kv "${result_receipt}" total_certified_capture_job_count 1
  expect_kv "${result_receipt}" runner_up_certified_retry false
  expect_kv "${result_receipt}" canonical_data_raw_access false
  expect_kv "${result_receipt}" final_holdout_access false
  local candidate
  candidate="$(mktemp /tmp/${schema_id}.result_verify.XXXXXX)"
  emit_result_receipt "${candidate}"
  cmp -s -- "${candidate}" "${result_receipt}" || {
    rm -f -- "${candidate}"
    fail "immutable final ablation receipt drifted"
  }
  rm -f -- "${candidate}"
}

write_result_receipt() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  emit_result_receipt "${candidate}"
  publish_immutable "${candidate}" "${result_receipt}"
  assert_operational_runner_identity
}

verify_complete() {
  verify_development_core
  verify_certified_attempt
  local selected lock_report
  selected="$(kv selected_arm "${selection_receipt}")"
  lock_report="$(arm_main_report "${selected}")"
  require_file "${certified_main_log}"
  require_file "${certified_replay_log}"
  [[ "$(stat -c '%A' -- "${certified_main_log}")" != *w* ]] ||
    fail "certified main log is writable"
  [[ "$(stat -c '%A' -- "${certified_replay_log}")" != *w* ]] ||
    fail "certified replay log is writable"
  validate_certified_report "${certified_main_report}" "${lock_report}"
  validate_certified_report "${certified_replay_report}" "${lock_report}"
  cmp -s -- "${certified_main_report}" "${certified_replay_report}" ||
    fail "certified main/replay reports differ"
  cmp -s -- "${certified_main_log}" "${certified_replay_log}" ||
    fail "certified main/replay logs differ"
  # Only after both reports prove that the helper enforced the pre-certified
  # lock may the verifier parse the captured certified probe.
  verify_certified_capture_status
  assert_only_selected_certified
  audit_complete_job_set
  [[ ! -e "${canonical_capture_result}" ]] ||
    fail "canonical certified result appeared after ablation routing"
  [[ ! -e "${canonical_affine_final}" ]] ||
    fail "canonical affine certified result appeared after ablation routing"
  verify_result_receipt
}

run_certified() {
  preflight_read_only
  assert_operational_runner_identity
  verify_development_core
  assert_operational_runner_identity
  exec 9>"${runtime_root}/.certified.lock"
  flock -n 9 || fail "another certified ablation process holds the lock"
  assert_operational_runner_identity
  verify_development_core
  if [[ -e "${result_receipt}" ]]; then
    verify_complete
    return
  fi
  if [[ -e "${certified_attempt}" || -e "${runtime_root}/certified" || \
    -e "${certified_job}" || \
    -e "${certified_capture_status}" || -e "${certified_capture_log}" || \
    -e "${certified_main_report}" || -e "${certified_replay_report}" || \
    -e "${certified_main_log}" || -e "${certified_replay_log}" ]]; then
    fail "incomplete certified attempt is consumed and cannot be retried"
  fi
  publish_certified_attempt_once
  assert_operational_runner_identity
  mkdir -p "${runtime_root}/certified"
  assert_operational_runner_identity
  local selected checkpoint config probe
  selected="$(kv selected_arm "${selection_receipt}")"
  checkpoint="$(arm_checkpoint_path "${selected}")"
  config="$(arm_capture_config_path "${selected}")"
  run_guarded_child "${runtime_exec}" \
    --config "${config}" \
    --job-dir "${certified_job}" \
    --source-range anchor_index \
    --anchor-index-begin "${certified_begin}" \
    --anchor-index-end "${certified_end}" \
    --input-representation-checkpoint "${checkpoint}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${certified_capture_log}" 2>&1 ||
    fail "certified capture failed; the sole attempt is permanently consumed"
  chmod 0444 "${certified_capture_log}"
  require_nonempty_file "${certified_job}/representation_edge_features.probe"
  seal_capture_job_files "${certified_job}"
  # The helper reconstructs and verifies the immutable development selection
  # lock before it opens the certified probe.  Do not parse that probe here.
  run_certified_reports
  probe="$(validate_capture_job "${certified_job}" "${certified_begin}" \
    "${certified_end}" "${certified_rows}" "${checkpoint}" "${config}")"
  [[ "${probe}" == "${certified_job}/representation_edge_features.probe" ]] ||
    fail "certified representation probe path drifted"
  write_certified_capture_status
  write_result_receipt
  verify_complete
  assert_operational_runner_identity
}

assert_operational_runner_identity
case "${mode}" in
--preflight)
  preflight_read_only
  ;;
--run-development)
  run_development
  ;;
--verify-development)
  verify_development_core
  ;;
--run-certified)
  run_certified
  ;;
--verify)
  verify_complete
  ;;
esac
assert_operational_runner_identity
