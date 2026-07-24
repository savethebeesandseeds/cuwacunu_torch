#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_representation_k1_objective_ab_v1"
upstream_schema="synthetic_mdn_frozen_representation_k1_isolation_v1"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
final_runtime_dir="${runtime_parent}/${schema_id}"
upstream_runtime_dir="${runtime_parent}/${upstream_schema}"
lock_dir="${final_runtime_dir}.lock"

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"
data_root="${benchmark_root}/data/raw"
installed_report="${artifacts_dir}/${schema_id}.report"
upstream_installed_report="${artifacts_dir}/${upstream_schema}.report"
production_mdn_jkimyei="${repo_root}/src/config/wikimyei.inference.expected_value.mdn.jkimyei"

helper_name="mdn_frozen_representation_k1_objective_ab.cpp"
runner_name="run_mdn_frozen_representation_k1_objective_ab.sh"
reference_helper_name="mdn_frozen_representation_k1_isolation.reference.cpp"
helper_src="${script_dir}/${helper_name}"
runner_src="${script_dir}/${runner_name}"
live_reference_helper="${script_dir}/mdn_frozen_representation_k1_isolation.cpp"
upstream_runner="${script_dir}/run_mdn_frozen_representation_k1_isolation.sh"
upstream_frozen_helper="${upstream_runtime_dir}/frozen_sources/mdn_frozen_representation_k1_isolation.cpp"
upstream_frozen_runner="${upstream_runtime_dir}/frozen_sources/run_mdn_frozen_representation_k1_isolation.sh"

alpha_source="${data_root}/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_source="${data_root}/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_source="${data_root}/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
prefix_rows=760

libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"

runtime_dir=""
build_dir=""
frozen_source_dir=""
frozen_include_root=""
frozen_contract_dir=""
input_prefix_dir=""
frozen_input_dir=""
helper_bin=""
representation_probe=""
raw_history_report=""
frozen_k1_report=""
alpha_prefix=""
beta_prefix=""
gamma_prefix=""
main_dir=""
replay_dir=""
main_probe=""
replay_probe=""
main_log=""
replay_log=""
lock_held=false

fail() {
  echo "error: $*" >&2
  exit 1
}

require_regular_file() {
  [[ -f "$1" && ! -L "$1" ]] ||
    fail "missing or symlinked required file: $1"
}

require_nonempty_file() {
  require_regular_file "$1"
  [[ -s "$1" ]] || fail "required file is empty: $1"
}

require_directory() {
  [[ -d "$1" && ! -L "$1" ]] ||
    fail "missing or symlinked required directory: $1"
}

file_value() {
  local path="$1"
  local key="$2"
  awk -F= -v key="${key}" '$1 == key { sub(/^[^=]*=/, ""); print; exit }' \
    "${path}"
}

expect_file_value() {
  local path="$1"
  local key="$2"
  local expected="$3"
  [[ "$(file_value "${path}" "${key}")" == "${expected}" ]] ||
    fail "unexpected ${key} in ${path}"
}

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  frozen_contract_dir="${runtime_dir}/frozen_contracts"
  input_prefix_dir="${runtime_dir}/input_prefixes"
  frozen_input_dir="${runtime_dir}/frozen_inputs"
  helper_bin="${build_dir}/mdn_frozen_representation_k1_objective_ab"
  representation_probe="${frozen_input_dir}/representation_edge_features.probe"
  raw_history_report="${frozen_input_dir}/synthetic_mdn_raw_history_isolation_v1.report"
  frozen_k1_report="${frozen_input_dir}/${upstream_schema}.report"
  alpha_prefix="${input_prefix_dir}/SYNALPHASYNUSD-1d-prefix-760.csv"
  beta_prefix="${input_prefix_dir}/SYNBETASYNUSD-1d-prefix-760.csv"
  gamma_prefix="${input_prefix_dir}/SYNGAMMASYNUSD-1d-prefix-760.csv"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
}

verify_live_prefix_matches_snapshot() {
  local source="$1"
  local snapshot="$2"
  require_regular_file "${source}"
  require_nonempty_file "${snapshot}"
  # Deliberately stop at row 760; protected rows must remain unopened.
  sed -n "1,${prefix_rows}p;${prefix_rows}q" "${source}" |
    cmp -s - "${snapshot}" ||
    fail "live causal prefix differs from sealed upstream snapshot: ${source}"
}

