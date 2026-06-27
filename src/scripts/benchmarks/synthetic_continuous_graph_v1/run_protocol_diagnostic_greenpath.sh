#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
config_path="${benchmark_root}/synthetic_benchmark.config"
benchmark_artifacts_dir="${benchmark_root}/artifacts"
manifest_path="${benchmark_artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"
runtime_root="${repo_root}/.runtime/cuwacunu_exec"
out_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/diagnostic_greenpath_v1"
raw_dir="${out_root}/raw"
oracle_input_dir="${out_root}/oracle_input"
report_path="${out_root}/synthetic_protocol_diagnostic_greenpath.v1.md"
policy_config_path="${raw_dir}/synthetic_benchmark.policy_training_ppo_v0.config"

mkdir -p "${raw_dir}" "${oracle_input_dir}"

run_capture() {
  local label="$1"
  local outfile="$2"
  shift 2
  set +e
  "$@" >"${outfile}" 2>&1
  local code=$?
  set -e
  printf '%s=%s\n' "${label}_exit_code" "${code}" >"${outfile}.status"
}

json_field_first() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":\"[^\"]*\"" "${file}" | head -1 |
    sed -E "s/^\"${field}\":\"(.*)\"$/\\1/" || true
}

json_number_first() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":[0-9][0-9]*" "${file}" | head -1 |
    sed -E "s/^\"${field}\"://" || true
}

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

status_code() {
  local status_file="$1"
  awk -F= '{print $2}' "${status_file}" 2>/dev/null | head -1
}

require_file() {
  if [[ ! -f "$1" ]]; then
    echo "required file missing: $1" >&2
    exit 2
  fi
}

config_mcp="${repo_root}/.build/hero/hero_config.mcp"
runtime_mcp="${repo_root}/.build/hero/hero_runtime.mcp"
lattice_mcp="${repo_root}/.build/hero/hero_lattice.mcp"
marshal_mcp="${repo_root}/.build/hero/hero_marshal.mcp"

require_file "${config_mcp}"
require_file "${runtime_mcp}"
require_file "${lattice_mcp}"
require_file "${marshal_mcp}"
require_file "${config_path}"

