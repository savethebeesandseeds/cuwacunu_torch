#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_frozen_representation_k1_isolation_v1"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
final_runtime_dir="${runtime_parent}/${schema_id}"
lock_dir="${final_runtime_dir}.lock"

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
data_root="${benchmark_root}/data/raw"
artifacts_dir="${benchmark_root}/artifacts"
installed_report="${artifacts_dir}/${schema_id}.report"
benchmark_manifest="${artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"
split_catalog="${benchmark_root}/ujcamei.source.splits.dsl"
retrieval_channels="${benchmark_root}/ujcamei.source.retrieval.channels.dsl"
source_registry="${benchmark_root}/ujcamei.source.registry.dsl"
topology_graph="${benchmark_root}/kikijyeba.topology.graph.dsl"
nodelift_spec="${repo_root}/src/config/wikimyei.expression.nodelift.srl.dsl"

helper_name="mdn_frozen_representation_k1_isolation.cpp"
runner_name="run_mdn_frozen_representation_k1_isolation.sh"
helper_src="${script_dir}/${helper_name}"
runner_src="${script_dir}/${runner_name}"

alpha_source="${data_root}/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_source="${data_root}/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_source="${data_root}/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
prefix_rows=760
capture_runtime="${runtime_parent}/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
capture_probe_source="${capture_runtime}/representation_edge_features.probe"
capture_job_manifest_source="${capture_runtime}/job.manifest"
capture_channel_report_source="${capture_runtime}/channel_inference.report"
capture_representation_report_source="${capture_runtime}/channel_inference.report.representation.lls"
capture_result_source="${capture_runtime}/runtime.result.fact"
raw_history_report_source="${artifacts_dir}/synthetic_mdn_raw_history_isolation_v1.report"
raw_helper_source="${script_dir}/mdn_raw_history_isolation.cpp"
raw_runner_source="${script_dir}/run_mdn_raw_history_isolation.sh"

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

