#!/usr/bin/env bash
set -euo pipefail
umask 077

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="synthetic_mdn_cached_feature_input_norm_ablation_v1"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1"
final_runtime_dir="${runtime_parent}/${schema_id}"
lock_dir="${final_runtime_dir}.lock"

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"
installed_report="${artifacts_dir}/${schema_id}.report"
benchmark_manifest="${artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"
split_catalog="${benchmark_root}/ujcamei.source.splits.dsl"
retrieval_channels="${benchmark_root}/ujcamei.source.retrieval.channels.dsl"
source_registry="${benchmark_root}/ujcamei.source.registry.dsl"
topology_graph="${benchmark_root}/kikijyeba.topology.graph.dsl"
nodelift_spec="${repo_root}/src/config/wikimyei.expression.nodelift.srl.dsl"

helper_name="mdn_cached_feature_input_norm_ablation.cpp"
runner_name="run_mdn_cached_feature_input_norm_ablation.sh"
helper_src="${script_dir}/${helper_name}"
runner_src="${script_dir}/${runner_name}"

capture_runtime="${runtime_parent}/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730"
capture_probe_source="${capture_runtime}/mdn_edge_context_features.probe"
capture_job_manifest_source="${capture_runtime}/job.manifest"
capture_channel_report_source="${capture_runtime}/channel_inference.report"
capture_representation_report_source="${capture_runtime}/channel_inference.report.representation.lls"
capture_result_source="${capture_runtime}/runtime.result.fact"

representation_training_dir="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001"
representation_training_manifest_source="${representation_training_dir}/job.manifest"
representation_checkpoint_fact_source="${representation_training_dir}/lattice.checkpoint.fact"
mdn_training_dir="${repo_root}/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001"
mdn_training_manifest_source="${mdn_training_dir}/job.manifest"
mdn_checkpoint_fact_source="${mdn_training_dir}/lattice.checkpoint.fact"

digest_header="${repo_root}/src/include/piaabo/digest/sha256.h"
mdn_include_dir="${repo_root}/src/include/wikimyei/inference/expected_value/mdn"
head_header="${mdn_include_dir}/mixture_density_network_head.h"
types_header="${mdn_include_dir}/mixture_density_network_types.h"
utils_header="${mdn_include_dir}/mixture_density_network_utils.h"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"

runtime_dir=""
build_dir=""
frozen_source_dir=""
frozen_include_root=""
frozen_contract_dir=""
frozen_input_dir=""
helper_bin=""
cached_probe=""
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
  frozen_input_dir="${runtime_dir}/frozen_inputs"
  helper_bin="${build_dir}/mdn_cached_feature_input_norm_ablation"
  cached_probe="${frozen_input_dir}/mdn_edge_context_features.probe"
  main_dir="${runtime_dir}/main"
  replay_dir="${runtime_dir}/replay"
  main_probe="${main_dir}/${schema_id}.probe"
  replay_probe="${replay_dir}/${schema_id}.probe"
  main_log="${main_dir}/${schema_id}.stdout.log"
  replay_log="${replay_dir}/${schema_id}.stdout.log"
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

manifest_value() {
  local key="$1"
  file_value "${benchmark_manifest}" "${key}"
}

validate_capture_probe() {
  local probe_path="${1:-${capture_probe_source}}"
  require_nonempty_file "${probe_path}"
  awk -F, '
    BEGIN {
      header = "record_schema,anchor_key,anchor_index,anchor_local_index," \
               "edge_index,edge_id,base_node_id,quote_node_id,channel_index," \
               "target_edge_close_return,feature_count,feature_values"
      schema = "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1"
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
      if (NF != 12 || $1 != schema || $3 != anchor ||
          $4 != (anchor % 64) || $5 != edge || $6 != edge_id[edge] ||
          $7 != base_id[edge] || $8 != "SYNUSD" || $9 != channel ||
          $11 != 400) exit 1
    }
    END { if (NR != 6571) exit 1 }
  ' "${probe_path}" ||
    fail "cached MDN feature probe schema/order/completeness check failed"
}

