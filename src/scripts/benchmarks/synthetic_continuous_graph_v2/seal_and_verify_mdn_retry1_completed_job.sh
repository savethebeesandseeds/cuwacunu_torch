#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly result_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.result.v1"
readonly runtime_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1"
readonly input_schema_id="synthetic_v2_mdn_train_isolated_v2_retry1.inputs.v1"
readonly source_schema_id="synthetic_v2_isolated_development_source_v1"
readonly interruption_schema_id="synthetic_v2_mdn_train_isolated_v2.interruption.v1"

readonly expected_execution_runner_sha256="93c477a6e4de3ddfbded94ca8f22db14cd7954800a319713c0a7961ccf2bb799"
readonly expected_input_receipt_sha256="d1ee004e82f10352c988be425677e1fb3a98ebd7e7ef7cae603b3570857408f4"
readonly expected_completion_correction_sha256="ab4ceb9a7d1e6d55c6830b3263abfbfe60225399283ef63673176c11d4ebc5d9"
readonly expected_inventory_master_sha256="43b36342443cd37d78d2639618ea87f8c8c6f43dce24e0ef4a73cdc9a6399f50"
readonly expected_config_sha256="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
readonly expected_log_sha256="4f74f73f1c153a8457ecb009a45f174425ccdabd6bd671c15779436f4af75479"
readonly expected_lock_sha256="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

readonly expected_original_runner_sha256="62defc3a15478c4aec1106c3a84105ef12aa1a65ec9bbafbc0669e96a62c2349"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_source_verifier_sha256="e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_cursor_erratum_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly expected_source_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
readonly expected_representation_runner_sha256="cb8cfcb926d40b7172fb744246db346d4da768c546c63b976d19a5e4801d8295"
readonly expected_representation_result_sha256="981971679e4c37ba23919aae549eab1bc1ea1c1452f1978c004802f4807dbd07"
readonly expected_representation_checkpoint_sha256="70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d"
readonly expected_objective_sha256="33b40ce2f6a76f0c9dddc67b9e3b162d1a171199b6d50b174dafaf854b135d5e"
readonly expected_recovery_amendment_sha256="a5b5a5dcda52c9e89d37f33db15678c5ceb67904f2c3ae187701123208573f8e"
readonly expected_interruption_sealer_sha256="824b6174f88d560210febb29f9b9db584a29de20da1d28c62fdde82a10269392"
readonly expected_interruption_receipt_sha256="57e73e038d61f9d3c60e79344e05995a8739f7b539dc604949b4ccb712aec1ea"
readonly expected_interrupted_checkpoint_sha256="ab67674cefd9749b11b8302829489ad5cec8f6242abe3f0cb3e5b6bb4983bdc5"

readonly expected_optimizer_steps=3500
readonly expected_seed=31
readonly expected_anchor_begin=0
readonly expected_anchor_end=2496
readonly expected_anchor_count=3261
readonly expected_directory_count=7
readonly expected_payload_file_count=23
readonly expected_job_file_count=16
readonly expected_component_file_count=1
readonly expected_system_file_count=2
readonly expected_payload_bytes=4201197994
readonly expected_representation_probe_rows=2016000
readonly expected_runtime_probe_records=40551
readonly expected_cursor_token="version=ujcamei.graph_anchor_cursor_report.v1|graph=4133db527907a8e4|reference_edge=SYN2ALPHASYN2USD|Hx=30|Hf=1|edges=3|accepted=3261|candidates=3261|skipped=0|first=1896047999999|last=2177711999999"
readonly expected_execution_chain="ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:train"
readonly expected_stream_plan="source(ujcamei.source.retrieval.graph_anchor:graph_anchor_edge_dataset_v1) -> nodelift(wikimyei.expression.nodelift.srl:nodelift_srl_v1) -> channel_representation(wikimyei.representation.encoding.mtf_jepa_mae_vicreg:mtf_jepa_mae_vicreg_v1) -> channel_inference(wikimyei.inference.expected_value.mdn:mdn_v1)"
readonly expected_objective_vector="learning_rate=0.001|max_steps=3500|batch=64|grad_clip=5|checkpoint_every=50|report_every=10|seed=31|mixtures=3|hidden=128|residual_depth=2|edge_aux=0|edge_aux_direction=0|edge_aux_rank=0|edge_aux_huber=0.01|edge_aux_logit_scale=50|direct_enabled=true|direct=100|direct_direction=5|direct_rank=5|direct_huber=0.5|direct_logit_scale=1|direct_target_scale=36|warmup_steps=800|warmup_nll=0|post_warmup_nll=1|warmup_direct_only=true|identity=edge_embedding_per_edge|base_edges=3|identity_dim=16|adapter_hidden=128"

fail() {
  echo "synthetic v2 MDN retry1 completion sealer: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
}

reject_symlink_components() {
  local path="$1"
  [[ "${path}" == /* ]] || fail "path is not absolute: ${path}"
  local current="/" rest component
  rest="${path#/}"
  while [[ -n "${rest}" ]]; do
    if [[ "${rest}" == */* ]]; then
      component="${rest%%/*}"
      rest="${rest#*/}"
    else
      component="${rest}"
      rest=""
    fi
    [[ -n "${component}" ]] || continue
    if [[ "${current}" == "/" ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains a symbolic-link component: ${current}"
  done
}

kv() {
  local path="$1" key="$2" count value
  count="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) count += 1;
    }
    END { print count + 0 }
  ' "${path}")"
  [[ "${count}" == 1 ]] ||
    fail "${path}: expected exactly one ${key}= entry, found ${count}"
  value="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) {
        value = substr($0, eq + 1);
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
        print value;
      }
    }
  ' "${path}")"
  printf '%s' "${value}"
}

expect_kv() {
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

verify_exact_kv_keys() {
  local path="$1"
  shift
  local expected actual
  expected="$(printf '%s\n' "$@" | LC_ALL=C sort)"
  actual="$(awk '
    NF == 0 { next }
    {
      eq = index($0, "=");
      if (eq == 0) {
        print "__MALFORMED_LINE__" NR;
        next;
      }
      key = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", key);
      if (key == "" || key ~ /[[:space:]]/) {
        print "__MALFORMED_KEY__" NR;
        next;
      }
      print key;
    }
  ' "${path}" | LC_ALL=C sort)"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: exact keyset mismatch"
}

expect_finite_number() {
  local path="$1" key="$2" value
  value="$(kv "${path}" "${key}")"
  [[ "${value}" =~ ^[-+]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)?$ ]] ||
    fail "${path}: ${key} is not a finite numeric token: ${value}"
}

