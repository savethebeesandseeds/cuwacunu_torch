#!/usr/bin/env bash
set -euo pipefail
umask 077
export LC_ALL=C

readonly official_repo="/cuwacunu"
readonly frozen_wrapper_sha256="68d08dab8d219bb9d59fc1a73c62becbc45ecff03ef203265bd2725059e6cc8c"
readonly frozen_final_sealer_sha256="88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0"
readonly official_closure="${official_repo}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"
readonly work="/tmp/cuwacunu_mdn_retry1_concurrency_closure_68d08dab"
readonly template="${work}/template"
readonly active="${work}/active"
readonly outputs="${work}/outputs"
readonly retry_rel=".runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1"
readonly closure_rel=".runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"
readonly wrapper_rel="src/scripts/benchmarks/synthetic_continuous_graph_v2/seal_and_verify_mdn_retry1_completion_concurrency_closure.sh"
readonly final_rel="src/scripts/benchmarks/synthetic_continuous_graph_v2/seal_and_verify_mdn_retry1_completed_job.sh"
readonly erratum_rel="src/config/benchmarks/synthetic_continuous_graph_v2/MDN_RETRY1_SEAL_CONCURRENCY_ERRATUM.md"
readonly correction_rel="src/config/benchmarks/synthetic_continuous_graph_v2/MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md"
readonly runner_rel="src/scripts/benchmarks/synthetic_continuous_graph_v2/run_mdn_train_isolated_v2_retry1.sh"
readonly schema="synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1"

fail() {
  echo "isolated concurrency-closure test: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_official_untouched() {
  [[ "$(sha256_of "${official_repo}/${wrapper_rel}")" == \
    "${frozen_wrapper_sha256}" ]] || fail "official frozen wrapper drifted"
  [[ "$(sha256_of "${official_repo}/${final_rel}")" == \
    "${frozen_final_sealer_sha256}" ]] || fail "official final sealer drifted"
  [[ ! -e "${official_closure}" && ! -L "${official_closure}" ]] ||
    fail "official closure runtime exists"
}

safe_reset_work() {
  [[ "${work}" == /tmp/cuwacunu_mdn_retry1_concurrency_closure_68d08dab ]] ||
    fail "unsafe work path"
  rm -rf -- "${work}"
  mkdir -p -- "${template}" "${outputs}"
}

write_mock_final_sealer() {
  local path="$1"
  mkdir -p -- "$(dirname "${path}")"
  cat >"${path}" <<'MOCK_FINAL'
#!/usr/bin/env bash
set -euo pipefail
umask 077
export LC_ALL=C

script_path="$(realpath -e -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname "${script_path}")"
repo_root="$(realpath -e -- "${script_dir}/../../../..")"
runtime_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1"
runtime_parent="$(dirname "${runtime_root}")"
result_receipt="${runtime_root}/result.status"
checkpoint="${runtime_root}/job/channel_inference.report.channel_mdn.pt"
staging_path="${runtime_parent}/.synthetic_v2_mdn_train_isolated_v2_retry1.result.status.candidate"
call_log="${repo_root}/.mock_authoritative_child.calls"

verify_exact_receipt() {
  [[ -f "$1" && ! -L "$1" ]]
}

# This unused function deliberately carries the final source's unique
# publication order so the real wrapper's reconstruction logic is exercised.
publication_order_fixture() {
  rm -f -- "${staging_path}"
  chmod 0555 -- "${runtime_root}"
  verify_exact_receipt "${result_receipt}"
}

emit_result() {
  echo "mdn_checkpoint=${checkpoint}"
  echo "mdn_checkpoint_sha256=$(sha256sum -- "${checkpoint}" | awk '{print $1}')"
  echo "completion_result=${result_receipt}"
  echo "completion_result_sha256=$(sha256sum -- "${result_receipt}" | awk '{print $1}')"
}

mode="${1:-}"
[[ "${mode}" == --seal || "${mode}" == --verify ]] || exit 64
printf '%s\n' "${mode}" >>"${call_log}"

if [[ "${mode}" == --seal && -e "${repo_root}/.mock_fail_seal" ]]; then
  echo "injected seal failure" >&2
  exit 42
fi

if [[ "${mode}" == --seal && -e "${repo_root}/.mock_drift_content" ]]; then
  target="${runtime_root}/job/job.state"
  chmod 0644 -- "${target}"
  printf 'drift\n' >>"${target}"
  chmod 0444 -- "${target}"
fi

if [[ "${mode}" == --seal && -e "${repo_root}/.mock_drift_inode" ]]; then
  target="${runtime_root}/job/job.state"
  parent="$(dirname "${target}")"
  chmod 0755 -- "${parent}"
  cp -- "${target}" "${parent}/.job.state.replacement"
  chmod 0444 -- "${parent}/.job.state.replacement"
  mv -f -- "${parent}/.job.state.replacement" "${target}"
  chmod 0555 -- "${parent}"
fi

if [[ "${mode}" == --seal && -e "${repo_root}/.mock_drift_wrapper" ]]; then
  target="${script_dir}/seal_and_verify_mdn_retry1_completion_concurrency_closure.sh"
  chmod 0755 -- "${target}"
  printf '\n# injected concurrent wrapper drift\n' >>"${target}"
  chmod 0555 -- "${target}"
fi

if [[ "${mode}" == --seal && -e "${repo_root}/.mock_drift_final" ]]; then
  chmod 0755 -- "${script_path}"
  printf '\n# injected concurrent final-sealer drift\n' >>"${script_path}"
  chmod 0555 -- "${script_path}"
fi

if [[ "${mode}" == --seal ]]; then
  find "${runtime_root}" -type f -exec chmod 0444 -- {} +
  find "${runtime_root}" -depth -type d -exec chmod 0555 -- {} +
fi

[[ -z "$(find "${runtime_root}" -perm /222 -print -quit)" ]]
emit_result
MOCK_FINAL
  chmod 0555 -- "${path}"
}

