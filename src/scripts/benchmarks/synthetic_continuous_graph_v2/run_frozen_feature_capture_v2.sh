#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_frozen_feature_capture_v1"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_data_closure_sha256="36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831"
readonly expected_protocol_amendment_sha256="3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5"
readonly expected_localization_addendum_sha256="2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4"
readonly expected_conditional_amendment_sha256="30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67"
readonly expected_fresh_preregistration_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly expected_diagnostic_preregistration_sha256="de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8"
readonly expected_ablation_preregistration_sha256="6a4175f431347387f33c250b747f1f34c29099aaf4b3c94a75ea2e4960cef6cd"
readonly train_begin=0
readonly train_end=2496
readonly validation_begin=2560
readonly validation_end=2816
readonly certified_begin=2880
readonly certified_end=3264
readonly forbidden_final_begin=3328
readonly train_rows=$(((train_end - train_begin) * 9))
readonly validation_rows=$(((validation_end - validation_begin) * 9))
readonly certified_rows=$(((certified_end - certified_begin) * 9))

fail() {
  echo "v2 frozen feature capture: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local key="$1"
  local path="$2"
  awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) {
        value = substr($0, eq + 1);
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
        print value;
        exit;
      }
    }
  ' "${path}"
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

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
closure_verifier="${script_dir}/prepare_and_seal_fresh_seed.sh"
protocol_amendment="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md"
localization_addendum="${benchmark_root}/REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md"
conditional_amendment="${benchmark_root}/REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md"
fresh_preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
diagnostic_preregistration="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md"
ablation_preregistration="${benchmark_root}/REPRESENTATION_ABLATION_PREREGISTRATION.md"
representation_runner="${script_dir}/run_representation_train_v2.sh"
mdn_runner="${script_dir}/run_mdn_train_v2.sh"
affine_helper="${script_dir}/frozen_representation_affine_probe.cpp"
affine_runner="${script_dir}/run_frozen_representation_affine_probe.sh"
representation_result="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_v1/result.status"
mdn_result="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_v1/result.status"
mdn_train_config="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_v1/synthetic_benchmark.train_core_channel_mdn.config"
mdn_policy_source="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/${schema_id}"
capture_config="${runtime_root}/synthetic_benchmark.frozen_feature_capture.config"
untrained_mdn_policy="${runtime_root}/wikimyei.inference.expected_value.mdn.untrained_control.jkimyei"
untrained_capture_config="${runtime_root}/synthetic_benchmark.untrained_representation_capture.config"
job_root="${runtime_root}/jobs"
train_job="${job_root}/train"
validation_job="${job_root}/validation"
certified_job="${job_root}/certified"
untrained_job_root="${runtime_root}/untrained_jobs"
untrained_train_job="${untrained_job_root}/train"
untrained_validation_job="${untrained_job_root}/validation"
selection_source_root="${runtime_root}/frozen_selection_sources"
frozen_affine_helper="${selection_source_root}/frozen_representation_affine_probe.cpp"
frozen_affine_runner="${selection_source_root}/run_frozen_representation_affine_probe.sh"
certified_attempt_receipt="${runtime_root}/certified.attempt.status"
input_receipt="${runtime_root}/inputs.status"
development_receipt="${runtime_root}/development.status"
affine_route_trigger="${runtime_root}/affine_route_trigger.status"
result_receipt="${runtime_root}/result.status"

mode="${1:---plan}"
[[ "$#" -le 1 ]] ||
  fail "usage: $0 [--plan|--run-development|--verify-development|--run-certified|--verify]"
case "${mode}" in
--plan | --run-development | --verify-development | --run-certified | --verify) ;;
--run)
  fail "unconditional --run is forbidden; use --run-development, then the immutable affine trigger, then --run-certified only for route=canonical_certification"
  ;;
*)
  fail "usage: $0 [--plan|--run-development|--verify-development|--run-certified|--verify]"
  ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
development_modes=--run-development,--verify-development
certified_modes=--run-certified,--verify
development_receipt=${development_receipt}
affine_route_trigger=${affine_route_trigger}
certified_authorization_route=canonical_certification
ablation_route_certified_capture=false
untrained_representation_train_capture_range=[${train_begin},${train_end})
untrained_representation_validation_capture_range=[${validation_begin},${validation_end})
expected_probe_rows=${train_rows},${validation_rows},${certified_rows}
probe_kinds=raw_representation_96,post_mdn_context_400
untrained_representation_control_ranges=[${train_begin},${train_end}),[${validation_begin},${validation_end})
untrained_representation_certified_access=false
purge_anchors_captured=false
certified_scoring_attempt_count=1
selection_sources_frozen_before_certified=true
development_maximum_anchor_read=$((validation_end - 1))
development_creates_or_reads_certified=false
model_state_mutation=false
final_holdout_access=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

checkpoint_from_result() {
  local result="$1"
  require_file "${result}"
  expect_kv "${result}" status complete
  local checkpoint
  checkpoint="$(kv checkpoint_path "${result}")"
  require_file "${checkpoint}"
  expect_kv "${result}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  printf '%s' "${checkpoint}"
}

verify_static_inputs() {
  require_file "${closure_report}"
  require_file "${closure_verifier}"
  require_file "${protocol_amendment}"
  require_file "${localization_addendum}"
  require_file "${conditional_amendment}"
  require_file "${fresh_preregistration}"
  require_file "${diagnostic_preregistration}"
  require_file "${ablation_preregistration}"
  require_file "${representation_runner}"
  require_file "${mdn_runner}"
  require_file "${representation_result}"
  require_file "${mdn_result}"
  require_file "${mdn_train_config}"
  require_file "${mdn_policy_source}"
  require_file "${affine_helper}"
  require_file "${affine_runner}"
  require_file "${runtime_exec}"
  [[ "$(sha256_of "${closure_report}")" == "${expected_data_closure_sha256}" ]] ||
    fail "fresh-seed data closure hash drifted"
  [[ "$(sha256_of "${runtime_exec}")" == "${expected_runtime_exec_sha256}" ]] ||
    fail "runtime executable hash drifted"
  [[ "$(sha256_of "${protocol_amendment}")" == "${expected_protocol_amendment_sha256}" ]] ||
    fail "diagnostic protocol amendment hash drifted"
  [[ "$(sha256_of "${localization_addendum}")" == "${expected_localization_addendum_sha256}" ]] ||
    fail "representation localization addendum hash drifted"
  [[ "$(sha256_of "${conditional_amendment}")" == "${expected_conditional_amendment_sha256}" ]] ||
    fail "conditional-certification amendment hash drifted"
  [[ "$(sha256_of "${fresh_preregistration}")" == "${expected_fresh_preregistration_sha256}" ]] ||
    fail "fresh-seed preregistration hash drifted"
  [[ "$(sha256_of "${diagnostic_preregistration}")" == "${expected_diagnostic_preregistration_sha256}" ]] ||
    fail "representation diagnostic preregistration hash drifted"
  [[ "$(sha256_of "${ablation_preregistration}")" == "${expected_ablation_preregistration_sha256}" ]] ||
    fail "representation ablation preregistration hash drifted"
  "${closure_verifier}" --verify >/dev/null
  "${representation_runner}" --verify >/dev/null
  "${mdn_runner}" --verify >/dev/null
}

