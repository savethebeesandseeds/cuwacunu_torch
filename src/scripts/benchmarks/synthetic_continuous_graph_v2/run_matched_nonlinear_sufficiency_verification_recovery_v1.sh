#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly PROTOCOL_ID="synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_v1"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_VERIFICATION_RECOVERY_PREREGISTRATION.md"
readonly PREREG_SHA="2a08ff959910ca6249be747dc028505ba73177127e5b079f1a1307720a25e7d7"

readonly SOURCE_RUNNER="/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_v1.sh"
readonly SOURCE_RUNNER_SHA="ce4c0374254cd85b0689f1338885b2d4f9b816c5f77ed207a6d48374414b9cc3"
readonly SOURCE_ROOT="/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1"
readonly SOURCE_LOCK="${SOURCE_ROOT}/.execution.lock"
readonly SOURCE_ATTEMPT="${SOURCE_ROOT}/attempt.status"
readonly SOURCE_ATTEMPT_SHA="8cb75b9b18b4f21938c46075446099e874bd5ad9ab1223fa0822237e0323eeb9"
readonly SOURCE_TERMINAL="${SOURCE_ROOT}/terminal.invalid.status"
readonly SOURCE_TERMINAL_SHA="2cec20e3d7ab6a8848e2a387c87a255c74a99f64145573b257fc9840cecea902"
readonly SOURCE_REJECTED_RESULT="${SOURCE_ROOT}/rejected.development.status"
readonly SOURCE_REJECTED_RESULT_SHA="49ea95c58e521f846d70b54fb45671fcd7d8fadc1532da75249cb672c3fad99d"
readonly SOURCE_MANIFEST="${SOURCE_ROOT}/artifact.manifest.status"
readonly SOURCE_MANIFEST_SHA="d3640df0982946cc4b7071e0a1a9dc048b33f1bc2f4519a2d1b49a113d2b630f"
readonly SOURCE_NONLINEAR_REPORT="${SOURCE_ROOT}/nonlinear/development.report"
readonly SOURCE_NONLINEAR_REPORT_SHA="34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36"

readonly RUNTIME_ROOT="/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly SCRATCH="${RUNTIME_ROOT}/.scratch"
readonly LOCK="${RUNTIME_ROOT}/.execution.lock"
readonly ATTEMPT="${RUNTIME_ROOT}/verification.attempt.status"
readonly VERIFICATION_LOG="${RUNTIME_ROOT}/verification.log"
readonly VERIFICATION_RECEIPT="${RUNTIME_ROOT}/verification.complete.status"
readonly RESULT="${RUNTIME_ROOT}/development.status"
readonly REJECTED_RESULT="${RUNTIME_ROOT}/rejected.development.status"
readonly TERMINAL="${RUNTIME_ROOT}/terminal.invalid.status"

readonly VERIFY_TIMEOUT_SECONDS=300
readonly VERIFY_GRACE_SECONDS=10

RECOVERY_ACTIVE=0
RECOVERY_STAGE="pre_attempt"

fail() {
  echo "[clear-signal:verification-recovery] ERROR: $*" >&2
  return 1
}

sha256() {
  sha256sum -- "$1" | cut -d' ' -f1
}

