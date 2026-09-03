#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1"
readonly REPORT_SCHEMA="synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_REPRESENTATION_AFFINE_INJECTION_OPTIMIZER_LOCALIZATION_PREREGISTRATION.md"
readonly PREREG_SHA="5c86fcb55b10e52ab322d271c0117f6184402a7d5234a32e13048237d7056b09"
readonly SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_injection_optimizer_localization_probe.cpp"
readonly SOURCE_SHA="7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b"
readonly BUILD_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_representation_affine_injection_optimizer_localization_probe.sh"
readonly BUILD_WRAPPER_SHA="2e5b39981302d55f8785389c4b01cb6dd4b38036d6d45ca10a798c69567004fd"

readonly TRAIN="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe"
readonly TRAIN_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly VALIDATION="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe"
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
readonly BUILD_DIR="${RUNTIME}/build"
readonly SCRATCH="${RUNTIME}/.scratch"
readonly LOCK="${RUNTIME}/.execution.lock"
readonly BIN="${BUILD_DIR}/frozen_representation_affine_injection_optimizer_localization"
readonly BUILD_RECEIPT="${BUILD_DIR}/build.status"
readonly ATTEMPT="${RUNTIME}/attempt.status"
readonly LOG="${RUNTIME}/evaluator.log"
readonly REPORT="${RUNTIME}/development.report"
readonly REJECTED_REPORT="${RUNTIME}/rejected.development.report"
readonly RESULT="${RUNTIME}/development.status"
readonly TERMINAL="${RUNTIME}/terminal.invalid.status"
readonly BUILD_TIMEOUT_SECONDS=300
readonly TIMEOUT_SECONDS=300
readonly TERM_GRACE_SECONDS=10

ACTIVE=0
STAGE="pre_attempt"
FAILURE_REASON="runner_failure"
EVALUATOR_STARTED=0
EVALUATOR_EXIT="not_started"
LOG_CANDIDATE=""
REPORT_CANDIDATE=""
TIMEOUT_PID=""
TIMEOUT_LAUNCHING=0
TIMEOUT_PREVIOUS_ASYNC_PID=""
PENDING_SIGNAL=""
PENDING_SIGNAL_EXIT=""

fail() { echo "[clear-signal:optimizer-localization] ERROR: $*" >&2; return 1; }
sha256() { sha256sum -- "$1" | awk '{print $1}'; }