validate_training_exposure() {
  require_nonempty_file "${representation_training_manifest_source}"
  require_nonempty_file "${representation_checkpoint_fact_source}"
  require_nonempty_file "${mdn_training_manifest_source}"
  require_nonempty_file "${mdn_checkpoint_fact_source}"

  expect_file_value "${representation_training_manifest_source}" \
    manifest_format kikijyeba.runtime.job_manifest.v1
  expect_file_value "${representation_training_manifest_source}" job_kind \
    channel_representation_mtf_jepa_mae_vicreg
  expect_file_value "${representation_training_manifest_source}" \
    resolved_anchor_index_begin 0
  expect_file_value "${representation_training_manifest_source}" \
    resolved_anchor_index_end 730
  expect_file_value "${representation_checkpoint_fact_source}" \
    fit_anchor_begin 0
  expect_file_value "${representation_checkpoint_fact_source}" fit_anchor_end \
    730
  expect_file_value "${representation_checkpoint_fact_source}" \
    fit_anchor_range_bound true

  expect_file_value "${mdn_training_manifest_source}" manifest_format \
    kikijyeba.runtime.job_manifest.v1
  expect_file_value "${mdn_training_manifest_source}" job_kind \
    channel_inference_mdn
  expect_file_value "${mdn_training_manifest_source}" \
    resolved_anchor_index_begin 0
  expect_file_value "${mdn_training_manifest_source}" \
    resolved_anchor_index_end 730
  expect_file_value "${mdn_checkpoint_fact_source}" fit_anchor_begin 0
  expect_file_value "${mdn_checkpoint_fact_source}" fit_anchor_end 730
  expect_file_value "${mdn_checkpoint_fact_source}" fit_anchor_range_bound true

  [[ "$(file_value "${capture_job_manifest_source}" \
    input_representation_checkpoint_path)" == \
    "$(file_value "${representation_checkpoint_fact_source}" \
      checkpoint_path)" ]] ||
    fail "capture representation checkpoint does not match sealed exposure fact"
  [[ "$(file_value "${capture_job_manifest_source}" \
    input_mdn_checkpoint_path)" == \
    "$(file_value "${mdn_checkpoint_fact_source}" checkpoint_path)" ]] ||
    fail "capture MDN checkpoint does not match sealed exposure fact"
}

validate_capture_provenance() {
  require_nonempty_file "${capture_job_manifest_source}"
  require_nonempty_file "${capture_channel_report_source}"
  require_nonempty_file "${capture_representation_report_source}"
  require_nonempty_file "${capture_result_source}"

  expect_file_value "${capture_job_manifest_source}" manifest_format \
    kikijyeba.runtime.job_manifest.v1
  expect_file_value "${capture_job_manifest_source}" job_kind \
    channel_inference_mdn
  expect_file_value "${capture_job_manifest_source}" \
    resolved_anchor_index_begin 0
  expect_file_value "${capture_job_manifest_source}" \
    resolved_anchor_index_end 730
  expect_file_value "${capture_job_manifest_source}" source_input_length 30
  expect_file_value "${capture_job_manifest_source}" source_future_length 1
  expect_file_value "${capture_job_manifest_source}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg
  [[ -n "$(file_value "${capture_job_manifest_source}" \
    input_representation_checkpoint_path)" ]] ||
    fail "capture lacks representation-checkpoint provenance"
  [[ -n "$(file_value "${capture_job_manifest_source}" \
    input_mdn_checkpoint_path)" ]] ||
    fail "capture lacks MDN-checkpoint provenance"

  expect_file_value "${capture_channel_report_source}" context_contract \
    graph_order.channel_node_representation.v1
  expect_file_value "${capture_channel_report_source}" channel_count 3
  expect_file_value "${capture_channel_report_source}" context_dim 32
  expect_file_value "${capture_channel_report_source}" \
    representation_checkpoint_loaded true
  expect_file_value "${capture_channel_report_source}" mdn_checkpoint_loaded \
    true
  expect_file_value "${capture_channel_report_source}" \
    requested_anchor_index_begin 0
  expect_file_value "${capture_channel_report_source}" \
    requested_anchor_index_end 730
  expect_file_value "${capture_channel_report_source}" \
    wave_streamed_anchor_count 730
  expect_file_value "${capture_channel_report_source}" \
    direct_edge_return_readout_identity_mode edge_embedding_per_edge
  expect_file_value "${capture_channel_report_source}" \
    direct_edge_return_readout_base_edge_count 3
  expect_file_value "${capture_channel_report_source}" \
    direct_edge_return_readout_identity_embedding_dim 16
  expect_file_value "${capture_channel_report_source}" \
    direct_edge_return_readout_adapter_hidden_dim 128
  expect_file_value "${capture_channel_report_source}" \
    mdn_edge_context_feature_probe_written true
  expect_file_value "${capture_channel_report_source}" \
    mdn_edge_context_feature_probe_row_count 6570
  expect_file_value "${capture_channel_report_source}" checkpoint_written false

  grep -Fq 'schema:str = wikimyei.representation.mtf_jepa_mae_vicreg.runtime.v1' \
    "${capture_representation_report_source}" ||
    fail "unexpected capture representation runtime schema"
  grep -Fq 'detach_to_cpu:bool = true' \
    "${capture_representation_report_source}" ||
    fail "captured representation was not detached to CPU"
  expect_file_value "${capture_result_source}" status completed
  expect_file_value "${capture_result_source}" checkpoint_written false
  expect_file_value "${capture_result_source}" model_state_mutated false

  validate_training_exposure
  validate_capture_probe
}

