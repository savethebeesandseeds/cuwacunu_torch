#!/usr/bin/env bash
set -euo pipefail
shopt -s inherit_errexit
umask 077

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_retry3"
readonly retry2_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry2"
readonly retry1_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1"
readonly prior_failed_schema_id="synthetic_v2_representation_ablation_isolated_v2"
readonly failure_closure_schema_id="synthetic_v2_representation_ablation_isolated_v2_prejob_failure_closure_v1"
readonly retry1_interruption_closure_schema_id="${retry1_schema_id}_interruption_closure_v1"
readonly expected_retry1_interruption_closure_receipt_sha256="e6c845233f3f434a9c46bead1b9fb825217492a5da7ae0a95174fc10b15e1117"
readonly expected_retry1_interruption_regular_inventory_sha256="c7cce9005bee5efaa5f85924624839afad57655b4bfdb3d0d4a774fd4bf60926"
readonly expected_retry1_interruption_directory_inventory_sha256="40c7b2cf3846f3c439c6ebfe8d86b2839c969996fe6ce211be3577e2d11613ee"
readonly expected_retry1_interruption_amendment_sha256="e7d99698ee4b62254a274ebbf3d699b9e00ced0ff727279767e1d4976d594002"
readonly expected_retry1_interruption_sealer_sha256="95478dbd60734116171c9bd2bc40890c8abd1ea59b0d60c1dccc4dcbdb1241a3"
readonly expected_retry1_runtime_content_inventory_sha256="6a677ec3c7f5da7907cfc624ab280ad93b703a06da6c7febb1b8c8a80e97ef05"
readonly expected_retry1_runner_sha256="bebd812c48ba318ec632ed490841188daef9cdd68e02a53616ca9feba809ae43"
readonly expected_retry1_endpoint_training_status_sha256="b0fa364d31f32471cf0ff3d69b2836203e4dc47fd79f4c0507a7d7367b2f5ed5"
readonly expected_retry1_endpoint_runtime_result_sha256="7d3275bc2bdd8d647f1f06f41c1d11097acff56cc4f5e8cbb7d2497978ce182c"
readonly expected_retry1_endpoint_report_sha256="99e370677d4dd1932aa4fd3af66b954e2969a3afc55b1be2436b8875e8c64740"
readonly expected_retry1_endpoint_manifest_sha256="f1e514f84f05ae898b8616204e75cfe4034f2b23ff972c525f62635349905c7c"
readonly expected_retry1_endpoint_checkpoint_sha256="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
readonly expected_retry2_amendment_sha256="414211345e95965f52d8a0ceb672b5efff74b2c495d67619ca2b3ac788060591"
readonly retry2_bootstrap_failure_closure_schema_id="${retry2_schema_id}_bootstrap_publication_failure_closure_v1"
readonly expected_retry2_bootstrap_failure_receipt_sha256="1bf5f81f63cc9a53d35eec9c7f56264d2de9f3a9d0567c52ee1772196931fab6"
readonly expected_retry2_bootstrap_failure_regular_inventory_sha256="b2c215543b34c79169fc62f6030cddf43cd53a747653367e0e797dc281646f3a"
readonly expected_retry2_bootstrap_failure_directory_inventory_sha256="2ac4b2ac3bf3941adac1d9b0d251b4fb95a0ee18775ca85c92da05a9c680f3c9"
readonly expected_retry2_bootstrap_failure_old_runner_sha256="84ce29197961a232887290f045050fa06316652cde31651f6e930b302aec69ba"
readonly expected_retry2_bootstrap_failure_old_amendment_sha256="414211345e95965f52d8a0ceb672b5efff74b2c495d67619ca2b3ac788060591"
readonly expected_retry2_bootstrap_failure_erratum_sha256="b71b9a953175a3dc1f510e5b6bb8ffe72411a9f21c70e31f96a960be9dd9acb6"
readonly expected_retry2_bootstrap_failure_observation_sha256="720b782bc4e5027589e18cbd7428df4527e2b8ed2fc1de07977a30ef8716de64"
readonly expected_retry2_bootstrap_failure_sealer_sha256="438d5de7230e23253b2771e07a574122c9beb2dc0e85b0dd6b02dac4892c68a1"
readonly expected_retry2_bootstrap_failure_empty_sha256="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
readonly expected_retry2_bootstrap_failure_device="66"
readonly expected_retry2_bootstrap_failure_runtime_parent_inode="13792273860165226"
readonly expected_retry2_bootstrap_failure_closure_inode="63331869759942850"
readonly expected_retry2_bootstrap_failure_scratch_inode="287948901175250063"
readonly expected_retry2_bootstrap_failure_candidate_inode="7318349394570920"
readonly expected_retry2_bootstrap_failure_lock_inode="9851624184966826"
readonly expected_retry2_bootstrap_failure_old_runner_inode="168884986026428276"
readonly expected_retry2_bootstrap_failure_sealer_inode="70368744177717898"
readonly expected_retry2_bootstrap_failure_receipt_bytes="7069"
readonly expected_retry2_bootstrap_failure_regular_inventory_bytes="255"
readonly expected_retry2_bootstrap_failure_directory_inventory_bytes="211"
readonly expected_retry2_bootstrap_failure_sealer_bytes="36718"
readonly expected_retry2_bootstrap_failure_file_count="9"
readonly expected_retry2_bootstrap_failure_directory_count="4"
readonly expected_retry2_bootstrap_failure_entry_count="13"
readonly retry2_windows_safe_publication_authority_schema_id="${retry2_schema_id}.windows_safe_publication_authority.v1"
readonly expected_retry2_windows_safe_publication_authority_sha256="12c45c6d2c933fb426c73c9101623b2f61840d2aef6ceba1cde919a2f807a177"
readonly expected_retry2_windows_safe_publication_runner_sha256="a099539da4d9d561ffe213cbb6aaabe510b942db5e73be60265042d2d4523cdb"
readonly expected_retry2_windows_safe_publication_runner_inode="168884986026428276"
readonly expected_retry2_windows_safe_publication_runner_device="66"
readonly expected_retry2_windows_safe_publication_runner_bytes="399999"
readonly retry2_tmp_scan_race_observation_schema_id="${retry2_schema_id}.pre_attempt_tmp_scan_race_observation.v1"
readonly expected_retry2_tmp_scan_race_observation_sha256="2ff00d3706a5edc8ed43d7f54e91006da45f8ceb3953545f12b569a0c332b776"
readonly expected_retry2_tmp_scan_race_observation_bytes="3202"
readonly expected_retry2_tmp_scan_race_observation_lines="57"
readonly expected_retry2_published_runtime_inode="110901140824013045"
readonly expected_retry2_published_runtime_lock_inode="23643898044239335"
readonly retry2_windows_safe_publication_authority_v2_schema_id="${retry2_schema_id}.windows_safe_publication_authority.v2"

# Retry3 is deliberately inert until the independently published Retry2
# interruption closure and completed-prefix bundle are pinned here.  All-zero
# SHA-256 values are fail-closed design sentinels, never wildcard authorities.
readonly retry2_stage04_interruption_closure_schema_id="${retry2_schema_id}_stage04_interruption_closure_v1"
readonly retry2_completed_prefix_bundle_schema_id="${retry2_schema_id}_completed_prefix_bundle_for_retry3_v1"
readonly unsealed_authority_sha256="0000000000000000000000000000000000000000000000000000000000000000"
readonly expected_retry2_stage04_interruption_closure_receipt_sha256="d3b5c587f135335d97ed27a20dd6aa17d9f02e67760378524f04f437bfe87903"
readonly expected_retry2_stage04_interruption_regular_inventory_sha256="943faa9ee84a7d8f9a2bc50ab2b710be6061f19c57489139de39f8aefab5ab9f"
readonly expected_retry2_stage04_interruption_directory_inventory_sha256="4b08c785ea4d3be07fa03d21dca4842149eed8b06da89ecbc914528ebf3aeef1"
readonly expected_retry2_stage04_interruption_amendment_sha256="c0310d27fea46c97ee9517362b809c6f53c8d848a3ea9023a3d7aaf1c3a347f6"
readonly expected_retry2_stage04_interruption_sealer_sha256="c7ea8d0ab52a8395da19aaca5e8d9136a1f0f4ed74c62c94df9a7b94eda0e05f"
readonly expected_retry2_completed_prefix_bundle_receipt_sha256="f0bf3009089c132a23f107e8286d7f785c25e64ed78b62f9419ab25fe6eb2b35"
readonly expected_retry2_completed_prefix_regular_inventory_sha256="82a9b96c4f0eaeb651aa1312d00ecf8f060cf6f2e6614b22fa1f32c3937603a5"
readonly expected_retry2_completed_prefix_directory_inventory_sha256="74d8f94798d1f5dcddcfdb952c6fa5ffe38f2a5e8657c9198b3579d54317114c"
readonly expected_retry2_completed_prefix_sealer_sha256="17dbdc1dc9d1e542d1c59f8f62ffe8ee0655f84638bcd7fce0dd8c54ac5e3d3d"
readonly expected_retry2_operational_runner_sha256="91915b7d32f0c1679d69e9077bbf8eb88777e367f590b34c2832a11fdcc26768"
readonly expected_retry2_stage00_attempt_sha256="ed75d13c10f3381023b9bd648eca4a25dc4eb1d5956ef28340049ef27d07fa69"
readonly expected_retry2_stage00_completion_sha256="3cccdeae5b2765cbcc2c7c03095562b4a3963538ce7618e439a62ad703635433"
readonly expected_retry2_stage01_attempt_sha256="629bec6dd7ec10465c1c11bf51c9710af2598bd4aa25cba3ca29b72c82c883c3"
readonly expected_retry2_stage01_completion_sha256="5f35515e76287c647e2bdd09a6b466b548c98393c8b40b3705f986def02ef741"
readonly expected_retry2_stage02_attempt_sha256="901c1d9e0501cdf23c2754fd1c18872dd05c905755aa3fa009d4d290a3356243"
readonly expected_retry2_stage02_completion_sha256="54371e6aa019d4b2af3be819c13162812a9dd13c80624c7efd7340dd166900f8"
readonly expected_retry2_stage03_attempt_sha256="ec7c471d3c4fafb959734d1ad8e7b716ab5535027951d55db0c812bb7fee6f1c"
readonly expected_retry2_stage03_completion_sha256="d0ba0e40b8489a660196e23ccb1c63bfc198dfa22a2d3b40115e48b12fd60693"
readonly expected_retry2_stage04_attempt_sha256="19a7597dbe5a94f97908de3103cfa62d4e144c7648a5c91fbe82398d6cb82ae2"
readonly expected_retry2_time_only_training_status_sha256="2643e01ff5788665a82da62408c98ff543421b941aee882ad1c8a692f28557b9"
readonly expected_retry2_time_only_checkpoint_sha256="f30aef1d8ea1c69ce17b2817e287355cf0d38e77076deaae4acdd560218972ac"
readonly expected_retry2_time_only_manifest_sha256="fb6ea4be431ffb18221f450b00876f3d40c98cd8a9911a907a9babc72b070dd9"
readonly expected_retry2_time_only_result_sha256="0334acc68fbde37deea3a578d4f8e08c5e2028d2c3ac070b5df5cd50bfc5bebe"
readonly expected_retry2_time_only_report_sha256="1a8323434eddd890ccabde2d85bdfe3584410f86da5f8ca185a02d15a8e8f4d1"
readonly expected_retry2_time_only_log_sha256="3ed5a767ce37af82a1d6a649f17718006b9dd5f22a5cb5e17293a736053dfe7a"

# These pins identify the separately sealed immutable retry1 endpoint bundle.
# Retry2 verifies that bundle directly and consumes only its own second copy.
readonly endpoint_bundle_schema_id="${retry1_schema_id}_endpoint_bundle_for_retry2_v1"
readonly expected_endpoint_bundle_sealer_sha256="b2edac9ef89d2ff630a5dbf33c041f2d3016c3fffbe74a66f5a5c38975d01a77"
readonly expected_endpoint_bundle_amendment_sha256="c94c282d93844563f83abf3e1826111e14d640370d38e778bee04070aa1303ad"
readonly expected_endpoint_bundle_receipt_sha256="ff675afc779b106f628f3ea65fe3409314bf6ea29a531100e73dfa1a3cca9f96"
readonly expected_endpoint_bundle_regular_inventory_sha256="78171e0900f9e034642a85320dbcabb2b52f57eed40a73ca55a973c5f65efc6d"
readonly expected_endpoint_bundle_directory_inventory_sha256="8e23a668bad459c9effdd89d45d4c6c461f546daf544d06a7e4ae9b653c9e6ec"
readonly expected_endpoint_bundle_content_inventory_sha256="bd2f8d55b4e3e3a3a06bf14749b28ea0bec01ea9c07aaa1c1628e9ed4f59e13f"
readonly expected_endpoint_bundle_total_file_count=26
readonly expected_endpoint_bundle_total_directory_count=11
readonly expected_endpoint_snapshot_file_count=21
readonly expected_endpoint_snapshot_directory_count=9
readonly expected_endpoint_snapshot_regular_file_bytes=32731999
readonly expected_endpoint_bundle_checkpoint_sha256="09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec"
readonly expected_endpoint_bundle_policy_sha256="c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8"
readonly expected_endpoint_bundle_net_sha256="42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e"
readonly expected_endpoint_bundle_train_config_sha256="c517ef409c1829413d18536851aecc48bb94e7f7ef2ed1386106511ee1e3ef28"
readonly expected_endpoint_bundle_capture_config_sha256="63e042f47bbdbc2970cd8afbfcc639fcec5a7a980aacbf7a59d8db592f97f821"
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
readonly -a development_stage_names=(
  initialize_from_retry2_prefix
  canonical_import_from_retry2
  endpoint_import_from_retry2
  time_only_import_from_retry2
  no_tf_alignment_training_restart
  endpoint_scale_capture
  time_only_capture
  no_tf_alignment_capture
  endpoint_scale_affine
  time_only_affine
  no_tf_alignment_affine
  selection_and_development
)
readonly development_stage_count="${#development_stage_names[@]}"
readonly minimum_cuwacunu_available_bytes=17179869184
readonly minimum_root_available_bytes=68719476736
readonly maximum_tmp_regular_file_bytes=1073741824
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
  echo "v2 representation ablation retry3: $*" >&2
  exit 1
}