require_frozen_current() {
  local path="$1" expected_mode="$2" canonical mode uid links
  [[ "$path" == /* && -f "$path" && ! -L "$path" ]] || fail "not a regular absolute file: ${path}"
  canonical="$(readlink -f -- "$path")"
  [[ "$canonical" == "$path" ]] || fail "non-canonical or symlinked file: ${path}"
  read -r mode uid links < <(stat -c '%a %u %h' -- "$path")
  [[ "$mode" == "$expected_mode" && "$uid" == 0 && "$links" == 1 ]] ||
    fail "invalid frozen metadata for ${path}: ${mode}:${uid}:${links}"
}

require_frozen_exact() {
  local path="$1" expected_sha="$2" expected_mode="$3"
  require_frozen_current "$path" "$expected_mode"
  [[ "$(sha256 "$path")" == "$expected_sha" ]] || fail "SHA-256 mismatch: ${path}"
}

require_private_directory() {
  local path="$1" canonical mode uid
  [[ "$path" == /* && -d "$path" && ! -L "$path" ]] || fail "not a private directory: ${path}"
  canonical="$(readlink -f -- "$path")"
  [[ "$canonical" == "$path" ]] || fail "non-canonical directory: ${path}"
  read -r mode uid < <(stat -c '%a %u' -- "$path")
  [[ "$mode" == 700 && "$uid" == 0 ]] || fail "invalid private-directory metadata: ${path} (${mode}:${uid})"
}

kv() {
  local path="$1" key="$2" line value="" count=0
  while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ "$line" == "${key}="* ]]; then
      value="${line#*=}"
      count=$((count + 1))
    fi
  done < "$path"
  [[ "$count" == 1 ]] || fail "expected one ${key} record in ${path}; found ${count}"
  printf '%s' "$value"
}

expect_kv() {
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv "$path" "$key")"
  [[ "$actual" == "$expected" ]] || fail "${path}: ${key} expected ${expected}, got ${actual}"
}

preflight_local() {
  require_frozen_current "$RUNNER" 555
  require_frozen_exact "$PREREG" "$PREREG_SHA" 444
  require_frozen_exact "$SOURCE_RUNNER" "$SOURCE_RUNNER_SHA" 555
  require_frozen_current "$SOURCE_LOCK" 600
}

preflight_static() {
  preflight_local
  require_frozen_exact "$SOURCE_ATTEMPT" "$SOURCE_ATTEMPT_SHA" 444
  require_frozen_exact "$SOURCE_TERMINAL" "$SOURCE_TERMINAL_SHA" 444
  require_frozen_exact "$SOURCE_REJECTED_RESULT" "$SOURCE_REJECTED_RESULT_SHA" 444
  require_frozen_exact "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA" 444
  require_frozen_exact "$SOURCE_NONLINEAR_REPORT" "$SOURCE_NONLINEAR_REPORT_SHA" 444
  expect_kv "$SOURCE_TERMINAL" status terminal_invalid
  expect_kv "$SOURCE_TERMINAL" failure_stage success_sealing_or_verification
  expect_kv "$SOURCE_TERMINAL" failure_command_ordinal 4
  expect_kv "$SOURCE_TERMINAL" failure_reason_code hard_timeout_or_forced_termination
  expect_kv "$SOURCE_TERMINAL" worker_exit_code 124
  expect_kv "$SOURCE_TERMINAL" rejected_development_receipt_sha256 "$SOURCE_REJECTED_RESULT_SHA"
  expect_kv "$SOURCE_TERMINAL" actual_raw_capture_invocations_validated 2
  expect_kv "$SOURCE_TERMINAL" evaluator_invocations_validated 1
  expect_kv "$SOURCE_TERMINAL" fits_completed 6
  expect_kv "$SOURCE_TERMINAL" optimizer_steps 21000
  expect_kv "$SOURCE_TERMINAL" maximum_anchor_read_upper_bound 2815
  expect_kv "$SOURCE_TERMINAL" same_protocol_retry_allowed false
  expect_kv "$SOURCE_TERMINAL" certified_input_access false
  expect_kv "$SOURCE_TERMINAL" final_holdout_access false
  expect_kv "$SOURCE_TERMINAL" policy_access false
  expect_kv "$SOURCE_REJECTED_RESULT" status complete
  expect_kv "$SOURCE_REJECTED_RESULT" attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect_kv "$SOURCE_REJECTED_RESULT" artifact_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$SOURCE_REJECTED_RESULT" classification "$(kv "$SOURCE_NONLINEAR_REPORT" classification)"
}

acquire_source_lock() {
  require_frozen_current "$SOURCE_LOCK" 600
  exec 8<> "$SOURCE_LOCK"
  flock -s -n 8 || fail "predecessor runtime is active"
}

ensure_runtime() {
  if [[ ! -e "$RUNTIME_ROOT" && ! -L "$RUNTIME_ROOT" ]]; then
    mkdir -m 0700 -- "$RUNTIME_ROOT"
  fi
  require_private_directory "$RUNTIME_ROOT"
  if [[ ! -e "$SCRATCH" && ! -L "$SCRATCH" ]]; then
    mkdir -m 0700 -- "$SCRATCH"
  fi
  require_private_directory "$SCRATCH"
}

open_lock() {
  ensure_runtime
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || true
  fi
  require_frozen_current "$LOCK" 600
  exec 9<> "$LOCK"
  flock -n 9 || fail "verification recovery is already active"
}

publish_once() {
  local candidate="$1" destination="$2"
  [[ -f "$candidate" && ! -L "$candidate" ]] || fail "publication candidate absent: ${candidate}"
  [[ ! -e "$destination" && ! -L "$destination" ]] || fail "publication destination exists: ${destination}"
  chmod 0444 -- "$candidate"
  require_frozen_current "$candidate" 444
  mv -T -n -- "$candidate" "$destination"
  [[ ! -e "$candidate" && -f "$destination" && ! -L "$destination" ]] || fail "atomic publication failed: ${destination}"
  require_frozen_current "$destination" 444
}

emit_attempt() {
  local candidate="${SCRATCH}/verification.attempt.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_attempt_v1"
    echo "status=consumed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_ordinal=1"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "source_runner_path=${SOURCE_RUNNER}"
    echo "source_runner_sha256=${SOURCE_RUNNER_SHA}"
    echo "source_attempt_path=${SOURCE_ATTEMPT}"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_path=${SOURCE_TERMINAL}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_rejected_result_path=${SOURCE_REJECTED_RESULT}"
    echo "source_rejected_result_sha256=${SOURCE_REJECTED_RESULT_SHA}"
    echo "source_manifest_path=${SOURCE_MANIFEST}"
    echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "source_nonlinear_report_path=${SOURCE_NONLINEAR_REPORT}"
    echo "source_nonlinear_report_sha256=${SOURCE_NONLINEAR_REPORT_SHA}"
    echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
    echo "verification_term_grace_seconds=${VERIFY_GRACE_SECONDS}"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "new_capture_invocations=0"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "inherited_validated_captures=2"
    echo "inherited_validated_evaluators=1"
    echo "inherited_fits=6"
    echo "inherited_optimizer_steps=21000"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$ATTEMPT"
}

validate_attempt() {
  require_frozen_current "$ATTEMPT" 444
  expect_kv "$ATTEMPT" schema_id synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_attempt_v1
  expect_kv "$ATTEMPT" status consumed
  expect_kv "$ATTEMPT" protocol_id "$PROTOCOL_ID"
  expect_kv "$ATTEMPT" attempt_ordinal 1
  expect_kv "$ATTEMPT" runner_path "$RUNNER"
  expect_kv "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect_kv "$ATTEMPT" preregistration_path "$PREREG"
  expect_kv "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"
  expect_kv "$ATTEMPT" source_runner_path "$SOURCE_RUNNER"
  expect_kv "$ATTEMPT" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect_kv "$ATTEMPT" source_attempt_path "$SOURCE_ATTEMPT"
  expect_kv "$ATTEMPT" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect_kv "$ATTEMPT" source_terminal_path "$SOURCE_TERMINAL"
  expect_kv "$ATTEMPT" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect_kv "$ATTEMPT" source_rejected_result_path "$SOURCE_REJECTED_RESULT"
  expect_kv "$ATTEMPT" source_rejected_result_sha256 "$SOURCE_REJECTED_RESULT_SHA"
  expect_kv "$ATTEMPT" source_manifest_path "$SOURCE_MANIFEST"
  expect_kv "$ATTEMPT" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$ATTEMPT" source_nonlinear_report_path "$SOURCE_NONLINEAR_REPORT"
  expect_kv "$ATTEMPT" source_nonlinear_report_sha256 "$SOURCE_NONLINEAR_REPORT_SHA"
  expect_kv "$ATTEMPT" verification_timeout_seconds "$VERIFY_TIMEOUT_SECONDS"
  expect_kv "$ATTEMPT" verification_term_grace_seconds "$VERIFY_GRACE_SECONDS"
  expect_kv "$ATTEMPT" import_only true
  expect_kv "$ATTEMPT" no_new_scientific_execution true
  expect_kv "$ATTEMPT" new_capture_invocations 0
  expect_kv "$ATTEMPT" new_evaluator_invocations 0
  expect_kv "$ATTEMPT" new_fits 0
  expect_kv "$ATTEMPT" new_optimizer_steps 0
  expect_kv "$ATTEMPT" inherited_validated_captures 2
  expect_kv "$ATTEMPT" inherited_validated_evaluators 1
  expect_kv "$ATTEMPT" inherited_fits 6
  expect_kv "$ATTEMPT" inherited_optimizer_steps 21000
  expect_kv "$ATTEMPT" retry_allowed false
  expect_kv "$ATTEMPT" resume_allowed false
  expect_kv "$ATTEMPT" certified_input_access false
  expect_kv "$ATTEMPT" final_holdout_access false
  expect_kv "$ATTEMPT" policy_access false
}

predecessor_validate() {
  bash -c '
    set -euo pipefail
    source "$1"
    [[ "$RUNNER" == "$1" && "$(sha256 "$1")" == "$2" ]] || fail "source runner identity mismatch"
    require_frozen_metadata "$3"
    [[ "$(sha256 "$3")" == "$4" ]] || fail "terminal SHA mismatch"
    expect_kv "$3" status terminal_invalid
    expect_kv "$3" failure_stage success_sealing_or_verification
    expect_kv "$3" failure_command_ordinal 4
    expect_kv "$3" failure_reason_code hard_timeout_or_forced_termination
    expect_kv "$3" worker_exit_code 124
    expect_kv "$3" actual_raw_capture_invocations_validated 2
    expect_kv "$3" evaluator_invocations_validated 1
    expect_kv "$3" fits_completed 6
    expect_kv "$3" optimizer_steps 21000
    expect_kv "$3" certified_input_access false
    expect_kv "$3" final_holdout_access false
    expect_kv "$3" policy_access false
    [[ "$REJECTED_RESULT" == "$5" && "$(sha256 "$REJECTED_RESULT")" == "$6" ]] || fail "rejected result identity mismatch"
    [[ "$MANIFEST" == "$7" && "$(sha256 "$MANIFEST")" == "$8" ]] || fail "manifest identity mismatch"
    [[ "$NONLINEAR_REPORT" == "$9" && "$(sha256 "$NONLINEAR_REPORT")" == "${10}" ]] || fail "nonlinear report identity mismatch"
    verify_self_test_receipt
    preflight_scientific_authority
    require_frozen_metadata "$REJECTED_RESULT"
    validate_success_inputs
    verify_manifest
    validate_result_receipt "$REJECTED_RESULT"
    scan_processes
    [[ "$(sha256 "$1")" == "$2" ]] || fail "source runner changed during verification"
    [[ "$(sha256 "$3")" == "$4" ]] || fail "terminal changed during verification"
    [[ "$(sha256 "$REJECTED_RESULT")" == "$6" ]] || fail "rejected result changed during verification"
    [[ "$(sha256 "$MANIFEST")" == "$8" ]] || fail "manifest changed during verification"
    [[ "$(sha256 "$NONLINEAR_REPORT")" == "${10}" ]] || fail "nonlinear report changed during verification"
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_worker_v1"
    echo "status=verified"
    echo "source_runner_sha256=$2"
    echo "predecessor_terminal_sha256=$4"
    echo "rejected_development_receipt_sha256=$6"
    echo "artifact_manifest_sha256=$8"
    echo "nonlinear_report_sha256=${10}"
    echo "inherited_classification=$(kv "$NONLINEAR_REPORT" classification)"
    echo "inherited_raw_strong_seed_count=$(kv "$NONLINEAR_REPORT" arm.raw_history_96.strong_seed_count)"
    echo "inherited_representation_strong_seed_count=$(kv "$NONLINEAR_REPORT" arm.representation_raw96.strong_seed_count)"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  ' recovery-validator "$SOURCE_RUNNER" "$SOURCE_RUNNER_SHA" "$SOURCE_TERMINAL" "$SOURCE_TERMINAL_SHA" \
    "$SOURCE_REJECTED_RESULT" "$SOURCE_REJECTED_RESULT_SHA" "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA" \
    "$SOURCE_NONLINEAR_REPORT" "$SOURCE_NONLINEAR_REPORT_SHA"
}

authorize_worker() {
  local attempt_sha="$1" lock_identity fd_identity
  local -a parent_argv=()
  mapfile -d '' -t parent_argv < "/proc/${PPID}/cmdline"
  [[ "${#parent_argv[@]}" == 7 ]] || fail "verification worker parent argv length mismatch"
  [[ "${parent_argv[0]##*/}" == timeout ]] || fail "verification worker parent is not GNU timeout"
  [[ "${parent_argv[1]}" == "--signal=TERM" ]] || fail "verification worker signal contract mismatch"
  [[ "${parent_argv[2]}" == "--kill-after=${VERIFY_GRACE_SECONDS}s" ]] || fail "verification worker grace contract mismatch"
  [[ "${parent_argv[3]}" == "${VERIFY_TIMEOUT_SECONDS}s" ]] || fail "verification worker timeout contract mismatch"
  [[ "${parent_argv[4]}" == "$RUNNER" && "${parent_argv[5]}" == "--verification-worker" && "${parent_argv[6]}" == "$attempt_sha" ]] ||
    fail "verification worker parent command mismatch"
  [[ -e /proc/$$/fd/9 ]] || fail "verification worker lacks inherited recovery lock"
  lock_identity="$(stat -Lc '%d:%i' -- "$LOCK")"
  fd_identity="$(stat -Lc '%d:%i' -- /proc/$$/fd/9)"
  [[ "$fd_identity" == "$lock_identity" ]] || fail "verification worker lock identity mismatch"
}

verification_worker() {
  [[ $# == 1 ]] || fail "verification worker requires the attempt SHA-256"
  authorize_worker "$1"
  preflight_local
  acquire_source_lock
  preflight_static
  validate_attempt
  [[ "$(sha256 "$ATTEMPT")" == "$1" ]] || fail "verification attempt capability mismatch"
  predecessor_validate
}

validate_worker_log() {
  local path="$1"
  require_frozen_current "$path" 444
  [[ "$(wc -l < "$path")" == 13 ]] || fail "verification log has unexpected line count"
  expect_kv "$path" schema_id synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_worker_v1
  expect_kv "$path" status verified
  expect_kv "$path" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect_kv "$path" predecessor_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect_kv "$path" rejected_development_receipt_sha256 "$SOURCE_REJECTED_RESULT_SHA"
  expect_kv "$path" artifact_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$path" nonlinear_report_sha256 "$SOURCE_NONLINEAR_REPORT_SHA"
  expect_kv "$path" inherited_classification "$(kv "$SOURCE_NONLINEAR_REPORT" classification)"
  expect_kv "$path" inherited_raw_strong_seed_count 3
  expect_kv "$path" inherited_representation_strong_seed_count 0
  expect_kv "$path" certified_input_access false
  expect_kv "$path" final_holdout_access false
  expect_kv "$path" policy_access false
}

emit_result() {
  local candidate="$1"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_receipt_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "diagnostic_authority=development_only"
    echo "development_only=true"
    echo "benchmark_acceptance_authority=false"
    echo "classification=$(kv "$SOURCE_NONLINEAR_REPORT" classification)"
    echo "recovery_kind=immutable_predecessor_artifact_reverification"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "verification_attempt_path=${ATTEMPT}"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_path=${VERIFICATION_LOG}"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
    echo "source_runner_path=${SOURCE_RUNNER}"
    echo "source_runner_sha256=${SOURCE_RUNNER_SHA}"
    echo "source_attempt_path=${SOURCE_ATTEMPT}"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_path=${SOURCE_TERMINAL}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_rejected_result_path=${SOURCE_REJECTED_RESULT}"
    echo "source_rejected_result_sha256=${SOURCE_REJECTED_RESULT_SHA}"
    echo "source_manifest_path=${SOURCE_MANIFEST}"
    echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "source_nonlinear_report_path=${SOURCE_NONLINEAR_REPORT}"
    echo "source_nonlinear_report_sha256=${SOURCE_NONLINEAR_REPORT_SHA}"
    echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
    echo "full_frozen_validator_pass=true"
    echo "verification_receipt_path=${VERIFICATION_RECEIPT}"
    echo "verification_receipt_sha256=$(sha256 "$VERIFICATION_RECEIPT")"
    echo "new_capture_invocations=0"
    echo "new_evaluator_invocations=0"
    echo "new_fits=0"
    echo "new_optimizer_steps=0"
    echo "inherited_validated_captures=2"
    echo "inherited_validated_evaluators=1"
    echo "inherited_fits=6"
    echo "inherited_optimizer_steps=21000"
    echo "maximum_anchor_read=2815"
    echo "inherited.raw_history_96.strong_seed_count=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.strong_seed_count)"
    echo "inherited.raw_history_96.pass=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.pass)"
    echo "inherited.raw_history_96.median.validation.directional_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.directional_accuracy)"
    echo "inherited.raw_history_96.median.validation.pairwise_rank_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.pairwise_rank_accuracy)"
    echo "inherited.raw_history_96.median.validation.correlation=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.correlation)"
    echo "inherited.raw_history_96.median.validation.rmse_target_rms_ratio=$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.rmse_target_rms_ratio)"
    echo "inherited.representation_raw96.strong_seed_count=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.strong_seed_count)"
    echo "inherited.representation_raw96.pass=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.pass)"
    echo "inherited.representation_raw96.median.train.directional_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.directional_accuracy)"
    echo "inherited.representation_raw96.median.train.pairwise_rank_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.pairwise_rank_accuracy)"
    echo "inherited.representation_raw96.median.train.correlation=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.correlation)"
    echo "inherited.representation_raw96.median.train.rmse_target_rms_ratio=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.rmse_target_rms_ratio)"
    echo "inherited.representation_raw96.median.validation.directional_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.directional_accuracy)"
    echo "inherited.representation_raw96.median.validation.pairwise_rank_accuracy=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.pairwise_rank_accuracy)"
    echo "inherited.representation_raw96.median.validation.correlation=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.correlation)"
    echo "inherited.representation_raw96.median.validation.rmse_target_rms_ratio=$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.rmse_target_rms_ratio)"
    echo "scientific_result_available=true"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
  } > "$candidate"
}

validate_result_receipt() {
  local path="$1"
  require_frozen_current "$path" 444
  expect_kv "$path" schema_id synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_development_receipt_v1
  expect_kv "$path" status complete
  expect_kv "$path" protocol_id "$PROTOCOL_ID"
  expect_kv "$path" diagnostic_authority development_only
  expect_kv "$path" development_only true
  expect_kv "$path" benchmark_acceptance_authority false
  expect_kv "$path" classification "$(kv "$SOURCE_NONLINEAR_REPORT" classification)"
  expect_kv "$path" recovery_kind immutable_predecessor_artifact_reverification
  expect_kv "$path" import_only true
  expect_kv "$path" no_new_scientific_execution true
  expect_kv "$path" runner_path "$RUNNER"
  expect_kv "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect_kv "$path" preregistration_path "$PREREG"
  expect_kv "$path" preregistration_sha256 "$PREREG_SHA"
  expect_kv "$path" verification_attempt_path "$ATTEMPT"
  expect_kv "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect_kv "$path" verification_log_path "$VERIFICATION_LOG"
  expect_kv "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect_kv "$path" source_runner_path "$SOURCE_RUNNER"
  expect_kv "$path" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect_kv "$path" source_attempt_path "$SOURCE_ATTEMPT"
  expect_kv "$path" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect_kv "$path" source_terminal_path "$SOURCE_TERMINAL"
  expect_kv "$path" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect_kv "$path" source_rejected_result_path "$SOURCE_REJECTED_RESULT"
  expect_kv "$path" source_rejected_result_sha256 "$SOURCE_REJECTED_RESULT_SHA"
  expect_kv "$path" source_manifest_path "$SOURCE_MANIFEST"
  expect_kv "$path" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$path" source_nonlinear_report_path "$SOURCE_NONLINEAR_REPORT"
  expect_kv "$path" source_nonlinear_report_sha256 "$SOURCE_NONLINEAR_REPORT_SHA"
  expect_kv "$path" verification_timeout_seconds "$VERIFY_TIMEOUT_SECONDS"
  expect_kv "$path" full_frozen_validator_pass true
  expect_kv "$path" verification_receipt_path "$VERIFICATION_RECEIPT"
  expect_kv "$path" verification_receipt_sha256 "$(sha256 "$VERIFICATION_RECEIPT")"
  expect_kv "$path" new_capture_invocations 0
  expect_kv "$path" new_evaluator_invocations 0
  expect_kv "$path" new_fits 0
  expect_kv "$path" new_optimizer_steps 0
  expect_kv "$path" inherited_validated_captures 2
  expect_kv "$path" inherited_validated_evaluators 1
  expect_kv "$path" inherited_fits 6
  expect_kv "$path" inherited_optimizer_steps 21000
  expect_kv "$path" maximum_anchor_read 2815
  expect_kv "$path" inherited.raw_history_96.strong_seed_count 3
  expect_kv "$path" inherited.raw_history_96.pass true
  expect_kv "$path" inherited.raw_history_96.median.validation.directional_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.directional_accuracy)"
  expect_kv "$path" inherited.raw_history_96.median.validation.pairwise_rank_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.pairwise_rank_accuracy)"
  expect_kv "$path" inherited.raw_history_96.median.validation.correlation "$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.correlation)"
  expect_kv "$path" inherited.raw_history_96.median.validation.rmse_target_rms_ratio "$(kv "$SOURCE_NONLINEAR_REPORT" arm.raw_history_96.median.validation.rmse_target_rms_ratio)"
  expect_kv "$path" inherited.representation_raw96.strong_seed_count 0
  expect_kv "$path" inherited.representation_raw96.pass false
  expect_kv "$path" inherited.representation_raw96.median.train.directional_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.directional_accuracy)"
  expect_kv "$path" inherited.representation_raw96.median.train.pairwise_rank_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.pairwise_rank_accuracy)"
  expect_kv "$path" inherited.representation_raw96.median.train.correlation "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.correlation)"
  expect_kv "$path" inherited.representation_raw96.median.train.rmse_target_rms_ratio "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.train.rmse_target_rms_ratio)"
  expect_kv "$path" inherited.representation_raw96.median.validation.directional_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.directional_accuracy)"
  expect_kv "$path" inherited.representation_raw96.median.validation.pairwise_rank_accuracy "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.pairwise_rank_accuracy)"
  expect_kv "$path" inherited.representation_raw96.median.validation.correlation "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.correlation)"
  expect_kv "$path" inherited.representation_raw96.median.validation.rmse_target_rms_ratio "$(kv "$SOURCE_NONLINEAR_REPORT" arm.representation_raw96.median.validation.rmse_target_rms_ratio)"
  expect_kv "$path" scientific_result_available true
  expect_kv "$path" certified_input_access false
  expect_kv "$path" final_holdout_access false
  expect_kv "$path" policy_access false
  expect_kv "$path" retry_allowed false
  expect_kv "$path" resume_allowed false
}

