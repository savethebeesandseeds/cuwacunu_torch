#!/usr/bin/env bash
set -euo pipefail
umask 077

readonly schema_id="synthetic_v2_representation_ablation_isolated_v2_prejob_failure_closure_v1"
readonly failed_schema_id="synthetic_v2_representation_ablation_isolated_v2"
readonly retry_schema_id="synthetic_v2_representation_ablation_isolated_v2_retry1"

readonly expected_amendment_sha256="7e2e71579c444c5190d824f5963d6cef3f66dc6203b0edd2a3bdd9f9c3cd9088"
readonly expected_complete_inventory_sha256="a467215c0fd46eba9a5a6957f161396adae30b4b06995ad803606b73826b830d"
readonly expected_failed_runner_sha256="af1824c7386f0c15a2ce1a0afb2f3b2a4d0fe2858e7b39cb10edd2ef52f3adda"
readonly expected_failure_log_sha256="4be9a577f7e07dc1cc20aa37f932b83a2f7f83cd52e08a123a5419681b607dc9"
readonly expected_config_derivation_sha256="d1bc0c64eaff3b404e7d6cc537ec131a7c445827f64f7d2501c62edfe9b8a2c4"
readonly expected_config_bundle_sha256="7f7fae337ec3132258b14e731bca240605b61c05527365c6700b2ff77d277feb"
readonly expected_job_runner_sha256="aaecc932ea72a98f24c791b94d0782e2f6805fa8e4a4e3a93cd5538375508120"
readonly expected_mtf_launcher_sha256="5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502"
readonly expected_exec_main_sha256="4d53435e792928dfc6c67f42734b55379cce53c6055f1c4104e998daa0711dde"
readonly expected_runtime_exec_sha256="9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff"
readonly expected_canonical_grammar_sha256="26f1d105ec04945024ac91806fc4206e21d81429c3a190782b7159af69d2e0a3"
readonly expected_regular_file_count=23
readonly expected_directory_count=12

fail() {
  echo "representation ablation pre-job failure closure: $*" >&2
  exit 1
}

sha256_of() {
  sha256sum -- "$1" | awk '{print $1}'
}

require_file() {
  [[ -f "$1" && ! -L "$1" ]] || fail "missing or symlinked file: $1"
}

require_dir() {
  [[ -d "$1" && ! -L "$1" ]] || fail "missing or symlinked directory: $1"
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
runtime_parent="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v2"
failed_runtime="${runtime_parent}/${failed_schema_id}"
closure_root="${runtime_parent}/${schema_id}"
staging_root="${runtime_parent}/.${schema_id}.candidate"
receipt_path="${closure_root}/failure_closure.status"
regular_inventory_path="${closure_root}/regular_files.inventory.tsv"
directory_inventory_path="${closure_root}/directories.inventory.tsv"
frozen_root="${closure_root}/frozen_sources"
frozen_sealer_path="${frozen_root}/$(basename "${script_path}")"
amendment_path="${script_dir}/REPRESENTATION_ABLATION_PREJOB_CONFIG_PATH_RECOVERY_AMENDMENT.md"
frozen_amendment_path="${frozen_root}/$(basename "${amendment_path}")"
retry_runtime="${runtime_parent}/${retry_schema_id}"

failed_lock_path="${failed_runtime}/.development.lock"
failed_runner_path="${failed_runtime}/frozen_sources/run_representation_ablation_v2.sh"
failed_base_config_path="${failed_runtime}/synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config"
failure_log_path="${failed_runtime}/arms/endpoint_scale/training.log"
endpoint_training_dir="${failed_runtime}/arms/endpoint_scale/training"
missing_grammar_path="${failed_runtime}/arms/endpoint_scale/config/grammar/representation.net.bnf"
config_derivation_path="${repo_root}/src/include/hero/config_derivation.h"
config_bundle_path="${repo_root}/src/include/kikijyeba/protocol/config_bundle.h"
job_runner_path="${repo_root}/src/include/hero/runtime_hero/runtime/job_runner.h"
mtf_launcher_path="${repo_root}/src/include/jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"
exec_main_path="${repo_root}/src/main/exec/cuwacunu_exec.cpp"
runtime_exec_path="${repo_root}/.build/exec/cuwacunu_exec"
canonical_grammar_path="${repo_root}/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf"

readonly process_start_sealer_sha256="$(sha256_of "${script_path}")"
readonly process_start_sealer_mode="$(stat -c '%a' -- "${script_path}")"
readonly process_start_sealer_links="$(stat -c '%h' -- "${script_path}")"
readonly process_start_sealer_uid="$(stat -c '%u' -- "${script_path}")"
readonly process_start_sealer_bytes="$(stat -c '%s' -- "${script_path}")"
readonly process_start_sealer_inode="$(stat -c '%i' -- "${script_path}")"
readonly process_start_sealer_device="$(stat -c '%d' -- "${script_path}")"

declare -a temporary_files=()
candidate_created_by_process=false

cleanup() {
  local path
  for path in "${temporary_files[@]:-}"; do
    [[ -n "${path}" ]] && rm -f -- "${path}" 2>/dev/null || true
  done
  if [[ "${candidate_created_by_process}" == true && \
    -d "${staging_root}" && ! -L "${staging_root}" ]]; then
    chmod u+w -- "${staging_root}" "${staging_root}/frozen_sources" \
      2>/dev/null || true
    rm -f -- "${staging_root}/failure_closure.status" \
      "${staging_root}/regular_files.inventory.tsv" \
      "${staging_root}/directories.inventory.tsv" \
      "${staging_root}/frozen_sources/$(basename "${script_path}")" \
      "${staging_root}/frozen_sources/$(basename "${amendment_path}")" \
      2>/dev/null || true
    rmdir -- "${staging_root}/frozen_sources" "${staging_root}" \
      2>/dev/null || true
  fi
}
trap cleanup EXIT

mode="${1:---plan}"
[[ "$#" -le 1 ]] || fail "usage: $0 [--plan|--audit|--seal|--verify]"
case "${mode}" in
--plan | --audit | --seal | --verify) ;;
*) fail "usage: $0 [--plan|--audit|--seal|--verify]" ;;
esac