reject_forbidden_path() {
  local path="$1"
  case "${path}" in
  "${retry2_runtime}" | "${retry2_runtime}"/*)
    fail "terminal Retry2 runtime is forbidden; consume only its sealed completed-prefix bundle: ${path}"
    ;;
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

require_not_writable() {
  local path="$1"
  local label="$2"
  local mode
  mode="$(stat -c '%A' -- "${path}")" ||
    fail "could not read permissions for ${label}: ${path}"
  [[ "${mode}" != *w* ]] || fail "${label} is writable: ${path}"
}

require_immutable_file() {
  require_nonempty_file "$1"
  require_not_writable "$1" "required immutable file"
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

path_is_absent() {
  reject_symlink_components "$1"
  [[ ! -e "$1" && ! -L "$1" ]]
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
process_owner_uid="$(id -u)"
process_owner_gid="$(id -g)"
process_start_runner_sha256="$(sha256_of "${script_path}")"
process_start_runner_inode="$(stat -c '%i' -- "${script_path}")"
process_start_runner_device="$(stat -c '%d' -- "${script_path}")"
process_start_runner_bytes="$(stat -c '%s' -- "${script_path}")"
process_start_runner_owner="$(stat -c '%u' -- "${script_path}")"
[[ "${process_start_runner_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
  fail "could not snapshot a valid operational runner sha256"
readonly process_owner_uid process_owner_gid process_start_runner_sha256
readonly process_start_runner_inode process_start_runner_device
readonly process_start_runner_bytes process_start_runner_owner

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

require_owned_single_link_file() {
  local path="$1" label="$2"
  require_file "${path}"
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
  ' "${path}")" || fail "could not count ${key}= entries in ${path}"
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
  ' "${path}")" || fail "could not read ${key}= from ${path}"
  printf '%s' "${value}"
}

validate_receipt_sha256_fields() {
  local path="$1"
  require_nonempty_file "${path}"
  LC_ALL=C awk '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      key = substr($0, 1, eq - 1);
      value = substr($0, eq + 1);
      if (key !~ /sha256$/) next;
      count += 1;
      if (key == "previous_stage_completion_sha256" && value == "none") {
        next;
      }
      if (length(value) != 64 || value !~ /^[0-9a-f]+$/) {
        printf "invalid receipt sha256 field %s=%s\n", key, value > "/dev/stderr";
        invalid = 1;
      }
    }
    END {
      if (count == 0) exit 43;
      if (invalid) exit 42;
    }
  ' "${path}" || fail "receipt has missing or malformed sha256 evidence: ${path}"
}

validate_local_receipt_nonempty_fields() {
  local path="$1"
  require_nonempty_file "${path}"
  LC_ALL=C awk '
    /^[[:space:]]*$/ { next; }
    {
      eq = index($0, "=");
      if (eq <= 1 || eq == length($0)) {
        printf "empty or malformed local receipt field at line %d\n", NR > "/dev/stderr";
        invalid = 1;
      }
    }
    END { if (invalid) exit 42; }
  ' "${path}" || fail "local receipt has an empty or malformed field: ${path}"
}

expect_kv() {
  local path="$1"
  local key="$2"
  local expected="$3"
  local actual
  case "${path}" in
  *.status) validate_receipt_sha256_fields "${path}" ;;
  esac
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
  local candidate_device destination_parent destination_device
  assert_operational_runner_identity
  require_contained_path "${candidate}" "${scratch_root}"
  require_contained_path "${destination}" "${runtime_root}"
  require_file "${candidate}"
  case "${destination}" in
  *.status)
    validate_receipt_sha256_fields "${candidate}"
    validate_local_receipt_nonempty_fields "${candidate}"
    ;;
  esac
  [[ "$(stat -c '%h' -- "${candidate}")" == 1 ]] ||
    fail "publication candidate has an external hard link: ${candidate}"
  [[ "$(stat -c '%u' -- "${candidate}")" == "${process_owner_uid}" ]] ||
    fail "publication candidate is not owned by the executing uid: ${candidate}"
  candidate_device="$(stat -c '%d' -- "${candidate}")" ||
    fail "could not read publication candidate device: ${candidate}"
  destination_parent="$(dirname "${destination}")" ||
    fail "could not resolve publication destination parent: ${destination}"
  require_dir "${destination_parent}"
  destination_device="$(stat -c '%d' -- "${destination_parent}")" ||
    fail "could not read publication destination device: ${destination_parent}"
  [[ "${candidate_device}" =~ ^[0-9]+$ && \
    "${destination_device}" =~ ^[0-9]+$ ]] ||
    fail "publication device metadata is malformed"
  [[ "${candidate_device}" == "${destination_device}" ]] ||
    fail "publication candidate/destination devices differ"
  reject_symlink_components "${destination}"
  chmod 0444 -- "${candidate}"
  if [[ -e "${destination}" || -L "${destination}" ]]; then
    require_immutable_file "${destination}"
    case "${destination}" in
    *.status)
      validate_receipt_sha256_fields "${destination}"
      validate_local_receipt_nonempty_fields "${destination}"
      ;;
    esac
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
  case "${destination}" in
  *.status)
    validate_receipt_sha256_fields "${destination}"
    validate_local_receipt_nonempty_fields "${destination}"
    ;;
  esac
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
retry2_runtime="${runtime_parent}/${retry2_schema_id}"
retry2_stage04_interruption_closure="${runtime_parent}/${retry2_stage04_interruption_closure_schema_id}"
retry2_stage04_interruption_closure_receipt="${retry2_stage04_interruption_closure}/interruption_closure.status"
retry2_stage04_interruption_regular_inventory="${retry2_stage04_interruption_closure}/source_regular_files.inventory.tsv"
retry2_stage04_interruption_directory_inventory="${retry2_stage04_interruption_closure}/source_directories.inventory.tsv"
retry2_stage04_interruption_snapshot="${retry2_stage04_interruption_closure}/source_snapshot"
retry2_stage04_interruption_live_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY2_STAGE04_INTERRUPTION_RECOVERY_AMENDMENT.md"
retry2_stage04_interruption_live_sealer="${script_dir}/seal_and_verify_representation_ablation_retry2_stage04_interruption_closure_v1.sh"
retry2_stage04_interruption_frozen_amendment="${retry2_stage04_interruption_closure}/frozen_sources/$(basename "${retry2_stage04_interruption_live_amendment}")"
retry2_stage04_interruption_frozen_sealer="${retry2_stage04_interruption_closure}/frozen_sources/$(basename "${retry2_stage04_interruption_live_sealer}")"
retry2_completed_prefix_bundle="${runtime_parent}/${retry2_completed_prefix_bundle_schema_id}"
retry2_completed_prefix_bundle_receipt="${retry2_completed_prefix_bundle}/completed_prefix_bundle.status"
retry2_completed_prefix_regular_inventory="${retry2_completed_prefix_bundle}/regular_files.inventory.tsv"
retry2_completed_prefix_directory_inventory="${retry2_completed_prefix_bundle}/directories.inventory.tsv"
retry2_completed_prefix_snapshot="${retry2_completed_prefix_bundle}/completed_prefix"
retry2_completed_prefix_live_sealer="${script_dir}/seal_and_verify_representation_ablation_retry2_completed_prefix_bundle_for_retry3_v1.sh"
retry2_completed_prefix_frozen_sealer="${retry2_completed_prefix_bundle}/frozen_sources/$(basename "${retry2_completed_prefix_live_sealer}")"
retry2_bootstrap_failure_closure="${runtime_parent}/${retry2_bootstrap_failure_closure_schema_id}"
retry2_bootstrap_failure_closure_candidate="${runtime_parent}/.${retry2_bootstrap_failure_closure_schema_id}.candidate"
retry2_bootstrap_failure_receipt="${retry2_bootstrap_failure_closure}/failure.status"
retry2_bootstrap_failure_regular_inventory="${retry2_bootstrap_failure_closure}/residue_regular_files.inventory.tsv"
retry2_bootstrap_failure_directory_inventory="${retry2_bootstrap_failure_closure}/residue_directories.inventory.tsv"
retry2_bootstrap_failure_frozen_root="${retry2_bootstrap_failure_closure}/frozen"
retry2_bootstrap_failure_frozen_old_runner="${retry2_bootstrap_failure_frozen_root}/old_runner.sh"
retry2_bootstrap_failure_frozen_old_amendment="${retry2_bootstrap_failure_frozen_root}/old_amendment.md"
retry2_bootstrap_failure_frozen_erratum="${retry2_bootstrap_failure_frozen_root}/windows_erratum.md"
retry2_bootstrap_failure_frozen_observation="${retry2_bootstrap_failure_frozen_root}/failure_observation.txt"
retry2_bootstrap_failure_frozen_sealer="${retry2_bootstrap_failure_frozen_root}/quarantine_sealer.sh"
retry2_bootstrap_failure_residue="${retry2_bootstrap_failure_closure}/residue"
retry2_bootstrap_failure_quarantined_candidate="${retry2_bootstrap_failure_residue}/${retry2_schema_id}.runtime_root.candidate"
retry2_bootstrap_failure_quarantined_lock="${retry2_bootstrap_failure_quarantined_candidate}/.development.lock"
prior_failed_runtime="${runtime_parent}/${prior_failed_schema_id}"
failure_closure_runtime="${runtime_parent}/${failure_closure_schema_id}"
retry1_runtime="${runtime_parent}/${retry1_schema_id}"
retry1_interruption_closure_runtime="${runtime_parent}/${retry1_interruption_closure_schema_id}"
retry1_interruption_closure_receipt="${retry1_interruption_closure_runtime}/interruption_closure.status"
retry1_interruption_regular_inventory="${retry1_interruption_closure_runtime}/regular_files.inventory.tsv"
retry1_interruption_directory_inventory="${retry1_interruption_closure_runtime}/directories.inventory.tsv"
retry1_interruption_frozen_amendment="${retry1_interruption_closure_runtime}/frozen_sources/REPRESENTATION_ABLATION_RETRY1_OPERATIONAL_INTERRUPTION_RECOVERY_AMENDMENT.md"
retry1_interruption_frozen_sealer="${retry1_interruption_closure_runtime}/frozen_sources/seal_and_verify_representation_ablation_retry1_interruption_closure_v1.sh"
retry1_interruption_live_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY1_OPERATIONAL_INTERRUPTION_RECOVERY_AMENDMENT.md"
retry1_interruption_live_sealer="${script_dir}/seal_and_verify_representation_ablation_retry1_interruption_closure_v1.sh"
retry1_live_runner="${script_dir}/run_representation_ablation_v2_retry1.sh"
retry1_endpoint_training_status="${retry1_runtime}/arms/endpoint_scale/training.status"
retry1_endpoint_runtime_result="${retry1_runtime}/arms/endpoint_scale/training/job/runtime.result.fact"
retry1_endpoint_report="${retry1_runtime}/arms/endpoint_scale/training/job/channel_representation.report"
retry1_endpoint_manifest="${retry1_runtime}/arms/endpoint_scale/training/job/job.manifest"
retry1_endpoint_checkpoint="${retry1_runtime}/arms/endpoint_scale/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"

endpoint_bundle_root="${runtime_parent}/${endpoint_bundle_schema_id}"
endpoint_bundle_snapshot="${endpoint_bundle_root}/source_snapshot"
endpoint_bundle_receipt="${endpoint_bundle_root}/endpoint_bundle.status"
endpoint_bundle_regular_inventory="${endpoint_bundle_root}/source_regular_files.inventory.tsv"
endpoint_bundle_directory_inventory="${endpoint_bundle_root}/source_directories.inventory.tsv"
endpoint_bundle_live_sealer="${script_dir}/seal_and_verify_representation_ablation_retry1_endpoint_bundle_for_retry2_v1.sh"
endpoint_bundle_live_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY1_ENDPOINT_BUNDLE_FOR_RETRY2_AMENDMENT.md"
endpoint_bundle_frozen_sealer="${endpoint_bundle_root}/frozen_sources/$(basename "${endpoint_bundle_live_sealer}")"
endpoint_bundle_frozen_amendment="${endpoint_bundle_root}/frozen_sources/$(basename "${endpoint_bundle_live_amendment}")"
endpoint_bundle_checkpoint="${endpoint_bundle_snapshot}/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
endpoint_bundle_policy="${endpoint_bundle_snapshot}/config/representation.jkimyei"
endpoint_bundle_net="${endpoint_bundle_snapshot}/config/representation.net"
endpoint_bundle_train_config="${endpoint_bundle_snapshot}/config/train.config"
endpoint_bundle_capture_config="${endpoint_bundle_snapshot}/config/capture.config"
retry2_amendment="${script_dir}/REPRESENTATION_ABLATION_RETRY2_STAGED_RECOVERY_AMENDMENT.md"
retry2_bootstrap_failure_live_erratum="${script_dir}/REPRESENTATION_ABLATION_RETRY2_BOOTSTRAP_PUBLICATION_WINDOWS_ERRATUM.md"
retry2_bootstrap_failure_live_observation="${script_dir}/REPRESENTATION_ABLATION_RETRY2_BOOTSTRAP_PUBLICATION_FAILURE_OBSERVATION.txt"
retry2_bootstrap_failure_live_sealer="${script_dir}/seal_and_quarantine_representation_ablation_retry2_bootstrap_publication_failure_v1.sh"
retry2_windows_safe_publication_authority="${script_dir}/REPRESENTATION_ABLATION_RETRY2_WINDOWS_SAFE_PUBLICATION_AUTHORITY.status"
retry2_tmp_scan_race_observation="${script_dir}/REPRESENTATION_ABLATION_RETRY2_PREATTEMPT_TMP_SCAN_RACE_OBSERVATION.status"
retry2_windows_safe_publication_authority_v2="${script_dir}/REPRESENTATION_ABLATION_RETRY2_WINDOWS_SAFE_PUBLICATION_AUTHORITY_V2.status"

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
frozen_runner="${frozen_root}/run_representation_ablation_v2_retry3.sh"
frozen_helper="${frozen_root}/frozen_representation_affine_probe.cpp"
frozen_binary="${frozen_root}/frozen_representation_affine_probe"
arms_root="${runtime_root}/arms"
config_closure="${runtime_root}/config_inputs.status"
effective_grammar_closure="${runtime_root}/effective_grammar_closure.status"
input_receipt="${runtime_root}/inputs.status"
retry_attempt_sentinel="${runtime_root}/stage.00.initialize_from_retry2_prefix.status"
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
endpoint_imports_root="${runtime_root}/imports"
endpoint_import_root="${endpoint_imports_root}/retry2_endpoint_v1"
endpoint_import_checkpoint="${endpoint_import_root}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
endpoint_import_source_bundle_receipt="${endpoint_import_root}/source_completed_prefix_bundle.status"
endpoint_import_source_status="${endpoint_import_root}/source_endpoint_import.status"
endpoint_import_receipt="${endpoint_import_root}/endpoint_import.status"
time_only_import_root="${endpoint_imports_root}/retry2_time_only_v1"
time_only_import_checkpoint="${time_only_import_root}/channel_representation.report.mtf_jepa_mae_vicreg.pt"
time_only_import_source_bundle_receipt="${time_only_import_root}/source_completed_prefix_bundle.status"
time_only_import_source_status="${time_only_import_root}/source_training.status"
time_only_import_receipt="${time_only_import_root}/time_only_import.status"

retry2_prefix_canonical_status="${retry2_completed_prefix_snapshot}/arms/canonical/import.status"
retry2_prefix_canonical_main_report="${retry2_completed_prefix_snapshot}/arms/canonical/affine/main.report"
retry2_prefix_canonical_replay_report="${retry2_completed_prefix_snapshot}/arms/canonical/affine/replay.report"
retry2_prefix_endpoint_status="${retry2_completed_prefix_snapshot}/imports/retry1_endpoint_v1/endpoint_import.status"
retry2_prefix_endpoint_checkpoint="${retry2_completed_prefix_snapshot}/imports/retry1_endpoint_v1/channel_representation.report.mtf_jepa_mae_vicreg.pt"
retry2_prefix_time_only_status="${retry2_completed_prefix_snapshot}/arms/time_only/training.status"
retry2_prefix_time_only_checkpoint="${retry2_completed_prefix_snapshot}/arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
retry2_prefix_time_only_manifest="${retry2_completed_prefix_snapshot}/arms/time_only/training/job/job.manifest"
retry2_prefix_time_only_result="${retry2_completed_prefix_snapshot}/arms/time_only/training/job/runtime.result.fact"
retry2_prefix_time_only_report="${retry2_completed_prefix_snapshot}/arms/time_only/training/job/channel_representation.report"
retry2_prefix_time_only_log="${retry2_completed_prefix_snapshot}/arms/time_only/training.log"
runtime_development_lock="${runtime_root}/.development.lock"
historical_bootstrap_scratch_root="${runtime_parent}/.${schema_id}.preflight_scratch"
bootstrap_scratch_root="${runtime_parent}/.${schema_id}.windows_safe.preflight_scratch"
runtime_root_candidate="${bootstrap_scratch_root}/${schema_id}.runtime_root.candidate"
runtime_root_candidate_lock="${runtime_root_candidate}/.development.lock"
runtime_publication_guard="${bootstrap_scratch_root}/${schema_id}.runtime_root.publication_in_progress.status"
scratch_root="${bootstrap_scratch_root}"
bootstrap_lock_acquired=false
development_lock_acquired=false
runtime_publication_ready=false

mode="${1:---plan}"
[[ "$#" -le 1 ]] ||
  fail "usage: $0 [--plan|--preflight|--advance-development|--verify-development|--run-certified|--verify]"
case "${mode}" in
--plan | --preflight | --advance-development | --verify-development | --run-certified | --verify) ;;
--run-development)
  fail "legacy monolithic --run-development is disabled; use --advance-development, which consumes exactly one fixed stage"
  ;;
--run)
  fail "unconditional --run is disabled; advance all development stages and only then use --run-certified"
  ;;
*)
  fail "usage: $0 [--plan|--preflight|--advance-development|--verify-development|--run-certified|--verify]"
  ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
development_driver=one_fixed_stage_per_advance_invocation
development_stage_count=${development_stage_count}
development_stage_order=00_initialize_from_retry2_prefix,01_canonical_import_from_retry2,02_endpoint_import_from_retry2,03_time_only_import_from_retry2,04_no_tf_alignment_training_restart,05_endpoint_scale_capture,06_time_only_capture,07_no_tf_alignment_capture,08_endpoint_scale_affine,09_time_only_affine,10_no_tf_alignment_affine,11_selection_and_development
legacy_run_development_enabled=false
completed_prefix_policy=verify_and_skip
attempt_without_completion_policy=terminal
partial_payload_adoption_authorized=false
checkpoint_resume_authorized=false
retry2_stage04_interruption_closure_schema_id=${retry2_stage04_interruption_closure_schema_id}
retry2_stage04_interruption_closure_receipt=${retry2_stage04_interruption_closure_receipt}
retry2_stage04_interruption_closure_receipt_sha256=${expected_retry2_stage04_interruption_closure_receipt_sha256}
retry2_completed_prefix_bundle_schema_id=${retry2_completed_prefix_bundle_schema_id}
retry2_completed_prefix_bundle_receipt=${retry2_completed_prefix_bundle_receipt}
retry2_completed_prefix_bundle_receipt_sha256=${expected_retry2_completed_prefix_bundle_receipt_sha256}
retry2_completed_prefix_payload=${retry2_completed_prefix_snapshot}
retry2_direct_runtime_access=false
retry2_completed_prefix_count=4
retry2_completed_prefix_head_sha256=${expected_retry2_stage03_completion_sha256}
retry2_terminal_stage04_attempt_sha256=${expected_retry2_stage04_attempt_sha256}
retry2_partial_stage04_artifact_reuse_authorized=false
endpoint_historical_training_reuse=verified_retry2_prefix_bundle_local_import_copy_only
time_only_historical_training_reuse=verified_retry2_prefix_bundle_local_import_copy_only
time_only_retry2_optimizer_steps=3000
time_only_retry3_optimizer_steps=0
time_only_retry3_training_job_created=false
no_tf_alignment_retry3_optimizer_start_step=0
no_tf_alignment_retry3_optimizer_steps=3000
no_tf_alignment_input_checkpoint=none
retry1_interruption_closure_receipt=${retry1_interruption_closure_receipt}
retry1_interruption_closure_receipt_sha256=${expected_retry1_interruption_closure_receipt_sha256}
retry1_runtime_content_inventory_sha256=${expected_retry1_runtime_content_inventory_sha256}
retry2_staged_recovery_amendment=${retry2_amendment}
retry2_staged_recovery_amendment_sha256=${expected_retry2_amendment_sha256}
retry2_bootstrap_failure_closure_schema_id=${retry2_bootstrap_failure_closure_schema_id}
retry2_bootstrap_failure_closure_path=${retry2_bootstrap_failure_closure}
retry2_bootstrap_failure_receipt_path=${retry2_bootstrap_failure_receipt}
retry2_bootstrap_failure_receipt_sha256=${expected_retry2_bootstrap_failure_receipt_sha256}
retry2_bootstrap_failure_regular_inventory_sha256=${expected_retry2_bootstrap_failure_regular_inventory_sha256}
retry2_bootstrap_failure_directory_inventory_sha256=${expected_retry2_bootstrap_failure_directory_inventory_sha256}
retry2_bootstrap_failure_scientific_input=false
retry2_windows_safe_publication_authority_v1_path=${retry2_windows_safe_publication_authority}
retry2_windows_safe_publication_authority_v1_sha256=${expected_retry2_windows_safe_publication_authority_sha256}
retry2_tmp_scan_race_observation_path=${retry2_tmp_scan_race_observation}
retry2_tmp_scan_race_observation_sha256=${expected_retry2_tmp_scan_race_observation_sha256}
retry2_windows_safe_publication_authority_v2_path=${retry2_windows_safe_publication_authority_v2}
retry2_windows_safe_publication_authority_v2_required=true
historical_bootstrap_scratch_path=${historical_bootstrap_scratch_root}
windows_safe_bootstrap_scratch_path=${bootstrap_scratch_root}
runtime_publication_guard_path=${runtime_publication_guard}
runtime_publication_lock_order=bootstrap_then_development_then_certified
runtime_publication_method=close_candidate_lock_then_no_clobber_rename_then_read_only_reopen
runtime_publication_guard_required=true
endpoint_bundle_schema_id=${endpoint_bundle_schema_id}
endpoint_bundle_sealer=${endpoint_bundle_live_sealer}
endpoint_bundle_sealer_sha256=${expected_endpoint_bundle_sealer_sha256}
endpoint_bundle_amendment=${endpoint_bundle_live_amendment}
endpoint_bundle_amendment_sha256=${expected_endpoint_bundle_amendment_sha256}
endpoint_bundle_receipt=${endpoint_bundle_receipt}
endpoint_bundle_receipt_sha256=${expected_endpoint_bundle_receipt_sha256}
endpoint_bundle_checkpoint_sha256=${expected_endpoint_bundle_checkpoint_sha256}
endpoint_bundle_policy_sha256=${expected_endpoint_bundle_policy_sha256}
endpoint_bundle_net_sha256=${expected_endpoint_bundle_net_sha256}
endpoint_bundle_train_config_sha256=${expected_endpoint_bundle_train_config_sha256}
endpoint_bundle_capture_config_sha256=${expected_endpoint_bundle_capture_config_sha256}
endpoint_import_root=${endpoint_import_root}
endpoint_import_copy_mode=cp_reflink_never
endpoint_import_hardlink_authorized=false
endpoint_import_retry3_optimizer_steps=0
minimum_cuwacunu_available_bytes=${minimum_cuwacunu_available_bytes}
minimum_root_available_bytes=${minimum_root_available_bytes}
maximum_tmp_regular_file_bytes=${maximum_tmp_regular_file_bytes}
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
initialization_stage_receipt=${retry_attempt_sentinel}
initialization_kind=fresh_retry3_with_fixed_retry2_prefix_authority
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
endpoint_scale_retry3_optimizer_steps=0
time_only_historical_optimizer_steps=3000
time_only_retry3_optimizer_steps=0
no_tf_alignment_retry3_optimizer_start_step=0
no_tf_alignment_retry3_optimizer_steps=${expected_steps}
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

verify_retry2_bootstrap_failure_closure_authority() {
  local special file_count directory_count entry_count receipt_lines path metadata
  local -a expected_directories expected_files

  path_is_absent "${retry2_bootstrap_failure_closure_candidate}" ||
    fail "retry2 bootstrap failure closure candidate remains"
  path_is_absent "${historical_bootstrap_scratch_root}" ||
    fail "historical retry2 bootstrap residue reappeared outside its closure"
  path_is_absent "${retry2_bootstrap_failure_closure}/.failure.status.prepared" ||
    fail "prepared retry2 bootstrap failure receipt remains"

  expected_directories=(
    "${retry2_bootstrap_failure_closure}"
    "${retry2_bootstrap_failure_frozen_root}"
    "${retry2_bootstrap_failure_residue}"
    "${retry2_bootstrap_failure_quarantined_candidate}"
  )
  expected_files=(
    "${retry2_bootstrap_failure_receipt}"
    "${retry2_bootstrap_failure_regular_inventory}"
    "${retry2_bootstrap_failure_directory_inventory}"
    "${retry2_bootstrap_failure_frozen_old_runner}"
    "${retry2_bootstrap_failure_frozen_old_amendment}"
    "${retry2_bootstrap_failure_frozen_erratum}"
    "${retry2_bootstrap_failure_frozen_observation}"
    "${retry2_bootstrap_failure_frozen_sealer}"
    "${retry2_bootstrap_failure_quarantined_lock}"
  )
  for path in "${expected_directories[@]}"; do
    require_dir "${path}"
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not inspect retry2 bootstrap failure closure directory: ${path}"
    [[ "${metadata}" == \
      "555:${process_owner_uid}:1:${expected_retry2_bootstrap_failure_device}" ]] ||
      fail "retry2 bootstrap failure closure directory metadata drifted: ${path}"
  done
  for path in "${expected_files[@]}"; do
    require_file "${path}"
    metadata="$(stat -c '%a:%u:%h:%d' -- "${path}")" ||
      fail "could not inspect retry2 bootstrap failure closure file: ${path}"
    [[ "${metadata}" == \
      "444:${process_owner_uid}:1:${expected_retry2_bootstrap_failure_device}" ]] ||
      fail "retry2 bootstrap failure closure file metadata drifted: ${path}"
  done

  verify_pinned_mode_file "${retry2_bootstrap_failure_receipt}" \
    "${expected_retry2_bootstrap_failure_receipt_sha256}" 444 \
    "retry2 bootstrap failure closure receipt"
  verify_pinned_mode_file "${retry2_bootstrap_failure_regular_inventory}" \
    "${expected_retry2_bootstrap_failure_regular_inventory_sha256}" 444 \
    "retry2 bootstrap failure regular-file inventory"
  verify_pinned_mode_file "${retry2_bootstrap_failure_directory_inventory}" \
    "${expected_retry2_bootstrap_failure_directory_inventory_sha256}" 444 \
    "retry2 bootstrap failure directory inventory"
  verify_pinned_mode_file "${retry2_bootstrap_failure_frozen_old_runner}" \
    "${expected_retry2_bootstrap_failure_old_runner_sha256}" 444 \
    "frozen failed retry2 runner"
  verify_pinned_mode_file "${retry2_bootstrap_failure_frozen_old_amendment}" \
    "${expected_retry2_bootstrap_failure_old_amendment_sha256}" 444 \
    "frozen old retry2 amendment"
  verify_pinned_mode_file "${retry2_bootstrap_failure_frozen_erratum}" \
    "${expected_retry2_bootstrap_failure_erratum_sha256}" 444 \
    "frozen retry2 bootstrap publication erratum"
  verify_pinned_mode_file "${retry2_bootstrap_failure_frozen_observation}" \
    "${expected_retry2_bootstrap_failure_observation_sha256}" 444 \
    "frozen retry2 bootstrap failure observation"
  verify_pinned_mode_file "${retry2_bootstrap_failure_frozen_sealer}" \
    "${expected_retry2_bootstrap_failure_sealer_sha256}" 444 \
    "frozen retry2 bootstrap failure sealer"
  verify_pinned_mode_file "${retry2_bootstrap_failure_quarantined_lock}" \
    "${expected_retry2_bootstrap_failure_empty_sha256}" 444 \
    "quarantined retry2 development lock"
  verify_pinned_mode_file "${retry2_bootstrap_failure_live_erratum}" \
    "${expected_retry2_bootstrap_failure_erratum_sha256}" 444 \
    "live retry2 bootstrap publication erratum"
  verify_pinned_mode_file "${retry2_bootstrap_failure_live_observation}" \
    "${expected_retry2_bootstrap_failure_observation_sha256}" 444 \
    "live retry2 bootstrap failure observation"
  verify_pinned_mode_file "${retry2_bootstrap_failure_live_sealer}" \
    "${expected_retry2_bootstrap_failure_sealer_sha256}" 555 \
    "live retry2 bootstrap failure sealer"

  [[ "$(stat -c '%s' -- "${retry2_bootstrap_failure_receipt}")" == \
    "${expected_retry2_bootstrap_failure_receipt_bytes}" ]] ||
    fail "retry2 bootstrap failure receipt size drifted"
  [[ "$(stat -c '%s' -- "${retry2_bootstrap_failure_regular_inventory}")" == \
    "${expected_retry2_bootstrap_failure_regular_inventory_bytes}" ]] ||
    fail "retry2 bootstrap failure regular inventory size drifted"
  [[ "$(stat -c '%s' -- "${retry2_bootstrap_failure_directory_inventory}")" == \
    "${expected_retry2_bootstrap_failure_directory_inventory_bytes}" ]] ||
    fail "retry2 bootstrap failure directory inventory size drifted"
  [[ "$(stat -c '%s' -- "${retry2_bootstrap_failure_quarantined_lock}")" == 0 ]] ||
    fail "quarantined retry2 development lock is not empty"
  receipt_lines="$(wc -l <"${retry2_bootstrap_failure_receipt}")" ||
    fail "could not count retry2 bootstrap failure receipt lines"
  [[ "${receipt_lines}" == 84 ]] ||
    fail "retry2 bootstrap failure receipt line count drifted"

  special="$(find "${retry2_bootstrap_failure_closure}" -xdev -mindepth 0 \
    ! -type f ! -type d -print -quit)" ||
    fail "could not scan retry2 bootstrap failure closure entry types"
  [[ -z "${special}" ]] ||
    fail "retry2 bootstrap failure closure contains a special entry: ${special}"
  file_count="$(find "${retry2_bootstrap_failure_closure}" -xdev -type f \
    -printf '.' | wc -c)" || fail "could not count retry2 bootstrap closure files"
  directory_count="$(find "${retry2_bootstrap_failure_closure}" -xdev -type d \
    -printf '.' | wc -c)" || fail "could not count retry2 bootstrap closure directories"
  entry_count="$(find "${retry2_bootstrap_failure_closure}" -xdev -mindepth 0 \
    -printf '.' | wc -c)" || fail "could not count retry2 bootstrap closure entries"
  [[ "${file_count}:${directory_count}:${entry_count}" == \
    "${expected_retry2_bootstrap_failure_file_count}:${expected_retry2_bootstrap_failure_directory_count}:${expected_retry2_bootstrap_failure_entry_count}" ]] ||
    fail "retry2 bootstrap failure closure cardinality drifted: ${file_count}:${directory_count}:${entry_count}"

  [[ "$(stat -c '%d:%i' -- "${runtime_parent}")" == \
    "${expected_retry2_bootstrap_failure_device}:${expected_retry2_bootstrap_failure_runtime_parent_inode}" ]] ||
    fail "retry2 bootstrap failure runtime-parent identity drifted"
  [[ "$(stat -c '%d:%i' -- "${retry2_bootstrap_failure_closure}")" == \
    "${expected_retry2_bootstrap_failure_device}:${expected_retry2_bootstrap_failure_closure_inode}" ]] ||
    fail "retry2 bootstrap failure closure identity drifted"
  [[ "$(stat -c '%d:%i' -- "${retry2_bootstrap_failure_residue}")" == \
    "${expected_retry2_bootstrap_failure_device}:${expected_retry2_bootstrap_failure_scratch_inode}" ]] ||
    fail "retry2 bootstrap failure quarantined scratch identity drifted"
  [[ "$(stat -c '%d:%i' -- "${retry2_bootstrap_failure_quarantined_candidate}")" == \
    "${expected_retry2_bootstrap_failure_device}:${expected_retry2_bootstrap_failure_candidate_inode}" ]] ||
    fail "retry2 bootstrap failure quarantined candidate identity drifted"
  [[ "$(stat -c '%d:%i' -- "${retry2_bootstrap_failure_quarantined_lock}")" == \
    "${expected_retry2_bootstrap_failure_device}:${expected_retry2_bootstrap_failure_lock_inode}" ]] ||
    fail "retry2 bootstrap failure quarantined lock identity drifted"

  validate_receipt_sha256_fields "${retry2_bootstrap_failure_receipt}"
  validate_local_receipt_nonempty_fields "${retry2_bootstrap_failure_receipt}"
  expect_kv "${retry2_bootstrap_failure_receipt}" schema_id \
    "${retry2_bootstrap_failure_closure_schema_id}"
  expect_kv "${retry2_bootstrap_failure_receipt}" status complete
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_kind \
    pre_attempt_bootstrap_publication_failure_quarantine
  expect_kv "${retry2_bootstrap_failure_receipt}" scientific_attempt_consumed false
  expect_kv "${retry2_bootstrap_failure_receipt}" optimizer_steps 0
  expect_kv "${retry2_bootstrap_failure_receipt}" candidate_adopted false
  expect_kv "${retry2_bootstrap_failure_receipt}" canonical_runtime_published false
  expect_kv "${retry2_bootstrap_failure_receipt}" stage00_attempt_published false
  expect_kv "${retry2_bootstrap_failure_receipt}" stage00_completion_published false
  expect_kv "${retry2_bootstrap_failure_receipt}" scientific_payload_created false
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_scope \
    container_cooperative_bootstrap_lock
  expect_kv "${retry2_bootstrap_failure_receipt}" host_wide_handle_exclusion_claimed false
  expect_kv "${retry2_bootstrap_failure_receipt}" observed_failure_exit_code 1
  expect_kv "${retry2_bootstrap_failure_receipt}" observed_failure_operation mv_-T_-n
  expect_kv "${retry2_bootstrap_failure_receipt}" observed_failure_result permission_denied
  expect_kv "${retry2_bootstrap_failure_receipt}" old_runner_path "${script_path}"
  expect_kv "${retry2_bootstrap_failure_receipt}" old_runner_sha256 \
    "${expected_retry2_bootstrap_failure_old_runner_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" old_runner_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_bootstrap_failure_receipt}" old_runner_inode \
    "${expected_retry2_bootstrap_failure_old_runner_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" old_amendment_path \
    "${retry2_amendment}"
  expect_kv "${retry2_bootstrap_failure_receipt}" old_amendment_sha256 \
    "${expected_retry2_bootstrap_failure_old_amendment_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" windows_erratum_path \
    "${retry2_bootstrap_failure_live_erratum}"
  expect_kv "${retry2_bootstrap_failure_receipt}" windows_erratum_sha256 \
    "${expected_retry2_bootstrap_failure_erratum_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" failure_observation_path \
    "${retry2_bootstrap_failure_live_observation}"
  expect_kv "${retry2_bootstrap_failure_receipt}" failure_observation_sha256 \
    "${expected_retry2_bootstrap_failure_observation_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_path \
    "${retry2_bootstrap_failure_live_sealer}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_sha256 \
    "${expected_retry2_bootstrap_failure_sealer_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_mode 555
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_uid \
    "${process_owner_uid}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_links 1
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_bytes \
    "${expected_retry2_bootstrap_failure_sealer_bytes}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantine_sealer_inode \
    "${expected_retry2_bootstrap_failure_sealer_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_old_runner_path \
    "${retry2_bootstrap_failure_frozen_old_runner}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_old_runner_sha256 \
    "${expected_retry2_bootstrap_failure_old_runner_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_old_amendment_path \
    "${retry2_bootstrap_failure_frozen_old_amendment}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_old_amendment_sha256 \
    "${expected_retry2_bootstrap_failure_old_amendment_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_windows_erratum_path \
    "${retry2_bootstrap_failure_frozen_erratum}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_windows_erratum_sha256 \
    "${expected_retry2_bootstrap_failure_erratum_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_failure_observation_path \
    "${retry2_bootstrap_failure_frozen_observation}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_failure_observation_sha256 \
    "${expected_retry2_bootstrap_failure_observation_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_quarantine_sealer_path \
    "${retry2_bootstrap_failure_frozen_sealer}"
  expect_kv "${retry2_bootstrap_failure_receipt}" frozen_quarantine_sealer_sha256 \
    "${expected_retry2_bootstrap_failure_sealer_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" regular_file_inventory_path \
    "${retry2_bootstrap_failure_regular_inventory}"
  expect_kv "${retry2_bootstrap_failure_receipt}" regular_file_inventory_sha256 \
    "${expected_retry2_bootstrap_failure_regular_inventory_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" directory_inventory_path \
    "${retry2_bootstrap_failure_directory_inventory}"
  expect_kv "${retry2_bootstrap_failure_receipt}" directory_inventory_sha256 \
    "${expected_retry2_bootstrap_failure_directory_inventory_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_inventory_phase \
    observed_pre_quarantine_pre_seal
  expect_kv "${retry2_bootstrap_failure_receipt}" original_bootstrap_scratch_path \
    "${historical_bootstrap_scratch_root}"
  expect_kv "${retry2_bootstrap_failure_receipt}" original_bootstrap_scratch_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_bootstrap_failure_receipt}" original_bootstrap_scratch_inode \
    "${expected_retry2_bootstrap_failure_scratch_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" original_candidate_root_inode \
    "${expected_retry2_bootstrap_failure_candidate_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" original_candidate_lock_inode \
    "${expected_retry2_bootstrap_failure_lock_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" original_candidate_lock_sha256 \
    "${expected_retry2_bootstrap_failure_empty_sha256}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantined_residue_path \
    "${retry2_bootstrap_failure_residue}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantined_candidate_root_path \
    "${retry2_bootstrap_failure_quarantined_candidate}"
  expect_kv "${retry2_bootstrap_failure_receipt}" quarantined_candidate_lock_path \
    "${retry2_bootstrap_failure_quarantined_lock}"
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_regular_file_count 1
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_directory_count 2
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_total_entry_count 3
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_source_absent_after_quarantine true
  expect_kv "${retry2_bootstrap_failure_receipt}" residue_inode_device_continuity_verified true
  expect_kv "${retry2_bootstrap_failure_receipt}" runtime_parent_path "${runtime_parent}"
  expect_kv "${retry2_bootstrap_failure_receipt}" runtime_parent_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_bootstrap_failure_receipt}" runtime_parent_inode \
    "${expected_retry2_bootstrap_failure_runtime_parent_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_root_path \
    "${retry2_bootstrap_failure_closure}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_root_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_root_inode \
    "${expected_retry2_bootstrap_failure_closure_inode}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_regular_file_count \
    "${expected_retry2_bootstrap_failure_file_count}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_directory_count \
    "${expected_retry2_bootstrap_failure_directory_count}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_total_entry_count \
    "${expected_retry2_bootstrap_failure_entry_count}"
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_regular_file_mode 0444
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_directory_mode 0555
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_publication_same_device true
  expect_kv "${retry2_bootstrap_failure_receipt}" closure_publication_no_clobber true
  expect_kv "${retry2_bootstrap_failure_receipt}" canonical_runtime_path "${runtime_root}"
  expect_kv "${retry2_bootstrap_failure_receipt}" canonical_runtime_absent_after_quarantine true
  expect_kv "${retry2_bootstrap_failure_receipt}" retry2_bootstrap_failure_is_scientific_input false
  expect_kv "${retry2_bootstrap_failure_receipt}" final_holdout_access false
  expect_kv "${retry2_bootstrap_failure_receipt}" policy_access false
}

verify_retry2_windows_safe_publication_authority() {
  verify_retry2_amendment_authority
  verify_retry2_bootstrap_failure_closure_authority
  verify_pinned_mode_file "${retry2_windows_safe_publication_authority}" \
    "${expected_retry2_windows_safe_publication_authority_sha256}" 444 \
    "retry2 Windows-safe publication authority v1"
  [[ "$(stat -c '%a:%u:%h' -- \
    "${retry2_windows_safe_publication_authority}")" == \
    "444:${process_owner_uid}:1" ]] ||
    fail "retry2 Windows-safe publication authority metadata drifted"
  validate_receipt_sha256_fields \
    "${retry2_windows_safe_publication_authority}"
  validate_local_receipt_nonempty_fields \
    "${retry2_windows_safe_publication_authority}"
  expect_kv "${retry2_windows_safe_publication_authority}" schema_id \
    "${retry2_windows_safe_publication_authority_schema_id}"
  expect_kv "${retry2_windows_safe_publication_authority}" status authorized
  expect_kv "${retry2_windows_safe_publication_authority}" authority_kind \
    post_quarantine_operational_runner_revision
  expect_kv "${retry2_windows_safe_publication_authority}" authority_scope \
    bootstrap_publication_and_transitive_evidence_only
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_path "${script_path}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_sha256 \
    "${expected_retry2_windows_safe_publication_runner_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_inode \
    "${expected_retry2_windows_safe_publication_runner_inode}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_device \
    "${expected_retry2_windows_safe_publication_runner_device}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_bytes \
    "${expected_retry2_windows_safe_publication_runner_bytes}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_owner_uid "${process_owner_uid}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_mode 0555
  expect_kv "${retry2_windows_safe_publication_authority}" \
    operational_ablation_runner_links 1
  expect_kv "${retry2_windows_safe_publication_authority}" \
    staged_recovery_amendment_path "${retry2_amendment}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    staged_recovery_amendment_sha256 "${expected_retry2_amendment_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    bootstrap_failure_closure_schema_id \
    "${retry2_bootstrap_failure_closure_schema_id}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    bootstrap_failure_closure_receipt_path \
    "${retry2_bootstrap_failure_receipt}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    bootstrap_failure_closure_receipt_sha256 \
    "${expected_retry2_bootstrap_failure_receipt_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    windows_publication_erratum_path \
    "${retry2_bootstrap_failure_live_erratum}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    windows_publication_erratum_sha256 \
    "${expected_retry2_bootstrap_failure_erratum_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    failure_observation_path "${retry2_bootstrap_failure_live_observation}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    failure_observation_sha256 \
    "${expected_retry2_bootstrap_failure_observation_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    quarantine_sealer_path "${retry2_bootstrap_failure_live_sealer}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    quarantine_sealer_sha256 \
    "${expected_retry2_bootstrap_failure_sealer_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    historical_failed_runner_sha256 \
    "${expected_retry2_bootstrap_failure_old_runner_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    bootstrap_runner_lock_path "${script_path}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_parent_path "${runtime_parent}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    historical_bootstrap_scratch_path "${historical_bootstrap_scratch_root}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    fresh_bootstrap_scratch_path "${bootstrap_scratch_root}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_candidate_path "${runtime_root_candidate}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_candidate_lock_path "${runtime_root_candidate_lock}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_publication_guard_path "${runtime_publication_guard}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    canonical_runtime_path "${runtime_root}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    canonical_runtime_lock_path "${runtime_development_lock}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_publication_lock_order bootstrap_then_development_then_certified
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_publication_method \
    close_candidate_lock_then_no_clobber_rename_then_read_only_reopen
  expect_kv "${retry2_windows_safe_publication_authority}" \
    runtime_root_publication_command mv_-T_-n
  expect_kv "${retry2_windows_safe_publication_authority}" \
    publication_guard_exclusive_create true
  expect_kv "${retry2_windows_safe_publication_authority}" \
    candidate_lock_descriptor_closed_before_rename true
  expect_kv "${retry2_windows_safe_publication_authority}" \
    canonical_lock_reopen_access read_only
  expect_kv "${retry2_windows_safe_publication_authority}" \
    canonical_lock_descriptor_verified_before_attempt true
  expect_kv "${retry2_windows_safe_publication_authority}" \
    guard_removed_only_after_lock_reopen_and_continuity true
  expect_kv "${retry2_windows_safe_publication_authority}" \
    historical_bootstrap_scratch_reuse false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    quarantined_candidate_adopted false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    fresh_full_preflight_required true
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_commands_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_seed_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_ranges_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_comparator_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    endpoint_import_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    development_stage_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    development_stage_count "${development_stage_count}"
  expect_kv "${retry2_windows_safe_publication_authority}" \
    required_independent_audit_count 3
  expect_kv "${retry2_windows_safe_publication_authority}" \
    scientific_attempt_consumed false
  expect_kv "${retry2_windows_safe_publication_authority}" optimizer_steps 0
  expect_kv "${retry2_windows_safe_publication_authority}" \
    certified_input_access false
  expect_kv "${retry2_windows_safe_publication_authority}" \
    final_holdout_access false
  expect_kv "${retry2_windows_safe_publication_authority}" policy_access false
}

verify_retry2_tmp_scan_race_observation() {
  verify_retry2_windows_safe_publication_authority
  verify_pinned_mode_file "${retry2_tmp_scan_race_observation}" \
    "${expected_retry2_tmp_scan_race_observation_sha256}" 444 \
    "retry2 pre-attempt tmp-scan race observation"
  [[ "$(stat -c '%s' -- "${retry2_tmp_scan_race_observation}")" == \
    "${expected_retry2_tmp_scan_race_observation_bytes}" ]] ||
    fail "retry2 tmp-scan race observation size drifted"
  [[ "$(wc -l <"${retry2_tmp_scan_race_observation}")" == \
    "${expected_retry2_tmp_scan_race_observation_lines}" ]] ||
    fail "retry2 tmp-scan race observation line count drifted"
  validate_receipt_sha256_fields "${retry2_tmp_scan_race_observation}"
  validate_local_receipt_nonempty_fields "${retry2_tmp_scan_race_observation}"
  expect_kv "${retry2_tmp_scan_race_observation}" schema_id \
    "${retry2_tmp_scan_race_observation_schema_id}"
  expect_kv "${retry2_tmp_scan_race_observation}" status observed
  expect_kv "${retry2_tmp_scan_race_observation}" failure_kind \
    bounded_tmp_scan_readdir_stat_race
  expect_kv "${retry2_tmp_scan_race_observation}" failure_phase \
    post_publication_pre_stage00_attempt_resource_gate
  expect_kv "${retry2_tmp_scan_race_observation}" observed_failure_exit_code 1
  expect_kv "${retry2_tmp_scan_race_observation}" observed_missing_path \
    /tmp/ccwGNbdb.s
  expect_kv "${retry2_tmp_scan_race_observation}" \
    operational_ablation_runner_path "${script_path}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    operational_ablation_runner_sha256 \
    "${expected_retry2_windows_safe_publication_runner_sha256}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    windows_safe_publication_authority_v1_path \
    "${retry2_windows_safe_publication_authority}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    windows_safe_publication_authority_v1_sha256 \
    "${expected_retry2_windows_safe_publication_authority_sha256}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    bootstrap_failure_closure_receipt_path \
    "${retry2_bootstrap_failure_receipt}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    bootstrap_failure_closure_receipt_sha256 \
    "${expected_retry2_bootstrap_failure_receipt_sha256}"
  expect_kv "${retry2_tmp_scan_race_observation}" canonical_runtime_path \
    "${runtime_root}"
  expect_kv "${retry2_tmp_scan_race_observation}" canonical_runtime_published true
  expect_kv "${retry2_tmp_scan_race_observation}" canonical_runtime_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_tmp_scan_race_observation}" canonical_runtime_inode \
    "${expected_retry2_published_runtime_inode}"
  expect_kv "${retry2_tmp_scan_race_observation}" canonical_runtime_lock_path \
    "${runtime_development_lock}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    canonical_runtime_lock_device "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    canonical_runtime_lock_inode "${expected_retry2_published_runtime_lock_inode}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    canonical_runtime_only_empty_development_lock true
  expect_kv "${retry2_tmp_scan_race_observation}" \
    runtime_publication_transition_completed true
  expect_kv "${retry2_tmp_scan_race_observation}" \
    runtime_publication_guard_absent true
  expect_kv "${retry2_tmp_scan_race_observation}" runtime_candidate_absent true
  expect_kv "${retry2_tmp_scan_race_observation}" \
    fresh_bootstrap_scratch_path "${bootstrap_scratch_root}"
  expect_kv "${retry2_tmp_scan_race_observation}" \
    historical_bootstrap_scratch_absent true
  expect_kv "${retry2_tmp_scan_race_observation}" stage00_attempt_published false
  expect_kv "${retry2_tmp_scan_race_observation}" stage00_completion_published false
  expect_kv "${retry2_tmp_scan_race_observation}" scientific_payload_created false
  expect_kv "${retry2_tmp_scan_race_observation}" scientific_attempt_consumed false
  expect_kv "${retry2_tmp_scan_race_observation}" optimizer_steps 0
  expect_kv "${retry2_tmp_scan_race_observation}" \
    proposed_operational_correction \
    gnu_find_ignore_readdir_race_on_bounded_tmp_scan
  expect_kv "${retry2_tmp_scan_race_observation}" scientific_contract_changed false
  expect_kv "${retry2_tmp_scan_race_observation}" final_holdout_access false
  expect_kv "${retry2_tmp_scan_race_observation}" policy_access false
}

verify_retry2_windows_safe_publication_authority_v2() {
  verify_retry2_tmp_scan_race_observation
  require_nonempty_file "${retry2_windows_safe_publication_authority_v2}"
  [[ "$(stat -c '%a:%u:%h' -- \
    "${retry2_windows_safe_publication_authority_v2}")" == \
    "444:${process_owner_uid}:1" ]] ||
    fail "retry2 Windows-safe publication authority v2 metadata drifted"
  validate_receipt_sha256_fields \
    "${retry2_windows_safe_publication_authority_v2}"
  validate_local_receipt_nonempty_fields \
    "${retry2_windows_safe_publication_authority_v2}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" schema_id \
    "${retry2_windows_safe_publication_authority_v2_schema_id}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" status authorized
  expect_kv "${retry2_windows_safe_publication_authority_v2}" authority_kind \
    post_publication_pre_attempt_tmp_scan_race_correction
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_path "${script_path}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_sha256 "${process_start_runner_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_inode "${process_start_runner_inode}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_device "${process_start_runner_device}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_bytes "${process_start_runner_bytes}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_owner_uid "${process_start_runner_owner}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_mode 0555
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    operational_ablation_runner_links 1
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_runner_sha256 \
    "${expected_retry2_windows_safe_publication_runner_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    windows_safe_publication_authority_v1_path \
    "${retry2_windows_safe_publication_authority}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    windows_safe_publication_authority_v1_sha256 \
    "${expected_retry2_windows_safe_publication_authority_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    tmp_scan_race_observation_path "${retry2_tmp_scan_race_observation}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    tmp_scan_race_observation_sha256 \
    "${expected_retry2_tmp_scan_race_observation_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    bootstrap_failure_closure_receipt_path \
    "${retry2_bootstrap_failure_receipt}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    bootstrap_failure_closure_receipt_sha256 \
    "${expected_retry2_bootstrap_failure_receipt_sha256}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    correction_scope bounded_tmp_scan_readdir_race_only
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    corrected_find_option -ignore_readdir_race
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    oversize_threshold_bytes "${maximum_tmp_regular_file_bytes}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    missing_file_race_tolerated true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    other_find_errors_tolerated false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    oversize_file_detection_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_runtime_path "${runtime_root}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_runtime_inode "${expected_retry2_published_runtime_inode}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_runtime_lock_path "${runtime_development_lock}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_runtime_lock_inode \
    "${expected_retry2_published_runtime_lock_inode}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_pre_attempt_root_reuse_authorized true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    canonical_root_lock_inode_continuity_required true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    runtime_publication_guard_absent true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    runtime_candidate_absent true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    fresh_bootstrap_scratch_empty true
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_stage00_attempt_published false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_stage00_completion_published false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_scientific_payload_created false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_scientific_attempt_consumed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    previous_optimizer_steps 0
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    scientific_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    scientific_commands_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    scientific_seed_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    scientific_ranges_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    scientific_comparator_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    endpoint_import_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    development_stage_contract_changed false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    development_stage_count "${development_stage_count}"
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    required_independent_audit_count 3
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    certified_input_access false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" \
    final_holdout_access false
  expect_kv "${retry2_windows_safe_publication_authority_v2}" policy_access false
  require_dir "${runtime_root}"
  require_file "${runtime_development_lock}"
  [[ "$(stat -c '%a:%u:%h:%d:%i' -- "${runtime_root}")" == \
    "700:${process_owner_uid}:1:${expected_retry2_bootstrap_failure_device}:${expected_retry2_published_runtime_inode}" ]] ||
    fail "retry2 published pre-attempt runtime identity drifted"
  [[ "$(stat -c '%a:%u:%h:%s:%d:%i' -- \
    "${runtime_development_lock}")" == \
    "600:${process_owner_uid}:1:0:${expected_retry2_bootstrap_failure_device}:${expected_retry2_published_runtime_lock_inode}" ]] ||
    fail "retry2 published pre-attempt development-lock identity drifted"
}

emit_retry2_bootstrap_failure_closure_bindings() {
  verify_retry2_bootstrap_failure_closure_authority
  cat <<BOOTSTRAP_FAILURE_BINDING
retry2_bootstrap_failure_closure_schema_id=${retry2_bootstrap_failure_closure_schema_id}
retry2_bootstrap_failure_closure_path=${retry2_bootstrap_failure_closure}
retry2_bootstrap_failure_closure_device=${expected_retry2_bootstrap_failure_device}
retry2_bootstrap_failure_closure_inode=${expected_retry2_bootstrap_failure_closure_inode}
retry2_bootstrap_failure_receipt_path=${retry2_bootstrap_failure_receipt}
retry2_bootstrap_failure_receipt_sha256=${expected_retry2_bootstrap_failure_receipt_sha256}
retry2_bootstrap_failure_regular_inventory_path=${retry2_bootstrap_failure_regular_inventory}
retry2_bootstrap_failure_regular_inventory_sha256=${expected_retry2_bootstrap_failure_regular_inventory_sha256}
retry2_bootstrap_failure_directory_inventory_path=${retry2_bootstrap_failure_directory_inventory}
retry2_bootstrap_failure_directory_inventory_sha256=${expected_retry2_bootstrap_failure_directory_inventory_sha256}
retry2_bootstrap_failure_erratum_path=${retry2_bootstrap_failure_live_erratum}
retry2_bootstrap_failure_erratum_sha256=${expected_retry2_bootstrap_failure_erratum_sha256}
retry2_bootstrap_failure_observation_path=${retry2_bootstrap_failure_live_observation}
retry2_bootstrap_failure_observation_sha256=${expected_retry2_bootstrap_failure_observation_sha256}
retry2_bootstrap_failure_sealer_path=${retry2_bootstrap_failure_live_sealer}
retry2_bootstrap_failure_sealer_sha256=${expected_retry2_bootstrap_failure_sealer_sha256}
retry2_bootstrap_failure_frozen_old_runner_path=${retry2_bootstrap_failure_frozen_old_runner}
retry2_bootstrap_failure_frozen_old_runner_sha256=${expected_retry2_bootstrap_failure_old_runner_sha256}
retry2_bootstrap_failure_frozen_old_amendment_path=${retry2_bootstrap_failure_frozen_old_amendment}
retry2_bootstrap_failure_frozen_old_amendment_sha256=${expected_retry2_bootstrap_failure_old_amendment_sha256}
retry2_bootstrap_failure_frozen_erratum_path=${retry2_bootstrap_failure_frozen_erratum}
retry2_bootstrap_failure_frozen_erratum_sha256=${expected_retry2_bootstrap_failure_erratum_sha256}
retry2_bootstrap_failure_frozen_observation_path=${retry2_bootstrap_failure_frozen_observation}
retry2_bootstrap_failure_frozen_observation_sha256=${expected_retry2_bootstrap_failure_observation_sha256}
retry2_bootstrap_failure_frozen_sealer_path=${retry2_bootstrap_failure_frozen_sealer}
retry2_bootstrap_failure_frozen_sealer_sha256=${expected_retry2_bootstrap_failure_sealer_sha256}
retry2_bootstrap_failure_scientific_input=false
retry2_bootstrap_failure_candidate_adopted=false
retry2_bootstrap_failure_scientific_attempt_consumed=false
retry2_bootstrap_failure_optimizer_steps=0
BOOTSTRAP_FAILURE_BINDING
}

verify_retry2_bootstrap_failure_closure_bindings() {
  local receipt="$1"
  verify_retry2_bootstrap_failure_closure_authority
  expect_kv "${receipt}" retry2_bootstrap_failure_closure_schema_id \
    "${retry2_bootstrap_failure_closure_schema_id}"
  expect_kv "${receipt}" retry2_bootstrap_failure_closure_path \
    "${retry2_bootstrap_failure_closure}"
  expect_kv "${receipt}" retry2_bootstrap_failure_closure_device \
    "${expected_retry2_bootstrap_failure_device}"
  expect_kv "${receipt}" retry2_bootstrap_failure_closure_inode \
    "${expected_retry2_bootstrap_failure_closure_inode}"
  expect_kv "${receipt}" retry2_bootstrap_failure_receipt_path \
    "${retry2_bootstrap_failure_receipt}"
  expect_kv "${receipt}" retry2_bootstrap_failure_receipt_sha256 \
    "${expected_retry2_bootstrap_failure_receipt_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_regular_inventory_path \
    "${retry2_bootstrap_failure_regular_inventory}"
  expect_kv "${receipt}" retry2_bootstrap_failure_regular_inventory_sha256 \
    "${expected_retry2_bootstrap_failure_regular_inventory_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_directory_inventory_path \
    "${retry2_bootstrap_failure_directory_inventory}"
  expect_kv "${receipt}" retry2_bootstrap_failure_directory_inventory_sha256 \
    "${expected_retry2_bootstrap_failure_directory_inventory_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_erratum_path \
    "${retry2_bootstrap_failure_live_erratum}"
  expect_kv "${receipt}" retry2_bootstrap_failure_erratum_sha256 \
    "${expected_retry2_bootstrap_failure_erratum_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_observation_path \
    "${retry2_bootstrap_failure_live_observation}"
  expect_kv "${receipt}" retry2_bootstrap_failure_observation_sha256 \
    "${expected_retry2_bootstrap_failure_observation_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_sealer_path \
    "${retry2_bootstrap_failure_live_sealer}"
  expect_kv "${receipt}" retry2_bootstrap_failure_sealer_sha256 \
    "${expected_retry2_bootstrap_failure_sealer_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_old_runner_path \
    "${retry2_bootstrap_failure_frozen_old_runner}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_old_runner_sha256 \
    "${expected_retry2_bootstrap_failure_old_runner_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_old_amendment_path \
    "${retry2_bootstrap_failure_frozen_old_amendment}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_old_amendment_sha256 \
    "${expected_retry2_bootstrap_failure_old_amendment_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_erratum_path \
    "${retry2_bootstrap_failure_frozen_erratum}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_erratum_sha256 \
    "${expected_retry2_bootstrap_failure_erratum_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_observation_path \
    "${retry2_bootstrap_failure_frozen_observation}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_observation_sha256 \
    "${expected_retry2_bootstrap_failure_observation_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_sealer_path \
    "${retry2_bootstrap_failure_frozen_sealer}"
  expect_kv "${receipt}" retry2_bootstrap_failure_frozen_sealer_sha256 \
    "${expected_retry2_bootstrap_failure_sealer_sha256}"
  expect_kv "${receipt}" retry2_bootstrap_failure_scientific_input false
  expect_kv "${receipt}" retry2_bootstrap_failure_candidate_adopted false
  expect_kv "${receipt}" retry2_bootstrap_failure_scientific_attempt_consumed false
  expect_kv "${receipt}" retry2_bootstrap_failure_optimizer_steps 0
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
  expect_kv "${failure_closure_receipt}" retry_schema_id "${retry1_schema_id}"
  expect_kv "${failure_closure_receipt}" retry_runtime_root "${retry1_runtime}"
  expect_kv "${failure_closure_receipt}" retry_restart_from_step_zero true
  expect_kv "${failure_closure_receipt}" \
    retry_requires_explicit_mtf_net_bnf_path true
  expect_kv "${failure_closure_receipt}" \
    retry_requires_effective_grammar_closure true
}

verify_retry1_interruption_authority() {
  verify_pinned_mode_file "${retry1_interruption_closure_receipt}" \
    "${expected_retry1_interruption_closure_receipt_sha256}" 444 \
    "retry1 operational interruption closure receipt"
  verify_pinned_mode_file "${retry1_interruption_regular_inventory}" \
    "${expected_retry1_interruption_regular_inventory_sha256}" 444 \
    "retry1 interruption regular-file inventory"
  verify_pinned_mode_file "${retry1_interruption_directory_inventory}" \
    "${expected_retry1_interruption_directory_inventory_sha256}" 444 \
    "retry1 interruption directory inventory"
  verify_pinned_mode_file "${retry1_interruption_frozen_amendment}" \
    "${expected_retry1_interruption_amendment_sha256}" 444 \
    "frozen retry1 interruption amendment"
  verify_pinned_mode_file "${retry1_interruption_frozen_sealer}" \
    "${expected_retry1_interruption_sealer_sha256}" 444 \
    "frozen retry1 interruption sealer"
  verify_pinned_mode_file "${retry1_interruption_live_amendment}" \
    "${expected_retry1_interruption_amendment_sha256}" 444 \
    "live retry1 interruption amendment"
  verify_pinned_mode_file "${retry1_interruption_live_sealer}" \
    "${expected_retry1_interruption_sealer_sha256}" 555 \
    "live retry1 interruption sealer"
  verify_pinned_mode_file "${retry1_live_runner}" \
    "${expected_retry1_runner_sha256}" 555 "retry1 ablation runner"

  expect_kv "${retry1_interruption_closure_receipt}" schema_id \
    "${retry1_interruption_closure_schema_id}"
  expect_kv "${retry1_interruption_closure_receipt}" status complete
  expect_kv "${retry1_interruption_closure_receipt}" \
    closure_kind external_operational_interruption
  expect_kv "${retry1_interruption_closure_receipt}" \
    evidence_class operational_interruption_boundary
  expect_kv "${retry1_interruption_closure_receipt}" scientific_evidence false
  expect_kv "${retry1_interruption_closure_receipt}" \
    metric_evidence_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    source_runtime_root "${retry1_runtime}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    source_runtime_mutated false
  expect_kv "${retry1_interruption_closure_receipt}" \
    regular_file_inventory_path "${retry1_interruption_regular_inventory}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    regular_file_inventory_sha256 \
    "${expected_retry1_interruption_regular_inventory_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    directory_inventory_path "${retry1_interruption_directory_inventory}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    directory_inventory_sha256 \
    "${expected_retry1_interruption_directory_inventory_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    content_inventory_sha256 "${expected_retry1_runtime_content_inventory_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" source_regular_file_count 49
  expect_kv "${retry1_interruption_closure_receipt}" source_regular_file_bytes 64939302
  expect_kv "${retry1_interruption_closure_receipt}" source_directory_count 25
  expect_kv "${retry1_interruption_closure_receipt}" source_symlink_count 0
  expect_kv "${retry1_interruption_closure_receipt}" source_special_entry_count 0
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry1_runner_sha256 "${expected_retry1_runner_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" endpoint_arm_status complete
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_training_status_sha256 \
    "${expected_retry1_endpoint_training_status_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_runtime_result_sha256 \
    "${expected_retry1_endpoint_runtime_result_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_report_sha256 "${expected_retry1_endpoint_report_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_manifest_sha256 "${expected_retry1_endpoint_manifest_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_checkpoint_sha256 "${expected_retry1_endpoint_checkpoint_sha256}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_completed_optimizer_steps 3000
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_direct_use_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_import_eligibility requires_separate_retry2_import_verifier
  expect_kv "${retry1_interruption_closure_receipt}" \
    endpoint_import_hardlink_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    time_only_arm_status incomplete
  expect_kv "${retry1_interruption_closure_receipt}" \
    time_only_steps_completed_observed 2880
  expect_kv "${retry1_interruption_closure_receipt}" \
    time_only_training_status_present false
  expect_kv "${retry1_interruption_closure_receipt}" \
    time_only_runtime_result_present false
  expect_kv "${retry1_interruption_closure_receipt}" \
    time_only_partial_artifact_reuse_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    no_tf_alignment_arm_status not_started
  expect_kv "${retry1_interruption_closure_receipt}" \
    partial_artifact_reuse_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" resume_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_schema_id "${retry2_schema_id}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_runtime_root "${retry2_runtime}"
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_time_only_restart_optimizer_step 0
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_no_tf_alignment_restart_optimizer_step 0
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_endpoint_import_requires_separate_verifier true
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_endpoint_import_requires_byte_identical_copy true
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_endpoint_import_requires_distinct_source_copy_identity true
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_endpoint_import_hardlink_authorized false
  expect_kv "${retry1_interruption_closure_receipt}" \
    retry2_endpoint_consumption retry2_local_import_copy_only
  expect_kv "${retry1_interruption_closure_receipt}" \
    certified_input_access false
  expect_kv "${retry1_interruption_closure_receipt}" final_holdout_access false

}

emit_ablation_runner_bindings() {
  assert_operational_runner_identity
  verify_retry2_windows_safe_publication_authority_v2
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
retry2_windows_safe_publication_authority_v1_path=${retry2_windows_safe_publication_authority}
retry2_windows_safe_publication_authority_v1_sha256=${expected_retry2_windows_safe_publication_authority_sha256}
retry2_tmp_scan_race_observation_path=${retry2_tmp_scan_race_observation}
retry2_tmp_scan_race_observation_sha256=${expected_retry2_tmp_scan_race_observation_sha256}
retry2_windows_safe_publication_authority_v2_path=${retry2_windows_safe_publication_authority_v2}
retry2_windows_safe_publication_authority_v2_sha256=$(sha256_of "${retry2_windows_safe_publication_authority_v2}")
RUNNER_BINDING
  emit_retry2_bootstrap_failure_closure_bindings
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
  verify_retry2_windows_safe_publication_authority_v2
  expect_kv "${receipt}" retry2_windows_safe_publication_authority_v1_path \
    "${retry2_windows_safe_publication_authority}"
  expect_kv "${receipt}" retry2_windows_safe_publication_authority_v1_sha256 \
    "${expected_retry2_windows_safe_publication_authority_sha256}"
  expect_kv "${receipt}" retry2_tmp_scan_race_observation_path \
    "${retry2_tmp_scan_race_observation}"
  expect_kv "${receipt}" retry2_tmp_scan_race_observation_sha256 \
    "${expected_retry2_tmp_scan_race_observation_sha256}"
  expect_kv "${receipt}" retry2_windows_safe_publication_authority_v2_path \
    "${retry2_windows_safe_publication_authority_v2}"
  expect_kv "${receipt}" retry2_windows_safe_publication_authority_v2_sha256 \
    "$(sha256_of "${retry2_windows_safe_publication_authority_v2}")"
  verify_retry2_bootstrap_failure_closure_bindings "${receipt}"
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
}

verify_static_inputs() {
  local command_name
  [[ "${train_rows}" == 22464 && "${validation_rows}" == 2304 && \
    "${certified_rows}" == 3429 ]] ||
    fail "fixed probe-row arithmetic drifted"
  for command_name in awk basename bash cat chmod cmp cp df dirname env find flock grep id ln \
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
  require_not_writable "${isolated_source_root}" "isolated source root"
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
  local grep_status
  require_nonempty_file "${path}"
  if LC_ALL=C grep -Fq 'data/raw' "${path}"; then
    fail "canonical data/raw path is forbidden: ${path}"
  else
    grep_status=$?
  fi
  [[ "${grep_status}" == 1 ]] ||
    fail "could not scan for canonical data/raw path: ${path}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.base_training_derivation.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.base_training_verify.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.base_training_config.XXXXXX")"
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
  done <<<"${receipts//;;/$'\n'}"
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
  require_not_writable "${canonical_affine_development}" \
    "canonical affine development runtime"
  for manifest in input_manifest.sha256 source_manifest.sha256 binary.sha256 \
    output_manifest.sha256 master.sha256; do
    require_immutable_file "${canonical_affine_development}/${manifest}"
    (
      cd "${canonical_affine_development}" &&
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
    path_is_absent "${job_root}/channel_policy.report" ||
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
    path_is_absent "${job_root}/channel_policy.report" ||
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
  path_is_absent "${canonical_capture_result}" ||
    fail "canonical certified capture exists despite the ablation route"
  path_is_absent "${canonical_affine_final}" ||
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
  verify_retry2_amendment_authority
  verify_retry2_bootstrap_failure_closure_authority
  verify_retry2_windows_safe_publication_authority_v2
  verify_retry1_interruption_authority
  verify_endpoint_bundle_authority_static
  verify_recovery_authority
  verify_read_only_preflight_inputs
  verify_mdn_retry1_authority

  # canonical_inputs transitively verifies the exact capture receipt and the
  # sealed representation_ablation_screen route without creating artifacts.
  canonical_inputs
  assert_operational_runner_identity
}

freeze_sources() {
  assert_operational_runner_identity
  canonical_inputs
  assert_operational_runner_identity
  if ! path_is_absent "${frozen_root}"; then
    verify_frozen_sources
    assert_operational_runner_identity
    return
  fi
  local candidate
  assert_operational_runner_identity
  candidate="$(mktemp -d "${scratch_root}/${schema_id}.frozen_sources.XXXXXXXX")"
  assert_operational_runner_identity
  cp -- "${script_path}" \
    "${candidate}/run_representation_ablation_v2_retry3.sh"
  cp -- "${affine_helper_source}" \
    "${candidate}/frozen_representation_affine_probe.cpp"
  cp -- "${affine_binary_source}" \
    "${candidate}/frozen_representation_affine_probe"
  assert_operational_runner_identity
  chmod 0444 "${candidate}/run_representation_ablation_v2_retry3.sh" \
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

  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.policy.XXXXXX")"
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

  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.net.XXXXXX")"
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

  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.train_config.XXXXXX")"
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

  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.capture_config.XXXXXX")"
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

  reverse="$(mktemp "${scratch_root}/${schema_id}.reverse.XXXXXX")"
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

  reverse="$(mktemp "${scratch_root}/${schema_id}.config_reverse.XXXXXX")"
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

  reverse="$(mktemp "${scratch_root}/${schema_id}.capture_config_reverse.XXXXXX")"
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
  local config="$1" raw="$2" resolved config_dir
  [[ -n "${raw}" ]] || fail "empty authored configuration path in ${config}"
  if [[ "${raw}" == /* ]]; then
    resolved="$(realpath -m -- "${raw}")" ||
      fail "could not resolve authored configuration path: ${raw}"
  else
    config_dir="$(dirname "${config}")" ||
      fail "could not resolve configuration directory: ${config}"
    resolved="$(realpath -m -- "${config_dir}/${raw}")" ||
      fail "could not resolve relative configuration path: ${raw}"
  fi
  [[ "${resolved}" == /* ]] ||
    fail "resolved configuration path is not absolute: ${resolved}"
  require_nonempty_file "${resolved}"
  printf '%s' "${resolved}"
}

default_effective_grammar_path() {
  local data_path="$1" data_filename grammar_filename data_dir resolved
  data_filename="$(basename -- "${data_path}")"
  [[ -n "${data_filename}" ]] ||
    fail "cannot derive grammar for empty data filename: ${data_path}"
  if [[ "${data_filename}" == kikijyeba.protocol.*.dsl && \
    "${data_filename}" != kikijyeba.protocol.dsl ]]; then
    grammar_filename="kikijyeba.protocol.dsl.bnf"
  else
    grammar_filename="${data_filename}.bnf"
  fi
  data_dir="$(dirname "${data_path}")" ||
    fail "could not resolve data-path directory: ${data_path}"
  resolved="$(realpath -m -- \
    "${data_dir}/grammar/${grammar_filename}")" ||
    fail "could not resolve default grammar path for ${data_path}"
  [[ "${resolved}" == /* ]] ||
    fail "resolved default grammar path is not absolute: ${resolved}"
  printf '%s' "${resolved}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.effective_grammar.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.effective_grammar_verify.XXXXXX")"
  emit_effective_grammar_closure "${candidate}"
  cmp -s -- "${candidate}" "${effective_grammar_closure}" || {
    rm -f -- "${candidate}"
    fail "effective grammar closure drifted"
  }
  rm -f -- "${candidate}"
}

emit_config_closure() {
  local destination="$1"
  local paths_file unsorted_paths_file path count=0 index=0 formatted config arm
  paths_file="$(mktemp \
    "${scratch_root}/${schema_id}.config_paths.XXXXXX")" ||
    fail "could not allocate sorted configuration closure inventory"
  unsorted_paths_file="$(mktemp \
    "${scratch_root}/${schema_id}.config_paths_unsorted.XXXXXX")" || {
    rm -f -- "${paths_file}"
    fail "could not allocate unsorted configuration closure inventory"
  }
  printf '%s\n' "${base_training_config}" "${capture_config}" \
      "${canonical_untrained_capture_config}" \
      "${canonical_untrained_mdn_policy}" \
      "${canonical_policy}" "${canonical_net}" "${canonical_mtf_net_bnf}" \
      "${effective_grammar_closure}" \
      "${retry2_stage04_interruption_closure_receipt}" \
      "${retry2_stage04_interruption_regular_inventory}" \
      "${retry2_stage04_interruption_directory_inventory}" \
      "${retry2_stage04_interruption_live_amendment}" \
      "${retry2_stage04_interruption_live_sealer}" \
      "${retry2_completed_prefix_bundle_receipt}" \
      "${retry2_completed_prefix_regular_inventory}" \
      "${retry2_completed_prefix_directory_inventory}" \
      "${retry2_completed_prefix_live_sealer}" \
      "${retry2_completed_prefix_frozen_sealer}" \
      "${retry2_completed_prefix_snapshot}/arms/endpoint_scale/config/representation.jkimyei" \
      "${retry2_completed_prefix_snapshot}/arms/endpoint_scale/config/representation.net" \
      "${retry2_completed_prefix_snapshot}/arms/endpoint_scale/config/train.config" \
      "${retry2_completed_prefix_snapshot}/arms/endpoint_scale/config/capture.config" \
      "${retry2_completed_prefix_snapshot}/arms/time_only/config/representation.jkimyei" \
      "${retry2_completed_prefix_snapshot}/arms/time_only/config/representation.net" \
      "${retry2_completed_prefix_snapshot}/arms/time_only/config/train.config" \
      "${retry2_completed_prefix_snapshot}/arms/time_only/config/capture.config" \
      "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/config/representation.jkimyei" \
      "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/config/representation.net" \
      "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/config/train.config" \
      "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/config/capture.config" \
      "${retry2_amendment}" \
      "${retry2_windows_safe_publication_authority}" \
      "${retry2_tmp_scan_race_observation}" \
      "${retry2_windows_safe_publication_authority_v2}" \
      "${retry2_bootstrap_failure_live_erratum}" \
      "${retry2_bootstrap_failure_live_observation}" \
      "${retry2_bootstrap_failure_live_sealer}" \
      "${retry2_bootstrap_failure_receipt}" \
      "${retry2_bootstrap_failure_regular_inventory}" \
      "${retry2_bootstrap_failure_directory_inventory}" \
      "${retry2_bootstrap_failure_frozen_old_runner}" \
      "${retry2_bootstrap_failure_frozen_old_amendment}" \
      "${retry2_bootstrap_failure_frozen_erratum}" \
      "${retry2_bootstrap_failure_frozen_observation}" \
      "${retry2_bootstrap_failure_frozen_sealer}" \
      "${retry1_interruption_closure_receipt}" \
      "${retry1_interruption_regular_inventory}" \
      "${retry1_interruption_directory_inventory}" \
      "${retry1_interruption_frozen_amendment}" \
      "${retry1_interruption_frozen_sealer}" \
      "${retry1_interruption_live_amendment}" \
      "${retry1_interruption_live_sealer}" \
      "${retry1_live_runner}" \
      "${endpoint_bundle_live_sealer}" "${endpoint_bundle_live_amendment}" \
      "${endpoint_bundle_frozen_sealer}" \
      "${endpoint_bundle_frozen_amendment}" \
      "${endpoint_bundle_regular_inventory}" \
      "${endpoint_bundle_directory_inventory}" \
      "${endpoint_bundle_receipt}" "${endpoint_bundle_checkpoint}" \
      "${endpoint_bundle_policy}" "${endpoint_bundle_net}" \
      "${endpoint_bundle_train_config}" "${endpoint_bundle_capture_config}" \
      "${recovery_amendment}" \
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
      "${preregistration}" >"${unsorted_paths_file}" || {
    rm -f -- "${paths_file}" "${unsorted_paths_file}"
    fail "could not emit fixed configuration closure paths"
  }
  for arm in "${challenger_arms[@]}"; do
    printf '%s\n' "$(arm_policy "${arm}")" "$(arm_net "${arm}")" \
      "$(arm_config "${arm}")" "$(arm_capture_config "${arm}")" \
      >>"${unsorted_paths_file}" || {
      rm -f -- "${paths_file}" "${unsorted_paths_file}"
      fail "could not emit challenger configuration closure paths: ${arm}"
    }
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
    ' "${config}" >>"${unsorted_paths_file}" || {
      rm -f -- "${paths_file}" "${unsorted_paths_file}"
      fail "could not extract authored paths from configuration: ${config}"
    }
  done
  LC_ALL=C sort -u -- "${unsorted_paths_file}" >"${paths_file}" || {
    rm -f -- "${paths_file}" "${unsorted_paths_file}"
    fail "could not sort configuration closure paths"
  }
  rm -f -- "${unsorted_paths_file}"
  count="$(wc -l <"${paths_file}")" || {
    rm -f -- "${paths_file}"
    fail "could not count configuration closure paths"
  }
  [[ "${count}" =~ ^[0-9]+$ ]] || {
    rm -f -- "${paths_file}"
    fail "configuration closure path count is malformed"
  }
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
  [[ "${index}" == "${count}" ]] || {
    rm -f -- "${paths_file}"
    fail "configuration closure path traversal was incomplete: ${index}/${count}"
  }
  rm -f -- "${paths_file}"
}

write_config_closure() {
  assert_operational_runner_identity
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.config_inputs.XXXXXX")"
  emit_config_closure "${candidate}"
  publish_immutable "${candidate}" "${config_closure}"
  assert_operational_runner_identity
}

verify_config_closure() {
  require_nonempty_file "${config_closure}"
  expect_kv "${config_closure}" schema_id "${schema_id}.config_inputs.v1"
  expect_kv "${config_closure}" status complete
  local candidate count index formatted path
  candidate="$(mktemp "${scratch_root}/${schema_id}.config_verify.XXXXXX")"
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
    echo "retry2_staged_recovery_amendment_path=${retry2_amendment}"
    echo "retry2_staged_recovery_amendment_sha256=$(sha256_of "${retry2_amendment}")"
    echo "retry1_interruption_closure_receipt_path=${retry1_interruption_closure_receipt}"
    echo "retry1_interruption_closure_receipt_sha256=$(sha256_of "${retry1_interruption_closure_receipt}")"
    echo "retry1_interruption_regular_inventory_path=${retry1_interruption_regular_inventory}"
    echo "retry1_interruption_regular_inventory_sha256=$(sha256_of "${retry1_interruption_regular_inventory}")"
    echo "retry1_interruption_directory_inventory_path=${retry1_interruption_directory_inventory}"
    echo "retry1_interruption_directory_inventory_sha256=$(sha256_of "${retry1_interruption_directory_inventory}")"
    echo "retry1_runtime_content_inventory_sha256=${expected_retry1_runtime_content_inventory_sha256}"
    echo "endpoint_bundle_sealer_path=${endpoint_bundle_live_sealer}"
    echo "endpoint_bundle_sealer_sha256=$(sha256_of "${endpoint_bundle_live_sealer}")"
    echo "endpoint_bundle_amendment_path=${endpoint_bundle_live_amendment}"
    echo "endpoint_bundle_amendment_sha256=$(sha256_of "${endpoint_bundle_live_amendment}")"
    echo "endpoint_bundle_receipt_path=${endpoint_bundle_receipt}"
    echo "endpoint_bundle_receipt_sha256=$(sha256_of "${endpoint_bundle_receipt}")"
    echo "endpoint_bundle_checkpoint_path=${endpoint_bundle_checkpoint}"
    echo "endpoint_bundle_checkpoint_sha256=$(sha256_of "${endpoint_bundle_checkpoint}")"
    echo "endpoint_bundle_policy_path=${endpoint_bundle_policy}"
    echo "endpoint_bundle_policy_sha256=$(sha256_of "${endpoint_bundle_policy}")"
    echo "endpoint_bundle_net_path=${endpoint_bundle_net}"
    echo "endpoint_bundle_net_sha256=$(sha256_of "${endpoint_bundle_net}")"
    echo "endpoint_bundle_train_config_path=${endpoint_bundle_train_config}"
    echo "endpoint_bundle_train_config_sha256=$(sha256_of "${endpoint_bundle_train_config}")"
    echo "endpoint_bundle_capture_config_path=${endpoint_bundle_capture_config}"
    echo "endpoint_bundle_capture_config_sha256=$(sha256_of "${endpoint_bundle_capture_config}")"
    echo "endpoint_direct_retry1_use_authorized=false"
    echo "retry2_completed_prefix_bundle_required=true"
    echo "endpoint_retry3_local_import_required=true"
    echo "time_only_retry3_local_import_required=true"
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
    echo "endpoint_scale_retry3_optimizer_steps=0"
    echo "time_only_historical_optimizer_steps=3000"
    echo "time_only_retry3_optimizer_steps=0"
    echo "no_tf_alignment_retry3_optimizer_start_step=0"
    echo "no_tf_alignment_retry3_optimizer_steps=${expected_steps}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.inputs.XXXXXX")"
  emit_inputs "${candidate}"
  publish_immutable "${candidate}" "${input_receipt}"
  assert_operational_runner_identity
}

verify_inputs() {
  require_nonempty_file "${input_receipt}"
  expect_kv "${input_receipt}" retry2_staged_recovery_amendment_path \
    "${retry2_amendment}"
  expect_kv "${input_receipt}" retry2_staged_recovery_amendment_sha256 \
    "${expected_retry2_amendment_sha256}"
  expect_kv "${input_receipt}" retry1_interruption_closure_receipt_path \
    "${retry1_interruption_closure_receipt}"
  expect_kv "${input_receipt}" retry1_interruption_closure_receipt_sha256 \
    "${expected_retry1_interruption_closure_receipt_sha256}"
  expect_kv "${input_receipt}" retry1_runtime_content_inventory_sha256 \
    "${expected_retry1_runtime_content_inventory_sha256}"
  expect_kv "${input_receipt}" endpoint_bundle_sealer_path \
    "${endpoint_bundle_live_sealer}"
  expect_kv "${input_receipt}" endpoint_bundle_sealer_sha256 \
    "${expected_endpoint_bundle_sealer_sha256}"
  expect_kv "${input_receipt}" endpoint_bundle_amendment_path \
    "${endpoint_bundle_live_amendment}"
  expect_kv "${input_receipt}" endpoint_bundle_amendment_sha256 \
    "${expected_endpoint_bundle_amendment_sha256}"
  expect_kv "${input_receipt}" endpoint_bundle_receipt_path \
    "${endpoint_bundle_receipt}"
  expect_kv "${input_receipt}" endpoint_bundle_receipt_sha256 \
    "${expected_endpoint_bundle_receipt_sha256}"
  expect_kv "${input_receipt}" endpoint_bundle_checkpoint_sha256 \
    "${expected_endpoint_bundle_checkpoint_sha256}"
  expect_kv "${input_receipt}" endpoint_direct_retry1_use_authorized false
  expect_kv "${input_receipt}" retry2_live_runtime_direct_use_authorized false
  expect_kv "${input_receipt}" retry2_completed_prefix_bundle_required true
  expect_kv "${input_receipt}" endpoint_retry3_local_import_required true
  expect_kv "${input_receipt}" time_only_retry3_local_import_required true
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.inputs_verify.XXXXXX")"
  emit_inputs "${candidate}"
  cmp -s -- "${candidate}" "${input_receipt}" || {
    rm -f -- "${candidate}"
    fail "ablation input receipt drifted"
  }
  rm -f -- "${candidate}"
  expect_kv "${input_receipt}" required_route representation_ablation_screen
  expect_kv "${input_receipt}" challenger_count 3
  expect_kv "${input_receipt}" challenger_seed 17
  expect_kv "${input_receipt}" endpoint_scale_retry3_optimizer_steps 0
  expect_kv "${input_receipt}" time_only_historical_optimizer_steps 3000
  expect_kv "${input_receipt}" time_only_retry3_optimizer_steps 0
  expect_kv "${input_receipt}" no_tf_alignment_retry3_optimizer_start_step 0
  expect_kv "${input_receipt}" no_tf_alignment_retry3_optimizer_steps \
    "${expected_steps}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.attempt_verify.XXXXXX")"
  emit_retry_attempt_sentinel "${candidate}"
  cmp -s -- "${candidate}" "${retry_attempt_sentinel}" || {
    rm -f -- "${candidate}"
    fail "immutable retry1 development attempt sentinel drifted"
  }
  rm -f -- "${candidate}"
}

publish_retry_attempt_sentinel_once() {
  assert_operational_runner_identity
  local preexisting_manifest
  path_is_absent "${retry_attempt_sentinel}" ||
    fail "retry1 development attempt is already consumed"
  path_is_absent "${canonical_import_receipt}" ||
    fail "canonical import predates the retry1 attempt sentinel"
  preexisting_manifest="$(find "${arms_root}" -type f -name job.manifest \
    -print -quit)" || fail "could not scan for pre-attempt Runtime jobs"
  [[ -z "${preexisting_manifest}" ]] ||
    fail "Runtime job predates the retry1 attempt sentinel: ${preexisting_manifest}"
  verify_effective_grammar_closure
  verify_config_closure
  verify_inputs
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.development_retry_attempt.XXXXXX")"
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
  path_is_absent "${job}/channel_inference.report" ||
    fail "representation-only training produced a forecast report for ${arm}"
  path_is_absent "${job}/channel_policy.report" ||
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.training_status.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.training_verify.XXXXXX")"
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
  local training_root job checkpoint log
  training_root="$(arm_root "${arm}")/training"
  job="$(arm_train_job "${arm}")"
  log="$(arm_root "${arm}")/training.log"
  reject_symlink_components "${job}"
  reject_symlink_components "$(arm_training_status "${arm}")"
  reject_symlink_components "${log}"
  if ! path_is_absent "${job}"; then
    ! path_is_absent "$(arm_training_status "${arm}")" ||
      fail "partial training job exists without status for ${arm}"
    verify_training_status "${arm}" >/dev/null
    assert_operational_runner_identity
    return
  fi
  path_is_absent "$(arm_training_status "${arm}")" ||
    fail "training status exists without a job for ${arm}"
  path_is_absent "${log}" ||
    fail "training log predates the ${arm} training attempt"
  path_is_absent "${training_root}" ||
    fail "training directory predates the ${arm} training attempt"
  mkdir -- "${training_root}"
  run_guarded_child "${runtime_exec}" \
    --config "$(arm_config "${arm}")" \
    --job-dir "${job}" \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "${arm} training failed; partial job is terminal; see ${log}"
  checkpoint="$(validate_training_job "${arm}")"
  local child_output
  for child_output in "${checkpoint}" "${job}/job.manifest" \
    "${job}/runtime.result.fact" "${job}/channel_representation.report" \
    "${log}"; do
    require_owned_single_link_file "${child_output}" "training child output"
  done
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
  path_is_absent "${job}/channel_policy.report" ||
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
    require_owned_single_link_file "${path}" "capture child output"
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
$(emit_arm_checkpoint_authority_binding "${arm}")
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.capture_status.XXXXXX")"
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
  checkpoint="$(verify_arm_checkpoint_authority "${arm}")"
  config="$(arm_capture_config "${arm}")"
  train_probe="$(validate_capture_job "$(arm_capture_job "${arm}" train)" \
    "${train_begin}" "${train_end}" "${train_rows}" "${checkpoint}" \
    "${config}")"
  validation_probe="$(validate_capture_job \
    "$(arm_capture_job "${arm}" validation)" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${checkpoint}" "${config}")"
  verify_capture_job_immutable "$(arm_capture_job "${arm}" train)"
  verify_capture_job_immutable "$(arm_capture_job "${arm}" validation)"
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.capture_verify.XXXXXX")"
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
  local status checkpoint config capture_root train_job validation_job train_probe
  local validation_probe log
  status="$(arm_capture_status "${arm}")"
  capture_root="$(arm_root "${arm}")/capture"
  train_job="$(arm_capture_job "${arm}" train)"
  validation_job="$(arm_capture_job "${arm}" validation)"
  if ! path_is_absent "${status}"; then
    verify_capture_status "${arm}"
    assert_operational_runner_identity
    return
  fi
  path_is_absent "${train_job}" && \
    path_is_absent "${validation_job}" ||
    fail "partial capture jobs exist without immutable status for ${arm}"
  path_is_absent "${capture_root}" ||
    fail "partial capture directory exists without immutable status for ${arm}"
  checkpoint="$(verify_arm_checkpoint_authority "${arm}")"
  config="$(arm_capture_config "${arm}")"
  mkdir -- "${capture_root}"
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
  path_is_absent "${validation_job}" ||
    fail "${arm} train capture created the validation job path"
  path_is_absent "${log}" ||
    fail "${arm} train capture created the validation log path"
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
  require_dir "$(arm_root canonical)"
  require_dir "$(arm_root canonical)/affine"
  [[ "$(stat -c '%a:%u' -- "$(arm_root canonical)")" == \
    "700:${process_owner_uid}" ]] ||
    fail "canonical arm root metadata drifted"
  [[ "$(stat -c '%a:%u' -- "$(arm_root canonical)/affine")" == \
    "700:${process_owner_uid}" ]] ||
    fail "canonical affine directory metadata drifted"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_import_verify.XXXXXX")"
  emit_canonical_import "${candidate}"
  cmp -s -- "${candidate}" "${canonical_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "canonical import receipt drifted"
  }
  rm -f -- "${candidate}"
}

import_canonical_arm() {
  assert_operational_runner_identity
  local canonical_root affine_dir candidate
  canonical_root="$(arm_root canonical)"
  affine_dir="${canonical_root}/affine"
  if ! path_is_absent "${canonical_import_receipt}"; then
    verify_canonical_import
    assert_operational_runner_identity
    return
  fi
  path_is_absent "$(arm_main_report canonical)" && \
    path_is_absent "$(arm_replay_report canonical)" ||
    fail "partial canonical import exists without immutable receipt"
  path_is_absent "${canonical_root}" ||
    fail "partial canonical arm root exists without immutable receipt"
  path_is_absent "${affine_dir}" ||
    fail "partial canonical import directory exists without immutable receipt"
  mkdir -- "${canonical_root}"
  mkdir -- "${affine_dir}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_main.XXXXXX")"
  cp -- "${canonical_report}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_main_report canonical)"
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_replay.XXXXXX")"
  cp -- "${canonical_replay_report}" "${candidate}"
  publish_immutable "${candidate}" "$(arm_replay_report canonical)"
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_import.XXXXXX")"
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
  require_not_writable "$(arm_main_log "${arm}")" \
    "affine main stdout log for ${arm}"
  require_not_writable "$(arm_replay_log "${arm}")" \
    "affine replay stdout log for ${arm}"
  cmp -s -- "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" ||
    fail "development affine main/replay reports differ for ${arm}"
  cmp -s -- "$(arm_main_log "${arm}")" "$(arm_replay_log "${arm}")" ||
    fail "development affine main/replay logs differ for ${arm}"
  validate_development_report "$(arm_main_report "${arm}")"
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.affine_verify.XXXXXX")"
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
  affine_dir="$(arm_root "${arm}")/affine"
  if ! path_is_absent "${status}"; then
    verify_affine_status "${arm}"
    assert_operational_runner_identity
    return
  fi
  for artifact in "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" "$(arm_main_log "${arm}")" \
    "$(arm_replay_log "${arm}")"; do
    path_is_absent "${artifact}" ||
      fail "partial affine evaluation exists without status for ${arm}: ${artifact}"
  done
  path_is_absent "${affine_dir}" ||
    fail "partial affine directory exists without status for ${arm}"
  verify_capture_status "${arm}"
  train_probe="$(kv train_probe_path "$(arm_capture_status "${arm}")")"
  validation_probe="$(kv validation_probe_path \
    "$(arm_capture_status "${arm}")")"
  mkdir -- "${affine_dir}"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --development-only \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --output "$(arm_main_report "${arm}")" \
    >"$(arm_main_log "${arm}")" 2>&1 ||
    fail "development affine main execution failed for ${arm}; attempt is terminal"
  path_is_absent "$(arm_replay_report "${arm}")" ||
    fail "development affine main created replay output for ${arm}"
  path_is_absent "$(arm_replay_log "${arm}")" ||
    fail "development affine main created replay log for ${arm}"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --development-only \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --output "$(arm_replay_report "${arm}")" \
    >"$(arm_replay_log "${arm}")" 2>&1 ||
    fail "development affine replay execution failed for ${arm}; attempt is terminal"
  for artifact in "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" "$(arm_main_log "${arm}")" \
    "$(arm_replay_log "${arm}")"; do
    require_owned_single_link_file "${artifact}" "development affine child output"
  done
  chmod 0444 "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" "$(arm_main_log "${arm}")" \
    "$(arm_replay_log "${arm}")"
  validate_development_report "$(arm_main_report "${arm}")"
  cmp -s -- "$(arm_main_report "${arm}")" \
    "$(arm_replay_report "${arm}")" ||
    fail "development affine main/replay reports differ for ${arm}"
  cmp -s -- "$(arm_main_log "${arm}")" "$(arm_replay_log "${arm}")" ||
    fail "development affine main/replay logs differ for ${arm}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.${arm}.affine_status.XXXXXX")"
  emit_affine_status "${arm}" "${candidate}"
  publish_immutable "${candidate}" "${status}"
  assert_operational_runner_identity
}

arm_checkpoint_path() {
  local arm="$1"
  if [[ "${arm}" == canonical ]]; then
    printf '%s' "${canonical_checkpoint}"
  elif [[ "${arm}" == endpoint_scale ]]; then
    verify_endpoint_import >/dev/null
    printf '%s' "${endpoint_import_checkpoint}"
  elif [[ "${arm}" == time_only ]]; then
    verify_time_only_import >/dev/null
    printf '%s' "${time_only_import_checkpoint}"
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
  local candidate_direction incumbent_direction candidate_rank incumbent_rank
  local candidate_correlation incumbent_correlation candidate_rmse incumbent_rmse
  local comparator_status
  candidate_direction="$(kv selected.validation.directional_accuracy \
    "${candidate_report}")" || fail "could not read candidate direction metric"
  incumbent_direction="$(kv selected.validation.directional_accuracy \
    "${incumbent_report}")" || fail "could not read incumbent direction metric"
  candidate_rank="$(kv selected.validation.pairwise_rank_accuracy \
    "${candidate_report}")" || fail "could not read candidate rank metric"
  incumbent_rank="$(kv selected.validation.pairwise_rank_accuracy \
    "${incumbent_report}")" || fail "could not read incumbent rank metric"
  candidate_correlation="$(kv selected.validation.correlation \
    "${candidate_report}")" || fail "could not read candidate correlation metric"
  incumbent_correlation="$(kv selected.validation.correlation \
    "${incumbent_report}")" || fail "could not read incumbent correlation metric"
  candidate_rmse="$(kv selected.validation.rmse \
    "${candidate_report}")" || fail "could not read candidate RMSE metric"
  incumbent_rmse="$(kv selected.validation.rmse \
    "${incumbent_report}")" || fail "could not read incumbent RMSE metric"
  require_number "${candidate_direction}" candidate_direction
  require_number "${incumbent_direction}" incumbent_direction
  require_number "${candidate_rank}" candidate_rank
  require_number "${incumbent_rank}" incumbent_rank
  require_number "${candidate_correlation}" candidate_correlation
  require_number "${incumbent_correlation}" incumbent_correlation
  require_number "${candidate_rmse}" candidate_rmse
  require_number "${incumbent_rmse}" incumbent_rmse
  if LC_ALL=C awk -v tolerance="${tie_tolerance}" \
    -v candidate_direction="${candidate_direction}" \
    -v incumbent_direction="${incumbent_direction}" \
    -v candidate_rank="${candidate_rank}" \
    -v incumbent_rank="${incumbent_rank}" \
    -v candidate_correlation="${candidate_correlation}" \
    -v incumbent_correlation="${incumbent_correlation}" \
    -v candidate_rmse="${candidate_rmse}" \
    -v incumbent_rmse="${incumbent_rmse}" '
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
  '; then
    return 0
  else
    comparator_status=$?
  fi
  [[ "${comparator_status}" == 1 ]] && return 1
  fail "cross-arm comparator execution failed with status ${comparator_status}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.comparator_candidate.XXXXXX")"
  incumbent="$(mktemp "${scratch_root}/${schema_id}.comparator_incumbent.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.selection_verify.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.selection.XXXXXX")"
  emit_selection "${candidate}"
  publish_immutable "${candidate}" "${selection_receipt}"
  verify_selection
  assert_operational_runner_identity
}

emit_development_completed_prefix_bindings() {
  local index tag completion attempt
  echo "development_completed_prefix_count=11"
  for ((index = 0; index < 11; ++index)); do
    tag="$(stage_tag "${index}")"
    completion="$(stage_completion_path "${index}")"
    require_immutable_file "${completion}"
    expect_mode_owner_links "${completion}" 444 \
      "development completed-prefix receipt"
    echo "development_stage_${tag}_name=${development_stage_names[${index}]}"
    echo "development_stage_${tag}_completion_path=${completion}"
    echo "development_stage_${tag}_completion_sha256=$(sha256_of "${completion}")"
  done
  attempt="$(stage_attempt_path 11)"
  require_immutable_file "${attempt}"
  expect_mode_owner_links "${attempt}" 444 "stage-11 attempt receipt"
  echo "development_stage_11_name=${development_stage_names[11]}"
  echo "development_stage_11_attempt_path=${attempt}"
  echo "development_stage_11_attempt_sha256=$(sha256_of "${attempt}")"
  echo "development_stage_11_completion_bound_here=false"
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
    emit_development_completed_prefix_bindings
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
      if [[ "${arm}" == endpoint_scale ]]; then
        echo "arm.${arm}.training_authority_kind=retry2_completed_prefix_endpoint_import"
        echo "arm.${arm}.endpoint_import_status_path=${endpoint_import_receipt}"
        echo "arm.${arm}.endpoint_import_status_sha256=$(sha256_of "${endpoint_import_receipt}")"
        echo "arm.${arm}.retry3_training_job_created=false"
        echo "arm.${arm}.retry3_training_status_created=false"
        echo "arm.${arm}.retry3_optimizer_steps=0"
      elif [[ "${arm}" == time_only ]]; then
        echo "arm.${arm}.training_authority_kind=retry2_completed_prefix_time_only_import"
        echo "arm.${arm}.time_only_import_status_path=${time_only_import_receipt}"
        echo "arm.${arm}.time_only_import_status_sha256=$(sha256_of "${time_only_import_receipt}")"
        echo "arm.${arm}.historical_optimizer_steps=3000"
        echo "arm.${arm}.retry3_training_job_created=false"
        echo "arm.${arm}.retry3_training_status_created=false"
        echo "arm.${arm}.retry3_optimizer_steps=0"
      else
        echo "arm.${arm}.training_authority_kind=fresh_retry3_training_from_optimizer_step_zero"
        echo "arm.${arm}.training_status_path=$(arm_training_status "${arm}")"
        echo "arm.${arm}.training_status_sha256=$(sha256_of "$(arm_training_status "${arm}")")"
        echo "arm.${arm}.retry3_optimizer_start_step=0"
        echo "arm.${arm}.retry3_optimizer_steps=${expected_steps}"
      fi
      echo "arm.${arm}.capture_status_path=$(arm_capture_status "${arm}")"
      echo "arm.${arm}.capture_status_sha256=$(sha256_of "$(arm_capture_status "${arm}")")"
      echo "arm.${arm}.affine_status_path=$(arm_affine_status "${arm}")"
      echo "arm.${arm}.affine_status_sha256=$(sha256_of "$(arm_affine_status "${arm}")")"
    done
    echo "challenger_count=3"
    echo "challenger_seed=17"
    echo "time_only_historical_optimizer_steps=3000"
    echo "time_only_retry3_optimizer_steps=0"
    echo "no_tf_alignment_retry3_optimizer_steps=${expected_steps}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.development.XXXXXX")"
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
  expect_kv "${development_receipt}" development_completed_prefix_count 11
  expect_kv "${development_receipt}" development_stage_11_attempt_path \
    "$(stage_attempt_path 11)"
  expect_kv "${development_receipt}" development_stage_11_attempt_sha256 \
    "$(sha256_of "$(stage_attempt_path 11)")"
  expect_kv "${development_receipt}" \
    development_stage_11_completion_bound_here false
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.development_verify.XXXXXX")"
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
    path_is_absent "${path}" ||
      fail "development-only phase found certified artifact: ${path}"
  done
}

audit_development_job_set() {
  local manifests count=0 manifest end
  manifests="$(find "${arms_root}" -type f -name job.manifest -print)" ||
    fail "could not enumerate development job manifests"
  while IFS= read -r manifest; do
    [[ -n "${manifest}" ]] || continue
    count=$((count + 1))
    end="$(kv resolved_anchor_index_end "${manifest}")"
    [[ "${end}" =~ ^[0-9]+$ && "${end}" -le 2816 ]] ||
      fail "development job escaped the development ranges: ${manifest}"
    reject_data_raw_file "${manifest}"
  done <<<"${manifests}"
  [[ "${count}" == 7 ]] ||
    fail "expected one fresh no-TF training job and six challenger capture jobs, found ${count}"
}

verify_development_core() {
  preflight_read_only
  verify_all_development_stage_chain_receipts
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
    verify_arm_checkpoint_authority "${arm}" >/dev/null
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
  path_is_absent "${retry_attempt_sentinel}" ||
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

emit_full_development_stage_chain_bindings() {
  local index tag completion
  echo "development_stage_completion_count=${development_stage_count}"
  for ((index = 0; index < development_stage_count; ++index)); do
    tag="$(stage_tag "${index}")"
    completion="$(stage_completion_path "${index}")"
    require_immutable_file "${completion}"
    expect_mode_owner_links "${completion}" 444 \
      "certified development-stage completion"
    echo "development_stage_${tag}_completion_path=${completion}"
    echo "development_stage_${tag}_completion_sha256=$(sha256_of "${completion}")"
  done
  completion="$(stage_completion_path "$((development_stage_count - 1))")"
  echo "development_stage_chain_head_path=${completion}"
  echo "development_stage_chain_head_sha256=$(sha256_of "${completion}")"
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
$(emit_full_development_stage_chain_bindings)
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
  expect_mode_owner_links "${certified_attempt}" 444 \
    "certified attempt receipt"
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
  expect_kv "${certified_attempt}" development_stage_completion_count \
    "${development_stage_count}"
  expect_kv "${certified_attempt}" development_stage_chain_head_path \
    "$(stage_completion_path "$((development_stage_count - 1))")"
  expect_kv "${certified_attempt}" development_stage_chain_head_sha256 \
    "$(sha256_of "$(stage_completion_path "$((development_stage_count - 1))")")"
  expect_kv "${certified_attempt}" published_before_certified_input true
  expect_kv "${certified_attempt}" runner_up_retry_authorized false
  expect_kv "${certified_attempt}" canonical_data_raw_access false
  expect_kv "${certified_attempt}" final_holdout_access false
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.attempt_verify.XXXXXX")"
  emit_certified_attempt "${candidate}"
  cmp -s -- "${candidate}" "${certified_attempt}" || {
    rm -f -- "${candidate}"
    fail "certified attempt receipt is incomplete or drifted"
  }
  rm -f -- "${candidate}"
}

publish_certified_attempt_once() {
  assert_runtime_publication_ready
  assert_operational_runner_identity
  path_is_absent "${certified_attempt}" ||
    fail "certified attempt already exists and is consumed"
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.certified_attempt.XXXXXX")"
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
  require_not_writable "${certified_capture_log}" "certified capture log"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.certified_capture_verify.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.certified_capture_status.XXXXXX")"
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
  local lock_selection_lines report_selection_lines
  lock_selection_lines="$(mktemp \
    "${scratch_root}/${schema_id}.certified_lock_selection.XXXXXX")" ||
    fail "could not create certified lock-selection comparison file"
  report_selection_lines="$(mktemp \
    "${scratch_root}/${schema_id}.certified_report_selection.XXXXXX")" || {
    rm -f -- "${lock_selection_lines}"
    fail "could not create certified report-selection comparison file"
  }
  selection_lines "${lock_report}" >"${lock_selection_lines}" || {
    rm -f -- "${lock_selection_lines}" "${report_selection_lines}"
    fail "could not extract the locked development selection"
  }
  selection_lines "${report}" >"${report_selection_lines}" || {
    rm -f -- "${lock_selection_lines}" "${report_selection_lines}"
    fail "could not extract the certified report selection"
  }
  cmp -s -- "${lock_selection_lines}" "${report_selection_lines}" || {
    rm -f -- "${lock_selection_lines}" "${report_selection_lines}"
    fail "runner-side certified selection comparison failed"
  }
  rm -f -- "${lock_selection_lines}" "${report_selection_lines}"
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
  local main_dir replay_dir artifact
  selected="$(kv selected_arm "${selection_receipt}")"
  train_probe="$(arm_train_probe_path "${selected}")"
  validation_probe="$(arm_validation_probe_path "${selected}")"
  certified_probe="${certified_job}/representation_edge_features.probe"
  lock_report="$(arm_main_report "${selected}")"
  main_dir="$(dirname "${certified_main_report}")"
  replay_dir="$(dirname "${certified_replay_report}")"
  path_is_absent "${certified_main_report}" && \
    path_is_absent "${certified_replay_report}" && \
    path_is_absent "${certified_main_log}" && \
    path_is_absent "${certified_replay_log}" ||
    fail "partial certified affine reports already exist; attempt is consumed"
  path_is_absent "${main_dir}" && path_is_absent "${replay_dir}" ||
    fail "certified capture created an affine output directory"
  mkdir -- "${main_dir}" "${replay_dir}"
  [[ "$(stat -c '%a:%u' -- "${main_dir}")" == \
    "700:${process_owner_uid}" && \
    "$(stat -c '%a:%u' -- "${replay_dir}")" == \
    "700:${process_owner_uid}" ]] ||
    fail "certified affine output directory metadata drifted"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --certified-input "${certified_probe}" \
    --selection-lock "${lock_report}" \
    --output "${certified_main_report}" >"${certified_main_log}" 2>&1 ||
    fail "certified affine main execution failed; attempt is permanently consumed"
  path_is_absent "${certified_replay_report}" ||
    fail "certified affine main created the replay output path"
  path_is_absent "${certified_replay_log}" ||
    fail "certified affine main created the replay log path"
  run_guarded_child env OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 \
    "${frozen_binary}" \
    --probe-kind representation \
    --train-input "${train_probe}" \
    --validation-input "${validation_probe}" \
    --certified-input "${certified_probe}" \
    --selection-lock "${lock_report}" \
    --output "${certified_replay_report}" >"${certified_replay_log}" 2>&1 ||
    fail "certified affine replay execution failed; attempt is permanently consumed"
  for artifact in "${certified_main_report}" "${certified_replay_report}" \
    "${certified_main_log}" "${certified_replay_log}"; do
    require_owned_single_link_file "${artifact}" "certified affine child output"
  done
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
    path_is_absent "$(arm_root "${arm}")/certified" ||
      fail "arm-local certified artifact is forbidden for ${arm}"
  done
  expect_kv "${certified_attempt}" selected_arm "${selected}"
  expect_kv "${certified_capture_status}" selected_arm "${selected}"
}

audit_complete_job_set() {
  local manifests count=0 manifest end
  manifests="$(find "${runtime_root}" -type f -name job.manifest -print)" ||
    fail "could not enumerate complete job manifests"
  while IFS= read -r manifest; do
    [[ -n "${manifest}" ]] || continue
    count=$((count + 1))
    end="$(kv resolved_anchor_index_end "${manifest}")"
    [[ "${end}" =~ ^[0-9]+$ && "${end}" -le 3261 ]] ||
      fail "job manifest crossed the isolated development prefix: ${manifest}"
    reject_data_raw_file "${manifest}"
  done <<<"${manifests}"
  [[ "${count}" == 8 ]] ||
    fail "expected seven development jobs and one certified job, found ${count}"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.result_verify.XXXXXX")"
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
  candidate="$(mktemp "${scratch_root}/${schema_id}.result.XXXXXX")"
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
  require_not_writable "${certified_main_log}" "certified main log"
  require_not_writable "${certified_replay_log}" "certified replay log"
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
  path_is_absent "${canonical_capture_result}" ||
    fail "canonical certified result appeared after ablation routing"
  path_is_absent "${canonical_affine_final}" ||
    fail "canonical affine certified result appeared after ablation routing"
  verify_result_receipt
}

verify_certified_lock_metadata() {
  local certified_lock="$1"
  require_file "${certified_lock}"
  [[ "$(stat -c '%a:%u:%h:%s' -- "${certified_lock}")" == \
    "600:${process_owner_uid}:1:0" ]] ||
    fail "certified lock metadata drifted"
}

run_certified() {
  assert_runtime_publication_ready
  assert_runner_bootstrap_lock
  resource_safety_gate
  preflight_read_only
  assert_operational_runner_identity
  verify_development_core
  assert_operational_runner_identity
  local certified_lock="${runtime_root}/.certified.lock"
  if ! path_is_absent "${certified_lock}"; then
    require_file "${certified_lock}"
    [[ "$(stat -c '%h' -- "${certified_lock}")" == 1 ]] ||
      fail "certified lock has an external hard link"
    exec {certified_lock_fd}<>"${certified_lock}"
  else
    exec {certified_lock_fd}>"${certified_lock}"
    chmod 0600 -- "${certified_lock}"
  fi
  verify_certified_lock_metadata "${certified_lock}"
  flock -n "${certified_lock_fd}" ||
    fail "another certified ablation process holds the lock"
  assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
    "certified ablation lock"
  assert_operational_runner_identity
  verify_development_core
  if ! path_is_absent "${result_receipt}"; then
    verify_complete
    resource_safety_gate
    verify_runtime_root_and_lock_metadata
    verify_certified_lock_metadata "${certified_lock}"
    assert_fd_matches_path "${development_lock_fd}" \
      "${runtime_development_lock}" "retry2 development lock"
    assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
      "certified ablation lock"
    return
  fi
  if ! path_is_absent "${certified_attempt}" || \
    ! path_is_absent "${runtime_root}/certified" || \
    ! path_is_absent "${certified_job}" || \
    ! path_is_absent "${certified_capture_status}" || \
    ! path_is_absent "${certified_capture_log}" || \
    ! path_is_absent "${certified_main_report}" || \
    ! path_is_absent "${certified_replay_report}" || \
    ! path_is_absent "${certified_main_log}" || \
    ! path_is_absent "${certified_replay_log}"; then
    fail "incomplete certified attempt is consumed and cannot be retried"
  fi
  assert_directory_empty "${scratch_root}" "pre-certified-attempt scratch"
  resource_safety_gate
  verify_runtime_root_and_lock_metadata
  verify_certified_lock_metadata "${certified_lock}"
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
    "certified ablation lock"
  publish_certified_attempt_once
  assert_operational_runner_identity
  mkdir -- "${runtime_root}/certified"
  [[ "$(stat -c '%a:%u' -- "${runtime_root}/certified")" == \
    "700:${process_owner_uid}" ]] || fail "certified root metadata drifted"
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
  require_owned_single_link_file "${certified_capture_log}" \
    "certified capture log"
  chmod 0444 "${certified_capture_log}"
  require_nonempty_file "${certified_job}/representation_edge_features.probe"
  seal_capture_job_files "${certified_job}"
  # The helper reconstructs and verifies the immutable development selection
  # lock before it opens the certified probe.  Do not parse that probe here.
  run_certified_reports
  assert_directory_empty "${scratch_root}" "certified child scratch"
  probe="$(validate_capture_job "${certified_job}" "${certified_begin}" \
    "${certified_end}" "${certified_rows}" "${checkpoint}" "${config}")"
  [[ "${probe}" == "${certified_job}/representation_edge_features.probe" ]] ||
    fail "certified representation probe path drifted"
  write_certified_capture_status
  assert_directory_empty "${scratch_root}" "certified receipt scratch"
  verify_runtime_root_and_lock_metadata
  verify_certified_lock_metadata "${certified_lock}"
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
    "certified ablation lock"
  write_result_receipt
  verify_complete
  verify_runtime_root_and_lock_metadata
  verify_certified_lock_metadata "${certified_lock}"
  assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
    "certified ablation lock"
  resource_safety_gate
  verify_runtime_root_and_lock_metadata
  verify_certified_lock_metadata "${certified_lock}"
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  assert_fd_matches_path "${certified_lock_fd}" "${certified_lock}" \
    "certified ablation lock"
  assert_operational_runner_identity
}

verify_retry2_amendment_authority() {
  require_resolved_sha256_pin "${expected_retry2_amendment_sha256}" \
    "retry2 staged-recovery amendment"
  verify_pinned_mode_file "${retry2_amendment}" \
    "${expected_retry2_amendment_sha256}" 444 \
    "retry2 staged-recovery amendment"
}

endpoint_source_inventory_value() {
  local relative_path="$1" column="$2"
  awk -F '\t' -v relative_path="${relative_path}" -v column="${column}" '
    NR > 1 && $1 == relative_path {
      count += 1;
      value = $column;
    }
    END {
      if (count != 1 || value == "") exit 1;
      print value;
    }
  ' "${endpoint_bundle_regular_inventory}" ||
    fail "endpoint source inventory binding is missing or duplicated: ${relative_path}"
}

verify_endpoint_bundle_exact_tree() {
  local root_device actual_files actual_directories actual_bytes
  local snapshot_files snapshot_directories
  local index prefix relative_path path expected_sha expected_bytes entry
  local tree_entries metadata_entry_count=0 expected_metadata_entry_count
  declare -A seen_relative_paths=()

  require_dir "${endpoint_bundle_root}"
  require_dir "${endpoint_bundle_snapshot}"
  [[ "$(sha256_of "${endpoint_bundle_regular_inventory}")" == \
    "${expected_endpoint_bundle_regular_inventory_sha256}" ]] ||
    fail "endpoint bundle regular inventory hash drifted"
  [[ "$(sha256_of "${endpoint_bundle_directory_inventory}")" == \
    "${expected_endpoint_bundle_directory_inventory_sha256}" ]] ||
    fail "endpoint bundle directory inventory hash drifted"

  entry="$(find "${endpoint_bundle_root}" -xdev -mindepth 0 \
    ! -type f ! -type d -print -quit)" ||
    fail "could not traverse the endpoint bundle exact tree"
  [[ -z "${entry}" ]] ||
    fail "endpoint bundle contains a symlink or special entry: ${entry}"
  actual_files="$(find "${endpoint_bundle_root}" -xdev -type f \
    -printf '.' | wc -c)" || fail "could not count endpoint bundle files"
  actual_directories="$(find "${endpoint_bundle_root}" -xdev -type d \
    -printf '.' | wc -c)" || fail "could not count endpoint bundle directories"
  [[ "${actual_files}" == "${expected_endpoint_bundle_total_file_count}" ]] ||
    fail "endpoint bundle total file count drifted"
  [[ "${actual_directories}" == \
    "${expected_endpoint_bundle_total_directory_count}" ]] ||
    fail "endpoint bundle total directory count drifted"
  snapshot_files="$(find "${endpoint_bundle_snapshot}" -xdev -type f \
    -printf '.' | wc -c)" || fail "could not count endpoint snapshot files"
  snapshot_directories="$(find "${endpoint_bundle_snapshot}" -xdev -type d \
    -printf '.' | wc -c)" ||
    fail "could not count endpoint snapshot directories"
  [[ "${snapshot_files}" == "${expected_endpoint_snapshot_file_count}" ]] ||
    fail "endpoint snapshot file count drifted"
  [[ "${snapshot_directories}" == \
    "${expected_endpoint_snapshot_directory_count}" ]] ||
    fail "endpoint snapshot directory count drifted"
  actual_bytes="$(find "${endpoint_bundle_snapshot}" -xdev -type f \
    -printf '%s\n' | awk '{ total += $1 } END { print total + 0 }')" ||
    fail "could not sum endpoint snapshot bytes"
  [[ "${actual_bytes}" == "${expected_endpoint_snapshot_regular_file_bytes}" ]] ||
    fail "endpoint snapshot regular-file byte count drifted"

  root_device="$(stat -c '%d' -- "${endpoint_bundle_root}")" ||
    fail "could not read endpoint bundle root device"
  [[ "${root_device}" =~ ^[0-9]+$ ]] ||
    fail "endpoint bundle root device is malformed"
  tree_entries="$(mktemp \
    "${scratch_root}/${schema_id}.endpoint_bundle_tree.XXXXXX")" ||
    fail "could not allocate endpoint bundle metadata inventory"
  find "${endpoint_bundle_root}" -xdev -mindepth 0 -print0 \
    >"${tree_entries}" || {
    rm -f -- "${tree_entries}"
    fail "could not enumerate endpoint bundle metadata"
  }
  while IFS= read -r -d '' entry; do
    metadata_entry_count=$((metadata_entry_count + 1))
    [[ "$(stat -c '%d' -- "${entry}")" == "${root_device}" ]] ||
      fail "endpoint bundle crosses a filesystem boundary: ${entry}"
    if [[ -f "${entry}" ]]; then
      expect_mode_owner_links "${entry}" 444 "endpoint bundle file"
    else
      [[ "$(stat -c '%a' -- "${entry}")" == 555 ]] ||
        fail "endpoint bundle directory mode drifted: ${entry}"
      [[ "$(stat -c '%u' -- "${entry}")" == "${process_owner_uid}" ]] ||
        fail "endpoint bundle directory owner drifted: ${entry}"
    fi
  done <"${tree_entries}"
  expected_metadata_entry_count=$((
    expected_endpoint_bundle_total_file_count +
      expected_endpoint_bundle_total_directory_count
  ))
  [[ "${metadata_entry_count}" == "${expected_metadata_entry_count}" ]] || {
    rm -f -- "${tree_entries}"
    fail "endpoint bundle metadata traversal was incomplete: ${metadata_entry_count}/${expected_metadata_entry_count}"
  }
  rm -f -- "${tree_entries}"

  for ((index = 0; index < expected_endpoint_snapshot_file_count; ++index)); do
    printf -v prefix 'endpoint_file_%02d' "${index}"
    relative_path="$(kv "${prefix}_relative_path" "${endpoint_bundle_receipt}")"
    [[ -n "${relative_path}" && "${relative_path}" != /* && \
      "${relative_path}" != *'/../'* && "${relative_path}" != '../'* && \
      "${relative_path}" != *'/..' && "${relative_path}" != '..' ]] ||
      fail "unsafe endpoint bundle relative path: ${relative_path}"
    [[ -z "${seen_relative_paths[${relative_path}]+x}" ]] ||
      fail "duplicate endpoint bundle relative path: ${relative_path}"
    seen_relative_paths["${relative_path}"]=1
    path="${endpoint_bundle_snapshot}/${relative_path}"
    require_contained_path "${path}" "${endpoint_bundle_snapshot}"
    require_immutable_file "${path}"
    expect_mode_owner_links "${path}" 444 "endpoint snapshot file"
    expected_sha="$(kv "${prefix}_sha256" "${endpoint_bundle_receipt}")"
    expected_bytes="$(kv "${prefix}_bytes" "${endpoint_bundle_receipt}")"
    [[ "${expected_sha}" =~ ^[0-9a-f]{64}$ && \
      "${expected_bytes}" =~ ^[0-9]+$ ]] ||
      fail "malformed endpoint receipt file binding: ${prefix}"
    [[ "$(sha256_of "${path}")" == "${expected_sha}" ]] ||
      fail "endpoint snapshot file hash drifted: ${relative_path}"
    [[ "$(stat -c '%s' -- "${path}")" == "${expected_bytes}" ]] ||
      fail "endpoint snapshot file size drifted: ${relative_path}"
  done
}

verify_endpoint_bundle_authority_static() {
  local value label
  for value in \
    "${expected_endpoint_bundle_sealer_sha256}" \
    "${expected_endpoint_bundle_amendment_sha256}" \
    "${expected_endpoint_bundle_receipt_sha256}" \
    "${expected_endpoint_bundle_checkpoint_sha256}" \
    "${expected_endpoint_bundle_policy_sha256}" \
    "${expected_endpoint_bundle_net_sha256}" \
    "${expected_endpoint_bundle_train_config_sha256}" \
    "${expected_endpoint_bundle_capture_config_sha256}"; do
    label="retry1 endpoint import bundle"
    require_resolved_sha256_pin "${value}" "${label}"
  done
  verify_pinned_mode_file "${endpoint_bundle_live_sealer}" \
    "${expected_endpoint_bundle_sealer_sha256}" 555 \
    "retry1 endpoint bundle sealer"
  verify_pinned_mode_file "${endpoint_bundle_live_amendment}" \
    "${expected_endpoint_bundle_amendment_sha256}" 444 \
    "retry1 endpoint bundle amendment"
  verify_pinned_mode_file "${endpoint_bundle_frozen_sealer}" \
    "${expected_endpoint_bundle_sealer_sha256}" 444 \
    "frozen retry1 endpoint bundle sealer"
  verify_pinned_mode_file "${endpoint_bundle_frozen_amendment}" \
    "${expected_endpoint_bundle_amendment_sha256}" 444 \
    "frozen retry1 endpoint bundle amendment"
  verify_pinned_mode_file "${endpoint_bundle_receipt}" \
    "${expected_endpoint_bundle_receipt_sha256}" 444 \
    "retry1 endpoint import bundle receipt"
  verify_pinned_mode_file "${endpoint_bundle_checkpoint}" \
    "${expected_endpoint_bundle_checkpoint_sha256}" 444 \
    "retry1 endpoint bundle checkpoint"
  verify_pinned_mode_file "${endpoint_bundle_policy}" \
    "${expected_endpoint_bundle_policy_sha256}" 444 \
    "retry1 endpoint bundle policy"
  verify_pinned_mode_file "${endpoint_bundle_net}" \
    "${expected_endpoint_bundle_net_sha256}" 444 \
    "retry1 endpoint bundle net"
  verify_pinned_mode_file "${endpoint_bundle_train_config}" \
    "${expected_endpoint_bundle_train_config_sha256}" 444 \
    "retry1 endpoint bundle training config"
  verify_pinned_mode_file "${endpoint_bundle_capture_config}" \
    "${expected_endpoint_bundle_capture_config_sha256}" 444 \
    "retry1 endpoint bundle capture config"
  require_immutable_file "${endpoint_bundle_regular_inventory}"
  require_immutable_file "${endpoint_bundle_directory_inventory}"
  verify_endpoint_bundle_exact_tree

  expect_kv "${endpoint_bundle_receipt}" schema_id \
    "${endpoint_bundle_schema_id}"
  expect_kv "${endpoint_bundle_receipt}" status complete
  expect_kv "${endpoint_bundle_receipt}" \
    bundle_kind immutable_historical_endpoint_evidence
  expect_kv "${endpoint_bundle_receipt}" historical_evidence_only true
  expect_kv "${endpoint_bundle_receipt}" bundle_runtime_root \
    "${endpoint_bundle_root}"
  expect_kv "${endpoint_bundle_receipt}" bundle_snapshot_root \
    "${endpoint_bundle_snapshot}"
  expect_kv "${endpoint_bundle_receipt}" bundle_receipt_path \
    "${endpoint_bundle_receipt}"
  expect_kv "${endpoint_bundle_receipt}" copy_method cp_reflink_never
  expect_kv "${endpoint_bundle_receipt}" \
    copy_byte_identity_verified_with_cmp true
  expect_kv "${endpoint_bundle_receipt}" \
    copy_sha256_identity_verified true
  expect_kv "${endpoint_bundle_receipt}" \
    copy_source_destination_inode_tuple_distinct true
  expect_kv "${endpoint_bundle_receipt}" copy_source_link_count 1
  expect_kv "${endpoint_bundle_receipt}" copy_destination_link_count 1
  expect_kv "${endpoint_bundle_receipt}" \
    interruption_closure_receipt_path \
    "${retry1_interruption_closure_receipt}"
  expect_kv "${endpoint_bundle_receipt}" \
    interruption_closure_receipt_sha256 \
    "${expected_retry1_interruption_closure_receipt_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    retry1_complete_content_inventory_sha256 \
    "${expected_retry1_runtime_content_inventory_sha256}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_regular_file_count 21
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_directory_count_including_root 9
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_descendant_directory_count 8
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_regular_file_bytes 32731999
  expect_kv "${endpoint_bundle_receipt}" endpoint_content_inventory_sha256 \
    "${expected_endpoint_bundle_content_inventory_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    source_regular_inventory_path "${endpoint_bundle_regular_inventory}"
  expect_kv "${endpoint_bundle_receipt}" source_regular_inventory_sha256 \
    "$(sha256_of "${endpoint_bundle_regular_inventory}")"
  expect_kv "${endpoint_bundle_receipt}" \
    source_directory_inventory_path "${endpoint_bundle_directory_inventory}"
  expect_kv "${endpoint_bundle_receipt}" source_directory_inventory_sha256 \
    "$(sha256_of "${endpoint_bundle_directory_inventory}")"
  expect_kv "${endpoint_bundle_receipt}" endpoint_bundle_sealer_path \
    "${endpoint_bundle_live_sealer}"
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_bundle_sealer_process_start_sha256 \
    "${expected_endpoint_bundle_sealer_sha256}"
  expect_kv "${endpoint_bundle_receipt}" frozen_endpoint_bundle_sealer_path \
    "${endpoint_bundle_frozen_sealer}"
  expect_kv "${endpoint_bundle_receipt}" frozen_endpoint_bundle_sealer_sha256 \
    "${expected_endpoint_bundle_sealer_sha256}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_bundle_amendment_path \
    "${endpoint_bundle_live_amendment}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_bundle_amendment_sha256 \
    "${expected_endpoint_bundle_amendment_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    frozen_endpoint_bundle_amendment_path "${endpoint_bundle_frozen_amendment}"
  expect_kv "${endpoint_bundle_receipt}" \
    frozen_endpoint_bundle_amendment_sha256 \
    "${expected_endpoint_bundle_amendment_sha256}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_checkpoint_source_path \
    "${retry1_endpoint_checkpoint}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_checkpoint_bundle_path \
    "${endpoint_bundle_checkpoint}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_checkpoint_file_sha256 \
    "${expected_retry1_endpoint_checkpoint_sha256}"
  [[ "${expected_endpoint_bundle_checkpoint_sha256}" == \
    "${expected_retry1_endpoint_checkpoint_sha256}" ]] ||
    fail "endpoint bundle checkpoint pin is not the historical endpoint hash"
  expect_kv "${endpoint_bundle_receipt}" endpoint_checkpoint_file_sha256 \
    "${expected_endpoint_bundle_checkpoint_sha256}"
  expect_kv "${endpoint_bundle_receipt}" endpoint_optimizer_steps 3000
  expect_kv "${endpoint_bundle_receipt}" endpoint_seed 17
  expect_kv "${endpoint_bundle_receipt}" endpoint_train_anchor_range \
    '[0,2496)'
  expect_kv "${endpoint_bundle_receipt}" endpoint_training_status complete
  expect_kv "${endpoint_bundle_receipt}" endpoint_runtime_result_status completed
  expect_kv "${endpoint_bundle_receipt}" endpoint_finite_parameter_check true
  expect_kv "${endpoint_bundle_receipt}" endpoint_nonfinite_output_count 0
  expect_kv "${endpoint_bundle_receipt}" endpoint_input_checkpoint_present false
  expect_kv "${endpoint_bundle_receipt}" endpoint_forecast_artifact_present false
  expect_kv "${endpoint_bundle_receipt}" endpoint_policy_artifact_present false
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_file_00_relative_path config/capture.config
  expect_kv "${endpoint_bundle_receipt}" endpoint_file_00_sha256 \
    "${expected_endpoint_bundle_capture_config_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_file_01_relative_path config/representation.jkimyei
  expect_kv "${endpoint_bundle_receipt}" endpoint_file_01_sha256 \
    "${expected_endpoint_bundle_policy_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_file_02_relative_path config/representation.net
  expect_kv "${endpoint_bundle_receipt}" endpoint_file_02_sha256 \
    "${expected_endpoint_bundle_net_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_file_03_relative_path config/train.config
  expect_kv "${endpoint_bundle_receipt}" endpoint_file_03_sha256 \
    "${expected_endpoint_bundle_train_config_sha256}"
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_file_06_relative_path \
    training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt
  expect_kv "${endpoint_bundle_receipt}" endpoint_file_06_sha256 \
    "${expected_endpoint_bundle_checkpoint_sha256}"
  expect_kv "${endpoint_bundle_receipt}" time_only_path_included false
  expect_kv "${endpoint_bundle_receipt}" \
    time_only_artifact_reuse_authorized false
  expect_kv "${endpoint_bundle_receipt}" no_tf_alignment_path_included false
  expect_kv "${endpoint_bundle_receipt}" \
    no_tf_alignment_artifact_reuse_authorized false
  expect_kv "${endpoint_bundle_receipt}" direct_retry1_use_authorized false
  expect_kv "${endpoint_bundle_receipt}" direct_bundle_use_authorized false
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_local_consumption_authorized false
  expect_kv "${endpoint_bundle_receipt}" \
    endpoint_import_authorized_by_this_bundle false
  expect_kv "${endpoint_bundle_receipt}" \
    separate_retry2_stage_verifier_required true
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_exact_scientific_config_equivalence_required true
  expect_kv "${endpoint_bundle_receipt}" retry2_second_local_copy_required true
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_second_copy_requires_reflink_never true
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_second_copy_requires_distinct_inode_tuple true
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_second_copy_requires_link_count_one true
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_runtime_schema_id "${retry2_schema_id}"
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_runtime_root "${retry2_runtime}"
  expect_kv "${endpoint_bundle_receipt}" \
    retry2_endpoint_consumption \
    retry2_local_second_copy_only_after_stage_verifier
}

normalize_endpoint_config_for_equivalence() {
  local config="$1" policy="$2" net="$3"
  awk -v policy="${policy}" -v net="${net}" '
    {
      line = $0;
      if (line ~ /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path[[:space:]]*=/) {
        sub(/=.*/, "= __ENDPOINT_POLICY__", line);
      } else if (line ~ /^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_path[[:space:]]*=/) {
        sub(/=.*/, "= __ENDPOINT_NET__", line);
      }
      print line;
    }
  ' "${config}"
}

