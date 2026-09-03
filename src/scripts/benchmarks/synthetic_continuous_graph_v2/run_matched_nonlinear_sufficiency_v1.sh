#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C
export LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly SCHEMA_ID="synthetic_v2_matched_nonlinear_sufficiency_development_v1"
readonly RUNTIME_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${SCHEMA_ID}"
readonly RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_v1.sh"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_PREREGISTRATION.md"
readonly PREREG_SHA="cbbf1d837aa741ed157beb2fbab5b01d6c6e004376e865b1f71f2732b46fa348"

readonly CAPTURE_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture.cpp"
readonly CAPTURE_SOURCE_SHA="cb6f02b232887e4619f76b65e4388be57a45ff576f2d81f13fc89c665f747c1c"
readonly CAPTURE_BUILD_SCRIPT="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_raw_nodelift_edge_feature_probe_capture.sh"
readonly CAPTURE_BUILD_SCRIPT_SHA="2ab04a6275b7d4077e7d43496a40c26e841ea59b8821c3419cd7e70e80a63ed7"
readonly COMMON_ARCHIVE="${ROOT}/.build/lib/libcommon.a"
readonly COMMON_ARCHIVE_SHA="853ade11707a8588194eda199e5a742e7363c2d1fd87f43285f3ad89414e06d3"
readonly TORCH_ARCHIVE="${ROOT}/.build/lib/libtorchwrap.a"
readonly TORCH_ARCHIVE_SHA="d9a128191f227a798219c9ee0c2ed8d4c6976916dba8b43eb66c6f25c21b279d"
readonly AFFINE_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp"
readonly AFFINE_SOURCE_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly NONLINEAR_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/matched_nonlinear_sufficiency_probe.cpp"
readonly NONLINEAR_SOURCE_SHA="40191953bbb8c05670e9037fbe0147b233065216bb4923428789b16252ebfca6"
readonly NONLINEAR_BUILD_SCRIPT="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_matched_nonlinear_sufficiency_probe.sh"
readonly NONLINEAR_BUILD_SCRIPT_SHA="8794bc98f860ff3352f72bb8fd974cfa16d7028c151d11b95a3c9d66e4920433"

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
readonly CAPTURE_BIN="${BUILD_DIR}/raw_nodelift_edge_feature_probe_capture"
readonly NONLINEAR_BIN="${BUILD_DIR}/matched_nonlinear_sufficiency_probe"
readonly CAPTURE_BUILD_RECEIPT="${BUILD_DIR}/capture.build.status"
readonly NONLINEAR_BUILD_RECEIPT="${BUILD_DIR}/nonlinear.build.status"
readonly RAW_TRAIN="${RUNTIME_ROOT}/capture/train.probe"
readonly RAW_VALIDATION="${RUNTIME_ROOT}/capture/validation.probe"
readonly RAW_TRAIN_REPORT="${RUNTIME_ROOT}/capture/train.report"
readonly RAW_VALIDATION_REPORT="${RUNTIME_ROOT}/capture/validation.report"
readonly NONLINEAR_REPORT="${RUNTIME_ROOT}/nonlinear/development.report"
readonly ATTEMPT="${RUNTIME_ROOT}/attempt.status"
readonly RESULT="${RUNTIME_ROOT}/development.status"
readonly LOCK="${RUNTIME_ROOT}/.execution.lock"

readonly PROBE_HEADER="record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,base_node_id,quote_node_id,channel_index,target_edge_close_return,feature_count,feature_values"
readonly REPRESENTATION_SCHEMA="kikijyeba.synthetic.representation_edge_feature_probe.v1"
readonly RAW_SCHEMA="kikijyeba.synthetic.raw_nodelift_edge_feature_probe.v1"

fail() { echo "[clear-signal:phase2b] ERROR: $*" >&2; exit 1; }
sha256() { sha256sum -- "$1" | awk '{print $1}'; }

kv() {
  local file="$1" key="$2"
  awk -F= -v key="$key" '$1 == key {n++; v=substr($0,length(key)+2)} END {if(n!=1) exit 2; print v}' "$file" ||
    fail "expected exactly one ${key} in ${file}"
}

