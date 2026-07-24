// SPDX-License-Identifier: MIT

#include "piaabo/digest/sha256.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace digest = cuwacunu::piaabo::digest;

namespace {

constexpr std::string_view kSchemaId =
    "synthetic_quasiperiodic_chart_manifest.v2";
constexpr std::string_view kBenchmarkId = "synthetic_continuous_graph_v2";
constexpr std::string_view kSeedCommitment =
    "7d22bae27c203eeec9fb2147d63d1cbe55d9de056db7c048f74e4798849ee227";
constexpr std::uint64_t kSplitMixInitialState = 0x7d22bae27c203eeeULL;
constexpr std::int64_t kStartMs = 1893456000000LL;
constexpr std::int64_t kDayMs = 86400000LL;
constexpr std::size_t kMasterDays = 4137U;
constexpr std::size_t kAcceptedAnchors = 4096U;
constexpr std::size_t kMaximumVisibleAnchorExclusive = 3264U;

struct interval_spec_t {
  std::string_view id;
  std::size_t days;
  std::size_t raw_rows;
  std::size_t development_rows;
  std::size_t input_length;
};

constexpr std::array<interval_spec_t, 3> kIntervals{{
    {"1d", 1U, 4126U, 3294U, 30U},
    {"3d", 3U, 1376U, 1098U, 10U},
    {"1w", 7U, 591U, 471U, 4U},
}};

struct instrument_spec_t {
  std::string_view instrument;
  std::string_view base;
  double initial_price;
  double return_scale;
};

constexpr std::array<instrument_spec_t, 3> kInstruments{{
    {"SYN2ALPHASYN2USD", "SYN2ALPHA", 100.0, 0.0070},
    {"SYN2BETASYN2USD", "SYN2BETA", 112.0, 0.0065},
    {"SYN2GAMMASYN2USD", "SYN2GAMMA", 87.0, 0.0060},
}};

constexpr std::array<std::array<double, 3>, 3> kMixing{{
    {{0.66, 0.25, -0.09}},
    {{-0.18, 0.63, 0.19}},
    {{0.24, -0.16, 0.60}},
}};

constexpr double kModulationDepth = 0.12;
constexpr double kPi = 0x1.921fb54442d18p+1;

struct bar_t {
  std::int64_t open_time_ms{};
  double open{};
  double high{};
  double low{};
  double close{};
  double volume{};
  std::int64_t close_time_ms{};
  double quote_volume{};
  std::uint64_t trades{};
  double taker_buy_base{};
  double taker_buy_quote{};
};

class splitmix64_t {
public:
  explicit splitmix64_t(std::uint64_t state) : state_(state) {}

