#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"
mdn_jkimyei="${repo_root}/src/config/wikimyei.inference.expected_value.mdn.jkimyei"
schema_id="${SYNTHETIC_MDN_OBJECTIVE_ABLATION_SCHEMA_ID:-synthetic_mdn_objective_readout_alignment_ablation.v1}"
output_probe="${SYNTHETIC_MDN_OBJECTIVE_ABLATION_OUTPUT_PROBE:-${artifacts_dir}/${schema_id}.probe}"
variants="${SYNTHETIC_MDN_OBJECTIVE_ABLATION_VARIANTS:-edge_dominant_post_nll_0}"

mkdir -p "${artifacts_dir}"

if [[ ! -f "${mdn_jkimyei}" ]]; then
  echo "missing MDN .jkimyei: ${mdn_jkimyei}" >&2
  exit 2
fi
if [[ ! -x "${script_dir}/run_mdn_probe_guided_diagnostic.sh" ]]; then
  echo "missing diagnostic runner: ${script_dir}/run_mdn_probe_guided_diagnostic.sh" >&2
  exit 2
fi

backup="$(mktemp)"
cp "${mdn_jkimyei}" "${backup}"
restore_config() {
  cp "${backup}" "${mdn_jkimyei}" || true
  rm -f "${backup}" || true
}
trap restore_config EXIT

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

set_training_key() {
  local key="$1"
  local value="$2"
  local tmp
  tmp="$(mktemp)"
  awk -v key="${key}" -v value="${value}" '
    BEGIN { replaced = 0 }
    $1 == key {
      sub(/=.*/, "= " value ";");
      replaced = 1;
    }
    { print }
    END { if (!replaced) exit 42 }
  ' "${mdn_jkimyei}" >"${tmp}" || {
    rm -f "${tmp}"
    echo "failed to set ${key}; key is not present in ${mdn_jkimyei}" >&2
    exit 3
  }
  mv "${tmp}" "${mdn_jkimyei}"
}

post_warmup_for_variant() {
  case "$1" in
  baseline_post_nll_1) echo "1.0" ;;
  edge_dominant_post_nll_0) echo "0.0" ;;
  edge_dominant_post_nll_0p1) echo "0.1" ;;
  *)
    echo "unknown objective/readout ablation variant: $1" >&2
    exit 4
    ;;
  esac
}

metric_from_probe() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

{
  echo "schema_id=${schema_id}"
  echo "created_at_epoch=$(date +%s)"
  echo "mdn_jkimyei=${mdn_jkimyei}"
  echo "variant_list=${variants}"
} >"${output_probe}"

variant_index=0
for variant in ${variants}; do
  post_warmup_nll="$(post_warmup_for_variant "${variant}")"
  cp "${backup}" "${mdn_jkimyei}"
  set_training_key "MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT" \
    "${post_warmup_nll}"

  variant_schema="${schema_id}.${variant}"
  variant_probe="${artifacts_dir}/${variant_schema}.probe"
  variant_raw_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${variant_schema}"

  SYNTHETIC_MDN_DIAGNOSTIC_SCHEMA_ID="${variant_schema}" \
    SYNTHETIC_MDN_DIAGNOSTIC_RAW_ROOT="${variant_raw_root}" \
    SYNTHETIC_MDN_DIAGNOSTIC_OUTPUT_PROBE="${variant_probe}" \
    "${script_dir}/run_mdn_probe_guided_diagnostic.sh"

  echo "" >>"${output_probe}"
  echo "variant.${variant_index}.id=${variant}" >>"${output_probe}"
  echo "variant.${variant_index}.post_warmup_nll_weight=${post_warmup_nll}" \
    >>"${output_probe}"
  echo "variant.${variant_index}.probe=${variant_probe}" >>"${output_probe}"
  echo "variant.${variant_index}.directional_accuracy=$(metric_from_probe diagnostic.directional_accuracy_last "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.margin_directional_accuracy=$(metric_from_probe diagnostic.margin_directional_accuracy_last "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.pairwise_rank_accuracy=$(metric_from_probe diagnostic.pairwise_rank_accuracy_last "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.margin_pairwise_rank_accuracy=$(metric_from_probe diagnostic.margin_pairwise_rank_accuracy_last "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.correlation=$(metric_from_probe diagnostic.correlation_last "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.scheduled_nll_weight_min=$(metric_from_probe diagnostic.direct_readout_scheduled_nll_weight_min "${variant_probe}")" \
    >>"${output_probe}"
  echo "variant.${variant_index}.scheduled_nll_weight_last=$(metric_from_probe diagnostic.direct_readout_scheduled_nll_weight_last "${variant_probe}")" \
    >>"${output_probe}"

  variant_index=$((variant_index + 1))
done

echo "" >>"${output_probe}"
echo "variant_count=${variant_index}" >>"${output_probe}"
echo "status=completed" >>"${output_probe}"