verify_endpoint_bundle_scientific_equivalence() {
  verify_endpoint_bundle_authority_static
  verify_arm_files endpoint_scale
  cmp -s -- "${endpoint_bundle_policy}" "$(arm_policy endpoint_scale)" ||
    fail "endpoint bundle policy differs from retry2 endpoint policy"
  cmp -s -- "${endpoint_bundle_net}" "$(arm_net endpoint_scale)" ||
    fail "endpoint bundle net differs from retry2 endpoint net"
  local left right
  left="$(mktemp "${scratch_root}/${schema_id}.endpoint_train_bundle.XXXXXX")"
  right="$(mktemp "${scratch_root}/${schema_id}.endpoint_train_retry2.XXXXXX")"
  normalize_endpoint_config_for_equivalence "${endpoint_bundle_train_config}" \
    "${endpoint_bundle_policy}" "${endpoint_bundle_net}" >"${left}"
  normalize_endpoint_config_for_equivalence "$(arm_config endpoint_scale)" \
    "$(arm_policy endpoint_scale)" "$(arm_net endpoint_scale)" >"${right}"
  cmp -s -- "${left}" "${right}" || {
    rm -f -- "${left}" "${right}"
    fail "endpoint bundle/retry2 training configs are not scientifically equivalent"
  }
  rm -f -- "${left}" "${right}"
  left="$(mktemp "${scratch_root}/${schema_id}.endpoint_capture_bundle.XXXXXX")"
  right="$(mktemp "${scratch_root}/${schema_id}.endpoint_capture_retry2.XXXXXX")"
  normalize_endpoint_config_for_equivalence "${endpoint_bundle_capture_config}" \
    "${endpoint_bundle_policy}" "${endpoint_bundle_net}" >"${left}"
  normalize_endpoint_config_for_equivalence \
    "$(arm_capture_config endpoint_scale)" "$(arm_policy endpoint_scale)" \
    "$(arm_net endpoint_scale)" >"${right}"
  cmp -s -- "${left}" "${right}" || {
    rm -f -- "${left}" "${right}"
    fail "endpoint bundle/retry2 capture configs are not scientifically equivalent"
  }
  rm -f -- "${left}" "${right}"
}