expect_kv() {
  local file="$1" key="$2" expected="$3" actual
  actual="$(kv "$file" "$key")"
  [[ "$actual" == "$expected" ]] ||
    fail "${file}: ${key} expected '${expected}', got '${actual}'"
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

require_file_hash() {
  local path="$1" expected="$2"
  require_canonical_path "$path"
  [[ -f "$path" && ! -L "$path" ]] || fail "not a regular file: ${path}"
  [[ "$(sha256 "$path")" == "$expected" ]] || fail "SHA-256 mismatch: ${path}"
}

require_frozen_file() {
  local path="$1" expected="$2" mode uid links
  require_file_hash "$path" "$expected"
  mode="$(stat -c '%a' -- "$path")"; uid="$(stat -c '%u' -- "$path")"; links="$(stat -c '%h' -- "$path")"
  (( (8#$mode & 8#222) == 0 )) || fail "frozen authority is writable: ${path}"
  [[ "$uid" == 0 && "$links" == 1 ]] || fail "frozen authority metadata mismatch: ${path}"
}

reject_forbidden_path() {
  case "$1" in
    */data/raw|*/data/raw/*|*certified*|*final_holdout*|*/data/final/*|*policy_checkpoint*)
      fail "forbidden Phase 2B path: $1" ;;
  esac
}

preflight_authority() {
  local path
  require_file_hash "$PREREG" "$PREREG_SHA"
  require_file_hash "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA"
  require_file_hash "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA"
  require_file_hash "$COMMON_ARCHIVE" "$COMMON_ARCHIVE_SHA"
  require_file_hash "$TORCH_ARCHIVE" "$TORCH_ARCHIVE_SHA"
  require_file_hash "$AFFINE_SOURCE" "$AFFINE_SOURCE_SHA"
  require_file_hash "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA"
  require_file_hash "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA"
  require_frozen_file "$PHASE2A_RECEIPT" "$PHASE2A_RECEIPT_SHA"
  require_frozen_file "$CAPTURE_CONFIG" "$CAPTURE_CONFIG_SHA"
  require_frozen_file "$SOURCE_CLOSURE" "$SOURCE_CLOSURE_SHA"
  require_frozen_file "$CURSOR_ERRATUM" "$CURSOR_ERRATUM_SHA"
  require_frozen_file "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA"
  require_frozen_file "$REPRESENTATION_TRAIN" "$REPRESENTATION_TRAIN_SHA"
  require_frozen_file "$REPRESENTATION_VALIDATION" "$REPRESENTATION_VALIDATION_SHA"
  for path in "$CAPTURE_CONFIG" "$SOURCE_CLOSURE" "$CURSOR_ERRATUM" "$SOURCE_MANIFEST" \
              "$REPRESENTATION_TRAIN" "$REPRESENTATION_VALIDATION"; do
    reject_forbidden_path "$path"
  done
  expect_kv "$PHASE2A_RECEIPT" schema_id synthetic_v2_frozen_encoder_channel_conditioned_affine_development_receipt_v1
  expect_kv "$PHASE2A_RECEIPT" status complete
  expect_kv "$PHASE2A_RECEIPT" diagnostic_authority development_only
  expect_kv "$PHASE2A_RECEIPT" classification edge_channel_affine_sufficiency_not_established
  expect_kv "$PHASE2A_RECEIPT" rung_b_authorized true
  expect_kv "$PHASE2A_RECEIPT" validation_strong_gate_pass false
  expect_kv "$PHASE2A_RECEIPT" certified_input_access false
  expect_kv "$PHASE2A_RECEIPT" final_holdout_access false
  expect_kv "$PHASE2A_RECEIPT" policy_access false
  expect_kv "$SOURCE_MANIFEST" isolated_source_root "$SOURCE_ROOT"
  expect_kv "$SOURCE_MANIFEST" canonical_data_raw_access false
  expect_kv "$SOURCE_MANIFEST" final_holdout_available false
  require_canonical_path "$SOURCE_ROOT"
  [[ -d "$SOURCE_ROOT" && ! -L "$SOURCE_ROOT" ]] || fail "invalid isolated source root"
}

open_lock() {
  local parent
  parent="$(dirname -- "$RUNTIME_ROOT")"; require_canonical_path "$parent"
  [[ -e "$RUNTIME_ROOT" ]] || mkdir -m 0700 -- "$RUNTIME_ROOT"
  require_canonical_path "$RUNTIME_ROOT"
  [[ -d "$RUNTIME_ROOT" && ! -L "$RUNTIME_ROOT" ]] || fail "invalid runtime root"
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then (set -o noclobber; : > "$LOCK") 2>/dev/null || true; fi
  require_canonical_path "$LOCK"
  [[ -f "$LOCK" && ! -L "$LOCK" ]] || fail "invalid lock"
  chmod 0600 -- "$LOCK"
  exec 9<> "$LOCK"
  flock -n 9 || fail "another Phase 2B operation holds the lock"
}

publish_receipt() {
  local candidate="$1" destination="$2"
  chmod 0444 -- "$candidate"
  # A hard link is the atomic no-clobber publication primitive here.  Unlike
  # `mv -n`, it returns failure when the destination already exists.
  ln -- "$candidate" "$destination" || fail "immutable publication failed: ${destination}"
  rm -f -- "$candidate"
}

write_build_receipt() {
  local kind="$1" source="$2" source_sha="$3" script="$4" script_sha="$5" binary="$6" receipt="$7" candidate
  candidate="${SCRATCH}/${kind}.build.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_build_v1"
    echo "status=complete"
    echo "kind=${kind}"
    echo "source_path=${source}"
    echo "source_sha256=${source_sha}"
    echo "build_script_path=${script}"
    echo "build_script_sha256=${script_sha}"
    if [[ "$kind" == capture ]]; then
      echo "common_archive_path=${COMMON_ARCHIVE}"
      echo "common_archive_sha256=${COMMON_ARCHIVE_SHA}"
      echo "torch_archive_path=${TORCH_ARCHIVE}"
      echo "torch_archive_sha256=${TORCH_ARCHIVE_SHA}"
    elif [[ "$kind" == nonlinear ]]; then
      echo "affine_source_path=${AFFINE_SOURCE}"
      echo "affine_source_sha256=${AFFINE_SOURCE_SHA}"
    fi
    echo "binary_path=${binary}"
    echo "binary_sha256=$(sha256 "$binary")"
    echo "build_before_attempt=true"
    echo "scientific_input_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$receipt"
}

verify_build_receipt() {
  local kind="$1" source="$2" source_sha="$3" script="$4" script_sha="$5" binary="$6" receipt="$7"
  require_canonical_path "$receipt"
  [[ -f "$receipt" && ! -L "$receipt" ]] || fail "invalid build receipt"
  (( (8#$(stat -c '%a' -- "$receipt") & 8#222) == 0 )) || fail "writable build receipt"
  expect_kv "$receipt" schema_id synthetic_v2_matched_nonlinear_sufficiency_build_v1
  expect_kv "$receipt" status complete
  expect_kv "$receipt" kind "$kind"
  expect_kv "$receipt" source_path "$source"; expect_kv "$receipt" source_sha256 "$source_sha"
  expect_kv "$receipt" build_script_path "$script"; expect_kv "$receipt" build_script_sha256 "$script_sha"
  if [[ "$kind" == capture ]]; then
    expect_kv "$receipt" common_archive_path "$COMMON_ARCHIVE"
    expect_kv "$receipt" common_archive_sha256 "$COMMON_ARCHIVE_SHA"
    expect_kv "$receipt" torch_archive_path "$TORCH_ARCHIVE"
    expect_kv "$receipt" torch_archive_sha256 "$TORCH_ARCHIVE_SHA"
    require_file_hash "$COMMON_ARCHIVE" "$COMMON_ARCHIVE_SHA"
    require_file_hash "$TORCH_ARCHIVE" "$TORCH_ARCHIVE_SHA"
  elif [[ "$kind" == nonlinear ]]; then
    expect_kv "$receipt" affine_source_path "$AFFINE_SOURCE"
    expect_kv "$receipt" affine_source_sha256 "$AFFINE_SOURCE_SHA"
    require_file_hash "$AFFINE_SOURCE" "$AFFINE_SOURCE_SHA"
  fi
  expect_kv "$receipt" binary_path "$binary"
  expect_kv "$receipt" build_before_attempt true; expect_kv "$receipt" scientific_input_access false
  require_canonical_path "$binary"
  [[ -f "$binary" && -x "$binary" && ! -L "$binary" ]] || fail "invalid frozen binary: ${binary}"
  (( (8#$(stat -c '%a' -- "$binary") & 8#222) == 0 )) || fail "binary is writable: ${binary}"
  [[ "$(sha256 "$binary")" == "$(kv "$receipt" binary_sha256)" ]] || fail "binary/receipt mismatch"
}

prepare() {
  preflight_authority; open_lock
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" ]] || fail "cannot prepare after attempt/result publication"
  mkdir -p -m 0700 -- "$BUILD_DIR" "$SCRATCH"
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch is not pristine"
  if [[ ! -e "$CAPTURE_BUILD_RECEIPT" ]]; then
    [[ ! -e "$CAPTURE_BIN" ]] || fail "unreceipted capture binary exists"
    timeout --signal=TERM --kill-after=10s 900s "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BIN"
    chmod 0555 -- "$CAPTURE_BIN"
    write_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  fi
  if [[ ! -e "$NONLINEAR_BUILD_RECEIPT" ]]; then
    [[ ! -e "$NONLINEAR_BIN" ]] || fail "unreceipted nonlinear binary exists"
    timeout --signal=TERM --kill-after=10s 900s "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BIN"
    chmod 0555 -- "$NONLINEAR_BIN"
    write_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  fi
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch gained an entry"
  echo "[clear-signal:phase2b] frozen builds prepared"
}

validate_capture_report() {
  local report="$1" probe="$2" range="$3" anchors="$4" rows="$5" maximum="$6"
  expect_kv "$report" schema_id synthetic_v2_raw_nodelift_edge_feature_probe_capture_v1
  expect_kv "$report" status complete; expect_kv "$report" diagnostic_phase 2B
  expect_kv "$report" diagnostic_authority development_only; expect_kv "$report" benchmark_acceptance_authority false
  expect_kv "$report" config_path "$CAPTURE_CONFIG"; expect_kv "$report" config_sha256 "$CAPTURE_CONFIG_SHA"
  expect_kv "$report" config_immutable true; expect_kv "$report" anchor_range "$range"
  expect_kv "$report" anchor_count "$anchors"; expect_kv "$report" maximum_anchor_read "$maximum"
  expect_kv "$report" source_order_policy sequential; expect_kv "$report" cursor_contiguous true
  expect_kv "$report" probe_path "$probe"; expect_kv "$report" probe_record_schema "$RAW_SCHEMA"
  expect_kv "$report" probe_header "$PROBE_HEADER"; expect_kv "$report" probe_rows "$rows"
  expect_kv "$report" probe_feature_count 96; expect_kv "$report" feature_layout base_32,quote_32,base_minus_quote_32
  expect_kv "$report" canonical_coordinate_order true; expect_kv "$report" history_right_aligned true
  expect_kv "$report" active_close_mask_count.channel_0 4; expect_kv "$report" active_close_mask_count.channel_1 10
  expect_kv "$report" active_close_mask_count.channel_2 30; expect_kv "$report" active_close_mask_contract_passed true
  expect_kv "$report" masked_close_values_zero true; expect_kv "$report" source_dtype float32
  expect_kv "$report" canonical_target_serialization_dtype float32; expect_kv "$report" target_mask_complete true
  expect_kv "$report" representation_model_constructed false; expect_kv "$report" representation_checkpoint_access false
  expect_kv "$report" representation_execution false; expect_kv "$report" mdn_model_constructed false
  expect_kv "$report" mdn_checkpoint_access false; expect_kv "$report" mdn_execution false
  expect_kv "$report" policy_model_constructed false; expect_kv "$report" policy_checkpoint_access false
  expect_kv "$report" policy_execution false; expect_kv "$report" optimizer_steps 0; expect_kv "$report" checkpoint_written false
}

validate_probe_identity() {
  local representation="$1" raw="$2" expected_rows="$3"
  awk -F, -v rep_schema="$REPRESENTATION_SCHEMA" -v raw_schema="$RAW_SCHEMA" -v header="$PROBE_HEADER" -v expected="$expected_rows" '
    NR==FNR {
      if (FNR==1) {if($0!=header) exit 10; next}
      if($1!=rep_schema || NF!=12) exit 11
      signature[FNR]=$2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10 FS $11
      n=FNR; next
    }
    FNR==1 {if($0!=header) exit 12; next}
    {
      if($1!=raw_schema || NF!=12 || $11!="96") exit 13
      current=$2 FS $3 FS $4 FS $5 FS $6 FS $7 FS $8 FS $9 FS $10 FS $11
      if(!(FNR in signature) || signature[FNR]!=current) exit 14
      m=FNR
    }
    END {if(n!=expected+1 || m!=expected+1) exit 15}
  ' "$representation" "$raw" || fail "raw/representation coordinate or target identity failed"
}

bool_value() { [[ "$1" == true || "$1" == false ]] || fail "invalid boolean: $1"; }
uint_le_three() { [[ "$1" =~ ^[0-3]$ ]] || fail "invalid strong-seed count: $1"; }
finite_number() {
  [[ "$1" =~ ^[-+]?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$ ]] || fail "invalid finite number: $1"
}

validate_nonlinear_report() {
  local report="$1" raw_count rep_count raw_pass rep_pass expected arm seed strong sum fingerprint raw_fingerprint=""
  expect_kv "$report" schema_id "$SCHEMA_ID"; expect_kv "$report" status complete
  expect_kv "$report" diagnostic_phase 2B; expect_kv "$report" diagnostic_authority development_only
  expect_kv "$report" benchmark_acceptance_authority false
  expect_kv "$report" representation_record_schema "$REPRESENTATION_SCHEMA"; expect_kv "$report" raw_record_schema "$RAW_SCHEMA"
  expect_kv "$report" representation_train_input "$REPRESENTATION_TRAIN"
  expect_kv "$report" representation_validation_input "$REPRESENTATION_VALIDATION"
  expect_kv "$report" raw_train_input "$RAW_TRAIN"; expect_kv "$report" raw_validation_input "$RAW_VALIDATION"
  expect_kv "$report" train_probe_rows 22464; expect_kv "$report" validation_probe_rows 2304
  expect_kv "$report" fit_anchor_range '[0,2496)'; expect_kv "$report" validation_anchor_range '[2560,2816)'
  expect_kv "$report" certified_anchor_range not_opened; expect_kv "$report" maximum_anchor_read 2815
  expect_kv "$report" final_holdout_access false; expect_kv "$report" policy_access false
  expect_kv "$report" representation_forward_executed false; expect_kv "$report" checkpoint_written false
  expect_kv "$report" coordinate_columns_exact_identity true; expect_kv "$report" target_columns_exact_identity true
  expect_kv "$report" target_tensor_shared_between_arms true; expect_kv "$report" row_order_exact_identity true
  expect_kv "$report" raw_right_alignment_zero_prefix_verified true; expect_kv "$report" raw_active_lengths 4,10,30
  expect_kv "$report" architecture linear_96_128_gelu_128_128_gelu_128_9
  expect_kv "$report" device cpu; expect_kv "$report" dtype float32; expect_kv "$report" deterministic_algorithms true
  expect_kv "$report" seeds 31,47,73; expect_kv "$report" steps_per_fit 3500; expect_kv "$report" batch_size 64
  expect_kv "$report" paired_initial_parameters_exact true; expect_kv "$report" paired_batch_schedule_exact true
  expect_kv "$report" validation_read_by_trainer false; expect_kv "$report" early_stopping false
  expect_kv "$report" seed_selection false; expect_kv "$report" hyperparameter_search false
  expect_kv "$report" retry false; expect_kv "$report" fits_completed 6
  for seed in 31 47 73; do
    raw_fingerprint="$(kv "$report" "arm.raw_history_96.seed_${seed}.schedule_fingerprint")"
    [[ "$raw_fingerprint" =~ ^[0-9a-f]+$ ]] || fail "invalid batch-schedule fingerprint"
    expect_kv "$report" "arm.representation_raw96.seed_${seed}.schedule_fingerprint" "$raw_fingerprint"
    for arm in raw_history_96 representation_raw96; do
      expect_kv "$report" "arm.${arm}.seed_${seed}.optimizer_steps" 3500
      finite_number "$(kv "$report" "arm.${arm}.seed_${seed}.last_loss")"
      finite_number "$(kv "$report" "arm.${arm}.seed_${seed}.maximum_gradient_norm")"
      strong="$(kv "$report" "arm.${arm}.seed_${seed}.validation_strong_gate_pass")"
      bool_value "$strong"
    done
  done
  raw_count="$(kv "$report" arm.raw_history_96.strong_seed_count)"; rep_count="$(kv "$report" arm.representation_raw96.strong_seed_count)"
  uint_le_three "$raw_count"; uint_le_three "$rep_count"
  for arm in raw_history_96 representation_raw96; do
    sum=0
    for seed in 31 47 73; do
      [[ "$(kv "$report" "arm.${arm}.seed_${seed}.validation_strong_gate_pass")" == true ]] && sum=$((sum + 1))
    done
    [[ "$sum" == "$(kv "$report" "arm.${arm}.strong_seed_count")" ]] || fail "${arm} strong-seed count mismatch"
  done
  raw_pass="$(kv "$report" arm.raw_history_96.pass)"; rep_pass="$(kv "$report" arm.representation_raw96.pass)"
  bool_value "$raw_pass"; bool_value "$rep_pass"
  [[ "$raw_pass" == "$([[ "$raw_count" -ge 2 ]] && echo true || echo false)" ]] || fail "raw pass/count mismatch"
  [[ "$rep_pass" == "$([[ "$rep_count" -ge 2 ]] && echo true || echo false)" ]] || fail "representation pass/count mismatch"
  if [[ "$raw_pass" == true && "$rep_pass" == true ]]; then expected=nonlinear_decodability_established
  elif [[ "$raw_pass" == true ]]; then expected=information_not_established_at_frozen_raw96_interface
  elif [[ "$rep_pass" == true ]]; then expected=representation_decodable_raw_history_control_invalid
  else expected=inconclusive_both_mlp_arms_failed; fi
  expect_kv "$report" classification "$expected"
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_attempt_v1"
    echo "status=consumed"
    echo "attempt_ordinal=1"
    echo "development_only=true"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "capture_source_sha256=${CAPTURE_SOURCE_SHA}"
    echo "capture_build_script_sha256=${CAPTURE_BUILD_SCRIPT_SHA}"
    echo "common_archive_sha256=${COMMON_ARCHIVE_SHA}"
    echo "torch_archive_sha256=${TORCH_ARCHIVE_SHA}"
    echo "nonlinear_source_sha256=${NONLINEAR_SOURCE_SHA}"
    echo "nonlinear_build_script_sha256=${NONLINEAR_BUILD_SCRIPT_SHA}"
    echo "affine_source_sha256=${AFFINE_SOURCE_SHA}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "capture_build_receipt_sha256=$(sha256 "$CAPTURE_BUILD_RECEIPT")"
    echo "nonlinear_build_receipt_sha256=$(sha256 "$NONLINEAR_BUILD_RECEIPT")"
    echo "raw_capture_invocations=2"
    echo "nonlinear_evaluator_invocations=1"
    echo "nonlinear_fit_count=6"
    echo "hard_timeout_seconds=5400"
    echo "retry_allowed=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$ATTEMPT"
}

scan_processes() {
  local p pid cmd
  for p in /proc/[0-9]*; do
    pid="${p##*/}"; [[ "$pid" == "$$" || "$pid" == "$PPID" || ! -r "$p/cmdline" ]] && continue
    cmd="$(tr '\0' ' ' < "$p/cmdline" 2>/dev/null || true)"
    [[ "$cmd" != *raw_nodelift_edge_feature_probe_capture* && "$cmd" != *matched_nonlinear_sufficiency_probe* ]] ||
      fail "Phase 2B process remains active: pid=${pid}"
  done
}

emit_result() {
  local candidate="${SCRATCH}/development.status.$$"
  {
    echo "schema_id=synthetic_v2_matched_nonlinear_sufficiency_development_receipt_v1"
    echo "status=complete"
    echo "benchmark_id=synthetic_continuous_graph_v2"
    echo "diagnostic_phase=2B"
    echo "diagnostic_authority=development_only"
    echo "classification=$(kv "$NONLINEAR_REPORT" classification)"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "phase2a_receipt_path=${PHASE2A_RECEIPT}"
    echo "phase2a_receipt_sha256=${PHASE2A_RECEIPT_SHA}"
    echo "attempt_path=${ATTEMPT}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "capture_source_sha256=${CAPTURE_SOURCE_SHA}"
    echo "capture_build_script_sha256=${CAPTURE_BUILD_SCRIPT_SHA}"
    echo "common_archive_path=${COMMON_ARCHIVE}"
    echo "common_archive_sha256=${COMMON_ARCHIVE_SHA}"
    echo "torch_archive_path=${TORCH_ARCHIVE}"
    echo "torch_archive_sha256=${TORCH_ARCHIVE_SHA}"
    echo "capture_build_receipt_sha256=$(sha256 "$CAPTURE_BUILD_RECEIPT")"
    echo "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    echo "nonlinear_source_sha256=${NONLINEAR_SOURCE_SHA}"
    echo "nonlinear_build_script_sha256=${NONLINEAR_BUILD_SCRIPT_SHA}"
    echo "affine_source_path=${AFFINE_SOURCE}"
    echo "affine_source_sha256=${AFFINE_SOURCE_SHA}"
    echo "nonlinear_build_receipt_sha256=$(sha256 "$NONLINEAR_BUILD_RECEIPT")"
    echo "nonlinear_binary_sha256=$(sha256 "$NONLINEAR_BIN")"
    echo "representation_train_probe_sha256=${REPRESENTATION_TRAIN_SHA}"
    echo "representation_validation_probe_sha256=${REPRESENTATION_VALIDATION_SHA}"
    echo "raw_train_probe_path=${RAW_TRAIN}"
    echo "raw_train_probe_sha256=$(sha256 "$RAW_TRAIN")"
    echo "raw_validation_probe_path=${RAW_VALIDATION}"
    echo "raw_validation_probe_sha256=$(sha256 "$RAW_VALIDATION")"
    echo "raw_train_report_sha256=$(sha256 "$RAW_TRAIN_REPORT")"
    echo "raw_validation_report_sha256=$(sha256 "$RAW_VALIDATION_REPORT")"
    echo "nonlinear_report_path=${NONLINEAR_REPORT}"
    echo "nonlinear_report_sha256=$(sha256 "$NONLINEAR_REPORT")"
    echo "raw_capture_invocations=2"
    echo "nonlinear_evaluator_invocations=1"
    echo "fits_completed=6"
    echo "hard_timeout_seconds=5400"
    echo "coordinate_target_identity_verified=true"
    echo "representation_execution=false"
    echo "mdn_execution=false"
    echo "checkpoint_written=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_access=false"
  } > "$candidate"
  publish_receipt "$candidate" "$RESULT"
}

worker() {
  [[ "${PHASE2B_WORKER_TOKEN:-}" =~ ^[0-9a-f]{64}$ ]] || fail "private worker capability absent"
  [[ -e /proc/$$/fd/9 ]] || fail "worker did not inherit execution lock"
  [[ "$(stat -Lc '%d:%i' -- /proc/$$/fd/9)" == "$(stat -Lc '%d:%i' -- "$LOCK")" ]] ||
    fail "worker execution-lock identity mismatch"
  preflight_authority
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  [[ ! -e "$ATTEMPT" && ! -e "$RESULT" ]] || fail "attempt/result already exists"
  mkdir -m 0700 -- "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$RUNTIME_ROOT/logs"
  emit_attempt
  "$CAPTURE_BIN" --config "$CAPTURE_CONFIG" --output-probe "$RAW_TRAIN" --output-report "$RAW_TRAIN_REPORT" --anchor-index-begin 0 --anchor-index-end 2496 >"$RUNTIME_ROOT/logs/capture.train.log" 2>&1
  validate_capture_report "$RAW_TRAIN_REPORT" "$RAW_TRAIN" '[0,2496)' 2496 22464 2495
  validate_probe_identity "$REPRESENTATION_TRAIN" "$RAW_TRAIN" 22464
  "$CAPTURE_BIN" --config "$CAPTURE_CONFIG" --output-probe "$RAW_VALIDATION" --output-report "$RAW_VALIDATION_REPORT" --anchor-index-begin 2560 --anchor-index-end 2816 >"$RUNTIME_ROOT/logs/capture.validation.log" 2>&1
  validate_capture_report "$RAW_VALIDATION_REPORT" "$RAW_VALIDATION" '[2560,2816)' 256 2304 2815
  validate_probe_identity "$REPRESENTATION_VALIDATION" "$RAW_VALIDATION" 2304
  "$NONLINEAR_BIN" --development-only --representation-train-input "$REPRESENTATION_TRAIN" --representation-validation-input "$REPRESENTATION_VALIDATION" --raw-train-input "$RAW_TRAIN" --raw-validation-input "$RAW_VALIDATION" --output "$NONLINEAR_REPORT" >"$RUNTIME_ROOT/logs/nonlinear.log" 2>&1
  validate_nonlinear_report "$NONLINEAR_REPORT"
  chmod 0444 -- "$RAW_TRAIN" "$RAW_VALIDATION" "$RAW_TRAIN_REPORT" "$RAW_VALIDATION_REPORT" "$NONLINEAR_REPORT" "$RUNTIME_ROOT"/logs/*.log
  emit_result
  chmod 0555 -- "$RUNTIME_ROOT/capture" "$RUNTIME_ROOT/nonlinear" "$RUNTIME_ROOT/logs" "$BUILD_DIR"
}

verify_result() {
  preflight_authority
  require_canonical_path "$RESULT"; [[ -f "$RESULT" && ! -L "$RESULT" ]] || fail "invalid result receipt"
  expect_kv "$RESULT" schema_id synthetic_v2_matched_nonlinear_sufficiency_development_receipt_v1
  expect_kv "$RESULT" status complete; expect_kv "$RESULT" diagnostic_authority development_only
  expect_kv "$RESULT" raw_capture_invocations 2; expect_kv "$RESULT" nonlinear_evaluator_invocations 1
  expect_kv "$RESULT" fits_completed 6; expect_kv "$RESULT" coordinate_target_identity_verified true
  expect_kv "$RESULT" certified_input_access false; expect_kv "$RESULT" final_holdout_access false; expect_kv "$RESULT" policy_access false
  require_file_hash "$ATTEMPT" "$(kv "$RESULT" attempt_sha256)"
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  require_file_hash "$RAW_TRAIN" "$(kv "$RESULT" raw_train_probe_sha256)"
  require_file_hash "$RAW_VALIDATION" "$(kv "$RESULT" raw_validation_probe_sha256)"
  require_file_hash "$RAW_TRAIN_REPORT" "$(kv "$RESULT" raw_train_report_sha256)"
  require_file_hash "$RAW_VALIDATION_REPORT" "$(kv "$RESULT" raw_validation_report_sha256)"
  require_file_hash "$NONLINEAR_REPORT" "$(kv "$RESULT" nonlinear_report_sha256)"
  validate_capture_report "$RAW_TRAIN_REPORT" "$RAW_TRAIN" '[0,2496)' 2496 22464 2495
  validate_capture_report "$RAW_VALIDATION_REPORT" "$RAW_VALIDATION" '[2560,2816)' 256 2304 2815
  validate_probe_identity "$REPRESENTATION_TRAIN" "$RAW_TRAIN" 22464
  validate_probe_identity "$REPRESENTATION_VALIDATION" "$RAW_VALIDATION" 2304
  validate_nonlinear_report "$NONLINEAR_REPORT"
  expect_kv "$RESULT" classification "$(kv "$NONLINEAR_REPORT" classification)"
  scan_processes
}

run_development() {
  preflight_authority; open_lock
  if [[ -e "$RESULT" ]]; then verify_result; echo "[clear-signal:phase2b] existing result verified"; return; fi
  [[ ! -e "$ATTEMPT" ]] || fail "the only Phase 2B attempt is consumed; retry is forbidden"
  verify_build_receipt capture "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" "$CAPTURE_BUILD_SCRIPT" "$CAPTURE_BUILD_SCRIPT_SHA" "$CAPTURE_BIN" "$CAPTURE_BUILD_RECEIPT"
  verify_build_receipt nonlinear "$NONLINEAR_SOURCE" "$NONLINEAR_SOURCE_SHA" "$NONLINEAR_BUILD_SCRIPT" "$NONLINEAR_BUILD_SCRIPT_SHA" "$NONLINEAR_BIN" "$NONLINEAR_BUILD_RECEIPT"
  local path token rc entry name
  for path in capture nonlinear logs; do [[ ! -e "$RUNTIME_ROOT/$path" && ! -L "$RUNTIME_ROOT/$path" ]] || fail "non-pristine preattempt path: $path"; done
  while IFS= read -r entry; do
    name="${entry##*/}"
    case "$name" in .execution.lock|.scratch|build) ;; *) fail "unexpected preattempt runtime entry: ${name}" ;; esac
  done < <(find "$RUNTIME_ROOT" -mindepth 1 -maxdepth 1 -print)
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] || fail "scratch is not pristine"
  token="$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')"
  set +e
  PHASE2B_WORKER_TOKEN="$token" timeout --signal=TERM --kill-after=30s 5400s "$RUNNER" --worker
  rc=$?
  set -e
  scan_processes
  if [[ "$rc" != 0 ]]; then
    [[ ! -e "$ATTEMPT" ]] && fail "worker preflight failed (${rc}); attempt remains unconsumed"
    fail "terminal Phase 2B attempt failed (${rc}); retry is forbidden"
  fi
  verify_result
  echo "[clear-signal:phase2b] complete: ${RESULT}"
}

plan() {
  preflight_authority
  echo "Project Clear Signal Phase 2B — matched nonlinear sufficiency"
  echo "runtime_root=${RUNTIME_ROOT}"
  echo "scope=development_only"
  echo "phase2a_rung_b_authorized=true"
  echo "raw_capture_invocations=2"
  echo "nonlinear_evaluator_invocations=1"
  echo "nonlinear_fit_count=6"
  echo "train_range=[0,2496)"
  echo "validation_range=[2560,2816)"
  echo "maximum_anchor_read=2815"
  echo "hard_timeout_seconds=5400"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_access=false"
  echo "builds_prepared=$([[ -e "$CAPTURE_BUILD_RECEIPT" && -e "$NONLINEAR_BUILD_RECEIPT" ]] && echo true || echo false)"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
}

main() {
  [[ $# == 1 ]] || fail "usage: $0 --plan|--prepare|--run-development|--verify-development"
  case "$1" in
    --plan) plan ;;
    --prepare) prepare ;;
    --run-development) run_development ;;
    --verify-development) [[ -e "$RESULT" ]] || fail "result absent"; verify_result; echo "[clear-signal:phase2b] result verified" ;;
    --worker) worker ;;
    *) fail "unsupported mode: $1" ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then main "$@"; fi