verify_hash() {
  local path="$1" expected="$2" label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${runtime_parent}/${runtime_schema_id}"
job_dir="${runtime_root}/job"

execution_runner="${script_dir}/run_mdn_train_isolated_v2_retry1.sh"
completion_correction="${benchmark_root}/MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md"
input_receipt="${runtime_root}/inputs.status"
config_snapshot="${runtime_root}/synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
log_path="${runtime_root}/mdn_train.retry1.log"
execution_lock="${runtime_root}/.execution.lock"
result_receipt="${runtime_root}/result.status"
staging_path="${runtime_parent}/.${runtime_schema_id}.result.status.candidate"

source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
source_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
source_closure="${source_runtime}/development_source_closure.status"
cursor_erratum_receipt="${source_runtime}/cursor_alignment_erratum.status"
source_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"

representation_runner="${script_dir}/run_representation_train_isolated_v2.sh"
representation_runtime="${runtime_parent}/synthetic_v2_representation_train_isolated_v2"
representation_result="${representation_runtime}/result.status"
representation_checkpoint="${representation_runtime}/job/channel_representation.report.mtf_jepa_mae_vicreg.pt"

original_runner="${script_dir}/run_mdn_train_isolated_v2.sh"
recovery_amendment="${benchmark_root}/MDN_ENGINE_INTERRUPTION_RECOVERY_AMENDMENT.md"
interruption_sealer="${script_dir}/seal_and_verify_interrupted_mdn_attempt_v2.sh"
interrupted_runtime="${runtime_parent}/synthetic_v2_mdn_train_isolated_v2"
interruption_receipt="${interrupted_runtime}/interruption.status"
interrupted_checkpoint="${interrupted_runtime}/job/channel_inference.report.channel_mdn.pt"

runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
objective_path="${benchmark_root}/wikimyei.inference.expected_value.mdn.v2.jkimyei"

report_path="${job_dir}/channel_inference.report"
checkpoint_path="${job_dir}/channel_inference.report.channel_mdn.pt"
mdn_lls_path="${job_dir}/channel_inference.report.mdn.lls"
nodelift_lls_path="${job_dir}/channel_inference.report.nodelift.lls"
representation_lls_path="${job_dir}/channel_inference.report.representation.lls"
manifest_path="${job_dir}/job.manifest"
job_state_path="${job_dir}/job.state"
lattice_checkpoint_path="${job_dir}/lattice.checkpoint.fact"
lattice_exposure_path="${job_dir}/lattice.exposure.fact"
lattice_source_path="${job_dir}/lattice.source_analytics.fact"
representation_probe_path="${job_dir}/representation_edge_features.probe"
checkpoint_io_path="${job_dir}/runtime.checkpoint_io.fact"
component_update_path="${job_dir}/runtime.component_training_update.fact"
health_path="${job_dir}/runtime.health_measurement.fact"
event_probe_path="${job_dir}/runtime.job_events.probe"
runtime_result_path="${job_dir}/runtime.result.fact"
spawn_ref_path="${runtime_root}/components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"
spawn_registry_path="${runtime_root}/system/component_spawn_registry.v1.lls"
runtime_layout_path="${runtime_root}/system/runtime_layout.v1.lls"

readonly -a expected_directories=(
  "."
  "components"
  "components/wikimyei.inference.expected_value.mdn"
  "components/wikimyei.inference.expected_value.mdn/spawns"
  "components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb"
  "job"
  "system"
)

readonly -a payload_relpaths=(
  ".execution.lock"
  "components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"
  "inputs.status"
  "job/channel_inference.report"
  "job/channel_inference.report.channel_mdn.pt"
  "job/channel_inference.report.mdn.lls"
  "job/channel_inference.report.nodelift.lls"
  "job/channel_inference.report.representation.lls"
  "job/job.manifest"
  "job/job.state"
  "job/lattice.checkpoint.fact"
  "job/lattice.exposure.fact"
  "job/lattice.source_analytics.fact"
  "job/representation_edge_features.probe"
  "job/runtime.checkpoint_io.fact"
  "job/runtime.component_training_update.fact"
  "job/runtime.health_measurement.fact"
  "job/runtime.job_events.probe"
  "job/runtime.result.fact"
  "mdn_train.retry1.log"
  "synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
  "system/component_spawn_registry.v1.lls"
  "system/runtime_layout.v1.lls"
)

declare -Ar expected_payload_hashes=(
  [".execution.lock"]="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  ["components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"]="b882808f6942c01d8b49829b529b738cb359ad0d1ac27fa1243c1c189fb907d0"
  ["inputs.status"]="d1ee004e82f10352c988be425677e1fb3a98ebd7e7ef7cae603b3570857408f4"
  ["job/channel_inference.report"]="07b53bb36b240794a0845091d2559153a482e354102da3f8b3cc9bb7e74833db"
  ["job/channel_inference.report.channel_mdn.pt"]="a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f"
  ["job/channel_inference.report.mdn.lls"]="a7a3a1768cccd45271fdede4a80d611f9fea7bacd2331068e51f1ef75800e78f"
  ["job/channel_inference.report.nodelift.lls"]="bde33bb12ba2cd4d36ad12235da11169540d36026349cbb2ec57dd489f5bb3d8"
  ["job/channel_inference.report.representation.lls"]="f24d73413bb2afb2e4a4403ec6f47d84e6c2935f5cb0b4a88fa06792a75a81ec"
  ["job/job.manifest"]="e82caf605e2d554de93345ca4f9eef802be7216297a0d5f299bf2f57fdc3db27"
  ["job/job.state"]="cb0ac2ad692f89ed763bffdbd2149f1f024f1f25325d9d44d4714b463ddbc484"
  ["job/lattice.checkpoint.fact"]="6f4bea543f338e7df93be90393c0fc9e3e1c0a7cc2651d83f47f698867d7d24b"
  ["job/lattice.exposure.fact"]="8721e577e053f840920d29d922d9b0d08d90f28e68b253fd990dc41622f7b16d"
  ["job/lattice.source_analytics.fact"]="6fb027764b12edf00d947b7c00ba7cc218e4477163b3570756fcc7e2a3fdeda9"
  ["job/representation_edge_features.probe"]="c584f07973eb3d453039086a5a6eea1f073bcb0b7d2b9aaea76b6a7459ed432a"
  ["job/runtime.checkpoint_io.fact"]="1cd37c868c32c7ab99e187eacdc2c9e4dffc4b77469717b9a3179f1a34def5df"
  ["job/runtime.component_training_update.fact"]="c06d30a100f0eed3871b4a588d350b4e2065da3b2373318be95a0644be57d7bd"
  ["job/runtime.health_measurement.fact"]="ed9eb3ad53fcb37b5974eac9a468f2ab214da9af5cd1e9fbf59952f0baa78958"
  ["job/runtime.job_events.probe"]="cf0e89ddf0ea4e2207ed311e6af93a33364a8e9bf250126d23403b8e34afc53b"
  ["job/runtime.result.fact"]="15f815b4a10a783bba92de43b9c01892e4fddb942f0dce242a995c02a5e102dc"
  ["mdn_train.retry1.log"]="4f74f73f1c153a8457ecb009a45f174425ccdabd6bd671c15779436f4af75479"
  ["synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"]="a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e"
  ["system/component_spawn_registry.v1.lls"]="0415c82a0c4278022de2dba7c9a4ddb9b2fffe5e0d94ff566ca8e2da47d0d10d"
  ["system/runtime_layout.v1.lls"]="fb2198270b44590399e7ae2d7ee95190ca5a3b88d3495943afac3149b5f7e1bc"
)

declare -Ar expected_payload_sizes=(
  [".execution.lock"]="0"
  ["components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"]="405"
  ["inputs.status"]="5978"
  ["job/channel_inference.report"]="14452"
  ["job/channel_inference.report.channel_mdn.pt"]="3228076"
  ["job/channel_inference.report.mdn.lls"]="3189"
  ["job/channel_inference.report.nodelift.lls"]="2200"
  ["job/channel_inference.report.representation.lls"]="2125"
  ["job/job.manifest"]="7589"
  ["job/job.state"]="4636"
  ["job/lattice.checkpoint.fact"]="3523"
  ["job/lattice.exposure.fact"]="9109"
  ["job/lattice.source_analytics.fact"]="3806"
  ["job/representation_edge_features.probe"]="4129261000"
  ["job/runtime.checkpoint_io.fact"]="1816"
  ["job/runtime.component_training_update.fact"]="1162"
  ["job/runtime.health_measurement.fact"]="2243"
  ["job/runtime.job_events.probe"]="68622341"
  ["job/runtime.result.fact"]="5930"
  ["mdn_train.retry1.log"]="12045"
  ["synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"]="5307"
  ["system/component_spawn_registry.v1.lls"]="705"
  ["system/runtime_layout.v1.lls"]="357"
)

readonly -a job_relpaths=(
  "channel_inference.report"
  "channel_inference.report.channel_mdn.pt"
  "channel_inference.report.mdn.lls"
  "channel_inference.report.nodelift.lls"
  "channel_inference.report.representation.lls"
  "job.manifest"
  "job.state"
  "lattice.checkpoint.fact"
  "lattice.exposure.fact"
  "lattice.source_analytics.fact"
  "representation_edge_features.probe"
  "runtime.checkpoint_io.fact"
  "runtime.component_training_update.fact"
  "runtime.health_measurement.fact"
  "runtime.job_events.probe"
  "runtime.result.fact"
)

input_receipt_keys=(
  schema_id runner_path runner_sha256 original_runner_path original_runner_sha256
  recovery_amendment_path recovery_amendment_sha256 interruption_sealer_path
  interruption_sealer_sha256 interruption_receipt_path
  interruption_receipt_sha256 interrupted_runtime_path
  isolated_source_verifier_path isolated_source_verifier_sha256
  isolated_source_closure_path isolated_source_closure_sha256
  source_isolation_amendment_sha256 isolated_source_protocol_sha256
  staged_hardening_amendment_sha256 cursor_alignment_correction_path
  cursor_alignment_correction_sha256 cursor_alignment_metadata_erratum_path
  cursor_alignment_metadata_erratum_sha256 cursor_alignment_erratum_receipt_path
  cursor_alignment_erratum_receipt_sha256 config_snapshot_path
  config_snapshot_sha256 runtime_exec_path runtime_exec_sha256
  representation_runner_path representation_runner_sha256
  representation_result_path representation_result_sha256
  representation_checkpoint_path representation_checkpoint_sha256
  mdn_objective_path mdn_objective_sha256 input_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_path
  forbidden_interrupted_mdn_checkpoint_sha256 train_anchor_begin
  train_anchor_end_exclusive expected_optimizer_steps expected_seed
  expected_accepted_anchor_count expected_candidate_anchor_count
  expected_maximum_available_anchor_index expected_execution_chain
  expected_stream_plan expected_objective_vector restart_from_step_zero
  quarantined_checkpoint_access canonical_data_raw_access
  final_holdout_available policy_access
)

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--seal|--verify]"
case "${mode}" in
--plan | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${result_schema_id}.completion_seal.plan
runtime_root=${runtime_root}
result_receipt_path=${result_receipt}
execution_runner_path=${execution_runner}
execution_runner_sha256=${expected_execution_runner_sha256}
completion_sealer_path=${script_path}
completion_sealer_sha256=$(sha256_of "${script_path}")
completion_correction_path=${completion_correction}
completion_correction_sha256=${expected_completion_correction_sha256}
payload_directory_count=${expected_directory_count}
payload_file_count_before_receipt=${expected_payload_file_count}
job_file_count=${expected_job_file_count}
component_file_count=${expected_component_file_count}
system_file_count=${expected_system_file_count}
payload_bytes_before_receipt=${expected_payload_bytes}
payload_inventory_master_sha256=${expected_inventory_master_sha256}
optimizer_steps=${expected_optimizer_steps}
train_anchor_range=[${expected_anchor_begin},${expected_anchor_end})
validator_failure_1=job_allowlist_omitted_seven_standard_runtime_facts
validator_failure_2=finite_parameter_check_component_encoding_mismatch
training_reexecution_authorized=false
receipt_publication_after_payload_seal=true
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

