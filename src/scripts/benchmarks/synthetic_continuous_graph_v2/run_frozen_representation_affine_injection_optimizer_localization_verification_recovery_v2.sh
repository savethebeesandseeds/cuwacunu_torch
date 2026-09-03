#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v2"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_VERIFICATION_RECOVERY_V2_PREREGISTRATION.md"
readonly PREREG_SHA="fbde44e0dfc3eb3758744b8a96e4760e4a7c00e159bbda904c80c9614441766a"
readonly VALIDATOR="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_injection_optimizer_localization_verification_recovery_v2_validator.awk"
readonly VALIDATOR_SHA="2b819f54ff1df0d1ccb39dbd5538cecad31ae957f8224ebb588671129807ec76"
readonly AWK_BIN="/usr/bin/mawk"
readonly AWK_BIN_SHA="301315e7e2e964b4e403824b3f6c7ad8db1023e4ce87e6f6c92bf367e047f311"

readonly SOURCE_PROTOCOL_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1"
readonly SOURCE_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${SOURCE_PROTOCOL_ID}"
readonly SOURCE_LOCK="${SOURCE_ROOT}/.execution.lock"
readonly SOURCE_ATTEMPT="${SOURCE_ROOT}/attempt.status"
readonly SOURCE_ATTEMPT_SHA="ec49afdec429f8937fcd6099c8be222ccdd3bb7870003ec48d55a725c51bf7a6"
readonly SOURCE_TERMINAL="${SOURCE_ROOT}/terminal.invalid.status"
readonly SOURCE_TERMINAL_SHA="fd0bcbe4cdda0ac8b77ccd0c30b409704d40a6a7c4cf8fa88108b7e830d395a2"
readonly SOURCE_REPORT="${SOURCE_ROOT}/rejected.development.report"
readonly SOURCE_REPORT_SHA="5b1ebcc7af65792074e653406a1a6f4120dc9ad4105adca5cbca90f6c5815f30"
readonly SOURCE_LOG="${SOURCE_ROOT}/evaluator.log"
readonly SOURCE_LOG_SHA="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
readonly SOURCE_BUILD_RECEIPT="${SOURCE_ROOT}/build/build.status"
readonly SOURCE_BUILD_RECEIPT_SHA="973f7dd179e76c915f0554c3980e658d4229125a81fbfe1f936cfad25b310e3c"
readonly SOURCE_BINARY="${SOURCE_ROOT}/build/frozen_representation_affine_injection_optimizer_localization"
readonly SOURCE_BINARY_SHA="b04f380db3cec472ae2ef589664d38a632b92779ccfcbbfd6c45005dbe20f801"
readonly SOURCE_RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_frozen_representation_affine_injection_optimizer_localization_v1.sh"
readonly SOURCE_RUNNER_SHA="97eaebc2b0dc609acc75ffa09fcd4569bd83fcdfe8c3255e6eab67b77e314afa"
readonly SOURCE_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_PREREGISTRATION.md"
readonly SOURCE_PREREG_SHA="5c86fcb55b10e52ab322d271c0117f6184402a7d5234a32e13048237d7056b09"
readonly SOURCE_EVALUATOR="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_injection_optimizer_localization_probe.cpp"
readonly SOURCE_EVALUATOR_SHA="7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b"
readonly SOURCE_BUILD_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_representation_affine_injection_optimizer_localization_probe.sh"
readonly SOURCE_BUILD_WRAPPER_SHA="2e5b39981302d55f8785389c4b01cb6dd4b38036d6d45ca10a798c69567004fd"

readonly V1_PROTOCOL_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v1"
readonly V1_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${V1_PROTOCOL_ID}"
readonly V1_LOCK="${V1_ROOT}/.execution.lock"
readonly V1_RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_frozen_representation_affine_injection_optimizer_localization_verification_recovery_v1.sh"
readonly V1_RUNNER_SHA="2f25692f0c9376da4902194cfabfd0c82b9d7ca72346224873531ffb4a9cdda8"
readonly V1_PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_VERIFICATION_RECOVERY_PREREGISTRATION.md"
readonly V1_PREREG_SHA="c9a8887aecb4b54e73b262ef50d7133bfce3c2279a202203f4b75221985727b4"
readonly V1_ATTEMPT="${V1_ROOT}/verification.attempt.status"
readonly V1_ATTEMPT_SHA="4d2cc6ac4d4be5282fbe596f7eaafe557606c3d37acbd18d6cb874921b1b861c"
readonly V1_LOG="${V1_ROOT}/verification.log"
readonly V1_LOG_SHA="a020f06cc7ad27d46f02a87e96fa95aded90e6787da1487a37eea928d0d370b7"
readonly V1_TERMINAL="${V1_ROOT}/terminal.invalid.status"
readonly V1_TERMINAL_SHA="660e6c6396828630092243ba1fd569a9b935aa6d2cc46863b3fb3c73b36786db"

readonly TRAIN_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly VALIDATION_SHA="8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd"
readonly PHASE2A_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1"
readonly PHASE2A_RECEIPT="${PHASE2A_ROOT}/development.status"
readonly PHASE2A_RECEIPT_SHA="b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5"
readonly PHASE2A_MAIN="${PHASE2A_ROOT}/main/development.report"
readonly PHASE2A_REPLAY="${PHASE2A_ROOT}/replay/development.report"
readonly PHASE2A_REPORT_SHA="2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a"
readonly PHASE2B_RESULT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_v1/development.status"
readonly PHASE2B_RESULT_SHA="cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418"
readonly PHASE2B_REPORT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1/nonlinear/development.report"
readonly PHASE2B_REPORT_SHA="34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36"

readonly RUNTIME="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly SCRATCH="${RUNTIME}/.scratch"
readonly LOCK="${RUNTIME}/.execution.lock"
readonly SELF_TEST_LOG="${RUNTIME}/validator.syntax_self_test.log"
readonly SELF_TEST_RECEIPT="${RUNTIME}/validator.syntax_self_test.status"
readonly ATTEMPT="${RUNTIME}/verification.attempt.status"
readonly VERIFICATION_LOG="${RUNTIME}/verification.log"
readonly VERIFICATION_RECEIPT="${RUNTIME}/verification.complete.status"
readonly RESULT="${RUNTIME}/development.status"
readonly TERMINAL="${RUNTIME}/terminal.invalid.status"

readonly SELF_TEST_TIMEOUT_SECONDS=5
readonly SELF_TEST_GRACE_SECONDS=1
readonly VERIFY_TIMEOUT_SECONDS=30
readonly VERIFY_GRACE_SECONDS=5
readonly DUPLICATE_MSE_ABS_TOL="1.25e-7"
readonly DUPLICATE_MSE_REL_TOL="1.25e-7"

declare -A KV_VALUE=()
declare -A KV_PRESENT=()
declare -A KV_LOADED=()

