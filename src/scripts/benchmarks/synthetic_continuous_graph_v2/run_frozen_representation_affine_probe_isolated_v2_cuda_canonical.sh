#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_frozen_representation_affine_probe_isolated_v2"
readonly result_schema_id="${schema_id}.result.v1"
readonly development_schema_id="synthetic_v2_frozen_affine_development_isolated_v2"
readonly trigger_schema_id="synthetic_v2_frozen_affine_route_trigger_isolated_v2"
readonly capture_schema_id="synthetic_v2_frozen_feature_capture_isolated_v2"
readonly source_schema_id="synthetic_v2_isolated_development_source_v1"
readonly cursor_erratum_schema_id="${source_schema_id}.cursor_alignment_erratum.v1"

readonly post384_development_file_id="synthetic_v2_frozen_representation_affine_development_isolated_v2"
readonly raw96_development_file_id="synthetic_v2_frozen_encoder_affine_development_isolated_v2"
readonly untrained_development_file_id="synthetic_v2_untrained_encoder_affine_control_isolated_v2"
readonly post384_development_schema_id="synthetic_v2_frozen_representation_affine_development_v1"
readonly raw96_development_schema_id="synthetic_v2_frozen_encoder_affine_development_v1"
readonly untrained_development_schema_id="synthetic_v2_untrained_encoder_affine_control_v1"
readonly post384_certified_file_id="synthetic_v2_frozen_representation_affine_probe_isolated_v2"
readonly raw96_certified_file_id="synthetic_v2_frozen_encoder_affine_probe_isolated_v2"
readonly post384_certified_schema_id="synthetic_v2_frozen_representation_affine_probe_v1"
readonly raw96_certified_schema_id="synthetic_v2_frozen_encoder_affine_probe_v1"

readonly expected_scientific_runner_sha256="ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173"
readonly expected_helper_sha256="45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939"
readonly expected_capture_runner_sha256="bb5f8fe728d81a71d6a8f603ef85686bdb70ae6c52c4ea6890f466d466a1cb32"
readonly expected_capture_development_sha256="fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6"
readonly expected_cuda_correction_sha256="d9c88f5c37771678016799afb157ec7661e3016eb58cdb4321da67b3329358ce"
readonly expected_cursor_erratum_verifier_sha256="e175d9bd0da0486b9abb969b3fbdbb8d2966c197cb1f9a9d4f323355c2cc0d99"
readonly expected_cursor_erratum_receipt_sha256="c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_fresh_preregistration_sha256="bbe8f9b737a5b9913728b35e2bb16784473c58b810c656b1028e3cec8dc46e56"
readonly expected_diagnostic_preregistration_sha256="de69d711090d44a65b7f6cadc59a65746a9577fb9ac3e2136de39a1f73c786f8"
readonly expected_protocol_amendment_sha256="3409a77a067e4d88283962f51f744630110d52dc002b6a747e1d2d73edf4c1a5"
readonly expected_localization_addendum_sha256="2657d1bdeba4d27d955593fe23a626fa629d8930624510ae91ebdb0e224f13e4"
readonly expected_conditional_amendment_sha256="30d7f89016a6e554dc2dc4462f8b1bb27a5ffb2333698f6cd051dfd80b1cab67"
readonly expected_ablation_preregistration_sha256="6a4175f431347387f33c250b747f1f34c29099aaf4b3c94a75ea2e4960cef6cd"
readonly expected_source_isolation_amendment_sha256="c2254ff49cecad622e32d8f994e3abba60aebe287dfad14b80e40ab4203e9b39"
readonly expected_isolated_source_protocol_sha256="1a926ea127c1a03e3daecdd2c84d7e64fca267097f7fd9d61ebdc5efb8fbf793"
readonly expected_staged_hardening_sha256="0630a94d2efb58596b8a6eaaf219579be663eaf36987d5514d8e4f01358a9888"
readonly expected_cursor_alignment_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_cursor_alignment_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"

readonly accepted_anchor_count=3261
readonly maximum_anchor_index=3260
readonly train_begin=0
readonly train_end=2496
readonly validation_begin=2560
readonly validation_end=2816
readonly certified_begin=2880
readonly certified_end=3261
readonly forbidden_final_begin=3328
readonly train_rows=$(((train_end - train_begin) * 9))
readonly validation_rows=$(((validation_end - validation_begin) * 9))
readonly certified_rows=$(((certified_end - certified_begin) * 9))

fail() {
  echo "v2 isolated frozen representation affine probe: $*" >&2
  exit 1
}

usage() {
  cat >&2 <<'USAGE'
usage: run_frozen_representation_affine_probe_isolated_v2_cuda_canonical.sh
       [--plan|--preflight|--run-development|--verify-development|--run-certified|--verify]

  --run-development     Score train/validation only and publish the route trigger.
  --verify-development  Verify the sealed development runtime and route trigger.
  --run-certified       Score the certified-development range only on the canonical route.
  --verify              Verify the sealed canonical certified runtime.
USAGE
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

# Capture the exact operational source identity before any non-trivial work.
# Non-plan modes continuously bind execution and receipts to this fixed
# process-start snapshot rather than re-describing a mutable live path.
script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
readonly process_owner_uid="$(id -u)"
readonly process_start_operational_runner_sha256="$(sha256_of "${script_path}")"
readonly process_start_operational_runner_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_operational_runner_device="$(stat -c '%d' -- "${script_path}")"
readonly process_start_operational_runner_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_operational_runner_owner="$(stat -c '%u' -- "${script_path}")"

kv_value() {
  local path="$1" key="$2" count value
  count="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) count += 1;
    }
    END { print count + 0 }
  ' "${path}")"
  [[ "${count}" == 1 ]] ||
    fail "${path} must contain exactly one ${key}= entry (found ${count})"
  value="$(awk -v key="${key}" '
    {
      eq = index($0, "=");
      if (eq == 0) next;
      lhs = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", lhs);
      if (lhs == key) {
        value = substr($0, eq + 1);
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", value);
        print value;
      }
    }
  ' "${path}")"
  printf '%s' "${value}"
}

expect_kv() {
  local path="$1" key="$2" expected="$3" actual
  actual="$(kv_value "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

reject_forbidden_path() {
  local path="$1"
  case "${path}" in
  */data/raw | */data/raw/* | */data/final | */data/final/*)
    fail "raw/final data path is forbidden: ${path}"
    ;;
  *synthetic_v2_representation_train_v1* | \
    *synthetic_v2_mdn_train_v1* | \
    *synthetic_v2_frozen_feature_capture_v1* | \
    *synthetic_v2_frozen_affine_development_v1*)
    fail "quarantined pre-isolation runtime path is forbidden: ${path}"
    ;;
  esac
}

reject_symlink_components() {
  local path="$1" current="/" rest component
  [[ "${path}" == /* ]] || fail "path is not absolute: ${path}"
  rest="${path#/}"
  while [[ -n "${rest}" ]]; do
    if [[ "${rest}" == */* ]]; then
      component="${rest%%/*}"
      rest="${rest#*/}"
    else
      component="${rest}"
      rest=""
    fi
    [[ -n "${component}" ]] || continue
    if [[ "${current}" == / ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains a symbolic-link component: ${current}"
  done
}

require_file() {
  local path="$1"
  reject_forbidden_path "${path}"
  reject_symlink_components "${path}"
  [[ -f "${path}" && ! -L "${path}" ]] ||
    fail "missing, non-regular, or symlinked file: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "file path is not canonical: ${path}"
}

require_nonempty_file() {
  require_file "$1"
  [[ -s "$1" ]] || fail "empty required file: $1"
}

require_immutable_file() {
  require_nonempty_file "$1"
  [[ "$(stat -c '%A' -- "$1")" != *w* ]] ||
    fail "immutable file retains a write bit: $1"
}

require_directory() {
  local path="$1"
  reject_forbidden_path "${path}"
  reject_symlink_components "${path}"
  [[ -d "${path}" && ! -L "${path}" ]] ||
    fail "missing or symlinked directory: ${path}"
  [[ "$(realpath -e -- "${path}")" == "${path}" ]] ||
    fail "directory path is not canonical: ${path}"
}

expect_mode_owner_links() {
  local path="$1" expected_mode="$2" label="$3"
  require_file "${path}"
  [[ "$(stat -c '%a' -- "${path}")" == "${expected_mode}" ]] ||
    fail "${label} mode is not exactly 0${expected_mode}: ${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an external hard link: ${path}"
  [[ "$(stat -c '%u' -- "${path}")" == "${process_owner_uid}" ]] ||
    fail "${label} is not owned by the executing uid: ${path}"
}

assert_operational_runner_identity() {
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == \
    "${process_start_operational_runner_sha256}" ]] ||
    fail "operational affine runner changed after process start"
  [[ "$(stat -c '%i' -- "${script_path}")" == \
    "${process_start_operational_runner_inode}" ]] ||
    fail "operational affine runner inode changed after process start"
  [[ "$(stat -c '%d' -- "${script_path}")" == \
    "${process_start_operational_runner_device}" ]] ||
    fail "operational affine runner device changed after process start"
  [[ "$(stat -c '%s' -- "${script_path}")" == \
    "${process_start_operational_runner_bytes}" ]] ||
    fail "operational affine runner size changed after process start"
  [[ "$(stat -c '%u' -- "${script_path}")" == \
    "${process_start_operational_runner_owner}" ]] ||
    fail "operational affine runner owner changed after process start"
  [[ "${process_start_operational_runner_owner}" == \
    "${process_owner_uid}" ]] ||
    fail "operational affine runner was not owned by the executing uid at process start"
  expect_mode_owner_links "${script_path}" 555 \
    "operational affine runner"
}

verify_cuda_alias_contract() {
  require_directory "${cuda_canonical_include}"
  require_directory "${cuda_canonical_library}"
  [[ -L "${cuda_include_alias}" ]] ||
    fail "CUDA include compatibility path is not a symlink: ${cuda_include_alias}"
  [[ -L "${cuda_library_alias}" ]] ||
    fail "CUDA library compatibility path is not a symlink: ${cuda_library_alias}"
  [[ "$(readlink -- "${cuda_include_alias}")" == \
    "${cuda_include_alias_text}" ]] ||
    fail "CUDA include compatibility symlink text drifted"
  [[ "$(readlink -- "${cuda_library_alias}")" == \
    "${cuda_library_alias_text}" ]] ||
    fail "CUDA library compatibility symlink text drifted"
  [[ "$(realpath -e -- "${cuda_include_alias}")" == \
    "${cuda_canonical_include}" ]] ||
    fail "CUDA include compatibility symlink resolves unexpectedly"
  [[ "$(realpath -e -- "${cuda_library_alias}")" == \
    "${cuda_canonical_library}" ]] ||
    fail "CUDA library compatibility symlink resolves unexpectedly"
  [[ -d "${cuda_include_alias}" && -d "${cuda_library_alias}" ]] ||
    fail "CUDA compatibility symlink target is not a directory"
}

