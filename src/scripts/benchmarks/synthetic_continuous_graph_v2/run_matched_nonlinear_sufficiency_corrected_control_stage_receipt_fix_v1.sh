#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C
export LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1"
readonly EVALUATOR_REPORT_SCHEMA="synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1"
readonly RUNTIME_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_v1.sh"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_STAGE_RECEIPT_FIX_PREREGISTRATION.md"
readonly PREREG_SHA="cd1460b3e1af0d9f2e719c8638b41f2c5ed8a4d6b65c82b34f6a1343ff645785"
readonly STAGE_RECEIPT_ROOT_CAUSE="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_STAGE_RECEIPT_FIX_ROOT_CAUSE.md"
readonly STAGE_RECEIPT_ROOT_CAUSE_SHA="c3e3bf69a1b2f25d2273c5abc25334988dd52de888dcaedced3deff4057836e2"
readonly STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID="synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1"
readonly STAGE_RECEIPT_PREDECESSOR_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_PREREGISTRATION.md"
readonly STAGE_RECEIPT_PREDECESSOR_PREREG_SHA="d22232ea9f3e71dce58c1c5a11beca32c028f6eb5961b5c2e62b7b949c2951f9"
readonly STAGE_RECEIPT_PREDECESSOR_RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_corrected_control_self_test_report_v1.sh"
readonly STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA="9e6328a7385972abe14aea57def6307a6a0a982632fc3097d6bcc94df8677458"
readonly STAGE_RECEIPT_PREDECESSOR_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID}"
readonly STAGE_RECEIPT_PREDECESSOR_TERMINAL="${STAGE_RECEIPT_PREDECESSOR_ROOT}/terminal.invalid.status"
readonly STAGE_RECEIPT_PREDECESSOR_TERMINAL_SHA="71cc4730d172541d50fab4ae9d46bc0b55138070994fc873b89acdecb8d01ccf"
readonly STAGE_RECEIPT_PREDECESSOR_ATTEMPT="${STAGE_RECEIPT_PREDECESSOR_ROOT}/attempt.status"
readonly STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA="b59308962ab31837e2002f7122db8f2a5d9309e784caa1b2f119656cd5ab97df"
readonly STAGE_RECEIPT_PREDECESSOR_WORKER_LOG="${STAGE_RECEIPT_PREDECESSOR_ROOT}/logs/worker.log"
readonly STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA="9ec0ea0d3728056a8dde8049b77819a372b1514d7b67320ff359886054ab6823"
readonly CORRECTED_CONTROL_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_PREREGISTRATION.md"
readonly CORRECTED_CONTROL_PREREG_SHA="54356d24de2cb11fedba9d7101b9ba97701d3c428b9d08f69ef9f208804a5719"
readonly PREDECESSOR_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_PREREGISTRATION.md"
readonly PREDECESSOR_PREREG_SHA="cbbf1d837aa741ed157beb2fbab5b01d6c6e004376e865b1f71f2732b46fa348"
readonly SELF_TEST_ROOT_CAUSE="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_ROOT_CAUSE_ADDENDUM.md"
readonly SELF_TEST_ROOT_CAUSE_SHA="ad9abce221bf4d93164ded532f20d668a7b5a4cf483c7b5a4759cf65a0d83ece"
readonly ROOT_CAUSE="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_V1_TERMINAL_ROOT_CAUSE.md"
readonly ROOT_CAUSE_SHA="7b3d8fb446c5585e24a0ecfdd9fee250f6781817c74d8d074a82411710dd6cf3"
readonly RAW_CONTROL_PREDECESSOR_TERMINAL="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_development_v1/terminal.invalid.status"
readonly RAW_CONTROL_PREDECESSOR_TERMINAL_SHA="a83154f71bee7eda29d3de8e221e55c03b4a42742cdb485a01ebb00ac1f41237"
readonly SELF_TEST_PREDECESSOR_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1"
readonly SELF_TEST_PREDECESSOR_TERMINAL="${SELF_TEST_PREDECESSOR_ROOT}/terminal.invalid.status"
readonly SELF_TEST_PREDECESSOR_TERMINAL_SHA="ad63fa5dfbb4da59a8efaa32c5577dc2436ad2e795864b51158ef16a4890f0ca"
readonly PREDECESSOR_SELF_TEST_OUTPUT="${SELF_TEST_PREDECESSOR_ROOT}/self_test/self_test.output"
readonly PREDECESSOR_SELF_TEST_OUTPUT_SHA="efed4a76d7ca7d46c4d3ce8ae5e3cb4be79fabf64f1f08707f28b1e9543c69e5"
readonly PREDECESSOR_SELF_TEST_LOG="${SELF_TEST_PREDECESSOR_ROOT}/self_test/self_test.log"
readonly PREDECESSOR_SELF_TEST_LOG_SHA="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

readonly CAPTURE_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.cpp"
readonly CAPTURE_SOURCE_SHA="b489548b7a8fec72c7933f359b694e5852282e721108453b6e338e3ec73b2c62"
readonly CAPTURE_PREDECESSOR_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture_corrected_control.cpp"
readonly CAPTURE_PREDECESSOR_SOURCE_SHA="f1483a0858c342b2477cc37e043bf5a894da369bd1e3ccb51ce04601710de2a8"
readonly CAPTURE_BUILD_SCRIPT="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.sh"
readonly CAPTURE_BUILD_SCRIPT_SHA="5c168802d7618c9c144b41480e8b406a2c2b883c314ab7d3a08ee33cbaadd2b8"
readonly CAPTURE_PREDECESSOR_BUILD_SCRIPT="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_raw_nodelift_edge_feature_probe_capture_corrected_control.sh"
readonly CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA="f994f98c5825e1d5d9c267dfe5b8ab59ac479e7fc1707af56c81efb7c06fb3d2"
readonly COMMON_ARCHIVE="${ROOT}/.build/lib/libcommon.a"
readonly COMMON_ARCHIVE_SHA="853ade11707a8588194eda199e5a742e7363c2d1fd87f43285f3ad89414e06d3"
readonly TORCH_ARCHIVE="${ROOT}/.build/lib/libtorchwrap.a"
readonly TORCH_ARCHIVE_SHA="d9a128191f227a798219c9ee0c2ed8d4c6976916dba8b43eb66c6f25c21b279d"
readonly AFFINE_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp"
readonly AFFINE_SOURCE_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly NONLINEAR_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/matched_nonlinear_sufficiency_corrected_control_probe.cpp"
readonly NONLINEAR_SOURCE_SHA="caddf0a96d13e9c425671a7067e48720f483de5ab40b933b5caf12b76ba99ef5"
readonly NONLINEAR_BUILD_SCRIPT="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_matched_nonlinear_sufficiency_corrected_control_probe.sh"
readonly NONLINEAR_BUILD_SCRIPT_SHA="73dbce2a5c7566f2dc24884bb0cc579ebdf66204d043bc434098b0ac4fb27816"

readonly PHASE2A_RECEIPT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1/development.status"
readonly PHASE2A_RECEIPT_SHA="b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5"
readonly CAPTURE_CONFIG="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/synthetic_benchmark.frozen_feature_capture.isolated.config"
readonly CAPTURE_CONFIG_SHA="eeea5620f1b271c0bd4527db6764c8f7b66eef5aced7b72d9d1b28d89443c9b3"
readonly SOURCE_CLOSURE="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/development_source_closure.status"
readonly SOURCE_CLOSURE_SHA="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly CURSOR_ERRATUM="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/cursor_alignment_erratum.status"
readonly CURSOR_ERRATUM_SHA="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly SOURCE_MANIFEST="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/source_manifest.status"
readonly SOURCE_MANIFEST_SHA="7cf41d721647579924620c9daf7e38931898ba28a02c71c38cc7cd6e3f6431fa"
readonly SOURCE_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/source"
readonly REPRESENTATION_TRAIN="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe"
readonly REPRESENTATION_TRAIN_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly REPRESENTATION_VALIDATION="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe"
readonly REPRESENTATION_VALIDATION_SHA="8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd"

readonly BUILD_DIR="${RUNTIME_ROOT}/build"
readonly SCRATCH="${RUNTIME_ROOT}/.scratch"
readonly CAPTURE_BIN="${BUILD_DIR}/raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report"
readonly NONLINEAR_BIN="${BUILD_DIR}/matched_nonlinear_sufficiency_corrected_control_probe"
readonly CAPTURE_BUILD_RECEIPT="${BUILD_DIR}/capture.build.status"
readonly NONLINEAR_BUILD_RECEIPT="${BUILD_DIR}/nonlinear.build.status"
readonly SELF_TEST_DIR="${RUNTIME_ROOT}/self_test"
readonly SELF_TEST_REPORT="${SELF_TEST_DIR}/self_test.report"
readonly SELF_TEST_STDOUT="${SELF_TEST_DIR}/self_test.stdout"
readonly SELF_TEST_STDOUT_NORMALIZED="${SELF_TEST_DIR}/self_test.stdout.normalized"
readonly SELF_TEST_STDERR="${SELF_TEST_DIR}/self_test.stderr"
readonly SELF_TEST_RECEIPT="${SELF_TEST_DIR}/self_test.status"
readonly RAW_TRAIN="${RUNTIME_ROOT}/capture/train.probe"
readonly RAW_VALIDATION="${RUNTIME_ROOT}/capture/validation.probe"
readonly RAW_TRAIN_REPORT="${RUNTIME_ROOT}/capture/train.report"
readonly RAW_VALIDATION_REPORT="${RUNTIME_ROOT}/capture/validation.report"
readonly NONLINEAR_REPORT="${RUNTIME_ROOT}/nonlinear/development.report"
readonly STAGES="${RUNTIME_ROOT}/stages"
readonly LOGS="${RUNTIME_ROOT}/logs"
readonly WORKER_LOG="${LOGS}/worker.log"
readonly ATTEMPT="${RUNTIME_ROOT}/attempt.status"
readonly MANIFEST="${RUNTIME_ROOT}/artifact.manifest.status"
readonly RESULT="${RUNTIME_ROOT}/development.status"
readonly REJECTED_RESULT="${RUNTIME_ROOT}/rejected.development.status"
readonly TERMINAL="${RUNTIME_ROOT}/terminal.invalid.status"
readonly LOCK="${RUNTIME_ROOT}/.execution.lock"
readonly SOFT_TIMEOUT_SECONDS=5350
readonly TERM_GRACE_SECONDS=10
readonly HARD_TIMEOUT_SECONDS=5400
readonly RESULT_VERIFY_TIMEOUT_SECONDS=30
readonly RESULT_VERIFY_GRACE_SECONDS=5
RUN_DEVELOPMENT_TIMEOUT_PID=""
RUN_DEVELOPMENT_CAPABILITY=""
RUN_DEVELOPMENT_PENDING_SIGNAL=""
RUN_DEVELOPMENT_LAUNCHING=0

readonly PROBE_HEADER="record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,base_node_id,quote_node_id,channel_index,target_edge_close_return,feature_count,feature_values"
readonly REPRESENTATION_SCHEMA="kikijyeba.synthetic.representation_edge_feature_probe.v1"
readonly RAW_SCHEMA="kikijyeba.synthetic.raw_nodelift_edge_feature_probe.corrected_control.v1"
readonly CAPTURE_REPORT_SCHEMA="synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_capture_v1"
readonly SELF_TEST_SCHEMA="synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_self_test_v1"



fail() { echo "[clear-signal:phase2b-corrected-stage-receipt-fix] ERROR: $*" >&2; exit 1; }
sha256() { sha256sum -- "$1" | awk '{print $1}'; }

kv() {
  local file="$1" key="$2"
  awk -F= -v key="$key" '$1 == key {n++; v=substr($0,length(key)+2)} END {if(n!=1) exit 2; print v}' "$file" ||
    fail "expected exactly one ${key} in ${file}"
}

expect_kv() {
  local file="$1" key="$2" expected="$3" actual
  actual="$(kv "$file" "$key")"
  [[ "$actual" == "$expected" ]] || fail "${file}: ${key} expected '${expected}', got '${actual}'"
}

expect_numeric_close() {
  local file="$1" key="$2" expected="$3" tolerance="$4" actual
  actual="$(kv "$file" "$key")"
  finite_number "$actual"
  awk -v actual="$actual" -v expected="$expected" -v tolerance="$tolerance" '
    BEGIN {
      delta = actual - expected
      if (delta < 0) delta = -delta
      exit !(delta <= tolerance)
    }
  ' || fail "${file}: ${key} expected numerically ${expected} +/- ${tolerance}, got '${actual}'"
}

