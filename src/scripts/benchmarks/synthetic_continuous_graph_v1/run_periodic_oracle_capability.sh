#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
config_path="${benchmark_root}/synthetic_benchmark.config"
benchmark_artifacts_dir="${benchmark_root}/artifacts"
manifest_path="${benchmark_artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"
data_root="${benchmark_root}/data/raw"
out_root="${repo_root}/.runtime/benchmarks/synthetic_continuous_graph_v1/periodic_oracle_capability_v1"
raw_dir="${out_root}/raw"
oracle_input_dir="${out_root}/oracle_input"
artifacts_dir="${out_root}/artifacts"
report_path="${out_root}/synthetic_periodic_oracle_protocol_capability.v1.report"
markdown_path="${out_root}/synthetic_periodic_oracle_protocol_capability.v1.md"

policy_report_path=""
threshold_vs_oracle="0.75"
threshold_vs_causal_baseline="1.00"

while [[ $# -gt 0 ]]; do
  case "$1" in
  --policy-report)
    policy_report_path="${2:-}"
    shift 2
    ;;
  --threshold-vs-oracle)
    threshold_vs_oracle="${2:-}"
    shift 2
    ;;
  --threshold-vs-causal-baseline)
    threshold_vs_causal_baseline="${2:-}"
    shift 2
    ;;
  *)
    echo "unknown argument: $1" >&2
    exit 2
    ;;
  esac
done

mkdir -p "${raw_dir}" "${oracle_input_dir}" "${artifacts_dir}"

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

first_metric() {
  local file="$1"
  shift
  local key
  for key in "$@"; do
    local value
    value="$(kv "${key}" "${file}")"
    if [[ -n "${value}" ]]; then
      printf '%s\n' "${value}"
      return 0
    fi
  done
}

reinforcement_learning_policy_final_equity() {
  local file="$1"
  awk -F= '
    $1 ~ /^policy_[0-9]+_kind$/ && $2 == "reinforcement_learning" {
      prefix = $1
      sub(/_kind$/, "", prefix)
      wanted = prefix "_mean_final_equity_numeraire"
    }
    wanted != "" && $1 == wanted {
      print $2
      exit
    }
  ' "${file}" 2>/dev/null || true
}

policy_metric_by_id() {
  local file="$1"
  local policy_id="$2"
  local metric_suffix="$3"
  awk -F= -v target_id="${policy_id}" -v suffix="${metric_suffix}" '
    $1 ~ /^policy_[0-9]+_id$/ && $2 == target_id {
      prefix = $1
      sub(/_id$/, "", prefix)
      wanted = prefix "_" suffix
    }
    wanted != "" && $1 == wanted {
      print $2
      exit
    }
  ' "${file}" 2>/dev/null || true
}

json_field_first() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":\"[^\"]*\"" "${file}" 2>/dev/null | head -1 |
    sed -E "s/^\"${field}\":\"(.*)\"$/\\1/" || true
}

json_bool_first() {
  local field="$1"
  local file="$2"
  grep -ao "\"${field}\":\\(true\\|false\\)" "${file}" 2>/dev/null | head -1 |
    sed -E "s/^\"${field}\"://" || true
}

status_code() {
  local status_file="$1"
  awk -F= '{print $2}' "${status_file}" 2>/dev/null | head -1 || true
}

run_optional_capture() {
  local label="$1"
  local outfile="$2"
  local exe="$3"
  shift 3
  if [[ ! -x "${exe}" ]]; then
    printf 'missing executable: %s\n' "${exe}" >"${outfile}"
    printf '%s=%s\n' "${label}_exit_code" "127" >"${outfile}.status"
    return 0
  fi
  set +e
  "${exe}" "$@" >"${outfile}" 2>&1
  local code=$?
  set -e
  printf '%s=%s\n' "${label}_exit_code" "${code}" >"${outfile}.status"
}

ratio_or_empty() {
  local numerator="$1"
  local denominator="$2"
  if [[ -z "${numerator}" || -z "${denominator}" ]]; then
    return 0
  fi
  awk -v n="${numerator}" -v d="${denominator}" \
    'BEGIN { if (d <= 0.0) exit 1; printf("%.12f", n / d) }' || true
}