ACTIVE=0
STAGE="pre_attempt"
CHILD_KIND="none"
CHILD_STARTED=0
CHILD_EXIT="not_started"
LOG_CANDIDATE=""
CURRENT_LOG_DEST=""
TIMEOUT_PID=""
TIMEOUT_LAUNCHING=0
TIMEOUT_PREVIOUS_ASYNC_PID=""
PENDING_SIGNAL=""
PENDING_SIGNAL_EXIT=""

fail() {
  echo "[clear-signal:optimizer-localization-recovery-v2] ERROR: $*" >&2
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
  [[ "$mode" == 700 && "$uid" == 0 ]] || fail "invalid private-directory metadata: ${path} (${mode}:${uid})"
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

require_key() { kv "$1" "$2" >/dev/null; }

publish_once() {
  local candidate="$1" destination="$2"
  [[ -f "$candidate" && ! -L "$candidate" && ! -e "$destination" && ! -L "$destination" ]] ||
    fail "unsafe publication: ${candidate} -> ${destination}"
  chmod 0444 -- "$candidate"
  require_file "$candidate" 444
  mv -T -n -- "$candidate" "$destination"
  [[ ! -e "$candidate" && -f "$destination" && ! -L "$destination" ]] ||
    fail "atomic no-clobber publication failed: ${destination}"
  require_file "$destination" 444
}

preflight_local() {
  require_file "$RUNNER" 555
  require_exact "$PREREG" "$PREREG_SHA" 444
  require_exact "$VALIDATOR" "$VALIDATOR_SHA" 444
  require_exact "$AWK_BIN" "$AWK_BIN_SHA" 755
  require_file "$SOURCE_LOCK" 600
  require_file "$V1_LOCK" 600
}

preflight_mechanical_files() {
  local path digest mode
  while IFS='|' read -r path digest mode; do
    require_exact "$path" "$digest" "$mode"
  done <<EOF
${RUNNER}|$(sha256 "$RUNNER")|555
${PREREG}|${PREREG_SHA}|444
${VALIDATOR}|${VALIDATOR_SHA}|444
${AWK_BIN}|${AWK_BIN_SHA}|755
${V1_RUNNER}|${V1_RUNNER_SHA}|555
${V1_PREREG}|${V1_PREREG_SHA}|444
${V1_ATTEMPT}|${V1_ATTEMPT_SHA}|444
${V1_LOG}|${V1_LOG_SHA}|444
${V1_TERMINAL}|${V1_TERMINAL_SHA}|444
EOF
  require_file "$SOURCE_LOCK" 600
  require_file "$V1_LOCK" 600
  [[ "$(wc -l < "$V1_LOG")" == 5 ]] || fail "V1 syntax-error log line count mismatch"
}

preflight_files() {
  local path digest mode
  preflight_mechanical_files
  while IFS='|' read -r path digest mode; do
    require_exact "$path" "$digest" "$mode"
  done <<EOF
${SOURCE_ATTEMPT}|${SOURCE_ATTEMPT_SHA}|444
${SOURCE_TERMINAL}|${SOURCE_TERMINAL_SHA}|444
${SOURCE_REPORT}|${SOURCE_REPORT_SHA}|444
${SOURCE_LOG}|${SOURCE_LOG_SHA}|444
${SOURCE_RUNNER}|${SOURCE_RUNNER_SHA}|555
${SOURCE_PREREG}|${SOURCE_PREREG_SHA}|444
${SOURCE_EVALUATOR}|${SOURCE_EVALUATOR_SHA}|444
${SOURCE_BUILD_WRAPPER}|${SOURCE_BUILD_WRAPPER_SHA}|555
${SOURCE_BUILD_RECEIPT}|${SOURCE_BUILD_RECEIPT_SHA}|444
${SOURCE_BINARY}|${SOURCE_BINARY_SHA}|555
${PHASE2A_RECEIPT}|${PHASE2A_RECEIPT_SHA}|444
${PHASE2A_MAIN}|${PHASE2A_REPORT_SHA}|444
${PHASE2A_REPLAY}|${PHASE2A_REPORT_SHA}|444
${PHASE2B_RESULT}|${PHASE2B_RESULT_SHA}|444
${PHASE2B_REPORT}|${PHASE2B_REPORT_SHA}|444
EOF
  [[ "$(stat -c '%s' -- "$SOURCE_LOG")" == 0 ]] || fail "source evaluator log is not empty"
}

preflight_lineage() {
  local path
  preflight_files
  for path in "$SOURCE_ATTEMPT" "$SOURCE_TERMINAL" "$SOURCE_BUILD_RECEIPT" \
              "$V1_ATTEMPT" "$V1_TERMINAL" "$PHASE2A_RECEIPT" \
              "$PHASE2B_RESULT" "$PHASE2B_REPORT"; do
    load_kv_file "$path"
  done

  expect "$SOURCE_ATTEMPT" status consumed
  expect "$SOURCE_ATTEMPT" protocol_id "$SOURCE_PROTOCOL_ID"
  expect "$SOURCE_ATTEMPT" runner_sha256 "$SOURCE_RUNNER_SHA"
  expect "$SOURCE_ATTEMPT" preregistration_sha256 "$SOURCE_PREREG_SHA"
  expect "$SOURCE_ATTEMPT" evaluator_source_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$SOURCE_ATTEMPT" build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$SOURCE_ATTEMPT" build_receipt_sha256 "$SOURCE_BUILD_RECEIPT_SHA"
  expect "$SOURCE_ATTEMPT" binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$SOURCE_ATTEMPT" train_probe_sha256 "$TRAIN_SHA"
  expect "$SOURCE_ATTEMPT" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$SOURCE_ATTEMPT" affine_oracle_grouped_fit_count 1
  expect "$SOURCE_ATTEMPT" affine_oracle_head_solve_count 9
  expect "$SOURCE_ATTEMPT" optimizer_fits_completed 1
  expect "$SOURCE_ATTEMPT" total_train_fit_procedures 2
  expect "$SOURCE_ATTEMPT" optimizer_step_limit 3500
  expect "$SOURCE_ATTEMPT" maximum_anchor_read 2815
  expect "$SOURCE_ATTEMPT" certified_input_access false
  expect "$SOURCE_ATTEMPT" final_holdout_access false
  expect "$SOURCE_ATTEMPT" policy_access false

  expect "$SOURCE_TERMINAL" status terminal_invalid
  expect "$SOURCE_TERMINAL" failure_stage report_validation
  expect "$SOURCE_TERMINAL" evaluator_started 1
  expect "$SOURCE_TERMINAL" evaluator_exit_code 0
  expect "$SOURCE_TERMINAL" attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$SOURCE_TERMINAL" evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$SOURCE_TERMINAL" rejected_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$SOURCE_TERMINAL" certified_input_access false
  expect "$SOURCE_TERMINAL" final_holdout_access false
  expect "$SOURCE_TERMINAL" policy_access false

  expect "$SOURCE_BUILD_RECEIPT" status complete
  expect "$SOURCE_BUILD_RECEIPT" source_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$SOURCE_BUILD_RECEIPT" build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$SOURCE_BUILD_RECEIPT" binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$SOURCE_BUILD_RECEIPT" compile_only true
  expect "$SOURCE_BUILD_RECEIPT" probe_access false

  expect "$V1_ATTEMPT" status consumed
  expect "$V1_ATTEMPT" protocol_id "$V1_PROTOCOL_ID"
  expect "$V1_ATTEMPT" runner_sha256 "$V1_RUNNER_SHA"
  expect "$V1_ATTEMPT" preregistration_sha256 "$V1_PREREG_SHA"
  expect "$V1_ATTEMPT" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$V1_ATTEMPT" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$V1_ATTEMPT" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$V1_ATTEMPT" source_evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$V1_ATTEMPT" duplicate_mse_pair_count 10
  expect "$V1_ATTEMPT" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$V1_ATTEMPT" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$V1_ATTEMPT" new_evaluator_invocations 0
  expect "$V1_ATTEMPT" new_fits 0
  expect "$V1_ATTEMPT" new_optimizer_steps 0
  expect "$V1_ATTEMPT" certified_input_access false
  expect "$V1_ATTEMPT" final_holdout_access false
  expect "$V1_ATTEMPT" policy_access false

  expect "$V1_TERMINAL" status terminal_invalid
  expect "$V1_TERMINAL" protocol_id "$V1_PROTOCOL_ID"
  expect "$V1_TERMINAL" failure_stage single_pass_report_verification
  expect "$V1_TERMINAL" failure_reason timeout_or_validation_failure
  expect "$V1_TERMINAL" exit_code 2
  expect "$V1_TERMINAL" attempt_consumed true
  expect "$V1_TERMINAL" worker_started 1
  expect "$V1_TERMINAL" worker_exit_code 2
  expect "$V1_TERMINAL" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$V1_TERMINAL" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$V1_TERMINAL" verification_log_sha256 "$V1_LOG_SHA"
  expect "$V1_TERMINAL" new_evaluator_invocations 0
  expect "$V1_TERMINAL" new_fits 0
  expect "$V1_TERMINAL" new_optimizer_steps 0
  expect "$V1_TERMINAL" scientific_result_available false
  expect "$V1_TERMINAL" certified_input_access false
  expect "$V1_TERMINAL" final_holdout_access false
  expect "$V1_TERMINAL" policy_access false

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
  expect "$PHASE2B_REPORT" maximum_anchor_read 2815
  expect "$PHASE2B_REPORT" representation_forward_executed false
  expect "$PHASE2B_REPORT" checkpoint_written false
  expect "$PHASE2B_REPORT" validation_read_by_trainer false
  expect "$PHASE2B_REPORT" steps_per_fit 3500
  expect "$PHASE2B_REPORT" batch_size 64
  expect "$PHASE2B_REPORT" optimizer adam
  expect "$PHASE2B_REPORT" final_holdout_access false
  expect "$PHASE2B_REPORT" policy_access false
}

acquire_lineage_locks() {
  local source_identity source_fd_identity v1_identity v1_fd_identity
  require_file "$SOURCE_LOCK" 600
  require_file "$V1_LOCK" 600
  exec 7< "$SOURCE_LOCK"
  flock -s -n 7 || fail "source runtime is active"
  exec 8< "$V1_LOCK"
  flock -s -n 8 || fail "V1 recovery runtime is active"
  source_identity="$(stat -Lc '%d:%i' -- "$SOURCE_LOCK")"
  source_fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/7)"
  v1_identity="$(stat -Lc '%d:%i' -- "$V1_LOCK")"
  v1_fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/8)"
  [[ "$source_fd_identity" == "$source_identity" ]] || fail "source lock identity mismatch"
  [[ "$v1_fd_identity" == "$v1_identity" ]] || fail "V1 lock identity mismatch"
}

