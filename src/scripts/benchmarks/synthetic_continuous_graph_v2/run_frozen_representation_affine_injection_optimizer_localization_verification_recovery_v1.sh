#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_verification_recovery_development_v1"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_VERIFICATION_RECOVERY_PREREGISTRATION.md"
readonly PREREG_SHA="c9a8887aecb4b54e73b262ef50d7133bfce3c2279a202203f4b75221985727b4"

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
readonly ATTEMPT="${RUNTIME}/verification.attempt.status"
readonly VERIFICATION_LOG="${RUNTIME}/verification.log"
readonly VERIFICATION_RECEIPT="${RUNTIME}/verification.complete.status"
readonly RESULT="${RUNTIME}/development.status"
readonly REJECTED_RESULT="${RUNTIME}/rejected.development.status"
readonly TERMINAL="${RUNTIME}/terminal.invalid.status"

readonly VERIFY_TIMEOUT_SECONDS=30
readonly VERIFY_GRACE_SECONDS=5
readonly DUPLICATE_MSE_ABS_TOL="1.25e-7"
readonly DUPLICATE_MSE_REL_TOL="1.25e-7"

declare -A KV_VALUE=()
declare -A KV_PRESENT=()
declare -A KV_LOADED=()

ACTIVE=0
STAGE="pre_attempt"
WORKER_STARTED=0
WORKER_EXIT="not_started"
LOG_CANDIDATE=""
TIMEOUT_PID=""
TIMEOUT_LAUNCHING=0
TIMEOUT_PREVIOUS_ASYNC_PID=""
PENDING_SIGNAL=""
PENDING_SIGNAL_EXIT=""

fail() {
  echo "[clear-signal:optimizer-localization-recovery] ERROR: $*" >&2
  return 1
}

sha256() {
  sha256sum -- "$1" | cut -d' ' -f1
}

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
}

