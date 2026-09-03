#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C LANG=C
umask 077

readonly ROOT="/cuwacunu"
readonly PROTOCOL_ID="synthetic_v2_frozen_mtf_prepool_domain_scale_affine_development_v1"
readonly DIAGNOSTIC_PHASE="prepool_domain_scale"
readonly RUNNER="$(readlink -f -- "${BASH_SOURCE[0]}")"
readonly PREREG="${ROOT}/src/config/benchmarks/synthetic_continuous_graph_v2/FROZEN_MTF_PREPOOL_DOMAIN_SCALE_AFFINE_PREREGISTRATION.md"
readonly PREREG_SHA="bb1d4f446e098284564d89c4e3cbb308174f3aff85b95f230d2483e14a469ab5"

readonly CAPTURE_SOURCE="${ROOT}/src/main/exec/cuwacunu_mtf_prepool_domain_scale_capture.cpp"
readonly CAPTURE_SOURCE_SHA="4d7c961129723f3983de17c2212a8ca4f1550327f472104d6369071d921aee54"
readonly CAPTURE_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_mtf_prepool_domain_scale_capture.sh"
readonly CAPTURE_WRAPPER_SHA="99fc145011c36671b6b8d55c1b546ec255454232a8c13ece5ea56fcb572a418f"
readonly EVALUATOR_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/mtf_prepool_domain_scale_channel_conditioned_affine_probe.cpp"
readonly EVALUATOR_SOURCE_SHA="ba13d95c4d33347cf4840f8eaa30616e095cf1c7dc3b0fa85de6a6c8f7c6f718"
readonly EVALUATOR_WRAPPER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_mtf_prepool_domain_scale_channel_conditioned_affine_probe.sh"
readonly EVALUATOR_WRAPPER_SHA="22d9047dfd8248fc65453744f1b4363ab5c4584b88574d7fa42e294754b9aaf6"
readonly SERVING_CAPTURE_SOURCE="${ROOT}/src/main/exec/cuwacunu_mtf_serving_pool_capture.cpp"
readonly SERVING_CAPTURE_SOURCE_SHA="8b76023ff0fa64ef0d431d450ef9534b8b020e6901b6978198a71a494c3a9edb"
readonly ENCODER_HEADER="${ROOT}/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"
readonly ENCODER_HEADER_SHA="27d80ade37dbae85379af8c75311e1a1e69cdd88cde4fecdee21221c399d1c21"
readonly PHASE2A_SOURCE="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_encoder_channel_conditioned_affine_probe.cpp"
readonly PHASE2A_SOURCE_SHA="5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570"
readonly CANONICAL_PARSER="${ROOT}/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp"
readonly CANONICAL_PARSER_SHA="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"

readonly COMPILER_ALIAS="/usr/bin/g++"
readonly COMPILER="/usr/bin/x86_64-linux-gnu-g++-12"
readonly COMPILER_SHA="dd91977c184e327710578363ad93ebb175c3a457b6236b874fd3911b7c055c65"
readonly COMMON_ARCHIVE="${ROOT}/.build/lib/libcommon.a"
readonly COMMON_ARCHIVE_SHA="853ade11707a8588194eda199e5a742e7363c2d1fd87f43285f3ad89414e06d3"
readonly TORCH_ARCHIVE="${ROOT}/.build/lib/libtorchwrap.a"
readonly TORCH_ARCHIVE_SHA="d9a128191f227a798219c9ee0c2ed8d4c6976916dba8b43eb66c6f25c21b279d"
readonly LIBTORCH_ROOT="${ROOT}/.external/libtorch-upd"
readonly CUDA_ROOT="/usr/local/cuda-12.4"
readonly -a DIRECT_LIBRARY_BINDINGS=(
  "${LIBTORCH_ROOT}/lib/libtorch_cuda.so|${LIBTORCH_ROOT}/lib/libtorch_cuda.so|23db5cf52d4986f420077e8620e26a3940acf347faf82ee637728164c54478ff"
  "${LIBTORCH_ROOT}/lib/libc10_cuda.so|${LIBTORCH_ROOT}/lib/libc10_cuda.so|31250c11fd4b95eed52c24b067772df9edba72bcdba8e257b6ecdea37835d852"
  "${LIBTORCH_ROOT}/lib/libtorch_cpu.so|${LIBTORCH_ROOT}/lib/libtorch_cpu.so|ad1ff2d39eac3f4c56c0ac35627a83c06ed367f0e7b075d1b437461c881e7116"
  "${LIBTORCH_ROOT}/lib/libtorch.so|${LIBTORCH_ROOT}/lib/libtorch.so|024ae7b7d8eae378c36c992e8a69e16a027794ca2d6b13a9700873bb73ba20f0"
  "${LIBTORCH_ROOT}/lib/libc10.so|${LIBTORCH_ROOT}/lib/libc10.so|735930b9588dde30c73856117530cf0d9da26003b86424695a10c8ceb891a0f4"
  "${CUDA_ROOT}/lib64/libcudart.so|${CUDA_ROOT}/targets/x86_64-linux/lib/libcudart.so.12.4.127|8774224f5b11a73b15d074a3fcce7327322c5c4cfdfd924d6a826779eec968fe"
  "${CUDA_ROOT}/lib64/libnvToolsExt.so|${CUDA_ROOT}/targets/x86_64-linux/lib/libnvToolsExt.so.1.0.0|847d78f275c8cc97442a278073204bdc1a0009ffc2559bf7248c70d92105dfdc"
  "/usr/lib/x86_64-linux-gnu/libcuda.so|/usr/lib/x86_64-linux-gnu/libcuda.so.1|57e0db4fcada1712297e0c9ab0d7d4beff59c663468876f77a262eda98a6e0b8"
)

readonly V2_PROTOCOL_ID="synthetic_v2_sealed_raw96_edge_channel_affine_re_evaluation_development_v2"
readonly V2_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${V2_PROTOCOL_ID}"
readonly V2_LOCK="${V2_ROOT}/.execution.lock"
readonly V2_RESULT="${V2_ROOT}/development.status"
readonly V2_RESULT_SHA="c9c1f6248f99b00fd50b2b61d2c161abe321057690c37368f3e811568221484d"
readonly V2_ATTEMPT="${V2_ROOT}/attempt.status"
readonly V2_ATTEMPT_SHA="44016b20ad3bac75efa9d1ea22edaa0de4a157be8b2277ad7f4fddbf30d3c8e8"
readonly V2_SCIENCE="${V2_ROOT}/evidence/science.complete.status"
readonly V2_SCIENCE_SHA="11b7c27d6b529e8489a8c5370b798e9755bd4e4d1aced7ec1b7d12d96c839e19"
readonly V2_PROJECTION="${V2_ROOT}/evidence/projection.complete.status"
readonly V2_PROJECTION_SHA="d2644ffabf676fb62883c6b4c5d9c2983c16ade9cfa348b5ee77972864924ae7"

readonly CANONICAL_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2"
readonly CANONICAL_LOCK="${CANONICAL_ROOT}/.execution.lock"
readonly CANONICAL_RESULT="${CANONICAL_ROOT}/development.status"
readonly CANONICAL_RESULT_SHA="fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6"
readonly CANONICAL_INPUTS="${CANONICAL_ROOT}/inputs.status"
readonly CANONICAL_INPUTS_SHA="fedbf63815d5806309ac4f6c469b379c685825e8ec83a3b9bf8250663f6e39b0"
readonly CAPTURE_CONFIG="${CANONICAL_ROOT}/synthetic_benchmark.frozen_feature_capture.isolated.config"
readonly CAPTURE_CONFIG_SHA="eeea5620f1b271c0bd4527db6764c8f7b66eef5aced7b72d9d1b28d89443c9b3"
readonly HIST_TRAIN="${CANONICAL_ROOT}/jobs/train/representation_edge_features.probe"
readonly HIST_TRAIN_SHA="d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75"
readonly HIST_VALIDATION="${CANONICAL_ROOT}/jobs/validation/representation_edge_features.probe"
readonly HIST_VALIDATION_SHA="8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd"
readonly HIST_TRAIN_MANIFEST="${CANONICAL_ROOT}/jobs/train/job.manifest"
readonly HIST_TRAIN_MANIFEST_SHA="d7dc64ab1d424160a30756bdadb449cb2ad27ce9788fbb184d13fdaf66526b6e"
readonly HIST_VALIDATION_MANIFEST="${CANONICAL_ROOT}/jobs/validation/job.manifest"
readonly HIST_VALIDATION_MANIFEST_SHA="0b6d85705e478321ad285f784d09391ca1255664f24624c962d57523d75ed02c"
readonly TRAIN_PROJECTION_SHA="f7a935fe83bb5e72388c63a4ab6e063d7908913f4cc52a5fcb652ead5e7dd08d"
readonly VALIDATION_PROJECTION_SHA="c1575a9936cca22f24c2e40908c8196c64bdfc7f89cdf15f70e822e92b16ec22"

readonly REPRESENTATION_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_isolated_v2"
readonly REPRESENTATION_LOCK="${REPRESENTATION_ROOT}/.execution.lock"
readonly CHECKPOINT="${REPRESENTATION_ROOT}/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"
readonly CHECKPOINT_SHA="70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d"

readonly SOURCE_AUTH_ROOT="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1"
readonly SOURCE_LOCK="${SOURCE_AUTH_ROOT}/.execution.lock"
readonly SOURCE_CLOSURE="${SOURCE_AUTH_ROOT}/development_source_closure.status"
readonly SOURCE_CLOSURE_SHA="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly CURSOR_ERRATUM="${SOURCE_AUTH_ROOT}/cursor_alignment_erratum.status"
readonly CURSOR_ERRATUM_SHA="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly SOURCE_ROOT="${SOURCE_AUTH_ROOT}/source"
readonly SOURCE_MANIFEST="${SOURCE_AUTH_ROOT}/source_manifest.status"
readonly SOURCE_MANIFEST_SHA="7cf41d721647579924620c9daf7e38931898ba28a02c71c38cc7cd6e3f6431fa"
readonly ISOLATED_REGISTRY="${SOURCE_AUTH_ROOT}/config/ujcamei.source.registry.development_prefix.dsl"
readonly ISOLATED_REGISTRY_SHA="54d87853a1d41facd54c24dc4031c2983e9cce40064a8ac7e793fe5fee77cf5c"
readonly ISOLATED_BASE_CONFIG="${SOURCE_AUTH_ROOT}/config/synthetic_benchmark.isolated_development.config"
readonly ISOLATED_BASE_CONFIG_SHA="9d5bb23194c5a227ec91cf5882225a26a4f2b1f3f631c167810bd7f71314d7ab"
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

readonly CAPTURE_REPORT_KEY_COUNT=65
readonly CAPTURE_REPORT_SORTED_KEY_SHA="a66a0a942371694b7374e6be109b0581d87b13e795190b7191a9529f3f0b3ecd"
readonly EVALUATOR_REPORT_KEY_COUNT=249
readonly EVALUATOR_REPORT_SORTED_KEY_SHA="cf47e8483e193eee90cd3fd52b90e30e63aa5e4a136bea4706969a4af6732811"
readonly TIE_TOLERANCE="1e-12"
readonly PASS_CLASS="prepool_domain_scale_affine_strong_gate_observed_development_only"
readonly NO_PASS_CLASS="prepool_domain_scale_affine_strong_gate_not_observed"
readonly PREP_TIMEOUT_SECONDS=900
readonly PREP_TERM_GRACE_SECONDS=10
readonly WORKER_TIMEOUT_SECONDS=5400
readonly TERM_GRACE_SECONDS=30

readonly RUNTIME="${ROOT}/.runtime/benchmarks/synthetic_continuous_graph_v2/${PROTOCOL_ID}"
readonly PREP_DIR="${RUNTIME}/prepared"
readonly EVIDENCE="${RUNTIME}/evidence"
readonly CAPTURE_EVIDENCE="${EVIDENCE}/capture"
readonly AFFINE_EVIDENCE="${EVIDENCE}/affine"
readonly SCRATCH="${RUNTIME}/.scratch"
readonly LOCK="${RUNTIME}/.execution.lock"
readonly CAPTURE_BIN="${PREP_DIR}/cuwacunu_mtf_prepool_domain_scale_capture"
readonly EVALUATOR_BIN="${PREP_DIR}/mtf_prepool_domain_scale_channel_conditioned_affine_probe"
readonly CAPTURE_BUILD_LOG="${PREP_DIR}/capture.build.log"
readonly EVALUATOR_BUILD_LOG="${PREP_DIR}/evaluator.build.log"
readonly PREP_RECEIPT="${PREP_DIR}/prepared.status"
readonly ATTEMPT="${RUNTIME}/attempt.status"
readonly CAPTURE_ROOT="${RUNTIME}/capture"
readonly CAPTURE_TRAIN_DIR="${CAPTURE_ROOT}/train"
readonly CAPTURE_VALIDATION_DIR="${CAPTURE_ROOT}/validation"
readonly TRAIN_REFERENCE="${CAPTURE_TRAIN_DIR}/all_tokens_reference.probe"
readonly TRAIN_PREPOOL="${CAPTURE_TRAIN_DIR}/prepool_domain_scale.probe"
readonly TRAIN_CAPTURE_REPORT="${CAPTURE_TRAIN_DIR}/capture.report"
readonly VALIDATION_REFERENCE="${CAPTURE_VALIDATION_DIR}/all_tokens_reference.probe"
readonly VALIDATION_PREPOOL="${CAPTURE_VALIDATION_DIR}/prepool_domain_scale.probe"
readonly VALIDATION_CAPTURE_REPORT="${CAPTURE_VALIDATION_DIR}/capture.report"
readonly TRAIN_CAPTURE_LAUNCH="${CAPTURE_EVIDENCE}/train.launch.status"
readonly TRAIN_CAPTURE_RETURNED="${CAPTURE_EVIDENCE}/train.returned.status"
readonly TRAIN_CAPTURE_COMPLETE="${CAPTURE_EVIDENCE}/train.complete.status"
readonly VALIDATION_CAPTURE_LAUNCH="${CAPTURE_EVIDENCE}/validation.launch.status"
readonly VALIDATION_CAPTURE_RETURNED="${CAPTURE_EVIDENCE}/validation.returned.status"
readonly VALIDATION_CAPTURE_COMPLETE="${CAPTURE_EVIDENCE}/validation.complete.status"
readonly PROJECTION_COMPLETE="${EVIDENCE}/projection.complete.status"
readonly AFFINE_ROOT="${RUNTIME}/affine"
readonly AFFINE_MAIN_DIR="${AFFINE_ROOT}/main"
readonly AFFINE_REPLAY_DIR="${AFFINE_ROOT}/replay"
readonly MAIN_REPORT="${AFFINE_MAIN_DIR}/development.report"
readonly REPLAY_REPORT="${AFFINE_REPLAY_DIR}/development.report"
readonly MAIN_LAUNCH="${AFFINE_EVIDENCE}/main.launch.status"
readonly MAIN_RETURNED="${AFFINE_EVIDENCE}/main.returned.status"
readonly MAIN_COMPLETE="${AFFINE_EVIDENCE}/main.complete.status"
readonly REPLAY_LAUNCH="${AFFINE_EVIDENCE}/replay.launch.status"
readonly REPLAY_RETURNED="${AFFINE_EVIDENCE}/replay.returned.status"
readonly REPLAY_COMPLETE="${AFFINE_EVIDENCE}/replay.complete.status"
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
PREP_ACTIVE_PID=""
PREP_SUCCESS=0

fail() {
  echo "[clear-signal:prepool-domain-scale] ERROR: $*" >&2
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
  canonical="$(readlink -f -- "$path")" || {
    fail "could not canonicalize file: ${path}"
    return 1
  }
  if [[ "$canonical" != "$path" ]]; then
    fail "noncanonical or symlinked file: ${path}"
    return 1
  fi
  read -r mode uid links < <(stat -c '%a %u %h' -- "$path") || {
    fail "could not stat file: ${path}"
    return 1
  }
  if [[ "$mode" != "$expected_mode" || "$uid" != 0 || "$links" != 1 ]]; then
    fail "invalid frozen metadata for ${path}: ${mode}:${uid}:${links}"
    return 1
  fi
}