verify_external_provenance() {
  verify_hash "${completion_correction}" \
    "${expected_completion_correction_sha256}" "completion correction"
  verify_hash "${execution_runner}" "${expected_execution_runner_sha256}" \
    "execution runner"
  verify_hash "${original_runner}" "${expected_original_runner_sha256}" \
    "original MDN runner"
  verify_hash "${runtime_exec}" "${expected_runtime_exec_sha256}" \
    "runtime executable"
  verify_hash "${source_verifier}" "${expected_source_verifier_sha256}" \
    "source verifier"
  verify_hash "${source_closure}" "${expected_source_closure_sha256}" \
    "source closure"
  verify_hash "${cursor_erratum_receipt}" "${expected_cursor_erratum_sha256}" \
    "cursor erratum receipt"
  verify_hash "${source_amendment}" "${expected_source_amendment_sha256}" \
    "source isolation amendment"
  verify_hash "${source_protocol}" "${expected_source_protocol_sha256}" \
    "source protocol"
  verify_hash "${staged_hardening}" "${expected_staged_hardening_sha256}" \
    "staged hardening amendment"
  verify_hash "${cursor_correction}" "${expected_cursor_correction_sha256}" \
    "cursor correction"
  verify_hash "${cursor_metadata_erratum}" \
    "${expected_cursor_metadata_erratum_sha256}" "cursor metadata erratum"
  verify_hash "${representation_runner}" \
    "${expected_representation_runner_sha256}" "representation runner"
  verify_hash "${representation_result}" \
    "${expected_representation_result_sha256}" "representation result"
  verify_hash "${representation_checkpoint}" \
    "${expected_representation_checkpoint_sha256}" \
    "representation checkpoint"
  verify_hash "${objective_path}" "${expected_objective_sha256}" \
    "MDN objective"
  verify_hash "${recovery_amendment}" \
    "${expected_recovery_amendment_sha256}" "recovery amendment"
  verify_hash "${interruption_sealer}" \
    "${expected_interruption_sealer_sha256}" "interruption sealer"
  verify_hash "${interruption_receipt}" \
    "${expected_interruption_receipt_sha256}" "interruption receipt"
  verify_hash "${interrupted_checkpoint}" \
    "${expected_interrupted_checkpoint_sha256}" \
    "forbidden interrupted checkpoint"

  "${source_verifier}" --verify >/dev/null
  "${interruption_sealer}" --verify >/dev/null

  expect_kv "${source_closure}" schema_id "${source_schema_id}"
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${source_closure}" skipped_outside_common_range 0
  expect_kv "${source_closure}" skipped_missing_edge_coverage 0
  expect_kv "${source_closure}" skipped_failed_fetch_probe 0
  expect_kv "${source_closure}" duplicate_anchor_count 0
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false

  expect_kv "${cursor_erratum_receipt}" schema_id \
    "${source_schema_id}.cursor_alignment_erratum.v1"
  expect_kv "${cursor_erratum_receipt}" status complete
  expect_kv "${cursor_erratum_receipt}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" maximum_anchor_index 3260
  expect_kv "${cursor_erratum_receipt}" canonical_data_raw_access false
  expect_kv "${cursor_erratum_receipt}" independent_final_evidence false

  expect_kv "${representation_result}" schema_id \
    synthetic_v2_representation_train_isolated_v2.result.v1
  expect_kv "${representation_result}" status complete
  expect_kv "${representation_result}" checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${representation_result}" checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${representation_result}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${representation_result}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${representation_result}" optimizer_steps 3000
  expect_kv "${representation_result}" canonical_data_raw_access false
  expect_kv "${representation_result}" final_holdout_available false
  expect_kv "${representation_result}" policy_access false

  expect_kv "${interruption_receipt}" schema_id \
    "${interruption_schema_id}"
  expect_kv "${interruption_receipt}" closure_status complete
  expect_kv "${interruption_receipt}" attempt_status interrupted
  expect_kv "${interruption_receipt}" scientific_evidence false
  expect_kv "${interruption_receipt}" metric_evidence_authorized false
  expect_kv "${interruption_receipt}" retry_schema_id "${runtime_schema_id}"
  expect_kv "${interruption_receipt}" retry_result_schema_id \
    "${result_schema_id}"
  expect_kv "${interruption_receipt}" retry_runtime_root "${runtime_root}"
  expect_kv "${interruption_receipt}" retry_runner_path \
    "${execution_runner}"
  expect_kv "${interruption_receipt}" retry_restart_from_step_zero true
  expect_kv "${interruption_receipt}" partial_artifact_reuse_authorized false
  expect_kv "${interruption_receipt}" canonical_data_raw_access false
  expect_kv "${interruption_receipt}" independent_final_evidence false
  expect_kv "${interruption_receipt}" policy_access false
}

verify_input_receipt() {
  verify_hash "${input_receipt}" "${expected_input_receipt_sha256}" \
    "retry1 input receipt"
  [[ "$(stat -c '%a' -- "${input_receipt}")" == 444 ]] ||
    fail "retry1 input receipt is not mode 0444"
  verify_exact_kv_keys "${input_receipt}" "${input_receipt_keys[@]}"
  expect_kv "${input_receipt}" schema_id "${input_schema_id}"
  expect_kv "${input_receipt}" runner_path "${execution_runner}"
  expect_kv "${input_receipt}" runner_sha256 \
    "${expected_execution_runner_sha256}"
  expect_kv "${input_receipt}" original_runner_path "${original_runner}"
  expect_kv "${input_receipt}" original_runner_sha256 \
    "${expected_original_runner_sha256}"
  expect_kv "${input_receipt}" recovery_amendment_path \
    "${recovery_amendment}"
  expect_kv "${input_receipt}" recovery_amendment_sha256 \
    "${expected_recovery_amendment_sha256}"
  expect_kv "${input_receipt}" interruption_sealer_path \
    "${interruption_sealer}"
  expect_kv "${input_receipt}" interruption_sealer_sha256 \
    "${expected_interruption_sealer_sha256}"
  expect_kv "${input_receipt}" interruption_receipt_path \
    "${interruption_receipt}"
  expect_kv "${input_receipt}" interruption_receipt_sha256 \
    "${expected_interruption_receipt_sha256}"
  expect_kv "${input_receipt}" interrupted_runtime_path \
    "${interrupted_runtime}"
  expect_kv "${input_receipt}" isolated_source_verifier_path \
    "${source_verifier}"
  expect_kv "${input_receipt}" isolated_source_verifier_sha256 \
    "${expected_source_verifier_sha256}"
  expect_kv "${input_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${input_receipt}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${input_receipt}" source_isolation_amendment_sha256 \
    "${expected_source_amendment_sha256}"
  expect_kv "${input_receipt}" isolated_source_protocol_sha256 \
    "${expected_source_protocol_sha256}"
  expect_kv "${input_receipt}" staged_hardening_amendment_sha256 \
    "${expected_staged_hardening_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_correction_path \
    "${cursor_correction}"
  expect_kv "${input_receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_correction_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_metadata_erratum}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_metadata_erratum_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_erratum_receipt}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "${expected_cursor_erratum_sha256}"
  expect_kv "${input_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${input_receipt}" config_snapshot_sha256 \
    "${expected_config_sha256}"
  expect_kv "${input_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${input_receipt}" runtime_exec_sha256 \
    "${expected_runtime_exec_sha256}"
  expect_kv "${input_receipt}" representation_runner_path \
    "${representation_runner}"
  expect_kv "${input_receipt}" representation_runner_sha256 \
    "${expected_representation_runner_sha256}"
  expect_kv "${input_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${input_receipt}" representation_result_sha256 \
    "${expected_representation_result_sha256}"
  expect_kv "${input_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${input_receipt}" representation_checkpoint_sha256 \
    "${expected_representation_checkpoint_sha256}"
  expect_kv "${input_receipt}" mdn_objective_path "${objective_path}"
  expect_kv "${input_receipt}" mdn_objective_sha256 \
    "${expected_objective_sha256}"
  expect_kv "${input_receipt}" input_mdn_checkpoint_path ''
  expect_kv "${input_receipt}" forbidden_interrupted_mdn_checkpoint_path \
    "${interrupted_checkpoint}"
  expect_kv "${input_receipt}" forbidden_interrupted_mdn_checkpoint_sha256 \
    "${expected_interrupted_checkpoint_sha256}"
  expect_kv "${input_receipt}" train_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${input_receipt}" train_anchor_end_exclusive \
    "${expected_anchor_end}"
  expect_kv "${input_receipt}" expected_optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${input_receipt}" expected_seed "${expected_seed}"
  expect_kv "${input_receipt}" expected_accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${input_receipt}" expected_candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${input_receipt}" expected_maximum_available_anchor_index 3260
  expect_kv "${input_receipt}" expected_execution_chain \
    "${expected_execution_chain}"
  expect_kv "${input_receipt}" expected_stream_plan \
    "${expected_stream_plan}"
  expect_kv "${input_receipt}" expected_objective_vector \
    "${expected_objective_vector}"
  expect_kv "${input_receipt}" restart_from_step_zero true
  expect_kv "${input_receipt}" quarantined_checkpoint_access false
  expect_kv "${input_receipt}" canonical_data_raw_access false
  expect_kv "${input_receipt}" final_holdout_available false
  expect_kv "${input_receipt}" policy_access false
}

