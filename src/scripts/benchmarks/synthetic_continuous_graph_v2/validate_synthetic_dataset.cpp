// SPDX-License-Identifier: MIT

#include "piaabo/digest/sha256.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace digest = cuwacunu::piaabo::digest;

namespace {

constexpr std::string_view kManifestRelativePath =
    "artifacts/synthetic_quasiperiodic_chart_manifest.v2.report";
constexpr std::string_view kSeedLabel =
    "cuwacunu.synthetic_continuous_graph_v2.process.seed.2026-07-16.01";
constexpr std::string_view kSeedCommitment =
    "7d22bae27c203eeec9fb2147d63d1cbe55d9de056db7c048f74e4798849ee227";
constexpr std::uint64_t kInitialRandomState = 0x7d22bae27c203eeeULL;
constexpr std::int64_t kEpochMs = 1893456000000LL;
constexpr std::int64_t kMillisPerDay = 86400000LL;
constexpr std::size_t kMasterCount = 4137U;
constexpr std::size_t kAcceptedCount = 4096U;
constexpr std::size_t kVisibleEnd = 3264U;
constexpr double kPi = 0x1.921fb54442d18p+1;

struct interval_contract_t {
  std::string_view name;
  std::size_t width_days;
  std::size_t raw_count;
  std::size_t development_count;
  std::size_t history;
};

constexpr std::array<interval_contract_t, 3> kIntervalContracts{{
    {"1d", 1U, 4126U, 3294U, 30U},
    {"3d", 3U, 1376U, 1098U, 10U},
    {"1w", 7U, 591U, 471U, 4U},
}};

struct instrument_contract_t {
  std::string_view name;
  std::string_view base;
  double origin;
  double scale;
};

constexpr std::array<instrument_contract_t, 3> kInstrumentContracts{{
    {"SYN2ALPHASYN2USD", "SYN2ALPHA", 100.0, 0.0070},
    {"SYN2BETASYN2USD", "SYN2BETA", 112.0, 0.0065},
    {"SYN2GAMMASYN2USD", "SYN2GAMMA", 87.0, 0.0060},
}};

constexpr std::array<std::array<double, 3>, 3> kLoadings{{
    {{0.66, 0.25, -0.09}},
    {{-0.18, 0.63, 0.19}},
    {{0.24, -0.16, 0.60}},
}};

struct row_t {
  std::int64_t open_time{};
  double open{};
  double high{};
  double low{};
  double close{};
  double volume{};
  std::int64_t close_time{};
  double quote_volume{};
  std::uint64_t trades{};
  double taker_base{};
  double taker_quote{};
};

struct parsed_csv_t {
  std::vector<std::string> lines;
  std::vector<row_t> rows;
  std::string contents;
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error("synthetic v2 validator: " + message);
}

void check(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::string trim(std::string_view text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(" \t\r\n");
  return std::string(text.substr(begin, end - begin + 1U));
}

[[nodiscard]] std::vector<std::string> split(std::string_view text,
                                             char delimiter) {
  std::vector<std::string> fields;
  std::size_t begin = 0U;
  while (true) {
    const auto end = text.find(delimiter, begin);
    fields.emplace_back(text.substr(begin, end - begin));
    if (end == std::string_view::npos) {
      return fields;
    }
    begin = end + 1U;
  }
}

template <typename Integer>
[[nodiscard]] Integer parse_integer(std::string_view text,
                                    std::string_view label) {
  Integer result{};
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), result, 10);
  check(!text.empty() && parsed.ec == std::errc{} &&
            parsed.ptr == text.data() + text.size(),
        std::string(label) + " is not an integer");
  return result;
}

[[nodiscard]] double parse_double(std::string_view text,
                                  std::string_view label) {
  const std::string owned(text);
  char *end = nullptr;
  errno = 0;
  const double result = std::strtod(owned.c_str(), &end);
  check(!text.empty() && errno != ERANGE &&
            end == owned.c_str() + owned.size() && std::isfinite(result),
        std::string(label) + " is not a finite number");
  return result;
}

