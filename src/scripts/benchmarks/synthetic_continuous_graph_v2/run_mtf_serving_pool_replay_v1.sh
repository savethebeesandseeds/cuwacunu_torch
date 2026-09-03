#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C
export LANG=C
umask 077

readonly SCHEMA_ID="synthetic_v2_mtf_serving_pool_replay_development_v1"
readonly ROOT="/cuwacunu"
readonly RUNTIME_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${SCHEMA_ID}"
readonly RUNNER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_mtf_serving_pool_replay_v1.sh"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MTF_SERVING_POOL_REPLAY_PREREGISTRATION.md"
readonly CAPTURE_SOURCE="${ROOT}/src/main/exec/cuwacunu_mtf_serving_pool_capture.cpp"
readonly CAPTURE_BUILD="${ROOT}/.build/exec/cuwacunu_mtf_serving_pool_capture"
readonly CAPTURE_BUILD_SHA="34a1fbe5daa6c7868696abc3bf772236439d72181fe1a1a90b93d3b169390c59"
readonly CAPTURE_BUILD_RECEIPT="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/MTF_SERVING_POOL_CAPTURE_BUILD_RECEIPT.status"
readonly CAPTURE_BUILD_RECEIPT_SHA="cc4c52d5a8cd6353ee30ba77330a90814eb09ffa8b754a8ae6a8a7bec3ea0df2"
readonly CAPTURE_SOURCE_SHA="8b76023ff0fa64ef0d431d450ef9534b8b020e6901b6978198a71a494c3a9edb"
readonly CAPTURE_MAKEFILE="${ROOT}/src/main/exec/Makefile"
readonly CAPTURE_MAKEFILE_SHA="c2d882120a34e77c1bc7f2251f9c78201dda1751ca1eb787297c9acec386f9af"
readonly CAPTURE_DEPENDENCY_MANIFEST="${ROOT}/.build/obj/.deps/cuwacunu_mtf_serving_pool_capture.d"
readonly CAPTURE_DEPENDENCY_MANIFEST_SHA="798f84a9a26a2af0374d247216ffce854e7e8bf0bd75006282bd1fa56dece587"
readonly CAPTURE_OBJECT="${ROOT}/.build/obj/.objs/cuwacunu_mtf_serving_pool_capture.o"
readonly CAPTURE_OBJECT_SHA="5ac296320b1e458ddf34db5942bbf217a5b38ba60a6d3e48374802b2b0721ce2"
readonly FROZEN_CAPTURE_BIN="${RUNTIME_ROOT}/bin/cuwacunu_mtf_serving_pool_capture"
readonly ATTEMPT="${RUNTIME_ROOT}/attempt.status"
readonly RESULT="${RUNTIME_ROOT}/development.status"
readonly LOCK="${RUNTIME_ROOT}/.execution.lock"

readonly CAPTURE_DEVELOPMENT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/development.status"
readonly CAPTURE_DEVELOPMENT_SHA="fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6"
readonly INPUT_RECEIPT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/inputs.status"
readonly INPUT_RECEIPT_SHA="fedbf63815d5806309ac4f6c469b379c685825e8ec83a3b9bf8250663f6e39b0"
readonly SOURCE_CLOSURE="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/development_source_closure.status"
readonly SOURCE_CLOSURE_SHA="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly CURSOR_ERRATUM="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/cursor_alignment_erratum.status"
readonly CURSOR_ERRATUM_SHA="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly SOURCE_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/source"
readonly ISOLATED_REGISTRY="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/config/ujcamei.source.registry.development_prefix.dsl"
readonly ISOLATED_REGISTRY_SHA="54d87853a1d41facd54c24dc4031c2983e9cce40064a8ac7e793fe5fee77cf5c"
readonly ISOLATED_BASE_CONFIG="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/config/synthetic_benchmark.isolated_development.config"
readonly ISOLATED_BASE_CONFIG_SHA="9d5bb23194c5a227ec91cf5882225a26a4f2b1f3f631c167810bd7f71314d7ab"
readonly SOURCE_MANIFEST="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/source_manifest.status"
readonly SOURCE_MANIFEST_SHA="7cf41d721647579924620c9daf7e38931898ba28a02c71c38cc7cd6e3f6431fa"
readonly CAPTURE_CONFIG="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/synthetic_benchmark.frozen_feature_capture.isolated.config"
readonly CAPTURE_CONFIG_SHA="eeea5620f1b271c0bd4527db6764c8f7b66eef5aced7b72d9d1b28d89443c9b3"
readonly CHECKPOINT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
readonly CHECKPOINT_SHA="70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d"
readonly HIST_TRAIN_PROBE="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe"
readonly HIST_TRAIN_PROBE_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly HIST_VALIDATION_PROBE="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe"
readonly HIST_VALIDATION_PROBE_SHA="8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd"
readonly HIST_TRAIN_MANIFEST="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/job.manifest"
readonly HIST_TRAIN_MANIFEST_SHA="d7dc64ab1d424160a30756bdadb449cb2ad27ce9788fbb184d13fdaf66526b6e"
readonly HIST_VALIDATION_MANIFEST="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/job.manifest"
readonly HIST_VALIDATION_MANIFEST_SHA="0b6d85705e478321ad285f784d09391ca1255664f24624c962d57523d75ed02c"

readonly MTF_DSL="${ROOT}/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl"
readonly MTF_DSL_SHA="68015c25d689227141ef62c94b8d0aa01549787f7454b0396ee4fee5c8aa61ba"
readonly MTF_GRAMMAR="${ROOT}/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl.bnf"
readonly MTF_GRAMMAR_SHA="ff6be58c9e70cafd906ffeac1a068f84fbd77bb9cbb7ac1a26e14e7e4d9e657a"
readonly RETRIEVAL_DSL="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/ujcamei.source.retrieval.channels.dsl"
readonly RETRIEVAL_DSL_SHA="36bcb2d4430f9e18673829bc4945ce04715d0f7749608177b8cd6a519fd58feb"
readonly SPLITS_DSL="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/ujcamei.source.splits.dsl"
readonly SPLITS_DSL_SHA="74bf90e6ef55dac297e8ee36184de2a81ea7827cc7af011c3c2d18815a4938a6"
readonly PROTOCOL_DSL="${ROOT}/src/config/kikijyeba.protocol.cwu_02v.dsl"
readonly PROTOCOL_DSL_SHA="d8b3fd860028c0f074f7e5a326db56284c0c40e6126e00ded0e2ac9a15eb8f1c"
readonly TOPOLOGY_DSL="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/kikijyeba.topology.graph.dsl"
readonly TOPOLOGY_DSL_SHA="6469e3408ae12cd070b4e84a72c9f6fe98170d49313d6e758a194b5852d4f440"
readonly NODELIFT_DSL="${ROOT}/src/config/wikimyei.expression.nodelift.srl.dsl"
readonly NODELIFT_DSL_SHA="41e47be6f6c07f62953396875d8a5c607c03391a5c3778a0e729d634a16bbecc"
readonly MTF_NET="${ROOT}/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.net"
readonly MTF_NET_SHA="df4398835b7eff3496ac8c20e7713b2d3d3a245754916c81b77271c696a08cda"

readonly AFFINE_SOURCE="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/frozen_selection_sources/frozen_representation_affine_probe.cpp"
readonly AFFINE_SOURCE_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly AFFINE_RUNNER="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/frozen_selection_sources/run_frozen_representation_affine_probe_isolated_v2.sh"
readonly AFFINE_RUNNER_SHA="ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173"
readonly AFFINE_BIN="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_affine_development_isolated_v2/bin/frozen_representation_affine_probe"
readonly AFFINE_BIN_SHA="733841623165e1be1dbf76e82264022292b5c16825211696800fd5876cddad3f"
readonly BASELINE_MAIN="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_affine_development_isolated_v2/main/synthetic_v2_frozen_encoder_affine_development_isolated_v2.report"
readonly BASELINE_REPLAY="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_affine_development_isolated_v2/replay/synthetic_v2_frozen_encoder_affine_development_isolated_v2.report"
readonly BASELINE_REPORT_SHA="e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e"

readonly PROBE_HEADER="record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,base_node_id,quote_node_id,channel_index,target_edge_close_return,feature_count,feature_values"
readonly -a ARMS=(all_tokens pool_time_tokens pool_frequency_tokens pool_domain_balanced)
readonly -a ALTERNATE_ARMS=(pool_time_tokens pool_frequency_tokens pool_domain_balanced)

fail() {
  echo "[clear-signal] ERROR: $*" >&2
  exit 1
}

sha256() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local file="$1" key="$2"
  awk -F= -v key="$key" '
    $1 == key { count++; value=substr($0, length(key)+2) }
    END { if (count != 1) exit 2; print value }
  ' "$file" || fail "expected exactly one ${key} in ${file}"
}

expect_kv() {
  local file="$1" key="$2" expected="$3" actual
  actual="$(kv "$file" "$key")"
  [[ "$actual" == "$expected" ]] ||
    fail "${file}: ${key} expected '${expected}', got '${actual}'"
}