validate_benchmark_contracts() {
  require_nonempty_file "${benchmark_manifest}"
  require_nonempty_file "${split_catalog}"
  require_nonempty_file "${retrieval_channels}"
  require_nonempty_file "${source_registry}"
  require_nonempty_file "${topology_graph}"
  require_nonempty_file "${nodelift_spec}"

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
  grep -Fq 'GAUGE_POLICY = uniform_per_component;' "${nodelift_spec}" ||
    fail "uniform NodeLift gauge contract missing"
}

preflight() {
  local command_name
  for command_name in awk chmod cmp cp g++ grep mkdir mktemp mv rm rmdir \
    sha256sum; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done

  require_regular_file "${helper_src}"
  require_regular_file "${runner_src}"
  require_directory "${artifacts_dir}"
  require_nonempty_file "${digest_header}"
  require_nonempty_file "${head_header}"
  require_nonempty_file "${types_header}"
  require_nonempty_file "${utils_header}"
  require_directory "${libtorch_path}/include"
  require_directory "${libtorch_path}/include/torch/csrc/api/include"
  require_directory "${libtorch_path}/lib"

  grep -Fq 'hidden->forward(input_norm->forward(x))' "${head_header}" ||
    fail "production DirectEdgeReturnHead sample-LayerNorm call changed"
  grep -Fq 'identity_mode == "edge_embedding_per_edge"' "${head_header}" ||
    fail "production edge_embedding_per_edge mode is unavailable"
  validate_benchmark_contracts
  validate_capture_provenance

  printf 'preflight_ok=true\n'
  printf 'schema_id=%s\n' "${schema_id}"
  printf 'capture_rows=6570\n'
  printf 'capture_anchor_range=[0,730)\n'
  printf 'upstream_validation_exposure=transductive\n'
  printf 'historical_or_holdout_input_opened=false\n'
}

snapshot_inputs() {
  mkdir "${frozen_input_dir}"
  cp -- "${capture_probe_source}" "${cached_probe}"
  cp -- "${capture_job_manifest_source}" \
    "${frozen_input_dir}/capture.job.manifest"
  cp -- "${capture_channel_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report"
  cp -- "${capture_representation_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report.representation.lls"
  cp -- "${capture_result_source}" \
    "${frozen_input_dir}/capture.runtime.result.fact"
  cp -- "${representation_training_manifest_source}" \
    "${frozen_input_dir}/representation_training.job.manifest"
  cp -- "${representation_checkpoint_fact_source}" \
    "${frozen_input_dir}/representation_training.lattice.checkpoint.fact"
  cp -- "${mdn_training_manifest_source}" \
    "${frozen_input_dir}/mdn_training.job.manifest"
  cp -- "${mdn_checkpoint_fact_source}" \
    "${frozen_input_dir}/mdn_training.lattice.checkpoint.fact"
  {
    echo "schema_id=${schema_id}"
    echo "cached_probe_schema=kikijyeba.synthetic.mdn_edge_context_feature_probe.v1"
    echo "cached_probe_rows=6570"
    echo "cached_probe_anchor_range=[0,730)"
    echo "fit_anchor_range=[0,554)"
    echo "purge_anchor_range=[554,584)"
    echo "validation_anchor_range=[584,730)"
    echo "upstream_representation_fit_anchor_range=[0,730)"
    echo "upstream_mdn_fit_anchor_range=[0,730)"
    echo "validation_upstream_exposure=transductive"
    echo "historical_input_used=false"
    echo "holdout_input_used=false"
  } >"${runtime_dir}/input_contract.status"
  (
    cd "${runtime_dir}"
    sha256sum \
      input_contract.status \
      frozen_inputs/mdn_edge_context_features.probe \
      frozen_inputs/capture.job.manifest \
      frozen_inputs/capture.channel_inference.report \
      frozen_inputs/capture.channel_inference.report.representation.lls \
      frozen_inputs/capture.runtime.result.fact \
      frozen_inputs/representation_training.job.manifest \
      frozen_inputs/representation_training.lattice.checkpoint.fact \
      frozen_inputs/mdn_training.job.manifest \
      frozen_inputs/mdn_training.lattice.checkpoint.fact \
      >input_manifest.sha256
  )
}