require_readonly_file() {
  local path="$1" canonical mode uid links
  if [[ "$path" != /* || ! -f "$path" || -L "$path" ]]; then
    fail "not a regular absolute file: ${path}"
    return 1
  fi
  canonical="$(readlink -f -- "$path")" || return 1
  [[ "$canonical" == "$path" ]] || {
    fail "noncanonical file: ${path}"
    return 1
  }
  read -r mode uid links < <(stat -c '%a %u %h' -- "$path") || return 1
  if (( (8#$mode & 8#222) != 0 )) || [[ "$uid" != 0 || "$links" != 1 ]]; then
    fail "file is not frozen root-owned single-link authority: ${path}"
    return 1
  fi
}

require_exact() {
  local observed
  require_file "$1" "$3" || return 1
  observed="$(sha256 "$1")" || return 1
  [[ "$observed" == "$2" ]] || {
    fail "SHA-256 mismatch: $1"
    return 1
  }
}

require_readonly_exact() {
  local observed
  require_readonly_file "$1" || return 1
  observed="$(sha256 "$1")" || return 1
  [[ "$observed" == "$2" ]] || {
    fail "SHA-256 mismatch: $1"
    return 1
  }
}

require_build_input_exact() {
  local path="$1" expected="$2" canonical observed
  if [[ "$path" != /* || ! -f "$path" || -L "$path" ]]; then
    fail "not a regular resolved build input: ${path}"
    return 1
  fi
  canonical="$(readlink -f -- "$path")" || return 1
  [[ "$canonical" == "$path" ]] || {
    fail "noncanonical resolved build input: ${path}"
    return 1
  }
  observed="$(sha256 "$path")" || return 1
  [[ "$observed" == "$expected" ]] || {
    fail "build-input SHA-256 mismatch: ${path}"
    return 1
  }
}

require_private_dir() {
  local path="$1" canonical mode uid
  if [[ "$path" != /* || ! -d "$path" || -L "$path" ]]; then
    fail "not a private directory: ${path}"
    return 1
  fi
  canonical="$(readlink -f -- "$path")" || return 1
  [[ "$canonical" == "$path" ]] || {
    fail "noncanonical directory: ${path}"
    return 1
  }
  read -r mode uid < <(stat -c '%a %u' -- "$path") || return 1
  [[ "$mode" == 700 && "$uid" == 0 ]] || {
    fail "invalid private directory metadata: ${path} (${mode}:${uid})"
    return 1
  }
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
    [[ -n "$key" ]] || {
      fail "empty key in ${path}"
      return 1
    }
    cache_key="${path}"$'\034'"${key}"
    [[ -z "${KV_PRESENT[$cache_key]+x}" ]] || {
      fail "duplicate key ${key} in ${path}"
      return 1
    }
    KV_PRESENT["$cache_key"]=1
    KV_VALUE["$cache_key"]="$value"
  done < "$path"
  KV_LOADED["$path"]=1
}

kv() {
  local path="$1" key="$2" cache_key="${1}"$'\034'"${2}"
  [[ -n "${KV_LOADED[$path]+x}" ]] || {
    fail "uncached key-value file: ${path}"
    return 1
  }
  [[ -n "${KV_PRESENT[$cache_key]+x}" ]] || {
    fail "missing key ${key} in ${path}"
    return 1
  }
  printf '%s' "${KV_VALUE[$cache_key]}"
}

expect() {
  local actual
  actual="$(kv "$1" "$2")" || return 1
  [[ "$actual" == "$3" ]] || {
    fail "$1: $2 expected '$3', got '$actual'"
    return 1
  }
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
  chmod "$mode" -- "$candidate" || {
    fail "could not freeze publication candidate: ${candidate}"
    return 1
  }
  require_file "$candidate" "${mode#0}" || return 1
  mv -T -n -- "$candidate" "$destination" || {
    fail "atomic publication move failed: ${destination}"
    return 1
  }
  if [[ -e "$candidate" || ! -f "$destination" || -L "$destination" ]]; then
    fail "no-clobber publication failed: ${destination}"
    return 1
  fi
  require_file "$destination" "${mode#0}" || return 1
}

require_resolved_binding() {
  local alias_path="$1" resolved_path="$2" expected_sha="$3"
  [[ -e "$alias_path" || -L "$alias_path" ]] || {
    fail "missing build link input: ${alias_path}"
    return 1
  }
  [[ "$(realpath -- "$alias_path")" == "$resolved_path" ]] || {
    fail "resolved build input mismatch: ${alias_path}"
    return 1
  }
  require_build_input_exact "$resolved_path" "$expected_sha"
}

ensure_runtime_and_lock() {
  [[ "$(id -u)" == 0 ]] || fail "runner requires uid 0"
  if [[ ! -e "$RUNTIME" ]]; then install -d -m 0700 -- "$RUNTIME"; fi
  require_private_dir "$RUNTIME"
  if [[ ! -e "$LOCK" ]]; then
    (set -o noclobber; : > "$LOCK") 2>/dev/null || {
      fail "could not create execution lock"
      return 1
    }
    chmod 0600 -- "$LOCK" || return 1
  fi
  require_file "$LOCK" 600
  install -d -m 0700 -- "$PREP_DIR" "$EVIDENCE" "$CAPTURE_EVIDENCE" \
    "$AFFINE_EVIDENCE" "$SCRATCH"
  require_private_dir "$PREP_DIR"
  require_private_dir "$EVIDENCE"
  require_private_dir "$CAPTURE_EVIDENCE"
  require_private_dir "$AFFINE_EVIDENCE"
  require_private_dir "$SCRATCH"
}

open_execution_lock_exclusive() {
  exec 9<>"$LOCK"
  flock -n 9 || fail "another protocol process owns the execution lock"
}

open_execution_lock_shared() {
  exec 9<"$LOCK"
  flock -s -n 9 || fail "protocol execution is active"
}

require_lock_file() { require_file "$1" "$2"; }

acquire_authority_locks() {
  require_lock_file "$V2_LOCK" 600
  require_lock_file "$CANONICAL_LOCK" 600
  require_lock_file "$REPRESENTATION_LOCK" 600
  require_lock_file "$SOURCE_LOCK" 600
  exec 3<"$V2_LOCK"
  exec 4<"$CANONICAL_LOCK"
  exec 5<"$REPRESENTATION_LOCK"
  exec 6<"$SOURCE_LOCK"
  flock -s 3
  flock -s 4
  flock -s 5
  flock -s 6
}

preflight_build_authorities() {
  local binding alias_path resolved_path expected_sha
  require_exact "$PREREG" "$PREREG_SHA" 444
  require_exact "$CAPTURE_SOURCE" "$CAPTURE_SOURCE_SHA" 444
  require_exact "$CAPTURE_WRAPPER" "$CAPTURE_WRAPPER_SHA" 555
  require_exact "$EVALUATOR_SOURCE" "$EVALUATOR_SOURCE_SHA" 444
  require_exact "$EVALUATOR_WRAPPER" "$EVALUATOR_WRAPPER_SHA" 555
  require_exact "$SERVING_CAPTURE_SOURCE" "$SERVING_CAPTURE_SOURCE_SHA" 444
  require_exact "$ENCODER_HEADER" "$ENCODER_HEADER_SHA" 444
  require_exact "$PHASE2A_SOURCE" "$PHASE2A_SOURCE_SHA" 444
  require_exact "$CANONICAL_PARSER" "$CANONICAL_PARSER_SHA" 444
  [[ "$(command -v g++)" == "$COMPILER_ALIAS" ]] || fail "g++ command path mismatch"
  [[ "$(realpath -- "$COMPILER_ALIAS")" == "$COMPILER" ]] ||
    fail "g++ resolved path mismatch"
  require_build_input_exact "$COMPILER" "$COMPILER_SHA"
  require_readonly_exact "$COMMON_ARCHIVE" "$COMMON_ARCHIVE_SHA"
  require_readonly_exact "$TORCH_ARCHIVE" "$TORCH_ARCHIVE_SHA"
  for binding in "${DIRECT_LIBRARY_BINDINGS[@]}"; do
    IFS='|' read -r alias_path resolved_path expected_sha <<<"$binding"
    require_resolved_binding "$alias_path" "$resolved_path" "$expected_sha"
  done
}

preflight_receipt_authorities() {
  require_exact "$V2_RESULT" "$V2_RESULT_SHA" 444
  require_exact "$V2_ATTEMPT" "$V2_ATTEMPT_SHA" 444
  require_exact "$V2_SCIENCE" "$V2_SCIENCE_SHA" 444
  require_exact "$V2_PROJECTION" "$V2_PROJECTION_SHA" 444
  require_exact "$CANONICAL_RESULT" "$CANONICAL_RESULT_SHA" 444
  require_exact "$CANONICAL_INPUTS" "$CANONICAL_INPUTS_SHA" 444
  require_exact "$SOURCE_CLOSURE" "$SOURCE_CLOSURE_SHA" 444
  require_exact "$CURSOR_ERRATUM" "$CURSOR_ERRATUM_SHA" 444
  require_exact "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA" 444
  require_lock_file "$V2_LOCK" 600
  require_lock_file "$CANONICAL_LOCK" 600
  require_lock_file "$REPRESENTATION_LOCK" 600
  require_lock_file "$SOURCE_LOCK" 600
}

validate_v2_result() {
  local item
  load_kv_file "$V2_RESULT" || return 1
  local -a contract=(
    "schema_id=${V2_PROTOCOL_ID}"
    "status=complete"
    "protocol_id=${V2_PROTOCOL_ID}"
    "classification=sealed_v2_raw96_edge_channel_affine_strong_gate_not_observed"
    "attempt_sha256=${V2_ATTEMPT_SHA}"
    "science_complete_sha256=${V2_SCIENCE_SHA}"
    "projection_complete_sha256=${V2_PROJECTION_SHA}"
    "unique_pair_count=7"
    "unique_pairs_completed=7"
    "logical_arm_count=8"
    "logical_arms_represented=8"
    "evaluator_invocations_completed=14"
    "analytic_candidate_fit_count=84"
    "edge_channel_head_solve_count=756"
    "main_replay_parity_check_count=7"
    "projection_split_check_count=14"
    "strong_gate_pass_count=0"
    "strong_stop_triggered=false"
    "strong_stop_unique_index=-1"
    "strong_stop_arm=not_applicable"
    "all_unique_pairs_evaluated=true"
    "capture_execution_count=0"
    "encoder_execution_count=0"
    "checkpoint_read_count=0"
    "representation_model_execution_count=0"
    "mdn_model_execution_count=0"
    "optimizer_fit_count=0"
    "optimizer_step_count=0"
    "refit_count=0"
    "certified_input_access=false"
    "final_holdout_access=false"
    "policy_access=false"
    "maximum_anchor_read=2815"
    "train_anchor_range=[0,2496)"
    "validation_anchor_range=[2560,2816)"
  )
  for item in "${contract[@]}"; do
    expect "$V2_RESULT" "${item%%=*}" "${item#*=}" || return 1
  done
  [[ ! -e "${V2_ROOT}/terminal.invalid.status" && ! -L "${V2_ROOT}/terminal.invalid.status" ]] ||
    fail "authoritative V2 unexpectedly has a terminal receipt"
  [[ -d "${V2_ROOT}/.scratch" && -z "$(find "${V2_ROOT}/.scratch" -mindepth 1 -print -quit)" ]] ||
    fail "authoritative V2 scratch is not empty"
}

validate_canonical_lineage_receipts() {
  local item
  load_kv_file "$CANONICAL_RESULT" || return 1
  load_kv_file "$SOURCE_CLOSURE" || return 1
  load_kv_file "$CURSOR_ERRATUM" || return 1
  local -a canonical_contract=(
    "schema_id=synthetic_v2_frozen_feature_capture_isolated_v2.development.v1"
    "status=complete"
    "input_receipt_path=${CANONICAL_INPUTS}"
    "input_receipt_sha256=${CANONICAL_INPUTS_SHA}"
    "isolated_source_closure_path=${SOURCE_CLOSURE}"
    "isolated_source_closure_sha256=${SOURCE_CLOSURE_SHA}"
    "cursor_alignment_erratum_receipt_path=${CURSOR_ERRATUM}"
    "cursor_alignment_erratum_receipt_sha256=${CURSOR_ERRATUM_SHA}"
    "isolated_source_root_path=${SOURCE_ROOT}"
    "capture_config_path=${CAPTURE_CONFIG}"
    "capture_config_sha256=${CAPTURE_CONFIG_SHA}"
    "representation_checkpoint_path=${CHECKPOINT}"
    "representation_checkpoint_sha256=${CHECKPOINT_SHA}"
    "train_capture_range=[0,2496)"
    "validation_capture_range=[2560,2816)"
    "maximum_anchor_read=2815"
    "train_probe_rows=22464"
    "validation_probe_rows=2304"
    "canonical_data_raw_access=false"
    "final_holdout_access=false"
    "policy_access=false"
    "trained_train_manifest_path=${HIST_TRAIN_MANIFEST}"
    "trained_train_manifest_sha256=${HIST_TRAIN_MANIFEST_SHA}"
    "trained_validation_manifest_path=${HIST_VALIDATION_MANIFEST}"
    "trained_validation_manifest_sha256=${HIST_VALIDATION_MANIFEST_SHA}"
  )
  for item in "${canonical_contract[@]}"; do
    expect "$CANONICAL_RESULT" "${item%%=*}" "${item#*=}" || return 1
  done
  local -a closure_contract=(
    "isolated_source_root_path=${SOURCE_ROOT}"
    "isolated_registry_path=${ISOLATED_REGISTRY}"
    "isolated_registry_sha256=${ISOLATED_REGISTRY_SHA}"
    "isolated_base_config_path=${ISOLATED_BASE_CONFIG}"
    "isolated_base_config_sha256=${ISOLATED_BASE_CONFIG_SHA}"
    "source_manifest_path=${SOURCE_MANIFEST}"
    "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    "strict_cache_freshness=pass"
    "source_tree_read_only=true"
    "config_read_only=true"
    "canonical_data_raw_access=false"
    "final_holdout_available=false"
    "accepted_anchor_count=3261"
    "candidate_anchor_count=3261"
    "maximum_anchor_index=3260"
  )
  for item in "${closure_contract[@]}"; do
    expect "$SOURCE_CLOSURE" "${item%%=*}" "${item#*=}" || return 1
  done
  local -a erratum_contract=(
    "accepted_anchor_count=3261"
    "candidate_anchor_count=3261"
    "maximum_anchor_index=3260"
    "train_anchor_range=[0,2496)"
    "validation_anchor_range=[2560,2816)"
    "certified_development_anchor_range=[2880,3261)"
    "canonical_data_raw_access=false"
    "final_holdout_available=false"
  )
  for item in "${erratum_contract[@]}"; do
    expect "$CURSOR_ERRATUM" "${item%%=*}" "${item#*=}" || return 1
  done
}

preflight_static_authorities() {
  require_exact "$RUNNER" "$(sha256 "$RUNNER")" 555
  preflight_build_authorities
  preflight_receipt_authorities
  validate_v2_result
  validate_canonical_lineage_receipts
}

validate_source_manifest() {
  local index kind path expected_sha expected_size count
  require_exact "$ISOLATED_REGISTRY" "$ISOLATED_REGISTRY_SHA" 444
  require_exact "$ISOLATED_BASE_CONFIG" "$ISOLATED_BASE_CONFIG_SHA" 444
  require_exact "$SOURCE_MANIFEST" "$SOURCE_MANIFEST_SHA" 444
  load_kv_file "$SOURCE_MANIFEST" || return 1
  expect "$SOURCE_MANIFEST" schema_id synthetic_v2_isolated_development_source_v1.source_manifest.v1
  expect "$SOURCE_MANIFEST" status complete
  expect "$SOURCE_MANIFEST" isolated_source_root "$SOURCE_ROOT"
  expect "$SOURCE_MANIFEST" prefix_source_count 9
  expect "$SOURCE_MANIFEST" mirror_csv_count 9
  expect "$SOURCE_MANIFEST" mirror_cache_count 18
  expect "$SOURCE_MANIFEST" canonical_data_raw_access false
  expect "$SOURCE_MANIFEST" accepted_anchor_count 3261
  expect "$SOURCE_MANIFEST" maximum_anchor_index 3260
  expect "$SOURCE_MANIFEST" final_holdout_available false
  [[ -d "$SOURCE_ROOT" && ! -L "$SOURCE_ROOT" ]] || fail "invalid isolated source root"
  count="$(find "$SOURCE_ROOT" -type f | wc -l | tr -d ' ')" || return 1
  [[ "$count" == 27 ]] || fail "isolated source file count mismatch"
  [[ -z "$(find "$SOURCE_ROOT" \( -type l -o \! -type f -a \! -type d \) -print -quit)" ]] ||
    fail "isolated source contains a symlink or special entry"
  [[ -z "$(find "$SOURCE_ROOT" -perm /222 -print -quit)" ]] ||
    fail "isolated source tree is writable"
  for index in 00 01 02 03 04 05 06 07 08; do
    for kind in mirror raw_cache normalized_cache; do
      path="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_path")" || return 1
      expected_sha="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_sha256")" || return 1
      expected_size="$(kv "$SOURCE_MANIFEST" "source.${index}.${kind}_size_bytes")" || return 1
      [[ "$path" == "$SOURCE_ROOT/"* ]] || fail "source artifact escapes isolated root"
      require_readonly_exact "$path" "$expected_sha" || return 1
      [[ "$(stat -c '%s' -- "$path")" == "$expected_size" ]] ||
        fail "isolated source size mismatch: ${path}"
    done
  done
}

preflight_science_inputs() {
  preflight_static_authorities
  require_exact "$CAPTURE_CONFIG" "$CAPTURE_CONFIG_SHA" 444
  require_exact "$CHECKPOINT" "$CHECKPOINT_SHA" 444
  require_exact "$HIST_TRAIN" "$HIST_TRAIN_SHA" 444
  require_exact "$HIST_VALIDATION" "$HIST_VALIDATION_SHA" 444
  require_exact "$HIST_TRAIN_MANIFEST" "$HIST_TRAIN_MANIFEST_SHA" 444
  require_exact "$HIST_VALIDATION_MANIFEST" "$HIST_VALIDATION_MANIFEST_SHA" 444
  require_exact "$MTF_DSL" "$MTF_DSL_SHA" 444
  require_exact "$MTF_GRAMMAR" "$MTF_GRAMMAR_SHA" 444
  require_exact "$RETRIEVAL_DSL" "$RETRIEVAL_DSL_SHA" 444
  require_exact "$SPLITS_DSL" "$SPLITS_DSL_SHA" 444
  require_exact "$PROTOCOL_DSL" "$PROTOCOL_DSL_SHA" 444
  require_exact "$TOPOLOGY_DSL" "$TOPOLOGY_DSL_SHA" 444
  require_exact "$NODELIFT_DSL" "$NODELIFT_DSL_SHA" 444
  require_exact "$MTF_NET" "$MTF_NET_SHA" 444
  validate_source_manifest
  require_exact "$CAPTURE_BIN" "$(kv "$PREP_RECEIPT" capture_binary_sha256)" 555
  require_exact "$EVALUATOR_BIN" "$(kv "$PREP_RECEIPT" evaluator_binary_sha256)" 555
}

preflight_pre_attempt() {
  preflight_static_authorities
  validate_prepare_receipt
}

stop_prepare_group() {
  local pid="$1"
  kill -TERM -- "-${pid}" 2>/dev/null || true
  if ! timeout --foreground 10s tail --pid="$pid" -f /dev/null >/dev/null 2>&1; then
    kill -KILL -- "-${pid}" 2>/dev/null || true
  fi
  wait "$pid" 2>/dev/null || true
}

prepare_signal() {
  local signal="$1"
  trap '' HUP INT TERM QUIT
  if [[ -n "$PREP_ACTIVE_PID" ]]; then stop_prepare_group "$PREP_ACTIVE_PID"; fi
  exit 128
}

run_prepare_wrapper() {
  local label="$1" wrapper="$2" output="$3" final_log="$4"
  local candidate_log="${SCRATCH}/${label}.build.log.candidate" rc
  [[ ! -e "$output" && ! -L "$output" && ! -e "$final_log" && ! -L "$final_log" &&
     ! -e "$candidate_log" && ! -L "$candidate_log" ]] ||
    fail "prepare output is not pristine for ${label}"
  setsid timeout --signal=TERM --kill-after=${PREP_TERM_GRACE_SECONDS}s \
    "${PREP_TIMEOUT_SECONDS}s" "$wrapper" "$output" >"$candidate_log" 2>&1 &
  PREP_ACTIVE_PID=$!
  set +e
  wait "$PREP_ACTIVE_PID"
  rc=$?
  set -e
  PREP_ACTIVE_PID=""
  if (( rc != 0 )); then
    rm -f -- "$output" "$candidate_log"
    fail "${label} compile-only wrapper failed with exit ${rc}"
    return 1
  fi
  require_file "$output" 555 || return 1
  publish_once "$candidate_log" "$final_log" 0444
}

emit_prepare_receipt() {
  local candidate="${SCRATCH}/prepared.status.candidate"
  [[ ! -e "$candidate" && ! -L "$candidate" ]] || fail "prepare receipt candidate exists"
  {
    echo "schema_id=${PROTOCOL_ID}.prepared.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preparation_kind=two_sequential_bounded_compile_only_wrappers"
    echo "capture_wrapper_path=${CAPTURE_WRAPPER}"
    echo "capture_wrapper_sha256=${CAPTURE_WRAPPER_SHA}"
    echo "capture_source_path=${CAPTURE_SOURCE}"
    echo "capture_source_sha256=${CAPTURE_SOURCE_SHA}"
    echo "capture_binary_path=${CAPTURE_BIN}"
    echo "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    echo "capture_build_log_path=${CAPTURE_BUILD_LOG}"
    echo "capture_build_log_sha256=$(sha256 "$CAPTURE_BUILD_LOG")"
    echo "evaluator_wrapper_path=${EVALUATOR_WRAPPER}"
    echo "evaluator_wrapper_sha256=${EVALUATOR_WRAPPER_SHA}"
    echo "evaluator_source_path=${EVALUATOR_SOURCE}"
    echo "evaluator_source_sha256=${EVALUATOR_SOURCE_SHA}"
    echo "evaluator_binary_path=${EVALUATOR_BIN}"
    echo "evaluator_binary_sha256=$(sha256 "$EVALUATOR_BIN")"
    echo "evaluator_build_log_path=${EVALUATOR_BUILD_LOG}"
    echo "evaluator_build_log_sha256=$(sha256 "$EVALUATOR_BUILD_LOG")"
    echo "compile_wrapper_invocation_count=2"
    echo "compile_wrappers_sequential=true"
    echo "compile_wrappers_foreground=true"
    echo "per_wrapper_timeout_seconds=${PREP_TIMEOUT_SECONDS}"
    echo "per_wrapper_term_grace_seconds=${PREP_TERM_GRACE_SECONDS}"
    echo "build_input_scope=frozen_scientific_sources_and_project_archives_plus_explicit_framework_link_inputs"
    echo "full_transitive_system_toolchain_or_elf_closure=false"
    echo "probe_input_access=false"
    echo "config_input_access=false"
    echo "checkpoint_access=false"
    echo "source_data_access=false"
    echo "policy_artifact_access=false"
    echo "model_execution=false"
    echo "evaluator_execution=false"
    echo "capture_execution=false"
  } >"$candidate"
  publish_once "$candidate" "$PREP_RECEIPT" 0444
}

validate_prepare_receipt() {
  local item
  require_file "$PREP_RECEIPT" 444 || return 1
  [[ "$(wc -l < "$PREP_RECEIPT")" == 39 ]] || fail "prepare receipt key count mismatch"
  load_kv_file "$PREP_RECEIPT" || return 1
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.prepared.v1"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "preregistration_path=${PREREG}"
    "preregistration_sha256=${PREREG_SHA}"
    "runner_path=${RUNNER}"
    "runner_sha256=$(sha256 "$RUNNER")"
    "preparation_kind=two_sequential_bounded_compile_only_wrappers"
    "capture_wrapper_path=${CAPTURE_WRAPPER}"
    "capture_wrapper_sha256=${CAPTURE_WRAPPER_SHA}"
    "capture_source_path=${CAPTURE_SOURCE}"
    "capture_source_sha256=${CAPTURE_SOURCE_SHA}"
    "capture_binary_path=${CAPTURE_BIN}"
    "capture_build_log_path=${CAPTURE_BUILD_LOG}"
    "evaluator_wrapper_path=${EVALUATOR_WRAPPER}"
    "evaluator_wrapper_sha256=${EVALUATOR_WRAPPER_SHA}"
    "evaluator_source_path=${EVALUATOR_SOURCE}"
    "evaluator_source_sha256=${EVALUATOR_SOURCE_SHA}"
    "evaluator_binary_path=${EVALUATOR_BIN}"
    "evaluator_build_log_path=${EVALUATOR_BUILD_LOG}"
    "compile_wrapper_invocation_count=2"
    "compile_wrappers_sequential=true"
    "compile_wrappers_foreground=true"
    "per_wrapper_timeout_seconds=${PREP_TIMEOUT_SECONDS}"
    "per_wrapper_term_grace_seconds=${PREP_TERM_GRACE_SECONDS}"
    "build_input_scope=frozen_scientific_sources_and_project_archives_plus_explicit_framework_link_inputs"
    "full_transitive_system_toolchain_or_elf_closure=false"
    "probe_input_access=false"
    "config_input_access=false"
    "checkpoint_access=false"
    "source_data_access=false"
    "policy_artifact_access=false"
    "model_execution=false"
    "evaluator_execution=false"
    "capture_execution=false"
  )
  for item in "${contract[@]}"; do
    expect "$PREP_RECEIPT" "${item%%=*}" "${item#*=}" || return 1
  done
  require_exact "$CAPTURE_BIN" "$(kv "$PREP_RECEIPT" capture_binary_sha256)" 555
  require_exact "$EVALUATOR_BIN" "$(kv "$PREP_RECEIPT" evaluator_binary_sha256)" 555
  require_exact "$CAPTURE_BUILD_LOG" "$(kv "$PREP_RECEIPT" capture_build_log_sha256)" 444
  require_exact "$EVALUATOR_BUILD_LOG" "$(kv "$PREP_RECEIPT" evaluator_build_log_sha256)" 444
}

prepare() {
  ensure_runtime_and_lock
  open_execution_lock_exclusive
  acquire_authority_locks
  preflight_static_authorities
  if [[ -e "$ATTEMPT" || -L "$ATTEMPT" || -e "$RESULT" || -L "$RESULT" ||
        -e "$TERMINAL" || -L "$TERMINAL" ]]; then
    fail "prepare is forbidden after scientific identity consumption"
    return 1
  fi
  if [[ -e "$PREP_RECEIPT" || -L "$PREP_RECEIPT" ]]; then
    validate_prepare_receipt
    echo "preparation already complete: ${PREP_RECEIPT}"
    return 0
  fi
  rm -f -- "$CAPTURE_BIN" "$EVALUATOR_BIN" "$CAPTURE_BUILD_LOG" \
    "$EVALUATOR_BUILD_LOG" "${SCRATCH}/capture.build.log.candidate" \
    "${SCRATCH}/evaluator.build.log.candidate"
  trap 'prepare_signal HUP' HUP
  trap 'prepare_signal INT' INT
  trap 'prepare_signal TERM' TERM
  trap 'prepare_signal QUIT' QUIT
  run_prepare_wrapper capture "$CAPTURE_WRAPPER" "$CAPTURE_BIN" "$CAPTURE_BUILD_LOG"
  run_prepare_wrapper evaluator "$EVALUATOR_WRAPPER" "$EVALUATOR_BIN" "$EVALUATOR_BUILD_LOG"
  emit_prepare_receipt
  validate_prepare_receipt
  PREP_SUCCESS=1
  trap - HUP INT TERM QUIT
  echo "preparation complete: ${PREP_RECEIPT}"
}

emit_attempt() {
  local candidate="${SCRATCH}/attempt.status.candidate"
  [[ ! -e "$candidate" && ! -L "$candidate" && ! -e "$ATTEMPT" && ! -L "$ATTEMPT" ]] ||
    fail "attempt identity is not pristine"
  {
    echo "schema_id=${PROTOCOL_ID}.attempt.v1"
    echo "status=committed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "project_goal=Project Clear Signal - Test the Frozen Pre-Pool Domain-by-Scale Surface"
    echo "diagnostic_phase=${DIAGNOSTIC_PHASE}"
    echo "diagnostic_authority=development_only"
    echo "benchmark_acceptance_authority=false"
    echo "certified_authorization_eligible=false"
    echo "classification_if_complete=${PASS_CLASS}|${NO_PASS_CLASS}"
    echo "preregistration_path=${PREREG}"
    echo "preregistration_sha256=${PREREG_SHA}"
    echo "runner_path=${RUNNER}"
    echo "runner_sha256=$(sha256 "$RUNNER")"
    echo "preparation_receipt_path=${PREP_RECEIPT}"
    echo "preparation_receipt_sha256=$(sha256 "$PREP_RECEIPT")"
    echo "capture_binary_path=${CAPTURE_BIN}"
    echo "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    echo "evaluator_binary_path=${EVALUATOR_BIN}"
    echo "evaluator_binary_sha256=$(sha256 "$EVALUATOR_BIN")"
    echo "predecessor_v2_result_path=${V2_RESULT}"
    echo "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
    echo "predecessor_v2_attempt_sha256=${V2_ATTEMPT_SHA}"
    echo "predecessor_v2_science_receipt_sha256=${V2_SCIENCE_SHA}"
    echo "predecessor_v2_projection_receipt_sha256=${V2_PROJECTION_SHA}"
    echo "predecessor_v2_strong_gate_pass_count=0"
    echo "canonical_result_path=${CANONICAL_RESULT}"
    echo "canonical_result_sha256=${CANONICAL_RESULT_SHA}"
    echo "canonical_inputs_path=${CANONICAL_INPUTS}"
    echo "canonical_inputs_sha256=${CANONICAL_INPUTS_SHA}"
    echo "source_closure_path=${SOURCE_CLOSURE}"
    echo "source_closure_sha256=${SOURCE_CLOSURE_SHA}"
    echo "cursor_erratum_path=${CURSOR_ERRATUM}"
    echo "cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}"
    echo "source_manifest_path=${SOURCE_MANIFEST}"
    echo "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    echo "capture_config_path=${CAPTURE_CONFIG}"
    echo "capture_config_sha256=${CAPTURE_CONFIG_SHA}"
    echo "checkpoint_path=${CHECKPOINT}"
    echo "checkpoint_sha256=${CHECKPOINT_SHA}"
    echo "historical_train_probe_path=${HIST_TRAIN}"
    echo "historical_train_probe_sha256=${HIST_TRAIN_SHA}"
    echo "historical_validation_probe_path=${HIST_VALIDATION}"
    echo "historical_validation_probe_sha256=${HIST_VALIDATION_SHA}"
    echo "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
    echo "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
    echo "train_anchor_range=[0,2496)"
    echo "validation_anchor_range=[2560,2816)"
    echo "maximum_anchor_read=2815"
    echo "train_probe_rows=22464"
    echo "validation_probe_rows=2304"
    echo "capture_process_invocations_planned=2"
    echo "capture_replay_invocations_planned=0"
    echo "checkpoint_load_count_planned=2"
    echo "encoder_forward_calls_planned=43"
    echo "encoder_anchor_participations_planned=2752"
    echo "evaluator_invocations_planned=2"
    echo "ridge_candidates_per_evaluator=6"
    echo "ridge_invariant_system_build_count_planned=18"
    echo "cholesky_factorization_count_planned=108"
    echo "cholesky_solve_count_planned=108"
    echo "conditioned_head_solve_count_planned=108"
    echo "historical_byte_parity_check_count_planned=2"
    echo "projection_check_count_planned=6"
    echo "main_replay_parity_check_count_planned=1"
    echo "capture_train_argv=${CAPTURE_BIN} --config ${CAPTURE_CONFIG} --input-representation-checkpoint ${CHECKPOINT} --output-dir ${CAPTURE_TRAIN_DIR} --anchor-index-begin 0 --anchor-index-end 2496"
    echo "capture_validation_argv=${CAPTURE_BIN} --config ${CAPTURE_CONFIG} --input-representation-checkpoint ${CHECKPOINT} --output-dir ${CAPTURE_VALIDATION_DIR} --anchor-index-begin 2560 --anchor-index-end 2816"
    echo "evaluator_main_argv=${EVALUATOR_BIN} --probe-kind prepool_domain_scale --development-only --train-input ${TRAIN_PREPOOL} --validation-input ${VALIDATION_PREPOOL} --output ${MAIN_REPORT}"
    echo "evaluator_replay_argv=${EVALUATOR_BIN} --probe-kind prepool_domain_scale --development-only --train-input ${TRAIN_PREPOOL} --validation-input ${VALIDATION_PREPOOL} --output ${REPLAY_REPORT}"
    echo "capture_flags_exact_once=true"
    echo "evaluator_flags_exact_once=true"
    echo "additional_cli_arguments=false"
    echo "main_replay_required=true"
    echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
    echo "private_worker_required=true"
    echo "authority_lock_count=4"
    echo "automatic_retry=false"
    echo "same_protocol_retry=false"
    echo "same_protocol_resume=false"
    echo "capture_replay=false"
    echo "refit=false"
    echo "early_stopping=false"
    echo "seed_selection=false"
    echo "hyperparameter_search=false"
    echo "optimizer_training=false"
    echo "checkpoint_write=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_model_access=false"
    echo "mdn_model_access=false"
    echo "full_transitive_system_toolchain_or_elf_closure=false"
  } >"$candidate"
  publish_once "$candidate" "$ATTEMPT" 0444
}

validate_attempt() {
  local item
  require_file "$ATTEMPT" 444 || return 1
  [[ "$(wc -l < "$ATTEMPT")" == 91 ]] || fail "attempt key count mismatch"
  load_kv_file "$ATTEMPT" || return 1
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.attempt.v1"
    "status=committed"
    "protocol_id=${PROTOCOL_ID}"
    "project_goal=Project Clear Signal - Test the Frozen Pre-Pool Domain-by-Scale Surface"
    "diagnostic_phase=${DIAGNOSTIC_PHASE}"
    "diagnostic_authority=development_only"
    "benchmark_acceptance_authority=false"
    "certified_authorization_eligible=false"
    "classification_if_complete=${PASS_CLASS}|${NO_PASS_CLASS}"
    "preregistration_path=${PREREG}"
    "preregistration_sha256=${PREREG_SHA}"
    "runner_path=${RUNNER}"
    "runner_sha256=$(sha256 "$RUNNER")"
    "preparation_receipt_path=${PREP_RECEIPT}"
    "preparation_receipt_sha256=$(sha256 "$PREP_RECEIPT")"
    "capture_binary_path=${CAPTURE_BIN}"
    "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    "evaluator_binary_path=${EVALUATOR_BIN}"
    "evaluator_binary_sha256=$(sha256 "$EVALUATOR_BIN")"
    "predecessor_v2_result_path=${V2_RESULT}"
    "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
    "predecessor_v2_attempt_sha256=${V2_ATTEMPT_SHA}"
    "predecessor_v2_science_receipt_sha256=${V2_SCIENCE_SHA}"
    "predecessor_v2_projection_receipt_sha256=${V2_PROJECTION_SHA}"
    "predecessor_v2_strong_gate_pass_count=0"
    "canonical_result_path=${CANONICAL_RESULT}"
    "canonical_result_sha256=${CANONICAL_RESULT_SHA}"
    "canonical_inputs_path=${CANONICAL_INPUTS}"
    "canonical_inputs_sha256=${CANONICAL_INPUTS_SHA}"
    "source_closure_path=${SOURCE_CLOSURE}"
    "source_closure_sha256=${SOURCE_CLOSURE_SHA}"
    "cursor_erratum_path=${CURSOR_ERRATUM}"
    "cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}"
    "source_manifest_path=${SOURCE_MANIFEST}"
    "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    "capture_config_path=${CAPTURE_CONFIG}"
    "capture_config_sha256=${CAPTURE_CONFIG_SHA}"
    "checkpoint_path=${CHECKPOINT}"
    "checkpoint_sha256=${CHECKPOINT_SHA}"
    "historical_train_probe_path=${HIST_TRAIN}"
    "historical_train_probe_sha256=${HIST_TRAIN_SHA}"
    "historical_validation_probe_path=${HIST_VALIDATION}"
    "historical_validation_probe_sha256=${HIST_VALIDATION_SHA}"
    "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
    "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
    "train_anchor_range=[0,2496)"
    "validation_anchor_range=[2560,2816)"
    "maximum_anchor_read=2815"
    "train_probe_rows=22464"
    "validation_probe_rows=2304"
    "capture_process_invocations_planned=2"
    "capture_replay_invocations_planned=0"
    "checkpoint_load_count_planned=2"
    "encoder_forward_calls_planned=43"
    "encoder_anchor_participations_planned=2752"
    "evaluator_invocations_planned=2"
    "ridge_candidates_per_evaluator=6"
    "ridge_invariant_system_build_count_planned=18"
    "cholesky_factorization_count_planned=108"
    "cholesky_solve_count_planned=108"
    "conditioned_head_solve_count_planned=108"
    "historical_byte_parity_check_count_planned=2"
    "projection_check_count_planned=6"
    "main_replay_parity_check_count_planned=1"
    "capture_train_argv=${CAPTURE_BIN} --config ${CAPTURE_CONFIG} --input-representation-checkpoint ${CHECKPOINT} --output-dir ${CAPTURE_TRAIN_DIR} --anchor-index-begin 0 --anchor-index-end 2496"
    "capture_validation_argv=${CAPTURE_BIN} --config ${CAPTURE_CONFIG} --input-representation-checkpoint ${CHECKPOINT} --output-dir ${CAPTURE_VALIDATION_DIR} --anchor-index-begin 2560 --anchor-index-end 2816"
    "evaluator_main_argv=${EVALUATOR_BIN} --probe-kind prepool_domain_scale --development-only --train-input ${TRAIN_PREPOOL} --validation-input ${VALIDATION_PREPOOL} --output ${MAIN_REPORT}"
    "evaluator_replay_argv=${EVALUATOR_BIN} --probe-kind prepool_domain_scale --development-only --train-input ${TRAIN_PREPOOL} --validation-input ${VALIDATION_PREPOOL} --output ${REPLAY_REPORT}"
    "capture_flags_exact_once=true"
    "evaluator_flags_exact_once=true"
    "additional_cli_arguments=false"
    "main_replay_required=true"
    "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    "term_grace_seconds=${TERM_GRACE_SECONDS}"
    "private_worker_required=true"
    "authority_lock_count=4"
    "automatic_retry=false"
    "same_protocol_retry=false"
    "same_protocol_resume=false"
    "capture_replay=false"
    "refit=false"
    "early_stopping=false"
    "seed_selection=false"
    "hyperparameter_search=false"
    "optimizer_training=false"
    "checkpoint_write=false"
    "certified_input_access=false"
    "final_holdout_access=false"
    "policy_model_access=false"
    "mdn_model_access=false"
    "full_transitive_system_toolchain_or_elf_closure=false"
  )
  for item in "${contract[@]}"; do
    expect "$ATTEMPT" "${item%%=*}" "${item#*=}" || return 1
  done
}

capture_paths() {
  local split="$1"
  case "$split" in
    train)
      printf '%s\n' "$CAPTURE_TRAIN_DIR" "$TRAIN_REFERENCE" "$TRAIN_PREPOOL" \
        "$TRAIN_CAPTURE_REPORT" "$TRAIN_CAPTURE_LAUNCH" "$TRAIN_CAPTURE_RETURNED" \
        "$TRAIN_CAPTURE_COMPLETE" "0" "2496" "2496" "22464" "39" "9984" \
        "239616" "78" "$HIST_TRAIN" "$HIST_TRAIN_SHA" "$TRAIN_PROJECTION_SHA"
      ;;
    validation)
      printf '%s\n' "$CAPTURE_VALIDATION_DIR" "$VALIDATION_REFERENCE" \
        "$VALIDATION_PREPOOL" "$VALIDATION_CAPTURE_REPORT" \
        "$VALIDATION_CAPTURE_LAUNCH" "$VALIDATION_CAPTURE_RETURNED" \
        "$VALIDATION_CAPTURE_COMPLETE" "2560" "2816" "256" "2304" "4" \
        "1024" "24576" "8" "$HIST_VALIDATION" "$HIST_VALIDATION_SHA" \
        "$VALIDATION_PROJECTION_SHA"
      ;;
    *) fail "unknown capture split: ${split}"; return 1 ;;
  esac
}

emit_capture_launch() {
  local split="$1" output_dir="$2" begin="$3" end="$4" destination="$5"
  local candidate="${SCRATCH}/capture.${split}.launch.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.capture_launch.v1"
    echo "status=committed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "split=${split}"
    echo "sequence=$([[ "$split" == train ]] && echo 1 || echo 2)"
    echo "argv_count_including_argv0=11"
    echo "argv.0=${CAPTURE_BIN}"
    echo "argv.1=--config"
    echo "argv.2=${CAPTURE_CONFIG}"
    echo "argv.3=--input-representation-checkpoint"
    echo "argv.4=${CHECKPOINT}"
    echo "argv.5=--output-dir"
    echo "argv.6=${output_dir}"
    echo "argv.7=--anchor-index-begin"
    echo "argv.8=${begin}"
    echo "argv.9=--anchor-index-end"
    echo "argv.10=${end}"
    echo "output_dir_absent_at_commit=true"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_capture_launch() {
  local path="$1" split="$2" output_dir="$3" begin="$4" end="$5" item
  load_kv_file "$path" || return 1
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.capture_launch.v1"
    "status=committed"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "split=${split}"
    "sequence=$([[ "$split" == train ]] && echo 1 || echo 2)"
    "argv_count_including_argv0=11"
    "argv.0=${CAPTURE_BIN}"
    "argv.1=--config"
    "argv.2=${CAPTURE_CONFIG}"
    "argv.3=--input-representation-checkpoint"
    "argv.4=${CHECKPOINT}"
    "argv.5=--output-dir"
    "argv.6=${output_dir}"
    "argv.7=--anchor-index-begin"
    "argv.8=${begin}"
    "argv.9=--anchor-index-end"
    "argv.10=${end}"
    "output_dir_absent_at_commit=true"
  )
  [[ "$(wc -l < "$path")" == 19 ]] || fail "capture launch key count mismatch"
  for item in "${contract[@]}"; do
    expect "$path" "${item%%=*}" "${item#*=}" || return 1
  done
}

emit_capture_returned() {
  local split="$1" rc="$2" log="$3" destination="$4"
  local candidate="${SCRATCH}/capture.${split}.returned.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.capture_returned.v1"
    echo "status=returned"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "split=${split}"
    echo "exit_code=${rc}"
    echo "log_path=${log}"
    echo "log_sha256=$(sha256 "$log")"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_capture_returned() {
  local path="$1" split="$2" log="$3" require_zero="${4:-false}" rc
  load_kv_file "$path" || return 1
  [[ "$(wc -l < "$path")" == 8 ]] || fail "capture returned key count mismatch"
  expect "$path" schema_id "${PROTOCOL_ID}.capture_returned.v1"
  expect "$path" status returned
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" split "$split"
  rc="$(kv "$path" exit_code)" || return 1
  [[ "$rc" =~ ^[0-9]+$ ]] || fail "invalid capture return code"
  if [[ "$require_zero" == true ]]; then [[ "$rc" == 0 ]] || fail "capture failed"; fi
  expect "$path" log_path "$log"
  require_exact "$log" "$(kv "$path" log_sha256)" 444
}

validate_capture_report() {
  local report="$1" split="$2" output_dir="$3" reference="$4" prepool="$5"
  local begin="$6" end="$7" anchors="$8" rows="$9" batches="${10}"
  local sample_nodes="${11}" cells="${12}" mdn_calls="${13}" min_tokens max_tokens item
  require_file "$report" 444 || return 1
  [[ "$(wc -l < "$report")" == "$CAPTURE_REPORT_KEY_COUNT" ]] ||
    fail "capture report line count mismatch: ${report}"
  [[ "$(sorted_key_sha256 "$report")" == "$CAPTURE_REPORT_SORTED_KEY_SHA" ]] ||
    fail "capture report key inventory mismatch: ${report}"
  load_kv_file "$report" || return 1
  local -a contract=(
    "schema_id=synthetic_v2_frozen_mtf_prepool_domain_scale_capture.v1"
    "status=complete"
    "config_path=${CAPTURE_CONFIG}"
    "representation_checkpoint_path=${CHECKPOINT}"
    "anchor_range=[${begin},${end})"
    "anchor_count=${anchors}"
    "maximum_anchor_read=$((end-1))"
    "probe_rows=${rows}"
    "source_order_policy=sequential"
    "graph_order_fingerprint=4133db527907a8e4"
    "checkpoint_load_count=1"
    "stream_batch_count=${batches}"
    "encoder_batch_passes=${batches}"
    "encoder_forward_calls=${batches}"
    "each_anchor_encoded_once=true"
    "same_encode_artifact_count=2"
    "sample_node_count=${sample_nodes}"
    "token_count_per_sample_node=72"
    "channel_count=3"
    "domain_count=2"
    "scale_count=4"
    "latent_width=32"
    "window_count.scale_0=7"
    "window_count.scale_1=3"
    "window_count.scale_2=1"
    "window_count.scale_3=1"
    "token_count_per_channel_domain=12"
    "token_count_per_domain=36"
    "summary_groups_per_channel=8"
    "summary_layout=domain_major_scale_minor_latent_minor"
    "summary_group_order=time_s0,time_s1,time_s2,time_s3,frequency_s0,frequency_s1,frequency_s2,frequency_s3"
    "summary_formula=masked_mean_encoded_tokens_by_channel_domain_scale"
    "summary_shape=[M,3,256]"
    "summary_node_channel_width=256"
    "summary_cell_count=${cells}"
    "summary_valid_cell_count=${cells}"
    "summary_all_cells_valid=true"
    "edge_feature_layout=base_256,quote_256,base_minus_quote_256"
    "edge_feature_width=768"
    "probe_file_creation_policy=exclusive"
    "capture_report_creation_policy=exclusive"
    "output.all_tokens_reference.policy=all_tokens"
    "output.all_tokens_reference.probe_path=${reference}"
    "output.all_tokens_reference.probe_rows=${rows}"
    "output.all_tokens_reference.node_channel_width=32"
    "output.all_tokens_reference.edge_feature_width=96"
    "output.prepool_domain_scale.policy=channel_domain_scale_mean"
    "output.prepool_domain_scale.probe_path=${prepool}"
    "output.prepool_domain_scale.probe_rows=${rows}"
    "output.prepool_domain_scale.node_channel_width=256"
    "output.prepool_domain_scale.edge_feature_width=768"
    "mdn_adapter_calls=${mdn_calls}"
    "mdn_model_constructed=false"
    "mdn_checkpoint_access=false"
    "mdn_execution=false"
    "policy_config_parsed_as_inert_dependency=true"
    "policy_model_constructed=false"
    "policy_checkpoint_access=false"
    "policy_execution=false"
    "policy_metric_access=false"
    "optimizer_steps=0"
    "model_parameter_or_buffer_value_mutated_after_checkpoint_load=false"
    "checkpoint_written=false"
  )
  for item in "${contract[@]}"; do
    expect "$report" "${item%%=*}" "${item#*=}" || return 1
  done
  min_tokens="$(kv "$report" minimum_valid_tokens_per_cell)" || return 1
  max_tokens="$(kv "$report" maximum_valid_tokens_per_cell)" || return 1
  [[ "$min_tokens" =~ ^[0-9]+$ && "$max_tokens" =~ ^[0-9]+$ ]] ||
    fail "invalid valid-token extrema"
  (( min_tokens >= 1 && min_tokens <= max_tokens && max_tokens <= 7 )) ||
    fail "valid-token extrema outside preregistered bounds"
  require_file "$reference" 444
  require_file "$prepool" 444
  require_private_dir "$output_dir"
}

emit_capture_complete() {
  local split="$1" report="$2" reference="$3" prepool="$4" rows="$5"
  local returned="$6" destination="$7" candidate="${SCRATCH}/capture.${split}.complete.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.capture_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "split=${split}"
    echo "returned_receipt_path=${returned}"
    echo "returned_receipt_sha256=$(sha256 "$returned")"
    echo "capture_report_path=${report}"
    echo "capture_report_sha256=$(sha256 "$report")"
    echo "all_tokens_reference_path=${reference}"
    echo "all_tokens_reference_sha256=$(sha256 "$reference")"
    echo "prepool_probe_path=${prepool}"
    echo "prepool_probe_sha256=$(sha256 "$prepool")"
    echo "probe_rows=${rows}"
    echo "checkpoint_load_count=1"
    echo "capture_process_completion_count=1"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_capture_complete() {
  local path="$1" split="$2" report="$3" reference="$4" prepool="$5"
  local rows="$6" returned="$7" item
  load_kv_file "$path" || return 1
  [[ "$(wc -l < "$path")" == 16 ]] || fail "capture complete key count mismatch"
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.capture_complete.v1"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "split=${split}"
    "returned_receipt_path=${returned}"
    "returned_receipt_sha256=$(sha256 "$returned")"
    "capture_report_path=${report}"
    "capture_report_sha256=$(sha256 "$report")"
    "all_tokens_reference_path=${reference}"
    "all_tokens_reference_sha256=$(sha256 "$reference")"
    "prepool_probe_path=${prepool}"
    "prepool_probe_sha256=$(sha256 "$prepool")"
    "probe_rows=${rows}"
    "checkpoint_load_count=1"
    "capture_process_completion_count=1"
  )
  for item in "${contract[@]}"; do
    expect "$path" "${item%%=*}" "${item#*=}" || return 1
  done
  require_exact "$report" "$(kv "$path" capture_report_sha256)" 444
  require_exact "$reference" "$(kv "$path" all_tokens_reference_sha256)" 444
  require_exact "$prepool" "$(kv "$path" prepool_probe_sha256)" 444
}

run_capture_split() {
  local split="$1" -a p
  mapfile -t p < <(capture_paths "$split")
  local output_dir="${p[0]}" reference="${p[1]}" prepool="${p[2]}"
  local report="${p[3]}" launch="${p[4]}" returned="${p[5]}" complete="${p[6]}"
  local begin="${p[7]}" end="${p[8]}" anchors="${p[9]}" rows="${p[10]}"
  local batches="${p[11]}" sample_nodes="${p[12]}" cells="${p[13]}"
  local mdn_calls="${p[14]}" log="${CAPTURE_EVIDENCE}/${split}.log"
  local log_candidate="${SCRATCH}/capture.${split}.log.candidate" rc count
  [[ ! -e "$output_dir" && ! -L "$output_dir" && ! -e "$log" && ! -L "$log" ]] ||
    fail "capture split output is not pristine: ${split}"
  emit_capture_launch "$split" "$output_dir" "$begin" "$end" "$launch"
  validate_capture_launch "$launch" "$split" "$output_dir" "$begin" "$end"
  set +e
  "$CAPTURE_BIN" --config "$CAPTURE_CONFIG" \
    --input-representation-checkpoint "$CHECKPOINT" \
    --output-dir "$output_dir" --anchor-index-begin "$begin" \
    --anchor-index-end "$end" >"$log_candidate" 2>&1
  rc=$?
  set -e
  publish_once "$log_candidate" "$log" 0444
  emit_capture_returned "$split" "$rc" "$log" "$returned"
  validate_capture_returned "$returned" "$split" "$log" false
  (( rc == 0 )) || fail "capture ${split} returned ${rc}"
  require_private_dir "$output_dir"
  count="$(find "$output_dir" -mindepth 1 -maxdepth 1 -type f | wc -l | tr -d ' ')" || return 1
  [[ "$count" == 3 && -z "$(find "$output_dir" -mindepth 1 -maxdepth 1 \! -type f -print -quit)" ]] ||
    fail "capture ${split} did not emit exactly three regular files"
  chmod 0444 -- "$reference" "$prepool" "$report"
  validate_capture_returned "$returned" "$split" "$log" true
  validate_capture_report "$report" "$split" "$output_dir" "$reference" "$prepool" \
    "$begin" "$end" "$anchors" "$rows" "$batches" "$sample_nodes" "$cells" "$mdn_calls"
  emit_capture_complete "$split" "$report" "$reference" "$prepool" "$rows" \
    "$returned" "$complete"
  validate_capture_complete "$complete" "$split" "$report" "$reference" "$prepool" \
    "$rows" "$returned"
}

projection_digest() {
  tail -n +2 -- "$1" | cut -d, -f2-10 | sha256sum | cut -d' ' -f1
}

emit_projection_complete() {
  local candidate="${SCRATCH}/projection.complete.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.projection_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "train_capture_complete_sha256=$(sha256 "$TRAIN_CAPTURE_COMPLETE")"
    echo "validation_capture_complete_sha256=$(sha256 "$VALIDATION_CAPTURE_COMPLETE")"
    echo "historical_train_path=${HIST_TRAIN}"
    echo "historical_train_sha256=${HIST_TRAIN_SHA}"
    echo "train_reference_path=${TRAIN_REFERENCE}"
    echo "train_reference_sha256=$(sha256 "$TRAIN_REFERENCE")"
    echo "train_prepool_path=${TRAIN_PREPOOL}"
    echo "train_prepool_sha256=$(sha256 "$TRAIN_PREPOOL")"
    echo "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
    echo "historical_validation_path=${HIST_VALIDATION}"
    echo "historical_validation_sha256=${HIST_VALIDATION_SHA}"
    echo "validation_reference_path=${VALIDATION_REFERENCE}"
    echo "validation_reference_sha256=$(sha256 "$VALIDATION_REFERENCE")"
    echo "validation_prepool_path=${VALIDATION_PREPOOL}"
    echo "validation_prepool_sha256=$(sha256 "$VALIDATION_PREPOOL")"
    echo "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
    echo "historical_byte_parity_check_count=2"
    echo "projection_check_count=6"
    echo "all_checks_pass=true"
    echo "projection_payload_persisted=false"
    echo "evaluator_invocations_started=0"
  } >"$candidate"
  publish_once "$candidate" "$PROJECTION_COMPLETE" 0444
}

validate_projection_complete() {
  local item path digest
  load_kv_file "$PROJECTION_COMPLETE" || return 1
  [[ "$(wc -l < "$PROJECTION_COMPLETE")" == 25 ]] ||
    fail "projection complete key count mismatch"
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.projection_complete.v1"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "train_capture_complete_sha256=$(sha256 "$TRAIN_CAPTURE_COMPLETE")"
    "validation_capture_complete_sha256=$(sha256 "$VALIDATION_CAPTURE_COMPLETE")"
    "historical_train_path=${HIST_TRAIN}"
    "historical_train_sha256=${HIST_TRAIN_SHA}"
    "train_reference_path=${TRAIN_REFERENCE}"
    "train_reference_sha256=$(sha256 "$TRAIN_REFERENCE")"
    "train_prepool_path=${TRAIN_PREPOOL}"
    "train_prepool_sha256=$(sha256 "$TRAIN_PREPOOL")"
    "train_projection_sha256=${TRAIN_PROJECTION_SHA}"
    "historical_validation_path=${HIST_VALIDATION}"
    "historical_validation_sha256=${HIST_VALIDATION_SHA}"
    "validation_reference_path=${VALIDATION_REFERENCE}"
    "validation_reference_sha256=$(sha256 "$VALIDATION_REFERENCE")"
    "validation_prepool_path=${VALIDATION_PREPOOL}"
    "validation_prepool_sha256=$(sha256 "$VALIDATION_PREPOOL")"
    "validation_projection_sha256=${VALIDATION_PROJECTION_SHA}"
    "historical_byte_parity_check_count=2"
    "projection_check_count=6"
    "all_checks_pass=true"
    "projection_payload_persisted=false"
    "evaluator_invocations_started=0"
  )
  for item in "${contract[@]}"; do
    expect "$PROJECTION_COMPLETE" "${item%%=*}" "${item#*=}" || return 1
  done
  cmp -s -- "$HIST_TRAIN" "$TRAIN_REFERENCE" ||
    fail "train all-token reference differs from historical authority"
  cmp -s -- "$HIST_VALIDATION" "$VALIDATION_REFERENCE" ||
    fail "validation all-token reference differs from historical authority"
  for path in "$HIST_TRAIN" "$TRAIN_REFERENCE" "$TRAIN_PREPOOL"; do
    digest="$(projection_digest "$path")" || return 1
    [[ "$digest" == "$TRAIN_PROJECTION_SHA" ]] || fail "train projection mismatch: ${path}"
  done
  for path in "$HIST_VALIDATION" "$VALIDATION_REFERENCE" "$VALIDATION_PREPOOL"; do
    digest="$(projection_digest "$path")" || return 1
    [[ "$digest" == "$VALIDATION_PROJECTION_SHA" ]] ||
      fail "validation projection mismatch: ${path}"
  done
}

run_projection_stage() {
  validate_capture_complete "$TRAIN_CAPTURE_COMPLETE" train "$TRAIN_CAPTURE_REPORT" \
    "$TRAIN_REFERENCE" "$TRAIN_PREPOOL" 22464 "$TRAIN_CAPTURE_RETURNED"
  validate_capture_complete "$VALIDATION_CAPTURE_COMPLETE" validation \
    "$VALIDATION_CAPTURE_REPORT" "$VALIDATION_REFERENCE" "$VALIDATION_PREPOOL" \
    2304 "$VALIDATION_CAPTURE_RETURNED"
  require_exact "$HIST_TRAIN" "$HIST_TRAIN_SHA" 444
  require_exact "$HIST_VALIDATION" "$HIST_VALIDATION_SHA" 444
  cmp -s -- "$HIST_TRAIN" "$TRAIN_REFERENCE" ||
    fail "train same-encode all-token byte parity failed"
  cmp -s -- "$HIST_VALIDATION" "$VALIDATION_REFERENCE" ||
    fail "validation same-encode all-token byte parity failed"
  local path
  for path in "$HIST_TRAIN" "$TRAIN_REFERENCE" "$TRAIN_PREPOOL"; do
    [[ "$(projection_digest "$path")" == "$TRAIN_PROJECTION_SHA" ]] ||
      fail "train coordinate/target projection mismatch"
  done
  for path in "$HIST_VALIDATION" "$VALIDATION_REFERENCE" "$VALIDATION_PREPOOL"; do
    [[ "$(projection_digest "$path")" == "$VALIDATION_PROJECTION_SHA" ]] ||
      fail "validation coordinate/target projection mismatch"
  done
  emit_projection_complete
  validate_projection_complete
}

validate_metric_group() {
  local file="$1" prefix="$2" expected_count="$3" key value
  expect "$file" "${prefix}.count" "$expected_count" || return 1
  expect "$file" "${prefix}.pairwise_rank_count" "$expected_count" || return 1
  for key in mae rmse target_rms prediction_rms rmse_target_rms_ratio; do
    value="$(kv "$file" "${prefix}.${key}")" || return 1
    finite_number "$value" && number_ge "$value" 0 ||
      fail "invalid nonnegative metric ${prefix}.${key}"
  done
  number_ge "$(kv "$file" "${prefix}.target_rms")" 1e-18 ||
    fail "target RMS is not positive: ${prefix}"
  for key in directional_accuracy pairwise_rank_accuracy best_asset_agreement; do
    value="$(kv "$file" "${prefix}.${key}")" || return 1
    finite_number "$value" && number_ge "$value" 0 && number_le "$value" 1 ||
      fail "metric outside [0,1]: ${prefix}.${key}"
  done
  value="$(kv "$file" "${prefix}.correlation")" || return 1
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

validate_evaluator_report() {
  local file="$1" index key prefix best selected strong=false partial=false item
  local -a ridges=(1e-12 1e-10 1e-8 1e-6 1e-4 1e-2)
  require_file "$file" 444 || return 1
  [[ "$(wc -l < "$file")" == "$EVALUATOR_REPORT_KEY_COUNT" ]] ||
    fail "evaluator report line count mismatch: ${file}"
  [[ "$(sorted_key_sha256 "$file")" == "$EVALUATOR_REPORT_SORTED_KEY_SHA" ]] ||
    fail "evaluator report key inventory mismatch: ${file}"
  load_kv_file "$file" || return 1
  local -a contract=(
    "schema_id=synthetic_v2_frozen_mtf_prepool_domain_scale_channel_conditioned_affine_development_v1"
    "status=complete"
    "benchmark_id=synthetic_continuous_graph_v2"
    "diagnostic_phase=prepool_domain_scale"
    "diagnostic_authority=development_only"
    "benchmark_acceptance_authority=false"
    "phase2a_nine_head_authority_sha256=${PHASE2A_SOURCE_SHA}"
    "frozen_parser_authority_sha256=${CANONICAL_PARSER_SHA}"
    "probe_kind=prepool_domain_scale"
    "probe_record_schema=kikijyeba.synthetic.representation_edge_feature_probe.v1"
    "train_probe_rows=22464"
    "validation_probe_rows=2304"
    "certified_probe_rows=0"
    "probe_rows_total=24768"
    "probe_ranges_disjoint=true"
    "fit_anchor_range=[0,2496)"
    "validation_anchor_range=[2560,2816)"
    "certified_anchor_range=not_opened"
    "purge_anchors_used=false"
    "maximum_anchor_read=2815"
    "final_holdout_begin=3328"
    "final_holdout_access=false"
    "policy_access=false"
    "refit_after_selection=false"
    "certified_candidates_scored=0"
    "feature_layout=base_256,quote_256,base_minus_quote_256"
    "probe_feature_width=768"
    "affine_feature_width=768"
    "edge_identity_feature_width_excluded=0"
    "summary_domain_count=2"
    "summary_scale_count=4"
    "summary_latent_width=32"
    "summary_group_count_per_channel=8"
    "summary_layout=domain_major_scale_minor_latent_minor"
    "fit_structure=one_weight_row_and_bias_per_edge_and_channel"
    "conditioned_head_count=9"
    "standardization_scope=train_core_all_edges_all_channels"
    "solver=float64_centered_cholesky_ridge"
    "ridge_scaling=gram_diagonal_plus_edge_channel_sample_count_times_alpha"
    "ridge_grid=1e-12,1e-10,1e-8,1e-6,1e-4,1e-2"
    "selection_scope=one_global_candidate_for_all_edge_channel_heads"
    "selection_order=validation_direction,validation_rank,validation_correlation,validation_rmse,smallest_alpha"
    "cached_ridge_invariant_systems=true"
    "ridge_invariant_system_build_count=9"
    "ridge_fit_attempt_count=6"
    "cholesky_factorization_count=54"
    "cholesky_solve_count=54"
    "maximum_cholesky_factorization_count=54"
    "maximum_cholesky_solve_count=54"
    "exclusive_output_creation=true"
    "numerically_valid_candidate_count=6"
  )
  for item in "${contract[@]}"; do
    expect "$file" "${item%%=*}" "${item#*=}" || return 1
  done
  numeric_equal "$(kv "$file" selection_tie_tolerance)" "$TIE_TOLERANCE" ||
    fail "selection tie tolerance mismatch"
  number_ge "$(kv "$file" context_identity_max_abs_delta)" 0 &&
    number_le "$(kv "$file" context_identity_max_abs_delta)" 2e-6 ||
    fail "context identity delta outside frozen parser tolerance"

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
      fail "candidate normalized residual invalid at ${index}"
    finite_number "$(kv "$file" "${prefix}.coefficient_l2_norm")" &&
      number_ge "$(kv "$file" "${prefix}.coefficient_l2_norm")" 0 ||
      fail "candidate coefficient norm invalid at ${index}"
    validate_metric_group "$file" "${prefix}.validation" 2304
    if (( index > 0 )) && candidate_better "$file" "$index" "$best"; then
      best="$index"
    fi
  done

  selected="$(kv "$file" selected_candidate_index)" || return 1
  [[ "$selected" == "$best" ]] || fail "selected candidate violates frozen comparator"
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

  if number_ge "$(kv "$file" selected.validation.directional_accuracy)" 0.95 &&
     number_ge "$(kv "$file" selected.validation.pairwise_rank_accuracy)" 0.95 &&
     number_ge "$(kv "$file" selected.validation.correlation)" 0.95 &&
     number_le "$(kv "$file" selected.validation.rmse_target_rms_ratio)" 0.25; then
    strong=true
  fi
  if number_ge "$(kv "$file" selected.validation.directional_accuracy)" 0.80 &&
     number_ge "$(kv "$file" selected.validation.pairwise_rank_accuracy)" 0.78; then
    partial=true
  fi
  expect "$file" validation_strong_gate_pass "$strong"
  expect "$file" certified_strong_gate_pass not_evaluated
  expect "$file" validation_partial_gate_pass "$partial"
  expect "$file" certified_partial_gate_pass not_evaluated
  expect "$file" rung_b_authorized false
  if [[ "$strong" == true ]]; then
    expect "$file" classification "$PASS_CLASS"
  else
    expect "$file" classification "$NO_PASS_CLASS"
  fi
  expect "$file" preregistered_strong_gate \
    'direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25'
  expect "$file" preregistered_partial_gate 'direction>=0.80,rank>=0.78'
}

emit_evaluator_launch() {
  local lane="$1" output="$2" destination="$3"
  local candidate="${SCRATCH}/affine.${lane}.launch.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.evaluator_launch.v1"
    echo "status=committed"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "lane=${lane}"
    echo "sequence=$([[ "$lane" == main ]] && echo 3 || echo 4)"
    echo "omp_num_threads=1"
    echo "mkl_num_threads=1"
    echo "argv_count_including_argv0=10"
    echo "argv.0=${EVALUATOR_BIN}"
    echo "argv.1=--probe-kind"
    echo "argv.2=prepool_domain_scale"
    echo "argv.3=--development-only"
    echo "argv.4=--train-input"
    echo "argv.5=${TRAIN_PREPOOL}"
    echo "argv.6=--validation-input"
    echo "argv.7=${VALIDATION_PREPOOL}"
    echo "argv.8=--output"
    echo "argv.9=${output}"
    echo "output_absent_at_commit=true"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_evaluator_launch() {
  local path="$1" lane="$2" output="$3" item
  load_kv_file "$path" || return 1
  [[ "$(wc -l < "$path")" == 21 ]] || fail "evaluator launch key count mismatch"
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.evaluator_launch.v1"
    "status=committed"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    "lane=${lane}"
    "sequence=$([[ "$lane" == main ]] && echo 3 || echo 4)"
    "omp_num_threads=1"
    "mkl_num_threads=1"
    "argv_count_including_argv0=10"
    "argv.0=${EVALUATOR_BIN}"
    "argv.1=--probe-kind"
    "argv.2=prepool_domain_scale"
    "argv.3=--development-only"
    "argv.4=--train-input"
    "argv.5=${TRAIN_PREPOOL}"
    "argv.6=--validation-input"
    "argv.7=${VALIDATION_PREPOOL}"
    "argv.8=--output"
    "argv.9=${output}"
    "output_absent_at_commit=true"
  )
  for item in "${contract[@]}"; do
    expect "$path" "${item%%=*}" "${item#*=}" || return 1
  done
}

emit_evaluator_returned() {
  local lane="$1" rc="$2" log="$3" destination="$4"
  local candidate="${SCRATCH}/affine.${lane}.returned.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.evaluator_returned.v1"
    echo "status=returned"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "lane=${lane}"
    echo "exit_code=${rc}"
    echo "log_path=${log}"
    echo "log_sha256=$(sha256 "$log")"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_evaluator_returned() {
  local path="$1" lane="$2" log="$3" require_zero="${4:-false}" rc
  load_kv_file "$path" || return 1
  [[ "$(wc -l < "$path")" == 8 ]] || fail "evaluator returned key count mismatch"
  expect "$path" schema_id "${PROTOCOL_ID}.evaluator_returned.v1"
  expect "$path" status returned
  expect "$path" protocol_id "$PROTOCOL_ID"
  expect "$path" attempt_sha256 "$(sha256 "$ATTEMPT")"
  expect "$path" lane "$lane"
  rc="$(kv "$path" exit_code)" || return 1
  [[ "$rc" =~ ^[0-9]+$ ]] || fail "invalid evaluator return code"
  if [[ "$require_zero" == true ]]; then [[ "$rc" == 0 ]] || fail "evaluator failed"; fi
  expect "$path" log_path "$log"
  require_exact "$log" "$(kv "$path" log_sha256)" 444
}

emit_evaluator_complete() {
  local lane="$1" report="$2" returned="$3" destination="$4"
  local candidate="${SCRATCH}/affine.${lane}.complete.status.candidate"
  {
    echo "schema_id=${PROTOCOL_ID}.evaluator_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "lane=${lane}"
    echo "returned_receipt_path=${returned}"
    echo "returned_receipt_sha256=$(sha256 "$returned")"
    echo "report_path=${report}"
    echo "report_sha256=$(sha256 "$report")"
    echo "selected_candidate_index=$(kv "$report" selected_candidate_index)"
    echo "selected_ridge=$(kv "$report" selected_ridge)"
    echo "validation_direction=$(kv "$report" selected.validation.directional_accuracy)"
    echo "validation_rank=$(kv "$report" selected.validation.pairwise_rank_accuracy)"
    echo "validation_correlation=$(kv "$report" selected.validation.correlation)"
    echo "validation_rmse=$(kv "$report" selected.validation.rmse)"
    echo "validation_rmse_target_rms_ratio=$(kv "$report" selected.validation.rmse_target_rms_ratio)"
    echo "validation_strong_gate_pass=$(kv "$report" validation_strong_gate_pass)"
    echo "validation_partial_gate_pass=$(kv "$report" validation_partial_gate_pass)"
    echo "classification=$(kv "$report" classification)"
    echo "ridge_invariant_system_build_count=9"
    echo "ridge_fit_attempt_count=6"
    echo "cholesky_factorization_count=54"
    echo "cholesky_solve_count=54"
    echo "conditioned_head_solve_count=54"
  } >"$candidate"
  publish_once "$candidate" "$destination" 0444
}

validate_evaluator_complete() {
  local path="$1" lane="$2" report="$3" returned="$4" item
  load_kv_file "$path" || return 1
  [[ "$(wc -l < "$path")" == 24 ]] || fail "evaluator complete key count mismatch"
  validate_evaluator_report "$report" || return 1
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.evaluator_complete.v1"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "lane=${lane}"
    "returned_receipt_path=${returned}"
    "returned_receipt_sha256=$(sha256 "$returned")"
    "report_path=${report}"
    "report_sha256=$(sha256 "$report")"
    "selected_candidate_index=$(kv "$report" selected_candidate_index)"
    "selected_ridge=$(kv "$report" selected_ridge)"
    "validation_direction=$(kv "$report" selected.validation.directional_accuracy)"
    "validation_rank=$(kv "$report" selected.validation.pairwise_rank_accuracy)"
    "validation_correlation=$(kv "$report" selected.validation.correlation)"
    "validation_rmse=$(kv "$report" selected.validation.rmse)"
    "validation_rmse_target_rms_ratio=$(kv "$report" selected.validation.rmse_target_rms_ratio)"
    "validation_strong_gate_pass=$(kv "$report" validation_strong_gate_pass)"
    "validation_partial_gate_pass=$(kv "$report" validation_partial_gate_pass)"
    "classification=$(kv "$report" classification)"
    "ridge_invariant_system_build_count=9"
    "ridge_fit_attempt_count=6"
    "cholesky_factorization_count=54"
    "cholesky_solve_count=54"
    "conditioned_head_solve_count=54"
  )
  for item in "${contract[@]}"; do
    expect "$path" "${item%%=*}" "${item#*=}" || return 1
  done
}

run_evaluator_lane() {
  local lane="$1" report launch returned complete outdir log log_candidate rc
  if [[ "$lane" == main ]]; then
    report="$MAIN_REPORT"; launch="$MAIN_LAUNCH"; returned="$MAIN_RETURNED"
    complete="$MAIN_COMPLETE"; outdir="$AFFINE_MAIN_DIR"
  else
    report="$REPLAY_REPORT"; launch="$REPLAY_LAUNCH"; returned="$REPLAY_RETURNED"
    complete="$REPLAY_COMPLETE"; outdir="$AFFINE_REPLAY_DIR"
  fi
  log="${AFFINE_EVIDENCE}/${lane}.log"
  log_candidate="${SCRATCH}/affine.${lane}.log.candidate"
  [[ ! -e "$report" && ! -L "$report" && ! -e "$log" && ! -L "$log" ]] ||
    fail "evaluator lane is not pristine: ${lane}"
  require_private_dir "$outdir"
  emit_evaluator_launch "$lane" "$report" "$launch"
  validate_evaluator_launch "$launch" "$lane" "$report"
  set +e
  OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 "$EVALUATOR_BIN" \
    --probe-kind prepool_domain_scale --development-only \
    --train-input "$TRAIN_PREPOOL" --validation-input "$VALIDATION_PREPOOL" \
    --output "$report" >"$log_candidate" 2>&1
  rc=$?
  set -e
  publish_once "$log_candidate" "$log" 0444
  emit_evaluator_returned "$lane" "$rc" "$log" "$returned"
  validate_evaluator_returned "$returned" "$lane" "$log" false
  (( rc == 0 )) || fail "evaluator ${lane} returned ${rc}"
  chmod 0444 -- "$report"
  validate_evaluator_returned "$returned" "$lane" "$log" true
  validate_evaluator_report "$report"
  emit_evaluator_complete "$lane" "$report" "$returned" "$complete"
  validate_evaluator_complete "$complete" "$lane" "$report" "$returned"
}

emit_science_complete() {
  local classification candidate="${SCRATCH}/science.complete.status.candidate"
  classification="$(kv "$MAIN_REPORT" classification)"
  {
    echo "schema_id=${PROTOCOL_ID}.science_complete.v1"
    echo "status=complete"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "projection_complete_path=${PROJECTION_COMPLETE}"
    echo "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    echo "train_capture_complete_sha256=$(sha256 "$TRAIN_CAPTURE_COMPLETE")"
    echo "validation_capture_complete_sha256=$(sha256 "$VALIDATION_CAPTURE_COMPLETE")"
    echo "main_complete_sha256=$(sha256 "$MAIN_COMPLETE")"
    echo "replay_complete_sha256=$(sha256 "$REPLAY_COMPLETE")"
    echo "main_report_path=${MAIN_REPORT}"
    echo "main_report_sha256=$(sha256 "$MAIN_REPORT")"
    echo "replay_report_path=${REPLAY_REPORT}"
    echo "replay_report_sha256=$(sha256 "$REPLAY_REPORT")"
    echo "main_replay_byte_identical=true"
    echo "classification=${classification}"
    echo "validation_strong_gate_pass=$(kv "$MAIN_REPORT" validation_strong_gate_pass)"
    echo "validation_partial_gate_pass=$(kv "$MAIN_REPORT" validation_partial_gate_pass)"
    echo "selected_candidate_index=$(kv "$MAIN_REPORT" selected_candidate_index)"
    echo "selected_ridge=$(kv "$MAIN_REPORT" selected_ridge)"
    echo "capture_process_invocations_completed=2"
    echo "capture_replay_invocations=0"
    echo "checkpoint_load_count=2"
    echo "encoder_forward_calls=43"
    echo "encoder_anchor_participations=2752"
    echo "capture_probe_artifact_count=4"
    echo "capture_report_count=2"
    echo "historical_byte_parity_check_count=2"
    echo "projection_check_count=6"
    echo "evaluator_invocations_completed=2"
    echo "validated_evaluator_report_count=2"
    echo "main_replay_parity_check_count=1"
    echo "ridge_invariant_system_build_count=18"
    echo "analytic_ridge_fit_attempt_count=12"
    echo "cholesky_factorization_count=108"
    echo "cholesky_solve_count=108"
    echo "conditioned_head_solve_count=108"
    echo "optimizer_fit_count=0"
    echo "optimizer_step_count=0"
    echo "refit_count=0"
    echo "retry_count=0"
    echo "early_stop_count=0"
    echo "scientific_execution_completed=true"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_model_access=false"
    echo "mdn_model_access=false"
    echo "checkpoint_written=false"
    echo "maximum_anchor_read=2815"
  } >"$candidate"
  publish_once "$candidate" "$SCIENCE_COMPLETE" 0444
}

validate_science_complete() {
  local file="${1:-$SCIENCE_COMPLETE}" item
  load_kv_file "$file" || return 1
  [[ "$(wc -l < "$file")" == 49 ]] || fail "science receipt key count mismatch"
  validate_projection_complete || return 1
  validate_capture_complete "$TRAIN_CAPTURE_COMPLETE" train "$TRAIN_CAPTURE_REPORT" \
    "$TRAIN_REFERENCE" "$TRAIN_PREPOOL" 22464 "$TRAIN_CAPTURE_RETURNED"
  validate_capture_complete "$VALIDATION_CAPTURE_COMPLETE" validation \
    "$VALIDATION_CAPTURE_REPORT" "$VALIDATION_REFERENCE" "$VALIDATION_PREPOOL" \
    2304 "$VALIDATION_CAPTURE_RETURNED"
  validate_evaluator_complete "$MAIN_COMPLETE" main "$MAIN_REPORT" "$MAIN_RETURNED"
  validate_evaluator_complete "$REPLAY_COMPLETE" replay "$REPLAY_REPORT" "$REPLAY_RETURNED"
  cmp -s -- "$MAIN_REPORT" "$REPLAY_REPORT" ||
    fail "main/replay evaluator reports differ"
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.science_complete.v1"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "projection_complete_path=${PROJECTION_COMPLETE}"
    "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    "train_capture_complete_sha256=$(sha256 "$TRAIN_CAPTURE_COMPLETE")"
    "validation_capture_complete_sha256=$(sha256 "$VALIDATION_CAPTURE_COMPLETE")"
    "main_complete_sha256=$(sha256 "$MAIN_COMPLETE")"
    "replay_complete_sha256=$(sha256 "$REPLAY_COMPLETE")"
    "main_report_path=${MAIN_REPORT}"
    "main_report_sha256=$(sha256 "$MAIN_REPORT")"
    "replay_report_path=${REPLAY_REPORT}"
    "replay_report_sha256=$(sha256 "$REPLAY_REPORT")"
    "main_replay_byte_identical=true"
    "classification=$(kv "$MAIN_REPORT" classification)"
    "validation_strong_gate_pass=$(kv "$MAIN_REPORT" validation_strong_gate_pass)"
    "validation_partial_gate_pass=$(kv "$MAIN_REPORT" validation_partial_gate_pass)"
    "selected_candidate_index=$(kv "$MAIN_REPORT" selected_candidate_index)"
    "selected_ridge=$(kv "$MAIN_REPORT" selected_ridge)"
    "capture_process_invocations_completed=2"
    "capture_replay_invocations=0"
    "checkpoint_load_count=2"
    "encoder_forward_calls=43"
    "encoder_anchor_participations=2752"
    "capture_probe_artifact_count=4"
    "capture_report_count=2"
    "historical_byte_parity_check_count=2"
    "projection_check_count=6"
    "evaluator_invocations_completed=2"
    "validated_evaluator_report_count=2"
    "main_replay_parity_check_count=1"
    "ridge_invariant_system_build_count=18"
    "analytic_ridge_fit_attempt_count=12"
    "cholesky_factorization_count=108"
    "cholesky_solve_count=108"
    "conditioned_head_solve_count=108"
    "optimizer_fit_count=0"
    "optimizer_step_count=0"
    "refit_count=0"
    "retry_count=0"
    "early_stop_count=0"
    "scientific_execution_completed=true"
    "certified_input_access=false"
    "final_holdout_access=false"
    "policy_model_access=false"
    "mdn_model_access=false"
    "checkpoint_written=false"
    "maximum_anchor_read=2815"
  )
  for item in "${contract[@]}"; do
    expect "$file" "${item%%=*}" "${item#*=}" || return 1
  done
}

emit_result_body() {
  cat <<EOF
schema_id=${PROTOCOL_ID}
status=complete
protocol_id=${PROTOCOL_ID}
benchmark_id=synthetic_continuous_graph_v2
project_goal=Project Clear Signal - Test the Frozen Pre-Pool Domain-by-Scale Surface
diagnostic_phase=${DIAGNOSTIC_PHASE}
diagnostic_authority=development_only
benchmark_acceptance_authority=false
certified_authorization_eligible=false
classification=$(kv "$SCIENCE_COMPLETE" classification)
preregistration_path=${PREREG}
preregistration_sha256=${PREREG_SHA}
runner_path=${RUNNER}
runner_sha256=$(sha256 "$RUNNER")
attempt_path=${ATTEMPT}
attempt_sha256=$(sha256 "$ATTEMPT")
preparation_receipt_path=${PREP_RECEIPT}
preparation_receipt_sha256=$(sha256 "$PREP_RECEIPT")
science_complete_path=${SCIENCE_COMPLETE}
science_complete_sha256=$(sha256 "$SCIENCE_COMPLETE")
projection_complete_path=${PROJECTION_COMPLETE}
projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")
predecessor_v2_result_path=${V2_RESULT}
predecessor_v2_result_sha256=${V2_RESULT_SHA}
predecessor_v2_strong_gate_pass_count=0
capture_source_path=${CAPTURE_SOURCE}
capture_source_sha256=${CAPTURE_SOURCE_SHA}
capture_wrapper_path=${CAPTURE_WRAPPER}
capture_wrapper_sha256=${CAPTURE_WRAPPER_SHA}
capture_binary_path=${CAPTURE_BIN}
capture_binary_sha256=$(sha256 "$CAPTURE_BIN")
evaluator_source_path=${EVALUATOR_SOURCE}
evaluator_source_sha256=${EVALUATOR_SOURCE_SHA}
evaluator_wrapper_path=${EVALUATOR_WRAPPER}
evaluator_wrapper_sha256=${EVALUATOR_WRAPPER_SHA}
evaluator_binary_path=${EVALUATOR_BIN}
evaluator_binary_sha256=$(sha256 "$EVALUATOR_BIN")
phase2a_nine_head_authority_sha256=${PHASE2A_SOURCE_SHA}
frozen_parser_authority_sha256=${CANONICAL_PARSER_SHA}
canonical_result_path=${CANONICAL_RESULT}
canonical_result_sha256=${CANONICAL_RESULT_SHA}
canonical_inputs_path=${CANONICAL_INPUTS}
canonical_inputs_sha256=${CANONICAL_INPUTS_SHA}
source_closure_path=${SOURCE_CLOSURE}
source_closure_sha256=${SOURCE_CLOSURE_SHA}
cursor_erratum_path=${CURSOR_ERRATUM}
cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}
source_manifest_path=${SOURCE_MANIFEST}
source_manifest_sha256=${SOURCE_MANIFEST_SHA}
capture_config_path=${CAPTURE_CONFIG}
capture_config_sha256=${CAPTURE_CONFIG_SHA}
checkpoint_path=${CHECKPOINT}
checkpoint_sha256=${CHECKPOINT_SHA}
historical_train_probe_path=${HIST_TRAIN}
historical_train_probe_sha256=${HIST_TRAIN_SHA}
historical_validation_probe_path=${HIST_VALIDATION}
historical_validation_probe_sha256=${HIST_VALIDATION_SHA}
train_capture_report_path=${TRAIN_CAPTURE_REPORT}
train_capture_report_sha256=$(sha256 "$TRAIN_CAPTURE_REPORT")
validation_capture_report_path=${VALIDATION_CAPTURE_REPORT}
validation_capture_report_sha256=$(sha256 "$VALIDATION_CAPTURE_REPORT")
train_all_tokens_reference_path=${TRAIN_REFERENCE}
train_all_tokens_reference_sha256=$(sha256 "$TRAIN_REFERENCE")
validation_all_tokens_reference_path=${VALIDATION_REFERENCE}
validation_all_tokens_reference_sha256=$(sha256 "$VALIDATION_REFERENCE")
train_prepool_probe_path=${TRAIN_PREPOOL}
train_prepool_probe_sha256=$(sha256 "$TRAIN_PREPOOL")
validation_prepool_probe_path=${VALIDATION_PREPOOL}
validation_prepool_probe_sha256=$(sha256 "$VALIDATION_PREPOOL")
main_report_path=${MAIN_REPORT}
main_report_sha256=$(sha256 "$MAIN_REPORT")
replay_report_path=${REPLAY_REPORT}
replay_report_sha256=$(sha256 "$REPLAY_REPORT")
main_replay_byte_identical=true
capture_report_key_count=${CAPTURE_REPORT_KEY_COUNT}
capture_report_sorted_key_sha256=${CAPTURE_REPORT_SORTED_KEY_SHA}
evaluator_report_key_count=${EVALUATOR_REPORT_KEY_COUNT}
evaluator_report_sorted_key_sha256=${EVALUATOR_REPORT_SORTED_KEY_SHA}
selected_candidate_index=$(kv "$MAIN_REPORT" selected_candidate_index)
selected_ridge=$(kv "$MAIN_REPORT" selected_ridge)
selected_validation_direction=$(kv "$MAIN_REPORT" selected.validation.directional_accuracy)
selected_validation_rank=$(kv "$MAIN_REPORT" selected.validation.pairwise_rank_accuracy)
selected_validation_correlation=$(kv "$MAIN_REPORT" selected.validation.correlation)
selected_validation_rmse=$(kv "$MAIN_REPORT" selected.validation.rmse)
selected_validation_rmse_target_rms_ratio=$(kv "$MAIN_REPORT" selected.validation.rmse_target_rms_ratio)
validation_strong_gate_pass=$(kv "$MAIN_REPORT" validation_strong_gate_pass)
validation_partial_gate_pass=$(kv "$MAIN_REPORT" validation_partial_gate_pass)
original_strong_gate=direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25
original_partial_gate=direction>=0.80,rank>=0.78
capture_process_invocations=2
capture_process_completions=2
capture_replay_invocations=0
checkpoint_load_count=2
encoder_forward_calls=43
encoder_anchor_participations=2752
capture_probe_artifact_count=4
capture_report_count=2
historical_byte_parity_check_count=2
projection_check_count=6
evaluator_invocations=2
evaluator_completions=2
validated_evaluator_report_count=2
main_replay_parity_check_count=1
ridge_invariant_system_build_count=18
analytic_ridge_fit_attempt_count=12
cholesky_factorization_count=108
cholesky_solve_count=108
conditioned_head_solve_count=108
optimizer_fit_count=0
optimizer_step_count=0
refit_count=0
retry_count=0
early_stop_count=0
automatic_retry=false
same_protocol_retry=false
same_protocol_resume=false
capture_replay=false
checkpoint_written=false
certified_input_access=false
final_holdout_access=false
policy_config_parsed_as_inert_dependency=true
policy_model_access=false
policy_execution=false
mdn_model_access=false
mdn_execution=false
maximum_anchor_read=2815
train_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
causal_reading_strong=current_cross_domain_cross_scale_serving_mean_is_exposed_information_loss_boundary_under_frozen_lineage
causal_reading_non_strong=closes_only_fixed_domain_scale_masked_mean_affine_surface
within_cell_window_order_preserved=false
downstream_head_nonlinear=false
successful_result_authorizes_certified_or_production=false
full_transitive_system_toolchain_or_elf_closure=false
scientific_execution_completed=true
scientific_result_available=true
EOF
}

validate_result_content() {
  local file="$1" item
  require_file "$file" 444 || return 1
  [[ "$(wc -l < "$file")" == 136 ]] || fail "development result key count mismatch"
  load_kv_file "$file" || return 1
  validate_science_complete "$SCIENCE_COMPLETE" || return 1
  local -a contract=(
    "schema_id=${PROTOCOL_ID}"
    "status=complete"
    "protocol_id=${PROTOCOL_ID}"
    "benchmark_id=synthetic_continuous_graph_v2"
    "project_goal=Project Clear Signal - Test the Frozen Pre-Pool Domain-by-Scale Surface"
    "diagnostic_phase=${DIAGNOSTIC_PHASE}"
    "diagnostic_authority=development_only"
    "benchmark_acceptance_authority=false"
    "certified_authorization_eligible=false"
    "classification=$(kv "$SCIENCE_COMPLETE" classification)"
    "preregistration_path=${PREREG}"
    "preregistration_sha256=${PREREG_SHA}"
    "runner_path=${RUNNER}"
    "runner_sha256=$(sha256 "$RUNNER")"
    "attempt_path=${ATTEMPT}"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "preparation_receipt_path=${PREP_RECEIPT}"
    "preparation_receipt_sha256=$(sha256 "$PREP_RECEIPT")"
    "science_complete_path=${SCIENCE_COMPLETE}"
    "science_complete_sha256=$(sha256 "$SCIENCE_COMPLETE")"
    "projection_complete_path=${PROJECTION_COMPLETE}"
    "projection_complete_sha256=$(sha256 "$PROJECTION_COMPLETE")"
    "predecessor_v2_result_path=${V2_RESULT}"
    "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
    "predecessor_v2_strong_gate_pass_count=0"
    "capture_source_sha256=${CAPTURE_SOURCE_SHA}"
    "capture_source_path=${CAPTURE_SOURCE}"
    "capture_wrapper_sha256=${CAPTURE_WRAPPER_SHA}"
    "capture_wrapper_path=${CAPTURE_WRAPPER}"
    "capture_binary_sha256=$(sha256 "$CAPTURE_BIN")"
    "capture_binary_path=${CAPTURE_BIN}"
    "evaluator_source_sha256=${EVALUATOR_SOURCE_SHA}"
    "evaluator_source_path=${EVALUATOR_SOURCE}"
    "evaluator_wrapper_sha256=${EVALUATOR_WRAPPER_SHA}"
    "evaluator_wrapper_path=${EVALUATOR_WRAPPER}"
    "evaluator_binary_sha256=$(sha256 "$EVALUATOR_BIN")"
    "evaluator_binary_path=${EVALUATOR_BIN}"
    "phase2a_nine_head_authority_sha256=${PHASE2A_SOURCE_SHA}"
    "frozen_parser_authority_sha256=${CANONICAL_PARSER_SHA}"
    "canonical_result_sha256=${CANONICAL_RESULT_SHA}"
    "canonical_result_path=${CANONICAL_RESULT}"
    "canonical_inputs_path=${CANONICAL_INPUTS}"
    "canonical_inputs_sha256=${CANONICAL_INPUTS_SHA}"
    "source_closure_path=${SOURCE_CLOSURE}"
    "source_closure_sha256=${SOURCE_CLOSURE_SHA}"
    "cursor_erratum_path=${CURSOR_ERRATUM}"
    "cursor_erratum_sha256=${CURSOR_ERRATUM_SHA}"
    "source_manifest_path=${SOURCE_MANIFEST}"
    "source_manifest_sha256=${SOURCE_MANIFEST_SHA}"
    "capture_config_path=${CAPTURE_CONFIG}"
    "capture_config_sha256=${CAPTURE_CONFIG_SHA}"
    "checkpoint_path=${CHECKPOINT}"
    "checkpoint_sha256=${CHECKPOINT_SHA}"
    "historical_train_probe_path=${HIST_TRAIN}"
    "historical_train_probe_sha256=${HIST_TRAIN_SHA}"
    "historical_validation_probe_path=${HIST_VALIDATION}"
    "historical_validation_probe_sha256=${HIST_VALIDATION_SHA}"
    "train_capture_report_path=${TRAIN_CAPTURE_REPORT}"
    "train_capture_report_sha256=$(sha256 "$TRAIN_CAPTURE_REPORT")"
    "validation_capture_report_path=${VALIDATION_CAPTURE_REPORT}"
    "validation_capture_report_sha256=$(sha256 "$VALIDATION_CAPTURE_REPORT")"
    "train_all_tokens_reference_path=${TRAIN_REFERENCE}"
    "train_all_tokens_reference_sha256=$(sha256 "$TRAIN_REFERENCE")"
    "validation_all_tokens_reference_path=${VALIDATION_REFERENCE}"
    "validation_all_tokens_reference_sha256=$(sha256 "$VALIDATION_REFERENCE")"
    "train_prepool_probe_path=${TRAIN_PREPOOL}"
    "train_prepool_probe_sha256=$(sha256 "$TRAIN_PREPOOL")"
    "validation_prepool_probe_path=${VALIDATION_PREPOOL}"
    "validation_prepool_probe_sha256=$(sha256 "$VALIDATION_PREPOOL")"
    "main_report_path=${MAIN_REPORT}"
    "main_report_sha256=$(sha256 "$MAIN_REPORT")"
    "replay_report_path=${REPLAY_REPORT}"
    "replay_report_sha256=$(sha256 "$REPLAY_REPORT")"
    "main_replay_byte_identical=true"
    "capture_report_key_count=${CAPTURE_REPORT_KEY_COUNT}"
    "capture_report_sorted_key_sha256=${CAPTURE_REPORT_SORTED_KEY_SHA}"
    "evaluator_report_key_count=${EVALUATOR_REPORT_KEY_COUNT}"
    "evaluator_report_sorted_key_sha256=${EVALUATOR_REPORT_SORTED_KEY_SHA}"
    "selected_candidate_index=$(kv "$MAIN_REPORT" selected_candidate_index)"
    "selected_ridge=$(kv "$MAIN_REPORT" selected_ridge)"
    "selected_validation_direction=$(kv "$MAIN_REPORT" selected.validation.directional_accuracy)"
    "selected_validation_rank=$(kv "$MAIN_REPORT" selected.validation.pairwise_rank_accuracy)"
    "selected_validation_correlation=$(kv "$MAIN_REPORT" selected.validation.correlation)"
    "selected_validation_rmse=$(kv "$MAIN_REPORT" selected.validation.rmse)"
    "selected_validation_rmse_target_rms_ratio=$(kv "$MAIN_REPORT" selected.validation.rmse_target_rms_ratio)"
    "validation_strong_gate_pass=$(kv "$MAIN_REPORT" validation_strong_gate_pass)"
    "validation_partial_gate_pass=$(kv "$MAIN_REPORT" validation_partial_gate_pass)"
    "original_strong_gate=direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25"
    "original_partial_gate=direction>=0.80,rank>=0.78"
    "capture_process_invocations=2"
    "capture_process_completions=2"
    "capture_replay_invocations=0"
    "checkpoint_load_count=2"
    "encoder_forward_calls=43"
    "encoder_anchor_participations=2752"
    "capture_probe_artifact_count=4"
    "capture_report_count=2"
    "historical_byte_parity_check_count=2"
    "projection_check_count=6"
    "evaluator_invocations=2"
    "evaluator_completions=2"
    "validated_evaluator_report_count=2"
    "main_replay_parity_check_count=1"
    "ridge_invariant_system_build_count=18"
    "analytic_ridge_fit_attempt_count=12"
    "cholesky_factorization_count=108"
    "cholesky_solve_count=108"
    "conditioned_head_solve_count=108"
    "optimizer_fit_count=0"
    "optimizer_step_count=0"
    "refit_count=0"
    "retry_count=0"
    "early_stop_count=0"
    "automatic_retry=false"
    "same_protocol_retry=false"
    "same_protocol_resume=false"
    "capture_replay=false"
    "checkpoint_written=false"
    "certified_input_access=false"
    "final_holdout_access=false"
    "policy_config_parsed_as_inert_dependency=true"
    "policy_model_access=false"
    "policy_execution=false"
    "mdn_model_access=false"
    "mdn_execution=false"
    "maximum_anchor_read=2815"
    "train_anchor_range=[0,2496)"
    "validation_anchor_range=[2560,2816)"
    "causal_reading_strong=current_cross_domain_cross_scale_serving_mean_is_exposed_information_loss_boundary_under_frozen_lineage"
    "causal_reading_non_strong=closes_only_fixed_domain_scale_masked_mean_affine_surface"
    "within_cell_window_order_preserved=false"
    "downstream_head_nonlinear=false"
    "successful_result_authorizes_certified_or_production=false"
    "full_transitive_system_toolchain_or_elf_closure=false"
    "scientific_execution_completed=true"
    "scientific_result_available=true"
  )
  for item in "${contract[@]}"; do
    expect "$file" "${item%%=*}" "${item#*=}" || return 1
  done
  local classification strong
  classification="$(kv "$file" classification)"
  strong="$(kv "$file" validation_strong_gate_pass)"
  if [[ "$strong" == true ]]; then
    [[ "$classification" == "$PASS_CLASS" ]] || fail "strong result classification mismatch"
  else
    [[ "$classification" == "$NO_PASS_CLASS" ]] || fail "non-strong result classification mismatch"
  fi
}

emit_result_candidate() {
  [[ ! -e "$RESULT_CANDIDATE" && ! -L "$RESULT_CANDIDATE" ]] ||
    fail "result candidate is not pristine"
  emit_result_body >"$RESULT_CANDIDATE"
  chmod 0444 -- "$RESULT_CANDIDATE"
  validate_result_content "$RESULT_CANDIDATE"
}

commit_result_final() {
  [[ ! -e "$RESULT" && ! -L "$RESULT" && -f "$RESULT_CANDIDATE" &&
     ! -L "$RESULT_CANDIDATE" ]] || fail "unsafe final result commit state"
  mv -T -n -- "$RESULT_CANDIDATE" "$RESULT"
}

validate_result() {
  require_file "$RESULT" 444
  validate_result_content "$RESULT"
}

authorize_private_worker() {
  local token="${CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_TOKEN:-}"
  local expected_identity="${CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_IDENTITY:-}"
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
  unset CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_TOKEN
  unset CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_IDENTITY token observed extra

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
    [[ -e "/proc/$$/fd/${fd}" ]] ||
      fail "private worker did not inherit authority lock FD ${fd}"
    [[ "$(stat -Lc '%d:%i' -- "/proc/$$/fd/${fd}")" == \
       "$(stat -Lc '%d:%i' -- "$authority_path")" ]] ||
      fail "private worker authority lock identity mismatch: ${authority_path}"
    flock -s -n "$fd" ||
      fail "private worker lost shared authority lock: ${authority_path}"
  done <<EOF
3|${V2_LOCK}
4|${CANONICAL_LOCK}
5|${REPRESENTATION_LOCK}
6|${SOURCE_LOCK}
EOF
}

private_worker() {
  authorize_private_worker
  trap 'exit 129' HUP
  trap 'exit 130' INT
  trap 'exit 143' TERM
  trap 'exit 131' QUIT
  validate_prepare_receipt
  validate_attempt
  preflight_science_inputs
  [[ ! -e "$RESULT" && ! -L "$RESULT" && ! -e "$TERMINAL" &&
     ! -L "$TERMINAL" && ! -e "$PROJECTION_COMPLETE" &&
     ! -e "$SCIENCE_COMPLETE" && ! -e "$RESULT_CANDIDATE" &&
     ! -e "$CAPTURE_ROOT" && ! -e "$AFFINE_ROOT" ]] ||
    fail "scientific lifecycle is not pristine"

  install -d -m 0700 -- "$CAPTURE_ROOT" "$AFFINE_ROOT" "$AFFINE_MAIN_DIR" \
    "$AFFINE_REPLAY_DIR"
  require_private_dir "$CAPTURE_ROOT"
  require_private_dir "$AFFINE_ROOT"
  require_private_dir "$AFFINE_MAIN_DIR"
  require_private_dir "$AFFINE_REPLAY_DIR"

  run_capture_split train
  run_capture_split validation
  run_projection_stage
  run_evaluator_lane main
  run_evaluator_lane replay
  cmp -s -- "$MAIN_REPORT" "$REPLAY_REPORT" ||
    fail "main/replay evaluator reports differ"
  emit_science_complete
  validate_science_complete
  emit_result_candidate
  commit_result_final
}

hash_or_not_available() {
  local path="$1"
  if [[ -f "$path" && ! -L "$path" ]]; then sha256 "$path"; else printf '%s' not_available; fi
}

publish_failure_evidence() {
  local candidate file root
  if [[ -f "$WORKER_LOG_CANDIDATE" && ! -L "$WORKER_LOG_CANDIDATE" &&
        ! -e "$WORKER_LOG" && ! -L "$WORKER_LOG" ]]; then
    publish_once "$WORKER_LOG_CANDIDATE" "$WORKER_LOG" 0444
  fi
  if [[ -f "$RESULT_CANDIDATE" && ! -L "$RESULT_CANDIDATE" &&
        ! -e "$REJECTED_RESULT" && ! -L "$REJECTED_RESULT" ]]; then
    publish_once "$RESULT_CANDIDATE" "$REJECTED_RESULT" 0444
  fi
  for root in "$EVIDENCE" "$CAPTURE_ROOT" "$AFFINE_ROOT" "$SCRATCH"; do
    [[ -d "$root" && ! -L "$root" ]] || continue
    while IFS= read -r -d '' file; do
      [[ -f "$file" && ! -L "$file" ]] || continue
      chmod 0444 -- "$file"
      require_file "$file" 444
    done < <(find "$root" -type f -print0)
  done

  if [[ ! -e "$REJECTED_EVIDENCE_MANIFEST" && ! -L "$REJECTED_EVIDENCE_MANIFEST" ]]; then
    candidate="${SCRATCH}/.rejected.evidence.sha256.candidate.$$"
    {
      for root in "$EVIDENCE" "$CAPTURE_ROOT" "$AFFINE_ROOT" "$SCRATCH"; do
        [[ -d "$root" && ! -L "$root" ]] || continue
        find "$root" -type f ! -path "$candidate" -print0
      done
      if [[ -f "$REJECTED_RESULT" && ! -L "$REJECTED_RESULT" ]]; then
        printf '%s\0' "$REJECTED_RESULT"
      fi
    } | LC_ALL=C sort -zu | while IFS= read -r -d '' file; do
      [[ -f "$file" && ! -L "$file" ]] || continue
      printf '%s  %s\n' "$(sha256 "$file")" "$file"
    done >"$candidate"
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
  local split lane -a p
  TERMINAL_CAPTURE_STARTED=0
  TERMINAL_CAPTURE_RETURNED=0
  TERMINAL_CAPTURE_COMPLETED=0
  TERMINAL_PROJECTION_COMPLETE=false
  TERMINAL_EVALUATOR_STARTED=0
  TERMINAL_EVALUATOR_RETURNED=0
  TERMINAL_VALIDATED_REPORTS=0
  TERMINAL_EVALUATOR_COMPLETED=0
  TERMINAL_MAIN_REPLAY_PARITY=0
  TERMINAL_SCIENCE_VALID=false
  TERMINAL_LAST_STAGE=attempt_consumed

  for split in train validation; do
    mapfile -t p < <(capture_paths "$split")
    strict_validator_status validate_capture_launch "${p[4]}" "$split" \
      "${p[0]}" "${p[7]}" "${p[8]}"
    if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
    TERMINAL_CAPTURE_STARTED=$((TERMINAL_CAPTURE_STARTED+1))
    TERMINAL_LAST_STAGE=capture_started
    strict_validator_status validate_capture_returned "${p[5]}" "$split" \
      "${CAPTURE_EVIDENCE}/${split}.log" false
    if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
    TERMINAL_CAPTURE_RETURNED=$((TERMINAL_CAPTURE_RETURNED+1))
    TERMINAL_LAST_STAGE=capture_returned
    strict_validator_status validate_capture_complete "${p[6]}" "$split" \
      "${p[3]}" "${p[1]}" "${p[2]}" "${p[10]}" "${p[5]}"
    if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
    TERMINAL_CAPTURE_COMPLETED=$((TERMINAL_CAPTURE_COMPLETED+1))
    TERMINAL_LAST_STAGE=capture_complete
  done

  strict_validator_status validate_projection_complete
  if (( STRICT_VALIDATOR_RC == 0 )); then
    TERMINAL_PROJECTION_COMPLETE=true
    TERMINAL_LAST_STAGE=projection_complete
  fi

  if [[ "$TERMINAL_PROJECTION_COMPLETE" == true ]]; then
    for lane in main replay; do
      local report launch returned complete log
      if [[ "$lane" == main ]]; then
        report="$MAIN_REPORT"; launch="$MAIN_LAUNCH"; returned="$MAIN_RETURNED"
        complete="$MAIN_COMPLETE"
      else
        report="$REPLAY_REPORT"; launch="$REPLAY_LAUNCH"; returned="$REPLAY_RETURNED"
        complete="$REPLAY_COMPLETE"
      fi
      log="${AFFINE_EVIDENCE}/${lane}.log"
      strict_validator_status validate_evaluator_launch "$launch" "$lane" "$report"
      if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
      TERMINAL_EVALUATOR_STARTED=$((TERMINAL_EVALUATOR_STARTED+1))
      TERMINAL_LAST_STAGE=evaluator_started
      strict_validator_status validate_evaluator_returned "$returned" "$lane" "$log" false
      if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
      TERMINAL_EVALUATOR_RETURNED=$((TERMINAL_EVALUATOR_RETURNED+1))
      TERMINAL_LAST_STAGE=evaluator_returned
      strict_validator_status validate_evaluator_report "$report"
      if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
      TERMINAL_VALIDATED_REPORTS=$((TERMINAL_VALIDATED_REPORTS+1))
      TERMINAL_LAST_STAGE=evaluator_report_validated
      strict_validator_status validate_evaluator_complete "$complete" "$lane" \
        "$report" "$returned"
      if (( STRICT_VALIDATOR_RC != 0 )); then break; fi
      TERMINAL_EVALUATOR_COMPLETED=$((TERMINAL_EVALUATOR_COMPLETED+1))
      TERMINAL_LAST_STAGE=evaluator_complete
    done
  fi

  if (( TERMINAL_EVALUATOR_COMPLETED == 2 )) &&
     cmp -s -- "$MAIN_REPORT" "$REPLAY_REPORT"; then
    TERMINAL_MAIN_REPLAY_PARITY=1
    TERMINAL_LAST_STAGE=main_replay_parity
  fi
  strict_validator_status validate_science_complete "$SCIENCE_COMPLETE"
  if (( STRICT_VALIDATOR_RC == 0 )); then
    load_kv_file "$SCIENCE_COMPLETE"
    TERMINAL_SCIENCE_VALID=true
    TERMINAL_LAST_STAGE=science_complete
  fi
  return 0
}

compute_terminal_actuals() {
  collect_terminal_progress
  TERMINAL_CLASSIFICATION=not_available
  TERMINAL_SCIENTIFIC_EXECUTION_COMPLETED=false
  if [[ "$TERMINAL_SCIENCE_VALID" == true ]]; then
    TERMINAL_SCIENTIFIC_EXECUTION_COMPLETED=true
    TERMINAL_CLASSIFICATION="$(kv "$SCIENCE_COMPLETE" classification)"
    TERMINAL_CHECKPOINT_LOADS=2
    TERMINAL_ENCODER_CALLS=43
    TERMINAL_ENCODER_ANCHORS=2752
    TERMINAL_RIDGE_SYSTEMS=18
    TERMINAL_RIDGE_ATTEMPTS=12
    TERMINAL_FACTORIZATIONS=108
    TERMINAL_SOLVES=108
    TERMINAL_HEAD_SOLVES=108
  else
    if (( TERMINAL_CAPTURE_STARTED == TERMINAL_CAPTURE_COMPLETED )); then
      case "$TERMINAL_CAPTURE_COMPLETED" in
        0)
          TERMINAL_CHECKPOINT_LOADS=0
          TERMINAL_ENCODER_CALLS=0
          TERMINAL_ENCODER_ANCHORS=0
          ;;
        1)
          TERMINAL_CHECKPOINT_LOADS=1
          TERMINAL_ENCODER_CALLS=39
          TERMINAL_ENCODER_ANCHORS=2496
          ;;
        2)
          TERMINAL_CHECKPOINT_LOADS=2
          TERMINAL_ENCODER_CALLS=43
          TERMINAL_ENCODER_ANCHORS=2752
          ;;
        *) fail "invalid validated capture completion count"; return 1 ;;
      esac
    else
      TERMINAL_CHECKPOINT_LOADS=not_available
      TERMINAL_ENCODER_CALLS=not_available
      TERMINAL_ENCODER_ANCHORS=not_available
    fi
    if (( TERMINAL_EVALUATOR_STARTED == TERMINAL_EVALUATOR_COMPLETED )); then
      TERMINAL_RIDGE_SYSTEMS=$((TERMINAL_EVALUATOR_COMPLETED*9))
      TERMINAL_RIDGE_ATTEMPTS=$((TERMINAL_EVALUATOR_COMPLETED*6))
      TERMINAL_FACTORIZATIONS=$((TERMINAL_EVALUATOR_COMPLETED*54))
      TERMINAL_SOLVES=$((TERMINAL_EVALUATOR_COMPLETED*54))
      TERMINAL_HEAD_SOLVES=$((TERMINAL_EVALUATOR_COMPLETED*54))
    else
      TERMINAL_RIDGE_SYSTEMS=not_available
      TERMINAL_RIDGE_ATTEMPTS=not_available
      TERMINAL_FACTORIZATIONS=not_available
      TERMINAL_SOLVES=not_available
      TERMINAL_HEAD_SOLVES=not_available
    fi
  fi
}

validate_rejected_evidence_manifest() {
  local line digest path observed seen="|"
  require_file "$REJECTED_EVIDENCE_MANIFEST" 444 || return 1
  while IFS= read -r line || [[ -n "$line" ]]; do
    [[ "$line" =~ ^([0-9a-f]{64})[[:space:]][[:space:]](/.*)$ ]] || {
      fail "malformed rejected-evidence manifest line"
      return 1
    }
    digest="${BASH_REMATCH[1]}"
    path="${BASH_REMATCH[2]}"
    [[ "$path" == "$RUNTIME/"* && "$path" != "$REJECTED_EVIDENCE_MANIFEST" ]] || {
      fail "rejected-evidence path escapes runtime: ${path}"
      return 1
    }
    [[ "$seen" != *"|${path}|"* ]] || {
      fail "duplicate rejected-evidence manifest path: ${path}"
      return 1
    }
    seen+="${path}|"
    require_file "$path" 444 || return 1
    observed="$(sha256 "$path")" || return 1
    [[ "$observed" == "$digest" ]] || {
      fail "rejected-evidence hash mismatch: ${path}"
      return 1
    }
  done <"$REJECTED_EVIDENCE_MANIFEST"
}

validate_terminal_content() {
  local file="$1" worker_exit="$2" failure_stage="$3" failure_reason="$4" item
  local projection_count=0
  [[ "$TERMINAL_PROJECTION_COMPLETE" == true ]] && projection_count=6
  require_file "$file" 444 || return 1
  [[ "$(wc -l < "$file")" == 54 ]] || fail "terminal receipt key count mismatch"
  load_kv_file "$file" || return 1
  [[ "$worker_exit" == not_available || "$worker_exit" =~ ^[0-9]+$ ]] ||
    fail "invalid terminal worker exit code"
  case "$failure_stage" in
    stale_attempt_recovery|signal|scientific_execution_or_precondition|post_worker_validation|bounded_worker) ;;
    *) fail "invalid terminal failure stage"; return 1 ;;
  esac
  [[ -n "$failure_reason" ]] || fail "terminal failure reason is empty"
  local -a contract=(
    "schema_id=${PROTOCOL_ID}.terminal.v1"
    "status=terminal_invalid"
    "protocol_id=${PROTOCOL_ID}"
    "classification=invalid_prepool_domain_scale_affine_protocol"
    "scientific_classification_if_complete=${TERMINAL_CLASSIFICATION}"
    "failure_stage=${failure_stage}"
    "failure_reason=${failure_reason}"
    "worker_exit_code=${worker_exit}"
    "attempt_consumed=true"
    "attempt_sha256=$(sha256 "$ATTEMPT")"
    "last_validated_lifecycle_stage=${TERMINAL_LAST_STAGE}"
    "capture_process_invocations_started=${TERMINAL_CAPTURE_STARTED}"
    "capture_process_invocations_returned=${TERMINAL_CAPTURE_RETURNED}"
    "validated_capture_completion_count=${TERMINAL_CAPTURE_COMPLETED}"
    "checkpoint_load_count=${TERMINAL_CHECKPOINT_LOADS}"
    "encoder_forward_calls=${TERMINAL_ENCODER_CALLS}"
    "encoder_anchor_participations=${TERMINAL_ENCODER_ANCHORS}"
    "projection_complete=${TERMINAL_PROJECTION_COMPLETE}"
    "projection_check_count=${projection_count}"
    "evaluator_invocations_started=${TERMINAL_EVALUATOR_STARTED}"
    "evaluator_invocations_returned=${TERMINAL_EVALUATOR_RETURNED}"
    "validated_evaluator_report_count=${TERMINAL_VALIDATED_REPORTS}"
    "validated_evaluator_completion_count=${TERMINAL_EVALUATOR_COMPLETED}"
    "validated_main_replay_parity_count=${TERMINAL_MAIN_REPLAY_PARITY}"
    "ridge_invariant_system_build_count=${TERMINAL_RIDGE_SYSTEMS}"
    "analytic_ridge_fit_attempt_count=${TERMINAL_RIDGE_ATTEMPTS}"
    "cholesky_factorization_count=${TERMINAL_FACTORIZATIONS}"
    "cholesky_solve_count=${TERMINAL_SOLVES}"
    "conditioned_head_solve_count=${TERMINAL_HEAD_SOLVES}"
    "science_complete_receipt_valid=${TERMINAL_SCIENCE_VALID}"
    "science_complete_receipt_sha256=$(hash_or_not_available "$SCIENCE_COMPLETE")"
    "scientific_execution_completed=${TERMINAL_SCIENTIFIC_EXECUTION_COMPLETED}"
    "scientific_result_available=false"
    "worker_log_sha256=$(hash_or_not_available "$WORKER_LOG")"
    "rejected_result_sha256=$(hash_or_not_available "$REJECTED_RESULT")"
    "rejected_evidence_manifest_sha256=$(sha256 "$REJECTED_EVIDENCE_MANIFEST")"
    "capture_replay_invocations=0"
    "optimizer_fit_count=0"
    "optimizer_step_count=0"
    "refit_count=0"
    "retry_count=0"
    "same_protocol_retry_allowed=false"
    "same_protocol_resume_allowed=false"
    "benchmark_acceptance_authority=false"
    "certified_authorization_eligible=false"
    "certified_input_access=false"
    "final_holdout_access=false"
    "policy_model_access=false"
    "mdn_model_access=false"
    "checkpoint_written=false"
    "maximum_anchor_read_upper_bound=2815"
    "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
    "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    "term_grace_seconds=${TERM_GRACE_SECONDS}"
  )
  for item in "${contract[@]}"; do
    expect "$file" "${item%%=*}" "${item#*=}" || return 1
  done
  validate_rejected_evidence_manifest
}

seal_terminal() {
  local worker_exit="$1" failure_stage="$2" failure_reason="$3"
  local candidate projection_count=0
  [[ -f "$ATTEMPT" && ! -L "$ATTEMPT" ]] || return 0
  [[ ! -e "$TERMINAL" && ! -L "$TERMINAL" ]] || return 0
  [[ ! -e "$RESULT" && ! -L "$RESULT" ]] || return 0

  compute_terminal_actuals
  publish_failure_evidence
  [[ "$TERMINAL_PROJECTION_COMPLETE" == true ]] && projection_count=6
  RUN_TERMINAL_SEAL_SEQUENCE=$((RUN_TERMINAL_SEAL_SEQUENCE+1))
  candidate="${SCRATCH}/terminal.invalid.status.candidate.$$.${RUN_TERMINAL_SEAL_SEQUENCE}"
  [[ ! -e "$candidate" && ! -L "$candidate" ]] ||
    fail "terminal receipt candidate already exists"
  {
    echo "schema_id=${PROTOCOL_ID}.terminal.v1"
    echo "status=terminal_invalid"
    echo "protocol_id=${PROTOCOL_ID}"
    echo "classification=invalid_prepool_domain_scale_affine_protocol"
    echo "scientific_classification_if_complete=${TERMINAL_CLASSIFICATION}"
    echo "failure_stage=${failure_stage}"
    echo "failure_reason=${failure_reason}"
    echo "worker_exit_code=${worker_exit}"
    echo "attempt_consumed=true"
    echo "attempt_sha256=$(sha256 "$ATTEMPT")"
    echo "last_validated_lifecycle_stage=${TERMINAL_LAST_STAGE}"
    echo "capture_process_invocations_started=${TERMINAL_CAPTURE_STARTED}"
    echo "capture_process_invocations_returned=${TERMINAL_CAPTURE_RETURNED}"
    echo "validated_capture_completion_count=${TERMINAL_CAPTURE_COMPLETED}"
    echo "checkpoint_load_count=${TERMINAL_CHECKPOINT_LOADS}"
    echo "encoder_forward_calls=${TERMINAL_ENCODER_CALLS}"
    echo "encoder_anchor_participations=${TERMINAL_ENCODER_ANCHORS}"
    echo "projection_complete=${TERMINAL_PROJECTION_COMPLETE}"
    echo "projection_check_count=${projection_count}"
    echo "evaluator_invocations_started=${TERMINAL_EVALUATOR_STARTED}"
    echo "evaluator_invocations_returned=${TERMINAL_EVALUATOR_RETURNED}"
    echo "validated_evaluator_report_count=${TERMINAL_VALIDATED_REPORTS}"
    echo "validated_evaluator_completion_count=${TERMINAL_EVALUATOR_COMPLETED}"
    echo "validated_main_replay_parity_count=${TERMINAL_MAIN_REPLAY_PARITY}"
    echo "ridge_invariant_system_build_count=${TERMINAL_RIDGE_SYSTEMS}"
    echo "analytic_ridge_fit_attempt_count=${TERMINAL_RIDGE_ATTEMPTS}"
    echo "cholesky_factorization_count=${TERMINAL_FACTORIZATIONS}"
    echo "cholesky_solve_count=${TERMINAL_SOLVES}"
    echo "conditioned_head_solve_count=${TERMINAL_HEAD_SOLVES}"
    echo "science_complete_receipt_valid=${TERMINAL_SCIENCE_VALID}"
    echo "science_complete_receipt_sha256=$(hash_or_not_available "$SCIENCE_COMPLETE")"
    echo "scientific_execution_completed=${TERMINAL_SCIENTIFIC_EXECUTION_COMPLETED}"
    echo "scientific_result_available=false"
    echo "worker_log_sha256=$(hash_or_not_available "$WORKER_LOG")"
    echo "rejected_result_sha256=$(hash_or_not_available "$REJECTED_RESULT")"
    echo "rejected_evidence_manifest_sha256=$(sha256 "$REJECTED_EVIDENCE_MANIFEST")"
    echo "capture_replay_invocations=0"
    echo "optimizer_fit_count=0"
    echo "optimizer_step_count=0"
    echo "refit_count=0"
    echo "retry_count=0"
    echo "same_protocol_retry_allowed=false"
    echo "same_protocol_resume_allowed=false"
    echo "benchmark_acceptance_authority=false"
    echo "certified_authorization_eligible=false"
    echo "certified_input_access=false"
    echo "final_holdout_access=false"
    echo "policy_model_access=false"
    echo "mdn_model_access=false"
    echo "checkpoint_written=false"
    echo "maximum_anchor_read_upper_bound=2815"
    echo "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
    echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
    echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
  } >"$candidate"
  chmod 0444 -- "$candidate"
  validate_terminal_content "$candidate" "$worker_exit" "$failure_stage" "$failure_reason"
  publish_once "$candidate" "$TERMINAL" 0444
}

validate_terminal() {
  local worker_exit failure_stage failure_reason
  require_file "$TERMINAL" 444 || return 1
  [[ ! -e "$RESULT" && ! -L "$RESULT" ]] ||
    fail "terminal and development result coexist"
  validate_attempt || return 1
  compute_terminal_actuals || return 1
  load_kv_file "$TERMINAL" || return 1
  worker_exit="$(kv "$TERMINAL" worker_exit_code)" || return 1
  failure_stage="$(kv "$TERMINAL" failure_stage)" || return 1
  failure_reason="$(kv "$TERMINAL" failure_reason)" || return 1
  validate_terminal_content "$TERMINAL" "$worker_exit" "$failure_stage" "$failure_reason"
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
    [[ "$command" != *synthetic_v2_frozen_mtf_prepool_domain_scale_affine_development_v1* &&
       "$command" != *run_frozen_mtf_prepool_domain_scale_affine_v1.sh* ]] ||
      fail "prepool protocol process remains active: pid=${pid}"
  done
}

create_private_capability() {
  local token="$1"
  RUN_CAPABILITY_PATH="$CAPABILITY_CANDIDATE"
  [[ ! -e "$RUN_CAPABILITY_PATH" && ! -L "$RUN_CAPABILITY_PATH" ]] ||
    fail "private worker capability path is not pristine"
  (set -o noclobber; printf '%s\n' "$token" >"$RUN_CAPABILITY_PATH") 2>/dev/null ||
    fail "could not create private worker capability"
  chmod 0400 -- "$RUN_CAPABILITY_PATH"
  require_file "$RUN_CAPABILITY_PATH" 400
  exec 8<"$RUN_CAPABILITY_PATH"
  RUN_CAPABILITY_IDENTITY="$(stat -Lc '%d:%i' -- "$RUN_CAPABILITY_PATH")"
  rm -f -- "$RUN_CAPABILITY_PATH"
  RUN_CAPABILITY_PATH=""
  [[ ! -e "$CAPABILITY_CANDIDATE" &&
     "$(stat -Lc '%h' -- /proc/$$/fd/8)" == 0 ]] ||
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
  if [[ -e "$TERMINAL" || -L "$TERMINAL" ]]; then
    fail "protocol is terminally invalid; retry is forbidden"
    return 1
  fi
  if [[ -e "$RESULT" || -L "$RESULT" ]]; then
    validate_result
    echo "[clear-signal:prepool-domain-scale] existing development result verified"
    return 0
  fi
  if [[ -e "$ATTEMPT" || -L "$ATTEMPT" ]]; then
    RUN_EXIT_GUARD_ACTIVE=1
    RUN_FAILURE_STAGE=stale_attempt_recovery
    RUN_FAILURE_REASON=previous_execution_interrupted_after_attempt
    trap 'run_exit_guard "$?"' EXIT
    trap '' HUP INT TERM QUIT
    seal_terminal not_available stale_attempt_recovery \
      previous_execution_interrupted_after_attempt
    RUN_EXIT_GUARD_ACTIVE=0
    trap - EXIT
    fail "the sole attempt was already consumed; retry/resume is forbidden"
    return 1
  fi
  [[ -e "$PREP_RECEIPT" && ! -L "$PREP_RECEIPT" ]] ||
    fail "compile-only preparation is required before development execution"
  preflight_pre_attempt
  scan_for_live_protocol_processes
  [[ -z "$(find "$SCRATCH" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
    fail "scratch is not pristine"
  [[ -z "$(find "$EVIDENCE" -type f -print -quit)" ]] ||
    fail "scientific evidence directory is not pristine"
  [[ ! -e "$CAPTURE_ROOT" && ! -L "$CAPTURE_ROOT" &&
     ! -e "$AFFINE_ROOT" && ! -L "$AFFINE_ROOT" &&
     ! -e "$WORKER_LOG" && ! -e "$REJECTED_RESULT" &&
     ! -e "$REJECTED_EVIDENCE_MANIFEST" ]] ||
    fail "runtime is not pristine before attempt"

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
  CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_TOKEN="$token" \
  CLEAR_SIGNAL_PREPOOL_DOMAIN_SCALE_WORKER_IDENTITY="$capability_identity" \
    setsid timeout --signal=TERM --kill-after="${TERM_GRACE_SECONDS}s" \
      "${WORKER_TIMEOUT_SECONDS}s" "$RUNNER" --private-worker \
      >"$WORKER_LOG_CANDIDATE" 2>&1 &
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
      RUN_FAILURE_STAGE=scientific_execution_or_precondition
      RUN_FAILURE_REASON=bounded_worker_nonzero_or_incomplete
      seal_terminal "$rc" "$RUN_FAILURE_STAGE" "$RUN_FAILURE_REASON"
      RUN_EXIT_GUARD_ACTIVE=0
      trap - EXIT HUP INT TERM QUIT
      fail "terminal development attempt failed (${rc}); retry is forbidden"
      return 1
    fi
    if [[ -f "$RESULT" && ! -L "$RESULT" ]]; then
      RUN_EXIT_GUARD_ACTIVE=0
      trap - EXIT HUP INT TERM QUIT
      validate_result
      publish_worker_log || true
      echo "[clear-signal:prepool-domain-scale] result committed before worker termination"
      return 0
    fi
    RUN_EXIT_GUARD_ACTIVE=0
    trap - EXIT HUP INT TERM QUIT
    fail "bounded worker failed (${rc}) and lifecycle evidence is missing or corrupt"
    return 1
  fi

  [[ -f "$RESULT" && ! -L "$RESULT" ]] ||
    fail "bounded worker exited zero without final result"
  validate_result
  publish_worker_log || true
  RUN_EXIT_GUARD_ACTIVE=0
  trap - EXIT HUP INT TERM QUIT
  echo "[clear-signal:prepool-domain-scale] complete: ${RESULT}"
}

plan() {
  preflight_static_authorities
  acquire_authority_locks
  echo "Project Clear Signal - Test the Frozen Pre-Pool Domain-by-Scale Surface"
  echo "protocol_id=${PROTOCOL_ID}"
  echo "scope=development_only"
  echo "predecessor_v2_result_sha256=${V2_RESULT_SHA}"
  echo "predecessor_v2_strong_gate_pass_count=0"
  echo "capture_process_invocations=2"
  echo "capture_replay_invocations=0"
  echo "evaluator_invocations=2"
  echo "evaluator_lane_order=main_then_replay"
  echo "capture_report_key_count=${CAPTURE_REPORT_KEY_COUNT}"
  echo "capture_report_sorted_key_sha256=${CAPTURE_REPORT_SORTED_KEY_SHA}"
  echo "evaluator_report_key_count=${EVALUATOR_REPORT_KEY_COUNT}"
  echo "evaluator_report_sorted_key_sha256=${EVALUATOR_REPORT_SORTED_KEY_SHA}"
  echo "worker_timeout_seconds=${WORKER_TIMEOUT_SECONDS}"
  echo "term_grace_seconds=${TERM_GRACE_SECONDS}"
  echo "prepare_wrapper_timeout_seconds=${PREP_TIMEOUT_SECONDS}"
  echo "prepare_wrapper_term_grace_seconds=${PREP_TERM_GRACE_SECONDS}"
  echo "same_encode_historical_byte_parity_required=true"
  echo "projection_checks_before_evaluator_call_1=6"
  echo "original_strong_gate=direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25"
  echo "original_partial_gate=direction>=0.80,rank>=0.78"
  echo "automatic_retry=false"
  echo "same_protocol_resume=false"
  echo "certified_input_access=false"
  echo "final_holdout_access=false"
  echo "policy_model_access=false"
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
  require_private_dir "$CAPTURE_EVIDENCE"
  require_private_dir "$AFFINE_EVIDENCE"
  require_private_dir "$SCRATCH"
  open_execution_lock_shared
  acquire_authority_locks
  if [[ -e "$RESULT" && ! -e "$TERMINAL" ]]; then
    validate_result
  elif [[ -e "$TERMINAL" && ! -e "$RESULT" ]]; then
    validate_terminal
  else
    fail "neither a unique result nor a unique terminal bundle is available"
  fi
  scan_for_live_protocol_processes
  echo "[clear-signal:prepool-domain-scale] development result verified read-only"
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
