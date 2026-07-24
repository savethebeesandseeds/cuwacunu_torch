#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_frozen_representation_affine_probe_v1"
readonly encoder_schema_id="synthetic_v2_frozen_encoder_affine_probe_v1"
readonly untrained_schema_id="synthetic_v2_untrained_encoder_affine_control_v1"
readonly development_schema_id="synthetic_v2_frozen_affine_development_v1"
readonly mdn_development_schema_id="synthetic_v2_frozen_representation_affine_development_v1"
readonly encoder_development_schema_id="synthetic_v2_frozen_encoder_affine_development_v1"
readonly trigger_schema_id="synthetic_v2_frozen_affine_route_trigger_v1"
readonly expected_closure_sha256="36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831"
readonly expected_preregistration_sha256="de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8"
readonly expected_protocol_amendment_sha256="3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5"
readonly expected_localization_addendum_sha256="2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4"
readonly expected_conditional_amendment_sha256="30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67"
readonly expected_capture_runner_sha256="7cdb4a26442f56ee4af477fdbe77e8bc1d9f7530648379e3839963d5c19a2e01"
readonly train_rows=22464
readonly validation_rows=2304
readonly certified_rows=3456

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
helper_source="${script_dir}/frozen_representation_affine_probe.cpp"
closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
closure_verifier="${script_dir}/prepare_and_seal_fresh_seed.sh"
preregistration="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md"
protocol_amendment="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md"
localization_addendum="${benchmark_root}/REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md"
conditional_amendment="${benchmark_root}/REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md"
capture_runner="${script_dir}/run_frozen_feature_capture_v2.sh"
capture_runtime="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_v1"
capture_development="${capture_runtime}/development.status"
capture_result="${capture_runtime}/result.status"
route_trigger="${capture_runtime}/affine_route_trigger.status"
capture_source_root="${capture_runtime}/frozen_selection_sources"
frozen_helper_source="${capture_source_root}/frozen_representation_affine_probe.cpp"
frozen_runner_source="${capture_source_root}/run_frozen_representation_affine_probe.sh"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
development_runtime="${runtime_parent}/synthetic_v2_frozen_affine_development_v1"
final_runtime="${runtime_parent}/${schema_id}"
artifact_report="${benchmark_root}/artifacts/synthetic_v2_frozen_representation_affine_probe.v1.report"
encoder_artifact_report="${benchmark_root}/artifacts/synthetic_v2_frozen_encoder_affine_probe.v1.report"
untrained_artifact_report="${benchmark_root}/artifacts/synthetic_v2_untrained_encoder_affine_control.v1.report"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"

fail() {
  echo "v2 frozen representation affine probe: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_nonempty_file() {
  require_file "$1"
  [[ -s "$1" ]] || fail "empty required file: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
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

publish_immutable() {
  local candidate="$1"
  local destination="$2"
  chmod 0444 "${candidate}"
  if [[ -e "${destination}" ]]; then
    require_file "${destination}"
    cmp -s -- "${candidate}" "${destination}" ||
      fail "immutable receipt drifted: ${destination}"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${destination}" ||
      fail "could not publish immutable receipt: ${destination}"
    rm -f -- "${candidate}"
  fi
}

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
stage_1_mode=development_only
development_capture_receipt=${capture_development}
development_arms=post_mdn_context_384,raw_encoder_96,untrained_raw_encoder_96
development_anchor_ranges=[0,2496),[2560,2816)
development_maximum_anchor_read=2815
route_trigger=${route_trigger}
route_rule=trained_raw96_validation_strong_gate_only
route_values=canonical_certification,representation_ablation_screen
stage_2_mode=canonical_certification_only_when_authorized
certified_anchor_range=[2880,3264)
certified_trained_arms=post_mdn_context_384,raw_encoder_96
untrained_certified_access=false
final_holdout_access=false
policy_access=false
PLAN
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] ||
  fail "usage: $0 [--plan|--preflight|--run|--run-development|--verify-development|--run-certified|--verify]"
case "${mode}" in
--plan | --preflight | --run | --run-development | --verify-development | --run-certified | --verify) ;;
*)
  fail "usage: $0 [--plan|--preflight|--run|--run-development|--verify-development|--run-certified|--verify]"
  ;;
esac

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

verify_common_static_inputs() {
  local command_name
  for command_name in awk bash chmod cmp cp find g++ grep ldd ln mktemp mv \
    readelf realpath sha256sum stat; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  require_nonempty_file "${helper_source}"
  require_nonempty_file "${script_path}"
  require_nonempty_file "${closure_report}"
  require_nonempty_file "${closure_verifier}"
  require_nonempty_file "${preregistration}"
  require_nonempty_file "${protocol_amendment}"
  require_nonempty_file "${localization_addendum}"
  require_nonempty_file "${conditional_amendment}"
  require_nonempty_file "${capture_runner}"
  require_dir "${libtorch_path}/include"
  require_dir "${libtorch_path}/lib"
  require_dir "${cuda_path}/include"
  require_dir "${cuda_path}/lib64"
  [[ "$(sha256_of "${closure_report}")" == "${expected_closure_sha256}" ]] ||
    fail "fresh-seed closure hash drifted"
  [[ "$(sha256_of "${preregistration}")" == "${expected_preregistration_sha256}" ]] ||
    fail "diagnostic preregistration hash drifted"
  [[ "$(sha256_of "${protocol_amendment}")" == "${expected_protocol_amendment_sha256}" ]] ||
    fail "diagnostic protocol amendment hash drifted"
  [[ "$(sha256_of "${localization_addendum}")" == "${expected_localization_addendum_sha256}" ]] ||
    fail "representation localization addendum hash drifted"
  [[ "$(sha256_of "${conditional_amendment}")" == "${expected_conditional_amendment_sha256}" ]] ||
    fail "conditional-certification amendment hash drifted"
  [[ "$(sha256_of "${capture_runner}")" == "${expected_capture_runner_sha256}" ]] ||
    fail "staged capture runner hash drifted"
  bash -n "${script_path}"
  "${closure_verifier}" --verify >/dev/null
}