ensure_runtime() {
  if [[ ! -e "$RUNTIME" && ! -L "$RUNTIME" ]]; then mkdir -m 0700 -- "$RUNTIME"; fi
  require_private_dir "$RUNTIME"
  if [[ ! -e "$SCRATCH" && ! -L "$SCRATCH" ]]; then mkdir -m 0700 -- "$SCRATCH"; fi
  require_private_dir "$SCRATCH"
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || true
    chmod 0600 -- "$LOCK"
  fi
  require_file "$LOCK" 600
}

open_lock_create() {
  ensure_runtime
  exec 9< "$LOCK"
  flock -n 9 || fail "V2 recovery is already active"
}

open_existing_lock() {
  local shared="$1"
  require_private_dir "$RUNTIME"
  require_private_dir "$SCRATCH"
  require_file "$LOCK" 600
  exec 9< "$LOCK"
  if [[ "$shared" == true ]]; then
    flock -s -n 9 || fail "V2 recovery is active"
  else
    flock -n 9 || fail "V2 recovery is already active"
  fi
}

open_optional_lock_read_only() {
  if [[ ! -e "$RUNTIME" && ! -L "$RUNTIME" ]]; then return 0; fi
  open_existing_lock true
}

emit_self_test_receipt() {
  local candidate="$1"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_validator_self_test_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "validator_path=${VALIDATOR}"
    echo "validator_sha256=${VALIDATOR_SHA}"
    echo "awk_path=${AWK_BIN}"
    echo "awk_sha256=${AWK_BIN_SHA}"
    echo "v1_runner_sha256=${V1_RUNNER_SHA}"
    echo "v1_preregistration_sha256=${V1_PREREG_SHA}"
    echo "v1_attempt_sha256=${V1_ATTEMPT_SHA}"
    echo "v1_log_sha256=${V1_LOG_SHA}"
    echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
    echo "syntax_self_test=true"
    echo "syntax_self_test_guard=END_first_statement"
    echo "input_0=/dev/null"
    echo "input_1=/dev/null"
    echo "self_test_timeout_seconds=${SELF_TEST_TIMEOUT_SECONDS}"
    echo "self_test_term_grace_seconds=${SELF_TEST_GRACE_SECONDS}"
    echo "self_test_exit_code=0"
    echo "self_test_log_path=${SELF_TEST_LOG}"
    echo "self_test_log_sha256=$(sha256 "$SELF_TEST_LOG")"
    echo "self_test_log_empty=true"
    echo "report_bytes_read=0"
    echo "probe_bytes_read=0"
    echo "scientific_attempt_consumed=false"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
}

