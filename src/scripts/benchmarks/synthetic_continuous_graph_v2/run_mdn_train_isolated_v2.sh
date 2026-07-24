#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_mdn_train_isolated_v2"
readonly result_schema_id="synthetic_v2_mdn_train_isolated_v2.result.v1"
readonly representation_result_schema_id="synthetic_v2_representation_train_isolated_v2.result.v1"
readonly source_schema_id="synthetic_v2_isolated_development_source_v1"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_source_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_alignment_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
readonly expected_steps=3500
readonly train_begin=0
readonly train_end=2496
readonly accepted_anchor_count=3261

fail() {
  echo "v2 isolated-source MDN train: $*" >&2
  exit 1
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

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local key="$1"
  local path="$2"
  local count value
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
  local path="$1"
  local key="$2"
  local expected="$3"
  local actual
  actual="$(kv "${key}" "${path}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
source_closure="${source_runtime}/development_source_closure.status"
source_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
source_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_alignment_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
cursor_alignment_erratum_receipt="${source_runtime}/cursor_alignment_erratum.status"
representation_runner="${script_dir}/run_representation_train_isolated_v2.sh"
representation_result="${runtime_parent}/synthetic_v2_representation_train_isolated_v2/result.status"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
runtime_root="${runtime_parent}/${schema_id}"
config_snapshot="${runtime_root}/synthetic_benchmark.train_core_channel_mdn.isolated.config"
job_dir="${runtime_root}/job"
log_path="${runtime_root}/mdn_train.log"
input_receipt="${runtime_root}/inputs.status"
result_receipt="${runtime_root}/result.status"

verify_fixed_directory() {
  local path="$1"
  reject_symlink_components "${path}"
  require_dir "${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "directory is not canonical: ${path}"
}

verify_source_hierarchy() {
  verify_fixed_directory "${runtime_parent}"
  verify_fixed_directory "${source_runtime}"
  reject_symlink_components "${source_closure}"
  reject_symlink_components "${representation_result}"
}

prepare_runtime_root() {
  local path
  verify_source_hierarchy
  reject_symlink_components "${runtime_root}"
  if [[ -e "${runtime_root}" || -L "${runtime_root}" ]]; then
    require_dir "${runtime_root}"
  else
    mkdir -- "${runtime_root}"
  fi
  verify_fixed_directory "${runtime_root}"
  for path in "${config_snapshot}" "${job_dir}" "${log_path}" \
    "${input_receipt}" "${result_receipt}" \
    "${runtime_root}/.execution.lock"; do
    reject_symlink_components "${path}"
  done
}

verify_runtime_hierarchy() {
  local path
  verify_source_hierarchy
  verify_fixed_directory "${runtime_root}"
  for path in "${config_snapshot}" "${job_dir}" "${log_path}" \
    "${input_receipt}" "${result_receipt}"; do
    reject_symlink_components "${path}"
  done
}

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--run|--verify]"
case "${mode}" in
--plan | --run | --verify) ;;
*) fail "usage: $0 [--plan|--run|--verify]" ;;
esac

if [[ "${mode}" == --plan ]]; then
  cat <<PLAN
schema_id=${schema_id}.plan
source_closure_schema=${source_schema_id}
input_representation=synthetic_v2_representation_train_isolated_v2
accepted_anchor_count=${accepted_anchor_count}
maximum_available_anchor=3260
train_anchor_range=[${train_begin},${train_end})
optimizer_steps=${expected_steps}
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
independent_final_evidence=false
policy_access=false
PLAN
  exit 0
fi

