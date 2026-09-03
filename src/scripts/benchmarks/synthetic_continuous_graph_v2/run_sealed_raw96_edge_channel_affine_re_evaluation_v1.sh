#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_sealed_raw96_edge_channel_affine_re_evaluation_development_v1"
readonly DIAGNOSTIC_PHASE="sealed_raw96_edge_channel_affine_re_evaluation"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/SEALED_RAW96_EDGE_CHANNEL_AFFINE_RE_EVALUATION_PREREGISTRATION.md"
readonly PREREG_SHA="0d4671e3526c27d4498f29e416798ed14b89d9cf872f8b220838c24127f20ff5"

readonly PHASE2A_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_encoder_channel_conditioned_affine_probe.cpp"
readonly PHASE2A_SOURCE_SHA="5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570"
readonly CANONICAL_PARSER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp"
readonly CANONICAL_PARSER_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly PHASE2A_BUILD_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_encoder_channel_conditioned_affine_probe.sh"
readonly PHASE2A_BUILD_WRAPPER_SHA="284b6cd4cef37b7cb965d9e92f1f55a5ab0aa02743d9553b13a71c25c21e0324"
readonly PHASE2A_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1"
readonly PHASE2A_RECEIPT="${PHASE2A_ROOT}/development.status"
readonly PHASE2A_RECEIPT_SHA="b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5"
readonly PHASE2A_BINARY="${PHASE2A_ROOT}/bin/frozen_encoder_channel_conditioned_affine_probe"
readonly PHASE2A_BINARY_SHA="efc2ece40bb0ab727447d12fe388060c9573e2dafebc9df2a9889ac510ba647d"
readonly PHASE2A_REFERENCE_REPORT_SHA="2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a"

readonly SERVING_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1"
readonly SERVING_RESULT="${SERVING_ROOT}/development.status"
readonly SERVING_RESULT_SHA="9fd91759db1665f534f5a2a304b19399991673a9bcb3ca2c380857e72858aee5"
readonly SERVING_LOCK="${SERVING_ROOT}/.execution.lock"
readonly SERVING_TRAIN_CAPTURE="${SERVING_ROOT}/capture/train/capture.report"
readonly SERVING_TRAIN_CAPTURE_SHA="5079c1beaa7efcfcb7dc4b519097b68e8a4222a4847f697735e5699205214d57"
readonly SERVING_VALIDATION_CAPTURE="${SERVING_ROOT}/capture/validation/capture.report"
readonly SERVING_VALIDATION_CAPTURE_SHA="439df8fe37b548005b4b6c3f50fd8e8ce9b104bd96a09a280edc68c07328c4f3"

readonly ABLATION_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3"
readonly ABLATION_RESULT="${ABLATION_ROOT}/development.status"
readonly ABLATION_RESULT_SHA="6b4a422d5045b1e2a6d4ffb47e28d750cbdcabdf8216eded4f8fef00d41d012d"
readonly ABLATION_SELECTION="${ABLATION_ROOT}/selection.status"
readonly ABLATION_SELECTION_SHA="4df2a2a5596c5d9139f6ce24a5b588e3f44e89577926c699b2ea6f0f957747bb"
readonly ABLATION_LOCK="${ABLATION_ROOT}/.development.lock"

readonly CANONICAL_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2"
readonly CANONICAL_RESULT="${CANONICAL_ROOT}/development.status"
readonly CANONICAL_RESULT_SHA="fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6"
readonly CANONICAL_LOCK="${CANONICAL_ROOT}/.execution.lock"
readonly CANONICAL_IMPORT="${ABLATION_ROOT}/arms/canonical/import.status"
readonly CANONICAL_IMPORT_SHA="dd7e07cd562bda53a2d48be60a19be169162f89dccc38feb9be1fee11f17ac25"
readonly CANONICAL_TRAIN="${CANONICAL_ROOT}/jobs/train/representation_edge_features.probe"
readonly CANONICAL_VALIDATION="${CANONICAL_ROOT}/jobs/validation/representation_edge_features.probe"

readonly ENDPOINT_RECEIPT="${ABLATION_ROOT}/arms/endpoint_scale/capture.status"
readonly ENDPOINT_RECEIPT_SHA="6db9e6cca7cdf60676af6e0af1dad66f5b8428a91a944e90a866b72af799b5b2"
readonly TIME_ONLY_RECEIPT="${ABLATION_ROOT}/arms/time_only/capture.status"
readonly TIME_ONLY_RECEIPT_SHA="1977584999c386c0eac025a244498f9a7b8da7e98e9607aa373ae1e741f1a433"
readonly NO_TF_RECEIPT="${ABLATION_ROOT}/arms/no_tf_alignment/capture.status"
readonly NO_TF_RECEIPT_SHA="0c9be7e9ccf0b8fe4e7cb2b5076f9fe430abdb57ea703a1637df05df8d425f52"

readonly -a ARM_IDS=(
  all_tokens
  pool_time_tokens
  pool_frequency_tokens
  pool_domain_balanced
  endpoint_scale
  time_only
  no_tf_alignment
)
readonly -a ARM_FAMILIES=(
  serving
  serving
  serving
  serving
  ablation
  ablation
  ablation
)
readonly -a ARM_TRAIN=(
  "${SERVING_ROOT}/capture/train/all_tokens.probe"
  "${SERVING_ROOT}/capture/train/pool_time_tokens.probe"
  "${SERVING_ROOT}/capture/train/pool_frequency_tokens.probe"
  "${SERVING_ROOT}/capture/train/pool_domain_balanced.probe"
  "${ABLATION_ROOT}/arms/endpoint_scale/capture/train/representation_edge_features.probe"
  "${ABLATION_ROOT}/arms/time_only/capture/train/representation_edge_features.probe"
  "${ABLATION_ROOT}/arms/no_tf_alignment/capture/train/representation_edge_features.probe"
)
readonly -a ARM_TRAIN_SHA=(
  d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75
  2c27b4983977bb6bfbf8f5449bd8f098d64bf825d12c0ff49ba82c59cee39656
  58e0ab63547739156e1db4288126cf808adeb16f037153d9a61bf2268354ab52
  1a99c7f85978232b74138aa1cc17da56277396e7c222f730cbd9dac2b1ca6dd4
  d79a2ff6eb4c615ade6e8ab4d476a0f576d8e98ca69b940dacf154251472ee38
  601f493c2f4eb801ea963c30afccbc39b36c9d7ff256875eea0564873d4722ca
  4db8424b6deeea9bff4a96963ff54d3051fc6adf375cfa45aa4195b33e639af7
)
readonly -a ARM_VALIDATION=(
  "${SERVING_ROOT}/capture/validation/all_tokens.probe"
  "${SERVING_ROOT}/capture/validation/pool_time_tokens.probe"
  "${SERVING_ROOT}/capture/validation/pool_frequency_tokens.probe"
  "${SERVING_ROOT}/capture/validation/pool_domain_balanced.probe"
  "${ABLATION_ROOT}/arms/endpoint_scale/capture/validation/representation_edge_features.probe"
  "${ABLATION_ROOT}/arms/time_only/capture/validation/representation_edge_features.probe"
  "${ABLATION_ROOT}/arms/no_tf_alignment/capture/validation/representation_edge_features.probe"
)
readonly -a ARM_VALIDATION_SHA=(
  8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
  1c9a4ef281d559d7cdd496ad71320ce131494ac36384713d9955a8f6f3cc8e1e
  27e79a43f8521d40a8011c97fce2d14e34d2b286a3473c8017867cdbb621f0b2
  af790faff213f27076c9a6fc18e0ca5c8ecf55f0ec4eaf75b601031da1b6ef42
  78c7e8139c6a3de5ea6d19fdfea2c50eb880c46ff406a7618835d6dc39e7c740
  c44cd11c8e222197f970f15ccb7d373eb95ffcd1b7c6ae126ea7defae51275cc
  0517678e2c724a8ce4f47b95472b90809c8487770d917e87a39042c64d9e853f
)
readonly -a ARM_SOURCE_RECEIPTS=(
  "${SERVING_RESULT}"
  "${SERVING_RESULT}"
  "${SERVING_RESULT}"
  "${SERVING_RESULT}"
  "${ENDPOINT_RECEIPT}"
  "${TIME_ONLY_RECEIPT}"
  "${NO_TF_RECEIPT}"
)
readonly -a ARM_SOURCE_RECEIPT_SHA=(
  "${SERVING_RESULT_SHA}"
  "${SERVING_RESULT_SHA}"
  "${SERVING_RESULT_SHA}"
  "${SERVING_RESULT_SHA}"
  "${ENDPOINT_RECEIPT_SHA}"
  "${TIME_ONLY_RECEIPT_SHA}"
  "${NO_TF_RECEIPT_SHA}"
)

readonly TRAIN_PROJECTION_SHA="f7a935fe83bb5e72388c63a4ab6e063d7908913f4cc52a5fcb652ead5e7dd08d"
readonly VALIDATION_PROJECTION_SHA="c1575a9936cca22f24c2e40908c8196c64bdfc7f89cdf15f70e822e92b16ec22"
readonly COMPLETE_REPORT_KEY_COUNT=234
readonly COMPLETE_REPORT_SORTED_KEY_SHA="67084d03ed7c441d539fe97c50114d8a13d1e90b42530e9c52eddd7476bef5f1"
readonly WORKER_TIMEOUT_SECONDS=900
readonly TERM_GRACE_SECONDS=10
readonly UNIQUE_PAIR_COUNT=7
readonly LOGICAL_ARM_COUNT=8
readonly MAX_EVALUATOR_INVOCATIONS=14
readonly RIDGE_CANDIDATE_COUNT=6
readonly HEAD_COUNT=9
readonly TIE_TOLERANCE="1e-12"
readonly STRONG_DIRECTION_MIN="0.95"
readonly STRONG_RANK_MIN="0.95"
readonly STRONG_CORRELATION_MIN="0.95"
readonly STRONG_RMSE_RATIO_MAX="0.25"
readonly PARTIAL_DIRECTION_MIN="0.80"
readonly PARTIAL_RANK_MIN="0.78"
readonly PASS_CLASS="sealed_v2_raw96_edge_channel_affine_strong_gate_observed_development_only"
readonly NO_PASS_CLASS="sealed_v2_raw96_edge_channel_affine_strong_gate_not_observed"

readonly RUNTIME="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly PREP_DIR="${RUNTIME}/prepared"
readonly EVIDENCE="${RUNTIME}/evidence"
readonly SCRATCH="${RUNTIME}/.scratch"
readonly LOCK="${RUNTIME}/.execution.lock"
readonly BIN="${PREP_DIR}/frozen_encoder_channel_conditioned_affine_probe"
readonly PREP_RECEIPT="${PREP_DIR}/prepared.status"
readonly ATTEMPT="${RUNTIME}/attempt.status"
readonly PROJECTION_COMPLETE="${EVIDENCE}/projection.complete.status"
readonly ALIAS_INPUT_RECEIPT="${EVIDENCE}/canonical_alias.input.status"
readonly ALIAS_REPORT_RECEIPT="${EVIDENCE}/canonical_alias.report.status"
readonly SCIENCE_COMPLETE="${EVIDENCE}/science.complete.status"
readonly WORKER_LOG="${EVIDENCE}/worker.log"
readonly RESULT="${RUNTIME}/development.status"
readonly REJECTED_RESULT="${RUNTIME}/rejected.development.status"
readonly REJECTED_EVIDENCE_MANIFEST="${RUNTIME}/rejected.evidence.sha256"
readonly TERMINAL="${RUNTIME}/terminal.invalid.status"
readonly WORKER_LOG_CANDIDATE="${SCRATCH}/worker.log.candidate"
readonly RESULT_CANDIDATE="${SCRATCH}/development.status.candidate"
readonly CAPABILITY_CANDIDATE="${SCRATCH}/worker.capability"

declare -A KV_VALUE=()
declare -A KV_PRESENT=()
declare -A KV_LOADED=()
declare -A ARM_DIRECTION=()
declare -A ARM_RANK=()
declare -A ARM_CORRELATION=()
declare -A ARM_RMSE=()
declare -A ARM_RMSE_RATIO=()
declare -A ARM_STRONG=()
declare -A ARM_SELECTED_INDEX=()
declare -A ARM_SELECTED_RIDGE=()

RUN_TIMEOUT_PID=""
RUN_LAUNCHING=0
RUN_PENDING_SIGNAL=""
RUN_EXIT_GUARD_ACTIVE=0
RUN_CAPABILITY_PATH=""
RUN_CAPABILITY_IDENTITY=""
RUN_CHILD_EXIT="not_started"
RUN_TERMINAL_SEAL_SEQUENCE=0
RUN_FAILURE_STAGE="bounded_worker"
RUN_FAILURE_REASON="worker_failure"

fail() {
  echo "[clear-signal:sealed-raw96-affine] ERROR: $*" >&2
  return 1
}

sha256() { sha256sum -- "$1" | cut -d' ' -f1; }

sorted_key_sha256() {
  awk -F= '{print $1}' "$1" | LC_ALL=C sort | sha256sum | cut -d' ' -f1
}

