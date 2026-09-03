#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

source_path="${script_dir}/mtf_prepool_domain_scale_channel_conditioned_affine_probe.cpp"
phase2a_source_path="${script_dir}/frozen_encoder_channel_conditioned_affine_probe.cpp"
pooled_source_path="${script_dir}/frozen_representation_affine_probe.cpp"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
compiler_command_path="/usr/bin/g++"
compiler_path="/usr/bin/x86_64-linux-gnu-g++-12"
readonly source_sha="ba13d95c4d33347cf4840f8eaa30616e095cf1c7dc3b0fa85de6a6c8f7c6f718"
readonly phase2a_source_sha="5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570"
readonly pooled_source_sha="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly compiler_sha="dd91977c184e327710578363ad93ebb175c3a457b6236b874fd3911b7c055c65"
readonly build_input_scope="frozen_scientific_sources_plus_explicit_framework_link_inputs"
readonly full_transitive_system_toolchain_or_elf_closure="false"

# alias path | resolved regular-file path | SHA-256 | diagnostic label
readonly -a direct_library_bindings=(
  "${libtorch_path}/lib/libtorch_cuda.so|${libtorch_path}/lib/libtorch_cuda.so|23db5cf52d4986f420077e8620e26a3940acf347faf82ee637728164c54478ff|libtorch_cuda"
  "${libtorch_path}/lib/libc10_cuda.so|${libtorch_path}/lib/libc10_cuda.so|31250c11fd4b95eed52c24b067772df9edba72bcdba8e257b6ecdea37835d852|libc10_cuda"
  "${libtorch_path}/lib/libtorch_cpu.so|${libtorch_path}/lib/libtorch_cpu.so|ad1ff2d39eac3f4c56c0ac35627a83c06ed367f0e7b075d1b437461c881e7116|libtorch_cpu"
  "${libtorch_path}/lib/libtorch.so|${libtorch_path}/lib/libtorch.so|024ae7b7d8eae378c36c992e8a69e16a027794ca2d6b13a9700873bb73ba20f0|libtorch"
  "${libtorch_path}/lib/libc10.so|${libtorch_path}/lib/libc10.so|735930b9588dde30c73856117530cf0d9da26003b86424695a10c8ceb891a0f4|libc10"
  "${cuda_path}/lib64/libcudart.so|${cuda_path}/targets/x86_64-linux/lib/libcudart.so.12.4.127|8774224f5b11a73b15d074a3fcce7327322c5c4cfdfd924d6a826779eec968fe|libcudart"
  "${cuda_path}/lib64/libnvToolsExt.so|${cuda_path}/targets/x86_64-linux/lib/libnvToolsExt.so.1.0.0|847d78f275c8cc97442a278073204bdc1a0009ffc2559bf7248c70d92105dfdc|libnvToolsExt"
  "/usr/lib/x86_64-linux-gnu/libcuda.so|/usr/lib/x86_64-linux-gnu/libcuda.so.1|57e0db4fcada1712297e0c9ab0d7d4beff59c663468876f77a262eda98a6e0b8|libcuda"
)

sha256() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_regular_sha() {
  local path="$1" expected="$2" label="$3"
  [[ -f "${path}" && ! -L "${path}" ]] || {
    echo "${label} must be a regular non-symlinked file: ${path}" >&2
    exit 1
  }
  [[ "$(sha256 "${path}")" == "${expected}" ]] || {
    echo "${label} SHA-256 mismatch: ${path}" >&2
    exit 1
  }
}

require_resolved_sha() {
  local alias_path="$1" expected_path="$2" expected_sha="$3" label="$4"
  [[ -e "${alias_path}" || -L "${alias_path}" ]] || {
    echo "missing ${label} link input: ${alias_path}" >&2
    exit 1
  }
  [[ "$(realpath -- "${alias_path}")" == "${expected_path}" ]] || {
    echo "${label} resolved-path mismatch: ${alias_path}" >&2
    exit 1
  }
  require_regular_sha "${expected_path}" "${expected_sha}" \
    "${label} resolved link input"
}