freeze_sources() {
  mkdir "${frozen_source_dir}" "${frozen_contract_dir}"
  mkdir -p \
    "${frozen_include_root}/piaabo/digest" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn"
  cp -- "${helper_src}" "${frozen_source_dir}/${helper_name}"
  cp -- "${runner_src}" "${frozen_source_dir}/${runner_name}"
  cp -- "${digest_header}" \
    "${frozen_include_root}/piaabo/digest/sha256.h"
  cp -- "${head_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
  cp -- "${types_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
  cp -- "${utils_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
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
      frozen_include/piaabo/digest/sha256.h \
      frozen_include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h \
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
  if [[ "${SYNTHETIC_MDN_CACHED_INPUT_NORM_AB_CPU:-false}" == "true" ]]; then
    device_args+=(--cpu)
  fi

  "${helper_bin}" \
    --input "${cached_probe}" \
    --head-header "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h" \
    --output "${output}" \
    --schema-id "${schema_id}" \
    "${device_args[@]}" \
    >"${log}" 2>&1
  require_nonempty_file "${output}"
}

validate_report() {
  local report="$1"
  require_nonempty_file "${report}"
  expect_file_value "${report}" schema_id "${schema_id}"
  expect_file_value "${report}" status complete
  expect_file_value "${report}" scope.causal_variable \
    input_norm_forward_call_only
  expect_file_value "${report}" scope.identical_initial_parameter_bytes true
  expect_file_value "${report}" execution.identity_mode \
    edge_embedding_per_edge
  expect_file_value "${report}" execution.steps 3500
  expect_file_value "${report}" execution.batch_size 64
  expect_file_value "${report}" execution.objective_regression_weight \
    100.000000000000
  expect_file_value "${report}" execution.objective_direction_weight \
    5.000000000000
  expect_file_value "${report}" execution.objective_rank_weight \
    5.000000000000
  expect_file_value "${report}" execution.objective_huber_beta \
    0.500000000000
  expect_file_value "${report}" execution.objective_target_scale \
    36.000000000000
  expect_file_value "${report}" boundary.fit '[0,554)'
  expect_file_value "${report}" boundary.purge '[554,584)'
  expect_file_value "${report}" boundary.validation '[584,730)'
  expect_file_value "${report}" provenance.validation_upstream_exposure \
    transductive
  expect_file_value "${report}" provenance.historical_input_used false
  expect_file_value "${report}" provenance.holdout_input_used false
  expect_file_value "${report}" provenance.checkpoint_written false
  expect_file_value "${report}" classification.end_to_end_conclusion \
    not_established
}

require_identical_probes() {
  validate_report "${main_probe}"
  validate_report "${replay_probe}"
  cmp -s "${main_probe}" "${replay_probe}" ||
    fail "main/replay reports are not byte-identical"
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
      bin/mdn_cached_feature_input_norm_ablation \
      input_manifest.sha256 \
      source_manifest.sha256 \
      contract_manifest.sha256 \
      output_manifest.sha256 \
      >master.sha256
  )
}

verify_runtime_seals() {
  require_directory "${runtime_dir}"
  require_directory "${build_dir}"
  require_directory "${frozen_source_dir}"
  require_directory "${frozen_include_root}"
  require_directory "${frozen_contract_dir}"
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
  validate_capture_probe "${cached_probe}"
  require_identical_probes
}

