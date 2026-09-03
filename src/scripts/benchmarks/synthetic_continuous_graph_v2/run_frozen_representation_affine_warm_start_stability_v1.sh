#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_frozen_representation_affine_warm_start_stability_development_v1"
readonly REPORT_SCHEMA="$PROTOCOL_ID"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_WARM_START_STABILITY_PREREGISTRATION.md"
readonly PREREG_SHA="15ecb55515c2293b9e864d43cce4009f802399613921495066b2c17f2956b4dd"
readonly SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_warm_start_stability_probe.cpp"
readonly SOURCE_SHA="dcc6112c7920092cca1e36e24afe33fb4e9393325437a67942016052ad32296d"
readonly BUILD_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_representation_affine_warm_start_stability_probe.sh"
readonly BUILD_WRAPPER_SHA="43440ad66bf480e281354c80bdd536babe436e0ea14c2d54be99616bac92a89b"

readonly TRAIN="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe"
readonly TRAIN_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly VALIDATION="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe"
readonly VALIDATION_SHA="8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd"
readonly CAPTURE_LOCK="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/.execution.lock"

readonly PHASE2A_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1"
readonly PHASE2A_RECEIPT="${PHASE2A_ROOT}/development.status"
readonly PHASE2A_RECEIPT_SHA="b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5"
readonly PHASE2A_MAIN="${PHASE2A_ROOT}/main/development.report"
readonly PHASE2A_REPLAY="${PHASE2A_ROOT}/replay/development.report"
readonly PHASE2A_REPORT_SHA="2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a"

readonly PHASE2B_RESULT_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_v1"
readonly PHASE2B_RESULT_LOCK="${PHASE2B_RESULT_ROOT}/.execution.lock"
readonly PHASE2B_RESULT="${PHASE2B_RESULT_ROOT}/development.status"
readonly PHASE2B_RESULT_SHA="cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418"
readonly PHASE2B_REPORT_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1"
readonly PHASE2B_REPORT_LOCK="${PHASE2B_REPORT_ROOT}/.execution.lock"
readonly PHASE2B_REPORT="${PHASE2B_REPORT_ROOT}/nonlinear/development.report"
readonly PHASE2B_REPORT_SHA="34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36"

readonly LOCALIZATION_SOURCE_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1"
readonly LOCALIZATION_SOURCE_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${LOCALIZATION_SOURCE_ID}"
readonly LOCALIZATION_SOURCE_LOCK="${LOCALIZATION_SOURCE_ROOT}/.execution.lock"
readonly LOCALIZATION_SOURCE_REPORT="${LOCALIZATION_SOURCE_ROOT}/rejected.development.report"
readonly LOCALIZATION_SOURCE_REPORT_SHA="5b1ebcc7af65792074e653406a1a6f4120dc9ad4105adca5cbca90f6c5815f30"
readonly LOCALIZATION_SOURCE_EVALUATOR="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_injection_optimizer_localization_probe.cpp"
readonly LOCALIZATION_SOURCE_EVALUATOR_SHA="7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b"
readonly LOCALIZATION_SOURCE_BUILD_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_representation_affine_injection_optimizer_localization_probe.sh"
readonly LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA="2e5b39981302d55f8785389c4b01cb6dd4b38036d6d45ca10a798c69567004fd"
readonly CANONICAL_AFFINE_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp"
readonly CANONICAL_AFFINE_SOURCE_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly PHASE2A_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_encoder_channel_conditioned_affine_probe.cpp"
readonly PHASE2A_SOURCE_SHA="5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570"

readonly LOCALIZATION_V3_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v3"
readonly LOCALIZATION_V3_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${LOCALIZATION_V3_ID}"
readonly LOCALIZATION_V3_LOCK="${LOCALIZATION_V3_ROOT}/.execution.lock"
readonly LOCALIZATION_V3_RESULT="${LOCALIZATION_V3_ROOT}/development.status"
readonly LOCALIZATION_V3_RESULT_SHA="ad9be3c76c69bafaa6e42ce63e1e10b05231878f87b465ffd8b56b8156237ad6"
readonly LOCALIZATION_V3_VERIFICATION="${LOCALIZATION_V3_ROOT}/verification.complete.status"
readonly LOCALIZATION_V3_VERIFICATION_SHA="edcda0efcfe00fb6f53fa005040e4a17c8d8c5a50b4c97656dbe2b4516ee665d"
readonly LOCALIZATION_V3_SELF_TEST="${LOCALIZATION_V3_ROOT}/validator.syntax_self_test.status"
readonly LOCALIZATION_V3_SELF_TEST_SHA="c0d29f26643f3d6f34c08820eaac88da67355fd305eb80b17398679ea834bb53"
readonly LOCALIZATION_V3_ATTEMPT="${LOCALIZATION_V3_ROOT}/verification.attempt.status"
readonly LOCALIZATION_V3_ATTEMPT_SHA="aac5242963fa6071a34385531b9de9192704e13f7423b441344337ba28ba3d7f"
readonly LOCALIZATION_V3_LOG="${LOCALIZATION_V3_ROOT}/verification.log"
readonly LOCALIZATION_V3_LOG_SHA="e8381a5a21e365aded459267fc7ea149bbb2a4f9bd778331b0f43efe713f7f77"
readonly LOCALIZATION_V3_RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_frozen_representation_affine_injection_optimizer_localization_verification_recovery_v3.sh"
readonly LOCALIZATION_V3_RUNNER_SHA="b951630c11299769c1cab41d8ba3eef53356845a83d542e73c706bfd778ea1bf"
readonly LOCALIZATION_V3_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_VERIFICATION_RECOVERY_V3_PREREGISTRATION.md"
readonly LOCALIZATION_V3_PREREG_SHA="f39567b505757d75be814fe2392ee12ea0618f615c28ecddea518e7fd0ffedce"
readonly LOCALIZATION_V3_VALIDATOR="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_injection_optimizer_localization_verification_recovery_v3_validator.awk"
readonly LOCALIZATION_V3_VALIDATOR_SHA="a93a4c6315c0c89f6b747490d3cb6112dbf506675ad188ed656d10693dbedbb2"

readonly RUNTIME="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly BUILD_DIR="${RUNTIME}/build"
readonly SCRATCH="${RUNTIME}/.scratch"
readonly LOCK="${RUNTIME}/.execution.lock"
readonly BIN="${BUILD_DIR}/frozen_representation_affine_warm_start_stability"
readonly BUILD_RECEIPT="${BUILD_DIR}/build.status"
readonly ATTEMPT="${RUNTIME}/attempt.status"
readonly EVALUATOR_STARTED_RECEIPT="${RUNTIME}/evaluator.started.status"
readonly EVALUATOR_RETURNED_RECEIPT="${RUNTIME}/evaluator.returned.status"
readonly SCIENCE_COMPLETE_RECEIPT="${RUNTIME}/science.complete.status"
readonly WORKER_LOG="${RUNTIME}/worker.log"
readonly EVALUATOR_LOG="${RUNTIME}/evaluator.log"
readonly REPORT="${RUNTIME}/frozen_representation_affine_warm_start_stability_report.txt"
readonly RESULT="${RUNTIME}/development.status"
readonly REJECTED_REPORT="${RUNTIME}/rejected.frozen_representation_affine_warm_start_stability_report.txt"
readonly REJECTED_RESULT="${RUNTIME}/rejected.development.status"
readonly TERMINAL="${RUNTIME}/terminal.invalid.status"

readonly WORKER_LOG_CANDIDATE="${SCRATCH}/worker.log.candidate"
readonly EVALUATOR_LOG_CANDIDATE="${SCRATCH}/evaluator.log.candidate"
readonly REPORT_CANDIDATE="${SCRATCH}/report.candidate"
readonly RESULT_CANDIDATE="${SCRATCH}/development.status.candidate"
readonly CAPABILITY_CANDIDATE="${SCRATCH}/worker.capability"

readonly BUILD_TIMEOUT_SECONDS=300
readonly WORKER_TIMEOUT_SECONDS=300
readonly TERM_GRACE_SECONDS=10
readonly OPTIMIZER_SEED=31
readonly OPTIMIZER_STEPS=3500
readonly BATCH_SIZE=64
readonly SCHEDULE_FINGERPRINT="f2fa41d284a42d60"
readonly STEP0_PARITY_TOLERANCE="1e-5"
readonly TRAIN_AGGREGATE_MSE_RATIO_MAX="1.05"
readonly TRAIN_HEAD_MSE_RATIO_MAX="1.10"
readonly TRAIN_METRIC_DEFICIT_MAX="0.01"
readonly TRAIN_RMSE_RATIO_INCREASE_MAX="0.05"
readonly VALIDATION_METRIC_DEFICIT_MAX="0.01"
readonly VALIDATION_RMSE_RATIO_INCREASE_MAX="0.05"
readonly CLEAR_STOP_AGGREGATE_MSE_RATIO_MIN="1.25"
readonly CLEAR_STOP_HEAD_MSE_RATIO_MIN="1.50"
readonly DUPLICATE_MSE_ABS_TOLERANCE="1.25e-7"
readonly DUPLICATE_MSE_REL_TOLERANCE="1.25e-7"
readonly COMPLETE_REPORT_KEY_COUNT=1153

declare -A KV_VALUE=()
declare -A KV_PRESENT=()
declare -A KV_LOADED=()

RUN_TIMEOUT_PID=""
RUN_LAUNCHING=0
RUN_PENDING_SIGNAL=""
RUN_EXIT_GUARD_ACTIVE=0
RUN_CAPABILITY_PATH=""
RUN_CAPABILITY_IDENTITY=""
RUN_CHILD_EXIT="not_started"
RUN_FAILURE_STAGE="bounded_worker"
RUN_FAILURE_REASON="worker_failure"

fail() {
  echo "[clear-signal:warm-start-stability] ERROR: $*" >&2
  return 1
}

sha256() { sha256sum -- "$1" | cut -d' ' -f1; }

