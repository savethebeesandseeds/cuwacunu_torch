#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
    const auto out_file = dir / "weights.init.pt.entropic_capacity.kv";

    {
      std::ofstream out(source_file, std::ios::binary | std::ios::trunc);
      out << "schema=piaabo.torch_compat.data_analytics.v1\n";
      out << "source_entropic_load=3.0\n";
    }
    {
      std::ofstream out(network_file, std::ios::binary | std::ios::trunc);
      out << "schema=piaabo.torch_compat.network_analytics.v4\n";
      out << "network_global_entropic_capacity=4.5\n";
    }

    cuwacunu::piaabo::torch_compat::entropic_capacity_comparison_report_t from_files{};
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
                    cuwacunu::piaabo::torch_compat::write_entropic_capacity_comparison_file(
                        from_files,
                        out_file,
                        &err),
                    "comparison sidecar write should succeed");

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
