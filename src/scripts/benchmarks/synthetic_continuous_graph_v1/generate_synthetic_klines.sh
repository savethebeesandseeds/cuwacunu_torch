#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
out_root="${benchmark_root}/data/raw"
artifacts_dir="${benchmark_root}/artifacts"
manifest_path="${artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"

row_count="${SYNTHETIC_ROW_COUNT:-1200}"
input_length="${SYNTHETIC_INPUT_LENGTH:-30}"
max_fundamental_period=12

generate_instrument_interval() {
  local instrument="$1"
  local model="$2"
  local phase="$3"
  local interval="$4"
  local interval_days="$5"
  local out_dir="${out_root}/${instrument}/${interval}"
  local out_file="${out_dir}/${instrument}-${interval}-all-years.csv"

  mkdir -p "${out_dir}"

  awk -v model="${model}" -v phase="${phase}" \
    -v interval_days="${interval_days}" -v row_count="${row_count}" '
    function price(t, pi) {
      pi = atan2(0, -1)
      if (model == "alpha") {
        return 100.0 * exp(0.060 * sin(2.0 * pi * t / 12.0 + phase) + 0.015 * cos(2.0 * pi * t / 6.0 + 0.50))
      }
      if (model == "beta") {
        return 100.0 * exp(0.055 * cos(2.0 * pi * t / 10.0 + phase) + 0.014 * sin(2.0 * pi * t / 5.0 + 0.25))
      }
      return 100.0 * exp(0.050 * sin(2.0 * pi * t / 8.0 + phase) + 0.016 * cos(2.0 * pi * t / 4.0 + 0.85))
    }
    BEGIN {
      start_ms = 1577836800000
      day_ms = 86400000
      step_ms = interval_days * day_ms
      pi = atan2(0, -1)
      for (t = 0; t < row_count; ++t) {
        open_time = start_ms + t * step_ms
        close_time = open_time + step_ms - 1
        close_price = price(t)
        open_price = (t == 0) ? close_price : price(t - 1)
        high_price = (open_price > close_price ? open_price : close_price) * 1.002
        low_price = (open_price < close_price ? open_price : close_price) * 0.998
        volume = 1000.0 + 50.0 * (1.0 + sin(2.0 * pi * t / 30.0 + phase))
        quote_volume = volume * close_price
        trade_count = 100 + (t % 50)
        taker_buy_base = 0.5 * volume
        taker_buy_quote = taker_buy_base * close_price
        printf("%.0f,%.8f,%.8f,%.8f,%.8f,%.8f,%.0f,%.8f,%d,%.8f,%.8f,0\n",
               open_time, open_price, high_price, low_price, close_price,
               volume, close_time, quote_volume, trade_count,
               taker_buy_base, taker_buy_quote)
      }
    }
  ' > "${out_file}"
}

generate_instrument() {
  local instrument="$1"
  local model="$2"
  local phase="$3"
  generate_instrument_interval "${instrument}" "${model}" "${phase}" "1w" "7"
  generate_instrument_interval "${instrument}" "${model}" "${phase}" "3d" "3"
  generate_instrument_interval "${instrument}" "${model}" "${phase}" "1d" "1"
}

mkdir -p "${artifacts_dir}"

generate_instrument "SYNALPHASYNUSD" "alpha" "0.0"
generate_instrument "SYNBETASYNUSD" "beta" "1.0"
generate_instrument "SYNGAMMASYNUSD" "gamma" "2.0"

accepted_count=$((row_count - input_length))
if ((accepted_count <= 0)); then
  echo "synthetic accepted anchor count is not positive" >&2
  exit 2
fi

train_begin=0
eval_begin=$((accepted_count * 65 / 100))
train_end=$((accepted_count * 73 / 117))
eval_end=$((accepted_count * 93 / 100))
test_begin="${eval_end}"
test_end="${accepted_count}"
embargo_gap_anchors=$((eval_begin - train_end))

minimum_train_cycles="$(
  awk -v n="$((train_end - train_begin))" -v p="${max_fundamental_period}" \
    'BEGIN { printf("%.6f", n / p) }'
)"
minimum_eval_cycles="$(
  awk -v n="$((eval_end - eval_begin))" -v p="${max_fundamental_period}" \
    'BEGIN { printf("%.6f", n / p) }'
)"
minimum_test_cycles="$(
  awk -v n="$((test_end - test_begin))" -v p="${max_fundamental_period}" \
    'BEGIN { printf("%.6f", n / p) }'
)"

periodic_chart_sufficient_cycles="false"
awk -v train="${minimum_train_cycles}" \
  -v eval="${minimum_eval_cycles}" \
  -v test="${minimum_test_cycles}" \
  'BEGIN { exit !((train >= 20.0) && (eval >= 10.0) && (test >= 3.0)) }' &&
  periodic_chart_sufficient_cycles="true"

source_function_digest="$(
  sha256sum "${BASH_SOURCE[0]}" \
    "${out_root}/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv" \
    "${out_root}/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv" \
    "${out_root}/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv" |
    sha256sum | awk '{print $1}'
)"

{
  echo "schema_id=synthetic_periodic_chart_manifest.v1"
  echo "benchmark_id=synthetic_continuous_graph_v1"
  echo "chart_family=cycle_rich_zero_drift_log_price.v1"
  echo "row_count=${row_count}"
  echo "input_length=${input_length}"
  echo "accepted_anchor_count=${accepted_count}"
  echo "train_anchor_begin=${train_begin}"
  echo "train_anchor_end_exclusive=${train_end}"
  echo "embargo_gap_anchors=${embargo_gap_anchors}"
  echo "eval_anchor_begin=${eval_begin}"
  echo "eval_anchor_end_exclusive=${eval_end}"
  echo "test_anchor_begin=${test_begin}"
  echo "test_anchor_end_exclusive=${test_end}"
  echo "source_split_policy=train_to_73_117_eval_65_93_test_93_100_embargo_gap"
  echo "instrument.SYNALPHASYNUSD.fundamental_period_anchors=12"
  echo "instrument.SYNBETASYNUSD.fundamental_period_anchors=10"
  echo "instrument.SYNGAMMASYNUSD.fundamental_period_anchors=8"
  echo "max_fundamental_period_anchors=${max_fundamental_period}"
  echo "minimum_train_cycles=${minimum_train_cycles}"
  echo "minimum_eval_cycles=${minimum_eval_cycles}"
  echo "minimum_test_cycles=${minimum_test_cycles}"
  echo "periodic_chart_sufficient_cycles=${periodic_chart_sufficient_cycles}"
  echo "chart_drift_model=none"
  echo "source_function_digest=${source_function_digest}"
} >"${manifest_path}"

printf 'generated synthetic kline CSVs under %s\n' "${out_root}"
printf 'wrote synthetic chart manifest %s\n' "${manifest_path}"