emit_cuda_operational_bindings() {
  assert_operational_runner_identity
  cat <<CUDA_OPERATIONAL
scientific_affine_runner_path=${scientific_runner}
scientific_affine_runner_sha256=${expected_scientific_runner_sha256}
scientific_affine_runner_mode=0755
scientific_affine_runner_links=1
scientific_affine_runner_owner_uid=${process_owner_uid}
capture_frozen_affine_runner_path=${frozen_runner}
capture_frozen_affine_runner_sha256=${expected_scientific_runner_sha256}
capture_frozen_affine_runner_mode=0444
capture_frozen_affine_runner_links=1
capture_frozen_affine_runner_owner_uid=${process_owner_uid}
operational_affine_runner_path=${script_path}
operational_affine_runner_sha256=${process_start_operational_runner_sha256}
operational_affine_runner_process_start_sha256=${process_start_operational_runner_sha256}
operational_affine_runner_process_start_inode=${process_start_operational_runner_inode}
operational_affine_runner_process_start_device=${process_start_operational_runner_device}
operational_affine_runner_process_start_bytes=${process_start_operational_runner_bytes}
operational_affine_runner_process_start_owner_uid=${process_start_operational_runner_owner}
operational_affine_runner_mode=0555
operational_affine_runner_links=1
cuda_canonical_path_correction_path=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
cuda_canonical_path_correction_mode=0444
cuda_canonical_path_correction_links=1
cuda_canonical_path_correction_owner_uid=${process_owner_uid}
frozen_feature_capture_runner_path=${capture_runner}
frozen_feature_capture_runner_sha256=${expected_capture_runner_sha256}
frozen_feature_capture_runner_mode=0555
frozen_feature_capture_runner_links=1
frozen_feature_capture_runner_owner_uid=${process_owner_uid}
frozen_capture_development_path=${capture_development}
frozen_capture_development_sha256=${expected_capture_development_sha256}
frozen_capture_development_mode=0444
frozen_capture_development_links=1
frozen_capture_development_owner_uid=${process_owner_uid}
scientific_affine_helper_path=${live_helper}
scientific_affine_helper_sha256=${expected_helper_sha256}
scientific_affine_helper_mode=0644
scientific_affine_helper_links=1
scientific_affine_helper_owner_uid=${process_owner_uid}
capture_frozen_affine_helper_path=${frozen_helper}
capture_frozen_affine_helper_sha256=${expected_helper_sha256}
capture_frozen_affine_helper_mode=0444
capture_frozen_affine_helper_links=1
capture_frozen_affine_helper_owner_uid=${process_owner_uid}
cuda_include_alias_path=${cuda_include_alias}
cuda_include_alias_readlink=${cuda_include_alias_text}
cuda_include_alias_realpath=${cuda_canonical_include}
cuda_canonical_include_path=${cuda_canonical_include}
cuda_library_alias_path=${cuda_library_alias}
cuda_library_alias_readlink=${cuda_library_alias_text}
cuda_library_alias_realpath=${cuda_canonical_library}
cuda_canonical_library_path=${cuda_canonical_library}
cuda_original_failure_path=${cuda_include_alias}
cuda_original_failure_reason=path_contains_symbolic_link_component
cuda_compile_include_argument=${cuda_include_alias}
cuda_link_library_argument=${cuda_library_alias}
cuda_runtime_rpath_argument=${cuda_library_alias}
cuda_alias_contract_verified=true
cuda_alias_exception_scope=two_exact_compatibility_symlinks_only
global_symlink_policy_relaxed=false
compile_helper_changed=false
scientific_contract_changed=false
CUDA_OPERATIONAL
}

verify_cuda_operational_bindings() {
  local receipt="$1"
  assert_operational_runner_identity
  verify_cuda_alias_contract
  expect_kv "${receipt}" scientific_affine_runner_path \
    "${scientific_runner}"
  expect_kv "${receipt}" scientific_affine_runner_sha256 \
    "${expected_scientific_runner_sha256}"
  expect_kv "${receipt}" scientific_affine_runner_mode 0755
  expect_kv "${receipt}" scientific_affine_runner_links 1
  expect_kv "${receipt}" scientific_affine_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" capture_frozen_affine_runner_path "${frozen_runner}"
  expect_kv "${receipt}" capture_frozen_affine_runner_sha256 \
    "${expected_scientific_runner_sha256}"
  expect_kv "${receipt}" capture_frozen_affine_runner_mode 0444
  expect_kv "${receipt}" capture_frozen_affine_runner_links 1
  expect_kv "${receipt}" capture_frozen_affine_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" operational_affine_runner_path "${script_path}"
  expect_kv "${receipt}" operational_affine_runner_sha256 \
    "${process_start_operational_runner_sha256}"
  expect_kv "${receipt}" operational_affine_runner_process_start_sha256 \
    "${process_start_operational_runner_sha256}"
  expect_kv "${receipt}" operational_affine_runner_process_start_inode \
    "${process_start_operational_runner_inode}"
  expect_kv "${receipt}" operational_affine_runner_process_start_device \
    "${process_start_operational_runner_device}"
  expect_kv "${receipt}" operational_affine_runner_process_start_bytes \
    "${process_start_operational_runner_bytes}"
  expect_kv "${receipt}" operational_affine_runner_process_start_owner_uid \
    "${process_start_operational_runner_owner}"
  expect_kv "${receipt}" operational_affine_runner_mode 0555
  expect_kv "${receipt}" operational_affine_runner_links 1
  expect_kv "${receipt}" cuda_canonical_path_correction_path \
    "${cuda_correction}"
  expect_kv "${receipt}" cuda_canonical_path_correction_sha256 \
    "${expected_cuda_correction_sha256}"
  expect_kv "${receipt}" cuda_canonical_path_correction_mode 0444
  expect_kv "${receipt}" cuda_canonical_path_correction_links 1
  expect_kv "${receipt}" cuda_canonical_path_correction_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" frozen_feature_capture_runner_path \
    "${capture_runner}"
  expect_kv "${receipt}" frozen_feature_capture_runner_sha256 \
    "${expected_capture_runner_sha256}"
  expect_kv "${receipt}" frozen_feature_capture_runner_mode 0555
  expect_kv "${receipt}" frozen_feature_capture_runner_links 1
  expect_kv "${receipt}" frozen_feature_capture_runner_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" frozen_capture_development_path \
    "${capture_development}"
  expect_kv "${receipt}" frozen_capture_development_sha256 \
    "${expected_capture_development_sha256}"
  expect_kv "${receipt}" frozen_capture_development_mode 0444
  expect_kv "${receipt}" frozen_capture_development_links 1
  expect_kv "${receipt}" frozen_capture_development_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" scientific_affine_helper_path "${live_helper}"
  expect_kv "${receipt}" scientific_affine_helper_sha256 \
    "${expected_helper_sha256}"
  expect_kv "${receipt}" scientific_affine_helper_mode 0644
  expect_kv "${receipt}" scientific_affine_helper_links 1
  expect_kv "${receipt}" scientific_affine_helper_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" capture_frozen_affine_helper_path "${frozen_helper}"
  expect_kv "${receipt}" capture_frozen_affine_helper_sha256 \
    "${expected_helper_sha256}"
  expect_kv "${receipt}" capture_frozen_affine_helper_mode 0444
  expect_kv "${receipt}" capture_frozen_affine_helper_links 1
  expect_kv "${receipt}" capture_frozen_affine_helper_owner_uid \
    "${process_owner_uid}"
  expect_kv "${receipt}" cuda_include_alias_path "${cuda_include_alias}"
  expect_kv "${receipt}" cuda_include_alias_readlink \
    "${cuda_include_alias_text}"
  expect_kv "${receipt}" cuda_include_alias_realpath \
    "${cuda_canonical_include}"
  expect_kv "${receipt}" cuda_canonical_include_path \
    "${cuda_canonical_include}"
  expect_kv "${receipt}" cuda_library_alias_path "${cuda_library_alias}"
  expect_kv "${receipt}" cuda_library_alias_readlink \
    "${cuda_library_alias_text}"
  expect_kv "${receipt}" cuda_library_alias_realpath \
    "${cuda_canonical_library}"
  expect_kv "${receipt}" cuda_canonical_library_path \
    "${cuda_canonical_library}"
  expect_kv "${receipt}" cuda_original_failure_path "${cuda_include_alias}"
  expect_kv "${receipt}" cuda_original_failure_reason \
    path_contains_symbolic_link_component
  expect_kv "${receipt}" cuda_compile_include_argument \
    "${cuda_include_alias}"
  expect_kv "${receipt}" cuda_link_library_argument \
    "${cuda_library_alias}"
  expect_kv "${receipt}" cuda_runtime_rpath_argument \
    "${cuda_library_alias}"
  expect_kv "${receipt}" cuda_alias_contract_verified true
  expect_kv "${receipt}" cuda_alias_exception_scope \
    two_exact_compatibility_symlinks_only
  expect_kv "${receipt}" global_symlink_policy_relaxed false
  expect_kv "${receipt}" compile_helper_changed false
  expect_kv "${receipt}" scientific_contract_changed false
}

require_contained_path() {
  local path="$1" root="$2"
  [[ "${path}" == "${root}" || "${path}" == "${root}/"* ]] ||
    fail "path escapes fixed root ${root}: ${path}"
}

publish_immutable_candidate() {
  local candidate="$1" destination="$2" root="$3" label="$4"
  assert_operational_runner_identity
  require_contained_path "${destination}" "${root}"
  reject_symlink_components "${destination}"
  chmod 0444 -- "${candidate}"
  if [[ -e "${destination}" || -L "${destination}" ]]; then
    require_immutable_file "${destination}"
    cmp -s -- "${candidate}" "${destination}" ||
      fail "immutable ${label} drifted: ${destination}"
    rm -f -- "${candidate}"
  else
    assert_operational_runner_identity
    ln -- "${candidate}" "${destination}" ||
      fail "could not atomically publish ${label}: ${destination}"
    assert_operational_runner_identity
    rm -f -- "${candidate}"
  fi
  require_immutable_file "${destination}"
  assert_operational_runner_identity
}

benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"

capture_runner="${script_dir}/run_frozen_feature_capture_isolated_v2.sh"
capture_runtime="${runtime_parent}/${capture_schema_id}"
capture_development="${capture_runtime}/development.status"
capture_result="${capture_runtime}/result.status"
route_trigger="${capture_runtime}/affine_route_trigger.status"
capture_train_job="${capture_runtime}/jobs/train"
capture_validation_job="${capture_runtime}/jobs/validation"
capture_certified_job="${capture_runtime}/jobs/certified"
capture_untrained_train_job="${capture_runtime}/untrained_jobs/train"
capture_untrained_validation_job="${capture_runtime}/untrained_jobs/validation"
capture_frozen_root="${capture_runtime}/frozen_selection_sources"
frozen_helper="${capture_frozen_root}/frozen_representation_affine_probe.cpp"
frozen_runner="${capture_frozen_root}/run_frozen_representation_affine_probe_isolated_v2.sh"
live_helper="${script_dir}/frozen_representation_affine_probe.cpp"
scientific_runner="${script_dir}/run_frozen_representation_affine_probe_isolated_v2.sh"
cuda_correction="${benchmark_root}/AFFINE_CUDA_CANONICAL_PATH_CORRECTION.md"

source_runtime="${runtime_parent}/synthetic_v2_isolated_development_source_v1"
source_closure="${source_runtime}/development_source_closure.status"
cursor_erratum_verifier="${script_dir}/seal_and_verify_cursor_alignment_erratum_v2.sh"
cursor_erratum_receipt="${source_runtime}/cursor_alignment_erratum.status"

development_runtime="${runtime_parent}/${development_schema_id}"
final_runtime="${runtime_parent}/${schema_id}"
development_binary="${development_runtime}/bin/frozen_representation_affine_probe"
development_status="${development_runtime}/development.status"
development_master="${development_runtime}/master.sha256"
post384_development_report="${development_runtime}/main/${post384_development_file_id}.report"
raw96_development_report="${development_runtime}/main/${raw96_development_file_id}.report"
untrained_development_report="${development_runtime}/main/${untrained_development_file_id}.report"
final_binary="${final_runtime}/bin/frozen_representation_affine_probe"
final_status="${final_runtime}/result.status"
final_master="${final_runtime}/master.sha256"
post384_certified_report="${final_runtime}/main/${post384_certified_file_id}.report"
raw96_certified_report="${final_runtime}/main/${raw96_certified_file_id}.report"
untrained_certified_report="${final_runtime}/main/${untrained_development_file_id}.report"

fresh_preregistration="${benchmark_root}/FRESH_SEED_PREREGISTRATION.md"
diagnostic_preregistration="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md"
protocol_amendment="${benchmark_root}/REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md"
localization_addendum="${benchmark_root}/REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md"
conditional_amendment="${benchmark_root}/REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md"
ablation_preregistration="${benchmark_root}/REPRESENTATION_ABLATION_PREREGISTRATION.md"
source_isolation_amendment="${benchmark_root}/DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md"
isolated_source_protocol="${benchmark_root}/ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md"
staged_hardening="${benchmark_root}/STAGED_EVALUATION_HARDENING_AMENDMENT.md"
cursor_alignment_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
cursor_alignment_metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
libtorch_path="${repo_root}/.external/libtorch-upd"
cuda_path="/usr/local/cuda-12.4"
cuda_include_alias="${cuda_path}/include"
cuda_library_alias="${cuda_path}/lib64"
cuda_include_alias_text="targets/x86_64-linux/include"
cuda_library_alias_text="targets/x86_64-linux/lib"
cuda_canonical_include="${cuda_path}/targets/x86_64-linux/include"
cuda_canonical_library="${cuda_path}/targets/x86_64-linux/lib"