[[nodiscard]] std::string read_regular_file(const fs::path &path,
                                            std::uintmax_t maximum_size) {
  const auto status = fs::symlink_status(path);
  check(fs::is_regular_file(status) && !fs::is_symlink(status),
        "required input is not a non-symlink regular file: " + path.string());
  const auto size = fs::file_size(path);
  check(size <= maximum_size,
        "required input exceeds its size limit: " + path.string());
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  check(input.good(), "cannot open " + path.string());
  const auto end = input.tellg();
  check(end >= 0, "cannot determine file size: " + path.string());
  std::string contents(static_cast<std::size_t>(end), '\0');
  input.seekg(0, std::ios::beg);
  if (!contents.empty()) {
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  }
  check(input.good() || input.eof(), "failed reading " + path.string());
  check(static_cast<std::size_t>(input.gcount()) == contents.size(),
        "short read from " + path.string());
  return contents;
}

[[nodiscard]] std::map<std::string, std::string>
parse_manifest(const std::string &contents) {
  std::map<std::string, std::string> result;
  std::istringstream input(contents);
  std::string line;
  std::size_t line_number = 0U;
  while (std::getline(input, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    check(!trim(line).empty(),
          "manifest contains an empty line at " + std::to_string(line_number));
    const auto separator = line.find('=');
    check(separator != std::string::npos,
          "manifest line lacks '=' at " + std::to_string(line_number));
    const auto key = trim(std::string_view(line).substr(0U, separator));
    const auto value = trim(std::string_view(line).substr(separator + 1U));
    check(!key.empty() && !value.empty(),
          "manifest line is incomplete at " + std::to_string(line_number));
    check(result.emplace(key, value).second,
          "manifest contains duplicate key: " + key);
  }
  return result;
}

[[nodiscard]] const std::string &
required(const std::map<std::string, std::string> &manifest,
         const std::string &key) {
  const auto found = manifest.find(key);
  check(found != manifest.end(), "manifest lacks required key: " + key);
  return found->second;
}

void require_value(const std::map<std::string, std::string> &manifest,
                   const std::string &key, std::string_view expected) {
  check(required(manifest, key) == expected,
        "manifest value mismatch for " + key);
}

class phase_source_t {
public:
  explicit phase_source_t(std::uint64_t state) : state_(state) {}

  [[nodiscard]] double next_phase() {
    state_ += 0x9e3779b97f4a7c15ULL;
    std::uint64_t mixed = state_;
    mixed = (mixed ^ (mixed >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27U)) * 0x94d049bb133111ebULL;
    mixed ^= mixed >> 31U;
    const double unit = static_cast<double>(mixed >> 11U) * 0x1.0p-53;
    return 2.0 * kPi * unit;
  }

private:
  std::uint64_t state_;
};

[[nodiscard]] std::array<double, 4> reference_periods() {
  std::array<double, 4> values{};
  values[0] = 17.0 * std::sqrt(2.0);
  values[1] = 19.0 * std::sqrt(3.0);
  values[2] = 23.0 * std::sqrt(5.0);
  values[3] = 25.0 * std::sqrt(7.0);
  return values;
}

[[nodiscard]] std::array<double, 4> reference_phases() {
  phase_source_t source(kInitialRandomState);
  return {source.next_phase(), source.next_phase(), source.next_phase(),
          source.next_phase()};
}

[[nodiscard]] double
reference_return(std::size_t asset, std::size_t time,
                 const std::array<double, 4> &period_values,
                 const std::array<double, 4> &phase_values) {
  std::array<double, 4> signal{};
  for (std::size_t component = 0; component < signal.size(); ++component) {
    const double angle =
        2.0 * kPi * static_cast<double>(time) / period_values[component] +
        phase_values[component];
    signal[component] = std::sin(angle);
  }
  const double linear_signal = kLoadings[asset][0] * signal[0] +
                               kLoadings[asset][1] * signal[1] +
                               kLoadings[asset][2] * signal[2];
  return kInstrumentContracts[asset].scale * (1.0 + 0.12 * signal[3]) *
         linear_signal;
}

[[nodiscard]] std::vector<row_t>
reference_master(std::size_t asset, const std::array<double, 4> &period_values,
                 const std::array<double, 4> &phase_values) {
  std::vector<row_t> rows;
  rows.reserve(kMasterCount);
  double prior_close = kInstrumentContracts[asset].origin;
  double prior_return = 0.0;
  for (std::size_t time = 0; time < kMasterCount; ++time) {
    const double change =
        reference_return(asset, time, period_values, phase_values);
    const double closing = prior_close * std::exp(change);
    const double range =
        0.0012 + 0.20 * std::abs(change) + 0.05 * std::abs(prior_return);
    const double volume =
        1000.0 * (1.0 + 0.075 * static_cast<double>(asset)) *
        (1.0 + 20.0 * std::abs(change) + 8.0 * std::abs(prior_return));
    const double quote = volume * 0.5 * (prior_close + closing);
    const auto trade_count = static_cast<std::uint64_t>(std::llround(
        120.0 + 12000.0 * std::abs(change) + 4000.0 * std::abs(prior_return)));
    const double buyer_share =
        0.5 + 0.08 * std::tanh(change / kInstrumentContracts[asset].scale);
    const auto opening_time =
        kEpochMs + static_cast<std::int64_t>(time) * kMillisPerDay;
    rows.push_back({opening_time, prior_close,
                    std::max(prior_close, closing) * std::exp(range),
                    std::min(prior_close, closing) * std::exp(-range), closing,
                    volume, opening_time + kMillisPerDay - 1LL, quote,
                    trade_count, volume * buyer_share, quote * buyer_share});
    prior_close = closing;
    prior_return = change;
  }
  return rows;
}

[[nodiscard]] std::vector<row_t>
reference_aggregate(const std::vector<row_t> &master, std::size_t days,
                    std::size_t count) {
  check(days > 0U && count * days <= master.size(),
        "reference aggregation exceeds master data");
  std::vector<row_t> result;
  result.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const std::size_t first = index * days;
    const std::size_t limit = first + days;
    row_t row = master[first];
    row.high = -std::numeric_limits<double>::infinity();
    row.low = std::numeric_limits<double>::infinity();
    row.close = master[limit - 1U].close;
    row.close_time = master[limit - 1U].close_time;
    row.volume = 0.0;
    row.quote_volume = 0.0;
    row.trades = 0U;
    row.taker_base = 0.0;
    row.taker_quote = 0.0;
    for (std::size_t day = first; day < limit; ++day) {
      row.high = std::max(row.high, master[day].high);
      row.low = std::min(row.low, master[day].low);
      row.volume += master[day].volume;
      row.quote_volume += master[day].quote_volume;
      row.trades += master[day].trades;
      row.taker_base += master[day].taker_base;
      row.taker_quote += master[day].taker_quote;
    }
    result.push_back(row);
  }
  return result;
}