preflight_static() {
  preflight_local
  require_file "$SOURCE_LOCK" 600
  require_exact "$SOURCE_ATTEMPT" "$SOURCE_ATTEMPT_SHA" 444
  require_exact "$SOURCE_TERMINAL" "$SOURCE_TERMINAL_SHA" 444
  require_exact "$SOURCE_REPORT" "$SOURCE_REPORT_SHA" 444
  require_exact "$SOURCE_LOG" "$SOURCE_LOG_SHA" 444
  [[ "$(stat -c '%s' -- "$SOURCE_LOG")" == 0 ]] || fail "predecessor evaluator log is not empty"
  require_exact "$SOURCE_RUNNER" "$SOURCE_RUNNER_SHA" 555
  require_exact "$SOURCE_PREREG" "$SOURCE_PREREG_SHA" 444
  require_exact "$SOURCE_EVALUATOR" "$SOURCE_EVALUATOR_SHA" 444
  require_exact "$SOURCE_BUILD_WRAPPER" "$SOURCE_BUILD_WRAPPER_SHA" 555
  require_exact "$SOURCE_BUILD_RECEIPT" "$SOURCE_BUILD_RECEIPT_SHA" 444
  require_exact "$SOURCE_BINARY" "$SOURCE_BINARY_SHA" 555
  require_exact "$PHASE2A_RECEIPT" "$PHASE2A_RECEIPT_SHA" 444
  require_exact "$PHASE2A_MAIN" "$PHASE2A_REPORT_SHA" 444
  require_exact "$PHASE2A_REPLAY" "$PHASE2A_REPORT_SHA" 444
  require_exact "$PHASE2B_RESULT" "$PHASE2B_RESULT_SHA" 444
  require_exact "$PHASE2B_REPORT" "$PHASE2B_REPORT_SHA" 444

  load_kv_file "$SOURCE_ATTEMPT"
  load_kv_file "$SOURCE_TERMINAL"
  load_kv_file "$SOURCE_BUILD_RECEIPT"
  load_kv_file "$PHASE2A_RECEIPT"
  load_kv_file "$PHASE2B_RESULT"
  load_kv_file "$PHASE2B_REPORT"

  expect "$SOURCE_ATTEMPT" schema_id synthetic_v2_affine_injection_optimizer_localization_attempt_v1
  expect "$SOURCE_ATTEMPT" status consumed
  expect "$SOURCE_ATTEMPT" protocol_id "$SOURCE_PROTOCOL_ID"
  expect "$SOURCE_ATTEMPT" attempt_ordinal 1
  expect "$SOURCE_ATTEMPT" runner_sha256 "$SOURCE_RUNNER_SHA"
  expect "$SOURCE_ATTEMPT" preregistration_sha256 "$SOURCE_PREREG_SHA"
  expect "$SOURCE_ATTEMPT" evaluator_source_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$SOURCE_ATTEMPT" build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$SOURCE_ATTEMPT" build_receipt_sha256 "$SOURCE_BUILD_RECEIPT_SHA"
  expect "$SOURCE_ATTEMPT" binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$SOURCE_ATTEMPT" train_probe_sha256 "$TRAIN_SHA"
  expect "$SOURCE_ATTEMPT" validation_probe_sha256 "$VALIDATION_SHA"
  expect "$SOURCE_ATTEMPT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$SOURCE_ATTEMPT" phase2a_main_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$SOURCE_ATTEMPT" phase2a_replay_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$SOURCE_ATTEMPT" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$SOURCE_ATTEMPT" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$SOURCE_ATTEMPT" evaluator_invocation_limit 1
  expect "$SOURCE_ATTEMPT" affine_oracle_grouped_fit_count 1
  expect "$SOURCE_ATTEMPT" affine_oracle_head_solve_count 9
  expect "$SOURCE_ATTEMPT" optimizer_fits_completed 1
  expect "$SOURCE_ATTEMPT" total_train_fit_procedures 2
  expect "$SOURCE_ATTEMPT" optimizer_step_limit 3500
  expect "$SOURCE_ATTEMPT" maximum_anchor_read 2815
  expect "$SOURCE_ATTEMPT" retry_allowed false
  expect "$SOURCE_ATTEMPT" resume_allowed false
  expect "$SOURCE_ATTEMPT" certified_input_access false
  expect "$SOURCE_ATTEMPT" final_holdout_access false
  expect "$SOURCE_ATTEMPT" policy_access false

  expect "$SOURCE_TERMINAL" schema_id synthetic_v2_affine_injection_optimizer_localization_terminal_v1
  expect "$SOURCE_TERMINAL" status terminal_invalid
  expect "$SOURCE_TERMINAL" protocol_id "$SOURCE_PROTOCOL_ID"
  expect "$SOURCE_TERMINAL" failure_stage report_validation
  expect "$SOURCE_TERMINAL" failure_reason runner_failure
  expect "$SOURCE_TERMINAL" evaluator_started 1
  expect "$SOURCE_TERMINAL" evaluator_exit_code 0
  expect "$SOURCE_TERMINAL" attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$SOURCE_TERMINAL" evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$SOURCE_TERMINAL" rejected_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$SOURCE_TERMINAL" retry_allowed false
  expect "$SOURCE_TERMINAL" resume_allowed false
  expect "$SOURCE_TERMINAL" certified_input_access false
  expect "$SOURCE_TERMINAL" final_holdout_access false
  expect "$SOURCE_TERMINAL" policy_access false

  expect "$SOURCE_BUILD_RECEIPT" schema_id synthetic_v2_affine_injection_optimizer_localization_build_v1
  expect "$SOURCE_BUILD_RECEIPT" status complete
  expect "$SOURCE_BUILD_RECEIPT" source_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$SOURCE_BUILD_RECEIPT" build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$SOURCE_BUILD_RECEIPT" binary_path "$SOURCE_BINARY"
  expect "$SOURCE_BUILD_RECEIPT" binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$SOURCE_BUILD_RECEIPT" compile_only true
  expect "$SOURCE_BUILD_RECEIPT" build_timeout_seconds 300
  expect "$SOURCE_BUILD_RECEIPT" term_grace_seconds 10
  expect "$SOURCE_BUILD_RECEIPT" probe_access false

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

acquire_source_lock() {
  require_file "$SOURCE_LOCK" 600
  exec 8<> "$SOURCE_LOCK"
  flock -s -n 8 || fail "predecessor runtime is active"
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

open_lock() {
  ensure_runtime
  exec 9<> "$LOCK"
  flock -n 9 || fail "verification recovery is already active"
}

open_existing_lock_read_only() {
  require_private_dir "$RUNTIME"
  require_private_dir "$SCRATCH"
  require_file "$LOCK" 600
  exec 9< "$LOCK"
  flock -s -n 9 || fail "verification recovery is active"
}

emit_attempt() {
  local candidate="${SCRATCH}/verification.attempt.$$.status"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_attempt_v1"
    echo "status=consumed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_ordinal=1"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
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
    echo "phase2a_main_report_sha256=${PHASE2A_REPORT_SHA}"
    echo "phase2a_replay_report_sha256=${PHASE2A_REPORT_SHA}"
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
    echo "new_capture_invocations=0"
    echo "new_representation_forward_invocations=0"
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
  expect "$ATTEMPT" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_attempt_v1
  expect "$ATTEMPT" status consumed
  expect "$ATTEMPT" protocol_id "$PROTOCOL_ID"
  expect "$ATTEMPT" attempt_ordinal 1
  expect "$ATTEMPT" runner_path "$RUNNER"
  expect "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$ATTEMPT" preregistration_path "$PREREG"
  expect "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"
  expect "$ATTEMPT" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$ATTEMPT" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$ATTEMPT" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$ATTEMPT" source_evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$ATTEMPT" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect "$ATTEMPT" source_preregistration_sha256 "$SOURCE_PREREG_SHA"
  expect "$ATTEMPT" source_evaluator_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$ATTEMPT" source_build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$ATTEMPT" source_build_receipt_sha256 "$SOURCE_BUILD_RECEIPT_SHA"
  expect "$ATTEMPT" source_binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$ATTEMPT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$ATTEMPT" phase2a_main_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$ATTEMPT" phase2a_replay_report_sha256 "$PHASE2A_REPORT_SHA"
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
  expect "$ATTEMPT" new_capture_invocations 0
  expect "$ATTEMPT" new_representation_forward_invocations 0
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
  local attempt_sha="$1" lock_identity fd_identity source_lock_identity source_fd_identity
  local -a parent_argv=()
  mapfile -d '' -t parent_argv < "/proc/${PPID}/cmdline"
  [[ "${#parent_argv[@]}" == 7 ]] || fail "verification worker parent argv length mismatch"
  [[ "${parent_argv[0]##*/}" == timeout ]] || fail "verification worker parent is not GNU timeout"
  [[ "${parent_argv[1]}" == "--signal=TERM" ]] || fail "verification worker signal mismatch"
  [[ "${parent_argv[2]}" == "--kill-after=${VERIFY_GRACE_SECONDS}s" ]] || fail "verification worker grace mismatch"
  [[ "${parent_argv[3]}" == "${VERIFY_TIMEOUT_SECONDS}s" ]] || fail "verification worker timeout mismatch"
  [[ "${parent_argv[4]}" == "$RUNNER" && "${parent_argv[5]}" == --verification-worker &&
     "${parent_argv[6]}" == "$attempt_sha" ]] || fail "verification worker command mismatch"
  [[ -e /proc/$$/fd/8 && -e /proc/$$/fd/9 ]] || fail "verification worker lacks inherited locks"
  source_lock_identity="$(stat -Lc '%d:%i' -- "$SOURCE_LOCK")"
  source_fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/8)"
  [[ "$source_fd_identity" == "$source_lock_identity" ]] || fail "source lock identity mismatch"
  lock_identity="$(stat -Lc '%d:%i' -- "$LOCK")"
  fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/9)"
  [[ "$fd_identity" == "$lock_identity" ]] || fail "recovery lock identity mismatch"
}

