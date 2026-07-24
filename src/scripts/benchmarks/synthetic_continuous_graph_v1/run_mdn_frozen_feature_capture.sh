#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
base_config="${SYNTHETIC_MDN_FROZEN_CAPTURE_BASE_CONFIG:-${benchmark_root}/synthetic_benchmark.config}"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"

anchor_begin="${SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_BEGIN:-0}"
anchor_end="${SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_END:-730}"
schema_id="${SYNTHETIC_MDN_FROZEN_CAPTURE_SCHEMA_ID:-synthetic_mdn_frozen_feature_capture.v1}"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"
job_dir="${SYNTHETIC_MDN_FROZEN_CAPTURE_JOB_DIR:-${runtime_root}/anchor_${anchor_begin}_${anchor_end}}"
capture_config="${SYNTHETIC_MDN_FROZEN_CAPTURE_CONFIG_PATH:-}"

representation_checkpoint="${SYNTHETIC_MDN_FROZEN_CAPTURE_REPRESENTATION_CHECKPOINT:-}"
mdn_checkpoint="${SYNTHETIC_MDN_FROZEN_CAPTURE_MDN_CHECKPOINT:-}"

require_file() {
  local path="$1"
  if [[ ! -f "${path}" ]]; then
    echo "required file missing: ${path}" >&2
    exit 2
  fi
}

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

if ! [[ "${anchor_begin}" =~ ^[0-9]+$ && "${anchor_end}" =~ ^[0-9]+$ ]] ||
  ((anchor_begin >= anchor_end)); then
  echo "capture bounds must be nonnegative integers with begin < end" >&2
  exit 2
fi
if ((anchor_end > 1088)) &&
  [[ "${SYNTHETIC_MDN_FROZEN_CAPTURE_ALLOW_UNCONSUMED_HOLDOUT:-false}" != "true" ]]; then
  echo "refusing to expose unconsumed holdout labels above anchor 1087" >&2
  exit 3
fi
if [[ -z "${representation_checkpoint}" || -z "${mdn_checkpoint}" ]]; then
  echo "set both frozen checkpoint environment variables" >&2
  exit 2
fi

require_file "${base_config}"
require_file "${runtime_exec}"
require_file "${representation_checkpoint}"
require_file "${mdn_checkpoint}"

if [[ -e "${job_dir}" ]]; then
  echo "capture job directory already exists: ${job_dir}" >&2
  exit 4
fi
mkdir -p "$(dirname "${job_dir}")"

if [[ -n "${capture_config}" ]]; then
  mkdir -p "$(dirname "${capture_config}")"
  temporary_config="${capture_config}"
  if [[ -e "${temporary_config}" ]]; then
    echo "capture config already exists before its job: ${temporary_config}" >&2
    exit 4
  fi
else
  temporary_config="$(mktemp "${benchmark_root}/.mdn_frozen_capture.XXXXXX.config")"
fi
cleanup() {
  if [[ -z "${capture_config}" ]]; then
    rm -f "${temporary_config}"
  fi
}
trap cleanup EXIT

awk '
  BEGIN { replaced = 0 }
  /^[[:space:]]*runtime_wave_id[[:space:]]*=/ {
    sub(/=.*/, "= cwu_02v_certified_replay_eval_mdn")
    replaced = 1
  }
  { print }
  END { if (!replaced) exit 42 }
' "${base_config}" >"${temporary_config}"

log_path="${runtime_root}/anchor_${anchor_begin}_${anchor_end}.log"
"${runtime_exec}" \
  --config "${temporary_config}" \
  --job-dir "${job_dir}" \
  --source-range anchor_index \
  --anchor-index-begin "${anchor_begin}" \
  --anchor-index-end "${anchor_end}" \
  --input-representation-checkpoint "${representation_checkpoint}" \
  --input-mdn-checkpoint "${mdn_checkpoint}" \
  --no-replay-artifacts >"${log_path}" 2>&1

result_fact="${job_dir}/runtime.result.fact"
manifest="${job_dir}/job.manifest"
probe="${job_dir}/mdn_edge_context_features.probe"
require_file "${result_fact}"
require_file "${manifest}"
require_file "${probe}"

if [[ "$(kv status "${result_fact}")" != "completed" ]]; then
  echo "frozen feature capture did not complete: ${job_dir}" >&2
  exit 5
fi
if [[ "$(kv resolved_anchor_index_begin "${manifest}")" != "${anchor_begin}" ]] ||
  [[ "$(kv resolved_anchor_index_end "${manifest}")" != "${anchor_end}" ]]; then
  echo "capture manifest range does not match the requested bounds" >&2
  exit 5
fi
if [[ "$(kv input_representation_checkpoint_path "${manifest}")" != "${representation_checkpoint}" ]] ||
  [[ "$(kv input_mdn_checkpoint_path "${manifest}")" != "${mdn_checkpoint}" ]]; then
  echo "capture manifest checkpoint paths do not match the request" >&2
  exit 5
fi
if [[ -n "${capture_config}" ]] &&
  [[ "$(kv config_path "${manifest}")" != "${capture_config}" ]]; then
  echo "capture manifest config path does not match the persisted config" >&2
  exit 5
fi

expected_rows="$(((anchor_end - anchor_begin) * 9))"
actual_rows="$(awk 'NR > 1 { ++rows } END { print rows + 0 }' "${probe}")"
if [[ "${actual_rows}" != "${expected_rows}" ]]; then
  echo "capture row count mismatch: expected ${expected_rows}, got ${actual_rows}" >&2
  exit 5
fi

printf 'probe_path=%s\n' "${probe}"
printf 'probe_rows=%s\n' "${actual_rows}"
printf 'probe_sha256=%s\n' "$(sha256sum "${probe}" | awk '{ print $1 }')"
printf 'representation_checkpoint_sha256=%s\n' \
  "$(sha256sum "${representation_checkpoint}" | awk '{ print $1 }')"
printf 'mdn_checkpoint_sha256=%s\n' \
  "$(sha256sum "${mdn_checkpoint}" | awk '{ print $1 }')"