[[nodiscard]] std::string canonical_line(const row_t &row) {
  std::ostringstream output;
  output << row.open_time << ',' << std::fixed << std::setprecision(8)
         << row.open << ',' << row.high << ',' << row.low << ',' << row.close
         << ',' << row.volume << ',' << row.close_time << ','
         << row.quote_volume << ',' << row.trades << ',' << row.taker_base
         << ',' << row.taker_quote << ",0";
  return output.str();
}

[[nodiscard]] row_t parse_csv_row(const std::string &line, const fs::path &path,
                                  std::size_t line_number) {
  const auto fields = split(line, ',');
  check(fields.size() == 12U, path.string() + " row " +
                                  std::to_string(line_number) +
                                  " does not contain 12 fields");
  check(parse_integer<std::uint64_t>(fields[11], "ignored field") == 0U,
        path.string() + " has non-zero ignored field");
  return {parse_integer<std::int64_t>(fields[0], "open_time"),
          parse_double(fields[1], "open"),
          parse_double(fields[2], "high"),
          parse_double(fields[3], "low"),
          parse_double(fields[4], "close"),
          parse_double(fields[5], "volume"),
          parse_integer<std::int64_t>(fields[6], "close_time"),
          parse_double(fields[7], "quote_volume"),
          parse_integer<std::uint64_t>(fields[8], "trades"),
          parse_double(fields[9], "taker_base"),
          parse_double(fields[10], "taker_quote")};
}

[[nodiscard]] parsed_csv_t parse_csv(const fs::path &path,
                                     std::size_t expected_count,
                                     std::size_t interval_days) {
  parsed_csv_t result;
  result.contents = read_regular_file(path, 64U * 1024U * 1024U);
  check(!result.contents.empty() && result.contents.back() == '\n',
        path.string() + " must end with a newline");
  std::istringstream input(result.contents);
  std::string line;
  while (std::getline(input, line)) {
    check(!line.empty() && line.back() != '\r',
          path.string() + " contains an empty or CRLF row");
    result.lines.push_back(line);
    result.rows.push_back(parse_csv_row(line, path, result.lines.size()));
  }
  check(result.rows.size() == expected_count,
        path.string() + " row count mismatch");

  const std::int64_t step =
      static_cast<std::int64_t>(interval_days) * kMillisPerDay;
  for (std::size_t index = 0; index < result.rows.size(); ++index) {
    const auto &row = result.rows[index];
    check(row.open > 0.0 && row.close > 0.0 && row.low > 0.0 &&
              row.high >= std::max(row.open, row.close) &&
              row.low <= std::min(row.open, row.close) && row.volume > 0.0 &&
              row.quote_volume > 0.0 && row.trades > 0U &&
              row.taker_base >= 0.0 && row.taker_base <= row.volume &&
              row.taker_quote >= 0.0 && row.taker_quote <= row.quote_volume,
          path.string() + " violates OHLCV invariants at row " +
              std::to_string(index));
    check(row.close_time == row.open_time + step - 1LL,
          path.string() + " has invalid interval duration");
    if (index > 0U) {
      const auto &previous = result.rows[index - 1U];
      check(row.open_time == previous.open_time + step,
            path.string() + " timestamp step mismatch");
      check(std::abs(row.open - previous.close) <= 1e-8,
            path.string() + " is not OHLC-continuous");
    }
  }
  return result;
}