patch_constant() {
  local path="$1" name="$2" value="$3"
  sed -i -E \
    "s|^readonly ${name}=.*$|readonly ${name}=\"${value}\"|" "${path}"
  [[ "$(grep -c -E "^readonly ${name}=\"${value}\"$" "${path}")" == 1 ]] ||
    fail "could not patch ${name}"
}

emit_reconstructed_mock_final() {
  local final="$1"
  awk '
    {
      if ($0 == "  rm -f -- \"${staging_path}\"") {
        first = $0;
        if ((getline second) <= 0 || (getline third) <= 0) exit 81;
        if (second == "  chmod 0555 -- \"${runtime_root}\"" &&
            third == "  verify_exact_receipt \"${result_receipt}\"") {
          matches += 1;
          print first;
          print third;
          print second;
          next;
        }
        print first; print second; print third; next;
      }
      print;
    }
    END { if (matches != 1) exit 82 }
  ' "${final}"
}

build_template() {
  local wrapper final retry_root benchmark_root script_root
  local rel path final_sha initial_sha result_sha checkpoint_sha master
  local final_bytes result_bytes
  wrapper="${template}/${wrapper_rel}"
  final="${template}/${final_rel}"
  retry_root="${template}/${retry_rel}"
  benchmark_root="${template}/src/config/benchmarks/synthetic_continuous_graph_v2"
  script_root="${template}/src/scripts/benchmarks/synthetic_continuous_graph_v2"
  mkdir -p -- "${benchmark_root}" "${script_root}" \
    "${retry_root}/components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb" \
    "${retry_root}/job" "${retry_root}/system"

  cp -- "${official_repo}/${wrapper_rel}" "${wrapper}"
  cp -- "${official_repo}/${erratum_rel}" "${template}/${erratum_rel}"
  cp -- "${official_repo}/${correction_rel}" "${template}/${correction_rel}"
  cp -- "${official_repo}/${runner_rel}" "${template}/${runner_rel}"
  write_mock_final_sealer "${final}"

  local -a payloads=(
    ".execution.lock"
    "components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref"
    "inputs.status"
    "job/channel_inference.report"
    "job/channel_inference.report.channel_mdn.pt"
    "job/channel_inference.report.mdn.lls"
    "job/channel_inference.report.nodelift.lls"
    "job/channel_inference.report.representation.lls"
    "job/job.manifest"
    "job/job.state"
    "job/lattice.checkpoint.fact"
    "job/lattice.exposure.fact"
    "job/lattice.source_analytics.fact"
    "job/representation_edge_features.probe"
    "job/runtime.checkpoint_io.fact"
    "job/runtime.component_training_update.fact"
    "job/runtime.health_measurement.fact"
    "job/runtime.job_events.probe"
    "job/runtime.result.fact"
    "mdn_train.retry1.log"
    "synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config"
    "system/component_spawn_registry.v1.lls"
    "system/runtime_layout.v1.lls"
  )
  for rel in "${payloads[@]}"; do
    path="${retry_root}/${rel}"
    if [[ "${rel}" == .execution.lock ]]; then
      : >"${path}"
    else
      printf 'isolated fixture: %s\n' "${rel}" >"${path}"
    fi
  done

  final_sha="$(sha256_of "${final}")"
  initial_sha="$(emit_reconstructed_mock_final "${final}" | sha256sum | awk '{print $1}')"
  final_bytes="$(stat -c '%s' -- "${final}")"
  checkpoint_sha="$(sha256_of "${retry_root}/job/channel_inference.report.channel_mdn.pt")"
  master="$(
    for rel in "${payloads[@]}"; do
      path="${retry_root}/${rel}"
      printf '%s %s %s\n' "$(sha256_of "${path}")" \
        "$(stat -c '%s' -- "${path}")" "${rel}"
    done | sort -k3,3 | sha256sum | awk '{print $1}'
  )"

  cat >"${retry_root}/result.status" <<RESULT
schema_id=synthetic_v2_mdn_train_isolated_v2_retry1.result.v1
status=complete
execution_runner_path=${active}/${runner_rel}
execution_runner_sha256=$(sha256_of "${template}/${runner_rel}")
completion_sealer_path=${active}/${final_rel}
completion_sealer_sha256=${final_sha}
completion_correction_path=${active}/${correction_rel}
completion_correction_sha256=$(sha256_of "${template}/${correction_rel}")
payload_inventory_master_sha256=${master}
checkpoint_path=${active}/${retry_rel}/job/channel_inference.report.channel_mdn.pt
checkpoint_sha256=${checkpoint_sha}
canonical_data_raw_access=false
final_holdout_available=false
independent_final_evidence=false
policy_access=false
RESULT
  result_sha="$(sha256_of "${retry_root}/result.status")"
  result_bytes="$(stat -c '%s' -- "${retry_root}/result.status")"

  patch_constant "${wrapper}" expected_initial_loaded_publisher_sha256 "${initial_sha}"
  patch_constant "${wrapper}" expected_final_sealer_sha256 "${final_sha}"
  patch_constant "${wrapper}" expected_result_sha256 "${result_sha}"
  patch_constant "${wrapper}" expected_payload_inventory_master_sha256 "${master}"
  patch_constant "${wrapper}" expected_checkpoint_sha256 "${checkpoint_sha}"
  patch_constant "${wrapper}" expected_result_bytes "${result_bytes}"
  patch_constant "${wrapper}" expected_final_sealer_bytes "${final_bytes}"

  chmod 0444 -- "${template}/${erratum_rel}" "${template}/${correction_rel}"
  chmod 0555 -- "${wrapper}" "${final}" "${template}/${runner_rel}"
  find "${retry_root}" -type f -exec chmod 0444 -- {} +
  find "${retry_root}" -depth -type d -exec chmod 0555 -- {} +

  bash -n "${wrapper}"
  printf '%s\n' "${initial_sha}" >"${work}/mock_initial.sha256"
  printf '%s\n' "$(sha256_of "${wrapper}")" >"${work}/transformed_wrapper.sha256"
}