require_canonical_path() {
  local path="$1" current="/" part
  [[ "$path" == /* ]] || fail "path is not absolute: ${path}"
  [[ -e "$path" ]] || fail "missing path: ${path}"
  while IFS= read -r part; do
    [[ -n "$part" ]] || continue
    current="${current%/}/${part}"
    [[ ! -L "$current" ]] || fail "symbolic-link component is forbidden: ${current}"
  done < <(printf '%s\n' "${path#/}" | tr '/' '\n')
  [[ "$(realpath -e -- "$path")" == "$path" ]] ||
    fail "path is not canonical: ${path}"
}

require_file_hash() {
  local path="$1" expected="$2"
  require_canonical_path "$path"
  [[ -f "$path" && ! -L "$path" ]] || fail "not a regular file: ${path}"
  [[ "$(sha256 "$path")" == "$expected" ]] || fail "SHA-256 mismatch: ${path}"
}

require_immutable_authority() {
  local path="$1" expected="$2" mode uid links
  require_file_hash "$path" "$expected"
  mode="$(stat -c '%a' -- "$path")"
  uid="$(stat -c '%u' -- "$path")"
  links="$(stat -c '%h' -- "$path")"
  [[ "$mode" == "444" && "$uid" == "0" && "$links" == "1" ]] ||
    fail "authority metadata mismatch for ${path}: mode=${mode} uid=${uid} links=${links}"
}

require_readonly_authority() {
  local path="$1" expected="$2" mode uid links
  require_file_hash "$path" "$expected"
  mode="$(stat -c '%a' -- "$path")"
  uid="$(stat -c '%u' -- "$path")"
  links="$(stat -c '%h' -- "$path")"
  (( (8#$mode & 8#222) == 0 )) || fail "authority is writable: ${path}"
  [[ "$uid" == 0 && "$links" == 1 ]] ||
    fail "authority metadata mismatch for ${path}: mode=${mode} uid=${uid} links=${links}"
}

reject_forbidden_path() {
  local path="$1"
  case "$path" in
    */data/raw|*/data/raw/*|*/data/final|*/data/final/*|*certified*|*final_holdout*|*policy_checkpoint*)
      fail "forbidden path in Phase 1: ${path}" ;;
  esac
}

preflight_operator_sources() {
  local path mode
  for path in "$RUNNER" "$PREREG" "$CAPTURE_SOURCE"; do
    require_canonical_path "$path"
    [[ -f "$path" && ! -L "$path" ]] || fail "invalid operator source: ${path}"
    mode=444
    [[ "$path" == "$RUNNER" ]] && mode=555
    [[ "$(stat -c '%a:%h:%u' -- "$path")" == "${mode}:1:0" ]] ||
      fail "operator source is not frozen: ${path}"
  done
}

validate_source_manifest() {
  local index kind path expected_sha expected_size root count seen="|"
  require_immutable_authority "$ISOLATED_REGISTRY" "$ISOLATED_REGISTRY_SHA"
  require_immutable_authority "$ISOLATED_BASE_CONFIG" "$ISOLATED_BASE_CONFIG_SHA"
  require_immutable_authority "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA"
  expect_kv "$SOURCE_MANIFEST" schema_id synthetic_v2_isolated_development_source_v1.source_manifest.v1
  expect_kv "$SOURCE_MANIFEST" status complete
  expect_kv "$SOURCE_MANIFEST" isolated_source_root "$SOURCE_ROOT"
  expect_kv "$SOURCE_MANIFEST" prefix_source_count 9
  expect_kv "$SOURCE_MANIFEST" mirror_csv_count 9
  expect_kv "$SOURCE_MANIFEST" mirror_cache_count 18
  expect_kv "$SOURCE_MANIFEST" canonical_data_raw_access false
  expect_kv "$SOURCE_MANIFEST" accepted_anchor_count 3261
  expect_kv "$SOURCE_MANIFEST" candidate_anchor_count 3261
  expect_kv "$SOURCE_MANIFEST" maximum_anchor_index 3260
  expect_kv "$SOURCE_MANIFEST" final_holdout_available false
  count="$(find "$SOURCE_ROOT" -type f | wc -l | tr -d ' ')"
  [[ "$count" == 27 ]] || fail "isolated source must contain exactly 27 regular artifacts, got ${count}"
  [[ -z "$(find "$SOURCE_ROOT" \( -type l -o \! -type f -a \! -type d \) -print -quit)" ]] ||
    fail "isolated source contains a symlink or special entry"
  [[ -z "$(find "$SOURCE_ROOT" -perm /222 -print -quit)" ]] || fail "isolated source is writable"
  for index in 00 01 02 03 04 05 06 07 08; do
    for kind in mirror raw_cache normalized_cache; do
      path="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_path")"
      expected_sha="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_sha256")"
      expected_size="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_size_bytes")"
      [[ "$path" == "$SOURCE_ROOT"/* ]] || fail "source artifact escapes isolated root: ${path}"
      require_readonly_authority "$path" "$expected_sha"
      [[ "$(stat -c '%s' -- "$path")" == "$expected_size" ]] || fail "source size mismatch: ${path}"
    done
  done
  for index in 00 01 02 03 04 05 06 07 08; do
    root="$(kv "$SOURCE_MANIFEST" "source.${index}.relative_path")"
    [[ "$seen" != *"|${root}|"* ]] || fail "duplicate source tuple: ${root}"
    seen+="${root}|"
    case "$root" in
      SYN2ALPHASYN2USD/1d/SYN2ALPHASYN2USD-1d-all-years.csv|\
      SYN2ALPHASYN2USD/3d/SYN2ALPHASYN2USD-3d-all-years.csv|\
      SYN2ALPHASYN2USD/1w/SYN2ALPHASYN2USD-1w-all-years.csv|\
      SYN2BETASYN2USD/1d/SYN2BETASYN2USD-1d-all-years.csv|\
      SYN2BETASYN2USD/3d/SYN2BETASYN2USD-3d-all-years.csv|\
      SYN2BETASYN2USD/1w/SYN2BETASYN2USD-1w-all-years.csv|\
      SYN2GAMMASYN2USD/1d/SYN2GAMMASYN2USD-1d-all-years.csv|\
      SYN2GAMMASYN2USD/3d/SYN2GAMMASYN2USD-3d-all-years.csv|\
      SYN2GAMMASYN2USD/1w/SYN2GAMMASYN2USD-1w-all-years.csv) ;;
      *) fail "unexpected source tuple: ${root}" ;;
    esac
  done
}

validate_capture_build_receipt() {
  require_immutable_authority "$CAPTURE_BUILD_RECEIPT" "$CAPTURE_BUILD_RECEIPT_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" schema_id synthetic_v2_mtf_serving_pool_capture_build.v1
  expect_kv "$CAPTURE_BUILD_RECEIPT" status complete
  expect_kv "$CAPTURE_BUILD_RECEIPT" build_environment unnamed_taoist
  expect_kv "$CAPTURE_BUILD_RECEIPT" capture_source_path "$CAPTURE_SOURCE"
  expect_kv "$CAPTURE_BUILD_RECEIPT" capture_source_sha256 "$CAPTURE_SOURCE_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" makefile_path "$CAPTURE_MAKEFILE"
  expect_kv "$CAPTURE_BUILD_RECEIPT" makefile_sha256 "$CAPTURE_MAKEFILE_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" dependency_manifest_path "$CAPTURE_DEPENDENCY_MANIFEST"
  expect_kv "$CAPTURE_BUILD_RECEIPT" dependency_manifest_sha256 "$CAPTURE_DEPENDENCY_MANIFEST_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" object_path "$CAPTURE_OBJECT"
  expect_kv "$CAPTURE_BUILD_RECEIPT" object_sha256 "$CAPTURE_OBJECT_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" capture_binary_path "$CAPTURE_BUILD"
  expect_kv "$CAPTURE_BUILD_RECEIPT" capture_binary_sha256 "$CAPTURE_BUILD_SHA"
  expect_kv "$CAPTURE_BUILD_RECEIPT" targeted_build_exit_code 0
  expect_kv "$CAPTURE_BUILD_RECEIPT" production_cuwacunu_exec_rebuilt false
  expect_kv "$CAPTURE_BUILD_RECEIPT" tests_executed_by_build false
  expect_kv "$CAPTURE_BUILD_RECEIPT" model_executed_by_build false
  require_file_hash "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA"
  require_file_hash "$CAPTURE_MAKEFILE" "$CAPTURE_MAKEFILE_SHA"
  require_file_hash "$CAPTURE_DEPENDENCY_MANIFEST" "$CAPTURE_DEPENDENCY_MANIFEST_SHA"
  require_file_hash "$CAPTURE_OBJECT" "$CAPTURE_OBJECT_SHA"
  require_file_hash "$CAPTURE_BUILD" "$CAPTURE_BUILD_SHA"
  local key path hash
  for key in mtf_header mtf_spec_header inference_launcher_header config_bundle_header; do
    path="$(kv "$CAPTURE_BUILD_RECEIPT" "${key}_path")"
    hash="$(kv "$CAPTURE_BUILD_RECEIPT" "${key}_sha256")"
    require_file_hash "$path" "$hash"
  done
}

preflight_authority() {
  preflight_operator_sources
  require_immutable_authority "$CAPTURE_DEVELOPMENT" "$CAPTURE_DEVELOPMENT_SHA"
  require_immutable_authority "$INPUT_RECEIPT" "$INPUT_RECEIPT_SHA"
  require_immutable_authority "$SOURCE_CLOSURE" "$SOURCE_CLOSURE_SHA"
  require_immutable_authority "$CURSOR_ERRATUM" "$CURSOR_ERRATUM_SHA"
  require_immutable_authority "$CAPTURE_CONFIG" "$CAPTURE_CONFIG_SHA"
  require_immutable_authority "$CHECKPOINT" "$CHECKPOINT_SHA"
  require_immutable_authority "$HIST_TRAIN_PROBE" "$HIST_TRAIN_PROBE_SHA"
  require_immutable_authority "$HIST_VALIDATION_PROBE" "$HIST_VALIDATION_PROBE_SHA"
  require_immutable_authority "$HIST_TRAIN_MANIFEST" "$HIST_TRAIN_MANIFEST_SHA"
  require_immutable_authority "$HIST_VALIDATION_MANIFEST" "$HIST_VALIDATION_MANIFEST_SHA"
  require_immutable_authority "$AFFINE_SOURCE" "$AFFINE_SOURCE_SHA"
  require_immutable_authority "$AFFINE_RUNNER" "$AFFINE_RUNNER_SHA"
  require_file_hash "$AFFINE_BIN" "$AFFINE_BIN_SHA"
  [[ "$(stat -c '%a:%h:%u' -- "$AFFINE_BIN")" == "555:1:0" ]] ||
    fail "affine binary metadata mismatch"
  require_immutable_authority "$BASELINE_MAIN" "$BASELINE_REPORT_SHA"
  require_immutable_authority "$BASELINE_REPLAY" "$BASELINE_REPORT_SHA"
  cmp -s -- "$BASELINE_MAIN" "$BASELINE_REPLAY" || fail "baseline main/replay differ"

  require_immutable_authority "$MTF_DSL" "$MTF_DSL_SHA"
  require_immutable_authority "$MTF_GRAMMAR" "$MTF_GRAMMAR_SHA"
  require_immutable_authority "$RETRIEVAL_DSL" "$RETRIEVAL_DSL_SHA"
  require_immutable_authority "$SPLITS_DSL" "$SPLITS_DSL_SHA"
  require_immutable_authority "$PROTOCOL_DSL" "$PROTOCOL_DSL_SHA"
  require_immutable_authority "$TOPOLOGY_DSL" "$TOPOLOGY_DSL_SHA"
  require_immutable_authority "$NODELIFT_DSL" "$NODELIFT_DSL_SHA"
  require_immutable_authority "$MTF_NET" "$MTF_NET_SHA"
  require_canonical_path "$SOURCE_ROOT"
  [[ -d "$SOURCE_ROOT" && ! -L "$SOURCE_ROOT" ]] || fail "invalid isolated source root"

  expect_kv "$CAPTURE_DEVELOPMENT" schema_id synthetic_v2_frozen_feature_capture_isolated_v2.development.v1
  expect_kv "$CAPTURE_DEVELOPMENT" status complete
  expect_kv "$CAPTURE_DEVELOPMENT" input_receipt_path "$INPUT_RECEIPT"
  expect_kv "$CAPTURE_DEVELOPMENT" input_receipt_sha256 "$INPUT_RECEIPT_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" isolated_source_closure_path "$SOURCE_CLOSURE"
  expect_kv "$CAPTURE_DEVELOPMENT" isolated_source_closure_sha256 "$SOURCE_CLOSURE_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" cursor_alignment_erratum_receipt_path "$CURSOR_ERRATUM"
  expect_kv "$CAPTURE_DEVELOPMENT" cursor_alignment_erratum_receipt_sha256 "$CURSOR_ERRATUM_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" isolated_source_root_path "$SOURCE_ROOT"
  expect_kv "$CAPTURE_DEVELOPMENT" capture_config_path "$CAPTURE_CONFIG"
  expect_kv "$CAPTURE_DEVELOPMENT" capture_config_sha256 "$CAPTURE_CONFIG_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" representation_checkpoint_path "$CHECKPOINT"
  expect_kv "$CAPTURE_DEVELOPMENT" representation_checkpoint_sha256 "$CHECKPOINT_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" train_capture_range '[0,2496)'
  expect_kv "$CAPTURE_DEVELOPMENT" validation_capture_range '[2560,2816)'
  expect_kv "$CAPTURE_DEVELOPMENT" maximum_anchor_read 2815
  expect_kv "$CAPTURE_DEVELOPMENT" train_probe_rows 22464
  expect_kv "$CAPTURE_DEVELOPMENT" validation_probe_rows 2304
  expect_kv "$CAPTURE_DEVELOPMENT" canonical_data_raw_access false
  expect_kv "$CAPTURE_DEVELOPMENT" final_holdout_access false
  expect_kv "$CAPTURE_DEVELOPMENT" policy_access false
  expect_kv "$CAPTURE_DEVELOPMENT" trained_train_manifest_path "$HIST_TRAIN_MANIFEST"
  expect_kv "$CAPTURE_DEVELOPMENT" trained_train_manifest_sha256 "$HIST_TRAIN_MANIFEST_SHA"
  expect_kv "$CAPTURE_DEVELOPMENT" trained_validation_manifest_path "$HIST_VALIDATION_MANIFEST"
  expect_kv "$CAPTURE_DEVELOPMENT" trained_validation_manifest_sha256 "$HIST_VALIDATION_MANIFEST_SHA"
  for path in "$HIST_TRAIN_MANIFEST" "$HIST_VALIDATION_MANIFEST"; do
    expect_kv "$path" component_spawn_fingerprint 5ba58d2de0fb7dcb
    expect_kv "$path" protocol_contract_fingerprint d8a39dbf11f94332
    expect_kv "$path" graph_order_fingerprint 4133db527907a8e4
  done

  expect_kv "$SOURCE_CLOSURE" isolated_source_root_path "$SOURCE_ROOT"
  expect_kv "$SOURCE_CLOSURE" isolated_registry_path "$ISOLATED_REGISTRY"
  expect_kv "$SOURCE_CLOSURE" isolated_registry_sha256 "$ISOLATED_REGISTRY_SHA"
  expect_kv "$SOURCE_CLOSURE" isolated_base_config_path "$ISOLATED_BASE_CONFIG"
  expect_kv "$SOURCE_CLOSURE" isolated_base_config_sha256 "$ISOLATED_BASE_CONFIG_SHA"
  expect_kv "$SOURCE_CLOSURE" source_manifest_path "$SOURCE_MANIFEST"
  expect_kv "$SOURCE_CLOSURE" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$SOURCE_CLOSURE" strict_cache_freshness pass
  expect_kv "$SOURCE_CLOSURE" source_tree_read_only true
  expect_kv "$SOURCE_CLOSURE" config_read_only true
  expect_kv "$SOURCE_CLOSURE" canonical_data_raw_access false
  expect_kv "$SOURCE_CLOSURE" final_holdout_available false
  expect_kv "$SOURCE_CLOSURE" accepted_anchor_count 3261
  expect_kv "$SOURCE_CLOSURE" candidate_anchor_count 3261
  expect_kv "$SOURCE_CLOSURE" maximum_anchor_index 3260
  expect_kv "$SOURCE_CLOSURE" skipped_outside_common_range 0
  expect_kv "$SOURCE_CLOSURE" skipped_missing_edge_coverage 0
  expect_kv "$SOURCE_CLOSURE" skipped_failed_fetch_probe 0
  expect_kv "$SOURCE_CLOSURE" duplicate_anchor_count 0
  expect_kv "$CURSOR_ERRATUM" accepted_anchor_count 3261
  expect_kv "$CURSOR_ERRATUM" candidate_anchor_count 3261
  expect_kv "$CURSOR_ERRATUM" maximum_anchor_index 3260
  expect_kv "$CURSOR_ERRATUM" train_anchor_range '[0,2496)'
  expect_kv "$CURSOR_ERRATUM" validation_anchor_range '[2560,2816)'
  expect_kv "$CURSOR_ERRATUM" certified_development_anchor_range '[2880,3261)'
  expect_kv "$CURSOR_ERRATUM" canonical_data_raw_access false
  expect_kv "$CURSOR_ERRATUM" final_holdout_available false

  validate_source_manifest
  validate_capture_build_receipt

  for path in "$CAPTURE_DEVELOPMENT" "$INPUT_RECEIPT" "$SOURCE_CLOSURE" \
    "$CURSOR_ERRATUM" "$CAPTURE_CONFIG" "$CHECKPOINT" "$HIST_TRAIN_PROBE" \
    "$HIST_VALIDATION_PROBE" "$AFFINE_SOURCE" "$AFFINE_RUNNER" "$AFFINE_BIN"; do
    reject_forbidden_path "$path"
  done
}

validate_probe() {
  local path="$1" begin="$2" end="$3" expected_rows="$4"
  require_canonical_path "$path"
  [[ -f "$path" && ! -L "$path" ]] || fail "invalid probe: ${path}"
  [[ "$(sed -n '1p' "$path")" == "$PROBE_HEADER" ]] || fail "probe header mismatch: ${path}"
  awk -F, -v begin="$begin" -v end="$end" -v rows="$expected_rows" '
    NR == 1 { next }
    {
      if (NF != 12) exit 10
      if ($1 != "kikijyeba.synthetic.representation_edge_feature_probe.v1") exit 11
      ai=$3+0; local=$4+0; edge=$5+0; channel=$9+0
      if ($3 !~ /^[0-9]+$/ || ai < begin || ai >= end) exit 12
      if ($4 !~ /^[0-9]+$/ || local < 0) exit 13
      if ($5 !~ /^[0-9]+$/ || edge < 0 || edge > 2) exit 14
      if ($9 !~ /^[0-9]+$/ || channel < 0 || channel > 2) exit 15
      if ($11 != 96) exit 16
      n=split($12, features, ";"); if (n != 96) exit 17
      if (edge == 0 && ($6 != "SYN2ALPHASYN2USD" || $7 != "SYN2ALPHA")) exit 18
      if (edge == 1 && ($6 != "SYN2BETASYN2USD" || $7 != "SYN2BETA")) exit 19
      if (edge == 2 && ($6 != "SYN2GAMMASYN2USD" || $7 != "SYN2GAMMA")) exit 20
      if ($8 != "SYN2USD") exit 21
      key=ai SUBSEP edge SUBSEP channel
      if (++seen[key] != 1) exit 22
      count[ai]++
      if (!(ai in anchor_key)) anchor_key[ai]=$2
      else if (anchor_key[ai] != $2) exit 23
      total++
    }
    END {
      if (total != rows) exit 30
      previous_key=""
      for (i=begin; i<end; ++i) {
        if (count[i] != 9) exit 31
        if (previous_key != "" && (anchor_key[i]+0) <= (previous_key+0)) exit 32
        previous_key=anchor_key[i]
      }
    }
  ' "$path" || fail "probe integrity validation failed: ${path}"
}

projection_sha() {
  local probe="$1" output="$2"
  tail -n +2 -- "$probe" | cut -d, -f2-10 > "$output"
  sha256 "$output"
}

validate_capture_report() {
  local path="$1" range="$2" anchors="$3" max_anchor="$4" rows="$5" split_dir expected_batches
  split_dir="$(dirname -- "$path")"
  expected_batches=$(( (anchors + 63) / 64 ))
  expect_kv "$path" schema_id synthetic_v2_mtf_serving_pool_capture.v1
  expect_kv "$path" status complete
  expect_kv "$path" config_path "$CAPTURE_CONFIG"
  expect_kv "$path" representation_checkpoint_path "$CHECKPOINT"
  expect_kv "$path" anchor_range "$range"
  expect_kv "$path" anchor_count "$anchors"
  expect_kv "$path" maximum_anchor_read "$max_anchor"
  expect_kv "$path" probe_rows "$rows"
  expect_kv "$path" source_order_policy sequential
  expect_kv "$path" graph_order_fingerprint 4133db527907a8e4
  expect_kv "$path" encoder_batch_passes "$expected_batches"
  expect_kv "$path" encoder_passes_per_anchor 1
  expect_kv "$path" both_domains_required true
  expect_kv "$path" both_domains_valid true
  expect_kv "$path" pool.all_tokens.policy all_tokens
  expect_kv "$path" pool.pool_time_tokens.policy time_only
  expect_kv "$path" pool.pool_frequency_tokens.policy frequency_only
  expect_kv "$path" pool.pool_domain_balanced.policy domain_balanced
  local arm
  for arm in "${ARMS[@]}"; do
    expect_kv "$path" "pool.${arm}.probe_path" "${split_dir}/${arm}.probe"
    expect_kv "$path" "pool.${arm}.probe_rows" "$rows"
  done
  expect_kv "$path" mdn_model_constructed false
  expect_kv "$path" mdn_checkpoint_access false
  expect_kv "$path" mdn_execution false
  expect_kv "$path" policy_config_parsed_as_inert_dependency true
  expect_kv "$path" policy_model_constructed false
  expect_kv "$path" policy_checkpoint_access false
  expect_kv "$path" policy_execution false
  expect_kv "$path" policy_metric_access false
  expect_kv "$path" optimizer_steps 0
  expect_kv "$path" model_state_mutated false
  expect_kv "$path" checkpoint_written false
}

numeric() {
  [[ "$1" =~ ^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$ ]] ||
    fail "not a finite decimal: $1"
  awk -v value="$1" 'BEGIN { exit !((value + 0) == (value + 0)) }' ||
    fail "non-finite number: $1"
}

validate_affine_report() {
  local path="$1" direction rank correlation rmse ratio residual valid_count gate expected_gate
  expect_kv "$path" schema_id synthetic_v2_frozen_encoder_affine_development_v1
  expect_kv "$path" status complete
  expect_kv "$path" probe_kind representation
  expect_kv "$path" classification development_selection_complete
  expect_kv "$path" fit_anchor_range '[0,2496)'
  expect_kv "$path" validation_anchor_range '[2560,2816)'
  expect_kv "$path" maximum_anchor_read 2815
  expect_kv "$path" certified_probe_rows 0
  expect_kv "$path" certified_anchor_range not_opened
  expect_kv "$path" certified_candidates_scored 0
  expect_kv "$path" final_holdout_access false
  expect_kv "$path" policy_access false
  expect_kv "$path" refit_after_selection false
  expect_kv "$path" certified_strong_gate_pass not_evaluated
  expect_kv "$path" certified_partial_gate_pass not_evaluated
  valid_count="$(kv "$path" numerically_valid_candidate_count)"
  [[ "$valid_count" =~ ^[1-9][0-9]*$ ]] || fail "no valid affine candidate: ${path}"
  residual="$(kv "$path" selected_maximum_normalized_residual)"
  direction="$(kv "$path" selected.validation.directional_accuracy)"
  rank="$(kv "$path" selected.validation.pairwise_rank_accuracy)"
  correlation="$(kv "$path" selected.validation.correlation)"
  rmse="$(kv "$path" selected.validation.rmse)"
  ratio="$(kv "$path" selected.validation.rmse_target_rms_ratio)"
  for value in "$residual" "$direction" "$rank" "$correlation" "$rmse" "$ratio" "$(kv "$path" selected_ridge)"; do
    numeric "$value"
  done
  awk -v value="$residual" 'BEGIN { exit !(value <= 1e-7) }' ||
    fail "selected affine residual exceeds 1e-7: ${path}"
  gate="$(kv "$path" validation_strong_gate_pass)"
  [[ "$gate" == true || "$gate" == false ]] || fail "invalid strong gate: ${path}"
  expected_gate="$(awk -v d="$direction" -v r="$rank" -v c="$correlation" -v q="$ratio" \
    'BEGIN { print (d >= 0.95 && r >= 0.95 && c >= 0.95 && q <= 0.25) ? "true" : "false" }')"
  [[ "$gate" == "$expected_gate" ]] || fail "strong gate does not recompute: ${path}"
}

report_is_better() {
  local candidate="$1" incumbent="$2"
  awk -v cd="$(kv "$candidate" selected.validation.directional_accuracy)" \
      -v cr="$(kv "$candidate" selected.validation.pairwise_rank_accuracy)" \
      -v cc="$(kv "$candidate" selected.validation.correlation)" \
      -v ce="$(kv "$candidate" selected.validation.rmse)" \
      -v bd="$(kv "$incumbent" selected.validation.directional_accuracy)" \
      -v br="$(kv "$incumbent" selected.validation.pairwise_rank_accuracy)" \
      -v bc="$(kv "$incumbent" selected.validation.correlation)" \
      -v be="$(kv "$incumbent" selected.validation.rmse)" '
    BEGIN {
      t=1e-12
      if (cd > bd+t) exit 0; if (cd < bd-t) exit 1
      if (cr > br+t) exit 0; if (cr < br-t) exit 1
      if (cc > bc+t) exit 0; if (cc < bc-t) exit 1
      if (ce < be-t) exit 0
      exit 1
    }
  '
}

seal_tree() {
  local path="$1"
  [[ -d "$path" && ! -L "$path" ]] || fail "cannot seal invalid directory: ${path}"
  while IFS= read -r entry; do
    if [[ -d "$entry" && ! -L "$entry" ]]; then
      chmod 0555 -- "$entry"
    elif [[ -f "$entry" && ! -L "$entry" ]]; then
      chmod 0444 -- "$entry"
    else
      fail "special or symlink entry in scientific tree: ${entry}"
    fi
  done < <(find "$path" -depth -mindepth 1 -print)
  chmod 0555 -- "$path"
}

verify_sealed_runtime() {
  local entry mode
  require_canonical_path "$RUNTIME_ROOT"
  [[ "$(stat -c '%a:%u' -- "$RUNTIME_ROOT")" == "555:0" ]] ||
    fail "sealed runtime-root metadata mismatch"
  [[ ! -e "${RUNTIME_ROOT}/.scratch" && ! -L "${RUNTIME_ROOT}/.scratch" ]] ||
    fail "scratch directory remains after completion"
  while IFS= read -r entry; do
    [[ ! -L "$entry" ]] || fail "symlink in sealed runtime: ${entry}"
    if [[ -d "$entry" ]]; then
      [[ "$(stat -c '%a:%u' -- "$entry")" == "555:0" ]] ||
        fail "unsealed directory: ${entry}"
    elif [[ -f "$entry" ]]; then
      mode=444
      [[ "$entry" == "$FROZEN_CAPTURE_BIN" ]] && mode=555
      [[ "$(stat -c '%a:%h:%u' -- "$entry")" == "${mode}:1:0" ]] ||
        fail "unsealed file: ${entry}"
    else
      fail "special entry in sealed runtime: ${entry}"
    fi
  done < <(find "$RUNTIME_ROOT" -mindepth 1 -print)
}

publish_immutable() {
  local candidate="$1" destination="$2"
  [[ -f "$candidate" && ! -L "$candidate" ]] || fail "invalid publication candidate"
  [[ ! -e "$destination" ]] || fail "refusing to overwrite ${destination}"
  chmod 0444 -- "$candidate"
  ln -- "$candidate" "$destination" || fail "atomic publication failed: ${destination}"
  rm -f -- "$candidate"
  [[ "$(stat -c '%a:%h:%u' -- "$destination")" == "444:1:0" ]] ||
    fail "published metadata mismatch: ${destination}"
}

verify_attempt() {
  local expected_sha="${1:-}"
  require_canonical_path "$ATTEMPT"
  [[ "$(stat -c '%a:%h:%u' -- "$ATTEMPT")" == "444:1:0" ]] || fail "attempt metadata mismatch"
  [[ -z "$expected_sha" || "$(sha256 "$ATTEMPT")" == "$expected_sha" ]] || fail "attempt hash mismatch"
  expect_kv "$ATTEMPT" schema_id "${SCHEMA_ID}.attempt.v1"
  expect_kv "$ATTEMPT" status consumed
  expect_kv "$ATTEMPT" attempt_ordinal 1
  expect_kv "$ATTEMPT" published_before_first_model_call true
  expect_kv "$ATTEMPT" train_anchor_range '[0,2496)'
  expect_kv "$ATTEMPT" validation_anchor_range '[2560,2816)'
  expect_kv "$ATTEMPT" maximum_anchor_read 2815
  expect_kv "$ATTEMPT" whole_run_timeout_seconds 5400
  expect_kv "$ATTEMPT" term_to_kill_grace_seconds 30
  expect_kv "$ATTEMPT" capture_development_sha256 "$CAPTURE_DEVELOPMENT_SHA"
  expect_kv "$ATTEMPT" source_closure_sha256 "$SOURCE_CLOSURE_SHA"
  expect_kv "$ATTEMPT" isolated_registry_sha256 "$ISOLATED_REGISTRY_SHA"
  expect_kv "$ATTEMPT" isolated_base_config_sha256 "$ISOLATED_BASE_CONFIG_SHA"
  expect_kv "$ATTEMPT" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$ATTEMPT" cursor_erratum_sha256 "$CURSOR_ERRATUM_SHA"
  expect_kv "$ATTEMPT" capture_config_sha256 "$CAPTURE_CONFIG_SHA"
  expect_kv "$ATTEMPT" representation_checkpoint_sha256 "$CHECKPOINT_SHA"
  expect_kv "$ATTEMPT" historical_train_probe_sha256 "$HIST_TRAIN_PROBE_SHA"
  expect_kv "$ATTEMPT" historical_validation_probe_sha256 "$HIST_VALIDATION_PROBE_SHA"
  expect_kv "$ATTEMPT" historical_train_manifest_sha256 "$HIST_TRAIN_MANIFEST_SHA"
  expect_kv "$ATTEMPT" historical_validation_manifest_sha256 "$HIST_VALIDATION_MANIFEST_SHA"
  expect_kv "$ATTEMPT" historical_component_spawn_fingerprint 5ba58d2de0fb7dcb
  expect_kv "$ATTEMPT" historical_protocol_contract_fingerprint d8a39dbf11f94332
  expect_kv "$ATTEMPT" historical_graph_order_fingerprint 4133db527907a8e4
  expect_kv "$ATTEMPT" mtf_dsl_sha256 "$MTF_DSL_SHA"
  expect_kv "$ATTEMPT" mtf_grammar_sha256 "$MTF_GRAMMAR_SHA"
  expect_kv "$ATTEMPT" capture_build_receipt_sha256 "$CAPTURE_BUILD_RECEIPT_SHA"
  expect_kv "$ATTEMPT" capture_binary_sha256 "$CAPTURE_BUILD_SHA"
  expect_kv "$ATTEMPT" affine_binary_sha256 "$AFFINE_BIN_SHA"
  expect_kv "$ATTEMPT" certified_input_access false
  expect_kv "$ATTEMPT" final_holdout_access false
  expect_kv "$ATTEMPT" canonical_data_raw_access false
}

emit_attempt() {
  local candidate="$1"
  cat > "$candidate" <<EOF
schema_id=${SCHEMA_ID}.attempt.v1
status=consumed
attempt_ordinal=1
published_before_first_model_call=true
runner_path=${RUNNER}
runner_sha256=$(sha256 "$RUNNER")
preregistration_path=${PREREG}
preregistration_sha256=$(sha256 "$PREREG")
capture_source_path=${CAPTURE_SOURCE}
capture_source_sha256=${CAPTURE_SOURCE_SHA}
capture_build_receipt_path=${CAPTURE_BUILD_RECEIPT}
capture_build_receipt_sha256=${CAPTURE_BUILD_RECEIPT_SHA}
capture_binary_path=${FROZEN_CAPTURE_BIN}
capture_binary_sha256=${CAPTURE_BUILD_SHA}
capture_development_path=${CAPTURE_DEVELOPMENT}
capture_development_sha256=${CAPTURE_DEVELOPMENT_SHA}
input_receipt_path=${INPUT_RECEIPT}
input_receipt_sha256=${INPUT_RECEIPT_SHA}
source_closure_path=${SOURCE_CLOSURE}
source_closure_sha256=${SOURCE_CLOSURE_SHA}
isolated_registry_path=${ISOLATED_REGISTRY}
isolated_registry_sha256=${ISOLATED_REGISTRY_SHA}
isolated_base_config_path=${ISOLATED_BASE_CONFIG}
isolated_base_config_sha256=${ISOLATED_BASE_CONFIG_SHA}
source_manifest_path=${SOURCE_MANIFEST}
source_manifest_sha256=${SOURCE_MANIFEST_SHA}
cursor_erratum_path=${CURSOR_ERRATUM}
cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}
capture_config_path=${CAPTURE_CONFIG}
capture_config_sha256=${CAPTURE_CONFIG_SHA}
representation_checkpoint_path=${CHECKPOINT}
representation_checkpoint_sha256=${CHECKPOINT_SHA}
historical_train_probe_path=${HIST_TRAIN_PROBE}
historical_train_probe_sha256=${HIST_TRAIN_PROBE_SHA}
historical_validation_probe_path=${HIST_VALIDATION_PROBE}
historical_validation_probe_sha256=${HIST_VALIDATION_PROBE_SHA}
historical_train_manifest_path=${HIST_TRAIN_MANIFEST}
historical_train_manifest_sha256=${HIST_TRAIN_MANIFEST_SHA}
historical_validation_manifest_path=${HIST_VALIDATION_MANIFEST}
historical_validation_manifest_sha256=${HIST_VALIDATION_MANIFEST_SHA}
historical_component_spawn_fingerprint=5ba58d2de0fb7dcb
historical_protocol_contract_fingerprint=d8a39dbf11f94332
historical_graph_order_fingerprint=4133db527907a8e4
mtf_dsl_path=${MTF_DSL}
mtf_dsl_sha256=${MTF_DSL_SHA}
mtf_grammar_path=${MTF_GRAMMAR}
mtf_grammar_sha256=${MTF_GRAMMAR_SHA}
retrieval_dsl_path=${RETRIEVAL_DSL}
retrieval_dsl_sha256=${RETRIEVAL_DSL_SHA}
splits_dsl_path=${SPLITS_DSL}
splits_dsl_sha256=${SPLITS_DSL_SHA}
protocol_dsl_path=${PROTOCOL_DSL}
protocol_dsl_sha256=${PROTOCOL_DSL_SHA}
topology_dsl_path=${TOPOLOGY_DSL}
topology_dsl_sha256=${TOPOLOGY_DSL_SHA}
nodelift_dsl_path=${NODELIFT_DSL}
nodelift_dsl_sha256=${NODELIFT_DSL_SHA}
mtf_net_path=${MTF_NET}
mtf_net_sha256=${MTF_NET_SHA}
affine_source_path=${AFFINE_SOURCE}
affine_source_sha256=${AFFINE_SOURCE_SHA}
affine_runner_path=${AFFINE_RUNNER}
affine_runner_sha256=${AFFINE_RUNNER_SHA}
affine_binary_path=${AFFINE_BIN}
affine_binary_sha256=${AFFINE_BIN_SHA}
baseline_main_report_path=${BASELINE_MAIN}
baseline_replay_report_path=${BASELINE_REPLAY}
baseline_report_sha256=${BASELINE_REPORT_SHA}
train_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
train_probe_rows=22464
validation_probe_rows=2304
maximum_anchor_read=2815
pool_order=all_tokens,pool_time_tokens,pool_frequency_tokens,pool_domain_balanced
pool_domain_balanced_definition=(time_mean+frequency_mean)/2
checkpoint_load_count=2
encoder_passes_per_anchor=1
whole_run_timeout_seconds=5400
term_to_kill_grace_seconds=30
automatic_retry=false
mdn_model_constructed=false
mdn_checkpoint_access=false
mdn_execution=false
policy_model_constructed=false
policy_checkpoint_access=false
policy_execution=false
canonical_data_raw_access=false
certified_input_access=false
final_holdout_access=false
EOF
}

run_capture() {
  local split="$1" begin="$2" end="$3"
  local out="${RUNTIME_ROOT}/capture/${split}" log="${RUNTIME_ROOT}/logs/capture_${split}.log"
  [[ ! -e "$out" && ! -e "$log" ]] || fail "capture output already exists: ${split}"
  "$FROZEN_CAPTURE_BIN" \
    --config "$CAPTURE_CONFIG" \
    --input-representation-checkpoint "$CHECKPOINT" \
    --output-dir "$out" \
    --anchor-index-begin "$begin" \
    --anchor-index-end "$end" > "$log" 2>&1
}

run_affine() {
  local arm="$1" lane="$2"
  local train="${RUNTIME_ROOT}/capture/train/${arm}.probe"
  local validation="${RUNTIME_ROOT}/capture/validation/${arm}.probe"
  local dir="${RUNTIME_ROOT}/affine/${arm}/${lane}"
  local report="${dir}/development.report" log="${dir}/stdout.log"
  mkdir -p -- "$dir"
  [[ ! -e "$report" && ! -e "$log" ]] || fail "affine output already exists: ${arm}/${lane}"
  OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 "$AFFINE_BIN" \
    --probe-kind representation \
    --development-only \
    --train-input "$train" \
    --validation-input "$validation" \
    --output "$report" > "$log" 2>&1
  validate_affine_report "$report"
}

scan_for_background_processes() {
  local proc pid cmd target fd found=0 uncertain=0 ancestors=" $$ " current="$$" parent
  while [[ "$current" =~ ^[0-9]+$ && "$current" -gt 1 && -r "/proc/${current}/stat" ]]; do
    parent="$(awk '{print $4}' "/proc/${current}/stat" 2>/dev/null || true)"
    [[ "$parent" =~ ^[0-9]+$ ]] || break
    ancestors+="${parent} "
    current="$parent"
  done
  for proc in /proc/[0-9]*; do
    pid="${proc##*/}"
    [[ "$ancestors" == *" ${pid} "* ]] && continue
    if [[ ! -r "${proc}/cmdline" ]]; then
      [[ -d "$proc" ]] && uncertain=$((uncertain + 1))
      continue
    fi
    cmd="$(tr '\0' ' ' < "${proc}/cmdline" 2>/dev/null || true)"
    if [[ "$cmd" == *"${FROZEN_CAPTURE_BIN}"* ||
          "$cmd" == *"${RUNTIME_ROOT}/capture/"* ||
          "$cmd" == *"${RUNTIME_ROOT}/affine/"* ]]; then
      echo "[clear-signal] lingering process ${pid}: ${cmd}" >&2
      found=$((found + 1))
    fi
    if [[ -d "${proc}/fd" ]]; then
      for fd in "${proc}/fd"/*; do
        [[ -e "$fd" || -L "$fd" ]] || continue
        target="$(readlink -- "$fd" 2>/dev/null || true)"
        if [[ "$target" == "$RUNTIME_ROOT" || "$target" == "$RUNTIME_ROOT"/* ]]; then
          echo "[clear-signal] process ${pid} retains runtime FD: ${target}" >&2
          found=$((found + 1))
        fi
      done
    elif [[ -d "$proc" ]]; then
      uncertain=$((uncertain + 1))
    fi
  done
  [[ "$uncertain" == 0 ]] || fail "process-reference scan was incomplete for ${uncertain} processes"
  [[ "$found" == 0 ]] || fail "scientific child process remains"
}

assert_inherited_execution_lock() {
  local capability target parent_pid parent_cmd grandparent_pid grandparent_cmd
  [[ "${CLEAR_SIGNAL_WORKER_TOKEN:-}" =~ ^[0-9a-f]{64}$ ]] ||
    fail "private worker capability is absent"
  [[ -e "/proc/$$/fd/8" && -f "/proc/$$/fd/8" ]] ||
    fail "private worker did not inherit capability FD 8"
  target="$(readlink -- "/proc/$$/fd/8")"
  [[ "$target" == "${RUNTIME_ROOT}/.scratch/worker.capability."*" (deleted)" ]] ||
    fail "private worker capability FD has unexpected provenance"
  IFS= read -r capability <&8 || fail "private worker capability FD is unreadable"
  [[ "$capability" == "$CLEAR_SIGNAL_WORKER_TOKEN" ]] ||
    fail "private worker capability mismatch"
  parent_pid="$PPID"
  [[ -r "/proc/${parent_pid}/cmdline" && -r "/proc/${parent_pid}/stat" ]] ||
    fail "private worker cannot inspect its timeout parent"
  parent_cmd="$(tr '\0' ' ' < "/proc/${parent_pid}/cmdline")"
  [[ "$parent_cmd" == *"timeout --signal=TERM --kill-after=30s 5400s ${RUNNER} --worker"* ]] ||
    fail "private worker parent is not the fixed GNU timeout supervisor"
  grandparent_pid="$(awk '{print $4}' "/proc/${parent_pid}/stat")"
  [[ "$grandparent_pid" =~ ^[0-9]+$ && -r "/proc/${grandparent_pid}/cmdline" ]] ||
    fail "private worker cannot inspect its operator parent"
  grandparent_cmd="$(tr '\0' ' ' < "/proc/${grandparent_pid}/cmdline")"
  [[ "$grandparent_cmd" == *"run_mtf_serving_pool_replay_v1.sh --run-development"* ]] ||
    fail "private worker was not launched by --run-development"
  [[ -e "/proc/$$/fd/9" ]] || fail "private worker did not inherit lock FD 9"
  [[ "$(readlink -e -- "/proc/$$/fd/9")" == "$LOCK" ]] ||
    fail "private worker lock FD does not resolve to the execution lock"
  [[ "$(stat -Lc '%d:%i' -- "/proc/$$/fd/9")" == "$(stat -Lc '%d:%i' -- "$LOCK")" ]] ||
    fail "private worker lock identity mismatch"
  flock -n 9 || fail "private worker does not own the execution lock"
}

worker() {
  assert_inherited_execution_lock
  [[ ! -e "$RESULT" && ! -L "$RESULT" ]] || fail "private worker refuses an existing result"
  [[ ! -e "$ATTEMPT" && ! -L "$ATTEMPT" ]] || fail "private worker refuses a consumed attempt"
  for path in capture affine logs projections; do
    [[ ! -e "${RUNTIME_ROOT}/${path}" && ! -L "${RUNTIME_ROOT}/${path}" ]] ||
      fail "private worker refuses pre-existing ${path} artifacts"
  done
  [[ -z "$(find "${RUNTIME_ROOT}/.scratch" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
    fail "private worker refuses a nonempty scratch directory"

  preflight_authority
  local attempt_candidate="${RUNTIME_ROOT}/.scratch/attempt.$$" attempt_sha
  emit_attempt "$attempt_candidate"
  publish_immutable "$attempt_candidate" "$ATTEMPT"
  attempt_sha="$(sha256 "$ATTEMPT")"
  verify_attempt "$attempt_sha"
  require_file_hash "$FROZEN_CAPTURE_BIN" "$(kv "$ATTEMPT" capture_binary_sha256)"

  mkdir -p -- "${RUNTIME_ROOT}/capture" "${RUNTIME_ROOT}/affine" "${RUNTIME_ROOT}/logs" "${RUNTIME_ROOT}/projections"
  run_capture train 0 2496
  run_capture validation 2560 2816

  validate_capture_report "${RUNTIME_ROOT}/capture/train/capture.report" '[0,2496)' 2496 2495 22464
  validate_capture_report "${RUNTIME_ROOT}/capture/validation/capture.report" '[2560,2816)' 256 2815 2304

  local split begin end rows arm reference projection hash
  for split in train validation; do
    if [[ "$split" == train ]]; then
      begin=0; end=2496; rows=22464; reference=""
    else
      begin=2560; end=2816; rows=2304; reference=""
    fi
    for arm in "${ARMS[@]}"; do
      validate_probe "${RUNTIME_ROOT}/capture/${split}/${arm}.probe" "$begin" "$end" "$rows"
      projection="${RUNTIME_ROOT}/projections/${split}.${arm}.coordinates_targets.csv"
      hash="$(projection_sha "${RUNTIME_ROOT}/capture/${split}/${arm}.probe" "$projection")"
      if [[ -z "$reference" ]]; then
        reference="$hash"
      elif [[ "$hash" != "$reference" ]]; then
        fail "coordinate/target projection differs for ${split}/${arm}"
      fi
    done
  done

  [[ "$(sha256 "${RUNTIME_ROOT}/capture/train/all_tokens.probe")" == "$HIST_TRAIN_PROBE_SHA" ]] ||
    fail "new all_tokens train probe is not byte-identical to historical authority"
  [[ "$(sha256 "${RUNTIME_ROOT}/capture/validation/all_tokens.probe")" == "$HIST_VALIDATION_PROBE_SHA" ]] ||
    fail "new all_tokens validation probe is not byte-identical to historical authority"

  validate_affine_report "$BASELINE_MAIN"
  validate_affine_report "$BASELINE_REPLAY"
  for arm in "${ALTERNATE_ARMS[@]}"; do
    run_affine "$arm" main
    run_affine "$arm" replay
    cmp -s -- "${RUNTIME_ROOT}/affine/${arm}/main/development.report" \
      "${RUNTIME_ROOT}/affine/${arm}/replay/development.report" ||
      fail "affine main/replay mismatch: ${arm}"
  done

  scan_for_background_processes

  seal_tree "${RUNTIME_ROOT}/capture"
  seal_tree "${RUNTIME_ROOT}/affine"
  seal_tree "${RUNTIME_ROOT}/logs"
  seal_tree "${RUNTIME_ROOT}/projections"
  chmod 0555 -- "${RUNTIME_ROOT}/bin"

  local result_candidate="${RUNTIME_ROOT}/.scratch/development.$$"
  emit_result "$result_candidate"
  publish_immutable "$result_candidate" "$RESULT"
  rmdir -- "${RUNTIME_ROOT}/.scratch"
  chmod 0444 -- "$LOCK"
  chmod 0555 -- "$RUNTIME_ROOT"
  verify_result
}

arm_report_path() {
  local arm="$1" lane="$2"
  if [[ "$arm" == all_tokens ]]; then
    [[ "$lane" == main ]] && printf '%s\n' "$BASELINE_MAIN" || printf '%s\n' "$BASELINE_REPLAY"
  else
    printf '%s\n' "${RUNTIME_ROOT}/affine/${arm}/${lane}/development.report"
  fi
}

emit_result() {
  local candidate="$1" attempt_sha best_arm=all_tokens best_report="$BASELINE_MAIN"
  local arm lane main replay gate alternate_strong=false classification
  attempt_sha="$(sha256 "$ATTEMPT")"
  for arm in "${ALTERNATE_ARMS[@]}"; do
    main="$(arm_report_path "$arm" main)"
    if report_is_better "$main" "$best_report"; then
      best_arm="$arm"
      best_report="$main"
    fi
    [[ "$(kv "$main" validation_strong_gate_pass)" == true ]] && alternate_strong=true
  done
  if [[ "$(kv "$BASELINE_MAIN" validation_strong_gate_pass)" == false && "$alternate_strong" == true ]]; then
    classification=development_serving_pool_sufficiency_candidate
  else
    classification=serving_pool_sufficiency_not_established
  fi

  cat > "$candidate" <<EOF
schema_id=${SCHEMA_ID}.development.v1
status=complete
classification=${classification}
scientific_scope=development_only
attempt_path=${ATTEMPT}
attempt_sha256=${attempt_sha}
runner_path=${RUNNER}
runner_sha256=$(sha256 "$RUNNER")
preregistration_path=${PREREG}
preregistration_sha256=$(sha256 "$PREREG")
capture_source_path=${CAPTURE_SOURCE}
capture_source_sha256=${CAPTURE_SOURCE_SHA}
capture_build_receipt_path=${CAPTURE_BUILD_RECEIPT}
capture_build_receipt_sha256=${CAPTURE_BUILD_RECEIPT_SHA}
capture_binary_path=${FROZEN_CAPTURE_BIN}
capture_binary_sha256=${CAPTURE_BUILD_SHA}
capture_development_path=${CAPTURE_DEVELOPMENT}
capture_development_sha256=${CAPTURE_DEVELOPMENT_SHA}
input_receipt_path=${INPUT_RECEIPT}
input_receipt_sha256=${INPUT_RECEIPT_SHA}
source_closure_path=${SOURCE_CLOSURE}
source_closure_sha256=${SOURCE_CLOSURE_SHA}
isolated_registry_path=${ISOLATED_REGISTRY}
isolated_registry_sha256=${ISOLATED_REGISTRY_SHA}
isolated_base_config_path=${ISOLATED_BASE_CONFIG}
isolated_base_config_sha256=${ISOLATED_BASE_CONFIG_SHA}
source_manifest_path=${SOURCE_MANIFEST}
source_manifest_sha256=${SOURCE_MANIFEST_SHA}
cursor_erratum_path=${CURSOR_ERRATUM}
cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}
capture_config_path=${CAPTURE_CONFIG}
capture_config_sha256=${CAPTURE_CONFIG_SHA}
representation_checkpoint_path=${CHECKPOINT}
representation_checkpoint_sha256=${CHECKPOINT_SHA}
historical_train_probe_path=${HIST_TRAIN_PROBE}
historical_train_probe_sha256=${HIST_TRAIN_PROBE_SHA}
historical_validation_probe_path=${HIST_VALIDATION_PROBE}
historical_validation_probe_sha256=${HIST_VALIDATION_PROBE_SHA}
historical_train_manifest_path=${HIST_TRAIN_MANIFEST}
historical_train_manifest_sha256=${HIST_TRAIN_MANIFEST_SHA}
historical_validation_manifest_path=${HIST_VALIDATION_MANIFEST}
historical_validation_manifest_sha256=${HIST_VALIDATION_MANIFEST_SHA}
historical_component_spawn_fingerprint=5ba58d2de0fb7dcb
historical_protocol_contract_fingerprint=d8a39dbf11f94332
historical_graph_order_fingerprint=4133db527907a8e4
mtf_dsl_path=${MTF_DSL}
mtf_dsl_sha256=${MTF_DSL_SHA}
mtf_grammar_path=${MTF_GRAMMAR}
mtf_grammar_sha256=${MTF_GRAMMAR_SHA}
retrieval_dsl_path=${RETRIEVAL_DSL}
retrieval_dsl_sha256=${RETRIEVAL_DSL_SHA}
splits_dsl_path=${SPLITS_DSL}
splits_dsl_sha256=${SPLITS_DSL_SHA}
protocol_dsl_path=${PROTOCOL_DSL}
protocol_dsl_sha256=${PROTOCOL_DSL_SHA}
topology_dsl_path=${TOPOLOGY_DSL}
topology_dsl_sha256=${TOPOLOGY_DSL_SHA}
nodelift_dsl_path=${NODELIFT_DSL}
nodelift_dsl_sha256=${NODELIFT_DSL_SHA}
mtf_net_path=${MTF_NET}
mtf_net_sha256=${MTF_NET_SHA}
affine_source_path=${AFFINE_SOURCE}
affine_source_sha256=${AFFINE_SOURCE_SHA}
affine_runner_path=${AFFINE_RUNNER}
affine_runner_sha256=${AFFINE_RUNNER_SHA}
affine_binary_path=${AFFINE_BIN}
affine_binary_sha256=${AFFINE_BIN_SHA}
train_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
actual_train_minimum_anchor=0
actual_train_maximum_anchor=2495
actual_validation_minimum_anchor=2560
actual_validation_maximum_anchor=2815
maximum_anchor_read=2815
train_probe_rows=22464
validation_probe_rows=2304
checkpoint_load_count=2
encoder_passes_per_anchor=1
source_order_policy=sequential
graph_order_fingerprint=4133db527907a8e4
coordinates_targets_identical=true
historical_all_tokens_byte_identical=true
selection_order=direction,rank,correlation,lower_rmse
selection_tie_tolerance=1e-12
selected_pool=${best_arm}
selected_pool_report=${best_report}
selected_pool_report_sha256=$(sha256 "$best_report")
selected_pool.validation.directional_accuracy=$(kv "$best_report" selected.validation.directional_accuracy)
selected_pool.validation.pairwise_rank_accuracy=$(kv "$best_report" selected.validation.pairwise_rank_accuracy)
selected_pool.validation.correlation=$(kv "$best_report" selected.validation.correlation)
selected_pool.validation.rmse=$(kv "$best_report" selected.validation.rmse)
selected_pool.validation.rmse_target_rms_ratio=$(kv "$best_report" selected.validation.rmse_target_rms_ratio)
selected_pool.validation_strong_gate_pass=$(kv "$best_report" validation_strong_gate_pass)
whole_run_timeout_seconds=5400
term_to_kill_grace_seconds=30
automatic_retry=false
model_state_mutated=false
checkpoint_written=false
mdn_model_constructed=false
mdn_checkpoint_access=false
mdn_execution=false
policy_config_parsed_as_inert_dependency=true
policy_model_constructed=false
policy_checkpoint_access=false
policy_execution=false
policy_metric_access=false
canonical_data_raw_access=false
certified_input_access=false
certified_candidates_scored=0
final_holdout_access=false
background_processes_remaining=0
EOF

  for split in train validation; do
    printf 'capture.%s.report_path=%s\n' "$split" "${RUNTIME_ROOT}/capture/${split}/capture.report" >> "$candidate"
    printf 'capture.%s.report_sha256=%s\n' "$split" "$(sha256 "${RUNTIME_ROOT}/capture/${split}/capture.report")" >> "$candidate"
    printf 'capture.%s.log_path=%s\n' "$split" "${RUNTIME_ROOT}/logs/capture_${split}.log" >> "$candidate"
    printf 'capture.%s.log_sha256=%s\n' "$split" "$(sha256 "${RUNTIME_ROOT}/logs/capture_${split}.log")" >> "$candidate"
    for arm in "${ARMS[@]}"; do
      printf 'pool.%s.%s.probe_path=%s\n' "$arm" "$split" "${RUNTIME_ROOT}/capture/${split}/${arm}.probe" >> "$candidate"
      printf 'pool.%s.%s.probe_sha256=%s\n' "$arm" "$split" "$(sha256 "${RUNTIME_ROOT}/capture/${split}/${arm}.probe")" >> "$candidate"
      printf 'pool.%s.%s.coordinates_targets_sha256=%s\n' "$arm" "$split" \
        "$(sha256 "${RUNTIME_ROOT}/projections/${split}.${arm}.coordinates_targets.csv")" >> "$candidate"
    done
  done

  for arm in "${ARMS[@]}"; do
    main="$(arm_report_path "$arm" main)"
    replay="$(arm_report_path "$arm" replay)"
    gate="$(kv "$main" validation_strong_gate_pass)"
    printf 'pool.%s.main_report_path=%s\n' "$arm" "$main" >> "$candidate"
    printf 'pool.%s.main_report_sha256=%s\n' "$arm" "$(sha256 "$main")" >> "$candidate"
    printf 'pool.%s.replay_report_path=%s\n' "$arm" "$replay" >> "$candidate"
    printf 'pool.%s.replay_report_sha256=%s\n' "$arm" "$(sha256 "$replay")" >> "$candidate"
    printf 'pool.%s.selected_ridge=%s\n' "$arm" "$(kv "$main" selected_ridge)" >> "$candidate"
    printf 'pool.%s.validation.directional_accuracy=%s\n' "$arm" "$(kv "$main" selected.validation.directional_accuracy)" >> "$candidate"
    printf 'pool.%s.validation.pairwise_rank_accuracy=%s\n' "$arm" "$(kv "$main" selected.validation.pairwise_rank_accuracy)" >> "$candidate"
    printf 'pool.%s.validation.correlation=%s\n' "$arm" "$(kv "$main" selected.validation.correlation)" >> "$candidate"
    printf 'pool.%s.validation.rmse=%s\n' "$arm" "$(kv "$main" selected.validation.rmse)" >> "$candidate"
    printf 'pool.%s.validation.rmse_target_rms_ratio=%s\n' "$arm" "$(kv "$main" selected.validation.rmse_target_rms_ratio)" >> "$candidate"
    printf 'pool.%s.validation_strong_gate_pass=%s\n' "$arm" "$gate" >> "$candidate"
    printf 'pool.%s.validation_partial_gate_pass=%s\n' "$arm" "$(kv "$main" validation_partial_gate_pass)" >> "$candidate"
    if [[ "$arm" != all_tokens ]]; then
      for lane in main replay; do
        printf 'pool.%s.%s_log_path=%s\n' "$arm" "$lane" "${RUNTIME_ROOT}/affine/${arm}/${lane}/stdout.log" >> "$candidate"
        printf 'pool.%s.%s_log_sha256=%s\n' "$arm" "$lane" "$(sha256 "${RUNTIME_ROOT}/affine/${arm}/${lane}/stdout.log")" >> "$candidate"
      done
    fi
  done
}

verify_result() {
  preflight_authority
  verify_sealed_runtime
  require_canonical_path "$RESULT"
  [[ "$(stat -c '%a:%h:%u' -- "$RESULT")" == "444:1:0" ]] || fail "result metadata mismatch"
  expect_kv "$RESULT" schema_id "${SCHEMA_ID}.development.v1"
  expect_kv "$RESULT" status complete
  expect_kv "$RESULT" scientific_scope development_only
  expect_kv "$RESULT" maximum_anchor_read 2815
  expect_kv "$RESULT" train_probe_rows 22464
  expect_kv "$RESULT" validation_probe_rows 2304
  expect_kv "$RESULT" checkpoint_load_count 2
  expect_kv "$RESULT" encoder_passes_per_anchor 1
  expect_kv "$RESULT" source_order_policy sequential
  expect_kv "$RESULT" graph_order_fingerprint 4133db527907a8e4
  expect_kv "$RESULT" coordinates_targets_identical true
  expect_kv "$RESULT" historical_all_tokens_byte_identical true
  expect_kv "$RESULT" model_state_mutated false
  expect_kv "$RESULT" checkpoint_written false
  expect_kv "$RESULT" mdn_model_constructed false
  expect_kv "$RESULT" mdn_checkpoint_access false
  expect_kv "$RESULT" mdn_execution false
  expect_kv "$RESULT" policy_config_parsed_as_inert_dependency true
  expect_kv "$RESULT" policy_model_constructed false
  expect_kv "$RESULT" policy_checkpoint_access false
  expect_kv "$RESULT" policy_execution false
  expect_kv "$RESULT" policy_metric_access false
  expect_kv "$RESULT" canonical_data_raw_access false
  expect_kv "$RESULT" certified_input_access false
  expect_kv "$RESULT" certified_candidates_scored 0
  expect_kv "$RESULT" final_holdout_access false
  expect_kv "$RESULT" background_processes_remaining 0
  expect_kv "$RESULT" train_anchor_range '[0,2496)'
  expect_kv "$RESULT" validation_anchor_range '[2560,2816)'
  expect_kv "$RESULT" actual_train_minimum_anchor 0
  expect_kv "$RESULT" actual_train_maximum_anchor 2495
  expect_kv "$RESULT" actual_validation_minimum_anchor 2560
  expect_kv "$RESULT" actual_validation_maximum_anchor 2815
  expect_kv "$RESULT" selection_order direction,rank,correlation,lower_rmse
  expect_kv "$RESULT" selection_tie_tolerance 1e-12
  expect_kv "$RESULT" whole_run_timeout_seconds 5400
  expect_kv "$RESULT" term_to_kill_grace_seconds 30
  expect_kv "$RESULT" automatic_retry false
  verify_attempt "$(kv "$RESULT" attempt_sha256)"
  [[ "$(sha256 "$RUNNER")" == "$(kv "$RESULT" runner_sha256)" ]] || fail "runner result binding mismatch"
  [[ "$(sha256 "$PREREG")" == "$(kv "$RESULT" preregistration_sha256)" ]] || fail "prereg result binding mismatch"
  [[ "$(sha256 "$CAPTURE_SOURCE")" == "$(kv "$RESULT" capture_source_sha256)" ]] || fail "capture source result binding mismatch"
  [[ "$(kv "$RESULT" capture_build_receipt_path)" == "$CAPTURE_BUILD_RECEIPT" ]] || fail "build-receipt path mismatch"
  [[ "$(kv "$RESULT" capture_build_receipt_sha256)" == "$CAPTURE_BUILD_RECEIPT_SHA" ]] || fail "build-receipt hash mismatch"
  require_file_hash "$FROZEN_CAPTURE_BIN" "$(kv "$RESULT" capture_binary_sha256)"
  [[ "$(kv "$RESULT" capture_binary_sha256)" == "$CAPTURE_BUILD_SHA" ]] || fail "capture binary provenance mismatch"
  expect_kv "$RESULT" capture_development_sha256 "$CAPTURE_DEVELOPMENT_SHA"
  expect_kv "$RESULT" input_receipt_sha256 "$INPUT_RECEIPT_SHA"
  expect_kv "$RESULT" source_closure_sha256 "$SOURCE_CLOSURE_SHA"
  expect_kv "$RESULT" isolated_registry_sha256 "$ISOLATED_REGISTRY_SHA"
  expect_kv "$RESULT" isolated_base_config_sha256 "$ISOLATED_BASE_CONFIG_SHA"
  expect_kv "$RESULT" source_manifest_sha256 "$SOURCE_MANIFEST_SHA"
  expect_kv "$RESULT" cursor_erratum_sha256 "$CURSOR_ERRATUM_SHA"
  expect_kv "$RESULT" capture_config_sha256 "$CAPTURE_CONFIG_SHA"
  expect_kv "$RESULT" representation_checkpoint_sha256 "$CHECKPOINT_SHA"
  expect_kv "$RESULT" historical_train_probe_sha256 "$HIST_TRAIN_PROBE_SHA"
  expect_kv "$RESULT" historical_validation_probe_sha256 "$HIST_VALIDATION_PROBE_SHA"
  expect_kv "$RESULT" historical_train_manifest_sha256 "$HIST_TRAIN_MANIFEST_SHA"
  expect_kv "$RESULT" historical_validation_manifest_sha256 "$HIST_VALIDATION_MANIFEST_SHA"
  expect_kv "$RESULT" historical_component_spawn_fingerprint 5ba58d2de0fb7dcb
  expect_kv "$RESULT" historical_protocol_contract_fingerprint d8a39dbf11f94332
  expect_kv "$RESULT" historical_graph_order_fingerprint 4133db527907a8e4
  expect_kv "$RESULT" mtf_dsl_sha256 "$MTF_DSL_SHA"
  expect_kv "$RESULT" mtf_grammar_sha256 "$MTF_GRAMMAR_SHA"
  expect_kv "$RESULT" retrieval_dsl_sha256 "$RETRIEVAL_DSL_SHA"
  expect_kv "$RESULT" splits_dsl_sha256 "$SPLITS_DSL_SHA"
  expect_kv "$RESULT" protocol_dsl_sha256 "$PROTOCOL_DSL_SHA"
  expect_kv "$RESULT" topology_dsl_sha256 "$TOPOLOGY_DSL_SHA"
  expect_kv "$RESULT" nodelift_dsl_sha256 "$NODELIFT_DSL_SHA"
  expect_kv "$RESULT" mtf_net_sha256 "$MTF_NET_SHA"
  expect_kv "$RESULT" affine_source_sha256 "$AFFINE_SOURCE_SHA"
  expect_kv "$RESULT" affine_runner_sha256 "$AFFINE_RUNNER_SHA"
  expect_kv "$RESULT" affine_binary_sha256 "$AFFINE_BIN_SHA"

  local split begin end rows anchors max_anchor range arm probe projection expected_projection main replay lane
  for split in train validation; do
    if [[ "$split" == train ]]; then
      begin=0; end=2496; rows=22464; anchors=2496; max_anchor=2495; range='[0,2496)'
    else
      begin=2560; end=2816; rows=2304; anchors=256; max_anchor=2815; range='[2560,2816)'
    fi
    require_file_hash "$(kv "$RESULT" "capture.${split}.report_path")" \
      "$(kv "$RESULT" "capture.${split}.report_sha256")"
    require_file_hash "$(kv "$RESULT" "capture.${split}.log_path")" \
      "$(kv "$RESULT" "capture.${split}.log_sha256")"
    validate_capture_report "$(kv "$RESULT" "capture.${split}.report_path")" \
      "$range" "$anchors" "$max_anchor" "$rows"
    for arm in "${ARMS[@]}"; do
      probe="$(kv "$RESULT" "pool.${arm}.${split}.probe_path")"
      require_file_hash "$probe" "$(kv "$RESULT" "pool.${arm}.${split}.probe_sha256")"
      validate_probe "$probe" "$begin" "$end" "$rows"
      projection="${RUNTIME_ROOT}/projections/${split}.${arm}.coordinates_targets.csv"
      require_file_hash "$projection" "$(kv "$RESULT" "pool.${arm}.${split}.coordinates_targets_sha256")"
      expected_projection="$(kv "$RESULT" "pool.all_tokens.${split}.coordinates_targets_sha256")"
      [[ "$(sha256 "$projection")" == "$expected_projection" ]] || fail "projection identity changed"
    done
  done
  [[ "$(sha256 "${RUNTIME_ROOT}/capture/train/all_tokens.probe")" == "$HIST_TRAIN_PROBE_SHA" ]] || fail "train baseline changed"
  [[ "$(sha256 "${RUNTIME_ROOT}/capture/validation/all_tokens.probe")" == "$HIST_VALIDATION_PROBE_SHA" ]] || fail "validation baseline changed"

  for arm in "${ARMS[@]}"; do
    main="$(kv "$RESULT" "pool.${arm}.main_report_path")"
    replay="$(kv "$RESULT" "pool.${arm}.replay_report_path")"
    require_file_hash "$main" "$(kv "$RESULT" "pool.${arm}.main_report_sha256")"
    require_file_hash "$replay" "$(kv "$RESULT" "pool.${arm}.replay_report_sha256")"
    cmp -s -- "$main" "$replay" || fail "report replay changed: ${arm}"
    validate_affine_report "$main"
    if [[ "$arm" != all_tokens ]]; then
      for lane in main replay; do
        require_file_hash "$(kv "$RESULT" "pool.${arm}.${lane}_log_path")" \
          "$(kv "$RESULT" "pool.${arm}.${lane}_log_sha256")"
      done
    fi
    [[ "$(kv "$RESULT" "pool.${arm}.selected_ridge")" == "$(kv "$main" selected_ridge)" ]] || fail "ridge binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation.directional_accuracy")" == "$(kv "$main" selected.validation.directional_accuracy)" ]] || fail "direction binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation.pairwise_rank_accuracy")" == "$(kv "$main" selected.validation.pairwise_rank_accuracy)" ]] || fail "rank binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation.correlation")" == "$(kv "$main" selected.validation.correlation)" ]] || fail "correlation binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation.rmse")" == "$(kv "$main" selected.validation.rmse)" ]] || fail "RMSE binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation.rmse_target_rms_ratio")" == "$(kv "$main" selected.validation.rmse_target_rms_ratio)" ]] || fail "RMSE ratio binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation_strong_gate_pass")" == "$(kv "$main" validation_strong_gate_pass)" ]] || fail "gate binding mismatch: ${arm}"
    [[ "$(kv "$RESULT" "pool.${arm}.validation_partial_gate_pass")" == "$(kv "$main" validation_partial_gate_pass)" ]] || fail "partial gate binding mismatch: ${arm}"
  done
  local best_arm=all_tokens best_report="$BASELINE_MAIN" alternate_strong=false expected_classification
  for arm in "${ALTERNATE_ARMS[@]}"; do
    main="$(arm_report_path "$arm" main)"
    report_is_better "$main" "$best_report" && { best_arm="$arm"; best_report="$main"; }
    [[ "$(kv "$main" validation_strong_gate_pass)" == true ]] && alternate_strong=true
  done
  [[ "$(kv "$RESULT" selected_pool)" == "$best_arm" ]] || fail "selected-pool recomputation mismatch"
  [[ "$(kv "$RESULT" selected_pool_report)" == "$best_report" ]] || fail "selected report path mismatch"
  [[ "$(kv "$RESULT" selected_pool_report_sha256)" == "$(sha256 "$best_report")" ]] || fail "selected report hash mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation.directional_accuracy)" == "$(kv "$best_report" selected.validation.directional_accuracy)" ]] || fail "selected direction summary mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation.pairwise_rank_accuracy)" == "$(kv "$best_report" selected.validation.pairwise_rank_accuracy)" ]] || fail "selected rank summary mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation.correlation)" == "$(kv "$best_report" selected.validation.correlation)" ]] || fail "selected correlation summary mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation.rmse)" == "$(kv "$best_report" selected.validation.rmse)" ]] || fail "selected RMSE summary mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation.rmse_target_rms_ratio)" == "$(kv "$best_report" selected.validation.rmse_target_rms_ratio)" ]] || fail "selected ratio summary mismatch"
  [[ "$(kv "$RESULT" selected_pool.validation_strong_gate_pass)" == "$(kv "$best_report" validation_strong_gate_pass)" ]] || fail "selected gate summary mismatch"
  if [[ "$(kv "$BASELINE_MAIN" validation_strong_gate_pass)" == false && "$alternate_strong" == true ]]; then
    expected_classification=development_serving_pool_sufficiency_candidate
  else
    expected_classification=serving_pool_sufficiency_not_established
  fi
  [[ "$(kv "$RESULT" classification)" == "$expected_classification" ]] || fail "classification recomputation mismatch"
  scan_for_background_processes
}

prepare_runtime_after_lock() {
  local path
  for path in "${RUNTIME_ROOT}/.scratch" "${RUNTIME_ROOT}/bin"; do
    if [[ ! -e "$path" && ! -L "$path" ]]; then
      mkdir -m 0700 -- "$path"
    fi
    require_canonical_path "$path"
    [[ -d "$path" && ! -L "$path" ]] || fail "invalid runtime directory: ${path}"
    [[ "$(stat -c '%a:%h:%u' -- "$path")" == "700:1:0" ]] ||
      fail "runtime directory metadata mismatch: ${path}"
  done
  [[ -z "$(find "${RUNTIME_ROOT}/.scratch" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
    fail "scratch directory is not pristine"
  if [[ ! -e "$FROZEN_CAPTURE_BIN" ]]; then
    local candidate="${RUNTIME_ROOT}/.scratch/capture.bin.$$"
    cp -- "$CAPTURE_BUILD" "$candidate"
    chmod 0555 -- "$candidate"
    mv -T -n -- "$candidate" "$FROZEN_CAPTURE_BIN" || fail "failed to publish frozen capture binary"
  fi
  [[ -f "$FROZEN_CAPTURE_BIN" && ! -L "$FROZEN_CAPTURE_BIN" ]] || fail "invalid frozen capture binary"
  [[ "$(stat -c '%a:%h:%u' -- "$FROZEN_CAPTURE_BIN")" == "555:1:0" ]] || fail "frozen capture metadata mismatch"
  [[ "$(sha256 "$FROZEN_CAPTURE_BIN")" == "$CAPTURE_BUILD_SHA" ]] || fail "frozen capture differs from build receipt"
  [[ -z "$(find "${RUNTIME_ROOT}/.scratch" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
    fail "scratch directory gained an unexpected entry"
}

open_execution_lock() {
  local runtime_parent
  runtime_parent="$(dirname -- "$RUNTIME_ROOT")"
  require_canonical_path "$runtime_parent"
  [[ -d "$runtime_parent" && ! -L "$runtime_parent" ]] || fail "invalid runtime parent"
  if [[ ! -e "$RUNTIME_ROOT" && ! -L "$RUNTIME_ROOT" ]]; then
    mkdir -m 0700 -- "$RUNTIME_ROOT"
  fi
  require_canonical_path "$RUNTIME_ROOT"
  [[ -d "$RUNTIME_ROOT" && ! -L "$RUNTIME_ROOT" ]] || fail "invalid runtime root"
  [[ "$(stat -c '%a:%h:%u' -- "$RUNTIME_ROOT")" == "700:1:0" ]] ||
    fail "runtime root metadata mismatch before execution"
  if [[ ! -e "$LOCK" && ! -L "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || true
  fi
  require_canonical_path "$LOCK"
  [[ -f "$LOCK" && ! -L "$LOCK" ]] || fail "invalid execution lock"
  [[ "$(stat -c '%a:%h:%u' -- "$LOCK")" == "600:1:0" ]] || fail "execution lock metadata mismatch"
  exec 9<> "$LOCK"
  [[ "$(stat -Lc '%d:%i' -- "/proc/$$/fd/9")" == "$(stat -Lc '%d:%i' -- "$LOCK")" ]] ||
    fail "opened execution lock identity mismatch"
  flock -n 9 || fail "another Clear Signal replay holds the execution lock"
}

preflight_health() {
  local free_kib gpu_free gpu_processes proc pid cmd
  command -v timeout >/dev/null || fail "GNU timeout is unavailable"
  command -v flock >/dev/null || fail "flock is unavailable"
  command -v nvidia-smi >/dev/null || fail "nvidia-smi is unavailable"
  free_kib="$(df -Pk -- "$(dirname -- "$RUNTIME_ROOT")" | awk 'NR == 2 {print $4}')"
  [[ "$free_kib" =~ ^[0-9]+$ && "$free_kib" -ge 2097152 ]] ||
    fail "less than 2 GiB is free for the bounded replay"
  gpu_free="$(nvidia-smi --query-gpu=memory.free --format=csv,noheader,nounits | awk 'NR == 1 {gsub(/ /, ""); print}')"
  [[ "$gpu_free" =~ ^[0-9]+$ && "$gpu_free" -ge 4096 ]] ||
    fail "less than 4 GiB of GPU memory is free"
  gpu_processes="$(nvidia-smi --query-compute-apps=pid --format=csv,noheader,nounits 2>/dev/null | awk 'NF {count++} END {print count+0}')"
  [[ "$gpu_processes" == 0 ]] || fail "another GPU compute process is active"
  for proc in /proc/[0-9]*; do
    pid="${proc##*/}"
    [[ "$pid" == "$$" || "$pid" == "$PPID" || ! -r "${proc}/cmdline" ]] && continue
    cmd="$(tr '\0' ' ' < "${proc}/cmdline" 2>/dev/null || true)"
    if [[ "$cmd" == *"cuwacunu_exec"* || "$cmd" == *"cuwacunu_mtf_serving_pool_capture"* ||
          "$cmd" == *"frozen_representation_affine_probe"* ]]; then
      fail "another Cuwacunu model/evaluator process is active: pid=${pid}"
    fi
  done
}