verify_frozen_sources() {
  require_dir "${capture_source_root}"
  require_nonempty_file "${frozen_helper_source}"
  require_nonempty_file "${frozen_runner_source}"
  cmp -s -- "${helper_source}" "${frozen_helper_source}" ||
    fail "live affine helper differs from capture-frozen helper"
  cmp -s -- "${script_path}" "${frozen_runner_source}" ||
    fail "live affine runner differs from capture-frozen runner"
}

validate_capture_development_contract() {
  require_nonempty_file "${capture_development}"
  expect_kv "${capture_development}" schema_id \
    synthetic_v2_frozen_feature_capture_v1.development.v1
  expect_kv "${capture_development}" status complete
  expect_kv "${capture_development}" maximum_anchor_read 2815
  expect_kv "${capture_development}" train_capture_range '[0,2496)'
  expect_kv "${capture_development}" validation_capture_range '[2560,2816)'
  expect_kv "${capture_development}" train_probe_rows "${train_rows}"
  expect_kv "${capture_development}" validation_probe_rows "${validation_rows}"
  expect_kv "${capture_development}" untrained_train_probe_rows "${train_rows}"
  expect_kv "${capture_development}" untrained_validation_probe_rows \
    "${validation_rows}"
  expect_kv "${capture_development}" certified_capture_created false
  expect_kv "${capture_development}" certified_attempt_created false
  expect_kv "${capture_development}" certified_result_created false
  expect_kv "${capture_development}" untrained_representation_certified_access false
  expect_kv "${capture_development}" final_holdout_scored false
  expect_kv "${capture_development}" policy_access false
  expect_kv "${capture_development}" conditional_amendment_sha256 \
    "${expected_conditional_amendment_sha256}"
  expect_kv "${capture_development}" runner_path "${capture_runner}"
  expect_kv "${capture_development}" runner_sha256 \
    "$(sha256_of "${capture_runner}")"
  expect_kv "${capture_development}" frozen_affine_helper_path \
    "${frozen_helper_source}"
  expect_kv "${capture_development}" frozen_affine_helper_sha256 \
    "$(sha256_of "${frozen_helper_source}")"
  expect_kv "${capture_development}" frozen_affine_runner_path \
    "${frozen_runner_source}"
  expect_kv "${capture_development}" frozen_affine_runner_sha256 \
    "$(sha256_of "${frozen_runner_source}")"
}

load_development_probes() {
  train_probe="$(bound_file "${capture_development}" train_probe_path train_probe_sha256)"
  validation_probe="$(bound_file "${capture_development}" validation_probe_path validation_probe_sha256)"
  encoder_train_probe="$(bound_file "${capture_development}" train_representation_probe_path train_representation_probe_sha256)"
  encoder_validation_probe="$(bound_file "${capture_development}" validation_representation_probe_path validation_representation_probe_sha256)"
  untrained_train_probe="$(bound_file "${capture_development}" untrained_train_representation_probe_path untrained_train_representation_probe_sha256)"
  untrained_validation_probe="$(bound_file "${capture_development}" untrained_validation_representation_probe_path untrained_validation_representation_probe_sha256)"
}

preflight_development() {
  verify_common_static_inputs
  "${capture_runner}" --verify-development >/dev/null
  verify_frozen_sources
  validate_capture_development_contract
  load_development_probes
}

compile_helper() {
  local runtime="$1"
  mkdir -p "${runtime}/bin"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -Werror -fPIC \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp" \
    -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${runtime}/bin/frozen_representation_affine_probe"
}

write_toolchain_provenance() {
  local runtime="$1"
  local compiler
  compiler="$(realpath -e -- "$(command -v g++)")"
  {
    echo "path=${compiler}"
    echo "sha256=$(sha256_of "${compiler}")"
    echo "target=$(g++ -dumpmachine)"
  } >"${runtime}/compiler.identity"
  ldd "${runtime}/bin/frozen_representation_affine_probe" \
    >"${runtime}/linked_libraries.txt"
  grep -Fq libtorch_cpu.so "${runtime}/linked_libraries.txt" ||
    fail "affine helper is not linked to libtorch_cpu"
}