emit_verification_receipt() {
  local candidate="$1"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_complete_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "verification_attempt_path=${ATTEMPT}"
    echo "verification_attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "verification_log_path=${VERIFICATION_LOG}"
    echo "verification_log_sha256=$(sha256 "$VERIFICATION_LOG")"
    echo "source_runner_sha256=${SOURCE_RUNNER_SHA}"
    echo "source_attempt_sha256=${SOURCE_ATTEMPT_SHA}"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_rejected_result_sha256=${SOURCE_REJECTED_RESULT_SHA}"
    echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "source_nonlinear_report_sha256=${SOURCE_NONLINEAR_REPORT_SHA}"
    echo "full_frozen_validator_pass=true"
    echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
    echo "import_only=true"
    echo "no_new_scientific_execution=true"
    echo "new_capture_invocations=0"
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
  require_frozen_current "$path" 444
  expect_kv "$path" schema_id synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_complete_v1
  expect_kv "$path" status complete
  expect_kv "$path" protocol_id "$PROTOCOL_ID"
  expect_kv "$path" runner_path "$RUNNER"
  expect_kv "$path" runner_sha256 "$(sha256 "$RUNNER")"
  expect_kv "$path" preregistration_path "$PREREG"
  expect_kv "$path" preregistration_sha256 "$PREREG_SHA"
  expect_kv "$path" verification_attempt_path "$ATTEMPT"
  expect_kv "$path" verification_attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect_kv "$path" verification_log_path "$VERIFICATION_LOG"
  expect_kv "$path" verification_log_sha256 "$(sha256 "$VERIFICATION_LOG")"
  expect_kv "$path" source_runner_sha256 "$SOURCE_RUNNER_SHA"
  expect_kv "$path" source_attempt_sha256 "$SOURCE_ATTEMPT_SHA"
  expect_kv "$path" source_terminal_sha256 "$SOURCE_TERMINAL_SHA"
  expect_kv "$path" source_rejected_result_sha256 "$SOURCE_REJECTED_RESULT_SHA"
  expect_kv "$path" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$path" source_nonlinear_report_sha256 "$SOURCE_NONLINEAR_REPORT_SHA"
  expect_kv "$path" full_frozen_validator_pass true
  expect_kv "$path" verification_timeout_seconds "$VERIFY_TIMEOUT_SECONDS"
  expect_kv "$path" import_only true
  expect_kv "$path" no_new_scientific_execution true
  expect_kv "$path" new_capture_invocations 0
  expect_kv "$path" new_evaluator_invocations 0
  expect_kv "$path" new_fits 0
  expect_kv "$path" new_optimizer_steps 0
  expect_kv "$path" certified_input_access false
  expect_kv "$path" final_holdout_access false
  expect_kv "$path" policy_access false
}