validate_report_single_pass() {
  awk \
    -v source_report_sha="$SOURCE_REPORT_SHA" \
    -v phase2a_report_sha="$PHASE2A_REPORT_SHA" \
    -v abs_tol="$DUPLICATE_MSE_ABS_TOL" \
    -v rel_tol="$DUPLICATE_MSE_REL_TOL" '
    function fatal(message) {
      print "report validation failed: " message > "/dev/stderr"
      failed = 1
      exit 1
    }
    function absolute(x) { return x < 0 ? -x : x }
    function maximum(a, b) { return a > b ? a : b }
    function is_number(x) {
      return x ~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/
    }
    function rv(key) {
      if (!(key in report_seen)) fatal("missing report key " key)
      return report[key]
    }
    function pv(key) {
      if (!(key in phase_seen)) fatal("missing Phase2A key " key)
      return phase[key]
    }
    function exact(key, expected, actual) {
      actual = rv(key)
      if ((actual "") != (expected "")) fatal(key " expected " expected ", got " actual)
    }
    function require_number(key, lower, upper, lower_closed, upper_closed, integer, value) {
      value = rv(key)
      if (!is_number(value)) fatal(key " is not a finite decimal number")
      value += 0
      if (lower != "" && (lower_closed ? value < lower : value <= lower)) fatal(key " violates lower bound")
      if (upper != "" && (upper_closed ? value > upper : value >= upper)) fatal(key " violates upper bound")
      if (integer && value != int(value)) fatal(key " is not integral")
      return value
    }
    function close_values(actual, expected, tolerance, label, delta) {
      if (!is_number(actual) || !is_number(expected)) fatal(label " has a nonnumeric operand")
      delta = absolute((actual + 0) - (expected + 0))
      if (delta > tolerance) fatal(label " exceeds absolute tolerance")
    }
    function close_report(key, expected, tolerance) {
      close_values(rv(key), expected, tolerance, key)
    }
    function metric(prefix, expected_count, field, ratio) {
      exact(prefix ".count", expected_count)
      exact(prefix ".pairwise_rank_count", expected_count)
      require_number(prefix ".mae", 0, "", 1, 1, 0)
      require_number(prefix ".rmse", 0, "", 1, 1, 0)
      require_number(prefix ".prediction_rms", 0, "", 1, 1, 0)
      require_number(prefix ".rmse_target_rms_ratio", 0, "", 1, 1, 0)
      require_number(prefix ".target_rms", 0, "", 0, 1, 0)
      for (field = 1; field <= 3; ++field) {
        bounded_field = (field == 1 ? "directional_accuracy" : (field == 2 ? "pairwise_rank_accuracy" : "best_asset_agreement"))
        require_number(prefix "." bounded_field, 0, 1, 1, 1, 0)
      }
      require_number(prefix ".correlation", -1, 1, 1, 1, 0)
      ratio = (rv(prefix ".rmse") + 0) / (rv(prefix ".target_rms") + 0)
      close_report(prefix ".rmse_target_rms_ratio", ratio, 1e-12)
    }
    function mixed_duplicate(execution_key, recomputed_key, a, b, scale, delta, relative_delta, allowance, fraction) {
      a = require_number(execution_key, 0, "", 0, 1, 0)
      b = require_number(recomputed_key, 0, "", 0, 1, 0)
      scale = maximum(absolute(a), absolute(b))
      delta = absolute(a - b)
      relative_delta = (scale == 0 ? 0 : delta / scale)
      allowance = abs_tol + rel_tol * scale
      fraction = (allowance == 0 ? (delta == 0 ? 0 : 1e300) : delta / allowance)
      duplicate_pair_count++
      if (delta > 1e-12) strict_failure_count++
      if (delta <= allowance) mixed_pass_count++; else fatal(execution_key " exceeds mixed tolerance")
      if (delta > max_abs_discrepancy) max_abs_discrepancy = delta
      if (relative_delta > max_rel_discrepancy) max_rel_discrepancy = relative_delta
      if (fraction > max_tolerance_fraction) max_tolerance_fraction = fraction
    }
    function update_direct_metric_delta(oracle_key, direct_key, delta) {
      delta = absolute((rv(oracle_key) + 0) - (rv(direct_key) + 0))
      if (delta > direct_metric_max_delta) direct_metric_max_delta = delta
    }
    FILENAME == ARGV[1] {
      separator = index($0, "=")
      if (separator < 2 || substr($0, length($0), 1) == "\r") fatal("malformed Phase2A line")
      key = substr($0, 1, separator - 1)
      if (key in phase_seen) fatal("duplicate Phase2A key " key)
      phase_seen[key] = 1
      phase[key] = substr($0, separator + 1)
      phase_lines++
      next
    }
    FILENAME == ARGV[2] {
      separator = index($0, "=")
      if (separator < 2 || substr($0, length($0), 1) == "\r") fatal("malformed report line")
      key = substr($0, 1, separator - 1)
      if (key in report_seen) fatal("duplicate report key " key)
      report_seen[key] = 1
      report[key] = substr($0, separator + 1)
      report_lines++
      next
    }
    END {
      if (failed) exit 1
      if (phase_lines != 234) fatal("Phase2A report line count is not 234")
      if (report_lines != 490) fatal("source report line count is not 490")

      exact("schema_id", "synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1")
      exact("status", "complete")
      exact("benchmark_id", "synthetic_continuous_graph_v2")
      exact("diagnostic_phase", "affine_injection_optimizer_localization")
      exact("diagnostic_authority", "development_only")
      exact("benchmark_acceptance_authority", "false")
      exact("train_input", "/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe")
      exact("validation_input", "/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe")
      exact("probe_kind", "representation")
      exact("probe_record_schema", "kikijyeba.synthetic.representation_edge_feature_probe.v1")
      exact("train_probe_rows", "22464")
      exact("validation_probe_rows", "2304")
      exact("certified_probe_rows", "0")
      exact("fit_anchor_range", "[0,2496)")
      exact("validation_anchor_range", "[2560,2816)")
      exact("maximum_anchor_read", "2815")
      exact("final_holdout_begin", "3328")
      close_report("fixed_ridge", 1e-12, 1e-24)
      exact("ridge_selection", "false")
      exact("oracle_phase2a_reference_validation", "external_runner_required")
      exact("head_index_formula", "channel*3+edge")
      exact("flat_row_order", "anchor,edge,channel")
      exact("direct_architecture", "Linear(96,9)+gather(channel*3+edge)")
      exact("injected_architecture", "Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)+gather(channel*3+edge)")
      exact("device", "cpu")
      exact("feature_dtype", "float32")
      exact("target_training_dtype", "float32")
      exact("metric_dtype", "float64")
      exact("oracle_solver_dtype", "float64")
      exact("deterministic_algorithms", "true")
      exact("deterministic_cudnn", "true")
      exact("deterministic_fill_uninitialized_memory", "true")
      exact("intraop_threads", "1")
      exact("interop_threads", "1")
      exact("float64_solver", "float64_centered_cholesky_ridge")
      exact("float32_feature_standardization", "train_core_all_edges_all_channels")
      exact("target_standardization", "train_core_per_edge_channel")
      exact("gelu_identity", "GELU(z)-GELU(-z)=z")
      exact("gelu_injected_hidden_units_per_layer", "18")
      exact("gelu_unused_hidden_units_per_layer", "110")
      exact("zero_optimizer_ladder_optimizer_steps", "0")
      require_number("feature_standardization_clamped_coordinate_count", 0, "", 1, 1, 1)
      require_number("target_standardization_clamped_coordinate_count", 0, "", 1, 1, 1)
      require_number("affine_maximum_normalized_residual", 0, "", 1, 1, 0)
      require_number("affine_coefficient_l2_norm", 0, "", 0, 1, 0)

      route_name[1] = "float64_oracle"
      route_name[2] = "direct_float32"
      route_name[3] = "paired_gelu_injected"
      route_name[4] = "direct_linear_adam_seed31"
      split_name[1] = "train"
      split_name[2] = "validation"
      for (route_index = 1; route_index <= 4; ++route_index) {
        for (split_index = 1; split_index <= 2; ++split_index) {
          split = split_name[split_index]
          count = (split == "train" ? 22464 : 2304)
          prefix = "route." route_name[route_index] "." split
          metric(prefix, count)
          for (channel = 0; channel <= 2; ++channel) {
            count = (split == "train" ? 7488 : 768)
            metric(prefix ".channel_" channel, count)
          }
        }
      }

      metric_field[1] = "mae"
      metric_field[2] = "rmse"
      metric_field[3] = "target_rms"
      metric_field[4] = "prediction_rms"
      metric_field[5] = "rmse_target_rms_ratio"
      metric_field[6] = "directional_accuracy"
      metric_field[7] = "pairwise_rank_accuracy"
      metric_field[8] = "best_asset_agreement"
      metric_field[9] = "correlation"
      for (split_index = 1; split_index <= 2; ++split_index) {
        split = split_name[split_index]
        for (channel_selector = -1; channel_selector <= 2; ++channel_selector) {
          suffix = (channel_selector < 0 ? "" : ".channel_" channel_selector)
          report_prefix = "route.float64_oracle." split suffix
          phase_prefix = "selected." split suffix
          exact(report_prefix ".count", pv(phase_prefix ".count"))
          exact(report_prefix ".pairwise_rank_count", pv(phase_prefix ".pairwise_rank_count"))
          for (field_index = 1; field_index <= 9; ++field_index) {
            close_report(report_prefix "." metric_field[field_index], pv(phase_prefix "." metric_field[field_index]), 1e-12)
          }
        }
      }

      close_report("direct_float32_parity_tolerance_standardized_target_units", 1e-3, 1e-15)
      close_report("paired_gelu_parity_tolerance_standardized_target_units", 1e-5, 1e-17)
      delta_key[1] = "delta.float64_oracle_vs_direct_float32.train.standardized_target_units_max_abs"
      delta_key[2] = "delta.float64_oracle_vs_direct_float32.validation.standardized_target_units_max_abs"
      delta_key[3] = "delta.direct_float32_vs_paired_gelu.train.standardized_target_units_max_abs"
      delta_key[4] = "delta.direct_float32_vs_paired_gelu.validation.standardized_target_units_max_abs"
      delta_key[5] = "delta.float64_oracle_vs_direct_float32.train.original_units_max_abs"
      delta_key[6] = "delta.float64_oracle_vs_direct_float32.validation.original_units_max_abs"
      delta_key[7] = "delta.direct_float32_vs_paired_gelu.train.original_units_max_abs"
      delta_key[8] = "delta.direct_float32_vs_paired_gelu.validation.original_units_max_abs"
      delta_key[9] = "delta.float64_oracle_vs_paired_gelu.train.original_units_max_abs"
      delta_key[10] = "delta.float64_oracle_vs_paired_gelu.validation.original_units_max_abs"
      for (delta_index = 1; delta_index <= 10; ++delta_index) require_number(delta_key[delta_index], 0, "", 1, 1, 0)
      float_pass = ((rv(delta_key[1]) + 0) <= 0.001 && (rv(delta_key[2]) + 0) <= 0.001 ? "true" : "false")
      gelu_pass = ((rv(delta_key[3]) + 0) <= 0.00001 && (rv(delta_key[4]) + 0) <= 0.00001 ? "true" : "false")
      exact("direct_float32_parity_pass", float_pass)
      exact("paired_gelu_parity_pass", gelu_pass)

      oracle_mse = require_number("route.float64_oracle.train.standardized_mse", 0, "", 0, 1, 0)
      adam_mse = require_number("route.direct_linear_adam_seed31.train.standardized_mse", 0, "", 0, 1, 0)
      aggregate_ratio = require_number("direct_linear_adam_to_oracle_train_standardized_mse_ratio", 0, "", 1, 1, 0)
      close_values(aggregate_ratio, adam_mse / oracle_mse, 1e-12, "aggregate standardized MSE ratio")
      recovery = (aggregate_ratio <= 1.05)
      maximum_head_ratio = 0
      for (head = 0; head <= 8; ++head) {
        oracle_head_key = "route.float64_oracle.train.head_" head ".standardized_mse"
        adam_head_key = "route.direct_linear_adam_seed31.train.head_" head ".standardized_mse"
        oracle_head = require_number(oracle_head_key, 0, "", 0, 1, 0)
        adam_head = require_number(adam_head_key, 0, "", 0, 1, 0)
        head_ratio = adam_head / oracle_head
        close_report("direct_linear_adam_to_oracle_train.head_" head ".standardized_mse_ratio", head_ratio, 1e-12)
        if (head_ratio > 1.10) recovery = 0
        if (head_ratio > maximum_head_ratio) maximum_head_ratio = head_ratio
        mixed_duplicate("direct_linear_adam.full_train_standardized_mse.head_" head ".edge_" (head % 3) ".channel_" int(head / 3), adam_head_key)
      }
      close_report("direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio", maximum_head_ratio, 1e-12)
      if ((rv("route.direct_linear_adam_seed31.train.directional_accuracy") + 0) < (rv("route.float64_oracle.train.directional_accuracy") + 0) - 0.01) recovery = 0
      if ((rv("route.direct_linear_adam_seed31.train.pairwise_rank_accuracy") + 0) < (rv("route.float64_oracle.train.pairwise_rank_accuracy") + 0) - 0.01) recovery = 0
      if ((rv("route.direct_linear_adam_seed31.train.correlation") + 0) < (rv("route.float64_oracle.train.correlation") + 0) - 0.01) recovery = 0
      if ((rv("route.direct_linear_adam_seed31.train.rmse_target_rms_ratio") + 0) > (rv("route.float64_oracle.train.rmse_target_rms_ratio") + 0) + 0.05) recovery = 0
      clear_failure = (!recovery && (aggregate_ratio >= 1.25 || maximum_head_ratio >= 1.50))
      exact("direct_linear_adam_recovery_gate_pass", recovery ? "true" : "false")
      exact("direct_linear_adam_clear_failure_gate_pass", clear_failure ? "true" : "false")
      exact("direct_linear_adam_recovery_gate", "train_aggregate_mse_ratio<=1.05,each_train_head_mse_ratio<=1.10,train_direction>=oracle-0.01,train_rank>=oracle-0.01,train_correlation>=oracle-0.01,train_rmse_ratio<=oracle+0.05")
      exact("direct_linear_adam_clear_failure_gate", "!recovery_and_(train_aggregate_mse_ratio>=1.25_or_max_train_head_mse_ratio>=1.50)")
      if (float_pass == "false") expected_classification = "float32_conditioning_failure"
      else if (gelu_pass == "false") expected_classification = "paired_gelu_execution_failure"
      else if (recovery) expected_classification = "deep_parameterization_or_optimization_failure"
      else if (clear_failure) expected_classification = "direct_linear_adam_optimizer_failure"
      else expected_classification = "optimizer_localization_inconclusive"
      exact("classification", expected_classification)

      exact("seed", "31")
      exact("affine_oracle_grouped_fit_count", "1")
      exact("affine_oracle_head_solve_count", "9")
      exact("optimizer_fits_completed", "1")
      exact("total_train_fit_procedures", "2")
      exact("optimizer_steps", "3500")
      exact("steps_per_fit", "3500")
      exact("batch_size", "64")
      exact("optimizer", "Adam")
      exact("batch_sampling", "mt19937_64_uniform_with_replacement")
      exact("early_stopping", "false")
      exact("seed_selection", "false")
      exact("hyperparameter_search", "false")
      exact("retry", "false")
      exact("refit", "false")
      close_report("learning_rate", 0.001, 1e-15)
      close_report("adam_beta1", 0.9, 1e-15)
      close_report("adam_beta2", 0.999, 1e-15)
      close_report("adam_epsilon", 1e-8, 1e-20)
      close_report("weight_decay", 0, 0)
      close_report("gradient_clip_norm", 5, 0)
      exact("batch_schedule_fingerprint", "f2fa41d284a42d60")
      require_number("direct_linear_adam.initial_full_train_standardized_mse", 0, "", 1, 1, 0)
      require_number("direct_linear_adam.final_full_train_standardized_mse", 0, "", 1, 1, 0)
      require_number("direct_linear_adam.last_minibatch_loss", 0, "", 1, 1, 0)
      require_number("direct_linear_adam.maximum_preclip_gradient_norm", 0, "", 1, 1, 0)
      mixed_duplicate("direct_linear_adam.final_full_train_standardized_mse", "route.direct_linear_adam_seed31.train.standardized_mse")
      require_number("direct_linear_adam.clipped_step_count", 0, 3500, 1, 1, 1)
      exact("validation_read_by_trainer", "false")
      exact("validation_driven_choice", "false")
      exact("representation_forward_executed", "false")
      exact("checkpoint_written", "false")
      exact("certified_input_access", "false")
      exact("final_holdout_access", "false")
      exact("policy_access", "false")
      if (duplicate_pair_count != 10 || mixed_pass_count != 10) fatal("duplicate MSE pair accounting mismatch")

      forecast_field[1] = "directional_accuracy"
      forecast_field[2] = "pairwise_rank_accuracy"
      forecast_field[3] = "correlation"
      forecast_field[4] = "rmse_target_rms_ratio"
      for (split_index = 1; split_index <= 2; ++split_index) {
        split = split_name[split_index]
        for (field_index = 1; field_index <= 4; ++field_index) {
          update_direct_metric_delta("route.float64_oracle." split "." forecast_field[field_index], "route.direct_float32." split "." forecast_field[field_index])
        }
      }

      print "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_worker_v1"
      print "status=verified"
      print "source_report_sha256=" source_report_sha
      print "phase2a_report_sha256=" phase2a_report_sha
      print "full_report_validation_pass=true"
      print "source_report_line_count=" report_lines
      print "phase2a_report_line_count=" phase_lines
      print "duplicate_mse_pair_count=" duplicate_pair_count
      print "duplicate_mse_strict_1e-12_failure_count=" strict_failure_count
      print "duplicate_mse_mixed_tolerance_pass_count=" mixed_pass_count
      print "duplicate_mse_absolute_tolerance=" abs_tol
      print "duplicate_mse_relative_tolerance=" rel_tol
      printf "duplicate_mse_maximum_absolute_discrepancy=%.17g\n", max_abs_discrepancy
      printf "duplicate_mse_maximum_relative_discrepancy=%.17g\n", max_rel_discrepancy
      printf "duplicate_mse_maximum_tolerance_fraction=%.17g\n", max_tolerance_fraction
      print "classification=" rv("classification")
      print "classification_order_preserved=true"
      print "direct_float32_parity_pass=" rv("direct_float32_parity_pass")
      print "paired_gelu_parity_pass=" rv("paired_gelu_parity_pass")
      print "direct_linear_adam_recovery_gate_pass=" rv("direct_linear_adam_recovery_gate_pass")
      print "direct_linear_adam_clear_failure_gate_pass=" rv("direct_linear_adam_clear_failure_gate_pass")
      print "direct_linear_adam_to_oracle_train_standardized_mse_ratio=" rv("direct_linear_adam_to_oracle_train_standardized_mse_ratio")
      print "direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio=" rv("direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio")
      printf "direct_float32_aggregate_forecast_metric_maximum_absolute_delta=%.17g\n", direct_metric_max_delta
      print "direct_float32_aggregate_forecast_metrics_within_0.001=" (direct_metric_max_delta <= 0.001 ? "true" : "false")
      for (split_index = 1; split_index <= 2; ++split_index) {
        split = split_name[split_index]
        for (field_index = 1; field_index <= 4; ++field_index) {
          field = forecast_field[field_index]
          print "float64_oracle." split "." field "=" rv("route.float64_oracle." split "." field)
          print "direct_float32." split "." field "=" rv("route.direct_float32." split "." field)
        }
      }
      print "probe_bytes_read_by_recovery=false"
      print "binary_executed_by_recovery=false"
      print "new_capture_invocations=0"
      print "new_representation_forward_invocations=0"
      print "new_evaluator_invocations=0"
      print "new_fits=0"
      print "new_optimizer_steps=0"
      print "certified_input_access=false"
      print "final_holdout_access=false"
      print "policy_access=false"
    }
  ' "$PHASE2A_MAIN" "$SOURCE_REPORT"
}