require_canonical_path() {
  local path="$1" current="/" part
  [[ "$path" == /* && -e "$path" ]] || fail "missing absolute path: ${path}"
  while IFS= read -r part; do
    [[ -n "$part" ]] || continue
    current="${current%/}/${part}"
    [[ ! -L "$current" ]] || fail "symlink component is forbidden: ${current}"
  done < <(printf '%s\n' "${path#/}" | tr '/' '\n')
  [[ "$(realpath -e -- "$path")" == "$path" ]] || fail "noncanonical path: ${path}"
}

require_frozen_metadata() {
  local path="$1" mode uid links
  require_canonical_path "$path"
  [[ -f "$path" && ! -L "$path" ]] || fail "not a regular file: ${path}"
  mode="$(stat -c '%a' -- "$path")"; uid="$(stat -c '%u' -- "$path")"; links="$(stat -c '%h' -- "$path")"
  (( (8#$mode & 8#222) == 0 )) || fail "authority is writable: ${path}"
  [[ "$uid" == 0 && "$links" == 1 ]] || fail "authority metadata mismatch: ${path}"
}

require_frozen_file() {
  local path="$1" expected="$2"
  require_frozen_metadata "$path"
  [[ "$(sha256 "$path")" == "$expected" ]] || fail "SHA-256 mismatch: ${path}"
}

reject_forbidden_path() {
  case "$1" in
    */data/raw|*/data/raw/*|*certified*|*final_holdout*|*/data/final/*|*policy_checkpoint*)
      fail "forbidden corrected-control path: $1" ;;
  esac
}

verify_predecessor_lineage() {
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" status terminal_invalid
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" classification invalid_pre_fit_raw_control_capture_contract_failure
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" fits_completed 0
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" optimizer_steps 0
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" scientific_result_available false
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" same_protocol_retry_allowed false
  expect_kv "$RAW_CONTROL_PREDECESSOR_TERMINAL" new_protocol_required true

  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" status terminal_invalid
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" classification invalid_data_free_self_test_failure
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" failure_stage self_test_validation
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" attempt_consumed false
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" fits_completed 0
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" optimizer_steps 0
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" scientific_result_available false
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" same_protocol_resume_allowed false
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" same_protocol_retry_allowed false
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" new_protocol_required true
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" capture_source_sha256 "$CAPTURE_PREDECESSOR_SOURCE_SHA"
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" capture_binary_sha256 e2e244eb7139f145f37763804e9c52c9d1850f125e78701c527618ed7ee1c042
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" self_test_output_sha256 "$PREDECESSOR_SELF_TEST_OUTPUT_SHA"
  expect_kv "$SELF_TEST_PREDECESSOR_TERMINAL" self_test_log_sha256 "$PREDECESSOR_SELF_TEST_LOG_SHA"
}

emit_stage_receipt_fix_lineage() {
  echo "stage_receipt_root_cause_path=${STAGE_RECEIPT_ROOT_CAUSE}"
  echo "stage_receipt_root_cause_sha256=${STAGE_RECEIPT_ROOT_CAUSE_SHA}"
  echo "immediate_predecessor_protocol_id=${STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID}"
  echo "immediate_predecessor_preregistration_path=${STAGE_RECEIPT_PREDECESSOR_PREREG}"
  echo "immediate_predecessor_preregistration_sha256=${STAGE_RECEIPT_PREDECESSOR_PREREG_SHA}"
  echo "immediate_predecessor_runner_path=${STAGE_RECEIPT_PREDECESSOR_RUNNER}"
  echo "immediate_predecessor_runner_sha256=${STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA}"
  echo "immediate_predecessor_terminal_path=${STAGE_RECEIPT_PREDECESSOR_TERMINAL}"
  echo "immediate_predecessor_terminal_sha256=${STAGE_RECEIPT_PREDECESSOR_TERMINAL_SHA}"
  echo "immediate_predecessor_attempt_path=${STAGE_RECEIPT_PREDECESSOR_ATTEMPT}"
  echo "immediate_predecessor_attempt_sha256=${STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA}"
  echo "immediate_predecessor_worker_log_path=${STAGE_RECEIPT_PREDECESSOR_WORKER_LOG}"
  echo "immediate_predecessor_worker_log_sha256=${STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA}"
}

verify_stage_receipt_fix_lineage_receipt() {
  local receipt="$1"
  expect_kv "$receipt" stage_receipt_root_cause_path "$STAGE_RECEIPT_ROOT_CAUSE"
  expect_kv "$receipt" stage_receipt_root_cause_sha256 "$STAGE_RECEIPT_ROOT_CAUSE_SHA"
  expect_kv "$receipt" immediate_predecessor_protocol_id "$STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID"
  expect_kv "$receipt" immediate_predecessor_preregistration_path "$STAGE_RECEIPT_PREDECESSOR_PREREG"
  expect_kv "$receipt" immediate_predecessor_preregistration_sha256 "$STAGE_RECEIPT_PREDECESSOR_PREREG_SHA"
  expect_kv "$receipt" immediate_predecessor_runner_path "$STAGE_RECEIPT_PREDECESSOR_RUNNER"
  expect_kv "$receipt" immediate_predecessor_runner_sha256 "$STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA"
  expect_kv "$receipt" immediate_predecessor_terminal_path "$STAGE_RECEIPT_PREDECESSOR_TERMINAL"
  expect_kv "$receipt" immediate_predecessor_terminal_sha256 "$STAGE_RECEIPT_PREDECESSOR_TERMINAL_SHA"
  expect_kv "$receipt" immediate_predecessor_attempt_path "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT"
  expect_kv "$receipt" immediate_predecessor_attempt_sha256 "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA"
  expect_kv "$receipt" immediate_predecessor_worker_log_path "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG"
  expect_kv "$receipt" immediate_predecessor_worker_log_sha256 "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA"
}

verify_stage_receipt_fix_predecessor_lineage() {
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_terminal_invalid_v1
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" status terminal_invalid
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" protocol_id "$STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" classification invalid_post_attempt_execution
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" attempt_path "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" attempt_sha256 "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" failure_stage post_attempt_setup
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" failure_command_ordinal 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" worker_log_sha256 "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" attempt_consumed true
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" same_protocol_resume_allowed false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" same_protocol_retry_allowed false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" new_protocol_required true
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" actual_raw_capture_invocations_started 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" actual_raw_capture_invocations_completed 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" actual_raw_capture_invocations_validated 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" evaluator_invocations_started 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" evaluator_invocations_completed 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" evaluator_invocations_validated 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" evaluator_report_attested 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" fits_completed 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" optimizer_steps 0
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" maximum_anchor_read_upper_bound none
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" scientific_result_available false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" representation_execution false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" mdn_execution false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" checkpoint_written false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" certified_input_access false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" final_holdout_access false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" policy_access false
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" status consumed
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" protocol_id "$STAGE_RECEIPT_PREDECESSOR_PROTOCOL_ID"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" attempt_ordinal 1
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" preregistration_sha256 "$STAGE_RECEIPT_PREDECESSOR_PREREG_SHA"
  expect_kv "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" runner_sha256 "$STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA"
}

preflight_build_authority() {
  require_frozen_metadata "$RUNNER"
  require_frozen_file "$PREREG" "$PREREG_SHA"
  require_frozen_file "$STAGE_RECEIPT_ROOT_CAUSE" "$STAGE_RECEIPT_ROOT_CAUSE_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_PREREG" "$STAGE_RECEIPT_PREDECESSOR_PREREG_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_RUNNER" "$STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" "$STAGE_RECEIPT_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG" "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA"
  require_frozen_file "$CORRECTED_CONTROL_PREREG" "$CORRECTED_CONTROL_PREREG_SHA"
  require_frozen_file "$PREDECESSOR_PREREG" "$PREDECESSOR_PREREG_SHA"
  require_frozen_file "$SELF_TEST_ROOT_CAUSE" "$SELF_TEST_ROOT_CAUSE_SHA"
  require_frozen_file "$ROOT_CAUSE" "$ROOT_CAUSE_SHA"
  require_frozen_file "$RAW_CONTROL_PREDECESSOR_TERMINAL" "$RAW_CONTROL_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$SELF_TEST_PREDECESSOR_TERMINAL" "$SELF_TEST_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$PREDECESSOR_SELF_TEST_OUTPUT" "$PREDECESSOR_SELF_TEST_OUTPUT_SHA"
  require_frozen_file "$PREDECESSOR_SELF_TEST_LOG" "$PREDECESSOR_SELF_TEST_LOG_SHA"
  require_frozen_file "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA"
  require_frozen_file "$CAPTURE_PREDECESSOR_SOURCE" "$CAPTURE_PREDECESSOR_SOURCE_SHA"
  require_frozen_file "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA"
  require_frozen_file "$CAPTURE_PREDECESSOR_BUILD_SCRIPT" "$CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA"
  require_frozen_file "$COMMON_ARCHIVE" "$COMMON_ARCHIVE_SHA"
  require_frozen_file "$TORCH_ARCHIVE" "$TORCH_ARCHIVE_SHA"
  require_frozen_file "$AFFINE_SOURCE" "$AFFINE_SOURCE_SHA"
  require_frozen_file "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA"
  require_frozen_file "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA"
  verify_predecessor_lineage
  verify_stage_receipt_fix_predecessor_lineage
}

preflight_capture_self_test_authority() {
  require_frozen_metadata "$RUNNER"
  require_frozen_file "$PREREG" "$PREREG_SHA"
  require_frozen_file "$STAGE_RECEIPT_ROOT_CAUSE" "$STAGE_RECEIPT_ROOT_CAUSE_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_PREREG" "$STAGE_RECEIPT_PREDECESSOR_PREREG_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_RUNNER" "$STAGE_RECEIPT_PREDECESSOR_RUNNER_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_TERMINAL" "$STAGE_RECEIPT_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT" "$STAGE_RECEIPT_PREDECESSOR_ATTEMPT_SHA"
  require_frozen_file "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG" "$STAGE_RECEIPT_PREDECESSOR_WORKER_LOG_SHA"
  require_frozen_file "$CORRECTED_CONTROL_PREREG" "$CORRECTED_CONTROL_PREREG_SHA"
  require_frozen_file "$PREDECESSOR_PREREG" "$PREDECESSOR_PREREG_SHA"
  require_frozen_file "$SELF_TEST_ROOT_CAUSE" "$SELF_TEST_ROOT_CAUSE_SHA"
  require_frozen_file "$ROOT_CAUSE" "$ROOT_CAUSE_SHA"
  require_frozen_file "$RAW_CONTROL_PREDECESSOR_TERMINAL" "$RAW_CONTROL_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$SELF_TEST_PREDECESSOR_TERMINAL" "$SELF_TEST_PREDECESSOR_TERMINAL_SHA"
  require_frozen_file "$PREDECESSOR_SELF_TEST_OUTPUT" "$PREDECESSOR_SELF_TEST_OUTPUT_SHA"
  require_frozen_file "$PREDECESSOR_SELF_TEST_LOG" "$PREDECESSOR_SELF_TEST_LOG_SHA"
  require_frozen_file "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA"
  require_frozen_file "$CAPTURE_PREDECESSOR_SOURCE" "$CAPTURE_PREDECESSOR_SOURCE_SHA"
  require_frozen_file "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA"
  require_frozen_file "$CAPTURE_PREDECESSOR_BUILD_SCRIPT" "$CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA"
  require_frozen_file "$COMMON_ARCHIVE" "$COMMON_ARCHIVE_SHA"
  require_frozen_file "$TORCH_ARCHIVE" "$TORCH_ARCHIVE_SHA"
  verify_predecessor_lineage
  verify_stage_receipt_fix_predecessor_lineage
}

verify_source_manifest_payloads() {
  local ordinal index kind path expected size
  expect_kv "$SOURCE_MANIFEST" prefix_source_count 9
  expect_kv "$SOURCE_MANIFEST" mirror_csv_count 9
  expect_kv "$SOURCE_MANIFEST" mirror_cache_count 18
  for ordinal in 0 1 2 3 4 5 6 7 8; do
    printf -v index '%02d' "$ordinal"
    for kind in mirror raw_cache normalized_cache; do
      path="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_path")"
      expected="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_sha256")"
      size="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_size_bytes")"
      [[ "$path" == "$SOURCE_ROOT"/* ]] || fail "source-manifest payload escapes isolated root: ${path}"
      reject_forbidden_path "$path"
      [[ "$expected" =~ ^[0-9a-f]{64}$ && "$size" =~ ^[0-9]+$ ]] || fail "invalid source-manifest payload binding"
      require_frozen_file "$path" "$expected"
      [[ "$(stat -c '%s' -- "$path")" == "$size" ]] || fail "source-manifest payload size mismatch: ${path}"
    done
  done
}

preflight_scientific_authority() {
  local path
  preflight_build_authority
  require_frozen_file "$PHASE2A_RECEIPT" "$PHASE2A_RECEIPT_SHA"
  require_frozen_file "$CAPTURE_CONFIG" "$CAPTURE_CONFIG_SHA"
  require_frozen_file "$SOURCE_CLOSURE" "$SOURCE_CLOSURE_SHA"
  require_frozen_file "$CURSOR_ERRATUM" "$CURSOR_ERRATUM_SHA"
  require_frozen_file "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA"
  require_frozen_file "$REPRESENTATION_TRAIN" "$REPRESENTATION_TRAIN_SHA"
  require_frozen_file "$REPRESENTATION_VALIDATION" "$REPRESENTATION_VALIDATION_SHA"
  for path in "$CAPTURE_CONFIG" "$SOURCE_CLOSURE" "$CURSOR_ERRATUM" "$SOURCE_MANIFEST" "$REPRESENTATION_TRAIN" "$REPRESENTATION_VALIDATION"; do
    reject_forbidden_path "$path"
  done
  expect_kv "$PHASE2A_RECEIPT" status complete
  expect_kv "$PHASE2A_RECEIPT" diagnostic_authority development_only
  expect_kv "$PHASE2A_RECEIPT" classification edge_channel_affine_sufficiency_not_established
  expect_kv "$PHASE2A_RECEIPT" rung_b_authorized true
  expect_kv "$PHASE2A_RECEIPT" certified_input_access false
  expect_kv "$PHASE2A_RECEIPT" final_holdout_access false
  expect_kv "$PHASE2A_RECEIPT" policy_access false
  expect_kv "$SOURCE_MANIFEST" isolated_source_root "$SOURCE_ROOT"
  expect_kv "$SOURCE_MANIFEST" canonical_data_raw_access false
  expect_kv "$SOURCE_MANIFEST" final_holdout_available false
  require_canonical_path "$SOURCE_ROOT"
  [[ -d "$SOURCE_ROOT" && ! -L "$SOURCE_ROOT" ]] || fail "invalid isolated source root"
  verify_source_manifest_payloads
}

open_lock() {
  local parent
  parent="$(dirname -- "$RUNTIME_ROOT")"; require_canonical_path "$parent"
  [[ -e "$RUNTIME_ROOT" ]] || mkdir -m 0700 -- "$RUNTIME_ROOT"
  require_canonical_path "$RUNTIME_ROOT"
  [[ -d "$RUNTIME_ROOT" && ! -L "$RUNTIME_ROOT" ]] || fail "invalid corrected-control runtime root"
  require_private_directory "$RUNTIME_ROOT"
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then (set -o noclobber; : > "$LOCK") 2>/dev/null || true; fi
  require_canonical_path "$LOCK"; [[ -f "$LOCK" && ! -L "$LOCK" ]] || fail "invalid lock"
  chmod 0600 -- "$LOCK"
  [[ "$(stat -c '%a:%u:%h' -- "$LOCK")" == "600:0:1" ]] || fail "lock metadata mismatch"
  exec 9<> "$LOCK"
  flock -n 9 || fail "another corrected-control operation holds the lock"
}

require_scratch_ready() {
  require_canonical_path "$SCRATCH"
  [[ -d "$SCRATCH" && ! -L "$SCRATCH" ]] || fail "invalid scratch directory"
  [[ "$(stat -c '%a:%u' -- "$SCRATCH")" == "700:0" ]] || fail "scratch metadata mismatch"
}

require_private_directory() {
  local directory="$1"
  require_canonical_path "$directory"
  [[ -d "$directory" && ! -L "$directory" ]] || fail "invalid runtime directory: ${directory}"
  [[ "$(stat -c '%a:%u' -- "$directory")" == "700:0" ]] || fail "runtime directory metadata mismatch: ${directory}"
}

publish_receipt() {
  local candidate="$1" destination="$2"
  require_scratch_ready
  [[ "$candidate" == "$SCRATCH/"* ]] || fail "receipt candidate is outside scratch: ${candidate}"
  require_canonical_path "$candidate"
  [[ ! -e "$destination" && ! -L "$destination" ]] || fail "receipt destination already exists: ${destination}"
  require_canonical_path "$(dirname -- "$destination")"
  chmod 0444 -- "$candidate"
  # Same-filesystem rename is the single atomic commit.  GNU mv -n may return
  # success after declining a raced destination, so candidate disappearance is
  # part of the publication proof.
  mv -T -n -- "$candidate" "$destination" || fail "immutable publication failed: ${destination}"
  [[ ! -e "$candidate" && ! -L "$candidate" ]] || fail "immutable no-clobber publication was declined: ${destination}"
  require_frozen_metadata "$destination"
}

write_build_receipt() {
  local kind="$1" source="$2" source_sha="$3" script="$4" script_sha="$5" binary="$6" receipt="$7" candidate
  require_scratch_ready
  candidate="${SCRATCH}/${kind}.build.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_build_v1"
    echo "status=complete"; echo "protocol_id=${PROTOCOL_ID}"; echo "kind=${kind}"
    emit_stage_receipt_fix_lineage
    echo "source_path=${source}"; echo "source_sha256=${source_sha}"
    echo "build_script_path=${script}"; echo "build_script_sha256=${script_sha}"
    if [[ "$kind" == capture ]]; then
      echo "predecessor_source_path=${CAPTURE_PREDECESSOR_SOURCE}"; echo "predecessor_source_sha256=${CAPTURE_PREDECESSOR_SOURCE_SHA}"
      echo "predecessor_build_script_path=${CAPTURE_PREDECESSOR_BUILD_SCRIPT}"; echo "predecessor_build_script_sha256=${CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA}"
      echo "common_archive_path=${COMMON_ARCHIVE}"; echo "common_archive_sha256=${COMMON_ARCHIVE_SHA}"
      echo "torch_archive_path=${TORCH_ARCHIVE}"; echo "torch_archive_sha256=${TORCH_ARCHIVE_SHA}"
    else
      echo "affine_source_path=${AFFINE_SOURCE}"; echo "affine_source_sha256=${AFFINE_SOURCE_SHA}"
    fi
    echo "binary_path=${binary}"; echo "binary_sha256=$(sha256 "$binary")"
    echo "build_before_attempt=true"; echo "scientific_input_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$receipt"
}

verify_build_receipt() {
  local kind="$1" source="$2" source_sha="$3" script="$4" script_sha="$5" binary="$6" receipt="$7"
  require_frozen_metadata "$receipt"
  expect_kv "$receipt" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_build_v1
  expect_kv "$receipt" status complete; expect_kv "$receipt" protocol_id "$PROTOCOL_ID"; expect_kv "$receipt" kind "$kind"
  verify_stage_receipt_fix_lineage_receipt "$receipt"
  expect_kv "$receipt" source_path "$source"; expect_kv "$receipt" source_sha256 "$source_sha"
  expect_kv "$receipt" build_script_path "$script"; expect_kv "$receipt" build_script_sha256 "$script_sha"
  if [[ "$kind" == capture ]]; then
    expect_kv "$receipt" predecessor_source_path "$CAPTURE_PREDECESSOR_SOURCE"; expect_kv "$receipt" predecessor_source_sha256 "$CAPTURE_PREDECESSOR_SOURCE_SHA"
    expect_kv "$receipt" predecessor_build_script_path "$CAPTURE_PREDECESSOR_BUILD_SCRIPT"; expect_kv "$receipt" predecessor_build_script_sha256 "$CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA"
    expect_kv "$receipt" common_archive_path "$COMMON_ARCHIVE"; expect_kv "$receipt" common_archive_sha256 "$COMMON_ARCHIVE_SHA"
    expect_kv "$receipt" torch_archive_path "$TORCH_ARCHIVE"; expect_kv "$receipt" torch_archive_sha256 "$TORCH_ARCHIVE_SHA"
  else
    expect_kv "$receipt" affine_source_path "$AFFINE_SOURCE"; expect_kv "$receipt" affine_source_sha256 "$AFFINE_SOURCE_SHA"
  fi
  expect_kv "$receipt" binary_path "$binary"; expect_kv "$receipt" build_before_attempt true; expect_kv "$receipt" scientific_input_access false
  require_frozen_metadata "$binary"; [[ -x "$binary" ]] || fail "binary is not executable: ${binary}"
  [[ "$(sha256 "$binary")" == "$(kv "$receipt" binary_sha256)" ]] || fail "binary/receipt mismatch"
}

prepare() {
  preflight_build_authority; open_lock
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] || fail "protocol is already attempted or retired"
  for directory in "$BUILD_DIR" "$SCRATCH"; do
    if [[ -e "$directory" || -L "$directory" ]]; then
      require_canonical_path "$directory"
      [[ -d "$directory" && ! -L "$directory" ]] || fail "invalid preparation directory: ${directory}"
    else
      mkdir -m 0700 -- "$directory"
      require_canonical_path "$directory"
    fi
    require_private_directory "$directory"
  done
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch is not pristine"
  if [[ ! -e "$CAPTURE_BUILD_RECEIPT" ]]; then
    [[ ! -e "$CAPTURE_BIN" && ! -L "$CAPTURE_BIN" ]] || fail "unreceipted capture binary exists"
    timeout --signal=TERM --kill-after=10s 900s "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BIN"
    chmod 0555 -- "$CAPTURE_BIN"
    write_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  fi
  if [[ ! -e "$NONLINEAR_BUILD_RECEIPT" ]]; then
    [[ ! -e "$NONLINEAR_BIN" && ! -L "$NONLINEAR_BIN" ]] || fail "unreceipted nonlinear binary exists"
    timeout --signal=TERM --kill-after=10s 900s "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BIN"
    chmod 0555 -- "$NONLINEAR_BIN"
    write_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  fi
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch gained an entry"
  echo "[clear-signal:phase2b-corrected-stage-receipt-fix] frozen builds prepared"
}

render_expected_self_test_report() {
  printf '%s\n' \
    'schema_id=synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_self_test_v1' \
    'status=passed' \
    'expected_case_count=8' \
    'expected_cases=false_structural_padding,oldest_in_capacity_false,multiple_true_finite,raw96_placement_and_serialization,canonical_stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,reject_nonfinite_true' \
    'observed_canonical_output_sha256=dadad8ab786ad5205792f4a1aea4eb9bd154b82c10405d3c5cea36b4423dd5d9' \
    'source_binary_binding_required_in_immutable_runner_receipt=true' \
    'project_artifact_access=false' \
    'status_line=corrected-control mask self-test passed'
}

validate_self_test_report() {
  local report="$1"
  cmp -s -- "$report" <(render_expected_self_test_report) || fail "self-test report bytes/order differ from the frozen eight-record payload"
  expect_kv "$report" schema_id "$SELF_TEST_SCHEMA"; expect_kv "$report" status passed
  expect_kv "$report" expected_case_count 8
  expect_kv "$report" expected_cases false_structural_padding,oldest_in_capacity_false,multiple_true_finite,raw96_placement_and_serialization,canonical_stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,reject_nonfinite_true
  expect_kv "$report" observed_canonical_output_sha256 dadad8ab786ad5205792f4a1aea4eb9bd154b82c10405d3c5cea36b4423dd5d9
  expect_kv "$report" source_binary_binding_required_in_immutable_runner_receipt true
  expect_kv "$report" project_artifact_access false
  expect_kv "$report" status_line "corrected-control mask self-test passed"
}

validate_self_test_stdout() {
  local stdout="$1" esc token token_regex expected_init expected_finit lines=()
  [[ "$(wc -l < "$stdout")" == 2 ]] || fail "self-test stdout record count mismatch"
  mapfile -t lines < "$stdout"
  [[ "${#lines[@]}" == 2 ]] || fail "self-test stdout contained an unterminated or extra record"
  esc=$'\033'
  token_regex="^\\[${esc}\\[36m0x([0-9]+)${esc}\\[0m\\]:"
  [[ "${lines[0]}" =~ $token_regex ]] || fail "self-test stdout init record has invalid ANSI framing or thread token"
  token="${BASH_REMATCH[1]}"
  expected_init="[${esc}[36m0x${token}${esc}[0m]: ${esc}[94mDEBUG${esc}[0m: [source_runtime_t] initializing static-global source snapshot (single mutable cache updated by explicit runtime call)"
  expected_finit="[${esc}[36m0x${token}${esc}[0m]: ${esc}[94mDEBUG${esc}[0m: [source_runtime_t] finalizing static-global source snapshot (last_config_path=<none>)"
  cmp -s -- "$stdout" <(printf '%s\n%s\n' "$expected_init" "$expected_finit") ||
    fail "self-test stdout was not exactly the two frozen ANSI init/finit records with one shared decimal thread token"
}

render_normalized_self_test_stdout() {
  printf '[\033[36m0x<thread-id>\033[0m]: \033[94mDEBUG\033[0m: [source_runtime_t] initializing static-global source snapshot (single mutable cache updated by explicit runtime call)\n'
  printf '[\033[36m0x<thread-id>\033[0m]: \033[94mDEBUG\033[0m: [source_runtime_t] finalizing static-global source snapshot (last_config_path=<none>)\n'
}

validate_self_test_stdout_normalized() {
  local normalized="$1"
  cmp -s -- "$normalized" <(render_normalized_self_test_stdout) ||
    fail "normalized self-test stdout differs from the frozen token-only normalization"
}

publish_self_test_stdout_normalized() {
  [[ ! -e "$SELF_TEST_STDOUT_NORMALIZED" && ! -L "$SELF_TEST_STDOUT_NORMALIZED" ]] ||
    fail "normalized self-test stdout path already exists"
  (set -o noclobber; render_normalized_self_test_stdout > "$SELF_TEST_STDOUT_NORMALIZED") 2>/dev/null ||
    fail "could not exclusively create normalized self-test stdout evidence"
  validate_self_test_stdout_normalized "$SELF_TEST_STDOUT_NORMALIZED"
}

validate_self_test_stderr() {
  local stderr="$1"
  [[ ! -s "$stderr" ]] || fail "self-test stderr was not empty"
}

seal_self_test_terminal() {
  local rc="$1" stage="$2" candidate="${SCRATCH}/terminal.invalid.status.$$"
  require_scratch_ready
  [[ ! -e "$TERMINAL" ]] || fail "terminal receipt already exists"
  [[ ! -e "$ATTEMPT" ]] || fail "self-test terminal cannot follow attempt"
  [[ -f "$SELF_TEST_REPORT" && ! -L "$SELF_TEST_REPORT" ]] && chmod 0444 -- "$SELF_TEST_REPORT"
  [[ -f "$SELF_TEST_STDOUT" && ! -L "$SELF_TEST_STDOUT" ]] && chmod 0444 -- "$SELF_TEST_STDOUT"
  [[ -f "$SELF_TEST_STDOUT_NORMALIZED" && ! -L "$SELF_TEST_STDOUT_NORMALIZED" ]] && chmod 0444 -- "$SELF_TEST_STDOUT_NORMALIZED"
  [[ -f "$SELF_TEST_STDERR" && ! -L "$SELF_TEST_STDERR" ]] && chmod 0444 -- "$SELF_TEST_STDERR"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_terminal_invalid_v1"
    echo "status=terminal_invalid"; echo "protocol_id=${PROTOCOL_ID}"
    emit_stage_receipt_fix_lineage
    echo "diagnostic_authority=development_only"; echo "benchmark_acceptance_authority=false"
    echo "classification=invalid_data_free_self_test_failure"; echo "failure_stage=${stage}"; echo "failure_exit_code=${rc}"
    echo "attempt_consumed=false"; echo "same_protocol_resume_allowed=false"; echo "same_protocol_retry_allowed=false"; echo "new_protocol_required=true"
    echo "capture_source_sha256=${CAPTURE_SOURCE_SHA}"; echo "capture_binary_sha256=$([[ -f "$CAPTURE_BIN" && ! -L "$CAPTURE_BIN" ]] && sha256 "$CAPTURE_BIN" || echo absent)"
    echo "self_test_report_sha256=$([[ -f "$SELF_TEST_REPORT" && ! -L "$SELF_TEST_REPORT" ]] && sha256 "$SELF_TEST_REPORT" || echo absent)"
    echo "self_test_stdout_sha256=$([[ -f "$SELF_TEST_STDOUT" && ! -L "$SELF_TEST_STDOUT" ]] && sha256 "$SELF_TEST_STDOUT" || echo absent)"
    echo "self_test_stdout_normalized_sha256=$([[ -f "$SELF_TEST_STDOUT_NORMALIZED" && ! -L "$SELF_TEST_STDOUT_NORMALIZED" ]] && sha256 "$SELF_TEST_STDOUT_NORMALIZED" || echo absent)"
    echo "self_test_stderr_sha256=$([[ -f "$SELF_TEST_STDERR" && ! -L "$SELF_TEST_STDERR" ]] && sha256 "$SELF_TEST_STDERR" || echo absent)"
    echo "isolated_development_source_access_started=false"; echo "representation_probe_access=false"
    echo "raw_capture_invocations_started=0"; echo "raw_capture_invocations_completed=0"; echo "evaluator_invocations_started=0"; echo "evaluator_invocations_completed=0"
    echo "fits_started=0"; echo "fits_completed=0"; echo "optimizer_steps=0"; echo "scientific_result_available=false"
    echo "maximum_anchor_read_upper_bound=none"; echo "certified_input_access=false"; echo "final_holdout_access=false"; echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$TERMINAL"
}

self_test_exit_guard() {
  local rc="$1"
  trap - EXIT HUP INT TERM
  if (( rc != 0 )) && [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_self_test_terminal "$rc" "${SELF_TEST_FAILURE_STAGE:-self_test_internal_failure}"
  fi
  exit "$rc"
}

verify_self_test_receipt() {
  require_frozen_metadata "$SELF_TEST_RECEIPT"
  require_frozen_metadata "$SELF_TEST_REPORT"
  require_frozen_metadata "$SELF_TEST_STDOUT"
  require_frozen_metadata "$SELF_TEST_STDOUT_NORMALIZED"
  require_frozen_metadata "$SELF_TEST_STDERR"
  expect_kv "$SELF_TEST_RECEIPT" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_receipt_v1
  expect_kv "$SELF_TEST_RECEIPT" status passed; expect_kv "$SELF_TEST_RECEIPT" protocol_id "$PROTOCOL_ID"; expect_kv "$SELF_TEST_RECEIPT" scientific_input_access false
  verify_stage_receipt_fix_lineage_receipt "$SELF_TEST_RECEIPT"
  expect_kv "$SELF_TEST_RECEIPT" preregistration_sha256 "$PREREG_SHA"
  expect_kv "$SELF_TEST_RECEIPT" self_test_root_cause_sha256 "$SELF_TEST_ROOT_CAUSE_SHA"
  expect_kv "$SELF_TEST_RECEIPT" predecessor_terminal_sha256 "$SELF_TEST_PREDECESSOR_TERMINAL_SHA"
  expect_kv "$SELF_TEST_RECEIPT" source_sha256 "$CAPTURE_SOURCE_SHA"
  expect_kv "$SELF_TEST_RECEIPT" source_path "$CAPTURE_SOURCE"; expect_kv "$SELF_TEST_RECEIPT" binary_path "$CAPTURE_BIN"
  expect_kv "$SELF_TEST_RECEIPT" capture_build_receipt_sha256 "$(sha256 "$CAPTURE_BUILD_RECEIPT")"
  expect_kv "$SELF_TEST_RECEIPT" report_path "$SELF_TEST_REPORT"
  expect_kv "$SELF_TEST_RECEIPT" stdout_path "$SELF_TEST_STDOUT"
  expect_kv "$SELF_TEST_RECEIPT" stdout_normalized_path "$SELF_TEST_STDOUT_NORMALIZED"
  expect_kv "$SELF_TEST_RECEIPT" stderr_path "$SELF_TEST_STDERR"
  expect_kv "$SELF_TEST_RECEIPT" expected_case_count 8
  expect_kv "$SELF_TEST_RECEIPT" expected_cases false_structural_padding,oldest_in_capacity_false,multiple_true_finite,raw96_placement_and_serialization,canonical_stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,reject_nonfinite_true
  expect_kv "$SELF_TEST_RECEIPT" observed_canonical_output_sha256 dadad8ab786ad5205792f4a1aea4eb9bd154b82c10405d3c5cea36b4423dd5d9
  expect_kv "$SELF_TEST_RECEIPT" report_created_exclusively_by_capture true
  expect_kv "$SELF_TEST_RECEIPT" report_validation passed
  expect_kv "$SELF_TEST_RECEIPT" stdout_normalization thread_token_replaced_after_exact_ansi_validation
  expect_kv "$SELF_TEST_RECEIPT" stdout_record_count 2
  expect_kv "$SELF_TEST_RECEIPT" stdout_validation passed
  expect_kv "$SELF_TEST_RECEIPT" stdout_normalized_validation passed
  expect_kv "$SELF_TEST_RECEIPT" stderr_empty true
  expect_kv "$SELF_TEST_RECEIPT" stderr_size_bytes 0
  expect_kv "$SELF_TEST_RECEIPT" stderr_validation passed
  expect_kv "$SELF_TEST_RECEIPT" source_access false; expect_kv "$SELF_TEST_RECEIPT" checkpoint_access false
  expect_kv "$SELF_TEST_RECEIPT" model_execution false; expect_kv "$SELF_TEST_RECEIPT" policy_access false
  [[ "$(sha256 "$CAPTURE_BIN")" == "$(kv "$SELF_TEST_RECEIPT" binary_sha256)" ]] || fail "self-test binary identity mismatch"
  [[ "$(sha256 "$SELF_TEST_REPORT")" == "$(kv "$SELF_TEST_RECEIPT" report_sha256)" ]] || fail "self-test report identity mismatch"
  [[ "$(stat -c '%s' -- "$SELF_TEST_REPORT")" == "$(kv "$SELF_TEST_RECEIPT" report_size_bytes)" ]] || fail "self-test report size mismatch"
  [[ "$(sha256 "$SELF_TEST_STDOUT")" == "$(kv "$SELF_TEST_RECEIPT" stdout_sha256)" ]] || fail "self-test stdout identity mismatch"
  [[ "$(stat -c '%s' -- "$SELF_TEST_STDOUT")" == "$(kv "$SELF_TEST_RECEIPT" stdout_size_bytes)" ]] || fail "self-test stdout size mismatch"
  [[ "$(sha256 "$SELF_TEST_STDOUT_NORMALIZED")" == "$(kv "$SELF_TEST_RECEIPT" stdout_normalized_sha256)" ]] || fail "normalized self-test stdout identity mismatch"
  [[ "$(stat -c '%s' -- "$SELF_TEST_STDOUT_NORMALIZED")" == "$(kv "$SELF_TEST_RECEIPT" stdout_normalized_size_bytes)" ]] || fail "normalized self-test stdout size mismatch"
  [[ "$(sha256 "$SELF_TEST_STDERR")" == "$(kv "$SELF_TEST_RECEIPT" stderr_sha256)" ]] || fail "self-test stderr identity mismatch"
  validate_self_test_report "$SELF_TEST_REPORT"
  validate_self_test_stdout "$SELF_TEST_STDOUT"
  validate_self_test_stdout_normalized "$SELF_TEST_STDOUT_NORMALIZED"
  validate_self_test_stderr "$SELF_TEST_STDERR"
}

self_test() {
  local rc candidate
  preflight_capture_self_test_authority; open_lock
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] || fail "protocol is already attempted or retired"
  require_scratch_ready
  SELF_TEST_FAILURE_STAGE=self_test_authority_validation
  trap 'self_test_exit_guard "$?"' EXIT
  trap 'SELF_TEST_FAILURE_STAGE=self_test_signal_hup; exit 129' HUP
  trap 'SELF_TEST_FAILURE_STAGE=self_test_signal_int; exit 130' INT
  trap 'SELF_TEST_FAILURE_STAGE=self_test_signal_term; exit 143' TERM
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  if [[ -e "$SELF_TEST_RECEIPT" || -L "$SELF_TEST_RECEIPT" ]]; then
    SELF_TEST_FAILURE_STAGE=self_test_existing_receipt_validation
    verify_self_test_receipt
    trap - EXIT HUP INT TERM
    echo "[clear-signal:phase2b-corrected-stage-receipt-fix] existing self-test verified"
    return
  fi
  SELF_TEST_FAILURE_STAGE=self_test_pristine_state_validation
  [[ ! -e "$SELF_TEST_DIR" && ! -L "$SELF_TEST_DIR" ]] || fail "unreceipted self-test directory exists"
  mkdir -m 0700 -- "$SELF_TEST_DIR"
  require_private_directory "$SELF_TEST_DIR"
  SELF_TEST_FAILURE_STAGE=self_test_execution
  if timeout --signal=TERM --kill-after=5s 120s "$CAPTURE_BIN" --self-test --self-test-report "$SELF_TEST_REPORT" >"$SELF_TEST_STDOUT" 2>"$SELF_TEST_STDERR"; then rc=0; else rc=$?; fi
  (( rc == 0 )) || exit "$rc"
  SELF_TEST_FAILURE_STAGE=self_test_validation
  validate_self_test_report "$SELF_TEST_REPORT"
  validate_self_test_stdout "$SELF_TEST_STDOUT"
  publish_self_test_stdout_normalized
  validate_self_test_stderr "$SELF_TEST_STDERR"
  SELF_TEST_FAILURE_STAGE=self_test_sealing
  chmod 0444 -- "$SELF_TEST_REPORT" "$SELF_TEST_STDOUT" "$SELF_TEST_STDOUT_NORMALIZED" "$SELF_TEST_STDERR"
  require_scratch_ready
  candidate="${SCRATCH}/self_test.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_receipt_v1"; echo "status=passed"; echo "protocol_id=${PROTOCOL_ID}"
    emit_stage_receipt_fix_lineage
    echo "preregistration_sha256=${PREREG_SHA}"; echo "self_test_root_cause_sha256=${SELF_TEST_ROOT_CAUSE_SHA}"; echo "predecessor_terminal_sha256=${SELF_TEST_PREDECESSOR_TERMINAL_SHA}"
    echo "source_path=${CAPTURE_SOURCE}"; echo "source_sha256=${CAPTURE_SOURCE_SHA}"
    echo "binary_path=${CAPTURE_BIN}"; echo "binary_sha256=$(sha256 "$CAPTURE_BIN")"
    echo "capture_build_receipt_sha256=$(sha256 "$CAPTURE_BUILD_RECEIPT")"
    echo "report_path=${SELF_TEST_REPORT}"; echo "report_sha256=$(sha256 "$SELF_TEST_REPORT")"; echo "report_size_bytes=$(stat -c '%s' -- "$SELF_TEST_REPORT")"
    echo "stdout_path=${SELF_TEST_STDOUT}"; echo "stdout_sha256=$(sha256 "$SELF_TEST_STDOUT")"; echo "stdout_size_bytes=$(stat -c '%s' -- "$SELF_TEST_STDOUT")"
    echo "stdout_normalized_path=${SELF_TEST_STDOUT_NORMALIZED}"; echo "stdout_normalized_sha256=$(sha256 "$SELF_TEST_STDOUT_NORMALIZED")"; echo "stdout_normalized_size_bytes=$(stat -c '%s' -- "$SELF_TEST_STDOUT_NORMALIZED")"
    echo "stderr_path=${SELF_TEST_STDERR}"; echo "stderr_sha256=$(sha256 "$SELF_TEST_STDERR")"; echo "stderr_size_bytes=$(stat -c '%s' -- "$SELF_TEST_STDERR")"
    echo "expected_case_count=8"; echo "expected_cases=$(kv "$SELF_TEST_REPORT" expected_cases)"
    echo "observed_canonical_output_sha256=$(kv "$SELF_TEST_REPORT" observed_canonical_output_sha256)"
    echo "report_created_exclusively_by_capture=true"; echo "report_validation=passed"; echo "stdout_normalization=thread_token_replaced_after_exact_ansi_validation"; echo "stdout_record_count=2"; echo "stdout_validation=passed"; echo "stdout_normalized_validation=passed"; echo "stderr_empty=true"; echo "stderr_validation=passed"
    echo "scientific_input_access=false"; echo "source_access=false"; echo "checkpoint_access=false"; echo "model_execution=false"; echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$SELF_TEST_RECEIPT"; chmod 0555 -- "$SELF_TEST_DIR"
  SELF_TEST_FAILURE_STAGE=self_test_receipt_verification
  verify_self_test_receipt
  trap - EXIT HUP INT TERM
  unset SELF_TEST_FAILURE_STAGE
  echo "[clear-signal:phase2b-corrected-stage-receipt-fix] data-free self-test passed"
}

validate_capture_report() {
  local report="$1" probe="$2" split="$3" range="$4" anchors="$5" rows="$6" maximum="$7" channel capacity min max hash fingerprint batches
  require_canonical_path "$report"; require_canonical_path "$probe"
  expect_kv "$report" schema_id "$CAPTURE_REPORT_SCHEMA"; expect_kv "$report" status complete
  expect_kv "$report" benchmark_id synthetic_continuous_graph_v2; expect_kv "$report" diagnostic_phase 2B
  expect_kv "$report" diagnostic_authority development_only; expect_kv "$report" benchmark_acceptance_authority false
  expect_kv "$report" config_path "$CAPTURE_CONFIG"; expect_kv "$report" config_sha256 "$CAPTURE_CONFIG_SHA"; expect_kv "$report" config_immutable true
  expect_kv "$report" anchor_range "$range"; expect_kv "$report" anchor_count "$anchors"; expect_kv "$report" maximum_anchor_read "$maximum"
  expect_kv "$report" source_order_policy sequential; expect_kv "$report" cursor_contiguous true
  batches="$(kv "$report" lifted_batch_count)"; [[ "$batches" =~ ^[1-9][0-9]*$ ]] || fail "invalid lifted batch count"
  fingerprint="$(kv "$report" graph_order_fingerprint)"; [[ -n "$fingerprint" && "$fingerprint" != *[[:space:]=]* ]] || fail "invalid graph-order fingerprint"
  expect_kv "$report" probe_path "$probe"; expect_kv "$report" probe_record_schema "$RAW_SCHEMA"; expect_kv "$report" probe_header "$PROBE_HEADER"
  expect_kv "$report" probe_rows "$rows"; expect_kv "$report" probe_feature_count 96; expect_kv "$report" feature_layout base_32,quote_32,base_minus_quote_32
  expect_kv "$report" row_order anchor_base_edge_channel; expect_kv "$report" canonical_coordinate_order true
  expect_kv "$report" channel_count 3; expect_kv "$report" node_count 4; expect_kv "$report" edge_count 3; expect_kv "$report" history_length 30; expect_kv "$report" padded_history_width 32
  expect_kv "$report" history_right_aligned true; expect_kv "$report" source_dtype float32; expect_kv "$report" canonical_target_serialization_dtype float32
  expect_kv "$report" close_coordinate 3; expect_kv "$report" structural_padding_contract_passed true; expect_kv "$report" variable_close_mask_within_capacity_allowed true; expect_kv "$report" masked_close_values_zero true
  for channel in 0 1 2; do
    case "$channel" in 0) capacity=4;; 1) capacity=10;; 2) capacity=30;; esac
    min="$(kv "$report" "close_mask_count.${split}.channel_${channel}.min")"; max="$(kv "$report" "close_mask_count.${split}.channel_${channel}.max")"
    [[ "$min" =~ ^[0-9]+$ && "$max" =~ ^[0-9]+$ ]] || fail "invalid close-mask bounds"
    (( min <= max && max <= capacity )) || fail "close-mask bounds exceed capacity"
    expect_kv "$report" "close_mask_count.${split}.channel_${channel}.capacity" "$capacity"
  done
  expect_kv "$report" close_mask_stream_schema synthetic_v2_nodelift_close_mask_v1
  expect_kv "$report" close_mask_stream_order anchor_index,node_index,channel_index
  hash="$(kv "$report" "close_mask_sha256.${split}")"; [[ "$hash" =~ ^[0-9a-f]{64}$ ]] || fail "invalid close-mask stream hash"
  expect_kv "$report" future_horizon 1; expect_kv "$report" target_definition future_base_minus_quote_close_coordinate; expect_kv "$report" target_mask_complete true
  expect_kv "$report" pre_encoder_mtf_channel_node_input_used true
  expect_kv "$report" representation_config_parsed_as_inert_dependency true
  expect_kv "$report" representation_model_constructed false; expect_kv "$report" representation_checkpoint_access false; expect_kv "$report" representation_execution false
  expect_kv "$report" mdn_config_parsed_as_inert_dependency true
  expect_kv "$report" mdn_model_constructed false; expect_kv "$report" mdn_checkpoint_access false; expect_kv "$report" mdn_execution false
  expect_kv "$report" policy_config_parsed_as_inert_dependency true
  expect_kv "$report" policy_model_constructed false; expect_kv "$report" policy_checkpoint_access false; expect_kv "$report" policy_execution false
  expect_kv "$report" checkpoint_cli_accepted false; expect_kv "$report" optimizer_steps 0; expect_kv "$report" checkpoint_written false
}

validate_probe_identity() {
  local representation="$1" raw="$2" expected_rows="$3"
  awk -F, -v rep_schema="$REPRESENTATION_SCHEMA" -v raw_schema="$RAW_SCHEMA" -v header="$PROBE_HEADER" -v expected="$expected_rows" '
    NR==FNR {if(FNR==1){if($0!=header)exit 10;next} if($1!=rep_schema||NF!=12)exit 11; sig[FNR]=$2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10 FS $11; n=FNR;next}
    FNR==1 {if($0!=header)exit 12;next}
    {if($1!=raw_schema||NF!=12||$11!="96")exit 13; cur=$2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10 FS $11; if(!(FNR in sig)||sig[FNR]!=cur)exit 14; m=FNR}
    END {if(n!=expected+1||m!=expected+1)exit 15}
  ' "$representation" "$raw" || fail "raw/representation coordinate or target identity failed"
}

finite_number() { [[ "$1" =~ ^[-+]?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$ ]] || fail "invalid finite number: $1"; }
unit_number() { finite_number "$1"; awk -v x="$1" 'BEGIN{exit !(x>=0&&x<=1)}' || fail "number outside [0,1]: $1"; }
nonnegative_number() { finite_number "$1"; awk -v x="$1" 'BEGIN{exit !(x>=0)}' || fail "negative metric: $1"; }
integer_between() { [[ "$1" =~ ^[0-9]+$ ]] && (( $1 >= $2 && $1 <= $3 )) || fail "integer outside [$2,$3]: $1"; }

validate_metric() {
  local report="$1" prefix="$2" count="$3" pairs="$4" key value
  expect_kv "$report" "${prefix}.count" "$count"; expect_kv "$report" "${prefix}.pairwise_rank_count" "$pairs"
  for key in mae rmse target_rms prediction_rms rmse_target_rms_ratio; do value="$(kv "$report" "${prefix}.${key}")"; nonnegative_number "$value"; done
  for key in directional_accuracy pairwise_rank_accuracy best_asset_agreement; do value="$(kv "$report" "${prefix}.${key}")"; unit_number "$value"; done
  value="$(kv "$report" "${prefix}.correlation")"; finite_number "$value"; awk -v x="$value" 'BEGIN{exit !(x>=-1.0000001&&x<=1.0000001)}' || fail "invalid correlation"
}

validate_median_metric() {
  local report="$1" prefix="$2" count="$3" pairs="$4" seed_prefix="$5" key expected
  validate_metric "$report" "$prefix" "$count" "$pairs"
  for key in mae rmse target_rms prediction_rms rmse_target_rms_ratio directional_accuracy pairwise_rank_accuracy best_asset_agreement correlation; do
    expected="$(printf '%s\n' "$(kv "$report" "${seed_prefix/SEED/31}.${key}")" "$(kv "$report" "${seed_prefix/SEED/47}.${key}")" "$(kv "$report" "${seed_prefix/SEED/73}.${key}")" | sort -g | sed -n '2p')"
    [[ "$(kv "$report" "${prefix}.${key}")" == "$expected" ]] || fail "median mismatch: ${prefix}.${key}"
  done
}

computed_gate() {
  local report="$1" prefix="$2"
  awk -v d="$(kv "$report" "${prefix}.directional_accuracy")" -v r="$(kv "$report" "${prefix}.pairwise_rank_accuracy")" -v c="$(kv "$report" "${prefix}.correlation")" -v q="$(kv "$report" "${prefix}.rmse_target_rms_ratio")" 'BEGIN{print(d>=.95&&r>=.95&&c>=.95&&q<=.25)?"true":"false"}'
}

computed_partial_gate() {
  local report="$1" prefix="$2"
  awk -v d="$(kv "$report" "${prefix}.directional_accuracy")" -v r="$(kv "$report" "${prefix}.pairwise_rank_accuracy")" 'BEGIN{print(d>=.80&&r>=.78)?"true":"false"}'
}

validate_nonlinear_report() {
  local report="$1" arm seed channel prefix strong sum count pass raw_pass rep_pass expected fingerprint
  expect_kv "$report" schema_id "$EVALUATOR_REPORT_SCHEMA"; expect_kv "$report" status complete; expect_kv "$report" benchmark_id synthetic_continuous_graph_v2; expect_kv "$report" diagnostic_phase 2B
  expect_kv "$report" diagnostic_authority development_only; expect_kv "$report" benchmark_acceptance_authority false
  expect_kv "$report" representation_record_schema "$REPRESENTATION_SCHEMA"; expect_kv "$report" raw_record_schema "$RAW_SCHEMA"
  expect_kv "$report" probe_header "$PROBE_HEADER"
  expect_kv "$report" representation_train_input "$REPRESENTATION_TRAIN"; expect_kv "$report" representation_validation_input "$REPRESENTATION_VALIDATION"
  expect_kv "$report" raw_train_input "$RAW_TRAIN"; expect_kv "$report" raw_validation_input "$RAW_VALIDATION"
  expect_kv "$report" train_probe_rows 22464; expect_kv "$report" validation_probe_rows 2304
  expect_kv "$report" fit_anchor_range '[0,2496)'; expect_kv "$report" validation_anchor_range '[2560,2816)'; expect_kv "$report" maximum_anchor_read 2815
  expect_kv "$report" certified_anchor_range not_opened; expect_kv "$report" final_holdout_access false; expect_kv "$report" policy_access false
  expect_kv "$report" representation_forward_executed false; expect_kv "$report" checkpoint_written false; expect_kv "$report" refit_after_evaluation false
  expect_kv "$report" coordinate_columns_exact_identity true; expect_kv "$report" target_columns_exact_identity true; expect_kv "$report" target_tensor_shared_between_arms true; expect_kv "$report" row_order_exact_identity true
  expect_kv "$report" feature_layout base_32,quote_32,base_minus_quote_32; expect_kv "$report" feature_width 96
  nonnegative_number "$(kv "$report" representation_feature_identity_max_abs_delta)"; nonnegative_number "$(kv "$report" raw_feature_identity_max_abs_delta)"
  awk -v x="$(kv "$report" representation_feature_identity_max_abs_delta)" 'BEGIN{exit !(x<=0.000002)}' || fail "representation identity delta exceeds tolerance"
  awk -v x="$(kv "$report" raw_feature_identity_max_abs_delta)" 'BEGIN{exit !(x<=0.000002)}' || fail "raw identity delta exceeds tolerance"
  expect_kv "$report" raw_configured_capacity_leading_zero_prefix_verified true; expect_kv "$report" raw_configured_capacity_padding_zero_values_checked 1287936
  expect_kv "$report" raw_configured_capacities 4,10,30; expect_kv "$report" raw_values_within_configured_capacity_may_be_zero true
  expect_kv "$report" input_standardization train_global_per_arm; expect_kv "$report" target_standardization train_per_edge_channel
  expect_numeric_close "$report" input_standard_deviation_floor 1e-8 1e-20
  expect_numeric_close "$report" target_standard_deviation_floor 1e-8 1e-20
  integer_between "$(kv "$report" raw_input_clamped_coordinate_count)" 0 96
  integer_between "$(kv "$report" representation_input_clamped_coordinate_count)" 0 96
  integer_between "$(kv "$report" target_clamped_coordinate_count)" 0 9
  expect_kv "$report" architecture linear_96_128_gelu_128_128_gelu_128_9; expect_kv "$report" head_selection channel_index_times_3_plus_edge_index
  expect_numeric_close "$report" dropout 0 1e-15; expect_kv "$report" normalization_layers false; expect_kv "$report" residual_branch false; expect_kv "$report" mixture_distribution false
  expect_kv "$report" loss standardized_target_mean_squared_error; expect_kv "$report" device cpu; expect_kv "$report" dtype float32; expect_kv "$report" deterministic_algorithms true
  expect_kv "$report" seeds 31,47,73; expect_kv "$report" steps_per_fit 3500; expect_kv "$report" batch_size 64; expect_kv "$report" optimizer adam
  expect_numeric_close "$report" learning_rate 0.001 1e-15
  expect_numeric_close "$report" adam_beta1 0.9 1e-15
  expect_numeric_close "$report" adam_beta2 0.999 1e-15
  expect_numeric_close "$report" adam_epsilon 1e-8 1e-20
  expect_numeric_close "$report" weight_decay 0 1e-15
  expect_numeric_close "$report" gradient_clip_norm 5 1e-15
  expect_kv "$report" batch_sampling mt19937_64_uniform_with_replacement
  expect_kv "$report" paired_initial_parameters_exact true; expect_kv "$report" paired_batch_schedule_exact true; expect_kv "$report" validation_read_by_trainer false
  expect_kv "$report" early_stopping false; expect_kv "$report" seed_selection false; expect_kv "$report" hyperparameter_search false; expect_kv "$report" retry false; expect_kv "$report" fits_completed 6
  expect_kv "$report" preregistered_strong_gate 'direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25'
  expect_kv "$report" arm_pass_rule validation_strong_gate_in_at_least_2_of_3_seeds
  for arm in raw_history_96 representation_raw96; do
    sum=0
    for seed in 31 47 73; do
      prefix="arm.${arm}.seed_${seed}"
      fingerprint="$(kv "$report" "${prefix}.schedule_fingerprint")"; [[ "$fingerprint" =~ ^[0-9a-f]{16}$ ]] || fail "invalid schedule fingerprint"
      expect_kv "$report" "${prefix}.optimizer_steps" 3500; finite_number "$(kv "$report" "${prefix}.last_loss")"; finite_number "$(kv "$report" "${prefix}.maximum_gradient_norm")"
      validate_metric "$report" "${prefix}.train" 22464 22464; validate_metric "$report" "${prefix}.validation" 2304 2304
      for channel in 0 1 2; do validate_metric "$report" "${prefix}.train.channel_${channel}" 7488 7488; validate_metric "$report" "${prefix}.validation.channel_${channel}" 768 768; done
      strong="$(computed_gate "$report" "${prefix}.validation")"; expect_kv "$report" "${prefix}.validation_strong_gate_pass" "$strong"
      expect_kv "$report" "${prefix}.validation_partial_gate_pass" "$(computed_partial_gate "$report" "${prefix}.validation")"
      [[ "$strong" == true ]] && sum=$((sum+1))
    done
    for seed in 31 47 73; do expect_kv "$report" "arm.${arm}.seed_${seed}.schedule_fingerprint" "$(kv "$report" "arm.raw_history_96.seed_${seed}.schedule_fingerprint")"; done
    validate_median_metric "$report" "arm.${arm}.median.train" 22464 22464 "arm.${arm}.seed_SEED.train"
    validate_median_metric "$report" "arm.${arm}.median.validation" 2304 2304 "arm.${arm}.seed_SEED.validation"
    for channel in 0 1 2; do
      validate_median_metric "$report" "arm.${arm}.median.train.channel_${channel}" 7488 7488 "arm.${arm}.seed_SEED.train.channel_${channel}"
      validate_median_metric "$report" "arm.${arm}.median.validation.channel_${channel}" 768 768 "arm.${arm}.seed_SEED.validation.channel_${channel}"
    done
    count="$(kv "$report" "arm.${arm}.strong_seed_count")"; [[ "$count" =~ ^[0-3]$ && "$count" == "$sum" ]] || fail "strong-seed count mismatch"
    pass=false; (( count >= 2 )) && pass=true; expect_kv "$report" "arm.${arm}.pass" "$pass"
  done
  raw_pass="$(kv "$report" arm.raw_history_96.pass)"; rep_pass="$(kv "$report" arm.representation_raw96.pass)"
  if [[ "$raw_pass" == true && "$rep_pass" == true ]]; then expected=nonlinear_decodability_established
  elif [[ "$raw_pass" == true ]]; then expected=information_not_established_at_frozen_raw96_interface
  elif [[ "$rep_pass" == true ]]; then expected=representation_decodable_raw_history_control_invalid
  else expected=inconclusive_both_mlp_arms_failed; fi
  expect_kv "$report" classification "$expected"
}

emit_stage() {
  local name state candidate
  [[ $# -ge 2 ]] || fail "emit_stage requires name and state"
  name="$1"
  state="$2"
  shift 2
  candidate="${SCRATCH}/${name}.${state}.$$"
  require_scratch_ready
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_stage_v1"; echo "status=${state}"; echo "protocol_id=${PROTOCOL_ID}"; echo "stage=${name}"
    for field in "$@"; do printf '%s\n' "$field"; done
  } > "$candidate"
  publish_receipt "$candidate" "${STAGES}/${name}.${state}.status"
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.status.$$"
  require_scratch_ready
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_attempt_v1"; echo "status=consumed"; echo "protocol_id=${PROTOCOL_ID}"; echo "attempt_ordinal=1"
    emit_stage_receipt_fix_lineage
    echo "preregistration_sha256=${PREREG_SHA}"; echo "corrected_control_preregistration_sha256=${CORRECTED_CONTROL_PREREG_SHA}"; echo "predecessor_preregistration_sha256=${PREDECESSOR_PREREG_SHA}"
    echo "self_test_root_cause_sha256=${SELF_TEST_ROOT_CAUSE_SHA}"; echo "root_cause_sha256=${ROOT_CAUSE_SHA}"
    echo "raw_control_predecessor_terminal_sha256=${RAW_CONTROL_PREDECESSOR_TERMINAL_SHA}"; echo "self_test_predecessor_terminal_sha256=${SELF_TEST_PREDECESSOR_TERMINAL_SHA}"; echo "evaluator_report_schema=${EVALUATOR_REPORT_SCHEMA}"
    echo "runner_sha256=$(sha256 "$RUNNER")"; echo "capture_source_sha256=${CAPTURE_SOURCE_SHA}"; echo "capture_build_script_sha256=${CAPTURE_BUILD_SCRIPT_SHA}"
    echo "capture_predecessor_source_sha256=${CAPTURE_PREDECESSOR_SOURCE_SHA}"; echo "capture_predecessor_build_script_sha256=${CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA}"
    echo "nonlinear_source_sha256=${NONLINEAR_SOURCE_SHA}"; echo "nonlinear_build_script_sha256=${NONLINEAR_BUILD_SCRIPT_SHA}"; echo "affine_source_sha256=${AFFINE_SOURCE_SHA}"
    echo "capture_build_receipt_sha256=$(sha256 "$CAPTURE_BUILD_RECEIPT")"; echo "nonlinear_build_receipt_sha256=$(sha256 "$NONLINEAR_BUILD_RECEIPT")"
    echo "self_test_receipt_sha256=$(sha256 "$SELF_TEST_RECEIPT")"; echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "capture_config_path=${CAPTURE_CONFIG}"; echo "capture_config_sha256=${CAPTURE_CONFIG_SHA}"
    echo "source_closure_path=${SOURCE_CLOSURE}"; echo "source_closure_sha256=${SOURCE_CLOSURE_SHA}"
    echo "cursor_erratum_path=${CURSOR_ERRATUM}"; echo "cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}"
    echo "source_manifest_path=${SOURCE_MANIFEST}"; echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "representation_train_path=${REPRESENTATION_TRAIN}"; echo "representation_train_sha256=${REPRESENTATION_TRAIN_SHA}"
    echo "representation_validation_path=${REPRESENTATION_VALIDATION}"; echo "representation_validation_sha256=${REPRESENTATION_VALIDATION_SHA}"
    echo "capture_binary_path=${CAPTURE_BIN}"; echo "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    echo "nonlinear_binary_path=${NONLINEAR_BIN}"; echo "nonlinear_binary_sha256=$(sha256 "$NONLINEAR_BIN")"
    echo "planned_raw_capture_invocations=2"; echo "planned_evaluator_invocations=1"; echo "planned_fits=6"
    echo "soft_timeout_seconds=${SOFT_TIMEOUT_SECONDS}"; echo "term_grace_seconds=${TERM_GRACE_SECONDS}"; echo "hard_timeout_seconds=${HARD_TIMEOUT_SECONDS}"
    echo "retry_allowed=false"; echo "resume_allowed=false"; echo "certified_input_access=false"; echo "final_holdout_access=false"; echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$ATTEMPT"
}

authorize_worker() {
  local token="${PHASE2B_CORRECTED_WORKER_TOKEN:-}" expected_identity="${PHASE2B_CORRECTED_WORKER_IDENTITY:-}"
  local observed extra actual_identity link_count parent_argv=()
  [[ "$token" =~ ^[0-9a-f]{64}$ && "$expected_identity" =~ ^[0-9]+:[0-9]+$ ]] || fail "private worker capability absent"
  [[ -e /proc/$$/fd/8 ]] || fail "worker did not inherit capability descriptor"
  actual_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/8)"
  link_count="$(stat -Lc '%h' -- /proc/$$/fd/8)"
  [[ "$actual_identity" == "$expected_identity" && "$link_count" == 0 ]] || fail "worker capability identity/lifetime mismatch"
  IFS= read -r observed <&8 || fail "worker capability is unreadable"
  if IFS= read -r extra <&8; then fail "worker capability has trailing records"; fi
  [[ "$observed" == "$token" ]] || fail "worker capability token mismatch"
  exec 8<&-
  unset PHASE2B_CORRECTED_WORKER_TOKEN PHASE2B_CORRECTED_WORKER_IDENTITY token observed extra
  mapfile -d '' -t parent_argv < "/proc/${PPID}/cmdline"
  [[ "${#parent_argv[@]}" == 6 && "${parent_argv[0]##*/}" == timeout &&
     "${parent_argv[1]}" == --signal=TERM && "${parent_argv[2]}" == "--kill-after=${TERM_GRACE_SECONDS}s" &&
     "${parent_argv[3]}" == "${SOFT_TIMEOUT_SECONDS}s" && "${parent_argv[4]}" == "$RUNNER" && "${parent_argv[5]}" == --worker ]] ||
    fail "worker is not supervised by the fixed bounded timeout"
  [[ -e /proc/$$/fd/9 ]] || fail "worker did not inherit execution lock"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/9)" == "$(stat -Lc '%d:%i' -- "$LOCK")" ]] || fail "worker lock identity mismatch"
  flock -n 9 || fail "worker does not own the inherited execution lock"
}