write_policy_training_config_copy() {
  : >"$2"
  local line indent key sep value
  while IFS= read -r line || [[ -n "${line}" ]]; do
    if [[ "${line}" =~ ^([[:space:]]*)([A-Za-z0-9_]+)([[:space:]]*=[[:space:]]*)(.*)$ ]]; then
      indent="${BASH_REMATCH[1]}"
      key="${BASH_REMATCH[2]}"
      sep="${BASH_REMATCH[3]}"
      value="${BASH_REMATCH[4]}"
      if [[ "${key}" == "runtime_wave_id" ]]; then
        value="policy_training_ppo_v0"
      elif [[ "${key}" == *_path && -n "${value}" && "${value}" != /* ]]; then
        value="$(realpath -m "${benchmark_root}/${value}")"
      fi
      printf '%s%s%s%s\n' "${indent}" "${key}" "${sep}" "${value}" >>"$2"
    else
      printf '%s\n' "${line}" >>"$2"
    fi
  done <"$1"
}

write_policy_training_config_copy "${config_path}" "${policy_config_path}"

"${script_dir}/generate_synthetic_klines.sh" >"${raw_dir}/generate_synthetic_klines.log" 2>&1
require_file "${manifest_path}"

run_capture config_status "${raw_dir}/hero_config_status.json" \
  "${config_mcp}" --global-config "${config_path}" --tool hero.config.status \
  --args-json '{}'

run_capture config_bundle "${raw_dir}/hero_config_bundle.json" \
  "${config_mcp}" --global-config "${config_path}" \
  --tool hero.config.inspect.bundle --args-json '{}'

run_capture runtime_status "${raw_dir}/hero_runtime_status.json" \
  "${runtime_mcp}" --global-config "${config_path}" --tool hero.runtime.status \
  --args-json '{}'

run_capture runtime_wave "${raw_dir}/hero_runtime_inspect_wave.json" \
  "${runtime_mcp}" --global-config "${config_path}" \
  --tool hero.runtime.inspect.wave --args-json '{}'

run_capture runtime_policy_wave "${raw_dir}/hero_runtime_policy_training_inspect_wave.json" \
  "${runtime_mcp}" --global-config "${policy_config_path}" \
  --tool hero.runtime.inspect.wave --args-json '{}'

run_capture lattice_status "${raw_dir}/hero_lattice_status.json" \
  "${lattice_mcp}" --global-config "${config_path}" --tool hero.lattice.status \
  --args-json '{}'

run_capture lattice_rep_deficit "${raw_dir}/lattice_cwu_02v_representation_train_core_ready.json" \
  "${lattice_mcp}" --global-config "${config_path}" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"cwu_02v_representation_train_core_ready"}'

run_capture lattice_mdn_deficit "${raw_dir}/lattice_cwu_02v_mdn_train_core_ready.json" \
  "${lattice_mcp}" --global-config "${config_path}" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"cwu_02v_mdn_train_core_ready"}'

run_capture lattice_policy_deficit "${raw_dir}/lattice_policy_training_artifact_ready.json" \
  "${lattice_mcp}" --global-config "${config_path}" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"policy_training_artifact_ready"}'

run_capture marshal_rep_plan "${raw_dir}/marshal_prepare_cwu_02v_representation_plan.json" \
  "${marshal_mcp}" --global-config "${config_path}" \
  --tool hero.marshal.prepare.train \
  --args-json '{"target_id":"cwu_02v_representation_train_core_ready","mode":"plan","profile":"single_wave_operator"}'

run_capture marshal_mdn_plan "${raw_dir}/marshal_prepare_cwu_02v_mdn_plan.json" \
  "${marshal_mcp}" --global-config "${config_path}" \
  --tool hero.marshal.prepare.train \
  --args-json '{"target_id":"cwu_02v_mdn_train_core_ready","mode":"plan","profile":"single_wave_operator"}'

run_capture marshal_policy_plan "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json" \
  "${marshal_mcp}" --global-config "${config_path}" \
  --tool hero.marshal.prepare.train \
  --args-json '{"target_id":"policy_training_artifact_ready","mode":"plan","profile":"single_wave_operator"}'

run_capture runtime_policy_dry_run "${raw_dir}/hero_runtime_policy_training_dry_run.json" \
  "${runtime_mcp}" --global-config "${policy_config_path}" --tool hero.runtime.run \
  --args-json '{"mode":"dry_run"}'

alpha_csv="${benchmark_root}/data/raw/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_csv="${benchmark_root}/data/raw/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_csv="${benchmark_root}/data/raw/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
require_file "${alpha_csv}"
require_file "${beta_csv}"
require_file "${gamma_csv}"

row_count="$(kv row_count "${manifest_path}")"
input_length="$(kv input_length "${manifest_path}")"
future_length=1
accepted_count="$(kv accepted_anchor_count "${manifest_path}")"
train_begin="$(kv train_anchor_begin "${manifest_path}")"
train_end="$(kv train_anchor_end_exclusive "${manifest_path}")"
eval_begin="$(kv eval_anchor_begin "${manifest_path}")"
eval_end="$(kv eval_anchor_end_exclusive "${manifest_path}")"
test_begin="$(kv test_anchor_begin "${manifest_path}")"
test_end="$(kv test_anchor_end_exclusive "${manifest_path}")"

{
  echo "anchor_begin=${eval_begin}"
  echo "anchor_end=${eval_end}"
} >"${oracle_input_dir}/lattice.exposure.fact"

"${script_dir}/compute_hindsight_oracle.sh" \
  --runtime-job-dir "${oracle_input_dir}" \
  --eval-anchor-begin "${eval_begin}" \
  --eval-anchor-end "${eval_end}" \
  --input-length "${input_length}" \
  --output "${out_root}/synthetic_hindsight_oracle.v1.report" \
  >"${raw_dir}/compute_hindsight_oracle.log" 2>&1

baseline_report="${out_root}/synthetic_baselines.v1.report"
awk -F, \
  -v alpha_file="${alpha_csv}" \
  -v beta_file="${beta_csv}" \
  -v gamma_file="${gamma_csv}" \
  -v begin="${eval_begin}" \
  -v end="${eval_end}" \
  -v input_length="${input_length}" \
  -v report_path="${baseline_report}" '
function abs(x) { return x < 0.0 ? -x : x; }
function log_growth(w0, w1, w2, w3, r1, r2, r3) {
  return log(w0 + w1 * exp(r1) + w2 * exp(r2) + w3 * exp(r3));
}
BEGIN {
  nodes[1] = "SYNALPHA";
  nodes[2] = "SYNBETA";
  nodes[3] = "SYNGAMMA";
  max_weight = 0.95;
  reserve = 0.05;
  raw_begin = begin + input_length - 1;
  raw_end = end + input_length - 1;
}
FILENAME == alpha_file { close_px["SYNALPHA", FNR - 1] = $5 + 0.0; }
FILENAME == beta_file { close_px["SYNBETA", FNR - 1] = $5 + 0.0; }
FILENAME == gamma_file { close_px["SYNGAMMA", FNR - 1] = $5 + 0.0; }
END {
  for (raw = raw_begin; raw < raw_end; ++raw) {
    for (i = 1; i <= 3; ++i) {
      node = nodes[i];
      r[node] = log(close_px[node, raw + 1] / close_px[node, raw]);
      prev_r[node] = log(close_px[node, raw] / close_px[node, raw - 1]);
    }
    equal_log += log_growth(0.25, 0.25, 0.25, 0.25,
                            r["SYNALPHA"], r["SYNBETA"], r["SYNGAMMA"]);
    for (i = 1; i <= 3; ++i) {
      node = nodes[i];
      fixed_log[node] += log_growth(reserve,
                                    node == "SYNALPHA" ? max_weight : 0.0,
                                    node == "SYNBETA" ? max_weight : 0.0,
                                    node == "SYNGAMMA" ? max_weight : 0.0,
                                    r["SYNALPHA"], r["SYNBETA"], r["SYNGAMMA"]);
    }
    selected = "SYNUSD";
    selected_prev = 0.0;
    for (i = 1; i <= 3; ++i) {
      node = nodes[i];
      if (prev_r[node] > selected_prev ||
          (abs(prev_r[node] - selected_prev) <= 1e-12 && selected != "SYNUSD" && node < selected)) {
        selected = node;
        selected_prev = prev_r[node];
      }
    }
    if (selected == "SYNUSD") {
      momentum_log += 0.0;
    } else {
      momentum_log += log_growth(reserve,
                                 selected == "SYNALPHA" ? max_weight : 0.0,
                                 selected == "SYNBETA" ? max_weight : 0.0,
                                 selected == "SYNGAMMA" ? max_weight : 0.0,
                                 r["SYNALPHA"], r["SYNBETA"], r["SYNGAMMA"]);
    }
    step_count += 1;
  }
  best_node = "SYNALPHA";
  best_log = fixed_log[best_node];
  for (i = 2; i <= 3; ++i) {
    node = nodes[i];
    if (fixed_log[node] > best_log) {
      best_node = node;
      best_log = fixed_log[node];
    }
  }
  print "schema_id=synthetic_baselines.v1" > report_path;
  print "benchmark_id=synthetic_continuous_graph_v1" >> report_path;
  print "protocol_id=cwu_02v" >> report_path;
  print "eval_anchor_begin=" begin >> report_path;
  print "eval_anchor_end_exclusive=" end >> report_path;
  print "input_length=" input_length >> report_path;
  print "baseline_step_count=" step_count >> report_path;
  printf("numeraire_only_total_log_growth=%.12f\n", 0.0) >> report_path;
  printf("numeraire_only_final_equity=%.12f\n", 1.0) >> report_path;
  printf("equal_weight_total_log_growth=%.12f\n", equal_log) >> report_path;
  printf("equal_weight_final_equity=%.12f\n", exp(equal_log)) >> report_path;
  print "best_fixed_asset_node=" best_node >> report_path;
  printf("best_fixed_asset_total_log_growth=%.12f\n", best_log) >> report_path;
  printf("best_fixed_asset_final_equity=%.12f\n", exp(best_log)) >> report_path;
  printf("one_step_momentum_total_log_growth=%.12f\n", momentum_log) >> report_path;
  printf("one_step_momentum_final_equity=%.12f\n", exp(momentum_log)) >> report_path;
}
' "${alpha_csv}" "${beta_csv}" "${gamma_csv}"

config_complete="$(grep -ao '"complete":[^,}]*' "${raw_dir}/hero_config_bundle.json" | head -1 | cut -d: -f2 || true)"
config_file_count="$(json_number_first file_count "${raw_dir}/hero_config_bundle.json")"
runtime_wave="$(json_field_first selected_wave_id "${raw_dir}/hero_runtime_status.json")"
runtime_protocol="$(json_field_first protocol_id "${raw_dir}/hero_runtime_status.json")"
runtime_job_kind="$(json_field_first job_kind "${raw_dir}/hero_runtime_status.json")"
runtime_target="$(json_field_first target_component_family_id "${raw_dir}/hero_runtime_status.json")"
runtime_policy_wave="$(json_field_first selected_wave_id "${raw_dir}/hero_runtime_policy_training_inspect_wave.json")"
runtime_policy_job_kind="$(json_field_first job_kind "${raw_dir}/hero_runtime_policy_training_inspect_wave.json")"
lattice_split_count="$(json_number_first split_count "${raw_dir}/hero_lattice_status.json")"
lattice_target_count="$(json_number_first target_count "${raw_dir}/hero_lattice_status.json")"
lattice_source_cursor="$(json_field_first source_cursor_token "${raw_dir}/hero_lattice_status.json")"
rep_primary_deficit="$(json_field_first primary_deficit_key "${raw_dir}/lattice_cwu_02v_representation_train_core_ready.json")"
mdn_primary_deficit="$(json_field_first primary_deficit_key "${raw_dir}/lattice_cwu_02v_mdn_train_core_ready.json")"
policy_primary_deficit="$(json_field_first primary_deficit_key "${raw_dir}/lattice_policy_training_artifact_ready.json")"
rep_blocker="$(json_field_first blocker_bucket "${raw_dir}/marshal_prepare_cwu_02v_representation_plan.json")"
mdn_blocker="$(json_field_first blocker_bucket "${raw_dir}/marshal_prepare_cwu_02v_mdn_plan.json")"
policy_blocker="$(json_field_first blocker_bucket "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json")"
runtime_dry_error="$(json_field_first message "${raw_dir}/hero_runtime_policy_training_dry_run.json")"
runtime_dry_status="$(status_code "${raw_dir}/hero_runtime_policy_training_dry_run.json.status")"
if [[ -z "${runtime_dry_error}" ]]; then
  runtime_dry_error="exit_code=${runtime_dry_status}"
fi

oracle_equity="$(awk -F= '$1=="oracle_final_equity_numeraire"{print $2}' "${out_root}/synthetic_hindsight_oracle.v1.report")"
oracle_log="$(awk -F= '$1=="oracle_total_log_growth"{print $2}' "${out_root}/synthetic_hindsight_oracle.v1.report")"
equal_equity="$(awk -F= '$1=="equal_weight_final_equity"{print $2}' "${baseline_report}")"
best_fixed_node="$(awk -F= '$1=="best_fixed_asset_node"{print $2}' "${baseline_report}")"
best_fixed_equity="$(awk -F= '$1=="best_fixed_asset_final_equity"{print $2}' "${baseline_report}")"
momentum_equity="$(awk -F= '$1=="one_step_momentum_final_equity"{print $2}' "${baseline_report}")"

source_identity_status="unknown"
source_identity_fixed="false"
if [[ "${lattice_source_cursor}" == *"SYN"* && "${lattice_source_cursor}" == *"accepted=${accepted_count}"* ]]; then
  source_identity_status="synthetic"
  source_identity_fixed="true"
elif [[ "${lattice_source_cursor}" == *"SYN"* ]]; then
  source_identity_status="synthetic-looking-but-anchor-count-mismatch"
elif [[ "${lattice_source_cursor}" == *"BTCUSDT"* ]]; then
  source_identity_status="market-looking"
else
  source_identity_status="not-synthetic-looking"
fi

if [[ "${source_identity_fixed}" == "true" ]]; then
  verdict_detail="The synthetic source identity is fixed. The greenpath is still not runnable through policy evaluation because the clean runtime has no representation/MDN reports or certified MDN replay artifact yet. The checked-in benchmark config currently selects the representation-training wave; this report uses a local dry-run config copy only for the explicit policy-training probe."
  identity_guidance="The benchmark config overlay is loaded and Lattice status now reports the expected synthetic source cursor:"
  next_required_fix="Before performance testing or PPO tuning, run the official stages through Marshal-aligned waves. The remaining blockers are training/evidence blockers, not a source-identity mismatch."
else
  verdict_detail="The diagnostic greenpath is not yet runnable through policy evaluation."
  identity_guidance="The benchmark config overlay is loaded, but Lattice status currently reports:"
  next_required_fix="Before performance testing or PPO tuning, make the official Runtime/Lattice greenpath prove that the active source cursor is the synthetic graph."
fi

cat >"${report_path}" <<EOF_REPORT
# synthetic_protocol_diagnostic_greenpath.v1

Generated at: $(date -u +"%Y-%m-%dT%H:%M:%SZ")

## Verdict

${verdict_detail}

Primary finding:

\`\`\`text
Lattice active source identity is ${source_identity_status}.
\`\`\`

${identity_guidance}

\`\`\`text
${lattice_source_cursor}
\`\`\`

For this synthetic benchmark, the expected source identity is a SYN* reference
edge with ${accepted_count} accepted anchors from the 9 local SYN* CSVs.

## Config And Tool Status

| Check | Result |
|---|---:|
| Config bundle complete | ${config_complete} |
| Config bundle file count | ${config_file_count} |
| Runtime active wave | ${runtime_wave} |
| Runtime protocol | ${runtime_protocol} |
| Runtime target | ${runtime_target} |
| Runtime job kind | ${runtime_job_kind} |
| Runtime policy dry-run wave | ${runtime_policy_wave} |
| Runtime policy dry-run job kind | ${runtime_policy_job_kind} |
| Lattice split count | ${lattice_split_count} |
| Lattice target count | ${lattice_target_count} |

## Official Greenpath State

| Stage | Target | Current result |
|---|---|---|
| Representation | cwu_02v_representation_train_core_ready | Lattice deficit: ${rep_primary_deficit}; Marshal blocker: ${rep_blocker} |
| MDN | cwu_02v_mdn_train_core_ready | Lattice deficit: ${mdn_primary_deficit}; Marshal blocker: ${mdn_blocker} |
| Policy artifact | policy_training_artifact_ready | Lattice deficit: ${policy_primary_deficit}; Marshal blocker: ${policy_blocker} |
| Runtime policy dry-run | ${runtime_policy_wave} | ${runtime_dry_error} |

The policy dry-run uses the report-local config copy shown in the raw evidence
directory so the checked-in benchmark config can remain on the active
representation-training wave. Its blocker is expected after a clean runtime if
no certified MDN replay job exists yet. Do not interpret policy quality until
the representation, MDN, replay, and policy stages have produced fresh
synthetic evidence.

## Synthetic Data Expectations

| Item | Value |
|---|---:|
| 1d rows per synthetic asset | ${row_count} |
| Input length | ${input_length} |
| Future length | ${future_length} |
| Expected synthetic accepted anchors | ${accepted_count} |
| Train range | [${train_begin}, ${train_end}) |
| Certified replay/eval range | [${eval_begin}, ${eval_end}) |
| Test range | [${test_begin}, ${test_end}) |

## Synthetic Oracle And Baselines

These metrics are computed directly from the synthetic CSVs over the expected
synthetic certified replay/eval range. They are diagnostic comparators only.

| Comparator | Final equity | Notes |
|---|---:|---|
| Numeraire-only | 1.000000000000 | Causal baseline |
| Equal-weight | ${equal_equity} | Causal baseline |
| Best fixed asset | ${best_fixed_equity} | Best asset: ${best_fixed_node}; hindsight over eval range |
| One-step momentum | ${momentum_equity} | Causal one-step lag heuristic |
| Hindsight oracle | ${oracle_equity} | Non-causal ceiling; total log growth ${oracle_log} |

Raw evidence is stored under:

\`\`\`text
${raw_dir}
${policy_config_path}
${out_root}/synthetic_hindsight_oracle.v1.report
${baseline_report}
\`\`\`

## Next Required Fix

${next_required_fix}

\`\`\`text
expected: reference_edge contains SYN* and accepted anchors match the synthetic
CSV domain
actual:   ${lattice_source_cursor}
\`\`\`

Run the stages in order through Marshal-aligned waves:

1. cwu_02v_representation_train_core_ready
2. cwu_02v_mdn_train_core_ready
3. cwu_02v certified replay / MDN eval target
4. policy_training_ppo_v0
5. policy_training_artifact_ready inspection

No policy-quality conclusion should be drawn from this report.
EOF_REPORT

printf '%s\n' "${report_path}"