  [[nodiscard]] std::uint64_t next() {
    std::uint64_t value = (state_ += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
  }

  [[nodiscard]] double unit_interval() {
    return static_cast<double>(next() >> 11U) * 0x1.0p-53;
  }

private:
  std::uint64_t state_;
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error("synthetic v2 generator: " + message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::array<double, 4> periods() {
  return {17.0 * std::sqrt(2.0), 19.0 * std::sqrt(3.0), 23.0 * std::sqrt(5.0),
          25.0 * std::sqrt(7.0)};
}

[[nodiscard]] std::array<double, 4> phases() {
  splitmix64_t random(kSplitMixInitialState);
  std::array<double, 4> result{};
  for (double &phase : result) {
    phase = 2.0 * kPi * random.unit_interval();
  }
  return result;
}

[[nodiscard]] double
daily_log_return(std::size_t instrument_index, std::size_t day,
                 const std::array<double, 4> &phase_values,
                 const std::array<double, 4> &period_values) {
  std::array<double, 4> oscillator{};
  for (std::size_t index = 0; index < oscillator.size(); ++index) {
    oscillator[index] =
        std::sin(2.0 * kPi * static_cast<double>(day) / period_values[index] +
                 phase_values[index]);
  }

  double carrier = 0.0;
  for (std::size_t index = 0; index < 3U; ++index) {
    carrier += kMixing[instrument_index][index] * oscillator[index];
  }
  return kInstruments[instrument_index].return_scale *
         (1.0 + kModulationDepth * oscillator[3]) * carrier;
}

[[nodiscard]] std::vector<bar_t>
make_master(std::size_t instrument_index,
            const std::array<double, 4> &phase_values,
            const std::array<double, 4> &period_values) {
  const auto &instrument = kInstruments[instrument_index];
  std::vector<bar_t> result;
  result.reserve(kMasterDays);

  double previous_close = instrument.initial_price;
  double previous_return = 0.0;
  for (std::size_t day = 0; day < kMasterDays; ++day) {
    const double log_return =
        daily_log_return(instrument_index, day, phase_values, period_values);
    const double close = previous_close * std::exp(log_return);
    const double spread =
        0.0012 + 0.20 * std::abs(log_return) + 0.05 * std::abs(previous_return);
    const double activity =
        1.0 + 20.0 * std::abs(log_return) + 8.0 * std::abs(previous_return);
    const double asset_activity_scale =
        1.0 + 0.075 * static_cast<double>(instrument_index);
    const double volume = 1000.0 * asset_activity_scale * activity;
    const double quote_volume = volume * 0.5 * (previous_close + close);
    const auto trades = static_cast<std::uint64_t>(
        std::llround(120.0 + 12000.0 * std::abs(log_return) +
                     4000.0 * std::abs(previous_return)));
    const double taker_fraction =
        0.5 + 0.08 * std::tanh(log_return / instrument.return_scale);

    bar_t bar;
    bar.open_time_ms = kStartMs + static_cast<std::int64_t>(day) * kDayMs;
    bar.open = previous_close;
    bar.high = std::max(previous_close, close) * std::exp(spread);
    bar.low = std::min(previous_close, close) * std::exp(-spread);
    bar.close = close;
    bar.volume = volume;
    bar.close_time_ms = bar.open_time_ms + kDayMs - 1LL;
    bar.quote_volume = quote_volume;
    bar.trades = trades;
    bar.taker_buy_base = volume * taker_fraction;
    bar.taker_buy_quote = quote_volume * taker_fraction;

    require(std::isfinite(bar.low) && bar.low > 0.0 &&
                std::isfinite(bar.high) && bar.high >= bar.low &&
                std::isfinite(bar.volume) && bar.volume > 0.0 &&
                bar.trades > 0U,
            "generated a non-finite or invalid daily bar");
    result.push_back(bar);
    previous_close = close;
    previous_return = log_return;
  }
  return result;
}

[[nodiscard]] std::vector<bar_t> aggregate(const std::vector<bar_t> &master,
                                           std::size_t days,
                                           std::size_t row_count) {
  require(days > 0U && row_count <= master.size() / days,
          "aggregate request exceeds the master path");
  std::vector<bar_t> result;
  result.reserve(row_count);
  for (std::size_t row = 0; row < row_count; ++row) {
    const std::size_t begin = row * days;
    const std::size_t end = begin + days;
    bar_t aggregate_bar = master[begin];
    aggregate_bar.close = master[end - 1U].close;
    aggregate_bar.close_time_ms = master[end - 1U].close_time_ms;
    aggregate_bar.high = master[begin].high;
    aggregate_bar.low = master[begin].low;
    aggregate_bar.volume = 0.0;
    aggregate_bar.quote_volume = 0.0;
    aggregate_bar.trades = 0U;
    aggregate_bar.taker_buy_base = 0.0;
    aggregate_bar.taker_buy_quote = 0.0;
    for (std::size_t day = begin; day < end; ++day) {
      aggregate_bar.high = std::max(aggregate_bar.high, master[day].high);
      aggregate_bar.low = std::min(aggregate_bar.low, master[day].low);
      aggregate_bar.volume += master[day].volume;
      aggregate_bar.quote_volume += master[day].quote_volume;
      aggregate_bar.trades += master[day].trades;
      aggregate_bar.taker_buy_base += master[day].taker_buy_base;
      aggregate_bar.taker_buy_quote += master[day].taker_buy_quote;
    }
    result.push_back(aggregate_bar);
  }
  return result;
}

[[nodiscard]] std::string format_bar(const bar_t &bar) {
  std::ostringstream output;
  output << bar.open_time_ms << ',' << std::fixed << std::setprecision(8)
         << bar.open << ',' << bar.high << ',' << bar.low << ',' << bar.close
         << ',' << bar.volume << ',' << bar.close_time_ms << ','
         << bar.quote_volume << ',' << bar.trades << ',' << bar.taker_buy_base
         << ',' << bar.taker_buy_quote << ",0\n";
  return output.str();
}

void write_bars(const fs::path &path, const std::vector<bar_t> &bars,
                std::size_t count) {
  require(count <= bars.size(), "requested output prefix exceeds row count");
  require(!fs::exists(path), "refusing to overwrite " + path.string());
  fs::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  require(output.good(), "cannot open " + path.string());
  for (std::size_t row = 0; row < count; ++row) {
    output << format_bar(bars[row]);
  }
  output.flush();
  require(output.good(), "failed while writing " + path.string());
}

[[nodiscard]] std::string read_file(const fs::path &path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  require(input.good(), "cannot read " + path.string());
  const auto end = input.tellg();
  require(end >= 0, "cannot determine size of " + path.string());
  std::string contents(static_cast<std::size_t>(end), '\0');
  input.seekg(0, std::ios::beg);
  if (!contents.empty()) {
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  }
  require(input.good() || input.eof(), "failed reading " + path.string());
  require(static_cast<std::size_t>(input.gcount()) == contents.size(),
          "short read from " + path.string());
  return contents;
}

[[nodiscard]] std::string relative_csv_path(std::string_view tree,
                                            std::string_view instrument,
                                            std::string_view interval) {
  return "data/" + std::string(tree) + "/" + std::string(instrument) + "/" +
         std::string(interval) + "/" + std::string(instrument) + "-" +
         std::string(interval) + "-all-years.csv";
}

[[nodiscard]] std::string precise(double value) {
  std::ostringstream output;
  output << std::setprecision(std::numeric_limits<double>::max_digits10)
         << value;
  return output.str();
}

void write_manifest(const fs::path &root,
                    const std::array<double, 4> &period_values,
                    const std::array<double, 4> &phase_values) {
  const fs::path manifest =
      root / "artifacts/synthetic_quasiperiodic_chart_manifest.v2.report";
  require(!fs::exists(manifest), "refusing to overwrite " + manifest.string());
  fs::create_directories(manifest.parent_path());

  struct binding_t {
    std::string tree;
    std::string instrument;
    std::string interval;
    std::string relative_path;
    std::string sha256;
  };
  std::vector<binding_t> bindings;
  for (const std::string_view tree :
       {std::string_view("raw"), std::string_view("development_prefix")}) {
    for (const auto &instrument : kInstruments) {
      for (const auto &interval : kIntervals) {
        const auto relative =
            relative_csv_path(tree, instrument.instrument, interval.id);
        bindings.push_back({std::string(tree),
                            std::string(instrument.instrument),
                            std::string(interval.id), relative,
                            digest::sha256_hex(read_file(root / relative))});
      }
    }
  }

  std::ostringstream dataset_binding;
  for (const auto &binding : bindings) {
    dataset_binding << binding.relative_path << '=' << binding.sha256 << '\n';
  }

  const std::string process_contract =
      "seed=sha256:cuwacunu.synthetic_continuous_graph_v2.process.seed."
      "2026-07-16.01;prng=splitmix64/top53;periods=17sqrt2,19sqrt3,"
      "23sqrt5,25sqrt7;return=sigma*(1+0.12*s3)*M*[s0,s1,s2];"
      "innovations=none;ohlcv=causal_daily_then_exact_aggregate;";

  std::ofstream output(manifest, std::ios::binary);
  require(output.good(), "cannot open manifest for writing");
  output << "schema_id=" << kSchemaId << '\n'
         << "benchmark_id=" << kBenchmarkId << '\n'
         << "chart_family=seeded_quasiperiodic_causal_ohlcv.v2\n"
         << "seed_label=cuwacunu.synthetic_continuous_graph_v2.process.seed."
            "2026-07-16.01\n"
         << "seed_commitment_sha256=" << kSeedCommitment << '\n'
         << "splitmix64_initial_state_hex=7d22bae27c203eee\n"
         << "splitmix64_mapping=top_53_bits_times_2^-53\n"
         << "stochastic_innovations=false\n"
         << "process_formula=sigma_i*(1+0.12*s_3(t))*sum_k(M_i_k*s_k(t))\n"
         << "oscillator_formula=sin(2*pi*t/P_k+phase_k)\n"
         << "source_process_digest=" << digest::sha256_hex(process_contract)
         << '\n'
         << "start_time_ms=" << kStartMs << '\n'
         << "master_day_count=" << kMasterDays << '\n'
         << "quote_asset=SYN2USD\n";
  const std::array<std::string_view, 4> period_expressions{
      "17*sqrt(2)", "19*sqrt(3)", "23*sqrt(5)", "25*sqrt(7)"};
  for (std::size_t index = 0; index < period_values.size(); ++index) {
    output << "period." << index << ".expression=" << period_expressions[index]
           << '\n'
           << "period." << index << ".days=" << precise(period_values[index])
           << '\n'
           << "phase." << index << ".radians=" << precise(phase_values[index])
           << '\n';
  }
  output << "modulation_depth=" << precise(kModulationDepth) << '\n';
  for (std::size_t instrument = 0; instrument < kInstruments.size();
       ++instrument) {
    const auto &spec = kInstruments[instrument];
    output << "instrument." << instrument << ".id=" << spec.instrument << '\n'
           << "instrument." << instrument << ".base_asset=" << spec.base << '\n'
           << "instrument." << instrument
           << ".initial_price=" << precise(spec.initial_price) << '\n'
           << "instrument." << instrument
           << ".return_scale=" << precise(spec.return_scale) << '\n';
    for (std::size_t carrier = 0; carrier < 3U; ++carrier) {
      output << "mixing." << instrument << '.' << carrier << '='
             << precise(kMixing[instrument][carrier]) << '\n';
    }
  }
  for (const auto &interval : kIntervals) {
    output << "interval." << interval.id << ".days=" << interval.days << '\n'
           << "interval." << interval.id
           << ".input_length=" << interval.input_length << '\n'
           << "interval." << interval.id << ".future_length=1\n"
           << "interval." << interval.id
           << ".raw_row_count=" << interval.raw_rows << '\n'
           << "interval." << interval.id
           << ".development_prefix_row_count=" << interval.development_rows
           << '\n';
  }
  output << "accepted_anchor_count=" << kAcceptedAnchors << '\n'
         << "anchor_zero_master_day_index=29\n"
         << "max_visible_anchor_exclusive=" << kMaximumVisibleAnchorExclusive
         << '\n'
         << "max_visible_anchor_master_day_index=3292\n"
         << "development_prefix_includes_one_step_future=true\n"
         << "development_prefix_derivation=exact_leading_rows_of_full_raw\n"
         << "train_anchor_begin=0\n"
         << "train_anchor_end_exclusive=2496\n"
         << "train_validation_embargo_begin=2496\n"
         << "train_validation_embargo_end_exclusive=2560\n"
         << "validation_anchor_begin=2560\n"
         << "validation_anchor_end_exclusive=2816\n"
         << "validation_certified_embargo_begin=2816\n"
         << "validation_certified_embargo_end_exclusive=2880\n"
         << "certified_eval_anchor_begin=2880\n"
         << "certified_eval_anchor_end_exclusive=3264\n"
         << "certified_final_embargo_begin=3264\n"
         << "certified_final_embargo_end_exclusive=3328\n"
         << "final_holdout_anchor_begin=3328\n"
         << "final_holdout_anchor_end_exclusive=4096\n"
         << "full_raw_contains_final_holdout=true\n"
         << "development_prefix_contains_final_holdout=false\n";
  for (const auto &binding : bindings) {
    const std::string key_prefix = "csv." + binding.tree + "." +
                                   binding.instrument + "." + binding.interval;
    output << key_prefix << ".relative_path=" << binding.relative_path << '\n'
           << key_prefix << ".sha256=" << binding.sha256 << '\n';
  }
  output << "dataset_content_digest="
         << digest::sha256_hex(dataset_binding.str()) << '\n'
         << "generation_complete=true\n";
  output.flush();
  require(output.good(), "failed while writing the manifest");
}

[[nodiscard]] fs::path parse_output_root(int argc, char **argv) {
  if (argc != 3 || std::string_view(argv[1]) != "--output-root") {
    fail("usage: generate_synthetic_klines --output-root PATH");
  }
  const fs::path root(argv[2]);
  require(!root.empty(), "output root is empty");
  if (fs::exists(root)) {
    require(fs::is_directory(root), "output root is not a directory");
    require(fs::is_empty(root), "output root is not empty");
  } else {
    fs::create_directories(root);
  }
  return fs::canonical(root);
}

} // namespace

int main(int argc, char **argv) {
  try {
    const fs::path root = parse_output_root(argc, argv);
    const auto period_values = periods();
    const auto phase_values = phases();

    for (std::size_t instrument_index = 0;
         instrument_index < kInstruments.size(); ++instrument_index) {
      const auto master =
          make_master(instrument_index, phase_values, period_values);
      for (const auto &interval : kIntervals) {
        const auto raw_bars =
            aggregate(master, interval.days, interval.raw_rows);
        const auto raw_path =
            root / relative_csv_path("raw",
                                     kInstruments[instrument_index].instrument,
                                     interval.id);
        const auto development_path =
            root / relative_csv_path("development_prefix",
                                     kInstruments[instrument_index].instrument,
                                     interval.id);
        write_bars(raw_path, raw_bars, raw_bars.size());
        write_bars(development_path, raw_bars, interval.development_rows);
      }
    }
    write_manifest(root, period_values, phase_values);
    std::cout << "generated synthetic v2 dataset under " << root << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
