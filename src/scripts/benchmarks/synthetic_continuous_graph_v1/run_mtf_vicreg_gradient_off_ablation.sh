#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
runner_path="${script_dir}/run_mtf_vicreg_gradient_off_ablation.sh"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
synthetic_data_root="${benchmark_root}/data"
base_config="${benchmark_root}/synthetic_benchmark.config"
representation_jkimyei="${benchmark_root}/wikimyei.representation.mtf_jepa_mae_vicreg.vicreg_gradient_off_v1.jkimyei"
canonical_representation_jkimyei="${repo_root}/src/config/wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei"
training_spec_header="${repo_root}/src/include/jkimyei/api/training_spec.h"
model_header="${repo_root}/src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"
config_bundle_header="${repo_root}/src/include/kikijyeba/protocol/config_bundle.h"
launcher_header="${repo_root}/src/include/jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"
probe_sidecar_header="${repo_root}/src/include/hero/runtime_hero/runtime/job_events_probe_sidecar.h"
runtime_exec="${repo_root}/.build/exec/cuwacunu_exec"
capture_runner="${script_dir}/run_mdn_frozen_feature_capture.sh"
ridge_runner="${script_dir}/run_mdn_cached_feature_runtime_head_parity.sh"
ridge_source="${script_dir}/mdn_cached_feature_runtime_head_parity.cpp"
production_head="${repo_root}/src/include/wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
ridge_compiler_command="g++"

schema_id="${SYNTHETIC_MTF_VICREG_GRADIENT_OFF_SCHEMA_ID:-synthetic_mtf_vicreg_gradient_off_v1}"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema_id}"
capture_schema="${schema_id}.frozen_capture"
representation_ridge_schema="${schema_id}.representation_train_eval_ridge_gate"
mdn_ridge_schema="${schema_id}.mdn_train_eval_ridge_gate"

train_begin=0
train_end=730
eval_begin=760
eval_end=1088
sealed_holdout_begin=1088
representation_steps=3000
mdn_steps=3500

config_dir="${runtime_root}/config"
logs_dir="${runtime_root}/logs"
representation_job="${runtime_root}/stages/representation_train"
mdn_job="${runtime_root}/stages/mdn_train"
representation_config="${config_dir}/representation_train.config"
mdn_config="${config_dir}/mdn_train.config"
inputs_manifest="${runtime_root}/experiment.inputs"
inputs_manifest_seal="${inputs_manifest}.sha256"
result_report="${runtime_root}/${schema_id}.report"
result_report_seal="${result_report}.sha256"
hash_manifest="${runtime_root}/${schema_id}.sha256"

capture_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${capture_schema}"
train_capture_job="${capture_root}/anchor_${train_begin}_${train_end}"
eval_capture_job="${capture_root}/anchor_${eval_begin}_${eval_end}"
train_capture_runtime_log="${capture_root}/anchor_${train_begin}_${train_end}.log"
eval_capture_runtime_log="${capture_root}/anchor_${eval_begin}_${eval_end}.log"
train_representation_probe="${train_capture_job}/representation_edge_features.probe"
eval_representation_probe="${eval_capture_job}/representation_edge_features.probe"
train_mdn_probe="${train_capture_job}/mdn_edge_context_features.probe"
eval_mdn_probe="${eval_capture_job}/mdn_edge_context_features.probe"

representation_ridge_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${representation_ridge_schema}"
representation_ridge_probe="${representation_ridge_root}/${representation_ridge_schema}.probe"
mdn_ridge_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${mdn_ridge_schema}"
mdn_ridge_probe="${mdn_ridge_root}/${mdn_ridge_schema}.probe"

die() {
  echo "VICReg-gradient-off ablation: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" ]] || die "required file missing: $1"
}

stage_file() {
  [[ -f "$1" ]]
}

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print substr($0, index($0, "=") + 1); exit }' \
    "${file}" 2>/dev/null || true
}

expect_kv() {
  local file="$1"
  local key="$2"
  local expected="$3"
  local actual
  if ! grep -q "^${key}=" "${file}" 2>/dev/null; then
    echo "missing ${key} in ${file}" >&2
    return 1
  fi
  actual="$(kv "${key}" "${file}")"
  if [[ "${actual}" != "${expected}" ]]; then
    echo "${file}: expected ${key}=${expected}, got ${actual}" >&2
    return 1
  fi
}

is_finite_numeric() {
  local value="${1:-}"
  [[ "${value}" =~ ^[+-]?(([0-9]+([.][0-9]*)?)|([.][0-9]+))([eE][+-]?[0-9]+)?$ ]]
}

expect_finite_numeric() {
  local file="$1"
  local key="$2"
  local actual
  actual="$(kv "${key}" "${file}")"
  if ! is_finite_numeric "${actual}"; then
    echo "${file}: expected finite numeric ${key}, got ${actual}" >&2
    return 1
  fi
}

expect_numeric() {
  local file="$1"
  local key="$2"
  local expected="$3"
  local actual
  actual="$(kv "${key}" "${file}")"
  if ! is_finite_numeric "${actual}" || ! is_finite_numeric "${expected}" ||
      ! LC_ALL=C awk -v a="${actual}" -v e="${expected}" \
        'BEGIN { exit !((a + 0) == (e + 0)) }'; then
    echo "${file}: expected numeric ${key}=${expected}, got ${actual}" >&2
    return 1
  fi
}

expect_numeric_strictly_between() {
  local file="$1"
  local key="$2"
  local lower="$3"
  local upper="$4"
  local actual
  actual="$(kv "${key}" "${file}")"
  if ! is_finite_numeric "${actual}" ||
      ! LC_ALL=C awk -v a="${actual}" -v lo="${lower}" -v hi="${upper}" \
        'BEGIN { exit !(a > lo && a < hi) }'; then
    echo "${file}: expected ${key} strictly between ${lower} and ${upper}, got ${actual}" >&2
    return 1
  fi
}

expect_boolean() {
  local file="$1"
  local key="$2"
  local actual
  actual="$(kv "${key}" "${file}")"
  if [[ "${actual}" != "true" && "${actual}" != "false" ]]; then
    echo "${file}: expected boolean ${key}, got ${actual}" >&2
    return 1
  fi
}

expect_truthy() {
  local file="$1"
  local key="$2"
  local actual
  actual="$(kv "${key}" "${file}")"
  if [[ "${actual}" != "true" && "${actual}" != "1" ]]; then
    echo "${file}: expected truthy ${key}, got ${actual}" >&2
    return 1
  fi
}

expect_nonempty() {
  local file="$1"
  local key="$2"
  local actual
  actual="$(kv "${key}" "${file}")"
  if [[ -z "${actual}" ]]; then
    echo "${file}: expected nonempty ${key}" >&2
    return 1
  fi
}

sha256() {
  sha256sum "$1" | awk '{ print $1 }'
}

print_plan() {
  cat <<EOF
schema_id=${schema_id}
classification=vicreg_gradient_only
outer_input_augmentation=canonical_light_phase_safe_v2
hidden_vicreg_weak_views=canonical
vicreg_branch_execution=enabled
vicreg_gradient_coefficient=0.0
train_range=[${train_begin},${train_end})
purge_range=[${train_end},${eval_begin})
protected_eval_range=[${eval_begin},${eval_end})
sealed_holdout_range=[${sealed_holdout_begin},1170)
representation_steps=${representation_steps}
mdn_steps=${mdn_steps}
runtime_root=${runtime_root}

Stages:
  1. representation from scratch on [${train_begin},${train_end})
  2. MDN from scratch on [${train_begin},${train_end}) with the explicit representation checkpoint
  3. frozen feature captures on [${train_begin},${train_end}) and [${eval_begin},${eval_end})
  4. CPU float64 ridge gates on raw representation features and matched-MDN
     readout features, both with train-only alpha selection, fraction 0.20,
     purge 30

This script defaults to plan mode. Pass --preflight to parse and dry-run the
custom configuration in a separate zero-step job. Pass --execute to start a
fresh approximately 2.5-hour GPU workflow or reuse only fully verified,
completed stages. Partial training or capture directories are rejected and
are never resumed. No anchor at or above ${sealed_holdout_begin} is read.
EOF
}