"${script_dir}/generate_synthetic_klines.sh" \
  >"${raw_dir}/generate_synthetic_klines.log" 2>&1

if [[ ! -f "${manifest_path}" ]]; then
  echo "synthetic chart manifest missing: ${manifest_path}" >&2
  exit 2
fi

row_count="$(kv row_count "${manifest_path}")"
input_length="$(kv input_length "${manifest_path}")"
accepted_count="$(kv accepted_anchor_count "${manifest_path}")"
eval_begin="$(kv eval_anchor_begin "${manifest_path}")"
eval_end="$(kv eval_anchor_end_exclusive "${manifest_path}")"
test_begin="$(kv test_anchor_begin "${manifest_path}")"
test_end="$(kv test_anchor_end_exclusive "${manifest_path}")"
periodic_chart_sufficient_cycles="$(kv periodic_chart_sufficient_cycles "${manifest_path}")"
minimum_train_cycles="$(kv minimum_train_cycles "${manifest_path}")"
minimum_eval_cycles="$(kv minimum_eval_cycles "${manifest_path}")"
minimum_test_cycles="$(kv minimum_test_cycles "${manifest_path}")"

{
  echo "anchor_begin=${eval_begin}"
  echo "anchor_end=${eval_end}"
} >"${oracle_input_dir}/lattice.exposure.fact"

"${script_dir}/compute_hindsight_oracle.sh" \
  --runtime-job-dir "${oracle_input_dir}" \
  --eval-anchor-begin "${eval_begin}" \
  --eval-anchor-end "${eval_end}" \
  --input-length "${input_length}" \
  --output "${artifacts_dir}/synthetic_hindsight_oracle.v1.report" \
  >"${raw_dir}/compute_hindsight_oracle.log" 2>&1

alpha_csv="${data_root}/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_csv="${data_root}/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_csv="${data_root}/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
baseline_report="${artifacts_dir}/synthetic_periodic_baselines.v1.report"

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
          (abs(prev_r[node] - selected_prev) <= 1e-12 &&
           selected != "SYNUSD" && node < selected)) {
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
  best_causal_log = equal_log;
  best_causal_name = "equal_weight";
  if (momentum_log > best_causal_log) {
    best_causal_log = momentum_log;
    best_causal_name = "one_step_momentum";
  }
  if (0.0 > best_causal_log) {
    best_causal_log = 0.0;
    best_causal_name = "numeraire_only";
  }
  best_reference_log = best_causal_log;
  best_reference_name = best_causal_name;
  if (best_log > best_reference_log) {
    best_reference_log = best_log;
    best_reference_name = "best_fixed_asset";
  }
  print "schema_id=synthetic_periodic_baselines.v1" > report_path;
  print "benchmark_id=synthetic_continuous_graph_v1" >> report_path;
  print "protocol_id=cwu_02v" >> report_path;
  print "eval_anchor_begin=" begin >> report_path;
  print "eval_anchor_end_exclusive=" end >> report_path;
  print "input_length=" input_length >> report_path;
  print "baseline_step_count=" step_count >> report_path;
  printf("numeraire_only_final_equity=%.12f\n", 1.0) >> report_path;
  printf("equal_weight_final_equity=%.12f\n", exp(equal_log)) >> report_path;
  print "best_fixed_asset_node=" best_node >> report_path;
  printf("best_fixed_asset_final_equity=%.12f\n", exp(best_log)) >> report_path;
  printf("one_step_momentum_final_equity=%.12f\n", exp(momentum_log)) >> report_path;
  print "best_causal_baseline_name=" best_causal_name >> report_path;
  printf("best_causal_baseline_final_equity=%.12f\n", exp(best_causal_log)) >> report_path;
  print "best_reference_baseline_name=" best_reference_name >> report_path;
  printf("best_reference_baseline_final_equity=%.12f\n", exp(best_reference_log)) >> report_path;
}
' "${alpha_csv}" "${beta_csv}" "${gamma_csv}"