[[nodiscard]] std::string csv_relative_path(std::string_view tree,
                                            std::string_view instrument,
                                            std::string_view interval) {
  return "data/" + std::string(tree) + "/" + std::string(instrument) + "/" +
         std::string(interval) + "/" + std::string(instrument) + "-" +
         std::string(interval) + "-all-years.csv";
}

[[nodiscard]] std::string binding_key(std::string_view tree,
                                      std::string_view instrument,
                                      std::string_view interval) {
  return "csv." + std::string(tree) + "." + std::string(instrument) + "." +
         std::string(interval);
}

void validate_manifest_contract(
    const std::map<std::string, std::string> &manifest,
    const std::array<double, 4> &period_values,
    const std::array<double, 4> &phase_values) {
  require_value(manifest, "schema_id",
                "synthetic_quasiperiodic_chart_manifest.v2");
  require_value(manifest, "benchmark_id", "synthetic_continuous_graph_v2");
  require_value(manifest, "chart_family",
                "seeded_quasiperiodic_causal_ohlcv.v2");
  require_value(manifest, "seed_label", kSeedLabel);
  require_value(manifest, "seed_commitment_sha256", kSeedCommitment);
  check(digest::sha256_hex(kSeedLabel) == kSeedCommitment,
        "seed commitment does not hash the canonical seed label");
  require_value(manifest, "splitmix64_initial_state_hex", "7d22bae27c203eee");
  require_value(manifest, "splitmix64_mapping", "top_53_bits_times_2^-53");
  require_value(manifest, "stochastic_innovations", "false");
  require_value(manifest, "process_formula",
                "sigma_i*(1+0.12*s_3(t))*sum_k(M_i_k*s_k(t))");
  require_value(manifest, "oscillator_formula", "sin(2*pi*t/P_k+phase_k)");
  const std::string process_contract =
      "seed=sha256:cuwacunu.synthetic_continuous_graph_v2.process.seed."
      "2026-07-16.01;prng=splitmix64/top53;periods=17sqrt2,19sqrt3,"
      "23sqrt5,25sqrt7;return=sigma*(1+0.12*s3)*M*[s0,s1,s2];"
      "innovations=none;ohlcv=causal_daily_then_exact_aggregate;";
  require_value(manifest, "source_process_digest",
                digest::sha256_hex(process_contract));
  require_value(manifest, "start_time_ms", "1893456000000");
  require_value(manifest, "master_day_count", "4137");
  require_value(manifest, "quote_asset", "SYN2USD");
  require_value(manifest, "accepted_anchor_count", "4096");
  require_value(manifest, "anchor_zero_master_day_index", "29");
  require_value(manifest, "max_visible_anchor_exclusive", "3264");
  require_value(manifest, "max_visible_anchor_master_day_index", "3292");
  require_value(manifest, "development_prefix_includes_one_step_future",
                "true");
  require_value(manifest, "development_prefix_derivation",
                "exact_leading_rows_of_full_raw");
  require_value(manifest, "development_prefix_contains_final_holdout", "false");
  require_value(manifest, "full_raw_contains_final_holdout", "true");
  require_value(manifest, "generation_complete", "true");
  check(parse_double(required(manifest, "modulation_depth"),
                     "manifest modulation depth") == 0.12,
        "manifest modulation depth mismatch");

  const std::array<std::string_view, 4> expressions{"17*sqrt(2)", "19*sqrt(3)",
                                                    "23*sqrt(5)", "25*sqrt(7)"};
  for (std::size_t index = 0; index < period_values.size(); ++index) {
    require_value(manifest, "period." + std::to_string(index) + ".expression",
                  expressions[index]);
    check(parse_double(
              required(manifest, "period." + std::to_string(index) + ".days"),
              "manifest period") == period_values[index],
          "manifest period value mismatch");
    check(parse_double(
              required(manifest, "phase." + std::to_string(index) + ".radians"),
              "manifest phase") == phase_values[index],
          "manifest phase value mismatch");
  }
  for (std::size_t asset = 0; asset < kInstrumentContracts.size(); ++asset) {
    require_value(manifest, "instrument." + std::to_string(asset) + ".id",
                  kInstrumentContracts[asset].name);
    require_value(manifest,
                  "instrument." + std::to_string(asset) + ".base_asset",
                  kInstrumentContracts[asset].base);
    check(parse_double(
              required(manifest, "instrument." + std::to_string(asset) +
                                     ".initial_price"),
              "manifest initial price") == kInstrumentContracts[asset].origin,
          "manifest initial price mismatch");
    check(parse_double(
              required(manifest,
                       "instrument." + std::to_string(asset) + ".return_scale"),
              "manifest return scale") == kInstrumentContracts[asset].scale,
          "manifest return scale mismatch");
    for (std::size_t carrier = 0; carrier < 3U; ++carrier) {
      check(parse_double(required(manifest, "mixing." + std::to_string(asset) +
                                                "." + std::to_string(carrier)),
                         "manifest mixing coefficient") ==
                kLoadings[asset][carrier],
            "manifest mixing coefficient mismatch");
    }
  }
  for (const auto &interval : kIntervalContracts) {
    const std::string prefix = "interval." + std::string(interval.name);
    require_value(manifest, prefix + ".days",
                  std::to_string(interval.width_days));
    require_value(manifest, prefix + ".input_length",
                  std::to_string(interval.history));
    require_value(manifest, prefix + ".future_length", "1");
    require_value(manifest, prefix + ".raw_row_count",
                  std::to_string(interval.raw_count));
    require_value(manifest, prefix + ".development_prefix_row_count",
                  std::to_string(interval.development_count));
  }
  require_value(manifest, "train_anchor_begin", "0");
  require_value(manifest, "train_anchor_end_exclusive", "2496");
  require_value(manifest, "train_validation_embargo_begin", "2496");
  require_value(manifest, "train_validation_embargo_end_exclusive", "2560");
  require_value(manifest, "validation_anchor_begin", "2560");
  require_value(manifest, "validation_anchor_end_exclusive", "2816");
  require_value(manifest, "validation_certified_embargo_begin", "2816");
  require_value(manifest, "validation_certified_embargo_end_exclusive", "2880");
  require_value(manifest, "certified_eval_anchor_begin", "2880");
  require_value(manifest, "certified_eval_anchor_end_exclusive", "3264");
  require_value(manifest, "certified_final_embargo_begin", "3264");
  require_value(manifest, "certified_final_embargo_end_exclusive", "3328");
  require_value(manifest, "final_holdout_anchor_begin", "3328");
  require_value(manifest, "final_holdout_anchor_end_exclusive", "4096");
}