set_runtime_paths() {
  runtime_dir="$1"
  build_dir="${runtime_dir}/bin"
  frozen_source_dir="${runtime_dir}/frozen_sources"
  frozen_include_root="${runtime_dir}/frozen_include"
  frozen_contract_dir="${runtime_dir}/frozen_contracts"
  input_prefix_dir="${runtime_dir}/input_prefixes"
  frozen_input_dir="${runtime_dir}/frozen_inputs"
  helper_bin="${build_dir}/mdn_frozen_representation_k1_isolation"
  representation_probe="${frozen_input_dir}/representation_edge_features.probe"
  raw_history_report="${frozen_input_dir}/synthetic_mdn_raw_history_isolation_v1.report"
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

# This deliberately exits while processing row 760. Do not replace it with a
# full-file line count or digest: the isolation boundary must remain unopened.
validate_source_prefix() {
  local source="$1"
  require_regular_file "${source}"
  awk -F, -v limit="${prefix_rows}" '
    NR <= limit && NF != 12 { malformed = 1; exit }
    NR == limit { complete = 1; exit }
    END { if (malformed || !complete) exit 1 }
  ' "${source}" ||
    fail "source does not contain ${prefix_rows} valid kline rows: ${source}"
}

manifest_value() {
  local key="$1"
  awk -F= -v key="${key}" '$1 == key { print $2; exit }' "${benchmark_manifest}"
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

validate_capture_probe() {
  local probe_path="${1:-${capture_probe_source}}"
  require_nonempty_file "${probe_path}"
  awk -F, '
    BEGIN {
      header = "record_schema,anchor_key,anchor_index,anchor_local_index," \
               "edge_index,edge_id,base_node_id,quote_node_id,channel_index," \
               "target_edge_close_return,feature_count,feature_values"
      schema = "kikijyeba.synthetic.representation_edge_feature_probe.v1"
      edge_id[0] = "SYNALPHASYNUSD"
      edge_id[1] = "SYNBETASYNUSD"
      edge_id[2] = "SYNGAMMASYNUSD"
      base_id[0] = "SYNALPHA"
      base_id[1] = "SYNBETA"
      base_id[2] = "SYNGAMMA"
    }
    NR == 1 {
      if ($0 != header || NF != 12) exit 1
      next
    }
    {
      ordinal = NR - 2
      anchor = int(ordinal / 9)
      within = ordinal % 9
      edge = int(within / 3)
      channel = within % 3
      if (NF != 12 || $1 != schema || $3 != anchor || $4 != (anchor % 64) ||
          $5 != edge || $6 != edge_id[edge] || $7 != base_id[edge] ||
          $8 != "SYNUSD" || $9 != channel || $11 != 96) exit 1
    }
    END { if (NR != 6571) exit 1 }
  ' "${probe_path}" ||
    fail "representation capture schema/order/completeness preflight failed"
}

validate_capture_provenance() {
  require_nonempty_file "${capture_job_manifest_source}"
  require_nonempty_file "${capture_channel_report_source}"
  require_nonempty_file "${capture_representation_report_source}"
  require_nonempty_file "${capture_result_source}"
  require_nonempty_file "${raw_history_report_source}"
  require_nonempty_file "${raw_helper_source}"
  require_nonempty_file "${raw_runner_source}"

  expect_file_value "${capture_job_manifest_source}" manifest_format \
    kikijyeba.runtime.job_manifest.v1
  expect_file_value "${capture_job_manifest_source}" job_kind \
    channel_inference_mdn
  expect_file_value "${capture_job_manifest_source}" resolved_anchor_index_begin 0
  expect_file_value "${capture_job_manifest_source}" resolved_anchor_index_end 730
  expect_file_value "${capture_job_manifest_source}" source_input_length 30
  expect_file_value "${capture_job_manifest_source}" source_future_length 1
  expect_file_value "${capture_job_manifest_source}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  expect_file_value "${capture_job_manifest_source}" active_representation_family \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  [[ -n "$(file_value "${capture_job_manifest_source}" \
    input_representation_checkpoint_path)" ]] ||
    fail "capture manifest lacks representation checkpoint provenance"

  expect_file_value "${capture_channel_report_source}" context_contract \
    graph_order.channel_node_representation.v1
  expect_file_value "${capture_channel_report_source}" context_value_shape \
    '[B,N,C,De]'
  expect_file_value "${capture_channel_report_source}" channel_count 3
  expect_file_value "${capture_channel_report_source}" context_dim 32
  expect_file_value "${capture_channel_report_source}" representation_checkpoint_loaded true
  expect_file_value "${capture_channel_report_source}" requested_anchor_index_begin 0
  expect_file_value "${capture_channel_report_source}" requested_anchor_index_end 730
  expect_file_value "${capture_channel_report_source}" wave_streamed_anchor_count 730
  expect_file_value "${capture_channel_report_source}" node_ids \
    SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA
  expect_file_value "${capture_channel_report_source}" edge_ids \
    SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD

  grep -Fq 'schema:str = wikimyei.representation.mtf_jepa_mae_vicreg.runtime.v1' \
    "${capture_representation_report_source}" ||
    fail "unexpected capture representation runtime schema"
  grep -Fq 'node_count[0,+inf):uint = 4' \
    "${capture_representation_report_source}" ||
    fail "unexpected captured representation node count"
  grep -Fq 'channel_count[0,+inf):uint = 3' \
    "${capture_representation_report_source}" ||
    fail "unexpected captured representation channel count"
  grep -Fq 'encoding_dim[0,+inf):uint = 32' \
    "${capture_representation_report_source}" ||
    fail "unexpected captured representation width"
  grep -Fq 'detach_to_cpu:bool = true' \
    "${capture_representation_report_source}" ||
    fail "capture representation was not detached to CPU"

  expect_file_value "${capture_result_source}" status completed
  expect_file_value "${capture_result_source}" checkpoint_written false
  expect_file_value "${capture_result_source}" model_state_mutated false
  expect_file_value "${raw_history_report_source}" schema_id \
    synthetic_mdn_raw_history_isolation_v1
  expect_file_value "${raw_history_report_source}" status complete
  expect_file_value "${raw_history_report_source}" \
    classification.raw_history_k1_pass true
  expect_file_value "${raw_history_report_source}" boundary.effective_fit '[1,554)'
  expect_file_value "${raw_history_report_source}" boundary.purge '[554,584)'
  expect_file_value "${raw_history_report_source}" boundary.validation '[584,730)'
  validate_capture_probe
}

validate_benchmark_contracts() {
  require_regular_file "${benchmark_manifest}"
  require_regular_file "${split_catalog}"
  require_regular_file "${retrieval_channels}"
  require_regular_file "${source_registry}"
  require_regular_file "${topology_graph}"
  require_regular_file "${nodelift_spec}"

  [[ "$(manifest_value row_count)" == "1200" ]] ||
    fail "unexpected synthetic row_count"
  [[ "$(manifest_value input_length)" == "30" ]] ||
    fail "unexpected synthetic input_length"
  [[ "$(manifest_value accepted_anchor_count)" == "1170" ]] ||
    fail "unexpected accepted_anchor_count"
  [[ "$(manifest_value train_anchor_begin)" == "0" &&
     "$(manifest_value train_anchor_end_exclusive)" == "730" ]] ||
    fail "unexpected benchmark train boundary"
  [[ "$(manifest_value eval_anchor_begin)" == "760" &&
     "$(manifest_value eval_anchor_end_exclusive)" == "1088" ]] ||
    fail "unexpected benchmark historical boundary"
  [[ "$(manifest_value test_anchor_begin)" == "1088" &&
     "$(manifest_value test_anchor_end_exclusive)" == "1170" ]] ||
    fail "unexpected benchmark holdout boundary"
  grep -Eq '\|[[:space:]]*1d[[:space:]]*\|[[:space:]]*true[[:space:]]*\|[[:space:]]*kline[[:space:]]*\|[[:space:]]*30[[:space:]]*\|[[:space:]]*1[[:space:]]*\|.*log_returns' \
    "${retrieval_channels}" || fail "active 1d log-return channel contract missing"
  awk -F'|' '
    /\|/ && $0 !~ /^[-\\]/ && $0 !~ /interval/ {
      interval = $2
      active = $3
      record_type = $4
      gsub(/[[:space:]]/, "", interval)
      gsub(/[[:space:]]/, "", active)
      gsub(/[[:space:]]/, "", record_type)
      if (active == "true" && record_type == "kline") {
        if (count == 2 && interval != "1d") exit 1
        ++count
      }
    }
    END { if (count != 3) exit 1 }
  ' "${retrieval_channels}" ||
    fail "1d retrieval channel is not canonical channel_index=2"
  grep -Fq 'GAUGE_POLICY = uniform_per_component;' "${nodelift_spec}" ||
    fail "uniform NodeLift gauge contract missing"
  grep -Fq 'LIFT_FUTURE = target_side;' "${nodelift_spec}" ||
    fail "target-side NodeLift contract missing"
}

preflight() {
  local command_name
  for command_name in awk chmod cmp cp g++ grep mkdir mktemp mv rm rmdir sed \
    sha256sum; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done

  require_regular_file "${helper_src}"
  require_regular_file "${runner_src}"
  require_directory "${artifacts_dir}"
  require_directory "${libtorch_path}/include"
  require_directory "${libtorch_path}/include/torch/csrc/api/include"
  require_directory "${libtorch_path}/lib"

  validate_source_prefix "${alpha_source}"
  validate_source_prefix "${beta_source}"
  validate_source_prefix "${gamma_source}"
  validate_benchmark_contracts
  validate_capture_provenance

  printf 'preflight_ok=true\n'
  printf 'schema_id=%s\n' "${schema_id}"
  printf 'source_prefix_rows=%s\n' "${prefix_rows}"
  printf 'representation_capture_rows=6570\n'
  printf 'representation_capture_channel_index=2\n'
}

snapshot_prefix() {
  local source="$1"
  local destination="$2"
  local actual_rows

  [[ ! -e "${destination}" && ! -L "${destination}" ]] ||
    fail "refusing to overwrite input snapshot: ${destination}"
  sed -n "1,${prefix_rows}p;${prefix_rows}q" "${source}" >"${destination}"
  actual_rows="$(awk 'END { print NR + 0 }' "${destination}")"
  [[ "${actual_rows}" == "${prefix_rows}" ]] ||
    fail "input snapshot row-count mismatch: ${destination}"
}

snapshot_inputs() {
  mkdir "${input_prefix_dir}"
  mkdir "${frozen_input_dir}"
  snapshot_prefix "${alpha_source}" "${alpha_prefix}"
  snapshot_prefix "${beta_source}" "${beta_prefix}"
  snapshot_prefix "${gamma_source}" "${gamma_prefix}"
  cp -- "${capture_probe_source}" "${representation_probe}"
  cp -- "${raw_history_report_source}" "${raw_history_report}"
  cp -- "${capture_job_manifest_source}" \
    "${frozen_input_dir}/capture.job.manifest"
  cp -- "${capture_channel_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report"
  cp -- "${capture_representation_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report.representation.lls"
  cp -- "${capture_result_source}" \
    "${frozen_input_dir}/capture.runtime.result.fact"

  {
    echo "schema_id=${schema_id}"
    echo "prefix_data_rows=${prefix_rows}"
    echo "representation_capture_rows=6570"
    echo "representation_capture_anchor_range=[0,730)"
    echo "representation_capture_channel_index=2"
    echo "raw_history_reference_schema=synthetic_mdn_raw_history_isolation_v1"
    echo "alpha_source=src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
    echo "beta_source=src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
    echo "gamma_source=src/config/benchmarks/synthetic_continuous_graph_v1/data/raw/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
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
      frozen_inputs/capture.job.manifest \
      frozen_inputs/capture.channel_inference.report \
      frozen_inputs/capture.channel_inference.report.representation.lls \
      frozen_inputs/capture.runtime.result.fact \
      >input_manifest.sha256
  )
}

freeze_sources() {
  mkdir "${frozen_source_dir}"
  mkdir "${frozen_contract_dir}"
  mkdir -p \
    "${frozen_include_root}/piaabo/digest" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn"
  cp -- "${helper_src}" "${frozen_source_dir}/${helper_name}"
  cp -- "${runner_src}" "${frozen_source_dir}/${runner_name}"
  cp -- "${raw_helper_source}" \
    "${frozen_source_dir}/mdn_raw_history_isolation.reference.cpp"
  cp -- "${raw_runner_source}" \
    "${frozen_source_dir}/run_mdn_raw_history_isolation.reference.sh"
  cp -- "${repo_root}/src/include/piaabo/digest/sha256.h" \
    "${frozen_include_root}/piaabo/digest/sha256.h"
  for header_name in \
    channel_context_mdn.h \
    channel_context_mdn_train_model.h \
    mixture_density_network_backbone.h \
    mixture_density_network_head.h \
    mixture_density_network_loss.h \
    mixture_density_network_types.h \
    mixture_density_network_utils.h; do
    cp -- \
      "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/${header_name}" \
      "${frozen_include_root}/wikimyei/inference/expected_value/mdn/${header_name}"
  done
  cp -- "${benchmark_manifest}" \
    "${frozen_contract_dir}/synthetic_periodic_chart_manifest.v1.report"
  cp -- "${split_catalog}" \
    "${frozen_contract_dir}/ujcamei.source.splits.dsl"
  cp -- "${retrieval_channels}" \
    "${frozen_contract_dir}/ujcamei.source.retrieval.channels.dsl"
  cp -- "${source_registry}" \
    "${frozen_contract_dir}/ujcamei.source.registry.dsl"
  cp -- "${topology_graph}" \
    "${frozen_contract_dir}/kikijyeba.topology.graph.dsl"
  cp -- "${nodelift_spec}" \
    "${frozen_contract_dir}/wikimyei.expression.nodelift.srl.dsl"
  (
    cd "${runtime_dir}"
    sha256sum \
      "frozen_sources/${helper_name}" \
      "frozen_sources/${runner_name}" \
      frozen_sources/mdn_raw_history_isolation.reference.cpp \
      frozen_sources/run_mdn_raw_history_isolation.reference.sh \
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
      >contract_manifest.sha256
  )
}

compile_helper() {
  mkdir "${build_dir}"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
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

  if [[ "${SYNTHETIC_MDN_FROZEN_REPRESENTATION_K1_CPU:-false}" == "true" ]]; then
    device_args+=(--cpu)
  fi

  "${helper_bin}" \
    --representation-probe "${representation_probe}" \
    --raw-history-report "${raw_history_report}" \
    --alpha-prefix "${alpha_prefix}" \
    --beta-prefix "${beta_prefix}" \
    --gamma-prefix "${gamma_prefix}" \
    --output "${output}" \
    --schema-id "${schema_id}" \
    "${device_args[@]}" \
    >"${log}" 2>&1
  require_nonempty_file "${output}"
}

require_identical_probes() {
  require_nonempty_file "${main_probe}"
  require_nonempty_file "${replay_probe}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay probes are not byte-identical"
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
      bin/mdn_frozen_representation_k1_isolation \
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
    fail "sealed input snapshot is not exactly ${prefix_rows} rows: ${path}"
}

verify_live_prefix_matches_snapshot() {
  local source="$1"
  local snapshot="$2"

  # Preserve the isolation boundary during reuse validation: compare only the
  # causal prefix and stop reading the live source as soon as row 760 arrives.
  sed -n "1,${prefix_rows}p;${prefix_rows}q" "${source}" |
    cmp -s - "${snapshot}" ||
    fail "live causal prefix differs from sealed input snapshot: ${source}"
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
  validate_capture_probe "${representation_probe}"
  require_identical_probes
}

verify_live_sources_match_frozen_runtime() {
  local header_name
  verify_live_prefix_matches_snapshot "${alpha_source}" "${alpha_prefix}"
  verify_live_prefix_matches_snapshot "${beta_source}" "${beta_prefix}"
  verify_live_prefix_matches_snapshot "${gamma_source}" "${gamma_prefix}"
  cmp -s "${helper_src}" "${frozen_source_dir}/${helper_name}" ||
    fail "live helper differs from sealed runtime; use a new schema or remove the stale incomplete result"
  cmp -s "${runner_src}" "${frozen_source_dir}/${runner_name}" ||
    fail "live runner differs from sealed runtime; use a new schema or remove the stale incomplete result"
  cmp -s "${raw_helper_source}" \
    "${frozen_source_dir}/mdn_raw_history_isolation.reference.cpp" ||
    fail "live raw-history reference helper differs from sealed runtime"
  cmp -s "${raw_runner_source}" \
    "${frozen_source_dir}/run_mdn_raw_history_isolation.reference.sh" ||
    fail "live raw-history reference runner differs from sealed runtime"
  cmp -s "${repo_root}/src/include/piaabo/digest/sha256.h" \
    "${frozen_include_root}/piaabo/digest/sha256.h" ||
    fail "live digest header differs from sealed runtime"
  for header_name in \
    channel_context_mdn.h \
    channel_context_mdn_train_model.h \
    mixture_density_network_backbone.h \
    mixture_density_network_head.h \
    mixture_density_network_loss.h \
    mixture_density_network_types.h \
    mixture_density_network_utils.h; do
    cmp -s \
      "${repo_root}/src/include/wikimyei/inference/expected_value/mdn/${header_name}" \
      "${frozen_include_root}/wikimyei/inference/expected_value/mdn/${header_name}" ||
      fail "live MDN header differs from sealed runtime: ${header_name}"
  done
  cmp -s "${benchmark_manifest}" \
    "${frozen_contract_dir}/synthetic_periodic_chart_manifest.v1.report" ||
    fail "live benchmark manifest differs from sealed runtime"
  cmp -s "${split_catalog}" \
    "${frozen_contract_dir}/ujcamei.source.splits.dsl" ||
    fail "live split catalog differs from sealed runtime"
  cmp -s "${retrieval_channels}" \
    "${frozen_contract_dir}/ujcamei.source.retrieval.channels.dsl" ||
    fail "live retrieval-channel contract differs from sealed runtime"
  cmp -s "${source_registry}" \
    "${frozen_contract_dir}/ujcamei.source.registry.dsl" ||
    fail "live source registry differs from sealed runtime"
  cmp -s "${topology_graph}" \
    "${frozen_contract_dir}/kikijyeba.topology.graph.dsl" ||
    fail "live topology graph differs from sealed runtime"
  cmp -s "${nodelift_spec}" \
    "${frozen_contract_dir}/wikimyei.expression.nodelift.srl.dsl" ||
    fail "live NodeLift spec differs from sealed runtime"
  cmp -s "${capture_probe_source}" "${representation_probe}" ||
    fail "live representation capture differs from sealed runtime"
  cmp -s "${raw_history_report_source}" "${raw_history_report}" ||
    fail "live raw-history reference report differs from sealed runtime"
  cmp -s "${capture_job_manifest_source}" \
    "${frozen_input_dir}/capture.job.manifest" ||
    fail "live capture job manifest differs from sealed runtime"
  cmp -s "${capture_channel_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report" ||
    fail "live capture channel report differs from sealed runtime"
  cmp -s "${capture_representation_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report.representation.lls" ||
    fail "live capture representation report differs from sealed runtime"
  cmp -s "${capture_result_source}" \
    "${frozen_input_dir}/capture.runtime.result.fact" ||
    fail "live capture result fact differs from sealed runtime"
}

install_report_no_clobber() {
  local temporary_report

  if [[ -e "${installed_report}" || -L "${installed_report}" ]]; then
    require_nonempty_file "${installed_report}"
    cmp -s "${main_probe}" "${installed_report}" ||
      fail "installed report differs from sealed main probe: ${installed_report}"
    return
  fi

  temporary_report="$(mktemp "${artifacts_dir}/.${schema_id}.report.XXXXXXXX")"
  cp -- "${main_probe}" "${temporary_report}"
  chmod 0644 "${temporary_report}"
  cmp -s "${main_probe}" "${temporary_report}" ||
    fail "temporary report copy differs from sealed main probe"
  mv -T -n "${temporary_report}" "${installed_report}" ||
    fail "atomic report installation failed: ${installed_report}"

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
  require_identical_probes
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

[[ "$#" == 1 ]] ||
  fail "usage: $0 {--preflight|--run|--verify}"

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