verify_bounded_inputs() {
  local binding alias_path expected_path expected_sha label

  [[ "$(command -v g++)" == "${compiler_command_path}" ]] || {
    echo "g++ command path mismatch" >&2
    exit 1
  }
  [[ "$(realpath -- "${compiler_command_path}")" == "${compiler_path}" ]] || {
    echo "g++ resolved-path mismatch" >&2
    exit 1
  }
  require_regular_sha "${compiler_path}" "${compiler_sha}" "resolved compiler"
  require_regular_sha "${source_path}" "${source_sha}" "new evaluator source"
  require_regular_sha "${phase2a_source_path}" "${phase2a_source_sha}" \
    "Phase 2A authority"
  require_regular_sha "${pooled_source_path}" "${pooled_source_sha}" \
    "frozen parser authority"

  for binding in "${direct_library_bindings[@]}"; do
    IFS='|' read -r alias_path expected_path expected_sha label <<<"${binding}"
    require_resolved_sha "${alias_path}" "${expected_path}" "${expected_sha}" \
      "${label}"
  done
}

usage() {
  cat >&2 <<'USAGE'
usage: build_mtf_prepool_domain_scale_channel_conditioned_affine_probe.sh OUTPUT_BINARY

Compile/link only from the bounded frozen inputs pinned by this wrapper. It
does not open probes, checkpoints, policies, or data, and it does not execute
a model or evaluator.

build_input_scope=frozen_scientific_sources_plus_explicit_framework_link_inputs
full_transitive_system_toolchain_or_elf_closure=false
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
verify_bounded_inputs

private_build_dir="$(mktemp -d "${output_parent}/.prepool-evaluator-build.XXXXXX")"
candidate="${private_build_dir}/mtf_prepool_domain_scale_affine_probe"
cleanup() {
  if [[ -n "${candidate:-}" && -e "${candidate}" ]]; then
    rm -f -- "${candidate}"
  fi
  if [[ -n "${private_build_dir:-}" && -d "${private_build_dir}" ]]; then
    rmdir -- "${private_build_dir}" 2>/dev/null || true
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM HUP

"${compiler_path}" -std=c++20 -O0 -g0 -Wall -Wextra -Werror -fPIC \
  -isystem "${libtorch_path}/include" \
  -isystem "${libtorch_path}/include/torch/csrc/api/include" \
  -isystem "${cuda_path}/include" \
  "${source_path}" \
  -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" \
  -Wl,-rpath,"${libtorch_path}/lib" \
  -Wl,-rpath,"${cuda_path}/lib64" \
  -Wl,--no-as-needed \
  "${libtorch_path}/lib/libtorch_cuda.so" \
  "${libtorch_path}/lib/libc10_cuda.so" \
  -Wl,--as-needed \
  "${libtorch_path}/lib/libtorch_cpu.so" \
  "${libtorch_path}/lib/libtorch.so" \
  "${libtorch_path}/lib/libc10.so" \
  "/usr/lib/x86_64-linux-gnu/libcuda.so.1" \
  "${cuda_path}/targets/x86_64-linux/lib/libcudart.so.12.4.127" \
  "${cuda_path}/targets/x86_64-linux/lib/libnvToolsExt.so.1.0.0" \
  -lstdc++ -lpthread -lm \
  -o "${candidate}"

verify_bounded_inputs
[[ -f "${candidate}" && ! -L "${candidate}" ]] || {
  echo "compiler did not produce a regular private candidate" >&2
  exit 1
}
chmod 0555 -- "${candidate}"
ln -T -- "${candidate}" "${output_path}" || {
  echo "atomic no-clobber publication failed: ${output_path}" >&2
  exit 1
}
rm -f -- "${candidate}"
rmdir -- "${private_build_dir}"
candidate=""
private_build_dir=""
trap - EXIT INT TERM HUP
[[ -f "${output_path}" && ! -L "${output_path}" ]] || {
  echo "published evaluator binary is invalid: ${output_path}" >&2
  exit 1
}

# Receipt boundary; the runner seals the published binary and runtime manifest.
printf '%s\n' "build_input_scope=${build_input_scope}" \
  "full_transitive_system_toolchain_or_elf_closure=${full_transitive_system_toolchain_or_elf_closure}"