verify_upstream_runtime() {
  require_directory "${upstream_runtime_dir}"
  require_nonempty_file "${upstream_runtime_dir}/master.sha256"
  require_nonempty_file "${upstream_installed_report}"
  require_nonempty_file "${upstream_runner}"
  require_nonempty_file "${upstream_frozen_runner}"
  cmp -s "${upstream_runner}" "${upstream_frozen_runner}" ||
    fail "live frozen K1 runner differs from its sealed upstream source"
  bash "${upstream_runner}" --verify >/dev/null
  cmp -s \
    "${upstream_runtime_dir}/main/${upstream_schema}.probe" \
    "${upstream_installed_report}" ||
    fail "installed frozen K1 report differs from its sealed upstream probe"
  cmp -s "${live_reference_helper}" "${upstream_frozen_helper}" ||
    fail "live frozen K1 helper differs from the sealed upstream helper"

  verify_live_prefix_matches_snapshot \
    "${alpha_source}" \
    "${upstream_runtime_dir}/input_prefixes/SYNALPHASYNUSD-1d-prefix-760.csv"
  verify_live_prefix_matches_snapshot \
    "${beta_source}" \
    "${upstream_runtime_dir}/input_prefixes/SYNBETASYNUSD-1d-prefix-760.csv"
  verify_live_prefix_matches_snapshot \
    "${gamma_source}" \
    "${upstream_runtime_dir}/input_prefixes/SYNGAMMASYNUSD-1d-prefix-760.csv"

  expect_file_value "${upstream_installed_report}" schema_id \
    "${upstream_schema}"
  expect_file_value "${upstream_installed_report}" status complete
  expect_file_value "${upstream_installed_report}" boundary.effective_fit \
    '[1,554)'
  expect_file_value "${upstream_installed_report}" boundary.purge '[554,584)'
  expect_file_value "${upstream_installed_report}" boundary.validation \
    '[584,730)'
  expect_file_value "${upstream_installed_report}" execution.seed 31
  expect_file_value "${upstream_installed_report}" execution.steps 3500
  expect_file_value "${upstream_installed_report}" execution.batch_size 64
  expect_file_value "${upstream_installed_report}" execution.mixture_count 1
  expect_file_value "${upstream_installed_report}" \
    classification.fit_featurewise_zscore_k1_pass false
}

preflight() {
  local command_name
  for command_name in awk bash chmod cmp cp g++ grep mkdir mktemp mv rm rmdir \
    sed sha256sum; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  require_nonempty_file "${helper_src}"
  require_nonempty_file "${runner_src}"
  require_nonempty_file "${live_reference_helper}"
  require_nonempty_file "${production_mdn_jkimyei}"
  require_directory "${artifacts_dir}"
  require_directory "${libtorch_path}/include"
  require_directory "${libtorch_path}/include/torch/csrc/api/include"
  require_directory "${libtorch_path}/lib"
  verify_upstream_runtime
  grep -Fq 'MDN_DIRECT_EDGE_RETURN_READOUT_HUBER_BETA = 0.5;' \
    "${production_mdn_jkimyei}" ||
    fail "production direct-readout Huber beta is not 0.5"
  grep -Fq 'MDN_DIRECT_EDGE_RETURN_READOUT_TARGET_SCALE = 36.0;' \
    "${production_mdn_jkimyei}" ||
    fail "production direct-readout target scale is not 36.0"

  printf 'preflight_ok=true\n'
  printf 'schema_id=%s\n' "${schema_id}"
  printf 'upstream_schema_id=%s\n' "${upstream_schema}"
  printf 'source_prefix_rows=%s\n' "${prefix_rows}"
  printf 'objective_arm_a=trainable_sigma_gaussian_nll\n'
  printf 'objective_arm_b=target_scaled_edge_huber\n'
}