requested_mode="${1:---plan}"
case "${1:---plan}" in
--plan)
  print_plan
  exit 0
  ;;
--preflight) ;;
--execute) ;;
*)
  echo "usage: $0 [--plan|--preflight|--execute]" >&2
  exit 2
  ;;
esac

[[ "${schema_id}" =~ ^[A-Za-z0-9._-]+$ ]] || die "invalid schema id: ${schema_id}"
((train_begin < train_end)) || die "invalid train range"
((eval_begin < eval_end)) || die "invalid eval range"
((train_end <= eval_begin)) || die "train and protected-eval ranges overlap"
((train_end <= sealed_holdout_begin)) || die "train range opens sealed holdout"
((eval_end <= sealed_holdout_begin)) || die "eval range opens sealed holdout"

require_file "${base_config}"
require_file "${representation_jkimyei}"
require_file "${canonical_representation_jkimyei}"
require_file "${runner_path}"
require_file "${training_spec_header}"
require_file "${model_header}"
require_file "${config_bundle_header}"
require_file "${launcher_header}"
require_file "${probe_sidecar_header}"
require_file "${runtime_exec}"
require_file "${capture_runner}"
require_file "${ridge_runner}"
require_file "${ridge_source}"
require_file "${production_head}"
[[ -d "${synthetic_data_root}" ]] || die "synthetic data root missing: ${synthetic_data_root}"
[[ -d "${libtorch_path}" ]] || die "libtorch root missing: ${libtorch_path}"
[[ -d "${cuda_path}" ]] || die "CUDA root missing: ${cuda_path}"
command -v "${ridge_compiler_command}" >/dev/null 2>&1 ||
  die "ridge compiler not found: ${ridge_compiler_command}"
command -v ldd >/dev/null 2>&1 || die "ldd is required for ridge provenance"
command -v readelf >/dev/null 2>&1 || die "readelf is required for ridge provenance"
ridge_compiler="$(readlink -f "$(command -v "${ridge_compiler_command}")")"
ridge_ldd_tool="$(readlink -f "$(command -v ldd)")"
ridge_readelf_tool="$(readlink -f "$(command -v readelf)")"
require_file "${ridge_compiler}"
require_file "${ridge_ldd_tool}"
require_file "${ridge_readelf_tool}"

validate_controlled_representation_spec() {
  local restored_control
  grep -qx '  LAMBDA_VICREG = 0.05;' "${canonical_representation_jkimyei}" ||
    die "canonical LAMBDA_VICREG drifted"
  grep -qx '  LAMBDA_VICREG = 0.0;' "${representation_jkimyei}" ||
    die "control does not zero LAMBDA_VICREG"
  grep -qx '  USE_VICREG_LOSS = true;' "${representation_jkimyei}" ||
    die "control disabled the VICReg branch instead of only its gradient"
  grep -qx '  USE_GLOBAL_VICREG = true;' "${representation_jkimyei}" ||
    die "control disabled global VICReg instead of only its gradient"
  grep -qx '  VICREG_VIEW_GAUSSIAN_JITTER_STD = 0.005;' \
    "${representation_jkimyei}" || die "control changed canonical hidden Gaussian weak views"
  grep -qx '  VICREG_VIEW_TIME_DROPOUT_SCALE = 0.10;' \
    "${representation_jkimyei}" || die "control changed canonical hidden time-drop weak views"
  restored_control="$(mktemp)"
  sed -e 's/LAMBDA_VICREG = 0\.0;/LAMBDA_VICREG = 0.05;/' \
    "${representation_jkimyei}" >"${restored_control}"
  if ! cmp -s "${canonical_representation_jkimyei}" "${restored_control}"; then
    diff -u "${canonical_representation_jkimyei}" "${restored_control}" >&2 || true
    rm -f "${restored_control}"
    die "custom representation spec differs from canonical beyond LAMBDA_VICREG"
  fi
  rm -f "${restored_control}"
}

validate_controlled_representation_spec

mkdir -p "${config_dir}" "${logs_dir}" "${runtime_root}/stages"
exec 9>"${runtime_root}/.execution.lock"
flock -n 9 || die "another process holds ${runtime_root}/.execution.lock"

write_config_snapshot() {
  local wave_id="$1"
  local destination="$2"
  local candidate
  candidate="$(mktemp "${config_dir}/.config.XXXXXX")"
  awk -v root="${benchmark_root}" -v wave="${wave_id}" \
      -v representation_spec="${representation_jkimyei}" '
    function trim(s) {
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", s);
      return s;
    }
    {
      eq = index($0, "=");
      if (eq == 0) {
        print;
        next;
      }
      key = trim(substr($0, 1, eq - 1));
      value = trim(substr($0, eq + 1));
      if (key == "runtime_wave_id") {
        print "    runtime_wave_id = " wave;
      } else if (key == "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path") {
        print "    " key " = " representation_spec;
      } else if (key ~ /_path$/ && value !~ /^\//) {
        print "    " key " = " root "/" value;
      } else {
        print;
      }
    }
  ' "${base_config}" >"${candidate}"

  if [[ -e "${destination}" ]]; then
    if ! cmp -s "${candidate}" "${destination}"; then
      rm -f "${candidate}"
      die "immutable config snapshot drifted: ${destination}"
    fi
    rm -f "${candidate}"
  else
    mv "${candidate}" "${destination}"
  fi
}

write_config_snapshot "train_core_mtf_jepa_mae_vicreg" "${representation_config}"
write_config_snapshot "train_core_channel_mdn" "${mdn_config}"