mode="${1:---plan}"
[[ "$#" -le 1 ]] || {
  usage
  exit 2
}
case "${mode}" in
--plan | --preflight | --run-development | --verify-development | --run-certified | --verify) ;;
--run) fail "unconditional --run is forbidden; development and certified stages are separate" ;;
*) usage; exit 2 ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
scientific_affine_runner_path=${scientific_runner}
scientific_affine_runner_sha256=${expected_scientific_runner_sha256}
operational_affine_runner_path=${script_path}
operational_affine_runner_sha256=${process_start_operational_runner_sha256}
operational_affine_runner_process_start_inode=${process_start_operational_runner_inode}
operational_affine_runner_process_start_device=${process_start_operational_runner_device}
operational_affine_runner_process_start_bytes=${process_start_operational_runner_bytes}
operational_affine_runner_process_start_owner_uid=${process_start_operational_runner_owner}
operational_affine_runner_required_mode=0555
operational_affine_runner_required_links=1
cuda_canonical_path_correction_path=${cuda_correction}
cuda_canonical_path_correction_sha256=${expected_cuda_correction_sha256}
cuda_canonical_path_correction_required_mode=0444
cuda_canonical_path_correction_required_links=1
frozen_capture_development_path=${capture_development}
frozen_capture_development_sha256=${expected_capture_development_sha256}
affine_helper_sha256=${expected_helper_sha256}
cuda_include_alias_path=${cuda_include_alias}
cuda_include_alias_readlink=${cuda_include_alias_text}
cuda_include_alias_realpath=${cuda_canonical_include}
cuda_library_alias_path=${cuda_library_alias}
cuda_library_alias_readlink=${cuda_library_alias_text}
cuda_library_alias_realpath=${cuda_canonical_library}
cuda_original_failure_path=${cuda_include_alias}
cuda_original_failure_reason=path_contains_symbolic_link_component
cuda_compile_include_argument=${cuda_include_alias}
cuda_link_library_argument=${cuda_library_alias}
cuda_runtime_rpath_argument=${cuda_library_alias}
cuda_alias_exception_scope=two_exact_compatibility_symlinks_only
global_symlink_policy_relaxed=false
compile_helper_changed=false
scientific_contract_changed=false
capture_development=${capture_development}
capture_result=${capture_result}
route_trigger=${route_trigger}
source_closure=${source_closure}
cursor_alignment_erratum_verifier=${cursor_erratum_verifier}
cursor_alignment_erratum_receipt=${cursor_erratum_receipt}
development_runtime=${development_runtime}
certified_runtime=${final_runtime}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
certified_development_anchor_range=[${certified_begin},${certified_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_development_probe_rows=${certified_rows}
development_maximum_anchor_read=$((validation_end - 1))
certified_maximum_anchor_read=${maximum_anchor_index}
route_rule=trained_raw96_validation_strong_gate_only
canonical_certification_route=canonical_certification
ablation_route_certified_access=false
certified_selection_lock_required=true
untrained_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
independent_final_evidence=false
policy_access=false
PLAN
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

# Plan remains usable while this candidate is mutable. Every operational mode
# rejects a mutable, linked, replaced, or post-start-modified wrapper here,
# before any non-plan work can begin.
assert_operational_runner_identity

emit_document_bindings() {
  cat <<DOCUMENTS
fresh_preregistration_path=${fresh_preregistration}
fresh_preregistration_sha256=$(sha256_of "${fresh_preregistration}")
diagnostic_preregistration_path=${diagnostic_preregistration}
diagnostic_preregistration_sha256=$(sha256_of "${diagnostic_preregistration}")
diagnostic_protocol_amendment_path=${protocol_amendment}
diagnostic_protocol_amendment_sha256=$(sha256_of "${protocol_amendment}")
localization_addendum_path=${localization_addendum}
localization_addendum_sha256=$(sha256_of "${localization_addendum}")
conditional_certification_amendment_path=${conditional_amendment}
conditional_certification_amendment_sha256=$(sha256_of "${conditional_amendment}")
ablation_preregistration_path=${ablation_preregistration}
ablation_preregistration_sha256=$(sha256_of "${ablation_preregistration}")
source_isolation_amendment_path=${source_isolation_amendment}
source_isolation_amendment_sha256=$(sha256_of "${source_isolation_amendment}")
isolated_source_protocol_path=${isolated_source_protocol}
isolated_source_protocol_sha256=$(sha256_of "${isolated_source_protocol}")
staged_hardening_amendment_path=${staged_hardening}
staged_hardening_amendment_sha256=$(sha256_of "${staged_hardening}")
cursor_alignment_correction_path=${cursor_alignment_correction}
cursor_alignment_correction_sha256=$(sha256_of "${cursor_alignment_correction}")
cursor_alignment_metadata_erratum_path=${cursor_alignment_metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${cursor_alignment_metadata_erratum}")
DOCUMENTS
}

verify_document_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" fresh_preregistration_path "${fresh_preregistration}"
  expect_kv "${receipt}" fresh_preregistration_sha256 "$(sha256_of "${fresh_preregistration}")"
  expect_kv "${receipt}" diagnostic_preregistration_path "${diagnostic_preregistration}"
  expect_kv "${receipt}" diagnostic_preregistration_sha256 "$(sha256_of "${diagnostic_preregistration}")"
  expect_kv "${receipt}" diagnostic_protocol_amendment_path "${protocol_amendment}"
  expect_kv "${receipt}" diagnostic_protocol_amendment_sha256 "$(sha256_of "${protocol_amendment}")"
  expect_kv "${receipt}" localization_addendum_path "${localization_addendum}"
  expect_kv "${receipt}" localization_addendum_sha256 "$(sha256_of "${localization_addendum}")"
  expect_kv "${receipt}" conditional_certification_amendment_path "${conditional_amendment}"
  expect_kv "${receipt}" conditional_certification_amendment_sha256 "$(sha256_of "${conditional_amendment}")"
  expect_kv "${receipt}" ablation_preregistration_path "${ablation_preregistration}"
  expect_kv "${receipt}" ablation_preregistration_sha256 "$(sha256_of "${ablation_preregistration}")"
  expect_kv "${receipt}" source_isolation_amendment_path "${source_isolation_amendment}"
  expect_kv "${receipt}" source_isolation_amendment_sha256 "$(sha256_of "${source_isolation_amendment}")"
  expect_kv "${receipt}" isolated_source_protocol_path "${isolated_source_protocol}"
  expect_kv "${receipt}" isolated_source_protocol_sha256 "$(sha256_of "${isolated_source_protocol}")"
  expect_kv "${receipt}" staged_hardening_amendment_path "${staged_hardening}"
  expect_kv "${receipt}" staged_hardening_amendment_sha256 "$(sha256_of "${staged_hardening}")"
  expect_kv "${receipt}" cursor_alignment_correction_path "${cursor_alignment_correction}"
  expect_kv "${receipt}" cursor_alignment_correction_sha256 "$(sha256_of "${cursor_alignment_correction}")"
  expect_kv "${receipt}" cursor_alignment_metadata_erratum_path "${cursor_alignment_metadata_erratum}"
  expect_kv "${receipt}" cursor_alignment_metadata_erratum_sha256 "$(sha256_of "${cursor_alignment_metadata_erratum}")"
}

emit_cursor_erratum_bindings() {
  cat <<ERRATUM
cursor_alignment_erratum_verifier_path=${cursor_erratum_verifier}
cursor_alignment_erratum_verifier_sha256=$(sha256_of "${cursor_erratum_verifier}")
cursor_alignment_erratum_receipt_path=${cursor_erratum_receipt}
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_erratum_receipt}")
ERRATUM
}

verify_cursor_erratum_bindings() {
  local receipt="$1"
  expect_kv "${receipt}" cursor_alignment_erratum_verifier_path "${cursor_erratum_verifier}"
  expect_kv "${receipt}" cursor_alignment_erratum_verifier_sha256 "$(sha256_of "${cursor_erratum_verifier}")"
  expect_kv "${receipt}" cursor_alignment_erratum_receipt_path "${cursor_erratum_receipt}"
  expect_kv "${receipt}" cursor_alignment_erratum_receipt_sha256 "$(sha256_of "${cursor_erratum_receipt}")"
}

verify_expected_document_hashes() {
  local path
  for path in "${fresh_preregistration}" "${diagnostic_preregistration}" \
    "${protocol_amendment}" "${localization_addendum}" \
    "${conditional_amendment}" "${ablation_preregistration}" \
    "${source_isolation_amendment}" "${isolated_source_protocol}" \
    "${staged_hardening}" "${cursor_alignment_correction}" \
    "${cursor_alignment_metadata_erratum}"; do
    require_file "${path}"
  done
  [[ "$(sha256_of "${fresh_preregistration}")" == "${expected_fresh_preregistration_sha256}" ]] ||
    fail "fresh-seed preregistration hash drifted"
  [[ "$(sha256_of "${diagnostic_preregistration}")" == "${expected_diagnostic_preregistration_sha256}" ]] ||
    fail "diagnostic preregistration hash drifted"
  [[ "$(sha256_of "${protocol_amendment}")" == "${expected_protocol_amendment_sha256}" ]] ||
    fail "diagnostic protocol amendment hash drifted"
  [[ "$(sha256_of "${localization_addendum}")" == "${expected_localization_addendum_sha256}" ]] ||
    fail "localization addendum hash drifted"
  [[ "$(sha256_of "${conditional_amendment}")" == "${expected_conditional_amendment_sha256}" ]] ||
    fail "conditional-certification amendment hash drifted"
  [[ "$(sha256_of "${ablation_preregistration}")" == "${expected_ablation_preregistration_sha256}" ]] ||
    fail "ablation preregistration hash drifted"
  [[ "$(sha256_of "${source_isolation_amendment}")" == "${expected_source_isolation_amendment_sha256}" ]] ||
    fail "source-isolation amendment hash drifted"
  [[ "$(sha256_of "${isolated_source_protocol}")" == "${expected_isolated_source_protocol_sha256}" ]] ||
    fail "isolated-source protocol hash drifted"
  [[ "$(sha256_of "${staged_hardening}")" == "${expected_staged_hardening_sha256}" ]] ||
    fail "staged hardening hash drifted"
  [[ "$(sha256_of "${cursor_alignment_correction}")" == "${expected_cursor_alignment_correction_sha256}" ]] ||
    fail "cursor-alignment correction hash drifted"
  [[ "$(sha256_of "${cursor_alignment_metadata_erratum}")" == "${expected_cursor_alignment_metadata_erratum_sha256}" ]] ||
    fail "cursor-alignment metadata erratum hash drifted"
}

verify_cursor_erratum_keyset() {
  local count unknown
  count="$(awk '
    index($0, "=") > 0 { count += 1 }
    END { print count + 0 }
  ' "${cursor_erratum_receipt}")"
  [[ "${count}" == 29 ]] ||
    fail "cursor erratum receipt key count is ${count}, expected 29"
  unknown="$(awk '
    BEGIN {
      split("schema_id status erratum_verifier_path erratum_verifier_sha256 isolated_source_verifier_path isolated_source_verifier_sha256 isolated_source_closure_path isolated_source_closure_sha256 original_cursor_correction_path original_cursor_correction_sha256 cursor_alignment_metadata_erratum_path cursor_alignment_metadata_erratum_sha256 dry_run_job_manifest_path dry_run_job_manifest_sha256 source_cursor_first_anchor_master_day_index source_cursor_last_anchor_master_day_index source_cursor_last_required_coarse_boundary_master_day_index source_cursor_first_anchor_key source_cursor_last_anchor_key accepted_anchor_count candidate_anchor_count maximum_anchor_index train_anchor_range validation_anchor_range certified_development_anchor_range certified_development_probe_rows canonical_data_raw_access final_holdout_available independent_final_evidence", keys, " ");
      for (slot in keys) allowed[keys[slot]] = 1;
    }
    {
      eq = index($0, "=");
      if (eq == 0) next;
      key = substr($0, 1, eq - 1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", key);
      if (!(key in allowed)) print key;
    }
  ' "${cursor_erratum_receipt}")"
  [[ -z "${unknown}" ]] ||
    fail "cursor erratum receipt contains an unexpected key: ${unknown}"
}

verify_source_contract() {
  require_file "${cursor_erratum_verifier}"
  [[ "$(sha256_of "${cursor_erratum_verifier}")" == \
    "${expected_cursor_erratum_verifier_sha256}" ]] ||
    fail "cursor erratum verifier hash drifted"
  require_immutable_file "${cursor_erratum_receipt}"
  [[ "$(stat -c '%a' -- "${cursor_erratum_receipt}")" == 444 ]] ||
    fail "cursor erratum receipt mode is not exactly 0444"
  [[ "$(sha256_of "${cursor_erratum_receipt}")" == \
    "${expected_cursor_erratum_receipt_sha256}" ]] ||
    fail "cursor erratum receipt hash drifted"
  require_immutable_file "${source_closure}"
  [[ "$(sha256_of "${source_closure}")" == "${expected_source_closure_sha256}" ]] ||
    fail "isolated source closure hash drifted"
  assert_operational_runner_identity
  "${cursor_erratum_verifier}" --verify >/dev/null
  assert_operational_runner_identity
  verify_cursor_erratum_keyset
  expect_kv "${cursor_erratum_receipt}" schema_id "${cursor_erratum_schema_id}"
  expect_kv "${cursor_erratum_receipt}" status complete
  expect_kv "${cursor_erratum_receipt}" erratum_verifier_path "${cursor_erratum_verifier}"
  expect_kv "${cursor_erratum_receipt}" erratum_verifier_sha256 "${expected_cursor_erratum_verifier_sha256}"
  expect_kv "${cursor_erratum_receipt}" isolated_source_closure_path "${source_closure}"
  expect_kv "${cursor_erratum_receipt}" isolated_source_closure_sha256 "${expected_source_closure_sha256}"
  expect_kv "${cursor_erratum_receipt}" original_cursor_correction_path "${cursor_alignment_correction}"
  expect_kv "${cursor_erratum_receipt}" original_cursor_correction_sha256 "${expected_cursor_alignment_correction_sha256}"
  expect_kv "${cursor_erratum_receipt}" cursor_alignment_metadata_erratum_path "${cursor_alignment_metadata_erratum}"
  expect_kv "${cursor_erratum_receipt}" cursor_alignment_metadata_erratum_sha256 "${expected_cursor_alignment_metadata_erratum_sha256}"
  expect_kv "${cursor_erratum_receipt}" source_cursor_first_anchor_master_day_index 29
  expect_kv "${cursor_erratum_receipt}" source_cursor_last_anchor_master_day_index 3289
  expect_kv "${cursor_erratum_receipt}" source_cursor_last_required_coarse_boundary_master_day_index 3290
  expect_kv "${cursor_erratum_receipt}" source_cursor_first_anchor_key 1896047999999
  expect_kv "${cursor_erratum_receipt}" source_cursor_last_anchor_key 2177711999999
  expect_kv "${cursor_erratum_receipt}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${cursor_erratum_receipt}" maximum_anchor_index "${maximum_anchor_index}"
  expect_kv "${cursor_erratum_receipt}" train_anchor_range "[${train_begin},${train_end})"
  expect_kv "${cursor_erratum_receipt}" validation_anchor_range "[${validation_begin},${validation_end})"
  expect_kv "${cursor_erratum_receipt}" certified_development_anchor_range "[${certified_begin},${certified_end})"
  expect_kv "${cursor_erratum_receipt}" certified_development_probe_rows "${certified_rows}"
  expect_kv "${cursor_erratum_receipt}" canonical_data_raw_access false
  expect_kv "${cursor_erratum_receipt}" final_holdout_available false
  expect_kv "${cursor_erratum_receipt}" independent_final_evidence false
  expect_kv "${source_closure}" schema_id "${source_schema_id}"
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${source_closure}" maximum_anchor_index "${maximum_anchor_index}"
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false
  expect_kv "${source_closure}" strict_cache_freshness pass
}

verify_common_static_inputs() {
  assert_operational_runner_identity
  local command_name
  for command_name in awk bash chmod cmp cp find flock g++ grep id ldd ln \
    mktemp mv readlink realpath rm sha256sum stat sync; do
    command -v "${command_name}" >/dev/null 2>&1 ||
      fail "missing required command: ${command_name}"
  done
  expect_mode_owner_links "${capture_runner}" 555 \
    "frozen feature-capture runner"
  [[ "$(sha256_of "${capture_runner}")" == \
    "${expected_capture_runner_sha256}" ]] ||
    fail "frozen feature-capture runner hash drifted"
  expect_mode_owner_links "${scientific_runner}" 755 \
    "scientific affine runner"
  [[ "$(sha256_of "${scientific_runner}")" == \
    "${expected_scientific_runner_sha256}" ]] ||
    fail "scientific affine runner hash drifted"
  expect_mode_owner_links "${live_helper}" 644 "scientific affine helper"
  [[ "$(sha256_of "${live_helper}")" == "${expected_helper_sha256}" ]] ||
    fail "affine helper hash drifted"
  require_immutable_file "${cuda_correction}"
  expect_mode_owner_links "${cuda_correction}" 444 \
    "CUDA canonical-path correction"
  [[ "$(sha256_of "${cuda_correction}")" == \
    "${expected_cuda_correction_sha256}" ]] ||
    fail "CUDA canonical-path correction hash drifted"
  verify_expected_document_hashes
  verify_source_contract
  require_directory "${runtime_parent}"
  require_directory "${libtorch_path}/include"
  require_directory "${libtorch_path}/lib"
  verify_cuda_alias_contract
  bash -n "${script_path}"
  bash -n "${scientific_runner}"
  assert_operational_runner_identity
}

verify_frozen_sources() {
  require_directory "${capture_frozen_root}"
  require_immutable_file "${frozen_helper}"
  require_immutable_file "${frozen_runner}"
  expect_mode_owner_links "${frozen_helper}" 444 \
    "capture-frozen affine helper"
  expect_mode_owner_links "${frozen_runner}" 444 \
    "capture-frozen affine runner"
  [[ "$(sha256_of "${frozen_helper}")" == "${expected_helper_sha256}" ]] ||
    fail "capture-frozen affine helper hash drifted"
  [[ "$(sha256_of "${frozen_runner}")" == \
    "${expected_scientific_runner_sha256}" ]] ||
    fail "capture-frozen affine runner hash drifted"
  cmp -s -- "${live_helper}" "${frozen_helper}" ||
    fail "live affine helper differs from capture-frozen helper"
  cmp -s -- "${scientific_runner}" "${frozen_runner}" ||
    fail "scientific affine runner differs from capture-frozen runner"
}

verify_capture_development_contract() {
  require_immutable_file "${capture_development}"
  expect_mode_owner_links "${capture_development}" 444 \
    "frozen feature-capture development receipt"
  [[ "$(sha256_of "${capture_development}")" == \
    "${expected_capture_development_sha256}" ]] ||
    fail "frozen feature-capture development receipt hash drifted"
  expect_kv "${capture_development}" schema_id "${capture_schema_id}.development.v1"
  expect_kv "${capture_development}" status complete
  expect_kv "${capture_development}" runner_path "${capture_runner}"
  expect_kv "${capture_development}" runner_sha256 \
    "${expected_capture_runner_sha256}"
  expect_kv "${capture_development}" isolated_source_closure_path "${source_closure}"
  expect_kv "${capture_development}" isolated_source_closure_sha256 "${expected_source_closure_sha256}"
  verify_cursor_erratum_bindings "${capture_development}"
  expect_kv "${capture_development}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${capture_development}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${capture_development}" maximum_available_anchor "${maximum_anchor_index}"
  expect_kv "${capture_development}" frozen_affine_helper_path "${frozen_helper}"
  expect_kv "${capture_development}" frozen_affine_helper_sha256 "$(sha256_of "${frozen_helper}")"
  expect_kv "${capture_development}" frozen_affine_runner_path "${frozen_runner}"
  expect_kv "${capture_development}" frozen_affine_runner_sha256 \
    "${expected_scientific_runner_sha256}"
  expect_kv "${capture_development}" affine_development_runtime "${development_runtime}"
  expect_kv "${capture_development}" affine_binary_expected_path "${development_binary}"
  expect_kv "${capture_development}" affine_development_status_expected_path "${development_status}"
  expect_kv "${capture_development}" affine_master_manifest_expected_path "${development_master}"
  expect_kv "${capture_development}" post384_validation_report_expected_path "${post384_development_report}"
  expect_kv "${capture_development}" raw96_validation_report_expected_path "${raw96_development_report}"
  expect_kv "${capture_development}" untrained_raw96_validation_report_expected_path "${untrained_development_report}"
  expect_kv "${capture_development}" affine_route_trigger_path "${route_trigger}"
  expect_kv "${capture_development}" train_capture_range "[${train_begin},${train_end})"
  expect_kv "${capture_development}" validation_capture_range "[${validation_begin},${validation_end})"
  expect_kv "${capture_development}" maximum_anchor_read "$((validation_end - 1))"
  expect_kv "${capture_development}" train_probe_rows "${train_rows}"
  expect_kv "${capture_development}" validation_probe_rows "${validation_rows}"
  expect_kv "${capture_development}" certified_capture_created false
  expect_kv "${capture_development}" certified_attempt_created false
  expect_kv "${capture_development}" certified_result_created false
  expect_kv "${capture_development}" purge_anchors_captured false
  expect_kv "${capture_development}" untrained_representation_certified_access false
  expect_kv "${capture_development}" canonical_data_raw_access false
  expect_kv "${capture_development}" final_holdout_access false
  expect_kv "${capture_development}" final_holdout_scored false
  expect_kv "${capture_development}" independent_final_evidence false
  expect_kv "${capture_development}" policy_access false
  verify_document_bindings "${capture_development}"
}

capture_bound_probe() {
  local key="$1" expected="$2"
  expect_kv "${capture_development}" "${key}_path" "${expected}"
  require_immutable_file "${expected}"
  expect_kv "${capture_development}" "${key}_sha256" "$(sha256_of "${expected}")"
  printf '%s' "${expected}"
}

load_development_probes() {
  train_probe="$(capture_bound_probe trained_train_context_probe \
    "${capture_train_job}/mdn_edge_context_features.probe")"
  validation_probe="$(capture_bound_probe trained_validation_context_probe \
    "${capture_validation_job}/mdn_edge_context_features.probe")"
  raw96_train_probe="$(capture_bound_probe trained_train_representation_probe \
    "${capture_train_job}/representation_edge_features.probe")"
  raw96_validation_probe="$(capture_bound_probe trained_validation_representation_probe \
    "${capture_validation_job}/representation_edge_features.probe")"
  untrained_train_probe="$(capture_bound_probe untrained_train_representation_probe \
    "${capture_untrained_train_job}/representation_edge_features.probe")"
  untrained_validation_probe="$(capture_bound_probe untrained_validation_representation_probe \
    "${capture_untrained_validation_job}/representation_edge_features.probe")"
}

preflight_development() {
  assert_operational_runner_identity
  verify_common_static_inputs
  assert_operational_runner_identity
  "${capture_runner}" --verify-development >/dev/null
  assert_operational_runner_identity
  verify_frozen_sources
  verify_capture_development_contract
  load_development_probes
  assert_operational_runner_identity
}

compile_helper() {
  local runtime="$1"
  mkdir -p -- "${runtime}/bin"
  g++ -std=c++20 -O0 -g0 -Wall -Wextra -Werror -fPIC \
    -isystem "${libtorch_path}/include" \
    -isystem "${libtorch_path}/include/torch/csrc/api/include" \
    -isystem "${cuda_path}/include" \
    "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp" \
    -L"${libtorch_path}/lib" -L"${cuda_path}/lib64" \
    -Wl,-rpath,"${libtorch_path}/lib" \
    -Wl,-rpath,"${cuda_path}/lib64" \
    -Wl,--no-as-needed -ltorch_cuda -lc10_cuda -Wl,--as-needed \
    -ltorch_cpu -ltorch -lc10 -lcuda -lcudart -lnvToolsExt \
    -lstdc++ -lpthread -lm \
    -o "${runtime}/bin/frozen_representation_affine_probe"
}

write_toolchain_provenance() {
  local runtime="$1" compiler
  assert_operational_runner_identity
  compiler="$(realpath -e -- "$(command -v g++)")"
  {
    echo "path=${compiler}"
    echo "sha256=$(sha256_of "${compiler}")"
    echo "target=$(g++ -dumpmachine)"
  } >"${runtime}/compiler.identity"
  ldd "${runtime}/bin/frozen_representation_affine_probe" \
    >"${runtime}/linked_libraries.txt"
  grep -Fq libtorch_cpu.so "${runtime}/linked_libraries.txt" ||
    fail "affine helper is not linked to libtorch_cpu"
  assert_operational_runner_identity
}

validate_affine_report() {
  local report="$1" expected_schema="$2" expected_kind="$3"
  local expected_certified_rows="$4" expected_maximum_anchor="$5"
  local expected_classification="$6" expected_selection_lock="${7:-}"
  local expected_record_schema expected_probe_width expected_affine_width
  local expected_excluded valid_candidates classification
  case "${expected_kind}" in
  mdn_context)
    expected_record_schema="kikijyeba.synthetic.mdn_edge_context_feature_probe.v1"
    expected_probe_width=400
    expected_affine_width=384
    expected_excluded=16
    ;;
  representation)
    expected_record_schema="kikijyeba.synthetic.representation_edge_feature_probe.v1"
    expected_probe_width=96
    expected_affine_width=96
    expected_excluded=0
    ;;
  *) fail "unsupported affine probe kind: ${expected_kind}" ;;
  esac
  require_nonempty_file "${report}"
  expect_kv "${report}" schema_id "${expected_schema}"
  expect_kv "${report}" status complete
  expect_kv "${report}" benchmark_id synthetic_continuous_graph_v2
  expect_kv "${report}" probe_kind "${expected_kind}"
  expect_kv "${report}" probe_record_schema "${expected_record_schema}"
  expect_kv "${report}" train_probe_rows "${train_rows}"
  expect_kv "${report}" validation_probe_rows "${validation_rows}"
  expect_kv "${report}" certified_probe_rows "${expected_certified_rows}"
  expect_kv "${report}" probe_rows_total \
    "$((train_rows + validation_rows + expected_certified_rows))"
  expect_kv "${report}" probe_ranges_disjoint true
  expect_kv "${report}" fit_anchor_range "[${train_begin},${train_end})"
  expect_kv "${report}" validation_anchor_range "[${validation_begin},${validation_end})"
  expect_kv "${report}" purge_anchors_used false
  expect_kv "${report}" maximum_anchor_read "${expected_maximum_anchor}"
  expect_kv "${report}" final_holdout_begin "${forbidden_final_begin}"
  expect_kv "${report}" final_holdout_access false
  expect_kv "${report}" policy_access false
  expect_kv "${report}" refit_after_selection false
  expect_kv "${report}" probe_feature_width "${expected_probe_width}"
  expect_kv "${report}" affine_feature_width "${expected_affine_width}"
  expect_kv "${report}" edge_identity_feature_width_excluded "${expected_excluded}"
  valid_candidates="$(kv_value "${report}" numerically_valid_candidate_count)"
  [[ "${valid_candidates}" =~ ^[1-6]$ ]] ||
    fail "report has no numerically valid declared ridge: ${report}"
  classification="$(kv_value "${report}" classification)"
  if [[ "${expected_classification}" == certified ]]; then
    case "${classification}" in
    strong_information_preservation | partial_information_preservation | \
      representation_or_exposed_interface_failure) ;;
    *) fail "unexpected certified classification: ${classification}" ;;
    esac
  else
    [[ "${classification}" == "${expected_classification}" ]] ||
      fail "unexpected development classification: ${classification}"
  fi
  if [[ "${expected_certified_rows}" == 0 ]]; then
    expect_kv "${report}" certified_anchor_range not_opened
    expect_kv "${report}" certified_candidates_scored 0
    expect_kv "${report}" certified_strong_gate_pass not_evaluated
    expect_kv "${report}" certified_partial_gate_pass not_evaluated
  else
    [[ -n "${expected_selection_lock}" ]] ||
      fail "certified report validator omitted its selection lock"
    expect_kv "${report}" certified_anchor_range "[${certified_begin},${certified_end})"
    expect_kv "${report}" certified_candidates_scored 1
    expect_kv "${report}" selection_lock_provided true
    expect_kv "${report}" selection_lock_verified true
    expect_kv "${report}" selection_lock_path "${expected_selection_lock}"
    expect_kv "${report}" selection_lock_schema_id \
      "$(kv_value "${expected_selection_lock}" schema_id)"
    expect_kv "${report}" selection_lock_probe_kind "${expected_kind}"
    expect_kv "${report}" selection_lock_selected_candidate_index \
      "$(kv_value "${expected_selection_lock}" selected_candidate_index)"
    expect_kv "${report}" selection_lock_selected_ridge \
      "$(kv_value "${expected_selection_lock}" selected_ridge)"
    case "$(kv_value "${report}" certified_strong_gate_pass)" in
    true | false) ;;
    *) fail "invalid certified strong-gate result in ${report}" ;;
    esac
    case "$(kv_value "${report}" certified_partial_gate_pass)" in
    true | false) ;;
    *) fail "invalid certified partial-gate result in ${report}" ;;
    esac
  fi
}