emit_endpoint_import_status() {
  local destination="$1"
  local local_receipt_sha imported_sha historical_inode historical_device
  local bundle_inode bundle_device imported_inode imported_device imported_links
  local_receipt_sha="$(sha256_of \
    "${endpoint_import_source_bundle_receipt}")" ||
    fail "could not hash local endpoint source-bundle receipt"
  imported_sha="$(sha256_of "${endpoint_import_checkpoint}")" ||
    fail "could not hash imported endpoint checkpoint"
  historical_inode="$(endpoint_source_inventory_value \
    training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt 7)" ||
    fail "could not read historical endpoint checkpoint inode"
  historical_device="$(endpoint_source_inventory_value \
    training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt 8)" ||
    fail "could not read historical endpoint checkpoint device"
  bundle_inode="$(stat -c '%i' -- "${endpoint_bundle_checkpoint}")" ||
    fail "could not read endpoint bundle checkpoint inode"
  bundle_device="$(stat -c '%d' -- "${endpoint_bundle_checkpoint}")" ||
    fail "could not read endpoint bundle checkpoint device"
  imported_inode="$(stat -c '%i' -- "${endpoint_import_checkpoint}")" ||
    fail "could not read imported endpoint checkpoint inode"
  imported_device="$(stat -c '%d' -- "${endpoint_import_checkpoint}")" ||
    fail "could not read imported endpoint checkpoint device"
  imported_links="$(stat -c '%h' -- "${endpoint_import_checkpoint}")" ||
    fail "could not read imported endpoint checkpoint link count"
  [[ "${local_receipt_sha}" =~ ^[0-9a-f]{64}$ && \
    "${imported_sha}" =~ ^[0-9a-f]{64}$ && \
    "${historical_inode}" =~ ^[0-9]+$ && \
    "${historical_device}" =~ ^[0-9]+$ && \
    "${bundle_inode}" =~ ^[0-9]+$ && "${bundle_device}" =~ ^[0-9]+$ && \
    "${imported_inode}" =~ ^[0-9]+$ && \
    "${imported_device}" =~ ^[0-9]+$ && "${imported_links}" == 1 ]] ||
    fail "endpoint import receipt identity evidence is malformed"
  cat >"${destination}" <<STATUS
schema_id=${schema_id}.retry1_endpoint_import.v1
status=complete
arm=endpoint_scale
source_bundle_schema_id=${endpoint_bundle_schema_id}
source_bundle_receipt_path=${endpoint_bundle_receipt}
source_bundle_receipt_sha256=${expected_endpoint_bundle_receipt_sha256}
local_source_bundle_receipt_path=${endpoint_import_source_bundle_receipt}
local_source_bundle_receipt_sha256=${local_receipt_sha}
retry1_interruption_closure_receipt_path=${retry1_interruption_closure_receipt}
retry1_interruption_closure_receipt_sha256=${expected_retry1_interruption_closure_receipt_sha256}
retry1_runtime_content_inventory_sha256=${expected_retry1_runtime_content_inventory_sha256}
historical_source_checkpoint_path=${retry1_endpoint_checkpoint}
historical_source_checkpoint_sha256=${expected_retry1_endpoint_checkpoint_sha256}
bundle_checkpoint_path=${endpoint_bundle_checkpoint}
bundle_checkpoint_sha256=${expected_endpoint_bundle_checkpoint_sha256}
imported_checkpoint_path=${endpoint_import_checkpoint}
imported_checkpoint_sha256=${imported_sha}
historical_source_checkpoint_reopened_for_retry2_verification=false
historical_source_checkpoint_used_as_retry2_model_input=false
historical_source_checkpoint_inode_recorded=${historical_inode}
historical_source_checkpoint_device_recorded=${historical_device}
historical_source_copy_identity_verified_by_bundle=true
bundle_checkpoint_inode=${bundle_inode}
bundle_checkpoint_device=${bundle_device}
imported_checkpoint_inode=${imported_inode}
imported_checkpoint_device=${imported_device}
imported_checkpoint_links=${imported_links}
copy_command=cp_--reflink=never
copy_count_from_retry1_source_to_bundle=1
copy_count_from_bundle_to_retry2_import=1
exact_second_copy_verified=true
byte_identical_copy_verified=true
distinct_retry1_source_copy_identity_verified=true
distinct_bundle_copy_identity_verified=true
hardlink_authorized=false
embedded_retry1_paths_rewritten=false
scientific_config_equivalence_verified=true
historical_source_optimizer_steps=3000
bundle_copy_optimizer_steps=0
retry2_import_optimizer_steps=0
retry2_training_job_created=false
retry2_training_status_created=false
retry2_runtime_result_created=false
retry2_checkpoint_resume=false
retry2_endpoint_checkpoint_authority=local_import_copy_only
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
policy_access=false
STATUS
}

