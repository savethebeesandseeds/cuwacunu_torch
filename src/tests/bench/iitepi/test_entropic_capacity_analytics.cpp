#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "piaabo/torch_compat/entropic_capacity_comparison.h"
#include "piaabo/torch_compat/network_analytics.h"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[test_entropic_capacity_analytics] FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

struct tiny_model_t final : torch::nn::Module {
  tiny_model_t() {
    register_parameter(
        "w_diag",
        torch::tensor({{3.0, 0.0}, {0.0, 1.0}}, torch::kFloat64),
        true);
    register_parameter(
        "w_lowrank",
        torch::tensor({{1.0, 0.0}, {0.0, 0.0}}, torch::kFloat64),
        true);
  }
};

bool has_lhs_key(const std::string& payload, const std::string& key) {
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string::npos) line_end = payload.size();
    std::string line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const std::size_t eq = line.find('=');
    if (eq != std::string::npos) {
      const std::string lhs_key =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              std::string_view(line).substr(0, eq));
      if (lhs_key == key) return true;
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return false;
}

std::string join_csv(const std::vector<std::string>& parts) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) out += ",";
    out += parts[i];
  }
  return out;
}

std::vector<int> make_repeating_tokens(std::size_t repeats) {
  std::vector<int> out;
  out.reserve(repeats * 3);
  for (std::size_t i = 0; i < repeats; ++i) {
    out.push_back(0);
    out.push_back(1);
    out.push_back(2);
  }
  return out;
}

std::vector<int> make_shuffled_tokens(std::size_t repeats) {
  std::vector<int> out = make_repeating_tokens(repeats);
  std::uint64_t state = 0x9e3779b97f4a7c15ULL;
  for (std::size_t i = out.size(); i > 1; --i) {
    state = state * 6364136223846793005ULL + 1ULL;
    const std::size_t j = static_cast<std::size_t>(state % i);
    std::swap(out[i - 1], out[j]);
  }
  return out;
}

std::vector<double> band_values_from_tokens(const std::vector<int>& tokens) {
  std::array<std::size_t, 3> counts{0, 0, 0};
  std::vector<double> out;
  out.reserve(tokens.size());
  for (int token : tokens) {
    const std::size_t bucket =
        static_cast<std::size_t>(std::clamp(token, 0, 2));
    const double base = (bucket == 0) ? -3.0 : (bucket == 1) ? 0.0 : 3.0;
    const double jitter = 1e-6 * static_cast<double>(counts[bucket]++);
    out.push_back(base + jitter);
  }
  return out;
}

std::vector<double> make_sine_series(std::size_t count, double cycles) {
  constexpr double kPi = 3.14159265358979323846;
  std::vector<double> out;
  out.reserve(count);
  const double denom = (count > 0) ? static_cast<double>(count) : 1.0;
  for (std::size_t i = 0; i < count; ++i) {
    const double phase =
        (2.0 * kPi * cycles * static_cast<double>(i)) / denom;
    out.push_back(std::sin(phase));
  }
  return out;
}

std::vector<double> make_trend_series(std::size_t count) {
  std::vector<double> out;
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(0.05 * static_cast<double>(i) + 0.2 * std::sin(0.1 * i));
  }
  return out;
}

std::vector<double> make_alternating_series(std::size_t count) {
  std::vector<double> out;
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back((i % 2 == 0) ? 1.0 : -1.0);
  }
  return out;
}

torch::Tensor make_anchor_feature_tensor(const std::vector<double>& series,
                                         std::int64_t anchor_feature_index,
                                         std::int64_t feature_count) {
  auto tensor = torch::zeros(
      {static_cast<long>(series.size()), 1, 1, static_cast<long>(feature_count)},
      torch::TensorOptions().dtype(torch::kFloat64));
  for (std::size_t i = 0; i < series.size(); ++i) {
    tensor.index_put_(
        {static_cast<long>(i), 0, 0, anchor_feature_index},
        series[i]);
  }
  return tensor;
}

}  // namespace