snapshot_inputs() {
  mkdir "${input_prefix_dir}" "${frozen_input_dir}"
  cp -- \
    "${upstream_runtime_dir}/input_prefixes/SYNALPHASYNUSD-1d-prefix-760.csv" \
    "${alpha_prefix}"
  cp -- \
    "${upstream_runtime_dir}/input_prefixes/SYNBETASYNUSD-1d-prefix-760.csv" \
    "${beta_prefix}"
  cp -- \
    "${upstream_runtime_dir}/input_prefixes/SYNGAMMASYNUSD-1d-prefix-760.csv" \
    "${gamma_prefix}"
  cp -- \
    "${upstream_runtime_dir}/frozen_inputs/representation_edge_features.probe" \
    "${representation_probe}"
  cp -- \
    "${upstream_runtime_dir}/frozen_inputs/synthetic_mdn_raw_history_isolation_v1.report" \
    "${raw_history_report}"
  cp -- "${upstream_installed_report}" "${frozen_k1_report}"
  cp -- "${upstream_runtime_dir}/master.sha256" \
    "${frozen_input_dir}/upstream.master.sha256"
  cp -- "${upstream_runtime_dir}/input_manifest.sha256" \
    "${frozen_input_dir}/upstream.input_manifest.sha256"
  cp -- "${upstream_runtime_dir}/source_manifest.sha256" \
    "${frozen_input_dir}/upstream.source_manifest.sha256"
  cp -- "${upstream_runtime_dir}/contract_manifest.sha256" \
    "${frozen_input_dir}/upstream.contract_manifest.sha256"
  cp -- "${upstream_runtime_dir}/output_manifest.sha256" \
    "${frozen_input_dir}/upstream.output_manifest.sha256"

  {
    echo "schema_id=${schema_id}"
    echo "upstream_schema_id=${upstream_schema}"
    echo "prefix_data_rows=${prefix_rows}"
    echo "effective_fit=[1,554)"
    echo "purge=[554,584)"
    echo "validation=[584,730)"
    echo "representation_capture_channel_index=2"
    echo "conditioning=fit_only_featurewise_zscore"
  } >"${runtime_dir}/input_contract.status"

  (
    cd "${runtime_dir}"
    sha256sum \
      input_contract.status \
      input_prefixes/SYNALPHASYNUSD-1d-prefix-760.csv \
      input_prefixes/SYNBETASYNUSD-1d-prefix-760.csv \
      input_prefixes/SYNGAMMASYNUSD-1d-prefix-760.csv \
      frozen_inputs/representation_edge_features.probe \
      frozen_inputs/synthetic_mdn_raw_history_isolation_v1.report \
      "frozen_inputs/${upstream_schema}.report" \
      frozen_inputs/upstream.master.sha256 \
      frozen_inputs/upstream.input_manifest.sha256 \
      frozen_inputs/upstream.source_manifest.sha256 \
      frozen_inputs/upstream.contract_manifest.sha256 \
      frozen_inputs/upstream.output_manifest.sha256 \
      >input_manifest.sha256
  )
}

freeze_sources() {
  mkdir "${frozen_source_dir}" "${frozen_include_root}" \
    "${frozen_contract_dir}"
  cp -- "${helper_src}" "${frozen_source_dir}/${helper_name}"
  cp -- "${runner_src}" "${frozen_source_dir}/${runner_name}"
  cp -- "${upstream_frozen_helper}" \
    "${frozen_source_dir}/${reference_helper_name}"
  cp -R -- "${upstream_runtime_dir}/frozen_include/." \
    "${frozen_include_root}/"
  cp -R -- "${upstream_runtime_dir}/frozen_contracts/." \
    "${frozen_contract_dir}/"
  cp -- "${production_mdn_jkimyei}" \
    "${frozen_contract_dir}/wikimyei.inference.expected_value.mdn.jkimyei"

  (
    cd "${runtime_dir}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${runner_name}" \
      "frozen_sources/${reference_helper_name}" \
      frozen_include/piaabo/digest/sha256.h \
      frozen_include/wikimyei/inference/expected_value/mdn/channel_context_mdn.h \
      frozen_include/wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_backbone.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_loss.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_types.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h \
      >source_manifest.sha256
    sha256sum \
      frozen_contracts/synthetic_periodic_chart_manifest.v1.report \
      frozen_contracts/ujcamei.source.splits.dsl \
      frozen_contracts/ujcamei.source.retrieval.channels.dsl \
      frozen_contracts/ujcamei.source.registry.dsl \
      frozen_contracts/kikijyeba.topology.graph.dsl \
      frozen_contracts/wikimyei.expression.nodelift.srl.dsl \
      frozen_contracts/wikimyei.inference.expected_value.mdn.jkimyei \
      >contract_manifest.sha256
  )
}

compile_helper() {
  mkdir "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
    -I"${frozen_source_dir}" \
    -I"${frozen_include_root}" \
    -I"${repo_root}/src" \
    -I"${repo_root}/src/include" \
    -I"${repo_root}/src/include/torch_compat" \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${frozen_source_dir}/${helper_name}" \
    -L"${libtorch_path}/lib" \
    -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${helper_bin}"
}