require_file() {
  local path="$1" expected_mode="$2" canonical mode uid links
  if [[ "$path" != /* || ! -f "$path" || -L "$path" ]]; then
    fail "not a regular absolute file: ${path}"
    return 1
  fi
  if ! canonical="$(readlink -f -- "$path")"; then
    fail "could not canonicalize file: ${path}"
    return 1
  fi
  if [[ "$canonical" != "$path" ]]; then
    fail "noncanonical or symlinked file: ${path}"
    return 1
  fi
  if ! read -r mode uid links < <(stat -c '%a %u %h' -- "$path"); then
    fail "could not stat file: ${path}"
    return 1
  fi
  if [[ "$mode" != "$expected_mode" || "$uid" != 0 || "$links" != 1 ]]; then
    fail "invalid frozen metadata for ${path}: ${mode}:${uid}:${links}"
    return 1
  fi
}

require_exact() {
  local observed
  require_file "$1" "$3" || return 1
  observed="$(sha256 "$1")" || return 1
  if [[ "$observed" != "$2" ]]; then
    fail "SHA-256 mismatch: $1"
    return 1
  fi
}

require_private_dir() {
  local path="$1" canonical mode uid
  if [[ "$path" != /* || ! -d "$path" || -L "$path" ]]; then
    fail "not a private directory: ${path}"
    return 1
  fi
  if ! canonical="$(readlink -f -- "$path")"; then
    fail "could not canonicalize directory: ${path}"
    return 1
  fi
  if [[ "$canonical" != "$path" ]]; then
    fail "noncanonical directory: ${path}"
    return 1
  fi
  if ! read -r mode uid < <(stat -c '%a %u' -- "$path"); then
    fail "could not stat directory: ${path}"
    return 1
  fi
  if [[ "$mode" != 700 || "$uid" != 0 ]]; then
    fail "invalid private directory metadata: ${path} (${mode}:${uid})"
    return 1
  fi
}

load_kv_file() {
  local path="$1" line key value cache_key
  [[ -z "${KV_LOADED[$path]+x}" ]] || return 0
  require_file "$path" 444 || return 1
  while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ -z "$line" || "$line" != *=* || "$line" == *$'\r' ]]; then
      fail "malformed key-value line in ${path}"
      return 1
    fi
    key="${line%%=*}"
    value="${line#*=}"
    if [[ -z "$key" ]]; then
      fail "empty key in ${path}"
      return 1
    fi
    cache_key="${path}"$'\034'"${key}"
    if [[ -n "${KV_PRESENT[$cache_key]+x}" ]]; then
      fail "duplicate key ${key} in ${path}"
      return 1
    fi
    KV_PRESENT["$cache_key"]=1
    KV_VALUE["$cache_key"]="$value"
  done < "$path"
  KV_LOADED["$path"]=1
}

kv() {
  local path="$1" key="$2" cache_key="${1}"$'\034'"${2}"
  if [[ -z "${KV_LOADED[$path]+x}" ]]; then
    fail "uncached key-value file: ${path}"
    return 1
  fi
  if [[ -z "${KV_PRESENT[$cache_key]+x}" ]]; then
    fail "missing key ${key} in ${path}"
    return 1
  fi
  printf '%s' "${KV_VALUE[$cache_key]}"
}

expect() {
  local actual
  actual="$(kv "$1" "$2")" || return 1
  if [[ "$actual" != "$3" ]]; then
    fail "$1: $2 expected '$3', got '$actual'"
    return 1
  fi
}

finite_number() {
  awk -v x="$1" 'BEGIN {
    if (x !~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/) exit 1
    y=x+0
    exit !((y-y)==0)
  }'
}

number_le() {
  finite_number "$1" && finite_number "$2" &&
    awk -v x="$1" -v y="$2" 'BEGIN { exit !(x<=y) }'
}

number_ge() {
  finite_number "$1" && finite_number "$2" &&
    awk -v x="$1" -v y="$2" 'BEGIN { exit !(x>=y) }'
}

numeric_equal() {
  finite_number "$1" && finite_number "$2" &&
    awk -v x="$1" -v y="$2" 'BEGIN { exit !(x==y) }'
}

publish_once() {
  local candidate="$1" destination="$2" mode="$3"
  if [[ ! -f "$candidate" || -L "$candidate" ||
        -e "$destination" || -L "$destination" ]]; then
    fail "unsafe publication: ${candidate} -> ${destination}"
    return 1
  fi
  if ! chmod "$mode" -- "$candidate"; then
    fail "could not freeze publication candidate: ${candidate}"
    return 1
  fi
  require_file "$candidate" "${mode#0}" || return 1
  if ! mv -T -n -- "$candidate" "$destination"; then
    fail "atomic publication move failed: ${destination}"
    return 1
  fi
  if [[ -e "$candidate" || ! -f "$destination" || -L "$destination" ]]; then
    fail "no-clobber publication failed: ${destination}"
    return 1
  fi
  require_file "$destination" "${mode#0}" || return 1
}

arm_root() { printf '%s/arms/%s' "$EVIDENCE" "$1"; }
arm_main_report() { printf '%s/main.report' "$(arm_root "$1")"; }
arm_replay_report() { printf '%s/replay.report' "$(arm_root "$1")"; }
arm_main_log() { printf '%s/main.log' "$(arm_root "$1")"; }
arm_replay_log() { printf '%s/replay.log' "$(arm_root "$1")"; }
arm_complete_receipt() { printf '%s/complete.status' "$(arm_root "$1")"; }
arm_projection_receipt() { printf '%s/projection.status' "$(arm_root "$1")"; }
invocation_launch_receipt() { printf '%s/%s.launch.status' "$(arm_root "$1")" "$2"; }
invocation_returned_receipt() { printf '%s/%s.returned.status' "$(arm_root "$1")" "$2"; }

assert_allowed_probe_path() {
  local path="$1"
  [[ "$path" == "${SERVING_ROOT}/"* || "$path" == "${ABLATION_ROOT}/"* ||
     "$path" == "${CANONICAL_ROOT}/"* ]] ||
    fail "probe path outside frozen authorities: ${path}"
  case "$path" in
    *"/data/raw/"*|*"/data/final/"*|*"/certified/"*|*"/final/"*|*"/policy/"*|*.pt|*.jkimyei)
      fail "protected or model path forbidden: ${path}" ;;
  esac
}

ensure_runtime_and_lock() {
  [[ "$(id -u)" == 0 ]] || fail "runner requires uid 0"
  if [[ ! -e "$RUNTIME" ]]; then
    install -d -m 0700 -- "$RUNTIME"
  fi
  require_private_dir "$RUNTIME"
  if [[ ! -e "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null ||
      fail "could not create execution lock"
    chmod 0600 -- "$LOCK"
  fi
  require_file "$LOCK" 600
  install -d -m 0700 -- "$PREP_DIR" "$EVIDENCE" "$SCRATCH"
  require_private_dir "$PREP_DIR"
  require_private_dir "$EVIDENCE"
  require_private_dir "$SCRATCH"
  local arm
  for arm in "${ARM_IDS[@]}"; do
    install -d -m 0700 -- "$(arm_root "$arm")"
    require_private_dir "$(arm_root "$arm")"
  done
}

open_execution_lock_exclusive() {
  exec 9<>"$LOCK"
  flock -n 9 || fail "another protocol process owns the execution lock"
}

open_execution_lock_shared() {
  exec 9<"$LOCK"
  flock -s -n 9 || fail "protocol execution is active"
}

require_lock_file() {
  local path="$1" mode="$2"
  require_file "$path" "$mode"
}

acquire_authority_locks() {
  require_lock_file "$SERVING_LOCK" 444
  require_lock_file "$ABLATION_LOCK" 600
  require_lock_file "$CANONICAL_LOCK" 600
  exec 3<"$SERVING_LOCK"
  exec 4<"$ABLATION_LOCK"
  exec 5<"$CANONICAL_LOCK"
  flock -s 3
  flock -s 4
  flock -s 5
}

preflight_static_authorities() {
  require_exact "$RUNNER" "$(sha256 "$RUNNER")" 555
  require_exact "$PREREG" "$PREREG_SHA" 444
  require_exact "$PHASE2A_SOURCE" "$PHASE2A_SOURCE_SHA" 444
  require_exact "$CANONICAL_PARSER" "$CANONICAL_PARSER_SHA" 444
  require_exact "$PHASE2A_BUILD_WRAPPER" "$PHASE2A_BUILD_WRAPPER_SHA" 555
  require_exact "$PHASE2A_RECEIPT" "$PHASE2A_RECEIPT_SHA" 444
  require_exact "$PHASE2A_BINARY" "$PHASE2A_BINARY_SHA" 555
  require_exact "$SERVING_RESULT" "$SERVING_RESULT_SHA" 444
  require_exact "$SERVING_TRAIN_CAPTURE" "$SERVING_TRAIN_CAPTURE_SHA" 444
  require_exact "$SERVING_VALIDATION_CAPTURE" "$SERVING_VALIDATION_CAPTURE_SHA" 444
  require_exact "$ABLATION_RESULT" "$ABLATION_RESULT_SHA" 444
  require_exact "$ABLATION_SELECTION" "$ABLATION_SELECTION_SHA" 444
  require_exact "$CANONICAL_RESULT" "$CANONICAL_RESULT_SHA" 444
  require_exact "$CANONICAL_IMPORT" "$CANONICAL_IMPORT_SHA" 444
  require_exact "$ENDPOINT_RECEIPT" "$ENDPOINT_RECEIPT_SHA" 444
  require_exact "$TIME_ONLY_RECEIPT" "$TIME_ONLY_RECEIPT_SHA" 444
  require_exact "$NO_TF_RECEIPT" "$NO_TF_RECEIPT_SHA" 444
  require_lock_file "$SERVING_LOCK" 444
  require_lock_file "$ABLATION_LOCK" 600
  require_lock_file "$CANONICAL_LOCK" 600
}

preflight_probe_inputs() {
  local index
  require_exact "$CANONICAL_TRAIN" "${ARM_TRAIN_SHA[0]}" 444
  require_exact "$CANONICAL_VALIDATION" "${ARM_VALIDATION_SHA[0]}" 444
  for index in "${!ARM_IDS[@]}"; do
    assert_allowed_probe_path "${ARM_TRAIN[$index]}"
    assert_allowed_probe_path "${ARM_VALIDATION[$index]}"
    require_exact "${ARM_TRAIN[$index]}" "${ARM_TRAIN_SHA[$index]}" 444
    require_exact "${ARM_VALIDATION[$index]}" "${ARM_VALIDATION_SHA[$index]}" 444
    [[ "${ARM_TRAIN[$index]}" != "${ARM_VALIDATION[$index]}" ]] ||
      fail "train and validation paths alias for ${ARM_IDS[$index]}"
  done
}

validate_authority_receipts() {
  local index id key
  local -a files=(
    "$PHASE2A_RECEIPT" "$SERVING_RESULT" "$ABLATION_RESULT"
    "$ABLATION_SELECTION" "$CANONICAL_RESULT" "$CANONICAL_IMPORT"
    "$ENDPOINT_RECEIPT" "$TIME_ONLY_RECEIPT" "$NO_TF_RECEIPT"
  )
  for key in "${files[@]}"; do load_kv_file "$key"; done

  expect "$PHASE2A_RECEIPT" schema_id synthetic_v2_frozen_encoder_channel_conditioned_affine_development_receipt_v1
  expect "$PHASE2A_RECEIPT" status complete
  expect "$PHASE2A_RECEIPT" diagnostic_authority development_only
  expect "$PHASE2A_RECEIPT" evaluator_source_sha256 "$PHASE2A_SOURCE_SHA"
  expect "$PHASE2A_RECEIPT" pooled_evaluator_source_sha256 "$CANONICAL_PARSER_SHA"
  expect "$PHASE2A_RECEIPT" build_target_sha256 "$PHASE2A_BUILD_WRAPPER_SHA"
  expect "$PHASE2A_RECEIPT" evaluator_binary_path "$PHASE2A_BINARY"
  expect "$PHASE2A_RECEIPT" evaluator_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$PHASE2A_RECEIPT" conditioned_head_count 9
  expect "$PHASE2A_RECEIPT" ridge_grid 1e-12,1e-10,1e-8,1e-6,1e-4,1e-2
  expect "$PHASE2A_RECEIPT" main_report_sha256 "$PHASE2A_REFERENCE_REPORT_SHA"
  expect "$PHASE2A_RECEIPT" replay_report_sha256 "$PHASE2A_REFERENCE_REPORT_SHA"
  expect "$PHASE2A_RECEIPT" main_replay_byte_identical true
  expect "$PHASE2A_RECEIPT" certified_input_access false
  expect "$PHASE2A_RECEIPT" final_holdout_access false
  expect "$PHASE2A_RECEIPT" policy_access false

  expect "$SERVING_RESULT" schema_id synthetic_v2_mtf_serving_pool_replay_development_v1.development.v1
  expect "$SERVING_RESULT" status complete
  expect "$SERVING_RESULT" scientific_scope development_only
  expect "$SERVING_RESULT" train_anchor_range '[0,2496)'
  expect "$SERVING_RESULT" validation_anchor_range '[2560,2816)'
  expect "$SERVING_RESULT" maximum_anchor_read 2815
  expect "$SERVING_RESULT" train_probe_rows 22464
  expect "$SERVING_RESULT" validation_probe_rows 2304
  expect "$SERVING_RESULT" graph_order_fingerprint 4133db527907a8e4
  expect "$SERVING_RESULT" coordinates_targets_identical true
  expect "$SERVING_RESULT" capture.train.report_path "$SERVING_TRAIN_CAPTURE"
  expect "$SERVING_RESULT" capture.train.report_sha256 "$SERVING_TRAIN_CAPTURE_SHA"
  expect "$SERVING_RESULT" capture.validation.report_path "$SERVING_VALIDATION_CAPTURE"
  expect "$SERVING_RESULT" capture.validation.report_sha256 "$SERVING_VALIDATION_CAPTURE_SHA"
  expect "$SERVING_RESULT" canonical_data_raw_access false
  expect "$SERVING_RESULT" certified_input_access false
  expect "$SERVING_RESULT" final_holdout_access false
  expect "$SERVING_RESULT" policy_config_parsed_as_inert_dependency true
  expect "$SERVING_RESULT" policy_model_constructed false
  expect "$SERVING_RESULT" policy_checkpoint_access false
  expect "$SERVING_RESULT" policy_execution false
  expect "$SERVING_RESULT" policy_metric_access false

  for index in 0 1 2 3; do
    id="${ARM_IDS[$index]}"
    expect "$SERVING_RESULT" "pool.${id}.train.probe_path" "${ARM_TRAIN[$index]}"
    expect "$SERVING_RESULT" "pool.${id}.train.probe_sha256" "${ARM_TRAIN_SHA[$index]}"
    expect "$SERVING_RESULT" "pool.${id}.train.coordinates_targets_sha256" "$TRAIN_PROJECTION_SHA"
    expect "$SERVING_RESULT" "pool.${id}.validation.probe_path" "${ARM_VALIDATION[$index]}"
    expect "$SERVING_RESULT" "pool.${id}.validation.probe_sha256" "${ARM_VALIDATION_SHA[$index]}"
    expect "$SERVING_RESULT" "pool.${id}.validation.coordinates_targets_sha256" "$VALIDATION_PROJECTION_SHA"
  done

  expect "$ABLATION_RESULT" schema_id synthetic_v2_representation_ablation_isolated_v2_retry3.development.v1
  expect "$ABLATION_RESULT" status complete
  expect "$ABLATION_SELECTION" schema_id synthetic_v2_representation_ablation_isolated_v2_retry3.selection.v1
  expect "$ABLATION_SELECTION" status complete
  expect "$ABLATION_SELECTION" arm_count 4
  expect "$ABLATION_SELECTION" selection_order validation_direction,validation_rank,validation_correlation,validation_rmse
  expect "$ABLATION_SELECTION" selection_tie_tolerance 1e-12
  expect "$ABLATION_SELECTION" selection_tie_preference canonical,endpoint_scale,time_only,no_tf_alignment
  expect "$ABLATION_SELECTION" maximum_anchor_read 2815
  expect "$ABLATION_SELECTION" certified_input_access false
  expect "$ABLATION_SELECTION" canonical_data_raw_access false
  expect "$ABLATION_SELECTION" final_holdout_access false
  expect "$ABLATION_SELECTION" policy_access false

  expect "$CANONICAL_RESULT" schema_id synthetic_v2_frozen_feature_capture_isolated_v2.development.v1
  expect "$CANONICAL_RESULT" status complete
  expect "$CANONICAL_IMPORT" schema_id synthetic_v2_representation_ablation_isolated_v2_retry3.retry2_canonical_import.v1
  expect "$CANONICAL_IMPORT" status complete
  expect "$CANONICAL_IMPORT" arm canonical
  expect "$CANONICAL_IMPORT" imported_main_report_sha256 e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e
  expect "$CANONICAL_IMPORT" imported_replay_report_sha256 e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e
  expect "$CANONICAL_IMPORT" byte_identical_copy_verified true

  local -a receipts=("$ENDPOINT_RECEIPT" "$TIME_ONLY_RECEIPT" "$NO_TF_RECEIPT")
  for index in 0 1 2; do
    id="${ARM_IDS[$((index+4))]}"
    key="${receipts[$index]}"
    expect "$key" schema_id synthetic_v2_representation_ablation_isolated_v2_retry3.capture.v1
    expect "$key" status complete
    expect "$key" arm "$id"
    expect "$key" train_probe_path "${ARM_TRAIN[$((index+4))]}"
    expect "$key" train_probe_sha256 "${ARM_TRAIN_SHA[$((index+4))]}"
    expect "$key" validation_probe_path "${ARM_VALIDATION[$((index+4))]}"
    expect "$key" validation_probe_sha256 "${ARM_VALIDATION_SHA[$((index+4))]}"
    expect "$key" train_anchor_range '[0,2496)'
    expect "$key" validation_anchor_range '[2560,2816)'
    expect "$key" train_probe_rows 22464
    expect "$key" validation_probe_rows 2304
    expect "$key" maximum_anchor_read 2815
    expect "$key" canonical_data_raw_access false
    expect "$key" certified_input_access false
    expect "$key" final_holdout_access false
    expect "$key" policy_access false
    expect "$ABLATION_SELECTION" "arm.${id}.train_probe_path" "${ARM_TRAIN[$((index+4))]}"
    expect "$ABLATION_SELECTION" "arm.${id}.train_probe_sha256" "${ARM_TRAIN_SHA[$((index+4))]}"
    expect "$ABLATION_SELECTION" "arm.${id}.validation_probe_path" "${ARM_VALIDATION[$((index+4))]}"
    expect "$ABLATION_SELECTION" "arm.${id}.validation_probe_sha256" "${ARM_VALIDATION_SHA[$((index+4))]}"
  done
  expect "$ABLATION_SELECTION" arm.canonical.train_probe_path "$CANONICAL_TRAIN"
  expect "$ABLATION_SELECTION" arm.canonical.train_probe_sha256 "${ARM_TRAIN_SHA[0]}"
  expect "$ABLATION_SELECTION" arm.canonical.validation_probe_path "$CANONICAL_VALIDATION"
  expect "$ABLATION_SELECTION" arm.canonical.validation_probe_sha256 "${ARM_VALIDATION_SHA[0]}"
}

preflight_science() {
  preflight_static_authorities
  validate_authority_receipts
  require_exact "$BIN" "$PHASE2A_BINARY_SHA" 555
  preflight_probe_inputs
}

preflight_pre_attempt() {
  preflight_static_authorities
  validate_authority_receipts
  require_exact "$BIN" "$PHASE2A_BINARY_SHA" 555
}

emit_prepare_receipt() {
  local candidate="${SCRATCH}/prepared.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.prepared.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "preparation_kind=sealed_binary_copy_only"
    echo "compile_executed=false"
    echo "probe_input_access=false"
    echo "source_binary_path=${PHASE2A_BINARY}"
    echo "source_binary_sha256=${PHASE2A_BINARY_SHA}"
    echo "copied_binary_path=${BIN}"
    echo "copied_binary_sha256=$(sha256 "$BIN")"
    echo "phase2a_source_sha256=${PHASE2A_SOURCE_SHA}"
    echo "canonical_parser_sha256=${CANONICAL_PARSER_SHA}"
    echo "build_wrapper_sha256=${PHASE2A_BUILD_WRAPPER_SHA}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
  } > "$candidate"
  publish_once "$candidate" "$PREP_RECEIPT" 0444
}

validate_prepare_receipt() {
  require_exact "$BIN" "$PHASE2A_BINARY_SHA" 555
  load_kv_file "$PREP_RECEIPT"
  expect "$PREP_RECEIPT" schema_id "${PROTOCOL_ID}.prepared.v1"
  expect "$PREP_RECEIPT" status complete
  expect "$PREP_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$PREP_RECEIPT" preparation_kind sealed_binary_copy_only
  expect "$PREP_RECEIPT" compile_executed false
  expect "$PREP_RECEIPT" probe_input_access false
  expect "$PREP_RECEIPT" source_binary_path "$PHASE2A_BINARY"
  expect "$PREP_RECEIPT" source_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$PREP_RECEIPT" copied_binary_path "$BIN"
  expect "$PREP_RECEIPT" copied_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$PREP_RECEIPT" phase2a_source_sha256 "$PHASE2A_SOURCE_SHA"
  expect "$PREP_RECEIPT" canonical_parser_sha256 "$CANONICAL_PARSER_SHA"
  expect "$PREP_RECEIPT" build_wrapper_sha256 "$PHASE2A_BUILD_WRAPPER_SHA"
  expect "$PREP_RECEIPT" preregistration_sha256 "$PREREG_SHA"
  expect "$PREP_RECEIPT" runner_sha256 "$(sha256 "$RUNNER")"
  [[ "$(wc -l < "$PREP_RECEIPT")" == 15 ]] ||
    fail "prepared receipt key count mismatch"
}

prepare() {
  local candidate
  ensure_runtime_and_lock
  open_execution_lock_exclusive
  acquire_authority_locks
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] ||
    fail "preparation forbidden after attempt or terminal publication"
  preflight_static_authorities
  validate_authority_receipts
  if [[ -e "$PREP_RECEIPT" || -e "$BIN" ]]; then
    [[ -e "$PREP_RECEIPT" && -e "$BIN" ]] ||
      fail "partial preparation state"
    validate_prepare_receipt
    echo "[clear-signal:sealed-raw96-affine] existing preparation verified"
    return
  fi
  candidate="${SCRATCH}/evaluator.binary.candidate"
  [[ ! -e "$candidate" ]] || fail "binary candidate already exists"
  cp --reflink=never -- "$PHASE2A_BINARY" "$candidate"
  chmod 0555 -- "$candidate"
  require_exact "$candidate" "$PHASE2A_BINARY_SHA" 555
  publish_once "$candidate" "$BIN" 0555
  emit_prepare_receipt
  validate_prepare_receipt
  echo "[clear-signal:sealed-raw96-affine] sealed evaluator bytes prepared"
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.status.candidate" index id
  {
    echo "schema_id=${PROTOCOL_ID}.attempt.v1"
    echo "status=committed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "diagnostic_phase=${DIAGNOSTIC_PHASE}"
    echo "diagnostic_authority=retrospective_development_only"
    echo "benchmark_acceptance_authority=false"
    echo "certified_authorization_eligible=false"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "prepared_receipt_path=${PREP_RECEIPT}"
    echo "prepared_receipt_sha256=$(sha256 "$PREP_RECEIPT")"
    echo "evaluator_binary_path=${BIN}"
    echo "evaluator_binary_sha256=$(sha256 "$BIN")"
    echo "phase2a_receipt_path=${PHASE2A_RECEIPT}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "serving_result_path=${SERVING_RESULT}"
    echo "serving_result_sha256=${SERVING_RESULT_SHA}"
    echo "ablation_result_path=${ABLATION_RESULT}"
    echo "ablation_result_sha256=${ABLATION_RESULT_SHA}"
    echo "ablation_selection_path=${ABLATION_SELECTION}"
    echo "ablation_selection_sha256=${ABLATION_SELECTION_SHA}"
    echo "canonical_result_path=${CANONICAL_RESULT}"
    echo "canonical_result_sha256=${CANONICAL_RESULT_SHA}"
    echo "canonical_import_path=${CANONICAL_IMPORT}"
    echo "canonical_import_sha256=${CANONICAL_IMPORT_SHA}"
    echo "unique_pair_count=${UNIQUE_PAIR_COUNT}"
    echo "logical_arm_count=${LOGICAL_ARM_COUNT}"
    echo "maximum_evaluator_invocations=${MAX_EVALUATOR_INVOCATIONS}"
    echo "evaluator_invocations_per_completed_arm=2"
    echo "ridge_candidate_count_per_invocation=${RIDGE_CANDIDATE_COUNT}"
    echo "conditioned_head_count=${HEAD_COUNT}"
    echo "maximum_candidate_fit_count=84"
    echo "maximum_head_solve_count=756"
    echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
    echo "execution_order=all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced,endpoint_scale,time_only,no_tf_alignment"
    echo "lane_order=main_then_replay"
    echo "stop_rule=first_recomputed_original_strong_gate_after_main_replay_parity"
    echo "ranking_rule=direction,rank,correlation,lower_rmse,fixed_order_tie"
    echo "selection_tie_tolerance=${TIE_TOLERANCE}"
    echo "ridge_grid=1e-12,1e-10,1e-8,1e-6,1e-4,1e-2"
    echo "projection_command=tail_-n_+2_--_PROBE_|_cut_-d,_-f2-10"
    echo "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
    echo "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
    echo "projection_inside_consumed_attempt=true"
    echo "projection_before_evaluator_call_1=true"
    echo "canonical_alias=ablation.canonical->all_tokens"
    echo "capture_execution=false"
    echo "encoder_execution=false"
    echo "checkpoint_read=false"
    echo "representation_model_execution=false"
    echo "mdn_model_execution=false"
    echo "optimizer_training=false"
    echo "refit=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "maximum_anchor_read=2815"
    echo "train_anchor_range=[0,2496)"
    echo "validation_anchor_range=[2560,2816)"
    for index in "${!ARM_IDS[@]}"; do
      id="${ARM_IDS[$index]}"
      echo "arm.${index}.id=${id}"
      echo "arm.${index}.family=${ARM_FAMILIES[$index]}"
      echo "arm.${index}.train_input=${ARM_TRAIN[$index]}"
      echo "arm.${index}.train_sha256=${ARM_TRAIN_SHA[$index]}"
      echo "arm.${index}.validation_input=${ARM_VALIDATION[$index]}"
      echo "arm.${index}.validation_sha256=${ARM_VALIDATION_SHA[$index]}"
      echo "arm.${index}.source_receipt=${ARM_SOURCE_RECEIPTS[$index]}"
      echo "arm.${index}.source_receipt_sha256=${ARM_SOURCE_RECEIPT_SHA[$index]}"
    done
  } > "$candidate"
  publish_once "$candidate" "$ATTEMPT" 0444
}

validate_attempt() {
  local index id
  load_kv_file "$ATTEMPT"
  expect "$ATTEMPT" schema_id "${PROTOCOL_ID}.attempt.v1"
  expect "$ATTEMPT" status committed
  expect "$ATTEMPT" protocol_id "$PROTOCOL_ID"
  expect "$ATTEMPT" diagnostic_phase "$DIAGNOSTIC_PHASE"
  expect "$ATTEMPT" diagnostic_authority retrospective_development_only
  expect "$ATTEMPT" benchmark_acceptance_authority false
  expect "$ATTEMPT" certified_authorization_eligible false
  expect "$ATTEMPT" preregistration_path "$PREREG"
  expect "$ATTEMPT" preregistration_sha256 "$PREREG_SHA"
  expect "$ATTEMPT" runner_path "$RUNNER"
  expect "$ATTEMPT" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$ATTEMPT" prepared_receipt_path "$PREP_RECEIPT"
  expect "$ATTEMPT" prepared_receipt_sha256 "$(sha256 "$PREP_RECEIPT")"
  expect "$ATTEMPT" evaluator_binary_path "$BIN"
  expect "$ATTEMPT" evaluator_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$ATTEMPT" phase2a_receipt_path "$PHASE2A_RECEIPT"
  expect "$ATTEMPT" phase2a_receipt_sha256 "$PHASE2A_RECEIPT_SHA"
  expect "$ATTEMPT" serving_result_path "$SERVING_RESULT"
  expect "$ATTEMPT" serving_result_sha256 "$SERVING_RESULT_SHA"
  expect "$ATTEMPT" ablation_result_path "$ABLATION_RESULT"
  expect "$ATTEMPT" ablation_result_sha256 "$ABLATION_RESULT_SHA"
  expect "$ATTEMPT" ablation_selection_path "$ABLATION_SELECTION"
  expect "$ATTEMPT" ablation_selection_sha256 "$ABLATION_SELECTION_SHA"
  expect "$ATTEMPT" canonical_result_path "$CANONICAL_RESULT"
  expect "$ATTEMPT" canonical_result_sha256 "$CANONICAL_RESULT_SHA"
  expect "$ATTEMPT" canonical_import_path "$CANONICAL_IMPORT"
  expect "$ATTEMPT" canonical_import_sha256 "$CANONICAL_IMPORT_SHA"
  expect "$ATTEMPT" unique_pair_count "$UNIQUE_PAIR_COUNT"
  expect "$ATTEMPT" logical_arm_count "$LOGICAL_ARM_COUNT"
  expect "$ATTEMPT" maximum_evaluator_invocations "$MAX_EVALUATOR_INVOCATIONS"
  expect "$ATTEMPT" evaluator_invocations_per_completed_arm 2
  expect "$ATTEMPT" ridge_candidate_count_per_invocation "$RIDGE_CANDIDATE_COUNT"
  expect "$ATTEMPT" conditioned_head_count "$HEAD_COUNT"
  expect "$ATTEMPT" maximum_candidate_fit_count 84
  expect "$ATTEMPT" maximum_head_solve_count 756
  expect "$ATTEMPT" worker_timeout_seconds "$WORKER_TIMEOUT_SECONDS"
  expect "$ATTEMPT" term_grace_seconds "$TERM_GRACE_SECONDS"
  expect "$ATTEMPT" execution_order all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced,endpoint_scale,time_only,no_tf_alignment
  expect "$ATTEMPT" lane_order main_then_replay
  expect "$ATTEMPT" stop_rule first_recomputed_original_strong_gate_after_main_replay_parity
  expect "$ATTEMPT" ranking_rule direction,rank,correlation,lower_rmse,fixed_order_tie
  expect "$ATTEMPT" selection_tie_tolerance "$TIE_TOLERANCE"
  expect "$ATTEMPT" ridge_grid 1e-12,1e-10,1e-8,1e-6,1e-4,1e-2
  expect "$ATTEMPT" projection_command 'tail_-n_+2_--_PROBE_|_cut_-d,_-f2-10'
  expect "$ATTEMPT" train_projection_sha256 "$TRAIN_PROJECTION_SHA"
  expect "$ATTEMPT" validation_projection_sha256 "$VALIDATION_PROJECTION_SHA"
  expect "$ATTEMPT" projection_inside_consumed_attempt true
  expect "$ATTEMPT" projection_before_evaluator_call_1 true
  expect "$ATTEMPT" canonical_alias 'ablation.canonical->all_tokens'
  expect "$ATTEMPT" capture_execution false
  expect "$ATTEMPT" encoder_execution false
  expect "$ATTEMPT" checkpoint_read false
  expect "$ATTEMPT" representation_model_execution false
  expect "$ATTEMPT" mdn_model_execution false
  expect "$ATTEMPT" optimizer_training false
  expect "$ATTEMPT" refit false
  expect "$ATTEMPT" certified_input_access false
  expect "$ATTEMPT" final_holdout_access false
  expect "$ATTEMPT" policy_access false
  expect "$ATTEMPT" maximum_anchor_read 2815
  expect "$ATTEMPT" train_anchor_range '[0,2496)'
  expect "$ATTEMPT" validation_anchor_range '[2560,2816)'
  for index in "${!ARM_IDS[@]}"; do
    id="${ARM_IDS[$index]}"
    expect "$ATTEMPT" "arm.${index}.id" "$id"
    expect "$ATTEMPT" "arm.${index}.family" "${ARM_FAMILIES[$index]}"
    expect "$ATTEMPT" "arm.${index}.train_input" "${ARM_TRAIN[$index]}"
    expect "$ATTEMPT" "arm.${index}.train_sha256" "${ARM_TRAIN_SHA[$index]}"
    expect "$ATTEMPT" "arm.${index}.validation_input" "${ARM_VALIDATION[$index]}"
    expect "$ATTEMPT" "arm.${index}.validation_sha256" "${ARM_VALIDATION_SHA[$index]}"
    expect "$ATTEMPT" "arm.${index}.source_receipt" "${ARM_SOURCE_RECEIPTS[$index]}"
    expect "$ATTEMPT" "arm.${index}.source_receipt_sha256" "${ARM_SOURCE_RECEIPT_SHA[$index]}"
  done
  [[ "$(wc -l < "$ATTEMPT")" == 118 ]] ||
    fail "attempt receipt key count mismatch"
}

validate_probe_shape() {
  local path="$1" split="$2" begin rows
  case "$split" in
    train) begin=0; rows=22464 ;;
    validation) begin=2560; rows=2304 ;;
    *) fail "invalid projection split: ${split}" ;;
  esac
  awk -F, -v begin="$begin" -v rows="$rows" '
    function numeric(x) {
      return x ~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/
    }
    function integer(x) { return x ~ /^[-+]?[0-9]+$/ }
    BEGIN {
      header="record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,base_node_id,quote_node_id,channel_index,target_edge_close_return,feature_count,feature_values"
      edge_id[0]="SYN2ALPHASYN2USD"; edge_id[1]="SYN2BETASYN2USD"; edge_id[2]="SYN2GAMMASYN2USD"
      base_id[0]="SYN2ALPHA"; base_id[1]="SYN2BETA"; base_id[2]="SYN2GAMMA"
    }
    NR==1 { if ($0 != header) exit 10; next }
    {
      if (NF != 12 || $1 != "kikijyeba.synthetic.representation_edge_feature_probe.v1") exit 11
      i=NR-2
      anchor=begin+int(i/9)
      edge=int((i%9)/3)
      channel=i%3
      if (!integer($2) || !integer($3) || !integer($4) || !integer($5) ||
          !integer($9) || !integer($11) || !numeric($10)) exit 12
      if (($3+0)!=anchor || ($4+0)!=(anchor-begin) || ($5+0)!=edge ||
          ($9+0)!=channel || ($11+0)!=96) exit 13
      if ($6 != edge_id[edge] || $7 != base_id[edge] || $8 != "SYN2USD") exit 14
      n=split($12, value, ";")
      if (n != 96) exit 15
      for (j=1; j<=n; ++j) if (!numeric(value[j])) exit 16
    }
    END { if (NR != rows+1) exit 17 }
  ' "$path" || fail "raw96 probe schema/cube validation failed: ${path}"
}

projection_digest() {
  tail -n +2 -- "$1" | cut -d, -f2-10 | sha256sum | cut -d' ' -f1
}

emit_projection_receipt() {
  local index="$1" id="${ARM_IDS[$1]}" train_digest="$2" validation_digest="$3"
  local destination candidate
  destination="$(arm_projection_receipt "$id")"
  candidate="${SCRATCH}/${id}.projection.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.arm_projection.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "arm_index=${index}"
    echo "arm_id=${id}"
    echo "train_input=${ARM_TRAIN[$index]}"
    echo "train_input_sha256=${ARM_TRAIN_SHA[$index]}"
    echo "train_row_count=22464"
    echo "train_projection_sha256=${train_digest}"
    echo "validation_input=${ARM_VALIDATION[$index]}"
    echo "validation_input_sha256=${ARM_VALIDATION_SHA[$index]}"
    echo "validation_row_count=2304"
    echo "validation_projection_sha256=${validation_digest}"
    echo "projection_payload_persisted=false"
    echo "evaluator_invocations_before_projection=0"
  } > "$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_projection_receipt() {
  local index="$1" id="${ARM_IDS[$1]}" path
  path="$(arm_projection_receipt "$id")"
  load_kv_file "$path"
  expect "$path" schema_id "${PROTOCOL_ID}.arm_projection.v1"
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" arm_index "$index"
  expect "$path" arm_id "$id"
  expect "$path" train_input "${ARM_TRAIN[$index]}"
  expect "$path" train_input_sha256 "${ARM_TRAIN_SHA[$index]}"
  expect "$path" train_row_count 22464
  expect "$path" train_projection_sha256 "$TRAIN_PROJECTION_SHA"
  expect "$path" validation_input "${ARM_VALIDATION[$index]}"
  expect "$path" validation_input_sha256 "${ARM_VALIDATION_SHA[$index]}"
  expect "$path" validation_row_count 2304
  expect "$path" validation_projection_sha256 "$VALIDATION_PROJECTION_SHA"
  expect "$path" projection_payload_persisted false
  expect "$path" evaluator_invocations_before_projection 0
  [[ "$(wc -l < "$path")" == 16 ]] ||
    fail "projection receipt key count mismatch: ${id}"
}

emit_alias_input_receipt() {
  local candidate="${SCRATCH}/canonical_alias.input.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.canonical_alias_input.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "executed_arm=all_tokens"
    echo "logical_alias=ablation.canonical"
    echo "executed_train_input=${ARM_TRAIN[0]}"
    echo "alias_train_input=${CANONICAL_TRAIN}"
    echo "train_sha256=${ARM_TRAIN_SHA[0]}"
    echo "train_byte_identical=true"
    echo "executed_validation_input=${ARM_VALIDATION[0]}"
    echo "alias_validation_input=${CANONICAL_VALIDATION}"
    echo "validation_sha256=${ARM_VALIDATION_SHA[0]}"
    echo "validation_byte_identical=true"
    echo "redundant_evaluator_execution=false"
  } > "$candidate"
  publish_once "$candidate" "$ALIAS_INPUT_RECEIPT" 0444
}

validate_alias_input_receipt() {
  load_kv_file "$ALIAS_INPUT_RECEIPT"
  expect "$ALIAS_INPUT_RECEIPT" schema_id "${PROTOCOL_ID}.canonical_alias_input.v1"
  expect "$ALIAS_INPUT_RECEIPT" status complete
  expect "$ALIAS_INPUT_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$ALIAS_INPUT_RECEIPT" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$ALIAS_INPUT_RECEIPT" executed_arm all_tokens
  expect "$ALIAS_INPUT_RECEIPT" logical_alias ablation.canonical
  expect "$ALIAS_INPUT_RECEIPT" executed_train_input "${ARM_TRAIN[0]}"
  expect "$ALIAS_INPUT_RECEIPT" alias_train_input "$CANONICAL_TRAIN"
  expect "$ALIAS_INPUT_RECEIPT" train_sha256 "${ARM_TRAIN_SHA[0]}"
  expect "$ALIAS_INPUT_RECEIPT" train_byte_identical true
  expect "$ALIAS_INPUT_RECEIPT" executed_validation_input "${ARM_VALIDATION[0]}"
  expect "$ALIAS_INPUT_RECEIPT" alias_validation_input "$CANONICAL_VALIDATION"
  expect "$ALIAS_INPUT_RECEIPT" validation_sha256 "${ARM_VALIDATION_SHA[0]}"
  expect "$ALIAS_INPUT_RECEIPT" validation_byte_identical true
  expect "$ALIAS_INPUT_RECEIPT" redundant_evaluator_execution false
  [[ "$(wc -l < "$ALIAS_INPUT_RECEIPT")" == 15 ]] ||
    fail "canonical alias input receipt key count mismatch"
}

emit_projection_complete() {
  local candidate="${SCRATCH}/projection.complete.status.candidate" index id
  {
    echo "schema_id=${PROTOCOL_ID}.projection_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "unique_pair_count=${UNIQUE_PAIR_COUNT}"
    echo "split_projection_check_count=14"
    echo "all_coordinate_target_projections_match=true"
    echo "canonical_alias_input_receipt_sha256=$(sha256 "$ALIAS_INPUT_RECEIPT")"
    echo "evaluator_invocations_before_stage=0"
    for index in "${!ARM_IDS[@]}"; do
      id="${ARM_IDS[$index]}"
      echo "arm.${index}.id=${id}"
      echo "arm.${index}.projection_receipt_sha256=$(sha256 "$(arm_projection_receipt "$id")")"
    done
  } > "$candidate"
  publish_once "$candidate" "$PROJECTION_COMPLETE" 0444
}

validate_projection_complete() {
  local index id
  load_kv_file "$PROJECTION_COMPLETE"
  expect "$PROJECTION_COMPLETE" schema_id "${PROTOCOL_ID}.projection_complete.v1"
  expect "$PROJECTION_COMPLETE" status complete
  expect "$PROJECTION_COMPLETE" protocol_id "$PROTOCOL_ID"
  expect "$PROJECTION_COMPLETE" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$PROJECTION_COMPLETE" unique_pair_count "$UNIQUE_PAIR_COUNT"
  expect "$PROJECTION_COMPLETE" split_projection_check_count 14
  expect "$PROJECTION_COMPLETE" all_coordinate_target_projections_match true
  expect "$PROJECTION_COMPLETE" canonical_alias_input_receipt_sha256 "$(sha256 "$ALIAS_INPUT_RECEIPT")"
  expect "$PROJECTION_COMPLETE" evaluator_invocations_before_stage 0
  validate_alias_input_receipt
  for index in "${!ARM_IDS[@]}"; do
    id="${ARM_IDS[$index]}"
    validate_projection_receipt "$index"
    expect "$PROJECTION_COMPLETE" "arm.${index}.id" "$id"
    expect "$PROJECTION_COMPLETE" "arm.${index}.projection_receipt_sha256" \
      "$(sha256 "$(arm_projection_receipt "$id")")"
  done
  [[ "$(wc -l < "$PROJECTION_COMPLETE")" == 23 ]] ||
    fail "projection-complete receipt key count mismatch"
}

run_projection_stage() {
  local index train_digest validation_digest
  [[ ! -e "$PROJECTION_COMPLETE" && ! -e "$ALIAS_INPUT_RECEIPT" ]] ||
    fail "projection stage is not pristine"
  for index in "${!ARM_IDS[@]}"; do
    validate_probe_shape "${ARM_TRAIN[$index]}" train
    train_digest="$(projection_digest "${ARM_TRAIN[$index]}")"
    [[ "$train_digest" == "$TRAIN_PROJECTION_SHA" ]] ||
      fail "train coordinate-target projection mismatch: ${ARM_IDS[$index]}"
    validate_probe_shape "${ARM_VALIDATION[$index]}" validation
    validation_digest="$(projection_digest "${ARM_VALIDATION[$index]}")"
    [[ "$validation_digest" == "$VALIDATION_PROJECTION_SHA" ]] ||
      fail "validation coordinate-target projection mismatch: ${ARM_IDS[$index]}"
    emit_projection_receipt "$index" "$train_digest" "$validation_digest"
    validate_projection_receipt "$index"
  done
  cmp -s -- "${ARM_TRAIN[0]}" "$CANONICAL_TRAIN" ||
    fail "all_tokens and canonical train probes are not byte-identical"
  cmp -s -- "${ARM_VALIDATION[0]}" "$CANONICAL_VALIDATION" ||
    fail "all_tokens and canonical validation probes are not byte-identical"
  emit_alias_input_receipt
  validate_alias_input_receipt
  emit_projection_complete
  validate_projection_complete
}

validate_metric_group() {
  local file="$1" prefix="$2" expected_count="$3" key value
  expect "$file" "${prefix}.count" "$expected_count"
  expect "$file" "${prefix}.pairwise_rank_count" "$expected_count"
  for key in mae rmse target_rms prediction_rms rmse_target_rms_ratio; do
    value="$(kv "$file" "${prefix}.${key}")"
    finite_number "$value" && number_ge "$value" 0 ||
      fail "invalid nonnegative metric ${prefix}.${key}"
  done
  number_ge "$(kv "$file" "${prefix}.target_rms")" 0.000000000000000001 ||
    fail "target RMS is not positive: ${prefix}"
  for key in directional_accuracy pairwise_rank_accuracy best_asset_agreement; do
    value="$(kv "$file" "${prefix}.${key}")"
    finite_number "$value" && number_ge "$value" 0 && number_le "$value" 1 ||
      fail "metric outside [0,1]: ${prefix}.${key}"
  done
  value="$(kv "$file" "${prefix}.correlation")"
  finite_number "$value" && number_ge "$value" -1 && number_le "$value" 1 ||
    fail "correlation outside [-1,1]: ${prefix}"
}

candidate_better() {
  local file="$1" candidate="$2" incumbent="$3"
  awk -v t="$TIE_TOLERANCE" \
      -v cd="$(kv "$file" "candidate.${candidate}.validation.directional_accuracy")" \
      -v id="$(kv "$file" "candidate.${incumbent}.validation.directional_accuracy")" \
      -v cr="$(kv "$file" "candidate.${candidate}.validation.pairwise_rank_accuracy")" \
      -v ir="$(kv "$file" "candidate.${incumbent}.validation.pairwise_rank_accuracy")" \
      -v cc="$(kv "$file" "candidate.${candidate}.validation.correlation")" \
      -v ic="$(kv "$file" "candidate.${incumbent}.validation.correlation")" \
      -v ce="$(kv "$file" "candidate.${candidate}.validation.rmse")" \
      -v ie="$(kv "$file" "candidate.${incumbent}.validation.rmse")" \
      -v ca="$(kv "$file" "candidate.${candidate}.ridge")" \
      -v ia="$(kv "$file" "candidate.${incumbent}.ridge")" '
    BEGIN {
      if (cd > id+t) exit 0; if (id > cd+t) exit 1
      if (cr > ir+t) exit 0; if (ir > cr+t) exit 1
      if (cc > ic+t) exit 0; if (ic > cc+t) exit 1
      if (ce+t < ie) exit 0; if (ie+t < ce) exit 1
      exit !(ca < ia)
    }'
}

validate_report() {
  local file="$1" index key prefix best selected strong=false partial=false
  local -a ridges=(1e-12 1e-10 1e-8 1e-6 1e-4 1e-2)
  require_file "$file" 444
  [[ "$(wc -l < "$file")" == "$COMPLETE_REPORT_KEY_COUNT" ]] ||
    fail "Phase2A report line count is not 234: ${file}"
  [[ "$(sorted_key_sha256 "$file")" == "$COMPLETE_REPORT_SORTED_KEY_SHA" ]] ||
    fail "Phase2A report key inventory mismatch: ${file}"
  load_kv_file "$file"
  expect "$file" schema_id synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1
  expect "$file" status complete
  expect "$file" benchmark_id synthetic_continuous_graph_v2
  expect "$file" diagnostic_phase 2A
  expect "$file" diagnostic_authority development_only
  expect "$file" benchmark_acceptance_authority false
  expect "$file" probe_kind representation
  expect "$file" probe_record_schema kikijyeba.synthetic.representation_edge_feature_probe.v1
  expect "$file" train_probe_rows 22464
  expect "$file" validation_probe_rows 2304
  expect "$file" certified_probe_rows 0
  expect "$file" probe_rows_total 24768
  expect "$file" probe_ranges_disjoint true
  expect "$file" fit_anchor_range '[0,2496)'
  expect "$file" validation_anchor_range '[2560,2816)'
  expect "$file" certified_anchor_range not_opened
  expect "$file" purge_anchors_used false
  expect "$file" maximum_anchor_read 2815
  expect "$file" final_holdout_begin 3328
  expect "$file" final_holdout_access false
  expect "$file" policy_access false
  expect "$file" refit_after_selection false
  expect "$file" certified_candidates_scored 0
  expect "$file" feature_layout base_32,quote_32,base_minus_quote_32
  expect "$file" probe_feature_width 96
  expect "$file" affine_feature_width 96
  expect "$file" edge_identity_feature_width_excluded 0
  expect "$file" fit_structure one_weight_row_and_bias_per_edge_and_channel
  expect "$file" conditioned_head_count 9
  expect "$file" standardization_scope train_core_all_edges_all_channels
  expect "$file" solver float64_centered_cholesky_ridge
  expect "$file" ridge_scaling gram_diagonal_plus_edge_channel_sample_count_times_alpha
  expect "$file" ridge_grid 1e-12,1e-10,1e-8,1e-6,1e-4,1e-2
  expect "$file" selection_scope one_global_candidate_for_all_edge_channel_heads
  expect "$file" selection_order validation_direction,validation_rank,validation_correlation,validation_rmse,smallest_alpha
  numeric_equal "$(kv "$file" selection_tie_tolerance)" "$TIE_TOLERANCE" ||
    fail "selection tie tolerance mismatch"
  expect "$file" numerically_valid_candidate_count 6
  numeric_equal "$(kv "$file" context_identity_max_abs_delta)" 0 ||
    fail "context identity delta is not zero"

  best=0
  for index in 0 1 2 3 4 5; do
    prefix="candidate.${index}"
    numeric_equal "$(kv "$file" "${prefix}.ridge")" "${ridges[$index]}" ||
      fail "ridge grid mismatch at candidate ${index}"
    expect "$file" "${prefix}.numerically_valid" true
    expect "$file" "${prefix}.rejection_reason" ""
    finite_number "$(kv "$file" "${prefix}.maximum_normalized_residual")" &&
      number_ge "$(kv "$file" "${prefix}.maximum_normalized_residual")" 0 &&
      number_le "$(kv "$file" "${prefix}.maximum_normalized_residual")" 1e-7 ||
      fail "candidate residual invalid at ${index}"
    finite_number "$(kv "$file" "${prefix}.coefficient_l2_norm")" &&
      number_ge "$(kv "$file" "${prefix}.coefficient_l2_norm")" 0 ||
      fail "candidate coefficient norm invalid at ${index}"
    validate_metric_group "$file" "${prefix}.validation" 2304
    if (( index > 0 )) && candidate_better "$file" "$index" "$best"; then
      best="$index"
    fi
  done

  selected="$(kv "$file" selected_candidate_index)"
  [[ "$selected" == "$best" ]] || fail "selected candidate does not match frozen comparator"
  expect "$file" selected_ridge "$(kv "$file" "candidate.${selected}.ridge")"
  expect "$file" selected_maximum_normalized_residual \
    "$(kv "$file" "candidate.${selected}.maximum_normalized_residual")"
  expect "$file" selected_coefficient_l2_norm \
    "$(kv "$file" "candidate.${selected}.coefficient_l2_norm")"
  validate_metric_group "$file" selected.train 22464
  validate_metric_group "$file" selected.validation 2304
  for key in count pairwise_rank_count mae rmse target_rms prediction_rms \
      rmse_target_rms_ratio directional_accuracy pairwise_rank_accuracy \
      best_asset_agreement correlation; do
    expect "$file" "selected.validation.${key}" \
      "$(kv "$file" "candidate.${selected}.validation.${key}")"
  done
  for index in 0 1 2; do
    validate_metric_group "$file" "selected.train.channel_${index}" 7488
    validate_metric_group "$file" "selected.validation.channel_${index}" 768
  done

  if number_ge "$(kv "$file" selected.validation.directional_accuracy)" "$STRONG_DIRECTION_MIN" &&
     number_ge "$(kv "$file" selected.validation.pairwise_rank_accuracy)" "$STRONG_RANK_MIN" &&
     number_ge "$(kv "$file" selected.validation.correlation)" "$STRONG_CORRELATION_MIN" &&
     number_le "$(kv "$file" selected.validation.rmse_target_rms_ratio)" "$STRONG_RMSE_RATIO_MAX"; then
    strong=true
  fi
  if number_ge "$(kv "$file" selected.validation.directional_accuracy)" "$PARTIAL_DIRECTION_MIN" &&
     number_ge "$(kv "$file" selected.validation.pairwise_rank_accuracy)" "$PARTIAL_RANK_MIN"; then
    partial=true
  fi
  expect "$file" validation_strong_gate_pass "$strong"
  expect "$file" certified_strong_gate_pass not_evaluated
  expect "$file" validation_partial_gate_pass "$partial"
  expect "$file" certified_partial_gate_pass not_evaluated
  if [[ "$strong" == true ]]; then
    expect "$file" rung_b_authorized false
    expect "$file" classification edge_channel_affine_sufficiency_established
  else
    expect "$file" rung_b_authorized true
    expect "$file" classification edge_channel_affine_sufficiency_not_established
  fi
  expect "$file" preregistered_strong_gate \
    'direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25'
  expect "$file" preregistered_partial_gate 'direction>=0.80,rank>=0.78'
}

emit_invocation_launch() {
  local index="$1" lane="$2" sequence="$3" id="${ARM_IDS[$1]}" path candidate
  path="$(invocation_launch_receipt "$id" "$lane")"
  candidate="${SCRATCH}/${id}.${lane}.launch.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.evaluator_launch.v1"
    echo "status=committed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "arm_index=${index}"
    echo "arm_id=${id}"
    echo "lane=${lane}"
    echo "invocation_sequence=${sequence}"
    echo "evaluator_binary_sha256=$(sha256 "$BIN")"
    echo "train_input=${ARM_TRAIN[$index]}"
    echo "train_input_sha256=${ARM_TRAIN_SHA[$index]}"
    echo "validation_input=${ARM_VALIDATION[$index]}"
    echo "validation_input_sha256=${ARM_VALIDATION_SHA[$index]}"
    echo "output_path=${SCRATCH}/${id}.${lane}.report.candidate"
    echo "launch_receipt_is_progress_attestation=false"
  } > "$candidate"
  publish_once "$candidate" "$path" 0444
}

emit_invocation_returned() {
  local index="$1" lane="$2" sequence="$3" exit_code="$4"
  local id="${ARM_IDS[$1]}" path candidate
  path="$(invocation_returned_receipt "$id" "$lane")"
  candidate="${SCRATCH}/${id}.${lane}.returned.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.evaluator_returned.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "launch_receipt_sha256=$(sha256 "$(invocation_launch_receipt "$id" "$lane")")"
    echo "arm_index=${index}"
    echo "arm_id=${id}"
    echo "lane=${lane}"
    echo "invocation_sequence=${sequence}"
    echo "evaluator_exit_code=${exit_code}"
    echo "report_candidate_present=$(if [[ -f "${SCRATCH}/${id}.${lane}.report.candidate" ]]; then echo true; else echo false; fi)"
  } > "$candidate"
  publish_once "$candidate" "$path" 0444
}

validate_invocation_launch_receipt() {
  local index="$1" lane="$2" sequence="$3" id="${ARM_IDS[$1]}"
  local launch output
  launch="$(invocation_launch_receipt "$id" "$lane")"
  output="${SCRATCH}/${id}.${lane}.report.candidate"
  load_kv_file "$launch"
  expect "$launch" schema_id "${PROTOCOL_ID}.evaluator_launch.v1"
  expect "$launch" status committed
  expect "$launch" protocol_id "$PROTOCOL_ID"
  expect "$launch" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$launch" projection_complete_sha256 "$(sha256 "$PROJECTION_COMPLETE")"
  expect "$launch" arm_index "$index"
  expect "$launch" arm_id "$id"
  expect "$launch" lane "$lane"
  expect "$launch" invocation_sequence "$sequence"
  expect "$launch" evaluator_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$launch" train_input "${ARM_TRAIN[$index]}"
  expect "$launch" train_input_sha256 "${ARM_TRAIN_SHA[$index]}"
  expect "$launch" validation_input "${ARM_VALIDATION[$index]}"
  expect "$launch" validation_input_sha256 "${ARM_VALIDATION_SHA[$index]}"
  expect "$launch" output_path "$output"
  expect "$launch" launch_receipt_is_progress_attestation false
  [[ "$(wc -l < "$launch")" == 16 ]] || fail "evaluator launch receipt key count mismatch"
}

validate_invocation_returned_evidence() {
  local index="$1" lane="$2" sequence="$3" id="${ARM_IDS[$1]}"
  local launch returned exit_code candidate_present
  launch="$(invocation_launch_receipt "$id" "$lane")"
  returned="$(invocation_returned_receipt "$id" "$lane")"
  validate_invocation_launch_receipt "$index" "$lane" "$sequence"
  load_kv_file "$returned"
  expect "$returned" schema_id "${PROTOCOL_ID}.evaluator_returned.v1"
  expect "$returned" status complete
  expect "$returned" protocol_id "$PROTOCOL_ID"
  expect "$returned" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$returned" launch_receipt_sha256 "$(sha256 "$launch")"
  expect "$returned" arm_index "$index"
  expect "$returned" arm_id "$id"
  expect "$returned" lane "$lane"
  expect "$returned" invocation_sequence "$sequence"
  exit_code="$(kv "$returned" evaluator_exit_code)"
  [[ "$exit_code" =~ ^[0-9]+$ ]] || fail "invalid evaluator exit code evidence"
  candidate_present="$(kv "$returned" report_candidate_present)"
  [[ "$candidate_present" == true || "$candidate_present" == false ]] ||
    fail "invalid report-candidate presence evidence"
  if [[ "$exit_code" == 0 ]]; then
    [[ "$candidate_present" == true ]] ||
      fail "zero evaluator exit did not publish an output candidate"
  fi
  [[ "$(wc -l < "$returned")" == 11 ]] || fail "evaluator return receipt key count mismatch"
}

validate_invocation_lifecycle() {
  local index="$1" lane="$2" sequence="$3" id="${ARM_IDS[$1]}" returned
  returned="$(invocation_returned_receipt "$id" "$lane")"
  validate_invocation_launch_receipt "$index" "$lane" "$sequence"
  validate_invocation_returned_evidence "$index" "$lane" "$sequence"
  expect "$returned" evaluator_exit_code 0
  expect "$returned" report_candidate_present true
}

run_evaluator_lane() {
  local index="$1" lane="$2" sequence="$3" id="${ARM_IDS[$1]}"
  local report log report_candidate log_candidate exit_code=0
  if [[ "$lane" == main ]]; then
    report="$(arm_main_report "$id")"
    log="$(arm_main_log "$id")"
  else
    report="$(arm_replay_report "$id")"
    log="$(arm_replay_log "$id")"
  fi
  report_candidate="${SCRATCH}/${id}.${lane}.report.candidate"
  log_candidate="${SCRATCH}/${id}.${lane}.log.candidate"
  [[ ! -e "$report" && ! -e "$log" && ! -e "$report_candidate" && ! -e "$log_candidate" &&
     ! -e "$(invocation_launch_receipt "$id" "$lane")" &&
     ! -e "$(invocation_returned_receipt "$id" "$lane")" ]] ||
    fail "evaluator lane is not pristine: ${id}/${lane}"
  emit_invocation_launch "$index" "$lane" "$sequence"
  set +e
  "$BIN" --probe-kind representation --development-only \
    --train-input "${ARM_TRAIN[$index]}" \
    --validation-input "${ARM_VALIDATION[$index]}" \
    --output "$report_candidate" > "$log_candidate" 2>&1
  exit_code=$?
  set -e
  emit_invocation_returned "$index" "$lane" "$sequence" "$exit_code"
  (( exit_code == 0 )) || return "$exit_code"
  require_file "$report_candidate" 600
  require_file "$log_candidate" 600
  chmod 0444 -- "$report_candidate" "$log_candidate"
  validate_report "$report_candidate"
  publish_once "$log_candidate" "$log" 0444
  publish_once "$report_candidate" "$report" 0444
  validate_report "$report"
  validate_invocation_lifecycle "$index" "$lane" "$sequence"
}

emit_arm_complete() {
  local index="$1" id="${ARM_IDS[$1]}" main replay candidate destination c
  main="$(arm_main_report "$id")"
  replay="$(arm_replay_report "$id")"
  destination="$(arm_complete_receipt "$id")"
  candidate="${SCRATCH}/${id}.complete.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.arm_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "arm_index=${index}"
    echo "arm_id=${id}"
    echo "arm_family=${ARM_FAMILIES[$index]}"
    echo "train_input=${ARM_TRAIN[$index]}"
    echo "train_input_sha256=${ARM_TRAIN_SHA[$index]}"
    echo "validation_input=${ARM_VALIDATION[$index]}"
    echo "validation_input_sha256=${ARM_VALIDATION_SHA[$index]}"
    echo "main_report_path=${main}"
    echo "main_report_sha256=$(sha256 "$main")"
    echo "replay_report_path=${replay}"
    echo "replay_report_sha256=$(sha256 "$replay")"
    echo "main_replay_byte_identical=true"
    echo "evaluator_invocations_completed=2"
    echo "analytic_candidate_fit_count=12"
    echo "edge_channel_head_solve_count=108"
    echo "selected_candidate_index=$(kv "$main" selected_candidate_index)"
    echo "selected_ridge=$(kv "$main" selected_ridge)"
    echo "selected_validation_direction=$(kv "$main" selected.validation.directional_accuracy)"
    echo "selected_validation_rank=$(kv "$main" selected.validation.pairwise_rank_accuracy)"
    echo "selected_validation_correlation=$(kv "$main" selected.validation.correlation)"
    echo "selected_validation_rmse=$(kv "$main" selected.validation.rmse)"
    echo "selected_validation_rmse_target_rms_ratio=$(kv "$main" selected.validation.rmse_target_rms_ratio)"
    echo "original_strong_gate_pass=$(kv "$main" validation_strong_gate_pass)"
    echo "original_partial_gate_pass=$(kv "$main" validation_partial_gate_pass)"
    for c in 0 1 2 3 4 5; do
      echo "candidate.${c}.ridge=$(kv "$main" "candidate.${c}.ridge")"
      echo "candidate.${c}.numerically_valid=$(kv "$main" "candidate.${c}.numerically_valid")"
      echo "candidate.${c}.maximum_normalized_residual=$(kv "$main" "candidate.${c}.maximum_normalized_residual")"
      echo "candidate.${c}.coefficient_l2_norm=$(kv "$main" "candidate.${c}.coefficient_l2_norm")"
      echo "candidate.${c}.validation.direction=$(kv "$main" "candidate.${c}.validation.directional_accuracy")"
      echo "candidate.${c}.validation.rank=$(kv "$main" "candidate.${c}.validation.pairwise_rank_accuracy")"
      echo "candidate.${c}.validation.correlation=$(kv "$main" "candidate.${c}.validation.correlation")"
      echo "candidate.${c}.validation.rmse=$(kv "$main" "candidate.${c}.validation.rmse")"
      echo "candidate.${c}.validation.rmse_target_rms_ratio=$(kv "$main" "candidate.${c}.validation.rmse_target_rms_ratio")"
    done
  } > "$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_arm_complete() {
  local index="$1" id="${ARM_IDS[$1]}" path main replay c
  path="$(arm_complete_receipt "$id")"
  main="$(arm_main_report "$id")"
  replay="$(arm_replay_report "$id")"
  validate_report "$main"
  validate_report "$replay"
  cmp -s -- "$main" "$replay" || fail "main/replay reports differ for ${id}"
  validate_invocation_lifecycle "$index" main "$((index*2+1))"
  validate_invocation_lifecycle "$index" replay "$((index*2+2))"
  load_kv_file "$path"
  expect "$path" schema_id "${PROTOCOL_ID}.arm_complete.v1"
  expect "$path" status complete
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" projection_complete_sha256 "$(sha256 "$PROJECTION_COMPLETE")"
  expect "$path" arm_index "$index"
  expect "$path" arm_id "$id"
  expect "$path" arm_family "${ARM_FAMILIES[$index]}"
  expect "$path" train_input "${ARM_TRAIN[$index]}"
  expect "$path" train_input_sha256 "${ARM_TRAIN_SHA[$index]}"
  expect "$path" validation_input "${ARM_VALIDATION[$index]}"
  expect "$path" validation_input_sha256 "${ARM_VALIDATION_SHA[$index]}"
  expect "$path" main_report_path "$main"
  expect "$path" main_report_sha256 "$(sha256 "$main")"
  expect "$path" replay_report_path "$replay"
  expect "$path" replay_report_sha256 "$(sha256 "$replay")"
  expect "$path" main_replay_byte_identical true
  expect "$path" evaluator_invocations_completed 2
  expect "$path" analytic_candidate_fit_count 12
  expect "$path" edge_channel_head_solve_count 108
  expect "$path" selected_candidate_index "$(kv "$main" selected_candidate_index)"
  expect "$path" selected_ridge "$(kv "$main" selected_ridge)"
  expect "$path" selected_validation_direction "$(kv "$main" selected.validation.directional_accuracy)"
  expect "$path" selected_validation_rank "$(kv "$main" selected.validation.pairwise_rank_accuracy)"
  expect "$path" selected_validation_correlation "$(kv "$main" selected.validation.correlation)"
  expect "$path" selected_validation_rmse "$(kv "$main" selected.validation.rmse)"
  expect "$path" selected_validation_rmse_target_rms_ratio "$(kv "$main" selected.validation.rmse_target_rms_ratio)"
  expect "$path" original_strong_gate_pass "$(kv "$main" validation_strong_gate_pass)"
  expect "$path" original_partial_gate_pass "$(kv "$main" validation_partial_gate_pass)"
  for c in 0 1 2 3 4 5; do
    expect "$path" "candidate.${c}.ridge" "$(kv "$main" "candidate.${c}.ridge")"
    expect "$path" "candidate.${c}.numerically_valid" true
    expect "$path" "candidate.${c}.maximum_normalized_residual" \
      "$(kv "$main" "candidate.${c}.maximum_normalized_residual")"
    expect "$path" "candidate.${c}.coefficient_l2_norm" \
      "$(kv "$main" "candidate.${c}.coefficient_l2_norm")"
    expect "$path" "candidate.${c}.validation.direction" \
      "$(kv "$main" "candidate.${c}.validation.directional_accuracy")"
    expect "$path" "candidate.${c}.validation.rank" \
      "$(kv "$main" "candidate.${c}.validation.pairwise_rank_accuracy")"
    expect "$path" "candidate.${c}.validation.correlation" \
      "$(kv "$main" "candidate.${c}.validation.correlation")"
    expect "$path" "candidate.${c}.validation.rmse" \
      "$(kv "$main" "candidate.${c}.validation.rmse")"
    expect "$path" "candidate.${c}.validation.rmse_target_rms_ratio" \
      "$(kv "$main" "candidate.${c}.validation.rmse_target_rms_ratio")"
  done
  [[ "$(wc -l < "$path")" == 83 ]] || fail "arm receipt key count mismatch: ${id}"
}

load_arm_metrics() {
  local index="$1" id="${ARM_IDS[$1]}" report
  validate_arm_complete "$index"
  report="$(arm_main_report "$id")"
  ARM_DIRECTION["$index"]="$(kv "$report" selected.validation.directional_accuracy)"
  ARM_RANK["$index"]="$(kv "$report" selected.validation.pairwise_rank_accuracy)"
  ARM_CORRELATION["$index"]="$(kv "$report" selected.validation.correlation)"
  ARM_RMSE["$index"]="$(kv "$report" selected.validation.rmse)"
  ARM_RMSE_RATIO["$index"]="$(kv "$report" selected.validation.rmse_target_rms_ratio)"
  ARM_STRONG["$index"]="$(kv "$report" validation_strong_gate_pass)"
  ARM_SELECTED_INDEX["$index"]="$(kv "$report" selected_candidate_index)"
  ARM_SELECTED_RIDGE["$index"]="$(kv "$report" selected_ridge)"
}

emit_alias_report_receipt() {
  local main replay candidate="${SCRATCH}/canonical_alias.report.status.candidate"
  main="$(arm_main_report all_tokens)"
  replay="$(arm_replay_report all_tokens)"
  {
    echo "schema_id=${PROTOCOL_ID}.canonical_alias_report.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "alias_input_receipt_sha256=$(sha256 "$ALIAS_INPUT_RECEIPT")"
    echo "executed_arm=all_tokens"
    echo "logical_alias=ablation.canonical"
    echo "executed_arm_receipt_sha256=$(sha256 "$(arm_complete_receipt all_tokens)")"
    echo "attributed_main_report_path=${main}"
    echo "attributed_main_report_sha256=$(sha256 "$main")"
    echo "attributed_replay_report_path=${replay}"
    echo "attributed_replay_report_sha256=$(sha256 "$replay")"
    echo "main_replay_byte_identical=true"
    echo "alias_input_byte_identical=true"
    echo "redundant_evaluator_execution=false"
  } > "$candidate"
  publish_once "$candidate" "$ALIAS_REPORT_RECEIPT" 0444
}

validate_alias_report_receipt() {
  local main replay
  main="$(arm_main_report all_tokens)"
  replay="$(arm_replay_report all_tokens)"
  validate_alias_input_receipt
  validate_arm_complete 0
  load_kv_file "$ALIAS_REPORT_RECEIPT"
  expect "$ALIAS_REPORT_RECEIPT" schema_id "${PROTOCOL_ID}.canonical_alias_report.v1"
  expect "$ALIAS_REPORT_RECEIPT" status complete
  expect "$ALIAS_REPORT_RECEIPT" protocol_id "$PROTOCOL_ID"
  expect "$ALIAS_REPORT_RECEIPT" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$ALIAS_REPORT_RECEIPT" alias_input_receipt_sha256 "$(sha256 "$ALIAS_INPUT_RECEIPT")"
  expect "$ALIAS_REPORT_RECEIPT" executed_arm all_tokens
  expect "$ALIAS_REPORT_RECEIPT" logical_alias ablation.canonical
  expect "$ALIAS_REPORT_RECEIPT" executed_arm_receipt_sha256 "$(sha256 "$(arm_complete_receipt all_tokens)")"
  expect "$ALIAS_REPORT_RECEIPT" attributed_main_report_path "$main"
  expect "$ALIAS_REPORT_RECEIPT" attributed_main_report_sha256 "$(sha256 "$main")"
  expect "$ALIAS_REPORT_RECEIPT" attributed_replay_report_path "$replay"
  expect "$ALIAS_REPORT_RECEIPT" attributed_replay_report_sha256 "$(sha256 "$replay")"
  expect "$ALIAS_REPORT_RECEIPT" main_replay_byte_identical true
  expect "$ALIAS_REPORT_RECEIPT" alias_input_byte_identical true
  expect "$ALIAS_REPORT_RECEIPT" redundant_evaluator_execution false
  [[ "$(wc -l < "$ALIAS_REPORT_RECEIPT")" == 15 ]] ||
    fail "canonical alias report receipt key count mismatch"
}

cross_arm_better() {
  local candidate="$1" incumbent="$2"
  awk -v t="$TIE_TOLERANCE" \
      -v cd="${ARM_DIRECTION[$candidate]}" -v id="${ARM_DIRECTION[$incumbent]}" \
      -v cr="${ARM_RANK[$candidate]}" -v ir="${ARM_RANK[$incumbent]}" \
      -v cc="${ARM_CORRELATION[$candidate]}" -v ic="${ARM_CORRELATION[$incumbent]}" \
      -v ce="${ARM_RMSE[$candidate]}" -v ie="${ARM_RMSE[$incumbent]}" '
    BEGIN {
      if (cd > id+t) exit 0; if (id > cd+t) exit 1
      if (cr > ir+t) exit 0; if (ir > cr+t) exit 1
      if (cc > ic+t) exit 0; if (ic > cc+t) exit 1
      if (ce+t < ie) exit 0
      exit 1
    }'
}

declare -a RANKED_INDEX=()

compute_descriptive_ranking() {
  local rank index best
  declare -A used=()
  RANKED_INDEX=()
  for rank in 0 1 2 3 4 5 6; do
    best=-1
    for index in 0 1 2 3 4 5 6; do
      [[ -z "${used[$index]+x}" ]] || continue
      if (( best < 0 )) || cross_arm_better "$index" "$best"; then best="$index"; fi
    done
    (( best >= 0 )) || fail "descriptive ranking could not select rank ${rank}"
    RANKED_INDEX["$rank"]="$best"
    used["$best"]=1
  done
}

emit_science_complete() {
  local completed="$1" pass_index="$2" classification="$3"
  local candidate="${SCRATCH}/science.complete.status.candidate" index id status
  local stop_triggered=false stop_arm=not_applicable all_evaluated=false ranking=false
  if (( pass_index >= 0 )); then
    stop_triggered=true
    stop_arm="${ARM_IDS[$pass_index]}"
    if (( completed == UNIQUE_PAIR_COUNT )); then all_evaluated=true; fi
  else
    all_evaluated=true
    ranking=true
  fi
  {
    echo "schema_id=${PROTOCOL_ID}.science_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "canonical_alias_report_sha256=$(sha256 "$ALIAS_REPORT_RECEIPT")"
    echo "classification=${classification}"
    echo "unique_pairs_completed=${completed}"
    echo "logical_arms_represented=$((completed+1))"
    echo "evaluator_invocations_completed=$((completed*2))"
    echo "analytic_candidate_fit_count=$((completed*12))"
    echo "edge_channel_head_solve_count=$((completed*108))"
    echo "main_replay_parity_check_count=${completed}"
    echo "projection_split_check_count=14"
    echo "strong_gate_pass_count=$(if (( pass_index >= 0 )); then echo 1; else echo 0; fi)"
    echo "strong_stop_triggered=${stop_triggered}"
    echo "strong_stop_unique_index=${pass_index}"
    echo "strong_stop_arm=${stop_arm}"
    echo "all_unique_pairs_evaluated=${all_evaluated}"
    echo "descriptive_ranking_available=${ranking}"
    echo "scientific_execution_completed=true"
    for index in "${!ARM_IDS[@]}"; do
      id="${ARM_IDS[$index]}"
      if (( index < completed )); then
        status=complete
        echo "arm.${index}.status=${status}"
        echo "arm.${index}.receipt_sha256=$(sha256 "$(arm_complete_receipt "$id")")"
      else
        status=not_evaluated_due_to_strong_gate
        echo "arm.${index}.status=${status}"
        echo "arm.${index}.receipt_sha256=not_available"
      fi
    done
  } > "$candidate"
  publish_once "$candidate" "$SCIENCE_COMPLETE" 0444
}

validate_science_complete() {
  local index id completed pass_index classification status expected
  load_kv_file "$SCIENCE_COMPLETE"
  expect "$SCIENCE_COMPLETE" schema_id "${PROTOCOL_ID}.science_complete.v1"
  expect "$SCIENCE_COMPLETE" status complete
  expect "$SCIENCE_COMPLETE" protocol_id "$PROTOCOL_ID"
  expect "$SCIENCE_COMPLETE" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$SCIENCE_COMPLETE" projection_complete_sha256 "$(sha256 "$PROJECTION_COMPLETE")"
  expect "$SCIENCE_COMPLETE" canonical_alias_report_sha256 "$(sha256 "$ALIAS_REPORT_RECEIPT")"
  validate_projection_complete
  validate_alias_report_receipt
  completed="$(kv "$SCIENCE_COMPLETE" unique_pairs_completed)"
  pass_index="$(kv "$SCIENCE_COMPLETE" strong_stop_unique_index)"
  classification="$(kv "$SCIENCE_COMPLETE" classification)"
  [[ "$completed" =~ ^[1-7]$ ]] && [[ "$pass_index" =~ ^(-1|[0-6])$ ]] ||
    fail "invalid science-complete bounds"
  for index in "${!ARM_IDS[@]}"; do
    id="${ARM_IDS[$index]}"
    if (( index < completed )); then
      load_arm_metrics "$index"
      expect "$SCIENCE_COMPLETE" "arm.${index}.status" complete
      expect "$SCIENCE_COMPLETE" "arm.${index}.receipt_sha256" \
        "$(sha256 "$(arm_complete_receipt "$id")")"
    else
      expect "$SCIENCE_COMPLETE" "arm.${index}.status" not_evaluated_due_to_strong_gate
      expect "$SCIENCE_COMPLETE" "arm.${index}.receipt_sha256" not_available
      [[ ! -e "$(arm_complete_receipt "$id")" && ! -e "$(arm_main_report "$id")" &&
         ! -e "$(arm_replay_report "$id")" ]] ||
        fail "unevaluated suffix contains scientific outputs: ${id}"
    fi
  done
  expect "$SCIENCE_COMPLETE" logical_arms_represented "$((completed+1))"
  expect "$SCIENCE_COMPLETE" evaluator_invocations_completed "$((completed*2))"
  expect "$SCIENCE_COMPLETE" analytic_candidate_fit_count "$((completed*12))"
  expect "$SCIENCE_COMPLETE" edge_channel_head_solve_count "$((completed*108))"
  expect "$SCIENCE_COMPLETE" main_replay_parity_check_count "$completed"
  expect "$SCIENCE_COMPLETE" projection_split_check_count 14
  expect "$SCIENCE_COMPLETE" scientific_execution_completed true
  if (( pass_index >= 0 )); then
    [[ "$completed" == "$((pass_index+1))" ]] ||
      fail "strong stop position and completed prefix differ"
    [[ "${ARM_STRONG[$pass_index]}" == true ]] || fail "strong stop arm did not pass"
    for ((index=0; index<pass_index; ++index)); do
      [[ "${ARM_STRONG[$index]}" == false ]] || fail "strong stop was not the first passing arm"
    done
    expect "$SCIENCE_COMPLETE" classification "$PASS_CLASS"
    expect "$SCIENCE_COMPLETE" strong_gate_pass_count 1
    expect "$SCIENCE_COMPLETE" strong_stop_triggered true
    expect "$SCIENCE_COMPLETE" strong_stop_arm "${ARM_IDS[$pass_index]}"
    if (( completed == UNIQUE_PAIR_COUNT )); then
      expect "$SCIENCE_COMPLETE" all_unique_pairs_evaluated true
    else
      expect "$SCIENCE_COMPLETE" all_unique_pairs_evaluated false
    fi
    expect "$SCIENCE_COMPLETE" descriptive_ranking_available false
  else
    [[ "$completed" == 7 ]] || fail "no-pass science receipt is not exhaustive"
    for index in 0 1 2 3 4 5 6; do
      [[ "${ARM_STRONG[$index]}" == false ]] || fail "no-pass receipt contains a passing arm"
    done
    expect "$SCIENCE_COMPLETE" classification "$NO_PASS_CLASS"
    expect "$SCIENCE_COMPLETE" strong_gate_pass_count 0
    expect "$SCIENCE_COMPLETE" strong_stop_triggered false
    expect "$SCIENCE_COMPLETE" strong_stop_arm not_applicable
    expect "$SCIENCE_COMPLETE" all_unique_pairs_evaluated true
    expect "$SCIENCE_COMPLETE" descriptive_ranking_available true
  fi
  [[ "$(wc -l < "$SCIENCE_COMPLETE")" == 35 ]] ||
    fail "science-complete receipt key count mismatch"
}

declare -A RANK_BY_INDEX=()

prepare_result_ranking() {
  local classification="$1" rank index
  RANK_BY_INDEX=()
  if [[ "$classification" == "$NO_PASS_CLASS" ]]; then
    compute_descriptive_ranking
    for rank in 0 1 2 3 4 5 6; do
      index="${RANKED_INDEX[$rank]}"
      RANK_BY_INDEX["$index"]="$((rank+1))"
    done
  fi
}

emit_result_body() {
  local classification completed pass_index stop_triggered stop_arm
  local all_evaluated ranking_available ranking_count logical_represented
  local index id execution_status receipt_sha main_sha replay_sha
  local selected_index selected_ridge direction rank correlation rmse rmse_ratio
  local strong partial descriptive_rank
  classification="$(kv "$SCIENCE_COMPLETE" classification)"
  completed="$(kv "$SCIENCE_COMPLETE" unique_pairs_completed)"
  pass_index="$(kv "$SCIENCE_COMPLETE" strong_stop_unique_index)"
  stop_triggered="$(kv "$SCIENCE_COMPLETE" strong_stop_triggered)"
  stop_arm="$(kv "$SCIENCE_COMPLETE" strong_stop_arm)"
  all_evaluated="$(kv "$SCIENCE_COMPLETE" all_unique_pairs_evaluated)"
  ranking_available="$(kv "$SCIENCE_COMPLETE" descriptive_ranking_available)"
  logical_represented="$((completed+1))"
  if [[ "$ranking_available" == true ]]; then ranking_count=7; else ranking_count=0; fi
  prepare_result_ranking "$classification"
  {
    echo "schema_id=${PROTOCOL_ID}"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "benchmark_id=synthetic_continuous_graph_v2"
    echo "project_goal=Project Clear Signal - Exhaust the Sealed V2 Raw96 Affine Inventory"
    echo "diagnostic_phase=${DIAGNOSTIC_PHASE}"
    echo "diagnostic_authority=retrospective_development_only"
    echo "benchmark_acceptance_authority=false"
    echo "certified_authorization_eligible=false"
    echo "classification=${classification}"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "attempt_path=${ATTEMPT}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "prepared_receipt_path=${PREP_RECEIPT}"
    echo "prepared_receipt_sha256=$(sha256 "$PREP_RECEIPT")"
    echo "science_complete_path=${SCIENCE_COMPLETE}"
    echo "science_complete_sha256=$(sha256 "$SCIENCE_COMPLETE")"
    echo "projection_complete_path=${PROJECTION_COMPLETE}"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "canonical_alias_input_receipt_path=${ALIAS_INPUT_RECEIPT}"
    echo "canonical_alias_input_receipt_sha256=$(sha256 "$ALIAS_INPUT_RECEIPT")"
    echo "canonical_alias_report_receipt_path=${ALIAS_REPORT_RECEIPT}"
    echo "canonical_alias_report_receipt_sha256=$(sha256 "$ALIAS_REPORT_RECEIPT")"
    echo "phase2a_source_path=${PHASE2A_SOURCE}"
    echo "phase2a_source_sha256=${PHASE2A_SOURCE_SHA}"
    echo "canonical_parser_path=${CANONICAL_PARSER}"
    echo "canonical_parser_sha256=${CANONICAL_PARSER_SHA}"
    echo "evaluator_binary_path=${BIN}"
    echo "evaluator_binary_sha256=$(sha256 "$BIN")"
    echo "serving_result_path=${SERVING_RESULT}"
    echo "serving_result_sha256=${SERVING_RESULT_SHA}"
    echo "ablation_result_path=${ABLATION_RESULT}"
    echo "ablation_result_sha256=${ABLATION_RESULT_SHA}"
    echo "ablation_selection_path=${ABLATION_SELECTION}"
    echo "ablation_selection_sha256=${ABLATION_SELECTION_SHA}"
    echo "unique_pair_count=${UNIQUE_PAIR_COUNT}"
    echo "logical_arm_count=${LOGICAL_ARM_COUNT}"
    echo "unique_pairs_completed=${completed}"
    echo "logical_arms_represented=${logical_represented}"
    echo "evaluator_invocations_completed=$((completed*2))"
    echo "analytic_candidate_fit_count=$((completed*12))"
    echo "edge_channel_head_solve_count=$((completed*108))"
    echo "main_replay_parity_check_count=${completed}"
    echo "projection_split_check_count=14"
    echo "strong_gate_pass_count=$(kv "$SCIENCE_COMPLETE" strong_gate_pass_count)"
    echo "strong_stop_triggered=${stop_triggered}"
    echo "strong_stop_unique_index=${pass_index}"
    echo "strong_stop_arm=${stop_arm}"
    echo "all_unique_pairs_evaluated=${all_evaluated}"
    echo "descriptive_ranking_available=${ranking_available}"
    echo "descriptive_ranked_pair_count=${ranking_count}"
    echo "execution_order=all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced,endpoint_scale,time_only,no_tf_alignment"
    echo "within_arm_selection=frozen_six_ridge_validation_comparator"
    echo "cross_arm_comparator=direction,rank,correlation,lower_rmse,fixed_order_tie"
    echo "selection_tie_tolerance=${TIE_TOLERANCE}"
    echo "original_strong_gate=direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25"
    echo "original_partial_gate=direction>=0.80,rank>=0.78"
    echo "retrospective_development_only=true"
    echo "fresh_confirmation=false"
    echo "successful_result_authorizes_next_stage=false"
    echo "candidate_summary_receipts_preserved=true"
    echo "capture_execution_count=0"
    echo "encoder_execution_count=0"
    echo "checkpoint_read_count=0"
    echo "representation_model_execution_count=0"
    echo "mdn_model_execution_count=0"
    echo "optimizer_fit_count=0"
    echo "optimizer_step_count=0"
    echo "refit_count=0"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "maximum_anchor_read=2815"
    echo "train_anchor_range=[0,2496)"
    echo "validation_anchor_range=[2560,2816)"
    echo "phase2a_complete_report_key_count=${COMPLETE_REPORT_KEY_COUNT}"
    echo "phase2a_complete_report_sorted_key_sha256=${COMPLETE_REPORT_SORTED_KEY_SHA}"
    echo "main_replay_required=true"
    echo "canonical_alias=ablation.canonical->all_tokens"
    for index in "${!ARM_IDS[@]}"; do
      id="${ARM_IDS[$index]}"
      if (( index < completed )); then
        execution_status=complete
        receipt_sha="$(sha256 "$(arm_complete_receipt "$id")")"
        main_sha="$(sha256 "$(arm_main_report "$id")")"
        replay_sha="$(sha256 "$(arm_replay_report "$id")")"
        selected_index="${ARM_SELECTED_INDEX[$index]}"
        selected_ridge="${ARM_SELECTED_RIDGE[$index]}"
        direction="${ARM_DIRECTION[$index]}"
        rank="${ARM_RANK[$index]}"
        correlation="${ARM_CORRELATION[$index]}"
        rmse="${ARM_RMSE[$index]}"
        rmse_ratio="${ARM_RMSE_RATIO[$index]}"
        strong="${ARM_STRONG[$index]}"
        partial="$(kv "$(arm_main_report "$id")" validation_partial_gate_pass)"
        descriptive_rank="${RANK_BY_INDEX[$index]:-not_applicable}"
      else
        execution_status=not_evaluated_due_to_strong_gate
        receipt_sha=not_available
        main_sha=not_available
        replay_sha=not_available
        selected_index=not_evaluated
        selected_ridge=not_evaluated
        direction=not_evaluated
        rank=not_evaluated
        correlation=not_evaluated
        rmse=not_evaluated
        rmse_ratio=not_evaluated
        strong=not_evaluated
        partial=not_evaluated
        descriptive_rank=not_applicable
      fi
      echo "arm.${index}.id=${id}"
      echo "arm.${index}.family=${ARM_FAMILIES[$index]}"
      echo "arm.${index}.train_input=${ARM_TRAIN[$index]}"
      echo "arm.${index}.train_sha256=${ARM_TRAIN_SHA[$index]}"
      echo "arm.${index}.validation_input=${ARM_VALIDATION[$index]}"
      echo "arm.${index}.validation_sha256=${ARM_VALIDATION_SHA[$index]}"
      echo "arm.${index}.source_receipt=${ARM_SOURCE_RECEIPTS[$index]}"
      echo "arm.${index}.source_receipt_sha256=${ARM_SOURCE_RECEIPT_SHA[$index]}"
      echo "arm.${index}.projection_receipt_path=$(arm_projection_receipt "$id")"
      echo "arm.${index}.projection_receipt_sha256=$(sha256 "$(arm_projection_receipt "$id")")"
      echo "arm.${index}.execution_status=${execution_status}"
      echo "arm.${index}.completion_receipt_path=$(arm_complete_receipt "$id")"
      echo "arm.${index}.completion_receipt_sha256=${receipt_sha}"
      echo "arm.${index}.main_report_path=$(arm_main_report "$id")"
      echo "arm.${index}.main_report_sha256=${main_sha}"
      echo "arm.${index}.replay_report_path=$(arm_replay_report "$id")"
      echo "arm.${index}.replay_report_sha256=${replay_sha}"
      echo "arm.${index}.selected_candidate_index=${selected_index}"
      echo "arm.${index}.selected_ridge=${selected_ridge}"
      echo "arm.${index}.validation_direction=${direction}"
      echo "arm.${index}.validation_rank=${rank}"
      echo "arm.${index}.validation_correlation=${correlation}"
      echo "arm.${index}.validation_rmse=${rmse}"
      echo "arm.${index}.validation_rmse_target_rms_ratio=${rmse_ratio}"
      echo "arm.${index}.original_strong_gate_pass=${strong}"
      echo "arm.${index}.original_partial_gate_pass=${partial}"
      echo "arm.${index}.descriptive_rank=${descriptive_rank}"
    done
    echo "logical_alias.id=ablation.canonical"
    echo "logical_alias.executed_arm=all_tokens"
    echo "logical_alias.train_input=${CANONICAL_TRAIN}"
    echo "logical_alias.validation_input=${CANONICAL_VALIDATION}"
    echo "logical_alias.input_receipt_sha256=$(sha256 "$ALIAS_INPUT_RECEIPT")"
    echo "logical_alias.report_receipt_sha256=$(sha256 "$ALIAS_REPORT_RECEIPT")"
    echo "logical_alias.report_attribution_complete=true"
    echo "logical_alias.redundant_evaluator_execution=false"
  }
}

validate_result_content() {
  local file="$1" classification completed pass_index ranking_available ranking_count
  local index id execution_status descriptive_rank expected_rank
  require_file "$file" 444
  [[ "$(wc -l < "$file")" == 279 ]] || fail "development result key count mismatch"
  load_kv_file "$file"
  expect "$file" schema_id "$PROTOCOL_ID"
  expect "$file" status complete
  expect "$file" protocol_id "$PROTOCOL_ID"
  expect "$file" benchmark_id synthetic_continuous_graph_v2
  expect "$file" project_goal 'Project Clear Signal - Exhaust the Sealed V2 Raw96 Affine Inventory'
  expect "$file" diagnostic_phase "$DIAGNOSTIC_PHASE"
  expect "$file" diagnostic_authority retrospective_development_only
  expect "$file" benchmark_acceptance_authority false
  expect "$file" certified_authorization_eligible false
  validate_science_complete
  classification="$(kv "$SCIENCE_COMPLETE" classification)"
  completed="$(kv "$SCIENCE_COMPLETE" unique_pairs_completed)"
  pass_index="$(kv "$SCIENCE_COMPLETE" strong_stop_unique_index)"
  ranking_available="$(kv "$SCIENCE_COMPLETE" descriptive_ranking_available)"
  if [[ "$ranking_available" == true ]]; then ranking_count=7; else ranking_count=0; fi
  prepare_result_ranking "$classification"
  expect "$file" classification "$classification"
  expect "$file" preregistration_path "$PREREG"
  expect "$file" preregistration_sha256 "$PREREG_SHA"
  expect "$file" runner_path "$RUNNER"
  expect "$file" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$file" attempt_path "$ATTEMPT"
  expect "$file" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$file" prepared_receipt_path "$PREP_RECEIPT"
  expect "$file" prepared_receipt_sha256 "$(sha256 "$PREP_RECEIPT")"
  expect "$file" science_complete_path "$SCIENCE_COMPLETE"
  expect "$file" science_complete_sha256 "$(sha256 "$SCIENCE_COMPLETE")"
  expect "$file" projection_complete_path "$PROJECTION_COMPLETE"
  expect "$file" projection_complete_sha256 "$(sha256 "$PROJECTION_COMPLETE")"
  expect "$file" canonical_alias_input_receipt_path "$ALIAS_INPUT_RECEIPT"
  expect "$file" canonical_alias_input_receipt_sha256 "$(sha256 "$ALIAS_INPUT_RECEIPT")"
  expect "$file" canonical_alias_report_receipt_path "$ALIAS_REPORT_RECEIPT"
  expect "$file" canonical_alias_report_receipt_sha256 "$(sha256 "$ALIAS_REPORT_RECEIPT")"
  expect "$file" phase2a_source_path "$PHASE2A_SOURCE"
  expect "$file" phase2a_source_sha256 "$PHASE2A_SOURCE_SHA"
  expect "$file" canonical_parser_path "$CANONICAL_PARSER"
  expect "$file" canonical_parser_sha256 "$CANONICAL_PARSER_SHA"
  expect "$file" evaluator_binary_path "$BIN"
  expect "$file" evaluator_binary_sha256 "$PHASE2A_BINARY_SHA"
  expect "$file" serving_result_path "$SERVING_RESULT"
  expect "$file" serving_result_sha256 "$SERVING_RESULT_SHA"
  expect "$file" ablation_result_path "$ABLATION_RESULT"
  expect "$file" ablation_result_sha256 "$ABLATION_RESULT_SHA"
  expect "$file" ablation_selection_path "$ABLATION_SELECTION"
  expect "$file" ablation_selection_sha256 "$ABLATION_SELECTION_SHA"
  expect "$file" unique_pair_count "$UNIQUE_PAIR_COUNT"
  expect "$file" logical_arm_count "$LOGICAL_ARM_COUNT"
  expect "$file" unique_pairs_completed "$completed"
  expect "$file" logical_arms_represented "$((completed+1))"
  expect "$file" evaluator_invocations_completed "$((completed*2))"
  expect "$file" analytic_candidate_fit_count "$((completed*12))"
  expect "$file" edge_channel_head_solve_count "$((completed*108))"
  expect "$file" main_replay_parity_check_count "$completed"
  expect "$file" projection_split_check_count 14
  expect "$file" strong_gate_pass_count "$(kv "$SCIENCE_COMPLETE" strong_gate_pass_count)"
  expect "$file" strong_stop_triggered "$(kv "$SCIENCE_COMPLETE" strong_stop_triggered)"
  expect "$file" strong_stop_unique_index "$pass_index"
  expect "$file" strong_stop_arm "$(kv "$SCIENCE_COMPLETE" strong_stop_arm)"
  expect "$file" all_unique_pairs_evaluated "$(kv "$SCIENCE_COMPLETE" all_unique_pairs_evaluated)"
  expect "$file" descriptive_ranking_available "$ranking_available"
  expect "$file" descriptive_ranked_pair_count "$ranking_count"
  expect "$file" execution_order all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced,endpoint_scale,time_only,no_tf_alignment
  expect "$file" within_arm_selection frozen_six_ridge_validation_comparator
  expect "$file" cross_arm_comparator direction,rank,correlation,lower_rmse,fixed_order_tie
  expect "$file" selection_tie_tolerance "$TIE_TOLERANCE"
  expect "$file" original_strong_gate 'direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25'
  expect "$file" original_partial_gate 'direction>=0.80,rank>=0.78'
  expect "$file" retrospective_development_only true
  expect "$file" fresh_confirmation false
  expect "$file" successful_result_authorizes_next_stage false
  expect "$file" candidate_summary_receipts_preserved true
  expect "$file" capture_execution_count 0
  expect "$file" encoder_execution_count 0
  expect "$file" checkpoint_read_count 0
  expect "$file" representation_model_execution_count 0
  expect "$file" mdn_model_execution_count 0
  expect "$file" optimizer_fit_count 0
  expect "$file" optimizer_step_count 0
  expect "$file" refit_count 0
  expect "$file" certified_input_access false
  expect "$file" final_holdout_access false
  expect "$file" policy_access false
  expect "$file" maximum_anchor_read 2815
  expect "$file" train_anchor_range '[0,2496)'
  expect "$file" validation_anchor_range '[2560,2816)'
  expect "$file" phase2a_complete_report_key_count "$COMPLETE_REPORT_KEY_COUNT"
  expect "$file" phase2a_complete_report_sorted_key_sha256 "$COMPLETE_REPORT_SORTED_KEY_SHA"
  expect "$file" main_replay_required true
  expect "$file" canonical_alias 'ablation.canonical->all_tokens'
  for index in "${!ARM_IDS[@]}"; do
    id="${ARM_IDS[$index]}"
    expect "$file" "arm.${index}.id" "$id"
    expect "$file" "arm.${index}.family" "${ARM_FAMILIES[$index]}"
    expect "$file" "arm.${index}.train_input" "${ARM_TRAIN[$index]}"
    expect "$file" "arm.${index}.train_sha256" "${ARM_TRAIN_SHA[$index]}"
    expect "$file" "arm.${index}.validation_input" "${ARM_VALIDATION[$index]}"
    expect "$file" "arm.${index}.validation_sha256" "${ARM_VALIDATION_SHA[$index]}"
    expect "$file" "arm.${index}.source_receipt" "${ARM_SOURCE_RECEIPTS[$index]}"
    expect "$file" "arm.${index}.source_receipt_sha256" "${ARM_SOURCE_RECEIPT_SHA[$index]}"
    expect "$file" "arm.${index}.projection_receipt_path" "$(arm_projection_receipt "$id")"
    expect "$file" "arm.${index}.projection_receipt_sha256" \
      "$(sha256 "$(arm_projection_receipt "$id")")"
    expect "$file" "arm.${index}.completion_receipt_path" "$(arm_complete_receipt "$id")"
    expect "$file" "arm.${index}.main_report_path" "$(arm_main_report "$id")"
    expect "$file" "arm.${index}.replay_report_path" "$(arm_replay_report "$id")"
    if (( index < completed )); then
      execution_status=complete
      validate_arm_complete "$index"
      expect "$file" "arm.${index}.completion_receipt_sha256" \
        "$(sha256 "$(arm_complete_receipt "$id")")"
      expect "$file" "arm.${index}.main_report_sha256" "$(sha256 "$(arm_main_report "$id")")"
      expect "$file" "arm.${index}.replay_report_sha256" "$(sha256 "$(arm_replay_report "$id")")"
      expect "$file" "arm.${index}.selected_candidate_index" "${ARM_SELECTED_INDEX[$index]}"
      expect "$file" "arm.${index}.selected_ridge" "${ARM_SELECTED_RIDGE[$index]}"
      expect "$file" "arm.${index}.validation_direction" "${ARM_DIRECTION[$index]}"
      expect "$file" "arm.${index}.validation_rank" "${ARM_RANK[$index]}"
      expect "$file" "arm.${index}.validation_correlation" "${ARM_CORRELATION[$index]}"
      expect "$file" "arm.${index}.validation_rmse" "${ARM_RMSE[$index]}"
      expect "$file" "arm.${index}.validation_rmse_target_rms_ratio" "${ARM_RMSE_RATIO[$index]}"
      expect "$file" "arm.${index}.original_strong_gate_pass" "${ARM_STRONG[$index]}"
      expect "$file" "arm.${index}.original_partial_gate_pass" \
        "$(kv "$(arm_main_report "$id")" validation_partial_gate_pass)"
      if [[ "$ranking_available" == true ]]; then
        expected_rank="${RANK_BY_INDEX[$index]}"
      else
        expected_rank=not_applicable
      fi
      expect "$file" "arm.${index}.descriptive_rank" "$expected_rank"
    else
      execution_status=not_evaluated_due_to_strong_gate
      expect "$file" "arm.${index}.completion_receipt_sha256" not_available
      expect "$file" "arm.${index}.main_report_sha256" not_available
      expect "$file" "arm.${index}.replay_report_sha256" not_available
      for descriptive_rank in selected_candidate_index selected_ridge validation_direction \
          validation_rank validation_correlation validation_rmse \
          validation_rmse_target_rms_ratio original_strong_gate_pass \
          original_partial_gate_pass; do
        expect "$file" "arm.${index}.${descriptive_rank}" not_evaluated
      done
      expect "$file" "arm.${index}.descriptive_rank" not_applicable
    fi
    expect "$file" "arm.${index}.execution_status" "$execution_status"
  done
  expect "$file" logical_alias.id ablation.canonical
  expect "$file" logical_alias.executed_arm all_tokens
  expect "$file" logical_alias.train_input "$CANONICAL_TRAIN"
  expect "$file" logical_alias.validation_input "$CANONICAL_VALIDATION"
  expect "$file" logical_alias.input_receipt_sha256 "$(sha256 "$ALIAS_INPUT_RECEIPT")"
  expect "$file" logical_alias.report_receipt_sha256 "$(sha256 "$ALIAS_REPORT_RECEIPT")"
  expect "$file" logical_alias.report_attribution_complete true
  expect "$file" logical_alias.redundant_evaluator_execution false
}

emit_result_candidate() {
  [[ ! -e "$RESULT_CANDIDATE" ]] || fail "result candidate already exists"
  emit_result_body > "$RESULT_CANDIDATE"
  chmod 0444 -- "$RESULT_CANDIDATE"
  validate_result_content "$RESULT_CANDIDATE"
}

commit_result_final() {
  require_file "$RESULT_CANDIDATE" 444
  [[ ! -e "$RESULT" && ! -L "$RESULT" ]] || fail "development result destination exists"
  mv -T -n -- "$RESULT_CANDIDATE" "$RESULT"
}

validate_result() {
  preflight_science
  validate_prepare_receipt
  validate_attempt
  validate_result_content "$RESULT"
}

authorize_private_worker() {
  local token="${CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_TOKEN:-}"
  local expected_identity="${CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_IDENTITY:-}"
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
  unset CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_TOKEN
  unset CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_IDENTITY token observed extra

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
3|${SERVING_LOCK}
4|${ABLATION_LOCK}
5|${CANONICAL_LOCK}
EOF
}

private_worker() {
  local index completed=0 pass_index=-1 classification
  authorize_private_worker
  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  trap 'exit 131' QUIT
  preflight_science
  validate_prepare_receipt
  validate_attempt
  [[ ! -e "$RESULT" && ! -e "$TERMINAL" && ! -e "$PROJECTION_COMPLETE" &&
     ! -e "$ALIAS_INPUT_RECEIPT" && ! -e "$ALIAS_REPORT_RECEIPT" &&
     ! -e "$SCIENCE_COMPLETE" && ! -e "$RESULT_CANDIDATE" ]] ||
    fail "scientific lifecycle is not pristine"

  run_projection_stage
  validate_projection_complete
  for index in 0 1 2 3 4 5 6; do
    run_evaluator_lane "$index" main "$((index*2+1))"
    run_evaluator_lane "$index" replay "$((index*2+2))"
    cmp -s -- "$(arm_main_report "${ARM_IDS[$index]}")" \
      "$(arm_replay_report "${ARM_IDS[$index]}")" ||
      fail "main/replay reports differ for ${ARM_IDS[$index]}"
    emit_arm_complete "$index"
    validate_arm_complete "$index"
    load_arm_metrics "$index"
    completed="$((index+1))"
    if (( index == 0 )); then
      emit_alias_report_receipt
      validate_alias_report_receipt
    fi
    if [[ "${ARM_STRONG[$index]}" == true ]]; then
      pass_index="$index"
      break
    fi
  done

  if (( pass_index >= 0 )); then
    classification="$PASS_CLASS"
  else
    [[ "$completed" == "$UNIQUE_PAIR_COUNT" ]] ||
      fail "no-pass classification requires exhaustive execution"
    classification="$NO_PASS_CLASS"
  fi
  emit_science_complete "$completed" "$pass_index" "$classification"
  validate_science_complete
  emit_result_candidate
  commit_result_final
}

hash_or_not_available() {
  local path="$1"
  if [[ -f "$path" && ! -L "$path" ]]; then sha256 "$path"; else printf '%s' not_available; fi
}

publish_failure_evidence() {
  local candidate file
  if [[ -f "$WORKER_LOG_CANDIDATE" && ! -L "$WORKER_LOG_CANDIDATE" &&
        ! -e "$WORKER_LOG" ]]; then
    publish_once "$WORKER_LOG_CANDIDATE" "$WORKER_LOG" 0444
  fi
  if [[ -f "$RESULT_CANDIDATE" && ! -L "$RESULT_CANDIDATE" &&
        ! -e "$REJECTED_RESULT" ]]; then
    publish_once "$RESULT_CANDIDATE" "$REJECTED_RESULT" 0444
  fi
  while IFS= read -r -d '' file; do
    [[ -L "$file" ]] && continue
    chmod 0444 -- "$file"
    require_file "$file" 444
  done < <(find "$SCRATCH" -type f -print0)

  if [[ ! -e "$REJECTED_EVIDENCE_MANIFEST" ]]; then
    candidate="${SCRATCH}/.rejected.evidence.sha256.candidate.$$"
    {
      while IFS= read -r -d '' file; do
        [[ -f "$file" && ! -L "$file" ]] || continue
        printf '%s  %s\n' "$(sha256 "$file")" "$file"
      done < <(find "$EVIDENCE" "$SCRATCH" -type f ! -path "$candidate" -print0 |
        LC_ALL=C sort -z)
    } > "$candidate"
    publish_once "$candidate" "$REJECTED_EVIDENCE_MANIFEST" 0444
  else
    require_file "$REJECTED_EVIDENCE_MANIFEST" 444
  fi
}

strict_validator_status() {
  local validator="$1"
  shift
  set +e
  bash -euo pipefail -c '
    source "$1"
    validator="$2"
    shift 2
    "$validator" "$@"
  ' _ "$RUNNER" "$validator" "$@" >/dev/null 2>&1
  STRICT_VALIDATOR_RC=$?
  set -e
}

collect_terminal_progress() {
  local index lane id report candidate sequence
  TERMINAL_PROJECTION_PAIRS=0
  TERMINAL_PROJECTION_COMPLETE=false
  TERMINAL_INVOCATIONS_STARTED=0
  TERMINAL_INVOCATIONS_RETURNED=0
  TERMINAL_VALIDATED_REPORTS=0
  TERMINAL_COMPLETED_PAIRS=0
  TERMINAL_SCIENCE_VALID=false
  TERMINAL_LAST_STAGE=attempt_consumed

  for index in 0 1 2 3 4 5 6; do
    strict_validator_status validate_projection_receipt "$index"
    if (( STRICT_VALIDATOR_RC == 0 )); then
      TERMINAL_PROJECTION_PAIRS="$((TERMINAL_PROJECTION_PAIRS+1))"
    else
      break
    fi
  done
  strict_validator_status validate_projection_complete
  if (( STRICT_VALIDATOR_RC == 0 )); then
    TERMINAL_PROJECTION_COMPLETE=true
    TERMINAL_LAST_STAGE=projection_complete
  fi

  for index in 0 1 2 3 4 5 6; do
    id="${ARM_IDS[$index]}"
    for lane in main replay; do
      if [[ "$lane" == main ]]; then
        sequence="$((index*2+1))"
      else
        sequence="$((index*2+2))"
      fi
      strict_validator_status validate_invocation_launch_receipt \
        "$index" "$lane" "$sequence"
      if (( STRICT_VALIDATOR_RC == 0 )); then
        TERMINAL_INVOCATIONS_STARTED="$((TERMINAL_INVOCATIONS_STARTED+1))"
        TERMINAL_LAST_STAGE=evaluator_started
      else
        break 2
      fi
      strict_validator_status validate_invocation_returned_evidence \
        "$index" "$lane" "$sequence"
      if (( STRICT_VALIDATOR_RC == 0 )); then
        TERMINAL_INVOCATIONS_RETURNED="$((TERMINAL_INVOCATIONS_RETURNED+1))"
        TERMINAL_LAST_STAGE=evaluator_returned
      fi
      if [[ "$lane" == main ]]; then
        report="$(arm_main_report "$id")"
      else
        report="$(arm_replay_report "$id")"
      fi
      candidate="${SCRATCH}/${id}.${lane}.report.candidate"
      strict_validator_status validate_report "$report"
      if (( STRICT_VALIDATOR_RC == 0 )); then
        TERMINAL_VALIDATED_REPORTS="$((TERMINAL_VALIDATED_REPORTS+1))"
        TERMINAL_LAST_STAGE=evaluator_report_validated
      else
        strict_validator_status validate_report "$candidate"
        if (( STRICT_VALIDATOR_RC == 0 )); then
          TERMINAL_VALIDATED_REPORTS="$((TERMINAL_VALIDATED_REPORTS+1))"
          TERMINAL_LAST_STAGE=evaluator_report_candidate_validated
        fi
      fi
    done
    strict_validator_status validate_arm_complete "$index"
    if (( STRICT_VALIDATOR_RC == 0 )); then
      TERMINAL_COMPLETED_PAIRS="$((TERMINAL_COMPLETED_PAIRS+1))"
      TERMINAL_LAST_STAGE=arm_complete
    else
      break
    fi
  done
  strict_validator_status validate_science_complete
  if (( STRICT_VALIDATOR_RC == 0 )); then
    load_kv_file "$SCIENCE_COMPLETE"
    TERMINAL_SCIENCE_VALID=true
    TERMINAL_LAST_STAGE=science_complete
  fi
  return 0
}

seal_terminal() {
  local worker_exit="$1" failure_stage="$2" failure_reason="$3"
  local candidate actual_candidate_fits actual_head_solves completed_pairs classification
  local scientific_execution_completed=false
  [[ -f "$ATTEMPT" && ! -L "$ATTEMPT" ]] || return 0
  [[ ! -e "$TERMINAL" && ! -L "$TERMINAL" ]] || return 0
  [[ ! -e "$RESULT" && ! -L "$RESULT" ]] || return 0

  collect_terminal_progress
  completed_pairs="$TERMINAL_COMPLETED_PAIRS"
  actual_candidate_fits=0
  actual_head_solves=0
  classification=not_available
  if [[ "$TERMINAL_SCIENCE_VALID" == true ]]; then
    scientific_execution_completed=true
    completed_pairs="$(kv "$SCIENCE_COMPLETE" unique_pairs_completed)"
    actual_candidate_fits="$(kv "$SCIENCE_COMPLETE" analytic_candidate_fit_count)"
    actual_head_solves="$(kv "$SCIENCE_COMPLETE" edge_channel_head_solve_count)"
    classification="$(kv "$SCIENCE_COMPLETE" classification)"
  elif (( TERMINAL_INVOCATIONS_STARTED > 0 )); then
    actual_candidate_fits=not_available
    actual_head_solves=not_available
  fi

  publish_failure_evidence
  RUN_TERMINAL_SEAL_SEQUENCE="$((RUN_TERMINAL_SEAL_SEQUENCE+1))"
  candidate="${SCRATCH}/terminal.invalid.status.candidate.$$.${RUN_TERMINAL_SEAL_SEQUENCE}"
  [[ ! -e "$candidate" && ! -L "$candidate" ]] ||
    fail "terminal receipt candidate already exists"
  {
    echo "schema_id=${PROTOCOL_ID}.terminal.v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_sealed_raw96_affine_re_evaluation_protocol"
    echo "scientific_classification_if_complete=${classification}"
    echo "failure_stage=${failure_stage}"
    echo "failure_reason=${failure_reason}"
    echo "worker_exit_code=${worker_exit}"
    echo "attempt_consumed=true"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "last_validated_lifecycle_stage=${TERMINAL_LAST_STAGE}"
    echo "projection_pairs_validated=${TERMINAL_PROJECTION_PAIRS}"
    echo "projection_complete=${TERMINAL_PROJECTION_COMPLETE}"
    echo "evaluator_invocations_started=${TERMINAL_INVOCATIONS_STARTED}"
    echo "evaluator_invocations_returned=${TERMINAL_INVOCATIONS_RETURNED}"
    echo "validated_evaluator_report_count=${TERMINAL_VALIDATED_REPORTS}"
    echo "validated_unique_pair_completion_count=${completed_pairs}"
    echo "validated_main_replay_parity_count=${completed_pairs}"
    echo "analytic_candidate_fit_count=${actual_candidate_fits}"
    echo "edge_channel_head_solve_count=${actual_head_solves}"
    echo "science_complete_receipt_valid=${TERMINAL_SCIENCE_VALID}"
    echo "science_complete_receipt_sha256=$(hash_or_not_available "$SCIENCE_COMPLETE")"
    echo "scientific_execution_completed=${scientific_execution_completed}"
    echo "scientific_result_available=false"
    echo "worker_log_sha256=$(hash_or_not_available "$WORKER_LOG")"
    echo "rejected_result_sha256=$(hash_or_not_available "$REJECTED_RESULT")"
    echo "rejected_evidence_manifest_sha256=$(sha256 "$REJECTED_EVIDENCE_MANIFEST")"
    echo "capture_execution_count=0"
    echo "encoder_execution_count=0"
    echo "checkpoint_read_count=0"
    echo "representation_model_execution_count=0"
    echo "mdn_model_execution_count=0"
    echo "optimizer_fit_count=0"
    echo "optimizer_step_count=0"
    echo "refit_count=0"
    echo "same_protocol_retry_allowed=false"
    echo "same_protocol_resume_allowed=false"
    echo "benchmark_acceptance_authority=false"
    echo "certified_authorization_eligible=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "maximum_anchor_read_upper_bound=2815"
  } > "$candidate"
  chmod 0444 -- "$candidate"
  require_file "$candidate" 444
  [[ "$(wc -l < "$candidate")" == 43 ]] || fail "terminal receipt key count mismatch"
  load_kv_file "$candidate"
  expect "$candidate" schema_id "${PROTOCOL_ID}.terminal.v1"
  expect "$candidate" status terminal_invalid
  expect "$candidate" protocol_id "$PROTOCOL_ID"
  expect "$candidate" attempt_consumed true
  expect "$candidate" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$candidate" scientific_result_available false
  expect "$candidate" same_protocol_retry_allowed false
  expect "$candidate" same_protocol_resume_allowed false
  expect "$candidate" certified_input_access false
  expect "$candidate" final_holdout_access false
  expect "$candidate" policy_access false
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

stop_and_reap_worker() {
  local signal_name="$1"
  [[ -n "$RUN_TIMEOUT_PID" ]] || return 0
  kill -s "$signal_name" -- "-${RUN_TIMEOUT_PID}" 2>/dev/null ||
    kill -s "$signal_name" -- "$RUN_TIMEOUT_PID" 2>/dev/null || true
  wait "$RUN_TIMEOUT_PID" 2>/dev/null || true
  RUN_TIMEOUT_PID=""
}

run_exit_guard() {
  local rc="$1"
  (( RUN_EXIT_GUARD_ACTIVE == 1 )) || return 0
  RUN_EXIT_GUARD_ACTIVE=0
  trap - EXIT
  trap '' HUP INT TERM QUIT
  stop_and_reap_worker TERM
  cleanup_capability
  if [[ -f "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_terminal "$rc" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON"
  fi
  exit "$rc"
}

run_signal() {
  local signal_name="$1" shell_exit="$2"
  RUN_FAILURE_STAGE=signal
  RUN_FAILURE_REASON="signal_${signal_name}"
  RUN_PENDING_SIGNAL="${signal_name}:${shell_exit}"
  if [[ "$RUN_LAUNCHING" == 1 && -z "$RUN_TIMEOUT_PID" ]]; then return 0; fi
  trap '' HUP INT TERM QUIT
  stop_and_reap_worker TERM
  cleanup_capability
  if [[ -f "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_terminal "$shell_exit" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON"
  fi
  exit "$shell_exit"
}

scan_for_live_protocol_processes() {
  local proc pid command
  for proc in /proc/[0-9]*; do
    pid="${proc##*/}"
    [[ "$pid" == "$$" || "$pid" == "$PPID" || ! -r "${proc}/cmdline" ]] && continue
    command="$(tr '\0' ' ' < "${proc}/cmdline" 2>/dev/null || true)"
    [[ "$command" != *sealed_raw96_edge_channel_affine_re_evaluation* ]] ||
      fail "sealed raw96 affine evaluator or worker remains active: pid=${pid}"
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
  if [[ -f "$WORKER_LOG" && ! -L "$WORKER_LOG" ]]; then
    require_file "$WORKER_LOG" 444 || return 1
    return
  fi
  require_file "$WORKER_LOG_CANDIDATE" 600 || return 1
  publish_once "$WORKER_LOG_CANDIDATE" "$WORKER_LOG" 0444 || return 1
}

run_development() {
  local token capability_identity rc pending_signal pending_exit
  ensure_runtime_and_lock
  open_execution_lock_exclusive
  acquire_authority_locks
  [[ ! -e "$TERMINAL" ]] || fail "protocol is terminally invalid; retry is forbidden"
  if [[ -e "$RESULT" ]]; then
    validate_result
    echo "[clear-signal:sealed-raw96-affine] existing development result verified"
    return
  fi
  if [[ -e "$ATTEMPT" ]]; then
    RUN_EXIT_GUARD_ACTIVE=1
    RUN_FAILURE_STAGE=stale_attempt_recovery
    RUN_FAILURE_REASON=previous_execution_interrupted_after_attempt
    trap 'run_exit_guard "$?"' EXIT
    trap '' HUP INT TERM QUIT
    seal_terminal not_available stale_attempt_recovery previous_execution_interrupted_after_attempt
    RUN_EXIT_GUARD_ACTIVE=0
    trap - EXIT
    fail "the sole attempt was already consumed and is terminal; retry/resume forbidden"
  fi
  [[ -e "$PREP_RECEIPT" && ! -L "$PREP_RECEIPT" ]] ||
    fail "sealed-binary preparation is required before development execution"
  preflight_pre_attempt
  validate_prepare_receipt
  scan_for_live_protocol_processes
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
    fail "scratch is not pristine"
  [[ -z "$(find "$EVIDENCE" -type f -print -quit)" ]] ||
    fail "scientific evidence directory is not pristine"
  [[ ! -e "$WORKER_LOG" && ! -e "$REJECTED_RESULT" &&
     ! -e "$REJECTED_EVIDENCE_MANIFEST" ]] ||
    fail "runtime contains rejected or worker evidence before attempt"

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
  [[ "$token" =~ ^[0-9a-f]{64}$ ]] || fail "failed to generate worker capability"
  create_private_capability "$token"
  capability_identity="$RUN_CAPABILITY_IDENTITY"
  [[ "$capability_identity" =~ ^[0-9]+:[0-9]+$ ]] ||
    fail "invalid private worker capability identity"
  [[ -z "$(jobs -pr)" ]] || fail "pre-existing background job is not allowed"

  set +e
  CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_TOKEN="$token" \
  CLEAR_SIGNAL_SEALED_RAW96_AFFINE_WORKER_IDENTITY="$capability_identity" \
    setsid timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" \
      "${WORKER_TIMEOUT_SECONDS}s" "$RUNNER" --private-worker \
      > "$WORKER_LOG_CANDIDATE" 2>&1 &
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

  trap '' HUP INT TERM QUIT

  if (( rc != 0 )); then
    if [[ -f "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
      RUN_FAILURE_STAGE=evaluator_execution_or_precondition
      RUN_FAILURE_REASON=bounded_worker_nonzero_or_incomplete
      seal_terminal "$rc" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON"
      RUN_EXIT_GUARD_ACTIVE=0
      trap - EXIT HUP INT TERM QUIT
      fail "terminal development attempt failed (${rc}); retry forbidden"
    fi
    if [[ -f "$RESULT" && ! -L "$RESULT" ]]; then
      RUN_EXIT_GUARD_ACTIVE=0
      trap - EXIT HUP INT TERM QUIT
      validate_result
      publish_worker_log || true
      echo "[clear-signal:sealed-raw96-affine] result committed before worker termination"
      return
    fi
    RUN_EXIT_GUARD_ACTIVE=0
    trap - EXIT HUP INT TERM QUIT
    fail "bounded worker failed (${rc}) and lifecycle evidence is missing or corrupt"
  fi

  RUN_FAILURE_STAGE=post_worker_validation
  RUN_FAILURE_REASON=post_worker_validation_failure
  [[ -f "$RESULT" && ! -L "$RESULT" ]] ||
    fail "bounded worker exited zero without the final development result"
  validate_result
  publish_worker_log
  RUN_EXIT_GUARD_ACTIVE=0
  trap - EXIT HUP INT TERM QUIT
  echo "[clear-signal:sealed-raw96-affine] complete: ${RESULT}"
}

plan() {
  preflight_static_authorities
  validate_authority_receipts
  echo "Project Clear Signal - Exhaust the Sealed V2 Raw96 Affine Inventory"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=retrospective_development_only"
  echo "unique_pair_count=${UNIQUE_PAIR_COUNT}"
  echo "logical_arm_count=${LOGICAL_ARM_COUNT}"
  echo "execution_order=all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced,endpoint_scale,time_only,no_tf_alignment"
  echo "canonical_alias=ablation.canonical->all_tokens"
  echo "lane_order=main_then_replay"
  echo "maximum_evaluator_invocations=${MAX_EVALUATOR_INVOCATIONS}"
  echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
  echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
  echo "projection_before_evaluator_call_1=true"
  echo "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
  echo "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
  echo "phase2a_complete_report_key_count=${COMPLETE_REPORT_KEY_COUNT}"
  echo "phase2a_complete_report_sorted_key_sha256=${COMPLETE_REPORT_SORTED_KEY_SHA}"
  echo "within_arm_selection=frozen_six_ridge_validation_comparator"
  echo "selection_tie_tolerance=${TIE_TOLERANCE}"
  echo "strong_stop_after_main_replay_parity=true"
  echo "cross_arm_descriptive_ranking_only_if_no_strong_pass=true"
  echo "capture_execution=false"
  echo "encoder_execution=false"
  echo "checkpoint_read=false"
  echo "representation_model_execution=false"
  echo "optimizer_training=false"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_access=false"
  echo "retry_allowed=false"
  echo "resume_allowed=false"
  echo "prepared=$([[ -e "$PREP_RECEIPT" ]] && echo true || echo false)"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "terminal_invalid=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
}

verify_development() {
  preflight_static_authorities
  require_private_dir "$RUNTIME"
  require_private_dir "$PREP_DIR"
  require_private_dir "$EVIDENCE"
  require_private_dir "$SCRATCH"
  open_execution_lock_shared
  acquire_authority_locks
  [[ -e "$RESULT" && ! -e "$TERMINAL" ]] ||
    fail "unique development result is unavailable"
  validate_result
  scan_for_live_protocol_processes
  echo "[clear-signal:sealed-raw96-affine] development result verified read-only"
}

main() {
  [[ $# == 1 ]] ||
    fail "usage: $0 --plan|--prepare|--run-development|--verify-development"
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