verify_exact_tree() {
  local expect_receipt="$1"
  reject_symlink_components "${runtime_root}"
  require_dir "${runtime_root}"
  [[ "$(realpath -e -- "${runtime_root}")" == "${runtime_root}" ]] ||
    fail "retry1 runtime root is not canonical"

  local entry rel dir_count=0 file_count=0 expected_total_files
  local -A allowed_dirs=() allowed_files=()
  for rel in "${expected_directories[@]}"; do
    allowed_dirs["${rel}"]=1
  done
  for rel in "${payload_relpaths[@]}"; do
    allowed_files["${rel}"]=1
  done

  while IFS= read -r -d '' entry; do
    if [[ "${entry}" == "${runtime_root}" ]]; then
      rel="."
    else
      rel="${entry#${runtime_root}/}"
    fi
    [[ ! -L "${entry}" ]] || fail "runtime contains symlink: ${rel}"
    if [[ -d "${entry}" ]]; then
      [[ -n "${allowed_dirs[${rel}]+present}" ]] ||
        fail "runtime contains unknown directory: ${rel}"
      ((dir_count += 1))
    elif [[ -f "${entry}" ]]; then
      if [[ -n "${allowed_files[${rel}]+present}" ]]; then
        ((file_count += 1))
      elif [[ "${expect_receipt}" == true && \
        "${entry}" == "${result_receipt}" ]]; then
        ((file_count += 1))
      else
        fail "runtime contains unknown file: ${rel}"
      fi
      [[ "$(stat -c '%h' -- "${entry}")" == 1 ]] ||
        fail "runtime file has an external hard link: ${rel}"
    else
      fail "runtime contains a special entry: ${rel}"
    fi
  done < <(find "${runtime_root}" -mindepth 0 -print0)

  [[ "${dir_count}" == "${expected_directory_count}" ]] ||
    fail "expected ${expected_directory_count} directories, found ${dir_count}"
  expected_total_files="${expected_payload_file_count}"
  [[ "${expect_receipt}" == true ]] && ((expected_total_files += 1))
  [[ "${file_count}" == "${expected_total_files}" ]] ||
    fail "expected ${expected_total_files} files, found ${file_count}"
}

emit_expected_inventory() {
  local rel
  for rel in "${payload_relpaths[@]}"; do
    printf '%s %s %s\n' "${expected_payload_hashes[${rel}]}" \
      "${expected_payload_sizes[${rel}]}" "${rel}"
  done
}

verify_payload_hashes_sizes_and_master() {
  local rel path actual_hash actual_size inventory_hash total_bytes=0
  for rel in "${payload_relpaths[@]}"; do
    path="${runtime_root}/${rel}"
    reject_symlink_components "${path}"
    require_file "${path}"
    actual_size="$(stat -c '%s' -- "${path}")"
    [[ "${actual_size}" == "${expected_payload_sizes[${rel}]}" ]] ||
      fail "payload size drifted: ${rel}"
    actual_hash="$(sha256_of "${path}")"
    [[ "${actual_hash}" == "${expected_payload_hashes[${rel}]}" ]] ||
      fail "payload hash drifted: ${rel}"
    total_bytes=$((total_bytes + actual_size))
  done
  [[ "${total_bytes}" == "${expected_payload_bytes}" ]] ||
    fail "payload byte total drifted: ${total_bytes}"
  inventory_hash="$(emit_expected_inventory | sha256sum | awk '{print $1}')"
  [[ "${inventory_hash}" == "${expected_inventory_master_sha256}" ]] ||
    fail "internal canonical inventory master drifted"
}

verify_source_receipts() {
  local manifest="$1"
  local isolated_source_root receipts receipt source instrument interval
  local edge_field instrument_field interval_field record_field source_field extra
  local count=0 key expected_source expected_receipt
  local -A seen=()
  isolated_source_root="$(kv "${source_closure}" isolated_source_root_path)"
  isolated_source_root="$(realpath -e -- "${isolated_source_root}")"
  [[ "${isolated_source_root}" == "${source_runtime}/source" ]] ||
    fail "unexpected isolated source root: ${isolated_source_root}"
  receipts="$(kv "${manifest}" source_file_receipts)"
  [[ -n "${receipts}" ]] || fail "job manifest omitted source receipts"
  [[ "${receipts}" != *"/data/raw/"* ]] ||
    fail "job manifest references canonical data/raw"
  while IFS= read -r receipt; do
    [[ -n "${receipt}" ]] || continue
    IFS='|' read -r edge_field instrument_field interval_field record_field \
      source_field extra <<<"${receipt}"
    [[ -z "${extra:-}" && "${edge_field}" == edge=* && \
      "${instrument_field}" == instrument=* && \
      "${interval_field}" == interval=* && \
      "${record_field}" == record_type=kline && \
      "${source_field}" == source=* ]] ||
      fail "malformed source receipt: ${receipt}"
    instrument="${instrument_field#instrument=}"
    interval="${interval_field#interval=}"
    [[ "${edge_field#edge=}" == "${instrument}" ]] ||
      fail "source receipt edge/instrument mismatch: ${receipt}"
    case "${instrument}" in
    SYN2ALPHASYN2USD | SYN2BETASYN2USD | SYN2GAMMASYN2USD) ;;
    *) fail "unexpected source instrument: ${instrument}" ;;
    esac
    case "${interval}" in
    1d | 3d | 1w) ;;
    *) fail "unexpected source interval: ${interval}" ;;
    esac
    source="${source_field#source=}"
    expected_source="${isolated_source_root}/${instrument}/${interval}/${instrument}-${interval}-all-years.csv"
    expected_receipt="edge=${instrument}|instrument=${instrument}|interval=${interval}|record_type=kline|source=${expected_source}"
    [[ "${receipt}" == "${expected_receipt}" ]] ||
      fail "source receipt differs from exact isolated descriptor: ${receipt}"
    reject_symlink_components "${source}"
    require_file "${source}"
    [[ "$(realpath -e -- "${source}")" == "${source}" ]] ||
      fail "source receipt is not canonical: ${source}"
    key="${instrument}/${interval}"
    [[ -z "${seen[${key}]:-}" ]] ||
      fail "duplicate source receipt: ${key}"
    seen["${key}"]=1
    ((count += 1))
  done < <(printf '%s\n' "${receipts}" | sed 's/;;/\n/g')
  [[ "${count}" == 9 ]] ||
    fail "expected nine isolated source receipts, found ${count}"
  for instrument in SYN2ALPHASYN2USD SYN2BETASYN2USD SYN2GAMMASYN2USD; do
    for interval in 1d 3d 1w; do
      [[ "${seen[${instrument}/${interval}]:-}" == 1 ]] ||
        fail "missing exact source receipt: ${instrument}/${interval}"
    done
  done
}

verify_config_and_objective() {
  verify_hash "${config_snapshot}" "${expected_config_sha256}" \
    "retry1 config snapshot"
  expect_kv "${config_snapshot}" runtime_wave_id train_core_channel_mdn
  expect_kv "${config_snapshot}" ujcamei_source_registry_dsl_path \
    "${source_runtime}/config/ujcamei.source.registry.development_prefix.dsl"
  expect_kv "${config_snapshot}" \
    wikimyei_inference_expected_value_mdn_jkimyei_path "${objective_path}"

  grep -Fqx '  TRAINING_ID = synthetic_continuous_graph_v2_channel_mdn_train_core_v1;' \
    "${objective_path}" || fail "MDN objective training id drifted"
  grep -Fqx '  MAX_STEPS = 3500;' "${objective_path}" ||
    fail "MDN objective max steps drifted"
  grep -Fqx '  BATCH_SIZE = 64;' "${objective_path}" ||
    fail "MDN objective batch size drifted"
  grep -Fqx '  GRAD_CLIP_NORM = 5.0;' "${objective_path}" ||
    fail "MDN objective grad clip drifted"
  grep -Fqx '  SEED = 31;' "${objective_path}" ||
    fail "MDN objective seed drifted"
  grep -Fqx '  MDN_DIRECT_EDGE_RETURN_READOUT_ENABLED = true;' \
    "${objective_path}" || fail "MDN objective direct readout drifted"
  grep -Fqx '  MDN_DIRECT_EDGE_RETURN_READOUT_LOSS_WEIGHT = 100.0;' \
    "${objective_path}" || fail "MDN objective direct loss weight drifted"
}