validate_trained_report() {
  local report="$1"
  local expected_schema="$2"
  local expected_kind="$3"
  local expected_width="$4"
  local expected_excluded="$5"
  local expected_certified_rows="$6"
  local expected_maximum_anchor="$7"
  local expected_classification="$8"
  require_nonempty_file "${report}"
  expect_kv "${report}" schema_id "${expected_schema}"
  expect_kv "${report}" status complete
  expect_kv "${report}" probe_kind "${expected_kind}"
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows "${expected_certified_rows}"
  expect_kv "${report}" probe_ranges_disjoint true
  expect_kv "${report}" fit_anchor_range '[0,2496)'
  expect_kv "${report}" validation_anchor_range '[2560,2816)'
  expect_kv "${report}" maximum_anchor_read "${expected_maximum_anchor}"
  expect_kv "${report}" final_holdout_begin 3328
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" purge_anchors_used false
  expect_kv "${report}" refit_after_selection false
  expect_kv "${report}" affine_feature_width "${expected_width}"
  expect_kv "${report}" edge_identity_feature_width_excluded \
    "${expected_excluded}"
  expect_kv "${report}" classification "${expected_classification}"
  case "${expected_classification}" in
  development_selection_complete | strong_information_preservation | partial_information_preservation | representation_or_exposed_interface_failure) ;;
  *) fail "unexpected trained-report classification: ${expected_classification}" ;;
  esac
  local valid_candidates
  valid_candidates="$(kv numerically_valid_candidate_count "${report}")"
  [[ "${valid_candidates}" =~ ^[1-6]$ ]] ||
    fail "report has no valid declared ridge candidate: ${report}"
  if [[ "${expected_certified_rows}" == 0 ]]; then
    expect_kv "${report}" certified_anchor_range not_opened
    expect_kv "${report}" certified_candidates_scored 0
    expect_kv "${report}" certified_strong_gate_pass not_evaluated
    expect_kv "${report}" certified_partial_gate_pass not_evaluated
  else
    expect_kv "${report}" certified_anchor_range '[2880,3264)'
    expect_kv "${report}" certified_candidates_scored 1
  fi
}

validate_untrained_report() {
  local report="$1"
  require_nonempty_file "${report}"
  expect_kv "${report}" schema_id "${untrained_schema_id}"
  expect_kv "${report}" status complete
  expect_kv "${report}" probe_kind representation
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows 0
  expect_kv "${report}" certified_anchor_range not_opened
  expect_kv "${report}" maximum_anchor_read 2815
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" certified_candidates_scored 0
  expect_kv "${report}" affine_feature_width 96
  expect_kv "${report}" edge_identity_feature_width_excluded 0
  expect_kv "${report}" classification \
    untrained_representation_validation_control
  local valid_candidates
  valid_candidates="$(kv numerically_valid_candidate_count "${report}")"
  [[ "${valid_candidates}" =~ ^[1-6]$ ]] ||
    fail "untrained report has no valid declared ridge candidate"
}

run_development_reports() {
  local runtime="$1"
  local branch
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  for branch in main replay; do
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind mdn_context \
      --development-only \
      --train-input "${train_probe}" \
      --validation-input "${validation_probe}" \
      --output "${runtime}/${branch}/${mdn_development_schema_id}.report" \
      >"${runtime}/${branch}/${mdn_development_schema_id}.stdout.log" 2>&1
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation \
      --development-only \
      --train-input "${encoder_train_probe}" \
      --validation-input "${encoder_validation_probe}" \
      --output "${runtime}/${branch}/${encoder_development_schema_id}.report" \
      >"${runtime}/${branch}/${encoder_development_schema_id}.stdout.log" 2>&1
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation \
      --validation-only \
      --train-input "${untrained_train_probe}" \
      --validation-input "${untrained_validation_probe}" \
      --output "${runtime}/${branch}/${untrained_schema_id}.report" \
      >"${runtime}/${branch}/${untrained_schema_id}.stdout.log" 2>&1
  done
}

verify_development_report_pairs() {
  local runtime="$1"
  local report
  for report in \
    "${mdn_development_schema_id}.report" \
    "${encoder_development_schema_id}.report" \
    "${untrained_schema_id}.report" \
    "${mdn_development_schema_id}.stdout.log" \
    "${encoder_development_schema_id}.stdout.log" \
    "${untrained_schema_id}.stdout.log"; do
    cmp -s -- "${runtime}/main/${report}" "${runtime}/replay/${report}" ||
      fail "development main/replay differ: ${report}"
  done
  validate_trained_report \
    "${runtime}/main/${mdn_development_schema_id}.report" \
    "${mdn_development_schema_id}" mdn_context 384 16 0 2815 \
    development_selection_complete
  validate_trained_report \
    "${runtime}/main/${encoder_development_schema_id}.report" \
    "${encoder_development_schema_id}" representation 96 0 0 2815 \
    development_selection_complete
  validate_untrained_report \
    "${runtime}/main/${untrained_schema_id}.report"
}

write_development_status() {
  local runtime="$1"
  cat >"${runtime}/development.status" <<STATUS
schema_id=${development_schema_id}
status=complete
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
capture_runner_path=${capture_runner}
capture_runner_sha256=$(sha256_of "${capture_runner}")
affine_runner_path=${frozen_runner_source}
affine_runner_sha256=$(sha256_of "${frozen_runner_source}")
affine_helper_path=${frozen_helper_source}
affine_helper_sha256=$(sha256_of "${frozen_helper_source}")
conditional_certification_amendment_path=${conditional_amendment}
conditional_certification_amendment_sha256=$(sha256_of "${conditional_amendment}")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
train_representation_probe_path=${encoder_train_probe}
train_representation_probe_sha256=$(sha256_of "${encoder_train_probe}")
validation_representation_probe_path=${encoder_validation_probe}
validation_representation_probe_sha256=$(sha256_of "${encoder_validation_probe}")
untrained_train_representation_probe_path=${untrained_train_probe}
untrained_train_representation_probe_sha256=$(sha256_of "${untrained_train_probe}")
untrained_validation_representation_probe_path=${untrained_validation_probe}
untrained_validation_representation_probe_sha256=$(sha256_of "${untrained_validation_probe}")
mdn_context_validation_report_path=${runtime}/main/${mdn_development_schema_id}.report
mdn_context_validation_report_sha256=$(sha256_of "${runtime}/main/${mdn_development_schema_id}.report")
raw96_validation_report_path=${runtime}/main/${encoder_development_schema_id}.report
raw96_validation_report_sha256=$(sha256_of "${runtime}/main/${encoder_development_schema_id}.report")
untrained_raw96_validation_report_path=${runtime}/main/${untrained_schema_id}.report
untrained_raw96_validation_report_sha256=$(sha256_of "${runtime}/main/${untrained_schema_id}.report")
train_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
maximum_anchor_read=2815
certified_input_access=false
certified_candidates_scored=0
final_holdout_access=false
policy_access=false
STATUS
  chmod 0444 "${runtime}/development.status"
}