void validate_causal_coverage(const std::array<parsed_csv_t, 3> &raw,
                              const std::array<parsed_csv_t, 3> &development) {
  check(kAcceptedCount == 4126U - 30U, "accepted anchor arithmetic changed");
  const std::size_t final_raw_anchor_day = 29U + kAcceptedCount - 1U;
  const std::size_t last_visible_day = 29U + kVisibleEnd - 1U;
  check(final_raw_anchor_day == 4124U && last_visible_day == 3292U,
        "anchor-to-master-day mapping changed");

  const auto has_current_and_future = [](const parsed_csv_t &csv,
                                         std::int64_t anchor_close_time) {
    const auto found =
        std::upper_bound(csv.rows.begin(), csv.rows.end(), anchor_close_time,
                         [](std::int64_t key, const row_t &row) {
                           return key < row.close_time;
                         });
    if (found == csv.rows.begin()) {
      return false;
    }
    const auto current = std::prev(found);
    return std::next(current) != csv.rows.end();
  };

  const std::int64_t raw_last_key =
      kEpochMs +
      static_cast<std::int64_t>(final_raw_anchor_day + 1U) * kMillisPerDay -
      1LL;
  const std::int64_t visible_last_key =
      kEpochMs +
      static_cast<std::int64_t>(last_visible_day + 1U) * kMillisPerDay - 1LL;
  for (std::size_t interval = 0; interval < kIntervalContracts.size();
       ++interval) {
    check(has_current_and_future(raw[interval], raw_last_key),
          "full raw interval lacks final-anchor current/future coverage: " +
              std::string(kIntervalContracts[interval].name));
    check(
        has_current_and_future(development[interval], visible_last_key),
        "development interval lacks certified-eval current/future coverage: " +
            std::string(kIntervalContracts[interval].name));
  }
}