verify_manifest() {
  expect_kv "${manifest_path}" manifest_format kikijyeba.runtime.job_manifest.v1
  expect_kv "${manifest_path}" config_path "${config_snapshot}"
  expect_kv "${manifest_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${manifest_path}" job_kind channel_inference_mdn
  expect_kv "${manifest_path}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest_path}" component_spawn_fingerprint 5ba58d2de0fb7dcb
  expect_kv "${manifest_path}" component_operator_surface_digest 6a65a1724dce0110
  expect_kv "${manifest_path}" mdn_assembly_fingerprint fe0bb67764d8ab24
  expect_kv "${manifest_path}" wave_id train_core_channel_mdn
  expect_kv "${manifest_path}" wave_action train
  expect_kv "${manifest_path}" execution_chain "${expected_execution_chain}"
  expect_kv "${manifest_path}" mutated_components \
    wikimyei.inference.expected_value.mdn
  expect_kv "${manifest_path}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  expect_kv "${manifest_path}" source_range_policy anchor_index
  expect_kv "${manifest_path}" source_order_policy random_per_epoch
  expect_kv "${manifest_path}" source_order_policy_explicit true
  expect_kv "${manifest_path}" resolved_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${manifest_path}" resolved_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${manifest_path}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${manifest_path}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${manifest_path}" skipped_outside_common_range 0
  expect_kv "${manifest_path}" skipped_missing_edge_coverage 0
  expect_kv "${manifest_path}" skipped_failed_fetch_probe 0
  expect_kv "${manifest_path}" duplicate_anchor_count 0
  expect_kv "${manifest_path}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${manifest_path}" stream_plan "${expected_stream_plan}"
  expect_kv "${manifest_path}" inference_training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${manifest_path}" input_representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${manifest_path}" input_mdn_checkpoint_path ''
  expect_kv "${manifest_path}" runtime_handoff_id ''
  expect_kv "${manifest_path}" runtime_handoff_digest ''
  expect_kv "${manifest_path}" policy_training_contract_schema ''
  expect_kv "${manifest_path}" policy_family_id ''
  expect_kv "${manifest_path}" policy_dsl_digest ''
  expect_kv "${manifest_path}" policy_net_digest ''
  verify_source_receipts "${manifest_path}"
}

verify_report() {
  expect_kv "${report_path}" training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${report_path}" config_path "${config_snapshot}"
  expect_kv "${report_path}" component_assembly_id mdn_v1
  expect_kv "${report_path}" input_representation_assembly_id \
    mtf_jepa_mae_vicreg_v1
  expect_kv "${report_path}" context_contract \
    graph_order.channel_node_representation.v1
  expect_kv "${report_path}" output_contract \
    graph_order.channel_node_future_distribution.v1
  expect_kv "${report_path}" channel_count 3
  expect_kv "${report_path}" context_dim 32
  expect_kv "${report_path}" future_horizon 1
  expect_kv "${report_path}" context_mode channel_context_strict
  expect_kv "${report_path}" target_domain channel_node_future
  expect_kv "${report_path}" mixture_count 3
  expect_kv "${report_path}" hidden_width 128
  expect_kv "${report_path}" residual_depth 2
  expect_kv "${report_path}" effective_batch_size 64
  expect_kv "${report_path}" checkpoint_every 50
  expect_kv "${report_path}" report_every 10
  expect_kv "${report_path}" seed "${expected_seed}"
  expect_kv "${report_path}" allow_untrained_representation false
  expect_kv "${report_path}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${report_path}" representation_checkpoint_loaded true
  expect_kv "${report_path}" mdn_checkpoint_path ''
  expect_kv "${report_path}" mdn_checkpoint_loaded false
  expect_kv "${report_path}" requested_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${report_path}" requested_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${report_path}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${report_path}" steps_attempted "${expected_optimizer_steps}"
  expect_kv "${report_path}" steps_completed "${expected_optimizer_steps}"
  expect_kv "${report_path}" skipped_batches 0
  expect_kv "${report_path}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${report_path}" nonfinite_output_count 0
  expect_kv "${report_path}" finite_parameter_check 1
  expect_kv "${report_path}" checkpoint_written true
  expect_kv "${report_path}" checkpoint_write_count 70
  expect_kv "${report_path}" last_checkpoint_optimizer_step \
    "${expected_optimizer_steps}"
  expect_kv "${report_path}" checkpoint_path "${checkpoint_path}"
  expect_kv "${report_path}" report_written true
  expect_kv "${report_path}" report_write_count 350
  expect_kv "${report_path}" last_report_attempted_step \
    "${expected_optimizer_steps}"
  expect_kv "${report_path}" report_path "${report_path}"
  expect_kv "${report_path}" representation_edge_feature_probe_written true
  expect_kv "${report_path}" representation_edge_feature_probe_row_count \
    "${expected_representation_probe_rows}"
  expect_kv "${report_path}" representation_edge_feature_probe_path \
    "${representation_probe_path}"
  expect_kv "${report_path}" edge_return_auxiliary_loss_weight 0
  expect_kv "${report_path}" direct_edge_return_readout_enabled true
  expect_kv "${report_path}" direct_edge_return_readout_loss_weight 100
  expect_kv "${report_path}" direct_edge_return_readout_warmup_steps 800
  expect_kv "${report_path}" direct_edge_return_readout_identity_mode \
    edge_embedding_per_edge
  expect_finite_number "${report_path}" last_loss
  expect_finite_number "${report_path}" mean_loss
  expect_finite_number "${report_path}" last_grad_norm
  expect_finite_number "${report_path}" max_grad_norm
  if grep -Eqi '=(nan|[-+]?inf)(,|$)' "${report_path}"; then
    fail "MDN report contains a non-finite numeric output"
  fi
}

verify_job_state() {
  expect_kv "${job_state_path}" state_format kikijyeba.runtime.job_state.v1
  expect_kv "${job_state_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${job_state_path}" job_kind channel_inference_mdn
  expect_kv "${job_state_path}" status completed
  expect_kv "${job_state_path}" error_message ''
  expect_kv "${job_state_path}" wave_id train_core_channel_mdn
  expect_kv "${job_state_path}" wave_action train
  expect_kv "${job_state_path}" source_cursor_token "${expected_cursor_token}"
  expect_kv "${job_state_path}" resolved_anchor_index_begin \
    "${expected_anchor_begin}"
  expect_kv "${job_state_path}" resolved_anchor_index_end \
    "${expected_anchor_end}"
  expect_kv "${job_state_path}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${job_state_path}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${job_state_path}" skipped_outside_common_range 0
  expect_kv "${job_state_path}" skipped_missing_edge_coverage 0
  expect_kv "${job_state_path}" skipped_failed_fetch_probe 0
  expect_kv "${job_state_path}" duplicate_anchor_count 0
  expect_kv "${job_state_path}" steps_attempted "${expected_optimizer_steps}"
  expect_kv "${job_state_path}" steps_completed "${expected_optimizer_steps}"
  expect_kv "${job_state_path}" skipped_batches 0
  expect_kv "${job_state_path}" optimizer_steps "${expected_optimizer_steps}"
  expect_kv "${job_state_path}" checkpoint_written true
  expect_kv "${job_state_path}" checkpoint_write_count 70
  expect_kv "${job_state_path}" checkpoint_path "${checkpoint_path}"
  expect_kv "${job_state_path}" report_written true
  expect_kv "${job_state_path}" report_write_count 350
  expect_kv "${job_state_path}" report_path "${report_path}"
  expect_kv "${job_state_path}" lattice_checkpoint_fact_written true
  expect_kv "${job_state_path}" lattice_checkpoint_fact_path \
    "${lattice_checkpoint_path}"
  expect_kv "${job_state_path}" lattice_exposure_fact_written true
  expect_kv "${job_state_path}" lattice_exposure_fact_path \
    "${lattice_exposure_path}"
  expect_kv "${job_state_path}" lattice_source_analytics_fact_written true
  expect_kv "${job_state_path}" lattice_source_analytics_fact_path \
    "${lattice_source_path}"
  expect_kv "${job_state_path}" runtime_result_fact_written true
  expect_kv "${job_state_path}" runtime_result_fact_path \
    "${runtime_result_path}"
  expect_kv "${job_state_path}" runtime_checkpoint_io_fact_written true
  expect_kv "${job_state_path}" runtime_checkpoint_io_fact_path \
    "${checkpoint_io_path}"
  expect_kv "${job_state_path}" runtime_health_measurement_fact_written true
  expect_kv "${job_state_path}" runtime_health_measurement_fact_path \
    "${health_path}"
  expect_kv "${job_state_path}" runtime_terminal_fact_error ''
  expect_kv "${job_state_path}" replay_artifacts_written false
  expect_kv "${job_state_path}" representation_edge_feature_probe_written true
  expect_kv "${job_state_path}" representation_edge_feature_probe_path \
    "${representation_probe_path}"
  expect_kv "${job_state_path}" representation_edge_feature_probe_row_count \
    "${expected_representation_probe_rows}"
  expect_kv "${job_state_path}" probe_records_written true
  expect_kv "${job_state_path}" probe_stream_path "${event_probe_path}"
  expect_kv "${job_state_path}" probe_record_count \
    "${expected_runtime_probe_records}"
}