validate_untrained_report() {
  local report="$1"
  validate_affine_report "${report}" "${untrained_development_schema_id}" \
    representation 0 "$((validation_end - 1))" \
    untrained_representation_validation_control
}

run_development_reports() {
  local runtime="$1" branch
  assert_operational_runner_identity
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  for branch in main replay; do
    assert_operational_runner_identity
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind mdn_context --development-only \
      --train-input "${train_probe}" \
      --validation-input "${validation_probe}" \
      --output "${runtime}/${branch}/${post384_development_file_id}.report" \
      >"${runtime}/${branch}/${post384_development_file_id}.stdout.log" 2>&1
    assert_operational_runner_identity
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation --development-only \
      --train-input "${raw96_train_probe}" \
      --validation-input "${raw96_validation_probe}" \
      --output "${runtime}/${branch}/${raw96_development_file_id}.report" \
      >"${runtime}/${branch}/${raw96_development_file_id}.stdout.log" 2>&1
    assert_operational_runner_identity
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation --validation-only \
      --train-input "${untrained_train_probe}" \
      --validation-input "${untrained_validation_probe}" \
      --output "${runtime}/${branch}/${untrained_development_file_id}.report" \
      >"${runtime}/${branch}/${untrained_development_file_id}.stdout.log" 2>&1
    assert_operational_runner_identity
  done
  assert_operational_runner_identity
}