verify_source_receipts() {
  local manifest="$1"
  local isolated_source_root receipts receipt source instrument interval
  local edge_field instrument_field interval_field record_field source_field extra
  local count=0 key expected_source expected_receipt
  local -A seen=()
  isolated_source_root="$(kv isolated_source_root_path "${source_closure}")"
  isolated_source_root="$(realpath -e -- "${isolated_source_root}")"
  [[ "${isolated_source_root}" == "${source_runtime}/source" ]] ||
    fail "unexpected isolated source root: ${isolated_source_root}"
  receipts="$(kv source_file_receipts "${manifest}")"
  [[ -n "${receipts}" ]] || fail "job manifest omitted source_file_receipts"
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

verify_static_inputs() {
  verify_source_hierarchy
  require_file "${source_closure}"
  require_file "${source_verifier}"
  require_file "${source_amendment}"
  require_file "${source_protocol}"
  require_file "${staged_hardening}"
  require_file "${cursor_alignment_correction}"
  require_file "${cursor_alignment_metadata_erratum}"
  require_file "${cursor_alignment_erratum_receipt}"
  require_file "${representation_runner}"
  require_file "${runtime_exec}"
  [[ "$(sha256_of "${runtime_exec}")" == "${expected_runtime_exec_sha256}" ]] ||
    fail "Runtime executable hash drifted"
  [[ "$(sha256_of "${source_amendment}")" == "${expected_source_amendment_sha256}" ]] ||
    fail "source-isolation amendment hash drifted"
  [[ "$(sha256_of "${source_protocol}")" == "${expected_source_protocol_sha256}" ]] ||
    fail "isolated-source protocol hash drifted"
  [[ "$(sha256_of "${staged_hardening}")" == "${expected_staged_hardening_sha256}" ]] ||
    fail "staged hardening hash drifted"
  [[ "$(sha256_of "${cursor_alignment_correction}")" == \
    "${expected_cursor_alignment_correction_sha256}" ]] ||
    fail "cursor-alignment correction hash drifted"
  [[ "$(sha256_of "${cursor_alignment_metadata_erratum}")" == \
    "${expected_cursor_alignment_metadata_erratum_sha256}" ]] ||
    fail "cursor-alignment metadata erratum hash drifted"
  "${source_verifier}" --verify >/dev/null
  expect_kv "${cursor_alignment_erratum_receipt}" schema_id \
    synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1
  expect_kv "${cursor_alignment_erratum_receipt}" status complete
  expect_kv "${cursor_alignment_erratum_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${cursor_alignment_erratum_receipt}" \
    cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${cursor_alignment_erratum_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${cursor_alignment_erratum_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${cursor_alignment_erratum_receipt}" maximum_anchor_index 3260
  "${representation_runner}" --verify >/dev/null
  expect_kv "${source_closure}" schema_id "${source_schema_id}"
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" maximum_anchor_index 3260
  expect_kv "${source_closure}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false
}

isolated_base_config_path() {
  local config registry active_root_count
  config="$(kv isolated_base_config_path "${source_closure}")"
  reject_symlink_components "${config}"
  require_file "${config}"
  expect_kv "${source_closure}" isolated_base_config_sha256 \
    "$(sha256_of "${config}")"
  [[ "${config}" == "${source_runtime}/config/synthetic_benchmark.isolated_development.config" ]] ||
    fail "unexpected isolated base config: ${config}"
  registry="$(kv ujcamei_source_registry_dsl_path "${config}")"
  reject_symlink_components "${registry}"
  expect_kv "${source_closure}" isolated_registry_path "${registry}"
  require_file "${registry}"
  expect_kv "${source_closure}" isolated_registry_sha256 \
    "$(sha256_of "${registry}")"
  expect_kv "${registry}" SOURCE_ROOT "${source_runtime}/source;"
  active_root_count="$(awk '
    /^[[:space:]]*SOURCE_ROOT[[:space:]]*=/ { ++count }
    END { print count + 0 }
  ' "${registry}")"
  [[ "${active_root_count}" == 1 ]] ||
    fail "isolated registry must contain exactly one active SOURCE_ROOT"
  expect_kv "${config}" runtime_wave_id policy_training_ppo_v0
  printf '%s' "${config}"
}

representation_checkpoint_path() {
  reject_symlink_components "${representation_result}"
  require_file "${representation_result}"
  [[ "$(stat -c '%A' -- "${representation_result}")" != *w* ]] ||
    fail "clean representation result receipt is writable"
  expect_kv "${representation_result}" schema_id \
    "${representation_result_schema_id}"
  expect_kv "${representation_result}" status complete
  expect_kv "${representation_result}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  expect_kv "${representation_result}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${representation_result}" restart_from_step_zero true
  expect_kv "${representation_result}" quarantined_checkpoint_access false
  expect_kv "${representation_result}" canonical_data_raw_access false
  local checkpoint
  checkpoint="$(kv checkpoint_path "${representation_result}")"
  [[ "${checkpoint}" == \
    "${runtime_parent}/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt" ]] ||
    fail "representation result points outside the exact clean job"
  reject_symlink_components "${checkpoint}"
  require_file "${checkpoint}"
  [[ "$(stat -c '%A' -- "${checkpoint}")" != *w* ]] ||
    fail "clean representation checkpoint is writable"
  expect_kv "${representation_result}" checkpoint_sha256 \
    "$(sha256_of "${checkpoint}")"
  printf '%s' "${checkpoint}"
}

derive_config_snapshot() {
  local base_config="$1"
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= train_core_channel_mdn");
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${base_config}"
}

verify_config_snapshot() {
  local base_config="$1"
  reject_symlink_components "${config_snapshot}"
  require_file "${config_snapshot}"
  [[ "$(stat -c '%A' -- "${config_snapshot}")" != *w* ]] ||
    fail "isolated MDN config is writable"
  cmp -s -- <(derive_config_snapshot "${base_config}") "${config_snapshot}" ||
    fail "isolated MDN config is not the fixed base derivation"
  expect_kv "${config_snapshot}" runtime_wave_id train_core_channel_mdn
  expect_kv "${config_snapshot}" ujcamei_source_registry_dsl_path \
    "${source_runtime}/config/ujcamei.source.registry.development_prefix.dsl"
}

write_config_snapshot() {
  local base_config="$1"
  local candidate
  mkdir -p "${runtime_root}"
  candidate="$(mktemp "${runtime_root}/.config.XXXXXX")"
  derive_config_snapshot "${base_config}" >"${candidate}" ||
    fail "could not derive isolated MDN config"
  chmod 0444 "${candidate}"
  if [[ -e "${config_snapshot}" ]]; then
    cmp -s -- "${candidate}" "${config_snapshot}" ||
      fail "immutable isolated MDN config drifted"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${config_snapshot}" ||
      fail "isolated MDN config publication failed"
    rm -f -- "${candidate}"
  fi
  verify_config_snapshot "${base_config}"
}

emit_inputs() {
  local destination="$1"
  local representation_checkpoint="$2"
  cat >"${destination}" <<INPUTS
schema_id=${schema_id}.inputs.v1
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
isolated_source_verifier_path=${source_verifier}
isolated_source_verifier_sha256=$(sha256_of "${source_verifier}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
source_isolation_amendment_sha256=$(sha256_of "${source_amendment}")
isolated_source_protocol_sha256=$(sha256_of "${source_protocol}")
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${cursor_alignment_metadata_erratum}")
cursor_alignment_erratum_receipt_path=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_alignment_erratum_receipt}")
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=$(sha256_of "${config_snapshot}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
train_anchor_begin=${train_begin}
train_anchor_end_exclusive=${train_end}
expected_optimizer_steps=${expected_steps}
expected_accepted_anchor_count=${accepted_anchor_count}
expected_candidate_anchor_count=${accepted_anchor_count}
expected_maximum_available_anchor_index=3260
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
final_holdout_available=false
policy_access=false
INPUTS
}

verify_input_receipt() {
  local representation_checkpoint="$1"
  require_file "${input_receipt}"
  [[ "$(stat -c '%A' -- "${input_receipt}")" != *w* ]] ||
    fail "input receipt is writable"
  expect_kv "${input_receipt}" schema_id "${schema_id}.inputs.v1"
  expect_kv "${input_receipt}" runner_path "${script_path}"
  expect_kv "${input_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${input_receipt}" isolated_source_verifier_path \
    "${source_verifier}"
  expect_kv "${input_receipt}" isolated_source_verifier_sha256 \
    "$(sha256_of "${source_verifier}")"
  expect_kv "${input_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${input_receipt}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  expect_kv "${input_receipt}" source_isolation_amendment_sha256 \
    "${expected_source_amendment_sha256}"
  expect_kv "${input_receipt}" isolated_source_protocol_sha256 \
    "${expected_source_protocol_sha256}"
  expect_kv "${input_receipt}" staged_hardening_amendment_sha256 \
    "${expected_staged_hardening_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${input_receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${input_receipt}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${input_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "$(sha256_of "${cursor_alignment_erratum_receipt}")"
  expect_kv "${input_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${input_receipt}" config_snapshot_sha256 \
    "$(sha256_of "${config_snapshot}")"
  expect_kv "${input_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${input_receipt}" runtime_exec_sha256 \
    "$(sha256_of "${runtime_exec}")"
  expect_kv "${input_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${input_receipt}" representation_result_sha256 \
    "$(sha256_of "${representation_result}")"
  expect_kv "${input_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${input_receipt}" representation_checkpoint_sha256 \
    "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${input_receipt}" train_anchor_begin "${train_begin}"
  expect_kv "${input_receipt}" train_anchor_end_exclusive "${train_end}"
  expect_kv "${input_receipt}" expected_optimizer_steps "${expected_steps}"
  expect_kv "${input_receipt}" expected_accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${input_receipt}" expected_candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${input_receipt}" expected_maximum_available_anchor_index 3260
  expect_kv "${input_receipt}" restart_from_step_zero true
  expect_kv "${input_receipt}" quarantined_checkpoint_access false
  expect_kv "${input_receipt}" canonical_data_raw_access false
  expect_kv "${input_receipt}" final_holdout_available false
  expect_kv "${input_receipt}" policy_access false
}

validate_job() {
  local representation_checkpoint="$1"
  local result="${job_dir}/runtime.result.fact"
  local manifest="${job_dir}/job.manifest"
  local report="${job_dir}/channel_inference.report"
  local checkpoint expected_checkpoint
  verify_fixed_directory "${job_dir}"
  reject_symlink_components "${result}"
  reject_symlink_components "${manifest}"
  reject_symlink_components "${report}"
  require_file "${result}"
  require_file "${manifest}"
  require_file "${report}"
  require_file "${log_path}"
  expect_kv "${result}" status completed
  expect_kv "${result}" wave_id train_core_channel_mdn
  expect_kv "${result}" wave_action train
  expect_kv "${result}" job_id \
    train_core_channel_mdn.train.channel_inference_mdn
  expect_kv "${result}" source_report_path "${report}"
  expect_kv "${result}" optimizer_steps "${expected_steps}"
  expect_kv "${result}" checkpoint_written true
  expect_kv "${result}" model_state_mutated true
  expect_kv "${result}" finite_parameter_check 1
  expect_kv "${result}" nonfinite_output_count 0
  expect_kv "${manifest}" config_path "${config_snapshot}"
  expect_kv "${manifest}" wave_id train_core_channel_mdn
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" source_range_policy anchor_index
  expect_kv "${manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${train_end}"
  expect_kv "${manifest}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  expect_kv "${report}" representation_checkpoint_loaded true
  expect_kv "${report}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${report}" mdn_checkpoint_loaded false
  expect_kv "${report}" training_id \
    synthetic_continuous_graph_v2_channel_mdn_train_core_v1
  expect_kv "${report}" wave_id train_core_channel_mdn
  expect_kv "${report}" optimizer_steps "${expected_steps}"
  expect_kv "${report}" finite_parameter_check 1
  expect_kv "${report}" nonfinite_output_count 0
  verify_source_receipts "${manifest}"
  checkpoint="$(kv checkpoint_path "${result}")"
  expected_checkpoint="${job_dir}/channel_inference.report.channel_mdn.pt"
  [[ "${checkpoint}" == "${expected_checkpoint}" ]] ||
    fail "unexpected MDN checkpoint path: ${checkpoint}"
  reject_symlink_components "${checkpoint}"
  require_file "${checkpoint}"
  [[ "$(realpath -e -- "${checkpoint}")" == "${checkpoint}" ]] ||
    fail "MDN checkpoint path is not canonical"
  printf '%s' "${checkpoint}"
}

seal_job_artifacts() {
  local symlink special
  verify_fixed_directory "${job_dir}"
  symlink="$(find "${job_dir}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "training job contains a symlink: ${symlink}"
  special="$(find "${job_dir}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "training job contains a special entry: ${special}"
  find "${job_dir}" -type f -exec chmod 0444 -- {} +
  find "${job_dir}" -depth -type d -exec chmod 0555 -- {} +
  chmod 0444 -- "${log_path}"
}

assert_job_artifacts_sealed() {
  local writable
  writable="$(find "${job_dir}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] ||
    fail "training job artifact remains writable: ${writable}"
  [[ "$(stat -c '%A' -- "${log_path}")" != *w* ]] ||
    fail "MDN training log remains writable"
}

write_result_receipt() {
  local checkpoint="$1"
  local representation_checkpoint="$2"
  local candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  cat >"${candidate}" <<RESULT
schema_id=${result_schema_id}
status=complete
job_dir=${job_dir}
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${cursor_alignment_metadata_erratum}")
cursor_alignment_erratum_receipt_path=${cursor_alignment_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_alignment_erratum_receipt}")
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=$(sha256_of "${config_snapshot}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_result_path=${representation_result}
representation_result_sha256=$(sha256_of "${representation_result}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
checkpoint_path=${checkpoint}
checkpoint_sha256=$(sha256_of "${checkpoint}")
job_manifest_path=${job_dir}/job.manifest
job_manifest_sha256=$(sha256_of "${job_dir}/job.manifest")
runtime_result_path=${job_dir}/runtime.result.fact
runtime_result_sha256=$(sha256_of "${job_dir}/runtime.result.fact")
mdn_report_path=${job_dir}/channel_inference.report
mdn_report_sha256=$(sha256_of "${job_dir}/channel_inference.report")
train_log_path=${log_path}
train_log_sha256=$(sha256_of "${log_path}")
train_anchor_range=[${train_begin},${train_end})
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor_index=3260
optimizer_steps=${expected_steps}
restart_from_step_zero=true
quarantined_checkpoint_access=false
canonical_data_raw_access=false
final_holdout_available=false
policy_access=false
RESULT
  chmod 0444 "${candidate}"
  if [[ -e "${result_receipt}" ]]; then
    cmp -s -- "${candidate}" "${result_receipt}" ||
      fail "immutable result receipt drifted"
    rm -f -- "${candidate}"
  else
    ln -- "${candidate}" "${result_receipt}" ||
      fail "result receipt publication failed"
    rm -f -- "${candidate}"
  fi
}

verify_completed() {
  verify_static_inputs
  verify_runtime_hierarchy
  local base_config representation_checkpoint checkpoint
  base_config="$(isolated_base_config_path)"
  verify_config_snapshot "${base_config}"
  representation_checkpoint="$(representation_checkpoint_path)"
  verify_input_receipt "${representation_checkpoint}"
  checkpoint="$(validate_job "${representation_checkpoint}")"
  assert_job_artifacts_sealed
  require_file "${result_receipt}"
  [[ "$(stat -c '%A' -- "${result_receipt}")" != *w* ]] ||
    fail "result receipt is writable"
  expect_kv "${result_receipt}" schema_id "${result_schema_id}"
  expect_kv "${result_receipt}" status complete
  expect_kv "${result_receipt}" job_dir "${job_dir}"
  expect_kv "${result_receipt}" runner_path "${script_path}"
  expect_kv "${result_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${result_receipt}" input_receipt_path "${input_receipt}"
  expect_kv "${result_receipt}" input_receipt_sha256 \
    "$(sha256_of "${input_receipt}")"
  expect_kv "${result_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${result_receipt}" isolated_source_closure_sha256 \
    "$(sha256_of "${source_closure}")"
  expect_kv "${result_receipt}" cursor_alignment_correction_path \
    "${cursor_alignment_correction}"
  expect_kv "${result_receipt}" cursor_alignment_correction_sha256 \
    "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${result_receipt}" cursor_alignment_metadata_erratum_path \
    "${cursor_alignment_metadata_erratum}"
  expect_kv "${result_receipt}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${result_receipt}" cursor_alignment_erratum_receipt_path \
    "${cursor_alignment_erratum_receipt}"
  expect_kv "${result_receipt}" cursor_alignment_erratum_receipt_sha256 \
    "$(sha256_of "${cursor_alignment_erratum_receipt}")"
  expect_kv "${result_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${result_receipt}" config_snapshot_sha256 \
    "$(sha256_of "${config_snapshot}")"
  expect_kv "${result_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${result_receipt}" runtime_exec_sha256 \
    "$(sha256_of "${runtime_exec}")"
  expect_kv "${result_receipt}" representation_result_path \
    "${representation_result}"
  expect_kv "${result_receipt}" representation_result_sha256 \
    "$(sha256_of "${representation_result}")"
  expect_kv "${result_receipt}" representation_checkpoint_path \
    "${representation_checkpoint}"
  expect_kv "${result_receipt}" representation_checkpoint_sha256 \
    "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${result_receipt}" checkpoint_path "${checkpoint}"
  expect_kv "${result_receipt}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  expect_kv "${result_receipt}" job_manifest_path "${job_dir}/job.manifest"
  expect_kv "${result_receipt}" job_manifest_sha256 \
    "$(sha256_of "${job_dir}/job.manifest")"
  expect_kv "${result_receipt}" runtime_result_path \
    "${job_dir}/runtime.result.fact"
  expect_kv "${result_receipt}" runtime_result_sha256 \
    "$(sha256_of "${job_dir}/runtime.result.fact")"
  expect_kv "${result_receipt}" mdn_report_path \
    "${job_dir}/channel_inference.report"
  expect_kv "${result_receipt}" mdn_report_sha256 \
    "$(sha256_of "${job_dir}/channel_inference.report")"
  expect_kv "${result_receipt}" train_log_path "${log_path}"
  expect_kv "${result_receipt}" train_log_sha256 \
    "$(sha256_of "${log_path}")"
  expect_kv "${result_receipt}" train_anchor_range \
    "[${train_begin},${train_end})"
  expect_kv "${result_receipt}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${result_receipt}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${result_receipt}" maximum_available_anchor_index 3260
  expect_kv "${result_receipt}" optimizer_steps "${expected_steps}"
  expect_kv "${result_receipt}" restart_from_step_zero true
  expect_kv "${result_receipt}" quarantined_checkpoint_access false
  expect_kv "${result_receipt}" canonical_data_raw_access false
  expect_kv "${result_receipt}" final_holdout_available false
  expect_kv "${result_receipt}" policy_access false
  echo "mdn_checkpoint=${checkpoint}"
  echo "mdn_checkpoint_sha256=$(sha256_of "${checkpoint}")"
}

if [[ "${mode}" == --verify ]]; then
  verify_completed
  exit 0
fi

verify_static_inputs
prepare_runtime_root
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || fail "another isolated MDN run holds the lock"
base_config="$(isolated_base_config_path)"
write_config_snapshot "${base_config}"
representation_checkpoint="$(representation_checkpoint_path)"

candidate_inputs="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
emit_inputs "${candidate_inputs}" "${representation_checkpoint}"
chmod 0444 "${candidate_inputs}"
if [[ -e "${input_receipt}" ]]; then
  cmp -s -- "${candidate_inputs}" "${input_receipt}" ||
    fail "immutable input receipt drifted"
  rm -f -- "${candidate_inputs}"
else
  ln -- "${candidate_inputs}" "${input_receipt}" ||
    fail "input receipt publication failed"
  rm -f -- "${candidate_inputs}"
fi

if [[ ! -e "${job_dir}" ]]; then
  "${runtime_exec}" \
    --config "${config_snapshot}" \
    --job-dir "${job_dir}" \
    --source-range anchor_index \
    --anchor-index-begin "${train_begin}" \
    --anchor-index-end "${train_end}" \
    --input-representation-checkpoint "${representation_checkpoint}" \
    --no-replay-artifacts >"${log_path}" 2>&1 ||
    fail "isolated MDN training failed; see ${log_path}"
fi

checkpoint="$(validate_job "${representation_checkpoint}")"
seal_job_artifacts
checkpoint="$(validate_job "${representation_checkpoint}")"
assert_job_artifacts_sealed
"${source_verifier}" --verify >/dev/null
write_result_receipt "${checkpoint}" "${representation_checkpoint}"
verify_completed