int main() try {
  bool ok = true;

  {
    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 64;
    opt.max_features = 16;

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t acc(opt);

    const auto features = torch::tensor(
        {{{{1.0, 0.0}, {0.0, 0.0}}},
         {{{2.0, 1.0}, {0.0, 0.0}}},
         {{{3.0, 2.0}, {0.0, 0.0}}},
         {{{4.0, 3.0}, {0.0, 0.0}}},
         {{{5.0, 4.0}, {0.0, 0.0}}},
         {{{6.0, 5.0}, {0.0, 0.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask = torch::ones({6, 1, 2}, torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(acc.ingest(features, mask), "data analytics ingest should succeed");
    const auto report = acc.summarize();

    ok = ok && expect(
                    report.schema == std::string(cuwacunu::piaabo::torch_compat::kDataAnalyticsSchemaCurrent),
                    "data analytics schema mismatch");
    ok = ok && expect(report.valid_sample_count == 6, "valid sample count mismatch");
    ok = ok && expect(report.source_entropic_load >= 1.0, "source entropic load should be >= 1");
    ok = ok && expect(report.source_entropic_load <= 4.1, "source entropic load upper bound sanity");

    const std::string kv =
        cuwacunu::piaabo::torch_compat::data_analytics_to_latent_lineage_state_text(
            report,
            opt,
            "tsi.source.dataloader.test");
    const auto source_identity = tsiemene::make_component_report_identity(
        "data_analytics",
        "tsi.source.dataloader.BTCUSDT",
        "tsi.source.dataloader",
        {},
        "contract.deadbeef");
    const std::string kv_with_identity =
        cuwacunu::piaabo::torch_compat::data_analytics_to_latent_lineage_state_text(
            report,
            opt,
            "tsi.source.dataloader.test",
            source_identity);
    ok = ok && expect(
                    has_lhs_key(kv, "source_entropic_load"),
                    "data analytics kv missing source_entropic_load");
    ok = ok && expect(
                    has_lhs_key(kv, "schema"),
                    "data analytics kv missing schema");
    ok = ok && expect(
                    has_lhs_key(kv_with_identity, "canonical_path"),
                    "data analytics kv missing canonical_path in component envelope");
    ok = ok && expect(
                    kv.find("schema:str = piaabo.torch_compat.data_analytics.v1") !=
                        std::string::npos,
                    "data analytics kv should keep legacy schema");
  }

  {
    using cuwacunu::piaabo::torch_compat::sequence_analytics_accumulator_t;

    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 8;
    opt.max_features = 32;

    sequence_analytics_accumulator_t acc(opt);
    const auto latent = torch::tensor(
        {{1.0, 0.0}, {2.0, 1.0}, {3.0, 1.5}, {4.0, 2.5}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto latent_mask =
        torch::ones({4}, torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    acc.ingest(latent, latent_mask),
                    "generic sequence analytics ingest should succeed");
    const auto report = acc.summarize();
    ok = ok && expect(
                    report.schema ==
                        std::string(
                            cuwacunu::piaabo::torch_compat::
                                kSequenceAnalyticsSchemaCurrent),
                    "generic sequence analytics schema mismatch");
    ok = ok && expect(
                    report.sequence_channels == 1,
                    "generic sequence analytics should infer one channel for [T,D]");
    ok = ok && expect(
                    report.sequence_timesteps == 4,
                    "generic sequence analytics timestep count mismatch");
    ok = ok && expect(
                    report.sequence_features_per_timestep == 2,
                    "generic sequence analytics feature count mismatch");

    const std::string kv =
        cuwacunu::piaabo::torch_compat::
            sequence_analytics_to_latent_lineage_state_text(
                report,
                opt,
                "wikimyei.latent_sequence");
    ok = ok && expect(
                    has_lhs_key(kv, "sequence_entropic_load"),
                    "generic sequence kv missing sequence_entropic_load");
    ok = ok && expect(
                    has_lhs_key(kv, "sequence_channels"),
                    "generic sequence kv missing sequence_channels");
    ok = ok && expect(
                    kv.find("schema:str = piaabo.torch_compat.sequence_analytics.v1") !=
                        std::string::npos,
                    "generic sequence kv should carry generic schema");
  }

  {
    using cuwacunu::piaabo::torch_compat::data_feature_names_for_record_type;
    using cuwacunu::piaabo::torch_compat::data_symbolic_analytics_accumulator_t;
    using cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t;

    data_symbolic_channel_descriptor_t descriptor{};
    descriptor.label = "BTCUSDT/1d/kline";
    descriptor.record_type = "kline";
    descriptor.anchor_feature = "close_price";
    descriptor.anchor_feature_index = 3;
    descriptor.feature_names = join_csv(data_feature_names_for_record_type("kline"));

    const auto kline_feature_count =
        static_cast<std::int64_t>(data_feature_names_for_record_type("kline").size());
    const auto mask_opts = torch::TensorOptions().dtype(torch::kFloat64);

    const auto low_values =
        band_values_from_tokens(make_repeating_tokens(/*repeats=*/60));
    const auto high_values =
        band_values_from_tokens(make_shuffled_tokens(/*repeats=*/60));
    const auto short_values =
        band_values_from_tokens(make_repeating_tokens(/*repeats=*/20));

    data_symbolic_analytics_accumulator_t low_acc({descriptor});
    data_symbolic_analytics_accumulator_t high_acc({descriptor});
    data_symbolic_analytics_accumulator_t short_acc({descriptor});

    ok = ok && expect(
                    low_acc.ingest(
                        make_anchor_feature_tensor(
                            low_values,
                            descriptor.anchor_feature_index,
                            kline_feature_count),
                        torch::ones({static_cast<long>(low_values.size()), 1, 1}, mask_opts)),
                    "low-complexity symbolic ingest should succeed");
    ok = ok && expect(
                    high_acc.ingest(
                        make_anchor_feature_tensor(
                            high_values,
                            descriptor.anchor_feature_index,
                            kline_feature_count),
                        torch::ones({static_cast<long>(high_values.size()), 1, 1}, mask_opts)),
                    "high-complexity symbolic ingest should succeed");
    ok = ok && expect(
                    short_acc.ingest(
                        make_anchor_feature_tensor(
                            short_values,
                            descriptor.anchor_feature_index,
                            kline_feature_count),
                        torch::ones({static_cast<long>(short_values.size()), 1, 1}, mask_opts)),
                    "short symbolic ingest should succeed");

    const auto low_report = low_acc.summarize();
    const auto high_report = high_acc.summarize();
    const auto short_report = short_acc.summarize();

    ok = ok && expect(
                    low_report.schema ==
                        std::string(
                            cuwacunu::piaabo::torch_compat::
                                kDataAnalyticsSymbolicSchemaCurrent),
                    "symbolic analytics schema mismatch");
    ok = ok && expect(low_report.channel_count == 1, "symbolic channel count mismatch");
    ok = ok && expect(
                    low_report.eligible_channel_count == 1,
                    "low symbolic report should have one eligible channel");
    ok = ok && expect(
                    high_report.eligible_channel_count == 1,
                    "high symbolic report should have one eligible channel");
    ok = ok && expect(
                    short_report.channels[0].eligible == false,
                    "short symbolic report should be ineligible");
    ok = ok && expect(
                    short_report.channels[0].lz76_normalized == 0.0,
                    "short symbolic report should zero normalized complexity");
    ok = ok && expect(
                    low_report.channels[0].observed_symbol_count >= 2,
                    "low symbolic report should observe multiple symbols");
    ok = ok && expect(
                    high_report.channels[0].lz76_normalized >
                        low_report.channels[0].lz76_normalized,
                    "high symbolic complexity should exceed repeating pattern");
    ok = ok && expect(
                    high_report.channels[0].information_density >
                        low_report.channels[0].information_density,
                    "high symbolic information density should exceed repeating pattern");
    ok = ok && expect(
                    low_report.channels[0].compression_ratio <
                        high_report.channels[0].compression_ratio,
                    "repeating symbolic pattern should compress better than shuffled");
    ok = ok && expect(
                    low_report.channels[0].autocorrelation_decay_lag >
                        high_report.channels[0].autocorrelation_decay_lag,
                    "repeating symbolic pattern should decay more slowly than shuffled");
    ok = ok && expect(
                    low_report.channels[0].power_spectrum_entropy <
                        high_report.channels[0].power_spectrum_entropy,
                    "repeating symbolic pattern should have lower spectral entropy");

    const auto symbolic_identity = tsiemene::make_component_report_identity(
        "data_analytics_symbolic",
        "tsi.source.dataloader.BTCUSDT",
        "tsi.source.dataloader",
        {},
        "contract.deadbeef");
    const std::string symbolic_kv =
        cuwacunu::piaabo::torch_compat::
            data_symbolic_analytics_to_latent_lineage_state_text(
                low_report,
                "tsi.source.dataloader.test",
                symbolic_identity);
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "channel_1_anchor_feature"),
                    "symbolic kv missing channel_1_anchor_feature");
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "channel_1_feature_names"),
                    "symbolic kv missing channel_1_feature_names");
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "channel_1_lz76_complexity"),
                    "symbolic kv missing channel_1_lz76_complexity");
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "channel_1_compression_ratio"),
                    "symbolic kv missing channel_1_compression_ratio");
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "channel_1_power_spectrum_entropy"),
                    "symbolic kv missing channel_1_power_spectrum_entropy");
    ok = ok && expect(
                    symbolic_kv.find("/*") == std::string::npos,
                    "symbolic kv must remain comment-free");
    ok = ok && expect(
                    symbolic_kv.find("channel_1_anchor_feature:str = close_price") !=
                        std::string::npos,
                    "symbolic kv should carry anchor feature name");
    ok = ok && expect(
                    symbolic_kv.find(
                        "channel_1_feature_names:str = "
                        "open_price,high_price,low_price,close_price,volume,"
                        "quote_asset_volume,number_of_trades,taker_buy_base_volume,"
                        "taker_buy_quote_volume") != std::string::npos,
                    "symbolic kv should carry ordered kline feature names");

    const std::string symbolic_pretty =
        cuwacunu::piaabo::torch_compat::data_symbolic_analytics_to_pretty_text(
            low_report,
            "tsi.source.dataloader.test",
            "/tmp/data_analytics.symbolic.latest.lls",
            /*use_color=*/false);
    ok = ok && expect(
                    symbolic_pretty.find("/* BTCUSDT/1d/kline */") != std::string::npos,
                    "symbolic pretty text should include channel comment");
    ok = ok && expect(
                    symbolic_pretty.find("/* anchor_feature: close_price */") !=
                        std::string::npos,
                    "symbolic pretty text should include anchor-feature comment");
    ok = ok && expect(
                    symbolic_kv.find(
                        "schema:str = piaabo.torch_compat.data_analytics_symbolic.v1") !=
                        std::string::npos,
                    "symbolic kv should keep legacy schema");
  }

  {
    using cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_accumulator_t;
    using cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t;
    using cuwacunu::piaabo::torch_compat::sequence_symbolic_stream_descriptor_t;
    using cuwacunu::piaabo::torch_compat::sequence_symbolic_stream_report_t;

    sequence_symbolic_stream_descriptor_t descriptor{};
    descriptor.label = "latent_dim_000";
    descriptor.stream_family = "latent_dimension";
    descriptor.anchor_feature = "value";
    descriptor.anchor_feature_index = 0;
    descriptor.feature_names = "value";

    const auto latent_values = band_values_from_tokens(make_shuffled_tokens(/*repeats=*/48));
    const auto mask_opts = torch::TensorOptions().dtype(torch::kFloat64);

    sequence_symbolic_analytics_accumulator_t acc({descriptor});
    ok = ok && expect(
                    acc.ingest(
                        make_anchor_feature_tensor(
                            latent_values,
                            descriptor.anchor_feature_index,
                            /*feature_count=*/1),
                        torch::ones(
                            {static_cast<long>(latent_values.size()), 1, 1},
                            mask_opts)),
                    "generic symbolic sequence ingest should succeed");

    const auto report = acc.summarize();
    ok = ok && expect(
                    report.schema ==
                        std::string(
                            cuwacunu::piaabo::torch_compat::
                                kSequenceAnalyticsSymbolicSchemaCurrent),
                    "generic symbolic schema mismatch");
    ok = ok && expect(report.stream_count == 1, "generic symbolic stream count mismatch");
    ok = ok && expect(
                    report.eligible_stream_count == 1,
                    "generic symbolic report should have one eligible stream");

    const std::string kv =
        cuwacunu::piaabo::torch_compat::
            sequence_symbolic_analytics_to_latent_lineage_state_text(
                report,
                "wikimyei.latent_sequence");
    ok = ok && expect(
                    has_lhs_key(kv, "stream_1_stream_family"),
                    "generic symbolic kv missing stream_1_stream_family");
    ok = ok && expect(
                    has_lhs_key(kv, "stream_1_entropy_rate_bits"),
                    "generic symbolic kv missing stream_1_entropy_rate_bits");
    ok = ok && expect(
                    has_lhs_key(kv, "reported_stream_count"),
                    "generic symbolic kv missing reported_stream_count");

    const std::string pretty =
        cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_to_pretty_text(
            report,
            "wikimyei.latent_sequence",
            "/tmp/sequence_analytics.symbolic.latest.lls",
            /*use_color=*/false);
    ok = ok && expect(
                    pretty.find("stream[1].stream_family") != std::string::npos,
                    "generic symbolic pretty text should use stream_family");
    ok = ok && expect(
                    pretty.find("/* latent_dim_000 */") != std::string::npos,
                    "generic symbolic pretty text should include stream comment");

    sequence_symbolic_analytics_report_t wide_report{};
    wide_report.stream_count = 40;
    wide_report.eligible_stream_count = 40;
    wide_report.lz76_normalized_mean = 0.5;
    wide_report.information_density_mean = 0.5;
    wide_report.compression_ratio_mean = 0.5;
    wide_report.autocorrelation_decay_lag_mean = 4.0;
    wide_report.power_spectrum_entropy_mean = 0.5;
    wide_report.hurst_exponent_mean = 0.5;
    for (std::size_t i = 0; i < 40; ++i) {
      sequence_symbolic_stream_report_t stream{};
      stream.label = "latent_dim_" + std::to_string(i);
      stream.stream_family = "latent_dimension";
      stream.anchor_feature = "value";
      stream.feature_names = "value";
      stream.valid_count = 256;
      stream.observed_symbol_count = 3;
      stream.eligible = true;
      stream.lz76_complexity = 10 + i;
      stream.lz76_normalized = 0.1 + 0.01 * static_cast<double>(i);
      stream.entropy_rate_bits = 0.2 + 0.005 * static_cast<double>(i);
      stream.information_density = 0.2 + 0.01 * static_cast<double>(i);
      stream.compression_ratio = 0.8 - 0.01 * static_cast<double>(i);
      stream.autocorrelation_decay_lag = 1.0 + static_cast<double>(i % 7);
      stream.power_spectrum_entropy = 0.2 + 0.01 * static_cast<double>(i);
      stream.hurst_exponent = 0.3 + 0.01 * static_cast<double>(i);
      wide_report.streams.push_back(std::move(stream));
    }
    wide_report.reported_stream_count = wide_report.stream_count;
    wide_report.lz76_normalized_min = wide_report.streams.front().lz76_normalized;
    wide_report.lz76_normalized_min_label = wide_report.streams.front().label;
    wide_report.lz76_normalized_max = wide_report.streams.back().lz76_normalized;
    wide_report.lz76_normalized_max_label = wide_report.streams.back().label;
    wide_report.information_density_min = wide_report.streams.front().information_density;
    wide_report.information_density_min_label = wide_report.streams.front().label;
    wide_report.information_density_max = wide_report.streams.back().information_density;
    wide_report.information_density_max_label = wide_report.streams.back().label;
    wide_report.compression_ratio_min = wide_report.streams.back().compression_ratio;
    wide_report.compression_ratio_min_label = wide_report.streams.back().label;
    wide_report.compression_ratio_max = wide_report.streams.front().compression_ratio;
    wide_report.compression_ratio_max_label = wide_report.streams.front().label;
    wide_report.autocorrelation_decay_lag_min = 1.0;
    wide_report.autocorrelation_decay_lag_min_label = wide_report.streams.front().label;
    wide_report.autocorrelation_decay_lag_max = 7.0;
    wide_report.autocorrelation_decay_lag_max_label = wide_report.streams[6].label;
    wide_report.power_spectrum_entropy_min = wide_report.streams.front().power_spectrum_entropy;
    wide_report.power_spectrum_entropy_min_label = wide_report.streams.front().label;
    wide_report.power_spectrum_entropy_max = wide_report.streams.back().power_spectrum_entropy;
    wide_report.power_spectrum_entropy_max_label = wide_report.streams.back().label;
    wide_report.hurst_exponent_min = wide_report.streams.front().hurst_exponent;
    wide_report.hurst_exponent_min_label = wide_report.streams.front().label;
    wide_report.hurst_exponent_max = wide_report.streams.back().hurst_exponent;
    wide_report.hurst_exponent_max_label = wide_report.streams.back().label;

    const std::string reduced_kv =
        cuwacunu::piaabo::torch_compat::
            sequence_symbolic_analytics_to_latent_lineage_state_text(
                wide_report,
                "wikimyei.latent_sequence");
    ok = ok && expect(
                    has_lhs_key(reduced_kv, "stream_report_reduced"),
                    "wide generic symbolic kv missing reduction flag");
    ok = ok && expect(
                    reduced_kv.find("reported_stream_count(0,+inf):uint = 16") !=
                        std::string::npos,
                    "wide generic symbolic kv should cap reported streams");
    ok = ok && expect(
                    reduced_kv.find("omitted_stream_count(0,+inf):uint = 24") !=
                        std::string::npos,
                    "wide generic symbolic kv should report omitted streams");
    ok = ok && expect(
                    reduced_kv.find("stream_17_label") == std::string::npos,
                    "wide generic symbolic kv should not serialize more than 16 streams");

    const std::string reduced_pretty =
        cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_to_pretty_text(
            wide_report,
            "wikimyei.latent_sequence",
            "/tmp/sequence_analytics.symbolic.latest.lls",
            /*use_color=*/false);
    ok = ok && expect(
                    reduced_pretty.find("/* reduced stream view: showing 16 representative streams out of 40") !=
                        std::string::npos,
                    "wide generic symbolic pretty text should explain reduction");
    ok = ok && expect(
                    reduced_pretty.find("/* generalized stream view: labels may represent latent or derived sequence streams") !=
                        std::string::npos,
                    "wide generic symbolic pretty text should explain generalized streams");
  }

  {
    using cuwacunu::piaabo::torch_compat::data_symbolic_analytics_accumulator_t;
    using cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t;

    data_symbolic_channel_descriptor_t descriptor{};
    descriptor.label = "BTCUSDT/1d/basic";
    descriptor.record_type = "basic";
    descriptor.anchor_feature = "value";
    descriptor.anchor_feature_index = 0;
    descriptor.feature_names = "value";

    const auto trend_values = make_trend_series(/*count=*/256);
    const auto alternating_values = make_alternating_series(/*count=*/256);
    const auto feature_count = std::int64_t{1};
    const auto mask_opts = torch::TensorOptions().dtype(torch::kFloat64);

    data_symbolic_analytics_accumulator_t trend_acc({descriptor});
    data_symbolic_analytics_accumulator_t alternating_acc({descriptor});
    ok = ok && expect(
                    trend_acc.ingest(
                        make_anchor_feature_tensor(
                            trend_values,
                            descriptor.anchor_feature_index,
                            feature_count),
                        torch::ones({static_cast<long>(trend_values.size()), 1, 1}, mask_opts)),
                    "trend symbolic ingest should succeed");
    ok = ok && expect(
                    alternating_acc.ingest(
                        make_anchor_feature_tensor(
                            alternating_values,
                            descriptor.anchor_feature_index,
                            feature_count),
                        torch::ones(
                            {static_cast<long>(alternating_values.size()), 1, 1},
                            mask_opts)),
                    "alternating symbolic ingest should succeed");

    const auto trend_report = trend_acc.summarize();
    const auto alternating_report = alternating_acc.summarize();
    ok = ok && expect(
                    trend_report.channels[0].hurst_exponent >
                        alternating_report.channels[0].hurst_exponent,
                    "trend series should have higher hurst estimate than alternating series");
  }

  tiny_model_t model{};
  cuwacunu::piaabo::torch_compat::network_analytics_options_t nopt{};
  nopt.spectral_max_elements = 64;
  const auto nrep =
      cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(model, nopt);

  ok = ok && expect(
                  nrep.schema == std::string(cuwacunu::piaabo::torch_compat::kNetworkAnalyticsSchemaCurrent),
                  "network analytics schema mismatch");
  ok = ok && expect(nrep.spectral_tensor_count >= 2, "expected at least two spectral tensors");
  ok = ok && expect(
                  nrep.network_capacity_tensor_count == nrep.spectral_tensor_count,
                  "capacity tensor count should match spectral tensor count");
  ok = ok && expect(
                  nrep.network_global_entropic_capacity > 0.0,
                  "network global entropic capacity should be > 0");
  ok = ok && expect(
                  nrep.network_entropic_bottleneck_min >= 0.0,
                  "network entropic bottleneck must be non-negative");
  ok = ok && expect(
                  nrep.network_effective_rank_p90 >= nrep.network_effective_rank_p50,
                  "effective rank quantiles should be ordered");

  const auto crep =
      cuwacunu::piaabo::torch_compat::summarize_entropic_capacity_comparison(4.0, 2.0);
  ok = ok && expect(crep.capacity_regime == "under_capacity", "ratio<0.8 should be under_capacity");

  const auto crep2 =
      cuwacunu::piaabo::torch_compat::summarize_entropic_capacity_comparison(4.0, 6.4);
  ok = ok && expect(crep2.capacity_regime == "surplus_capacity", "ratio>1.5 should be surplus_capacity");

  {
    const auto tmp = std::filesystem::temp_directory_path();
    const auto dir = tmp / "cuwacunu_entropic_capacity_test";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    const auto source_file = dir / "data_analytics.latest.lls";
    const auto network_file = dir / "weights.init.pt.network_analytics.lls";
    const auto out_file = dir / "weights.init.pt.entropic_capacity.lls";
    const auto symbolic_file = dir / "data_analytics.symbolic.latest.lls";

    {
      std::ofstream out(source_file, std::ios::binary | std::ios::trunc);
      out << "schema:str = piaabo.torch_compat.data_analytics.v1\n";
      out << "source_entropic_load(0,+inf):double = 3.000000000000\n";
    }
    {
      std::ofstream out(network_file, std::ios::binary | std::ios::trunc);
      out << "schema:str = piaabo.torch_compat.network_analytics.v4\n";
      out << "network_global_entropic_capacity(0,+inf):double = 4.500000000000\n";
    }

    cuwacunu::piaabo::torch_compat::entropic_capacity_comparison_report_t from_files{};
    cuwacunu::piaabo::torch_compat::entropic_capacity_comparison_report_t
        from_payloads{};
    std::string err;
    ok = ok && expect(
                    cuwacunu::piaabo::torch_compat::summarize_entropic_capacity_comparison_from_files(
                        source_file,
                        network_file,
                        &from_files,
                        &err),
                    "comparison from files should succeed");
    ok = ok && expect(from_files.capacity_margin > 1.4, "capacity margin from files mismatch");
    ok = ok && expect(
                    cuwacunu::piaabo::torch_compat::
                        summarize_entropic_capacity_comparison_from_payloads(
                            "schema:str = piaabo.torch_compat.data_analytics.v1\n"
                            "run_id:str = run_runtime_001\n"
                            "source_entropic_load(0,+inf):double = 3.000000000000\n",
                            "schema:str = piaabo.torch_compat.network_analytics.v4\n"
                            "run_id:str = run_runtime_001\n"
                            "network_global_entropic_capacity(0,+inf):double = "
                            "4.500000000000\n",
                            &from_payloads,
                            &err),
                    "comparison from payloads should succeed");
    ok = ok && expect(
                    from_payloads.capacity_margin > 1.4,
                    "capacity margin from payloads mismatch");
    err.clear();
    ok = ok && expect(
                    !cuwacunu::piaabo::torch_compat::
                        summarize_entropic_capacity_comparison_from_payloads(
                            "schema:str = piaabo.torch_compat.data_analytics.v1\n",
                            "schema:str = piaabo.torch_compat.network_analytics.v4\n"
                            "network_global_entropic_capacity(0,+inf):double = "
                            "4.500000000000\n",
                            &from_payloads,
                            &err),
                    "comparison from payloads should fail when source key is missing");
    ok = ok && expect(
                    err.find("source_entropic_load") != std::string::npos,
                    "missing source key error should mention source_entropic_load");

    ok = ok && expect(
                    cuwacunu::piaabo::torch_compat::write_entropic_capacity_comparison_file(
                        from_files,
                        out_file,
                        &err),
                    "comparison sidecar write should succeed");

    {
      cuwacunu::piaabo::torch_compat::data_symbolic_channel_report_t channel{};
      channel.label = "BTCUSDT/1d/kline";
      channel.record_type = "kline";
      channel.anchor_feature = "close_price";
      channel.feature_names =
          "open_price,high_price,low_price,close_price,volume,"
          "quote_asset_volume,number_of_trades,taker_buy_base_volume,"
          "taker_buy_quote_volume";
      channel.valid_count = 180;
      channel.observed_symbol_count = 3;
      channel.eligible = true;
      channel.lz76_complexity = 24;
      channel.lz76_normalized = 0.71;
      channel.entropy_rate_bits = 0.42;
      channel.information_density = 0.27;
      channel.compression_ratio = 0.38;
      channel.autocorrelation_decay_lag = 7.0;
      channel.power_spectrum_entropy = 0.19;
      channel.hurst_exponent = 0.84;

      cuwacunu::piaabo::torch_compat::data_symbolic_analytics_report_t symbolic_report{};
      symbolic_report.channel_count = 1;
      symbolic_report.eligible_channel_count = 1;
      symbolic_report.lz76_normalized_mean = channel.lz76_normalized;
      symbolic_report.lz76_normalized_min = channel.lz76_normalized;
      symbolic_report.lz76_normalized_min_label = channel.label;
      symbolic_report.lz76_normalized_max = channel.lz76_normalized;
      symbolic_report.lz76_normalized_max_label = channel.label;
      symbolic_report.information_density_mean = channel.information_density;
      symbolic_report.information_density_min = channel.information_density;
      symbolic_report.information_density_min_label = channel.label;
      symbolic_report.information_density_max = channel.information_density;
      symbolic_report.information_density_max_label = channel.label;
      symbolic_report.compression_ratio_mean = channel.compression_ratio;
      symbolic_report.compression_ratio_min = channel.compression_ratio;
      symbolic_report.compression_ratio_min_label = channel.label;
      symbolic_report.compression_ratio_max = channel.compression_ratio;
      symbolic_report.compression_ratio_max_label = channel.label;
      symbolic_report.autocorrelation_decay_lag_mean =
          channel.autocorrelation_decay_lag;
      symbolic_report.autocorrelation_decay_lag_min =
          channel.autocorrelation_decay_lag;
      symbolic_report.autocorrelation_decay_lag_min_label = channel.label;
      symbolic_report.autocorrelation_decay_lag_max =
          channel.autocorrelation_decay_lag;
      symbolic_report.autocorrelation_decay_lag_max_label = channel.label;
      symbolic_report.power_spectrum_entropy_mean =
          channel.power_spectrum_entropy;
      symbolic_report.power_spectrum_entropy_min = channel.power_spectrum_entropy;
      symbolic_report.power_spectrum_entropy_min_label = channel.label;
      symbolic_report.power_spectrum_entropy_max = channel.power_spectrum_entropy;
      symbolic_report.power_spectrum_entropy_max_label = channel.label;
      symbolic_report.hurst_exponent_mean = channel.hurst_exponent;
      symbolic_report.hurst_exponent_min = channel.hurst_exponent;
      symbolic_report.hurst_exponent_min_label = channel.label;
      symbolic_report.hurst_exponent_max = channel.hurst_exponent;
      symbolic_report.hurst_exponent_max_label = channel.label;
      symbolic_report.channels.push_back(channel);

      ok = ok && expect(
                      cuwacunu::piaabo::torch_compat::write_data_symbolic_analytics_file(
                          symbolic_report,
                          symbolic_file,
                          "tsi.source.dataloader.test",
                          &err),
                      "symbolic report write should succeed");

      std::ifstream in(symbolic_file, std::ios::binary);
      std::string payload(
          (std::istreambuf_iterator<char>(in)),
          std::istreambuf_iterator<char>());
      ok = ok && expect(
                      has_lhs_key(payload, "channel_1_information_density"),
                      "symbolic sidecar should include information density");
      ok = ok && expect(
                      has_lhs_key(payload, "channel_1_compression_ratio"),
                      "symbolic sidecar should include compression ratio");
      ok = ok && expect(
                      has_lhs_key(payload, "channel_1_autocorrelation_decay_lag"),
                      "symbolic sidecar should include autocorrelation decay");
      ok = ok && expect(
                      has_lhs_key(payload, "channel_1_power_spectrum_entropy"),
                      "symbolic sidecar should include power spectrum entropy");
      ok = ok && expect(
                      has_lhs_key(payload, "channel_1_hurst_exponent"),
                      "symbolic sidecar should include hurst exponent");
      ok = ok && expect(
                      payload.find("/*") == std::string::npos,
                      "symbolic sidecar should remain comment-free");
    }

    std::ifstream in(out_file, std::ios::binary);
    std::string payload((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ok = ok && expect(
                    has_lhs_key(payload, "capacity_ratio"),
                    "comparison sidecar should include capacity_ratio");
  }

  if (!ok) return 1;
  std::cout << "[test_entropic_capacity_analytics] PASS\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_entropic_capacity_analytics] exception: " << e.what() << "\n";
  return 1;
}