write_development_contract() {
  local runtime="$1"
  cat >"${runtime}/execution_contract.status" <<CONTRACT
schema_id=${development_schema_id}.execution_contract.v1
capture_development_sha256=$(sha256_of "${capture_development}")
closure_sha256=$(sha256_of "${closure_report}")
preregistration_sha256=$(sha256_of "${preregistration}")
protocol_amendment_sha256=$(sha256_of "${protocol_amendment}")
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_certification_amendment_sha256=$(sha256_of "${conditional_amendment}")
frozen_helper_sha256=$(sha256_of "${frozen_helper_source}")
frozen_runner_sha256=$(sha256_of "${frozen_runner_source}")
fit_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
maximum_anchor_read=2815
certified_input_access=false
final_holdout_access=false
policy_access=false
omp_threads=1
mkl_threads=1
CONTRACT
}

write_development_manifests() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "${train_probe}" "${validation_probe}" \
      "${encoder_train_probe}" "${encoder_validation_probe}" \
      "${untrained_train_probe}" "${untrained_validation_probe}" \
      "${capture_development}" "${closure_report}" \
      "${preregistration}" "${protocol_amendment}" \
      "${localization_addendum}" "${conditional_amendment}" \
      execution_contract.status >input_manifest.sha256
    sha256sum frozen_sources/frozen_representation_affine_probe.cpp \
      frozen_sources/run_frozen_representation_affine_probe.sh \
      >source_manifest.sha256
    sha256sum bin/frozen_representation_affine_probe >binary.sha256
    sha256sum \
      main/${mdn_development_schema_id}.report \
      main/${mdn_development_schema_id}.stdout.log \
      main/${encoder_development_schema_id}.report \
      main/${encoder_development_schema_id}.stdout.log \
      main/${untrained_schema_id}.report \
      main/${untrained_schema_id}.stdout.log \
      replay/${mdn_development_schema_id}.report \
      replay/${mdn_development_schema_id}.stdout.log \
      replay/${encoder_development_schema_id}.report \
      replay/${encoder_development_schema_id}.stdout.log \
      replay/${untrained_schema_id}.report \
      replay/${untrained_schema_id}.stdout.log \
      development.status >output_manifest.sha256
    sha256sum input_manifest.sha256 source_manifest.sha256 binary.sha256 \
      output_manifest.sha256 compiler.identity linked_libraries.txt \
      >master.sha256
  )
}

verify_manifests() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
    sha256sum -c binary.sha256 >/dev/null
    sha256sum -c output_manifest.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
}

seal_runtime_permissions() {
  local runtime="$1"
  find "${runtime}" -type f -exec chmod 0444 {} +
  chmod 0555 "${runtime}/bin/frozen_representation_affine_probe"
  find "${runtime}" -depth -type d -exec chmod 0555 {} +
}

verify_development_runtime_core() {
  require_dir "${development_runtime}"
  require_nonempty_file "${development_runtime}/development.status"
  require_nonempty_file "${development_runtime}/master.sha256"
  [[ "$(stat -c '%A' -- "${development_runtime}/bin/frozen_representation_affine_probe")" != *w* ]] ||
    fail "development affine binary is writable"
  [[ "$(stat -c '%A' -- "${development_runtime}/development.status")" != *w* ]] ||
    fail "development affine receipt is writable"
  verify_manifests "${development_runtime}"
  verify_development_report_pairs "${development_runtime}"
  cmp -s -- "${helper_source}" \
    "${development_runtime}/frozen_sources/frozen_representation_affine_probe.cpp" ||
    fail "live helper differs from development-frozen source"
  cmp -s -- "${script_path}" \
    "${development_runtime}/frozen_sources/run_frozen_representation_affine_probe.sh" ||
    fail "live runner differs from development-frozen source"
  cmp -s -- "${frozen_helper_source}" \
    "${development_runtime}/frozen_sources/frozen_representation_affine_probe.cpp" ||
    fail "capture-frozen helper differs from development source"
  cmp -s -- "${frozen_runner_source}" \
    "${development_runtime}/frozen_sources/run_frozen_representation_affine_probe.sh" ||
    fail "capture-frozen runner differs from development source"
  expect_kv "${development_runtime}/development.status" schema_id \
    "${development_schema_id}"
  expect_kv "${development_runtime}/development.status" status complete
  expect_kv "${development_runtime}/development.status" \
    capture_development_sha256 "$(sha256_of "${capture_development}")"
  expect_kv "${development_runtime}/development.status" maximum_anchor_read 2815
  expect_kv "${development_runtime}/development.status" certified_input_access false
  expect_kv "${development_runtime}/development.status" final_holdout_access false
}

numeric_gate_ge() {
  local value="$1"
  local threshold="$2"
  LC_ALL=C awk -v value="${value}" -v threshold="${threshold}" 'BEGIN {
    number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
    if (value !~ number) exit 42;
    print (value + 0 >= threshold + 0) ? "true" : "false";
  }'
}

