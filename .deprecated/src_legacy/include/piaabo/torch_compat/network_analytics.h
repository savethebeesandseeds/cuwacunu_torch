/* network_analytics.h */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "piaabo/dlogs.h"
#include "tsiemene/tsi.report.h"

namespace torch {
namespace nn {
class Module;
}  // namespace nn
}  // namespace torch

namespace cuwacunu {
namespace camahjucunu {
struct network_design_instruction_t;
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

inline constexpr std::string_view kNetworkAnalyticsSchemaV5 =
    "piaabo.torch_compat.network_analytics.v5";
inline constexpr std::string_view kNetworkAnalyticsSchemaCurrent =
    kNetworkAnalyticsSchemaV5;
inline constexpr std::string_view kNetworkDesignAnalyticsSchemaV5 =
    "piaabo.torch_compat.network_design_analytics.v5";
inline constexpr std::string_view kNetworkDesignAnalyticsSchemaCurrent =
    kNetworkDesignAnalyticsSchemaV5;

DEV_WARNING("network_analytics: pending improvements include node<->module crosswalk. \n");
DEV_WARNING("explicit DSL edges, bottleneck metrics (articulation/bridge/shared-trunk). \n");
DEV_WARNING("capacity concentration metrics (param/norm HHI), and expanded matrix diagnostics.\n");

struct network_analytics_options_t {
  double near_zero_epsilon{1e-8};
  std::int64_t log10_abs_histogram_bins{72};
  double log10_abs_histogram_min{-12.0};
  double log10_abs_histogram_max{6.0};
  bool include_buffers{true};
  bool enable_spectral_metrics{true};
  std::int64_t spectral_max_elements{1048576};
  std::int64_t anomaly_top_k{5};
};

struct analytics_topk_entry_t {
  std::string tensor_name{};
  double value{0.0};
};

struct network_analytics_report_t {
  std::string schema{std::string(kNetworkAnalyticsSchemaCurrent)};
  network_analytics_options_t normalized_options{};

  std::uint64_t tensor_count{0};
  std::uint64_t trainable_tensor_count{0};
  std::uint64_t total_parameter_count{0};
  std::uint64_t finite_parameter_count{0};
  std::uint64_t nan_parameter_count{0};
  std::uint64_t inf_parameter_count{0};

  double finite_ratio{0.0};
  double non_finite_ratio{0.0};
  double stddev{0.0};
  double min{0.0};
  double max{0.0};

  double l1_mean_abs{0.0};
  double l2_rms{0.0};
  double max_abs{0.0};
  double max_abs_over_rms{0.0};

  double near_zero_threshold{0.0};
  double near_zero_ratio{0.0};
  double exact_zero_ratio{0.0};

  double abs_energy_entropy{0.0};
  double log10_abs_histogram_entropy{0.0};

  double tensor_rms_mean{0.0};
  double tensor_rms_std{0.0};
  double tensor_rms_cv{0.0};
  double tensor_rms_min{0.0};
  double tensor_rms_max{0.0};
  double tensor_rms_max_over_min{0.0};

  double abs_p50{0.0};
  double abs_p90{0.0};
  double abs_p99{0.0};
  double log10_abs_p50{0.0};
  double log10_abs_iqr{0.0};

  std::string max_abs_tensor_name{};
  double max_abs_tensor_value{0.0};

  std::uint64_t buffer_tensor_count{0};
  std::uint64_t total_buffer_count{0};
  std::uint64_t finite_buffer_count{0};
  std::uint64_t nan_buffer_count{0};
  std::uint64_t inf_buffer_count{0};
  double finite_buffer_ratio{0.0};
  std::string max_abs_buffer_name{};
  double max_abs_buffer_value{0.0};

  std::uint64_t matrix_tensor_count{0};
  std::uint64_t spectral_tensor_count{0};
  std::uint64_t spectral_skipped_tensor_count{0};
  std::uint64_t spectral_failed_tensor_count{0};
  double spectral_norm_mean{0.0};
  double spectral_norm_max{0.0};
  double stable_rank_mean{0.0};
  double stable_rank_min{0.0};
  double effective_rank_mean{0.0};
  double effective_rank_min{0.0};
  double row_norm_cv_mean{0.0};
  double col_norm_cv_mean{0.0};
  double network_global_entropic_capacity{0.0};
  double network_entropic_bottleneck_min{0.0};
  double network_effective_rank_p50{0.0};
  double network_effective_rank_p90{0.0};
  std::uint64_t network_capacity_tensor_count{0};