print_plan() {
  cat <<PLAN
schema_id=${schema_id}.plan
failed_runtime_schema_id=${failed_schema_id}
failed_runtime_root=${failed_runtime}
failed_regular_file_count=${expected_regular_file_count}
failed_directory_count=${expected_directory_count}
failed_complete_inventory_sha256=${expected_complete_inventory_sha256}
failure_log_path=${failure_log_path}
failure_log_sha256=${expected_failure_log_sha256}
missing_grammar_path=${missing_grammar_path}
closure_runtime_root=${closure_root}
closure_receipt_path=${receipt_path}
publication_scope=separate_closure_runtime_only
failed_runtime_mutation_authorized=false
attempt_scientific_evidence=false
retry_schema_id=${retry_schema_id}
retry_runtime_root=${retry_runtime}
PLAN
}

emit_expected_regular_inventory() {
  cat <<'INVENTORY' | tr '|' '\t'
relative_path|mode|uid|gid|links|bytes|inode|device|sha256
.development.lock|600|0|0|1|0|3096224744582911|66|e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
arms/canonical/affine/main.report|444|0|0|1|9738|4503599627726489|66|e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e
arms/canonical/affine/replay.report|444|0|0|1|9738|5629499534569114|66|e816c9cc318ce76c273cf78e6028178eaae19e04f8837e3e2587ff459ae3d49e
arms/canonical/import.status|444|0|0|1|5936|7881299348254367|66|e53fd1d0a6bb8046ae4f4056a4fbfee7208d9df03b0072a0acf338ac1634adf8
arms/endpoint_scale/config/capture.config|444|0|0|1|5446|5629499534569051|66|3d7b0ff75f5026ba1efc383fad33f010baeacd209b1e9530ac6e15bf4bd541c5
arms/endpoint_scale/config/representation.jkimyei|444|0|0|1|2119|5910974511279648|66|c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8
arms/endpoint_scale/config/representation.net|444|0|0|1|602|6192449487990305|66|42a078766e0dfdb8f0074b69d3dc1eacb63f52ec806dd3b4355b3b280c02593e
arms/endpoint_scale/config/train.config|444|0|0|1|5443|8444249301675555|66|0a8b2322a3730a085147e46d14cf22e1eb3c467e95833f8598490b2ca38faf25
arms/endpoint_scale/training.log|600|0|0|1|3009|4222124651015842|66|4be9a577f7e07dc1cc20aa37f932b83a2f7f83cd52e08a123a5419681b607dc9
arms/no_tf_alignment/config/capture.config|444|0|0|1|5448|7318349394833010|66|da8597f4825313ab2ed6b0e5b894cb4cb24473185c3fc5b31f91c78209eb596d
arms/no_tf_alignment/config/representation.jkimyei|444|0|0|1|2119|7318349394833005|66|1503407ad50dd86a5ba855c7247e0efdb4b78c11a22c97d359a8b2e64b518d37
arms/no_tf_alignment/config/representation.net|444|0|0|1|603|4785074604437103|66|df4398835b7eff3496ac8c20e7713b2d3d3a245754916c81b77271c696a08cda
arms/no_tf_alignment/config/train.config|444|0|0|1|5445|5629499534569073|66|eff19b3d1760e3ce95a39bea5a9dad693cd12fec97b8620d7e79995468dec3b1
arms/time_only/config/capture.config|444|0|0|1|5436|6755399441411689|66|84f1f8f1e9fb12be5c232495e4f066d04b40d40bcee8567be082a137162c8564
arms/time_only/config/representation.jkimyei|444|0|0|1|2119|7318349394832997|66|c1898f3a7aaa5183a8e6e0341f8dbbdc087456ee7f8701175ea70720d682f4d8
arms/time_only/config/representation.net|444|0|0|1|604|14636698789310054|66|b60dbbf805a6abac25488a738fcfc9136d6a6de79ac688fea265b3d3963f280e
arms/time_only/config/train.config|444|0|0|1|5433|5910974511279719|66|a443b8542a0070d0f41d540f45f0f75147f6cd87f5cc422a58789e9a61798760
config_inputs.status|444|0|0|1|21308|7318349394833015|66|cdcf020fadfb604ab12fc14a16872584f298253c521284aacac6e12f52d9b957
frozen_sources/frozen_representation_affine_probe|555|0|0|1|744608|9288674231807502|66|733841623165e1be1dbf76e82264022292b5c16825211696800fd5876cddad3f
frozen_sources/frozen_representation_affine_probe.cpp|444|0|0|1|47520|5910974511279621|66|45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939
frozen_sources/run_representation_ablation_v2.sh|444|0|0|1|213358|9851624185228802|66|af1824c7386f0c15a2ce1a0afb2f3b2a4d0fe2858e7b39cb10edd2ef52f3adda
inputs.status|444|0|0|1|19805|11258999068782210|66|bbd8b6923355829cf3932dd6ab3de778b80acd6eb3b73231388d2820ef7869ab
synthetic_benchmark.train_core_mtf_jepa_mae_vicreg.isolated.config|444|0|0|1|5315|4222124651425536|66|e9db78f15fb3cd9a0475f51ff99e900bff53dc11d9daf13d1565c2dc3a43d4e7
INVENTORY
}