config_dependency_manifest() {
  awk -F= '
    /_path[[:space:]]*=/ {
      value = substr($0, index($0, "=") + 1)
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
      if (value ~ /^\//) print value
    }
  ' "${representation_config}" | sort -u | while IFS= read -r path; do
    require_file "${path}"
    sha256sum "${path}"
  done
}

synthetic_data_manifest() {
  find "${synthetic_data_root}" -type f -print0 | sort -z |
    xargs -0 sha256sum
}

seal_files() {
  local destination="$1"
  shift
  local candidate
  candidate="$(mktemp "$(dirname "${destination}")/.seal.XXXXXX")"
  local file
  for file in "$@"; do
    require_file "${file}"
    sha256sum "${file}"
  done >"${candidate}"
  if [[ -e "${destination}" ]]; then
    if ! cmp -s "${candidate}" "${destination}"; then
      rm -f "${candidate}"
      die "sealed stage artifacts drifted: ${destination}"
    fi
    rm -f "${candidate}"
  else
    mv "${candidate}" "${destination}"
  fi
}

verify_sha256_manifest() {
  local manifest="$1"
  require_file "${manifest}"
  sha256sum -c "${manifest}" >/dev/null ||
    die "SHA-256 manifest verification failed: ${manifest}"
}

install_immutable_candidate() {
  local candidate="$1"
  local destination="$2"
  local label="$3"
  if [[ -e "${destination}" ]]; then
    if ! cmp -s "${candidate}" "${destination}"; then
      rm -f "${candidate}"
      die "${label} drifted for existing schema: ${destination}"
    fi
    rm -f "${candidate}"
  else
    mv "${candidate}" "${destination}"
  fi
}

write_inputs_manifest() {
  local candidate
  candidate="$(mktemp "${runtime_root}/.inputs.XXXXXX")"
  {
    echo "schema_id=${schema_id}"
    echo "experiment=vicreg_gradient_off_v1"
    echo "outer_input_augmentation=canonical_light_phase_safe_v2"
    echo "hidden_vicreg_weak_views=canonical"
    echo "hidden_vicreg_weak_view_gaussian_jitter_std=0.005"
    echo "hidden_vicreg_weak_view_time_dropout_scale=0.10"
    echo "hidden_vicreg_weak_view_time_dropout_prob_effective=0.01"
    echo "use_vicreg_loss=true"
    echo "use_global_vicreg=true"
    echo "lambda_vicreg=0.0"
    echo "rng_schedule_policy=canonical_branch_and_draws_preserved"
    echo "train_anchor_range=[${train_begin},${train_end})"
    echo "protected_eval_anchor_range=[${eval_begin},${eval_end})"
    echo "sealed_holdout_begin=${sealed_holdout_begin}"
    echo "config_dependency_count=$(config_dependency_manifest | wc -l)"
    echo "config_dependency_manifest_sha256=$(config_dependency_manifest | sha256sum | awk '{ print $1 }')"
    echo "synthetic_data_file_count=$(find "${synthetic_data_root}" -type f | wc -l)"
    echo "synthetic_data_manifest_sha256=$(synthetic_data_manifest | sha256sum | awk '{ print $1 }')"
    echo "runner_sha256=$(sha256 "${runner_path}")"
    echo "base_config_sha256=$(sha256 "${base_config}")"
    echo "representation_jkimyei_sha256=$(sha256 "${representation_jkimyei}")"
    echo "canonical_representation_jkimyei_sha256=$(sha256 "${canonical_representation_jkimyei}")"
    echo "training_spec_header_sha256=$(sha256 "${training_spec_header}")"
    echo "model_header_sha256=$(sha256 "${model_header}")"
    echo "config_bundle_header_sha256=$(sha256 "${config_bundle_header}")"
    echo "launcher_header_sha256=$(sha256 "${launcher_header}")"
    echo "probe_sidecar_header_sha256=$(sha256 "${probe_sidecar_header}")"
    echo "representation_config_sha256=$(sha256 "${representation_config}")"
    echo "mdn_config_sha256=$(sha256 "${mdn_config}")"
    echo "runtime_exec_sha256=$(sha256 "${runtime_exec}")"
    echo "capture_runner_sha256=$(sha256 "${capture_runner}")"
    echo "ridge_runner_sha256=$(sha256 "${ridge_runner}")"
    echo "ridge_source_sha256=$(sha256 "${ridge_source}")"
    echo "production_head_sha256=$(sha256 "${production_head}")"
    echo "ridge_compiler_path=${ridge_compiler}"
    echo "ridge_compiler_sha256=$(sha256 "${ridge_compiler}")"
    echo "ridge_compiler_version_sha256=$("${ridge_compiler}" --version | sha256sum | awk '{ print $1 }')"
    echo "ridge_ldd_tool_path=${ridge_ldd_tool}"
    echo "ridge_ldd_tool_sha256=$(sha256 "${ridge_ldd_tool}")"
    echo "ridge_readelf_tool_path=${ridge_readelf_tool}"
    echo "ridge_readelf_tool_sha256=$(sha256 "${ridge_readelf_tool}")"
  } >"${candidate}"
  if [[ -e "${inputs_manifest}" ]]; then
    if ! cmp -s "${candidate}" "${inputs_manifest}"; then
      rm -f "${candidate}"
      die "experiment inputs changed; use a new schema id instead of mixing stages"
    fi
    rm -f "${candidate}"
  else
    mv "${candidate}" "${inputs_manifest}"
  fi
  seal_files "${inputs_manifest_seal}" "${inputs_manifest}"
  verify_sha256_manifest "${inputs_manifest_seal}"
}

write_inputs_manifest

next_log() {
  local label="$1"
  local index=0
  while [[ -e "${logs_dir}/${label}.attempt_${index}.log" ]]; do
    index=$((index + 1))
  done
  stage_log="${logs_dir}/${label}.attempt_${index}.log"
}

run_preflight() {
  local preflight_job="${runtime_root}/preflight/representation_dry_run"
  local result="${preflight_job}/runtime.result.fact"
  local manifest="${preflight_job}/job.manifest"
  if [[ ! -e "${preflight_job}" ]]; then
    mkdir -p "$(dirname "${preflight_job}")"
    next_log representation_preflight
    echo "starting zero-step representation preflight; log=${stage_log}"
    "${runtime_exec}" \
      --config "${representation_config}" \
      --job-dir "${preflight_job}" \
      --dry-run \
      --source-range anchor_index \
      --anchor-index-begin "${train_begin}" \
      --anchor-index-end "${train_end}" \
      --no-replay-artifacts >"${stage_log}" 2>&1 ||
      die "representation preflight failed; see ${stage_log}"
  fi
  if [[ ! -f "${result}" || ! -f "${manifest}" ]]; then
    die "existing preflight path is partial; automatic resume is unsupported: ${preflight_job}"
  fi
  require_file "${result}"
  require_file "${manifest}"
  expect_kv "${result}" status dry_run ||
    die "preflight result is not dry_run: ${result}"
  expect_kv "${result}" checkpoint_written false ||
    die "preflight unexpectedly wrote a checkpoint"
  expect_kv "${result}" model_state_mutated false ||
    die "preflight unexpectedly mutated model state"
  expect_numeric "${result}" optimizer_steps 0 ||
    die "preflight unexpectedly optimized model state"
  expect_nonempty "${result}" config_bundle_id ||
    die "preflight omitted config-bundle identity"
  expect_kv "${manifest}" config_path "${representation_config}" ||
    die "preflight config mismatch"
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg ||
    die "preflight wave mismatch"
  expect_kv "${manifest}" source_order_policy random_per_epoch ||
    die "preflight source order mismatch"
  expect_nonempty "${manifest}" protocol_contract_fingerprint ||
    die "preflight omitted protocol-contract identity"
  expect_numeric "${manifest}" resolved_anchor_index_begin "${train_begin}" ||
    die "preflight train begin mismatch"
  expect_numeric "${manifest}" resolved_anchor_index_end "${train_end}" ||
    die "preflight train end mismatch"
  seal_files "${preflight_job}/stage.sha256" "${result}" "${manifest}"
  verify_sha256_manifest "${preflight_job}/stage.sha256"
  echo "preflight complete: ${preflight_job}"
}

validate_representation_stage() {
  local result="${representation_job}/runtime.result.fact"
  local manifest="${representation_job}/job.manifest"
  local report="${representation_job}/channel_representation.report"
  stage_file "${result}" || return 1
  stage_file "${manifest}" || return 1
  stage_file "${report}" || return 1
  expect_kv "${result}" status completed || return 1
  expect_kv "${result}" checkpoint_written true || return 1
  expect_kv "${result}" model_state_mutated true || return 1
  expect_truthy "${result}" finite_parameter_check || return 1
  expect_numeric "${result}" nonfinite_output_count 0 || return 1
  expect_numeric "${result}" optimizer_steps "${representation_steps}" || return 1
  expect_kv "${manifest}" config_path "${representation_config}" || return 1
  expect_kv "${manifest}" wave_id train_core_mtf_jepa_mae_vicreg || return 1
  expect_kv "${manifest}" wave_action train || return 1
  expect_kv "${manifest}" execution_chain \
    "ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train" || return 1
  expect_kv "${manifest}" mutated_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg || return 1
  expect_kv "${manifest}" frozen_components "" || return 1
  expect_kv "${manifest}" source_order_policy random_per_epoch || return 1
  expect_numeric "${manifest}" resolved_anchor_index_begin "${train_begin}" || return 1
  expect_numeric "${manifest}" resolved_anchor_index_end "${train_end}" || return 1
  expect_kv "${manifest}" input_representation_checkpoint_path "" || return 1
  expect_kv "${manifest}" input_mdn_checkpoint_path "" || return 1
  expect_kv "${report}" augmentation_profile light_phase_safe_v2 || return 1
  expect_numeric "${report}" seed 17 || return 1
  expect_numeric "${report}" dropout 0 || return 1
  expect_numeric "${report}" mask_ratio_time 0.10 || return 1
  expect_numeric "${report}" mask_ratio_frequency 0.05 || return 1
  expect_numeric "${report}" mask_ratio_channel 0 || return 1
  expect_numeric "${report}" min_context_ratio 0.75 || return 1
  expect_numeric "${report}" vicreg_view_gaussian_jitter_std 0.005 || return 1
  expect_numeric "${report}" vicreg_view_time_dropout_scale 0.10 || return 1
  expect_numeric "${report}" vicreg_view_time_dropout_prob_effective 0.01 || return 1
  expect_kv "${report}" use_vicreg_loss true || return 1
  expect_kv "${report}" use_global_vicreg true || return 1
  expect_numeric "${report}" lambda_vicreg 0 || return 1
  expect_numeric "${report}" lambda_global_vicreg 0.25 || return 1
  expect_numeric "${report}" gaussian_jitter_std 0.001 || return 1
  expect_numeric "${report}" feature_dropout_prob 0 || return 1
  expect_numeric "${report}" history_dropout_prob 0 || return 1
  expect_numeric "${report}" time_crop_jitter_max 0 || return 1
  expect_numeric "${report}" time_dilation_min 0.98 || return 1
  expect_numeric "${report}" time_dilation_max 1.02 || return 1
  expect_numeric "${report}" time_warp_max 0.01 || return 1
  expect_numeric "${report}" amplitude_scale_min 0.98 || return 1
  expect_numeric "${report}" amplitude_scale_max 1.02 || return 1
  expect_numeric "${report}" amplitude_shift_std 0 || return 1
  expect_numeric "${report}" frequency_mask_ratio 0.02 || return 1
  expect_numeric "${report}" frequency_jitter_std 0.01 || return 1
  expect_numeric "${report}" phase_jitter_max 0 || return 1
  expect_numeric "${report}" channel_dropout_prob 0 || return 1
  expect_numeric "${report}" cross_channel_dropout_prob 0 || return 1
  expect_numeric "${report}" node_dropout_prob 0 || return 1
  expect_numeric "${report}" edge_dropout_prob 0 || return 1
  expect_numeric "${report}" magnitude_normalization_noise_std 0 || return 1
  expect_numeric_strictly_between "${report}" \
    augmented_feature_retention_fraction 0 1 || return 1
  expect_finite_numeric "${report}" max_grad_norm || return 1
  representation_checkpoint="$(kv checkpoint_path "${result}")"
  stage_file "${representation_checkpoint}" || return 1
}

validate_mdn_stage() {
  local result="${mdn_job}/runtime.result.fact"
  local manifest="${mdn_job}/job.manifest"
  local report="${mdn_job}/channel_inference.report"
  stage_file "${result}" || return 1
  stage_file "${manifest}" || return 1
  stage_file "${report}" || return 1
  expect_kv "${result}" status completed || return 1
  expect_kv "${result}" checkpoint_written true || return 1
  expect_kv "${result}" model_state_mutated true || return 1
  expect_truthy "${result}" finite_parameter_check || return 1
  expect_numeric "${result}" nonfinite_output_count 0 || return 1
  expect_numeric "${result}" optimizer_steps "${mdn_steps}" || return 1
  expect_kv "${manifest}" config_path "${mdn_config}" || return 1
  expect_kv "${manifest}" wave_id train_core_channel_mdn || return 1
  expect_kv "${manifest}" wave_action train || return 1
  expect_kv "${manifest}" execution_chain \
    "ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:train" || return 1
  expect_kv "${manifest}" mutated_components \
    wikimyei.inference.expected_value.mdn || return 1
  expect_kv "${manifest}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg || return 1
  expect_kv "${manifest}" source_order_policy random_per_epoch || return 1
  expect_numeric "${manifest}" resolved_anchor_index_begin "${train_begin}" || return 1
  expect_numeric "${manifest}" resolved_anchor_index_end "${train_end}" || return 1
  expect_kv "${manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint}" || return 1
  expect_kv "${manifest}" input_mdn_checkpoint_path "" || return 1
  expect_kv "${report}" representation_checkpoint_loaded true || return 1
  expect_kv "${report}" representation_checkpoint_path \
    "${representation_checkpoint}" || return 1
  expect_kv "${report}" mdn_checkpoint_loaded false || return 1
  mdn_checkpoint="$(kv checkpoint_path "${result}")"
  stage_file "${mdn_checkpoint}" || return 1
}

run_representation_stage() {
  write_inputs_manifest
  if [[ -e "${representation_job}" ]]; then
    validate_representation_stage ||
      die "existing representation stage is partial or invalid; automatic resume is unsupported: ${representation_job}"
    seal_files "${representation_job}/stage.sha256" \
      "${representation_job}/runtime.result.fact" \
      "${representation_job}/job.manifest" \
      "${representation_job}/channel_representation.report" \
      "${representation_checkpoint}"
    verify_sha256_manifest "${representation_job}/stage.sha256"
    echo "reusing verified representation stage: ${representation_job}"
    return
  fi
  next_log representation_train
  echo "starting representation stage; log=${stage_log}"
  {
    echo "config=${representation_config}"
    echo "config_sha256=$(sha256 "${representation_config}")"
    "${runtime_exec}" \
      --config "${representation_config}" \
      --job-dir "${representation_job}" \
      --source-range anchor_index \
      --anchor-index-begin "${train_begin}" \
      --anchor-index-end "${train_end}" \
      --no-replay-artifacts
  } >"${stage_log}" 2>&1 || die "representation stage failed; see ${stage_log}"
  validate_representation_stage || die "representation stage failed post-run verification"
  seal_files "${representation_job}/stage.sha256" \
    "${representation_job}/runtime.result.fact" \
    "${representation_job}/job.manifest" \
    "${representation_job}/channel_representation.report" \
    "${representation_checkpoint}"
  verify_sha256_manifest "${representation_job}/stage.sha256"
}

run_mdn_stage() {
  write_inputs_manifest
  if [[ -e "${mdn_job}" ]]; then
    validate_mdn_stage ||
      die "existing MDN stage is partial or invalid; automatic resume is unsupported: ${mdn_job}"
    seal_files "${mdn_job}/stage.sha256" \
      "${mdn_job}/runtime.result.fact" "${mdn_job}/job.manifest" \
      "${mdn_job}/channel_inference.report" "${mdn_checkpoint}"
    verify_sha256_manifest "${mdn_job}/stage.sha256"
    echo "reusing verified MDN stage: ${mdn_job}"
    return
  fi
  next_log mdn_train
  echo "starting MDN stage; log=${stage_log}"
  {
    echo "config=${mdn_config}"
    echo "config_sha256=$(sha256 "${mdn_config}")"
    echo "representation_checkpoint=${representation_checkpoint}"
    echo "representation_checkpoint_sha256=$(sha256 "${representation_checkpoint}")"
    "${runtime_exec}" \
      --config "${mdn_config}" \
      --job-dir "${mdn_job}" \
      --source-range anchor_index \
      --anchor-index-begin "${train_begin}" \
      --anchor-index-end "${train_end}" \
      --input-representation-checkpoint "${representation_checkpoint}" \
      --no-replay-artifacts
  } >"${stage_log}" 2>&1 || die "MDN stage failed; see ${stage_log}"
  validate_mdn_stage || die "MDN stage failed post-run verification"
  seal_files "${mdn_job}/stage.sha256" \
    "${mdn_job}/runtime.result.fact" "${mdn_job}/job.manifest" \
    "${mdn_job}/channel_inference.report" "${mdn_checkpoint}"
  verify_sha256_manifest "${mdn_job}/stage.sha256"
}

validate_capture() {
  local begin="$1"
  local end="$2"
  local job="$3"
  local result="${job}/runtime.result.fact"
  local manifest="${job}/job.manifest"
  local capture_config="${job}.config"
  local inference_report="${job}/channel_inference.report"
  local representation_probe="${job}/representation_edge_features.probe"
  local mdn_probe="${job}/mdn_edge_context_features.probe"
  local expected_rows=$(((end - begin) * 9))
  local actual_rows
  local representation_anchor_bounds
  local mdn_anchor_bounds
  ((end <= sealed_holdout_begin)) || return 1
  stage_file "${result}" || return 1
  stage_file "${manifest}" || return 1
  stage_file "${capture_config}" || return 1
  stage_file "${inference_report}" || return 1
  stage_file "${representation_probe}" || return 1
  stage_file "${mdn_probe}" || return 1
  expect_kv "${result}" status completed || return 1
  expect_kv "${result}" checkpoint_written false || return 1
  expect_kv "${result}" model_state_mutated false || return 1
  expect_truthy "${result}" finite_parameter_check || return 1
  expect_numeric "${result}" nonfinite_output_count 0 || return 1
  expect_numeric "${result}" optimizer_steps 0 || return 1
  expect_kv "${manifest}" config_path "${capture_config}" || return 1
  expect_kv "${manifest}" wave_id cwu_02v_certified_replay_eval_mdn || return 1
  expect_kv "${manifest}" wave_action run || return 1
  expect_kv "${manifest}" execution_chain \
    "ujcamei.source.registry:run -> wikimyei.expression.nodelift.srl:run -> wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen -> wikimyei.inference.expected_value.mdn:run" || return 1
  expect_kv "${manifest}" mutated_components "" || return 1
  expect_kv "${manifest}" frozen_components \
    wikimyei.representation.encoding.mtf_jepa_mae_vicreg || return 1
  expect_kv "${manifest}" source_order_policy sequential || return 1
  expect_numeric "${manifest}" resolved_anchor_index_begin "${begin}" || return 1
  expect_numeric "${manifest}" resolved_anchor_index_end "${end}" || return 1
  expect_kv "${manifest}" input_representation_checkpoint_path \
    "${representation_checkpoint}" || return 1
  expect_kv "${manifest}" input_mdn_checkpoint_path "${mdn_checkpoint}" || return 1
  expect_kv "${inference_report}" representation_checkpoint_loaded true || return 1
  expect_kv "${inference_report}" representation_checkpoint_path \
    "${representation_checkpoint}" || return 1
  expect_kv "${inference_report}" mdn_checkpoint_loaded true || return 1
  expect_kv "${inference_report}" mdn_checkpoint_path "${mdn_checkpoint}" || return 1
  actual_rows="$(awk 'NR > 1 { ++n } END { print n + 0 }' "${representation_probe}")"
  [[ "${actual_rows}" == "${expected_rows}" ]] || return 1
  representation_anchor_bounds="$(awk -F, 'NR > 1 {
    numeric = "^[+-]?(([0-9]+([.][0-9]*)?)|([.][0-9]+))([eE][+-]?[0-9]+)?$";
    if ($3 !~ numeric || $11 !~ numeric) bad = 1;
    if (!seen || $3 < lo) lo = $3;
    if (!seen || $3 > hi) hi = $3;
    seen = 1;
    if ($11 != 96) bad = 1;
  } END {
    if (!seen || bad) exit 1;
    print lo ":" hi;
  }' "${representation_probe}")" || return 1
  [[ "${representation_anchor_bounds}" == "${begin}:$((end - 1))" ]] || return 1
  actual_rows="$(awk 'NR > 1 { ++n } END { print n + 0 }' "${mdn_probe}")"
  [[ "${actual_rows}" == "${expected_rows}" ]] || return 1
  mdn_anchor_bounds="$(awk -F, 'NR > 1 {
    numeric = "^[+-]?(([0-9]+([.][0-9]*)?)|([.][0-9]+))([eE][+-]?[0-9]+)?$";
    if ($3 !~ numeric || $11 !~ numeric) bad = 1;
    if (!seen || $3 < lo) lo = $3;
    if (!seen || $3 > hi) hi = $3;
    seen = 1;
    if ($11 != 400) bad = 1;
  } END {
    if (!seen || bad) exit 1;
    print lo ":" hi;
  }' "${mdn_probe}")" || return 1
  [[ "${mdn_anchor_bounds}" == "${begin}:$((end - 1))" ]]
}