verify_endpoint_import() {
  local candidate historical_inode historical_device
  local imported_identity bundle_identity historical_identity
  verify_endpoint_bundle_scientific_equivalence
  require_dir "${endpoint_imports_root}"
  require_dir "${endpoint_import_root}"
  [[ "$(stat -c '%a:%u' -- "${endpoint_imports_root}")" == \
    "700:${process_owner_uid}" ]] || fail "endpoint imports root metadata drifted"
  [[ "$(stat -c '%a:%u' -- "${endpoint_import_root}")" == \
    "700:${process_owner_uid}" ]] || fail "endpoint import root metadata drifted"
  require_immutable_file "${endpoint_import_source_bundle_receipt}"
  require_immutable_file "${endpoint_import_checkpoint}"
  require_immutable_file "${endpoint_import_receipt}"
  cmp -s -- "${endpoint_bundle_receipt}" \
    "${endpoint_import_source_bundle_receipt}" ||
    fail "local endpoint source-bundle receipt was rewritten"
  cmp -s -- "${endpoint_bundle_checkpoint}" "${endpoint_import_checkpoint}" ||
    fail "retry2 endpoint checkpoint differs from the sibling bundle"
  [[ "$(stat -c '%h' -- "${endpoint_import_checkpoint}")" == 1 ]] ||
    fail "retry2 endpoint checkpoint has an external hard link"
  imported_identity="$(stat -c '%i:%d' -- \
    "${endpoint_import_checkpoint}")" ||
    fail "could not read retry2 endpoint checkpoint identity"
  bundle_identity="$(stat -c '%i:%d' -- \
    "${endpoint_bundle_checkpoint}")" ||
    fail "could not read endpoint bundle checkpoint identity"
  historical_inode="$(endpoint_source_inventory_value \
    training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt 7)" ||
    fail "could not read historical endpoint checkpoint inode"
  historical_device="$(endpoint_source_inventory_value \
    training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt 8)" ||
    fail "could not read historical endpoint checkpoint device"
  [[ "${imported_identity}" =~ ^[0-9]+:[0-9]+$ && \
    "${bundle_identity}" =~ ^[0-9]+:[0-9]+$ && \
    "${historical_inode}" =~ ^[0-9]+$ && \
    "${historical_device}" =~ ^[0-9]+$ ]] ||
    fail "endpoint checkpoint identity metadata is malformed"
  historical_identity="${historical_inode}:${historical_device}"
  [[ "${imported_identity}" != "${bundle_identity}" ]] ||
    fail "retry2 endpoint checkpoint aliases the sibling bundle inode"
  [[ "${imported_identity}" != "${historical_identity}" ]] ||
    fail "retry2 endpoint checkpoint aliases the recorded historical source inode"
  path_is_absent "$(arm_root endpoint_scale)/training" && \
    path_is_absent "$(arm_train_job endpoint_scale)" && \
    path_is_absent "$(arm_training_status endpoint_scale)" && \
    path_is_absent "$(arm_root endpoint_scale)/training.log" ||
    fail "retry2 fabricated endpoint training artifacts"
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_import_verify.XXXXXX")"
  emit_endpoint_import_status "${candidate}"
  cmp -s -- "${candidate}" "${endpoint_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "retry2 endpoint import receipt drifted"
  }
  rm -f -- "${candidate}"
  printf '%s' "${endpoint_import_checkpoint}"
}

run_endpoint_import() {
  verify_endpoint_bundle_scientific_equivalence
  path_is_absent "${endpoint_imports_root}" ||
    fail "endpoint imports parent predates its stage attempt"
  path_is_absent "${endpoint_import_root}" ||
    fail "endpoint import payload predates its stage attempt"
  path_is_absent "$(arm_root endpoint_scale)/training" && \
    path_is_absent "$(arm_train_job endpoint_scale)" && \
    path_is_absent "$(arm_training_status endpoint_scale)" && \
    path_is_absent "$(arm_root endpoint_scale)/training.log" ||
    fail "retry2 endpoint training artifacts are forbidden"
  mkdir -- "${endpoint_imports_root}"
  mkdir -- "${endpoint_import_root}"
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_source_bundle.XXXXXX")"
  cp --reflink=never -- "${endpoint_bundle_receipt}" "${candidate}"
  publish_immutable "${candidate}" "${endpoint_import_source_bundle_receipt}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_checkpoint.XXXXXX")"
  cp --reflink=never -- "${endpoint_bundle_checkpoint}" "${candidate}"
  publish_immutable "${candidate}" "${endpoint_import_checkpoint}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_import.XXXXXX")"
  emit_endpoint_import_status "${candidate}"
  publish_immutable "${candidate}" "${endpoint_import_receipt}"
  verify_endpoint_import >/dev/null
}

verify_arm_checkpoint_authority() {
  local arm="$1"
  case "${arm}" in
  endpoint_scale)
    verify_endpoint_import
    ;;
  time_only)
    verify_time_only_import
    ;;
  no_tf_alignment)
    verify_training_status no_tf_alignment
    ;;
  *)
    fail "no challenger checkpoint authority for arm ${arm}"
    ;;
  esac
}

emit_arm_checkpoint_authority_binding() {
  local arm="$1" status
  case "${arm}" in
  endpoint_scale)
    cat <<AUTHORITY
checkpoint_authority_kind=retry2_completed_prefix_endpoint_import
endpoint_import_status_path=${endpoint_import_receipt}
endpoint_import_status_sha256=$(sha256_of "${endpoint_import_receipt}")
retry3_training_job_created=false
retry3_training_status_created=false
retry3_optimizer_steps=0
AUTHORITY
    ;;
  time_only)
    cat <<AUTHORITY
checkpoint_authority_kind=retry2_completed_prefix_time_only_import
time_only_import_status_path=${time_only_import_receipt}
time_only_import_status_sha256=$(sha256_of "${time_only_import_receipt}")
historical_optimizer_steps=3000
retry3_training_job_created=false
retry3_training_status_created=false
retry3_optimizer_steps=0
AUTHORITY
    ;;
  no_tf_alignment)
    status="$(arm_training_status no_tf_alignment)"
    cat <<AUTHORITY
checkpoint_authority_kind=fresh_retry3_training_from_optimizer_step_zero
training_status_path=${status}
training_status_sha256=$(sha256_of "${status}")
retry3_optimizer_start_step=0
retry3_optimizer_steps=${expected_steps}
input_representation_checkpoint_path=none
input_mdn_checkpoint_path=none
AUTHORITY
    ;;
  *) fail "unsupported checkpoint authority binding arm: ${arm}" ;;
  esac
}

assert_directory_empty() {
  local path="$1" label="$2" first_entry
  require_dir "${path}"
  first_entry="$(find "${path}" -mindepth 1 -print -quit)" ||
    fail "could not traverse ${label}: ${path}"
  [[ -z "${first_entry}" ]] ||
    fail "${label} contains an interrupted or foreign payload: ${first_entry}"
}

verify_bootstrap_scratch_boundary() {
  assert_runner_bootstrap_lock
  path_is_absent "${retry2_bootstrap_failure_closure_candidate}" ||
    fail "retry2 bootstrap failure closure candidate remains"
  path_is_absent "${historical_bootstrap_scratch_root}" ||
    fail "historical retry2 bootstrap scratch reappeared"
  path_is_absent "${runtime_publication_guard}" ||
    fail "retry2 runtime publication guard remains"
  path_is_absent "${bootstrap_scratch_root}" && return
  require_dir "${bootstrap_scratch_root}"
  [[ "$(stat -c '%a' -- "${bootstrap_scratch_root}")" == 700 ]] ||
    fail "bootstrap scratch mode is not 0700"
  [[ "$(stat -c '%u' -- "${bootstrap_scratch_root}")" == \
    "${process_owner_uid}" ]] || fail "bootstrap scratch owner drifted"
  assert_directory_empty "${bootstrap_scratch_root}" "bootstrap scratch"
}

prepare_bootstrap_scratch() {
  assert_runner_bootstrap_lock
  reject_symlink_components "${runtime_parent}"
  require_dir "${runtime_parent}"
  if path_is_absent "${bootstrap_scratch_root}"; then
    mkdir -- "${bootstrap_scratch_root}"
    chmod 0700 -- "${bootstrap_scratch_root}"
  fi
  verify_bootstrap_scratch_boundary
  scratch_root="${bootstrap_scratch_root}"
  export TMPDIR="${scratch_root}"
}

prepare_runtime_scratch() {
  scratch_root="${runtime_root}/scratch"
  if path_is_absent "${scratch_root}"; then
    mkdir -- "${scratch_root}"
    chmod 0700 -- "${scratch_root}"
  fi
  require_dir "${scratch_root}"
  [[ "$(stat -c '%a' -- "${scratch_root}")" == 700 ]] ||
    fail "runtime scratch mode is not 0700"
  [[ "$(stat -c '%u' -- "${scratch_root}")" == "${process_owner_uid}" ]] ||
    fail "runtime scratch owner drifted"
  assert_directory_empty "${scratch_root}" "runtime scratch"
  export TMPDIR="${scratch_root}"
}

use_existing_runtime_scratch() {
  scratch_root="${runtime_root}/scratch"
  require_dir "${scratch_root}"
  [[ "$(stat -c '%a' -- "${scratch_root}")" == 700 ]] ||
    fail "runtime scratch mode is not 0700"
  [[ "$(stat -c '%u' -- "${scratch_root}")" == "${process_owner_uid}" ]] ||
    fail "runtime scratch owner drifted"
  assert_directory_empty "${scratch_root}" "runtime scratch"
  export TMPDIR="${scratch_root}"
}

available_bytes() {
  local path="$1"
  df -PB1 -- "${path}" | awk 'NR == 2 { print $4 }'
}

resource_safety_gate() {
  local cuwacunu_available root_available tmp_same_filesystem=false tmp_offender
  local cuwacunu_device root_device tmp_device
  require_dir /cuwacunu
  require_dir /
  require_dir /tmp
  cuwacunu_available="$(available_bytes /cuwacunu)"
  root_available="$(available_bytes /)"
  [[ "${cuwacunu_available}" =~ ^[0-9]+$ ]] ||
    fail "could not read /cuwacunu available bytes"
  [[ "${root_available}" =~ ^[0-9]+$ ]] ||
    fail "could not read / available bytes"
  ((cuwacunu_available >= minimum_cuwacunu_available_bytes)) ||
    fail "/cuwacunu free-space gate failed: ${cuwacunu_available} < ${minimum_cuwacunu_available_bytes}"
  ((root_available >= minimum_root_available_bytes)) ||
    fail "/ free-space gate failed: ${root_available} < ${minimum_root_available_bytes}"
  cuwacunu_device="$(stat -c '%d' -- /cuwacunu)" ||
    fail "could not read /cuwacunu device"
  root_device="$(stat -c '%d' -- /)" || fail "could not read / device"
  tmp_device="$(stat -c '%d' -- /tmp)" || fail "could not read /tmp device"
  [[ "${cuwacunu_device}" =~ ^[0-9]+$ && \
    "${root_device}" =~ ^[0-9]+$ && "${tmp_device}" =~ ^[0-9]+$ ]] ||
    fail "resource gate device metadata is malformed"
  if [[ "${tmp_device}" == "${root_device}" ]]; then
    tmp_same_filesystem=true
  fi
  tmp_offender="$(find /tmp -xdev -ignore_readdir_race \
    -type f -size +1073741824c \
    -print -quit)" || fail "could not complete the bounded /tmp safety scan"
  [[ -z "${tmp_offender}" ]] ||
    fail "/tmp contains a regular file larger than ${maximum_tmp_regular_file_bytes} bytes: ${tmp_offender}"
  last_gate_cuwacunu_available_bytes="${cuwacunu_available}"
  last_gate_root_available_bytes="${root_available}"
  last_gate_cuwacunu_device="${cuwacunu_device}"
  last_gate_root_device="${root_device}"
  last_gate_tmp_device="${tmp_device}"
  last_gate_tmp_same_as_root="${tmp_same_filesystem}"
  last_gate_tmp_oversize_regular_file_count=0
}

assert_fd_matches_path() {
  local fd="$1" path="$2" label="$3"
  local fd_identity path_identity
  require_file "${path}"
  fd_identity="$(stat -Lc '%i:%d' -- "/proc/$$/fd/${fd}")" ||
    fail "could not read ${label} descriptor identity"
  path_identity="$(stat -c '%i:%d' -- "${path}")" ||
    fail "could not read ${label} path identity"
  [[ "${fd_identity}" =~ ^[0-9]+:[0-9]+$ && \
    "${path_identity}" =~ ^[0-9]+:[0-9]+$ ]] ||
    fail "${label} identity metadata is malformed"
  [[ "${fd_identity}" == "${path_identity}" ]] ||
    fail "${label} descriptor/path identity drifted"
}

assert_runner_bootstrap_lock() {
  [[ "${bootstrap_lock_acquired}" == true ]] ||
    fail "retry2 bootstrap runner lock was not acquired"
  [[ "${bootstrap_lock_fd:-}" =~ ^[0-9]+$ ]] ||
    fail "retry2 bootstrap runner lock descriptor is invalid"
  assert_fd_matches_path "${bootstrap_lock_fd}" "${script_path}" \
    "retry2 bootstrap runner lock"
  assert_operational_runner_identity
}

acquire_runner_bootstrap_lock() {
  [[ "${bootstrap_lock_acquired}" == false ]] ||
    fail "retry2 bootstrap runner lock was acquired more than once"
  exec {bootstrap_lock_fd}<"${script_path}"
  flock -n "${bootstrap_lock_fd}" ||
    fail "another retry2 bootstrap process holds the runner lock"
  bootstrap_lock_acquired=true
  assert_runner_bootstrap_lock
}

assert_runtime_candidate_only_lock() {
  local entry_count first_entry
  require_dir "${runtime_root_candidate}"
  [[ "$(stat -c '%a:%u' -- "${runtime_root_candidate}")" == \
    "700:${process_owner_uid}" ]] || fail "runtime candidate metadata drifted"
  require_file "${runtime_root_candidate_lock}"
  [[ "$(stat -c '%a:%u:%h:%s' -- "${runtime_root_candidate_lock}")" == \
    "600:${process_owner_uid}:1:0" ]] ||
    fail "runtime candidate lock metadata drifted"
  entry_count="$(find "${runtime_root_candidate}" -mindepth 1 -maxdepth 1 \
    -printf '.' | wc -c)" || fail "could not enumerate retry2 runtime candidate"
  [[ "${entry_count}" == 1 ]] ||
    fail "retry2 runtime candidate does not contain exactly one entry"
  first_entry="$(find "${runtime_root_candidate}" -mindepth 1 -maxdepth 1 \
    -print -quit)" || fail "could not inspect retry2 runtime candidate"
  [[ "${first_entry}" == "${runtime_root_candidate_lock}" ]] ||
    fail "retry2 runtime candidate contains a foreign payload: ${first_entry}"
}

assert_bootstrap_candidate_only() {
  local entry_count first_entry
  require_dir "${bootstrap_scratch_root}"
  entry_count="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 \
    -printf '.' | wc -c)" || fail "could not enumerate bootstrap candidate state"
  [[ "${entry_count}" == 1 ]] ||
    fail "bootstrap scratch does not contain exactly the runtime candidate"
  first_entry="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 \
    -print -quit)" || fail "could not inspect bootstrap candidate state"
  [[ "${first_entry}" == "${runtime_root_candidate}" ]] ||
    fail "bootstrap scratch contains a foreign entry: ${first_entry}"
}

assert_bootstrap_candidate_and_guard() {
  local entry_count candidate_seen=0 guard_seen=0 entry entries
  require_dir "${bootstrap_scratch_root}"
  entry_count="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 \
    -printf '.' | wc -c)" || fail "could not enumerate guarded bootstrap state"
  [[ "${entry_count}" == 2 ]] ||
    fail "guarded bootstrap scratch does not contain exactly two entries"
  entries="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 -print)" ||
    fail "could not capture guarded bootstrap entry paths"
  while IFS= read -r entry; do
    case "${entry}" in
    "${runtime_root_candidate}") candidate_seen=$((candidate_seen + 1)) ;;
    "${runtime_publication_guard}") guard_seen=$((guard_seen + 1)) ;;
    *) fail "guarded bootstrap scratch contains a foreign entry: ${entry}" ;;
    esac
  done <<<"${entries}"
  [[ "${candidate_seen}:${guard_seen}" == "1:1" ]] ||
    fail "guarded bootstrap scratch entry set drifted"
}

assert_bootstrap_guard_only() {
  local entry_count first_entry
  require_dir "${bootstrap_scratch_root}"
  entry_count="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 \
    -printf '.' | wc -c)" || fail "could not enumerate post-rename bootstrap state"
  [[ "${entry_count}" == 1 ]] ||
    fail "post-rename bootstrap scratch does not contain exactly the guard"
  first_entry="$(find "${bootstrap_scratch_root}" -mindepth 1 -maxdepth 1 \
    -print -quit)" || fail "could not inspect post-rename bootstrap state"
  [[ "${first_entry}" == "${runtime_publication_guard}" ]] ||
    fail "post-rename bootstrap scratch contains a foreign entry: ${first_entry}"
}

emit_runtime_publication_guard() {
  local root_inode="$1" root_device="$2" lock_inode="$3" lock_device="$4"
  local parent_inode="$5" parent_device="$6" scratch_inode="$7" scratch_device="$8"
  cat <<PUBLICATION_GUARD
schema_id=${schema_id}.runtime_root_publication_guard.v1
status=publication_in_progress
operational_ablation_runner_path=${script_path}
operational_ablation_runner_sha256=${process_start_runner_sha256}
operational_ablation_runner_inode=${process_start_runner_inode}
operational_ablation_runner_device=${process_start_runner_device}
bootstrap_runner_lock_path=${script_path}
bootstrap_failure_closure_receipt_path=${retry2_bootstrap_failure_receipt}
bootstrap_failure_closure_receipt_sha256=${expected_retry2_bootstrap_failure_receipt_sha256}
runtime_parent_path=${runtime_parent}
runtime_parent_inode=${parent_inode}
runtime_parent_device=${parent_device}
bootstrap_scratch_path=${bootstrap_scratch_root}
bootstrap_scratch_inode=${scratch_inode}
bootstrap_scratch_device=${scratch_device}
runtime_candidate_path=${runtime_root_candidate}
runtime_candidate_inode=${root_inode}
runtime_candidate_device=${root_device}
runtime_candidate_lock_path=${runtime_root_candidate_lock}
runtime_candidate_lock_inode=${lock_inode}
runtime_candidate_lock_device=${lock_device}
canonical_runtime_path=${runtime_root}
canonical_runtime_lock_path=${runtime_development_lock}
publication_method=close_candidate_lock_then_no_clobber_rename_then_read_only_reopen
candidate_lock_closed_before_rename=true
guard_removal_condition=canonical_read_only_lock_reopened_and_continuity_verified
scientific_attempt_consumed=false
PUBLICATION_GUARD
}

verify_runtime_publication_guard() {
  local root_inode="$1" root_device="$2" lock_inode="$3" lock_device="$4"
  local parent_inode="$5" parent_device="$6" scratch_inode="$7" scratch_device="$8"
  local expected_sha actual_sha
  require_nonempty_file "${runtime_publication_guard}"
  [[ "$(stat -c '%a:%u:%h:%d' -- "${runtime_publication_guard}")" == \
    "600:${process_owner_uid}:1:${root_device}" ]] ||
    fail "runtime publication guard metadata drifted"
  expected_sha="$(emit_runtime_publication_guard "${root_inode}" "${root_device}" \
    "${lock_inode}" "${lock_device}" "${parent_inode}" "${parent_device}" \
    "${scratch_inode}" "${scratch_device}" | sha256sum | awk '{print $1}')" ||
    fail "could not derive runtime publication guard hash"
  [[ "${expected_sha}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "runtime publication guard expected hash is malformed"
  actual_sha="$(sha256_of "${runtime_publication_guard}")" ||
    fail "could not hash runtime publication guard"
  [[ "${actual_sha}" == "${expected_sha}" ]] ||
    fail "runtime publication guard content drifted"
  validate_receipt_sha256_fields "${runtime_publication_guard}"
  validate_local_receipt_nonempty_fields "${runtime_publication_guard}"
}

assert_runtime_root_only_lock() {
  verify_runtime_root_and_lock_metadata
  local entry_count first_entry
  entry_count="$(find "${runtime_root}" -mindepth 1 -maxdepth 1 \
    -printf '.' | wc -c)" ||
    fail "could not enumerate the pre-attempt retry2 runtime"
  [[ "${entry_count}" == 1 ]] ||
    fail "pre-attempt retry2 runtime does not contain exactly one entry"
  first_entry="$(find "${runtime_root}" -mindepth 1 -maxdepth 1 \
    -print -quit)" || fail "could not inspect the pre-attempt retry2 runtime"
  [[ "${first_entry}" == "${runtime_development_lock}" ]] ||
    fail "pre-attempt retry2 runtime contains payload: ${first_entry}"
}

verify_runtime_root_and_lock_metadata() {
  require_dir "${runtime_root}"
  [[ "$(stat -c '%a' -- "${runtime_root}")" == 700 ]] ||
    fail "retry2 runtime root mode is not 0700"
  [[ "$(stat -c '%u' -- "${runtime_root}")" == "${process_owner_uid}" ]] ||
    fail "retry2 runtime root owner drifted"
  require_file "${runtime_development_lock}"
  [[ "$(stat -c '%a' -- "${runtime_development_lock}")" == 600 ]] ||
    fail "retry2 development lock mode is not 0600"
  [[ "$(stat -c '%u' -- "${runtime_development_lock}")" == \
    "${process_owner_uid}" ]] || fail "retry2 development lock owner drifted"
  [[ "$(stat -c '%h' -- "${runtime_development_lock}")" == 1 ]] ||
    fail "retry2 development lock has an external hard link"
  [[ "$(stat -c '%s' -- "${runtime_development_lock}")" == 0 ]] ||
    fail "retry2 development lock is not empty"
}

assert_development_lock_held() {
  [[ "${development_lock_acquired}" == true ]] ||
    fail "retry2 development lock was not acquired"
  [[ "${development_lock_fd:-}" =~ ^[0-9]+$ ]] ||
    fail "retry2 development lock descriptor is invalid"
  verify_runtime_root_and_lock_metadata
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
}

assert_runtime_publication_ready() {
  [[ "${runtime_publication_ready}" == true ]] ||
    fail "retry2 runtime publication is not ready"
  assert_runner_bootstrap_lock
  assert_development_lock_held
  verify_retry2_windows_safe_publication_authority_v2
  path_is_absent "${runtime_publication_guard}" ||
    fail "retry2 runtime publication guard remains after publication"
  verify_bootstrap_scratch_boundary
}

acquire_or_create_runtime_lock() {
  assert_runner_bootstrap_lock
  [[ "${development_lock_acquired}" == false ]] ||
    fail "retry2 development lock was acquired more than once"
  [[ "${runtime_publication_ready}" == false ]] ||
    fail "retry2 runtime publication readiness was set before lock acquisition"
  if path_is_absent "${runtime_root}"; then
    local candidate_root_inode candidate_root_device
    local candidate_lock_inode candidate_lock_device runtime_parent_device
    local bootstrap_scratch_inode bootstrap_scratch_device
    local runtime_parent_inode runtime_parent_identity closed_development_lock_fd
    [[ "${scratch_root}" == "${bootstrap_scratch_root}" ]] ||
      fail "new retry2 runtime must be published from bootstrap scratch"
    verify_retry2_bootstrap_failure_closure_authority
    verify_bootstrap_scratch_boundary
    path_is_absent "${runtime_root_candidate}" ||
      fail "retry2 runtime-root candidate already exists"
    path_is_absent "${runtime_publication_guard}" ||
      fail "retry2 runtime publication guard already exists"
    runtime_parent_identity="$(stat -c '%i:%d' -- "${runtime_parent}")" ||
      fail "could not capture runtime publication-parent identity"
    [[ "${runtime_parent_identity}" =~ ^[0-9]+:[0-9]+$ ]] ||
      fail "runtime publication-parent identity is malformed"
    runtime_parent_inode="${runtime_parent_identity%%:*}"
    mkdir -- "${runtime_root_candidate}"
    chmod 0700 -- "${runtime_root_candidate}"
    path_is_absent "${runtime_root_candidate_lock}" ||
      fail "retry2 runtime candidate lock unexpectedly exists"
    exec {development_lock_fd}>"${runtime_root_candidate_lock}"
    chmod 0600 -- "${runtime_root_candidate_lock}"
    flock -n "${development_lock_fd}" ||
      fail "could not lock the unpublished retry2 runtime candidate"
    assert_runtime_candidate_only_lock
    assert_bootstrap_candidate_only
    assert_fd_matches_path "${development_lock_fd}" \
      "${runtime_root_candidate_lock}" "unpublished retry2 development lock"
    candidate_root_inode="$(stat -c '%i' -- "${runtime_root_candidate}")" ||
      fail "could not read runtime candidate inode"
    candidate_root_device="$(stat -c '%d' -- "${runtime_root_candidate}")" ||
      fail "could not read runtime candidate device"
    candidate_lock_inode="$(stat -c '%i' -- "${runtime_root_candidate_lock}")" ||
      fail "could not read runtime candidate lock inode"
    candidate_lock_device="$(stat -c '%d' -- "${runtime_root_candidate_lock}")" ||
      fail "could not read runtime candidate lock device"
    runtime_parent_device="$(stat -c '%d' -- "${runtime_parent}")" ||
      fail "could not read runtime publication-parent device"
    bootstrap_scratch_device="$(stat -c '%d' -- "${bootstrap_scratch_root}")" ||
      fail "could not read bootstrap scratch device"
    bootstrap_scratch_inode="$(stat -c '%i' -- "${bootstrap_scratch_root}")" ||
      fail "could not read bootstrap scratch inode"
    [[ "${candidate_root_inode}" =~ ^[0-9]+$ && \
      "${candidate_root_device}" =~ ^[0-9]+$ && \
      "${candidate_lock_inode}" =~ ^[0-9]+$ && \
      "${candidate_lock_device}" =~ ^[0-9]+$ && \
      "${runtime_parent_inode}" =~ ^[0-9]+$ && \
      "${runtime_parent_device}" =~ ^[0-9]+$ && \
      "${bootstrap_scratch_inode}" =~ ^[0-9]+$ && \
      "${bootstrap_scratch_device}" =~ ^[0-9]+$ ]] ||
      fail "runtime publication device metadata is malformed"
    [[ "${candidate_root_device}" == "${candidate_lock_device}" && \
      "${candidate_root_device}" == "${runtime_parent_device}" && \
      "${candidate_root_device}" == "${bootstrap_scratch_device}" ]] ||
      fail "runtime candidate is not on the publication filesystem"

    (
      set -o noclobber
      emit_runtime_publication_guard "${candidate_root_inode}" \
        "${candidate_root_device}" "${candidate_lock_inode}" \
        "${candidate_lock_device}" "${runtime_parent_inode}" \
        "${runtime_parent_device}" "${bootstrap_scratch_inode}" \
        "${bootstrap_scratch_device}" >"${runtime_publication_guard}"
    ) || fail "could not exclusively create runtime publication guard"
    chmod 0600 -- "${runtime_publication_guard}"
    verify_runtime_publication_guard "${candidate_root_inode}" \
      "${candidate_root_device}" "${candidate_lock_inode}" \
      "${candidate_lock_device}" "${runtime_parent_inode}" \
      "${runtime_parent_device}" "${bootstrap_scratch_inode}" \
      "${bootstrap_scratch_device}"
    assert_bootstrap_candidate_and_guard
    assert_runner_bootstrap_lock
    assert_runtime_candidate_only_lock
    assert_fd_matches_path "${development_lock_fd}" \
      "${runtime_root_candidate_lock}" "unpublished retry2 development lock"

    closed_development_lock_fd="${development_lock_fd}"
    [[ "${closed_development_lock_fd}" =~ ^[0-9]+$ ]] ||
      fail "candidate development lock descriptor is malformed"
    exec {development_lock_fd}<&-
    [[ ! -e "/proc/$$/fd/${closed_development_lock_fd}" && \
      ! -L "/proc/$$/fd/${closed_development_lock_fd}" ]] ||
      fail "candidate development lock descriptor remained open"
    unset development_lock_fd

    assert_runner_bootstrap_lock
    assert_runtime_candidate_only_lock
    verify_runtime_publication_guard "${candidate_root_inode}" \
      "${candidate_root_device}" "${candidate_lock_inode}" \
      "${candidate_lock_device}" "${runtime_parent_inode}" \
      "${runtime_parent_device}" "${bootstrap_scratch_inode}" \
      "${bootstrap_scratch_device}"
    assert_bootstrap_candidate_and_guard
    [[ "$(stat -c '%i:%d' -- "${runtime_parent}")" == \
      "${runtime_parent_identity}" ]] ||
      fail "runtime publication-parent identity drifted"
    [[ "$(stat -c '%i:%d' -- "${bootstrap_scratch_root}")" == \
      "${bootstrap_scratch_inode}:${bootstrap_scratch_device}" ]] ||
      fail "bootstrap scratch identity drifted before runtime publication"
    path_is_absent "${runtime_root}" ||
      fail "retry2 runtime appeared before publication"
    mv -T -n -- "${runtime_root_candidate}" "${runtime_root}" ||
      fail "could not atomically publish retry2 runtime root"
    path_is_absent "${runtime_root_candidate}" && \
      [[ -d "${runtime_root}" && ! -L "${runtime_root}" ]] ||
      fail "atomic retry2 runtime-root publication did not complete"
    [[ "$(stat -c '%i:%d' -- "${runtime_root}")" == \
      "${candidate_root_inode}:${candidate_root_device}" ]] ||
      fail "published retry2 runtime-root identity drifted"
    [[ "$(stat -c '%i:%d' -- "${runtime_development_lock}")" == \
      "${candidate_lock_inode}:${candidate_lock_device}" ]] ||
      fail "published retry2 development-lock identity drifted"
    assert_runtime_root_only_lock
    assert_bootstrap_guard_only
    verify_runtime_publication_guard "${candidate_root_inode}" \
      "${candidate_root_device}" "${candidate_lock_inode}" \
      "${candidate_lock_device}" "${runtime_parent_inode}" \
      "${runtime_parent_device}" "${bootstrap_scratch_inode}" \
      "${bootstrap_scratch_device}"

    exec {development_lock_fd}<"${runtime_development_lock}"
    flock -n "${development_lock_fd}" ||
      fail "could not reacquire the published retry2 development lock"
    development_lock_acquired=true
    assert_development_lock_held
    assert_runtime_root_only_lock
    [[ "$(stat -c '%i:%d' -- "${runtime_root}")" == \
      "${candidate_root_inode}:${candidate_root_device}" ]] ||
      fail "reopened retry2 runtime-root identity drifted"
    [[ "$(stat -c '%i:%d' -- "${runtime_development_lock}")" == \
      "${candidate_lock_inode}:${candidate_lock_device}" ]] ||
      fail "reopened retry2 development-lock identity drifted"
    verify_runtime_publication_guard "${candidate_root_inode}" \
      "${candidate_root_device}" "${candidate_lock_inode}" \
      "${candidate_lock_device}" "${runtime_parent_inode}" \
      "${runtime_parent_device}" "${bootstrap_scratch_inode}" \
      "${bootstrap_scratch_device}"
    assert_bootstrap_guard_only
    assert_runner_bootstrap_lock
    verify_retry2_bootstrap_failure_closure_authority
    [[ "$(stat -c '%i:%d' -- "${runtime_parent}")" == \
      "${runtime_parent_identity}" ]] ||
      fail "runtime publication-parent identity drifted before guard removal"
    [[ "$(stat -c '%i:%d' -- "${bootstrap_scratch_root}")" == \
      "${bootstrap_scratch_inode}:${bootstrap_scratch_device}" ]] ||
      fail "bootstrap scratch identity drifted before guard removal"
    rm -- "${runtime_publication_guard}" ||
      fail "could not remove the completed runtime publication guard"
    path_is_absent "${runtime_publication_guard}" ||
      fail "runtime publication guard remained after removal"
    assert_directory_empty "${bootstrap_scratch_root}" "bootstrap scratch"
  else
    verify_bootstrap_scratch_boundary
    verify_runtime_root_and_lock_metadata
    exec {development_lock_fd}<"${runtime_development_lock}"
    flock -n "${development_lock_fd}" ||
      fail "another retry2 development process holds the runtime lock"
    development_lock_acquired=true
    assert_development_lock_held
  fi
  runtime_publication_ready=true
  assert_runtime_publication_ready
}

acquire_existing_runtime_lock() {
  assert_runner_bootstrap_lock
  [[ "${development_lock_acquired}" == false ]] ||
    fail "retry2 development lock was acquired more than once"
  [[ "${runtime_publication_ready}" == false ]] ||
    fail "retry2 runtime publication readiness was set before existing-lock acquisition"
  verify_bootstrap_scratch_boundary
  ! path_is_absent "${runtime_root}" ||
    fail "retry2 runtime does not exist"
  verify_runtime_root_and_lock_metadata
  exec {development_lock_fd}<"${runtime_development_lock}"
  flock -n "${development_lock_fd}" ||
    fail "another retry2 process holds the runtime development lock"
  development_lock_acquired=true
  assert_development_lock_held
  runtime_publication_ready=true
  assert_runtime_publication_ready
}

stage_tag() {
  printf '%02d' "$1"
}

stage_attempt_path() {
  local index="$1" tag
  tag="$(stage_tag "${index}")"
  printf '%s/stage.%s.%s.attempt.status' "${runtime_root}" "${tag}" \
    "${development_stage_names[${index}]}"
}

stage_completion_path() {
  local index="$1" tag
  tag="$(stage_tag "${index}")"
  printf '%s/stage.%s.%s.status' "${runtime_root}" "${tag}" \
    "${development_stage_names[${index}]}"
}

stage_previous_binding() {
  local index="$1"
  if ((index == 0)); then
    echo "previous_stage_completion_path=none"
    echo "previous_stage_completion_sha256=none"
  else
    local previous
    previous="$(stage_completion_path "$((index - 1))")"
    require_immutable_file "${previous}"
    echo "previous_stage_completion_path=${previous}"
    echo "previous_stage_completion_sha256=$(sha256_of "${previous}")"
  fi
}

emit_stage_primary_bindings() {
  local index="$1" arm status log
  case "${index}" in
  0)
    echo "primary_artifact_kind=initialization_inputs"
    echo "primary_artifact_path=${input_receipt}"
    echo "primary_artifact_sha256=$(sha256_of "${input_receipt}")"
    echo "config_closure_path=${config_closure}"
    echo "config_closure_sha256=$(sha256_of "${config_closure}")"
    echo "effective_grammar_closure_path=${effective_grammar_closure}"
    echo "effective_grammar_closure_sha256=$(sha256_of "${effective_grammar_closure}")"
    echo "frozen_runner_path=${frozen_runner}"
    echo "frozen_runner_sha256=$(sha256_of "${frozen_runner}")"
    ;;
  1)
    echo "primary_artifact_kind=canonical_import"
    echo "primary_artifact_path=${canonical_import_receipt}"
    echo "primary_artifact_sha256=$(sha256_of "${canonical_import_receipt}")"
    ;;
  2)
    echo "primary_artifact_kind=retry2_completed_prefix_endpoint_import"
    echo "primary_artifact_path=${endpoint_import_receipt}"
    echo "primary_artifact_sha256=$(sha256_of "${endpoint_import_receipt}")"
    echo "endpoint_import_checkpoint_path=${endpoint_import_checkpoint}"
    echo "endpoint_import_checkpoint_sha256=$(sha256_of "${endpoint_import_checkpoint}")"
    ;;
  3)
    echo "primary_artifact_kind=retry2_completed_prefix_time_only_import"
    echo "primary_artifact_path=${time_only_import_receipt}"
    echo "primary_artifact_sha256=$(sha256_of "${time_only_import_receipt}")"
    echo "time_only_import_checkpoint_path=${time_only_import_checkpoint}"
    echo "time_only_import_checkpoint_sha256=$(sha256_of "${time_only_import_checkpoint}")"
    ;;
  4)
    arm=no_tf_alignment
    status="$(arm_training_status "${arm}")"
    log="$(arm_root "${arm}")/training.log"
    echo "primary_artifact_kind=fresh_retry3_training_from_optimizer_step_zero"
    echo "primary_artifact_path=${status}"
    echo "primary_artifact_sha256=$(sha256_of "${status}")"
    echo "training_log_path=${log}"
    echo "training_log_sha256=$(sha256_of "${log}")"
    ;;
  5 | 6 | 7)
    case "${index}" in
    5) arm=endpoint_scale ;;
    6) arm=time_only ;;
    7) arm=no_tf_alignment ;;
    esac
    status="$(arm_capture_status "${arm}")"
    echo "primary_artifact_kind=retry3_feature_capture"
    echo "primary_artifact_path=${status}"
    echo "primary_artifact_sha256=$(sha256_of "${status}")"
    echo "capture_train_log_path=$(arm_root "${arm}")/capture/train.log"
    echo "capture_train_log_sha256=$(sha256_of "$(arm_root "${arm}")/capture/train.log")"
    echo "capture_validation_log_path=$(arm_root "${arm}")/capture/validation.log"
    echo "capture_validation_log_sha256=$(sha256_of "$(arm_root "${arm}")/capture/validation.log")"
    ;;
  8 | 9 | 10)
    case "${index}" in
    8) arm=endpoint_scale ;;
    9) arm=time_only ;;
    10) arm=no_tf_alignment ;;
    esac
    status="$(arm_affine_status "${arm}")"
    echo "primary_artifact_kind=retry3_affine_development"
    echo "primary_artifact_path=${status}"
    echo "primary_artifact_sha256=$(sha256_of "${status}")"
    ;;
  11)
    echo "primary_artifact_kind=selection_and_development"
    echo "primary_artifact_path=${development_receipt}"
    echo "primary_artifact_sha256=$(sha256_of "${development_receipt}")"
    echo "selection_receipt_path=${selection_receipt}"
    echo "selection_receipt_sha256=$(sha256_of "${selection_receipt}")"
    ;;
  *) fail "invalid stage binding index: ${index}" ;;
  esac
}