emit_terminal() {
  local rc="$1" stage="$2" reason="$3" candidate="${SCRATCH}/terminal.$$"
  [[ ! -e "$TERMINAL" ]] || return 0
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_verification_recovery_terminal_invalid_v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_verification_recovery"
    echo "failure_stage=${stage}"
    echo "failure_reason_code=${reason}"
    echo "exit_code=${rc}"
    echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
    echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
    echo "source_rejected_result_sha256=${SOURCE_REJECTED_RESULT_SHA}"
    echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "verification_log_sha256=$([[ -e "$VERIFICATION_LOG" ]] && sha256 "$VERIFICATION_LOG" || echo not_available)"
    echo "verification_receipt_sha256=$([[ -e "$VERIFICATION_RECEIPT" ]] && sha256 "$VERIFICATION_RECEIPT" || echo not_available)"
    echo "new_capture_invocations=0"
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
  [[ -f "$RESULT" && ! -e "$REJECTED_RESULT" ]] || fail "cannot retire recovery result"
  mv -T -n -- "$RESULT" "$REJECTED_RESULT"
  [[ ! -e "$RESULT" && -f "$REJECTED_RESULT" ]] || fail "could not retire recovery result"
  require_frozen_current "$REJECTED_RESULT" 444
}

recovery_exit_guard() {
  local rc="$1"
  trap - EXIT HUP INT TERM
  if [[ "$RECOVERY_ACTIVE" == 1 && -e "$ATTEMPT" && ! -e "$TERMINAL" ]]; then
    if [[ -e "$RESULT" ]]; then
      retire_result || exit "$rc"
    fi
    [[ ! -e "$RESULT" ]] || exit "$rc"
    emit_terminal "$rc" "$RECOVERY_STAGE" interrupted_or_nonzero_exit || true
  fi
  exit "$rc"
}