run_probe() {
  local output="$1"
  local log="$2"
  local device_args=()

  export CUBLAS_WORKSPACE_CONFIG=:4096:8
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  if [[ "${SYNTHETIC_MDN_FROZEN_REPRESENTATION_K1_OBJECTIVE_AB_CPU:-false}" == \
    "true" ]]; then
    device_args+=(--cpu)
  fi

  "${helper_bin}" \
    --representation-probe "${representation_probe}" \
    --raw-history-report "${raw_history_report}" \
    --frozen-k1-report "${frozen_k1_report}" \
    --alpha-prefix "${alpha_prefix}" \
    --beta-prefix "${beta_prefix}" \
    --gamma-prefix "${gamma_prefix}" \
    --output "${output}" \
    --schema-id "${schema_id}" \
    "${device_args[@]}" \
    >"${log}" 2>&1
  require_nonempty_file "${output}"
}

require_identical_outputs() {
  require_nonempty_file "${main_probe}"
  require_nonempty_file "${replay_probe}"
  require_nonempty_file "${main_log}"
  require_nonempty_file "${replay_log}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay reports are not byte-identical"
  cmp -s "${main_log}" "${replay_log}" ||
    fail "main/replay logs are not byte-identical"
}

seal_outputs() {
  (
    cd "${runtime_dir}"
    sha256sum \
      "main/${schema_id}.probe" \
      "main/${schema_id}.stdout.log" \
      "replay/${schema_id}.probe" \
      "replay/${schema_id}.stdout.log" \
      >output_manifest.sha256
    sha256sum \
      bin/mdn_frozen_representation_k1_objective_ab \
      input_manifest.sha256 \
      source_manifest.sha256 \
      contract_manifest.sha256 \
      output_manifest.sha256 \
      >master.sha256
  )
}

verify_snapshot_rows() {
  local path="$1"
  local actual_rows
  actual_rows="$(awk 'END { print NR + 0 }' "${path}")"
  [[ "${actual_rows}" == "${prefix_rows}" ]] ||
    fail "sealed snapshot is not exactly ${prefix_rows} rows: ${path}"
}

verify_runtime_seals() {
  require_directory "${runtime_dir}"
  require_directory "${build_dir}"
  require_directory "${frozen_source_dir}"
  require_directory "${frozen_include_root}"
  require_directory "${frozen_contract_dir}"
  require_directory "${input_prefix_dir}"
  require_directory "${frozen_input_dir}"
  require_directory "${main_dir}"
  require_directory "${replay_dir}"
  require_nonempty_file "${runtime_dir}/input_manifest.sha256"
  require_nonempty_file "${runtime_dir}/source_manifest.sha256"
  require_nonempty_file "${runtime_dir}/contract_manifest.sha256"
  require_nonempty_file "${runtime_dir}/output_manifest.sha256"
  require_nonempty_file "${runtime_dir}/master.sha256"
  (
    cd "${runtime_dir}"
    sha256sum -c input_manifest.sha256 >/dev/null
    sha256sum -c source_manifest.sha256 >/dev/null
    sha256sum -c contract_manifest.sha256 >/dev/null
    sha256sum -c output_manifest.sha256 >/dev/null
    sha256sum -c master.sha256 >/dev/null
  )
  verify_snapshot_rows "${alpha_prefix}"
  verify_snapshot_rows "${beta_prefix}"
  verify_snapshot_rows "${gamma_prefix}"
  require_identical_outputs
  expect_file_value "${main_probe}" schema_id "${schema_id}"
  expect_file_value "${main_probe}" status complete
  expect_file_value "${main_probe}" boundary.historical_opened false
  expect_file_value "${main_probe}" boundary.unconsumed_holdout_opened false
}

verify_live_sources_match_frozen_runtime() {
  verify_upstream_runtime
  verify_live_prefix_matches_snapshot "${alpha_source}" "${alpha_prefix}"
  verify_live_prefix_matches_snapshot "${beta_source}" "${beta_prefix}"
  verify_live_prefix_matches_snapshot "${gamma_source}" "${gamma_prefix}"
  cmp -s "${helper_src}" "${frozen_source_dir}/${helper_name}" ||
    fail "live objective A/B helper differs from sealed runtime"
  cmp -s "${runner_src}" "${frozen_source_dir}/${runner_name}" ||
    fail "live objective A/B runner differs from sealed runtime"
  cmp -s "${upstream_frozen_helper}" \
    "${frozen_source_dir}/${reference_helper_name}" ||
    fail "upstream reference helper differs from sealed objective runtime"
  cmp -s \
    "${upstream_runtime_dir}/frozen_inputs/representation_edge_features.probe" \
    "${representation_probe}" ||
    fail "upstream representation probe differs from sealed objective input"
  cmp -s \
    "${upstream_runtime_dir}/frozen_inputs/synthetic_mdn_raw_history_isolation_v1.report" \
    "${raw_history_report}" ||
    fail "upstream raw-history report differs from sealed objective input"
  cmp -s "${upstream_installed_report}" "${frozen_k1_report}" ||
    fail "upstream frozen K1 report differs from sealed objective input"
  cmp -s "${production_mdn_jkimyei}" \
    "${frozen_contract_dir}/wikimyei.inference.expected_value.mdn.jkimyei" ||
    fail "production MDN objective config differs from sealed contract"
}