worker_scientific() {
  local rc
  emit_stage raw_train_capture started command_ordinal=1 maximum_anchor_read_upper_bound=2495
  if "$CAPTURE_BIN" --config "$CAPTURE_CONFIG" --output-probe "$RAW_TRAIN" --output-report "$RAW_TRAIN_REPORT" --anchor-index-begin 0 --anchor-index-end 2496 >"$LOGS/capture.train.log" 2>&1; then rc=0; else rc=$?; fi
  emit_stage raw_train_capture command_returned command_ordinal=1 child_exit_code="$rc"
  (( rc == 0 )) || return "$rc"
  validate_capture_report "$RAW_TRAIN_REPORT" "$RAW_TRAIN" train '[0,2496)' 2496 22464 2495
  validate_probe_identity "$REPRESENTATION_TRAIN" "$RAW_TRAIN" 22464
  chmod 0444 -- "$RAW_TRAIN" "$RAW_TRAIN_REPORT" "$LOGS/capture.train.log"
  emit_stage raw_train_capture complete command_ordinal=1 probe_sha256="$(sha256 "$RAW_TRAIN")" report_sha256="$(sha256 "$RAW_TRAIN_REPORT")" log_sha256="$(sha256 "$LOGS/capture.train.log")" maximum_anchor_read=2495
  emit_stage raw_validation_capture started command_ordinal=2 maximum_anchor_read_upper_bound=2815
  if "$CAPTURE_BIN" --config "$CAPTURE_CONFIG" --output-probe "$RAW_VALIDATION" --output-report "$RAW_VALIDATION_REPORT" --anchor-index-begin 2560 --anchor-index-end 2816 >"$LOGS/capture.validation.log" 2>&1; then rc=0; else rc=$?; fi
  emit_stage raw_validation_capture command_returned command_ordinal=2 child_exit_code="$rc"
  (( rc == 0 )) || return "$rc"
  validate_capture_report "$RAW_VALIDATION_REPORT" "$RAW_VALIDATION" validation '[2560,2816)' 256 2304 2815
  validate_probe_identity "$REPRESENTATION_VALIDATION" "$RAW_VALIDATION" 2304
  chmod 0444 -- "$RAW_VALIDATION" "$RAW_VALIDATION_REPORT" "$LOGS/capture.validation.log"
  emit_stage raw_validation_capture complete command_ordinal=2 probe_sha256="$(sha256 "$RAW_VALIDATION")" report_sha256="$(sha256 "$RAW_VALIDATION_REPORT")" log_sha256="$(sha256 "$LOGS/capture.validation.log")" maximum_anchor_read=2815
  emit_stage nonlinear_evaluator started command_ordinal=3 planned_fits=6 planned_optimizer_steps=21000
  if "$NONLINEAR_BIN" --development-only --representation-train-input "$REPRESENTATION_TRAIN" --representation-validation-input "$REPRESENTATION_VALIDATION" --raw-train-input "$RAW_TRAIN" --raw-validation-input "$RAW_VALIDATION" --output "$NONLINEAR_REPORT" >"$LOGS/nonlinear.log" 2>&1; then rc=0; else rc=$?; fi
  emit_stage nonlinear_evaluator command_returned command_ordinal=3 child_exit_code="$rc"
  (( rc == 0 )) || return "$rc"
  validate_nonlinear_report "$NONLINEAR_REPORT"
  chmod 0444 -- "$NONLINEAR_REPORT" "$LOGS/nonlinear.log"
  emit_stage nonlinear_evaluator complete command_ordinal=3 report_sha256="$(sha256 "$NONLINEAR_REPORT")" log_sha256="$(sha256 "$LOGS/nonlinear.log")" fits_completed=6 optimizer_steps=21000
}

