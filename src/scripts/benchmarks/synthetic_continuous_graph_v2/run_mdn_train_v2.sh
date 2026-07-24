#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_mdn_train_v1"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_data_closure_sha256="36345440fae3ef03d548083e1a44ad05dca19b502aa186b45b04cc01b128c831"
readonly expected_steps=3500
readonly train_begin=0
readonly train_end=2496

fail() {
  echo "v2 MDN train: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local key="$1"
  local path="$2"
  awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) {
        value = substr($0, eq + 1);
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
        print value;
        exit;
      }
    }
  ' "${path}"
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
closure_report="${benchmark_root}/artifacts/fresh_seed_data_closure.v2.report"
closure_verifier="${script_dir}/prepare_and_seal_fresh_seed.sh"
representation_runner="${script_dir}/run_representation_train_v2.sh"
representation_result="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_v1/result.status"
representation_config="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/preparation/config/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.config"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/${schema_id}"
config_snapshot="${runtime_root}/synthetic_benchmark.train_core_channel_mdn.config"
job_dir="${runtime_root}/job"
log_path="${runtime_root}/mdn_train.log"
input_receipt="${runtime_root}/inputs.status"
result_receipt="${runtime_root}/result.status"

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--run|--verify]"
case "${mode}" in
--plan | --run | --verify) ;;
*) fail "usage: $0 [--plan|--run|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
train_anchor_range=[${train_begin},${train_end})
optimizer_steps=${expected_steps}
input_representation=synthetic_v2_representation_train_v1
mdn_policy=synthetic_continuous_graph_v2_channel_mdn_train_core_v1
final_holdout_access=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

representation_checkpoint_path() {
  require_file "${representation_result}"
  local checkpoint
  checkpoint="$(kv checkpoint_path "${representation_result}")"
  require_file "${checkpoint}"
  expect_kv "${representation_result}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  printf '%s' "${checkpoint}"
}

write_config_snapshot() {
  require_file "${representation_config}"
  local candidate
  candidate="$(mktemp "${runtime_root}/.config.XXXXXX")"
  awk '
    BEGIN { replaced = 0 }
    /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
      sub(/=.*/, "= train_core_channel_mdn");
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${representation_config}" >"${candidate}"
  chmod 0444 "${candidate}"
  if [[ -e "${config_snapshot}" ]]; then
    cmp -s "${candidate}" "${config_snapshot}" || fail "MDN config snapshot drifted"
    rm -f -- "${candidate}"
  else
    ln "${candidate}" "${config_snapshot}" || fail "MDN config snapshot publication failed"
    rm -f -- "${candidate}"
  fi
  expect_kv "${config_snapshot}" runtime_wave_id train_core_channel_mdn
}

verify_static_inputs() {
  require_file "${closure_report}"
  require_file "${closure_verifier}"
  require_file "${representation_runner}"
  require_file "${runtime_exec}"
  [[ "$(sha256_of "${closure_report}")" == "${expected_data_closure_sha256}" ]] ||
    fail "fresh-seed data closure hash drifted"
  [[ "$(sha256_of "${runtime_exec}")" == "${expected_runtime_exec_sha256}" ]] ||
    fail "runtime executable hash drifted"
  "${closure_verifier}" --verify >/dev/null
  "${representation_runner}" --verify >/dev/null
  representation_checkpoint_path >/dev/null
}