verification_worker() {
  [[ $# == 1 ]] || fail "verification worker requires the attempt SHA-256"
  authorize_worker "$1"
  preflight_static
  validate_attempt
  [[ "$(sha256 "$ATTEMPT")" == "$1" ]] || fail "verification attempt capability mismatch"
  validate_report_single_pass
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
  expect "$path" new_capture_invocations 0
  expect "$path" new_representation_forward_invocations 0
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
}

emit_verification_receipt() {
  local candidate="$1"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_complete_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
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
    echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "probe_bytes_read_by_recovery=false"
    echo "binary_executed_by_recovery=false"
    echo "new_capture_invocations=0"
    echo "new_representation_forward_invocations=0"
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
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_complete_v1
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$path" preregistration_sha256 "$PREREG_SHA"
  expect "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect "$path" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$path" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$path" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$path" source_evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$path" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect "$path" source_preregistration_sha256 "$SOURCE_PREREG_SHA"
  expect "$path" source_evaluator_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$path" source_build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$path" source_build_receipt_sha256 "$SOURCE_BUILD_RECEIPT_SHA"
  expect "$path" source_binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$path" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$path" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$path" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$path" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$path" full_report_validation_pass true
  expect "$path" classification float32_conditioning_failure
  expect "$path" duplicate_mse_pair_count 10
  expect "$path" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$path" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$path" duplicate_mse_strict_1e-12_failure_count 10
  expect "$path" duplicate_mse_mixed_tolerance_pass_count 10
  expect "$path" duplicate_mse_maximum_absolute_discrepancy "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_absolute_discrepancy)"
  expect "$path" duplicate_mse_maximum_relative_discrepancy "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_relative_discrepancy)"
  expect "$path" duplicate_mse_maximum_tolerance_fraction "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_tolerance_fraction)"
  expect "$path" verification_timeout_seconds "$VERIFY_TIMEOUT_SECONDS"
  expect "$path" import_only true
  expect "$path" no_new_scientific_execution true
  expect "$path" probe_bytes_read_by_recovery false
  expect "$path" binary_executed_by_recovery false
  expect "$path" new_capture_invocations 0
  expect "$path" new_representation_forward_invocations 0
  expect "$path" new_evaluator_invocations 0
  expect "$path" new_fits 0
  expect "$path" new_optimizer_steps 0
  expect "$path" certified_input_access false
  expect "$path" final_holdout_access false
  expect "$path" policy_access false
}

emit_result() {
  local candidate="$1" split field
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_development_receipt_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "diagnostic_authority=development_only"
    echo "benchmark_acceptance_authority=false"
    echo "scientific_result_available=true"
    echo "recovery_kind=immutable_predecessor_report_verification"
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
    for split in train validation; do
      for field in directional_accuracy pairwise_rank_accuracy correlation rmse_target_rms_ratio; do
        echo "float64_oracle.${split}.${field}=$(kv "$VERIFICATION_LOG" "float64_oracle.${split}.${field}")"
        echo "direct_float32.${split}.${field}=$(kv "$VERIFICATION_LOG" "direct_float32.${split}.${field}")"
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
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "verification_attempt_path=${ATTEMPT}"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_path=${VERIFICATION_LOG}"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
    echo "verification_receipt_path=${VERIFICATION_RECEIPT}"
    echo "verification_receipt_sha256=$(sha256 "$VERIFICATION_RECEIPT")"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_report_path=${SOURCE_REPORT}"
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
    echo "new_capture_invocations=0"
    echo "new_representation_forward_invocations=0"
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
  local path="$1" split field
  load_kv_file "$path"
  expect "$path" schema_id synthetic_v2_affine_injection_optimizer_localization_verification_recovery_development_receipt_v1
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" diagnostic_authority development_only
  expect "$path" benchmark_acceptance_authority false
  expect "$path" scientific_result_available true
  expect "$path" recovery_kind immutable_predecessor_report_verification
  expect "$path" classification float32_conditioning_failure
  expect "$path" classification_order_preserved true
  expect "$path" direct_float32_parity_pass false
  expect "$path" paired_gelu_parity_pass true
  expect "$path" direct_linear_adam_recovery_gate_pass false
  expect "$path" direct_linear_adam_clear_failure_gate_pass true
  expect "$path" direct_linear_adam_to_oracle_train_standardized_mse_ratio "$(kv "$VERIFICATION_LOG" direct_linear_adam_to_oracle_train_standardized_mse_ratio)"
  expect "$path" direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio "$(kv "$VERIFICATION_LOG" direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio)"
  expect "$path" direct_float32_aggregate_forecast_metric_maximum_absolute_delta "$(kv "$VERIFICATION_LOG" direct_float32_aggregate_forecast_metric_maximum_absolute_delta)"
  expect "$path" direct_float32_aggregate_forecast_metrics_within_0.001 true
  for split in train validation; do
    for field in directional_accuracy pairwise_rank_accuracy correlation rmse_target_rms_ratio; do
      expect "$path" "float64_oracle.${split}.${field}" "$(kv "$VERIFICATION_LOG" "float64_oracle.${split}.${field}")"
      expect "$path" "direct_float32.${split}.${field}" "$(kv "$VERIFICATION_LOG" "direct_float32.${split}.${field}")"
    done
  done
  expect "$path" duplicate_mse_pair_count 10
  expect "$path" duplicate_mse_absolute_tolerance "$DUPLICATE_MSE_ABS_TOL"
  expect "$path" duplicate_mse_relative_tolerance "$DUPLICATE_MSE_REL_TOL"
  expect "$path" duplicate_mse_strict_1e-12_failure_count 10
  expect "$path" duplicate_mse_mixed_tolerance_pass_count 10
  expect "$path" duplicate_mse_maximum_absolute_discrepancy "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_absolute_discrepancy)"
  expect "$path" duplicate_mse_maximum_relative_discrepancy "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_relative_discrepancy)"
  expect "$path" duplicate_mse_maximum_tolerance_fraction "$(kv "$VERIFICATION_LOG" duplicate_mse_maximum_tolerance_fraction)"
  expect "$path" runner_path "$RUNNER"
  expect "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$path" preregistration_path "$PREREG"
  expect "$path" preregistration_sha256 "$PREREG_SHA"
  expect "$path" verification_attempt_path "$ATTEMPT"
  expect "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" verification_log_path "$VERIFICATION_LOG"
  expect "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect "$path" verification_receipt_path "$VERIFICATION_RECEIPT"
  expect "$path" verification_receipt_sha256 "$(sha256 "$VERIFICATION_RECEIPT")"
  expect "$path" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect "$path" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect "$path" source_report_path "$SOURCE_REPORT"
  expect "$path" source_report_sha256 "$SOURCE_REPORT_SHA"
  expect "$path" source_evaluator_log_sha256 "$SOURCE_LOG_SHA"
  expect "$path" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect "$path" source_preregistration_sha256 "$SOURCE_PREREG_SHA"
  expect "$path" source_evaluator_sha256 "$SOURCE_EVALUATOR_SHA"
  expect "$path" source_build_wrapper_sha256 "$SOURCE_BUILD_WRAPPER_SHA"
  expect "$path" source_build_receipt_sha256 "$SOURCE_BUILD_RECEIPT_SHA"
  expect "$path" source_binary_sha256 "$SOURCE_BINARY_SHA"
  expect "$path" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$path" phase2a_report_sha256 "$PHASE2A_REPORT_SHA"
  expect "$path" phase2b_result_sha256 "$PHASE2B_RESULT_SHA"
  expect "$path" phase2b_report_sha256 "$PHASE2B_REPORT_SHA"
  expect "$path" import_only true
  expect "$path" no_new_scientific_execution true
  expect "$path" probe_bytes_read_by_recovery false
  expect "$path" binary_executed_by_recovery false
  expect "$path" new_capture_invocations 0
  expect "$path" new_representation_forward_invocations 0
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
  local rc="$1" stage="$2" reason="$3" candidate="${SCRATCH}/terminal.$$.status"
  [[ ! -e "$TERMINAL" ]] || return 0
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_verification_recovery_terminal_v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_verification_recovery"
    echo "failure_stage=${stage}"
    echo "failure_reason=${reason}"
    echo "exit_code=${rc}"
    echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
    echo "worker_started=${WORKER_STARTED}"
    echo "worker_exit_code=${WORKER_EXIT}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_report_sha256=${SOURCE_REPORT_SHA}"
    echo "verification_log_sha256=$([[ -e "$VERIFICATION_LOG" ]] && sha256 "$VERIFICATION_LOG" || echo not_available)"
    echo "new_capture_invocations=0"
    echo "new_representation_forward_invocations=0"
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

retire_result() {
  [[ -f "$RESULT" && ! -e "$REJECTED_RESULT" ]] || fail "cannot retire incomplete recovery result"
  mv -T -n -- "$RESULT" "$REJECTED_RESULT"
  [[ ! -e "$RESULT" && -f "$REJECTED_RESULT" ]] || fail "could not retire incomplete recovery result"
  require_file "$REJECTED_RESULT" 444
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
  WORKER_EXIT="$child_exit"
  TIMEOUT_PID=""
  TIMEOUT_LAUNCHING=0
  PENDING_SIGNAL=""
  PENDING_SIGNAL_EXIT=""
}

seal_failure_log() {
  if [[ -n "$LOG_CANDIDATE" && -f "$LOG_CANDIDATE" && ! -e "$VERIFICATION_LOG" ]]; then
    publish_once "$LOG_CANDIDATE" "$VERIFICATION_LOG" || true
  fi
}

on_exit() {
  local rc=$?
  trap - EXIT HUP INT TERM QUIT
  set +e
  adopt_timeout_pid_during_launch
  if [[ -n "$TIMEOUT_PID" ]]; then stop_and_reap_timeout_child TERM; fi
  if [[ "$ACTIVE" == 1 && -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_failure_log
    emit_terminal "$rc" "$STAGE" interrupted_or_nonzero_exit
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

verify_development() {
  preflight_static
  require_private_dir "$RUNTIME"
  require_private_dir "$SCRATCH"
  require_file "$LOCK" 600
  [[ -f "$RESULT" && -f "$VERIFICATION_RECEIPT" && -f "$VERIFICATION_LOG" && ! -e "$TERMINAL" ]] ||
    fail "unique recovery result is absent"
  validate_attempt
  validate_worker_log "$VERIFICATION_LOG"
  validate_verification_receipt "$VERIFICATION_RECEIPT"
  validate_result "$RESULT"
}

run_verification() {
  local path attempt_sha receipt_candidate result_candidate rc pending_signal pending_exit
  preflight_local
  open_lock
  acquire_source_lock
  preflight_static
  [[ ! -e "$TERMINAL" ]] || fail "verification-recovery protocol is terminal"
  if [[ -e "$RESULT" ]]; then
    verify_development
    echo "[clear-signal:optimizer-localization-recovery] existing result verified"
    return 0
  fi
  if [[ -e "$ATTEMPT" ]]; then
    ACTIVE=1
    STAGE="orphan_attempt_reentry"
    trap on_exit EXIT
    emit_terminal 1 "$STAGE" incomplete_consumed_attempt
    ACTIVE=0
    trap - EXIT
    fail "the only verification attempt is consumed"
  fi
  for path in "$VERIFICATION_LOG" "$VERIFICATION_RECEIPT" "$REJECTED_RESULT"; do
    [[ ! -e "$path" && ! -L "$path" ]] || fail "non-pristine recovery path: ${path}"
  done
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "recovery scratch is not pristine"

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
  attempt_sha="$(sha256 "$ATTEMPT")"
  LOG_CANDIDATE="${SCRATCH}/verification.log.$$"
  WORKER_STARTED=1
  TIMEOUT_PREVIOUS_ASYNC_PID="${!:-}"
  TIMEOUT_LAUNCHING=1
  timeout --signal=TERM --kill-after="${VERIFY_GRACE_SECONDS}s" "${VERIFY_TIMEOUT_SECONDS}s" \
    "$RUNNER" --verification-worker "$attempt_sha" > "$LOG_CANDIDATE" 2>&1 &
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
  WORKER_EXIT="$rc"
  TIMEOUT_PID=""
  publish_once "$LOG_CANDIDATE" "$VERIFICATION_LOG"
  LOG_CANDIDATE=""
  if [[ "$rc" != 0 ]]; then
    emit_terminal "$rc" "$STAGE" timeout_or_validation_failure
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
  echo "[clear-signal:optimizer-localization-recovery] complete: ${RESULT}"
}

plan() {
  preflight_static
  echo "Project Clear Signal — optimizer-localization verification recovery"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=development_only"
  echo "operation=single_pass_cached_verification_only"
  echo "source_failure_stage=$(kv "$SOURCE_TERMINAL" failure_stage)"
  echo "source_evaluator_exit_code=$(kv "$SOURCE_TERMINAL" evaluator_exit_code)"
  echo "source_report_sha256=${SOURCE_REPORT_SHA}"
  echo "duplicate_mse_pair_count=10"
  echo "duplicate_mse_absolute_tolerance=${DUPLICATE_MSE_ABS_TOL}"
  echo "duplicate_mse_relative_tolerance=${DUPLICATE_MSE_REL_TOL}"
  echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
  echo "new_capture_invocations=0"
  echo "new_representation_forward_invocations=0"
  echo "new_evaluator_invocations=0"
  echo "new_fits=0"
  echo "new_optimizer_steps=0"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "terminal_invalid=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
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
    --run-verification)
      [[ $# == 1 ]] || fail "--run-verification accepts no arguments"
      run_verification
      ;;
    --verify-development)
      [[ $# == 1 ]] || fail "--verify-development accepts no arguments"
      open_existing_lock_read_only
      acquire_source_lock
      verify_development
      echo "[clear-signal:optimizer-localization-recovery] result verified"
      ;;
    --verification-worker)
      [[ $# == 2 ]] || fail "--verification-worker requires one attempt SHA-256"
      verification_worker "$2"
      ;;
    *)
      fail "usage: $0 --plan|--run-verification|--verify-development"
      ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