verify_lattice_facts() {
  expect_kv "${lattice_checkpoint_path}" schema \
    kikijyeba.lattice.checkpoint.v1
  expect_kv "${lattice_checkpoint_path}" fact_type checkpoint_identity
  expect_kv "${lattice_checkpoint_path}" generation_manifest_schema \
    component_checkpoint_generation_manifest.v1
  expect_kv "${lattice_checkpoint_path}" checkpoint_path \
    "${checkpoint_path}"
  expect_kv "${lattice_checkpoint_path}" checkpoint_file_digest \
    2e1e3962f29512d8
  expect_kv "${lattice_checkpoint_path}" generation_id b2552f8160c4d8ef
  expect_kv "${lattice_checkpoint_path}" parent_generation_ids \
    05ccdeb3ccbcb5ea
  expect_kv "${lattice_checkpoint_path}" read_checkpoint_digests \
    d6bd8d36a4f95957
  expect_kv "${lattice_checkpoint_path}" producer_component_update_fact_digest \
    05a103d54d651ea8
  expect_kv "${lattice_checkpoint_path}" fit_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${lattice_checkpoint_path}" fit_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${lattice_checkpoint_path}" influence_complete true
  expect_kv "${lattice_checkpoint_path}" influence_source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${lattice_checkpoint_path}" component \
    wikimyei.inference.expected_value.mdn
  expect_kv "${lattice_checkpoint_path}" created_by_job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${lattice_checkpoint_path}" created_by_exposure_fact_id \
    da7c310d49226051
  expect_kv "${lattice_checkpoint_path}" input_checkpoints \
    "${representation_checkpoint}"

  expect_kv "${lattice_exposure_path}" schema kikijyeba.lattice.exposure.v1
  expect_kv "${lattice_exposure_path}" fact_type exposure
  expect_kv "${lattice_exposure_path}" target_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${lattice_exposure_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${lattice_exposure_path}" job_status completed
  expect_kv "${lattice_exposure_path}" wave_action train
  expect_kv "${lattice_exposure_path}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${lattice_exposure_path}" anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${lattice_exposure_path}" anchor_end "${expected_anchor_end}"
  expect_kv "${lattice_exposure_path}" accepted_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${lattice_exposure_path}" candidate_anchor_count \
    "${expected_anchor_count}"
  expect_kv "${lattice_exposure_path}" skipped_outside_common_range 0
  expect_kv "${lattice_exposure_path}" skipped_missing_edge_coverage 0
  expect_kv "${lattice_exposure_path}" skipped_failed_fetch_probe 0
  expect_kv "${lattice_exposure_path}" duplicate_anchor_count 0
  expect_kv "${lattice_exposure_path}" completed_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${lattice_exposure_path}" completed_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${lattice_exposure_path}" batches_seen \
    "${expected_optimizer_steps}"
  expect_kv "${lattice_exposure_path}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${lattice_exposure_path}" finite_loss true
  expect_kv "${lattice_exposure_path}" nonfinite_output_count 0
  expect_kv "${lattice_exposure_path}" finite_parameter_check true
  expect_kv "${lattice_exposure_path}" gradients_finite false
  expect_finite_number "${lattice_exposure_path}" mean_loss
  expect_finite_number "${lattice_exposure_path}" last_grad_norm
  expect_finite_number "${lattice_exposure_path}" max_grad_norm
  expect_kv "${lattice_exposure_path}" output_checkpoint \
    "${checkpoint_path}"
  expect_kv "${lattice_exposure_path}" checkpoint_path_reported \
    "${checkpoint_path}"
  expect_kv "${lattice_exposure_path}" checkpoint_digest_verified false
  expect_kv "${lattice_exposure_path}" checkpoint_file_exists true
  expect_kv "${lattice_exposure_path}" checkpoint_bytes 0
  expect_kv "${lattice_exposure_path}" checkpoint_artifact_status ''
  expect_kv "${lattice_exposure_path}" synthetic_training_passed false
  expect_kv "${lattice_exposure_path}" market_readiness_claimed false
  expect_kv "${lattice_exposure_path}" fact_digest da7c310d49226051

  expect_kv "${lattice_source_path}" schema \
    kikijyeba.lattice.source_analytics.v1
  expect_kv "${lattice_source_path}" fact_type source_analytics
  expect_kv "${lattice_source_path}" parent_exposure_fact_digest \
    da7c310d49226051
  expect_kv "${lattice_source_path}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${lattice_source_path}" target_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${lattice_source_path}" job_status completed
  expect_kv "${lattice_source_path}" anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${lattice_source_path}" anchor_end "${expected_anchor_end}"
  expect_kv "${lattice_source_path}" completed_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${lattice_source_path}" completed_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${lattice_source_path}" source_file_receipts \
    "$(kv "${manifest_path}" source_file_receipts)"
  expect_kv "${lattice_source_path}" source_receipt_fact_count 9
  expect_kv "${lattice_source_path}" sample_validity_fraction 1
  expect_kv "${lattice_source_path}" missingness_fraction 0
  expect_kv "${lattice_source_path}" duplicate_sample_count 0
  expect_kv "${lattice_source_path}" source_health_level ok
  expect_kv "${lattice_source_path}" visibility_only true
  expect_kv "${lattice_source_path}" readiness_authority false
  expect_kv "${lattice_source_path}" coverage_authority false
  expect_kv "${lattice_source_path}" leakage_authority false
  expect_kv "${lattice_source_path}" contract_identity_authority false
}

verify_runtime_facts() {
  expect_kv "${component_update_path}" schema runtime.component_training_update.v1
  expect_kv "${component_update_path}" fact_type component_training_update
  expect_kv "${component_update_path}" component_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${component_update_path}" component_role mdn
  expect_kv "${component_update_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${component_update_path}" update_kind train
  expect_kv "${component_update_path}" fit_anchor_begin \
    "${expected_anchor_begin}"
  expect_kv "${component_update_path}" fit_anchor_end \
    "${expected_anchor_end}"
  expect_kv "${component_update_path}" read_checkpoint_digests \
    d6bd8d36a4f95957
  expect_kv "${component_update_path}" write_checkpoint_digest \
    2e1e3962f29512d8
  expect_kv "${component_update_path}" write_generation_id b2552f8160c4d8ef
  expect_kv "${component_update_path}" optimizer_update_count \
    "${expected_optimizer_steps}"
  expect_kv "${component_update_path}" source_cursor_token \
    "${expected_cursor_token}"
  expect_kv "${component_update_path}" fact_digest 05a103d54d651ea8

  expect_kv "${checkpoint_io_path}" fact_type runtime.checkpoint_io.fact
  expect_kv "${checkpoint_io_path}" schema_version 1
  expect_kv "${checkpoint_io_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${checkpoint_io_path}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${checkpoint_io_path}" checkpoint_path "${checkpoint_path}"
  expect_kv "${checkpoint_io_path}" checkpoint_path_reported \
    "${checkpoint_path}"
  expect_kv "${checkpoint_io_path}" checkpoint_written true
  expect_kv "${checkpoint_io_path}" checkpoint_write_count 70
  expect_kv "${checkpoint_io_path}" representation_checkpoint_loaded true
  expect_kv "${checkpoint_io_path}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${checkpoint_io_path}" mdn_checkpoint_loaded false
  expect_kv "${checkpoint_io_path}" mdn_checkpoint_path ''
  expect_kv "${checkpoint_io_path}" model_state_mutated true
  expect_kv "${checkpoint_io_path}" source_report_path "${report_path}"
  expect_kv "${checkpoint_io_path}" source_report_digest bc9498e8a5a7d83a
  expect_kv "${checkpoint_io_path}" checkpoint_artifact_digest ''
  expect_kv "${checkpoint_io_path}" checkpoint_artifact_status ''
  expect_kv "${checkpoint_io_path}" checkpoint_bytes ''
  expect_kv "${checkpoint_io_path}" checkpoint_digest_reported ''
  expect_kv "${checkpoint_io_path}" checkpoint_digest_verified ''
  expect_kv "${checkpoint_io_path}" checkpoint_file_exists ''

  expect_kv "${health_path}" fact_type runtime.health_measurement.fact
  expect_kv "${health_path}" schema_version 1
  expect_kv "${health_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${health_path}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${health_path}" source_report_path "${report_path}"
  expect_kv "${health_path}" source_report_digest bc9498e8a5a7d83a
  expect_kv "${health_path}" finite_parameter_check 1
  expect_kv "${health_path}" nonfinite_output_count 0
  expect_finite_number "${health_path}" grad_norm_last
  expect_finite_number "${health_path}" grad_norm_max_pre_clip
  expect_finite_number "${health_path}" sigma_min
  expect_finite_number "${health_path}" sigma_max

  expect_kv "${runtime_result_path}" fact_type runtime.result.fact
  expect_kv "${runtime_result_path}" schema_version 1
  expect_kv "${runtime_result_path}" status completed
  expect_kv "${runtime_result_path}" error_message ''
  expect_kv "${runtime_result_path}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${runtime_result_path}" job_kind channel_inference_mdn
  expect_kv "${runtime_result_path}" target_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${runtime_result_path}" wave_id train_core_channel_mdn
  expect_kv "${runtime_result_path}" wave_action train
  expect_kv "${runtime_result_path}" source_report_path "${report_path}"
  expect_kv "${runtime_result_path}" source_report_digest bc9498e8a5a7d83a
  expect_kv "${runtime_result_path}" optimizer_steps \
    "${expected_optimizer_steps}"
  expect_kv "${runtime_result_path}" checkpoint_path "${checkpoint_path}"
  expect_kv "${runtime_result_path}" checkpoint_written true
  expect_kv "${runtime_result_path}" model_state_mutated true
  expect_kv "${runtime_result_path}" finite_parameter_check 1
  expect_kv "${runtime_result_path}" nonfinite_output_count 0
  expect_finite_number "${runtime_result_path}" final_loss
  expect_finite_number "${runtime_result_path}" mean_loss
  expect_finite_number "${runtime_result_path}" grad_norm_last
  expect_finite_number "${runtime_result_path}" grad_norm_max_pre_clip
  if grep -Eqi '=(nan|[-+]?inf)(,|$)' "${health_path}" \
    "${runtime_result_path}"; then
    fail "terminal MDN health/result fact contains a non-finite numeric output"
  fi
}

verify_lls_and_runtime_inventory_semantics() {
  grep -Fqx 'schema:str = wikimyei.inference.expected_value.mdn.runtime.v1' \
    "${mdn_lls_path}" || fail "MDN LLS schema drifted"
  grep -Fqx 'optimizer_steps[0,+inf):uint = 3500' "${mdn_lls_path}" ||
    fail "MDN LLS optimizer step drifted"
  grep -Fqx 'wave_pulse_index[0,+inf):uint = 3500' "${mdn_lls_path}" ||
    fail "MDN LLS wave pulse drifted"
  grep -Fqx 'valid_target_count[0,+inf):uint = 5376' "${mdn_lls_path}" ||
    fail "MDN LLS valid-target count drifted"
  grep -Fqx 'trained:bool = true' "${mdn_lls_path}" ||
    fail "MDN LLS trained token drifted"
  grep -Fqx 'skipped:bool = false' "${mdn_lls_path}" ||
    fail "MDN LLS skipped token drifted"
  grep -Fqx 'optimizer_step_applied:bool = true' "${mdn_lls_path}" ||
    fail "MDN LLS optimizer application drifted"
  grep -Fqx 'nonfinite_output_count[0,+inf):uint = 0' "${mdn_lls_path}" ||
    fail "MDN LLS non-finite count drifted"
  grep -Eq '^loss\(-inf,\+inf\):double = [-+]?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$' \
    "${mdn_lls_path}" || fail "MDN LLS loss is not finite"
  grep -Eq '^grad_norm\[0,\+inf\):double = [0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$' \
    "${mdn_lls_path}" || fail "MDN LLS gradient norm is not finite"
  grep -Fqx 'schema:str = wikimyei.expression.nodelift.srl.runtime.v1' \
    "${nodelift_lls_path}" || fail "nodelift LLS schema drifted"
  grep -Fqx 'schema:str = wikimyei.representation.mtf_jepa_mae_vicreg.runtime.v1' \
    "${representation_lls_path}" || fail "representation LLS schema drifted"
  local shared_line
  for shared_line in \
    'batch_cursor_token:str = graph=4133db527907a8e4|begin=3|end=2389|requested_batch=64|anchors=64|first=1947542399999|last=1905206399999|index_count=64|index_first=596|index_last=106|index_hash=3b89c3c95daab5ef' \
    'graph_order_fingerprint:str = 4133db527907a8e4' \
    'wave_id:str = train_core_channel_mdn' \
    'source_range_policy:str = anchor_index' \
    'source_order_policy:str = random_per_epoch' \
    'requested_anchor_index_begin[0,+inf):uint = 0' \
    'requested_anchor_index_end[0,+inf):uint = 2496'; do
    grep -Fqx "${shared_line}" "${mdn_lls_path}" ||
      fail "MDN LLS final batch/wave identity drifted: ${shared_line}"
    grep -Fqx "${shared_line}" "${nodelift_lls_path}" ||
      fail "nodelift LLS final batch/wave identity drifted: ${shared_line}"
    grep -Fqx "${shared_line}" "${representation_lls_path}" ||
      fail "representation LLS final batch/wave identity drifted: ${shared_line}"
  done

  expect_kv "${spawn_ref_path}" schema \
    kikijyeba.runtime.component_spawn_ref.v1
  expect_kv "${spawn_ref_path}" component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${spawn_ref_path}" component_spawn_id rpu
  expect_kv "${spawn_ref_path}" component_spawn_fingerprint \
    5ba58d2de0fb7dcb
  expect_kv "${spawn_ref_path}" component_spawn_registry_id \
    beb0da09d45f3ccb
  expect_kv "${spawn_ref_path}" registry_path \
    system/component_spawn_registry.v1.lls

  expect_kv "${spawn_registry_path}" schema \
    kikijyeba.component_spawn_registry.v1
  expect_kv "${spawn_registry_path}" component_spawn_registry_id \
    beb0da09d45f3ccb
  expect_kv "${spawn_registry_path}" entry_count 1
  expect_kv "${spawn_registry_path}" entry_0_component_family_id \
    wikimyei.inference.expected_value.mdn
  expect_kv "${spawn_registry_path}" entry_0_component_spawn_fingerprint \
    5ba58d2de0fb7dcb
  expect_kv "${spawn_registry_path}" entry_0_first_seen_runtime_root \
    "${runtime_root}"

  expect_kv "${runtime_layout_path}" schema kikijyeba.runtime.layout.v1
  expect_kv "${runtime_layout_path}" runtime_root "${runtime_root}"
  expect_kv "${runtime_layout_path}" system_dir system
  expect_kv "${runtime_layout_path}" components_dir components
}

verify_completion_semantics() {
  verify_input_receipt
  verify_config_and_objective
  verify_manifest
  verify_report
  verify_job_state
  verify_lattice_facts
  verify_runtime_facts
  verify_lls_and_runtime_inventory_semantics

  [[ "$(realpath -e -- "${checkpoint_path}")" == "${checkpoint_path}" ]] ||
    fail "completed checkpoint is not canonical"
  [[ "${checkpoint_path}" != "${interrupted_checkpoint}" ]] ||
    fail "completed checkpoint aliases interrupted checkpoint path"
  [[ "$(sha256_of "${checkpoint_path}")" != \
    "${expected_interrupted_checkpoint_sha256}" ]] ||
    fail "completed checkpoint equals interrupted checkpoint bytes"
}

emit_receipt() {
  local destination="$1" index rel payload_rel
  cat >"${destination}" <<RECEIPT_HEAD
schema_id=${result_schema_id}
status=complete
runtime_root=${runtime_root}
job_dir=${job_dir}
runner_path=${execution_runner}
runner_sha256=${expected_execution_runner_sha256}
execution_runner_path=${execution_runner}
execution_runner_sha256=${expected_execution_runner_sha256}
completion_sealer_path=${script_path}
completion_sealer_sha256=$(sha256_of "${script_path}")
completion_correction_path=${completion_correction}
completion_correction_sha256=${expected_completion_correction_sha256}
validator_failure_reason=job_allowlist_omitted_seven_standard_runtime_facts_and_finite_parameter_check_component_encoding_mismatch
training_reexecution=false
input_receipt_path=${input_receipt}
input_receipt_sha256=${expected_input_receipt_sha256}
original_runner_path=${original_runner}
original_runner_sha256=${expected_original_runner_sha256}
recovery_amendment_path=${recovery_amendment}
recovery_amendment_sha256=${expected_recovery_amendment_sha256}
interruption_sealer_path=${interruption_sealer}
interruption_sealer_sha256=${expected_interruption_sealer_sha256}
interruption_receipt_path=${interruption_receipt}
interruption_receipt_sha256=${expected_interruption_receipt_sha256}
interrupted_runtime_path=${interrupted_runtime}
isolated_source_verifier_path=${source_verifier}
isolated_source_verifier_sha256=${expected_source_verifier_sha256}
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=${expected_source_closure_sha256}
cursor_alignment_erratum_receipt_path=${cursor_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=${expected_cursor_erratum_sha256}
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=${expected_config_sha256}
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=${expected_runtime_exec_sha256}
representation_runner_path=${representation_runner}
representation_runner_sha256=${expected_representation_runner_sha256}
representation_result_path=${representation_result}
representation_result_sha256=${expected_representation_result_sha256}
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=${expected_representation_checkpoint_sha256}
mdn_objective_path=${objective_path}
mdn_objective_sha256=${expected_objective_sha256}
input_mdn_checkpoint_path=
forbidden_interrupted_mdn_checkpoint_path=${interrupted_checkpoint}
forbidden_interrupted_mdn_checkpoint_sha256=${expected_interrupted_checkpoint_sha256}
checkpoint_path=${checkpoint_path}
checkpoint_sha256=${expected_payload_hashes[job/channel_inference.report.channel_mdn.pt]}
job_manifest_path=${manifest_path}
job_manifest_sha256=${expected_payload_hashes[job/job.manifest]}
runtime_result_path=${runtime_result_path}
runtime_result_sha256=${expected_payload_hashes[job/runtime.result.fact]}
mdn_report_path=${report_path}
mdn_report_sha256=${expected_payload_hashes[job/channel_inference.report]}
train_log_path=${log_path}
train_log_sha256=${expected_log_sha256}
execution_lock_path=${execution_lock}
execution_lock_sha256=${expected_lock_sha256}
payload_inventory_format=sha256_space_bytes_space_relative_path_lf.c_locale_path_order.v1
payload_inventory_master_sha256=${expected_inventory_master_sha256}
payload_directory_count=${expected_directory_count}
payload_file_count_before_receipt=${expected_payload_file_count}
sealed_file_count_including_receipt=$((expected_payload_file_count + 1))
payload_bytes_before_receipt=${expected_payload_bytes}
job_file_count=${expected_job_file_count}
component_file_count=${expected_component_file_count}
system_file_count=${expected_system_file_count}
RECEIPT_HEAD
  for index in "${!job_relpaths[@]}"; do
    rel="${job_relpaths[${index}]}"
    payload_rel="job/${rel}"
    printf 'job_file_%02d_path=%s/%s\n' "${index}" "${job_dir}" "${rel}" \
      >>"${destination}"
    printf 'job_file_%02d_sha256=%s\n' "${index}" \
      "${expected_payload_hashes[${payload_rel}]}" >>"${destination}"
    printf 'job_file_%02d_bytes=%s\n' "${index}" \
      "${expected_payload_sizes[${payload_rel}]}" >>"${destination}"
  done
  cat >>"${destination}" <<RECEIPT_TAIL
component_file_00_path=${spawn_ref_path}
component_file_00_sha256=${expected_payload_hashes[components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref]}
component_file_00_bytes=${expected_payload_sizes[components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref]}
system_file_00_path=${spawn_registry_path}
system_file_00_sha256=${expected_payload_hashes[system/component_spawn_registry.v1.lls]}
system_file_00_bytes=${expected_payload_sizes[system/component_spawn_registry.v1.lls]}
system_file_01_path=${runtime_layout_path}
system_file_01_sha256=${expected_payload_hashes[system/runtime_layout.v1.lls]}
system_file_01_bytes=${expected_payload_sizes[system/runtime_layout.v1.lls]}
config_snapshot_bytes=${expected_payload_sizes[synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config]}
input_receipt_bytes=${expected_payload_sizes[inputs.status]}
train_log_bytes=${expected_payload_sizes[mdn_train.retry1.log]}
execution_lock_bytes=${expected_payload_sizes[.execution.lock]}
representation_probe_row_count=${expected_representation_probe_rows}
runtime_event_probe_record_count=${expected_runtime_probe_records}
train_anchor_range=[${expected_anchor_begin},${expected_anchor_end})
accepted_anchor_count=${expected_anchor_count}
candidate_anchor_count=${expected_anchor_count}
maximum_available_anchor_index=3260
skipped_outside_common_range=0
skipped_missing_edge_coverage=0
skipped_failed_fetch_probe=0
duplicate_anchor_count=0
optimizer_steps=${expected_optimizer_steps}
seed=${expected_seed}
execution_chain=${expected_execution_chain}
stream_plan=${expected_stream_plan}
objective_vector=${expected_objective_vector}
report_finite_parameter_check_token=1
runtime_result_finite_parameter_check_token=1
runtime_health_finite_parameter_check_token=1
lattice_exposure_finite_parameter_check_token=true
lattice_exposure_gradients_finite_token=false
runtime_checkpoint_io_publication_defaults_preserved=true
lattice_exposure_publication_defaults_preserved=true
payload_sealed_before_receipt_publication=true
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
final_holdout_available=false
independent_final_evidence=false
policy_access=false
RECEIPT_TAIL
}

verify_exact_receipt() {
  local inspected_path="${1:-${result_receipt}}"
  reject_symlink_components "${inspected_path}"
  require_file "${inspected_path}"
  [[ "$(stat -c '%h' -- "${inspected_path}")" == 1 ]] ||
    fail "completion receipt has an external hard link"

  emit_receipt /dev/stdout | awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || key ~ /[[:space:]]/ || seen[key]++) exit 42;
    }
  ' >/dev/null ||
    fail "internal expected completion receipt keyset is invalid"
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || key ~ /[[:space:]]/ || seen[key]++) exit 42;
    }
  ' "${inspected_path}" >/dev/null ||
    fail "completion receipt contains a malformed or duplicate key"
  cmp -s -- <(emit_receipt /dev/stdout) "${inspected_path}" ||
    fail "completion receipt content drifted"
}

