#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
out_root="${benchmark_root}/data/raw"

generate_instrument_interval() {
  local instrument="$1"
  local model="$2"
  local phase="$3"
  local interval="$4"
  local interval_days="$5"
  local out_dir="${out_root}/${instrument}/${interval}"
  local out_file="${out_dir}/${instrument}-${interval}-all-years.csv"

  mkdir -p "${out_dir}"

  awk -v model="${model}" -v phase="${phase}" -v interval_days="${interval_days}" '
    function price(t, pi) {
      pi = atan2(0, -1)
      if (model == "alpha") {
        return 100.0 * exp(0.00055 * t + 0.050 * sin(2.0 * pi * t / 48.0))
      }
      if (model == "beta") {
        return 100.0 * exp(0.00035 * t + 0.060 * cos(2.0 * pi * t / 64.0 + 0.70))
      }
      return 100.0 * exp(0.00045 * t + 0.035 * sin(2.0 * pi * t / 32.0 + 0.20) + 0.025 * cos(2.0 * pi * t / 96.0))
    }
    BEGIN {
      start_ms = 1577836800000
      day_ms = 86400000
      step_ms = interval_days * day_ms
      pi = atan2(0, -1)
      for (t = 0; t < 1200; ++t) {
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

generate_instrument "SYNALPHASYNUSD" "alpha" "0.0"
generate_instrument "SYNBETASYNUSD" "beta" "1.0"
generate_instrument "SYNGAMMASYNUSD" "gamma" "2.0"

printf 'generated synthetic kline CSVs under %s\n' "${out_root}"