emit_expected_directory_inventory() {
  cat <<'INVENTORY' | tr '|' '\t'
relative_path|mode|uid|gid|links|bytes|inode|device
.|700|0|0|1|4096|15199648743115278|66
arms|700|0|0|1|4096|7599824371543567|66
arms/canonical|700|0|0|1|4096|16607023626284688|66
arms/canonical/affine|700|0|0|1|4096|11258999068782232|66
arms/endpoint_scale|700|0|0|1|4096|11258999068782108|66
arms/endpoint_scale/config|700|0|0|1|4096|7599824371543581|66
arms/endpoint_scale/training|700|0|0|1|4096|9288674231807648|66
arms/no_tf_alignment|700|0|0|1|4096|7318349394833003|66
arms/no_tf_alignment/config|700|0|0|1|4096|7881299348254316|66
arms/time_only|700|0|0|1|4096|7881299348254303|66
arms/time_only/config|700|0|0|1|4096|9851624185228897|66
frozen_sources|700|0|0|1|4096|19140298416680435|66
INVENTORY
}

relative_path_of() {
  local path="$1"
  if [[ "${path}" == "${failed_runtime}" ]]; then
    printf '.'
  else
    printf '%s' "${path#${failed_runtime}/}"
  fi
}

emit_current_regular_inventory() {
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\tsha256\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && "${rel}" != *'|'* ]] ||
      fail "failed runtime has a non-canonical inventory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}" \
      "$(sha256_of "${path}")"
  done < <(find "${failed_runtime}" -type f -print0 | LC_ALL=C sort -z)
}