validate_self_test_receipt() {
  local path="$1"
  load_kv_file "$path"
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_validator_self_test_v1
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$path" preregistration_sha256 "$PREREG_SHA"
  expect "$path" validator_path "$VALIDATOR"
  expect "$path" validator_sha256 "$VALIDATOR_SHA"
  expect "$path" awk_path "$AWK_BIN"
  expect "$path" awk_sha256 "$AWK_BIN_SHA"
  expect "$path" v1_runner_sha256 "$V1_RUNNER_SHA"
  expect "$path" v1_preregistration_sha256 "$V1_PREREG_SHA"
  expect "$path" v1_attempt_sha256 "$V1_ATTEMPT_SHA"
  expect "$path" v1_log_sha256 "$V1_LOG_SHA"
  expect "$path" v1_terminal_sha256 "$V1_TERMINAL_SHA"
  expect "$path" syntax_self_test true
  expect "$path" syntax_self_test_guard END_first_statement
  expect "$path" input_0 /dev/null
  expect "$path" input_1 /dev/null
  expect "$path" self_test_timeout_seconds "$SELF_TEST_TIMEOUT_SECONDS"
  expect "$path" self_test_term_grace_seconds "$SELF_TEST_GRACE_SECONDS"
  expect "$path" self_test_exit_code 0
  expect "$path" self_test_log_path "$SELF_TEST_LOG"
  expect "$path" self_test_log_sha256 "$(sha256 "$SELF_TEST_LOG")"
  expect "$path" self_test_log_empty true
  expect "$path" report_bytes_read 0
  expect "$path" probe_bytes_read 0
  expect "$path" scientific_attempt_consumed false
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
  [[ "$(stat -c '%s' -- "$SELF_TEST_LOG")" == 0 ]] || fail "self-test log is not empty"
}

emit_attempt() {
  local candidate="${SCRATCH}/verification.attempt.$$.status"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_attempt_v1"
    echo "status=consumed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_ordinal=1"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "validator_sha256=${VALIDATOR_SHA}"
    echo "awk_sha256=${AWK_BIN_SHA}"
    echo "validator_self_test_receipt_sha256=$(sha256 "$SELF_TEST_RECEIPT")"
    echo "v1_runner_sha256=${V1_RUNNER_SHA}"
    echo "v1_preregistration_sha256=${V1_PREREG_SHA}"
    echo "v1_attempt_sha256=${V1_ATTEMPT_SHA}"
    echo "v1_log_sha256=${V1_LOG_SHA}"
    echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_report_sha256=${SOURCE_REPORT_SHA}"
    echo "source_evaluator_log_sha256=${SOURCE_LOG_SHA}"
    echo "source_runner_sha256=${SOURCE_RUNNER_SHA}"
    echo "source_preregistration_sha256=${SOURCE_PREREG_SHA}"
    echo "source_evaluator_sha256=${SOURCE_EVALUATOR_SHA}"
    echo "source_build_wrapper_sha256=${SOURCE_BUILD_WRAPPER_SHA}"
    echo "source_build_receipt_sha256=${SOURCE_BUILD_RECEIPT_SHA}"
    echo "source_binary_sha256=${SOURCE_BINARY_SHA}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "phase2a_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "phase2b_result_sha256=${PHASE2B_RESULT_SHA}"
    echo "phase2b_report_sha256=${PHASE2B_REPORT_SHA}"
    echo "duplicate_mse_pair_count=10"
    echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOL}"
    echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOL}"
    echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
    echo "verification_term_grace_seconds=${VERIFY_GRACE_SECONDS}"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "probe_bytes_read_by_recovery=false"
    echo "binary_executed_by_recovery=false"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$ATTEMPT"
}

validate_attempt() {
  load_kv_file "$ATTEMPT"
  expect "$ATTEMPT" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_attempt_v1
  expect "$ATTEMPT" status consumed
  expect "$ATTEMPT" protocol_id "$PROTOCOL_ID"
  expect "$ATTEMPT" attempt_ordinal 1
  expect "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"
  expect "$ATTEMPT" validator_sha256 "$VALIDATOR_SHA"
  expect "$ATTEMPT" awk_sha256 "$AWK_BIN_SHA"
  expect "$ATTEMPT" validator_self_test_receipt_sha256 "$(sha256 "$SELF_TEST_RECEIPT")"
  expect "$ATTEMPT" v1_runner_sha256 "$V1_RUNNER_SHA"
  expect "$ATTEMPT" v1_preregistration_sha256 "$V1_PREREG_SHA"
  expect "$ATTEMPT" v1_attempt_sha256 "$V1_ATTEMPT_SHA"
  expect "$ATTEMPT" v1_log_sha256 "$V1_LOG_SHA"
  expect "$ATTEMPT" v1_terminal_sha256 "$V1_TERMINAL_SHA"
  expect "$ATTEMPT" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$ATTEMPT" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$ATTEMPT" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$ATTEMPT" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$ATTEMPT" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$ATTEMPT" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$ATTEMPT" duplicate_mse_pair_count 10
  expect "$ATTEMPT" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$ATTEMPT" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$ATTEMPT" verification_timeout_seconds "$VERIFY_TIMEOUT_SECONDS"
  expect "$ATTEMPT" verification_term_grace_seconds "$VERIFY_GRACE_SECONDS"
  expect "$ATTEMPT" import_only true
  expect "$ATTEMPT" no_new_scientific_execution true
  expect "$ATTEMPT" probe_bytes_read_by_recovery false
  expect "$ATTEMPT" binary_executed_by_recovery false
  expect "$ATTEMPT" new_evaluator_invocations 0
  expect "$ATTEMPT" new_fits 0
  expect "$ATTEMPT" new_optimizer_steps 0
  expect "$ATTEMPT" retry_allowed false
  expect "$ATTEMPT" resume_allowed false
  expect "$ATTEMPT" certified_input_access false
  expect "$ATTEMPT" final_holdout_access false
  expect "$ATTEMPT" policy_access false
}

authorize_worker() {
  local attempt_sha="$1" fd identity
  local -a parent_argv=()
  mapfile -d '' -t parent_argv < "/proc/${PPID}/cmdline"
  [[ "${#parent_argv[@]}" == 7 ]] || fail "verification worker parent argv length mismatch"
  [[ "${parent_argv[0]##*/}" == timeout ]] || fail "verification worker parent is not GNU timeout"
  [[ "${parent_argv[1]}" == --signal=TERM ]] || fail "verification worker signal mismatch"
  [[ "${parent_argv[2]}" == "--kill-after=${VERIFY_GRACE_SECONDS}s" ]] || fail "verification worker grace mismatch"
  [[ "${parent_argv[3]}" == "${VERIFY_TIMEOUT_SECONDS}s" ]] || fail "verification worker timeout mismatch"
  [[ "${parent_argv[4]}" == "$RUNNER" && "${parent_argv[5]}" == --verification-worker &&
     "${parent_argv[6]}" == "$attempt_sha" ]] || fail "verification worker command mismatch"
  for fd in 7 8 9; do [[ -e "/proc/$$/fd/${fd}" ]] || fail "verification worker lacks fd ${fd}"; done
  identity="$(stat -Lc '%d:%i' -- "$SOURCE_LOCK")"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/7)" == "$identity" ]] || fail "source lock identity mismatch"
  identity="$(stat -Lc '%d:%i' -- "$V1_LOCK")"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/8)" == "$identity" ]] || fail "V1 lock identity mismatch"
  identity="$(stat -Lc '%d:%i' -- "$LOCK")"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/9)" == "$identity" ]] || fail "V2 lock identity mismatch"
}