worker() {
  local rc
  authorize_worker
  trap 'rc=$?; trap - EXIT HUP INT TERM; if [[ -f "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then seal_post_attempt_terminal "$rc"; fi; exit "$rc"' EXIT
  trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
  preflight_scientific_authority
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  verify_self_test_receipt
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] || fail "attempt/result/terminal already exists"
  require_scratch_ready
  for directory in "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$STAGES" "$LOGS"; do
    [[ ! -e "$directory" && ! -L "$directory" ]] || fail "non-pristine worker directory: ${directory}"
  done
  # Consume the sole attempt before the first persistent scientific directory.
  emit_attempt
  mkdir -m 0700 -- "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$STAGES" "$LOGS"
  for directory in "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$STAGES" "$LOGS"; do require_private_directory "$directory"; done
  worker_scientific >"$WORKER_LOG" 2>&1
  scan_processes
  chmod 0444 -- "$WORKER_LOG"; freeze_present_files
  validate_success_inputs
  emit_manifest; verify_manifest
  chmod 0555 -- "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$STAGES" "$LOGS" "$BUILD_DIR"
  # Defer external termination across the single atomic success commit.  All
  # scientific work, process scanning, sealing, and verification are complete.
  trap '' HUP INT TERM
  # RESULT is the literal final success commit.  No command follows it.
  emit_result
}

