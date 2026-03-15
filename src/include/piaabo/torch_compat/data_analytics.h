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
inline constexpr std::string_view kSequenceAnalyticsSchemaV1 =
    "piaabo.torch_compat.sequence_analytics.v1";
inline constexpr std::string_view kSequenceAnalyticsSchemaCurrent =
    kSequenceAnalyticsSchemaV1;
inline constexpr std::string_view kSequenceAnalyticsLatestReportFilename =
    "sequence_analytics.latest.lls";
inline constexpr std::string_view kEmbeddingSequenceAnalyticsSchemaV1 =
    "piaabo.torch_compat.embedding_sequence_analytics.v1";
inline constexpr std::string_view kEmbeddingSequenceAnalyticsSchemaCurrent =
    kEmbeddingSequenceAnalyticsSchemaV1;
inline constexpr std::string_view
    kEmbeddingSequenceAnalyticsLatestReportFilename =
        "embedding_sequence_analytics.latest.lls";
inline constexpr std::string_view kDataAnalyticsSymbolicSchemaV1 =
    "piaabo.torch_compat.data_analytics_symbolic.v1";
inline constexpr std::string_view kDataAnalyticsSymbolicSchemaCurrent =
    kDataAnalyticsSymbolicSchemaV1;
inline constexpr std::string_view kDataAnalyticsSymbolicLatestReportFilename =
    "data_analytics.symbolic.latest.lls";
inline constexpr std::string_view kSequenceAnalyticsSymbolicSchemaV1 =
    "piaabo.torch_compat.sequence_analytics_symbolic.v1";
inline constexpr std::string_view kSequenceAnalyticsSymbolicSchemaCurrent =
    kSequenceAnalyticsSymbolicSchemaV1;
inline constexpr std::string_view kSequenceAnalyticsSymbolicLatestReportFilename =
    "sequence_analytics.symbolic.latest.lls";
inline constexpr std::string_view kEmbeddingSequenceAnalyticsSymbolicSchemaV1 =
    "piaabo.torch_compat.embedding_sequence_analytics_symbolic.v1";
inline constexpr std::string_view
    kEmbeddingSequenceAnalyticsSymbolicSchemaCurrent =
        kEmbeddingSequenceAnalyticsSymbolicSchemaV1;
inline constexpr std::string_view
    kEmbeddingSequenceAnalyticsSymbolicLatestReportFilename =
        "embedding_sequence_analytics.symbolic.latest.lls";

struct data_analytics_options_t {
  std::int64_t max_samples{4096};
  std::int64_t max_features{2048};
  double mask_epsilon{1e-12};
  double standardize_epsilon{1e-8};
};

struct sequence_analytics_report_t {
  std::string schema{std::string(kSequenceAnalyticsSchemaCurrent)};

  std::uint64_t sample_count{0};
  std::uint64_t valid_sample_count{0};
  std::uint64_t skipped_sample_count{0};

  std::int64_t sequence_channels{0};
  std::int64_t sequence_timesteps{0};
  std::int64_t sequence_features_per_timestep{0};
  std::int64_t sequence_flat_feature_count{0};
  std::int64_t sequence_effective_feature_count{0};

  double sequence_entropic_load{0.0};
  double sequence_cov_trace{0.0};
  std::uint64_t sequence_nonzero_eigen_count{0};
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

struct sequence_symbolic_stream_descriptor_t {
  std::string label{};
  std::string stream_family{};
  std::string anchor_feature{};
  std::string feature_names{};
  std::int64_t anchor_feature_index{-1};
};

struct data_symbolic_channel_descriptor_t {
  std::string label{};
  std::string record_type{};
  std::string anchor_feature{};
  std::string feature_names{};
  std::int64_t anchor_feature_index{-1};
};

struct sequence_symbolic_stream_report_t {
  std::string label{};
  std::string stream_family{};
  std::string anchor_feature{};
  std::string feature_names{};
  std::uint64_t valid_count{0};
  std::uint64_t observed_symbol_count{0};
  bool eligible{false};
  std::uint64_t lz76_complexity{0};
  double lz76_normalized{0.0};
  double entropy_rate_bits{0.0};
  double information_density{0.0};
  double compression_ratio{0.0};
  double autocorrelation_decay_lag{0.0};
  double power_spectrum_entropy{0.0};
  double hurst_exponent{0.0};
};

struct data_symbolic_channel_report_t {
  std::string label{};
  std::string record_type{};
  std::string anchor_feature{};
  std::string feature_names{};
  std::uint64_t valid_count{0};
  std::uint64_t observed_symbol_count{0};
  bool eligible{false};
  std::uint64_t lz76_complexity{0};
  double lz76_normalized{0.0};
  double entropy_rate_bits{0.0};
  double information_density{0.0};
  double compression_ratio{0.0};
  double autocorrelation_decay_lag{0.0};
  double power_spectrum_entropy{0.0};
  double hurst_exponent{0.0};
};

struct sequence_symbolic_analytics_report_t {
  std::string schema{std::string(kSequenceAnalyticsSymbolicSchemaCurrent)};