verification_worker() {
  [[ $# == 1 ]] || fail "verification worker requires the attempt SHA-256"
  authorize_worker "$1"
  preflight_lineage
  validate_self_test_receipt "$SELF_TEST_RECEIPT"
  validate_attempt
  [[ "$(sha256 "$ATTEMPT")" == "$1" ]] || fail "verification attempt capability mismatch"
  "$AWK_BIN" \
    -v source_report_sha="$SOURCE_REPORT_SHA" \
    -v phase2a_report_sha="$PHASE2A_REPORT_SHA" \
    -v abs_tol="$DUPLICATE_MSE_ABS_TOL" \
    -v rel_tol="$DUPLICATE_MSE_REL_TOL" \
    -f "$VALIDATOR" "$PHASE2A_MAIN" "$SOURCE_REPORT"
  [[ "$(sha256 "$VALIDATOR")" == "$VALIDATOR_SHA" ]] || fail "validator changed during verification"
  [[ "$(sha256 "$SOURCE_REPORT")" == "$SOURCE_REPORT_SHA" ]] || fail "source report changed during verification"
  [[ "$(sha256 "$PHASE2A_MAIN")" == "$PHASE2A_REPORT_SHA" ]] || fail "Phase2A report changed during verification"
}

validate_worker_log() {
  local path="$1"
  load_kv_file "$path"
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_worker_v1
  expect "$path" status verified
  expect "$path" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$path" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$path" full_report_validation_pass true
  expect "$path" source_report_line_count 490
  expect "$path" phase2a_report_line_count 234
  expect "$path" duplicate_mse_pair_count 10
  expect "$path" duplicate_mse_strict_1e-12_failure_count 10
  expect "$path" duplicate_mse_mixed_tolerance_pass_count 10
  expect "$path" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$path" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$path" classification float32_conditioning_failure
  expect "$path" classification_order_preserved true
  expect "$path" direct_float32_parity_pass false
  expect "$path" paired_gelu_parity_pass true
  expect "$path" direct_linear_adam_recovery_gate_pass false
  expect "$path" direct_linear_adam_clear_failure_gate_pass true
  expect "$path" direct_float32_aggregate_forecast_metrics_within_0.001 true
  expect "$path" probe_bytes_read_by_recovery false
  expect "$path" binary_executed_by_recovery false
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
  require_key "$path" duplicate_mse_maximum_absolute_discrepancy
  require_key "$path" duplicate_mse_maximum_relative_discrepancy
  require_key "$path" duplicate_mse_maximum_tolerance_fraction
}

emit_verification_receipt() {
  local candidate="$1"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_complete_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "validator_sha256=${VALIDATOR_SHA}"
    echo "awk_sha256=${AWK_BIN_SHA}"
    echo "validator_self_test_receipt_sha256=$(sha256 "$SELF_TEST_RECEIPT")"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
    echo "v1_attempt_sha256=${V1_ATTEMPT_SHA}"
    echo "v1_log_sha256=${V1_LOG_SHA}"
    echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
    echo "source_report_sha256=${SOURCE_REPORT_SHA}"
    echo "phase2a_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "full_report_validation_pass=true"
    echo "classification=$(kv "$VERIFICATION_LOG" classification)"
    echo "duplicate_mse_pair_count=$(kv "$VERIFICATION_LOG" duplicate_mse_pair_count)"
    echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOL}"
    echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOL}"
    echo "duplicate_mse_strict_1e-12_failure_count=$(kv "$VERIFICATION_LOG" duplicate_mse_strict_1e-12_failure_count)"
    echo "duplicate_mse_mixed_tolerance_pass_count=$(kv "$VERIFICATION_LOG" duplicate_mse_mixed_tolerance_pass_count)"
    echo "duplicate_mse_maximum_absolute_discrepancy=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_absolute_discrepancy)"
    echo "duplicate_mse_maximum_relative_discrepancy=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_relative_discrepancy)"
    echo "duplicate_mse_maximum_tolerance_fraction=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_tolerance_fraction)"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "probe_bytes_read_by_recovery=false"
    echo "binary_executed_by_recovery=false"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
}

validate_verification_receipt() {
  local path="$1"
  load_kv_file "$path"
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_complete_v1
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$path" preregistration_sha256 "$PREREG_SHA"
  expect "$path" validator_sha256 "$VALIDATOR_SHA"
  expect "$path" awk_sha256 "$AWK_BIN_SHA"
  expect "$path" validator_self_test_receipt_sha256 "$(sha256 "$SELF_TEST_RECEIPT")"
  expect "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect "$path" v1_attempt_sha256 "$V1_ATTEMPT_SHA"
  expect "$path" v1_log_sha256 "$V1_LOG_SHA"
  expect "$path" v1_terminal_sha256 "$V1_TERMINAL_SHA"
  expect "$path" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$path" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$path" full_report_validation_pass true
  expect "$path" classification float32_conditioning_failure
  expect "$path" duplicate_mse_pair_count 10
  expect "$path" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$path" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$path" duplicate_mse_strict_1e-12_failure_count 10
  expect "$path" duplicate_mse_mixed_tolerance_pass_count 10
  expect "$path" import_only true
  expect "$path" no_new_scientific_execution true
  expect "$path" probe_bytes_read_by_recovery false
  expect "$path" binary_executed_by_recovery false
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
}