emit_stage_attempt() {
  local index="$1" destination="$2" tag
  local runtime_root_mode runtime_root_owner runtime_root_inode runtime_root_device
  local lock_mode lock_owner lock_links lock_bytes lock_inode lock_device
  local observed_cuwacunu="${3:-${last_gate_cuwacunu_available_bytes:-}}"
  local observed_root="${4:-${last_gate_root_available_bytes:-}}"
  local observed_cuwacunu_device="${5:-${last_gate_cuwacunu_device:-}}"
  local observed_root_device="${6:-${last_gate_root_device:-}}"
  local observed_tmp_device="${7:-${last_gate_tmp_device:-}}"
  local observed_tmp_same="${8:-${last_gate_tmp_same_as_root:-}}"
  local observed_tmp_count="${9:-${last_gate_tmp_oversize_regular_file_count:-}}"
  [[ "${observed_cuwacunu}" =~ ^[0-9]+$ && \
    "${observed_root}" =~ ^[0-9]+$ && \
    "${observed_cuwacunu_device}" =~ ^[0-9]+$ && \
    "${observed_root_device}" =~ ^[0-9]+$ && \
    "${observed_tmp_device}" =~ ^[0-9]+$ && \
    "${observed_tmp_count}" == 0 ]] ||
    fail "invalid pre-attempt resource observation"
  [[ "${observed_tmp_same}" == true || "${observed_tmp_same}" == false ]] ||
    fail "invalid pre-attempt /tmp filesystem observation"
  runtime_root_mode="$(stat -c '%a' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root mode"
  runtime_root_owner="$(stat -c '%u' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root owner"
  runtime_root_inode="$(stat -c '%i' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root inode"
  runtime_root_device="$(stat -c '%d' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root device"
  lock_mode="$(stat -c '%a' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock mode"
  lock_owner="$(stat -c '%u' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock owner"
  lock_links="$(stat -c '%h' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock link count"
  lock_bytes="$(stat -c '%s' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock size"
  lock_inode="$(stat -c '%i' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock inode"
  lock_device="$(stat -c '%d' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock device"
  [[ "${runtime_root_mode}" == 700 && \
    "${runtime_root_owner}" == "${process_owner_uid}" && \
    "${runtime_root_inode}" =~ ^[0-9]+$ && \
    "${runtime_root_device}" =~ ^[0-9]+$ && \
    "${lock_mode}" == 600 && "${lock_owner}" == "${process_owner_uid}" && \
    "${lock_links}" == 1 && "${lock_bytes}" == 0 && \
    "${lock_inode}" =~ ^[0-9]+$ && "${lock_device}" =~ ^[0-9]+$ ]] ||
    fail "retry2 stage-attempt root or lock metadata is malformed"
  tag="$(stage_tag "${index}")"
  {
    echo "schema_id=${schema_id}.development_stage_attempt.v1"
    echo "status=consumed"
    echo "stage_ordinal=${tag}"
    echo "stage_name=${development_stage_names[${index}]}"
    stage_previous_binding "${index}"
    emit_ablation_runner_bindings
    echo "retry2_staged_recovery_amendment_path=${retry2_amendment}"
    echo "retry2_staged_recovery_amendment_sha256=${expected_retry2_amendment_sha256}"
    echo "retry1_interruption_closure_receipt_path=${retry1_interruption_closure_receipt}"
    echo "retry1_interruption_closure_receipt_sha256=${expected_retry1_interruption_closure_receipt_sha256}"
    echo "runtime_root_path=${runtime_root}"
    echo "runtime_root_mode=${runtime_root_mode}"
    echo "runtime_root_owner_uid=${runtime_root_owner}"
    echo "runtime_root_inode=${runtime_root_inode}"
    echo "runtime_root_device=${runtime_root_device}"
    echo "development_lock_path=${runtime_development_lock}"
    echo "development_lock_mode=${lock_mode}"
    echo "development_lock_owner_uid=${lock_owner}"
    echo "development_lock_links=${lock_links}"
    echo "development_lock_bytes=${lock_bytes}"
    echo "development_lock_inode=${lock_inode}"
    echo "development_lock_device=${lock_device}"
    echo "minimum_cuwacunu_available_bytes=${minimum_cuwacunu_available_bytes}"
    echo "minimum_root_available_bytes=${minimum_root_available_bytes}"
    echo "maximum_tmp_regular_file_bytes=${maximum_tmp_regular_file_bytes}"
    echo "pre_attempt_cuwacunu_available_bytes=${observed_cuwacunu}"
    echo "pre_attempt_root_available_bytes=${observed_root}"
    echo "pre_attempt_cuwacunu_device=${observed_cuwacunu_device}"
    echo "pre_attempt_root_device=${observed_root_device}"
    echo "pre_attempt_tmp_device=${observed_tmp_device}"
    echo "pre_attempt_tmp_same_as_root=${observed_tmp_same}"
    echo "pre_attempt_tmp_oversize_regular_file_count=${observed_tmp_count}"
    echo "pre_attempt_resource_gate_pass=true"
    if ((index == 3)); then
      echo "time_only_authority_kind=retry2_completed_prefix_import"
      echo "time_only_historical_optimizer_steps=3000"
      echo "time_only_retry3_optimizer_steps=0"
      echo "time_only_retry3_training_job_created=false"
    elif ((index == 4)); then
      echo "source_retry2_terminal_stage04_attempt_sha256=${expected_retry2_stage04_attempt_sha256}"
      echo "source_retry2_partial_payload_reuse_authorized=false"
      echo "retry3_optimizer_start_step=0"
      echo "retry3_requested_optimizer_steps=${expected_steps}"
      echo "input_representation_checkpoint_path=none"
      echo "input_mdn_checkpoint_path=none"
    fi
    echo "attempt_without_completion_terminal=true"
    echo "partial_payload_adoption_authorized=false"
    echo "checkpoint_resume_authorized=false"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

emit_stage_completion() {
  local index="$1" destination="$2" tag attempt
  local observed_cuwacunu="${3:-${last_gate_cuwacunu_available_bytes:-}}"
  local observed_root="${4:-${last_gate_root_available_bytes:-}}"
  local observed_cuwacunu_device="${5:-${last_gate_cuwacunu_device:-}}"
  local observed_root_device="${6:-${last_gate_root_device:-}}"
  local observed_tmp_device="${7:-${last_gate_tmp_device:-}}"
  local observed_tmp_same="${8:-${last_gate_tmp_same_as_root:-}}"
  local observed_tmp_count="${9:-${last_gate_tmp_oversize_regular_file_count:-}}"
  [[ "${observed_cuwacunu}" =~ ^[0-9]+$ && \
    "${observed_root}" =~ ^[0-9]+$ && \
    "${observed_cuwacunu_device}" =~ ^[0-9]+$ && \
    "${observed_root_device}" =~ ^[0-9]+$ && \
    "${observed_tmp_device}" =~ ^[0-9]+$ && \
    "${observed_tmp_count}" == 0 ]] ||
    fail "invalid post-stage resource observation"
  [[ "${observed_tmp_same}" == true || "${observed_tmp_same}" == false ]] ||
    fail "invalid post-stage /tmp filesystem observation"
  tag="$(stage_tag "${index}")"
  attempt="$(stage_attempt_path "${index}")"
  {
    echo "schema_id=${schema_id}.development_stage_completion.v1"
    echo "status=complete"
    echo "stage_ordinal=${tag}"
    echo "stage_name=${development_stage_names[${index}]}"
    stage_previous_binding "${index}"
    echo "stage_attempt_path=${attempt}"
    echo "stage_attempt_sha256=$(sha256_of "${attempt}")"
    emit_stage_primary_bindings "${index}"
    emit_ablation_runner_bindings
    echo "one_fixed_stage_this_invocation=true"
    echo "pre_completion_semantic_verification_pass=true"
    echo "post_stage_cuwacunu_available_bytes=${observed_cuwacunu}"
    echo "post_stage_root_available_bytes=${observed_root}"
    echo "post_stage_cuwacunu_device=${observed_cuwacunu_device}"
    echo "post_stage_root_device=${observed_root_device}"
    echo "post_stage_tmp_device=${observed_tmp_device}"
    echo "post_stage_tmp_same_as_root=${observed_tmp_same}"
    echo "post_stage_tmp_oversize_regular_file_count=${observed_tmp_count}"
    echo "post_stage_resource_gate_pass=true"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_stage_attempt_receipt() {
  local index="$1" path candidate
  local runtime_root_inode runtime_root_device lock_inode lock_device
  path="$(stage_attempt_path "${index}")"
  require_immutable_file "${path}"
  expect_mode_owner_links "${path}" 444 "development stage attempt"
  verify_runtime_root_and_lock_metadata
  expect_kv "${path}" runtime_root_path "${runtime_root}"
  expect_kv "${path}" runtime_root_mode 700
  expect_kv "${path}" runtime_root_owner_uid "${process_owner_uid}"
  runtime_root_inode="$(stat -c '%i' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root inode during attempt verification"
  runtime_root_device="$(stat -c '%d' -- "${runtime_root}")" ||
    fail "could not read retry2 runtime-root device during attempt verification"
  lock_inode="$(stat -c '%i' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock inode during attempt verification"
  lock_device="$(stat -c '%d' -- "${runtime_development_lock}")" ||
    fail "could not read retry2 development-lock device during attempt verification"
  [[ "${runtime_root_inode}" =~ ^[0-9]+$ && \
    "${runtime_root_device}" =~ ^[0-9]+$ && "${lock_inode}" =~ ^[0-9]+$ && \
    "${lock_device}" =~ ^[0-9]+$ ]] ||
    fail "retry2 stage-attempt identity metadata is malformed"
  expect_kv "${path}" runtime_root_inode "${runtime_root_inode}"
  expect_kv "${path}" runtime_root_device "${runtime_root_device}"
  expect_kv "${path}" development_lock_path "${runtime_development_lock}"
  expect_kv "${path}" development_lock_mode 600
  expect_kv "${path}" development_lock_owner_uid "${process_owner_uid}"
  expect_kv "${path}" development_lock_links 1
  expect_kv "${path}" development_lock_bytes 0
  expect_kv "${path}" development_lock_inode "${lock_inode}"
  expect_kv "${path}" development_lock_device "${lock_device}"
  (( $(kv pre_attempt_cuwacunu_available_bytes "${path}") >=
    minimum_cuwacunu_available_bytes )) ||
    fail "stage attempt records a failed /cuwacunu space gate"
  (( $(kv pre_attempt_root_available_bytes "${path}") >=
    minimum_root_available_bytes )) ||
    fail "stage attempt records a failed / space gate"
  expect_kv "${path}" pre_attempt_tmp_oversize_regular_file_count 0
  expect_kv "${path}" pre_attempt_resource_gate_pass true
  candidate="$(mktemp "${scratch_root}/${schema_id}.stage_attempt_verify.XXXXXX")"
  emit_stage_attempt "${index}" "${candidate}" \
    "$(kv pre_attempt_cuwacunu_available_bytes "${path}")" \
    "$(kv pre_attempt_root_available_bytes "${path}")" \
    "$(kv pre_attempt_cuwacunu_device "${path}")" \
    "$(kv pre_attempt_root_device "${path}")" \
    "$(kv pre_attempt_tmp_device "${path}")" \
    "$(kv pre_attempt_tmp_same_as_root "${path}")" \
    "$(kv pre_attempt_tmp_oversize_regular_file_count "${path}")"
  cmp -s -- "${candidate}" "${path}" || {
    rm -f -- "${candidate}"
    fail "stage attempt receipt drifted: ${path}"
  }
  rm -f -- "${candidate}"
}

verify_stage_completion_receipt() {
  local index="$1" path candidate
  path="$(stage_completion_path "${index}")"
  require_immutable_file "${path}"
  expect_mode_owner_links "${path}" 444 "development stage completion"
  (( $(kv post_stage_cuwacunu_available_bytes "${path}") >=
    minimum_cuwacunu_available_bytes )) ||
    fail "stage completion records a failed /cuwacunu space gate"
  (( $(kv post_stage_root_available_bytes "${path}") >=
    minimum_root_available_bytes )) ||
    fail "stage completion records a failed / space gate"
  expect_kv "${path}" post_stage_tmp_oversize_regular_file_count 0
  expect_kv "${path}" post_stage_resource_gate_pass true
  expect_kv "${path}" pre_completion_semantic_verification_pass true
  verify_stage_attempt_receipt "${index}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.stage_status_verify.XXXXXX")"
  emit_stage_completion "${index}" "${candidate}" \
    "$(kv post_stage_cuwacunu_available_bytes "${path}")" \
    "$(kv post_stage_root_available_bytes "${path}")" \
    "$(kv post_stage_cuwacunu_device "${path}")" \
    "$(kv post_stage_root_device "${path}")" \
    "$(kv post_stage_tmp_device "${path}")" \
    "$(kv post_stage_tmp_same_as_root "${path}")" \
    "$(kv post_stage_tmp_oversize_regular_file_count "${path}")"
  cmp -s -- "${candidate}" "${path}" || {
    rm -f -- "${candidate}"
    fail "stage completion receipt drifted: ${path}"
  }
  rm -f -- "${candidate}"
}

# The inherited receipt fields use this name. In retry2 it is the completed
# initialization stage, not a one-shot whole-development attempt.
verify_retry_attempt_sentinel() {
  [[ "${retry_attempt_sentinel}" == "$(stage_completion_path 0)" ]] ||
    fail "retry2 initialization-stage alias drifted"
  verify_stage_completion_receipt 0
}

publish_stage_attempt() {
  local index="$1" path candidate
  assert_runtime_publication_ready
  path="$(stage_attempt_path "${index}")"
  path_is_absent "${path}" || fail "stage attempt already exists: ${path}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.stage_attempt.XXXXXX")"
  emit_stage_attempt "${index}" "${candidate}"
  publish_immutable "${candidate}" "${path}"
  verify_stage_attempt_receipt "${index}"
}

publish_stage_completion() {
  local index="$1" path candidate
  path="$(stage_completion_path "${index}")"
  path_is_absent "${path}" || fail "stage completion already exists: ${path}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.stage_status.XXXXXX")"
  emit_stage_completion "${index}" "${candidate}"
  publish_immutable "${candidate}" "${path}"
  verify_stage_completion_receipt "${index}"
}

assert_stage_payload_absent() {
  local index="$1" path
  case "${index}" in
  0)
    assert_runtime_root_only_lock
    ;;
  1)
    for path in "${canonical_import_receipt}" \
      "$(arm_root canonical)" \
      "$(arm_root canonical)/affine" \
      "$(arm_main_report canonical)" "$(arm_replay_report canonical)"; do
      path_is_absent "${path}" || fail "canonical-import payload predates attempt"
    done
    ;;
  2)
    for path in "${endpoint_imports_root}" "${endpoint_import_root}" \
      "$(arm_root endpoint_scale)/training" \
      "$(arm_train_job endpoint_scale)" \
      "$(arm_training_status endpoint_scale)" \
      "$(arm_root endpoint_scale)/training.log"; do
      path_is_absent "${path}" ||
        fail "endpoint-import or forbidden endpoint-training payload predates attempt: ${path}"
    done
    ;;
  3)
    for path in "${time_only_import_root}" \
      "${time_only_import_checkpoint}" "${time_only_import_receipt}" \
      "$(arm_root time_only)/training" \
      "$(arm_train_job time_only)" \
      "$(arm_training_status time_only)" \
      "$(arm_root time_only)/training.log"; do
      path_is_absent "${path}" ||
        fail "time-only import or forbidden training payload predates its Retry3 stage attempt: ${path}"
    done
    ;;
  4)
    local arm=no_tf_alignment
    for path in "$(arm_root "${arm}")/training" \
      "$(arm_train_job "${arm}")" \
      "$(arm_training_status "${arm}")" "$(arm_root "${arm}")/training.log"; do
      path_is_absent "${path}" || fail "${arm} training payload predates attempt"
    done
    ;;
  5 | 6 | 7)
    local arm
    case "${index}" in
    5) arm=endpoint_scale ;;
    6) arm=time_only ;;
    7) arm=no_tf_alignment ;;
    esac
    for path in "$(arm_capture_status "${arm}")" \
      "$(arm_root "${arm}")/capture"; do
      path_is_absent "${path}" || fail "${arm} capture payload predates attempt"
    done
    ;;
  8 | 9 | 10)
    local arm
    case "${index}" in
    8) arm=endpoint_scale ;;
    9) arm=time_only ;;
    10) arm=no_tf_alignment ;;
    esac
    for path in "$(arm_affine_status "${arm}")" \
      "$(arm_root "${arm}")/affine"; do
      path_is_absent "${path}" || fail "${arm} affine payload predates attempt"
    done
    ;;
  11)
    for path in "${selection_receipt}" "${development_receipt}"; do
      path_is_absent "${path}" ||
        fail "selection/development payload predates attempt"
    done
    ;;
  *) fail "invalid stage index: ${index}" ;;
  esac
}

verify_stage_artifacts() {
  local index="$1" arm
  case "${index}" in
  0)
    verify_base_training_config
    verify_frozen_sources
    for arm in "${challenger_arms[@]}"; do verify_arm_files "${arm}"; done
    verify_effective_grammar_closure
    verify_config_closure
    verify_inputs
    verify_endpoint_bundle_scientific_equivalence
    ;;
  1) verify_canonical_import ;;
  2) verify_endpoint_import >/dev/null ;;
  3)
    verify_time_only_import >/dev/null
    ;;
  4)
    verify_training_status no_tf_alignment >/dev/null
    verify_success_log "$(arm_root no_tf_alignment)/training.log"
    ;;
  5)
    verify_capture_status endpoint_scale
    verify_success_log "$(arm_root endpoint_scale)/capture/train.log"
    verify_success_log "$(arm_root endpoint_scale)/capture/validation.log"
    ;;
  6)
    verify_capture_status time_only
    verify_success_log "$(arm_root time_only)/capture/train.log"
    verify_success_log "$(arm_root time_only)/capture/validation.log"
    ;;
  7)
    verify_capture_status no_tf_alignment
    verify_success_log "$(arm_root no_tf_alignment)/capture/train.log"
    verify_success_log "$(arm_root no_tf_alignment)/capture/validation.log"
    ;;
  8) verify_affine_status endpoint_scale ;;
  9) verify_affine_status time_only ;;
  10) verify_affine_status no_tf_alignment ;;
  11)
    verify_selection
    verify_development_receipt
    audit_development_job_set
    if path_is_absent "${certified_attempt}"; then
      assert_no_local_certified_artifacts
    else
      # After certification begins, replace the historical absence check with
      # the immutable transitive proof that the attempt binds all 12 stages.
      verify_certified_attempt
    fi
    ;;
  *) fail "invalid stage index: ${index}" ;;
  esac
}

verify_completed_stage_prefix() {
  local index="$1" cursor
  for ((cursor = 0; cursor < index; ++cursor)); do
    verify_stage_completion_receipt "${cursor}"
    verify_stage_artifacts "${cursor}"
  done
}

verify_all_development_stage_chain_receipts() {
  local index
  for ((index = 0; index < development_stage_count; ++index)); do
    verify_stage_completion_receipt "${index}"
  done
}

find_next_development_stage() {
  local index attempt completion later
  next_development_stage=""
  for ((index = 0; index < development_stage_count; ++index)); do
    attempt="$(stage_attempt_path "${index}")"
    completion="$(stage_completion_path "${index}")"
    if ! path_is_absent "${completion}"; then
      ! path_is_absent "${attempt}" ||
        fail "stage completion exists without attempt: ${completion}"
      verify_stage_completion_receipt "${index}"
      verify_stage_artifacts "${index}"
      continue
    fi
    path_is_absent "${attempt}" ||
      fail "stage attempt has no completion and is terminal: ${attempt}"
    for ((later = index + 1; later < development_stage_count; ++later)); do
      path_is_absent "$(stage_attempt_path "${later}")" && \
        path_is_absent "$(stage_completion_path "${later}")" ||
        fail "later stage marker exists before stage ${index}"
    done
    next_development_stage="${index}"
    return
  done
  next_development_stage="${development_stage_count}"
}

seal_success_log() {
  local path="$1"
  require_owned_single_link_file "${path}" "successful stage log"
  chmod 0444 -- "${path}"
  [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
    fail "successful stage log was not sealed: ${path}"
}

verify_success_log() {
  local path="$1"
  require_file "${path}"
  [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
    fail "successful stage log is writable or has wrong mode: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "successful stage log has an external hard link: ${path}"
}

run_one_development_stage() {
  local index="$1"
  case "${index}" in
  0)
    prepare_runtime_scratch
    write_base_training_config
    assert_no_local_certified_artifacts
    canonical_inputs
    freeze_sources
    generate_all_arm_files
    verify_endpoint_bundle_scientific_equivalence
    write_effective_grammar_closure
    verify_effective_grammar_closure
    write_config_closure
    write_inputs
    verify_config_closure
    verify_inputs
    ;;
  1) import_canonical_arm ;;
  2) run_endpoint_import ;;
  3)
    run_time_only_import
    ;;
  4)
    run_training_arm no_tf_alignment
    seal_success_log "$(arm_root no_tf_alignment)/training.log"
    ;;
  5)
    run_capture_arm endpoint_scale
    seal_success_log "$(arm_root endpoint_scale)/capture/train.log"
    seal_success_log "$(arm_root endpoint_scale)/capture/validation.log"
    ;;
  6)
    run_capture_arm time_only
    seal_success_log "$(arm_root time_only)/capture/train.log"
    seal_success_log "$(arm_root time_only)/capture/validation.log"
    ;;
  7)
    run_capture_arm no_tf_alignment
    seal_success_log "$(arm_root no_tf_alignment)/capture/train.log"
    seal_success_log "$(arm_root no_tf_alignment)/capture/validation.log"
    ;;
  8) run_affine_arm endpoint_scale ;;
  9) run_affine_arm time_only ;;
  10) run_affine_arm no_tf_alignment ;;
  11)
    write_selection
    assert_no_local_certified_artifacts
    write_development_receipt
    assert_no_local_certified_artifacts
    ;;
  *) fail "invalid development stage index: ${index}" ;;
  esac
}

advance_development_once() {
  assert_operational_runner_identity
  assert_runner_bootstrap_lock
  resource_safety_gate

  local preexisting_root=false index cursor
  if ! path_is_absent "${runtime_root}"; then
    preexisting_root=true
  fi
  if [[ "${preexisting_root}" == false ]]; then
    prepare_bootstrap_scratch
    # This is the single authoritative full preflight for this consuming
    # stage-00 invocation. It is read-only with respect to retry2 scientific
    # state and precedes root, lock-file, and stage-attempt publication.
    preflight_read_only
    assert_directory_empty "${bootstrap_scratch_root}" "bootstrap scratch"
    acquire_or_create_runtime_lock
    find_next_development_stage
    index="${next_development_stage}"
  else
    acquire_or_create_runtime_lock
    if ! path_is_absent "$(stage_completion_path 0)"; then
      prepare_runtime_scratch
      # Shell-local canonical path bindings are not persisted in receipts.
      # Re-establish and verify them before completed-prefix reproduction.
      canonical_inputs
    else
      prepare_bootstrap_scratch
    fi
    find_next_development_stage
    index="${next_development_stage}"
  fi
  [[ "${index}" =~ ^[0-9]+$ ]] ||
    fail "completed-prefix scan did not return a stage index"
  assert_directory_empty "${scratch_root}" "completed-prefix scratch"
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  if ((index == development_stage_count)); then
    verify_all_development_stage_chain_receipts
    if path_is_absent "${certified_attempt}"; then
      assert_no_local_certified_artifacts
    else
      verify_certified_attempt
    fi
    resource_safety_gate
    echo "retry2 development is already complete"
    return
  fi

  if ((index == 0)); then
    # A prior interruption may have left exactly root+lock before the attempt.
    # Re-run the read-only full preflight; that bootstrap does not consume the
    # scientific attempt.
    if [[ "${preexisting_root}" == true ]]; then
      preflight_read_only
      assert_directory_empty "${bootstrap_scratch_root}" "bootstrap scratch"
    fi
    # Root plus the single empty lock file is the complete allowed operational
    # bootstrap state. The runtime scratch directory is created only after the
    # stage-00 attempt is durable.
    assert_runtime_root_only_lock
  else
    [[ "${scratch_root}" == "${runtime_root}/scratch" ]] ||
      fail "completed-prefix verification did not use retry2 runtime scratch"
  fi

  for ((cursor = index; cursor < development_stage_count; ++cursor)); do
    if ((cursor == 0)); then
      continue
    fi
    assert_stage_payload_absent "${cursor}"
  done
  assert_no_local_certified_artifacts
  resource_safety_gate
  verify_runtime_root_and_lock_metadata
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  publish_stage_attempt "${index}"
  run_one_development_stage "${index}"
  resource_safety_gate
  assert_no_local_certified_artifacts
  verify_stage_artifacts "${index}"
  assert_directory_empty "${scratch_root}" "stage semantic-verification scratch"
  resource_safety_gate
  verify_runtime_root_and_lock_metadata
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  publish_stage_completion "${index}"
  verify_stage_artifacts "${index}"
  assert_directory_empty "${scratch_root}" "stage completion-verification scratch"
  verify_runtime_root_and_lock_metadata
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  resource_safety_gate
  echo "completed retry2 development stage $(stage_tag "${index}") ${development_stage_names[${index}]}"
}

preflight_without_attempt() {
  assert_runner_bootstrap_lock
  prepare_bootstrap_scratch
  resource_safety_gate
  preflight_read_only
  assert_directory_empty "${bootstrap_scratch_root}" "bootstrap scratch"
  resource_safety_gate
}

# ---------------------------------------------------------------------------
# Retry3 recovery boundary.
#
# These definitions intentionally override the inherited Retry2 authority
# emitters at runtime.  No Retry2 receipt is ever reproduced with Retry3's
# current runner binding.  Historical claims are accepted only through the
# fixed-hash interruption closure and its separately sealed completed-prefix
# bundle.