[[nodiscard]] fs::path parse_root(int argc, char **argv) {
  check(argc == 3 && std::string_view(argv[1]) == "--benchmark-root",
        "usage: validate_synthetic_dataset --benchmark-root PATH");
  const fs::path candidate(argv[2]);
  check(!candidate.empty() && fs::is_directory(candidate),
        "benchmark root is not a directory");
  check(!fs::is_symlink(fs::symlink_status(candidate)),
        "benchmark root must not be a symbolic link");
  return fs::canonical(candidate);
}

} // namespace

int main(int argc, char **argv) {
  try {
    const fs::path root = parse_root(argc, argv);
    const std::string manifest_contents =
        read_regular_file(root / kManifestRelativePath, 512U * 1024U);
    const auto manifest = parse_manifest(manifest_contents);
    const auto period_values = reference_periods();
    const auto phase_values = reference_phases();
    validate_manifest_contract(manifest, period_values, phase_values);

    std::map<std::string, std::string> actual_hashes;
    for (std::size_t asset = 0; asset < kInstrumentContracts.size(); ++asset) {
      const auto master = reference_master(asset, period_values, phase_values);
      std::array<parsed_csv_t, 3> raw_csvs;
      std::array<parsed_csv_t, 3> development_csvs;
      for (std::size_t interval_index = 0;
           interval_index < kIntervalContracts.size(); ++interval_index) {
        const auto &interval = kIntervalContracts[interval_index];
        const auto expected = reference_aggregate(master, interval.width_days,
                                                  interval.raw_count);
        const std::string raw_relative = csv_relative_path(
            "raw", kInstrumentContracts[asset].name, interval.name);
        const std::string development_relative =
            csv_relative_path("development_prefix",
                              kInstrumentContracts[asset].name, interval.name);
        raw_csvs[interval_index] = parse_csv(
            root / raw_relative, interval.raw_count, interval.width_days);
        development_csvs[interval_index] =
            parse_csv(root / development_relative, interval.development_count,
                      interval.width_days);

        for (std::size_t row = 0; row < expected.size(); ++row) {
          check(raw_csvs[interval_index].lines[row] ==
                    canonical_line(expected[row]),
                raw_relative +
                    " differs from the independently reconstructed "
                    "process at row " +
                    std::to_string(row));
        }
        for (std::size_t row = 0; row < interval.development_count; ++row) {
          check(development_csvs[interval_index].lines[row] ==
                    raw_csvs[interval_index].lines[row],
                development_relative +
                    " is not an exact leading-row view of full raw at row " +
                    std::to_string(row));
        }

        for (const std::string_view tree :
             {std::string_view("raw"),
              std::string_view("development_prefix")}) {
          const bool is_raw = tree == "raw";
          const std::string relative =
              is_raw ? raw_relative : development_relative;
          const auto &contents =
              is_raw ? raw_csvs[interval_index].contents
                     : development_csvs[interval_index].contents;
          const std::string key = binding_key(
              tree, kInstrumentContracts[asset].name, interval.name);
          require_value(manifest, key + ".relative_path", relative);
          const std::string actual_sha = digest::sha256_hex(contents);
          require_value(manifest, key + ".sha256", actual_sha);
          check(actual_hashes.emplace(relative, actual_sha).second,
                "duplicate dataset binding: " + relative);
        }
      }
      validate_causal_coverage(raw_csvs, development_csvs);
    }

    std::ostringstream dataset_binding;
    for (const std::string_view tree :
         {std::string_view("raw"), std::string_view("development_prefix")}) {
      for (const auto &instrument : kInstrumentContracts) {
        for (const auto &interval : kIntervalContracts) {
          const auto relative =
              csv_relative_path(tree, instrument.name, interval.name);
          const auto found = actual_hashes.find(relative);
          check(found != actual_hashes.end(),
                "missing dataset hash binding: " + relative);
          dataset_binding << relative << '=' << found->second << '\n';
        }
      }
    }
    require_value(manifest, "dataset_content_digest",
                  digest::sha256_hex(dataset_binding.str()));
    std::cout << "synthetic v2 dataset validation passed root=" << root << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
