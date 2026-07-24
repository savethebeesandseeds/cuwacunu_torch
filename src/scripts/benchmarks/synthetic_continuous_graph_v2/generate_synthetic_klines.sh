#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${SYNTHETIC_V2_BENCHMARK_ROOT:-${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2}"
manifest_name="synthetic_quasiperiodic_chart_manifest.v2.report"

fail() {
  echo "synthetic v2 generation: $*" >&2
  exit 1
}

[[ -d "${benchmark_root}" ]] ||
  fail "benchmark root must already exist: ${benchmark_root}"
[[ ! -L "${benchmark_root}" ]] ||
  fail "benchmark root must not be a symbolic link: ${benchmark_root}"
benchmark_root="$(cd "${benchmark_root}" && pwd -P)"

data_target="${benchmark_root}/data"
manifest_target="${benchmark_root}/artifacts/${manifest_name}"
lock_path="${benchmark_root}/.synthetic_v2_generation.lock"
stage_root="${benchmark_root}/.synthetic_v2_generation.stage.$$"
build_root="$(mktemp -d /tmp/cuwacunu_synthetic_v2_build.XXXXXX)"
payload_root="${stage_root}/payload"
data_installed=false
manifest_installed=false
lock_acquired=false
manifest_install_temp=""

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  if [[ "${status}" -ne 0 ]]; then
    if [[ "${manifest_installed}" == false && "${data_installed}" == true &&
          -d "${data_target}" && -d "${stage_root}" ]]; then
      mkdir -p "${payload_root}"
      mv "${data_target}" "${payload_root}/data.rollback" 2>/dev/null || true
    fi
  fi
  if [[ -n "${manifest_install_temp}" ]]; then
    rm -f -- "${manifest_install_temp}"
  fi
  rm -rf -- "${build_root}"
  rm -rf -- "${stage_root}"
  if [[ "${lock_acquired}" == true ]]; then
    rm -f -- "${lock_path}"
  fi
  exit "${status}"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

(
  set -o noclobber
  printf 'pid=%s\n' "$$" >"${lock_path}"
) 2>/dev/null || fail "generation lock already exists: ${lock_path}"
lock_acquired=true

[[ ! -e "${data_target}" ]] ||
  fail "refusing to replace existing data tree: ${data_target}"
[[ ! -e "${manifest_target}" ]] ||
  fail "refusing to replace existing manifest: ${manifest_target}"
mkdir "${stage_root}"
mkdir "${payload_root}"

generator_binary="${build_root}/generate_synthetic_klines"
validator_binary="${build_root}/validate_synthetic_dataset"
compile_flags=(-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc/include)

(
  cd "${repo_root}"
  g++ "${compile_flags[@]}" \
    src/scripts/benchmarks/synthetic_continuous_graph_v2/generate_synthetic_klines.cpp \
    -o "${generator_binary}"
  g++ "${compile_flags[@]}" \
    src/scripts/benchmarks/synthetic_continuous_graph_v2/validate_synthetic_dataset.cpp \
    -o "${validator_binary}"
)

"${generator_binary}" --output-root "${payload_root}"
"${validator_binary}" --benchmark-root "${payload_root}"

[[ -d "${payload_root}/data/raw" ]] ||
  fail "staged raw data tree is absent"
[[ -d "${payload_root}/data/development_prefix" ]] ||
  fail "staged development-prefix tree is absent"
[[ -f "${payload_root}/artifacts/${manifest_name}" ]] ||
  fail "staged manifest is absent"
[[ ! -e "${data_target}" ]] ||
  fail "data target appeared during generation"
[[ ! -e "${manifest_target}" ]] ||
  fail "manifest target appeared during generation"

# The two data views move together as one directory rename on the benchmark
# filesystem. The manifest is linked into place only after the data tree is
# complete, so its presence is the publication marker.
mv "${payload_root}/data" "${data_target}"
data_installed=true
mkdir -p "${benchmark_root}/artifacts"
manifest_install_temp="${benchmark_root}/artifacts/.${manifest_name}.install.$$"
cp "${payload_root}/artifacts/${manifest_name}" "${manifest_install_temp}"
chmod 0444 "${manifest_install_temp}"
ln "${manifest_install_temp}" "${manifest_target}" ||
  fail "manifest publication lost a no-clobber race"
rm -f -- "${manifest_install_temp}"
manifest_installed=true

"${validator_binary}" --benchmark-root "${benchmark_root}"
printf 'generated and validated synthetic v2 data under %s\n' "${benchmark_root}"
