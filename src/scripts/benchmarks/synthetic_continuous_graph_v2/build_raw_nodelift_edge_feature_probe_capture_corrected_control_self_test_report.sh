#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
source_path="${script_dir}/raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.cpp"
predecessor_source="${script_dir}/raw_nodelift_edge_feature_probe_capture_corrected_control.cpp"
predecessor_wrapper="${script_dir}/build_raw_nodelift_edge_feature_probe_capture_corrected_control.sh"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
common_archive="${repo_root}/.build/lib/libcommon.a"
torch_archive="${repo_root}/.build/lib/libtorchwrap.a"
readonly source_sha="b489548b7a8fec72c7933f359b694e5852282e721108453b6e338e3ec73b2c62"
readonly predecessor_source_sha="f1483a0858c342b2477cc37e043bf5a894da369bd1e3ccb51ce04601710de2a8"
readonly predecessor_wrapper_sha="f994f98c5825e1d5d9c267dfe5b8ab59ac479e7fc1707af56c81efb7c06fb3d2"
readonly common_archive_sha="853ade11707a8588194eda199e5a742e7363c2d1fd87f43285f3ad89414e06d3"
readonly torch_archive_sha="d9a128191f227a798219c9ee0c2ed8d4c6976916dba8b43eb66c6f25c21b279d"

usage() {
  cat >&2 <<'USAGE'
usage: build_raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.sh ABSENT_ABSOLUTE_BINARY

Compile/link only. This script does not open benchmark probes, checkpoints,
policies, or data. It does not run the resulting capture binary.
USAGE
  exit 2
}

verify_binding() {
  local path="$1"
  local expected_sha="$2"
  local label="$3"
  [[ -f "${path}" && ! -L "${path}" ]] || {
    echo "missing non-symlinked ${label}: ${path}" >&2
    exit 1
  }
  [[ "$(sha256sum -- "${path}" | awk '{print $1}')" == "${expected_sha}" ]] || {
    echo "${label} SHA-256 mismatch: ${path}" >&2
    exit 1
  }
}

verify_inputs() {
  verify_binding "${source_path}" "${source_sha}" "successor capture source"
  verify_binding "${predecessor_source}" "${predecessor_source_sha}" \
    "frozen predecessor capture source"
  verify_binding "${predecessor_wrapper}" "${predecessor_wrapper_sha}" \
    "frozen predecessor build wrapper"
  verify_binding "${common_archive}" "${common_archive_sha}" \
    "canonical common archive"
  verify_binding "${torch_archive}" "${torch_archive_sha}" \
    "canonical torch archive"
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
output_parent="$(dirname "${output_path}")"
[[ -d "${output_parent}" && ! -L "${output_parent}" ]] || {
  echo "output parent must be an existing non-symlinked directory" >&2
  exit 1
}

# This bounded successor consumes the existing canonical archives and includes
# the exact frozen corrected-control capture source. Do not refresh the broad
# project object graph here. The runner separately binds this wrapper and the
# resulting binary.
verify_inputs

temporary="$(mktemp "${output_parent}/.raw_nodelift_corrected_control_self_test_report_capture.XXXXXXXX")"
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

verify_inputs
chmod 0555 -- "${temporary}"
ln -- "${temporary}" "${output_path}" || {
  echo "atomic hard-link no-clobber binary publication failed" >&2
  exit 1
}
rm -f -- "${temporary}"
trap - EXIT