require_file() {
  local path="$1" expected_mode="$2" canonical mode uid links
  [[ "$path" == /* && -f "$path" && ! -L "$path" ]] || fail "not a regular absolute file: ${path}"
  canonical="$(readlink -f -- "$path")"
  [[ "$canonical" == "$path" ]] || fail "noncanonical or symlinked file: ${path}"
  read -r mode uid links < <(stat -c '%a %u %h' -- "$path")
  [[ "$mode" == "$expected_mode" && "$uid" == 0 && "$links" == 1 ]] ||
    fail "invalid frozen metadata for ${path}: ${mode}:${uid}:${links}"
}

require_exact() {
  require_file "$1" "$3"
  [[ "$(sha256 "$1")" == "$2" ]] || fail "SHA-256 mismatch: $1"
}

require_private_dir() {
  local path="$1" canonical mode uid
  [[ "$path" == /* && -d "$path" && ! -L "$path" ]] || fail "not a private directory: ${path}"
  canonical="$(readlink -f -- "$path")"
  [[ "$canonical" == "$path" ]] || fail "noncanonical directory: ${path}"
  read -r mode uid < <(stat -c '%a %u' -- "$path")
  [[ "$mode" == 700 && "$uid" == 0 ]] || fail "invalid private directory metadata: ${path} (${mode}:${uid})"
}

load_kv_file() {
  local path="$1" line key value cache_key
  [[ -z "${KV_LOADED[$path]+x}" ]] || return 0
  require_file "$path" 444
  while IFS= read -r line || [[ -n "$line" ]]; do
    [[ -n "$line" && "$line" == *=* && "$line" != *$'\r' ]] || fail "malformed key-value line in ${path}"
    key="${line%%=*}"
    value="${line#*=}"
    [[ -n "$key" ]] || fail "empty key in ${path}"
    cache_key="${path}"$'\034'"${key}"
    [[ -z "${KV_PRESENT[$cache_key]+x}" ]] || fail "duplicate key ${key} in ${path}"
    KV_PRESENT["$cache_key"]=1
    KV_VALUE["$cache_key"]="$value"
  done < "$path"
  KV_LOADED["$path"]=1
}

kv() {
  local path="$1" key="$2" cache_key="${1}"$'\034'"${2}"
  [[ -n "${KV_LOADED[$path]+x}" ]] || fail "uncached key-value file: ${path}"
  [[ -n "${KV_PRESENT[$cache_key]+x}" ]] || fail "missing key ${key} in ${path}"
  printf '%s' "${KV_VALUE[$cache_key]}"
}

expect() {
  local actual
  actual="$(kv "$1" "$2")"
  [[ "$actual" == "$3" ]] || fail "$1: $2 expected '$3', got '$actual'"
}

finite_number() {
  awk -v x="$1" 'BEGIN {
    if (x !~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/) exit 1;
    y=x+0; exit !((y-y)==0);
  }'
}

number_le() { finite_number "$1" && finite_number "$2" && awk -v x="$1" -v y="$2" 'BEGIN { exit !(x<=y) }'; }
number_ge() { finite_number "$1" && finite_number "$2" && awk -v x="$1" -v y="$2" 'BEGIN { exit !(x>=y) }'; }
close_to() {
  finite_number "$1" && finite_number "$2" && finite_number "$3" &&
    awk -v x="$1" -v y="$2" -v t="$3" 'BEGIN { d=x-y; if(d<0)d=-d; exit !(d<=t) }'
}

mixed_close() {
  finite_number "$1" && finite_number "$2" && finite_number "$3" && finite_number "$4" &&
    awk -v x="$1" -v y="$2" -v a="$3" -v r="$4" 'BEGIN {
      d=x-y; if(d<0)d=-d; ax=x; if(ax<0)ax=-ax; ay=y; if(ay<0)ay=-ay;
      scale=(ax>ay?ax:ay); exit !(d<=a+r*scale);
    }'
}

publish_once() {
  local candidate="$1" destination="$2" mode="$3"
  [[ -f "$candidate" && ! -L "$candidate" && ! -e "$destination" && ! -L "$destination" ]] ||
    fail "unsafe publication: ${candidate} -> ${destination}"
  chmod "$mode" -- "$candidate"
  require_file "$candidate" "${mode#0}"
  mv -T -n -- "$candidate" "$destination"
  [[ ! -e "$candidate" ]] || fail "no-clobber publication failed: ${destination}"
  require_file "$destination" "${mode#0}"
}

assert_unprotected_scientific_path() {
  local path="$1"
  [[ "$path" == /* ]] || fail "scientific path is not absolute: ${path}"
  case "${path,,}" in
    *raw-source*|*raw_source*|*certified*|*final-holdout*|*final_holdout*|*/policy/*|*mdn*|*checkpoint*)
      fail "protected scientific path is forbidden: ${path}" ;;
  esac
}

preflight_compile() {
  require_file "$RUNNER" 555
  [[ "$PREREG_SHA" != __* && "$SOURCE_SHA" != __* && "$BUILD_WRAPPER_SHA" != __* ]] ||
    fail "frozen source pins are not finalized"
  require_exact "$PREREG" "$PREREG_SHA" 444
  require_exact "$SOURCE" "$SOURCE_SHA" 444
  require_exact "$BUILD_WRAPPER" "$BUILD_WRAPPER_SHA" 555
}

ensure_runtime_and_lock() {
  local path
  for path in "$RUNTIME" "$BUILD_DIR" "$SCRATCH"; do
    if [[ ! -e "$path" && ! -L "$path" ]]; then mkdir -m 0700 -- "$path"; fi
    require_private_dir "$path"
  done
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || true
    chmod 0600 -- "$LOCK"
  fi
  require_file "$LOCK" 600
}

open_execution_lock_exclusive() {
  local path_identity fd_identity
  require_file "$LOCK" 600
  exec 9<> "$LOCK"
  flock -n 9 || fail "protocol is already active"
  path_identity="$(stat -Lc '%d:%i' -- "$LOCK")"
  fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/9)"
  [[ "$path_identity" == "$fd_identity" ]] || fail "execution lock identity mismatch"
}

open_execution_lock_shared() {
  local path_identity fd_identity
  require_file "$LOCK" 600
  exec 9< "$LOCK"
  flock -s -n 9 || fail "protocol is active"
  path_identity="$(stat -Lc '%d:%i' -- "$LOCK")"
  fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/9)"
  [[ "$path_identity" == "$fd_identity" ]] || fail "execution lock identity mismatch"
}

lock_shared_fd() {
  local fd="$1" path="$2" path_identity fd_identity
  require_file "$path" 600
  eval "exec ${fd}<\"\$path\""
  flock -s -n "$fd" || fail "immutable authority is active: ${path}"
  path_identity="$(stat -Lc '%d:%i' -- "$path")"
  fd_identity="$(stat -Lc '%d:%i' -- "/proc/$$/fd/${fd}")"
  [[ "$path_identity" == "$fd_identity" ]] || fail "authority lock identity mismatch: ${path}"
}

acquire_authority_locks() {
  lock_shared_fd 3 "$CAPTURE_LOCK"
  lock_shared_fd 4 "$LOCALIZATION_SOURCE_LOCK"
  lock_shared_fd 5 "$LOCALIZATION_V3_LOCK"
  lock_shared_fd 6 "$PHASE2B_RESULT_LOCK"
  lock_shared_fd 7 "$PHASE2B_REPORT_LOCK"
}

preflight_frozen_files() {
  local path digest mode
  while IFS='|' read -r path digest mode; do
    require_exact "$path" "$digest" "$mode"
  done <<EOF
${TRAIN}|${TRAIN_SHA}|444
${VALIDATION}|${VALIDATION_SHA}|444
${PHASE2A_RECEIPT}|${PHASE2A_RECEIPT_SHA}|444
${PHASE2A_MAIN}|${PHASE2A_REPORT_SHA}|444
${PHASE2A_REPLAY}|${PHASE2A_REPORT_SHA}|444
${PHASE2B_RESULT}|${PHASE2B_RESULT_SHA}|444
${PHASE2B_REPORT}|${PHASE2B_REPORT_SHA}|444
${LOCALIZATION_SOURCE_REPORT}|${LOCALIZATION_SOURCE_REPORT_SHA}|444
${LOCALIZATION_SOURCE_EVALUATOR}|${LOCALIZATION_SOURCE_EVALUATOR_SHA}|444
${LOCALIZATION_SOURCE_BUILD_WRAPPER}|${LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA}|555
${CANONICAL_AFFINE_SOURCE}|${CANONICAL_AFFINE_SOURCE_SHA}|444
${PHASE2A_SOURCE}|${PHASE2A_SOURCE_SHA}|444
${LOCALIZATION_V3_RESULT}|${LOCALIZATION_V3_RESULT_SHA}|444
${LOCALIZATION_V3_VERIFICATION}|${LOCALIZATION_V3_VERIFICATION_SHA}|444
${LOCALIZATION_V3_SELF_TEST}|${LOCALIZATION_V3_SELF_TEST_SHA}|444
${LOCALIZATION_V3_ATTEMPT}|${LOCALIZATION_V3_ATTEMPT_SHA}|444
${LOCALIZATION_V3_LOG}|${LOCALIZATION_V3_LOG_SHA}|444
${LOCALIZATION_V3_RUNNER}|${LOCALIZATION_V3_RUNNER_SHA}|555
${LOCALIZATION_V3_PREREG}|${LOCALIZATION_V3_PREREG_SHA}|444
${LOCALIZATION_V3_VALIDATOR}|${LOCALIZATION_V3_VALIDATOR_SHA}|444
EOF
}

preflight_authority_receipts() {
  local path
  preflight_frozen_files
  for path in "$PHASE2A_RECEIPT" "$PHASE2A_MAIN" "$PHASE2B_RESULT" "$PHASE2B_REPORT" \
              "$LOCALIZATION_V3_RESULT" "$LOCALIZATION_V3_VERIFICATION"; do
    load_kv_file "$path"
  done

  expect "$PHASE2A_RECEIPT" status complete
  expect "$PHASE2A_RECEIPT" train_probe_sha256 "$TRAIN_SHA"
  expect "$PHASE2A_RECEIPT" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$PHASE2A_RECEIPT" main_replay_byte_identical true
  expect "$PHASE2A_RECEIPT" maximum_anchor_read 2815
  expect "$PHASE2A_RECEIPT" certified_input_access false
  expect "$PHASE2A_RECEIPT" final_holdout_access false
  expect "$PHASE2A_RECEIPT" policy_access false

  expect "$PHASE2B_RESULT" status complete
  expect "$PHASE2B_RESULT" source_nonlinear_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$PHASE2B_RESULT" maximum_anchor_read 2815
  expect "$PHASE2B_RESULT" certified_input_access false
  expect "$PHASE2B_RESULT" final_holdout_access false
  expect "$PHASE2B_RESULT" policy_access false
  expect "$PHASE2B_REPORT" representation_train_input "$TRAIN"
  expect "$PHASE2B_REPORT" representation_validation_input "$VALIDATION"
  expect "$PHASE2B_REPORT" head_selection channel_index_times_3_plus_edge_index
  expect "$PHASE2B_REPORT" maximum_anchor_read 2815
  expect "$PHASE2B_REPORT" steps_per_fit "$OPTIMIZER_STEPS"
  expect "$PHASE2B_REPORT" batch_size "$BATCH_SIZE"
  expect "$PHASE2B_REPORT" optimizer adam
  expect "$PHASE2B_REPORT" validation_read_by_trainer false
  expect "$PHASE2B_REPORT" representation_forward_executed false
  expect "$PHASE2B_REPORT" checkpoint_written false
  expect "$PHASE2B_REPORT" final_holdout_access false
  expect "$PHASE2B_REPORT" policy_access false

  expect "$LOCALIZATION_V3_RESULT" status complete
  expect "$LOCALIZATION_V3_RESULT" protocol_id "$LOCALIZATION_V3_ID"
  expect "$LOCALIZATION_V3_RESULT" diagnostic_authority development_only
  expect "$LOCALIZATION_V3_RESULT" benchmark_acceptance_authority false
  expect "$LOCALIZATION_V3_RESULT" scientific_result_available true
  expect "$LOCALIZATION_V3_RESULT" classification float32_conditioning_failure
  expect "$LOCALIZATION_V3_RESULT" direct_float32_parity_pass false
  expect "$LOCALIZATION_V3_RESULT" paired_gelu_parity_pass true
  expect "$LOCALIZATION_V3_RESULT" direct_linear_adam_recovery_gate_pass false
  expect "$LOCALIZATION_V3_RESULT" direct_linear_adam_clear_failure_gate_pass true
  expect "$LOCALIZATION_V3_RESULT" source_report_sha256 "$LOCALIZATION_SOURCE_REPORT_SHA"
  expect "$LOCALIZATION_V3_RESULT" source_evaluator_sha256 "$LOCALIZATION_SOURCE_EVALUATOR_SHA"
  expect "$LOCALIZATION_V3_RESULT" source_build_wrapper_sha256 "$LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA"
  expect "$LOCALIZATION_V3_RESULT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$LOCALIZATION_V3_RESULT" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$LOCALIZATION_V3_RESULT" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$LOCALIZATION_V3_RESULT" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$LOCALIZATION_V3_RESULT" validator_self_test_receipt_sha256 "$LOCALIZATION_V3_SELF_TEST_SHA"
  expect "$LOCALIZATION_V3_RESULT" verification_attempt_sha256 "$LOCALIZATION_V3_ATTEMPT_SHA"
  expect "$LOCALIZATION_V3_RESULT" verification_log_sha256 "$LOCALIZATION_V3_LOG_SHA"
  expect "$LOCALIZATION_V3_RESULT" verification_receipt_sha256 "$LOCALIZATION_V3_VERIFICATION_SHA"
  expect "$LOCALIZATION_V3_RESULT" maximum_anchor_read 2815
  expect "$LOCALIZATION_V3_RESULT" certified_input_access false
  expect "$LOCALIZATION_V3_RESULT" final_holdout_access false
  expect "$LOCALIZATION_V3_RESULT" policy_access false

  expect "$LOCALIZATION_V3_VERIFICATION" status complete
  expect "$LOCALIZATION_V3_VERIFICATION" protocol_id "$LOCALIZATION_V3_ID"
  expect "$LOCALIZATION_V3_VERIFICATION" source_report_sha256 "$LOCALIZATION_SOURCE_REPORT_SHA"
  expect "$LOCALIZATION_V3_VERIFICATION" new_evaluator_invocations 0
  expect "$LOCALIZATION_V3_VERIFICATION" new_fits 0
  expect "$LOCALIZATION_V3_VERIFICATION" new_optimizer_steps 0
  expect "$LOCALIZATION_V3_VERIFICATION" certified_input_access false
  expect "$LOCALIZATION_V3_VERIFICATION" final_holdout_access false
  expect "$LOCALIZATION_V3_VERIFICATION" policy_access false
}

preflight_science() {
  preflight_compile
  assert_unprotected_scientific_path "$TRAIN"
  assert_unprotected_scientific_path "$VALIDATION"
  assert_unprotected_scientific_path "$REPORT_CANDIDATE"
  [[ "$TRAIN" != "$VALIDATION" && "$TRAIN" != "$REPORT_CANDIDATE" &&
     "$VALIDATION" != "$REPORT_CANDIDATE" ]] || fail "scientific input/output paths are not distinct"
  preflight_authority_receipts
}

emit_build_receipt() {
  local candidate="${SCRATCH}/build.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_build_v1"
    echo "status=complete"
    echo "source_path=${SOURCE}"
    echo "source_sha256=${SOURCE_SHA}"
    echo "build_wrapper_path=${BUILD_WRAPPER}"
    echo "build_wrapper_sha256=${BUILD_WRAPPER_SHA}"
    echo "binary_path=${BIN}"
    echo "binary_sha256=$(sha256 "$BIN")"
    echo "compile_only=true"
    echo "build_timeout_seconds=${BUILD_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
    echo "probe_access=false"
    echo "model_access=false"
    echo "checkpoint_access=false"
  } > "$candidate"
  publish_once "$candidate" "$BUILD_RECEIPT" 0444
}

validate_build_receipt() {
  require_file "$BUILD_RECEIPT" 444
  require_file "$BIN" 555
  load_kv_file "$BUILD_RECEIPT"
  expect "$BUILD_RECEIPT" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_build_v1
  expect "$BUILD_RECEIPT" status complete
  expect "$BUILD_RECEIPT" source_path "$SOURCE"
  expect "$BUILD_RECEIPT" source_sha256 "$SOURCE_SHA"
  expect "$BUILD_RECEIPT" build_wrapper_path "$BUILD_WRAPPER"
  expect "$BUILD_RECEIPT" build_wrapper_sha256 "$BUILD_WRAPPER_SHA"
  expect "$BUILD_RECEIPT" binary_path "$BIN"
  expect "$BUILD_RECEIPT" binary_sha256 "$(sha256 "$BIN")"
  expect "$BUILD_RECEIPT" compile_only true
  expect "$BUILD_RECEIPT" build_timeout_seconds "$BUILD_TIMEOUT_SECONDS"
  expect "$BUILD_RECEIPT" term_grace_seconds "$TERM_GRACE_SECONDS"
  expect "$BUILD_RECEIPT" probe_access false
  expect "$BUILD_RECEIPT" model_access false
  expect "$BUILD_RECEIPT" checkpoint_access false
}

prepare_locked() {
  local rc=0
  preflight_compile
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] ||
    fail "scientific lifecycle already started"
  if [[ -e "$BUILD_RECEIPT" || -L "$BUILD_RECEIPT" ]]; then
    validate_build_receipt
    return
  fi
  [[ ! -e "$BIN" && ! -L "$BIN" ]] || fail "unreceipted build output exists"
  set +e
  timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" "${BUILD_TIMEOUT_SECONDS}s" \
    "$BUILD_WRAPPER" "$BIN"
  rc=$?
  set -e
  (( rc == 0 )) || fail "compile-only preparation exited ${rc}"
  require_file "$BIN" 555
  emit_build_receipt
  validate_build_receipt
}

prepare() {
  preflight_compile
  ensure_runtime_and_lock
  open_execution_lock_exclusive
  prepare_locked
  echo "[clear-signal:warm-start-stability] compile-only preparation verified"
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_attempt_v1"
    echo "status=consumed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_ordinal=1"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "evaluator_source_sha256=${SOURCE_SHA}"
    echo "build_wrapper_sha256=${BUILD_WRAPPER_SHA}"
    echo "build_receipt_sha256=$(sha256 "$BUILD_RECEIPT")"
    echo "binary_sha256=$(sha256 "$BIN")"
    echo "train_probe_sha256=${TRAIN_SHA}"
    echo "validation_probe_sha256=${VALIDATION_SHA}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "phase2a_main_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "phase2a_replay_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "phase2b_result_sha256=${PHASE2B_RESULT_SHA}"
    echo "phase2b_report_sha256=${PHASE2B_REPORT_SHA}"
    echo "localization_v3_result_sha256=${LOCALIZATION_V3_RESULT_SHA}"
    echo "localization_v3_verification_sha256=${LOCALIZATION_V3_VERIFICATION_SHA}"
    echo "localization_source_report_sha256=${LOCALIZATION_SOURCE_REPORT_SHA}"
    echo "localization_source_evaluator_sha256=${LOCALIZATION_SOURCE_EVALUATOR_SHA}"
    echo "localization_source_build_wrapper_sha256=${LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA}"
    echo "canonical_affine_source_sha256=${CANONICAL_AFFINE_SOURCE_SHA}"
    echo "phase2a_source_sha256=${PHASE2A_SOURCE_SHA}"
    echo "evaluator_invocation_limit=1"
    echo "planned_capture_invocations=0"
    echo "planned_representation_forward_invocations=0"
    echo "planned_affine_oracle_grouped_fit_count=1"
    echo "planned_affine_oracle_head_solve_count=9"
    echo "planned_optimizer_fits=1"
    echo "planned_optimizer_steps=${OPTIMIZER_STEPS}"
    echo "planned_total_train_fit_procedures=2"
    echo "optimizer_seed=${OPTIMIZER_SEED}"
    echo "batch_size=${BATCH_SIZE}"
    echo "batch_schedule_fingerprint=${SCHEDULE_FINGERPRINT}"
    echo "step0_parity_tolerance=${STEP0_PARITY_TOLERANCE}"
    echo "step0_parity_failure_terminal=true"
    echo "duplicate_mse_pair_count=2"
    echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOLERANCE}"
    echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOLERANCE}"
    echo "expected_complete_report_key_count=${COMPLETE_REPORT_KEY_COUNT}"
    echo "optimizer_constructed_after_step0_parity=true"
    echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
    echo "maximum_anchor_read=2815"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
    echo "early_stop_allowed=false"
    echo "validation_read_by_trainer=false"
    echo "representation_forward_executed=false"
    echo "checkpoint_access=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$ATTEMPT" 0444
}

validate_attempt() {
  require_file "$ATTEMPT" 444
  load_kv_file "$ATTEMPT"
  expect "$ATTEMPT" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_attempt_v1
  expect "$ATTEMPT" status consumed
  expect "$ATTEMPT" protocol_id "$PROTOCOL_ID"
  expect "$ATTEMPT" attempt_ordinal 1
  expect "$ATTEMPT" runner_path "$RUNNER"
  expect "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"
  expect "$ATTEMPT" evaluator_source_sha256 "$SOURCE_SHA"
  expect "$ATTEMPT" build_wrapper_sha256 "$BUILD_WRAPPER_SHA"
  expect "$ATTEMPT" build_receipt_sha256 "$(sha256 "$BUILD_RECEIPT")"
  expect "$ATTEMPT" binary_sha256 "$(sha256 "$BIN")"
  expect "$ATTEMPT" train_probe_sha256 "$TRAIN_SHA"
  expect "$ATTEMPT" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$ATTEMPT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$ATTEMPT" phase2a_main_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$ATTEMPT" phase2a_replay_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$ATTEMPT" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$ATTEMPT" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$ATTEMPT" localization_v3_result_sha256 "$LOCALIZATION_V3_RESULT_SHA"
  expect "$ATTEMPT" localization_v3_verification_sha256 "$LOCALIZATION_V3_VERIFICATION_SHA"
  expect "$ATTEMPT" localization_source_report_sha256 "$LOCALIZATION_SOURCE_REPORT_SHA"
  expect "$ATTEMPT" localization_source_evaluator_sha256 "$LOCALIZATION_SOURCE_EVALUATOR_SHA"
  expect "$ATTEMPT" localization_source_build_wrapper_sha256 "$LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA"
  expect "$ATTEMPT" canonical_affine_source_sha256 "$CANONICAL_AFFINE_SOURCE_SHA"
  expect "$ATTEMPT" phase2a_source_sha256 "$PHASE2A_SOURCE_SHA"
  expect "$ATTEMPT" evaluator_invocation_limit 1
  expect "$ATTEMPT" planned_capture_invocations 0
  expect "$ATTEMPT" planned_representation_forward_invocations 0
  expect "$ATTEMPT" planned_affine_oracle_grouped_fit_count 1
  expect "$ATTEMPT" planned_affine_oracle_head_solve_count 9
  expect "$ATTEMPT" planned_optimizer_fits 1
  expect "$ATTEMPT" planned_optimizer_steps "$OPTIMIZER_STEPS"
  expect "$ATTEMPT" planned_total_train_fit_procedures 2
  expect "$ATTEMPT" optimizer_seed "$OPTIMIZER_SEED"
  expect "$ATTEMPT" batch_size "$BATCH_SIZE"
  expect "$ATTEMPT" batch_schedule_fingerprint "$SCHEDULE_FINGERPRINT"
  expect "$ATTEMPT" step0_parity_tolerance "$STEP0_PARITY_TOLERANCE"
  expect "$ATTEMPT" step0_parity_failure_terminal true
  expect "$ATTEMPT" duplicate_mse_pair_count 2
  expect "$ATTEMPT" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOLERANCE"
  expect "$ATTEMPT" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOLERANCE"
  expect "$ATTEMPT" expected_complete_report_key_count "$COMPLETE_REPORT_KEY_COUNT"
  expect "$ATTEMPT" optimizer_constructed_after_step0_parity true
  expect "$ATTEMPT" worker_timeout_seconds "$WORKER_TIMEOUT_SECONDS"
  expect "$ATTEMPT" term_grace_seconds "$TERM_GRACE_SECONDS"
  expect "$ATTEMPT" maximum_anchor_read 2815
  expect "$ATTEMPT" retry_allowed false
  expect "$ATTEMPT" resume_allowed false
  expect "$ATTEMPT" early_stop_allowed false
  expect "$ATTEMPT" validation_read_by_trainer false
  expect "$ATTEMPT" representation_forward_executed false
  expect "$ATTEMPT" checkpoint_access false
  expect "$ATTEMPT" certified_input_access false
  expect "$ATTEMPT" final_holdout_access false
  expect "$ATTEMPT" policy_access false
}

emit_evaluator_started() {
  local candidate="${SCRATCH}/evaluator.started.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_evaluator_started_v1"
    echo "status=started"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "command_ordinal=1"
    echo "binary_sha256=$(sha256 "$BIN")"
    echo "train_probe_sha256=${TRAIN_SHA}"
    echo "validation_probe_sha256=${VALIDATION_SHA}"
    echo "output_path=${REPORT_CANDIDATE}"
    echo "evaluator_invocation_count=1"
  } > "$candidate"
  publish_once "$candidate" "$EVALUATOR_STARTED_RECEIPT" 0444
}

emit_evaluator_returned() {
  local exit_code="$1" candidate="${SCRATCH}/evaluator.returned.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_evaluator_returned_v1"
    echo "status=returned"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "evaluator_started_receipt_sha256=$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
    echo "evaluator_exit_code=${exit_code}"
    echo "evaluator_invocation_count=1"
    echo "report_candidate_present=$([[ -f "$REPORT_CANDIDATE" && ! -L "$REPORT_CANDIDATE" ]] && echo true || echo false)"
    echo "optimizer_step_count_if_success=${OPTIMIZER_STEPS}"
  } > "$candidate"
  publish_once "$candidate" "$EVALUATOR_RETURNED_RECEIPT" 0444
}

validate_evaluator_lifecycle() {
  require_file "$EVALUATOR_STARTED_RECEIPT" 444
  require_file "$EVALUATOR_RETURNED_RECEIPT" 444
  load_kv_file "$EVALUATOR_STARTED_RECEIPT"
  load_kv_file "$EVALUATOR_RETURNED_RECEIPT"
  expect "$EVALUATOR_STARTED_RECEIPT" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_evaluator_started_v1
  expect "$EVALUATOR_STARTED_RECEIPT" status started
  expect "$EVALUATOR_STARTED_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$EVALUATOR_STARTED_RECEIPT" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$EVALUATOR_STARTED_RECEIPT" command_ordinal 1
  expect "$EVALUATOR_STARTED_RECEIPT" binary_sha256 "$(sha256 "$BIN")"
  expect "$EVALUATOR_STARTED_RECEIPT" train_probe_sha256 "$TRAIN_SHA"
  expect "$EVALUATOR_STARTED_RECEIPT" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$EVALUATOR_STARTED_RECEIPT" output_path "$REPORT_CANDIDATE"
  expect "$EVALUATOR_STARTED_RECEIPT" evaluator_invocation_count 1
  expect "$EVALUATOR_RETURNED_RECEIPT" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_evaluator_returned_v1
  expect "$EVALUATOR_RETURNED_RECEIPT" status returned
  expect "$EVALUATOR_RETURNED_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$EVALUATOR_RETURNED_RECEIPT" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$EVALUATOR_RETURNED_RECEIPT" evaluator_started_receipt_sha256 "$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
  expect "$EVALUATOR_RETURNED_RECEIPT" evaluator_exit_code 0
  expect "$EVALUATOR_RETURNED_RECEIPT" evaluator_invocation_count 1
  expect "$EVALUATOR_RETURNED_RECEIPT" report_candidate_present true
  expect "$EVALUATOR_RETURNED_RECEIPT" optimizer_step_count_if_success "$OPTIMIZER_STEPS"
}

validate_metric_group() {
  local file="$1" prefix="$2" expected_count="$3" expected_rank_count="$4" field value
  expect "$file" "${prefix}.count" "$expected_count"
  expect "$file" "${prefix}.pairwise_rank_count" "$expected_rank_count"
  for field in mae rmse target_rms prediction_rms rmse_target_rms_ratio; do
    value="$(kv "$file" "${prefix}.${field}")"
    finite_number "$value" && number_ge "$value" 0 || fail "invalid nonnegative metric ${prefix}.${field}"
  done
  for field in directional_accuracy pairwise_rank_accuracy best_asset_agreement; do
    value="$(kv "$file" "${prefix}.${field}")"
    finite_number "$value" && number_ge "$value" 0 && number_le "$value" 1 ||
      fail "invalid unit-interval metric ${prefix}.${field}"
  done
  value="$(kv "$file" "${prefix}.correlation")"
  finite_number "$value" && number_ge "$value" -1 && number_le "$value" 1 ||
    fail "invalid correlation ${prefix}.correlation"
}

validate_head_metric_group() {
  local file="$1" prefix="$2" expected_count="$3" field value
  expect "$file" "${prefix}.count" "$expected_count"
  for field in mae rmse target_rms prediction_rms rmse_target_rms_ratio standardized_mse; do
    value="$(kv "$file" "${prefix}.${field}")"
    finite_number "$value" && number_ge "$value" 0 || fail "invalid nonnegative metric ${prefix}.${field}"
  done
  value="$(kv "$file" "${prefix}.directional_accuracy")"
  finite_number "$value" && number_ge "$value" 0 && number_le "$value" 1 ||
    fail "invalid directional accuracy ${prefix}"
  value="$(kv "$file" "${prefix}.correlation")"
  finite_number "$value" && number_ge "$value" -1 && number_le "$value" 1 ||
    fail "invalid correlation ${prefix}"
}

validate_route_metrics() {
  local file="$1" route split anchors aggregate_count aggregate_rank_count channel_count channel_rank_count
  local channel edge head prefix head_mse head_mse_sum aggregate_mse expected_aggregate_mse
  for route in float64_oracle direct_float32 affine_warm_start_step0 affine_warm_start_final; do
    for split in train validation; do
      if [[ "$split" == train ]]; then anchors=2496; else anchors=256; fi
      aggregate_count=$((anchors * 9))
      aggregate_rank_count="$aggregate_count"
      channel_count=$((anchors * 3))
      channel_rank_count="$channel_count"
      validate_metric_group "$file" "route.${route}.${split}.aggregate" "$aggregate_count" "$aggregate_rank_count"
      aggregate_mse="$(kv "$file" "route.${route}.${split}.aggregate.standardized_mse")"
      finite_number "$aggregate_mse" && number_ge "$aggregate_mse" 0 ||
        fail "invalid aggregate standardized MSE: ${route}.${split}"
      head_mse_sum=0
      for channel in 0 1 2; do
        validate_metric_group "$file" "route.${route}.${split}.channel_${channel}" "$channel_count" "$channel_rank_count"
        for edge in 0 1 2; do
          head=$((channel * 3 + edge))
          prefix="route.${route}.${split}.head_${head}.edge_${edge}.channel_${channel}"
          validate_head_metric_group "$file" "$prefix" "$anchors"
          head_mse="$(kv "$file" "${prefix}.standardized_mse")"
          head_mse_sum="$(awk -v s="$head_mse_sum" -v x="$head_mse" 'BEGIN {printf "%.17g", s+x}')"
        done
      done
      expected_aggregate_mse="$(awk -v s="$head_mse_sum" 'BEGIN {printf "%.17g", s/9.0}')"
      close_to "$aggregate_mse" "$expected_aggregate_mse" 1e-12 ||
        fail "aggregate standardized MSE is inconsistent with nine equal-count heads: ${route}.${split}"
    done
  done
}

validate_oracle_against_phase2a() {
  local file="$1" split channel report_prefix reference_prefix field
  for split in train validation; do
    report_prefix="route.float64_oracle.${split}.aggregate"
    reference_prefix="selected.${split}"
    for field in count pairwise_rank_count; do
      expect "$file" "${report_prefix}.${field}" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")"
    done
    for field in mae rmse target_rms prediction_rms rmse_target_rms_ratio directional_accuracy pairwise_rank_accuracy best_asset_agreement correlation; do
      close_to "$(kv "$file" "${report_prefix}.${field}")" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")" 1e-12 ||
        fail "float64 oracle aggregate does not reproduce Phase2A: ${split}.${field}"
    done
    for channel in 0 1 2; do
      report_prefix="route.float64_oracle.${split}.channel_${channel}"
      reference_prefix="selected.${split}.channel_${channel}"
      for field in count pairwise_rank_count; do
        expect "$file" "${report_prefix}.${field}" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")"
      done
      for field in mae rmse target_rms prediction_rms rmse_target_rms_ratio directional_accuracy pairwise_rank_accuracy best_asset_agreement correlation; do
        close_to "$(kv "$file" "${report_prefix}.${field}")" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")" 1e-12 ||
          fail "float64 oracle channel does not reproduce Phase2A: ${split}.${channel}.${field}"
      done
    done
  done
}

validate_aggregate_against_localization_v3() {
  local file="$1" route split receipt_route field report_field
  for route in float64_oracle direct_float32; do
    receipt_route="$route"
    for split in train validation; do
      for field in directional_accuracy pairwise_rank_accuracy correlation rmse_target_rms_ratio; do
        report_field="$field"
        close_to "$(kv "$file" "route.${route}.${split}.aggregate.${report_field}")" \
          "$(kv "$LOCALIZATION_V3_RESULT" "${receipt_route}.${split}.${field}")" 1e-12 ||
          fail "${route} aggregate does not reproduce authoritative V3: ${split}.${field}"
      done
    done
  done
}

validate_gate_values() {
  local file="$1" expected actual numerator denominator ratio pass=true all_heads=true
  local channel edge head maximum_head_ratio=0 head_ratio direction_deficit rank_deficit correlation_deficit rmse_increase
  local train_gate=true validation_gate=true stability_gate=true aggregate_threshold_pass=false head_threshold_pass=false clear_gate=false expected_class

  close_to "$(kv "$file" step0_parity_tolerance_standardized_target_units)" "$STEP0_PARITY_TOLERANCE" 1e-17 ||
    fail "step-zero parity tolerance drift"
  for actual in \
    "$(kv "$file" step0_train_max_abs_delta_standardized_target_units)" \
    "$(kv "$file" step0_validation_max_abs_delta_standardized_target_units)"; do
    finite_number "$actual" && number_ge "$actual" 0 || fail "invalid step-zero parity delta"
  done
  expected=false
  number_le "$(kv "$file" step0_train_max_abs_delta_standardized_target_units)" "$STEP0_PARITY_TOLERANCE" && expected=true
  expect "$file" step0_train_parity_pass "$expected"
  [[ "$expected" == true ]] || fail "complete report violates train step-zero parity precondition"
  expected=false
  number_le "$(kv "$file" step0_validation_max_abs_delta_standardized_target_units)" "$STEP0_PARITY_TOLERANCE" && expected=true
  expect "$file" step0_validation_parity_pass "$expected"
  [[ "$expected" == true ]] || fail "complete report violates validation step-zero parity precondition"
  expect "$file" step0_parity_gate_pass true

  close_to "$(kv "$file" train_aggregate_standardized_mse_ratio_limit)" "$TRAIN_AGGREGATE_MSE_RATIO_MAX" 1e-15 ||
    fail "train aggregate MSE limit drift"
  numerator="$(kv "$file" route.affine_warm_start_final.train.aggregate.standardized_mse)"
  denominator="$(kv "$file" route.float64_oracle.train.aggregate.standardized_mse)"
  number_ge "$denominator" 0.000000000000000001 || fail "nonpositive oracle train aggregate standardized MSE"
  ratio="$(awk -v n="$numerator" -v d="$denominator" 'BEGIN { printf "%.17g", n/d }')"
  close_to "$(kv "$file" train_aggregate_standardized_mse_ratio_to_oracle)" "$ratio" 1e-12 ||
    fail "train aggregate standardized MSE ratio mismatch"
  expected=false; number_le "$ratio" "$TRAIN_AGGREGATE_MSE_RATIO_MAX" && expected=true
  expect "$file" train_aggregate_standardized_mse_ratio_pass "$expected"
  [[ "$expected" == true ]] || train_gate=false

  close_to "$(kv "$file" train_head_standardized_mse_ratio_limit)" "$TRAIN_HEAD_MSE_RATIO_MAX" 1e-15 ||
    fail "train head MSE limit drift"
  for channel in 0 1 2; do
    for edge in 0 1 2; do
      head=$((channel * 3 + edge))
      numerator="$(kv "$file" "route.affine_warm_start_final.train.head_${head}.edge_${edge}.channel_${channel}.standardized_mse")"
      denominator="$(kv "$file" "route.float64_oracle.train.head_${head}.edge_${edge}.channel_${channel}.standardized_mse")"
      number_ge "$denominator" 0.000000000000000001 || fail "nonpositive oracle train head standardized MSE: ${head}"
      head_ratio="$(awk -v n="$numerator" -v d="$denominator" 'BEGIN { printf "%.17g", n/d }')"
      close_to "$(kv "$file" "train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_to_oracle")" "$head_ratio" 1e-12 ||
        fail "train head standardized MSE ratio mismatch: ${head}"
      expected=false; number_le "$head_ratio" "$TRAIN_HEAD_MSE_RATIO_MAX" && expected=true
      expect "$file" "train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_pass" "$expected"
      [[ "$expected" == true ]] || all_heads=false
      if number_ge "$head_ratio" "$maximum_head_ratio"; then maximum_head_ratio="$head_ratio"; fi
    done
  done
  expect "$file" train_every_head_standardized_mse_ratio_pass "$all_heads"
  close_to "$(kv "$file" train_maximum_head_standardized_mse_ratio_to_oracle)" "$maximum_head_ratio" 1e-12 ||
    fail "maximum train head standardized MSE ratio mismatch"
  [[ "$all_heads" == true ]] || train_gate=false

  close_to "$(kv "$file" metric_deficit_limit)" "$TRAIN_METRIC_DEFICIT_MAX" 1e-15 || fail "metric deficit limit drift"
  close_to "$(kv "$file" rmse_target_rms_ratio_increase_limit)" "$TRAIN_RMSE_RATIO_INCREASE_MAX" 1e-15 ||
    fail "RMSE ratio increase limit drift"
  direction_deficit="$(awk -v o="$(kv "$file" route.float64_oracle.train.aggregate.directional_accuracy)" -v f="$(kv "$file" route.affine_warm_start_final.train.aggregate.directional_accuracy)" 'BEGIN {printf "%.17g", o-f}')"
  rank_deficit="$(awk -v o="$(kv "$file" route.float64_oracle.train.aggregate.pairwise_rank_accuracy)" -v f="$(kv "$file" route.affine_warm_start_final.train.aggregate.pairwise_rank_accuracy)" 'BEGIN {printf "%.17g", o-f}')"
  correlation_deficit="$(awk -v o="$(kv "$file" route.float64_oracle.train.aggregate.correlation)" -v f="$(kv "$file" route.affine_warm_start_final.train.aggregate.correlation)" 'BEGIN {printf "%.17g", o-f}')"
  rmse_increase="$(awk -v o="$(kv "$file" route.float64_oracle.train.aggregate.rmse_target_rms_ratio)" -v f="$(kv "$file" route.affine_warm_start_final.train.aggregate.rmse_target_rms_ratio)" 'BEGIN {printf "%.17g", f-o}')"
  for expected in "$direction_deficit" "$rank_deficit" "$correlation_deficit" "$rmse_increase"; do finite_number "$expected" || fail "invalid train gate delta"; done
  close_to "$(kv "$file" train_direction_deficit_to_oracle)" "$direction_deficit" 1e-12 || fail "train direction deficit mismatch"
  close_to "$(kv "$file" train_rank_deficit_to_oracle)" "$rank_deficit" 1e-12 || fail "train rank deficit mismatch"
  close_to "$(kv "$file" train_correlation_deficit_to_oracle)" "$correlation_deficit" 1e-12 || fail "train correlation deficit mismatch"
  close_to "$(kv "$file" train_rmse_target_rms_ratio_increase_to_oracle)" "$rmse_increase" 1e-12 || fail "train RMSE ratio increase mismatch"
  expected=false; number_le "$direction_deficit" "$TRAIN_METRIC_DEFICIT_MAX" && expected=true; expect "$file" train_direction_pass "$expected"; [[ "$expected" == true ]] || train_gate=false
  expected=false; number_le "$rank_deficit" "$TRAIN_METRIC_DEFICIT_MAX" && expected=true; expect "$file" train_rank_pass "$expected"; [[ "$expected" == true ]] || train_gate=false
  expected=false; number_le "$correlation_deficit" "$TRAIN_METRIC_DEFICIT_MAX" && expected=true; expect "$file" train_correlation_pass "$expected"; [[ "$expected" == true ]] || train_gate=false
  expected=false; number_le "$rmse_increase" "$TRAIN_RMSE_RATIO_INCREASE_MAX" && expected=true; expect "$file" train_rmse_target_rms_ratio_pass "$expected"; [[ "$expected" == true ]] || train_gate=false
  expect "$file" train_preservation_gate_pass "$train_gate"

  direction_deficit="$(awk -v d="$(kv "$file" route.direct_float32.validation.aggregate.directional_accuracy)" -v f="$(kv "$file" route.affine_warm_start_final.validation.aggregate.directional_accuracy)" 'BEGIN {printf "%.17g", d-f}')"
  rank_deficit="$(awk -v d="$(kv "$file" route.direct_float32.validation.aggregate.pairwise_rank_accuracy)" -v f="$(kv "$file" route.affine_warm_start_final.validation.aggregate.pairwise_rank_accuracy)" 'BEGIN {printf "%.17g", d-f}')"
  correlation_deficit="$(awk -v d="$(kv "$file" route.direct_float32.validation.aggregate.correlation)" -v f="$(kv "$file" route.affine_warm_start_final.validation.aggregate.correlation)" 'BEGIN {printf "%.17g", d-f}')"
  rmse_increase="$(awk -v d="$(kv "$file" route.direct_float32.validation.aggregate.rmse_target_rms_ratio)" -v f="$(kv "$file" route.affine_warm_start_final.validation.aggregate.rmse_target_rms_ratio)" 'BEGIN {printf "%.17g", f-d}')"
  for expected in "$direction_deficit" "$rank_deficit" "$correlation_deficit" "$rmse_increase"; do finite_number "$expected" || fail "invalid validation guard delta"; done
  close_to "$(kv "$file" validation_direction_deficit_to_direct_float32)" "$direction_deficit" 1e-12 || fail "validation direction deficit mismatch"
  close_to "$(kv "$file" validation_rank_deficit_to_direct_float32)" "$rank_deficit" 1e-12 || fail "validation rank deficit mismatch"
  close_to "$(kv "$file" validation_correlation_deficit_to_direct_float32)" "$correlation_deficit" 1e-12 || fail "validation correlation deficit mismatch"
  close_to "$(kv "$file" validation_rmse_target_rms_ratio_increase_to_direct_float32)" "$rmse_increase" 1e-12 || fail "validation RMSE ratio increase mismatch"
  expected=false; number_le "$direction_deficit" "$VALIDATION_METRIC_DEFICIT_MAX" && expected=true; expect "$file" validation_direction_pass "$expected"; [[ "$expected" == true ]] || validation_gate=false
  expected=false; number_le "$rank_deficit" "$VALIDATION_METRIC_DEFICIT_MAX" && expected=true; expect "$file" validation_rank_pass "$expected"; [[ "$expected" == true ]] || validation_gate=false
  expected=false; number_le "$correlation_deficit" "$VALIDATION_METRIC_DEFICIT_MAX" && expected=true; expect "$file" validation_correlation_pass "$expected"; [[ "$expected" == true ]] || validation_gate=false
  expected=false; number_le "$rmse_increase" "$VALIDATION_RMSE_RATIO_INCREASE_MAX" && expected=true; expect "$file" validation_rmse_target_rms_ratio_pass "$expected"; [[ "$expected" == true ]] || validation_gate=false
  expect "$file" validation_guard_gate_pass "$validation_gate"

  [[ "$train_gate" == true && "$validation_gate" == true ]] || stability_gate=false
  expect "$file" warm_start_stability_gate_pass "$stability_gate"
  close_to "$(kv "$file" clear_stop_aggregate_standardized_mse_ratio_threshold)" "$CLEAR_STOP_AGGREGATE_MSE_RATIO_MIN" 1e-15 || fail "clear-stop aggregate threshold drift"
  close_to "$(kv "$file" clear_stop_maximum_head_standardized_mse_ratio_threshold)" "$CLEAR_STOP_HEAD_MSE_RATIO_MIN" 1e-15 || fail "clear-stop head threshold drift"
  number_ge "$ratio" "$CLEAR_STOP_AGGREGATE_MSE_RATIO_MIN" && aggregate_threshold_pass=true
  number_ge "$maximum_head_ratio" "$CLEAR_STOP_HEAD_MSE_RATIO_MIN" && head_threshold_pass=true
  expect "$file" clear_stop_aggregate_threshold_pass "$aggregate_threshold_pass"
  expect "$file" clear_stop_maximum_head_threshold_pass "$head_threshold_pass"
  if [[ "$stability_gate" == false && ( "$aggregate_threshold_pass" == true || "$head_threshold_pass" == true ) ]]; then clear_gate=true; fi
  expect "$file" clear_stop_gate_pass "$clear_gate"
  if [[ "$stability_gate" == true ]]; then expected_class=warm_start_stability_established
  elif [[ "$clear_gate" == true ]]; then expected_class=optimizer_destabilization_clear_stop
  else expected_class=warm_start_stability_inconclusive; fi
  expect "$file" classification "$expected_class"
}

validate_report() {
  local file="$1" field value
  require_file "$file" 444
  [[ "$(wc -l < "$file")" == "$COMPLETE_REPORT_KEY_COUNT" ]] ||
    fail "complete report key/line count mismatch"
  load_kv_file "$file"
  expect "$file" schema_id "$REPORT_SCHEMA"
  expect "$file" status complete
  expect "$file" benchmark_id synthetic_continuous_graph_v2
  expect "$file" diagnostic_phase affine_warm_start_stability
  expect "$file" diagnostic_authority development_only
  expect "$file" benchmark_acceptance_authority false
  expect "$file" train_input "$TRAIN"
  expect "$file" validation_input "$VALIDATION"
  expect "$file" probe_kind representation
  expect "$file" probe_record_schema kikijyeba.synthetic.representation_edge_feature_probe.v1
  expect "$file" train_probe_rows 22464
  expect "$file" validation_probe_rows 2304
  expect "$file" certified_probe_rows 0
  expect "$file" fit_anchor_range '[0,2496)'
  expect "$file" validation_anchor_range '[2560,2816)'
  expect "$file" maximum_anchor_read 2815
  expect "$file" final_holdout_begin 3328
  expect "$file" validation_read_by_trainer false
  expect "$file" validation_driven_choice false
  expect "$file" representation_forward_executed false
  expect "$file" checkpoint_read false
  expect "$file" checkpoint_written false
  expect "$file" model_input_access false
  expect "$file" certified_input_access false
  expect "$file" final_holdout_access false
  expect "$file" policy_access false

  close_to "$(kv "$file" fixed_ridge)" 1e-12 1e-24 || fail "fixed ridge drift"
  expect "$file" ridge_selection false
  expect "$file" head_index_formula 'channel*3+edge'
  expect "$file" flat_row_order anchor,edge,channel
  expect "$file" architecture 'Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)+gather(channel*3+edge)'
  expect "$file" initialization paired_gelu_exact_affine_injection
  expect "$file" gelu_identity 'GELU(z)-GELU(-z)=z'
  expect "$file" gelu_injected_hidden_units_per_layer 18
  expect "$file" gelu_unused_hidden_units_per_layer 110
  expect "$file" optimizer_constructed_after_injection true
  expect "$file" optimizer_constructed_after_step0_parity true
  expect "$file" all_parameters_trainable true
  expect "$file" frozen_parameter_tensor_count 0
  expect "$file" parameter_tensor_count 6
  expect "$file" trainable_parameter_count 30089
  expect "$file" device cpu
  expect "$file" feature_dtype float32
  expect "$file" target_training_dtype float32
  expect "$file" metric_dtype float64
  expect "$file" oracle_solver_dtype float64
  expect "$file" deterministic_algorithms true
  expect "$file" deterministic_cudnn true
  expect "$file" deterministic_fill_uninitialized_memory true
  expect "$file" intraop_threads 1
  expect "$file" interop_threads 1
  expect "$file" float64_solver float64_centered_cholesky_ridge
  expect "$file" float32_feature_standardization train_core_all_edges_all_channels
  expect "$file" target_standardization train_core_per_edge_channel
  value="$(kv "$file" feature_standardization_clamped_coordinate_count)"
  finite_number "$value" && number_ge "$value" 0 && number_le "$value" 96 && [[ "$value" =~ ^[0-9]+$ ]] ||
    fail "invalid feature standardization clamp count"
  value="$(kv "$file" target_standardization_clamped_coordinate_count)"
  finite_number "$value" && number_ge "$value" 0 && number_le "$value" 9 && [[ "$value" =~ ^[0-9]+$ ]] ||
    fail "invalid target standardization clamp count"
  value="$(kv "$file" affine_maximum_normalized_residual)"
  finite_number "$value" && number_ge "$value" 0 && number_le "$value" 1e-7 || fail "invalid affine solver residual"
  value="$(kv "$file" affine_coefficient_l2_norm)"
  finite_number "$value" && number_ge "$value" 0.000000000000000001 || fail "invalid affine coefficient norm"

  expect "$file" seed "$OPTIMIZER_SEED"
  expect "$file" affine_oracle_grouped_fit_count 1
  expect "$file" affine_oracle_head_solve_count 9
  expect "$file" optimizer_fits_completed 1
  expect "$file" total_train_fit_procedures 2
  expect "$file" optimizer_steps_completed "$OPTIMIZER_STEPS"
  expect "$file" steps_per_fit "$OPTIMIZER_STEPS"
  expect "$file" batch_size "$BATCH_SIZE"
  expect "$file" batch_schedule_fingerprint "$SCHEDULE_FINGERPRINT"
  close_to "$(kv "$file" learning_rate)" 0.001 1e-15 || fail "learning-rate drift"
  close_to "$(kv "$file" adam_beta1)" 0.9 1e-15 || fail "Adam beta1 drift"
  close_to "$(kv "$file" adam_beta2)" 0.999 1e-15 || fail "Adam beta2 drift"
  close_to "$(kv "$file" adam_epsilon)" 1e-8 1e-20 || fail "Adam epsilon drift"
  close_to "$(kv "$file" weight_decay)" 0 0 || fail "weight decay drift"
  close_to "$(kv "$file" gradient_clip_norm)" 5 0 || fail "gradient clip drift"
  expect "$file" optimizer Adam
  expect "$file" batch_sampling mt19937_64_uniform_with_replacement
  expect "$file" early_stopping false
  expect "$file" seed_selection false
  expect "$file" hyperparameter_search false
  expect "$file" retry false
  expect "$file" refit false
  for field in initial_full_train_standardized_mse final_full_train_standardized_mse last_minibatch_loss maximum_preclip_gradient_norm; do
    value="$(kv "$file" "warm_start.${field}")"
    finite_number "$value" && number_ge "$value" 0 || fail "invalid warm-start diagnostic: ${field}"
  done
  mixed_close "$(kv "$file" warm_start.initial_full_train_standardized_mse)" \
    "$(kv "$file" route.affine_warm_start_step0.train.aggregate.standardized_mse)" \
    "$DUPLICATE_MSE_ABS_TOLERANCE" "$DUPLICATE_MSE_REL_TOLERANCE" ||
    fail "step-zero full-train MSE mismatch"
  mixed_close "$(kv "$file" warm_start.final_full_train_standardized_mse)" \
    "$(kv "$file" route.affine_warm_start_final.train.aggregate.standardized_mse)" \
    "$DUPLICATE_MSE_ABS_TOLERANCE" "$DUPLICATE_MSE_REL_TOLERANCE" ||
    fail "final full-train MSE mismatch"
  value="$(kv "$file" warm_start.clipped_step_count)"
  [[ "$value" =~ ^[0-9]+$ ]] && (( value >= 0 && value <= OPTIMIZER_STEPS )) || fail "invalid clipped-step count"

  expect "$file" train_preservation_gate 'train_aggregate_standardized_mse_ratio_to_oracle<=1.05,every_train_head_standardized_mse_ratio_to_oracle<=1.10,train_direction>=oracle-0.01,train_rank>=oracle-0.01,train_correlation>=oracle-0.01,train_rmse_target_rms_ratio<=oracle+0.05'
  expect "$file" validation_guard_gate 'validation_direction>=direct_float32-0.01,validation_rank>=direct_float32-0.01,validation_correlation>=direct_float32-0.01,validation_rmse_target_rms_ratio<=direct_float32+0.05'
  expect "$file" clear_stop_gate 'step0_parity_and_not_stable_and_(train_aggregate_standardized_mse_ratio_to_oracle>=1.25_or_train_maximum_head_standardized_mse_ratio_to_oracle>=1.50)'

  validate_route_metrics "$file"
  validate_oracle_against_phase2a "$file"
  validate_aggregate_against_localization_v3 "$file"
  validate_gate_values "$file"
}

emit_result_body() {
  local report="$1" channel edge head key
  echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_development_receipt_v1"
  echo "status=complete"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "diagnostic_authority=development_only"
  echo "benchmark_acceptance_authority=false"
  echo "scientific_result_available=true"
  echo "classification=$(kv "$report" classification)"
  for key in \
    step0_train_max_abs_delta_standardized_target_units \
    step0_validation_max_abs_delta_standardized_target_units \
    step0_train_parity_pass step0_validation_parity_pass step0_parity_gate_pass \
    train_aggregate_standardized_mse_ratio_to_oracle \
    train_aggregate_standardized_mse_ratio_pass \
    train_maximum_head_standardized_mse_ratio_to_oracle \
    train_every_head_standardized_mse_ratio_pass \
    train_direction_deficit_to_oracle train_direction_pass \
    train_rank_deficit_to_oracle train_rank_pass \
    train_correlation_deficit_to_oracle train_correlation_pass \
    train_rmse_target_rms_ratio_increase_to_oracle train_rmse_target_rms_ratio_pass \
    train_preservation_gate_pass \
    validation_direction_deficit_to_direct_float32 validation_direction_pass \
    validation_rank_deficit_to_direct_float32 validation_rank_pass \
    validation_correlation_deficit_to_direct_float32 validation_correlation_pass \
    validation_rmse_target_rms_ratio_increase_to_direct_float32 validation_rmse_target_rms_ratio_pass \
    validation_guard_gate_pass warm_start_stability_gate_pass \
    clear_stop_aggregate_threshold_pass clear_stop_maximum_head_threshold_pass clear_stop_gate_pass; do
    echo "${key}=$(kv "$report" "$key")"
  done
  for channel in 0 1 2; do
    for edge in 0 1 2; do
      head=$((channel * 3 + edge))
      key="train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_to_oracle"
      echo "${key}=$(kv "$report" "$key")"
      key="train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_pass"
      echo "${key}=$(kv "$report" "$key")"
    done
  done
  echo "runner_sha256=$(sha256 "$RUNNER")"
  echo "preregistration_sha256=${PREREG_SHA}"
  echo "evaluator_source_sha256=${SOURCE_SHA}"
  echo "build_wrapper_sha256=${BUILD_WRAPPER_SHA}"
  echo "build_receipt_sha256=$(sha256 "$BUILD_RECEIPT")"
  echo "binary_sha256=$(sha256 "$BIN")"
  echo "attempt_sha256=$(sha256 "$ATTEMPT")"
  echo "evaluator_started_receipt_sha256=$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
  echo "evaluator_returned_receipt_sha256=$(sha256 "$EVALUATOR_RETURNED_RECEIPT")"
  echo "science_complete_receipt_sha256=$(sha256 "$SCIENCE_COMPLETE_RECEIPT")"
  echo "worker_log_sha256=$(sha256 "$WORKER_LOG")"
  echo "evaluator_log_sha256=$(sha256 "$EVALUATOR_LOG")"
  echo "report_path=${REPORT}"
  echo "report_sha256=$(sha256 "$REPORT")"
  echo "train_probe_sha256=${TRAIN_SHA}"
  echo "validation_probe_sha256=${VALIDATION_SHA}"
  echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
  echo "phase2a_report_sha256=${PHASE2A_REPORT_SHA}"
  echo "phase2b_result_sha256=${PHASE2B_RESULT_SHA}"
  echo "phase2b_report_sha256=${PHASE2B_REPORT_SHA}"
  echo "localization_v3_result_sha256=${LOCALIZATION_V3_RESULT_SHA}"
  echo "localization_v3_verification_sha256=${LOCALIZATION_V3_VERIFICATION_SHA}"
  echo "localization_source_report_sha256=${LOCALIZATION_SOURCE_REPORT_SHA}"
  echo "localization_source_evaluator_sha256=${LOCALIZATION_SOURCE_EVALUATOR_SHA}"
  echo "localization_source_build_wrapper_sha256=${LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA}"
  echo "canonical_affine_source_sha256=${CANONICAL_AFFINE_SOURCE_SHA}"
  echo "phase2a_source_sha256=${PHASE2A_SOURCE_SHA}"
  echo "capture_invocations=0"
  echo "representation_forward_invocations=0"
  echo "evaluator_invocations=1"
  echo "affine_oracle_grouped_fit_count=1"
  echo "affine_oracle_head_solve_count=9"
  echo "optimizer_fits_completed=1"
  echo "optimizer_steps_completed=${OPTIMIZER_STEPS}"
  echo "total_train_fit_procedures=2"
  echo "seed=${OPTIMIZER_SEED}"
  echo "batch_size=${BATCH_SIZE}"
  echo "batch_schedule_fingerprint=${SCHEDULE_FINGERPRINT}"
  echo "validation_read_by_trainer=false"
  echo "validation_driven_choice=false"
  echo "representation_forward_executed=false"
  echo "checkpoint_read=false"
  echo "checkpoint_written=false"
  echo "model_input_access=false"
  echo "maximum_anchor_read=2815"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_access=false"
  echo "retry_allowed=false"
  echo "resume_allowed=false"
}

validate_result_content() {
  local file="$1" channel edge head key
  require_file "$file" 444
  load_kv_file "$file"
  expect "$file" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_development_receipt_v1
  expect "$file" status complete
  expect "$file" protocol_id "$PROTOCOL_ID"
  expect "$file" diagnostic_authority development_only
  expect "$file" benchmark_acceptance_authority false
  expect "$file" scientific_result_available true
  expect "$file" classification "$(kv "$REPORT" classification)"
  for key in \
    step0_train_max_abs_delta_standardized_target_units \
    step0_validation_max_abs_delta_standardized_target_units \
    step0_train_parity_pass step0_validation_parity_pass step0_parity_gate_pass \
    train_aggregate_standardized_mse_ratio_to_oracle \
    train_aggregate_standardized_mse_ratio_pass \
    train_maximum_head_standardized_mse_ratio_to_oracle \
    train_every_head_standardized_mse_ratio_pass \
    train_direction_deficit_to_oracle train_direction_pass \
    train_rank_deficit_to_oracle train_rank_pass \
    train_correlation_deficit_to_oracle train_correlation_pass \
    train_rmse_target_rms_ratio_increase_to_oracle train_rmse_target_rms_ratio_pass \
    train_preservation_gate_pass \
    validation_direction_deficit_to_direct_float32 validation_direction_pass \
    validation_rank_deficit_to_direct_float32 validation_rank_pass \
    validation_correlation_deficit_to_direct_float32 validation_correlation_pass \
    validation_rmse_target_rms_ratio_increase_to_direct_float32 validation_rmse_target_rms_ratio_pass \
    validation_guard_gate_pass warm_start_stability_gate_pass \
    clear_stop_aggregate_threshold_pass clear_stop_maximum_head_threshold_pass clear_stop_gate_pass; do
    expect "$file" "$key" "$(kv "$REPORT" "$key")"
  done
  for channel in 0 1 2; do
    for edge in 0 1 2; do
      head=$((channel * 3 + edge))
      for key in \
        "train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_to_oracle" \
        "train_head_${head}.edge_${edge}.channel_${channel}.standardized_mse_ratio_pass"; do
        expect "$file" "$key" "$(kv "$REPORT" "$key")"
      done
    done
  done
  expect "$file" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$file" preregistration_sha256 "$PREREG_SHA"
  expect "$file" evaluator_source_sha256 "$SOURCE_SHA"
  expect "$file" build_wrapper_sha256 "$BUILD_WRAPPER_SHA"
  expect "$file" build_receipt_sha256 "$(sha256 "$BUILD_RECEIPT")"
  expect "$file" binary_sha256 "$(sha256 "$BIN")"
  expect "$file" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$file" evaluator_started_receipt_sha256 "$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
  expect "$file" evaluator_returned_receipt_sha256 "$(sha256 "$EVALUATOR_RETURNED_RECEIPT")"
  expect "$file" science_complete_receipt_sha256 "$(sha256 "$SCIENCE_COMPLETE_RECEIPT")"
  expect "$file" worker_log_sha256 "$(sha256 "$WORKER_LOG")"
  expect "$file" evaluator_log_sha256 "$(sha256 "$EVALUATOR_LOG")"
  expect "$file" report_path "$REPORT"
  expect "$file" report_sha256 "$(sha256 "$REPORT")"
  expect "$file" train_probe_sha256 "$TRAIN_SHA"
  expect "$file" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$file" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$file" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$file" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$file" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$file" localization_v3_result_sha256 "$LOCALIZATION_V3_RESULT_SHA"
  expect "$file" localization_v3_verification_sha256 "$LOCALIZATION_V3_VERIFICATION_SHA"
  expect "$file" localization_source_report_sha256 "$LOCALIZATION_SOURCE_REPORT_SHA"
  expect "$file" localization_source_evaluator_sha256 "$LOCALIZATION_SOURCE_EVALUATOR_SHA"
  expect "$file" localization_source_build_wrapper_sha256 "$LOCALIZATION_SOURCE_BUILD_WRAPPER_SHA"
  expect "$file" canonical_affine_source_sha256 "$CANONICAL_AFFINE_SOURCE_SHA"
  expect "$file" phase2a_source_sha256 "$PHASE2A_SOURCE_SHA"
  expect "$file" capture_invocations 0
  expect "$file" representation_forward_invocations 0
  expect "$file" evaluator_invocations 1
  expect "$file" affine_oracle_grouped_fit_count 1
  expect "$file" affine_oracle_head_solve_count 9
  expect "$file" optimizer_fits_completed 1
  expect "$file" optimizer_steps_completed "$OPTIMIZER_STEPS"
  expect "$file" total_train_fit_procedures 2
  expect "$file" seed "$OPTIMIZER_SEED"
  expect "$file" batch_size "$BATCH_SIZE"
  expect "$file" batch_schedule_fingerprint "$SCHEDULE_FINGERPRINT"
  expect "$file" validation_read_by_trainer false
  expect "$file" validation_driven_choice false
  expect "$file" representation_forward_executed false
  expect "$file" checkpoint_read false
  expect "$file" checkpoint_written false
  expect "$file" model_input_access false
  expect "$file" maximum_anchor_read 2815
  expect "$file" certified_input_access false
  expect "$file" final_holdout_access false
  expect "$file" policy_access false
  expect "$file" retry_allowed false
  expect "$file" resume_allowed false
}

emit_result() {
  local candidate="$RESULT_CANDIDATE"
  [[ ! -e "$candidate" && ! -L "$candidate" ]] || fail "result candidate path is not pristine"
  load_kv_file "$REPORT"
  emit_result_body "$REPORT" > "$candidate"
  chmod 0444 -- "$candidate"
  validate_result_content "$candidate"
  [[ -f "$candidate" && ! -L "$candidate" && ! -e "$RESULT" && ! -L "$RESULT" ]] ||
    fail "final result commit precondition failed"
  trap '' HUP INT TERM QUIT
  mv -T -n -- "$candidate" "$RESULT"
  RUN_EXIT_GUARD_ACTIVE=0
  trap - EXIT HUP INT TERM QUIT
}

validate_result() {
  local file="$1"
  preflight_science
  validate_build_receipt
  validate_attempt
  validate_evaluator_lifecycle
  require_file "$WORKER_LOG" 444
  require_file "$EVALUATOR_LOG" 444
  validate_report "$REPORT"
  validate_science_complete
  validate_result_content "$file"
  [[ ! -e "$TERMINAL" && ! -e "$REJECTED_RESULT" && ! -e "$REJECTED_REPORT" ]] ||
    fail "result coexists with terminal or rejected artifacts"
}

hash_or_not_available() {
  local path="$1"
  if [[ -f "$path" && ! -L "$path" ]]; then sha256 "$path"; else printf '%s' not_available; fi
}

retire_committed_success() {
  if [[ -f "$RESULT" && ! -L "$RESULT" && ! -e "$REJECTED_RESULT" ]]; then
    mv -T -n -- "$RESULT" "$REJECTED_RESULT"
    require_file "$REJECTED_RESULT" 444
  fi
  if [[ -f "$REPORT" && ! -L "$REPORT" && ! -e "$REJECTED_REPORT" ]]; then
    mv -T -n -- "$REPORT" "$REJECTED_REPORT"
    require_file "$REJECTED_REPORT" 444
  fi
}

publish_failure_evidence() {
  if [[ -f "$WORKER_LOG_CANDIDATE" && ! -L "$WORKER_LOG_CANDIDATE" && ! -e "$WORKER_LOG" ]]; then
    publish_once "$WORKER_LOG_CANDIDATE" "$WORKER_LOG" 0444 || true
  fi
  if [[ -f "$EVALUATOR_LOG_CANDIDATE" && ! -L "$EVALUATOR_LOG_CANDIDATE" && ! -e "$EVALUATOR_LOG" ]]; then
    publish_once "$EVALUATOR_LOG_CANDIDATE" "$EVALUATOR_LOG" 0444 || true
  fi
  if [[ -f "$REPORT_CANDIDATE" && ! -L "$REPORT_CANDIDATE" && ! -e "$REJECTED_REPORT" ]]; then
    publish_once "$REPORT_CANDIDATE" "$REJECTED_REPORT" 0444 || true
  fi
  if [[ -f "$RESULT_CANDIDATE" && ! -L "$RESULT_CANDIDATE" && ! -e "$REJECTED_RESULT" ]]; then
    publish_once "$RESULT_CANDIDATE" "$REJECTED_RESULT" 0444 || true
  fi
}

seal_terminal() {
  local worker_exit="$1" failure_stage="$2" failure_reason="$3"
  local candidate evaluator_started=0 evaluator_exit=not_returned report_state=absent
  local scientific_execution_completed=false optimizer_steps_completed=not_available
  [[ -f "$ATTEMPT" && ! -L "$ATTEMPT" ]] || return 0
  [[ ! -e "$TERMINAL" && ! -L "$TERMINAL" ]] || return 0
  retire_committed_success
  publish_failure_evidence
  if [[ -f "$EVALUATOR_STARTED_RECEIPT" && ! -L "$EVALUATOR_STARTED_RECEIPT" ]]; then evaluator_started=1; fi
  if [[ -f "$EVALUATOR_RETURNED_RECEIPT" && ! -L "$EVALUATOR_RETURNED_RECEIPT" ]]; then
    load_kv_file "$EVALUATOR_RETURNED_RECEIPT" || true
    evaluator_exit="$(kv "$EVALUATOR_RETURNED_RECEIPT" evaluator_exit_code 2>/dev/null || printf '%s' malformed_return_receipt)"
  fi
  if [[ -f "$REJECTED_REPORT" && ! -L "$REJECTED_REPORT" ]]; then report_state=rejected;
  elif [[ -f "$REPORT" && ! -L "$REPORT" ]]; then report_state=committed; fi
  if [[ -f "$SCIENCE_COMPLETE_RECEIPT" && ! -L "$SCIENCE_COMPLETE_RECEIPT" ]]; then
    scientific_execution_completed=true
    optimizer_steps_completed="$OPTIMIZER_STEPS"
  fi
  candidate="${SCRATCH}/terminal.invalid.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_terminal_v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_warm_start_stability_protocol"
    echo "failure_stage=${failure_stage}"
    echo "failure_reason=${failure_reason}"
    echo "worker_exit_code=${worker_exit}"
    echo "attempt_consumed=true"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "evaluator_started=${evaluator_started}"
    echo "evaluator_exit_code=${evaluator_exit}"
    echo "evaluator_started_receipt_sha256=$(hash_or_not_available "$EVALUATOR_STARTED_RECEIPT")"
    echo "evaluator_returned_receipt_sha256=$(hash_or_not_available "$EVALUATOR_RETURNED_RECEIPT")"
    echo "science_complete_receipt_sha256=$(hash_or_not_available "$SCIENCE_COMPLETE_RECEIPT")"
    echo "worker_log_sha256=$(hash_or_not_available "$WORKER_LOG")"
    echo "evaluator_log_sha256=$(hash_or_not_available "$EVALUATOR_LOG")"
    echo "report_state=${report_state}"
    echo "rejected_report_sha256=$(hash_or_not_available "$REJECTED_REPORT")"
    echo "rejected_result_sha256=$(hash_or_not_available "$REJECTED_RESULT")"
    echo "scientific_execution_completed=${scientific_execution_completed}"
    echo "scientific_result_available=false"
    echo "optimizer_steps_completed=${optimizer_steps_completed}"
    echo "same_protocol_retry_allowed=false"
    echo "same_protocol_resume_allowed=false"
    echo "maximum_anchor_read_upper_bound=2815"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$TERMINAL" 0444
}

cleanup_capability() {
  exec 8<&- 2>/dev/null || true
  RUN_CAPABILITY_IDENTITY=""
  if [[ -n "$RUN_CAPABILITY_PATH" ]]; then
    rm -f -- "$RUN_CAPABILITY_PATH" 2>/dev/null || true
    RUN_CAPABILITY_PATH=""
  fi
}

run_exit_guard() {
  local rc="$1"
  (( RUN_EXIT_GUARD_ACTIVE == 1 )) || return 0
  RUN_EXIT_GUARD_ACTIVE=0
  trap - EXIT
  trap '' HUP INT TERM QUIT
  stop_and_reap_worker TERM
  cleanup_capability
  if [[ -f "$ATTEMPT" && ! -e "$TERMINAL" ]]; then
    seal_terminal "$rc" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON" || true
  fi
  exit "$rc"
}

stop_and_reap_worker() {
  local signal_name="$1"
  [[ -n "$RUN_TIMEOUT_PID" ]] || return 0
  kill -s "$signal_name" -- "-${RUN_TIMEOUT_PID}" 2>/dev/null ||
    kill -s "$signal_name" -- "$RUN_TIMEOUT_PID" 2>/dev/null || true
  wait "$RUN_TIMEOUT_PID" 2>/dev/null || true
  RUN_TIMEOUT_PID=""
}

run_signal() {
  local signal_name="$1" shell_exit="$2"
  RUN_FAILURE_STAGE="signal"
  RUN_FAILURE_REASON="signal_${signal_name}"
  RUN_PENDING_SIGNAL="$signal_name:$shell_exit"
  if [[ "$RUN_LAUNCHING" == 1 && -z "$RUN_TIMEOUT_PID" ]]; then return 0; fi
  trap '' HUP INT TERM QUIT
  stop_and_reap_worker TERM
  if [[ -f "$ATTEMPT" && ! -e "$TERMINAL" ]]; then
    seal_terminal "$shell_exit" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON" || true
  fi
  exit "$shell_exit"
}

authorize_private_worker() {
  local token="${CLEAR_SIGNAL_WARM_START_WORKER_TOKEN:-}"
  local expected_identity="${CLEAR_SIGNAL_WARM_START_WORKER_IDENTITY:-}"
  local observed extra actual_identity link_count parent_argv=()
  [[ "$token" =~ ^[0-9a-f]{64}$ && "$expected_identity" =~ ^[0-9]+:[0-9]+$ ]] ||
    fail "private worker capability absent"
  [[ -e /proc/$$/fd/8 ]] || fail "private worker did not inherit capability descriptor"
  actual_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/8)"
  link_count="$(stat -Lc '%h' -- /proc/$$/fd/8)"
  [[ "$actual_identity" == "$expected_identity" && "$link_count" == 0 ]] ||
    fail "private worker capability identity/lifetime mismatch"
  IFS= read -r observed <&8 || fail "private worker capability is unreadable"
  if IFS= read -r extra <&8; then fail "private worker capability has trailing records"; fi
  [[ "$observed" == "$token" ]] || fail "private worker capability token mismatch"
  exec 8<&-
  unset CLEAR_SIGNAL_WARM_START_WORKER_TOKEN CLEAR_SIGNAL_WARM_START_WORKER_IDENTITY token observed extra
  mapfile -d '' -t parent_argv < "/proc/${PPID}/cmdline"
  [[ "${#parent_argv[@]}" == 6 && "${parent_argv[0]##*/}" == timeout &&
     "${parent_argv[1]}" == --signal=TERM &&
     "${parent_argv[2]}" == "--kill-after=${TERM_GRACE_SECONDS}s" &&
     "${parent_argv[3]}" == "${WORKER_TIMEOUT_SECONDS}s" &&
     "${parent_argv[4]}" == "$RUNNER" && "${parent_argv[5]}" == --private-worker ]] ||
    fail "private worker is not supervised by the fixed timeout"
  [[ -e /proc/$$/fd/9 ]] || fail "private worker did not inherit execution lock"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/9)" == "$(stat -Lc '%d:%i' -- "$LOCK")" ]] ||
    fail "private worker execution lock identity mismatch"
  flock -n 9 || fail "private worker does not own inherited execution lock"
  local fd authority_path
  while IFS='|' read -r fd authority_path; do
    [[ -e "/proc/$$/fd/${fd}" ]] || fail "private worker did not inherit authority lock FD ${fd}"
    [[ "$(stat -Lc '%d:%i' -- "/proc/$$/fd/${fd}")" == "$(stat -Lc '%d:%i' -- "$authority_path")" ]] ||
      fail "private worker authority lock identity mismatch: ${authority_path}"
    flock -s -n "$fd" || fail "private worker lost shared authority lock: ${authority_path}"
  done <<EOF