reset_active() {
  [[ "${active}" == "${work}/active" ]] || fail "unsafe active path"
  rm -rf -- "${active}"
  mkdir -p -- "${active}"
  cp -a -- "${template}/." "${active}/"
}

run_fail() {
  local name="$1"
  shift
  local rc
  set +e
  "$@" >"${outputs}/${name}.stdout" 2>"${outputs}/${name}.stderr"
  rc=$?
  set -e
  [[ "${rc}" != 0 ]] || fail "${name}: expected failure"
  printf '%s\n' "${rc}" >"${outputs}/${name}.exit"
}

assert_completed_tree() {
  local closure="$1"
  [[ "$(find "${closure}" -type d | wc -l)" == 1 ]] ||
    fail "completed closure contains a subdirectory"
  [[ "$(find "${closure}" -type f | wc -l)" == 10 ]] ||
    fail "completed closure file count is not ten"
  [[ -z "$(find "${closure}" -perm /222 -print -quit)" ]] ||
    fail "completed closure contains a writable entry"
  [[ "$(stat -c '%a' -- "${closure}")" == 555 ]] ||
    fail "completed closure root is not mode0555"
  while IFS= read -r path; do
    [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
      fail "completed closure file is not mode0444: ${path}"
    [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
      fail "completed closure file link count is not one: ${path}"
  done < <(find "${closure}" -type f)
  [[ "$(stat -c '%s' -- "${closure}/.closure.lock")" == 0 ]] ||
    fail "completed closure lock is not empty"
  [[ "$(sha256_of "${closure}/.closure.lock")" == \
    e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 ]] ||
    fail "completed closure lock hash drifted"
}

case_happy_idempotent_and_verify() {
  reset_active
  local wrapper="${active}/${wrapper_rel}" closure="${active}/${closure_rel}"
  "${wrapper}" --seal >"${outputs}/happy.seal"
  "${wrapper}" --verify >"${outputs}/happy.verify"
  "${wrapper}" --seal >"${outputs}/happy.idempotent_seal"
  assert_completed_tree "${closure}"
  [[ "$(grep -c '^--seal$' "${active}/.mock_authoritative_child.calls")" == 1 ]] ||
    fail "happy path invoked authoritative --seal more than once"
  [[ "$(grep -c '^--verify$' "${active}/.mock_authoritative_child.calls")" == 3 ]] ||
    fail "verify/idempotent paths did not invoke authoritative --verify"
  cp -a -- "${active}" "${work}/completed_for_recovery"
  echo "case_happy_idempotent_and_verify=PASS"
}

case_unknown_foreign_stage() {
  reset_active
  local parent="${active}/.runtime/benchmarks/synthetic_continuous_graph_v2"
  local stage="${parent}/.${schema}.foreign.candidate"
  printf 'foreign\n' >"${stage}"
  chmod 0444 -- "${stage}"
  run_fail unknown_foreign_stage "${active}/${wrapper_rel}" --seal
  [[ -f "${stage}" ]] || fail "unknown foreign stage was not retained"
  [[ ! -e "${active}/.mock_authoritative_child.calls" ]] ||
    fail "unknown foreign stage reached child"
  echo "case_unknown_foreign_stage=PASS"
}

case_known_wrong_stage() {
  reset_active
  local parent="${active}/.runtime/benchmarks/synthetic_continuous_graph_v2"
  local stage="${parent}/.${schema}.reconstructed_initial_publisher.3cce.sh.candidate"
  printf 'wrong known candidate\n' >"${stage}"
  chmod 0444 -- "${stage}"
  run_fail known_wrong_stage "${active}/${wrapper_rel}" --seal
  [[ -f "${stage}" ]] || fail "known wrong stage was not retained"
  [[ ! -e "${active}/${closure_rel}/attempt.started" ]] ||
    fail "known wrong stage published an attempt"
  [[ ! -e "${active}/.mock_authoritative_child.calls" ]] ||
    fail "known wrong stage reached child"
  echo "case_known_wrong_stage=PASS"
}

case_exact_unlinked_static_candidate() {
  reset_active
  local parent="${active}/.runtime/benchmarks/synthetic_continuous_graph_v2"
  local stage="${parent}/.${schema}.reconstructed_initial_publisher.3cce.sh.candidate"
  emit_reconstructed_mock_final "${active}/${final_rel}" >"${stage}"
  chmod 0444 -- "${stage}"
  "${active}/${wrapper_rel}" --seal >"${outputs}/exact_unlinked.seal"
  [[ ! -e "${stage}" ]] || fail "exact unlinked candidate was not consumed"
  assert_completed_tree "${active}/${closure_rel}"
  echo "case_exact_unlinked_static_candidate=PASS"
}

case_linked_success_and_receipt_recovery() {
  rm -rf -- "${active}"
  cp -a -- "${work}/completed_for_recovery" "${active}"
  local closure="${active}/${closure_rel}"
  local parent="${active}/.runtime/benchmarks/synthetic_continuous_graph_v2"
  local success_stage="${parent}/.${schema}.success.status.candidate"
  local receipt_stage="${parent}/.${schema}.completion_concurrency.status.candidate"
  ln -- "${closure}/success.status" "${success_stage}"
  ln -- "${closure}/completion_concurrency.status" "${receipt_stage}"
  [[ "$(stat -c '%h' -- "${closure}/success.status")" == 2 ]] || fail "success link fixture failed"
  [[ "$(stat -c '%h' -- "${closure}/completion_concurrency.status")" == 2 ]] || fail "receipt link fixture failed"
  "${active}/${wrapper_rel}" --seal >"${outputs}/linked_recovery.seal"
  [[ ! -e "${success_stage}" && ! -e "${receipt_stage}" ]] ||
    fail "linked recovery left candidates"
  assert_completed_tree "${closure}"
  echo "case_linked_success_and_receipt_recovery=PASS"
}

case_child_failure_and_attempt_refusal() {
  reset_active
  : >"${active}/.mock_fail_seal"
  run_fail child_failure "${active}/${wrapper_rel}" --seal
  local attempt="${active}/${closure_rel}/attempt.started"
  [[ -f "${attempt}" && ! -e "${active}/${closure_rel}/success.status" ]] ||
    fail "child failure did not leave the one-attempt marker"
  [[ "$(wc -l <"${active}/.mock_authoritative_child.calls")" == 1 ]] ||
    fail "child failure call count is not one"
  rm -f -- "${active}/.mock_fail_seal"
  run_fail attempt_refusal "${active}/${wrapper_rel}" --seal
  [[ "$(wc -l <"${active}/.mock_authoritative_child.calls")" == 1 ]] ||
    fail "ambiguous retry invoked the child again"
  grep -q 'refusing ambiguous adoption re-execution' \
    "${outputs}/attempt_refusal.stderr" || fail "attempt refusal reason drifted"
  echo "case_child_seal_failure=PASS"
  echo "case_attempt_without_success_refuses_no_child=PASS"
}

case_content_drift_rejection() {
  reset_active
  : >"${active}/.mock_drift_content"
  run_fail content_drift "${active}/${wrapper_rel}" --seal
  [[ -f "${active}/${closure_rel}/attempt.started" ]] || fail "content drift lacks attempt marker"
  [[ ! -e "${active}/${closure_rel}/success.status" ]] || fail "content drift published success"
  echo "case_content_drift_rejection=PASS"
}

case_inode_drift_rejection() {
  reset_active
  local target="${active}/${retry_rel}/job/job.state" before after
  before="$(stat -c '%i' -- "${target}")"
  : >"${active}/.mock_drift_inode"
  run_fail inode_drift "${active}/${wrapper_rel}" --seal
  after="$(stat -c '%i' -- "${target}")"
  [[ "${before}" != "${after}" ]] || fail "inode drift fixture did not replace inode"
  [[ ! -e "${active}/${closure_rel}/success.status" ]] || fail "inode drift published success"
  echo "case_inode_drift_rejection=PASS"
}

case_source_drift_rejection() {
  reset_active
  printf '\n# preexisting final drift\n' >>"${active}/${final_rel}"
  chmod 0555 -- "${active}/${final_rel}"
  run_fail preexisting_final_drift "${active}/${wrapper_rel}" --seal
  [[ ! -e "${active}/${closure_rel}" ]] ||
    fail "preexisting source drift mutated closure state before guard"
  echo "case_preexisting_final_source_drift_rejection=PASS"

  reset_active
  : >"${active}/.mock_drift_wrapper"
  run_fail concurrent_wrapper_drift "${active}/${wrapper_rel}" --seal
  [[ -f "${active}/${closure_rel}/attempt.started" ]] ||
    fail "concurrent wrapper drift did not occur after attempt"
  [[ ! -e "${active}/${closure_rel}/success.status" ]] ||
    fail "concurrent wrapper drift published success"
  echo "case_concurrent_wrapper_source_drift_rejection=PASS"

  reset_active
  : >"${active}/.mock_drift_final"
  run_fail concurrent_final_drift "${active}/${wrapper_rel}" --seal
  [[ -f "${active}/${closure_rel}/attempt.started" ]] ||
    fail "concurrent final drift did not occur after attempt"
  [[ ! -e "${active}/${closure_rel}/success.status" ]] ||
    fail "concurrent final drift published success"
  echo "case_concurrent_final_source_drift_rejection=PASS"
}

main() {
  require_official_untouched
  safe_reset_work
  build_template
  reset_active
  "${active}/${wrapper_rel}" --plan >"${outputs}/transformed.plan"
  grep -q "reconstructed_publisher_proof_sha256=$(cat "${work}/mock_initial.sha256")" \
    "${outputs}/transformed.plan" || fail "transformed reconstruction proof failed"
  case_happy_idempotent_and_verify
  case_unknown_foreign_stage
  case_known_wrong_stage
  case_exact_unlinked_static_candidate
  case_linked_success_and_receipt_recovery
  case_child_failure_and_attempt_refusal
  case_content_drift_rejection
  case_inode_drift_rejection
  case_source_drift_rejection
  require_official_untouched
  echo "frozen_wrapper_sha256=${frozen_wrapper_sha256}"
  echo "transformed_wrapper_sha256=$(cat "${work}/transformed_wrapper.sha256")"
  echo "mock_initial_publisher_sha256=$(cat "${work}/mock_initial.sha256")"
  echo "official_closure_absent=true"
  echo "isolated_matrix=PASS"
}

main "$@"