verify_development_report_pairs() {
  local runtime="$1" file
  for file in \
    "${post384_development_file_id}.report" \
    "${post384_development_file_id}.stdout.log" \
    "${raw96_development_file_id}.report" \
    "${raw96_development_file_id}.stdout.log" \
    "${untrained_development_file_id}.report" \
    "${untrained_development_file_id}.stdout.log"; do
    require_file "${runtime}/main/${file}"
    require_file "${runtime}/replay/${file}"
    cmp -s -- "${runtime}/main/${file}" "${runtime}/replay/${file}" ||
      fail "development main/replay differ: ${file}"
  done
  validate_affine_report \
    "${runtime}/main/${post384_development_file_id}.report" \
    "${post384_development_schema_id}" mdn_context 0 \
    "$((validation_end - 1))" development_selection_complete
  validate_affine_report \
    "${runtime}/main/${raw96_development_file_id}.report" \
    "${raw96_development_schema_id}" representation 0 \
    "$((validation_end - 1))" development_selection_complete
  validate_untrained_report \
    "${runtime}/main/${untrained_development_file_id}.report"
}

write_development_contract() {
  local runtime="$1"
  assert_operational_runner_identity
  cat >"${runtime}/execution_contract.status" <<CONTRACT
schema_id=${development_schema_id}.execution_contract.v1
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
cursor_alignment_erratum_verifier_sha256=$(sha256_of "${cursor_erratum_verifier}")
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_erratum_receipt}")
capture_runner_sha256=$(sha256_of "${capture_runner}")
frozen_affine_runner_sha256=$(sha256_of "${frozen_runner}")
frozen_affine_helper_sha256=$(sha256_of "${frozen_helper}")
$(emit_cuda_operational_bindings)
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
fit_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
certified_development_anchor_range=[${certified_begin},${certified_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_development_probe_rows=${certified_rows}
maximum_anchor_read=$((validation_end - 1))
certified_input_access=false
certified_candidates_scored=0
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
omp_threads=1
mkl_threads=1
CONTRACT
  assert_operational_runner_identity
}

emit_development_input_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "${capture_development}" "${source_closure}" \
      "${cursor_erratum_verifier}" "${cursor_erratum_receipt}" \
      "${capture_runner}" \
      "${train_probe}" "${validation_probe}" \
      "${raw96_train_probe}" "${raw96_validation_probe}" \
      "${untrained_train_probe}" "${untrained_validation_probe}" \
      "${fresh_preregistration}" "${diagnostic_preregistration}" \
      "${protocol_amendment}" "${localization_addendum}" \
      "${conditional_amendment}" "${ablation_preregistration}" \
      "${source_isolation_amendment}" "${isolated_source_protocol}" \
      "${staged_hardening}" "${cursor_alignment_correction}" \
      "${cursor_alignment_metadata_erratum}" execution_contract.status
  )
}

emit_source_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum frozen_sources/frozen_representation_affine_probe.cpp \
      frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh
  )
}

emit_development_output_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "main/${post384_development_file_id}.report" \
      "main/${post384_development_file_id}.stdout.log" \
      "main/${raw96_development_file_id}.report" \
      "main/${raw96_development_file_id}.stdout.log" \
      "main/${untrained_development_file_id}.report" \
      "main/${untrained_development_file_id}.stdout.log" \
      "replay/${post384_development_file_id}.report" \
      "replay/${post384_development_file_id}.stdout.log" \
      "replay/${raw96_development_file_id}.report" \
      "replay/${raw96_development_file_id}.stdout.log" \
      "replay/${untrained_development_file_id}.report" \
      "replay/${untrained_development_file_id}.stdout.log"
  )
}

emit_binary_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum bin/frozen_representation_affine_probe
  )
}

emit_master_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum input_manifest.sha256 source_manifest.sha256 binary.sha256 \
      output_manifest.sha256 compiler.identity linked_libraries.txt
  )
}

write_development_manifests() {
  local runtime="$1"
  assert_operational_runner_identity
  emit_development_input_manifest "${runtime}" >"${runtime}/input_manifest.sha256"
  emit_source_manifest "${runtime}" >"${runtime}/source_manifest.sha256"
  emit_binary_manifest "${runtime}" >"${runtime}/binary.sha256"
  emit_development_output_manifest "${runtime}" >"${runtime}/output_manifest.sha256"
  emit_master_manifest "${runtime}" >"${runtime}/master.sha256"
  assert_operational_runner_identity
}

verify_manifest_checksums() {
  local runtime="$1" manifest="$2"
  require_file "${runtime}/${manifest}"
  (
    cd "${runtime}"
    sha256sum -c -- "${manifest}" >/dev/null
  ) || fail "manifest checksum verification failed: ${runtime}/${manifest}"
}

verify_development_manifests() {
  local runtime="$1"
  cmp -s -- <(emit_development_input_manifest "${runtime}") \
    "${runtime}/input_manifest.sha256" ||
    fail "development input manifest differs from exact inventory"
  cmp -s -- <(emit_source_manifest "${runtime}") \
    "${runtime}/source_manifest.sha256" ||
    fail "development source manifest differs from exact inventory"
  cmp -s -- <(emit_binary_manifest "${runtime}") \
    "${runtime}/binary.sha256" ||
    fail "development binary manifest differs from exact inventory"
  cmp -s -- <(emit_development_output_manifest "${runtime}") \
    "${runtime}/output_manifest.sha256" ||
    fail "development output manifest differs from exact inventory"
  cmp -s -- <(emit_master_manifest "${runtime}") \
    "${runtime}/master.sha256" ||
    fail "development master manifest differs from exact six-entry inventory"
  local manifest
  for manifest in input_manifest.sha256 source_manifest.sha256 binary.sha256 \
    output_manifest.sha256 master.sha256; do
    verify_manifest_checksums "${runtime}" "${manifest}"
  done
}