emit_current_directory_inventory() {
  local path rel stat_fields mode_value uid_value gid_value links_value
  local bytes_value inode_value device_value
  printf 'relative_path\tmode\tuid\tgid\tlinks\tbytes\tinode\tdevice\n'
  while IFS= read -r -d '' path; do
    rel="$(relative_path_of "${path}")"
    [[ "${rel}" != *$'\n'* && "${rel}" != *$'\t'* && "${rel}" != *'|'* ]] ||
      fail "failed runtime has a non-canonical directory path: ${rel}"
    stat_fields="$(stat -c '%a %u %g %h %s %i %d' -- "${path}")"
    read -r mode_value uid_value gid_value links_value bytes_value inode_value \
      device_value <<<"${stat_fields}"
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${rel}" "${mode_value}" "${uid_value}" "${gid_value}" \
      "${links_value}" "${bytes_value}" "${inode_value}" "${device_value}"
  done < <(find "${failed_runtime}" -type d -print0 | LC_ALL=C sort -z)
}

new_temp_file() {
  local __result_var="$1"
  local template="$2"
  local created
  created="$(mktemp "${template}")"
  temporary_files+=("${created}")
  printf -v "${__result_var}" '%s' "${created}"
}

verify_hash() {
  local path="$1"
  local expected="$2"
  local label="$3"
  reject_symlink_components "${path}"
  require_file "${path}"
  [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
    fail "${label} has an unexpected hard-link count"
  [[ "$(sha256_of "${path}")" == "${expected}" ]] ||
    fail "${label} hash drifted: ${path}"
}

assert_process_start_source_identity() {
  reject_symlink_components "${script_path}"
  require_file "${script_path}"
  [[ "$(sha256_of "${script_path}")" == "${process_start_sealer_sha256}" ]] ||
    fail "executing sealer bytes changed after process start"
  [[ "$(stat -c '%a' -- "${script_path}")" == \
    "${process_start_sealer_mode}" ]] || fail "executing sealer mode changed"
  [[ "$(stat -c '%h' -- "${script_path}")" == \
    "${process_start_sealer_links}" ]] || fail "executing sealer links changed"
  [[ "$(stat -c '%u' -- "${script_path}")" == \
    "${process_start_sealer_uid}" ]] || fail "executing sealer owner changed"
  [[ "$(stat -c '%s' -- "${script_path}")" == \
    "${process_start_sealer_bytes}" ]] || fail "executing sealer size changed"
  [[ "$(stat -c '%i' -- "${script_path}")" == \
    "${process_start_sealer_inode}" ]] || fail "executing sealer inode changed"
  [[ "$(stat -c '%d' -- "${script_path}")" == \
    "${process_start_sealer_device}" ]] || fail "executing sealer device changed"
  [[ "${process_start_sealer_mode}" == 555 ]] ||
    fail "executing sealer must be frozen mode 0555"
  [[ "${process_start_sealer_links}" == 1 ]] ||
    fail "executing sealer must have link count one"
}

verify_external_authorities() {
  assert_process_start_source_identity
  verify_hash "${amendment_path}" "${expected_amendment_sha256}" \
    "pre-job recovery amendment"
  [[ "$(stat -c '%a' -- "${amendment_path}")" == 444 ]] ||
    fail "pre-job recovery amendment must be mode 0444"
  verify_hash "${config_derivation_path}" \
    "${expected_config_derivation_sha256}" "grammar derivation source"
  verify_hash "${config_bundle_path}" "${expected_config_bundle_sha256}" \
    "graph-first config bundle source"
  verify_hash "${job_runner_path}" "${expected_job_runner_sha256}" \
    "Runtime job runner source"
  verify_hash "${mtf_launcher_path}" "${expected_mtf_launcher_sha256}" \
    "MTF launcher source"
  verify_hash "${exec_main_path}" "${expected_exec_main_sha256}" \
    "Runtime executable main source"
  verify_hash "${runtime_exec_path}" "${expected_runtime_exec_sha256}" \
    "Runtime executable"
  verify_hash "${canonical_grammar_path}" \
    "${expected_canonical_grammar_sha256}" "canonical MTF net grammar"
}

compute_complete_inventory_sha256() {
  (
    cd "${failed_runtime}"
    find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum |
      awk '{print $1}'
  )
}

verify_exact_failed_tree() {
  reject_symlink_components "${failed_runtime}"
  require_dir "${failed_runtime}"
  [[ "$(realpath -e -- "${failed_runtime}")" == "${failed_runtime}" ]] ||
    fail "failed runtime root is not canonical"

  local entry
  while IFS= read -r -d '' entry; do
    [[ ! -L "${entry}" ]] || fail "failed runtime contains a symlink: ${entry}"
    [[ -f "${entry}" || -d "${entry}" ]] ||
      fail "failed runtime contains a special entry: ${entry}"
  done < <(find "${failed_runtime}" -mindepth 0 -print0)

  local expected_regular actual_regular expected_directories actual_directories
  new_temp_file expected_regular "/tmp/${schema_id}.regular.expected.XXXXXX"
  new_temp_file actual_regular "/tmp/${schema_id}.regular.actual.XXXXXX"
  new_temp_file expected_directories "/tmp/${schema_id}.directories.expected.XXXXXX"
  new_temp_file actual_directories "/tmp/${schema_id}.directories.actual.XXXXXX"
  emit_expected_regular_inventory >"${expected_regular}"
  emit_current_regular_inventory >"${actual_regular}"
  emit_expected_directory_inventory >"${expected_directories}"
  emit_current_directory_inventory >"${actual_directories}"
  cmp -s -- "${expected_regular}" "${actual_regular}" ||
    fail "failed-runtime regular-file inventory drifted"
  cmp -s -- "${expected_directories}" "${actual_directories}" ||
    fail "failed-runtime directory identity inventory drifted"

  [[ "$(($(wc -l <"${actual_regular}") - 1))" == \
    "${expected_regular_file_count}" ]] || fail "regular-file count drifted"
  [[ "$(($(wc -l <"${actual_directories}") - 1))" == \
    "${expected_directory_count}" ]] || fail "directory count drifted"
  [[ "$(compute_complete_inventory_sha256)" == \
    "${expected_complete_inventory_sha256}" ]] ||
    fail "complete regular-file inventory digest drifted"
}

verify_failure_boundary() {
  verify_hash "${failed_runner_path}" "${expected_failed_runner_sha256}" \
    "failed attempt frozen runner"
  verify_hash "${failure_log_path}" "${expected_failure_log_sha256}" \
    "endpoint failure log"
  require_dir "${endpoint_training_dir}"
  [[ -z "$(find "${endpoint_training_dir}" -mindepth 1 -print -quit)" ]] ||
    fail "endpoint training directory is no longer empty"
  [[ ! -e "${missing_grammar_path}" && ! -L "${missing_grammar_path}" ]] ||
    fail "formerly missing arm-local grammar now exists"

  local expected_error
  expected_error="[graph_first_config] unable to open file: ${missing_grammar_path}"
  [[ "$(grep -Fxc -- "${expected_error}" "${failure_log_path}")" == 1 ]] ||
    fail "endpoint log does not contain exactly one expected grammar failure"
  [[ "$(grep -Fc -- '[graph_first_config] unable to open file:' \
    "${failure_log_path}")" == 1 ]] ||
    fail "endpoint log contains an unexpected graph-first open-failure count"
  [[ "$(grep -Fc -- 'last_config_path=<none>' "${failure_log_path}")" == 1 ]] ||
    fail "endpoint log source-finalization boundary drifted"

  local arm config kind
  for arm in endpoint_scale time_only no_tf_alignment; do
    for kind in train capture; do
      config="${failed_runtime}/arms/${arm}/config/${kind}.config"
      require_file "${config}"
      if grep -Eq \
        '^[[:space:]]*wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path[[:space:]]*=' \
        "${config}"; then
        fail "failed arm config unexpectedly has an explicit MTF net grammar: ${config}"
      fi
      expect_kv "${config}" \
        wikimyei_representation_mtf_jepa_mae_vicreg_net_path \
        "${failed_runtime}/arms/${arm}/config/representation.net"
    done
  done

  local forbidden
  forbidden="$(find "${failed_runtime}" -mindepth 1 \
    \( -name job.manifest -o -name runtime.result.fact -o -name '*.pt' \
    -o -name '*.pth' -o -name '*.ckpt' -o -iname '*checkpoint*' \
    -o -name '*.probe' -o -name training.status -o -name capture.status \
    -o -name affine.status -o -name development.status -o -name result.status \
    -o -iname '*selection*' -o -iname '*certified*' \) -print -quit)"
  [[ -z "${forbidden}" ]] ||
    fail "failed runtime contains a forbidden downstream artifact: ${forbidden}"
  [[ -z "$(find "${failed_runtime}" -type d -name job -print -quit)" ]] ||
    fail "failed runtime contains a Runtime job directory"
}

verify_failed_attempt() {
  verify_external_authorities
  verify_exact_failed_tree
  verify_failure_boundary
  assert_process_start_source_identity
}

emit_receipt() {
  local destination="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  local frozen_sealer="$4"
  local frozen_amendment="$5"
  cat >"${destination}" <<RECEIPT
schema_id=${schema_id}
status=complete
closure_kind=external_pre_job_configuration_failure
evidence_class=operational_failure_boundary
scientific_evidence=false
metric_evidence_authorized=false
failed_attempt_counts_as_challenger_result=false
closure_runtime_root=${closure_root}
closure_receipt_path=${receipt_path}
publication_method=atomic_no_clobber_directory_rename
failed_runtime_mutated=false
failed_runtime_receipt_published_in_place=false
failure_closure_sealer_path=${script_path}
failure_closure_sealer_process_start_sha256=${process_start_sealer_sha256}
failure_closure_sealer_process_start_mode=0${process_start_sealer_mode}
failure_closure_sealer_process_start_links=${process_start_sealer_links}
failure_closure_sealer_process_start_owner_uid=${process_start_sealer_uid}
failure_closure_sealer_process_start_bytes=${process_start_sealer_bytes}
failure_closure_sealer_process_start_inode=${process_start_sealer_inode}
failure_closure_sealer_process_start_device=${process_start_sealer_device}
frozen_failure_closure_sealer_path=${frozen_sealer_path}
frozen_failure_closure_sealer_sha256=$(sha256_of "${frozen_sealer}")
recovery_amendment_path=${amendment_path}
recovery_amendment_sha256=${expected_amendment_sha256}
frozen_recovery_amendment_path=${frozen_amendment_path}
frozen_recovery_amendment_sha256=$(sha256_of "${frozen_amendment}")
failed_runtime_schema_id=${failed_schema_id}
failed_runtime_root=${failed_runtime}
failed_runtime_root_mode=0700
failed_runtime_root_links=1
failed_runtime_root_owner_uid=0
failed_runtime_root_owner_gid=0
failed_runtime_root_bytes=4096
failed_runtime_root_inode=15199648743115278
failed_runtime_root_device=66
regular_file_inventory_path=${regular_inventory_path}
regular_file_inventory_sha256=$(sha256_of "${regular_inventory}")
regular_file_count=${expected_regular_file_count}
directory_inventory_path=${directory_inventory_path}
directory_inventory_sha256=$(sha256_of "${directory_inventory}")
directory_count=${expected_directory_count}
complete_inventory_algorithm=cd_failed_root_find_dot_regular_files_c_sort_z_sha256sum_then_sha256sum
complete_inventory_sha256=${expected_complete_inventory_sha256}
failed_runner_path=${failed_runner_path}
failed_runner_sha256=${expected_failed_runner_sha256}
failed_base_config_path=${failed_base_config_path}
failed_base_config_sha256=e9db78f15fb3cd9a0475f51ff99e900bff53dc11d9daf13d1565c2dc3a43d4e7
failure_log_path=${failure_log_path}
failure_log_sha256=${expected_failure_log_sha256}
failure_log_expected_error=[graph_first_config] unable to open file: ${missing_grammar_path}
missing_grammar_path=${missing_grammar_path}
missing_grammar_present=false
observed_endpoint_training_descendant_count=0
observed_job_manifest_count=0
observed_runtime_result_count=0
observed_checkpoint_count=0
observed_probe_count=0
observed_selection_artifact_count=0
observed_certified_artifact_count=0
observed_challenger_training_status_count=0
observed_development_result_count=0
observed_process_exit_status_claim=none
observed_process_liveness_claim=none
config_derivation_source_path=${config_derivation_path}
config_derivation_source_sha256=${expected_config_derivation_sha256}
config_bundle_source_path=${config_bundle_path}
config_bundle_source_sha256=${expected_config_bundle_sha256}
job_runner_source_path=${job_runner_path}
job_runner_source_sha256=${expected_job_runner_sha256}
mtf_launcher_source_path=${mtf_launcher_path}
mtf_launcher_source_sha256=${expected_mtf_launcher_sha256}
runtime_exec_main_source_path=${exec_main_path}
runtime_exec_main_source_sha256=${expected_exec_main_sha256}
runtime_exec_path=${runtime_exec_path}
runtime_exec_sha256=${expected_runtime_exec_sha256}
canonical_mtf_net_grammar_path=${canonical_grammar_path}
canonical_mtf_net_grammar_sha256=${expected_canonical_grammar_sha256}
inference_basis=pinned_config_derivation_config_bundle_job_runner_and_mtf_launcher_control_flow
inferred_wave_settings_load_reached=true
inferred_graph_first_bundle_loaded=false
inferred_runtime_job_creation_reached=false
inferred_optimizer_construction_reached=false
inferred_optimizer_steps=0
resume_authorized=false
partial_artifact_reuse_authorized=false
retry_required=true
retry_schema_id=${retry_schema_id}
retry_runtime_root=${retry_runtime}
retry_restart_from_step_zero=true
retry_requires_explicit_mtf_net_bnf_path=true
retry_requires_effective_grammar_closure=true
canonical_data_raw_access=false
final_holdout_access=false
policy_access=false
RECEIPT
}

verify_receipt_exact() {
  local inspected_receipt="$1"
  local regular_inventory="$2"
  local directory_inventory="$3"
  local frozen_sealer="$4"
  local frozen_amendment="$5"
  reject_symlink_components "${inspected_receipt}"
  require_file "${inspected_receipt}"
  [[ "$(stat -c '%a' -- "${inspected_receipt}")" == 444 ]] ||
    fail "failure closure receipt is not mode 0444"
  [[ "$(stat -c '%h' -- "${inspected_receipt}")" == 1 ]] ||
    fail "failure closure receipt has an unexpected hard-link count"

  local expected_receipt
  new_temp_file expected_receipt "/tmp/${schema_id}.receipt.expected.XXXXXX"
  emit_receipt "${expected_receipt}" "${regular_inventory}" \
    "${directory_inventory}" "${frozen_sealer}" "${frozen_amendment}"
  awk '
    index($0, "=") == 0 { exit 41 }
    {
      key = substr($0, 1, index($0, "=") - 1);
      if (key == "" || seen[key]++) exit 42;
    }
  ' "${inspected_receipt}" ||
    fail "failure closure receipt has a malformed or duplicate key"
  cmp -s -- "${expected_receipt}" "${inspected_receipt}" ||
    fail "failure closure receipt content drifted"
}

verify_closure_tree_at() {
  local root="$1"
  local regular_inventory="${root}/regular_files.inventory.tsv"
  local directory_inventory="${root}/directories.inventory.tsv"
  local receipt="${root}/failure_closure.status"
  local frozen_dir="${root}/frozen_sources"
  local frozen_sealer="${frozen_dir}/$(basename "${script_path}")"
  local frozen_amendment="${frozen_dir}/$(basename "${amendment_path}")"

  reject_symlink_components "${root}"
  require_dir "${root}"
  require_dir "${frozen_dir}"
  local entry rel entry_count=0
  while IFS= read -r -d '' entry; do
    ((entry_count += 1))
    [[ ! -L "${entry}" ]] || fail "closure contains a symlink: ${entry}"
    if [[ "${entry}" == "${root}" ]]; then
      rel="."
    else
      rel="${entry#${root}/}"
    fi
    case "${rel}" in
    . | frozen_sources) [[ -d "${entry}" ]] || fail "closure path is not a directory: ${rel}" ;;
    failure_closure.status | regular_files.inventory.tsv | directories.inventory.tsv | \
      "frozen_sources/$(basename "${script_path}")" | \
      "frozen_sources/$(basename "${amendment_path}")")
      [[ -f "${entry}" ]] || fail "closure path is not a regular file: ${rel}"
      ;;
    *) fail "closure contains an unknown entry: ${rel}" ;;
    esac
  done < <(find "${root}" -mindepth 0 -print0)
  [[ "${entry_count}" == 7 ]] ||
    fail "closure expected seven total entries, found ${entry_count}"

  local path
  for path in "${receipt}" "${regular_inventory}" "${directory_inventory}" \
    "${frozen_sealer}" "${frozen_amendment}"; do
    require_file "${path}"
    [[ "$(stat -c '%a' -- "${path}")" == 444 ]] ||
      fail "closure file is not mode 0444: ${path}"
    [[ "$(stat -c '%h' -- "${path}")" == 1 ]] ||
      fail "closure file has an unexpected hard-link count: ${path}"
  done
  [[ "$(stat -c '%a' -- "${root}")" == 555 ]] ||
    fail "closure root is not mode 0555"
  [[ "$(stat -c '%a' -- "${frozen_dir}")" == 555 ]] ||
    fail "closure frozen-source directory is not mode 0555"

  local expected_regular expected_directories
  new_temp_file expected_regular "/tmp/${schema_id}.published_regular.XXXXXX"
  new_temp_file expected_directories "/tmp/${schema_id}.published_directories.XXXXXX"
  emit_expected_regular_inventory >"${expected_regular}"
  emit_expected_directory_inventory >"${expected_directories}"
  cmp -s -- "${expected_regular}" "${regular_inventory}" ||
    fail "published regular-file inventory content drifted"
  cmp -s -- "${expected_directories}" "${directory_inventory}" ||
    fail "published directory inventory content drifted"
  [[ "$(sha256_of "${frozen_sealer}")" == \
    "${process_start_sealer_sha256}" ]] || fail "frozen sealer hash drifted"
  cmp -s -- "${script_path}" "${frozen_sealer}" ||
    fail "live and frozen failure-closure sealers differ"
  [[ "$(sha256_of "${frozen_amendment}")" == \
    "${expected_amendment_sha256}" ]] || fail "frozen amendment hash drifted"
  cmp -s -- "${amendment_path}" "${frozen_amendment}" ||
    fail "live and frozen recovery amendments differ"
  verify_receipt_exact "${receipt}" "${regular_inventory}" \
    "${directory_inventory}" "${frozen_sealer}" "${frozen_amendment}"
}