freeze_present_files() {
  local directory
  for directory in "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$STAGES" "$LOGS"; do
    [[ -d "$directory" ]] && find "$directory" -type f -exec chmod 0444 -- {} +
  done
}

stage_exists() { [[ -f "${STAGES}/$1.$2.status" ]]; }

seal_post_attempt_terminal() {
  local rc="$1" train_started=0 train_returned=0 train_done=0 validation_started=0 validation_returned=0 validation_done=0 eval_started=0 eval_returned=0 eval_done=0 eval_attested=0 stage ordinal max_anchor fits steps reason candidate
  [[ -f "$ATTEMPT" ]] || fail "cannot seal post-attempt terminal without attempt"
  [[ ! -e "$TERMINAL" && ! -e "$RESULT" ]] || fail "terminal/result already exists"
  stage_exists raw_train_capture started && train_started=1; stage_exists raw_train_capture command_returned && train_returned=1; stage_exists raw_train_capture complete && train_done=1
  stage_exists raw_validation_capture started && validation_started=1; stage_exists raw_validation_capture command_returned && validation_returned=1; stage_exists raw_validation_capture complete && validation_done=1
  stage_exists nonlinear_evaluator started && eval_started=1; stage_exists nonlinear_evaluator command_returned && eval_returned=1; stage_exists nonlinear_evaluator complete && eval_done=1
  if (( train_started == 0 )); then stage=post_attempt_setup; ordinal=0
  elif (( train_done == 0 )); then stage=raw_train_capture; ordinal=1
  elif (( validation_done == 0 )); then stage=raw_validation_capture; ordinal=2
  elif (( eval_done == 0 )); then stage=nonlinear_evaluator; ordinal=3
  else stage=success_sealing_or_verification; ordinal=4; fi
  if (( validation_started )); then max_anchor=2815; elif (( train_started )); then max_anchor=2495; else max_anchor=none; fi
  if (( eval_done )); then
    fits=6; steps=21000; eval_attested=1
  elif (( eval_returned )) && [[ -f "$NONLINEAR_REPORT" && ! -L "$NONLINEAR_REPORT" ]]; then
    set +e; (validate_nonlinear_report "$NONLINEAR_REPORT") >/dev/null 2>&1; eval_attested=$?; set -e
    if (( eval_attested == 0 )); then fits=6; steps=21000; eval_attested=1; else fits=unknown_after_returned_evaluator; steps=unknown_after_returned_evaluator; eval_attested=0; fi
  elif (( eval_started )); then fits=unknown_after_started_evaluator; steps=unknown_after_started_evaluator
  else fits=0; steps=0; fi
  reason=nonzero_exit
  [[ "$rc" == 124 || "$rc" == 137 || "$rc" == 143 ]] && reason=hard_timeout_or_forced_termination
  freeze_present_files
  require_scratch_ready
  candidate="${SCRATCH}/terminal.invalid.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_terminal_invalid_v1"; echo "status=terminal_invalid"; echo "protocol_id=${PROTOCOL_ID}"
    emit_stage_receipt_fix_lineage
    echo "diagnostic_authority=development_only"; echo "benchmark_acceptance_authority=false"; echo "classification=invalid_post_attempt_execution"
    echo "attempt_path=${ATTEMPT}"; echo "attempt_sha256=$(sha256 "$ATTEMPT")"; echo "failure_stage=${stage}"; echo "failure_command_ordinal=${ordinal}"; echo "failure_reason_code=${reason}"; echo "worker_exit_code=${rc}"
    echo "worker_log_sha256=$([[ -f "$WORKER_LOG" ]] && sha256 "$WORKER_LOG" || echo absent)"
    echo "raw_train_log_sha256=$([[ -f "$LOGS/capture.train.log" ]] && sha256 "$LOGS/capture.train.log" || echo absent)"
    echo "raw_validation_log_sha256=$([[ -f "$LOGS/capture.validation.log" ]] && sha256 "$LOGS/capture.validation.log" || echo absent)"
    echo "nonlinear_log_sha256=$([[ -f "$LOGS/nonlinear.log" ]] && sha256 "$LOGS/nonlinear.log" || echo absent)"
    echo "rejected_development_receipt_sha256=$([[ -f "$REJECTED_RESULT" && ! -L "$REJECTED_RESULT" ]] && sha256 "$REJECTED_RESULT" || echo absent)"
    echo "attempt_consumed=true"; echo "same_protocol_resume_allowed=false"; echo "same_protocol_retry_allowed=false"; echo "new_protocol_required=true"
    echo "actual_raw_capture_invocations_started=$((train_started+validation_started))"; echo "actual_raw_capture_invocations_completed=$((train_returned+validation_returned))"; echo "actual_raw_capture_invocations_validated=$((train_done+validation_done))"
    echo "evaluator_invocations_started=${eval_started}"; echo "evaluator_invocations_completed=${eval_returned}"; echo "evaluator_invocations_validated=${eval_done}"; echo "evaluator_report_attested=${eval_attested}"; echo "fits_completed=${fits}"; echo "optimizer_steps=${steps}"
    echo "maximum_anchor_read_upper_bound=${max_anchor}"; echo "scientific_result_available=false"
    echo "representation_execution=false"; echo "mdn_execution=false"; echo "checkpoint_written=false"; echo "certified_input_access=false"; echo "final_holdout_access=false"; echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$TERMINAL"
}