emit_development_status() {
  local runtime="$1"
  cat <<STATUS
schema_id=${development_schema_id}
status=complete
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
capture_runner_path=${capture_runner}
capture_runner_sha256=$(sha256_of "${capture_runner}")
affine_runner_path=${frozen_runner}
affine_runner_sha256=$(sha256_of "${frozen_runner}")
affine_helper_path=${frozen_helper}
affine_helper_sha256=$(sha256_of "${frozen_helper}")
$(emit_cuda_operational_bindings)
affine_binary_path=${development_binary}
affine_binary_sha256=$(sha256_of "${runtime}/bin/frozen_representation_affine_probe")
affine_master_manifest_path=${development_master}
affine_master_manifest_sha256=$(sha256_of "${runtime}/master.sha256")
train_probe_path=${train_probe}
train_probe_sha256=$(sha256_of "${train_probe}")
validation_probe_path=${validation_probe}
validation_probe_sha256=$(sha256_of "${validation_probe}")
train_representation_probe_path=${raw96_train_probe}
train_representation_probe_sha256=$(sha256_of "${raw96_train_probe}")
validation_representation_probe_path=${raw96_validation_probe}
validation_representation_probe_sha256=$(sha256_of "${raw96_validation_probe}")
untrained_train_representation_probe_path=${untrained_train_probe}
untrained_train_representation_probe_sha256=$(sha256_of "${untrained_train_probe}")
untrained_validation_representation_probe_path=${untrained_validation_probe}
untrained_validation_representation_probe_sha256=$(sha256_of "${untrained_validation_probe}")
post384_validation_report_path=${post384_development_report}
post384_validation_report_sha256=$(sha256_of "${runtime}/main/${post384_development_file_id}.report")
raw96_validation_report_path=${raw96_development_report}
raw96_validation_report_sha256=$(sha256_of "${runtime}/main/${raw96_development_file_id}.report")
untrained_raw96_validation_report_path=${untrained_development_report}
untrained_raw96_validation_report_sha256=$(sha256_of "${runtime}/main/${untrained_development_file_id}.report")
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
certified_development_anchor_range=[${certified_begin},${certified_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_development_probe_rows=${certified_rows}
maximum_anchor_read=$((validation_end - 1))
certified_input_access=false
certified_candidates_scored=0
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
STATUS
}

write_development_status() {
  local runtime="$1"
  assert_operational_runner_identity
  emit_development_status "${runtime}" >"${runtime}/development.status"
  assert_operational_runner_identity
}

verify_development_status() {
  local runtime="$1"
  require_immutable_file "${runtime}/development.status"
  cmp -s -- <(emit_development_status "${runtime}") \
    "${runtime}/development.status" ||
    fail "development status differs from its complete fixed contract"
  expect_kv "${runtime}/development.status" schema_id "${development_schema_id}"
  expect_kv "${runtime}/development.status" status complete
  expect_kv "${runtime}/development.status" affine_master_manifest_path "${development_master}"
  expect_kv "${runtime}/development.status" maximum_anchor_read "$((validation_end - 1))"
  expect_kv "${runtime}/development.status" certified_input_access false
  expect_kv "${runtime}/development.status" certified_candidates_scored 0
  expect_kv "${runtime}/development.status" untrained_representation_certified_access false
  expect_kv "${runtime}/development.status" canonical_data_raw_access false
  expect_kv "${runtime}/development.status" final_holdout_access false
  expect_kv "${runtime}/development.status" final_holdout_scored false
  expect_kv "${runtime}/development.status" independent_final_evidence false
  expect_kv "${runtime}/development.status" policy_access false
  verify_cuda_operational_bindings "${runtime}/development.status"
  verify_cursor_erratum_bindings "${runtime}/development.status"
  verify_document_bindings "${runtime}/development.status"
}

assert_development_inventory() {
  local runtime="$1" path actual_count
  local -a expected=(
    bin/frozen_representation_affine_probe
    compiler.identity linked_libraries.txt execution_contract.status
    input_manifest.sha256 source_manifest.sha256 binary.sha256
    output_manifest.sha256 master.sha256 development.status
    frozen_sources/frozen_representation_affine_probe.cpp
    frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh
  )
  local branch file_id suffix
  for branch in main replay; do
    for file_id in "${post384_development_file_id}" \
      "${raw96_development_file_id}" "${untrained_development_file_id}"; do
      for suffix in report stdout.log; do
        expected+=("${branch}/${file_id}.${suffix}")
      done
    done
  done
  actual_count="$(find "${runtime}" -type f -print | wc -l | tr -d '[:space:]')"
  [[ "${actual_count}" == "${#expected[@]}" ]] ||
    fail "development runtime file count ${actual_count} differs from ${#expected[@]}"
  for path in "${expected[@]}"; do
    require_file "${runtime}/${path}"
  done
}

seal_runtime() {
  local runtime="$1" symlink special
  assert_operational_runner_identity
  symlink="$(find "${runtime}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "runtime contains symlink: ${symlink}"
  special="$(find "${runtime}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "runtime contains special entry: ${special}"
  find "${runtime}" -type f -exec chmod 0444 -- {} +
  chmod 0555 -- "${runtime}/bin/frozen_representation_affine_probe"
  find "${runtime}" -depth -type d -exec chmod 0555 -- {} +
  assert_operational_runner_identity
}

assert_runtime_sealed() {
  local runtime="$1" symlink special writable
  require_directory "${runtime}"
  symlink="$(find "${runtime}" -type l -print -quit)"
  [[ -z "${symlink}" ]] || fail "sealed runtime contains symlink: ${symlink}"
  special="$(find "${runtime}" -mindepth 1 ! -type d ! -type f -print -quit)"
  [[ -z "${special}" ]] || fail "sealed runtime contains special entry: ${special}"
  writable="$(find "${runtime}" -perm /222 -print -quit)"
  [[ -z "${writable}" ]] || fail "sealed runtime retains write permission: ${writable}"
  [[ -x "${runtime}/bin/frozen_representation_affine_probe" ]] ||
    fail "sealed affine binary is not executable"
}

verify_development_runtime() {
  assert_operational_runner_identity
  require_directory "${development_runtime}"
  assert_development_inventory "${development_runtime}"
  assert_runtime_sealed "${development_runtime}"
  verify_development_manifests "${development_runtime}"
  verify_cuda_operational_bindings \
    "${development_runtime}/execution_contract.status"
  verify_development_report_pairs "${development_runtime}"
  verify_development_status "${development_runtime}"
  cmp -s -- "${frozen_helper}" \
    "${development_runtime}/frozen_sources/frozen_representation_affine_probe.cpp" ||
    fail "development helper differs from capture-frozen helper"
  cmp -s -- "${frozen_runner}" \
    "${development_runtime}/frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh" ||
    fail "development runner differs from capture-frozen runner"
  grep -Fq libtorch_cpu.so "${development_runtime}/linked_libraries.txt" ||
    fail "sealed development binary lacks libtorch_cpu provenance"
  assert_operational_runner_identity
}

numeric_gate_boolean() {
  local value="$1" comparison="$2" threshold="$3"
  LC_ALL=C awk -v value="${value}" -v comparison="${comparison}" \
    -v threshold="${threshold}" '
    BEGIN {
      number = "^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$";
      if (value !~ number) exit 42;
      if (comparison == "ge")
        print (value + 0 >= threshold + 0) ? "true" : "false";
      else if (comparison == "le")
        print (value + 0 <= threshold + 0) ? "true" : "false";
      else exit 43;
    }
  ' || fail "invalid numeric gate input: ${value} ${comparison} ${threshold}"
}

load_raw96_route() {
  direction_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_development_report}" selected.validation.directional_accuracy)" ge 0.95)"
  rank_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_development_report}" selected.validation.pairwise_rank_accuracy)" ge 0.95)"
  correlation_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_development_report}" selected.validation.correlation)" ge 0.95)"
  rmse_gate="$(numeric_gate_boolean \
    "$(kv_value "${raw96_development_report}" selected.validation.rmse_target_rms_ratio)" le 0.25)"
  strong_gate=false
  if [[ "${direction_gate}" == true && "${rank_gate}" == true && \
    "${correlation_gate}" == true && "${rmse_gate}" == true ]]; then
    strong_gate=true
  fi
  expect_kv "${raw96_development_report}" validation_strong_gate_pass "${strong_gate}"
  if [[ "${strong_gate}" == true ]]; then
    expected_route=canonical_certification
  else
    expected_route=representation_ablation_screen
  fi
}

emit_route_trigger() {
  load_raw96_route
  cat <<TRIGGER
schema_id=${trigger_schema_id}
status=complete
route=${expected_route}
maximum_anchor_read=$((validation_end - 1))
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
capture_runner_path=${capture_runner}
capture_runner_sha256=$(sha256_of "${capture_runner}")
affine_runner_path=${frozen_runner}
affine_runner_sha256=$(sha256_of "${frozen_runner}")
affine_helper_path=${frozen_helper}
affine_helper_sha256=$(sha256_of "${frozen_helper}")
$(emit_cuda_operational_bindings)
affine_binary_path=${development_binary}
affine_binary_sha256=$(sha256_of "${development_binary}")
affine_development_receipt_path=${development_status}
affine_development_receipt_sha256=$(sha256_of "${development_status}")
affine_master_manifest_path=${development_master}
affine_master_manifest_sha256=$(sha256_of "${development_master}")
post384_validation_report_path=${post384_development_report}
post384_validation_report_sha256=$(sha256_of "${post384_development_report}")
raw96_validation_report_path=${raw96_development_report}
raw96_validation_report_sha256=$(sha256_of "${raw96_development_report}")
untrained_raw96_validation_report_path=${untrained_development_report}
untrained_raw96_validation_report_sha256=$(sha256_of "${untrained_development_report}")
raw96_validation_direction_gate_pass=${direction_gate}
raw96_validation_pairwise_rank_gate_pass=${rank_gate}
raw96_validation_correlation_gate_pass=${correlation_gate}
raw96_validation_rmse_ratio_gate_pass=${rmse_gate}
raw96_validation_strong_gate_pass=${strong_gate}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
certified_capture_opened=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
TRIGGER
}

verify_route_trigger() {
  assert_operational_runner_identity
  verify_development_runtime
  load_raw96_route
  require_immutable_file "${route_trigger}"
  [[ "$(stat -c '%a' -- "${route_trigger}")" == 444 ]] ||
    fail "route trigger mode is not exactly 0444"
  cmp -s -- <(emit_route_trigger) "${route_trigger}" ||
    fail "route trigger differs from the complete raw96-only fixed contract"
  expect_kv "${route_trigger}" schema_id "${trigger_schema_id}"
  expect_kv "${route_trigger}" status complete
  expect_kv "${route_trigger}" route "${expected_route}"
  expect_kv "${route_trigger}" affine_master_manifest_path "${development_master}"
  expect_kv "${route_trigger}" raw96_validation_strong_gate_pass "${strong_gate}"
  expect_kv "${route_trigger}" certified_capture_opened false
  expect_kv "${route_trigger}" canonical_data_raw_access false
  expect_kv "${route_trigger}" final_holdout_access false
  expect_kv "${route_trigger}" final_holdout_scored false
  expect_kv "${route_trigger}" independent_final_evidence false
  expect_kv "${route_trigger}" policy_access false
  verify_cuda_operational_bindings "${route_trigger}"
  verify_cursor_erratum_bindings "${route_trigger}"
  verify_document_bindings "${route_trigger}"
  assert_operational_runner_identity
}

publish_route_trigger() {
  assert_operational_runner_identity
  verify_development_runtime
  assert_operational_runner_identity
  require_directory "${capture_runtime}"
  local candidate
  assert_operational_runner_identity
  candidate="$(mktemp "${capture_runtime}/.affine_route_trigger.XXXXXX")"
  assert_operational_runner_identity
  emit_route_trigger >"${candidate}"
  assert_operational_runner_identity
  publish_immutable_candidate "${candidate}" "${route_trigger}" \
    "${capture_runtime}" "affine route trigger"
  assert_operational_runner_identity
  sync -f "${route_trigger}"
  sync -f "${capture_runtime}"
  assert_operational_runner_identity
  verify_route_trigger
  assert_operational_runner_identity
}