numeric_gate_le() {
  local value="$1"
  local threshold="$2"
  LC_ALL=C awk -v value="${value}" -v threshold="${threshold}" 'BEGIN {
    number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
    if (value !~ number) exit 42;
    print (value + 0 <= threshold + 0) ? "true" : "false";
  }'
}

load_raw96_gate() {
  raw96_report="${development_runtime}/main/${encoder_development_schema_id}.report"
  require_nonempty_file "${raw96_report}"
  direction_gate="$(numeric_gate_ge \
    "$(kv selected.validation.directional_accuracy "${raw96_report}")" 0.95)"
  pairwise_rank_gate="$(numeric_gate_ge \
    "$(kv selected.validation.pairwise_rank_accuracy "${raw96_report}")" 0.95)"
  correlation_gate="$(numeric_gate_ge \
    "$(kv selected.validation.correlation "${raw96_report}")" 0.95)"
  rmse_ratio_gate="$(numeric_gate_le \
    "$(kv selected.validation.rmse_target_rms_ratio "${raw96_report}")" 0.25)"
  strong_gate="$(kv validation_strong_gate_pass "${raw96_report}")"
  [[ "${strong_gate}" == true || "${strong_gate}" == false ]] ||
    fail "raw96 report has invalid validation_strong_gate_pass"
  local conjunction=false
  if [[ "${direction_gate}" == true && "${pairwise_rank_gate}" == true &&
        "${correlation_gate}" == true && "${rmse_ratio_gate}" == true ]]; then
    conjunction=true
  fi
  [[ "${strong_gate}" == "${conjunction}" ]] ||
    fail "raw96 helper strong gate disagrees with mechanical component gates"
  if [[ "${strong_gate}" == true ]]; then
    route=canonical_certification
  else
    route=representation_ablation_screen
  fi
}

publish_route_trigger() {
  verify_development_runtime_core
  load_raw96_gate
  local candidate
  candidate="$(mktemp "${capture_runtime}/.affine_route_trigger.XXXXXX")"
  cat >"${candidate}" <<TRIGGER
schema_id=${trigger_schema_id}
status=complete
route=${route}
maximum_anchor_read=2815
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
capture_runner_path=${capture_runner}
capture_runner_sha256=$(sha256_of "${capture_runner}")
affine_runner_path=${frozen_runner_source}
affine_runner_sha256=$(sha256_of "${frozen_runner_source}")
affine_helper_path=${frozen_helper_source}
affine_helper_sha256=$(sha256_of "${frozen_helper_source}")
conditional_amendment_path=${conditional_amendment}
conditional_amendment_sha256=$(sha256_of "${conditional_amendment}")
affine_binary_path=${development_runtime}/bin/frozen_representation_affine_probe
affine_binary_sha256=$(sha256_of "${development_runtime}/bin/frozen_representation_affine_probe")
affine_development_receipt_path=${development_runtime}/development.status
affine_development_receipt_sha256=$(sha256_of "${development_runtime}/development.status")
post384_validation_report_path=${development_runtime}/main/${mdn_development_schema_id}.report
post384_validation_report_sha256=$(sha256_of "${development_runtime}/main/${mdn_development_schema_id}.report")
raw96_validation_report_path=${raw96_report}
raw96_validation_report_sha256=$(sha256_of "${raw96_report}")
untrained_raw96_validation_report_path=${development_runtime}/main/${untrained_schema_id}.report
untrained_raw96_validation_report_sha256=$(sha256_of "${development_runtime}/main/${untrained_schema_id}.report")
raw96_validation_direction_gate_pass=${direction_gate}
raw96_validation_pairwise_rank_gate_pass=${pairwise_rank_gate}
raw96_validation_correlation_gate_pass=${correlation_gate}
raw96_validation_rmse_ratio_gate_pass=${rmse_ratio_gate}
raw96_validation_strong_gate_pass=${strong_gate}
certified_capture_opened=false
final_holdout_access=false
policy_access=false
TRIGGER
  publish_immutable "${candidate}" "${route_trigger}"
}