verify_development() {
  preflight_local
  acquire_source_lock
  preflight_static
  require_private_directory "$RUNTIME_ROOT"
  require_private_directory "$SCRATCH"
  [[ -f "$RESULT" && -f "$VERIFICATION_RECEIPT" && ! -e "$TERMINAL" ]] || fail "unique recovery result is absent"
  validate_attempt
  validate_worker_log "$VERIFICATION_LOG"
  validate_verification_receipt "$VERIFICATION_RECEIPT"
  validate_result_receipt "$RESULT"
}

run_verification() {
  local path attempt_sha log_candidate verification_candidate result_candidate rc
  preflight_local
  open_lock
  acquire_source_lock
  preflight_static
  [[ ! -e "$TERMINAL" ]] || fail "verification-recovery protocol is terminal"
  if [[ -e "$RESULT" ]]; then
    if [[ -e "$VERIFICATION_RECEIPT" ]]; then
      verify_development
      echo "[clear-signal:verification-recovery] existing result verified"
      return
    fi
    retire_result
    emit_terminal 1 result_integrity incomplete_committed_result
    fail "committed result lacks its verification receipt"
  fi
  if [[ -e "$ATTEMPT" ]]; then
    RECOVERY_ACTIVE=1
    RECOVERY_STAGE="orphan_attempt_reentry"
    trap 'recovery_exit_guard "$?"' EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    emit_terminal 1 "$RECOVERY_STAGE" incomplete_consumed_attempt
    RECOVERY_ACTIVE=0
    trap - EXIT HUP INT TERM
    fail "the only verification attempt is consumed"
  fi
  for path in "$VERIFICATION_LOG" "$VERIFICATION_RECEIPT" "$REJECTED_RESULT"; do
    [[ ! -e "$path" && ! -L "$path" ]] || fail "non-pristine recovery path: ${path}"
  done
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "recovery scratch is not pristine"
  RECOVERY_ACTIVE=1
  RECOVERY_STAGE="attempt_publication"
  trap 'recovery_exit_guard "$?"' EXIT
  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  emit_attempt
  RECOVERY_STAGE="frozen_bundle_verification"
  attempt_sha="$(sha256 "$ATTEMPT")"
  log_candidate="${SCRATCH}/verification.log.$$"
  set +e
  timeout --signal=TERM --kill-after="${VERIFY_GRACE_SECONDS}s" "${VERIFY_TIMEOUT_SECONDS}s" \
    "$RUNNER" --verification-worker "$attempt_sha" > "$log_candidate" 2>&1
  rc=$?
  set -e
  publish_once "$log_candidate" "$VERIFICATION_LOG"
  if [[ "$rc" != 0 ]]; then
    emit_terminal "$rc" "$RECOVERY_STAGE" timeout_or_validation_failure
    RECOVERY_ACTIVE=0
    trap - EXIT HUP INT TERM
    fail "verification recovery failed before publication: ${rc}"
  fi
  validate_worker_log "$VERIFICATION_LOG"
  verification_candidate="${SCRATCH}/verification.complete.status.$$"
  emit_verification_receipt "$verification_candidate"
  chmod 0444 -- "$verification_candidate"
  validate_verification_receipt "$verification_candidate"
  publish_once "$verification_candidate" "$VERIFICATION_RECEIPT"
  validate_verification_receipt "$VERIFICATION_RECEIPT"
  result_candidate="${SCRATCH}/development.status.$$"
  emit_result "$result_candidate"
  chmod 0444 -- "$result_candidate"
  validate_result_receipt "$result_candidate"
  RECOVERY_STAGE="final_result_commit"
  trap '' HUP INT TERM
  publish_once "$result_candidate" "$RESULT"
  RECOVERY_ACTIVE=0
  trap - EXIT HUP INT TERM
  echo "[clear-signal:verification-recovery] complete: ${RESULT}"
}

plan() {
  preflight_static
  echo "Project Clear Signal — immutable verification recovery"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=development_only"
  echo "operation=verification_only"
  echo "new_capture_invocations=0"
  echo "new_evaluator_invocations=0"
  echo "new_fits=0"
  echo "new_optimizer_steps=0"
  echo "inherited_validated_captures=2"
  echo "inherited_validated_evaluators=1"
  echo "inherited_fits=6"
  echo "inherited_optimizer_steps=21000"
  echo "verification_timeout_seconds=${VERIFY_TIMEOUT_SECONDS}"
  echo "source_terminal_sha256=${SOURCE_TERMINAL_SHA}"
  echo "source_rejected_result_sha256=${SOURCE_REJECTED_RESULT_SHA}"
  echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "verification_receipt_present=$([[ -e "$VERIFICATION_RECEIPT" ]] && echo true || echo false)"
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
      verify_development
      echo "[clear-signal:verification-recovery] result verified"
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