config_mcp="${repo_root}/.build/hero/hero_config.mcp"
runtime_mcp="${repo_root}/.build/hero/hero_runtime.mcp"
lattice_mcp="${repo_root}/.build/hero/hero_lattice.mcp"
marshal_mcp="${repo_root}/.build/hero/hero_marshal.mcp"

run_optional_capture config_status "${raw_dir}/hero_config_status.json" \
  "${config_mcp}" --global-config "${config_path}" --tool hero.config.status \
  --args-json '{}'

run_optional_capture runtime_wave "${raw_dir}/hero_runtime_inspect_wave.json" \
  "${runtime_mcp}" --global-config "${config_path}" \
  --tool hero.runtime.inspect.wave --args-json '{}'

run_optional_capture lattice_status "${raw_dir}/hero_lattice_status.json" \
  "${lattice_mcp}" --global-config "${config_path}" --tool hero.lattice.status \
  --args-json '{}'

run_optional_capture marshal_policy_plan "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json" \
  "${marshal_mcp}" --global-config "${config_path}" \
  --tool hero.marshal.prepare.train \
  --args-json '{"target_id":"policy_training_artifact_ready","mode":"plan","profile":"single_wave_operator"}'

config_status_code="$(status_code "${raw_dir}/hero_config_status.json.status")"
runtime_wave_status_code="$(status_code "${raw_dir}/hero_runtime_inspect_wave.json.status")"
lattice_status_code="$(status_code "${raw_dir}/hero_lattice_status.json.status")"
marshal_policy_status_code="$(status_code "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json.status")"
runtime_wave_message="$(json_field_first message "${raw_dir}/hero_runtime_inspect_wave.json")"
lattice_status_message="$(json_field_first message "${raw_dir}/hero_lattice_status.json")"
marshal_policy_blocker="$(json_field_first blocker_bucket "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json")"
marshal_policy_ok="$(json_bool_first ok "${raw_dir}/marshal_prepare_policy_training_artifact_plan.json")"

protocol_probe_status="ok"
protocol_probe_reason="protocol probe commands completed"
if [[ "${config_status_code}" != "0" ]]; then
  protocol_probe_status="blocked"
  protocol_probe_reason="hero.config.status failed"
elif [[ "${runtime_wave_status_code}" != "0" ]]; then
  protocol_probe_status="blocked"
  protocol_probe_reason="hero.runtime.inspect.wave failed"
elif [[ "${lattice_status_code}" != "0" ]]; then
  protocol_probe_status="blocked"
  protocol_probe_reason="hero.lattice.status failed"
elif [[ "${marshal_policy_status_code}" != "0" ]]; then
  protocol_probe_status="blocked"
  protocol_probe_reason="hero.marshal.prepare.train policy plan failed"
fi

oracle_equity="$(kv oracle_final_equity_numeraire "${artifacts_dir}/synthetic_hindsight_oracle.v1.report")"
equal_equity="$(kv equal_weight_final_equity "${baseline_report}")"
best_fixed_node="$(kv best_fixed_asset_node "${baseline_report}")"
best_fixed_equity="$(kv best_fixed_asset_final_equity "${baseline_report}")"
momentum_equity="$(kv one_step_momentum_final_equity "${baseline_report}")"
best_causal_name="$(kv best_causal_baseline_name "${baseline_report}")"
best_causal_equity="$(kv best_causal_baseline_final_equity "${baseline_report}")"
best_reference_name="$(kv best_reference_baseline_name "${baseline_report}")"
best_reference_equity="$(kv best_reference_baseline_final_equity "${baseline_report}")"