retry2_completed_prefix_expected_files() {
  cat <<'FILES'
completed_prefix_bundle.status
directories.inventory.tsv
frozen_sources/seal_and_verify_representation_ablation_retry2_completed_prefix_bundle_for_retry3_v1.sh
regular_files.inventory.tsv
completed_prefix/operational_authority/run_representation_ablation_v2_retry2.sh
completed_prefix/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config
completed_prefix/effective_grammar_closure.status
completed_prefix/config_inputs.status
completed_prefix/inputs.status
completed_prefix/frozen_sources/run_representation_ablation_v2_retry2.sh
completed_prefix/frozen_sources/frozen_representation_affine_probe.cpp
completed_prefix/frozen_sources/frozen_representation_affine_probe
completed_prefix/arms/endpoint_scale/config/capture.config
completed_prefix/arms/endpoint_scale/config/representation.jkimyei
completed_prefix/arms/endpoint_scale/config/representation.net
completed_prefix/arms/endpoint_scale/config/train.config
completed_prefix/arms/time_only/config/capture.config
completed_prefix/arms/time_only/config/representation.jkimyei
completed_prefix/arms/time_only/config/representation.net
completed_prefix/arms/time_only/config/train.config
completed_prefix/arms/no_tf_alignment/config/capture.config
completed_prefix/arms/no_tf_alignment/config/representation.jkimyei
completed_prefix/arms/no_tf_alignment/config/representation.net
completed_prefix/arms/no_tf_alignment/config/train.config
completed_prefix/arms/canonical/affine/main.report
completed_prefix/arms/canonical/affine/replay.report
completed_prefix/arms/canonical/import.status
completed_prefix/imports/retry1_endpoint_v1/channel_representation.report.mtf_jepa_mae_vicreg.pt
completed_prefix/imports/retry1_endpoint_v1/endpoint_import.status
completed_prefix/imports/retry1_endpoint_v1/source_endpoint_import_bundle.status
completed_prefix/arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c/component_spawn.ref
completed_prefix/arms/time_only/training/job/channel_representation.report
completed_prefix/arms/time_only/training/job/channel_representation.report.mtf_jepa_mae_vicreg.pt
completed_prefix/arms/time_only/training/job/job.manifest
completed_prefix/arms/time_only/training/job/job.state
completed_prefix/arms/time_only/training/job/lattice.checkpoint.fact
completed_prefix/arms/time_only/training/job/lattice.exposure.fact
completed_prefix/arms/time_only/training/job/lattice.source_analytics.fact
completed_prefix/arms/time_only/training/job/runtime.checkpoint_io.fact
completed_prefix/arms/time_only/training/job/runtime.component_training_update.fact
completed_prefix/arms/time_only/training/job/runtime.health_measurement.fact
completed_prefix/arms/time_only/training/job/runtime.job_events.probe
completed_prefix/arms/time_only/training/job/runtime.result.fact
completed_prefix/arms/time_only/training/system/component_spawn_registry.v1.lls
completed_prefix/arms/time_only/training/system/runtime_layout.v1.lls
completed_prefix/arms/time_only/training.log
completed_prefix/arms/time_only/training.status
completed_prefix/stage.00.initialize.attempt.status
completed_prefix/stage.00.initialize.status
completed_prefix/stage.01.canonical_import.attempt.status
completed_prefix/stage.01.canonical_import.status
completed_prefix/stage.02.endpoint_import.attempt.status
completed_prefix/stage.02.endpoint_import.status
completed_prefix/stage.03.time_only_training.attempt.status
completed_prefix/stage.03.time_only_training.status
FILES
}

retry2_completed_prefix_expected_directories() {
  cat <<'DIRECTORIES'
.
./completed_prefix
./completed_prefix/operational_authority
./completed_prefix/frozen_sources
./completed_prefix/arms
./completed_prefix/arms/endpoint_scale
./completed_prefix/arms/endpoint_scale/config
./completed_prefix/arms/time_only
./completed_prefix/arms/time_only/config
./completed_prefix/arms/time_only/training
./completed_prefix/arms/time_only/training/components
./completed_prefix/arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg
./completed_prefix/arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns
./completed_prefix/arms/time_only/training/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/lws_ca75885f5cadac7c
./completed_prefix/arms/time_only/training/job
./completed_prefix/arms/time_only/training/system
./completed_prefix/arms/no_tf_alignment
./completed_prefix/arms/no_tf_alignment/config
./completed_prefix/arms/canonical
./completed_prefix/arms/canonical/affine
./completed_prefix/imports
./completed_prefix/imports/retry1_endpoint_v1
./frozen_sources
DIRECTORIES
}

assert_retry3_recovery_pins_sealed() {
  local label pin
  while IFS='=' read -r label pin; do
    [[ "${pin}" =~ ^[0-9a-f]{64}$ && \
      "${pin}" != "${unsealed_authority_sha256}" ]] ||
      fail "Retry3 recovery authority is not sealed: ${label}"
  done <<PINS
stage04_interruption_receipt=${expected_retry2_stage04_interruption_closure_receipt_sha256}
stage04_interruption_regular_inventory=${expected_retry2_stage04_interruption_regular_inventory_sha256}
stage04_interruption_directory_inventory=${expected_retry2_stage04_interruption_directory_inventory_sha256}
stage04_interruption_amendment=${expected_retry2_stage04_interruption_amendment_sha256}
stage04_interruption_sealer=${expected_retry2_stage04_interruption_sealer_sha256}
completed_prefix_receipt=${expected_retry2_completed_prefix_bundle_receipt_sha256}
completed_prefix_regular_inventory=${expected_retry2_completed_prefix_regular_inventory_sha256}
completed_prefix_directory_inventory=${expected_retry2_completed_prefix_directory_inventory_sha256}
completed_prefix_sealer=${expected_retry2_completed_prefix_sealer_sha256}
PINS
}