run_capture() {
  local begin="$1"
  local end="$2"
  local job="$3"
  local label="$4"
  write_inputs_manifest
  if [[ -e "${job}" ]]; then
    validate_capture "${begin}" "${end}" "${job}" ||
      die "existing capture is partial or invalid; automatic resume is unsupported: ${job}"
    seal_files "${job}/stage.sha256" "${job}.config" \
      "${job}/runtime.result.fact" "${job}/job.manifest" \
      "${job}/channel_inference.report" \
      "${job}/representation_edge_features.probe" \
      "${job}/mdn_edge_context_features.probe" \
      "${capture_root}/anchor_${begin}_${end}.log"
    verify_sha256_manifest "${job}/stage.sha256"
    echo "reusing verified capture: ${job}"
    return
  fi
  ((end <= sealed_holdout_begin)) || die "capture would open sealed holdout"
  next_log "${label}"
  echo "starting frozen capture [${begin},${end}); log=${stage_log}"
  SYNTHETIC_MDN_FROZEN_CAPTURE_SCHEMA_ID="${capture_schema}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_BASE_CONFIG="${representation_config}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_JOB_DIR="${job}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_CONFIG_PATH="${job}.config" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_BEGIN="${begin}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_ANCHOR_END="${end}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_REPRESENTATION_CHECKPOINT="${representation_checkpoint}" \
    SYNTHETIC_MDN_FROZEN_CAPTURE_MDN_CHECKPOINT="${mdn_checkpoint}" \
    "${capture_runner}" >"${stage_log}" 2>&1 ||
    die "frozen capture failed; see ${stage_log}"
  validate_capture "${begin}" "${end}" "${job}" ||
    die "frozen capture failed post-run verification: ${job}"
  seal_files "${job}/stage.sha256" "${job}.config" \
    "${job}/runtime.result.fact" "${job}/job.manifest" \
    "${job}/channel_inference.report" \
    "${job}/representation_edge_features.probe" \
    "${job}/mdn_edge_context_features.probe" \
    "${capture_root}/anchor_${begin}_${end}.log"
  verify_sha256_manifest "${job}/stage.sha256"
}

