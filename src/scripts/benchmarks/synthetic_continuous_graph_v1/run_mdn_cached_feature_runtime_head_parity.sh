#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_SCHEMA_ID:-synthetic_mdn_cached_feature_runtime_head_parity.v1}"
runtime_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"
build_dir="${runtime_dir}/bin"
helper_src="${script_dir}/mdn_cached_feature_runtime_head_parity.cpp"
helper_bin="${build_dir}/mdn_cached_feature_runtime_head_parity"
output_probe="${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_OUTPUT_PROBE:-${runtime_dir}/${schema_id}.probe}"

input_probe="${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_INPUT_PROBE:-}"
if [[ -z "${input_probe}" ]]; then
  input_probe="$(
    find "${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn" \
      -type f -name mdn_edge_context_features.probe -printf '%T@ %p\n' 2>/dev/null \
      | sort -n \
      | tail -1 \
      | cut -d' ' -f2-
  )"
fi

if [[ -z "${input_probe}" || ! -f "${input_probe}" ]]; then
  echo "missing mdn_edge_context_features.probe; set SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_INPUT_PROBE" >&2
  exit 1
fi

libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
head_header="${repo_root}/src/include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
mkdir -p "${build_dir}" "$(dirname "${output_probe}")"

if [[ ! -x "${helper_bin}" || "${helper_src}" -nt "${helper_bin}" || \
      "${head_header}" -nt "${helper_bin}" ]]; then
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${repo_root}/src" \
    -I"${repo_root}/src/include" \
    -I"${repo_root}/src/include/torch_compat" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${helper_src}" \
    -L"${libtorch_path}/lib" \
    -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${helper_bin}"
fi

device_args=()
if [[ "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_CPU:-false}" == "true" ]]; then
  device_args+=(--cpu)
fi

ridge_mode_args=()
if [[ "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_ONLY:-false}" == "true" ]]; then
  ridge_mode_args+=(--ridge-only)
fi

evaluation_input_args=()
if [[ -n "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_EVALUATION_INPUT_PROBE:-}" ]]; then
  evaluation_input_args+=(
    --evaluation-input
    "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_EVALUATION_INPUT_PROBE}"
  )
fi

"${helper_bin}" \
  --input "${input_probe}" \
  --output "${output_probe}" \
  --schema-id "${schema_id}" \
  --identity-modes "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_IDENTITY_MODES:-shared,edge_embedding,per_edge,edge_embedding_per_edge}" \
  --feature-dim "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_FEATURE_DIM:-128}" \
  --identity-embedding-dim "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_IDENTITY_EMBEDDING_DIM:-16}" \
  --steps "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_STEPS:-3500}" \
  --batch-size "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_BATCH_SIZE:-64}" \
  --overfit-anchors "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_OVERFIT_ANCHORS:-16}" \
  --overfit-steps "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_OVERFIT_STEPS:-2000}" \
  --standardized-linear-steps "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_STANDARDIZED_LINEAR_STEPS:-3500}" \
  --seed "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_SEED:-31}" \
  --fit-fraction "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_FIT_FRACTION:-0.70}" \
  --ridge-alphas "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_ALPHAS:-1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1,10}" \
  --ridge-validation-fraction "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_VALIDATION_FRACTION:-0.20}" \
  --ridge-validation-gap "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_VALIDATION_GAP:-0}" \
  --ridge-direction-gate "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_DIRECTION_GATE:-0.95}" \
  --ridge-rank-gate "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_RANK_GATE:-0.95}" \
  --learning-rate "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_LEARNING_RATE:-0.001}" \
  --grad-clip-norm "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_GRAD_CLIP_NORM:-5.0}" \
  --regression-weight "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_REGRESSION_WEIGHT:-100.0}" \
  --direction-weight "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_DIRECTION_WEIGHT:-5.0}" \
  --rank-weight "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RANK_WEIGHT:-5.0}" \
  --huber-beta "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_HUBER_BETA:-0.5}" \
  --logit-scale "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_LOGIT_SCALE:-1.0}" \
  --target-scale "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_TARGET_SCALE:-36.0}" \
  --margin-eps "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_MARGIN_EPS:-0.001}" \
  --rank-margin-eps "${SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RANK_MARGIN_EPS:-0.001}" \
  "${device_args[@]}" \
  "${ridge_mode_args[@]}" \
  "${evaluation_input_args[@]}"