policy_report_status="missing"
policy_final_equity=""
policy_initial_equity=""
policy_growth_multiple=""
if [[ -n "${policy_report_path}" ]]; then
  if [[ -f "${policy_report_path}" ]]; then
    policy_report_status="present"
    policy_final_equity="$(
      first_metric "${policy_report_path}" \
        policy_final_equity_numeraire final_policy_equity_numeraire \
        policy_eval_final_equity_numeraire final_equity_numeraire \
        final_equity
    )"
    if [[ -z "${policy_final_equity}" ]]; then
      policy_final_equity="$(
        reinforcement_learning_policy_final_equity "${policy_report_path}"
      )"
    fi
    policy_initial_equity="$(
      first_metric "${policy_report_path}" initial_equity_numeraire \
        policy_initial_equity_numeraire
    )"
    if [[ -z "${policy_initial_equity}" ]]; then
      policy_initial_equity="$(
        policy_metric_by_id "${policy_report_path}" numeraire_only.v1 \
          mean_final_equity_numeraire
      )"
    fi
    if [[ -n "${policy_final_equity}" ]]; then
      if [[ -n "${policy_initial_equity}" ]]; then
        policy_growth_multiple="$(
          ratio_or_empty "${policy_final_equity}" "${policy_initial_equity}"
        )"
      else
        policy_growth_multiple="${policy_final_equity}"
      fi
    fi
    if [[ -z "${policy_final_equity}" ]]; then
      policy_report_status="present_metric_missing"
    fi
  else
    policy_report_status="path_missing"
  fi
fi

policy_vs_oracle_ratio="$(ratio_or_empty "${policy_growth_multiple}" "${oracle_equity}")"
policy_vs_best_causal_baseline_ratio="$(
  ratio_or_empty "${policy_growth_multiple}" "${best_causal_equity}"
)"
policy_vs_best_reference_baseline_ratio="$(
  ratio_or_empty "${policy_growth_multiple}" "${best_reference_equity}"
)"

verdict="unproven"
verdict_reason="no policy performance report was supplied"
if [[ "${periodic_chart_sufficient_cycles}" != "true" ]]; then
  verdict="blocked"
  verdict_reason="periodic synthetic chart does not have sufficient cycles"
elif [[ "${protocol_probe_status}" == "blocked" ]]; then
  verdict="blocked"
  verdict_reason="${protocol_probe_reason}"
elif [[ -n "${policy_report_path}" && "${policy_report_status}" != "present" ]]; then
  verdict="blocked"
  verdict_reason="policy report was requested but not readable or missing a final equity metric"
elif [[ -n "${policy_final_equity}" ]]; then
  if awk -v r="${policy_vs_oracle_ratio}" -v t="${threshold_vs_oracle}" \
      'BEGIN { exit !(r >= t) }' &&
    awk -v r="${policy_vs_best_causal_baseline_ratio}" \
      -v t="${threshold_vs_causal_baseline}" \
      'BEGIN { exit !(r >= t) }'; then
    verdict="pass"
    verdict_reason="policy met the causal baseline and oracle-ratio thresholds"
  else
    verdict="fail"
    verdict_reason="policy did not meet the causal baseline and oracle-ratio thresholds"
  fi
fi

