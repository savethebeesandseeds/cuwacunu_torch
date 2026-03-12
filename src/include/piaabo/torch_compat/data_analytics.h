#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/tsi.report.h"

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

inline constexpr std::string_view kDataAnalyticsSchemaV1 =
    "piaabo.torch_compat.data_analytics.v1";
inline constexpr std::string_view kDataAnalyticsSchemaCurrent =
    kDataAnalyticsSchemaV1;
inline constexpr std::string_view kDataAnalyticsLatestReportFilename =
    "data_analytics.latest.lls";

struct data_analytics_options_t {
  std::int64_t max_samples{4096};
  std::int64_t max_features{2048};
  double mask_epsilon{1e-12};
  double standardize_epsilon{1e-8};
};

struct data_source_analytics_report_t {
  std::string schema{std::string(kDataAnalyticsSchemaCurrent)};

  std::uint64_t sample_count{0};
  std::uint64_t valid_sample_count{0};
  std::uint64_t skipped_sample_count{0};

  std::int64_t source_channels{0};
  std::int64_t source_timesteps{0};
  std::int64_t source_features_per_timestep{0};
  std::int64_t source_flat_feature_count{0};
  std::int64_t source_effective_feature_count{0};

  double source_entropic_load{0.0};
  double source_cov_trace{0.0};
  std::uint64_t source_nonzero_eigen_count{0};
};

class data_source_analytics_accumulator_t {
 public:
  explicit data_source_analytics_accumulator_t(
      data_analytics_options_t options = {});

  void reset();
  bool ingest(const torch::Tensor& features, const torch::Tensor& mask = {});

  [[nodiscard]] data_source_analytics_report_t summarize() const;
  [[nodiscard]] const data_analytics_options_t& options() const noexcept {
    return options_;
  }

 private:
  data_analytics_options_t options_{};
  std::vector<torch::Tensor> rows_{};
  std::vector<double> row_weights_{};
  std::uint64_t sample_count_{0};
  std::uint64_t skipped_sample_count_{0};
  std::int64_t source_channels_{0};
  std::int64_t source_timesteps_{0};
  std::int64_t source_features_per_timestep_{0};
  std::int64_t source_flat_feature_count_{0};
  std::int64_t source_effective_feature_count_{0};
};

[[nodiscard]] std::string data_analytics_to_latent_lineage_state_text(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view source_label = {},
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] bool write_data_analytics_file(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view source_label = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string extract_data_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] bool is_supported_data_analytics_schema(
    std::string_view schema);

[[nodiscard]] std::filesystem::path source_data_analytics_root_directory();

[[nodiscard]] std::filesystem::path source_data_analytics_contract_directory(
    std::string_view contract_hash);

[[nodiscard]] std::filesystem::path source_data_analytics_instance_directory(
    std::string_view contract_hash,
    std::string_view source_instance);

[[nodiscard]] std::filesystem::path source_data_analytics_latest_file_path(
    std::string_view contract_hash,
    std::string_view source_instance);

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