  std::uint64_t stream_count{0};
  std::uint64_t eligible_stream_count{0};
  std::uint64_t reported_stream_count{0};
  std::uint64_t omitted_stream_count{0};
  bool stream_report_reduced{false};
  std::string stream_report_reduction_mode{};

  double lz76_normalized_mean{0.0};
  double lz76_normalized_min{0.0};
  std::string lz76_normalized_min_label{};
  double lz76_normalized_max{0.0};
  std::string lz76_normalized_max_label{};

  double information_density_mean{0.0};
  double information_density_min{0.0};
  std::string information_density_min_label{};
  double information_density_max{0.0};
  std::string information_density_max_label{};

  double compression_ratio_mean{0.0};
  double compression_ratio_min{0.0};
  std::string compression_ratio_min_label{};
  double compression_ratio_max{0.0};
  std::string compression_ratio_max_label{};

  double autocorrelation_decay_lag_mean{0.0};
  double autocorrelation_decay_lag_min{0.0};
  std::string autocorrelation_decay_lag_min_label{};
  double autocorrelation_decay_lag_max{0.0};
  std::string autocorrelation_decay_lag_max_label{};

  double power_spectrum_entropy_mean{0.0};
  double power_spectrum_entropy_min{0.0};
  std::string power_spectrum_entropy_min_label{};
  double power_spectrum_entropy_max{0.0};
  std::string power_spectrum_entropy_max_label{};

  double hurst_exponent_mean{0.0};
  double hurst_exponent_min{0.0};
  std::string hurst_exponent_min_label{};
  double hurst_exponent_max{0.0};
  std::string hurst_exponent_max_label{};

  std::vector<sequence_symbolic_stream_report_t> streams{};
};

struct sequence_symbolic_report_compaction_options_t {
  std::int64_t reduce_when_stream_count_exceeds{32};
  std::int64_t max_reported_streams{16};
};

struct data_symbolic_analytics_report_t {
  std::string schema{std::string(kDataAnalyticsSymbolicSchemaCurrent)};

  std::uint64_t channel_count{0};
  std::uint64_t eligible_channel_count{0};

  double lz76_normalized_mean{0.0};
  double lz76_normalized_min{0.0};
  std::string lz76_normalized_min_label{};
  double lz76_normalized_max{0.0};
  std::string lz76_normalized_max_label{};

  double information_density_mean{0.0};
  double information_density_min{0.0};
  std::string information_density_min_label{};
  double information_density_max{0.0};
  std::string information_density_max_label{};

  double compression_ratio_mean{0.0};
  double compression_ratio_min{0.0};
  std::string compression_ratio_min_label{};
  double compression_ratio_max{0.0};
  std::string compression_ratio_max_label{};

  double autocorrelation_decay_lag_mean{0.0};
  double autocorrelation_decay_lag_min{0.0};
  std::string autocorrelation_decay_lag_min_label{};
  double autocorrelation_decay_lag_max{0.0};
  std::string autocorrelation_decay_lag_max_label{};

  double power_spectrum_entropy_mean{0.0};
  double power_spectrum_entropy_min{0.0};
  std::string power_spectrum_entropy_min_label{};
  double power_spectrum_entropy_max{0.0};
  std::string power_spectrum_entropy_max_label{};

  double hurst_exponent_mean{0.0};
  double hurst_exponent_min{0.0};
  std::string hurst_exponent_min_label{};
  double hurst_exponent_max{0.0};
  std::string hurst_exponent_max_label{};

  std::vector<data_symbolic_channel_report_t> channels{};
};

class sequence_analytics_accumulator_t {
 public:
  explicit sequence_analytics_accumulator_t(
      data_analytics_options_t options = {});

  void reset();
  bool ingest(const torch::Tensor& features, const torch::Tensor& mask = {});

  [[nodiscard]] sequence_analytics_report_t summarize() const;
  [[nodiscard]] const data_analytics_options_t& options() const noexcept {
    return options_;
  }

 private:
  data_analytics_options_t options_{};
  std::vector<torch::Tensor> rows_{};
  std::vector<double> row_weights_{};
  std::uint64_t sample_count_{0};
  std::uint64_t skipped_sample_count_{0};
  std::int64_t sequence_channels_{0};
  std::int64_t sequence_timesteps_{0};
  std::int64_t sequence_features_per_timestep_{0};
  std::int64_t sequence_flat_feature_count_{0};
  std::int64_t sequence_effective_feature_count_{0};
};

class data_source_analytics_accumulator_t {
 public:
  explicit data_source_analytics_accumulator_t(
      data_analytics_options_t options = {});

  void reset();
  bool ingest(const torch::Tensor& features, const torch::Tensor& mask = {});