{
  echo "schema_id=synthetic_periodic_oracle_protocol_capability.v1"
  echo "benchmark_id=synthetic_continuous_graph_v1"
  echo "protocol_id=cwu_02v"
  echo "capability_question=can_protocol_win_simple_deterministic_periodic_continuous_charts"
  echo "protocol_capable_on_synthetic_periodic_charts=${verdict}"
  echo "capability_result_reason=${verdict_reason}"
  echo "chart_manifest_path=${manifest_path}"
  echo "row_count=${row_count}"
  echo "accepted_anchor_count=${accepted_count}"
  echo "eval_anchor_begin=${eval_begin}"
  echo "eval_anchor_end_exclusive=${eval_end}"
  echo "test_anchor_begin=${test_begin}"
  echo "test_anchor_end_exclusive=${test_end}"
  echo "minimum_train_cycles=${minimum_train_cycles}"
  echo "minimum_eval_cycles=${minimum_eval_cycles}"
  echo "minimum_test_cycles=${minimum_test_cycles}"
  echo "periodic_chart_sufficient_cycles=${periodic_chart_sufficient_cycles}"
  echo "oracle_report_path=${artifacts_dir}/synthetic_hindsight_oracle.v1.report"
  echo "oracle_final_equity_numeraire=${oracle_equity}"
  echo "baseline_report_path=${baseline_report}"
  echo "numeraire_only_final_equity=1.000000000000"
  echo "equal_weight_final_equity=${equal_equity}"
  echo "best_fixed_asset_node=${best_fixed_node}"
  echo "best_fixed_asset_final_equity=${best_fixed_equity}"
  echo "one_step_momentum_final_equity=${momentum_equity}"
  echo "best_causal_baseline_name=${best_causal_name}"
  echo "best_causal_baseline_final_equity=${best_causal_equity}"
  echo "best_reference_baseline_name=${best_reference_name}"
  echo "best_reference_baseline_final_equity=${best_reference_equity}"
  echo "policy_report_path=${policy_report_path}"
  echo "policy_report_status=${policy_report_status}"
  echo "policy_final_equity_numeraire=${policy_final_equity}"
  echo "policy_initial_equity_numeraire=${policy_initial_equity}"
  echo "policy_final_equity_growth_multiple=${policy_growth_multiple}"
  echo "policy_vs_oracle_ratio=${policy_vs_oracle_ratio}"
  echo "policy_vs_best_causal_baseline_ratio=${policy_vs_best_causal_baseline_ratio}"
  echo "policy_vs_best_reference_baseline_ratio=${policy_vs_best_reference_baseline_ratio}"
  echo "capability_threshold_policy_vs_oracle=${threshold_vs_oracle}"
  echo "capability_threshold_policy_vs_causal_baseline=${threshold_vs_causal_baseline}"
  echo "protocol_probe_status=${protocol_probe_status}"
  echo "protocol_probe_reason=${protocol_probe_reason}"
  echo "config_status_exit_code=${config_status_code}"
  echo "runtime_wave_exit_code=${runtime_wave_status_code}"
  echo "runtime_wave_message=${runtime_wave_message}"
  echo "lattice_status_exit_code=${lattice_status_code}"
  echo "lattice_status_message=${lattice_status_message}"
  echo "marshal_policy_plan_exit_code=${marshal_policy_status_code}"
  echo "marshal_policy_plan_ok=${marshal_policy_ok}"
  echo "marshal_policy_plan_blocker=${marshal_policy_blocker}"
} >"${report_path}"

{
  echo "# synthetic_periodic_oracle_protocol_capability.v1"
  echo
  echo "Verdict: \`${verdict}\`"
  echo
  echo "Reason: ${verdict_reason}"
  echo
  echo "## Chart"
  echo
  echo "- Rows: ${row_count}"
  echo "- Accepted anchors: ${accepted_count}"
  echo "- Eval anchors: [${eval_begin}, ${eval_end})"
  echo "- Test anchors: [${test_begin}, ${test_end})"
  echo "- Minimum train/eval/test cycles: ${minimum_train_cycles} / ${minimum_eval_cycles} / ${minimum_test_cycles}"
  echo "- Sufficient cycles: ${periodic_chart_sufficient_cycles}"
  echo
  echo "## Reference Results"
  echo
  echo "- Hindsight oracle final equity: ${oracle_equity}"
  echo "- Equal-weight final equity: ${equal_equity}"
  echo "- Best fixed asset: ${best_fixed_node} at ${best_fixed_equity}"
  echo "- One-step momentum final equity: ${momentum_equity}"
  echo "- Best causal baseline: ${best_causal_name} at ${best_causal_equity}"
  echo
  echo "## Policy"
  echo
  echo "- Policy report: ${policy_report_status}"
  echo "- Policy final equity: ${policy_final_equity:-not available}"
  echo "- Policy initial equity: ${policy_initial_equity:-not available}"
  echo "- Policy growth multiple: ${policy_growth_multiple:-not available}"
  echo "- Policy/oracle ratio: ${policy_vs_oracle_ratio:-not available}"
  echo "- Policy/best-causal-baseline ratio: ${policy_vs_best_causal_baseline_ratio:-not available}"
  echo
  echo "## Protocol Probe"
  echo
  echo "- Status: ${protocol_probe_status}"
  echo "- Reason: ${protocol_probe_reason}"
  echo "- Runtime wave message: ${runtime_wave_message:-none}"
  echo "- Lattice status message: ${lattice_status_message:-none}"
  echo "- Marshal policy blocker: ${marshal_policy_blocker:-none}"
} >"${markdown_path}"

cat "${report_path}"