emit_inputs() {
  local destination="$1"
  local representation_checkpoint="$2"
  cat >"${destination}" <<INPUTS
schema_id=${schema_id}.inputs.v1
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
data_closure_path=${closure_report}
data_closure_sha256=$(sha256_of "${closure_report}")
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=$(sha256_of "${config_snapshot}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
train_anchor_begin=${train_begin}
train_anchor_end_exclusive=${train_end}
expected_optimizer_steps=${expected_steps}
final_holdout_access=false
policy_access=false
INPUTS
}

verify_input_receipt() {
  local representation_checkpoint="$1"
  require_file "${input_receipt}"
  expect_kv "${input_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${input_receipt}" data_closure_sha256 "$(sha256_of "${closure_report}")"
  expect_kv "${input_receipt}" config_snapshot_sha256 "$(sha256_of "${config_snapshot}")"
  expect_kv "${input_receipt}" runtime_exec_sha256 "$(sha256_of "${runtime_exec}")"
  expect_kv "${input_receipt}" representation_checkpoint_sha256 "$(sha256_of "${representation_checkpoint}")"
  expect_kv "${input_receipt}" final_holdout_access false
  expect_kv "${input_receipt}" policy_access false
}

validate_job() {
  local representation_checkpoint="$1"
  local result="${job_dir}/runtime.result.fact"
  local manifest="${job_dir}/job.manifest"
  local report="${job_dir}/channel_inference.report"
  local checkpoint
  require_file "${result}"
  require_file "${manifest}"
  require_file "${report}"
  expect_kv "${result}" status completed
  expect_kv "${result}" optimizer_steps "${expected_steps}"
  expect_kv "${result}" checkpoint_written true
  expect_kv "${result}" model_state_mutated true
  expect_kv "${manifest}" config_path "${config_snapshot}"
  expect_kv "${manifest}" wave_id train_core_channel_mdn
  expect_kv "${manifest}" wave_action train
  expect_kv "${manifest}" resolved_anchor_index_begin "${train_begin}"
  expect_kv "${manifest}" resolved_anchor_index_end "${train_end}"
  expect_kv "${manifest}" accepted_anchor_count 4096
  expect_kv "${manifest}" input_representation_checkpoint_path "${representation_checkpoint}"
  expect_kv "${manifest}" input_mdn_checkpoint_path ''
  expect_kv "${report}" representation_checkpoint_loaded true
  expect_kv "${report}" representation_checkpoint_path "${representation_checkpoint}"
  expect_kv "${report}" mdn_checkpoint_loaded false
  checkpoint="$(kv checkpoint_path "${result}")"
  require_file "${checkpoint}"
  printf '%s' "${checkpoint}"
}

write_result_receipt() {
  local checkpoint="$1"
  local representation_checkpoint="$2"
  local candidate
  candidate="$(mktemp "${runtime_root}/.result.XXXXXX")"
  cat >"${candidate}" <<RESULT
schema_id=${schema_id}.result.v1
status=complete
job_dir=${job_dir}
runner_path=${script_path}
runner_sha256=$(sha256_of "${script_path}")
input_receipt_path=${input_receipt}
input_receipt_sha256=$(sha256_of "${input_receipt}")
config_snapshot_path=${config_snapshot}
config_snapshot_sha256=$(sha256_of "${config_snapshot}")
runtime_exec_path=${runtime_exec}
runtime_exec_sha256=$(sha256_of "${runtime_exec}")
representation_checkpoint_path=${representation_checkpoint}
representation_checkpoint_sha256=$(sha256_of "${representation_checkpoint}")
checkpoint_path=${checkpoint}
checkpoint_sha256=$(sha256_of "${checkpoint}")
job_manifest_sha256=$(sha256_of "${job_dir}/job.manifest")
runtime_result_sha256=$(sha256_of "${job_dir}/runtime.result.fact")
mdn_report_sha256=$(sha256_of "${job_dir}/channel_inference.report")
train_anchor_range=[${train_begin},${train_end})
optimizer_steps=${expected_steps}
final_holdout_access=false
policy_access=false
RESULT
  chmod 0444 "${candidate}"
  if [[ -e "${result_receipt}" ]]; then
    cmp -s "${candidate}" "${result_receipt}" || fail "result receipt drifted"
    rm -f -- "${candidate}"
  else
    ln "${candidate}" "${result_receipt}" || fail "result receipt publication failed"
    rm -f -- "${candidate}"
  fi
}

verify_completed() {
  verify_static_inputs
  require_file "${config_snapshot}"
  local representation_checkpoint checkpoint
  representation_checkpoint="$(representation_checkpoint_path)"
  verify_input_receipt "${representation_checkpoint}"
  checkpoint="$(validate_job "${representation_checkpoint}")"
  require_file "${result_receipt}"
  expect_kv "${result_receipt}" runner_path "${script_path}"
  expect_kv "${result_receipt}" runner_sha256 "$(sha256_of "${script_path}")"
  expect_kv "${result_receipt}" input_receipt_path "${input_receipt}"
  expect_kv "${result_receipt}" input_receipt_sha256 "$(sha256_of "${input_receipt}")"
  expect_kv "${result_receipt}" config_snapshot_path "${config_snapshot}"
  expect_kv "${result_receipt}" config_snapshot_sha256 "$(sha256_of "${config_snapshot}")"
  expect_kv "${result_receipt}" runtime_exec_path "${runtime_exec}"
  expect_kv "${result_receipt}" runtime_exec_sha256 "$(sha256_of "${runtime_exec}")"
  expect_kv "${result_receipt}" checkpoint_path "${checkpoint}"
  expect_kv "${result_receipt}" checkpoint_sha256 "$(sha256_of "${checkpoint}")"
  expect_kv "${result_receipt}" job_manifest_sha256 "$(sha256_of "${job_dir}/job.manifest")"
  expect_kv "${result_receipt}" runtime_result_sha256 "$(sha256_of "${job_dir}/runtime.result.fact")"
  expect_kv "${result_receipt}" mdn_report_sha256 "$(sha256_of "${job_dir}/channel_inference.report")"
  expect_kv "${result_receipt}" final_holdout_access false
  expect_kv "${result_receipt}" policy_access false
  echo "mdn_checkpoint=${checkpoint}"
  echo "mdn_checkpoint_sha256=$(sha256_of "${checkpoint}")"
}

if [[ "${mode}" == --verify ]]; then
  verify_completed
  exit 0
fi

verify_static_inputs
mkdir -p "${runtime_root}"
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || fail "another MDN run holds the execution lock"
write_config_snapshot
representation_checkpoint="$(representation_checkpoint_path)"

candidate_inputs="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
emit_inputs "${candidate_inputs}" "${representation_checkpoint}"
chmod 0444 "${candidate_inputs}"
if [[ -e "${input_receipt}" ]]; then
  cmp -s "${candidate_inputs}" "${input_receipt}" || fail "input receipt drifted"
  rm -f -- "${candidate_inputs}"
else
  ln "${candidate_inputs}" "${input_receipt}" || fail "input receipt publication failed"
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
    fail "MDN training failed; see ${log_path}"
fi

checkpoint="$(validate_job "${representation_checkpoint}")"
write_result_receipt "${checkpoint}" "${representation_checkpoint}"
"${closure_verifier}" --verify >/dev/null
verify_completed