install_report_no_clobber() {
  local temporary_report
  if [[ -e "${installed_report}" || -L "${installed_report}" ]]; then
    require_nonempty_file "${installed_report}"
    cmp -s "${main_probe}" "${installed_report}" ||
      fail "installed report differs from sealed main probe"
    return
  fi
  temporary_report="$(mktemp "${artifacts_dir}/.${schema_id}.report.XXXXXXXX")"
  cp -- "${main_probe}" "${temporary_report}"
  chmod 0644 "${temporary_report}"
  cmp -s "${main_probe}" "${temporary_report}" ||
    fail "temporary report copy differs from sealed main probe"
  mv -T -n "${temporary_report}" "${installed_report}" ||
    fail "atomic report installation failed"
  if [[ -e "${temporary_report}" || -L "${temporary_report}" ]]; then
    if [[ -f "${installed_report}" && ! -L "${installed_report}" ]] &&
      cmp -s "${main_probe}" "${installed_report}"; then
      rm -f -- "${temporary_report}"
    else
      fail "no-clobber report installation postcondition failed"
    fi
  fi
  require_nonempty_file "${installed_report}"
  cmp -s "${main_probe}" "${installed_report}" ||
    fail "installed report differs from sealed main probe"
}

verify_complete() {
  set_runtime_paths "${final_runtime_dir}"
  verify_runtime_seals
  require_nonempty_file "${installed_report}"
  cmp -s "${main_probe}" "${installed_report}" ||
    fail "installed report differs from sealed main probe"
  printf 'verified_runtime=%s\n' "${final_runtime_dir}"
  printf 'verified_report=%s\n' "${installed_report}"
}

release_lock() {
  if [[ "${lock_held}" == "true" ]]; then
    rmdir "${lock_dir}" 2>/dev/null || true
    lock_held=false
  fi
}

run_all() {
  local scratch_runtime
  if [[ -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" &&
    -f "${final_runtime_dir}/master.sha256" ]]; then
    set_runtime_paths "${final_runtime_dir}"
    verify_runtime_seals
    verify_live_sources_match_frozen_runtime
    install_report_no_clobber
    verify_complete
    return
  fi

  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "refusing to overwrite partial runtime: ${final_runtime_dir}"
  [[ ! -e "${installed_report}" && ! -L "${installed_report}" ]] ||
    fail "report exists without a sealed runtime: ${installed_report}"
  preflight
  [[ ! -L "${runtime_parent}" ]] || fail "runtime parent is a symlink"
  mkdir -p "${runtime_parent}"
  require_directory "${runtime_parent}"
  mkdir "${lock_dir}" || fail "another installation holds ${lock_dir}"
  lock_held=true
  trap release_lock EXIT

  [[ ! -e "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "final runtime appeared after lock acquisition"
  scratch_runtime="$(mktemp -d "${runtime_parent}/${schema_id}.scratch.XXXXXXXX")"
  require_directory "${scratch_runtime}"
  set_runtime_paths "${scratch_runtime}"
  mkdir "${main_dir}" "${replay_dir}"
  snapshot_inputs
  freeze_sources
  compile_helper
  run_probe "${main_probe}" "${main_log}"
  run_probe "${replay_probe}" "${replay_log}"
  require_identical_outputs
  seal_outputs
  verify_runtime_seals

  mv -T -n "${scratch_runtime}" "${final_runtime_dir}" ||
    fail "atomic runtime installation failed; scratch runtime preserved"
  [[ ! -e "${scratch_runtime}" && ! -L "${scratch_runtime}" &&
    -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "no-clobber runtime installation postcondition failed"
  set_runtime_paths "${final_runtime_dir}"
  install_report_no_clobber
  release_lock
  trap - EXIT
  verify_complete
}

[[ "$#" == 1 ]] || fail "usage: $0 {--preflight|--run|--verify}"
case "$1" in
  --preflight)
    preflight
    ;;
  --run)
    run_all
    ;;
  --verify)
    verify_complete
    ;;
  *)
    fail "usage: $0 {--preflight|--run|--verify}"
    ;;
esac
