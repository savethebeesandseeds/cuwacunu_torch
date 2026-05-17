#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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

bool expect_near(double lhs, double rhs, double tol, const std::string& msg) {
  if (std::abs(lhs - rhs) > tol) {
    std::cerr << "[test_entropic_capacity_analytics] FAIL: " << msg
              << " lhs=" << lhs << " rhs=" << rhs << " tol=" << tol << "\n";
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

std::string read_file_text(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(
      (std::istreambuf_iterator<char>(in)),
      std::istreambuf_iterator<char>());
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
            "BTCUSDT");
    const auto source_identity = tsiemene::make_component_report_identity(
        "tsi.source.dataloader",
        "bind.train.v1",
        "source.data");
    const std::string kv_with_identity =
        cuwacunu::piaabo::torch_compat::data_analytics_to_latent_lineage_state_text(
            report,
            opt,
            "BTCUSDT",
            source_identity);
    ok = ok && expect(
                    has_lhs_key(kv, "source_entropic_load"),
                    "data analytics kv missing source_entropic_load");
    ok = ok && expect(
                    has_lhs_key(kv, "schema"),
                    "data analytics kv missing schema");
    ok = ok && expect(
                    has_lhs_key(kv_with_identity, "semantic_taxon") &&
                        kv_with_identity.find("source.data") !=
                            std::string::npos,
                    "data analytics kv missing source semantic_taxon");
    ok = ok && expect(
                    has_lhs_key(kv_with_identity, "canonical_path") &&
                        kv_with_identity.find("canonical_path:str = tsi.source.dataloader") !=
                            std::string::npos,
                    "data analytics kv missing canonical_path");
    ok = ok && expect(
                    has_lhs_key(kv_with_identity, "binding_id") &&
                        kv_with_identity.find("binding_id:str = bind.train.v1") !=
                            std::string::npos,
                    "data analytics kv missing binding_id");
    ok = ok && expect(
                    has_lhs_key(kv_with_identity, "source_label") &&
                        kv_with_identity.find("source_label:str = BTCUSDT") !=
                            std::string::npos,
                    "data analytics kv missing bare source_label");
    ok = ok && expect(
                    kv.find("schema:str = piaabo.torch_compat.data_analytics.v2") !=
                        std::string::npos,
                    "data analytics kv should carry v2 schema");
    ok = ok && expect(
                    kv.find("sample_count[0,+inf):uint = 6") != std::string::npos,
                    "data analytics kv should emit nonnegative uint domains");
    ok = ok && expect(
                    kv.find("max_samples[1,+inf):int = 64") != std::string::npos,
                    "data analytics kv should emit positive int domains");
  }

  {
    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 16;
    opt.max_features = 16;
    opt.mask_epsilon = 0.25;

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t zero_acc(opt);
    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t garbage_acc(
        opt);

    const auto features_zero = torch::tensor(
        {{{{1.0, 10.0}, {0.0, 0.0}}},
         {{{2.0, 20.0}, {0.0, 0.0}}},
         {{{3.0, 30.0}, {0.0, 0.0}}},
         {{{4.0, 40.0}, {0.0, 0.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto features_garbage = torch::tensor(
        {{{{1.0, 10.0}, {1000.0, -1000.0}}},
         {{{2.0, 20.0}, {2000.0, -2000.0}}},
         {{{3.0, 30.0}, {3000.0, -3000.0}}},
         {{{4.0, 40.0}, {4000.0, -4000.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask = torch::tensor(
        {{{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}}},
        torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    zero_acc.ingest(features_zero, mask),
                    "mask-aware zero ingest should succeed");
    ok = ok && expect(
                    garbage_acc.ingest(features_garbage, mask),
                    "mask-aware garbage ingest should succeed");

    const auto zero_report = zero_acc.summarize();
    const auto garbage_report = garbage_acc.summarize();
    ok = ok && expect(
                    zero_report.valid_sample_count == 4,
                    "partial-valid rows above threshold should be accepted");
    ok = ok && expect(
                    zero_report.source_effective_feature_count == 2,
                    "effective feature count should reflect only valid sampled columns");
    ok = ok && expect_near(
                    zero_report.source_entropic_load,
                    garbage_report.source_entropic_load,
                    1e-9,
                    "masked garbage should not change entropic load");
    ok = ok && expect_near(
                    zero_report.source_cov_trace,
                    garbage_report.source_cov_trace,
                    1e-9,
                    "masked garbage should not change covariance trace");
    ok = ok && expect(
                    zero_report.source_nonzero_eigen_count ==
                        garbage_report.source_nonzero_eigen_count,
                    "masked garbage should not change nonzero eigen count");
  }

  {
    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 16;
    opt.max_features = 16;
    opt.mask_epsilon = 0.25;

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t zero_acc(opt);
    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t nonfinite_acc(
        opt);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const auto features_zero = torch::tensor(
        {{{{1.0, 10.0}, {0.0, 0.0}}},
         {{{2.0, 20.0}, {0.0, 0.0}}},
         {{{3.0, 30.0}, {0.0, 0.0}}},
         {{{4.0, 40.0}, {0.0, 0.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto features_nonfinite = torch::tensor(
        {{{{1.0, 10.0}, {nan, inf}}},
         {{{2.0, 20.0}, {nan, -inf}}},
         {{{3.0, 30.0}, {nan, inf}}},
         {{{4.0, 40.0}, {nan, -inf}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask = torch::tensor(
        {{{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}}},
        torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    zero_acc.ingest(features_zero, mask),
                    "mask-aware zero ingest should succeed");
    ok = ok && expect(
                    nonfinite_acc.ingest(features_nonfinite, mask),
                    "masked non-finite ingest should succeed");

    const auto zero_report = zero_acc.summarize();
    const auto nonfinite_report = nonfinite_acc.summarize();
    ok = ok && expect(
                    nonfinite_report.valid_sample_count == 4,
                    "masked non-finite rows should still be accepted");
    ok = ok && expect_near(
                    zero_report.source_entropic_load,
                    nonfinite_report.source_entropic_load,
                    1e-9,
                    "masked non-finite values should not change entropic load");
    ok = ok && expect_near(
                    zero_report.source_cov_trace,
                    nonfinite_report.source_cov_trace,
                    1e-9,
                    "masked non-finite values should not change covariance trace");
    ok = ok && expect(
                    zero_report.source_nonzero_eigen_count ==
                        nonfinite_report.source_nonzero_eigen_count,
                    "masked non-finite values should not change nonzero eigen count");
  }

  {
    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 16;
    opt.max_features = 16;

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t acc(opt);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const auto features = torch::tensor(
        {{{{1.0, 10.0}, {2.0, 20.0}}},
         {{{3.0, 30.0}, {nan, 40.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask = torch::ones(
        {2, 1, 2}, torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    acc.ingest(features, mask),
                    "batched ingest with one invalid sample should continue");
    const auto report = acc.summarize();
    ok = ok && expect(report.sample_count == 2, "unmasked non-finite sample count mismatch");
    ok = ok && expect(
                    report.valid_sample_count == 1,
                    "only the finite sample should contribute");
    ok = ok && expect(
                    report.skipped_sample_count == 1,
                    "unmasked non-finite sample should be skipped");
  }

  {
    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 16;
    opt.max_features = 16;
    opt.mask_epsilon = 0.5;

    cuwacunu::piaabo::torch_compat::data_source_analytics_accumulator_t acc(opt);
    const auto features = torch::tensor(
        {{{{1.0, 10.0}, {0.0, 0.0}}},
         {{{2.0, 20.0}, {0.0, 0.0}}},
         {{{3.0, 30.0}, {0.0, 0.0}}},
         {{{4.0, 40.0}, {0.0, 0.0}}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask = torch::tensor(
        {{{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}},
         {{1.0, 0.0}}},
        torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    acc.ingest(features, mask),
                    "threshold-gated ingest should still normalize successfully");
    const auto report = acc.summarize();
    ok = ok && expect(report.sample_count == 4, "threshold-gated sample count mismatch");
    ok = ok && expect(
                    report.valid_sample_count == 0,
                    "rows at or below mask_epsilon should be skipped");
    ok = ok && expect(
                    report.skipped_sample_count == 4,
                    "rows at or below mask_epsilon should count as skipped");
  }

  {
    using cuwacunu::piaabo::torch_compat::source_data_analytics_contract_directory;
    using cuwacunu::piaabo::torch_compat::source_data_analytics_context_directory;

    ok = ok && expect(
                    source_data_analytics_contract_directory("0x").empty(),
                    "0x-only contract hash should not yield a directory");
    ok = ok && expect(
                    source_data_analytics_contract_directory("   ").empty(),
                    "whitespace contract hash should not yield a directory");
    ok = ok && expect(
                    source_data_analytics_context_directory(
                        "0x", "tsi.source.dataloader", "BTCUSDT|01.01.2009|31.12.2009")
                        .empty(),
                    "empty normalized contract token should invalidate context paths");
    const auto sanitized =
        source_data_analytics_contract_directory("contract/hash");
    ok = ok && expect(
                    sanitized.filename() == "contract_hash",
                    "path token sanitization should replace unsafe separators");
    const auto full_hash =
        source_data_analytics_contract_directory("0xabcdef0123456789");
    ok = ok && expect(
                    full_hash.filename() == "abcdef0123456789",
                    "contract hash path token should no longer truncate");
    ok = ok && expect(
                    full_hash.parent_path().filename() == "contracts",
                    "source analytics paths should bucket contract hashes");
    ok = ok && expect(
                    full_hash.parent_path().parent_path().filename() ==
                        "dataloader",
                    "source analytics paths should use the canonical source root");
    const auto context_dir = source_data_analytics_context_directory(
        "0xabcdef0123456789",
        "tsi.source.dataloader",
        "BTCUSDT|01.01.2009|31.12.2009");
    ok = ok && expect(
                    context_dir.filename() == "BTCUSDT_01.01.2009_31.12.2009",
                    "source runtime cursor should be path-normalized");
    ok = ok && expect(
                    context_dir.parent_path().filename() == "contexts",
                    "source analytics paths should partition by context buckets");
    ok = ok && expect(
                    context_dir.parent_path().parent_path().filename() ==
                        "abcdef0123456789",
                    "source analytics paths should partition by contract hash");
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
                    kv.find("schema:str = piaabo.torch_compat.sequence_analytics.v2") !=
                        std::string::npos,
                    "generic sequence kv should carry v2 schema");
  }

  {
    using cuwacunu::piaabo::torch_compat::sequence_analytics_accumulator_t;

    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 8;
    opt.max_features = 32;

    sequence_analytics_accumulator_t acc(opt);
    const auto three_d = torch::tensor(
        {{{1.0}, {2.0}, {3.0}, {4.0}},
         {{5.0}, {6.0}, {7.0}, {8.0}},
         {{9.0}, {10.0}, {11.0}, {12.0}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto mask =
        torch::ones({3, 4}, torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    acc.ingest(three_d, mask),
                    "3D generic sequence ingest should succeed");
    const auto report = acc.summarize();
    ok = ok && expect(
                    report.sequence_channels == 3,
                    "3D generic sequence should treat the first axis as channels");
    ok = ok && expect(
                    report.sequence_timesteps == 4,
                    "3D generic sequence timestep count mismatch");
    ok = ok && expect(
                    report.sequence_features_per_timestep == 1,
                    "3D generic sequence feature count mismatch");
  }

  {
    using cuwacunu::piaabo::torch_compat::sequence_analytics_accumulator_t;

    cuwacunu::piaabo::torch_compat::data_analytics_options_t opt{};
    opt.max_samples = 8;
    opt.max_features = 32;

    sequence_analytics_accumulator_t acc(opt);
    const auto first = torch::tensor(
        {{1.0, 0.0}, {2.0, 1.0}, {3.0, 1.5}, {4.0, 2.5}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto second = torch::tensor(
        {{{1.0, 2.0}, {3.0, 4.0}},
         {{5.0, 6.0}, {7.0, 8.0}}},
        torch::TensorOptions().dtype(torch::kFloat64));
    const auto first_mask =
        torch::ones({4}, torch::TensorOptions().dtype(torch::kFloat64));
    const auto second_mask =
        torch::ones({2, 2}, torch::TensorOptions().dtype(torch::kFloat64));

    ok = ok && expect(
                    acc.ingest(first, first_mask),
                    "baseline geometry ingest should succeed");
    ok = ok && expect(
                    !acc.ingest(second, second_mask),
                    "mismatched normalized geometry should be rejected");
    const auto report = acc.summarize();
    ok = ok && expect(
                    report.sample_count == 2,
                    "geometry-rejection sample count mismatch");
    ok = ok && expect(
                    report.valid_sample_count == 1,
                    "geometry-rejection should preserve only the first ingest");
    ok = ok && expect(
                    report.skipped_sample_count == 1,
                    "geometry-rejection should count the rejected ingest as skipped");
    ok = ok && expect(
                    report.sequence_channels == 1 && report.sequence_timesteps == 4 &&
                        report.sequence_features_per_timestep == 2,
                    "geometry-rejection should preserve the locked layout");
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
        "tsi.source.dataloader",
        "bind.train.v1",
        "source.data");
    const std::string symbolic_kv =
        cuwacunu::piaabo::torch_compat::
            data_symbolic_analytics_to_latent_lineage_state_text(
                low_report,
                "BTCUSDT",
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
                    has_lhs_key(symbolic_kv, "semantic_taxon") &&
                        symbolic_kv.find("source.data") !=
                            std::string::npos,
                    "symbolic kv missing source semantic_taxon");
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "canonical_path") &&
                        symbolic_kv.find("canonical_path:str = tsi.source.dataloader") !=
                            std::string::npos,
                    "symbolic kv missing canonical_path");
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
    ok = ok && expect(
                    has_lhs_key(symbolic_kv, "source_label") &&
                        symbolic_kv.find("source_label:str = BTCUSDT") !=
                            std::string::npos,
                    "symbolic kv missing bare source_label");

    const std::string symbolic_pretty =
        cuwacunu::piaabo::torch_compat::data_symbolic_analytics_to_pretty_text(
            low_report,
            "BTCUSDT",
            "/tmp/data_analytics.symbolic.v2.latest.lls",
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
                        "schema:str = piaabo.torch_compat.data_analytics_symbolic.v2") !=
                        std::string::npos,
                    "symbolic kv should carry v2 schema");
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
            "/tmp/sequence_analytics.symbolic.v2.latest.lls",
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
                    reduced_kv.find("reported_stream_count[0,+inf):uint = 16") !=
                        std::string::npos,
                    "wide generic symbolic kv should cap reported streams");
    ok = ok && expect(
                    reduced_kv.find("omitted_stream_count[0,+inf):uint = 24") !=
                        std::string::npos,
                    "wide generic symbolic kv should report omitted streams");
    ok = ok && expect(
                    reduced_kv.find("stream_17_label") == std::string::npos,
                    "wide generic symbolic kv should not serialize more than 16 streams");

    const std::string reduced_pretty =
        cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_to_pretty_text(
            wide_report,
            "wikimyei.latent_sequence",
            "/tmp/sequence_analytics.symbolic.v2.latest.lls",
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

  {
    using cuwacunu::piaabo::torch_compat::data_symbolic_analytics_accumulator_t;
    using cuwacunu::piaabo::torch_compat::data_symbolic_channel_descriptor_t;

    data_symbolic_channel_descriptor_t descriptor{};
    descriptor.label = "BTCUSDT/1d/basic";
    descriptor.record_type = "basic";
    descriptor.anchor_feature = "value";
    descriptor.anchor_feature_index = 0;
    descriptor.feature_names = "value";

    const auto sample_count = std::size_t{150};
    auto features = torch::zeros(
        {static_cast<long>(sample_count), 1, 3, 1},
        torch::TensorOptions().dtype(torch::kFloat64));
    auto mask = torch::zeros(
        {static_cast<long>(sample_count), 1, 3},
        torch::TensorOptions().dtype(torch::kFloat64));
    for (std::size_t i = 0; i < sample_count; ++i) {
      const double anchor_value = static_cast<double>((i % 7) - 3);
      features.index_put_({static_cast<long>(i), 0, 0, 0}, -99.0);
      features.index_put_({static_cast<long>(i), 0, 1, 0}, anchor_value);
      features.index_put_({static_cast<long>(i), 0, 2, 0}, 1000.0 + i);
      mask.index_put_({static_cast<long>(i), 0, 0}, 1.0);
      mask.index_put_({static_cast<long>(i), 0, 1}, 1.0);
      mask.index_put_({static_cast<long>(i), 0, 2}, 0.0);
    }

    data_symbolic_analytics_accumulator_t acc({descriptor});
    ok = ok && expect(
                    acc.ingest(features, mask),
                    "symbolic last-valid-timestep ingest should succeed");
    const auto report = acc.summarize();
    ok = ok && expect(
                    report.eligible_channel_count == 1,
                    "last-valid symbolic series should stay eligible");
    ok = ok && expect(
                    report.channels[0].valid_count == sample_count,
                    "last-valid symbolic series should use the last valid timestep per sample");
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

    const auto symbolic_file = dir / "data_analytics.symbolic.v2.latest.lls";
    cuwacunu::piaabo::torch_compat::entropic_capacity_comparison_report_t
        from_payloads{};
    std::string err;
    ok = ok && expect(
                    cuwacunu::piaabo::torch_compat::
                        summarize_entropic_capacity_comparison_from_payloads(
                            "schema:str = piaabo.torch_compat.data_analytics.v2\n"
                            "source_entropic_load[0,+inf):double = 3.000000000000\n",
                            "schema:str = piaabo.torch_compat.network_analytics.v5\n"
                            "network_global_entropic_capacity(0,+inf):double = "
                            "4.500000000000\n",
                            &from_payloads,
                            &err),
                    "comparison from payloads should succeed");
    ok = ok && expect(
                    from_payloads.capacity_margin > 1.4,
                    "capacity margin from payloads mismatch");
    const auto comparison_text =
        cuwacunu::piaabo::torch_compat::
            entropic_capacity_comparison_to_latent_lineage_state_text(
                from_payloads);
    ok = ok && expect(
                    has_lhs_key(comparison_text, "capacity_ratio"),
                    "comparison text should include capacity_ratio");
    ok = ok && expect(
                    comparison_text.find("source_entropic_load") !=
                        std::string::npos,
                    "comparison text should include source_entropic_load");
    err.clear();
    ok = ok && expect(
                    !cuwacunu::piaabo::torch_compat::
                        summarize_entropic_capacity_comparison_from_payloads(
                            "schema:str = piaabo.torch_compat.data_analytics.v2\n",
                            "schema:str = piaabo.torch_compat.network_analytics.v5\n"
                            "network_global_entropic_capacity(0,+inf):double = "
                            "4.500000000000\n",
                            &from_payloads,
                            &err),
                    "comparison from payloads should fail when source key is missing");
    ok = ok && expect(
                    err.find("source_entropic_load") != std::string::npos,
                    "missing source key error should mention source_entropic_load");

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
                          "BTCUSDT",
                          &err),
                      "symbolic report write should succeed");

      std::string payload = read_file_text(symbolic_file);
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
      ok = ok && expect(
                      has_lhs_key(payload, "source_label") &&
                          payload.find("source_label:str = BTCUSDT") !=
                              std::string::npos,
                      "symbolic sidecar should carry bare source_label");

      const auto failing_target = dir / "data_analytics.symbolic.atomic_target";
      std::filesystem::create_directories(failing_target, ec);
      err.clear();
      ok = ok && expect(
                      !cuwacunu::piaabo::torch_compat::write_data_symbolic_analytics_file(
                          symbolic_report,
                          failing_target,
                          "BTCUSDT",
                          &err),
                      "atomic writer should fail when the destination is a directory");
      ok = ok && expect(
                      std::filesystem::is_directory(failing_target),
                      "atomic writer failure should preserve the existing target");
      bool left_temp_file = false;
      for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto name = entry.path().filename().string();
        if (name.find("data_analytics.symbolic.atomic_target.tmp.") == 0) {
          left_temp_file = true;
          break;
        }
      }
      ok = ok && expect(
                      !left_temp_file,
                      "atomic writer failure should clean up temporary files");
    }
  }

  if (!ok) return 1;
  std::cout << "[test_entropic_capacity_analytics] PASS\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_entropic_capacity_analytics] exception: " << e.what() << "\n";
  return 1;
}