verify_live_sources_match_frozen_runtime() {
  cmp -s "${helper_src}" "${frozen_source_dir}/${helper_name}" ||
    fail "live helper differs from sealed runtime"
  cmp -s "${runner_src}" "${frozen_source_dir}/${runner_name}" ||
    fail "live runner differs from sealed runtime"
  cmp -s "${digest_header}" \
    "${frozen_include_root}/piaabo/digest/sha256.h" ||
    fail "live digest header differs from sealed runtime"
  cmp -s "${head_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h" ||
    fail "live production head header differs from sealed runtime"
  cmp -s "${types_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_types.h" ||
    fail "live MDN types header differs from sealed runtime"
  cmp -s "${utils_header}" \
    "${frozen_include_root}/wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h" ||
    fail "live MDN utils header differs from sealed runtime"
  cmp -s "${benchmark_manifest}" \
    "${frozen_contract_dir}/synthetic_periodic_chart_manifest.v1.report" ||
    fail "live benchmark manifest differs from sealed runtime"
  cmp -s "${split_catalog}" \
    "${frozen_contract_dir}/ujcamei.source.splits.dsl" ||
    fail "live split catalog differs from sealed runtime"
  cmp -s "${retrieval_channels}" \
    "${frozen_contract_dir}/ujcamei.source.retrieval.channels.dsl" ||
    fail "live retrieval contract differs from sealed runtime"
  cmp -s "${source_registry}" \
    "${frozen_contract_dir}/ujcamei.source.registry.dsl" ||
    fail "live source registry differs from sealed runtime"
  cmp -s "${topology_graph}" \
    "${frozen_contract_dir}/kikijyeba.topology.graph.dsl" ||
    fail "live topology graph differs from sealed runtime"
  cmp -s "${nodelift_spec}" \
    "${frozen_contract_dir}/wikimyei.expression.nodelift.srl.dsl" ||
    fail "live NodeLift contract differs from sealed runtime"
  cmp -s "${capture_probe_source}" "${cached_probe}" ||
    fail "live cached feature probe differs from sealed runtime"
  cmp -s "${capture_job_manifest_source}" \
    "${frozen_input_dir}/capture.job.manifest" ||
    fail "live capture manifest differs from sealed runtime"
  cmp -s "${capture_channel_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report" ||
    fail "live capture report differs from sealed runtime"
  cmp -s "${capture_representation_report_source}" \
    "${frozen_input_dir}/capture.channel_inference.report.representation.lls" ||
    fail "live capture representation report differs from sealed runtime"
  cmp -s "${capture_result_source}" \
    "${frozen_input_dir}/capture.runtime.result.fact" ||
    fail "live capture result differs from sealed runtime"
  cmp -s "${representation_training_manifest_source}" \
    "${frozen_input_dir}/representation_training.job.manifest" ||
    fail "live representation training manifest differs from sealed runtime"
  cmp -s "${representation_checkpoint_fact_source}" \
    "${frozen_input_dir}/representation_training.lattice.checkpoint.fact" ||
    fail "live representation checkpoint fact differs from sealed runtime"
  cmp -s "${mdn_training_manifest_source}" \
    "${frozen_input_dir}/mdn_training.job.manifest" ||
    fail "live MDN training manifest differs from sealed runtime"
  cmp -s "${mdn_checkpoint_fact_source}" \
    "${frozen_input_dir}/mdn_training.lattice.checkpoint.fact" ||
    fail "live MDN checkpoint fact differs from sealed runtime"
}

install_report_no_clobber() {
  local temporary_report
  if [[ -e "${installed_report}" || -L "${installed_report}" ]]; then
    require_nonempty_file "${installed_report}"
    cmp -s "${main_probe}" "${installed_report}" ||
      fail "installed report differs from sealed main report"
    return
  fi
  temporary_report="$(mktemp "${artifacts_dir}/.${schema_id}.report.XXXXXXXX")"
  cp -- "${main_probe}" "${temporary_report}"
  chmod 0644 "${temporary_report}"
  cmp -s "${main_probe}" "${temporary_report}" ||
    fail "temporary report copy differs from main report"
  mv -T -n "${temporary_report}" "${installed_report}" ||
    fail "atomic no-clobber report installation failed"
  if [[ -e "${temporary_report}" || -L "${temporary_report}" ]]; then
    if [[ -f "${installed_report}" && ! -L "${installed_report}" ]] &&
      cmp -s "${main_probe}" "${installed_report}"; then
      rm -f -- "${temporary_report}"
    else
      fail "report installation postcondition failed"
    fi
  fi
  require_nonempty_file "${installed_report}"
  cmp -s "${main_probe}" "${installed_report}" ||
    fail "installed report differs from main report"
}

verify_complete() {
  set_runtime_paths "${final_runtime_dir}"
  verify_runtime_seals
  require_nonempty_file "${installed_report}"
  cmp -s "${main_probe}" "${installed_report}" ||
    fail "installed report differs from sealed main report"
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
    fail "atomic runtime installation failed; scratch preserved"
  [[ ! -e "${scratch_runtime}" && ! -L "${scratch_runtime}" &&
     -d "${final_runtime_dir}" && ! -L "${final_runtime_dir}" ]] ||
    fail "runtime installation postcondition failed"
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