run_development() {
  assert_operational_runner_identity
  preflight_development
  assert_operational_runner_identity
  if [[ ! -e "${development_runtime}" && ! -L "${development_runtime}" ]]; then
    [[ ! -e "${route_trigger}" && ! -L "${route_trigger}" ]] ||
      fail "route trigger exists without its fixed development runtime"
    local runtime lock
    lock="${development_runtime}.lock"
    reject_symlink_components "${lock}"
    assert_operational_runner_identity
    mkdir -- "${lock}" || fail "another development affine run holds the lock"
    trap 'rmdir "'"${lock}"'" 2>/dev/null || true' EXIT
    assert_operational_runner_identity
    runtime="$(mktemp -d "${runtime_parent}/${development_schema_id}.candidate.XXXXXXXX")"
    assert_operational_runner_identity
    mkdir -p -- "${runtime}/frozen_sources" "${runtime}/main" "${runtime}/replay"
    cp -- "${frozen_helper}" \
      "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp"
    cp -- "${frozen_runner}" \
      "${runtime}/frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh"
    assert_operational_runner_identity
    verify_cuda_alias_contract
    assert_operational_runner_identity
    compile_helper "${runtime}"
    assert_operational_runner_identity
    verify_cuda_alias_contract
    assert_operational_runner_identity
    write_toolchain_provenance "${runtime}"
    assert_operational_runner_identity
    write_development_contract "${runtime}"
    assert_operational_runner_identity
    run_development_reports "${runtime}"
    assert_operational_runner_identity
    verify_development_report_pairs "${runtime}"
    assert_operational_runner_identity
    write_development_manifests "${runtime}"
    assert_operational_runner_identity
    verify_development_manifests "${runtime}"
    assert_operational_runner_identity
    "${cursor_erratum_verifier}" --verify >/dev/null
    assert_operational_runner_identity
    write_development_status "${runtime}"
    assert_operational_runner_identity
    assert_development_inventory "${runtime}"
    assert_operational_runner_identity
    seal_runtime "${runtime}"
    assert_operational_runner_identity
    mv -T -n -- "${runtime}" "${development_runtime}" ||
      fail "atomic development runtime installation failed; candidate retained at ${runtime}"
    assert_operational_runner_identity
    rmdir -- "${lock}"
    trap - EXIT
    assert_operational_runner_identity
  fi
  assert_operational_runner_identity
  verify_development_runtime
  assert_operational_runner_identity
  "${cursor_erratum_verifier}" --verify >/dev/null
  assert_operational_runner_identity
  publish_route_trigger
  assert_operational_runner_identity
  "${capture_runner}" --verify-development >/dev/null
  assert_operational_runner_identity
  echo "development_status=${development_status}"
  echo "development_status_sha256=$(sha256_of "${development_status}")"
  echo "route_trigger=${route_trigger}"
  echo "route=$(kv_value "${route_trigger}" route)"
  assert_operational_runner_identity
}

capture_result_bound_probe() {
  local key="$1" expected="$2"
  expect_kv "${capture_result}" "${key}_path" "${expected}"
  require_immutable_file "${expected}"
  expect_kv "${capture_result}" "${key}_sha256" "$(sha256_of "${expected}")"
  printf '%s' "${expected}"
}

verify_capture_result_contract() {
  require_immutable_file "${capture_result}"
  expect_kv "${capture_result}" schema_id "${capture_schema_id}.result.v1"
  expect_kv "${capture_result}" status complete
  expect_kv "${capture_result}" route canonical_certification
  expect_kv "${capture_result}" runner_path "${capture_runner}"
  expect_kv "${capture_result}" runner_sha256 "$(sha256_of "${capture_runner}")"
  expect_kv "${capture_result}" development_receipt_path "${capture_development}"
  expect_kv "${capture_result}" development_receipt_sha256 "$(sha256_of "${capture_development}")"
  expect_kv "${capture_result}" affine_route_trigger_path "${route_trigger}"
  expect_kv "${capture_result}" affine_route_trigger_sha256 "$(sha256_of "${route_trigger}")"
  expect_kv "${capture_result}" isolated_source_closure_path "${source_closure}"
  expect_kv "${capture_result}" isolated_source_closure_sha256 "${expected_source_closure_sha256}"
  verify_cursor_erratum_bindings "${capture_result}"
  expect_kv "${capture_result}" accepted_anchor_count "${accepted_anchor_count}"
  expect_kv "${capture_result}" candidate_anchor_count "${accepted_anchor_count}"
  expect_kv "${capture_result}" maximum_available_anchor "${maximum_anchor_index}"
  expect_kv "${capture_result}" train_capture_range "[${train_begin},${train_end})"
  expect_kv "${capture_result}" validation_capture_range "[${validation_begin},${validation_end})"
  expect_kv "${capture_result}" certified_capture_range "[${certified_begin},${certified_end})"
  expect_kv "${capture_result}" train_probe_rows "${train_rows}"
  expect_kv "${capture_result}" validation_probe_rows "${validation_rows}"
  expect_kv "${capture_result}" certified_probe_rows "${certified_rows}"
  expect_kv "${capture_result}" maximum_anchor_scored "${maximum_anchor_index}"
  expect_kv "${capture_result}" purge_anchors_captured false
  expect_kv "${capture_result}" certified_scoring_attempt_count 1
  expect_kv "${capture_result}" untrained_representation_certified_access false
  expect_kv "${capture_result}" canonical_data_raw_access false
  expect_kv "${capture_result}" final_holdout_access false
  expect_kv "${capture_result}" final_holdout_scored false
  expect_kv "${capture_result}" independent_final_evidence false
  expect_kv "${capture_result}" policy_access false
  verify_document_bindings "${capture_result}"
}

load_certified_probes() {
  certified_probe="$(capture_result_bound_probe certified_context_probe \
    "${capture_certified_job}/mdn_edge_context_features.probe")"
  raw96_certified_probe="$(capture_result_bound_probe certified_representation_probe \
    "${capture_certified_job}/representation_edge_features.probe")"
}

preflight_certified() {
  assert_operational_runner_identity
  verify_common_static_inputs
  verify_frozen_sources
  verify_capture_development_contract
  load_development_probes
  verify_development_runtime
  verify_route_trigger
  [[ "${expected_route}" == canonical_certification && \
    "${strong_gate}" == true ]] ||
    fail "certified affine scoring is forbidden by route=${expected_route}"
  assert_operational_runner_identity
  "${capture_runner}" --verify >/dev/null
  assert_operational_runner_identity
  verify_capture_result_contract
  load_certified_probes
  assert_operational_runner_identity
}

selection_lines() {
  awk '
    /^candidate\.[0-9]+\.(ridge|numerically_valid|rejection_reason|maximum_normalized_residual|coefficient_l2_norm)=/ { print; next }
    /^candidate\.[0-9]+\.validation\./ { print; next }
    /^selected_candidate_index=/ { print; next }
    /^selected_ridge=/ { print; next }
    /^selected_maximum_normalized_residual=/ { print; next }
    /^selected_coefficient_l2_norm=/ { print; next }
    /^selected\.train\./ { print; next }
    /^selected\.validation\./ { print; next }
    /^validation_(strong|partial)_gate_pass=/ { print; next }
  ' "$1"
}

assert_selection_unchanged() {
  local development_report="$1" certified_report="$2"
  cmp -s -- <(selection_lines "${development_report}") \
    <(selection_lines "${certified_report}") ||
    fail "certified execution changed its locked development selection"
}

run_certified_reports() {
  local runtime="$1" branch
  assert_operational_runner_identity
  export OMP_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  for branch in main replay; do
    assert_operational_runner_identity
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind mdn_context \
      --train-input "${train_probe}" \
      --validation-input "${validation_probe}" \
      --selection-lock "${post384_development_report}" \
      --certified-input "${certified_probe}" \
      --output "${runtime}/${branch}/${post384_certified_file_id}.report" \
      >"${runtime}/${branch}/${post384_certified_file_id}.stdout.log" 2>&1
    assert_operational_runner_identity
    "${runtime}/bin/frozen_representation_affine_probe" \
      --probe-kind representation \
      --train-input "${raw96_train_probe}" \
      --validation-input "${raw96_validation_probe}" \
      --selection-lock "${raw96_development_report}" \
      --certified-input "${raw96_certified_probe}" \
      --output "${runtime}/${branch}/${raw96_certified_file_id}.report" \
      >"${runtime}/${branch}/${raw96_certified_file_id}.stdout.log" 2>&1
    assert_operational_runner_identity
    cp -- "${development_runtime}/${branch}/${untrained_development_file_id}.report" \
      "${runtime}/${branch}/${untrained_development_file_id}.report"
    cp -- "${development_runtime}/${branch}/${untrained_development_file_id}.stdout.log" \
      "${runtime}/${branch}/${untrained_development_file_id}.stdout.log"
    assert_operational_runner_identity
  done
  assert_operational_runner_identity
}

verify_certified_report_pairs() {
  local runtime="$1" file
  for file in \
    "${post384_certified_file_id}.report" \
    "${post384_certified_file_id}.stdout.log" \
    "${raw96_certified_file_id}.report" \
    "${raw96_certified_file_id}.stdout.log" \
    "${untrained_development_file_id}.report" \
    "${untrained_development_file_id}.stdout.log"; do
    require_file "${runtime}/main/${file}"
    require_file "${runtime}/replay/${file}"
    cmp -s -- "${runtime}/main/${file}" "${runtime}/replay/${file}" ||
      fail "certified main/replay differ: ${file}"
  done
  validate_affine_report \
    "${runtime}/main/${post384_certified_file_id}.report" \
    "${post384_certified_schema_id}" mdn_context "${certified_rows}" \
    "${maximum_anchor_index}" certified "${post384_development_report}"
  validate_affine_report \
    "${runtime}/main/${raw96_certified_file_id}.report" \
    "${raw96_certified_schema_id}" representation "${certified_rows}" \
    "${maximum_anchor_index}" certified "${raw96_development_report}"
  validate_untrained_report \
    "${runtime}/main/${untrained_development_file_id}.report"
  cmp -s -- "${development_runtime}/main/${untrained_development_file_id}.report" \
    "${runtime}/main/${untrained_development_file_id}.report" ||
    fail "untrained control changed in certified runtime"
  assert_selection_unchanged "${post384_development_report}" \
    "${runtime}/main/${post384_certified_file_id}.report"
  assert_selection_unchanged "${raw96_development_report}" \
    "${runtime}/main/${raw96_certified_file_id}.report"
}

write_certified_contract() {
  local runtime="$1"
  assert_operational_runner_identity
  cat >"${runtime}/execution_contract.status" <<CONTRACT
schema_id=${schema_id}.execution_contract.v1
route=canonical_certification
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
capture_result_path=${capture_result}
capture_result_sha256=$(sha256_of "${capture_result}")
affine_route_trigger_path=${route_trigger}
affine_route_trigger_sha256=$(sha256_of "${route_trigger}")
development_affine_status_path=${development_status}
development_affine_status_sha256=$(sha256_of "${development_status}")
development_affine_master_manifest_path=${development_master}
development_affine_master_manifest_sha256=$(sha256_of "${development_master}")
post384_selection_lock_path=${post384_development_report}
post384_selection_lock_sha256=$(sha256_of "${post384_development_report}")
raw96_selection_lock_path=${raw96_development_report}
raw96_selection_lock_sha256=$(sha256_of "${raw96_development_report}")
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
cursor_alignment_erratum_verifier_sha256=$(sha256_of "${cursor_erratum_verifier}")
cursor_alignment_erratum_receipt_sha256=$(sha256_of "${cursor_erratum_receipt}")
frozen_affine_runner_sha256=$(sha256_of "${frozen_runner}")
frozen_affine_helper_sha256=$(sha256_of "${frozen_helper}")
$(emit_cuda_operational_bindings)
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
fit_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
certified_development_anchor_range=[${certified_begin},${certified_end})
certified_development_probe_rows=${certified_rows}
maximum_anchor_read=${maximum_anchor_index}
certified_trained_arms=2
certified_candidates_scored_per_trained_arm=1
untrained_certified_candidates_scored=0
development_selection_reused=true
selection_lock_verified=true
refit_after_development_selection=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
omp_threads=1
mkl_threads=1
CONTRACT
  assert_operational_runner_identity
}

emit_final_input_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "${capture_development}" "${capture_result}" "${route_trigger}" \
      "${source_closure}" "${cursor_erratum_verifier}" \
      "${cursor_erratum_receipt}" "${capture_runner}" \
      "${development_status}" "${development_master}" \
      "${post384_development_report}" "${raw96_development_report}" \
      "${untrained_development_report}" \
      "${train_probe}" "${validation_probe}" \
      "${raw96_train_probe}" "${raw96_validation_probe}" \
      "${untrained_train_probe}" "${untrained_validation_probe}" \
      "${certified_probe}" "${raw96_certified_probe}" \
      "${fresh_preregistration}" "${diagnostic_preregistration}" \
      "${protocol_amendment}" "${localization_addendum}" \
      "${conditional_amendment}" \
      "${ablation_preregistration}" "${source_isolation_amendment}" \
      "${isolated_source_protocol}" "${staged_hardening}" \
      "${cursor_alignment_correction}" \
      "${cursor_alignment_metadata_erratum}" execution_contract.status
  )
}