validate_retry3_inventory_relative_path() {
  local relative_path="$1" allow_dot="${2:-false}"
  [[ -n "${relative_path}" && "${relative_path}" != /* && \
    "${relative_path}" != *$'\n'* && "${relative_path}" != *$'\r'* && \
    "${relative_path}" != *$'\t'* ]] ||
    fail "unsafe Retry3 recovery inventory relative path: ${relative_path}"
  if [[ "${relative_path}" == . ]]; then
    [[ "${allow_dot}" == true ]] ||
      fail "dot is forbidden for a regular-file inventory path"
    return
  fi
  [[ "/${relative_path}/" != *'/../'* && \
    "/${relative_path}/" != *'/./'* && \
    "/${relative_path}/" != *'//'* ]] ||
    fail "non-canonical Retry3 recovery inventory relative path: ${relative_path}"
}

verify_retry2_stage04_interruption_closure_inventory_tree() {
  local regular_header directory_header regular_fd directory_fd
  local relative_path source_mode source_uid source_gid source_links
  local source_bytes source_inode source_device source_mtime source_sha256
  local snapshot_mode snapshot_uid snapshot_gid snapshot_links snapshot_bytes
  local snapshot_inode snapshot_device snapshot_mtime snapshot_sha256
  local inodes_distinct bytes_identical path actual_metadata
  local regular_count=0 directory_count=0 regular_bytes=0
  local actual_files actual_directories total_files total_directories
  local snapshot_content_sha256 special
  declare -A seen_regular_paths=() seen_directory_paths=()

  IFS= read -r regular_header \
    <"${retry2_stage04_interruption_regular_inventory}" ||
    fail "could not read Retry2 interruption regular inventory header"
  [[ "${regular_header}" == $'relative_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_mtime\tsource_sha256\tsnapshot_mode\tsnapshot_uid\tsnapshot_gid\tsnapshot_links\tsnapshot_bytes\tsnapshot_inode\tsnapshot_device\tsnapshot_mtime\tsnapshot_sha256\tinodes_distinct\tbytes_identical' ]] ||
    fail "Retry2 interruption regular inventory header drifted"
  awk -F '\t' 'NR == 1 { next } NF != 21 { exit 42 }
    END { if (NR != 61) exit 43 }' \
    "${retry2_stage04_interruption_regular_inventory}" ||
    fail "Retry2 interruption regular inventory shape drifted"
  exec {regular_fd}<"${retry2_stage04_interruption_regular_inventory}" ||
    fail "could not open Retry2 interruption regular inventory"
  IFS= read -r regular_header <&${regular_fd} ||
    fail "could not consume Retry2 interruption regular inventory header"
  while IFS=$'\t' read -r relative_path source_mode source_uid source_gid \
    source_links source_bytes source_inode source_device source_mtime \
    source_sha256 snapshot_mode snapshot_uid snapshot_gid snapshot_links \
    snapshot_bytes snapshot_inode snapshot_device snapshot_mtime \
    snapshot_sha256 inodes_distinct bytes_identical <&${regular_fd}; do
    validate_retry3_inventory_relative_path "${relative_path}"
    [[ -z "${seen_regular_paths[${relative_path}]+x}" ]] ||
      fail "duplicate Retry2 interruption regular inventory path: ${relative_path}"
    seen_regular_paths["${relative_path}"]=1
    [[ "${source_mode}" =~ ^[0-7]{3,4}$ && \
      "${source_uid}" =~ ^[0-9]+$ && "${source_gid}" =~ ^[0-9]+$ && \
      "${source_links}" =~ ^[0-9]+$ && "${source_bytes}" =~ ^[0-9]+$ && \
      "${source_inode}" =~ ^[0-9]+$ && "${source_device}" =~ ^[0-9]+$ && \
      -n "${source_mtime}" && "${source_sha256}" =~ ^[0-9a-f]{64}$ && \
      "${snapshot_mode}" =~ ^[0-7]{3,4}$ && \
      "${snapshot_uid}" =~ ^[0-9]+$ && "${snapshot_gid}" =~ ^[0-9]+$ && \
      "${snapshot_links}" =~ ^[0-9]+$ && \
      "${snapshot_bytes}" =~ ^[0-9]+$ && \
      "${snapshot_inode}" =~ ^[0-9]+$ && \
      "${snapshot_device}" =~ ^[0-9]+$ && -n "${snapshot_mtime}" && \
      "${snapshot_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
      fail "malformed Retry2 interruption regular inventory row: ${relative_path}"
    [[ "${source_uid}:${source_gid}:${source_links}" == \
      "${process_owner_uid}:${process_owner_gid}:1" && \
      "${snapshot_mode}:${snapshot_uid}:${snapshot_gid}:${snapshot_links}" == \
      "444:${process_owner_uid}:${process_owner_gid}:1" && \
      "${source_bytes}" == "${snapshot_bytes}" && \
      "${source_sha256}" == "${snapshot_sha256}" && \
      "${source_device}:${source_inode}" != \
      "${snapshot_device}:${snapshot_inode}" && \
      "${inodes_distinct}" == true && "${bytes_identical}" == true ]] ||
      fail "Retry2 interruption regular inventory semantics drifted: ${relative_path}"
    path="${retry2_stage04_interruption_snapshot}/${relative_path}"
    require_contained_path "${path}" "${retry2_stage04_interruption_snapshot}"
    require_file "${path}"
    actual_metadata="$(stat -c '%a:%u:%g:%h:%s:%i:%d:%y' -- "${path}")"
    [[ "${actual_metadata}" == \
      "${snapshot_mode}:${snapshot_uid}:${snapshot_gid}:${snapshot_links}:${snapshot_bytes}:${snapshot_inode}:${snapshot_device}:${snapshot_mtime}" ]] ||
      fail "Retry2 interruption snapshot file metadata differs from inventory: ${relative_path}"
    [[ "$(sha256_of "${path}")" == "${snapshot_sha256}" ]] ||
      fail "Retry2 interruption snapshot file hash differs from inventory: ${relative_path}"
    regular_count=$((regular_count + 1))
    regular_bytes=$((regular_bytes + snapshot_bytes))
  done
  exec {regular_fd}<&-
  [[ "${regular_count}" == 60 && "${regular_bytes}" == 58518408 ]] ||
    fail "Retry2 interruption snapshot regular-file aggregate drifted"

  IFS= read -r directory_header \
    <"${retry2_stage04_interruption_directory_inventory}" ||
    fail "could not read Retry2 interruption directory inventory header"
  [[ "${directory_header}" == $'relative_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_mtime\tsnapshot_mode\tsnapshot_uid\tsnapshot_gid\tsnapshot_links\tsnapshot_bytes\tsnapshot_inode\tsnapshot_device\tsnapshot_mtime\tinodes_distinct' ]] ||
    fail "Retry2 interruption directory inventory header drifted"
  awk -F '\t' 'NR == 1 { next } NF != 18 { exit 42 }
    END { if (NR != 29) exit 43 }' \
    "${retry2_stage04_interruption_directory_inventory}" ||
    fail "Retry2 interruption directory inventory shape drifted"
  exec {directory_fd}<"${retry2_stage04_interruption_directory_inventory}" ||
    fail "could not open Retry2 interruption directory inventory"
  IFS= read -r directory_header <&${directory_fd} ||
    fail "could not consume Retry2 interruption directory inventory header"
  while IFS=$'\t' read -r relative_path source_mode source_uid source_gid \
    source_links source_bytes source_inode source_device source_mtime \
    snapshot_mode snapshot_uid snapshot_gid snapshot_links snapshot_bytes \
    snapshot_inode snapshot_device snapshot_mtime inodes_distinct \
    <&${directory_fd}; do
    validate_retry3_inventory_relative_path "${relative_path}" true
    [[ -z "${seen_directory_paths[${relative_path}]+x}" ]] ||
      fail "duplicate Retry2 interruption directory inventory path: ${relative_path}"
    seen_directory_paths["${relative_path}"]=1
    [[ "${source_mode}" =~ ^[0-7]{3,4}$ && \
      "${source_uid}" =~ ^[0-9]+$ && "${source_gid}" =~ ^[0-9]+$ && \
      "${source_links}" =~ ^[0-9]+$ && "${source_bytes}" =~ ^[0-9]+$ && \
      "${source_inode}" =~ ^[0-9]+$ && "${source_device}" =~ ^[0-9]+$ && \
      -n "${source_mtime}" && "${snapshot_mode}" =~ ^[0-7]{3,4}$ && \
      "${snapshot_uid}" =~ ^[0-9]+$ && "${snapshot_gid}" =~ ^[0-9]+$ && \
      "${snapshot_links}" =~ ^[0-9]+$ && \
      "${snapshot_bytes}" =~ ^[0-9]+$ && \
      "${snapshot_inode}" =~ ^[0-9]+$ && \
      "${snapshot_device}" =~ ^[0-9]+$ && -n "${snapshot_mtime}" ]] ||
      fail "malformed Retry2 interruption directory inventory row: ${relative_path}"
    [[ "${source_uid}:${source_gid}:${source_links}" == \
      "${process_owner_uid}:${process_owner_gid}:1" && \
      "${snapshot_mode}:${snapshot_uid}:${snapshot_gid}:${snapshot_links}" == \
      "555:${process_owner_uid}:${process_owner_gid}:1" && \
      "${source_device}:${source_inode}" != \
      "${snapshot_device}:${snapshot_inode}" && \
      "${inodes_distinct}" == true ]] ||
      fail "Retry2 interruption directory inventory semantics drifted: ${relative_path}"
    if [[ "${relative_path}" == . ]]; then
      path="${retry2_stage04_interruption_snapshot}"
    else
      path="${retry2_stage04_interruption_snapshot}/${relative_path}"
    fi
    require_contained_path "${path}" "${retry2_stage04_interruption_snapshot}"
    require_dir "${path}"
    actual_metadata="$(stat -c '%a:%u:%g:%h:%s:%i:%d:%y' -- "${path}")"
    [[ "${actual_metadata}" == \
      "${snapshot_mode}:${snapshot_uid}:${snapshot_gid}:${snapshot_links}:${snapshot_bytes}:${snapshot_inode}:${snapshot_device}:${snapshot_mtime}" ]] ||
      fail "Retry2 interruption snapshot directory metadata differs from inventory: ${relative_path}"
    directory_count=$((directory_count + 1))
  done
  exec {directory_fd}<&-
  [[ "${directory_count}" == 28 ]] ||
    fail "Retry2 interruption snapshot directory aggregate drifted"

  special="$(find "${retry2_stage04_interruption_snapshot}" -xdev \
    ! -type f ! -type d -print -quit)" ||
    fail "could not scan Retry2 interruption snapshot entry types"
  [[ -z "${special}" ]] ||
    fail "Retry2 interruption snapshot contains a special entry: ${special}"
  actual_files="$(find "${retry2_stage04_interruption_snapshot}" -xdev \
    -type f -printf x | wc -c)" ||
    fail "could not count Retry2 interruption snapshot files"
  actual_directories="$(find "${retry2_stage04_interruption_snapshot}" -xdev \
    -type d -printf x | wc -c)" ||
    fail "could not count Retry2 interruption snapshot directories"
  [[ "${actual_files}" == "${regular_count}" && \
    "${actual_directories}" == "${directory_count}" ]] ||
    fail "Retry2 interruption snapshot contains an uninventoryed entry"
  total_files="$(find "${retry2_stage04_interruption_closure}" -xdev \
    -type f -printf x | wc -c)" ||
    fail "could not count Retry2 interruption closure files"
  total_directories="$(find "${retry2_stage04_interruption_closure}" -xdev \
    -type d -printf x | wc -c)" ||
    fail "could not count Retry2 interruption closure directories"
  [[ "${total_files}" == 65 && "${total_directories}" == 30 ]] ||
    fail "Retry2 interruption closure exact full-tree count drifted"
  snapshot_content_sha256="$(
    cd "${retry2_stage04_interruption_snapshot}"
    find . -xdev -type f -print0 | LC_ALL=C sort -z |
      xargs -0 sha256sum | sha256sum | awk '{print $1}'
  )" || fail "could not recompute Retry2 interruption snapshot content inventory"
  [[ "${snapshot_content_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "Retry2 interruption snapshot content inventory is malformed"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_regular_file_count "${regular_count}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_directory_count "${directory_count}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_entry_count_below_root 87
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_regular_file_bytes "${regular_bytes}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_content_inventory_sha256 "${snapshot_content_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_content_inventory_sha256 "${snapshot_content_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_file_count "${regular_count}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_directory_count "${directory_count}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_file_bytes "${regular_bytes}"
}

verify_retry2_stage04_interruption_closure_authority() {
  local entries path special
  assert_retry3_recovery_pins_sealed
  verify_pinned_mode_file "${retry2_stage04_interruption_closure_receipt}" \
    "${expected_retry2_stage04_interruption_closure_receipt_sha256}" 444 \
    "Retry2 stage-04 interruption closure receipt"
  verify_pinned_mode_file "${retry2_stage04_interruption_regular_inventory}" \
    "${expected_retry2_stage04_interruption_regular_inventory_sha256}" 444 \
    "Retry2 stage-04 interruption regular inventory"
  verify_pinned_mode_file "${retry2_stage04_interruption_directory_inventory}" \
    "${expected_retry2_stage04_interruption_directory_inventory_sha256}" 444 \
    "Retry2 stage-04 interruption directory inventory"
  verify_pinned_mode_file "${retry2_stage04_interruption_live_amendment}" \
    "${expected_retry2_stage04_interruption_amendment_sha256}" 444 \
    "Retry2 stage-04 interruption amendment"
  verify_pinned_mode_file "${retry2_stage04_interruption_frozen_amendment}" \
    "${expected_retry2_stage04_interruption_amendment_sha256}" 444 \
    "frozen Retry2 stage-04 interruption amendment"
  verify_pinned_mode_file "${retry2_stage04_interruption_live_sealer}" \
    "${expected_retry2_stage04_interruption_sealer_sha256}" 555 \
    "Retry2 stage-04 interruption sealer"
  verify_pinned_mode_file "${retry2_stage04_interruption_frozen_sealer}" \
    "${expected_retry2_stage04_interruption_sealer_sha256}" 444 \
    "frozen Retry2 stage-04 interruption sealer"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" schema_id \
    "${retry2_stage04_interruption_closure_schema_id}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" status complete
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_schema_id "${retry2_schema_id}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_runtime_root "${retry2_runtime}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_runtime_mutated false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_regular_file_inventory_path \
    "${retry2_stage04_interruption_regular_inventory}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_regular_file_inventory_sha256 \
    "${expected_retry2_stage04_interruption_regular_inventory_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_directory_inventory_path \
    "${retry2_stage04_interruption_directory_inventory}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_directory_inventory_sha256 \
    "${expected_retry2_stage04_interruption_directory_inventory_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_path "${retry2_stage04_interruption_snapshot}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_files_byte_identical true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_files_distinct_inodes true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_files_single_link true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_regular_files_mode 0444
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_directories_mode 0555
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_symlink_count 0
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    source_snapshot_special_entry_count 0
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    interruption_sealer_path "${retry2_stage04_interruption_live_sealer}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    interruption_sealer_process_start_sha256 \
    "${expected_retry2_stage04_interruption_sealer_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    frozen_interruption_sealer_path "${retry2_stage04_interruption_frozen_sealer}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    frozen_interruption_sealer_sha256 \
    "${expected_retry2_stage04_interruption_sealer_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    interruption_amendment_path "${retry2_stage04_interruption_live_amendment}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    interruption_amendment_sha256 \
    "${expected_retry2_stage04_interruption_amendment_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    frozen_interruption_amendment_path "${retry2_stage04_interruption_frozen_amendment}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    frozen_interruption_amendment_sha256 \
    "${expected_retry2_stage04_interruption_amendment_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_00_attempt_sha256 "${expected_retry2_stage00_attempt_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_00_completion_sha256 "${expected_retry2_stage00_completion_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_01_attempt_sha256 "${expected_retry2_stage01_attempt_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_01_completion_sha256 "${expected_retry2_stage01_completion_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_02_attempt_sha256 "${expected_retry2_stage02_attempt_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_02_completion_sha256 "${expected_retry2_stage02_completion_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_03_attempt_sha256 "${expected_retry2_stage03_attempt_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_03_completion_sha256 "${expected_retry2_stage03_completion_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_04_attempt_sha256 "${expected_retry2_stage04_attempt_sha256}"
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_04_attempt_status consumed
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_04_scientific_attempt_consumed true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_04_completion_present false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    stage_04_attempt_without_completion_terminal true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    partial_payload_adoption_authorized false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    checkpoint_resume_authorized false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    same_runtime_reentry_authorized false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    completed_prefix_import_authorized_by_this_closure false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    recovery_requires_new_schema true
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    no_tf_alignment_restart_optimizer_step 0
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    certified_input_access false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" \
    final_holdout_access false
  expect_kv "${retry2_stage04_interruption_closure_receipt}" policy_access false
  require_dir "${retry2_stage04_interruption_snapshot}"
  special="$(find "${retry2_stage04_interruption_closure}" -xdev \
    ! -type f ! -type d -print -quit)" ||
    fail "could not scan Retry2 interruption closure entry types"
  [[ -z "${special}" ]] ||
    fail "Retry2 interruption closure contains a symlink or special entry"
  entries="$(find "${retry2_stage04_interruption_closure}" -xdev -type f -print)" ||
    fail "could not enumerate Retry2 interruption closure files"
  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    require_file "${path}"
    [[ "$(stat -c '%a:%u:%g:%h' -- "${path}")" == \
      "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "Retry2 interruption closure file metadata drifted: ${path}"
  done <<<"${entries}"
  entries="$(find "${retry2_stage04_interruption_closure}" -xdev -type d -print)" ||
    fail "could not enumerate Retry2 interruption closure directories"
  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    require_dir "${path}"
    [[ "$(stat -c '%a:%u:%g:%h' -- "${path}")" == \
      "555:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "Retry2 interruption closure directory metadata drifted: ${path}"
  done <<<"${entries}"
  verify_retry2_stage04_interruption_closure_inventory_tree
}

verify_retry2_completed_prefix_exact_tree() {
  local actual expected entries path special
  require_dir "${retry2_completed_prefix_bundle}"
  special="$(find "${retry2_completed_prefix_bundle}" -xdev \
    ! -type f ! -type d -print -quit)" ||
    fail "could not scan completed-prefix bundle entry types"
  [[ -z "${special}" ]] ||
    fail "completed-prefix bundle contains a symlink or special entry: ${special}"
  actual="$(cd "${retry2_completed_prefix_bundle}" && \
    find . -xdev -type f -printf '%P\n' | LC_ALL=C sort)" ||
    fail "could not enumerate completed-prefix bundle files"
  expected="$(retry2_completed_prefix_expected_files | LC_ALL=C sort)" ||
    fail "could not construct completed-prefix file allowlist"
  [[ "${actual}" == "${expected}" ]] ||
    fail "completed-prefix bundle regular-file tree differs from its exact allowlist"
  actual="$(cd "${retry2_completed_prefix_bundle}" && \
    find . -xdev -type d -printf '%p\n' | LC_ALL=C sort)" ||
    fail "could not enumerate completed-prefix bundle directories"
  expected="$(retry2_completed_prefix_expected_directories | LC_ALL=C sort)" ||
    fail "could not construct completed-prefix directory allowlist"
  [[ "${actual}" == "${expected}" ]] ||
    fail "completed-prefix bundle directory tree differs from its exact allowlist"
  entries="$(find "${retry2_completed_prefix_bundle}" -xdev -type f -print)" ||
    fail "could not enumerate completed-prefix bundle metadata"
  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    require_file "${path}"
    [[ "$(stat -c '%a:%u:%g:%h' -- "${path}")" == \
      "444:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "completed-prefix file metadata drifted: ${path}"
  done <<<"${entries}"
  entries="$(find "${retry2_completed_prefix_bundle}" -xdev -type d -print)" ||
    fail "could not enumerate completed-prefix bundle directory metadata"
  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    require_dir "${path}"
    [[ "$(stat -c '%a:%u:%g:%h' -- "${path}")" == \
      "555:${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "completed-prefix directory metadata drifted: ${path}"
  done <<<"${entries}"
}

verify_retry2_completed_prefix_inventory_shape() {
  local regular_header directory_header regular_fd directory_fd
  local entry_index source_relative_path source_path source_mode source_uid
  local source_gid source_links source_bytes source_inode source_device
  local source_sha256 destination_relative_path destination_path
  local destination_mode destination_uid destination_gid destination_links
  local destination_bytes destination_inode destination_device
  local destination_sha256 byte_identical distinct_inode expected_index
  local expected_source_path expected_destination_relative_path
  local expected_destination_path source_metadata destination_metadata prefix
  local relative_path mode_value uid_value gid_value links_value bytes_value
  local inode_value device_value path actual_metadata
  local regular_count=0 directory_count=0 payload_bytes=0
  local actual_files actual_directories actual_bundle_files
  local actual_bundle_directories content_inventory_sha256
  local metadata_inventory_sha256 regular_inventory_sha256
  local directory_inventory_sha256
  declare -A seen_destination_paths=() seen_directory_paths=()

  IFS= read -r regular_header <"${retry2_completed_prefix_regular_inventory}" ||
    fail "could not read completed-prefix regular inventory header"
  [[ "${regular_header}" == $'entry_index\tsource_relative_path\tsource_path\tsource_mode\tsource_uid\tsource_gid\tsource_links\tsource_bytes\tsource_inode\tsource_device\tsource_sha256\tdestination_relative_path\tdestination_path\tdestination_mode\tdestination_uid\tdestination_gid\tdestination_links\tdestination_bytes\tdestination_inode\tdestination_device\tdestination_sha256\tbyte_identical\tdistinct_inode' ]] ||
    fail "completed-prefix regular inventory header drifted"
  [[ "$(wc -l <"${retry2_completed_prefix_regular_inventory}")" == 52 ]] ||
    fail "completed-prefix regular inventory must contain exactly 51 payload entries"
  awk -F '\t' -v uid="${process_owner_uid}" \
    -v gid="${process_owner_gid}" '
    NR == 1 { next }
    NF != 23 || $1 != sprintf("%02d", NR - 2) ||
      $14 != "444" || $15 != uid || $16 != gid || $17 != 1 ||
      $11 != $21 || $22 != "true" || $23 != "true" { exit 42 }
  ' "${retry2_completed_prefix_regular_inventory}" ||
    fail "completed-prefix regular inventory semantics drifted"

  exec {regular_fd}<"${retry2_completed_prefix_regular_inventory}" ||
    fail "could not open completed-prefix regular inventory"
  IFS= read -r regular_header <&${regular_fd} ||
    fail "could not consume completed-prefix regular inventory header"
  while IFS=$'\t' read -r entry_index source_relative_path source_path \
    source_mode source_uid source_gid source_links source_bytes source_inode \
    source_device source_sha256 destination_relative_path destination_path \
    destination_mode destination_uid destination_gid destination_links \
    destination_bytes destination_inode destination_device destination_sha256 \
    byte_identical distinct_inode <&${regular_fd}; do
    printf -v expected_index '%02d' "${regular_count}"
    [[ "${entry_index}" == "${expected_index}" ]] ||
      fail "completed-prefix inventory entry index drifted: ${entry_index}"
    validate_retry3_inventory_relative_path "${source_relative_path}"
    validate_retry3_inventory_relative_path "${destination_relative_path}"
    [[ -z "${seen_destination_paths[${destination_relative_path}]+x}" ]] ||
      fail "duplicate completed-prefix destination path: ${destination_relative_path}"
    seen_destination_paths["${destination_relative_path}"]=1
    if ((regular_count == 0)); then
      [[ "${source_relative_path}" == @operational_retry2_runner ]] ||
        fail "completed-prefix operational-runner source label drifted"
      expected_source_path="${script_dir}/run_representation_ablation_v2_retry2.sh"
      expected_destination_relative_path=operational_authority/run_representation_ablation_v2_retry2.sh
    else
      expected_source_path="${retry2_stage04_interruption_snapshot}/${source_relative_path}"
      expected_destination_relative_path="${source_relative_path}"
    fi
    expected_destination_path="${retry2_completed_prefix_snapshot}/${expected_destination_relative_path}"
    [[ "${source_path}" == "${expected_source_path}" && \
      "${destination_relative_path}" == \
      "${expected_destination_relative_path}" && \
      "${destination_path}" == "${expected_destination_path}" ]] ||
      fail "completed-prefix inventory path binding drifted: ${entry_index}"
    require_file "${source_path}"
    require_contained_path "${destination_path}" \
      "${retry2_completed_prefix_snapshot}"
    require_file "${destination_path}"
    [[ "${source_mode}" =~ ^[0-7]{3,4}$ && \
      "${source_uid}" =~ ^[0-9]+$ && "${source_gid}" =~ ^[0-9]+$ && \
      "${source_links}" =~ ^[0-9]+$ && "${source_bytes}" =~ ^[0-9]+$ && \
      "${source_inode}" =~ ^[0-9]+$ && "${source_device}" =~ ^[0-9]+$ && \
      "${source_sha256}" =~ ^[0-9a-f]{64}$ && \
      "${destination_mode}" =~ ^[0-7]{3,4}$ && \
      "${destination_uid}" =~ ^[0-9]+$ && \
      "${destination_gid}" =~ ^[0-9]+$ && \
      "${destination_links}" =~ ^[0-9]+$ && \
      "${destination_bytes}" =~ ^[0-9]+$ && \
      "${destination_inode}" =~ ^[0-9]+$ && \
      "${destination_device}" =~ ^[0-9]+$ && \
      "${destination_sha256}" =~ ^[0-9a-f]{64}$ ]] ||
      fail "malformed completed-prefix regular inventory row: ${entry_index}"
    [[ "${source_uid}:${source_gid}:${source_links}" == \
      "${process_owner_uid}:${process_owner_gid}:1" && \
      "${destination_mode}:${destination_uid}:${destination_gid}:${destination_links}" == \
      "444:${process_owner_uid}:${process_owner_gid}:1" && \
      "${source_bytes}" == "${destination_bytes}" && \
      "${source_sha256}" == "${destination_sha256}" && \
      "${source_device}:${source_inode}" != \
      "${destination_device}:${destination_inode}" && \
      "${byte_identical}" == true && "${distinct_inode}" == true ]] ||
      fail "completed-prefix regular inventory row semantics drifted: ${entry_index}"
    if ((regular_count == 0)); then
      [[ "${source_mode}" == 555 ]] ||
        fail "completed-prefix operational-runner source mode drifted"
    else
      [[ "${source_mode}" == 444 ]] ||
        fail "completed-prefix closure-snapshot source mode drifted: ${entry_index}"
    fi
    source_metadata="$(stat -c '%a:%u:%g:%h:%s:%i:%d' -- "${source_path}")"
    [[ "${source_metadata}" == \
      "${source_mode}:${source_uid}:${source_gid}:${source_links}:${source_bytes}:${source_inode}:${source_device}" ]] ||
      fail "completed-prefix source metadata differs from inventory: ${entry_index}"
    destination_metadata="$(stat -c '%a:%u:%g:%h:%s:%i:%d' -- \
      "${destination_path}")"
    [[ "${destination_metadata}" == \
      "${destination_mode}:${destination_uid}:${destination_gid}:${destination_links}:${destination_bytes}:${destination_inode}:${destination_device}" ]] ||
      fail "completed-prefix destination metadata differs from inventory: ${entry_index}"
    [[ "$(sha256_of "${source_path}")" == "${source_sha256}" && \
      "$(sha256_of "${destination_path}")" == "${destination_sha256}" ]] ||
      fail "completed-prefix source or destination hash differs from inventory: ${entry_index}"
    cmp -s -- "${source_path}" "${destination_path}" ||
      fail "completed-prefix source and destination bytes differ: ${entry_index}"
    printf -v prefix 'payload_file_%02d' "${regular_count}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_source_relative_path" "${source_relative_path}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_source_path" "${source_path}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_source_sha256" "${source_sha256}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_destination_relative_path" "${destination_relative_path}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_destination_path" "${destination_path}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_destination_sha256" "${destination_sha256}"
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_byte_identical" true
    expect_kv "${retry2_completed_prefix_bundle_receipt}" \
      "${prefix}_distinct_inode" true
    regular_count=$((regular_count + 1))
    payload_bytes=$((payload_bytes + destination_bytes))
  done
  exec {regular_fd}<&-
  [[ "${regular_count}" == 51 ]] ||
    fail "completed-prefix regular inventory traversal was incomplete"

  IFS= read -r directory_header <"${retry2_completed_prefix_directory_inventory}" ||
    fail "could not read completed-prefix directory inventory header"
  [[ "${directory_header}" == $'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice' ]] ||
    fail "completed-prefix directory inventory header drifted"
  [[ "$(wc -l <"${retry2_completed_prefix_directory_inventory}")" == 22 ]] ||
    fail "completed-prefix directory inventory must contain exactly 21 payload entries"
  awk -F '\t' -v uid="${process_owner_uid}" \
    -v gid="${process_owner_gid}" '
    NR == 1 { next }
    NF != 8 || $2 != "555" || $3 != uid || $4 != gid || $5 != 1 {
      exit 42
    }
  ' "${retry2_completed_prefix_directory_inventory}" ||
    fail "completed-prefix directory inventory semantics drifted"

  exec {directory_fd}<"${retry2_completed_prefix_directory_inventory}" ||
    fail "could not open completed-prefix directory inventory"
  IFS= read -r directory_header <&${directory_fd} ||
    fail "could not consume completed-prefix directory inventory header"
  while IFS=$'\t' read -r relative_path mode_value uid_value gid_value \
    links_value bytes_value inode_value device_value <&${directory_fd}; do
    validate_retry3_inventory_relative_path "${relative_path}" true
    [[ -z "${seen_directory_paths[${relative_path}]+x}" ]] ||
      fail "duplicate completed-prefix directory inventory path: ${relative_path}"
    seen_directory_paths["${relative_path}"]=1
    [[ "${mode_value}" == 555 && "${uid_value}" =~ ^[0-9]+$ && \
      "${gid_value}" =~ ^[0-9]+$ && "${links_value}" =~ ^[0-9]+$ && \
      "${bytes_value}" =~ ^[0-9]+$ && "${inode_value}" =~ ^[0-9]+$ && \
      "${device_value}" =~ ^[0-9]+$ && \
      "${uid_value}:${gid_value}:${links_value}" == \
      "${process_owner_uid}:${process_owner_gid}:1" ]] ||
      fail "malformed completed-prefix directory inventory row: ${relative_path}"
    if [[ "${relative_path}" == . ]]; then
      path="${retry2_completed_prefix_snapshot}"
    else
      path="${retry2_completed_prefix_snapshot}/${relative_path}"
    fi
    require_contained_path "${path}" "${retry2_completed_prefix_snapshot}"
    require_dir "${path}"
    actual_metadata="$(stat -c '%a:%u:%g:%h:%s:%i:%d' -- "${path}")"
    [[ "${actual_metadata}" == \
      "${mode_value}:${uid_value}:${gid_value}:${links_value}:${bytes_value}:${inode_value}:${device_value}" ]] ||
      fail "completed-prefix directory metadata differs from inventory: ${relative_path}"
    directory_count=$((directory_count + 1))
  done
  exec {directory_fd}<&-
  [[ "${directory_count}" == 21 ]] ||
    fail "completed-prefix directory inventory traversal was incomplete"

  actual_files="$(find "${retry2_completed_prefix_snapshot}" -xdev \
    -type f -printf x | wc -c)" ||
    fail "could not count completed-prefix payload files"
  actual_directories="$(find "${retry2_completed_prefix_snapshot}" -xdev \
    -type d -printf x | wc -c)" ||
    fail "could not count completed-prefix payload directories"
  [[ "${actual_files}" == "${regular_count}" && \
    "${actual_directories}" == "${directory_count}" ]] ||
    fail "completed-prefix payload contains an uninventoryed entry"
  actual_bundle_files="$(find "${retry2_completed_prefix_bundle}" -xdev \
    -type f -printf x | wc -c)" ||
    fail "could not count completed-prefix bundle files"
  actual_bundle_directories="$(find "${retry2_completed_prefix_bundle}" -xdev \
    -type d -printf x | wc -c)" ||
    fail "could not count completed-prefix bundle directories"
  [[ "${actual_bundle_files}" == 55 && \
    "${actual_bundle_directories}" == 23 ]] ||
    fail "completed-prefix bundle exact full-tree count drifted"

  regular_inventory_sha256="$(sha256_of \
    "${retry2_completed_prefix_regular_inventory}")"
  directory_inventory_sha256="$(sha256_of \
    "${retry2_completed_prefix_directory_inventory}")"
  content_inventory_sha256="$(awk -F '\t' \
    'NR > 1 { print $12 "\t" $21 }' \
    "${retry2_completed_prefix_regular_inventory}" | \
    sha256sum | awk '{print $1}')" ||
    fail "could not recompute completed-prefix content inventory"
  metadata_inventory_sha256="$(
    {
      printf 'regular_files.inventory.tsv\t%s\n' \
        "${regular_inventory_sha256}"
      printf 'directories.inventory.tsv\t%s\n' \
        "${directory_inventory_sha256}"
    } | sha256sum | awk '{print $1}'
  )" || fail "could not recompute completed-prefix metadata inventory"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_content_inventory_algorithm \
    sha256_of_tab_separated_destination_relative_path_and_destination_sha256_lines
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_content_inventory_sha256 "${content_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_metadata_inventory_algorithm \
    sha256_of_labeled_regular_and_directory_inventory_sha256_lines
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_metadata_inventory_sha256 "${metadata_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_regular_file_count "${regular_count}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_directory_count "${directory_count}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_regular_file_bytes "${payload_bytes}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    bundle_regular_file_count "${actual_bundle_files}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    bundle_directory_count "${actual_bundle_directories}"
}

verify_retry2_completed_prefix_scientific_files() {
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/operational_authority/run_representation_ablation_v2_retry2.sh" \
    "${expected_retry2_operational_runner_sha256}" 444 \
    "Retry2 operational runner bundle copy"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/frozen_sources/run_representation_ablation_v2_retry2.sh" \
    "${expected_retry2_operational_runner_sha256}" 444 \
    "Retry2 runtime-frozen runner bundle copy"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.00.initialize.attempt.status" \
    "${expected_retry2_stage00_attempt_sha256}" 444 "Retry2 stage-00 attempt"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.00.initialize.status" \
    "${expected_retry2_stage00_completion_sha256}" 444 "Retry2 stage-00 completion"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.01.canonical_import.attempt.status" \
    "${expected_retry2_stage01_attempt_sha256}" 444 "Retry2 stage-01 attempt"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.01.canonical_import.status" \
    "${expected_retry2_stage01_completion_sha256}" 444 "Retry2 stage-01 completion"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.02.endpoint_import.attempt.status" \
    "${expected_retry2_stage02_attempt_sha256}" 444 "Retry2 stage-02 attempt"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.02.endpoint_import.status" \
    "${expected_retry2_stage02_completion_sha256}" 444 "Retry2 stage-02 completion"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.03.time_only_training.attempt.status" \
    "${expected_retry2_stage03_attempt_sha256}" 444 "Retry2 stage-03 attempt"
  verify_pinned_mode_file \
    "${retry2_completed_prefix_snapshot}/stage.03.time_only_training.status" \
    "${expected_retry2_stage03_completion_sha256}" 444 "Retry2 stage-03 completion"
  verify_pinned_mode_file "${retry2_prefix_time_only_status}" \
    "${expected_retry2_time_only_training_status_sha256}" 444 \
    "Retry2 time-only training status"
  verify_pinned_mode_file "${retry2_prefix_time_only_checkpoint}" \
    "${expected_retry2_time_only_checkpoint_sha256}" 444 \
    "Retry2 time-only checkpoint"
  verify_pinned_mode_file "${retry2_prefix_time_only_manifest}" \
    "${expected_retry2_time_only_manifest_sha256}" 444 \
    "Retry2 time-only manifest"
  verify_pinned_mode_file "${retry2_prefix_time_only_result}" \
    "${expected_retry2_time_only_result_sha256}" 444 \
    "Retry2 time-only Runtime result"
  verify_pinned_mode_file "${retry2_prefix_time_only_report}" \
    "${expected_retry2_time_only_report_sha256}" 444 \
    "Retry2 time-only representation report"
  verify_pinned_mode_file "${retry2_prefix_time_only_log}" \
    "${expected_retry2_time_only_log_sha256}" 444 \
    "Retry2 time-only training log"
  verify_pinned_mode_file "${retry2_prefix_endpoint_checkpoint}" \
    "${expected_retry1_endpoint_checkpoint_sha256}" 444 \
    "Retry2 endpoint import checkpoint"
  verify_pinned_mode_file "${retry2_prefix_canonical_main_report}" \
    "${expected_raw96_report_sha256}" 444 "Retry2 canonical main report"
  verify_pinned_mode_file "${retry2_prefix_canonical_replay_report}" \
    "${expected_raw96_report_sha256}" 444 "Retry2 canonical replay report"
  path_is_absent "${retry2_completed_prefix_snapshot}/stage.04.no_tf_alignment_training.attempt.status" ||
    fail "Retry2 terminal stage-04 attempt leaked into the reusable bundle"
  path_is_absent "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/training" &&
    path_is_absent "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/training.log" &&
    path_is_absent "${retry2_completed_prefix_snapshot}/arms/no_tf_alignment/training.status" ||
    fail "Retry2 partial no-TF payload leaked into the reusable bundle"
}

verify_retry2_completed_prefix_bundle_authority() {
  assert_retry3_recovery_pins_sealed
  verify_pinned_mode_file "${retry2_completed_prefix_bundle_receipt}" \
    "${expected_retry2_completed_prefix_bundle_receipt_sha256}" 444 \
    "Retry2 completed-prefix bundle receipt"
  verify_pinned_mode_file "${retry2_completed_prefix_regular_inventory}" \
    "${expected_retry2_completed_prefix_regular_inventory_sha256}" 444 \
    "Retry2 completed-prefix regular inventory"
  verify_pinned_mode_file "${retry2_completed_prefix_directory_inventory}" \
    "${expected_retry2_completed_prefix_directory_inventory_sha256}" 444 \
    "Retry2 completed-prefix directory inventory"
  verify_pinned_mode_file "${retry2_completed_prefix_live_sealer}" \
    "${expected_retry2_completed_prefix_sealer_sha256}" 555 \
    "Retry2 completed-prefix live sealer"
  verify_pinned_mode_file "${retry2_completed_prefix_frozen_sealer}" \
    "${expected_retry2_completed_prefix_sealer_sha256}" 444 \
    "Retry2 completed-prefix frozen sealer"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" schema_id \
    "${retry2_completed_prefix_bundle_schema_id}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" status complete
  expect_kv "${retry2_completed_prefix_bundle_receipt}" bundle_root \
    "${retry2_completed_prefix_bundle}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" completed_prefix_root \
    "${retry2_completed_prefix_snapshot}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    regular_inventory_path "${retry2_completed_prefix_regular_inventory}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    regular_inventory_sha256 \
    "${expected_retry2_completed_prefix_regular_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    directory_inventory_path "${retry2_completed_prefix_directory_inventory}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    directory_inventory_sha256 \
    "${expected_retry2_completed_prefix_directory_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_bundle_sealer_path \
    "${retry2_completed_prefix_live_sealer}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    completed_prefix_bundle_sealer_process_start_sha256 \
    "${expected_retry2_completed_prefix_sealer_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    frozen_completed_prefix_bundle_sealer_path \
    "${retry2_completed_prefix_frozen_sealer}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    frozen_completed_prefix_bundle_sealer_sha256 \
    "${expected_retry2_completed_prefix_sealer_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_schema_id \
    "${retry2_stage04_interruption_closure_schema_id}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_root "${retry2_stage04_interruption_closure}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_receipt_path \
    "${retry2_stage04_interruption_closure_receipt}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_receipt_sha256 \
    "${expected_retry2_stage04_interruption_closure_receipt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_regular_inventory_path \
    "${retry2_stage04_interruption_regular_inventory}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_regular_inventory_sha256 \
    "${expected_retry2_stage04_interruption_regular_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_directory_inventory_path \
    "${retry2_stage04_interruption_directory_inventory}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_directory_inventory_sha256 \
    "${expected_retry2_stage04_interruption_directory_inventory_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    interruption_closure_source_snapshot_root \
    "${retry2_stage04_interruption_snapshot}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" completed_stage_count 4
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_00_attempt_sha256 "${expected_retry2_stage00_attempt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_00_completion_sha256 \
    "${expected_retry2_stage00_completion_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_01_attempt_sha256 "${expected_retry2_stage01_attempt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_01_completion_sha256 \
    "${expected_retry2_stage01_completion_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_02_attempt_sha256 "${expected_retry2_stage02_attempt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_02_completion_sha256 \
    "${expected_retry2_stage02_completion_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_03_attempt_sha256 "${expected_retry2_stage03_attempt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_03_completion_sha256 \
    "${expected_retry2_stage03_completion_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_04_attempt_sha256 "${expected_retry2_stage04_attempt_sha256}"
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    retry2_stage_04_attempt_included false
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    stages_00_through_03_reuse_authorized true
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    stage_04_reuse_authorized false
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    no_tf_alignment_partial_payload_included false
  expect_kv "${retry2_completed_prefix_bundle_receipt}" \
    no_tf_alignment_restart_optimizer_step 0
  verify_retry2_completed_prefix_exact_tree
  verify_retry2_completed_prefix_inventory_shape
  verify_retry2_completed_prefix_scientific_files
}

emit_retry3_recovery_authority_bindings() {
  verify_retry2_stage04_interruption_closure_authority
  verify_retry2_completed_prefix_bundle_authority
  cat <<RECOVERY
retry2_stage04_interruption_closure_schema_id=${retry2_stage04_interruption_closure_schema_id}
retry2_stage04_interruption_closure_receipt_path=${retry2_stage04_interruption_closure_receipt}
retry2_stage04_interruption_closure_receipt_sha256=${expected_retry2_stage04_interruption_closure_receipt_sha256}
retry2_stage04_interruption_regular_inventory_path=${retry2_stage04_interruption_regular_inventory}
retry2_stage04_interruption_regular_inventory_sha256=${expected_retry2_stage04_interruption_regular_inventory_sha256}
retry2_stage04_interruption_directory_inventory_path=${retry2_stage04_interruption_directory_inventory}
retry2_stage04_interruption_directory_inventory_sha256=${expected_retry2_stage04_interruption_directory_inventory_sha256}
retry2_completed_prefix_bundle_schema_id=${retry2_completed_prefix_bundle_schema_id}
retry2_completed_prefix_bundle_receipt_path=${retry2_completed_prefix_bundle_receipt}
retry2_completed_prefix_bundle_receipt_sha256=${expected_retry2_completed_prefix_bundle_receipt_sha256}
retry2_completed_prefix_regular_inventory_path=${retry2_completed_prefix_regular_inventory}
retry2_completed_prefix_regular_inventory_sha256=${expected_retry2_completed_prefix_regular_inventory_sha256}
retry2_completed_prefix_directory_inventory_path=${retry2_completed_prefix_directory_inventory}
retry2_completed_prefix_directory_inventory_sha256=${expected_retry2_completed_prefix_directory_inventory_sha256}
retry2_completed_prefix_count=4
retry2_completed_prefix_head_sha256=${expected_retry2_stage03_completion_sha256}
retry2_terminal_stage04_attempt_sha256=${expected_retry2_stage04_attempt_sha256}
retry2_live_runtime_direct_use_authorized=false
retry2_partial_stage04_artifact_reuse_authorized=false
retry3_no_tf_alignment_restart_optimizer_step=0
RECOVERY
}

verify_retry3_recovery_authority_bindings() {
  local receipt="$1"
  verify_retry2_stage04_interruption_closure_authority
  verify_retry2_completed_prefix_bundle_authority
  expect_kv "${receipt}" retry2_stage04_interruption_closure_schema_id \
    "${retry2_stage04_interruption_closure_schema_id}"
  expect_kv "${receipt}" retry2_stage04_interruption_closure_receipt_path \
    "${retry2_stage04_interruption_closure_receipt}"
  expect_kv "${receipt}" retry2_stage04_interruption_closure_receipt_sha256 \
    "${expected_retry2_stage04_interruption_closure_receipt_sha256}"
  expect_kv "${receipt}" retry2_completed_prefix_bundle_schema_id \
    "${retry2_completed_prefix_bundle_schema_id}"
  expect_kv "${receipt}" retry2_completed_prefix_bundle_receipt_path \
    "${retry2_completed_prefix_bundle_receipt}"
  expect_kv "${receipt}" retry2_completed_prefix_bundle_receipt_sha256 \
    "${expected_retry2_completed_prefix_bundle_receipt_sha256}"
  expect_kv "${receipt}" retry2_completed_prefix_count 4
  expect_kv "${receipt}" retry2_completed_prefix_head_sha256 \
    "${expected_retry2_stage03_completion_sha256}"
  expect_kv "${receipt}" retry2_terminal_stage04_attempt_sha256 \
    "${expected_retry2_stage04_attempt_sha256}"
  expect_kv "${receipt}" retry2_live_runtime_direct_use_authorized false
  expect_kv "${receipt}" retry2_partial_stage04_artifact_reuse_authorized false
  expect_kv "${receipt}" retry3_no_tf_alignment_restart_optimizer_step 0
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
  emit_retry3_recovery_authority_bindings
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
  verify_retry3_recovery_authority_bindings "${receipt}"
}

# Old operational-incident verifiers are never allowed to bind the current
# Retry3 runner.  Existing internal call sites resolve to these fixed recovery
# gates instead.
verify_retry2_bootstrap_failure_closure_authority() {
  verify_retry2_stage04_interruption_closure_authority
}

verify_retry2_windows_safe_publication_authority_v2() {
  verify_retry2_completed_prefix_bundle_authority
}

preflight_read_only() {
  assert_operational_runner_identity
  verify_retry2_stage04_interruption_closure_authority
  verify_retry2_completed_prefix_bundle_authority
  verify_recovery_authority
  verify_read_only_preflight_inputs
  verify_mdn_retry1_authority
  canonical_inputs
  assert_operational_runner_identity
}

retry2_prefix_arm_policy() {
  printf '%s/arms/%s/config/representation.jkimyei' \
    "${retry2_completed_prefix_snapshot}" "$1"
}

retry2_prefix_arm_net() {
  printf '%s/arms/%s/config/representation.net' \
    "${retry2_completed_prefix_snapshot}" "$1"
}

retry2_prefix_arm_config() {
  printf '%s/arms/%s/config/train.config' \
    "${retry2_completed_prefix_snapshot}" "$1"
}

retry2_prefix_arm_capture_config() {
  printf '%s/arms/%s/config/capture.config' \
    "${retry2_completed_prefix_snapshot}" "$1"
}

verify_retry2_prefix_arm_scientific_equivalence() {
  local arm="$1" source_policy source_net left right
  source_policy="$(retry2_prefix_arm_policy "${arm}")"
  source_net="$(retry2_prefix_arm_net "${arm}")"
  verify_retry2_completed_prefix_bundle_authority
  cmp -s -- "${source_policy}" "$(arm_policy "${arm}")" ||
    fail "Retry3 ${arm} policy differs from its sealed Retry2 prefix authority"
  cmp -s -- "${source_net}" "$(arm_net "${arm}")" ||
    fail "Retry3 ${arm} network differs from its sealed Retry2 prefix authority"
  left="$(mktemp "${scratch_root}/${schema_id}.${arm}.prefix_train_left.XXXXXX")"
  right="$(mktemp "${scratch_root}/${schema_id}.${arm}.prefix_train_right.XXXXXX")"
  normalize_endpoint_config_for_equivalence \
    "$(retry2_prefix_arm_config "${arm}")" "${source_policy}" "${source_net}" \
    >"${left}"
  normalize_endpoint_config_for_equivalence "$(arm_config "${arm}")" \
    "$(arm_policy "${arm}")" "$(arm_net "${arm}")" >"${right}"
  cmp -s -- "${left}" "${right}" || {
    rm -f -- "${left}" "${right}"
    fail "Retry3 ${arm} training config changes more than local absolute paths"
  }
  normalize_endpoint_config_for_equivalence \
    "$(retry2_prefix_arm_capture_config "${arm}")" \
    "${source_policy}" "${source_net}" >"${left}"
  normalize_endpoint_config_for_equivalence "$(arm_capture_config "${arm}")" \
    "$(arm_policy "${arm}")" "$(arm_net "${arm}")" >"${right}"
  cmp -s -- "${left}" "${right}" || {
    rm -f -- "${left}" "${right}"
    fail "Retry3 ${arm} capture config changes more than local absolute paths"
  }
  rm -f -- "${left}" "${right}"
}

verify_endpoint_bundle_scientific_equivalence() {
  local arm
  verify_retry2_completed_prefix_bundle_authority
  for arm in "${challenger_arms[@]}"; do
    verify_retry2_prefix_arm_scientific_equivalence "${arm}"
  done
}

copy_retry2_prefix_file_immutable() {
  local source="$1" destination="$2" label="$3" candidate
  require_immutable_file "${source}"
  path_is_absent "${destination}" ||
    fail "${label} destination predates its stage attempt: ${destination}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.prefix_copy.XXXXXX")"
  cp --reflink=never -- "${source}" "${candidate}" ||
    fail "could not copy ${label} from the completed-prefix bundle"
  publish_immutable "${candidate}" "${destination}"
  cmp -s -- "${source}" "${destination}" ||
    fail "${label} copy is not byte-identical"
  [[ "$(stat -c '%i:%d' -- "${source}")" != \
    "$(stat -c '%i:%d' -- "${destination}")" ]] ||
    fail "${label} copy aliases its completed-prefix source inode"
}

emit_canonical_import() {
  local destination="$1"
  {
    echo "schema_id=${schema_id}.retry2_canonical_import.v1"
    echo "status=complete"
    echo "arm=canonical"
    emit_ablation_runner_bindings
    echo "retry_attempt_sentinel_path=${retry_attempt_sentinel}"
    echo "retry_attempt_sentinel_sha256=$(sha256_of "${retry_attempt_sentinel}")"
    echo "source_completed_prefix_bundle_path=${retry2_completed_prefix_bundle_receipt}"
    echo "source_completed_prefix_bundle_sha256=${expected_retry2_completed_prefix_bundle_receipt_sha256}"
    echo "source_retry2_stage_01_completion_sha256=${expected_retry2_stage01_completion_sha256}"
    echo "source_canonical_import_status_path=${retry2_prefix_canonical_status}"
    echo "source_canonical_import_status_sha256=$(sha256_of "${retry2_prefix_canonical_status}")"
    echo "source_main_report_path=${retry2_prefix_canonical_main_report}"
    echo "source_main_report_sha256=${expected_raw96_report_sha256}"
    echo "source_replay_report_path=${retry2_prefix_canonical_replay_report}"
    echo "source_replay_report_sha256=${expected_raw96_report_sha256}"
    echo "imported_main_report_path=$(arm_main_report canonical)"
    echo "imported_main_report_sha256=$(sha256_of "$(arm_main_report canonical)")"
    echo "imported_replay_report_path=$(arm_replay_report canonical)"
    echo "imported_replay_report_sha256=$(sha256_of "$(arm_replay_report canonical)")"
    echo "copy_method=cp_--reflink=never"
    echo "byte_identical_copy_verified=true"
    echo "distinct_source_copy_identity_verified=true"
    echo "retry3_optimizer_steps=0"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_canonical_import() {
  local candidate
  verify_retry2_completed_prefix_bundle_authority
  require_dir "$(arm_root canonical)"
  require_dir "$(arm_root canonical)/affine"
  [[ "$(stat -c '%a:%u' -- "$(arm_root canonical)")" == \
    "700:${process_owner_uid}" ]] ||
    fail "Retry3 canonical import arm directory metadata drifted"
  [[ "$(stat -c '%a:%u' -- "$(arm_root canonical)/affine")" == \
    "700:${process_owner_uid}" ]] ||
    fail "Retry3 canonical import affine directory metadata drifted"
  verify_pinned_mode_file "$(arm_main_report canonical)" \
    "${expected_raw96_report_sha256}" 444 "Retry3 canonical main report"
  verify_pinned_mode_file "$(arm_replay_report canonical)" \
    "${expected_raw96_report_sha256}" 444 "Retry3 canonical replay report"
  cmp -s -- "${retry2_prefix_canonical_main_report}" \
    "$(arm_main_report canonical)" ||
    fail "Retry3 canonical main import differs from its bundle source"
  cmp -s -- "${retry2_prefix_canonical_replay_report}" \
    "$(arm_replay_report canonical)" ||
    fail "Retry3 canonical replay import differs from its bundle source"
  require_immutable_file "${canonical_import_receipt}"
  expect_kv "${canonical_import_receipt}" schema_id \
    "${schema_id}.retry2_canonical_import.v1"
  verify_ablation_runner_bindings "${canonical_import_receipt}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_import_verify.XXXXXX")"
  emit_canonical_import "${candidate}"
  cmp -s -- "${candidate}" "${canonical_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "Retry3 canonical import receipt drifted"
  }
  rm -f -- "${candidate}"
}

import_canonical_arm() {
  verify_retry2_completed_prefix_bundle_authority
  path_is_absent "$(arm_root canonical)" ||
    fail "Retry3 canonical import root predates its stage attempt"
  mkdir -- "$(arm_root canonical)"
  mkdir -- "$(arm_root canonical)/affine"
  copy_retry2_prefix_file_immutable "${retry2_prefix_canonical_main_report}" \
    "$(arm_main_report canonical)" "canonical main report"
  copy_retry2_prefix_file_immutable "${retry2_prefix_canonical_replay_report}" \
    "$(arm_replay_report canonical)" "canonical replay report"
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.canonical_import.XXXXXX")"
  emit_canonical_import "${candidate}"
  publish_immutable "${candidate}" "${canonical_import_receipt}"
  verify_canonical_import
}

emit_endpoint_import_status() {
  local destination="$1"
  {
    echo "schema_id=${schema_id}.retry2_endpoint_import.v1"
    echo "status=complete"
    echo "arm=endpoint_scale"
    emit_ablation_runner_bindings
    echo "source_completed_prefix_bundle_path=${retry2_completed_prefix_bundle_receipt}"
    echo "source_completed_prefix_bundle_sha256=${expected_retry2_completed_prefix_bundle_receipt_sha256}"
    echo "local_source_bundle_receipt_path=${endpoint_import_source_bundle_receipt}"
    echo "local_source_bundle_receipt_sha256=$(sha256_of "${endpoint_import_source_bundle_receipt}")"
    echo "source_retry2_stage_02_completion_sha256=${expected_retry2_stage02_completion_sha256}"
    echo "source_endpoint_import_status_path=${retry2_prefix_endpoint_status}"
    echo "source_endpoint_import_status_sha256=$(sha256_of "${retry2_prefix_endpoint_status}")"
    echo "local_source_endpoint_import_status_path=${endpoint_import_source_status}"
    echo "local_source_endpoint_import_status_sha256=$(sha256_of "${endpoint_import_source_status}")"
    echo "source_checkpoint_path=${retry2_prefix_endpoint_checkpoint}"
    echo "source_checkpoint_sha256=${expected_retry1_endpoint_checkpoint_sha256}"
    echo "imported_checkpoint_path=${endpoint_import_checkpoint}"
    echo "imported_checkpoint_sha256=$(sha256_of "${endpoint_import_checkpoint}")"
    echo "copy_method=cp_--reflink=never"
    echo "byte_identical_copy_verified=true"
    echo "distinct_bundle_copy_identity_verified=true"
    echo "historical_source_optimizer_steps=3000"
    echo "retry3_import_optimizer_steps=0"
    echo "retry3_training_job_created=false"
    echo "retry3_training_status_created=false"
    echo "retry3_runtime_result_created=false"
    echo "retry3_checkpoint_resume=false"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_endpoint_import() {
  local candidate
  verify_retry2_completed_prefix_bundle_authority
  require_immutable_file "${endpoint_import_source_bundle_receipt}"
  require_immutable_file "${endpoint_import_source_status}"
  verify_pinned_mode_file "${endpoint_import_checkpoint}" \
    "${expected_retry1_endpoint_checkpoint_sha256}" 444 \
    "Retry3 endpoint checkpoint import"
  [[ "$(sha256_of "${endpoint_import_source_bundle_receipt}")" == \
    "${expected_retry2_completed_prefix_bundle_receipt_sha256}" ]] ||
    fail "Retry3 endpoint local bundle receipt drifted"
  cmp -s -- "${retry2_prefix_endpoint_status}" "${endpoint_import_source_status}" ||
    fail "Retry3 endpoint source-status copy drifted"
  cmp -s -- "${retry2_prefix_endpoint_checkpoint}" "${endpoint_import_checkpoint}" ||
    fail "Retry3 endpoint checkpoint copy drifted"
  [[ "$(stat -c '%i:%d' -- "${retry2_prefix_endpoint_checkpoint}")" != \
    "$(stat -c '%i:%d' -- "${endpoint_import_checkpoint}")" ]] ||
    fail "Retry3 endpoint checkpoint aliases its bundle source"
  require_immutable_file "${endpoint_import_receipt}"
  expect_kv "${endpoint_import_receipt}" schema_id \
    "${schema_id}.retry2_endpoint_import.v1"
  verify_ablation_runner_bindings "${endpoint_import_receipt}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_import_verify.XXXXXX")"
  emit_endpoint_import_status "${candidate}"
  cmp -s -- "${candidate}" "${endpoint_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "Retry3 endpoint import receipt drifted"
  }
  rm -f -- "${candidate}"
  printf '%s' "${endpoint_import_checkpoint}"
}

run_endpoint_import() {
  verify_endpoint_bundle_scientific_equivalence
  path_is_absent "${endpoint_imports_root}" ||
    fail "Retry3 imports parent predates endpoint stage attempt"
  mkdir -- "${endpoint_imports_root}"
  mkdir -- "${endpoint_import_root}"
  copy_retry2_prefix_file_immutable "${retry2_completed_prefix_bundle_receipt}" \
    "${endpoint_import_source_bundle_receipt}" "endpoint source bundle receipt"
  copy_retry2_prefix_file_immutable "${retry2_prefix_endpoint_status}" \
    "${endpoint_import_source_status}" "endpoint source import status"
  copy_retry2_prefix_file_immutable "${retry2_prefix_endpoint_checkpoint}" \
    "${endpoint_import_checkpoint}" "endpoint checkpoint"
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.endpoint_import.XXXXXX")"
  emit_endpoint_import_status "${candidate}"
  publish_immutable "${candidate}" "${endpoint_import_receipt}"
  verify_endpoint_import >/dev/null
}

emit_time_only_import_status() {
  local destination="$1"
  {
    echo "schema_id=${schema_id}.retry2_time_only_import.v1"
    echo "status=complete"
    echo "arm=time_only"
    emit_ablation_runner_bindings
    echo "source_completed_prefix_bundle_path=${retry2_completed_prefix_bundle_receipt}"
    echo "source_completed_prefix_bundle_sha256=${expected_retry2_completed_prefix_bundle_receipt_sha256}"
    echo "local_source_bundle_receipt_path=${time_only_import_source_bundle_receipt}"
    echo "local_source_bundle_receipt_sha256=$(sha256_of "${time_only_import_source_bundle_receipt}")"
    echo "source_retry2_stage_03_completion_sha256=${expected_retry2_stage03_completion_sha256}"
    echo "source_training_status_path=${retry2_prefix_time_only_status}"
    echo "source_training_status_sha256=${expected_retry2_time_only_training_status_sha256}"
    echo "local_source_training_status_path=${time_only_import_source_status}"
    echo "local_source_training_status_sha256=$(sha256_of "${time_only_import_source_status}")"
    echo "source_job_manifest_path=${retry2_prefix_time_only_manifest}"
    echo "source_job_manifest_sha256=${expected_retry2_time_only_manifest_sha256}"
    echo "source_runtime_result_path=${retry2_prefix_time_only_result}"
    echo "source_runtime_result_sha256=${expected_retry2_time_only_result_sha256}"
    echo "source_representation_report_path=${retry2_prefix_time_only_report}"
    echo "source_representation_report_sha256=${expected_retry2_time_only_report_sha256}"
    echo "source_training_log_path=${retry2_prefix_time_only_log}"
    echo "source_training_log_sha256=${expected_retry2_time_only_log_sha256}"
    echo "source_checkpoint_path=${retry2_prefix_time_only_checkpoint}"
    echo "source_checkpoint_sha256=${expected_retry2_time_only_checkpoint_sha256}"
    echo "imported_checkpoint_path=${time_only_import_checkpoint}"
    echo "imported_checkpoint_sha256=$(sha256_of "${time_only_import_checkpoint}")"
    echo "scientific_config_equivalence_verified=true"
    echo "copy_method=cp_--reflink=never"
    echo "byte_identical_copy_verified=true"
    echo "distinct_bundle_copy_identity_verified=true"
    echo "historical_source_optimizer_steps=3000"
    echo "retry3_import_optimizer_steps=0"
    echo "retry3_training_job_created=false"
    echo "retry3_training_status_created=false"
    echo "retry3_runtime_result_created=false"
    echo "retry3_checkpoint_resume=false"
    echo "canonical_data_raw_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } >"${destination}"
}

verify_retry2_time_only_source_semantics() {
  verify_retry2_completed_prefix_bundle_authority
  expect_kv "${retry2_prefix_time_only_status}" schema_id \
    "${retry2_schema_id}.training.v1"
  expect_kv "${retry2_prefix_time_only_status}" status complete
  expect_kv "${retry2_prefix_time_only_status}" arm time_only
  expect_kv "${retry2_prefix_time_only_status}" runner_sha256 \
    "${expected_retry2_operational_runner_sha256}"
  expect_kv "${retry2_prefix_time_only_status}" checkpoint_sha256 \
    "${expected_retry2_time_only_checkpoint_sha256}"
  expect_kv "${retry2_prefix_time_only_status}" optimizer_steps 3000
  expect_kv "${retry2_prefix_time_only_result}" status completed
  expect_kv "${retry2_prefix_time_only_result}" optimizer_steps 3000
  expect_kv "${retry2_prefix_time_only_result}" checkpoint_written true
  expect_kv "${retry2_prefix_time_only_result}" model_state_mutated true
  expect_kv "${retry2_prefix_time_only_result}" finite_parameter_check true
  expect_kv "${retry2_prefix_time_only_report}" optimizer_steps 3000
  expect_kv "${retry2_prefix_time_only_report}" seed 17
  expect_kv "${retry2_prefix_time_only_report}" use_frequency_tokens false
  expect_kv "${retry2_prefix_time_only_report}" lambda_tf_align 0.1
  expect_kv "${retry2_prefix_time_only_manifest}" input_representation_checkpoint_path ''
  expect_kv "${retry2_prefix_time_only_manifest}" input_mdn_checkpoint_path ''
}

verify_time_only_import() {
  local candidate
  verify_retry2_time_only_source_semantics
  verify_retry2_prefix_arm_scientific_equivalence time_only
  require_immutable_file "${time_only_import_source_bundle_receipt}"
  require_immutable_file "${time_only_import_source_status}"
  verify_pinned_mode_file "${time_only_import_checkpoint}" \
    "${expected_retry2_time_only_checkpoint_sha256}" 444 \
    "Retry3 time-only checkpoint import"
  [[ "$(sha256_of "${time_only_import_source_bundle_receipt}")" == \
    "${expected_retry2_completed_prefix_bundle_receipt_sha256}" ]] ||
    fail "Retry3 time-only local bundle receipt drifted"
  [[ "$(sha256_of "${time_only_import_source_status}")" == \
    "${expected_retry2_time_only_training_status_sha256}" ]] ||
    fail "Retry3 time-only source-status copy drifted"
  cmp -s -- "${retry2_prefix_time_only_checkpoint}" "${time_only_import_checkpoint}" ||
    fail "Retry3 time-only checkpoint copy drifted"
  [[ "$(stat -c '%i:%d' -- "${retry2_prefix_time_only_checkpoint}")" != \
    "$(stat -c '%i:%d' -- "${time_only_import_checkpoint}")" ]] ||
    fail "Retry3 time-only checkpoint aliases its bundle source"
  path_is_absent "$(arm_root time_only)/training" &&
    path_is_absent "$(arm_root time_only)/training.log" &&
    path_is_absent "$(arm_training_status time_only)" ||
    fail "Retry3 fabricated a time-only training history instead of importing"
  require_immutable_file "${time_only_import_receipt}"
  expect_kv "${time_only_import_receipt}" schema_id \
    "${schema_id}.retry2_time_only_import.v1"
  verify_ablation_runner_bindings "${time_only_import_receipt}"
  candidate="$(mktemp "${scratch_root}/${schema_id}.time_only_import_verify.XXXXXX")"
  emit_time_only_import_status "${candidate}"
  cmp -s -- "${candidate}" "${time_only_import_receipt}" || {
    rm -f -- "${candidate}"
    fail "Retry3 time-only import receipt drifted"
  }
  rm -f -- "${candidate}"
  printf '%s' "${time_only_import_checkpoint}"
}

run_time_only_import() {
  verify_retry2_time_only_source_semantics
  verify_retry2_prefix_arm_scientific_equivalence time_only
  require_dir "${endpoint_imports_root}"
  path_is_absent "${time_only_import_root}" ||
    fail "Retry3 time-only import root predates its stage attempt"
  mkdir -- "${time_only_import_root}"
  copy_retry2_prefix_file_immutable "${retry2_completed_prefix_bundle_receipt}" \
    "${time_only_import_source_bundle_receipt}" "time-only source bundle receipt"
  copy_retry2_prefix_file_immutable "${retry2_prefix_time_only_status}" \
    "${time_only_import_source_status}" "time-only source training status"
  copy_retry2_prefix_file_immutable "${retry2_prefix_time_only_checkpoint}" \
    "${time_only_import_checkpoint}" "time-only checkpoint"
  local candidate
  candidate="$(mktemp "${scratch_root}/${schema_id}.time_only_import.XXXXXX")"
  emit_time_only_import_status "${candidate}"
  publish_immutable "${candidate}" "${time_only_import_receipt}"
  verify_time_only_import >/dev/null
}

assert_operational_runner_identity
acquire_runner_bootstrap_lock
assert_runner_bootstrap_lock
verify_retry2_bootstrap_failure_closure_authority
verify_retry2_windows_safe_publication_authority_v2
verify_bootstrap_scratch_boundary
case "${mode}" in
--verify-development | --run-certified | --verify)
  acquire_existing_runtime_lock
  use_existing_runtime_scratch
  ;;
esac
case "${mode}" in
--preflight)
  preflight_without_attempt
  ;;
--advance-development)
  advance_development_once
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
case "${mode}" in
--advance-development | --verify-development | --run-certified | --verify)
  assert_runtime_publication_ready
  assert_directory_empty "${scratch_root}" "runtime scratch"
  assert_fd_matches_path "${development_lock_fd}" \
    "${runtime_development_lock}" "retry2 development lock"
  ;;
esac
verify_retry2_bootstrap_failure_closure_authority
verify_retry2_windows_safe_publication_authority_v2
verify_bootstrap_scratch_boundary
assert_runner_bootstrap_lock
assert_operational_runner_identity