prepare_ridge_helper_environment() {
  local schema="$1"
  local ridge_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/${schema}"
  local build_dir="${ridge_root}/bin"
  local helper_bin="${build_dir}/mdn_cached_feature_runtime_head_parity"
  local compiler_version="${ridge_root}/ridge_helper.compiler.version"
  local linker_version="${ridge_root}/ridge_helper.linker.version"
  local normalized_ldd="${ridge_root}/ridge_helper.ldd.normalized"
  local dynamic_section="${ridge_root}/ridge_helper.dynamic_section"
  local linked_libraries="${ridge_root}/ridge_helper.linked_libraries.sha256"
  local provenance="${ridge_root}/ridge_helper.build.provenance"
  local helper_seal="${ridge_root}/ridge_helper.execution_inputs.sha256"
  local linker
  local candidate

  mkdir -p "${build_dir}"
  if [[ ! -x "${helper_bin}" || "${ridge_source}" -nt "${helper_bin}" ||
        "${production_head}" -nt "${helper_bin}" ]]; then
    "${ridge_compiler}" -std=c++20 -O0 -g0 -Wall -Wextra -fPIC \
      -I"${repo_root}/src" \
      -I"${repo_root}/src/include" \
      -I"${repo_root}/src/include/torch_compat" \
      -isystem "${libtorch_path}/include" \
      -isystem "${libtorch_path}/include/torch/csrc/api/include" \
      -isystem "${cuda_path}/include" \
      "${ridge_source}" \
      -L"${libtorch_path}/lib" \
      -L"${cuda_path}/lib64" \
      -Wl,-rpath,"${libtorch_path}/lib" \
      -Wl,-rpath,"${cuda_path}/lib64" \
      -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
      -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
      -lstdc++ -lpthread -lm \
      -o "${helper_bin}"
  fi
  [[ -x "${helper_bin}" ]] || die "ridge helper build did not produce an executable: ${helper_bin}"
  [[ ! "${ridge_source}" -nt "${helper_bin}" ]] ||
    die "ridge helper is older than its source after build: ${helper_bin}"
  [[ ! "${production_head}" -nt "${helper_bin}" ]] ||
    die "ridge helper is older than its production-head header after build: ${helper_bin}"

  linker="$("${ridge_compiler}" -print-prog-name=ld)"
  if [[ "${linker}" != /* ]]; then
    linker="$(command -v "${linker}")"
  fi
  linker="$(readlink -f "${linker}")"
  require_file "${linker}"

  candidate="$(mktemp "${ridge_root}/.compiler-version.XXXXXX")"
  LC_ALL=C "${ridge_compiler}" --version >"${candidate}"
  install_immutable_candidate "${candidate}" "${compiler_version}" \
    "ridge compiler version"

  candidate="$(mktemp "${ridge_root}/.linker-version.XXXXXX")"
  LC_ALL=C "${linker}" --version >"${candidate}"
  install_immutable_candidate "${candidate}" "${linker_version}" \
    "ridge linker version"

  candidate="$(mktemp "${ridge_root}/.ldd.XXXXXX")"
  LC_ALL=C "${ridge_ldd_tool}" "${helper_bin}" |
    sed -E 's/[[:space:]]+\(0x[[:xdigit:]]+\)$//' >"${candidate}"
  if grep -q '=> not found$' "${candidate}"; then
    rm -f "${candidate}"
    die "ridge helper has unresolved linked libraries: ${helper_bin}"
  fi
  install_immutable_candidate "${candidate}" "${normalized_ldd}" \
    "normalized ridge-helper ldd"

  candidate="$(mktemp "${ridge_root}/.dynamic.XXXXXX")"
  LC_ALL=C "${ridge_readelf_tool}" -d "${helper_bin}" >"${candidate}"
  install_immutable_candidate "${candidate}" "${dynamic_section}" \
    "ridge-helper dynamic section"

  candidate="$(mktemp "${ridge_root}/.linked-libraries.XXXXXX")"
  awk '
    $2 == "=>" && $3 ~ /^\// { print $3 }
    $1 ~ /^\// { print $1 }
  ' "${normalized_ldd}" | sort -u | while IFS= read -r library; do
    library="$(readlink -f "${library}")"
    require_file "${library}"
    sha256sum "${library}"
  done >"${candidate}"
  [[ -s "${candidate}" ]] || {
    rm -f "${candidate}"
    die "ridge-helper linked-library manifest is empty"
  }
  install_immutable_candidate "${candidate}" "${linked_libraries}" \
    "ridge-helper linked-library manifest"
  verify_sha256_manifest "${linked_libraries}"

  candidate="$(mktemp "${ridge_root}/.build-provenance.XXXXXX")"
  {
    echo "schema_id=${schema}"
    echo "helper_binary=${helper_bin}"
    echo "helper_binary_sha256=$(sha256 "${helper_bin}")"
    echo "helper_source_sha256=$(sha256 "${ridge_source}")"
    echo "production_head_sha256=$(sha256 "${production_head}")"
    echo "ridge_runner_sha256=$(sha256 "${ridge_runner}")"
    echo "compiler=${ridge_compiler}"
    echo "compiler_sha256=$(sha256 "${ridge_compiler}")"
    echo "compiler_version_sha256=$(sha256 "${compiler_version}")"
    echo "linker=${linker}"
    echo "linker_sha256=$(sha256 "${linker}")"
    echo "linker_version_sha256=$(sha256 "${linker_version}")"
    echo "ldd_tool=${ridge_ldd_tool}"
    echo "ldd_tool_sha256=$(sha256 "${ridge_ldd_tool}")"
    echo "readelf_tool=${ridge_readelf_tool}"
    echo "readelf_tool_sha256=$(sha256 "${ridge_readelf_tool}")"
    echo "normalized_ldd_sha256=$(sha256 "${normalized_ldd}")"
    echo "dynamic_section_sha256=$(sha256 "${dynamic_section}")"
    echo "linked_libraries_manifest_sha256=$(sha256 "${linked_libraries}")"
    echo "compile_contract=runner_exact_cxx20_o0_g0_libtorch_cuda"
  } >"${candidate}"
  install_immutable_candidate "${candidate}" "${provenance}" \
    "ridge-helper build provenance"

  seal_files "${helper_seal}" "${helper_bin}" "${ridge_source}" \
    "${production_head}" "${ridge_runner}" "${ridge_compiler}" \
    "${compiler_version}" "${linker}" "${linker_version}" \
    "${ridge_ldd_tool}" "${ridge_readelf_tool}" "${normalized_ldd}" \
    "${dynamic_section}" "${linked_libraries}" "${provenance}" \
    "${inputs_manifest}"
  verify_sha256_manifest "${helper_seal}"
  verify_sha256_manifest "${linked_libraries}"

  prepared_ridge_helper_bin="${helper_bin}"
  prepared_ridge_helper_provenance="${provenance}"
  prepared_ridge_helper_seal="${helper_seal}"
  prepared_ridge_linked_libraries="${linked_libraries}"
}

validate_ridge() {
  local schema="$1"
  local output_probe="$2"
  local input_probe="$3"
  local evaluation_probe="$4"
  local feature_dim="$5"
  local source_feature_count
  local dynamic_feature_count
  local ignored_identity_count
  if [[ "${feature_dim}" == "32" ]]; then
    source_feature_count=96
    dynamic_feature_count=96
    ignored_identity_count=0
  else
    source_feature_count=400
    dynamic_feature_count=384
    ignored_identity_count=16
  fi
  stage_file "${output_probe}" || return 1
  expect_kv "${output_probe}" schema_id "${schema}" || return 1
  expect_kv "${output_probe}" input_probe "${input_probe}" || return 1
  expect_kv "${output_probe}" evaluation_input_probe "${evaluation_probe}" || return 1
  expect_kv "${output_probe}" device cpu || return 1
  expect_kv "${output_probe}" ridge_only true || return 1
  expect_numeric "${output_probe}" feature_dim "${feature_dim}" || return 1
  expect_numeric "${output_probe}" anchor_count 1058 || return 1
  expect_numeric "${output_probe}" primary_anchor_count 730 || return 1
  expect_numeric "${output_probe}" anchor_min 0 || return 1
  expect_numeric "${output_probe}" anchor_max 1087 || return 1
  expect_numeric "${output_probe}" edge_count 3 || return 1
  expect_numeric "${output_probe}" channel_count 3 || return 1
  expect_numeric "${output_probe}" source_feature_count "${source_feature_count}" || return 1
  expect_numeric "${output_probe}" dynamic_feature_count "${dynamic_feature_count}" || return 1
  expect_numeric "${output_probe}" ignored_cached_identity_feature_count \
    "${ignored_identity_count}" || return 1
  expect_numeric "${output_probe}" reconstruction_max_abs_error 0 || return 1
  expect_numeric "${output_probe}" quote_consistency_max_abs_error 0 || return 1
  expect_numeric "${output_probe}" ridge_validation_fraction 0.20 || return 1
  expect_numeric "${output_probe}" ridge_validation_gap 30 || return 1
  expect_numeric "${output_probe}" ridge_direction_gate 0.95 || return 1
  expect_numeric "${output_probe}" ridge_rank_gate 0.95 || return 1
  expect_kv "${output_probe}" ridge.selection_train_anchor_range "[0,554)" || return 1
  expect_kv "${output_probe}" ridge.validation_purge_anchor_range "[554,584)" || return 1
  expect_kv "${output_probe}" ridge.validation_anchor_range "[584,730)" || return 1
  expect_kv "${output_probe}" ridge.diagnostic_confirmation_anchor_range \
    "[760,1088)" || return 1
  expect_boolean "${output_probe}" ridge.selection_gate_pass || return 1
  expect_finite_numeric "${output_probe}" ridge.selected_alpha || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.selected.validation.directional_accuracy || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.selected.validation.pairwise_rank_accuracy || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.selected.validation.rmse || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.refit.diagnostic_confirmation.directional_accuracy || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.refit.diagnostic_confirmation.pairwise_rank_accuracy || return 1
  expect_finite_numeric "${output_probe}" \
    ridge.refit.diagnostic_confirmation.rmse || return 1
}

run_ridge() {
  local label="$1"
  local schema="$2"
  local output_probe="$3"
  local input_probe="$4"
  local evaluation_probe="$5"
  local feature_dim="$6"
  write_inputs_manifest
  prepare_ridge_helper_environment "${schema}"
  if [[ -e "${output_probe}" ]]; then
    validate_ridge "${schema}" "${output_probe}" "${input_probe}" \
      "${evaluation_probe}" "${feature_dim}" ||
      die "existing ridge output is partial or invalid; automatic resume is unsupported: ${output_probe}"
    seal_files "${output_probe}.sha256" "${output_probe}" \
      "${prepared_ridge_helper_bin}" "${prepared_ridge_helper_provenance}" \
      "${prepared_ridge_helper_seal}" "${prepared_ridge_linked_libraries}"
    verify_sha256_manifest "${output_probe}.sha256"
    echo "reusing verified ridge gate: ${output_probe}"
    return
  fi
  next_log "${label}"
  echo "starting ${label}; log=${stage_log}"
  verify_sha256_manifest "${prepared_ridge_helper_seal}"
  verify_sha256_manifest "${prepared_ridge_linked_libraries}"
  SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_SCHEMA_ID="${schema}" \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_OUTPUT_PROBE="${output_probe}" \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_INPUT_PROBE="${input_probe}" \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_EVALUATION_INPUT_PROBE="${evaluation_probe}" \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_FEATURE_DIM="${feature_dim}" \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_ONLY=true \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_CPU=true \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_VALIDATION_FRACTION=0.20 \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_VALIDATION_GAP=30 \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_DIRECTION_GATE=0.95 \
    SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_RANK_GATE=0.95 \
    "${ridge_runner}" >"${stage_log}" 2>&1 || die "${label} failed; see ${stage_log}"
  validate_ridge "${schema}" "${output_probe}" "${input_probe}" \
    "${evaluation_probe}" "${feature_dim}" ||
    die "${label} failed post-run verification"
  seal_files "${output_probe}.sha256" "${output_probe}" \
    "${prepared_ridge_helper_bin}" "${prepared_ridge_helper_provenance}" \
    "${prepared_ridge_helper_seal}" "${prepared_ridge_linked_libraries}"
  verify_sha256_manifest "${output_probe}.sha256"
}

if [[ "${requested_mode}" == "--preflight" ]]; then
  run_preflight
  exit 0
fi

run_representation_stage
run_mdn_stage
run_capture "${train_begin}" "${train_end}" "${train_capture_job}" capture_train
run_capture "${eval_begin}" "${eval_end}" "${eval_capture_job}" capture_eval
require_file "${train_capture_runtime_log}"
require_file "${eval_capture_runtime_log}"
run_ridge representation_ridge_gate "${representation_ridge_schema}" \
  "${representation_ridge_probe}" "${train_representation_probe}" \
  "${eval_representation_probe}" 32
run_ridge mdn_ridge_gate "${mdn_ridge_schema}" "${mdn_ridge_probe}" \
  "${train_mdn_probe}" "${eval_mdn_probe}" 128
write_inputs_manifest
verify_sha256_manifest "${inputs_manifest_seal}"
verify_sha256_manifest "${representation_job}/stage.sha256"
verify_sha256_manifest "${mdn_job}/stage.sha256"
verify_sha256_manifest "${train_capture_job}/stage.sha256"
verify_sha256_manifest "${eval_capture_job}/stage.sha256"
verify_sha256_manifest "${representation_ridge_probe}.sha256"
verify_sha256_manifest "${mdn_ridge_probe}.sha256"
verify_sha256_manifest "${representation_ridge_root}/ridge_helper.execution_inputs.sha256"
verify_sha256_manifest "${mdn_ridge_root}/ridge_helper.execution_inputs.sha256"
verify_sha256_manifest "${representation_ridge_root}/ridge_helper.linked_libraries.sha256"
verify_sha256_manifest "${mdn_ridge_root}/ridge_helper.linked_libraries.sha256"

hash_candidate="$(mktemp "${runtime_root}/.hashes.XXXXXX")"
{
  sha256sum "${runner_path}" "${base_config}" "${representation_jkimyei}" \
    "${canonical_representation_jkimyei}" "${training_spec_header}" \
    "${model_header}" "${config_bundle_header}" "${launcher_header}" \
    "${probe_sidecar_header}" "${representation_config}" "${mdn_config}" \
    "${runtime_exec}" "${capture_runner}" "${ridge_runner}" \
    "${ridge_source}" "${production_head}" "${inputs_manifest}" \
    "${inputs_manifest_seal}" \
    "${representation_checkpoint}" "${mdn_checkpoint}" \
    "${train_representation_probe}" "${eval_representation_probe}" \
    "${train_mdn_probe}" "${eval_mdn_probe}" \
    "${train_capture_runtime_log}" "${eval_capture_runtime_log}" \
    "${representation_ridge_probe}" "${mdn_ridge_probe}"
  sha256sum \
    "${representation_ridge_root}/bin/mdn_cached_feature_runtime_head_parity" \
    "${representation_ridge_root}/ridge_helper.compiler.version" \
    "${representation_ridge_root}/ridge_helper.linker.version" \
    "${representation_ridge_root}/ridge_helper.ldd.normalized" \
    "${representation_ridge_root}/ridge_helper.dynamic_section" \
    "${representation_ridge_root}/ridge_helper.linked_libraries.sha256" \
    "${representation_ridge_root}/ridge_helper.build.provenance" \
    "${representation_ridge_root}/ridge_helper.execution_inputs.sha256" \
    "${mdn_ridge_root}/bin/mdn_cached_feature_runtime_head_parity" \
    "${mdn_ridge_root}/ridge_helper.compiler.version" \
    "${mdn_ridge_root}/ridge_helper.linker.version" \
    "${mdn_ridge_root}/ridge_helper.ldd.normalized" \
    "${mdn_ridge_root}/ridge_helper.dynamic_section" \
    "${mdn_ridge_root}/ridge_helper.linked_libraries.sha256" \
    "${mdn_ridge_root}/ridge_helper.build.provenance" \
    "${mdn_ridge_root}/ridge_helper.execution_inputs.sha256"
  config_dependency_manifest
  synthetic_data_manifest
  sha256sum "${representation_job}/runtime.result.fact" \
    "${representation_job}/job.manifest" \
    "${representation_job}/channel_representation.report" \
    "${representation_job}/stage.sha256" \
    "${mdn_job}/runtime.result.fact" "${mdn_job}/job.manifest" \
    "${mdn_job}/channel_inference.report" "${mdn_job}/stage.sha256" \
    "${train_capture_job}.config" "${train_capture_job}/runtime.result.fact" \
    "${train_capture_job}/job.manifest" \
    "${train_capture_job}/channel_inference.report" \
    "${train_capture_job}/stage.sha256" \
    "${eval_capture_job}.config" "${eval_capture_job}/runtime.result.fact" \
    "${eval_capture_job}/job.manifest" \
    "${eval_capture_job}/channel_inference.report" \
    "${eval_capture_job}/stage.sha256" \
    "${representation_ridge_probe}.sha256" "${mdn_ridge_probe}.sha256"
  while IFS= read -r log_file; do
    sha256sum "${log_file}"
  done < <(find "${logs_dir}" -maxdepth 1 -type f -name '*.log' | sort)
} >"${hash_candidate}"
install_immutable_candidate "${hash_candidate}" "${hash_manifest}" \
  "final hash manifest"

report_candidate="$(mktemp "${runtime_root}/.report.XXXXXX")"
{
  echo "schema_id=${schema_id}"
  echo "status=complete"
  echo "diagnostic_authority=diagnostic_only"
  echo "benchmark_acceptance_authority=false"
  echo "ablation_scope=vicreg_gradient_only"
  echo "outer_input_augmentation_profile=light_phase_safe_v2"
  echo "hidden_vicreg_weak_views=canonical"
  echo "hidden_vicreg_weak_view_time_dropout_scale=0.10"
  echo "hidden_vicreg_weak_view_time_dropout_prob_effective=0.01"
  echo "hidden_vicreg_weak_view_gaussian_jitter_std=0.005"
  echo "use_vicreg_loss=true"
  echo "use_global_vicreg=true"
  echo "lambda_vicreg=0"
  echo "vicreg_branch_and_rng_schedule_preserved=true"
  echo "vicreg_gradient_contribution_disabled=true"
  echo "canonical_spec_difference=LAMBDA_VICREG_0.05_to_0.0"
  echo "baseline_exact_code_identity_proven=false"
  echo "raw_ridge_comparator_probe_boundary_reliable=false"
  echo "raw_actual_probe_boundary=pre_mdn_adapter_representation_node_encoding"
  echo "post_mdn_actual_probe_boundary=post_adapter_direct_edge_features"
  echo "runner=${runner_path}"
  echo "runner_sha256=$(sha256 "${runner_path}")"
  echo "experiment_inputs=${inputs_manifest}"
  echo "experiment_inputs_sha256=$(sha256 "${inputs_manifest}")"
  echo "experiment_inputs_seal=${inputs_manifest_seal}"
  echo "experiment_inputs_seal_sha256=$(sha256 "${inputs_manifest_seal}")"
  echo "canonical_representation_jkimyei=${canonical_representation_jkimyei}"
  echo "canonical_representation_jkimyei_sha256=$(sha256 "${canonical_representation_jkimyei}")"
  echo "train_anchor_range=[${train_begin},${train_end})"
  echo "protected_eval_anchor_range=[${eval_begin},${eval_end})"
  echo "unconsumed_holdout_anchor_range=[${sealed_holdout_begin},1170)"
  echo "unconsumed_holdout_opened=false"
  echo "representation_report=${representation_job}/channel_representation.report"
  echo "representation_report_sha256=$(sha256 "${representation_job}/channel_representation.report")"
  echo "representation_report_vicreg_view_gaussian_jitter_std=$(kv vicreg_view_gaussian_jitter_std "${representation_job}/channel_representation.report")"
  echo "representation_report_vicreg_view_time_dropout_scale=$(kv vicreg_view_time_dropout_scale "${representation_job}/channel_representation.report")"
  echo "representation_report_vicreg_view_time_dropout_prob_effective=$(kv vicreg_view_time_dropout_prob_effective "${representation_job}/channel_representation.report")"
  echo "representation_report_lambda_vicreg=$(kv lambda_vicreg "${representation_job}/channel_representation.report")"
  echo "representation_report_use_vicreg_loss=$(kv use_vicreg_loss "${representation_job}/channel_representation.report")"
  echo "representation_report_use_global_vicreg=$(kv use_global_vicreg "${representation_job}/channel_representation.report")"
  echo "representation_report_max_grad_norm=$(kv max_grad_norm "${representation_job}/channel_representation.report")"
  echo "representation_checkpoint=${representation_checkpoint}"
  echo "representation_checkpoint_sha256=$(sha256 "${representation_checkpoint}")"
  echo "mdn_checkpoint=${mdn_checkpoint}"
  echo "mdn_checkpoint_sha256=$(sha256 "${mdn_checkpoint}")"
  echo "train_representation_probe=${train_representation_probe}"
  echo "train_representation_probe_sha256=$(sha256 "${train_representation_probe}")"
  echo "eval_representation_probe=${eval_representation_probe}"
  echo "eval_representation_probe_sha256=$(sha256 "${eval_representation_probe}")"
  echo "train_mdn_probe=${train_mdn_probe}"
  echo "train_mdn_probe_sha256=$(sha256 "${train_mdn_probe}")"
  echo "eval_mdn_probe=${eval_mdn_probe}"
  echo "eval_mdn_probe_sha256=$(sha256 "${eval_mdn_probe}")"
  echo "representation_ridge_probe=${representation_ridge_probe}"
  echo "representation_ridge_probe_sha256=$(sha256 "${representation_ridge_probe}")"
  echo "representation_ridge_helper_provenance=${representation_ridge_root}/ridge_helper.build.provenance"
  echo "representation_ridge_helper_provenance_sha256=$(sha256 "${representation_ridge_root}/ridge_helper.build.provenance")"
  echo "representation_ridge_helper_execution_inputs_sha256=$(sha256 "${representation_ridge_root}/ridge_helper.execution_inputs.sha256")"
  echo "representation_ridge_selection_gate_pass=$(kv ridge.selection_gate_pass "${representation_ridge_probe}")"
  echo "representation_ridge_selected_alpha=$(kv ridge.selected_alpha "${representation_ridge_probe}")"
  echo "representation_ridge_validation_directional_accuracy=$(kv ridge.selected.validation.directional_accuracy "${representation_ridge_probe}")"
  echo "representation_ridge_validation_pairwise_rank_accuracy=$(kv ridge.selected.validation.pairwise_rank_accuracy "${representation_ridge_probe}")"
  echo "representation_ridge_eval_directional_accuracy=$(kv ridge.refit.diagnostic_confirmation.directional_accuracy "${representation_ridge_probe}")"
  echo "representation_ridge_eval_pairwise_rank_accuracy=$(kv ridge.refit.diagnostic_confirmation.pairwise_rank_accuracy "${representation_ridge_probe}")"
  echo "mdn_ridge_probe=${mdn_ridge_probe}"
  echo "mdn_ridge_probe_sha256=$(sha256 "${mdn_ridge_probe}")"
  echo "mdn_ridge_helper_provenance=${mdn_ridge_root}/ridge_helper.build.provenance"
  echo "mdn_ridge_helper_provenance_sha256=$(sha256 "${mdn_ridge_root}/ridge_helper.build.provenance")"
  echo "mdn_ridge_helper_execution_inputs_sha256=$(sha256 "${mdn_ridge_root}/ridge_helper.execution_inputs.sha256")"
  echo "mdn_ridge_selection_gate_pass=$(kv ridge.selection_gate_pass "${mdn_ridge_probe}")"
  echo "mdn_ridge_selected_alpha=$(kv ridge.selected_alpha "${mdn_ridge_probe}")"
  echo "mdn_ridge_validation_directional_accuracy=$(kv ridge.selected.validation.directional_accuracy "${mdn_ridge_probe}")"
  echo "mdn_ridge_validation_pairwise_rank_accuracy=$(kv ridge.selected.validation.pairwise_rank_accuracy "${mdn_ridge_probe}")"
  echo "mdn_ridge_eval_directional_accuracy=$(kv ridge.refit.diagnostic_confirmation.directional_accuracy "${mdn_ridge_probe}")"
  echo "mdn_ridge_eval_pairwise_rank_accuracy=$(kv ridge.refit.diagnostic_confirmation.pairwise_rank_accuracy "${mdn_ridge_probe}")"
  echo "hash_manifest=${hash_manifest}"
  echo "hash_manifest_sha256=$(sha256 "${hash_manifest}")"
} >"${report_candidate}"
install_immutable_candidate "${report_candidate}" "${result_report}" \
  "final experiment report"
seal_files "${result_report_seal}" "${result_report}" "${hash_manifest}"
verify_sha256_manifest "${result_report_seal}"

echo "completed VICReg gradient-off ablation"
echo "report=${result_report}"
echo "report_seal=${result_report_seal}"
echo "representation_ridge_probe=${representation_ridge_probe}"
echo "mdn_ridge_probe=${mdn_ridge_probe}"