build_staging_closure() {
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "external closure staging already exists: ${staging_root}"
  mkdir -- "${staging_root}"
  candidate_created_by_process=true
  mkdir -- "${staging_root}/frozen_sources"
  emit_current_regular_inventory >"${staging_root}/regular_files.inventory.tsv"
  emit_current_directory_inventory >"${staging_root}/directories.inventory.tsv"
  cp -- "${script_path}" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")"
  cp -- "${amendment_path}" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  assert_process_start_source_identity
  emit_receipt "${staging_root}/failure_closure.status" \
    "${staging_root}/regular_files.inventory.tsv" \
    "${staging_root}/directories.inventory.tsv" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  chmod 0444 -- "${staging_root}/failure_closure.status" \
    "${staging_root}/regular_files.inventory.tsv" \
    "${staging_root}/directories.inventory.tsv" \
    "${staging_root}/frozen_sources/$(basename "${script_path}")" \
    "${staging_root}/frozen_sources/$(basename "${amendment_path}")"
  chmod 0555 -- "${staging_root}/frozen_sources" "${staging_root}"
  verify_closure_tree_at "${staging_root}"
}

publish_staging_closure() {
  [[ ! -e "${closure_root}" && ! -L "${closure_root}" ]] ||
    fail "refusing to overwrite existing closure runtime"
  mv -T -n -- "${staging_root}" "${closure_root}"
  if [[ -e "${staging_root}" || -L "${staging_root}" ]]; then
    fail "atomic no-clobber closure publication did not consume staging"
  fi
  candidate_created_by_process=false
}

