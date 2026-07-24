#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

schema_id="${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_SCHEMA_ID:-synthetic_mdn_edge_identity_readout_comparator.v1}"
artifacts_dir="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1/artifacts"
build_dir="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}/bin"
helper_src="${script_dir}/mdn_edge_identity_readout_comparator.cpp"
helper_bin="${build_dir}/mdn_edge_identity_readout_comparator"
output_probe="${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_OUTPUT_PROBE:-${artifacts_dir}/${schema_id}.probe}"

input_probe="${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_INPUT_PROBE:-}"
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
  echo "missing mdn_edge_context_features.probe; set SYNTHETIC_MDN_EDGE_ID_COMPARATOR_INPUT_PROBE" >&2
  exit 1
fi

mkdir -p "${build_dir}" "${artifacts_dir}"

if [[ ! -x "${helper_bin}" || "${helper_src}" -nt "${helper_bin}" ]]; then
  g++ -std=c++20 -O2 -Wall -Wextra -pedantic "${helper_src}" -o "${helper_bin}"
fi

standardize_args=()
if [[ "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_STANDARDIZE:-true}" == "false" ]]; then
  standardize_args+=(--no-standardize)
fi

layer_norm_args=()
if [[ "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_SAMPLE_LAYER_NORM:-false}" == "true" ]]; then
  layer_norm_args+=(--sample-layer-norm)
fi

"${helper_bin}" \
  --input "${input_probe}" \
  --output "${output_probe}" \
  --schema-id "${schema_id}" \
  --fit-fraction "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_FIT_FRACTION:-0.70}" \
  --margin-eps "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_MARGIN_EPS:-0.001}" \
  --rank-margin-eps "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_RANK_MARGIN_EPS:-0.001}" \
  --ridge-lambda "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_RIDGE_LAMBDA:-1e-6}" \
  --max-features "${SYNTHETIC_MDN_EDGE_ID_COMPARATOR_MAX_FEATURES:-0}" \
  "${standardize_args[@]}" \
  "${layer_norm_args[@]}"

echo "${output_probe}"