verify_route_trigger() {
  require_nonempty_file "${route_trigger}"
  verify_development_runtime_core
  load_raw96_gate
  expect_kv "${route_trigger}" schema_id "${trigger_schema_id}"
  expect_kv "${route_trigger}" status complete
  expect_kv "${route_trigger}" route "${route}"
  expect_kv "${route_trigger}" maximum_anchor_read 2815
  expect_kv "${route_trigger}" capture_development_path \
    "${capture_development}"
  expect_kv "${route_trigger}" capture_development_sha256 \
    "$(sha256_of "${capture_development}")"
  expect_kv "${route_trigger}" capture_runner_path "${capture_runner}"
  expect_kv "${route_trigger}" capture_runner_sha256 \
    "$(sha256_of "${capture_runner}")"
  expect_kv "${route_trigger}" affine_runner_path "${frozen_runner_source}"
  expect_kv "${route_trigger}" affine_runner_sha256 \
    "$(sha256_of "${frozen_runner_source}")"
  expect_kv "${route_trigger}" affine_helper_path "${frozen_helper_source}"
  expect_kv "${route_trigger}" affine_helper_sha256 \
    "$(sha256_of "${frozen_helper_source}")"
  expect_kv "${route_trigger}" conditional_amendment_path \
    "${conditional_amendment}"
  expect_kv "${route_trigger}" conditional_amendment_sha256 \
    "${expected_conditional_amendment_sha256}"
  expect_kv "${route_trigger}" affine_binary_path \
    "${development_runtime}/bin/frozen_representation_affine_probe"
  expect_kv "${route_trigger}" affine_binary_sha256 \
    "$(sha256_of "${development_runtime}/bin/frozen_representation_affine_probe")"
  expect_kv "${route_trigger}" affine_development_receipt_path \
    "${development_runtime}/development.status"
  expect_kv "${route_trigger}" affine_development_receipt_sha256 \
    "$(sha256_of "${development_runtime}/development.status")"
  expect_kv "${route_trigger}" post384_validation_report_path \
    "${development_runtime}/main/${mdn_development_schema_id}.report"
  expect_kv "${route_trigger}" post384_validation_report_sha256 \
    "$(sha256_of "${development_runtime}/main/${mdn_development_schema_id}.report")"
  expect_kv "${route_trigger}" raw96_validation_report_path "${raw96_report}"
  expect_kv "${route_trigger}" raw96_validation_report_sha256 \
    "$(sha256_of "${raw96_report}")"
  expect_kv "${route_trigger}" untrained_raw96_validation_report_path \
    "${development_runtime}/main/${untrained_schema_id}.report"
  expect_kv "${route_trigger}" untrained_raw96_validation_report_sha256 \
    "$(sha256_of "${development_runtime}/main/${untrained_schema_id}.report")"
  expect_kv "${route_trigger}" raw96_validation_direction_gate_pass \
    "${direction_gate}"
  expect_kv "${route_trigger}" raw96_validation_pairwise_rank_gate_pass \
    "${pairwise_rank_gate}"
  expect_kv "${route_trigger}" raw96_validation_correlation_gate_pass \
    "${correlation_gate}"
  expect_kv "${route_trigger}" raw96_validation_rmse_ratio_gate_pass \
    "${rmse_ratio_gate}"
  expect_kv "${route_trigger}" raw96_validation_strong_gate_pass \
    "${strong_gate}"
  expect_kv "${route_trigger}" certified_capture_opened false
  expect_kv "${route_trigger}" final_holdout_access false
  expect_kv "${route_trigger}" policy_access false
}

run_development() {
  preflight_development
  if [[ ! -e "${development_runtime}" ]]; then
    local runtime lock
    lock="${development_runtime}.lock"
    runtime="$(mktemp -d "${runtime_parent}/synthetic_v2_frozen_affine_development_v1.candidate.XXXXXXXX")"
    mkdir "${lock}" || fail "another development affine run holds the lock"
    trap 'rmdir "'"${lock}"'" 2>/dev/null || true' EXIT
    mkdir -p "${runtime}/frozen_sources" "${runtime}/main" "${runtime}/replay"
    cp -- "${frozen_helper_source}" \
      "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp"
    cp -- "${frozen_runner_source}" \
      "${runtime}/frozen_sources/run_frozen_representation_affine_probe.sh"
    chmod 0444 "${runtime}/frozen_sources/"*
    compile_helper "${runtime}"
    write_toolchain_provenance "${runtime}"
    write_development_contract "${runtime}"
    run_development_reports "${runtime}"
    verify_development_report_pairs "${runtime}"
    write_development_status "${runtime}"
    write_development_manifests "${runtime}"
    verify_manifests "${runtime}"
    seal_runtime_permissions "${runtime}"
    mv -T -n "${runtime}" "${development_runtime}" ||
      fail "atomic development runtime installation failed; candidate retained at ${runtime}"
    rmdir "${lock}"
    trap - EXIT
  fi
  verify_development_runtime_core
  publish_route_trigger
  verify_route_trigger
  echo "development_status=${development_runtime}/development.status"
  echo "development_status_sha256=$(sha256_of "${development_runtime}/development.status")"
  echo "route_trigger=${route_trigger}"
  echo "route=$(kv route "${route_trigger}")"
}

validate_capture_result_contract() {
  require_nonempty_file "${capture_result}"
  expect_kv "${capture_result}" status complete
  expect_kv "${capture_result}" train_probe_rows "${train_rows}"
  expect_kv "${capture_result}" validation_probe_rows "${validation_rows}"
  expect_kv "${capture_result}" certified_probe_rows "${certified_rows}"
  expect_kv "${capture_result}" certified_scoring_attempt_count 1
  expect_kv "${capture_result}" maximum_anchor_scored 3263
  expect_kv "${capture_result}" final_holdout_scored false
  expect_kv "${capture_result}" policy_access false
}

load_certified_probes() {
  certified_probe="$(bound_file "${capture_result}" certified_probe_path certified_probe_sha256)"
  encoder_certified_probe="$(bound_file "${capture_result}" certified_representation_probe_path certified_representation_probe_sha256)"
}