receipt_matches_expected() {
  local inspected_path="$1"
  cmp -s -- <(emit_receipt /dev/stdout) "${inspected_path}"
}

prepare_external_staging() {
  reject_symlink_components "${runtime_parent}"
  require_dir "${runtime_parent}"
  [[ "$(realpath -e -- "${runtime_parent}")" == "${runtime_parent}" ]] ||
    fail "runtime parent is not canonical"
  reject_symlink_components "${staging_path}"

  if [[ -e "${staging_path}" || -L "${staging_path}" ]]; then
    require_file "${staging_path}"
    [[ "$(stat -c '%h' -- "${staging_path}")" == 1 ]] ||
      fail "external staging file has unexpected hard links"
    [[ "$(stat -c '%u' -- "${staging_path}")" == "$(id -u)" ]] ||
      fail "external staging file has unexpected ownership"
    if receipt_matches_expected "${staging_path}"; then
      chmod 0444 -- "${staging_path}"
      verify_exact_receipt "${staging_path}"
      return
    fi
    fail "refusing to remove or replace nonmatching deterministic staging residue"
  fi

  if ! (set -o noclobber; emit_receipt "${staging_path}"); then
    fail "could not create deterministic external result staging file"
  fi
  chmod 0444 -- "${staging_path}"
  verify_exact_receipt "${staging_path}"
}