emit_result() {
  local candidate="$1" split_id field
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_development_receipt_v2"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "diagnostic_authority=development_only"
    echo "benchmark_acceptance_authority=false"
    echo "scientific_result_available=true"
    echo "recovery_kind=immutable_predecessor_report_verification_parse_fix"
    echo "classification=$(kv "$VERIFICATION_LOG" classification)"
    echo "classification_order_preserved=$(kv "$VERIFICATION_LOG" classification_order_preserved)"
    echo "direct_float32_parity_pass=$(kv "$VERIFICATION_LOG" direct_float32_parity_pass)"
    echo "paired_gelu_parity_pass=$(kv "$VERIFICATION_LOG" paired_gelu_parity_pass)"
    echo "direct_linear_adam_recovery_gate_pass=$(kv "$VERIFICATION_LOG" direct_linear_adam_recovery_gate_pass)"
    echo "direct_linear_adam_clear_failure_gate_pass=$(kv "$VERIFICATION_LOG" direct_linear_adam_clear_failure_gate_pass)"
    echo "direct_linear_adam_to_oracle_train_standardized_mse_ratio=$(kv "$VERIFICATION_LOG" direct_linear_adam_to_oracle_train_standardized_mse_ratio)"
    echo "direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio=$(kv "$VERIFICATION_LOG" direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio)"
    echo "direct_float32_aggregate_forecast_metric_maximum_absolute_delta=$(kv "$VERIFICATION_LOG" direct_float32_aggregate_forecast_metric_maximum_absolute_delta)"
    echo "direct_float32_aggregate_forecast_metrics_within_0.001=$(kv "$VERIFICATION_LOG" direct_float32_aggregate_forecast_metrics_within_0.001)"
    for split_id in train validation; do
      for field in directional_accuracy pairwise_rank_accuracy correlation rmse_target_rms_ratio; do
        echo "float64_oracle.${split_id}.${field}=$(kv "$VERIFICATION_LOG" "float64_oracle.${split_id}.${field}")"
        echo "direct_float32.${split_id}.${field}=$(kv "$VERIFICATION_LOG" "direct_float32.${split_id}.${field}")"
      done
    done
    echo "duplicate_mse_pair_count=$(kv "$VERIFICATION_LOG" duplicate_mse_pair_count)"
    echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOL}"
    echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOL}"
    echo "duplicate_mse_strict_1e-12_failure_count=$(kv "$VERIFICATION_LOG" duplicate_mse_strict_1e-12_failure_count)"
    echo "duplicate_mse_mixed_tolerance_pass_count=$(kv "$VERIFICATION_LOG" duplicate_mse_mixed_tolerance_pass_count)"
    echo "duplicate_mse_maximum_absolute_discrepancy=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_absolute_discrepancy)"
    echo "duplicate_mse_maximum_relative_discrepancy=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_relative_discrepancy)"
    echo "duplicate_mse_maximum_tolerance_fraction=$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_tolerance_fraction)"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "validator_sha256=${VALIDATOR_SHA}"
    echo "awk_sha256=${AWK_BIN_SHA}"
    echo "validator_self_test_receipt_sha256=$(sha256 "$SELF_TEST_RECEIPT")"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
    echo "verification_receipt_sha256=$(sha256 "$VERIFICATION_RECEIPT")"
    echo "v1_runner_sha256=${V1_RUNNER_SHA}"
    echo "v1_preregistration_sha256=${V1_PREREG_SHA}"
    echo "v1_attempt_sha256=${V1_ATTEMPT_SHA}"
    echo "v1_log_sha256=${V1_LOG_SHA}"
    echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_report_sha256=${SOURCE_REPORT_SHA}"
    echo "source_evaluator_log_sha256=${SOURCE_LOG_SHA}"
    echo "source_runner_sha256=${SOURCE_RUNNER_SHA}"
    echo "source_preregistration_sha256=${SOURCE_PREREG_SHA}"
    echo "source_evaluator_sha256=${SOURCE_EVALUATOR_SHA}"
    echo "source_build_wrapper_sha256=${SOURCE_BUILD_WRAPPER_SHA}"
    echo "source_build_receipt_sha256=${SOURCE_BUILD_RECEIPT_SHA}"
    echo "source_binary_sha256=${SOURCE_BINARY_SHA}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "phase2a_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "phase2b_result_sha256=${PHASE2B_RESULT_SHA}"
    echo "phase2b_report_sha256=${PHASE2B_REPORT_SHA}"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "probe_bytes_read_by_recovery=false"
    echo "binary_executed_by_recovery=false"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "inherited_evaluator_invocations=1"
    echo "inherited_affine_oracle_grouped_fit_count=1"
    echo "inherited_affine_oracle_head_solve_count=9"
    echo "inherited_optimizer_fits_completed=1"
    echo "inherited_total_train_fit_procedures=2"
    echo "inherited_optimizer_steps=3500"
    echo "maximum_anchor_read=2815"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
  } > "$candidate"
}

validate_result() {
  local path="$1" split_id field
  load_kv_file "$path"
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_development_receipt_v2
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" diagnostic_authority development_only
  expect "$path" benchmark_acceptance_authority false
  expect "$path" scientific_result_available true
  expect "$path" recovery_kind immutable_predecessor_report_verification_parse_fix
  expect "$path" classification float32_conditioning_failure
  expect "$path" classification_order_preserved true
  expect "$path" direct_float32_parity_pass false
  expect "$path" paired_gelu_parity_pass true
  expect "$path" direct_linear_adam_recovery_gate_pass false
  expect "$path" direct_linear_adam_clear_failure_gate_pass true
  expect "$path" direct_float32_aggregate_forecast_metrics_within_0.001 true
  for split_id in train validation; do
    for field in directional_accuracy pairwise_rank_accuracy correlation rmse_target_rms_ratio; do
      expect "$path" "float64_oracle.${split_id}.${field}" "$(kv "$VERIFICATION_LOG" "float64_oracle.${split_id}.${field}")"
      expect "$path" "direct_float32.${split_id}.${field}" "$(kv "$VERIFICATION_LOG" "direct_float32.${split_id}.${field}")"
    done
  done
  expect "$path" duplicate_mse_pair_count 10
  expect "$path" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$path" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$path" duplicate_mse_strict_1e-12_failure_count 10
  expect "$path" duplicate_mse_mixed_tolerance_pass_count 10
  expect "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$path" preregistration_sha256 "$PREREG_SHA"
  expect "$path" validator_sha256 "$VALIDATOR_SHA"
  expect "$path" awk_sha256 "$AWK_BIN_SHA"
  expect "$path" validator_self_test_receipt_sha256 "$(sha256 "$SELF_TEST_RECEIPT")"
  expect "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect "$path" verification_receipt_sha256 "$(sha256 "$VERIFICATION_RECEIPT")"
  expect "$path" v1_attempt_sha256 "$V1_ATTEMPT_SHA"
  expect "$path" v1_log_sha256 "$V1_LOG_SHA"
  expect "$path" v1_terminal_sha256 "$V1_TERMINAL_SHA"
  expect "$path" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$path" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$path" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$path" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$path" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$path" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$path" import_only true
  expect "$path" no_new_scientific_execution true
  expect "$path" probe_bytes_read_by_recovery false
  expect "$path" binary_executed_by_recovery false
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" inherited_evaluator_invocations 1
  expect "$path" inherited_affine_oracle_grouped_fit_count 1
  expect "$path" inherited_affine_oracle_head_solve_count 9
  expect "$path" inherited_optimizer_fits_completed 1
  expect "$path" inherited_total_train_fit_procedures 2
  expect "$path" inherited_optimizer_steps 3500
  expect "$path" maximum_anchor_read 2815
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
  expect "$path" retry_allowed false
  expect "$path" resume_allowed false
}