preflight_certified() {
  verify_common_static_inputs
  verify_frozen_sources
  validate_capture_development_contract
  load_development_probes
  verify_route_trigger
  expect_kv "${route_trigger}" route canonical_certification
  expect_kv "${route_trigger}" raw96_validation_strong_gate_pass true
  "${capture_runner}" --verify >/dev/null
  validate_capture_result_contract
  load_certified_probes
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

assert_selection_unchanged() {
  local development_report="$1"
  local certified_report="$2"
  cmp -s <(selection_lines "${development_report}") \
    <(selection_lines "${certified_report}") ||
    fail "certified execution changed its pre-certified fit or validation selection"
}

run_certified_reports() {
  local runtime="$1"
  local branch
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  for branch in main replay; do
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind mdn_context \
      --train-input "${train_probe}" \
      --validation-input "${validation_probe}" \
      --certified-input "${certified_probe}" \
      --output "${runtime}/${branch}/${schema_id}.report" \
      >"${runtime}/${branch}/${schema_id}.stdout.log" 2>&1
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation \
      --train-input "${encoder_train_probe}" \
      --validation-input "${encoder_validation_probe}" \
      --certified-input "${encoder_certified_probe}" \
      --output "${runtime}/${branch}/${encoder_schema_id}.report" \
      >"${runtime}/${branch}/${encoder_schema_id}.stdout.log" 2>&1
    cp -- "${development_runtime}/${branch}/${untrained_schema_id}.report" \
      "${runtime}/${branch}/${untrained_schema_id}.report"
    cp -- "${development_runtime}/${branch}/${untrained_schema_id}.stdout.log" \
      "${runtime}/${branch}/${untrained_schema_id}.stdout.log"
  done
}

verify_certified_report_pairs() {
  local runtime="$1"
  local report
  for report in \
    "${schema_id}.report" "${schema_id}.stdout.log" \
    "${encoder_schema_id}.report" "${encoder_schema_id}.stdout.log" \
    "${untrained_schema_id}.report" "${untrained_schema_id}.stdout.log"; do
    cmp -s -- "${runtime}/main/${report}" "${runtime}/replay/${report}" ||
      fail "certified main/replay differ: ${report}"
  done
  validate_trained_report "${runtime}/main/${schema_id}.report" \
    "${schema_id}" mdn_context 384 16 "${certified_rows}" 3263 \
    "$(kv classification "${runtime}/main/${schema_id}.report")"
  validate_trained_report "${runtime}/main/${encoder_schema_id}.report" \
    "${encoder_schema_id}" representation 96 0 "${certified_rows}" 3263 \
    "$(kv classification "${runtime}/main/${encoder_schema_id}.report")"
  validate_untrained_report "${runtime}/main/${untrained_schema_id}.report"
  assert_selection_unchanged \
    "${development_runtime}/main/${mdn_development_schema_id}.report" \
    "${runtime}/main/${schema_id}.report"
  assert_selection_unchanged \
    "${development_runtime}/main/${encoder_development_schema_id}.report" \
    "${runtime}/main/${encoder_schema_id}.report"
}

write_certified_contract() {
  local runtime="$1"
  cat >"${runtime}/execution_contract.status" <<CONTRACT
schema_id=${schema_id}.execution_contract.v2
capture_development_sha256=$(sha256_of "${capture_development}")
development_affine_status_sha256=$(sha256_of "${development_runtime}/development.status")
route_trigger_sha256=$(sha256_of "${route_trigger}")
capture_result_sha256=$(sha256_of "${capture_result}")
conditional_certification_amendment_sha256=$(sha256_of "${conditional_amendment}")
precertified_helper_sha256=$(sha256_of "${frozen_helper_source}")
precertified_runner_sha256=$(sha256_of "${frozen_runner_source}")
fit_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
certified_anchor_range=[2880,3264)
certified_trained_arms=2
certified_candidates_scored_per_arm=1
untrained_certified_candidates_scored=0
development_selection_reused=true
refit_after_development_selection=false
final_holdout_access=false
policy_access=false
omp_threads=1
mkl_threads=1
CONTRACT
}

write_final_status() {
  local runtime="$1"
  cat >"${runtime}/result.status" <<STATUS
schema_id=${schema_id}.result.v2
status=complete
route=canonical_certification
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
development_affine_status_path=${development_runtime}/development.status
development_affine_status_sha256=$(sha256_of "${development_runtime}/development.status")
route_trigger_path=${route_trigger}
route_trigger_sha256=$(sha256_of "${route_trigger}")
capture_result_path=${capture_result}
capture_result_sha256=$(sha256_of "${capture_result}")
mdn_context_report_path=${runtime}/main/${schema_id}.report
mdn_context_report_sha256=$(sha256_of "${runtime}/main/${schema_id}.report")
raw96_report_path=${runtime}/main/${encoder_schema_id}.report
raw96_report_sha256=$(sha256_of "${runtime}/main/${encoder_schema_id}.report")
untrained_raw96_report_path=${runtime}/main/${untrained_schema_id}.report
untrained_raw96_report_sha256=$(sha256_of "${runtime}/main/${untrained_schema_id}.report")
development_selection_reused=true
maximum_anchor_read=3263
final_holdout_access=false
policy_access=false
STATUS
  chmod 0444 "${runtime}/result.status"
}

write_certified_manifests() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "${train_probe}" "${validation_probe}" "${certified_probe}" \
      "${encoder_train_probe}" "${encoder_validation_probe}" \
      "${encoder_certified_probe}" \
      "${untrained_train_probe}" "${untrained_validation_probe}" \
      "${capture_development}" "${development_runtime}/development.status" \
      "${route_trigger}" "${capture_result}" "${closure_report}" \
      "${preregistration}" "${protocol_amendment}" \
      "${localization_addendum}" "${conditional_amendment}" \
      execution_contract.status >input_manifest.sha256
    sha256sum frozen_sources/frozen_representation_affine_probe.cpp \
      frozen_sources/run_frozen_representation_affine_probe.sh \
      >source_manifest.sha256
    sha256sum bin/frozen_representation_affine_probe >binary.sha256
    sha256sum \
      main/${schema_id}.report main/${schema_id}.stdout.log \
      main/${encoder_schema_id}.report main/${encoder_schema_id}.stdout.log \
      main/${untrained_schema_id}.report main/${untrained_schema_id}.stdout.log \
      replay/${schema_id}.report replay/${schema_id}.stdout.log \
      replay/${encoder_schema_id}.report replay/${encoder_schema_id}.stdout.log \
      replay/${untrained_schema_id}.report replay/${untrained_schema_id}.stdout.log \
      result.status >output_manifest.sha256
    sha256sum input_manifest.sha256 source_manifest.sha256 binary.sha256 \
      output_manifest.sha256 compiler.identity linked_libraries.txt \
      >master.sha256
  )
}