retire_invalid_result() {
  [[ -f "$RESULT" && ! -L "$RESULT" ]] || fail "invalid result is not a regular file"
  require_canonical_path "$RESULT"
  [[ ! -e "$REJECTED_RESULT" && ! -L "$REJECTED_RESULT" ]] || fail "rejected result path already exists"
  mv -T -n -- "$RESULT" "$REJECTED_RESULT" || fail "could not retire invalid result"
  [[ ! -e "$RESULT" && -f "$REJECTED_RESULT" && ! -L "$REJECTED_RESULT" ]] || fail "invalid result retirement was incomplete"
  chmod 0444 -- "$REJECTED_RESULT"
  require_frozen_metadata "$REJECTED_RESULT"
}

emit_manifest() {
  local candidate="${SCRATCH}/artifact.manifest.status.$$"
  require_scratch_ready
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_manifest_v1"; echo "status=complete"; echo "protocol_id=${PROTOCOL_ID}"
    for binding in "attempt:${ATTEMPT}" "self_test_receipt:${SELF_TEST_RECEIPT}" "capture_build_receipt:${CAPTURE_BUILD_RECEIPT}" "nonlinear_build_receipt:${NONLINEAR_BUILD_RECEIPT}" "raw_train_probe:${RAW_TRAIN}" "raw_train_report:${RAW_TRAIN_REPORT}" "raw_validation_probe:${RAW_VALIDATION}" "raw_validation_report:${RAW_VALIDATION_REPORT}" "nonlinear_report:${NONLINEAR_REPORT}" "raw_train_started_stage:${STAGES}/raw_train_capture.started.status" "raw_train_returned_stage:${STAGES}/raw_train_capture.command_returned.status" "raw_train_stage:${STAGES}/raw_train_capture.complete.status" "raw_validation_started_stage:${STAGES}/raw_validation_capture.started.status" "raw_validation_returned_stage:${STAGES}/raw_validation_capture.command_returned.status" "raw_validation_stage:${STAGES}/raw_validation_capture.complete.status" "nonlinear_started_stage:${STAGES}/nonlinear_evaluator.started.status" "nonlinear_returned_stage:${STAGES}/nonlinear_evaluator.command_returned.status" "nonlinear_stage:${STAGES}/nonlinear_evaluator.complete.status" "worker_log:${WORKER_LOG}" "raw_train_log:${LOGS}/capture.train.log" "raw_validation_log:${LOGS}/capture.validation.log" "nonlinear_log:${LOGS}/nonlinear.log"; do
      name="${binding%%:*}"; path="${binding#*:}"; require_frozen_metadata "$path"; echo "${name}_path=${path}"; echo "${name}_sha256=$(sha256 "$path")"
    done
  } > "$candidate"
  publish_receipt "$candidate" "$MANIFEST"
}

