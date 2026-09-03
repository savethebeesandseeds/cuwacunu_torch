#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

source_path="${script_dir}/frozen_representation_affine_injection_optimizer_localization_probe.cpp"
canonical_source_path="${script_dir}/frozen_representation_affine_probe.cpp"
phase2a_source_path="${script_dir}/frozen_encoder_channel_conditioned_affine_probe.cpp"
readonly source_sha="7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b"
readonly canonical_source_sha="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly phase2a_source_sha="5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"

usage() {
  cat >&2 <<'USAGE'
usage: build_frozen_representation_affine_injection_optimizer_localization_probe.sh OUTPUT_BINARY

Compile only. This target does not open probes, data, checkpoints, models, or policy.
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
for required_source in "${source_path}" "${canonical_source_path}" "${phase2a_source_path}"; do
  [[ -f "${required_source}" && ! -L "${required_source}" ]] || {
    echo "missing regular evaluator source: ${required_source}" >&2
    exit 1
  }
done
[[ "$(sha256sum -- "${source_path}" | awk '{print $1}')" == "${source_sha}" ]] || {
  echo "affine-injection evaluator source SHA-256 mismatch: ${source_path}" >&2
  exit 1
}
[[ "$(sha256sum -- "${canonical_source_path}" | awk '{print $1}')" == "${canonical_source_sha}" ]] || {
  echo "canonical affine source SHA-256 mismatch: ${canonical_source_path}" >&2
  exit 1
}
[[ "$(sha256sum -- "${phase2a_source_path}" | awk '{print $1}')" == "${phase2a_source_sha}" ]] || {
  echo "Phase 2A conditioned affine source SHA-256 mismatch: ${phase2a_source_path}" >&2
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

temporary="$(mktemp "${output_parent}/.affine_injection_optimizer_localization.XXXXXXXX")"
cleanup() {
  rm -f -- "${temporary}"
}
trap cleanup EXIT

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
  -o "${temporary}"

[[ "$(sha256sum -- "${source_path}" | awk '{print $1}')" == "${source_sha}" ]] || {
  echo "affine-injection evaluator source changed during compilation" >&2
  exit 1
}
[[ "$(sha256sum -- "${canonical_source_path}" | awk '{print $1}')" == "${canonical_source_sha}" ]] || {
  echo "canonical affine source changed during compilation" >&2
  exit 1
}
[[ "$(sha256sum -- "${phase2a_source_path}" | awk '{print $1}')" == "${phase2a_source_sha}" ]] || {
  echo "Phase 2A conditioned affine source changed during compilation" >&2
  exit 1
}
chmod 0555 -- "${temporary}"
ln -- "${temporary}" "${output_path}" || {
  echo "atomic no-clobber binary publication failed" >&2
  exit 1
}
rm -f -- "${temporary}"
trap - EXIT