publish_artifact() {
  local source="$1"
  local destination="$2"
  if [[ -e "${destination}" ]]; then
    require_nonempty_file "${destination}"
    cmp -s -- "${source}" "${destination}" ||
      fail "durable artifact already contains different content: ${destination}"
  else
    cp -- "${source}" "${destination}"
    chmod 0444 "${destination}"
  fi
}

verify_final_runtime() {
  require_dir "${final_runtime}"
  require_nonempty_file "${final_runtime}/result.status"
  require_nonempty_file "${final_runtime}/master.sha256"
  [[ "$(stat -c '%A' -- "${final_runtime}/bin/frozen_representation_affine_probe")" != *w* ]] ||
    fail "certified affine binary is writable"
  verify_manifests "${final_runtime}"
  verify_certified_report_pairs "${final_runtime}"
  cmp -s -- "${helper_source}" \
    "${final_runtime}/frozen_sources/frozen_representation_affine_probe.cpp" ||
    fail "live helper differs from certified-frozen source"
  cmp -s -- "${script_path}" \
    "${final_runtime}/frozen_sources/run_frozen_representation_affine_probe.sh" ||
    fail "live runner differs from certified-frozen source"
  expect_kv "${final_runtime}/result.status" status complete
  expect_kv "${final_runtime}/result.status" route canonical_certification
  expect_kv "${final_runtime}/result.status" development_selection_reused true
  expect_kv "${final_runtime}/result.status" maximum_anchor_read 3263
  expect_kv "${final_runtime}/result.status" final_holdout_access false
  require_nonempty_file "${artifact_report}"
  cmp -s -- "${artifact_report}" "${final_runtime}/main/${schema_id}.report" ||
    fail "durable MDN-context artifact differs from sealed report"
  require_nonempty_file "${encoder_artifact_report}"
  cmp -s -- "${encoder_artifact_report}" \
    "${final_runtime}/main/${encoder_schema_id}.report" ||
    fail "durable encoder artifact differs from sealed report"
  require_nonempty_file "${untrained_artifact_report}"
  cmp -s -- "${untrained_artifact_report}" \
    "${final_runtime}/main/${untrained_schema_id}.report" ||
    fail "durable untrained artifact differs from sealed report"
}

run_certified() {
  preflight_certified
  if [[ ! -e "${final_runtime}" ]]; then
    local runtime lock
    lock="${final_runtime}.lock"
    runtime="$(mktemp -d "${runtime_parent}/${schema_id}.candidate.XXXXXXXX")"
    mkdir "${lock}" || fail "another certified affine run holds the lock"
    trap 'rmdir "'"${lock}"'" 2>/dev/null || true' EXIT
    mkdir -p "${runtime}/frozen_sources" "${runtime}/main" "${runtime}/replay"
    cp -- "${frozen_helper_source}" \
      "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp"
    cp -- "${frozen_runner_source}" \
      "${runtime}/frozen_sources/run_frozen_representation_affine_probe.sh"
    chmod 0444 "${runtime}/frozen_sources/"*
    compile_helper "${runtime}"
    write_toolchain_provenance "${runtime}"
    write_certified_contract "${runtime}"
    run_certified_reports "${runtime}"
    verify_certified_report_pairs "${runtime}"
    write_final_status "${runtime}"
    write_certified_manifests "${runtime}"
    verify_manifests "${runtime}"
    seal_runtime_permissions "${runtime}"
    mv -T -n "${runtime}" "${final_runtime}" ||
      fail "atomic certified runtime installation failed; candidate retained at ${runtime}"
    rmdir "${lock}"
    trap - EXIT
  fi
  publish_artifact "${final_runtime}/main/${schema_id}.report" \
    "${artifact_report}"
  publish_artifact "${final_runtime}/main/${encoder_schema_id}.report" \
    "${encoder_artifact_report}"
  publish_artifact "${final_runtime}/main/${untrained_schema_id}.report" \
    "${untrained_artifact_report}"
  verify_final_runtime
  echo "report_path=${artifact_report}"
  echo "report_sha256=$(sha256_of "${artifact_report}")"
  echo "encoder_report_path=${encoder_artifact_report}"
  echo "encoder_report_sha256=$(sha256_of "${encoder_artifact_report}")"
  echo "untrained_report_path=${untrained_artifact_report}"
  echo "untrained_report_sha256=$(sha256_of "${untrained_artifact_report}")"
}

case "${mode}" in
--preflight)
  preflight_development
  echo "preflight=development_inputs_valid"
  ;;
--run | --run-development)
  run_development
  ;;
--verify-development)
  preflight_development
  verify_development_runtime_core
  verify_route_trigger
  echo "development_status=${development_runtime}/development.status"
  echo "route=$(kv route "${route_trigger}")"
  ;;
--run-certified)
  run_certified
  ;;
--verify)
  preflight_certified
  verify_final_runtime
  echo "report_path=${artifact_report}"
  echo "report_sha256=$(sha256_of "${artifact_report}")"
  ;;
*)
  fail "unreachable mode"
  ;;
esac