  [[nodiscard]] data_source_analytics_report_t summarize() const;
  [[nodiscard]] const data_analytics_options_t& options() const noexcept {
    return core_.options();
  }

 private:
  sequence_analytics_accumulator_t core_{};
};

class sequence_symbolic_analytics_accumulator_t {
 public:
  explicit sequence_symbolic_analytics_accumulator_t(
      std::vector<sequence_symbolic_stream_descriptor_t> stream_descriptors = {});

  void reset();
  bool ingest(const torch::Tensor& features, const torch::Tensor& mask = {});

  [[nodiscard]] sequence_symbolic_analytics_report_t summarize() const;
  [[nodiscard]] const std::vector<sequence_symbolic_stream_descriptor_t>&
  descriptors() const noexcept {
    return descriptors_;
  }

 private:
  std::vector<sequence_symbolic_stream_descriptor_t> descriptors_{};
  std::vector<std::vector<double>> stream_series_{};
};

class data_symbolic_analytics_accumulator_t {
 public:
  explicit data_symbolic_analytics_accumulator_t(
      std::vector<data_symbolic_channel_descriptor_t> channel_descriptors = {});

  void reset();
  bool ingest(const torch::Tensor& features, const torch::Tensor& mask = {});

  [[nodiscard]] data_symbolic_analytics_report_t summarize() const;
  [[nodiscard]] const std::vector<data_symbolic_channel_descriptor_t>&
  descriptors() const noexcept {
    return descriptors_;
  }

 private:
  std::vector<data_symbolic_channel_descriptor_t> descriptors_{};
  sequence_symbolic_analytics_accumulator_t core_{};
};

[[nodiscard]] const std::vector<std::string>& data_feature_names_for_record_type(
    std::string_view record_type);

[[nodiscard]] std::string_view data_symbolic_anchor_feature_name_for_record_type(
    std::string_view record_type);

[[nodiscard]] std::int64_t data_symbolic_anchor_feature_index_for_record_type(
    std::string_view record_type);

[[nodiscard]] sequence_analytics_report_t make_sequence_analytics_report(
    const data_source_analytics_report_t& report);

[[nodiscard]] data_source_analytics_report_t make_data_source_analytics_report(
    const sequence_analytics_report_t& report);

[[nodiscard]] sequence_symbolic_stream_descriptor_t
make_sequence_symbolic_stream_descriptor(
    const data_symbolic_channel_descriptor_t& descriptor);

[[nodiscard]] data_symbolic_channel_descriptor_t
make_data_symbolic_channel_descriptor(
    const sequence_symbolic_stream_descriptor_t& descriptor);

[[nodiscard]] sequence_symbolic_analytics_report_t
make_sequence_symbolic_analytics_report(
    const data_symbolic_analytics_report_t& report);

[[nodiscard]] sequence_symbolic_analytics_report_t
compact_sequence_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report,
    sequence_symbolic_report_compaction_options_t options = {});

[[nodiscard]] data_symbolic_analytics_report_t
make_data_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report);

[[nodiscard]] std::string sequence_analytics_to_latent_lineage_state_text(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view sequence_label = {},
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string data_analytics_to_latent_lineage_state_text(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view source_label = {},
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string sequence_symbolic_analytics_to_latent_lineage_state_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label = {},
    const tsiemene::component_report_identity_t& report_identity = {},
    sequence_symbolic_report_compaction_options_t compaction_options = {});

[[nodiscard]] std::string data_symbolic_analytics_to_latent_lineage_state_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label = {},
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string sequence_symbolic_analytics_to_pretty_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label = {},
    std::string_view output_filename = {},
    bool use_color = false,
    sequence_symbolic_report_compaction_options_t compaction_options = {});

[[nodiscard]] std::string data_symbolic_analytics_to_pretty_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label = {},
    std::string_view output_filename = {},
    bool use_color = false);

[[nodiscard]] bool write_sequence_analytics_file(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view sequence_label = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] bool write_data_analytics_file(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view source_label = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] bool write_sequence_symbolic_analytics_file(
    const sequence_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view sequence_label = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {},
    sequence_symbolic_report_compaction_options_t compaction_options = {});

[[nodiscard]] bool write_data_symbolic_analytics_file(
    const data_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view source_label = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string extract_sequence_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] std::string extract_data_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] std::string extract_sequence_symbolic_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] std::string extract_data_symbolic_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] bool is_supported_sequence_analytics_schema(
    std::string_view schema);

[[nodiscard]] bool is_supported_data_analytics_schema(
    std::string_view schema);

[[nodiscard]] bool is_supported_sequence_symbolic_analytics_schema(
    std::string_view schema);

[[nodiscard]] bool is_supported_data_symbolic_analytics_schema(
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

[[nodiscard]] std::filesystem::path
source_data_analytics_symbolic_latest_file_path(
    std::string_view contract_hash,
    std::string_view source_instance);

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