emit_terminal() {
  local rc="$1" reason="$2" candidate="${SCRATCH}/terminal.$$.status"
  [[ ! -e "$TERMINAL" ]] || return 0
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_v2_terminal_v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_verification_recovery_v2"
    echo "failure_stage=${STAGE}"
    echo "failure_reason=${reason}"
    echo "exit_code=${rc}"
    echo "child_kind=${CHILD_KIND}"
    echo "child_started=${CHILD_STARTED}"
    echo "child_exit_code=${CHILD_EXIT}"
    echo "self_test_receipt_sha256=$([[ -e "$SELF_TEST_RECEIPT" ]] && sha256 "$SELF_TEST_RECEIPT" || echo not_available)"
    echo "verification_attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
    echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_report_sha256=${SOURCE_REPORT_SHA}"
    echo "current_log_sha256=$([[ -n "$CURRENT_LOG_DEST" && -e "$CURRENT_LOG_DEST" ]] && sha256 "$CURRENT_LOG_DEST" || echo not_available)"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "scientific_result_available=false"
    echo "same_protocol_retry_allowed=false"
    echo "same_protocol_resume_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$TERMINAL"
}

adopt_timeout_pid_during_launch() {
  local candidate="${!:-}"
  if [[ "$TIMEOUT_LAUNCHING" == 1 && -z "$TIMEOUT_PID" && "$candidate" =~ ^[1-9][0-9]*$ &&
        "$candidate" != "$TIMEOUT_PREVIOUS_ASYNC_PID" ]]; then
    TIMEOUT_PID="$candidate"
  fi
}

stop_and_reap_timeout_child() {
  local signal="$1" child_exit
  [[ -n "$TIMEOUT_PID" ]] || return 0
  kill -s "$signal" "$TIMEOUT_PID" 2>/dev/null || true
  if wait "$TIMEOUT_PID"; then child_exit=0; else child_exit=$?; fi
  CHILD_EXIT="$child_exit"
  TIMEOUT_PID=""
  TIMEOUT_LAUNCHING=0
  PENDING_SIGNAL=""
  PENDING_SIGNAL_EXIT=""
}

seal_failure_log() {
  if [[ -n "$LOG_CANDIDATE" && -f "$LOG_CANDIDATE" && -n "$CURRENT_LOG_DEST" &&
        ! -e "$CURRENT_LOG_DEST" ]]; then
    publish_once "$LOG_CANDIDATE" "$CURRENT_LOG_DEST" || true
  fi
}

