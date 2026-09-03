#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

source_path="${script_dir}/frozen_encoder_channel_conditioned_affine_probe.cpp"
pooled_source_path="${script_dir}/frozen_representation_affine_probe.cpp"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"

usage() {
  cat >&2 <<'USAGE'
usage: build_frozen_encoder_channel_conditioned_affine_probe.sh OUTPUT_BINARY

Compile only. This target does not open probes, checkpoints, policies, or data.
USAGE
  exit 2
}

[[ "$#" == 1 ]] || usage
output_path="$1"
[[ "${output_path}" == /* ]] || {
  echo "output binary path must be absolute" >&2
  exit 2
}
[[ ! -e "${output_path}" && ! -L "${output_path}" ]] || {
  echo "refusing to overwrite output path: ${output_path}" >&2
  exit 1
}
[[ -f "${source_path}" && ! -L "${source_path}" ]] || {
  echo "missing evaluator source: ${source_path}" >&2
  exit 1
}
[[ -f "${pooled_source_path}" && ! -L "${pooled_source_path}" ]] || {
  echo "missing pooled evaluator source: ${pooled_source_path}" >&2
  exit 1
}
[[ -d "${libtorch_path}/include" && -d "${libtorch_path}/lib" ]] || {
  echo "missing bundled LibTorch tree: ${libtorch_path}" >&2
  exit 1
}
[[ -d "${cuda_path}/include" && -d "${cuda_path}/lib64" ]] || {
  echo "missing CUDA 12.4 tree: ${cuda_path}" >&2
  exit 1
}

output_parent="$(dirname "${output_path}")"
[[ -d "${output_parent}" && ! -L "${output_parent}" ]] || {
  echo "output parent must be an existing non-symlinked directory: ${output_parent}" >&2
  exit 1
}

g++ -std=c++20 -O0 -g0 -Wall -Wextra -Werror -fPIC \
  -isystem "${libtorch_path}/include" \
  -isystem "${libtorch_path}/include/torch/csrc/api/include" \
  -isystem "${cuda_path}/include" \
  "${source_path}" \
  -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" \
  -Wl,-rpath,"${libtorch_path}/lib" \
  -Wl,-rpath,"${cuda_path}/lib64" \
  -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
  -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
  -lstdc++ -lpthread -lm \
  -o "${output_path}"