write_config_snapshot() {
  require_file "${mdn_train_config}"
  local candidate
  candidate="$(mktemp "${runtime_root}/.config.XXXXXX")"
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= cwu_02v_certified_replay_eval_mdn");
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${mdn_train_config}" >"${candidate}" ||
    fail "could not create capture config"
  chmod 0444 "${candidate}"
  if [[ -e "${capture_config}" ]]; then
    cmp -s "${candidate}" "${capture_config}" || fail "capture config drifted"
    rm -f -- "${candidate}"
  else
    ln "${candidate}" "${capture_config}" || fail "capture config publication failed"
    rm -f -- "${candidate}"
  fi
  expect_kv "${capture_config}" runtime_wave_id cwu_02v_certified_replay_eval_mdn

  local policy_candidate
  policy_candidate="$(mktemp "${runtime_root}/.untrained_policy.XXXXXX")"
  awk '
    /ALLOW_UNTRAINED_REPRESENTATION/ { exit 43 }
    /^[[:space:]]*SEED[[:space:]]*=/ {
      print "  SEED = 17;";
      seeded = 1;
      next;
    }
    /^[[:space:]]*};[[:space:]]*$/ && !inserted {
      print "  ALLOW_UNTRAINED_REPRESENTATION = true;";
      inserted = 1;
    }
    { print }
    END { if (!inserted || !seeded) exit 42 }
  ' "${mdn_policy_source}" >"${policy_candidate}" ||
    fail "could not create untrained-control MDN policy"
  chmod 0444 "${policy_candidate}"
  if [[ -e "${untrained_mdn_policy}" ]]; then
    cmp -s "${policy_candidate}" "${untrained_mdn_policy}" ||
      fail "untrained-control MDN policy drifted"
    rm -f -- "${policy_candidate}"
  else
    ln "${policy_candidate}" "${untrained_mdn_policy}" ||
      fail "untrained-control MDN policy publication failed"
    rm -f -- "${policy_candidate}"
  fi
  expect_kv "${untrained_mdn_policy}" ALLOW_UNTRAINED_REPRESENTATION 'true;'
  expect_kv "${untrained_mdn_policy}" SEED '17;'

  local untrained_config_candidate
  untrained_config_candidate="$(mktemp "${runtime_root}/.untrained_config.XXXXXX")"
  awk -v path="${untrained_mdn_policy}" '
    BEGIN { replaced = 0 }
    /^[[:space:]]*wikimyei_inference_expected_value_mdn_jkimyei_path[[:space:]]*=/ {
      sub(/=.*/, "= " path);
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${capture_config}" >"${untrained_config_candidate}" ||
    fail "could not create untrained representation capture config"
  chmod 0444 "${untrained_config_candidate}"
  if [[ -e "${untrained_capture_config}" ]]; then
    cmp -s "${untrained_config_candidate}" "${untrained_capture_config}" ||
      fail "untrained representation capture config drifted"
    rm -f -- "${untrained_config_candidate}"
  else
    ln "${untrained_config_candidate}" "${untrained_capture_config}" ||
      fail "untrained representation capture config publication failed"
    rm -f -- "${untrained_config_candidate}"
  fi
  expect_kv "${untrained_capture_config}" \
    wikimyei_inference_expected_value_mdn_jkimyei_path \
    "${untrained_mdn_policy}"
}

verify_selection_sources() {
  [[ -d "${selection_source_root}" && ! -L "${selection_source_root}" ]] ||
    fail "invalid frozen selection source directory"
  require_file "${frozen_affine_helper}"
  require_file "${frozen_affine_runner}"
  cmp -s "${affine_helper}" "${frozen_affine_helper}" ||
    fail "live affine helper differs from pre-certified frozen source"
  cmp -s "${affine_runner}" "${frozen_affine_runner}" ||
    fail "live affine runner differs from pre-certified frozen source"
}

freeze_selection_sources() {
  if [[ -e "${selection_source_root}" ]]; then
    verify_selection_sources
    return
  fi
  local candidate
  candidate="$(mktemp -d "${runtime_root}/.selection_sources.XXXXXXXX")"
  cp -- "${affine_helper}" "${candidate}/frozen_representation_affine_probe.cpp"
  cp -- "${affine_runner}" "${candidate}/run_frozen_representation_affine_probe.sh"
  chmod 0444 "${candidate}/frozen_representation_affine_probe.cpp" \
    "${candidate}/run_frozen_representation_affine_probe.sh"
  mv -T -n "${candidate}" "${selection_source_root}" ||
    fail "could not atomically freeze affine selection sources"
  verify_selection_sources
}