3|${CAPTURE_LOCK}
4|${LOCALIZATION_SOURCE_LOCK}
5|${LOCALIZATION_V3_LOCK}
6|${PHASE2B_RESULT_LOCK}
7|${PHASE2B_REPORT_LOCK}
EOF
}

emit_science_complete() {
  local candidate="${SCRATCH}/science.complete.status.$$"
  {
    echo "schema_id=synthetic_v2_frozen_representation_affine_warm_start_stability_science_complete_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "evaluator_started_receipt_sha256=$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
    echo "evaluator_returned_receipt_sha256=$(sha256 "$EVALUATOR_RETURNED_RECEIPT")"
    echo "evaluator_log_sha256=$(sha256 "$EVALUATOR_LOG")"
    echo "report_sha256=$(sha256 "$REPORT")"
    echo "evaluator_invocations_completed=1"
    echo "optimizer_fits_completed=1"
    echo "optimizer_steps_completed=${OPTIMIZER_STEPS}"
    echo "maximum_anchor_read=2815"
  } > "$candidate"
  publish_once "$candidate" "$SCIENCE_COMPLETE_RECEIPT" 0444
}

validate_science_complete() {
  require_file "$SCIENCE_COMPLETE_RECEIPT" 444
  load_kv_file "$SCIENCE_COMPLETE_RECEIPT"
  expect "$SCIENCE_COMPLETE_RECEIPT" schema_id synthetic_v2_frozen_representation_affine_warm_start_stability_science_complete_v1
  expect "$SCIENCE_COMPLETE_RECEIPT" status complete
  expect "$SCIENCE_COMPLETE_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$SCIENCE_COMPLETE_RECEIPT" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$SCIENCE_COMPLETE_RECEIPT" evaluator_started_receipt_sha256 "$(sha256 "$EVALUATOR_STARTED_RECEIPT")"
  expect "$SCIENCE_COMPLETE_RECEIPT" evaluator_returned_receipt_sha256 "$(sha256 "$EVALUATOR_RETURNED_RECEIPT")"
  expect "$SCIENCE_COMPLETE_RECEIPT" evaluator_log_sha256 "$(sha256 "$EVALUATOR_LOG")"
  expect "$SCIENCE_COMPLETE_RECEIPT" report_sha256 "$(sha256 "$REPORT")"
  expect "$SCIENCE_COMPLETE_RECEIPT" evaluator_invocations_completed 1
  expect "$SCIENCE_COMPLETE_RECEIPT" optimizer_fits_completed 1
  expect "$SCIENCE_COMPLETE_RECEIPT" optimizer_steps_completed "$OPTIMIZER_STEPS"
  expect "$SCIENCE_COMPLETE_RECEIPT" maximum_anchor_read 2815
}