acquire_failed_runtime_lock() {
  reject_symlink_components "${failed_lock_path}"
  require_file "${failed_lock_path}"
  exec {failed_lock_fd}<"${failed_lock_path}"
  flock -n "${failed_lock_fd}" ||
    fail "failed runtime development lock is held; refusing to inspect"
}

verify_published() {
  verify_failed_attempt
  [[ ! -e "${staging_root}" && ! -L "${staging_root}" ]] ||
    fail "external closure staging residue exists: ${staging_root}"
  verify_closure_tree_at "${closure_root}"
  verify_failed_attempt
  echo "failure_closure_receipt=${receipt_path}"
  echo "failure_closure_receipt_sha256=$(sha256_of "${receipt_path}")"
}

if [[ "${mode}" == --plan ]]; then
  print_plan
  exit 0
fi

acquire_failed_runtime_lock
verify_failed_attempt

if [[ "${mode}" == --audit ]]; then
  echo "failed_runtime_audit=complete"
  echo "failed_complete_inventory_sha256=${expected_complete_inventory_sha256}"
  exit 0
fi

if [[ "${mode}" == --verify ]]; then
  verify_published
  exit 0
fi

if [[ -e "${closure_root}" || -L "${closure_root}" ]]; then
  verify_published
  exit 0
fi

build_staging_closure
verify_failed_attempt
publish_staging_closure
verify_published