emit_inputs() {
  local destination="$1"
  local representation_checkpoint="$2"
  local mdn_checkpoint="$3"
  cat >"${destination}" <<INPUTS
schema_id=${schema_id}.inputs.v2
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
data_closure_path=${closure_report}
data_closure_sha256=$(sha256_of "${closure_report}")
closure_verifier_path=${closure_verifier}
closure_verifier_sha256=$(sha256_of "${closure_verifier}")
fresh_preregistration_path=${fresh_preregistration}
fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")
diagnostic_preregistration_path=${diagnostic_preregistration}
diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")
ablation_preregistration_path=${ablation_preregistration}
ablation_preregistration_sha256=$(sha256_of "${ablation_preregistration}")
protocol_amendment_path=${protocol_amendment}
protocol_amendment_sha256=$(sha256_of "${protocol_amendment}")
localization_addendum_path=${localization_addendum}
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
representation_runner_path=${representation_runner}
representation_runner_sha256=$(sha256_of "${representation_runner}")
mdn_runner_path=${mdn_runner}
mdn_runner_sha256=$(sha256_of "${mdn_runner}")
affine_helper_source_path=${affine_helper}
affine_helper_source_sha256=$(sha256_of "${affine_helper}")
affine_runner_source_path=${affine_runner}
affine_runner_source_sha256=$(sha256_of "${affine_runner}")
frozen_affine_helper_path=${frozen_affine_helper}
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_path=${frozen_affine_runner}
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
mdn_train_config_path=${mdn_train_config}
mdn_train_config_sha256=$(sha256_of "${mdn_train_config}")
mdn_policy_source_path=${mdn_policy_source}
mdn_policy_source_sha256=$(sha256_of "${mdn_policy_source}")
config_snapshot_path=${capture_config}
config_snapshot_sha256=$(sha256_of "${capture_config}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
mdn_result_path=${mdn_result}
mdn_result_sha256=$(sha256_of "${mdn_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
untrained_representation_train_capture_range=[${train_begin},${train_end})
untrained_representation_validation_capture_range=[${validation_begin},${validation_end})
train_expected_probe_rows=${train_rows}
validation_expected_probe_rows=${validation_rows}
certified_expected_probe_rows=${certified_rows}
purge_anchors_captured=false
certified_scoring_attempt_count=1
certified_authorization_trigger_path=${affine_route_trigger}
certified_authorization_required_route=canonical_certification
selection_sources_frozen_before_certified=true
untrained_representation_certified_access=false
forbidden_final_anchor_begin=${forbidden_final_begin}
final_holdout_scored=false
policy_access=false
INPUTS
}

verify_input_receipt() {
  local representation_checkpoint="$1"
  local mdn_checkpoint="$2"
  require_file "${input_receipt}"
  expect_kv "${input_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${input_receipt}" data_closure_sha256 "$(sha256_of "${closure_report}")"
  expect_kv "${input_receipt}" closure_verifier_sha256 "$(sha256_of "${closure_verifier}")"
  expect_kv "${input_receipt}" fresh_preregistration_sha256 "$(sha256_of "${fresh_preregistration}")"
  expect_kv "${input_receipt}" diagnostic_preregistration_sha256 "$(sha256_of "${diagnostic_preregistration}")"
  expect_kv "${input_receipt}" ablation_preregistration_sha256 "$(sha256_of "${ablation_preregistration}")"
  expect_kv "${input_receipt}" protocol_amendment_sha256 "$(sha256_of "${protocol_amendment}")"
  expect_kv "${input_receipt}" localization_addendum_sha256 "$(sha256_of "${localization_addendum}")"
  expect_kv "${input_receipt}" conditional_amendment_sha256 "$(sha256_of "${conditional_amendment}")"
  expect_kv "${input_receipt}" representation_runner_sha256 "$(sha256_of "${representation_runner}")"
  expect_kv "${input_receipt}" mdn_runner_sha256 "$(sha256_of "${mdn_runner}")"
  expect_kv "${input_receipt}" affine_helper_source_sha256 "$(sha256_of "${affine_helper}")"
  expect_kv "${input_receipt}" affine_runner_source_sha256 "$(sha256_of "${affine_runner}")"
  expect_kv "${input_receipt}" frozen_affine_helper_sha256 "$(sha256_of "${frozen_affine_helper}")"
  expect_kv "${input_receipt}" frozen_affine_runner_sha256 "$(sha256_of "${frozen_affine_runner}")"
  expect_kv "${input_receipt}" mdn_train_config_sha256 "$(sha256_of "${mdn_train_config}")"
  expect_kv "${input_receipt}" mdn_policy_source_sha256 "$(sha256_of "${mdn_policy_source}")"
  expect_kv "${input_receipt}" config_snapshot_sha256 "$(sha256_of "${capture_config}")"
  expect_kv "${input_receipt}" untrained_capture_config_sha256 "$(sha256_of "${untrained_capture_config}")"
  expect_kv "${input_receipt}" untrained_mdn_policy_sha256 "$(sha256_of "${untrained_mdn_policy}")"
  expect_kv "${input_receipt}" runtime_exec_sha256 "$(sha256_of "${runtime_exec}")"
  expect_kv "${input_receipt}" representation_result_sha256 "$(sha256_of "${representation_result}")"
  expect_kv "${input_receipt}" mdn_result_sha256 "$(sha256_of "${mdn_result}")"
  expect_kv "${input_receipt}" representation_checkpoint_sha256 "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${input_receipt}" mdn_checkpoint_sha256 "$(sha256_of "${mdn_checkpoint}")"
  expect_kv "${input_receipt}" purge_anchors_captured false
  expect_kv "${input_receipt}" certified_scoring_attempt_count 1
  expect_kv "${input_receipt}" certified_authorization_trigger_path \
    "${affine_route_trigger}"
  expect_kv "${input_receipt}" certified_authorization_required_route \
    canonical_certification
  expect_kv "${input_receipt}" selection_sources_frozen_before_certified true
  expect_kv "${input_receipt}" untrained_representation_certified_access false
  expect_kv "${input_receipt}" untrained_representation_train_capture_range \
    "[${train_begin},${train_end})"
  expect_kv "${input_receipt}" untrained_representation_validation_capture_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${input_receipt}" forbidden_final_anchor_begin "${forbidden_final_begin}"
  expect_kv "${input_receipt}" final_holdout_scored false
  expect_kv "${input_receipt}" policy_access false
}

validate_probe_file() {
  local probe="$1"
  local begin="$2"
  local end="$3"
  local expected_rows="$4"
  local expected_schema="$5"
  local expected_width="$6"
  local rows min_anchor max_anchor
  require_file "${probe}"
  awk -F, -v schema="${expected_schema}" -v width="${expected_width}" '
    NR == 1 {
      expected = "record_schema,anchor_key,anchor_index,anchor_local_index," \
                 "edge_index,edge_id,base_node_id,quote_node_id," \
                 "channel_index,target_edge_close_return,feature_count," \
                 "feature_values";
      if ($0 != expected) exit 41;
      next;
    }
    NF != 12 || $1 != schema || $11 + 0 != width { exit 42 }
  ' "${probe}" || fail "probe schema or width mismatch: ${probe}"
  rows="$(awk 'NR > 1 { ++rows } END { print rows + 0 }' "${probe}")"
  [[ "${rows}" == "${expected_rows}" ]] ||
    fail "probe row count mismatch: expected ${expected_rows}, got ${rows}"
  min_anchor="$(awk -F, 'NR == 2 { min = $3 + 0 } NR > 1 && $3 + 0 < min { min = $3 + 0 } END { print min + 0 }' "${probe}")"
  max_anchor="$(awk -F, 'NR > 1 { if ($3 + 0 > max) max = $3 + 0 } END { print max + 0 }' "${probe}")"
  [[ "${min_anchor}" == "${begin}" && "${max_anchor}" == "$((end - 1))" ]] ||
    fail "probe anchor range mismatch: [${min_anchor},${max_anchor}]"
}

validate_job() {
  local job="$1"
  local begin="$2"
  local end="$3"
  local expected_rows="$4"
  local representation_checkpoint="$5"
  local mdn_checkpoint="$6"
  local expected_config="${7:-${capture_config}}"
  local result="${job}/runtime.result.fact"
  local manifest="${job}/job.manifest"
  local report="${job}/channel_inference.report"
  local probe="${job}/mdn_edge_context_features.probe"
  local representation_probe="${job}/representation_edge_features.probe"
  require_file "${result}"
  require_file "${manifest}"
  require_file "${report}"
  require_file "${probe}"
  require_file "${representation_probe}"
  expect_kv "${result}" status completed
  expect_kv "${result}" optimizer_steps 0
  expect_kv "${result}" checkpoint_written false
  expect_kv "${result}" model_state_mutated false
  expect_kv "${manifest}" config_path "${expected_config}"
  expect_kv "${manifest}" wave_id cwu_02v_certified_replay_eval_mdn
  expect_kv "${manifest}" wave_action run
  expect_kv "${manifest}" resolved_anchor_index_begin "${begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${end}"
  expect_kv "${manifest}" accepted_anchor_count 4096
  expect_kv "${manifest}" input_representation_checkpoint_path "${representation_checkpoint}"
  expect_kv "${manifest}" input_mdn_checkpoint_path "${mdn_checkpoint}"
  if [[ -n "${representation_checkpoint}" ]]; then
    expect_kv "${report}" representation_checkpoint_loaded true
    expect_kv "${report}" representation_checkpoint_path "${representation_checkpoint}"
    expect_kv "${report}" seed 31
    expect_kv "${report}" allow_untrained_representation false
  else
    expect_kv "${report}" representation_checkpoint_loaded false
    expect_kv "${report}" representation_checkpoint_path ''
    expect_kv "${report}" seed 17
    expect_kv "${report}" allow_untrained_representation true
  fi
  expect_kv "${report}" mdn_checkpoint_loaded true
  expect_kv "${report}" mdn_checkpoint_path "${mdn_checkpoint}"
  expect_kv "${report}" requested_anchor_index_begin "${begin}"
  expect_kv "${report}" requested_anchor_index_end "${end}"
  expect_kv "${report}" wave_streamed_anchor_count "$((end - begin))"
  expect_kv "${report}" mdn_edge_context_feature_probe_written true
  expect_kv "${report}" mdn_edge_context_feature_probe_row_count "${expected_rows}"
  expect_kv "${report}" mdn_edge_context_feature_probe_path "${probe}"
  expect_kv "${report}" representation_edge_feature_probe_written true
  expect_kv "${report}" representation_edge_feature_probe_row_count "${expected_rows}"
  expect_kv "${report}" representation_edge_feature_probe_path "${representation_probe}"
  expect_kv "${report}" representation_edge_feature_probe_error ''
  expect_kv "${report}" edge_return_projection_valid_count "${expected_rows}"
  expect_kv "${report}" direct_edge_return_readout_valid_count "${expected_rows}"
  validate_probe_file "${probe}" "${begin}" "${end}" "${expected_rows}" \
    kikijyeba.synthetic.mdn_edge_context_feature_probe.v1 400
  validate_probe_file "${representation_probe}" "${begin}" "${end}" \
    "${expected_rows}" \
    kikijyeba.synthetic.representation_edge_feature_probe.v1 96
  printf '%s' "${probe}"
}

assert_certified_artifacts_absent() {
  local path
  for path in "${certified_job}" "${certified_attempt_receipt}" \
    "${result_receipt}" "${runtime_root}/certified.log"; do
    [[ ! -e "${path}" ]] ||
      fail "development-only phase found forbidden certified artifact: ${path}"
  done
}

write_development_receipt() {
  local representation_checkpoint="$1"
  local mdn_checkpoint="$2"
  local train_probe="$3"
  local validation_probe="$4"
  local candidate
  assert_certified_artifacts_absent
  candidate="$(mktemp "${runtime_root}/.development.XXXXXX")"
  cat >"${candidate}" <<DEVELOPMENT
schema_id=${schema_id}.development.v1
status=complete
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
data_closure_path=${closure_report}
data_closure_sha256=$(sha256_of "${closure_report}")
fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")
diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")
ablation_preregistration_sha256=$(sha256_of "${ablation_preregistration}")
protocol_amendment_sha256=$(sha256_of "${protocol_amendment}")
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
config_snapshot_path=${capture_config}
config_snapshot_sha256=$(sha256_of "${capture_config}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
frozen_affine_helper_path=${frozen_affine_helper}
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_path=${frozen_affine_runner}
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
untrained_train_representation_probe_path=${untrained_train_job}/representation_edge_features.probe
untrained_train_representation_probe_sha256=$(sha256_of "${untrained_train_job}/representation_edge_features.probe")
untrained_train_probe_rows=${train_rows}
untrained_train_report_path=${untrained_train_job}/channel_inference.report
untrained_train_report_sha256=$(sha256_of "${untrained_train_job}/channel_inference.report")
untrained_train_manifest_path=${untrained_train_job}/job.manifest
untrained_train_manifest_sha256=$(sha256_of "${untrained_train_job}/job.manifest")
untrained_train_runtime_result_path=${untrained_train_job}/runtime.result.fact
untrained_train_runtime_result_sha256=$(sha256_of "${untrained_train_job}/runtime.result.fact")
untrained_validation_representation_probe_path=${untrained_validation_job}/representation_edge_features.probe
untrained_validation_representation_probe_sha256=$(sha256_of "${untrained_validation_job}/representation_edge_features.probe")
untrained_validation_probe_rows=${validation_rows}
untrained_validation_report_path=${untrained_validation_job}/channel_inference.report
untrained_validation_report_sha256=$(sha256_of "${untrained_validation_job}/channel_inference.report")
untrained_validation_manifest_path=${untrained_validation_job}/job.manifest
untrained_validation_manifest_sha256=$(sha256_of "${untrained_validation_job}/job.manifest")
untrained_validation_runtime_result_path=${untrained_validation_job}/runtime.result.fact
untrained_validation_runtime_result_sha256=$(sha256_of "${untrained_validation_job}/runtime.result.fact")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
train_probe_rows=${train_rows}
train_representation_probe_path=${train_job}/representation_edge_features.probe
train_representation_probe_sha256=$(sha256_of "${train_job}/representation_edge_features.probe")
train_report_path=${train_job}/channel_inference.report
train_report_sha256=$(sha256_of "${train_job}/channel_inference.report")
train_manifest_path=${train_job}/job.manifest
train_manifest_sha256=$(sha256_of "${train_job}/job.manifest")
train_runtime_result_path=${train_job}/runtime.result.fact
train_runtime_result_sha256=$(sha256_of "${train_job}/runtime.result.fact")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
validation_probe_rows=${validation_rows}
validation_representation_probe_path=${validation_job}/representation_edge_features.probe
validation_representation_probe_sha256=$(sha256_of "${validation_job}/representation_edge_features.probe")
validation_report_path=${validation_job}/channel_inference.report
validation_report_sha256=$(sha256_of "${validation_job}/channel_inference.report")
validation_manifest_path=${validation_job}/job.manifest
validation_manifest_sha256=$(sha256_of "${validation_job}/job.manifest")
validation_runtime_result_path=${validation_job}/runtime.result.fact
validation_runtime_result_sha256=$(sha256_of "${validation_job}/runtime.result.fact")
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
untrained_representation_train_capture_range=[${train_begin},${train_end})
untrained_representation_validation_capture_range=[${validation_begin},${validation_end})
maximum_anchor_read=$((validation_end - 1))
certified_capture_created=false
certified_attempt_created=false
certified_result_created=false
purge_anchors_captured=false
untrained_representation_certified_access=false
affine_route_trigger_path=${affine_route_trigger}
final_holdout_scored=false
policy_access=false
DEVELOPMENT
  chmod 0444 "${candidate}"
  if [[ -e "${development_receipt}" ]]; then
    cmp -s "${candidate}" "${development_receipt}" ||
      fail "immutable development receipt drifted"
    rm -f -- "${candidate}"
  else
    ln "${candidate}" "${development_receipt}" ||
      fail "development receipt publication failed"
    rm -f -- "${candidate}"
  fi
}

verify_bound_file() {
  local receipt="$1"
  local path_key="$2"
  local sha_key="$3"
  local expected_path="$4"
  expect_kv "${receipt}" "${path_key}" "${expected_path}"
  require_file "${expected_path}"
  expect_kv "${receipt}" "${sha_key}" "$(sha256_of "${expected_path}")"
}

verify_development_core() {
  verify_static_inputs
  require_file "${capture_config}"
  require_file "${untrained_capture_config}"
  require_file "${untrained_mdn_policy}"
  require_file "${development_receipt}"
  [[ "$(stat -c '%A' -- "${development_receipt}")" != *w* ]] ||
    fail "development receipt is writable"
  verify_selection_sources
  local representation_checkpoint mdn_checkpoint train_probe validation_probe
  representation_checkpoint="$(checkpoint_from_result "${representation_result}")"
  mdn_checkpoint="$(checkpoint_from_result "${mdn_result}")"
  verify_input_receipt "${representation_checkpoint}" "${mdn_checkpoint}"
  validate_job "${untrained_train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" '' "${mdn_checkpoint}" "${untrained_capture_config}" \
    >/dev/null
  validate_job "${untrained_validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" '' "${mdn_checkpoint}" \
    "${untrained_capture_config}" >/dev/null
  train_probe="$(validate_job "${train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" "${representation_checkpoint}" "${mdn_checkpoint}")"
  validation_probe="$(validate_job "${validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${representation_checkpoint}" \
    "${mdn_checkpoint}")"

  expect_kv "${development_receipt}" schema_id "${schema_id}.development.v1"
  expect_kv "${development_receipt}" status complete
  verify_bound_file "${development_receipt}" runner_path runner_sha256 \
    "${script_path}"
  verify_bound_file "${development_receipt}" input_receipt_path \
    input_receipt_sha256 "${input_receipt}"
  verify_bound_file "${development_receipt}" data_closure_path \
    data_closure_sha256 "${closure_report}"
  verify_bound_file "${development_receipt}" conditional_amendment_path \
    conditional_amendment_sha256 "${conditional_amendment}"
  verify_bound_file "${development_receipt}" config_snapshot_path \
    config_snapshot_sha256 "${capture_config}"
  verify_bound_file "${development_receipt}" untrained_capture_config_path \
    untrained_capture_config_sha256 "${untrained_capture_config}"
  verify_bound_file "${development_receipt}" untrained_mdn_policy_path \
    untrained_mdn_policy_sha256 "${untrained_mdn_policy}"
  verify_bound_file "${development_receipt}" runtime_exec_path \
    runtime_exec_sha256 "${runtime_exec}"
  verify_bound_file "${development_receipt}" frozen_affine_helper_path \
    frozen_affine_helper_sha256 "${frozen_affine_helper}"
  verify_bound_file "${development_receipt}" frozen_affine_runner_path \
    frozen_affine_runner_sha256 "${frozen_affine_runner}"
  expect_kv "${development_receipt}" fresh_preregistration_sha256 \
    "$(sha256_of "${fresh_preregistration}")"
  expect_kv "${development_receipt}" diagnostic_preregistration_sha256 \
    "$(sha256_of "${diagnostic_preregistration}")"
  expect_kv "${development_receipt}" ablation_preregistration_sha256 \
    "$(sha256_of "${ablation_preregistration}")"
  expect_kv "${development_receipt}" protocol_amendment_sha256 \
    "$(sha256_of "${protocol_amendment}")"
  expect_kv "${development_receipt}" localization_addendum_sha256 \
    "$(sha256_of "${localization_addendum}")"
  expect_kv "${development_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${development_receipt}" representation_checkpoint_sha256 \
    "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${development_receipt}" mdn_checkpoint_path "${mdn_checkpoint}"
  expect_kv "${development_receipt}" mdn_checkpoint_sha256 \
    "$(sha256_of "${mdn_checkpoint}")"
  verify_bound_file "${development_receipt}" \
    untrained_train_representation_probe_path \
    untrained_train_representation_probe_sha256 \
    "${untrained_train_job}/representation_edge_features.probe"
  verify_bound_file "${development_receipt}" untrained_train_report_path \
    untrained_train_report_sha256 "${untrained_train_job}/channel_inference.report"
  verify_bound_file "${development_receipt}" untrained_train_manifest_path \
    untrained_train_manifest_sha256 "${untrained_train_job}/job.manifest"
  verify_bound_file "${development_receipt}" untrained_train_runtime_result_path \
    untrained_train_runtime_result_sha256 \
    "${untrained_train_job}/runtime.result.fact"
  verify_bound_file "${development_receipt}" \
    untrained_validation_representation_probe_path \
    untrained_validation_representation_probe_sha256 \
    "${untrained_validation_job}/representation_edge_features.probe"
  verify_bound_file "${development_receipt}" untrained_validation_report_path \
    untrained_validation_report_sha256 \
    "${untrained_validation_job}/channel_inference.report"
  verify_bound_file "${development_receipt}" untrained_validation_manifest_path \
    untrained_validation_manifest_sha256 "${untrained_validation_job}/job.manifest"
  verify_bound_file "${development_receipt}" \
    untrained_validation_runtime_result_path \
    untrained_validation_runtime_result_sha256 \
    "${untrained_validation_job}/runtime.result.fact"
  verify_bound_file "${development_receipt}" train_probe_path train_probe_sha256 \
    "${train_probe}"
  verify_bound_file "${development_receipt}" train_representation_probe_path \
    train_representation_probe_sha256 \
    "${train_job}/representation_edge_features.probe"
  verify_bound_file "${development_receipt}" train_report_path train_report_sha256 \
    "${train_job}/channel_inference.report"
  verify_bound_file "${development_receipt}" train_manifest_path \
    train_manifest_sha256 "${train_job}/job.manifest"
  verify_bound_file "${development_receipt}" train_runtime_result_path \
    train_runtime_result_sha256 "${train_job}/runtime.result.fact"
  verify_bound_file "${development_receipt}" validation_probe_path \
    validation_probe_sha256 "${validation_probe}"
  verify_bound_file "${development_receipt}" validation_representation_probe_path \
    validation_representation_probe_sha256 \
    "${validation_job}/representation_edge_features.probe"
  verify_bound_file "${development_receipt}" validation_report_path \
    validation_report_sha256 "${validation_job}/channel_inference.report"
  verify_bound_file "${development_receipt}" validation_manifest_path \
    validation_manifest_sha256 "${validation_job}/job.manifest"
  verify_bound_file "${development_receipt}" validation_runtime_result_path \
    validation_runtime_result_sha256 "${validation_job}/runtime.result.fact"
  expect_kv "${development_receipt}" untrained_train_probe_rows "${train_rows}"
  expect_kv "${development_receipt}" untrained_validation_probe_rows \
    "${validation_rows}"
  expect_kv "${development_receipt}" train_probe_rows "${train_rows}"
  expect_kv "${development_receipt}" validation_probe_rows "${validation_rows}"
  expect_kv "${development_receipt}" train_capture_range \
    "[${train_begin},${train_end})"
  expect_kv "${development_receipt}" validation_capture_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${development_receipt}" maximum_anchor_read \
    "$((validation_end - 1))"
  expect_kv "${development_receipt}" certified_capture_created false
  expect_kv "${development_receipt}" certified_attempt_created false
  expect_kv "${development_receipt}" certified_result_created false
  expect_kv "${development_receipt}" purge_anchors_captured false
  expect_kv "${development_receipt}" untrained_representation_certified_access false
  expect_kv "${development_receipt}" affine_route_trigger_path \
    "${affine_route_trigger}"
  expect_kv "${development_receipt}" final_holdout_scored false
  expect_kv "${development_receipt}" policy_access false
}

numeric_gate_boolean() {
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
  ' || fail "invalid numeric gate input: value=${value}, comparison=${comparison}"
}

validate_development_affine_report() {
  local report="$1"
  local expected_schema="$2"
  local expected_probe_kind="$3"
  require_file "${report}"
  expect_kv "${report}" schema_id "${expected_schema}"
  expect_kv "${report}" status complete
  expect_kv "${report}" probe_kind "${expected_probe_kind}"
  expect_kv "${report}" certified_probe_rows 0
  expect_kv "${report}" certified_anchor_range not_opened
  expect_kv "${report}" certified_candidates_scored 0
  expect_kv "${report}" maximum_anchor_read "$((validation_end - 1))"
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
}

verify_affine_trigger_structure() {
  local route direction_gate rank_gate correlation_gate rmse_gate strong_gate
  local raw96_report post384_report untrained_raw96_report affine_binary
  local affine_development gate report_strong computed_strong
  local computed_direction computed_rank computed_correlation computed_rmse
  require_file "${affine_route_trigger}"
  [[ "$(stat -c '%A' -- "${affine_route_trigger}")" != *w* ]] ||
    fail "affine route trigger is writable"
  expect_kv "${affine_route_trigger}" schema_id \
    synthetic_v2_frozen_affine_route_trigger_v1
  expect_kv "${affine_route_trigger}" status complete
  expect_kv "${affine_route_trigger}" maximum_anchor_read \
    "$((validation_end - 1))"
  expect_kv "${affine_route_trigger}" certified_capture_opened false
  expect_kv "${affine_route_trigger}" final_holdout_access false
  expect_kv "${affine_route_trigger}" policy_access false
  verify_bound_file "${affine_route_trigger}" capture_development_path \
    capture_development_sha256 "${development_receipt}"
  verify_bound_file "${affine_route_trigger}" capture_runner_path \
    capture_runner_sha256 "${script_path}"
  verify_bound_file "${affine_route_trigger}" affine_runner_path \
    affine_runner_sha256 "${frozen_affine_runner}"
  verify_bound_file "${affine_route_trigger}" affine_helper_path \
    affine_helper_sha256 "${frozen_affine_helper}"
  affine_binary="$(kv affine_binary_path "${affine_route_trigger}")"
  require_file "${affine_binary}"
  expect_kv "${affine_route_trigger}" affine_binary_sha256 \
    "$(sha256_of "${affine_binary}")"
  affine_development="$(kv affine_development_receipt_path \
    "${affine_route_trigger}")"
  require_file "${affine_development}"
  expect_kv "${affine_route_trigger}" affine_development_receipt_sha256 \
    "$(sha256_of "${affine_development}")"
  expect_kv "${affine_development}" status complete
  expect_kv "${affine_development}" maximum_anchor_read \
    "$((validation_end - 1))"

  raw96_report="$(kv raw96_validation_report_path "${affine_route_trigger}")"
  require_file "${raw96_report}"
  expect_kv "${affine_route_trigger}" raw96_validation_report_sha256 \
    "$(sha256_of "${raw96_report}")"
  post384_report="$(kv post384_validation_report_path \
    "${affine_route_trigger}")"
  require_file "${post384_report}"
  expect_kv "${affine_route_trigger}" post384_validation_report_sha256 \
    "$(sha256_of "${post384_report}")"
  untrained_raw96_report="$(kv untrained_raw96_validation_report_path \
    "${affine_route_trigger}")"
  require_file "${untrained_raw96_report}"
  expect_kv "${affine_route_trigger}" \
    untrained_raw96_validation_report_sha256 \
    "$(sha256_of "${untrained_raw96_report}")"

  validate_development_affine_report "${raw96_report}" \
    synthetic_v2_frozen_encoder_affine_development_v1 representation
  validate_development_affine_report "${post384_report}" \
    synthetic_v2_frozen_representation_affine_development_v1 mdn_context
  validate_development_affine_report "${untrained_raw96_report}" \
    synthetic_v2_untrained_encoder_affine_control_v1 representation

  route="$(kv route "${affine_route_trigger}")"
  direction_gate="$(kv raw96_validation_direction_gate_pass "${affine_route_trigger}")"
  rank_gate="$(kv raw96_validation_pairwise_rank_gate_pass "${affine_route_trigger}")"
  correlation_gate="$(kv raw96_validation_correlation_gate_pass "${affine_route_trigger}")"
  rmse_gate="$(kv raw96_validation_rmse_ratio_gate_pass "${affine_route_trigger}")"
  strong_gate="$(kv raw96_validation_strong_gate_pass "${affine_route_trigger}")"
  for gate in "${direction_gate}" "${rank_gate}" "${correlation_gate}" \
    "${rmse_gate}" "${strong_gate}"; do
    [[ "${gate}" == true || "${gate}" == false ]] ||
      fail "affine route trigger contains a non-boolean gate: ${gate}"
  done
  computed_direction="$(numeric_gate_boolean \
    "$(kv selected.validation.directional_accuracy "${raw96_report}")" \
    ge 0.95)"
  computed_rank="$(numeric_gate_boolean \
    "$(kv selected.validation.pairwise_rank_accuracy "${raw96_report}")" \
    ge 0.95)"
  computed_correlation="$(numeric_gate_boolean \
    "$(kv selected.validation.correlation "${raw96_report}")" ge 0.95)"
  computed_rmse="$(numeric_gate_boolean \
    "$(kv selected.validation.rmse_target_rms_ratio "${raw96_report}")" \
    le 0.25)"
  [[ "${direction_gate}" == "${computed_direction}" ]] ||
    fail "trigger direction gate disagrees with the raw96 development report"
  [[ "${rank_gate}" == "${computed_rank}" ]] ||
    fail "trigger rank gate disagrees with the raw96 development report"
  [[ "${correlation_gate}" == "${computed_correlation}" ]] ||
    fail "trigger correlation gate disagrees with the raw96 development report"
  [[ "${rmse_gate}" == "${computed_rmse}" ]] ||
    fail "trigger RMSE-ratio gate disagrees with the raw96 development report"
  computed_strong=false
  if [[ "${computed_direction}" == true && "${computed_rank}" == true && \
    "${computed_correlation}" == true && "${computed_rmse}" == true ]]; then
    computed_strong=true
  fi
  report_strong="$(kv validation_strong_gate_pass "${raw96_report}")"
  [[ "${report_strong}" == true || "${report_strong}" == false ]] ||
    fail "raw96 development report has invalid validation_strong_gate_pass"
  [[ "${report_strong}" == "${computed_strong}" ]] ||
    fail "raw96 report strong gate disagrees with its selected validation metrics"
  [[ "${strong_gate}" == "${report_strong}" ]] ||
    fail "trigger strong gate disagrees with the raw96 development report"
  case "${route}" in
  canonical_certification)
    [[ "${direction_gate}" == true && "${rank_gate}" == true && \
      "${correlation_gate}" == true && "${rmse_gate}" == true && \
      "${strong_gate}" == true ]] ||
      fail "canonical certification route is inconsistent with raw96 gates"
    ;;
  representation_ablation_screen)
    [[ "${strong_gate}" == false ]] ||
      fail "ablation route is inconsistent with raw96 strong gate"
    [[ "${direction_gate}" == false || "${rank_gate}" == false || \
      "${correlation_gate}" == false || "${rmse_gate}" == false ]] ||
      fail "ablation route lacks a failed raw96 component gate"
    ;;
  *) fail "unknown affine route: ${route}" ;;
  esac
}

require_canonical_certification_trigger() {
  verify_affine_trigger_structure
  local route
  route="$(kv route "${affine_route_trigger}")"
  if [[ "${route}" == representation_ablation_screen ]]; then
    fail "route=representation_ablation_screen permanently forbids canonical certified capture"
  fi
  [[ "${route}" == canonical_certification ]] ||
    fail "canonical certification was not authorized"
}

verify_development_completed() {
  assert_certified_artifacts_absent
  verify_development_core
  if [[ -e "${affine_route_trigger}" ]]; then
    verify_affine_trigger_structure
  fi
  echo "development_receipt_path=${development_receipt}"
  echo "maximum_anchor_read=$((validation_end - 1))"
  echo "certified_capture_created=false"
}

run_job_once() {
  local job="$1"
  local log="$2"
  local begin="$3"
  local end="$4"
  local representation_checkpoint="$5"
  local mdn_checkpoint="$6"
  if [[ -e "${job}" ]]; then
    return
  fi
  "${runtime_exec}" \
    --config "${capture_config}" \
    --job-dir "${job}" \
    --source-range anchor_index \
    --anchor-index-begin "${begin}" \
    --anchor-index-end "${end}" \
    --input-representation-checkpoint "${representation_checkpoint}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "capture failed irrecoverably for [${begin},${end}); see ${log}"
}

run_untrained_job_once() {
  local job="$1"
  local log="$2"
  local begin="$3"
  local end="$4"
  local mdn_checkpoint="$5"
  if [[ -e "${job}" ]]; then
    return
  fi
  "${runtime_exec}" \
    --config "${untrained_capture_config}" \
    --job-dir "${job}" \
    --source-range anchor_index \
    --anchor-index-begin "${begin}" \
    --anchor-index-end "${end}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "untrained representation capture failed for [${begin},${end}); see ${log}"
}

run_certified_job_once() {
  local representation_checkpoint="$1"
  local mdn_checkpoint="$2"
  local log="${runtime_root}/certified.log"
  require_canonical_certification_trigger
  if [[ -e "${certified_job}" ]]; then
    require_file "${certified_attempt_receipt}"
    return
  fi
  [[ ! -e "${certified_attempt_receipt}" ]] ||
    fail "certified attempt was already consumed; refusing a retry"
  local candidate
  candidate="$(mktemp "${runtime_root}/.certified_attempt.XXXXXX")"
  cat >"${candidate}" <<ATTEMPT
schema_id=${schema_id}.certified_attempt.v1
status=authorized_and_started
attempt_ordinal=1
certified_anchor_range=[${certified_begin},${certified_end})
job_dir=${certified_job}
capture_runner_sha256=$(sha256_of "${script_path}")
input_receipt_sha256=$(sha256_of "${input_receipt}")
development_receipt_path=${development_receipt}
development_receipt_sha256=$(sha256_of "${development_receipt}")
affine_route_trigger_path=${affine_route_trigger}
affine_route_trigger_sha256=$(sha256_of "${affine_route_trigger}")
route=canonical_certification
config_snapshot_sha256=$(sha256_of "${capture_config}")
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
final_holdout_scored=false
policy_access=false
ATTEMPT
  chmod 0444 "${candidate}"
  ln "${candidate}" "${certified_attempt_receipt}" ||
    fail "could not publish certified-attempt receipt"
  rm -f -- "${candidate}"
  "${runtime_exec}" \
    --config "${capture_config}" \
    --job-dir "${certified_job}" \
    --source-range anchor_index \
    --anchor-index-begin "${certified_begin}" \
    --anchor-index-end "${certified_end}" \
    --input-representation-checkpoint "${representation_checkpoint}" \
    --input-mdn-checkpoint "${mdn_checkpoint}" \
    --no-replay-artifacts >"${log}" 2>&1 ||
    fail "certified capture failed irrecoverably; retry is forbidden; see ${log}"
}

write_result_receipt() {
  local representation_checkpoint="$1"
  local mdn_checkpoint="$2"
  local train_probe="$3"
  local validation_probe="$4"
  local certified_probe="$5"
  local candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  cat >"${candidate}" <<RESULT
schema_id=${schema_id}.result.v2
status=complete
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
development_receipt_path=${development_receipt}
development_receipt_sha256=$(sha256_of "${development_receipt}")
affine_route_trigger_path=${affine_route_trigger}
affine_route_trigger_sha256=$(sha256_of "${affine_route_trigger}")
route=canonical_certification
config_snapshot_path=${capture_config}
config_snapshot_sha256=$(sha256_of "${capture_config}")
untrained_capture_config_path=${untrained_capture_config}
untrained_capture_config_sha256=$(sha256_of "${untrained_capture_config}")
untrained_mdn_policy_path=${untrained_mdn_policy}
untrained_mdn_policy_sha256=$(sha256_of "${untrained_mdn_policy}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
frozen_affine_helper_path=${frozen_affine_helper}
frozen_affine_helper_sha256=$(sha256_of "${frozen_affine_helper}")
frozen_affine_runner_path=${frozen_affine_runner}
frozen_affine_runner_sha256=$(sha256_of "${frozen_affine_runner}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
mdn_checkpoint_path=${mdn_checkpoint}
mdn_checkpoint_sha256=$(sha256_of "${mdn_checkpoint}")
untrained_train_representation_probe_path=${untrained_train_job}/representation_edge_features.probe
untrained_train_representation_probe_sha256=$(sha256_of "${untrained_train_job}/representation_edge_features.probe")
untrained_train_probe_rows=${train_rows}
untrained_train_report_path=${untrained_train_job}/channel_inference.report
untrained_train_report_sha256=$(sha256_of "${untrained_train_job}/channel_inference.report")
untrained_train_manifest_path=${untrained_train_job}/job.manifest
untrained_train_manifest_sha256=$(sha256_of "${untrained_train_job}/job.manifest")
untrained_train_runtime_result_path=${untrained_train_job}/runtime.result.fact
untrained_train_runtime_result_sha256=$(sha256_of "${untrained_train_job}/runtime.result.fact")
untrained_validation_representation_probe_path=${untrained_validation_job}/representation_edge_features.probe
untrained_validation_representation_probe_sha256=$(sha256_of "${untrained_validation_job}/representation_edge_features.probe")
untrained_validation_probe_rows=${validation_rows}
untrained_validation_report_path=${untrained_validation_job}/channel_inference.report
untrained_validation_report_sha256=$(sha256_of "${untrained_validation_job}/channel_inference.report")
untrained_validation_manifest_path=${untrained_validation_job}/job.manifest
untrained_validation_manifest_sha256=$(sha256_of "${untrained_validation_job}/job.manifest")
untrained_validation_runtime_result_path=${untrained_validation_job}/runtime.result.fact
untrained_validation_runtime_result_sha256=$(sha256_of "${untrained_validation_job}/runtime.result.fact")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
train_probe_rows=${train_rows}
train_representation_probe_path=${train_job}/representation_edge_features.probe
train_representation_probe_sha256=$(sha256_of "${train_job}/representation_edge_features.probe")
train_report_path=${train_job}/channel_inference.report
train_report_sha256=$(sha256_of "${train_job}/channel_inference.report")
train_manifest_path=${train_job}/job.manifest
train_manifest_sha256=$(sha256_of "${train_job}/job.manifest")
train_runtime_result_path=${train_job}/runtime.result.fact
train_runtime_result_sha256=$(sha256_of "${train_job}/runtime.result.fact")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
validation_probe_rows=${validation_rows}
validation_representation_probe_path=${validation_job}/representation_edge_features.probe
validation_representation_probe_sha256=$(sha256_of "${validation_job}/representation_edge_features.probe")
validation_report_path=${validation_job}/channel_inference.report
validation_report_sha256=$(sha256_of "${validation_job}/channel_inference.report")
validation_manifest_path=${validation_job}/job.manifest
validation_manifest_sha256=$(sha256_of "${validation_job}/job.manifest")
validation_runtime_result_path=${validation_job}/runtime.result.fact
validation_runtime_result_sha256=$(sha256_of "${validation_job}/runtime.result.fact")
certified_probe_path=${certified_probe}
certified_probe_sha256=$(sha256_of "${certified_probe}")
certified_probe_rows=${certified_rows}
certified_representation_probe_path=${certified_job}/representation_edge_features.probe
certified_representation_probe_sha256=$(sha256_of "${certified_job}/representation_edge_features.probe")
certified_report_path=${certified_job}/channel_inference.report
certified_report_sha256=$(sha256_of "${certified_job}/channel_inference.report")
certified_manifest_path=${certified_job}/job.manifest
certified_manifest_sha256=$(sha256_of "${certified_job}/job.manifest")
certified_runtime_result_path=${certified_job}/runtime.result.fact
certified_runtime_result_sha256=$(sha256_of "${certified_job}/runtime.result.fact")
certified_attempt_receipt_path=${certified_attempt_receipt}
certified_attempt_receipt_sha256=$(sha256_of "${certified_attempt_receipt}")
train_capture_range=[${train_begin},${train_end})
validation_capture_range=[${validation_begin},${validation_end})
certified_capture_range=[${certified_begin},${certified_end})
untrained_representation_train_capture_range=[${train_begin},${train_end})
untrained_representation_validation_capture_range=[${validation_begin},${validation_end})
purge_anchors_captured=false
certified_scoring_attempt_count=1
selection_sources_frozen_before_certified=true
untrained_representation_certified_access=false
maximum_anchor_scored=$((certified_end - 1))
final_holdout_scored=false
policy_access=false
RESULT
  chmod 0444 "${candidate}"
  if [[ -e "${result_receipt}" ]]; then
    cmp -s "${candidate}" "${result_receipt}" || fail "result receipt drifted"
    rm -f -- "${candidate}"
  else
    ln "${candidate}" "${result_receipt}" || fail "result receipt publication failed"
    rm -f -- "${candidate}"
  fi
}

verify_result_bound_file() {
  local path_key="$1"
  local sha_key="$2"
  local expected_path="$3"
  verify_bound_file "${result_receipt}" "${path_key}" "${sha_key}" \
    "${expected_path}"
}

verify_completed() {
  verify_development_core
  require_canonical_certification_trigger
  verify_static_inputs
  require_file "${capture_config}"
  require_file "${untrained_capture_config}"
  require_file "${untrained_mdn_policy}"
  verify_selection_sources
  local representation_checkpoint mdn_checkpoint
  local train_probe validation_probe certified_probe
  representation_checkpoint="$(checkpoint_from_result "${representation_result}")"
  mdn_checkpoint="$(checkpoint_from_result "${mdn_result}")"
  verify_input_receipt "${representation_checkpoint}" "${mdn_checkpoint}"
  validate_job "${untrained_train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" '' "${mdn_checkpoint}" "${untrained_capture_config}" \
    >/dev/null
  validate_job "${untrained_validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" '' "${mdn_checkpoint}" \
    "${untrained_capture_config}" >/dev/null
  train_probe="$(validate_job "${train_job}" "${train_begin}" "${train_end}" "${train_rows}" "${representation_checkpoint}" "${mdn_checkpoint}")"
  validation_probe="$(validate_job "${validation_job}" "${validation_begin}" "${validation_end}" "${validation_rows}" "${representation_checkpoint}" "${mdn_checkpoint}")"
  certified_probe="$(validate_job "${certified_job}" "${certified_begin}" "${certified_end}" "${certified_rows}" "${representation_checkpoint}" "${mdn_checkpoint}")"
  require_file "${result_receipt}"
  expect_kv "${result_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${result_receipt}" representation_checkpoint_sha256 \
    "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${result_receipt}" mdn_checkpoint_path "${mdn_checkpoint}"
  expect_kv "${result_receipt}" mdn_checkpoint_sha256 \
    "$(sha256_of "${mdn_checkpoint}")"
  expect_kv "${result_receipt}" train_probe_rows "${train_rows}"
  expect_kv "${result_receipt}" validation_probe_rows "${validation_rows}"
  expect_kv "${result_receipt}" certified_probe_rows "${certified_rows}"
  expect_kv "${result_receipt}" train_capture_range \
    "[${train_begin},${train_end})"
  expect_kv "${result_receipt}" validation_capture_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${result_receipt}" certified_capture_range \
    "[${certified_begin},${certified_end})"
  expect_kv "${result_receipt}" purge_anchors_captured false
  verify_result_bound_file runner_path runner_sha256 "${script_path}"
  verify_result_bound_file input_receipt_path input_receipt_sha256 "${input_receipt}"
  verify_result_bound_file development_receipt_path \
    development_receipt_sha256 "${development_receipt}"
  verify_result_bound_file affine_route_trigger_path \
    affine_route_trigger_sha256 "${affine_route_trigger}"
  expect_kv "${result_receipt}" route canonical_certification
  verify_result_bound_file conditional_amendment_path \
    conditional_amendment_sha256 "${conditional_amendment}"
  verify_result_bound_file config_snapshot_path config_snapshot_sha256 "${capture_config}"
  verify_result_bound_file untrained_capture_config_path untrained_capture_config_sha256 "${untrained_capture_config}"
  verify_result_bound_file untrained_mdn_policy_path untrained_mdn_policy_sha256 "${untrained_mdn_policy}"
  verify_result_bound_file runtime_exec_path runtime_exec_sha256 "${runtime_exec}"
  expect_kv "${result_receipt}" localization_addendum_sha256 "$(sha256_of "${localization_addendum}")"
  verify_result_bound_file frozen_affine_helper_path frozen_affine_helper_sha256 "${frozen_affine_helper}"
  verify_result_bound_file frozen_affine_runner_path frozen_affine_runner_sha256 "${frozen_affine_runner}"
  verify_result_bound_file untrained_train_representation_probe_path untrained_train_representation_probe_sha256 "${untrained_train_job}/representation_edge_features.probe"
  verify_result_bound_file untrained_train_report_path untrained_train_report_sha256 "${untrained_train_job}/channel_inference.report"
  verify_result_bound_file untrained_train_manifest_path untrained_train_manifest_sha256 "${untrained_train_job}/job.manifest"
  verify_result_bound_file untrained_train_runtime_result_path untrained_train_runtime_result_sha256 "${untrained_train_job}/runtime.result.fact"
  verify_result_bound_file untrained_validation_representation_probe_path untrained_validation_representation_probe_sha256 "${untrained_validation_job}/representation_edge_features.probe"
  verify_result_bound_file untrained_validation_report_path untrained_validation_report_sha256 "${untrained_validation_job}/channel_inference.report"
  verify_result_bound_file untrained_validation_manifest_path untrained_validation_manifest_sha256 "${untrained_validation_job}/job.manifest"
  verify_result_bound_file untrained_validation_runtime_result_path untrained_validation_runtime_result_sha256 "${untrained_validation_job}/runtime.result.fact"
  expect_kv "${result_receipt}" untrained_train_probe_rows "${train_rows}"
  expect_kv "${result_receipt}" untrained_validation_probe_rows "${validation_rows}"
  expect_kv "${result_receipt}" train_probe_path "${train_probe}"
  expect_kv "${result_receipt}" train_probe_sha256 "$(sha256_of "${train_probe}")"
  verify_result_bound_file train_representation_probe_path train_representation_probe_sha256 "${train_job}/representation_edge_features.probe"
  verify_result_bound_file train_report_path train_report_sha256 "${train_job}/channel_inference.report"
  verify_result_bound_file train_manifest_path train_manifest_sha256 "${train_job}/job.manifest"
  verify_result_bound_file train_runtime_result_path train_runtime_result_sha256 "${train_job}/runtime.result.fact"
  expect_kv "${result_receipt}" validation_probe_path "${validation_probe}"
  expect_kv "${result_receipt}" validation_probe_sha256 "$(sha256_of "${validation_probe}")"
  verify_result_bound_file validation_representation_probe_path validation_representation_probe_sha256 "${validation_job}/representation_edge_features.probe"
  verify_result_bound_file validation_report_path validation_report_sha256 "${validation_job}/channel_inference.report"
  verify_result_bound_file validation_manifest_path validation_manifest_sha256 "${validation_job}/job.manifest"
  verify_result_bound_file validation_runtime_result_path validation_runtime_result_sha256 "${validation_job}/runtime.result.fact"
  expect_kv "${result_receipt}" certified_probe_path "${certified_probe}"
  expect_kv "${result_receipt}" certified_probe_sha256 "$(sha256_of "${certified_probe}")"
  verify_result_bound_file certified_representation_probe_path certified_representation_probe_sha256 "${certified_job}/representation_edge_features.probe"
  verify_result_bound_file certified_report_path certified_report_sha256 "${certified_job}/channel_inference.report"
  verify_result_bound_file certified_manifest_path certified_manifest_sha256 "${certified_job}/job.manifest"
  verify_result_bound_file certified_runtime_result_path certified_runtime_result_sha256 "${certified_job}/runtime.result.fact"
  verify_result_bound_file certified_attempt_receipt_path certified_attempt_receipt_sha256 "${certified_attempt_receipt}"
  expect_kv "${certified_attempt_receipt}" status authorized_and_started
  expect_kv "${certified_attempt_receipt}" attempt_ordinal 1
  expect_kv "${certified_attempt_receipt}" capture_runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${certified_attempt_receipt}" input_receipt_sha256 "$(sha256_of "${input_receipt}")"
  expect_kv "${certified_attempt_receipt}" development_receipt_path \
    "${development_receipt}"
  expect_kv "${certified_attempt_receipt}" development_receipt_sha256 \
    "$(sha256_of "${development_receipt}")"
  expect_kv "${certified_attempt_receipt}" affine_route_trigger_path \
    "${affine_route_trigger}"
  expect_kv "${certified_attempt_receipt}" affine_route_trigger_sha256 \
    "$(sha256_of "${affine_route_trigger}")"
  expect_kv "${certified_attempt_receipt}" route canonical_certification
  expect_kv "${certified_attempt_receipt}" conditional_amendment_sha256 \
    "$(sha256_of "${conditional_amendment}")"
  expect_kv "${certified_attempt_receipt}" frozen_affine_helper_sha256 "$(sha256_of "${frozen_affine_helper}")"
  expect_kv "${certified_attempt_receipt}" frozen_affine_runner_sha256 "$(sha256_of "${frozen_affine_runner}")"
  expect_kv "${result_receipt}" certified_scoring_attempt_count 1
  expect_kv "${result_receipt}" selection_sources_frozen_before_certified true
  expect_kv "${result_receipt}" untrained_representation_certified_access false
  expect_kv "${result_receipt}" untrained_representation_train_capture_range \
    "[${train_begin},${train_end})"
  expect_kv "${result_receipt}" untrained_representation_validation_capture_range \
    "[${validation_begin},${validation_end})"
  expect_kv "${result_receipt}" maximum_anchor_scored "$((certified_end - 1))"
  expect_kv "${result_receipt}" final_holdout_scored false
  expect_kv "${result_receipt}" policy_access false
  echo "train_probe_path=${train_probe}"
  echo "validation_probe_path=${validation_probe}"
  echo "certified_probe_path=${certified_probe}"
}

if [[ "${mode}" == --verify ]]; then
  verify_completed
  exit 0
fi

if [[ "${mode}" == --verify-development ]]; then
  verify_development_completed
  exit 0
fi

((certified_end <= forbidden_final_begin)) || fail "capture crosses final boundary"
verify_static_inputs
mkdir -p "${runtime_root}"
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || fail "another capture holds the execution lock"

if [[ "${mode}" == --run-development ]]; then
  assert_certified_artifacts_absent
  write_config_snapshot
  freeze_selection_sources
  representation_checkpoint="$(checkpoint_from_result "${representation_result}")"
  mdn_checkpoint="$(checkpoint_from_result "${mdn_result}")"

  candidate_inputs="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
  emit_inputs "${candidate_inputs}" "${representation_checkpoint}" \
    "${mdn_checkpoint}"
  chmod 0444 "${candidate_inputs}"
  if [[ -e "${input_receipt}" ]]; then
    cmp -s "${candidate_inputs}" "${input_receipt}" ||
      fail "input receipt drifted"
    rm -f -- "${candidate_inputs}"
  else
    ln "${candidate_inputs}" "${input_receipt}" ||
      fail "input receipt publication failed"
    rm -f -- "${candidate_inputs}"
  fi

  mkdir -p "${job_root}" "${untrained_job_root}"
  run_untrained_job_once "${untrained_train_job}" \
    "${runtime_root}/untrained_train.log" "${train_begin}" "${train_end}" \
    "${mdn_checkpoint}"
  validate_job "${untrained_train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" '' "${mdn_checkpoint}" "${untrained_capture_config}" \
    >/dev/null
  run_untrained_job_once "${untrained_validation_job}" \
    "${runtime_root}/untrained_validation.log" "${validation_begin}" \
    "${validation_end}" "${mdn_checkpoint}"
  validate_job "${untrained_validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" '' "${mdn_checkpoint}" \
    "${untrained_capture_config}" >/dev/null
  run_job_once "${train_job}" "${runtime_root}/train.log" \
    "${train_begin}" "${train_end}" "${representation_checkpoint}" \
    "${mdn_checkpoint}"
  train_probe="$(validate_job "${train_job}" "${train_begin}" "${train_end}" \
    "${train_rows}" "${representation_checkpoint}" "${mdn_checkpoint}")"
  run_job_once "${validation_job}" "${runtime_root}/validation.log" \
    "${validation_begin}" "${validation_end}" "${representation_checkpoint}" \
    "${mdn_checkpoint}"
  validation_probe="$(validate_job "${validation_job}" "${validation_begin}" \
    "${validation_end}" "${validation_rows}" "${representation_checkpoint}" \
    "${mdn_checkpoint}")"
  write_development_receipt "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${train_probe}" "${validation_probe}"
  "${closure_verifier}" --verify >/dev/null
  verify_development_completed
  exit 0
fi

[[ "${mode}" == --run-certified ]] || fail "unreachable capture mode: ${mode}"
if [[ -e "${result_receipt}" ]]; then
  verify_completed
  exit 0
fi
verify_development_core
require_canonical_certification_trigger
representation_checkpoint="$(checkpoint_from_result "${representation_result}")"
mdn_checkpoint="$(checkpoint_from_result "${mdn_result}")"
run_certified_job_once "${representation_checkpoint}" "${mdn_checkpoint}"
certified_probe="$(validate_job "${certified_job}" "${certified_begin}" \
  "${certified_end}" "${certified_rows}" "${representation_checkpoint}" \
  "${mdn_checkpoint}")"
train_probe="$(kv train_probe_path "${development_receipt}")"
validation_probe="$(kv validation_probe_path "${development_receipt}")"
require_file "${train_probe}"
require_file "${validation_probe}"
write_result_receipt "${representation_checkpoint}" "${mdn_checkpoint}" \
  "${train_probe}" "${validation_probe}" "${certified_probe}"
"${closure_verifier}" --verify >/dev/null
verify_completed
