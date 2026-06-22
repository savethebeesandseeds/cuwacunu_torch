#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
BENCHMARK_ROOT="${REPO_ROOT}/src/config/benchmarks/synthetic_continuous_graph_v1"
DATA_ROOT="$BENCHMARK_ROOT/data/raw"
GENERATOR_PATH="$SCRIPT_DIR/generate_synthetic_klines.sh"

runtime_job_dir=""
eval_anchor_begin=""
eval_anchor_end=""
input_length=30
max_node_weight=0.95
output_path=""

while [[ $# -gt 0 ]]; do
  case "$1" in
  --runtime-job-dir)
    runtime_job_dir="${2:-}"
    shift 2
    ;;
  --eval-anchor-begin)
    eval_anchor_begin="${2:-}"
    shift 2
    ;;
  --eval-anchor-end)
    eval_anchor_end="${2:-}"
    shift 2
    ;;
  --input-length)
    input_length="${2:-}"
    shift 2
    ;;
  --max-node-weight)
    max_node_weight="${2:-}"
    shift 2
    ;;
  --output)
    output_path="${2:-}"
    shift 2
    ;;
  *)
    echo "unknown argument: $1" >&2
    exit 2
    ;;
  esac
done

if [[ -z "$runtime_job_dir" ]]; then
  echo "--runtime-job-dir is required" >&2
  exit 2
fi
if [[ ! -d "$runtime_job_dir" ]]; then
  echo "runtime job dir does not exist: $runtime_job_dir" >&2
  exit 2
fi

exposure_fact="$runtime_job_dir/lattice.exposure.fact"
if [[ -z "$eval_anchor_begin" ]]; then
  eval_anchor_begin="$(awk -F= '$1=="anchor_begin"{print $2}' "$exposure_fact")"
fi
if [[ -z "$eval_anchor_end" ]]; then
  eval_anchor_end="$(awk -F= '$1=="anchor_end"{print $2}' "$exposure_fact")"
fi
if [[ -z "$eval_anchor_begin" || -z "$eval_anchor_end" ]]; then
  echo "could not resolve eval anchor range" >&2
  exit 2
fi

artifacts_dir="$runtime_job_dir/artifacts"
mkdir -p "$artifacts_dir"
if [[ -z "$output_path" ]]; then
  output_path="$artifacts_dir/synthetic_hindsight_oracle.v1.report"
fi
action_trace_path="$artifacts_dir/synthetic_hindsight_oracle.v1.actions.csv"

alpha_csv="$DATA_ROOT/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_csv="$DATA_ROOT/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_csv="$DATA_ROOT/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
for csv in "$alpha_csv" "$beta_csv" "$gamma_csv"; do
  if [[ ! -f "$csv" ]]; then
    echo "missing synthetic CSV: $csv" >&2
    exit 2
  fi
done

source_function_digest="$(
  sha256sum "$GENERATOR_PATH" "$alpha_csv" "$beta_csv" "$gamma_csv" |
    sha256sum | awk '{print $1}'
)"

tmp_report="$(mktemp)"
tmp_trace="$(mktemp)"
awk -F, \
  -v alpha_file="$alpha_csv" \
  -v beta_file="$beta_csv" \
  -v gamma_file="$gamma_csv" \
  -v begin="$eval_anchor_begin" \
  -v end="$eval_anchor_end" \
  -v input_length="$input_length" \
  -v max_weight="$max_node_weight" \
  -v source_digest="$source_function_digest" \
  -v trace_path="$tmp_trace" \
  -v report_path="$tmp_report" '