  std::vector<analytics_topk_entry_t> top_nonfinite_ratio_tensors{};
  std::vector<analytics_topk_entry_t> top_max_abs_over_rms_tensors{};
  std::vector<analytics_topk_entry_t> top_near_zero_ratio_tensors{};
  std::vector<analytics_topk_entry_t> top_low_stable_rank_tensors{};
  std::vector<analytics_topk_entry_t> top_low_effective_rank_tensors{};
  std::vector<analytics_topk_entry_t> top_high_spectral_norm_tensors{};
};

struct network_design_analytics_report_t {
  std::string schema{std::string(kNetworkDesignAnalyticsSchemaCurrent)};
  bool analysis_valid{true};
  std::string analysis_error{};
  std::uint64_t duplicate_node_id_count{0};
  std::string duplicate_node_id_example{};

  std::string network_id{};
  std::string assembly_tag{};

  std::uint64_t node_count{0};
  std::uint64_t export_count{0};
  std::uint64_t parameter_count{0};

  std::uint64_t internal_edge_count{0};
  std::uint64_t export_edge_count{0};
  std::uint64_t total_edge_count{0};

  std::uint64_t weakly_connected_components{0};
  std::uint64_t isolated_node_count{0};
  bool has_internal_cycle{false};
  std::uint64_t acyclic_longest_path_nodes{0};
  std::uint64_t topological_order_count{0};
  double topological_order_ratio{0.0};

  double internal_edge_density{0.0};
  double edge_surplus{0.0};
  double mean_in_degree{0.0};
  double mean_out_degree{0.0};
  std::uint64_t max_in_degree{0};
  std::uint64_t max_out_degree{0};
  double active_fanout_mean{0.0};

  std::uint64_t source_count{0};
  std::uint64_t internal_sink_count{0};
  std::uint64_t export_reachable_node_count{0};
  double export_reachable_ratio{0.0};
  std::uint64_t dead_end_node_count{0};
  std::uint64_t orphan_node_count{0};
  std::uint64_t longest_source_to_export_path_nodes{0};
  std::uint64_t median_source_to_export_path_nodes{0};
  std::uint64_t branch_node_count{0};
  std::uint64_t merge_node_count{0};

  std::uint64_t scc_count{0};
  std::uint64_t largest_scc_size{0};
  std::uint64_t cyclic_node_count{0};
  double cyclic_node_ratio{0.0};

  std::uint64_t skip_edge_count{0};
  double skip_edge_ratio{0.0};
  double mean_skip_span{0.0};
  std::uint64_t max_skip_span{0};

  std::uint64_t explicit_edge_count{0};
  std::uint64_t inferred_edge_count{0};
  std::uint64_t unresolved_identifier_token_count{0};
  std::uint64_t self_reference_token_count{0};

  std::uint64_t distinct_node_kind_count{0};
  double node_kind_entropy{0.0};
  double in_degree_entropy{0.0};
  double out_degree_entropy{0.0};
  double edge_kind_transition_entropy{0.0};
};

[[nodiscard]] network_analytics_report_t summarize_module_network_analytics(
    const torch::nn::Module& model,
    const network_analytics_options_t& options = {});

[[nodiscard]] network_design_analytics_report_t summarize_network_design_analytics(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction);

[[nodiscard]] bool resolve_network_analytics_options_from_network_design(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction,
    network_analytics_options_t* out_options,
    std::string* error = nullptr);

[[nodiscard]] std::string network_analytics_to_latent_lineage_state_text(
    const network_analytics_report_t& report,
    std::string_view checkpoint_filename = {},
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] std::string network_analytics_to_pretty_text(
    const network_analytics_report_t& report,
    std::string_view network_label = {},
    bool use_color = true);

[[nodiscard]] std::string network_design_analytics_to_latent_lineage_state_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label = {});

[[nodiscard]] std::string network_design_analytics_to_pretty_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label = {},
    bool use_color = true);

[[nodiscard]] std::string extract_analytics_kv_schema(
    std::string_view payload);

[[nodiscard]] bool is_supported_network_analytics_schema(
    std::string_view schema);

[[nodiscard]] bool is_supported_network_design_analytics_schema(
    std::string_view schema);

[[nodiscard]] bool write_network_analytics_file(
    const torch::nn::Module& model,
    const std::filesystem::path& output_file,
    const network_analytics_options_t& options = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

[[nodiscard]] bool write_network_analytics_sidecar_for_checkpoint(
    const torch::nn::Module& model,
    const std::filesystem::path& checkpoint_file,
    std::filesystem::path* out_sidecar_file = nullptr,
    const network_analytics_options_t& options = {},
    std::string* error = nullptr,
    const tsiemene::component_report_identity_t& report_identity = {});

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