render_result() {
  local destination="$1"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_receipt_v1"; echo "status=complete"; echo "protocol_id=${PROTOCOL_ID}"
    emit_stage_receipt_fix_lineage
    echo "benchmark_id=synthetic_continuous_graph_v2"; echo "diagnostic_phase=2B"; echo "diagnostic_authority=development_only"; echo "benchmark_acceptance_authority=false"
    echo "classification=$(kv "$NONLINEAR_REPORT" classification)"; echo "preregistration_sha256=${PREREG_SHA}"; echo "corrected_control_preregistration_sha256=${CORRECTED_CONTROL_PREREG_SHA}"; echo "predecessor_preregistration_sha256=${PREDECESSOR_PREREG_SHA}"
    echo "self_test_root_cause_sha256=${SELF_TEST_ROOT_CAUSE_SHA}"; echo "root_cause_sha256=${ROOT_CAUSE_SHA}"
    echo "raw_control_predecessor_terminal_sha256=${RAW_CONTROL_PREDECESSOR_TERMINAL_SHA}"; echo "self_test_predecessor_terminal_sha256=${SELF_TEST_PREDECESSOR_TERMINAL_SHA}"; echo "evaluator_report_schema=${EVALUATOR_REPORT_SCHEMA}"
    echo "runner_path=${RUNNER}"; echo "runner_sha256=$(sha256 "$RUNNER")"; echo "attempt_path=${ATTEMPT}"; echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "self_test_receipt_sha256=$(sha256 "$SELF_TEST_RECEIPT")"; echo "artifact_manifest_path=${MANIFEST}"; echo "artifact_manifest_sha256=$(sha256 "$MANIFEST")"
    echo "raw_capture_invocations=2"; echo "nonlinear_evaluator_invocations=1"; echo "fits_completed=6"; echo "optimizer_steps=21000"; echo "soft_timeout_seconds=${SOFT_TIMEOUT_SECONDS}"; echo "term_grace_seconds=${TERM_GRACE_SECONDS}"; echo "hard_timeout_seconds=${HARD_TIMEOUT_SECONDS}"
    echo "coordinate_target_identity_verified=true"; echo "maximum_anchor_read=2815"; echo "representation_execution=false"; echo "mdn_execution=false"; echo "checkpoint_written=false"
    echo "certified_input_access=false"; echo "final_holdout_access=false"; echo "policy_access=false"; echo "retry_allowed=false"; echo "resume_allowed=false"
  } > "$destination"
}

emit_result() {
  local candidate="${SCRATCH}/development.status.$$"
  require_scratch_ready
  render_result "$candidate"
  validate_result_receipt "$candidate"
  publish_receipt "$candidate" "$RESULT"
}

validate_stage_receipt() {
  local path="$1" stage="$2" state="$3" ordinal="$4"
  require_frozen_metadata "$path"
  expect_kv "$path" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_stage_v1
  expect_kv "$path" status "$state"
  expect_kv "$path" protocol_id "$PROTOCOL_ID"
  expect_kv "$path" stage "$stage"
  expect_kv "$path" command_ordinal "$ordinal"
  case "$state" in
    started)
      case "$stage" in
        raw_train_capture) expect_kv "$path" maximum_anchor_read_upper_bound 2495 ;;
        raw_validation_capture) expect_kv "$path" maximum_anchor_read_upper_bound 2815 ;;
        nonlinear_evaluator)
          expect_kv "$path" planned_fits 6
          expect_kv "$path" planned_optimizer_steps 21000
          ;;
      esac
      ;;
    command_returned) expect_kv "$path" child_exit_code 0 ;;
    complete)
      case "$stage" in
        raw_train_capture)
          expect_kv "$path" probe_sha256 "$(sha256 "$RAW_TRAIN")"
          expect_kv "$path" report_sha256 "$(sha256 "$RAW_TRAIN_REPORT")"
          expect_kv "$path" log_sha256 "$(sha256 "$LOGS/capture.train.log")"
          expect_kv "$path" maximum_anchor_read 2495
          ;;
        raw_validation_capture)
          expect_kv "$path" probe_sha256 "$(sha256 "$RAW_VALIDATION")"
          expect_kv "$path" report_sha256 "$(sha256 "$RAW_VALIDATION_REPORT")"
          expect_kv "$path" log_sha256 "$(sha256 "$LOGS/capture.validation.log")"
          expect_kv "$path" maximum_anchor_read 2815
          ;;
        nonlinear_evaluator)
          expect_kv "$path" report_sha256 "$(sha256 "$NONLINEAR_REPORT")"
          expect_kv "$path" log_sha256 "$(sha256 "$LOGS/nonlinear.log")"
          expect_kv "$path" fits_completed 6
          expect_kv "$path" optimizer_steps 21000
          ;;
      esac
      ;;
  esac
}

verify_manifest() {
  local binding name path expected_path
  require_frozen_metadata "$MANIFEST"
  expect_kv "$MANIFEST" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_manifest_v1
  expect_kv "$MANIFEST" status complete
  expect_kv "$MANIFEST" protocol_id "$PROTOCOL_ID"
  for binding in \
    "attempt:${ATTEMPT}" \
    "self_test_receipt:${SELF_TEST_RECEIPT}" \
    "capture_build_receipt:${CAPTURE_BUILD_RECEIPT}" \
    "nonlinear_build_receipt:${NONLINEAR_BUILD_RECEIPT}" \
    "raw_train_probe:${RAW_TRAIN}" \
    "raw_train_report:${RAW_TRAIN_REPORT}" \
    "raw_validation_probe:${RAW_VALIDATION}" \
    "raw_validation_report:${RAW_VALIDATION_REPORT}" \
    "nonlinear_report:${NONLINEAR_REPORT}" \
    "raw_train_started_stage:${STAGES}/raw_train_capture.started.status" \
    "raw_train_returned_stage:${STAGES}/raw_train_capture.command_returned.status" \
    "raw_train_stage:${STAGES}/raw_train_capture.complete.status" \
    "raw_validation_started_stage:${STAGES}/raw_validation_capture.started.status" \
    "raw_validation_returned_stage:${STAGES}/raw_validation_capture.command_returned.status" \
    "raw_validation_stage:${STAGES}/raw_validation_capture.complete.status" \
    "nonlinear_started_stage:${STAGES}/nonlinear_evaluator.started.status" \
    "nonlinear_returned_stage:${STAGES}/nonlinear_evaluator.command_returned.status" \
    "nonlinear_stage:${STAGES}/nonlinear_evaluator.complete.status" \
    "worker_log:${WORKER_LOG}" \
    "raw_train_log:${LOGS}/capture.train.log" \
    "raw_validation_log:${LOGS}/capture.validation.log" \
    "nonlinear_log:${LOGS}/nonlinear.log"; do
    name="${binding%%:*}"
    expected_path="${binding#*:}"
    path="$(kv "$MANIFEST" "${name}_path")"
    [[ "$path" == "$expected_path" ]] || fail "manifest path mismatch: ${name}"
    require_frozen_metadata "$path"
    [[ "$(sha256 "$path")" == "$(kv "$MANIFEST" "${name}_sha256")" ]] || fail "manifest hash mismatch: ${name}"
  done
  validate_stage_receipt "$STAGES/raw_train_capture.started.status" raw_train_capture started 1
  validate_stage_receipt "$STAGES/raw_train_capture.command_returned.status" raw_train_capture command_returned 1
  validate_stage_receipt "$STAGES/raw_train_capture.complete.status" raw_train_capture complete 1
  validate_stage_receipt "$STAGES/raw_validation_capture.started.status" raw_validation_capture started 2
  validate_stage_receipt "$STAGES/raw_validation_capture.command_returned.status" raw_validation_capture command_returned 2
  validate_stage_receipt "$STAGES/raw_validation_capture.complete.status" raw_validation_capture complete 2
  validate_stage_receipt "$STAGES/nonlinear_evaluator.started.status" nonlinear_evaluator started 3
  validate_stage_receipt "$STAGES/nonlinear_evaluator.command_returned.status" nonlinear_evaluator command_returned 3
  validate_stage_receipt "$STAGES/nonlinear_evaluator.complete.status" nonlinear_evaluator complete 3
}

validate_result_receipt() {
  local receipt="$1"
  expect_kv "$receipt" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_receipt_v1
  expect_kv "$receipt" status complete; expect_kv "$receipt" protocol_id "$PROTOCOL_ID"
  verify_stage_receipt_fix_lineage_receipt "$receipt"
  expect_kv "$receipt" benchmark_id synthetic_continuous_graph_v2; expect_kv "$receipt" diagnostic_phase 2B
  expect_kv "$receipt" diagnostic_authority development_only; expect_kv "$receipt" benchmark_acceptance_authority false
  expect_kv "$receipt" preregistration_sha256 "$PREREG_SHA"; expect_kv "$receipt" corrected_control_preregistration_sha256 "$CORRECTED_CONTROL_PREREG_SHA"; expect_kv "$receipt" predecessor_preregistration_sha256 "$PREDECESSOR_PREREG_SHA"
  expect_kv "$receipt" self_test_root_cause_sha256 "$SELF_TEST_ROOT_CAUSE_SHA"; expect_kv "$receipt" root_cause_sha256 "$ROOT_CAUSE_SHA"
  expect_kv "$receipt" raw_control_predecessor_terminal_sha256 "$RAW_CONTROL_PREDECESSOR_TERMINAL_SHA"; expect_kv "$receipt" self_test_predecessor_terminal_sha256 "$SELF_TEST_PREDECESSOR_TERMINAL_SHA"; expect_kv "$receipt" evaluator_report_schema "$EVALUATOR_REPORT_SCHEMA"
  expect_kv "$receipt" runner_path "$RUNNER"; expect_kv "$receipt" runner_sha256 "$(sha256 "$RUNNER")"
  expect_kv "$receipt" attempt_path "$ATTEMPT"; expect_kv "$receipt" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect_kv "$receipt" self_test_receipt_sha256 "$(sha256 "$SELF_TEST_RECEIPT")"
  expect_kv "$receipt" artifact_manifest_path "$MANIFEST"; expect_kv "$receipt" artifact_manifest_sha256 "$(sha256 "$MANIFEST")"
  expect_kv "$receipt" raw_capture_invocations 2; expect_kv "$receipt" nonlinear_evaluator_invocations 1
  expect_kv "$receipt" fits_completed 6; expect_kv "$receipt" optimizer_steps 21000
  expect_kv "$receipt" soft_timeout_seconds "$SOFT_TIMEOUT_SECONDS"; expect_kv "$receipt" term_grace_seconds "$TERM_GRACE_SECONDS"; expect_kv "$receipt" hard_timeout_seconds "$HARD_TIMEOUT_SECONDS"
  expect_kv "$receipt" coordinate_target_identity_verified true; expect_kv "$receipt" maximum_anchor_read 2815
  expect_kv "$receipt" representation_execution false; expect_kv "$receipt" mdn_execution false; expect_kv "$receipt" checkpoint_written false
  expect_kv "$receipt" certified_input_access false; expect_kv "$receipt" final_holdout_access false; expect_kv "$receipt" policy_access false
  expect_kv "$receipt" retry_allowed false; expect_kv "$receipt" resume_allowed false
  expect_kv "$receipt" classification "$(kv "$NONLINEAR_REPORT" classification)"
}

