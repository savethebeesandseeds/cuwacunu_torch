#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
source_path="${script_dir}/raw_nodelift_edge_feature_probe_capture.cpp"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
common_archive="${repo_root}/.build/lib/libcommon.a"
torch_archive="${repo_root}/.build/lib/libtorchwrap.a"
readonly common_archive_sha="853ade11707a8588194eda199e5a742e7363c2d1fd87f43285f3ad89414e06d3"
readonly torch_archive_sha="d9a128191f227a798219c9ee0c2ed8d4c6976916dba8b43eb66c6f25c21b279d"

usage() {
  cat >&2 <<'USAGE'
usage: build_raw_nodelift_edge_feature_probe_capture.sh ABSENT_ABSOLUTE_BINARY

Compile/link only. This script does not open benchmark probes, checkpoints,
policies, or data. It does not run the resulting capture binary.
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
  echo "missing capture source: ${source_path}" >&2
  exit 1
}
output_parent="$(dirname "${output_path}")"
[[ -d "${output_parent}" && ! -L "${output_parent}" ]] || {
  echo "output parent must be an existing non-symlinked directory" >&2
  exit 1
}

# This bounded diagnostic consumes the already-built canonical archives. Do
# not refresh the broad project object graph here: doing so is outside this
# compile-only target's scope and can invoke secure-clean behavior on bundled
# dependency inputs. The runner binds the archive identities used below.
for binding in "${common_archive}:${common_archive_sha}" "${torch_archive}:${torch_archive_sha}"; do
  path="${binding%%:*}"
  expected_sha="${binding##*:}"
  [[ -f "${path}" && ! -L "${path}" ]] || {
    echo "missing canonical project archive: ${path}" >&2
    exit 1
  }
  [[ "$(sha256sum -- "${path}" | awk '{print $1}')" == "${expected_sha}" ]] || {
    echo "canonical project archive SHA-256 mismatch: ${path}" >&2
    exit 1
  }
done

temporary="$(mktemp "${output_parent}/.raw_nodelift_capture.XXXXXXXX")"
cleanup() {
  rm -f -- "${temporary}"
}
trap cleanup EXIT

g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
  -I"${repo_root}/src" \
  -I"${repo_root}/src/include" \
  -I"${repo_root}/src/include/torch_compat" \
  -isystem "${libtorch_path}/include" \
  -isystem "${libtorch_path}/include/torch/csrc/api/include" \
  -isystem "${cuda_path}/include" \
  "${source_path}" \
  -Wl,--start-group "${common_archive}" "${torch_archive}" -Wl,--end-group \
  -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" \
  -Wl,-rpath,"${libtorch_path}/lib" \
  -Wl,-rpath,"${cuda_path}/lib64" \
  -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
  -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
  -lstdc++ -lpthread -lm \
  -o "${temporary}"
[[ "$(sha256sum -- "${common_archive}" | awk '{print $1}')" == "${common_archive_sha}" ]] || {
  echo "canonical common archive changed during compilation" >&2
  exit 1
}
[[ "$(sha256sum -- "${torch_archive}" | awk '{print $1}')" == "${torch_archive_sha}" ]] || {
  echo "canonical torch archive changed during compilation" >&2
  exit 1
}
chmod 0555 -- "${temporary}"
mv -T -n -- "${temporary}" "${output_path}" || {
  echo "atomic no-clobber binary publication failed" >&2
  exit 1
}
trap - EXIT