on_exit() {
  local rc=$?
  trap - EXIT HUP INT TERM QUIT
  set +e
  adopt_timeout_pid_during_launch
  [[ -z "$TIMEOUT_PID" ]] || stop_and_reap_timeout_child TERM
  if [[ "$ACTIVE" == 1 && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_failure_log
    emit_terminal "$rc" interrupted_or_nonzero_exit
  fi
  exit "$rc"
}

on_signal() {
  local signal="$1" shell_exit="$2"
  adopt_timeout_pid_during_launch
  if [[ "$TIMEOUT_LAUNCHING" == 1 && -z "$TIMEOUT_PID" ]]; then
    [[ -n "$PENDING_SIGNAL" ]] || { PENDING_SIGNAL="$signal"; PENDING_SIGNAL_EXIT="$shell_exit"; }
    return 0
  fi
  trap - HUP INT TERM QUIT
  stop_and_reap_timeout_child "$signal"
  exit "$shell_exit"
}

run_captured_child() {
  local timeout_seconds="$1" grace_seconds="$2" pending_signal pending_exit rc
  shift 2
  CHILD_STARTED=1
  TIMEOUT_PREVIOUS_ASYNC_PID="${!:-}"
  TIMEOUT_LAUNCHING=1
  timeout --signal=TERM --kill-after="${grace_seconds}s" "${timeout_seconds}s" \
    "$@" > "$LOG_CANDIDATE" 2>&1 &
  TIMEOUT_PID=$!
  TIMEOUT_LAUNCHING=0
  if [[ -n "$PENDING_SIGNAL" ]]; then
    pending_signal="$PENDING_SIGNAL"
    pending_exit="$PENDING_SIGNAL_EXIT"
    PENDING_SIGNAL=""
    PENDING_SIGNAL_EXIT=""
    on_signal "$pending_signal" "$pending_exit"
  fi
  set +e
  wait "$TIMEOUT_PID"
  rc=$?
  set -e
  CHILD_EXIT="$rc"
  TIMEOUT_PID=""
  return "$rc"
}

verify_preparation_internal() {
  require_file "$SELF_TEST_LOG" 444
  require_file "$SELF_TEST_RECEIPT" 444
  validate_self_test_receipt "$SELF_TEST_RECEIPT"
  [[ ! -e "$TERMINAL" ]] || fail "V2 recovery is terminal"
}

prepare() {
  local rc receipt_candidate
  preflight_local
  open_lock_create
  acquire_lineage_locks
  preflight_mechanical_files
  [[ ! -e "$TERMINAL" ]] || fail "V2 recovery is terminal"
  if [[ -e "$SELF_TEST_RECEIPT" ]]; then
    verify_preparation_internal
    echo "[clear-signal:optimizer-localization-recovery-v2] existing self-test verified"
    return 0
  fi
  if [[ -e "$SELF_TEST_LOG" ]]; then
    STAGE="orphan_self_test_reentry"
    CHILD_KIND="validator_syntax_self_test"
    CURRENT_LOG_DEST="$SELF_TEST_LOG"
    emit_terminal 1 incomplete_self_test_publication
    fail "validator self-test log exists without its receipt"
  fi
  [[ ! -e "$SELF_TEST_LOG" && ! -e "$ATTEMPT" && ! -e "$VERIFICATION_LOG" &&
     ! -e "$VERIFICATION_RECEIPT" && ! -e "$RESULT" ]] || fail "non-pristine V2 preparation state"
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "V2 scratch is not pristine"

  ACTIVE=1
  STAGE="validator_syntax_self_test"
  CHILD_KIND="validator_syntax_self_test"
  CURRENT_LOG_DEST="$SELF_TEST_LOG"
  LOG_CANDIDATE="${SCRATCH}/validator.syntax_self_test.$$.log"
  trap on_exit EXIT
  trap 'on_signal HUP 129' HUP
  trap 'on_signal INT 130' INT
  trap 'on_signal TERM 143' TERM
  trap 'on_signal QUIT 131' QUIT
  if run_captured_child "$SELF_TEST_TIMEOUT_SECONDS" "$SELF_TEST_GRACE_SECONDS" \
      "$AWK_BIN" -v syntax_self_test=1 -f "$VALIDATOR" /dev/null /dev/null; then
    rc=0
  else
    rc=$?
  fi
  publish_once "$LOG_CANDIDATE" "$SELF_TEST_LOG"
  LOG_CANDIDATE=""
  if [[ "$rc" != 0 || "$(stat -c '%s' -- "$SELF_TEST_LOG")" != 0 ]]; then
    [[ "$rc" != 0 ]] || rc=1
    emit_terminal "$rc" syntax_self_test_failure
    ACTIVE=0
    trap - EXIT HUP INT TERM QUIT
    fail "validator syntax self-test failed: ${rc}"
  fi
  receipt_candidate="${SCRATCH}/validator.syntax_self_test.$$.status"
  emit_self_test_receipt "$receipt_candidate"
  chmod 0444 -- "$receipt_candidate"
  validate_self_test_receipt "$receipt_candidate"
  publish_once "$receipt_candidate" "$SELF_TEST_RECEIPT"
  ACTIVE=0
  trap - EXIT HUP INT TERM QUIT
  echo "[clear-signal:optimizer-localization-recovery-v2] self-test complete: ${SELF_TEST_RECEIPT}"
}

verify_development_internal() {
  verify_preparation_internal
  [[ -f "$ATTEMPT" && -f "$VERIFICATION_LOG" && -f "$VERIFICATION_RECEIPT" &&
     -f "$RESULT" && ! -e "$TERMINAL" ]] || fail "unique V2 recovery result is absent"
  validate_attempt
  validate_worker_log "$VERIFICATION_LOG"
  validate_verification_receipt "$VERIFICATION_RECEIPT"
  validate_result "$RESULT"
}

run_verification() {
  local rc attempt_sha receipt_candidate result_candidate
  preflight_local
  open_existing_lock false
  acquire_lineage_locks
  preflight_lineage
  verify_preparation_internal
  if [[ -e "$RESULT" ]]; then
    verify_development_internal
    echo "[clear-signal:optimizer-localization-recovery-v2] existing result verified"
    return 0
  fi
  if [[ -e "$ATTEMPT" || -e "$VERIFICATION_LOG" || -e "$VERIFICATION_RECEIPT" ]]; then
    STAGE="orphan_verification_reentry"
    CHILD_KIND="verification_worker"
    [[ -e "$VERIFICATION_LOG" ]] && CURRENT_LOG_DEST="$VERIFICATION_LOG"
    emit_terminal 1 incomplete_consumed_verification_attempt
    fail "V2 verification attempt is already consumed or incomplete"
  fi
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "V2 scratch is not pristine"

  ACTIVE=1
  STAGE="attempt_publication"
  trap on_exit EXIT
  trap 'on_signal HUP 129' HUP
  trap 'on_signal INT 130' INT
  trap 'on_signal TERM 143' TERM
  trap 'on_signal QUIT 131' QUIT
  emit_attempt
  validate_attempt

  STAGE="single_pass_report_verification"
  CHILD_KIND="verification_worker"
  CURRENT_LOG_DEST="$VERIFICATION_LOG"
  LOG_CANDIDATE="${SCRATCH}/verification.$$.log"
  attempt_sha="$(sha256 "$ATTEMPT")"
  if run_captured_child "$VERIFY_TIMEOUT_SECONDS" "$VERIFY_GRACE_SECONDS" \
      "$RUNNER" --verification-worker "$attempt_sha"; then
    rc=0
  else
    rc=$?
  fi
  publish_once "$LOG_CANDIDATE" "$VERIFICATION_LOG"
  LOG_CANDIDATE=""
  if [[ "$rc" != 0 ]]; then
    emit_terminal "$rc" timeout_or_validation_failure
    ACTIVE=0
    trap - EXIT HUP INT TERM QUIT
    fail "verification worker failed: ${rc}"
  fi
  validate_worker_log "$VERIFICATION_LOG"

  STAGE="verification_receipt_publication"
  receipt_candidate="${SCRATCH}/verification.complete.$$.status"
  emit_verification_receipt "$receipt_candidate"
  chmod 0444 -- "$receipt_candidate"
  validate_verification_receipt "$receipt_candidate"
  publish_once "$receipt_candidate" "$VERIFICATION_RECEIPT"
  load_kv_file "$VERIFICATION_RECEIPT"
  validate_verification_receipt "$VERIFICATION_RECEIPT"

  STAGE="final_result_commit"
  result_candidate="${SCRATCH}/development.$$.status"
  emit_result "$result_candidate"
  chmod 0444 -- "$result_candidate"
  validate_result "$result_candidate"
  trap '' HUP INT TERM QUIT
  publish_once "$result_candidate" "$RESULT"
  ACTIVE=0
  trap - EXIT HUP INT TERM QUIT
  echo "[clear-signal:optimizer-localization-recovery-v2] complete: ${RESULT}"
}

plan() {
  preflight_local
  open_optional_lock_read_only
  acquire_lineage_locks
  preflight_mechanical_files
  echo "Project Clear Signal — optimizer-localization verification recovery V2"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=development_only"
  echo "operation=mechanical_parse_fix_verification_only"
  echo "validator_sha256=${VALIDATOR_SHA}"
  echo "awk_sha256=${AWK_BIN_SHA}"
  echo "v1_attempt_sha256=${V1_ATTEMPT_SHA}"
  echo "v1_log_sha256=${V1_LOG_SHA}"
  echo "v1_terminal_sha256=${V1_TERMINAL_SHA}"
  echo "self_test_timeout_seconds=${SELF_TEST_TIMEOUT_SECONDS}"
  echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
  echo "self_test_receipt_present=$([[ -e "$SELF_TEST_RECEIPT" ]] && echo true || echo false)"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "terminal_invalid=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
  echo "new_evaluator_invocations=0"
  echo "new_fits=0"
  echo "new_optimizer_steps=0"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_access=false"
}

main() {
  case "${1:-}" in
    --plan)
      [[ $# == 1 ]] || fail "--plan accepts no arguments"
      plan
      ;;
    --prepare)
      [[ $# == 1 ]] || fail "--prepare accepts no arguments"
      prepare
      ;;
    --run-verification)
      [[ $# == 1 ]] || fail "--run-verification accepts no arguments"
      run_verification
      ;;
    --verify-preparation)
      [[ $# == 1 ]] || fail "--verify-preparation accepts no arguments"
      preflight_local
      open_existing_lock true
      acquire_lineage_locks
      preflight_mechanical_files
      verify_preparation_internal
      echo "[clear-signal:optimizer-localization-recovery-v2] self-test verified"
      ;;
    --verify-development)
      [[ $# == 1 ]] || fail "--verify-development accepts no arguments"
      preflight_local
      open_existing_lock true
      acquire_lineage_locks
      preflight_lineage
      verify_development_internal
      echo "[clear-signal:optimizer-localization-recovery-v2] result verified"
      ;;
    --verification-worker)
      [[ $# == 2 ]] || fail "--verification-worker requires one attempt SHA-256"
      verification_worker "$2"
      ;;
    *)
      fail "usage: $0 --plan|--prepare|--run-verification|--verify-preparation|--verify-development"
      ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