run_development() {
  preflight_authority
  require_canonical_path "$RUNNER"
  require_canonical_path "$PREREG"
  require_canonical_path "$CAPTURE_SOURCE"
  require_canonical_path "$CAPTURE_BUILD"
  [[ -f "$CAPTURE_BUILD" && -x "$CAPTURE_BUILD" ]] || fail "capture binary is not built"
  [[ "$(stat -c '%a' -- "$RUNNER")" == "555" ]] || fail "runner must be frozen mode 0555"
  [[ "$(stat -c '%a' -- "$PREREG")" == "444" ]] || fail "preregistration must be frozen mode 0444"
  [[ "$(stat -c '%a' -- "$CAPTURE_SOURCE")" == "444" ]] || fail "capture source must be frozen mode 0444"

  if [[ -e "$RESULT" ]]; then
    verify_result
    echo "[clear-signal] development result already complete and verified"
    return 0
  fi
  open_execution_lock
  [[ ! -e "$ATTEMPT" ]] || fail "attempt is already consumed; retry/resume is forbidden"
  local path
  for path in capture affine logs projections; do
    [[ ! -e "${RUNTIME_ROOT}/${path}" && ! -L "${RUNTIME_ROOT}/${path}" ]] ||
      fail "scientific artifacts exist before the attempt receipt: ${path}"
  done
  prepare_runtime_after_lock
  preflight_health

  local worker_token capability_file rc scan_rc
  worker_token="$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')"
  [[ "$worker_token" =~ ^[0-9a-f]{64}$ ]] || fail "failed to create worker capability"
  capability_file="${RUNTIME_ROOT}/.scratch/worker.capability.$$"
  printf '%s\n' "$worker_token" > "$capability_file"
  chmod 0600 -- "$capability_file"
  exec 8< "$capability_file"
  rm -f -- "$capability_file"
  set +e
  CLEAR_SIGNAL_WORKER_TOKEN="$worker_token" timeout --signal=TERM --kill-after=30s 5400s \
    "$RUNNER" --worker
  rc=$?
  set -e
  exec 8<&-
  if [[ "$rc" != 0 ]]; then
    set +e
    scan_for_background_processes
    scan_rc=$?
    set -e
    [[ "$scan_rc" == 0 ]] || fail "worker status ${rc}; process cleanup could not be proven"
    if [[ -e "$ATTEMPT" ]]; then
      fail "terminal consumed attempt ended with worker status ${rc}; no retry is allowed"
    fi
    fail "worker preflight ended with status ${rc} before attempt publication; the scientific attempt remains unconsumed"
  fi

  scan_for_background_processes
  verify_result
  echo "[clear-signal] development replay complete: ${RESULT}"
}