validate_success_inputs() {
  require_frozen_metadata "$ATTEMPT"
  expect_kv "$ATTEMPT" schema_id synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_attempt_v1
  expect_kv "$ATTEMPT" status consumed; expect_kv "$ATTEMPT" protocol_id "$PROTOCOL_ID"; expect_kv "$ATTEMPT" attempt_ordinal 1
  verify_stage_receipt_fix_lineage_receipt "$ATTEMPT"
  expect_kv "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"; expect_kv "$ATTEMPT" corrected_control_preregistration_sha256 "$CORRECTED_CONTROL_PREREG_SHA"; expect_kv "$ATTEMPT" predecessor_preregistration_sha256 "$PREDECESSOR_PREREG_SHA"
  expect_kv "$ATTEMPT" self_test_root_cause_sha256 "$SELF_TEST_ROOT_CAUSE_SHA"; expect_kv "$ATTEMPT" root_cause_sha256 "$ROOT_CAUSE_SHA"
  expect_kv "$ATTEMPT" raw_control_predecessor_terminal_sha256 "$RAW_CONTROL_PREDECESSOR_TERMINAL_SHA"; expect_kv "$ATTEMPT" self_test_predecessor_terminal_sha256 "$SELF_TEST_PREDECESSOR_TERMINAL_SHA"; expect_kv "$ATTEMPT" evaluator_report_schema "$EVALUATOR_REPORT_SCHEMA"
  expect_kv "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect_kv "$ATTEMPT" capture_source_sha256 "$CAPTURE_SOURCE_SHA"; expect_kv "$ATTEMPT" capture_build_script_sha256 "$CAPTURE_BUILD_SCRIPT_SHA"
  expect_kv "$ATTEMPT" capture_predecessor_source_sha256 "$CAPTURE_PREDECESSOR_SOURCE_SHA"; expect_kv "$ATTEMPT" capture_predecessor_build_script_sha256 "$CAPTURE_PREDECESSOR_BUILD_SCRIPT_SHA"
  expect_kv "$ATTEMPT" nonlinear_source_sha256 "$NONLINEAR_SOURCE_SHA"; expect_kv "$ATTEMPT" nonlinear_build_script_sha256 "$NONLINEAR_BUILD_SCRIPT_SHA"; expect_kv "$ATTEMPT" affine_source_sha256 "$AFFINE_SOURCE_SHA"
  expect_kv "$ATTEMPT" capture_build_receipt_sha256 "$(sha256 "$CAPTURE_BUILD_RECEIPT")"; expect_kv "$ATTEMPT" nonlinear_build_receipt_sha256 "$(sha256 "$NONLINEAR_BUILD_RECEIPT")"
  expect_kv "$ATTEMPT" self_test_receipt_sha256 "$(sha256 "$SELF_TEST_RECEIPT")"; expect_kv "$ATTEMPT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect_kv "$ATTEMPT" capture_config_path "$CAPTURE_CONFIG"; expect_kv "$ATTEMPT" capture_config_sha256 "$CAPTURE_CONFIG_SHA"
  expect_kv "$ATTEMPT" source_closure_path "$SOURCE_CLOSURE"; expect_kv "$ATTEMPT" source_closure_sha256 "$SOURCE_CLOSURE_SHA"
  expect_kv "$ATTEMPT" cursor_erratum_path "$CURSOR_ERRATUM"; expect_kv "$ATTEMPT" cursor_erratum_sha256 "$CURSOR_ERRATUM_SHA"
  expect_kv "$ATTEMPT" source_manifest_path "$SOURCE_MANIFEST"; expect_kv "$ATTEMPT" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$ATTEMPT" representation_train_path "$REPRESENTATION_TRAIN"; expect_kv "$ATTEMPT" representation_train_sha256 "$REPRESENTATION_TRAIN_SHA"
  expect_kv "$ATTEMPT" representation_validation_path "$REPRESENTATION_VALIDATION"; expect_kv "$ATTEMPT" representation_validation_sha256 "$REPRESENTATION_VALIDATION_SHA"
  expect_kv "$ATTEMPT" capture_binary_path "$CAPTURE_BIN"; expect_kv "$ATTEMPT" capture_binary_sha256 "$(sha256 "$CAPTURE_BIN")"
  expect_kv "$ATTEMPT" nonlinear_binary_path "$NONLINEAR_BIN"; expect_kv "$ATTEMPT" nonlinear_binary_sha256 "$(sha256 "$NONLINEAR_BIN")"
  expect_kv "$ATTEMPT" planned_raw_capture_invocations 2; expect_kv "$ATTEMPT" planned_evaluator_invocations 1; expect_kv "$ATTEMPT" planned_fits 6
  expect_kv "$ATTEMPT" soft_timeout_seconds "$SOFT_TIMEOUT_SECONDS"; expect_kv "$ATTEMPT" term_grace_seconds "$TERM_GRACE_SECONDS"; expect_kv "$ATTEMPT" hard_timeout_seconds "$HARD_TIMEOUT_SECONDS"
  expect_kv "$ATTEMPT" retry_allowed false; expect_kv "$ATTEMPT" resume_allowed false
  expect_kv "$ATTEMPT" certified_input_access false; expect_kv "$ATTEMPT" final_holdout_access false; expect_kv "$ATTEMPT" policy_access false
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  verify_self_test_receipt
  validate_capture_report "$RAW_TRAIN_REPORT" "$RAW_TRAIN" train '[0,2496)' 2496 22464 2495
  validate_capture_report "$RAW_VALIDATION_REPORT" "$RAW_VALIDATION" validation '[2560,2816)' 256 2304 2815
  validate_probe_identity "$REPRESENTATION_TRAIN" "$RAW_TRAIN" 22464
  validate_probe_identity "$REPRESENTATION_VALIDATION" "$RAW_VALIDATION" 2304
  validate_nonlinear_report "$NONLINEAR_REPORT"
}

verify_result() {
  [[ ! -e "$TERMINAL" ]] || fail "success and terminal receipts coexist"
  verify_self_test_receipt; preflight_scientific_authority; require_frozen_metadata "$RESULT"
  validate_success_inputs; verify_manifest
  validate_result_receipt "$RESULT"
}

scan_processes() {
  local p pid cmd
  for p in /proc/[0-9]*; do
    pid="${p##*/}"; [[ "$pid" == "$$" || "$pid" == "$PPID" || ! -r "$p/cmdline" ]] && continue
    cmd="$(tr '\0' ' ' < "$p/cmdline" 2>/dev/null || true)"
    [[ "$cmd" != *raw_nodelift_edge_feature_probe_capture_corrected_control* && "$cmd" != *matched_nonlinear_sufficiency_corrected_control_probe* ]] || fail "corrected-control process remains active: pid=${pid}"
  done
}

run_development_signal() {
  local rc="$1"
  RUN_DEVELOPMENT_PENDING_SIGNAL="$rc"
  if [[ -z "$RUN_DEVELOPMENT_TIMEOUT_PID" ]]; then
    (( RUN_DEVELOPMENT_LAUNCHING == 1 )) && return 0
    exit "$rc"
  fi
  trap - HUP INT TERM
  kill -TERM -- "-$RUN_DEVELOPMENT_TIMEOUT_PID" 2>/dev/null || kill -TERM -- "$RUN_DEVELOPMENT_TIMEOUT_PID" 2>/dev/null || true
  wait "$RUN_DEVELOPMENT_TIMEOUT_PID" 2>/dev/null || true
  RUN_DEVELOPMENT_TIMEOUT_PID=""
  exit "$rc"
}

run_development_exit_guard() {
  local rc="$1"
  trap - EXIT HUP INT TERM
  exec 8<&- 2>/dev/null || true
  if [[ -n "$RUN_DEVELOPMENT_CAPABILITY" ]]; then
    rm -f -- "$RUN_DEVELOPMENT_CAPABILITY" 2>/dev/null || true
    RUN_DEVELOPMENT_CAPABILITY=""
  fi
  if [[ -f "$ATTEMPT" && -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    retire_invalid_result
  fi
  if [[ -f "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_post_attempt_terminal "$rc"
  fi
  exit "$rc"
}

run_development() {
  local path name token capability capability_identity rc
  preflight_build_authority; open_lock
  [[ ! -e "$TERMINAL" ]] || fail "corrected-control protocol is terminally invalid"
  if [[ -e "$RESULT" ]]; then verify_result; echo "[clear-signal:phase2b-corrected-stage-receipt-fix] existing result verified"; return; fi
  [[ ! -e "$ATTEMPT" ]] || fail "the only corrected-control attempt is consumed; retry/resume forbidden"
  verify_self_test_receipt
  preflight_scientific_authority
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"; verify_self_test_receipt
  for path in capture nonlinear stages logs; do [[ ! -e "$RUNTIME_ROOT/$path" && ! -L "$RUNTIME_ROOT/$path" ]] || fail "non-pristine preattempt path: ${path}"; done
  while IFS= read -r path; do name="${path##*/}"; case "$name" in .execution.lock|.scratch|build|self_test) ;; *) fail "unexpected preattempt entry: ${name}";; esac; done < <(find "$RUNTIME_ROOT" -mindepth 1 -maxdepth 1 -print)
  require_scratch_ready
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch is not pristine"
  trap 'run_development_exit_guard "$?"' EXIT
  trap 'run_development_signal 129' HUP
  trap 'run_development_signal 130' INT
  trap 'run_development_signal 143' TERM
  token="$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')"
  capability="${SCRATCH}/worker.capability.$$"
  RUN_DEVELOPMENT_CAPABILITY="$capability"
  (set -o noclobber; printf '%s\n' "$token" > "$capability") 2>/dev/null || fail "could not create private worker capability"
  chmod 0400 -- "$capability"; require_canonical_path "$capability"
  [[ -f "$capability" && ! -L "$capability" && "$(stat -c '%h' -- "$capability")" == 1 ]] || fail "invalid private worker capability"
  exec 8< "$capability"; capability_identity="$(stat -Lc '%d:%i' -- "$capability")"; rm -f -- "$capability"
  RUN_DEVELOPMENT_CAPABILITY=""
  [[ ! -e "$capability" && "$(stat -Lc '%h' -- /proc/$$/fd/8)" == 0 ]] || fail "private worker capability was not unlinked"
  set +e
  RUN_DEVELOPMENT_LAUNCHING=1
  PHASE2B_CORRECTED_WORKER_TOKEN="$token" PHASE2B_CORRECTED_WORKER_IDENTITY="$capability_identity" \
    setsid timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" "${SOFT_TIMEOUT_SECONDS}s" "$RUNNER" --worker &
  RUN_DEVELOPMENT_TIMEOUT_PID=$!
  RUN_DEVELOPMENT_LAUNCHING=0
  if [[ -n "$RUN_DEVELOPMENT_PENDING_SIGNAL" ]]; then
    run_development_signal "$RUN_DEVELOPMENT_PENDING_SIGNAL"
  fi
  wait "$RUN_DEVELOPMENT_TIMEOUT_PID"
  rc=$?
  RUN_DEVELOPMENT_TIMEOUT_PID=""
  set -e
  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  exec 8<&-; unset token capability_identity
  if (( rc != 0 )); then
    if [[ -f "$RESULT" && ! -e "$TERMINAL" ]]; then
      if timeout --signal=TERM --kill-after="${RESULT_VERIFY_GRACE_SECONDS}s" "${RESULT_VERIFY_TIMEOUT_SECONDS}s" "$RUNNER" --verify-development; then
        trap - EXIT HUP INT TERM
        echo "[clear-signal:phase2b-corrected-stage-receipt-fix] complete: ${RESULT}"
        return
      fi
      retire_invalid_result
      seal_post_attempt_terminal 1
      trap - EXIT HUP INT TERM
      fail "committed corrected-control result failed verification"
    fi
    if [[ -e "$TERMINAL" ]]; then trap - EXIT HUP INT TERM; fail "terminal corrected-control attempt failed (${rc}); retry forbidden"; fi
    if [[ -e "$ATTEMPT" && ! -e "$RESULT" ]]; then seal_post_attempt_terminal "$rc"; trap - EXIT HUP INT TERM; fail "terminal corrected-control attempt failed (${rc}); retry forbidden"; fi
    trap - EXIT HUP INT TERM
    fail "worker preflight failed (${rc}); attempt remains unconsumed"
  fi
  if [[ ! -f "$RESULT" || -e "$TERMINAL" ]]; then
    [[ ! -e "$ATTEMPT" || -e "$TERMINAL" ]] || seal_post_attempt_terminal 1
    trap - EXIT HUP INT TERM
    fail "timed worker returned without a unique success receipt"
  fi
  timeout --signal=TERM --kill-after="${RESULT_VERIFY_GRACE_SECONDS}s" "${RESULT_VERIFY_TIMEOUT_SECONDS}s" "$RUNNER" --verify-development
  trap - EXIT HUP INT TERM
  echo "[clear-signal:phase2b-corrected-stage-receipt-fix] complete: ${RESULT}"
}

plan() {
  preflight_build_authority
  echo "Project Clear Signal Phase 2B — corrected-control matched nonlinear sufficiency"
  echo "protocol_id=${PROTOCOL_ID}"; echo "runtime_root=${RUNTIME_ROOT}"; echo "scope=development_only"; echo "attempt_ordinal=1"
  echo "predecessor_terminals_verified=3"; echo "evaluator_report_schema=${EVALUATOR_REPORT_SCHEMA}"; echo "raw_capture_invocations=2"; echo "nonlinear_evaluator_invocations=1"; echo "nonlinear_fit_count=6"
  echo "train_range=[0,2496)"; echo "validation_range=[2560,2816)"; echo "maximum_anchor_read=2815"; echo "soft_timeout_seconds=${SOFT_TIMEOUT_SECONDS}"; echo "term_grace_seconds=${TERM_GRACE_SECONDS}"; echo "hard_timeout_seconds=${HARD_TIMEOUT_SECONDS}"
  echo "retry_allowed=false"; echo "resume_allowed=false"; echo "certified_input_access=false"; echo "final_holdout_access=false"; echo "policy_access=false"
  echo "builds_prepared=$([[ -e "$CAPTURE_BUILD_RECEIPT" && -e "$NONLINEAR_BUILD_RECEIPT" ]] && echo true || echo false)"
  echo "self_test_passed=$([[ -e "$SELF_TEST_RECEIPT" ]] && echo true || echo false)"; echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"; echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"; echo "terminal_invalid=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
}

main() {
  [[ $# == 1 ]] || fail "usage: $0 --plan|--prepare|--self-test|--run-development|--verify-development"
  case "$1" in
    --plan) plan;; --prepare) prepare;; --self-test) self_test;; --run-development) run_development;;
    --verify-development) [[ -e "$RESULT" ]] || fail "result absent"; verify_result; echo "[clear-signal:phase2b-corrected-stage-receipt-fix] result verified";;
    --worker) worker;; *) fail "unsupported mode: $1";;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then main "$@"; fi