recover_published_staging() {
  if [[ ! -e "${staging_path}" && ! -L "${staging_path}" ]]; then
    return
  fi
  reject_symlink_components "${staging_path}"
  require_file "${staging_path}"
  if [[ ! -e "${result_receipt}" && ! -L "${result_receipt}" ]]; then
    return
  fi
  reject_symlink_components "${result_receipt}"
  require_file "${result_receipt}"
  [[ "${staging_path}" -ef "${result_receipt}" ]] ||
    fail "published result and external staging are not the same inode"
  receipt_matches_expected "${staging_path}" ||
    fail "published result staging content drifted"
  [[ "$(stat -c '%h' -- "${staging_path}")" == 2 ]] ||
    fail "published result staging has an unexpected link count"
  rm -f -- "${staging_path}"
}

seal_payload_except_root() {
  local rel
  for rel in "${payload_relpaths[@]}"; do
    chmod 0444 -- "${runtime_root}/${rel}"
  done
  for rel in "${expected_directories[@]}"; do
    [[ "${rel}" == "." ]] && continue
    chmod 0555 -- "${runtime_root}/${rel}"
  done
}

assert_payload_sealed_except_root() {
  local rel
  for rel in "${payload_relpaths[@]}"; do
    [[ "$(stat -c '%a' -- "${runtime_root}/${rel}")" == 444 ]] ||
      fail "payload file is not mode 0444: ${rel}"
  done
  for rel in "${expected_directories[@]}"; do
    [[ "${rel}" == "." ]] && continue
    [[ "$(stat -c '%a' -- "${runtime_root}/${rel}")" == 555 ]] ||
      fail "payload directory is not mode 0555: ${rel}"
  done
}

seal_completed_tree() {
  seal_payload_except_root
  chmod 0444 -- "${result_receipt}"
  chmod 0555 -- "${runtime_root}"
}

assert_completed_tree_sealed() {
  local rel writable
  assert_payload_sealed_except_root
  [[ "$(stat -c '%a' -- "${result_receipt}")" == 444 ]] ||
    fail "result receipt is not mode 0444"
  [[ "$(stat -c '%a' -- "${runtime_root}")" == 555 ]] ||
    fail "runtime root is not mode 0555"
  writable="$(find "${runtime_root}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "completed runtime contains a writable entry: ${writable}"
}

publish_receipt_after_payload_seal() {
  [[ ! -e "${result_receipt}" && ! -L "${result_receipt}" ]] ||
    fail "refusing to overwrite existing completion receipt"
  [[ "$(stat -c '%A' -- "${runtime_root}")" == *w* ]] ||
    fail "runtime root must remain writable until atomic receipt publication"
  if ! ln -- "${staging_path}" "${result_receipt}"; then
    fail "atomic no-clobber completion receipt publication failed"
  fi
  rm -f -- "${staging_path}"
  chmod 0555 -- "${runtime_root}"
  verify_exact_receipt "${result_receipt}"
}

acquire_execution_lock() {
  reject_symlink_components "${execution_lock}"
  require_file "${execution_lock}"
  exec {execution_lock_fd}<"${execution_lock}"
  flock -n "${execution_lock_fd}" ||
    fail "retry1 execution lock is held; refusing to seal or verify"
}

verify_payload() {
  local expect_receipt="$1"
  verify_exact_tree "${expect_receipt}"
  verify_payload_hashes_sizes_and_master
  verify_completion_semantics
}

verify_sealed() {
  verify_external_provenance
  verify_payload true
  verify_exact_receipt "${result_receipt}"
  assert_completed_tree_sealed
  echo "mdn_checkpoint=${checkpoint_path}"
  echo "mdn_checkpoint_sha256=${expected_payload_hashes[job/channel_inference.report.channel_mdn.pt]}"
  echo "completion_result=${result_receipt}"
  echo "completion_result_sha256=$(sha256_of "${result_receipt}")"
}

acquire_execution_lock

if [[ "${mode}" == --verify ]]; then
  verify_sealed
  exit 0
fi

recover_published_staging

if [[ -e "${result_receipt}" || -L "${result_receipt}" ]]; then
  verify_external_provenance
  verify_payload true
  verify_exact_receipt "${result_receipt}"
  seal_completed_tree
  verify_sealed
  exit 0
fi

verify_external_provenance
verify_payload false
prepare_external_staging
seal_payload_except_root
verify_exact_tree false
verify_payload_hashes_sizes_and_master
assert_payload_sealed_except_root
publish_receipt_after_payload_seal
verify_sealed
