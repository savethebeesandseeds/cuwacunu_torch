#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_isolated_development_source_v1.cursor_alignment_erratum.v1"
readonly expected_source_closure_sha256="0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7"
readonly expected_source_verifier_sha256="dca034ec2440c7ab9caa936dee965879fe4cbd48ca29fdd6432e62f73af1cf05"
readonly expected_original_correction_sha256="132135e874df78f52363bc544d2c7df339648301d1ce736ee9f83493d3a1114d"
readonly expected_metadata_erratum_sha256="88579f046fe953093fac813df1b0309bb8724fb460d321603fb7f821660cacd6"
readonly accepted_anchor_count=3261
readonly maximum_anchor_index=3260
readonly first_anchor_master_day_index=29
readonly last_anchor_master_day_index=3289
readonly last_required_coarse_boundary_master_day_index=3290
readonly first_anchor_key=1896047999999
readonly last_anchor_key=2177711999999

fail() {
  echo "synthetic v2 cursor-alignment erratum: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

reject_symlink_components() {
  local path="$1"
  [[ "${path}" == /* ]] || fail "path is not absolute: ${path}"
  local current="/" rest component
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
    if [[ "${current}" == "/" ]]; then
      current="/${component}"
    else
      current="${current}/${component}"
    fi
    [[ ! -L "${current}" ]] ||
      fail "path contains a symbolic-link component: ${current}"
  done
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

kv() {
  local path="$1"
  local key="$2"
  local count value
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
    fail "${path}: expected exactly one ${key}= entry, found ${count}"
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
  local path="$1"
  local key="$2"
  local expected="$3"
  local actual
  actual="$(kv "${path}" "${key}")"
  [[ "${actual}" == "${expected}" ]] ||
    fail "${path}: expected ${key}=${expected}, found ${actual}"
}

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v2"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1"
source_verifier="${script_dir}/prepare_and_seal_isolated_development_source_v2.sh"
source_closure="${runtime_root}/development_source_closure.status"
job_manifest="${runtime_root}/job/isolated_development_cache_build_dry_run/job.manifest"
original_correction="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md"
metadata_erratum="${benchmark_root}/DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md"
erratum_receipt="${runtime_root}/cursor_alignment_erratum.status"

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--seal|--verify]"
case "${mode}" in
--plan | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
source_closure_path=${source_closure}
source_closure_retained_immutable=true
source_cursor_first_anchor_master_day_index=${first_anchor_master_day_index}
source_cursor_last_anchor_master_day_index=${last_anchor_master_day_index}
source_cursor_last_required_coarse_boundary_master_day_index=${last_required_coarse_boundary_master_day_index}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_anchor_index=${maximum_anchor_index}
certified_development_range=[2880,3261)
canonical_data_raw_access=false
independent_final_evidence=false
PLAN
}

verify_static_chain() {
  local path cursor_token
  for path in "${source_verifier}" "${source_closure}" "${job_manifest}" \
    "${original_correction}" "${metadata_erratum}"; do
    reject_symlink_components "${path}"
    require_file "${path}"
  done
  [[ "$(sha256_of "${source_verifier}")" == \
    "${expected_source_verifier_sha256}" ]] ||
    fail "isolated-source verifier hash drifted"
  [[ "$(sha256_of "${source_closure}")" == \
    "${expected_source_closure_sha256}" ]] ||
    fail "isolated-source closure hash drifted"
  [[ "$(sha256_of "${original_correction}")" == \
    "${expected_original_correction_sha256}" ]] ||
    fail "original cursor correction hash drifted"
  [[ "$(sha256_of "${metadata_erratum}")" == \
    "${expected_metadata_erratum_sha256}" ]] ||
    fail "cursor metadata erratum hash drifted"
  "${source_verifier}" --verify >/dev/null
  expect_kv "${source_closure}" schema_id \
    synthetic_v2_isolated_development_source_v1
  expect_kv "${source_closure}" status complete
  expect_kv "${source_closure}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_closure}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${source_closure}" maximum_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${source_closure}" source_cursor_first_master_day_index 29
  expect_kv "${source_closure}" source_cursor_last_master_day_index 3290
  expect_kv "${source_closure}" skipped_outside_common_range 0
  expect_kv "${source_closure}" skipped_missing_edge_coverage 0
  expect_kv "${source_closure}" skipped_failed_fetch_probe 0
  expect_kv "${source_closure}" duplicate_anchor_count 0
  expect_kv "${source_closure}" cursor_alignment_correction_path \
    "${original_correction}"
  expect_kv "${source_closure}" cursor_alignment_correction_sha256 \
    "${expected_original_correction_sha256}"
  expect_kv "${source_closure}" canonical_data_raw_access false
  expect_kv "${source_closure}" final_holdout_available false
  cursor_token="$(kv "${job_manifest}" source_cursor_token)"
  [[ "${cursor_token}" == *"|Hx=30|Hf=1|"* &&
    "${cursor_token}" == *"|accepted=${accepted_anchor_count}|"* &&
    "${cursor_token}" == *"|candidates=${accepted_anchor_count}|"* &&
    "${cursor_token}" == *"|first=${first_anchor_key}|"* &&
    "${cursor_token}" == *"|last=${last_anchor_key}" ]] ||
    fail "production cursor token disagrees with metadata erratum"
}

verify_exact_receipt_keys() {
  local line key
  while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ "${line}" == *"="* ]] ||
      fail "cursor erratum receipt contains a malformed line"
    key="${line%%=*}"
    case "${key}" in
    schema_id | status | erratum_verifier_path | erratum_verifier_sha256 | \
      isolated_source_verifier_path | isolated_source_verifier_sha256 | \
      isolated_source_closure_path | isolated_source_closure_sha256 | \
      original_cursor_correction_path | original_cursor_correction_sha256 | \
      cursor_alignment_metadata_erratum_path | \
      cursor_alignment_metadata_erratum_sha256 | dry_run_job_manifest_path | \
      dry_run_job_manifest_sha256 | \
      source_cursor_first_anchor_master_day_index | \
      source_cursor_last_anchor_master_day_index | \
      source_cursor_last_required_coarse_boundary_master_day_index | \
      source_cursor_first_anchor_key | source_cursor_last_anchor_key | \
      accepted_anchor_count | candidate_anchor_count | maximum_anchor_index | \
      train_anchor_range | validation_anchor_range | \
      certified_development_anchor_range | certified_development_probe_rows | \
      canonical_data_raw_access | final_holdout_available | \
      independent_final_evidence) ;;
    *) fail "cursor erratum receipt contains an unknown key: ${key}" ;;
    esac
  done <"${erratum_receipt}"
}

emit_receipt() {
  local destination="$1"
  cat >"${destination}" <<RECEIPT
schema_id=${schema_id}
status=complete
erratum_verifier_path=${script_path}
erratum_verifier_sha256=$(sha256_of "${script_path}")
isolated_source_verifier_path=${source_verifier}
isolated_source_verifier_sha256=$(sha256_of "${source_verifier}")
isolated_source_closure_path=${source_closure}
isolated_source_closure_sha256=$(sha256_of "${source_closure}")
original_cursor_correction_path=${original_correction}
original_cursor_correction_sha256=$(sha256_of "${original_correction}")
cursor_alignment_metadata_erratum_path=${metadata_erratum}
cursor_alignment_metadata_erratum_sha256=$(sha256_of "${metadata_erratum}")
dry_run_job_manifest_path=${job_manifest}
dry_run_job_manifest_sha256=$(sha256_of "${job_manifest}")
source_cursor_first_anchor_master_day_index=${first_anchor_master_day_index}
source_cursor_last_anchor_master_day_index=${last_anchor_master_day_index}
source_cursor_last_required_coarse_boundary_master_day_index=${last_required_coarse_boundary_master_day_index}
source_cursor_first_anchor_key=${first_anchor_key}
source_cursor_last_anchor_key=${last_anchor_key}
accepted_anchor_count=${accepted_anchor_count}
candidate_anchor_count=${accepted_anchor_count}
maximum_anchor_index=${maximum_anchor_index}
train_anchor_range=[0,2496)
validation_anchor_range=[2560,2816)
certified_development_anchor_range=[2880,3261)
certified_development_probe_rows=3429
canonical_data_raw_access=false
final_holdout_available=false
independent_final_evidence=false
RECEIPT
}

verify_receipt() {
  verify_static_chain
  reject_symlink_components "${erratum_receipt}"
  require_file "${erratum_receipt}"
  [[ "$(stat -c '%A' -- "${erratum_receipt}")" != *w* ]] ||
    fail "cursor erratum receipt is writable"
  verify_exact_receipt_keys
  expect_kv "${erratum_receipt}" schema_id "${schema_id}"
  expect_kv "${erratum_receipt}" status complete
  expect_kv "${erratum_receipt}" erratum_verifier_path "${script_path}"
  expect_kv "${erratum_receipt}" erratum_verifier_sha256 \
    "$(sha256_of "${script_path}")"
  expect_kv "${erratum_receipt}" isolated_source_verifier_path \
    "${source_verifier}"
  expect_kv "${erratum_receipt}" isolated_source_verifier_sha256 \
    "${expected_source_verifier_sha256}"
  expect_kv "${erratum_receipt}" isolated_source_closure_path \
    "${source_closure}"
  expect_kv "${erratum_receipt}" isolated_source_closure_sha256 \
    "${expected_source_closure_sha256}"
  expect_kv "${erratum_receipt}" original_cursor_correction_path \
    "${original_correction}"
  expect_kv "${erratum_receipt}" original_cursor_correction_sha256 \
    "${expected_original_correction_sha256}"
  expect_kv "${erratum_receipt}" cursor_alignment_metadata_erratum_path \
    "${metadata_erratum}"
  expect_kv "${erratum_receipt}" cursor_alignment_metadata_erratum_sha256 \
    "${expected_metadata_erratum_sha256}"
  expect_kv "${erratum_receipt}" dry_run_job_manifest_path "${job_manifest}"
  expect_kv "${erratum_receipt}" dry_run_job_manifest_sha256 \
    "$(sha256_of "${job_manifest}")"
  expect_kv "${erratum_receipt}" source_cursor_first_anchor_master_day_index \
    "${first_anchor_master_day_index}"
  expect_kv "${erratum_receipt}" source_cursor_last_anchor_master_day_index \
    "${last_anchor_master_day_index}"
  expect_kv "${erratum_receipt}" source_cursor_last_required_coarse_boundary_master_day_index \
    "${last_required_coarse_boundary_master_day_index}"
  expect_kv "${erratum_receipt}" source_cursor_first_anchor_key \
    "${first_anchor_key}"
  expect_kv "${erratum_receipt}" source_cursor_last_anchor_key \
    "${last_anchor_key}"
  expect_kv "${erratum_receipt}" accepted_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${erratum_receipt}" candidate_anchor_count \
    "${accepted_anchor_count}"
  expect_kv "${erratum_receipt}" maximum_anchor_index \
    "${maximum_anchor_index}"
  expect_kv "${erratum_receipt}" train_anchor_range '[0,2496)'
  expect_kv "${erratum_receipt}" validation_anchor_range '[2560,2816)'
  expect_kv "${erratum_receipt}" certified_development_anchor_range \
    '[2880,3261)'
  expect_kv "${erratum_receipt}" certified_development_probe_rows 3429
  expect_kv "${erratum_receipt}" canonical_data_raw_access false
  expect_kv "${erratum_receipt}" final_holdout_available false
  expect_kv "${erratum_receipt}" independent_final_evidence false
}

if [[ "${mode}" == --plan ]]; then
  verify_static_chain
  print_plan
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  verify_receipt
  echo "verified_cursor_alignment_erratum=${erratum_receipt}"
  exit 0
fi

verify_static_chain
reject_symlink_components "${erratum_receipt}"
candidate="$(mktemp "${runtime_root}/.cursor_erratum.XXXXXX")"
emit_receipt "${candidate}"
chmod 0444 "${candidate}"
if [[ -e "${erratum_receipt}" ]]; then
  require_file "${erratum_receipt}"
  cmp -s -- "${candidate}" "${erratum_receipt}" ||
    fail "immutable cursor erratum receipt drifted"
  rm -f -- "${candidate}"
else
  ln -- "${candidate}" "${erratum_receipt}" ||
    fail "could not publish cursor erratum receipt"
  rm -f -- "${candidate}"
fi
verify_receipt
echo "sealed_cursor_alignment_erratum=${erratum_receipt}"