private_worker() {
  local evaluator_exit=0
  authorize_private_worker
  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  trap 'exit 131' QUIT
  preflight_science
  validate_build_receipt
  validate_attempt
  [[ ! -e "$EVALUATOR_STARTED_RECEIPT" && ! -e "$EVALUATOR_RETURNED_RECEIPT" && ! -e "$SCIENCE_COMPLETE_RECEIPT" &&
     ! -e "$REPORT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] ||
    fail "scientific lifecycle is not pristine"
  [[ ! -e "$REPORT_CANDIDATE" && ! -e "$EVALUATOR_LOG_CANDIDATE" ]] ||
    fail "scientific candidate paths are not pristine"

  emit_evaluator_started
  set +e
  "$BIN" --development-only --train-input "$TRAIN" --validation-input "$VALIDATION" \
    --output "$REPORT_CANDIDATE" > "$EVALUATOR_LOG_CANDIDATE" 2>&1
  evaluator_exit=$?
  set -e
  emit_evaluator_returned "$evaluator_exit"
  (( evaluator_exit == 0 )) || return "$evaluator_exit"

  require_file "$REPORT_CANDIDATE" 600
  require_file "$EVALUATOR_LOG_CANDIDATE" 600
  chmod 0444 -- "$REPORT_CANDIDATE" "$EVALUATOR_LOG_CANDIDATE"
  validate_report "$REPORT_CANDIDATE"
  publish_once "$EVALUATOR_LOG_CANDIDATE" "$EVALUATOR_LOG" 0444
  publish_once "$REPORT_CANDIDATE" "$REPORT" 0444
  validate_report "$REPORT"
  validate_attempt
  validate_evaluator_lifecycle
  emit_science_complete
  validate_science_complete
}

scan_for_live_protocol_processes() {
  local proc pid command
  for proc in /proc/[0-9]*; do
    pid="${proc##*/}"
    [[ "$pid" == "$$" || "$pid" == "$PPID" || ! -r "${proc}/cmdline" ]] && continue
    command="$(tr '\0' ' ' < "${proc}/cmdline" 2>/dev/null || true)"
    [[ "$command" != *frozen_representation_affine_warm_start_stability* ]] ||
      fail "warm-start evaluator or worker remains active: pid=${pid}"
  done
}

create_private_capability() {
  local token="$1"
  RUN_CAPABILITY_PATH="$CAPABILITY_CANDIDATE"
  [[ ! -e "$RUN_CAPABILITY_PATH" && ! -L "$RUN_CAPABILITY_PATH" ]] ||
    fail "private worker capability path is not pristine"
  (set -o noclobber; printf '%s\n' "$token" > "$RUN_CAPABILITY_PATH") 2>/dev/null ||
    fail "could not create private worker capability"
  chmod 0400 -- "$RUN_CAPABILITY_PATH"
  require_file "$RUN_CAPABILITY_PATH" 400
  exec 8< "$RUN_CAPABILITY_PATH"
  RUN_CAPABILITY_IDENTITY="$(stat -Lc '%d:%i' -- "$RUN_CAPABILITY_PATH")"
  rm -f -- "$RUN_CAPABILITY_PATH"
  RUN_CAPABILITY_PATH=""
  [[ ! -e "$CAPABILITY_CANDIDATE" && "$(stat -Lc '%h' -- /proc/$$/fd/8)" == 0 ]] ||
    fail "private worker capability was not unlinked"
}

publish_worker_log() {
  require_file "$WORKER_LOG_CANDIDATE" 600
  publish_once "$WORKER_LOG_CANDIDATE" "$WORKER_LOG" 0444
}

run_development() {
  local token capability_identity rc pending_signal pending_exit
  preflight_compile
  ensure_runtime_and_lock
  open_execution_lock_exclusive
  acquire_authority_locks
  [[ ! -e "$TERMINAL" ]] || fail "protocol is terminally invalid; retry is forbidden"
  if [[ -e "$RESULT" ]]; then
    validate_result "$RESULT"
    echo "[clear-signal:warm-start-stability] existing development result verified"
    return
  fi
  if [[ -e "$ATTEMPT" ]]; then
    seal_terminal not_available stale_attempt_recovery previous_execution_interrupted_after_attempt
    fail "the sole development attempt was already consumed and is now terminal; retry/resume forbidden"
  fi
  [[ -e "$BUILD_RECEIPT" && ! -L "$BUILD_RECEIPT" ]] ||
    fail "compile-only preparation is required before development execution"
  preflight_science
  validate_build_receipt
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch is not pristine"
  [[ ! -e "$WORKER_LOG" && ! -e "$EVALUATOR_LOG" && ! -e "$REPORT" &&
     ! -e "$REJECTED_REPORT" && ! -e "$REJECTED_RESULT" &&
     ! -e "$EVALUATOR_STARTED_RECEIPT" && ! -e "$EVALUATOR_RETURNED_RECEIPT" &&
     ! -e "$SCIENCE_COMPLETE_RECEIPT" ]] || fail "runtime is not pristine"

  RUN_EXIT_GUARD_ACTIVE=1
  trap 'run_exit_guard "$?"' EXIT
  trap 'run_signal HUP 129' HUP
  trap 'run_signal INT 130' INT
  trap 'run_signal TERM 143' TERM
  trap 'run_signal QUIT 131' QUIT

  RUN_LAUNCHING=1
  emit_attempt
  validate_attempt
  token="$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')"
  [[ "$token" =~ ^[0-9a-f]{64}$ ]] || fail "failed to generate private worker capability"
  create_private_capability "$token"
  capability_identity="$RUN_CAPABILITY_IDENTITY"
  [[ "$capability_identity" =~ ^[0-9]+:[0-9]+$ ]] || fail "invalid private worker capability identity"
  [[ -z "$(jobs -pr)" ]] || fail "pre-existing background job is not allowed"

  set +e
  CLEAR_SIGNAL_WARM_START_WORKER_TOKEN="$token" \
  CLEAR_SIGNAL_WARM_START_WORKER_IDENTITY="$capability_identity" \
    setsid timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" "${WORKER_TIMEOUT_SECONDS}s" \
      "$RUNNER" --private-worker > "$WORKER_LOG_CANDIDATE" 2>&1 &
  RUN_TIMEOUT_PID=$!
  RUN_LAUNCHING=0
  if [[ -n "$RUN_PENDING_SIGNAL" ]]; then
    pending_signal="${RUN_PENDING_SIGNAL%%:*}"
    pending_exit="${RUN_PENDING_SIGNAL##*:}"
    run_signal "$pending_signal" "$pending_exit"
  fi
  wait "$RUN_TIMEOUT_PID"
  rc=$?
  RUN_CHILD_EXIT="$rc"
  RUN_TIMEOUT_PID=""
  set -e
  [[ -z "$(jobs -pr)" ]] || fail "bounded worker was not fully reaped"
  cleanup_capability
  unset token capability_identity

  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  trap 'exit 131' QUIT

  if (( rc != 0 )); then
    if [[ -f "$ATTEMPT" && ! -e "$TERMINAL" ]]; then
      RUN_FAILURE_STAGE="evaluator_execution_or_precondition"
      RUN_FAILURE_REASON="bounded_worker_nonzero_or_incomplete"
      seal_terminal "$rc" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON"
      RUN_EXIT_GUARD_ACTIVE=0
      trap - EXIT HUP INT TERM QUIT
      fail "terminal development attempt failed (${rc}); retry forbidden"
    fi
    RUN_EXIT_GUARD_ACTIVE=0
    trap - EXIT HUP INT TERM QUIT
    fail "bounded worker failed (${rc}) but required attempt lifecycle evidence is missing or corrupt"
  fi

  RUN_FAILURE_STAGE="post_worker_validation"
  RUN_FAILURE_REASON="post_worker_validation_failure"
  scan_for_live_protocol_processes
  publish_worker_log
  validate_attempt
  validate_evaluator_lifecycle
  validate_report "$REPORT"
  validate_science_complete
  emit_result
  echo "[clear-signal:warm-start-stability] complete: ${RESULT}" || true
}

plan() {
  preflight_compile
  echo "Project Clear Signal — affine warm-start stability"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=development_only"
  echo "attempt_ordinal=1"
  echo "train_range=[0,2496)"
  echo "validation_range=[2560,2816)"
  echo "maximum_anchor_read=2815"
  echo "model=96_to_128_to_128_to_9_paired_gelu_affine_warm_start"
  echo "optimizer_seed=${OPTIMIZER_SEED}"
  echo "optimizer_steps=${OPTIMIZER_STEPS}"
  echo "batch_size=${BATCH_SIZE}"
  echo "batch_schedule_fingerprint=${SCHEDULE_FINGERPRINT}"
  echo "step0_parity_tolerance=${STEP0_PARITY_TOLERANCE}"
  echo "step0_parity_failure_terminal=true"
  echo "duplicate_mse_pair_count=2"
  echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOLERANCE}"
  echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOLERANCE}"
  echo "expected_complete_report_key_count=${COMPLETE_REPORT_KEY_COUNT}"
  echo "train_aggregate_mse_ratio_limit=${TRAIN_AGGREGATE_MSE_RATIO_MAX}"
  echo "train_each_head_mse_ratio_limit=${TRAIN_HEAD_MSE_RATIO_MAX}"
  echo "train_metric_deficit_limit=${TRAIN_METRIC_DEFICIT_MAX}"
  echo "train_rmse_ratio_increase_limit=${TRAIN_RMSE_RATIO_INCREASE_MAX}"
  echo "validation_metric_deficit_limit=${VALIDATION_METRIC_DEFICIT_MAX}"
  echo "validation_rmse_ratio_increase_limit=${VALIDATION_RMSE_RATIO_INCREASE_MAX}"
  echo "clear_stop_aggregate_mse_ratio_threshold=${CLEAR_STOP_AGGREGATE_MSE_RATIO_MIN}"
  echo "clear_stop_maximum_head_mse_ratio_threshold=${CLEAR_STOP_HEAD_MSE_RATIO_MIN}"
  echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
  echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
  echo "evaluator_invocation_limit=1"
  echo "optimizer_fit_limit=1"
  echo "retry_allowed=false"
  echo "resume_allowed=false"
  echo "validation_read_by_trainer=false"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_access=false"
  echo "build_prepared=$([[ -e "$BUILD_RECEIPT" ]] && echo true || echo false)"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "terminal_invalid=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
}

verify_development() {
  preflight_compile
  require_private_dir "$RUNTIME"
  require_private_dir "$BUILD_DIR"
  require_private_dir "$SCRATCH"
  open_execution_lock_shared
  acquire_authority_locks
  [[ -e "$RESULT" && ! -e "$TERMINAL" ]] || fail "unique development result is unavailable"
  validate_result "$RESULT"
  scan_for_live_protocol_processes
  echo "[clear-signal:warm-start-stability] development result verified read-only"
}

main() {
  [[ $# == 1 ]] || fail "usage: $0 --plan|--prepare|--run-development|--verify-development"
  case "$1" in
    --plan) plan ;;
    --prepare) prepare ;;
    --run-development) run_development ;;
    --verify-development) verify_development ;;
    --private-worker) private_worker ;;
    *) fail "unsupported mode: $1" ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then main "$@"; fi