emit_final_output_manifest() {
  local runtime="$1"
  (
    cd "${runtime}"
    sha256sum \
      "main/${post384_certified_file_id}.report" \
      "main/${post384_certified_file_id}.stdout.log" \
      "main/${raw96_certified_file_id}.report" \
      "main/${raw96_certified_file_id}.stdout.log" \
      "main/${untrained_development_file_id}.report" \
      "main/${untrained_development_file_id}.stdout.log" \
      "replay/${post384_certified_file_id}.report" \
      "replay/${post384_certified_file_id}.stdout.log" \
      "replay/${raw96_certified_file_id}.report" \
      "replay/${raw96_certified_file_id}.stdout.log" \
      "replay/${untrained_development_file_id}.report" \
      "replay/${untrained_development_file_id}.stdout.log"
  )
}

write_final_manifests() {
  local runtime="$1"
  assert_operational_runner_identity
  emit_final_input_manifest "${runtime}" >"${runtime}/input_manifest.sha256"
  emit_source_manifest "${runtime}" >"${runtime}/source_manifest.sha256"
  emit_binary_manifest "${runtime}" >"${runtime}/binary.sha256"
  emit_final_output_manifest "${runtime}" >"${runtime}/output_manifest.sha256"
  emit_master_manifest "${runtime}" >"${runtime}/master.sha256"
  assert_operational_runner_identity
}

verify_final_manifests() {
  local runtime="$1" manifest
  cmp -s -- <(emit_final_input_manifest "${runtime}") \
    "${runtime}/input_manifest.sha256" ||
    fail "certified input manifest differs from exact inventory"
  cmp -s -- <(emit_source_manifest "${runtime}") \
    "${runtime}/source_manifest.sha256" ||
    fail "certified source manifest differs from exact inventory"
  cmp -s -- <(emit_binary_manifest "${runtime}") \
    "${runtime}/binary.sha256" ||
    fail "certified binary manifest differs from exact inventory"
  cmp -s -- <(emit_final_output_manifest "${runtime}") \
    "${runtime}/output_manifest.sha256" ||
    fail "certified output manifest differs from exact inventory"
  cmp -s -- <(emit_master_manifest "${runtime}") \
    "${runtime}/master.sha256" ||
    fail "certified master manifest differs from exact six-entry inventory"
  for manifest in input_manifest.sha256 source_manifest.sha256 binary.sha256 \
    output_manifest.sha256 master.sha256; do
    verify_manifest_checksums "${runtime}" "${manifest}"
  done
}

emit_final_status() {
  local runtime="$1"
  cat <<STATUS
schema_id=${result_schema_id}
status=complete
route=canonical_certification
capture_development_path=${capture_development}
capture_development_sha256=$(sha256_of "${capture_development}")
capture_result_path=${capture_result}
capture_result_sha256=$(sha256_of "${capture_result}")
affine_route_trigger_path=${route_trigger}
affine_route_trigger_sha256=$(sha256_of "${route_trigger}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
$(emit_cursor_erratum_bindings)
capture_runner_path=${capture_runner}
capture_runner_sha256=$(sha256_of "${capture_runner}")
affine_runner_path=${frozen_runner}
affine_runner_sha256=$(sha256_of "${frozen_runner}")
affine_helper_path=${frozen_helper}
affine_helper_sha256=$(sha256_of "${frozen_helper}")
$(emit_cuda_operational_bindings)
development_affine_status_path=${development_status}
development_affine_status_sha256=$(sha256_of "${development_status}")
development_affine_master_manifest_path=${development_master}
development_affine_master_manifest_sha256=$(sha256_of "${development_master}")
post384_selection_lock_path=${post384_development_report}
post384_selection_lock_sha256=$(sha256_of "${post384_development_report}")
raw96_selection_lock_path=${raw96_development_report}
raw96_selection_lock_sha256=$(sha256_of "${raw96_development_report}")
certified_context_probe_path=${certified_probe}
certified_context_probe_sha256=$(sha256_of "${certified_probe}")
certified_representation_probe_path=${raw96_certified_probe}
certified_representation_probe_sha256=$(sha256_of "${raw96_certified_probe}")
affine_binary_path=${final_binary}
affine_binary_sha256=$(sha256_of "${runtime}/bin/frozen_representation_affine_probe")
affine_master_manifest_path=${final_master}
affine_master_manifest_sha256=$(sha256_of "${runtime}/master.sha256")
post384_certified_report_path=${post384_certified_report}
post384_certified_report_sha256=$(sha256_of "${runtime}/main/${post384_certified_file_id}.report")
raw96_certified_report_path=${raw96_certified_report}
raw96_certified_report_sha256=$(sha256_of "${runtime}/main/${raw96_certified_file_id}.report")
untrained_raw96_report_path=${untrained_certified_report}
untrained_raw96_report_sha256=$(sha256_of "${runtime}/main/${untrained_development_file_id}.report")
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_available_anchor=${maximum_anchor_index}
train_anchor_range=[${train_begin},${train_end})
validation_anchor_range=[${validation_begin},${validation_end})
certified_development_anchor_range=[${certified_begin},${certified_end})
train_probe_rows=${train_rows}
validation_probe_rows=${validation_rows}
certified_development_probe_rows=${certified_rows}
maximum_anchor_read=${maximum_anchor_index}
certified_scoring_attempt_count=1
development_selection_reused=true
selection_lock_verified=true
refit_after_development_selection=false
untrained_representation_certified_access=false
canonical_data_raw_access=false
final_holdout_access=false
final_holdout_scored=false
independent_final_evidence=false
policy_access=false
$(emit_document_bindings)
STATUS
}

write_final_status() {
  local runtime="$1"
  assert_operational_runner_identity
  emit_final_status "${runtime}" >"${runtime}/result.status"
  assert_operational_runner_identity
}

verify_final_status() {
  local runtime="$1"
  require_immutable_file "${runtime}/result.status"
  cmp -s -- <(emit_final_status "${runtime}") "${runtime}/result.status" ||
    fail "certified result differs from its complete fixed contract"
  expect_kv "${runtime}/result.status" schema_id "${result_schema_id}"
  expect_kv "${runtime}/result.status" status complete
  expect_kv "${runtime}/result.status" route canonical_certification
  expect_kv "${runtime}/result.status" affine_route_trigger_path "${route_trigger}"
  expect_kv "${runtime}/result.status" development_affine_master_manifest_path "${development_master}"
  expect_kv "${runtime}/result.status" affine_master_manifest_path "${final_master}"
  expect_kv "${runtime}/result.status" maximum_anchor_read "${maximum_anchor_index}"
  expect_kv "${runtime}/result.status" selection_lock_verified true
  expect_kv "${runtime}/result.status" untrained_representation_certified_access false
  expect_kv "${runtime}/result.status" canonical_data_raw_access false
  expect_kv "${runtime}/result.status" final_holdout_access false
  expect_kv "${runtime}/result.status" final_holdout_scored false
  expect_kv "${runtime}/result.status" independent_final_evidence false
  expect_kv "${runtime}/result.status" policy_access false
  verify_cuda_operational_bindings "${runtime}/result.status"
  verify_cursor_erratum_bindings "${runtime}/result.status"
  verify_document_bindings "${runtime}/result.status"
}

assert_final_inventory() {
  local runtime="$1" path actual_count branch file_id suffix
  local -a expected=(
    bin/frozen_representation_affine_probe
    compiler.identity linked_libraries.txt execution_contract.status
    input_manifest.sha256 source_manifest.sha256 binary.sha256
    output_manifest.sha256 master.sha256 result.status
    frozen_sources/frozen_representation_affine_probe.cpp
    frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh
  )
  for branch in main replay; do
    for file_id in "${post384_certified_file_id}" \
      "${raw96_certified_file_id}" "${untrained_development_file_id}"; do
      for suffix in report stdout.log; do
        expected+=("${branch}/${file_id}.${suffix}")
      done
    done
  done
  actual_count="$(find "${runtime}" -type f -print | wc -l | tr -d '[:space:]')"
  [[ "${actual_count}" == "${#expected[@]}" ]] ||
    fail "certified runtime file count ${actual_count} differs from ${#expected[@]}"
  for path in "${expected[@]}"; do
    require_file "${runtime}/${path}"
  done
}

verify_final_runtime() {
  assert_operational_runner_identity
  require_directory "${final_runtime}"
  assert_final_inventory "${final_runtime}"
  assert_runtime_sealed "${final_runtime}"
  verify_final_manifests "${final_runtime}"
  verify_cuda_operational_bindings \
    "${final_runtime}/execution_contract.status"
  verify_certified_report_pairs "${final_runtime}"
  verify_final_status "${final_runtime}"
  cmp -s -- "${frozen_helper}" \
    "${final_runtime}/frozen_sources/frozen_representation_affine_probe.cpp" ||
    fail "certified helper differs from capture-frozen helper"
  cmp -s -- "${frozen_runner}" \
    "${final_runtime}/frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh" ||
    fail "certified runner differs from capture-frozen runner"
  grep -Fq libtorch_cpu.so "${final_runtime}/linked_libraries.txt" ||
    fail "sealed certified binary lacks libtorch_cpu provenance"
  assert_operational_runner_identity
}

run_certified() {
  assert_operational_runner_identity
  preflight_certified
  assert_operational_runner_identity
  if [[ ! -e "${final_runtime}" && ! -L "${final_runtime}" ]]; then
    local runtime lock
    lock="${final_runtime}.lock"
    reject_symlink_components "${lock}"
    assert_operational_runner_identity
    mkdir -- "${lock}" || fail "another certified affine run holds the lock"
    trap 'rmdir "'"${lock}"'" 2>/dev/null || true' EXIT
    assert_operational_runner_identity
    runtime="$(mktemp -d "${runtime_parent}/${schema_id}.candidate.XXXXXXXX")"
    assert_operational_runner_identity
    mkdir -p -- "${runtime}/frozen_sources" "${runtime}/main" "${runtime}/replay"
    cp -- "${frozen_helper}" \
      "${runtime}/frozen_sources/frozen_representation_affine_probe.cpp"
    cp -- "${frozen_runner}" \
      "${runtime}/frozen_sources/run_frozen_representation_affine_probe_isolated_v2.sh"
    assert_operational_runner_identity
    verify_cuda_alias_contract
    assert_operational_runner_identity
    compile_helper "${runtime}"
    assert_operational_runner_identity
    verify_cuda_alias_contract
    assert_operational_runner_identity
    write_toolchain_provenance "${runtime}"
    assert_operational_runner_identity
    write_certified_contract "${runtime}"
    assert_operational_runner_identity
    run_certified_reports "${runtime}"
    assert_operational_runner_identity
    verify_certified_report_pairs "${runtime}"
    assert_operational_runner_identity
    write_final_manifests "${runtime}"
    assert_operational_runner_identity
    verify_final_manifests "${runtime}"
    assert_operational_runner_identity
    "${cursor_erratum_verifier}" --verify >/dev/null
    assert_operational_runner_identity
    write_final_status "${runtime}"
    assert_operational_runner_identity
    assert_final_inventory "${runtime}"
    assert_operational_runner_identity
    seal_runtime "${runtime}"
    assert_operational_runner_identity
    mv -T -n -- "${runtime}" "${final_runtime}" ||
      fail "atomic certified runtime installation failed; candidate retained at ${runtime}"
    assert_operational_runner_identity
    rmdir -- "${lock}"
    trap - EXIT
    assert_operational_runner_identity
  fi
  assert_operational_runner_identity
  verify_final_runtime
  assert_operational_runner_identity
  "${cursor_erratum_verifier}" --verify >/dev/null
  assert_operational_runner_identity
  echo "result_status=${final_status}"
  echo "result_status_sha256=$(sha256_of "${final_status}")"
  echo "post384_report=${post384_certified_report}"
  echo "raw96_report=${raw96_certified_report}"
  assert_operational_runner_identity
}

assert_operational_runner_identity
((certified_end <= accepted_anchor_count)) ||
  fail "certified-development range exceeds the corrected source domain"
((certified_end <= forbidden_final_begin)) ||
  fail "certified-development range crosses the forbidden final boundary"

case "${mode}" in
--preflight)
  preflight_development
  echo "preflight=development_inputs_valid"
  ;;
--run-development)
  run_development
  ;;
--verify-development)
  preflight_development
  verify_development_runtime
  verify_route_trigger
  echo "development_status=${development_status}"
  echo "route=$(kv_value "${route_trigger}" route)"
  ;;
--run-certified)
  run_certified
  ;;
--verify)
  preflight_certified
  verify_final_runtime
  echo "result_status=${final_status}"
  echo "result_status_sha256=$(sha256_of "${final_status}")"
  ;;
*) fail "unreachable mode: ${mode}" ;;
esac

assert_operational_runner_identity