require_file() {
  local path="$1" mode="$2" canonical actual_mode uid links
  [[ "$path" == /* && -f "$path" && ! -L "$path" ]] || fail "not a regular absolute file: ${path}"
  canonical="$(readlink -f -- "$path")"
  [[ "$canonical" == "$path" ]] || fail "noncanonical file: ${path}"
  read -r actual_mode uid links < <(stat -c '%a %u %h' -- "$path")
  [[ "$actual_mode" == "$mode" && "$uid" == 0 && "$links" == 1 ]] ||
    fail "invalid metadata for ${path}: ${actual_mode}:${uid}:${links}"
}

require_exact() {
  require_file "$1" "$3"
  [[ "$(sha256 "$1")" == "$2" ]] || fail "SHA-256 mismatch: $1"
}

require_private_dir() {
  local path="$1" mode uid
  [[ -d "$path" && ! -L "$path" && "$(readlink -f -- "$path")" == "$path" ]] ||
    fail "not a canonical private directory: ${path}"
  read -r mode uid < <(stat -c '%a %u' -- "$path")
  [[ "$mode" == 700 && "$uid" == 0 ]] || fail "invalid private directory metadata: ${path}"
}

kv() {
  local file="$1" key="$2" line value="" count=0
  while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ "$line" == "${key}="* ]]; then value="${line#*=}"; count=$((count + 1)); fi
  done < "$file"
  [[ "$count" == 1 ]] || fail "expected exactly one ${key} in ${file}; found ${count}"
  printf '%s' "$value"
}

expect() {
  local actual
  actual="$(kv "$1" "$2")"
  [[ "$actual" == "$3" ]] || fail "$1: $2 expected '$3', got '$actual'"
}

number_test() {
  local value="$1" expression="$2"
  awk -v x="$value" "BEGIN { if (x !~ /^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$/) exit 2; exit !(${expression}); }"
}

close_to() {
  local value="$1" expected="$2" tolerance="$3"
  awk -v x="$value" -v y="$expected" -v t="$tolerance" 'BEGIN {d=x-y; if(d<0)d=-d; exit !(d<=t)}'
}

publish_once() {
  local candidate="$1" destination="$2" mode="$3"
  [[ -f "$candidate" && ! -L "$candidate" && ! -e "$destination" && ! -L "$destination" ]] ||
    fail "unsafe publication: ${candidate} -> ${destination}"
  chmod "$mode" -- "$candidate"
  require_file "$candidate" "$mode"
  mv -T -n -- "$candidate" "$destination"
  [[ ! -e "$candidate" ]] || fail "no-clobber publication failed: ${destination}"
  require_file "$destination" "$mode"
}

preflight_compile() {
  require_file "$RUNNER" 555
  require_exact "$PREREG" "$PREREG_SHA" 444
  [[ "$SOURCE_SHA" != __* && "$BUILD_WRAPPER_SHA" != __* ]] || fail "source hash pins are not finalized"
  require_exact "$SOURCE" "$SOURCE_SHA" 444
  require_exact "$BUILD_WRAPPER" "$BUILD_WRAPPER_SHA" 555
}

preflight_science() {
  preflight_compile
  require_exact "$TRAIN" "$TRAIN_SHA" 444
  require_exact "$VALIDATION" "$VALIDATION_SHA" 444
  require_exact "$PHASE2A_RECEIPT" "$PHASE2A_RECEIPT_SHA" 444
  require_exact "$PHASE2A_MAIN" "$PHASE2A_REPORT_SHA" 444
  require_exact "$PHASE2A_REPLAY" "$PHASE2A_REPORT_SHA" 444
  require_exact "$PHASE2B_RESULT" "$PHASE2B_RESULT_SHA" 444
  require_exact "$PHASE2B_REPORT" "$PHASE2B_REPORT_SHA" 444
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

ensure_dirs_and_lock() {
  local dir
  for dir in "$RUNTIME" "$BUILD_DIR" "$SCRATCH"; do
    if [[ ! -e "$dir" && ! -L "$dir" ]]; then mkdir -m 0700 -- "$dir"; fi
    require_private_dir "$dir"
  done
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || true
    chmod 0600 -- "$LOCK"
  fi
  require_file "$LOCK" 600
  exec 9<> "$LOCK"
  flock -n 9 || fail "protocol is already active"
}

emit_build_receipt() {
  local candidate="${SCRATCH}/build.$$.status"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_build_v1"
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
  } > "$candidate"
  publish_once "$candidate" "$BUILD_RECEIPT" 444
}

validate_build() {
  require_file "$BUILD_RECEIPT" 444
  require_file "$BIN" 555
  expect "$BUILD_RECEIPT" status complete
  expect "$BUILD_RECEIPT" source_sha256 "$SOURCE_SHA"
  expect "$BUILD_RECEIPT" build_wrapper_sha256 "$BUILD_WRAPPER_SHA"
  expect "$BUILD_RECEIPT" binary_path "$BIN"
  expect "$BUILD_RECEIPT" binary_sha256 "$(sha256 "$BIN")"
  expect "$BUILD_RECEIPT" compile_only true
  expect "$BUILD_RECEIPT" build_timeout_seconds "$BUILD_TIMEOUT_SECONDS"
  expect "$BUILD_RECEIPT" term_grace_seconds "$TERM_GRACE_SECONDS"
  expect "$BUILD_RECEIPT" probe_access false
}

prepare_locked() {
  local build_exit=0
  preflight_compile
  if [[ -e "$BUILD_RECEIPT" || -L "$BUILD_RECEIPT" ]]; then validate_build; return; fi
  [[ ! -e "$BIN" && ! -L "$BIN" ]] || fail "unreceipted binary exists"
  if timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" "${BUILD_TIMEOUT_SECONDS}s" \
    "$BUILD_WRAPPER" "$BIN"; then
    build_exit=0
  else
    build_exit=$?
  fi
  [[ "$build_exit" == 0 ]] || fail "compile-only preparation exited ${build_exit}"
  require_file "$BIN" 555
  emit_build_receipt
  validate_build
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.$$.status"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_attempt_v1"
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
    echo "evaluator_invocation_limit=1"
    echo "affine_oracle_grouped_fit_count=1"
    echo "affine_oracle_head_solve_count=9"
    echo "optimizer_fits_completed=1"
    echo "total_train_fit_procedures=2"
    echo "optimizer_step_limit=3500"
    echo "timeout_seconds=${TIMEOUT_SECONDS}"
    echo "build_timeout_seconds=${BUILD_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
    echo "maximum_anchor_read=2815"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$ATTEMPT" 444
}

metric_valid() {
  local file="$1" prefix="$2" expected_count="$3" field value ratio
  expect "$file" "${prefix}.count" "$expected_count"
  expect "$file" "${prefix}.pairwise_rank_count" "$expected_count"
  for field in mae rmse prediction_rms rmse_target_rms_ratio; do
    value="$(kv "$file" "${prefix}.${field}")"; number_test "$value" 'x>=0'
  done
  value="$(kv "$file" "${prefix}.target_rms")"; number_test "$value" 'x>0'
  for field in directional_accuracy pairwise_rank_accuracy best_asset_agreement; do
    value="$(kv "$file" "${prefix}.${field}")"; number_test "$value" 'x>=0 && x<=1'
  done
  value="$(kv "$file" "${prefix}.correlation")"; number_test "$value" 'x>=-1 && x<=1'
  ratio="$(awk -v r="$(kv "$file" "${prefix}.rmse")" -v t="$(kv "$file" "${prefix}.target_rms")" 'BEGIN {printf "%.17g", r/t}')"
  close_to "$(kv "$file" "${prefix}.rmse_target_rms_ratio")" "$ratio" 1e-12
}

validate_oracle_against_phase2a() {
  local file="$1" split channel suffix field report_prefix reference_prefix
  for split in train validation; do
    for channel in aggregate 0 1 2; do
      if [[ "$channel" == aggregate ]]; then suffix=""; else suffix=".channel_${channel}"; fi
      report_prefix="route.float64_oracle.${split}${suffix}"
      reference_prefix="selected.${split}${suffix}"
      for field in count pairwise_rank_count; do
        expect "$file" "${report_prefix}.${field}" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")"
      done
      for field in mae rmse target_rms prediction_rms rmse_target_rms_ratio directional_accuracy pairwise_rank_accuracy best_asset_agreement correlation; do
        close_to "$(kv "$file" "${report_prefix}.${field}")" "$(kv "$PHASE2A_MAIN" "${reference_prefix}.${field}")" 1e-12
      done
    done
  done
}

validate_report_content() {
  local file="$1" route split channel suffix count field value float_pass=false gelu_pass=false
  local recovery=true clear=false expected_class oracle_mse adam_mse aggregate_ratio expected_ratio
  local oracle_head adam_head head_ratio maximum_head_ratio=0 expected_max_head_ratio
  expect "$file" schema_id "$REPORT_SCHEMA"
  expect "$file" status complete
  expect "$file" benchmark_id synthetic_continuous_graph_v2
  expect "$file" diagnostic_phase affine_injection_optimizer_localization
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
  close_to "$(kv "$file" fixed_ridge)" 1e-12 1e-24
  expect "$file" ridge_selection false
  expect "$file" oracle_phase2a_reference_validation external_runner_required
  expect "$file" head_index_formula 'channel*3+edge'
  expect "$file" flat_row_order anchor,edge,channel
  expect "$file" direct_architecture 'Linear(96,9)+gather(channel*3+edge)'
  expect "$file" injected_architecture 'Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)+gather(channel*3+edge)'
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
  expect "$file" gelu_identity 'GELU(z)-GELU(-z)=z'
  expect "$file" gelu_injected_hidden_units_per_layer 18
  expect "$file" gelu_unused_hidden_units_per_layer 110
  expect "$file" zero_optimizer_ladder_optimizer_steps 0
  number_test "$(kv "$file" feature_standardization_clamped_coordinate_count)" 'x>=0 && x==int(x)'
  number_test "$(kv "$file" target_standardization_clamped_coordinate_count)" 'x>=0 && x==int(x)'
  number_test "$(kv "$file" affine_maximum_normalized_residual)" 'x>=0'
  number_test "$(kv "$file" affine_coefficient_l2_norm)" 'x>0'
  for route in float64_oracle direct_float32 paired_gelu_injected direct_linear_adam_seed31; do
    for split in train validation; do
      if [[ "$split" == train ]]; then count=22464; else count=2304; fi
      metric_valid "$file" "route.${route}.${split}" "$count"
      for channel in 0 1 2; do
        if [[ "$split" == train ]]; then count=7488; else count=768; fi
        metric_valid "$file" "route.${route}.${split}.channel_${channel}" "$count"
      done
    done
  done
  validate_oracle_against_phase2a "$file"
  close_to "$(kv "$file" direct_float32_parity_tolerance_standardized_target_units)" 1e-3 1e-15
  close_to "$(kv "$file" paired_gelu_parity_tolerance_standardized_target_units)" 1e-5 1e-17
  for field in \
    delta.float64_oracle_vs_direct_float32.train.standardized_target_units_max_abs \
    delta.float64_oracle_vs_direct_float32.validation.standardized_target_units_max_abs \
    delta.direct_float32_vs_paired_gelu.train.standardized_target_units_max_abs \
    delta.direct_float32_vs_paired_gelu.validation.standardized_target_units_max_abs \
    delta.float64_oracle_vs_direct_float32.train.original_units_max_abs \
    delta.float64_oracle_vs_direct_float32.validation.original_units_max_abs \
    delta.direct_float32_vs_paired_gelu.train.original_units_max_abs \
    delta.direct_float32_vs_paired_gelu.validation.original_units_max_abs \
    delta.float64_oracle_vs_paired_gelu.train.original_units_max_abs \
    delta.float64_oracle_vs_paired_gelu.validation.original_units_max_abs; do
    number_test "$(kv "$file" "$field")" 'x>=0'
  done
  if number_test "$(kv "$file" delta.float64_oracle_vs_direct_float32.train.standardized_target_units_max_abs)" 'x<=0.001' &&
     number_test "$(kv "$file" delta.float64_oracle_vs_direct_float32.validation.standardized_target_units_max_abs)" 'x<=0.001'; then float_pass=true; fi
  if number_test "$(kv "$file" delta.direct_float32_vs_paired_gelu.train.standardized_target_units_max_abs)" 'x<=0.00001' &&
     number_test "$(kv "$file" delta.direct_float32_vs_paired_gelu.validation.standardized_target_units_max_abs)" 'x<=0.00001'; then gelu_pass=true; fi
  expect "$file" direct_float32_parity_pass "$float_pass"
  expect "$file" paired_gelu_parity_pass "$gelu_pass"

  oracle_mse="$(kv "$file" route.float64_oracle.train.standardized_mse)"; number_test "$oracle_mse" 'x>0'
  adam_mse="$(kv "$file" route.direct_linear_adam_seed31.train.standardized_mse)"; number_test "$adam_mse" 'x>0'
  aggregate_ratio="$(kv "$file" direct_linear_adam_to_oracle_train_standardized_mse_ratio)"; number_test "$aggregate_ratio" 'x>=0'
  expected_ratio="$(awk -v a="$adam_mse" -v o="$oracle_mse" 'BEGIN {printf "%.17g", a/o}')"
  close_to "$aggregate_ratio" "$expected_ratio" 1e-12
  number_test "$aggregate_ratio" 'x<=1.05' || recovery=false
  for channel in 0 1 2 3 4 5 6 7 8; do
    oracle_head="$(kv "$file" "route.float64_oracle.train.head_${channel}.standardized_mse")"; number_test "$oracle_head" 'x>0'
    adam_head="$(kv "$file" "route.direct_linear_adam_seed31.train.head_${channel}.standardized_mse")"; number_test "$adam_head" 'x>0'
    head_ratio="$(awk -v a="$adam_head" -v o="$oracle_head" 'BEGIN {printf "%.17g", a/o}')"
    close_to "$(kv "$file" "direct_linear_adam_to_oracle_train.head_${channel}.standardized_mse_ratio")" "$head_ratio" 1e-12
    number_test "$head_ratio" 'x<=1.10' || recovery=false
    number_test "$head_ratio" "x>${maximum_head_ratio}" && maximum_head_ratio="$head_ratio" || true
    close_to "$(kv "$file" "direct_linear_adam.full_train_standardized_mse.head_${channel}.edge_$((channel % 3)).channel_$((channel / 3))")" "$adam_head" 1e-12
  done
  expected_max_head_ratio="$(kv "$file" direct_linear_adam_to_oracle_train_maximum_head_standardized_mse_ratio)"; number_test "$expected_max_head_ratio" 'x>=0'
  close_to "$expected_max_head_ratio" "$maximum_head_ratio" 1e-12
  number_test "$(kv "$file" route.direct_linear_adam_seed31.train.directional_accuracy)" "x>=$(kv "$file" route.float64_oracle.train.directional_accuracy)-0.01" || recovery=false
  number_test "$(kv "$file" route.direct_linear_adam_seed31.train.pairwise_rank_accuracy)" "x>=$(kv "$file" route.float64_oracle.train.pairwise_rank_accuracy)-0.01" || recovery=false
  number_test "$(kv "$file" route.direct_linear_adam_seed31.train.correlation)" "x>=$(kv "$file" route.float64_oracle.train.correlation)-0.01" || recovery=false
  number_test "$(kv "$file" route.direct_linear_adam_seed31.train.rmse_target_rms_ratio)" "x<=$(kv "$file" route.float64_oracle.train.rmse_target_rms_ratio)+0.05" || recovery=false
  if [[ "$recovery" == false ]] && { number_test "$aggregate_ratio" 'x>=1.25' || number_test "$maximum_head_ratio" 'x>=1.50'; }; then clear=true; fi
  expect "$file" direct_linear_adam_recovery_gate_pass "$recovery"
  expect "$file" direct_linear_adam_clear_failure_gate_pass "$clear"
  expect "$file" direct_linear_adam_recovery_gate 'train_aggregate_mse_ratio<=1.05,each_train_head_mse_ratio<=1.10,train_direction>=oracle-0.01,train_rank>=oracle-0.01,train_correlation>=oracle-0.01,train_rmse_ratio<=oracle+0.05'
  expect "$file" direct_linear_adam_clear_failure_gate '!recovery_and_(train_aggregate_mse_ratio>=1.25_or_max_train_head_mse_ratio>=1.50)'
  if [[ "$float_pass" == false ]]; then expected_class="float32_conditioning_failure"
  elif [[ "$gelu_pass" == false ]]; then expected_class="paired_gelu_execution_failure"
  elif [[ "$recovery" == true ]]; then expected_class="deep_parameterization_or_optimization_failure"
  elif [[ "$clear" == true ]]; then expected_class="direct_linear_adam_optimizer_failure"
  else expected_class="optimizer_localization_inconclusive"; fi
  expect "$file" classification "$expected_class"
  expect "$file" seed 31
  expect "$file" affine_oracle_grouped_fit_count 1
  expect "$file" affine_oracle_head_solve_count 9
  expect "$file" optimizer_fits_completed 1
  expect "$file" total_train_fit_procedures 2
  expect "$file" optimizer_steps 3500
  expect "$file" steps_per_fit 3500
  expect "$file" batch_size 64
  expect "$file" optimizer Adam
  expect "$file" batch_sampling mt19937_64_uniform_with_replacement
  expect "$file" early_stopping false
  expect "$file" seed_selection false
  expect "$file" hyperparameter_search false
  expect "$file" retry false
  expect "$file" refit false
  close_to "$(kv "$file" learning_rate)" 0.001 1e-15
  close_to "$(kv "$file" adam_beta1)" 0.9 1e-15
  close_to "$(kv "$file" adam_beta2)" 0.999 1e-15
  close_to "$(kv "$file" adam_epsilon)" 1e-8 1e-20
  close_to "$(kv "$file" weight_decay)" 0 0
  close_to "$(kv "$file" gradient_clip_norm)" 5 0
  expect "$file" batch_schedule_fingerprint f2fa41d284a42d60
  for field in initial_full_train_standardized_mse final_full_train_standardized_mse last_minibatch_loss maximum_preclip_gradient_norm; do
    value="$(kv "$file" "direct_linear_adam.${field}")"; number_test "$value" 'x>=0'
  done
  close_to "$(kv "$file" direct_linear_adam.final_full_train_standardized_mse)" "$adam_mse" 1e-12
  value="$(kv "$file" direct_linear_adam.clipped_step_count)"; number_test "$value" 'x>=0 && x<=3500 && x==int(x)'
  expect "$file" validation_read_by_trainer false
  expect "$file" validation_driven_choice false
  expect "$file" representation_forward_executed false
  expect "$file" checkpoint_written false
  expect "$file" certified_input_access false
  expect "$file" final_holdout_access false
  expect "$file" policy_access false
}

emit_result() {
  local candidate="${SCRATCH}/result.$$.status"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_development_receipt_v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "diagnostic_authority=development_only"
    echo "benchmark_acceptance_authority=false"
    echo "scientific_result_available=true"
    echo "classification=$(kv "$REPORT" classification)"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "build_receipt_sha256=$(sha256 "$BUILD_RECEIPT")"
    echo "evaluator_log_sha256=$(sha256 "$LOG")"
    echo "report_sha256=$(sha256 "$REPORT")"
    echo "capture_invocations=0"
    echo "representation_forward_invocations=0"
    echo "evaluator_invocations=1"
    echo "affine_oracle_grouped_fit_count=1"
    echo "affine_oracle_head_solve_count=9"
    echo "optimizer_fits_completed=1"
    echo "total_train_fit_procedures=2"
    echo "optimizer_steps=3500"
    echo "maximum_anchor_read=2815"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
  } > "$candidate"
  validate_result_content "$candidate"
  publish_once "$candidate" "$RESULT" 444
}

validate_result_content() {
  local file="$1"
  expect "$file" schema_id synthetic_v2_affine_injection_optimizer_localization_development_receipt_v1
  expect "$file" status complete
  expect "$file" protocol_id "$PROTOCOL_ID"
  expect "$file" diagnostic_authority development_only
  expect "$file" benchmark_acceptance_authority false
  expect "$file" scientific_result_available true
  expect "$file" classification "$(kv "$REPORT" classification)"
  expect "$file" runner_sha256 "$(sha256 "$RUNNER")"
  expect "$file" preregistration_sha256 "$PREREG_SHA"
  expect "$file" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$file" build_receipt_sha256 "$(sha256 "$BUILD_RECEIPT")"
  expect "$file" evaluator_log_sha256 "$(sha256 "$LOG")"
  expect "$file" report_sha256 "$(sha256 "$REPORT")"
  expect "$file" capture_invocations 0
  expect "$file" representation_forward_invocations 0
  expect "$file" evaluator_invocations 1
  expect "$file" affine_oracle_grouped_fit_count 1
  expect "$file" affine_oracle_head_solve_count 9
  expect "$file" optimizer_fits_completed 1
  expect "$file" total_train_fit_procedures 2
  expect "$file" optimizer_steps 3500
  expect "$file" maximum_anchor_read 2815
  expect "$file" certified_input_access false
  expect "$file" final_holdout_access false
  expect "$file" policy_access false
  expect "$file" retry_allowed false
  expect "$file" resume_allowed false
}

seal_failure_evidence() {
  if [[ -n "$LOG_CANDIDATE" && -f "$LOG_CANDIDATE" && ! -e "$LOG" ]]; then publish_once "$LOG_CANDIDATE" "$LOG" 444 || true; fi
  if [[ -n "$REPORT_CANDIDATE" && -f "$REPORT_CANDIDATE" && ! -e "$REJECTED_REPORT" ]]; then
    publish_once "$REPORT_CANDIDATE" "$REJECTED_REPORT" 444 || true
  fi
}

emit_terminal() {
  local candidate="${SCRATCH}/terminal.$$.status" attempt_sha="absent" log_sha="absent" rejected_sha="absent"
  [[ -f "$ATTEMPT" ]] && attempt_sha="$(sha256 "$ATTEMPT")"
  [[ -f "$LOG" ]] && log_sha="$(sha256 "$LOG")"
  [[ -f "$REJECTED_REPORT" ]] && rejected_sha="$(sha256 "$REJECTED_REPORT")"
  {
    echo "schema_id=synthetic_v2_affine_injection_optimizer_localization_terminal_v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "failure_stage=${STAGE}"
    echo "failure_reason=${FAILURE_REASON}"
    echo "evaluator_started=${EVALUATOR_STARTED}"
    echo "evaluator_exit_code=${EVALUATOR_EXIT}"
    echo "attempt_sha256=${attempt_sha}"
    echo "evaluator_log_sha256=${log_sha}"
    echo "rejected_report_sha256=${rejected_sha}"
    echo "retry_allowed=false"
    echo "resume_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_once "$candidate" "$TERMINAL" 444
}

on_exit() {
  local code=$?
  trap - EXIT INT TERM HUP QUIT
  set +e
  adopt_timeout_pid_during_launch
  if [[ -n "$TIMEOUT_PID" ]]; then
    FAILURE_REASON="unexpected_exit_with_live_timeout_child"
    stop_and_reap_timeout_child TERM
  fi
  if [[ "$ACTIVE" == 1 && ! -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    seal_failure_evidence
    emit_terminal
  fi
  exit "$code"
}

adopt_timeout_pid_during_launch() {
  local candidate="${!:-}"
  if [[ "$TIMEOUT_LAUNCHING" == 1 && -z "$TIMEOUT_PID" &&
        "$candidate" =~ ^[1-9][0-9]*$ &&
        "$candidate" != "$TIMEOUT_PREVIOUS_ASYNC_PID" ]]; then
    TIMEOUT_PID="$candidate"
  fi
}

stop_and_reap_timeout_child() {
  local signal="$1" child_exit
  [[ -n "$TIMEOUT_PID" ]] || return 0
  kill -s "$signal" "$TIMEOUT_PID" 2>/dev/null || true
  if wait "$TIMEOUT_PID"; then child_exit=0; else child_exit=$?; fi
  EVALUATOR_EXIT="$child_exit"
  TIMEOUT_PID=""
  TIMEOUT_LAUNCHING=0
  PENDING_SIGNAL=""
  PENDING_SIGNAL_EXIT=""
}

on_signal() {
  local signal="$1" shell_exit="$2"
  FAILURE_REASON="signal_${signal}"
  adopt_timeout_pid_during_launch
  if [[ "$TIMEOUT_LAUNCHING" == 1 && -z "$TIMEOUT_PID" ]]; then
    if [[ -z "$PENDING_SIGNAL" ]]; then
      PENDING_SIGNAL="$signal"
      PENDING_SIGNAL_EXIT="$shell_exit"
    fi
    return 0
  fi
  trap - INT TERM HUP QUIT
  stop_and_reap_timeout_child "$signal"
  exit "$shell_exit"
}

run_development() {
  ensure_dirs_and_lock
  [[ ! -e "$ATTEMPT" && ! -L "$ATTEMPT" && ! -e "$RESULT" && ! -L "$RESULT" &&
     ! -e "$TERMINAL" && ! -L "$TERMINAL" && ! -e "$REPORT" && ! -L "$REPORT" ]] ||
    fail "attempt/result/report/terminal already exists; retry is forbidden"
  prepare_locked
  preflight_science
  [[ -z "$(find "$SCRATCH" -mindepth 1 -print -quit)" ]] || fail "scratch is not empty"
  ACTIVE=1
  trap on_exit EXIT
  trap 'on_signal INT 130' INT
  trap 'on_signal TERM 143' TERM
  trap 'on_signal HUP 129' HUP
  trap 'on_signal QUIT 131' QUIT
  STAGE="attempt_publication"
  emit_attempt
  STAGE="evaluator"
  LOG_CANDIDATE="${SCRATCH}/evaluator.$$.log"
  REPORT_CANDIDATE="${SCRATCH}/report.$$.status"
  EVALUATOR_STARTED=1
  [[ -z "$(jobs -pr)" ]] || { FAILURE_REASON="unexpected_prior_background_job"; return 1; }
  TIMEOUT_PREVIOUS_ASYNC_PID="${!:-}"
  TIMEOUT_LAUNCHING=1
  timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" "${TIMEOUT_SECONDS}s" \
    "$BIN" --development-only --train-input "$TRAIN" --validation-input "$VALIDATION" --output "$REPORT_CANDIDATE" \
    > "$LOG_CANDIDATE" 2>&1 &
  TIMEOUT_PID=$!
  [[ "$TIMEOUT_PID" =~ ^[1-9][0-9]*$ ]] || { FAILURE_REASON="invalid_timeout_child_pid"; return 1; }
  TIMEOUT_LAUNCHING=0
  if [[ -n "$PENDING_SIGNAL" ]]; then
    on_signal "$PENDING_SIGNAL" "$PENDING_SIGNAL_EXIT"
  fi
  if wait "$TIMEOUT_PID"; then EVALUATOR_EXIT=0; else EVALUATOR_EXIT=$?; fi
  TIMEOUT_PID=""
  TIMEOUT_PREVIOUS_ASYNC_PID=""
  if [[ "$EVALUATOR_EXIT" != 0 ]]; then FAILURE_REASON="evaluator_exit_${EVALUATOR_EXIT}"; return 1; fi
  STAGE="report_validation"
  validate_report_content "$REPORT_CANDIDATE"
  publish_once "$LOG_CANDIDATE" "$LOG" 444
  LOG_CANDIDATE=""
  publish_once "$REPORT_CANDIDATE" "$REPORT" 444
  REPORT_CANDIDATE=""
  validate_report_content "$REPORT"
  STAGE="result_publication"
  emit_result
  ACTIVE=0
  trap - EXIT INT TERM HUP QUIT
  echo "development_result=${RESULT}"
  echo "classification=$(kv "$RESULT" classification)"
}

validate_success() {
  preflight_science
  require_private_dir "$RUNTIME"; require_private_dir "$BUILD_DIR"; require_private_dir "$SCRATCH"
  require_file "$LOCK" 600
  validate_build
  require_file "$ATTEMPT" 444; require_file "$LOG" 444; require_file "$REPORT" 444; require_file "$RESULT" 444
  expect "$ATTEMPT" status consumed
  expect "$ATTEMPT" attempt_ordinal 1
  expect "$ATTEMPT" evaluator_invocation_limit 1
  expect "$ATTEMPT" affine_oracle_grouped_fit_count 1
  expect "$ATTEMPT" affine_oracle_head_solve_count 9
  expect "$ATTEMPT" optimizer_fits_completed 1
  expect "$ATTEMPT" total_train_fit_procedures 2
  expect "$ATTEMPT" optimizer_step_limit 3500
  expect "$ATTEMPT" build_timeout_seconds "$BUILD_TIMEOUT_SECONDS"
  expect "$ATTEMPT" timeout_seconds "$TIMEOUT_SECONDS"
  expect "$ATTEMPT" term_grace_seconds "$TERM_GRACE_SECONDS"
  validate_report_content "$REPORT"
  validate_result_content "$RESULT"
  [[ ! -e "$TERMINAL" && ! -e "$REJECTED_REPORT" ]] || fail "success conflicts with terminal evidence"
  [[ -z "$(find "$SCRATCH" -mindepth 1 -print -quit)" ]] || fail "scratch is not empty"
}

plan() {
  preflight_compile
  echo "protocol_id=${PROTOCOL_ID}"
  echo "diagnostic_authority=development_only"
  echo "planned_capture_invocations=0"
  echo "planned_representation_forward_invocations=0"
  echo "planned_evaluator_invocations=1"
  echo "planned_affine_oracle_grouped_fit_count=1"
  echo "planned_affine_oracle_head_solve_count=9"
  echo "planned_optimizer_fits_completed=1"
  echo "planned_total_train_fit_procedures=2"
  echo "planned_optimizer_steps=3500"
  echo "build_timeout_seconds=${BUILD_TIMEOUT_SECONDS}"
  echo "evaluator_timeout_seconds=${TIMEOUT_SECONDS}"
  echo "maximum_anchor_read=2815"
  echo "attempt_exists=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_exists=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "terminal_exists=$([[ -e "$TERMINAL" ]] && echo true || echo false)"
}

usage() {
  echo "usage: $(basename "$RUNNER") --plan|--prepare|--run-development|--verify-development" >&2
  exit 2
}

[[ $# == 1 ]] || usage
case "$1" in
  --plan) plan ;;
  --prepare)
    ensure_dirs_and_lock
    [[ ! -e "$ATTEMPT" && ! -e "$RESULT" && ! -e "$TERMINAL" ]] || fail "scientific lifecycle already consumed"
    prepare_locked
    echo "build_receipt=${BUILD_RECEIPT}"
    ;;
  --run-development) run_development ;;
  --verify-development)
    [[ -d "$RUNTIME" && ! -L "$RUNTIME" ]] || fail "runtime root absent"
    if [[ -e "$TERMINAL" ]]; then require_file "$TERMINAL" 444; fail "protocol is terminal-invalid"; fi
    validate_success
    echo "verified_development_result=${RESULT}"
    ;;
  *) usage ;;
esac