function abs(x) { return x < 0.0 ? -x : x; }
BEGIN {
  nodes[1] = "SYNALPHA";
  nodes[2] = "SYNBETA";
  nodes[3] = "SYNGAMMA";
  eps = 1e-12;
  numeraire_weight = 1.0 - max_weight;
  raw_begin = begin + input_length - 1;
  raw_end = end + input_length - 1;
  equity = 1.0;
  peak = 1.0;
  max_drawdown = 0.0;
  turnover_l1 = 0.0;
  prev_numeraire = 1.0;
  for (i = 1; i <= 3; ++i) {
    prev_weight[nodes[i]] = 0.0;
  }
  print "anchor,raw_row,selected_node,log_growth,equity" > trace_path;
}
FILENAME == alpha_file {
  close_px["SYNALPHA", FNR - 1] = $5 + 0.0;
  close_time[FNR - 1] = $7;
}
FILENAME == beta_file { close_px["SYNBETA", FNR - 1] = $5 + 0.0; }
FILENAME == gamma_file { close_px["SYNGAMMA", FNR - 1] = $5 + 0.0; }
END {
  step_count = 0;
  for (raw = raw_begin; raw < raw_end; ++raw) {
    selected = "SYNUSD";
    best_return = 0.0;
    for (i = 1; i <= 3; ++i) {
      node = nodes[i];
      current_px = close_px[node, raw];
      next_px = close_px[node, raw + 1];
      if (current_px <= 0.0 || next_px <= 0.0) {
        printf("missing or nonpositive close for %s raw row %d\n", node, raw) > "/dev/stderr";
        exit 3;
      }
      ret = log(next_px / current_px);
      if (ret > best_return + eps ||
          (ret >= best_return - eps && ret > 0.0 &&
           (selected == "SYNUSD" || node < selected))) {
        best_return = ret;
        selected = node;
      }
    }

    if (selected == "SYNUSD") {
      growth = 0.0;
      new_numeraire = 1.0;
    } else {
      growth = log(numeraire_weight + max_weight * exp(best_return));
      new_numeraire = numeraire_weight;
    }

    turnover_l1 += abs(new_numeraire - prev_numeraire);
    for (i = 1; i <= 3; ++i) {
      node = nodes[i];
      new_weight = (node == selected) ? max_weight : 0.0;
      turnover_l1 += abs(new_weight - prev_weight[node]);
      prev_weight[node] = new_weight;
    }
    prev_numeraire = new_numeraire;

    total_log_growth += growth;
    equity *= exp(growth);
    if (equity > peak) {
      peak = equity;
    }
    drawdown = peak > 0.0 ? (peak - equity) / peak : 0.0;
    if (drawdown > max_drawdown) {
      max_drawdown = drawdown;
    }
    step_count += 1;
    printf("%d,%d,%s,%.12f,%.12f\n",
           begin + step_count - 1, raw, selected, growth, equity) >> trace_path;
  }

  print "oracle_schema_id=synthetic_hindsight_oracle.v1" > report_path;
  print "benchmark_id=synthetic_continuous_graph_v1" >> report_path;
  print "protocol_id=cwu_02v" >> report_path;
  print "eval_anchor_begin=" begin >> report_path;
  print "eval_anchor_end_exclusive=" end >> report_path;
  print "input_length=" input_length >> report_path;
  print "raw_close_row_begin=" raw_begin >> report_path;
  print "raw_close_row_end_exclusive=" raw_end >> report_path;
  print "source_function_digest=" source_digest >> report_path;
  print "oracle_algorithm_id=one_step_hindsight_long_only_simplex_no_cost.v1" >> report_path;
  print "oracle_causal=false" >> report_path;
  print "oracle_training_input_allowed=false" >> report_path;
  print "oracle_readiness_authority=false" >> report_path;
  print "oracle_policy_acceptance_authority=false" >> report_path;
  print "oracle_deployment_authority=false" >> report_path;
  print "oracle_live_authority=false" >> report_path;
  printf("max_node_weight=%.12f\n", max_weight) >> report_path;
  printf("oracle_total_log_growth=%.12f\n", total_log_growth) >> report_path;
  printf("oracle_final_equity_numeraire=%.12f\n", equity) >> report_path;
  printf("oracle_max_drawdown=%.12f\n", max_drawdown) >> report_path;
  printf("oracle_turnover_l1=%.12f\n", turnover_l1) >> report_path;
  print "oracle_step_count=" step_count >> report_path;
}
' "$alpha_csv" "$beta_csv" "$gamma_csv"

action_trace_digest="$(sha256sum "$tmp_trace" | awk '{print $1}')"
{
  cat "$tmp_report"
  echo "oracle_action_trace_path=$action_trace_path"
  echo "oracle_action_trace_digest=$action_trace_digest"
} > "$output_path"
mv "$tmp_trace" "$action_trace_path"
rm -f "$tmp_report"

cat "$output_path"