plan() {
  preflight_authority
  echo "Project Clear Signal — MTF serving-pool causal isolation"
  echo "scope=development_only"
  echo "attempt_consumed=$([[ -e "$ATTEMPT" ]] && echo true || echo false)"
  echo "result_present=$([[ -e "$RESULT" ]] && echo true || echo false)"
  echo "train_range=[0,2496)"
  echo "validation_range=[2560,2816)"
  echo "maximum_anchor_read=2815"
  echo "checkpoint_sha256=${CHECKPOINT_SHA}"
  echo "capture_invocations=2"
  echo "affine_invocations=6"
  echo "mdn_execution=false"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "hard_timeout_seconds=5400"
}

main() {
  [[ $# -ge 1 ]] || fail "usage: $0 --plan|--run-development|--verify-development"
  case "$1" in
    --plan)
      [[ $# == 1 ]] || fail "--plan takes no arguments"
      plan
      ;;
    --run-development)
      [[ $# == 1 ]] || fail "--run-development takes no arguments"
      run_development
      ;;
    --verify-development)
      [[ $# == 1 ]] || fail "--verify-development takes no arguments"
      [[ -e "$RESULT" ]] || fail "development result is absent"
      verify_result
      echo "[clear-signal] development result verified"
      ;;
    --worker)
      [[ $# == 1 ]] || fail "invalid private worker invocation"
      worker
      ;;
    *) fail "unsupported mode: $1" ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
