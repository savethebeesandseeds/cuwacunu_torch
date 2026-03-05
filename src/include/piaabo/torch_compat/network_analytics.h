/* network_analytics.h */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

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

struct network_analytics_options_t {
  double near_zero_epsilon{1e-8};
  std::int64_t log10_abs_histogram_bins{72};
  double log10_abs_histogram_min{-12.0};
  double log10_abs_histogram_max{6.0};
};

struct network_analytics_report_t {
  std::string schema{"piaabo.torch_compat.network_analytics.v1"};

  std::uint64_t tensor_count{0};
  std::uint64_t trainable_tensor_count{0};
  std::uint64_t total_parameter_count{0};
  std::uint64_t finite_parameter_count{0};
  std::uint64_t nan_parameter_count{0};
  std::uint64_t inf_parameter_count{0};

  double finite_ratio{0.0};
  double non_finite_ratio{0.0};

  double mean{0.0};
  double stddev{0.0};
  double min{0.0};
  double max{0.0};

  double l1_mean_abs{0.0};
  double l2_rms{0.0};
  double max_abs{0.0};
  double max_abs_over_rms{0.0};

  double near_zero_threshold{0.0};
  double near_zero_ratio{0.0};
  double positive_ratio{0.0};
  double negative_ratio{0.0};
  double exact_zero_ratio{0.0};

  double abs_energy_entropy{0.0};
  double log10_abs_histogram_entropy{0.0};

  double layer_rms_mean{0.0};
  double layer_rms_std{0.0};
  double layer_rms_cv{0.0};
  double layer_rms_min{0.0};
  double layer_rms_max{0.0};
  double layer_rms_max_over_min{0.0};

  std::string max_abs_tensor_name{};
  double max_abs_tensor_value{0.0};
};

struct network_design_analytics_report_t {
  std::string schema{"piaabo.torch_compat.network_design_analytics.v1"};

  std::string network_id{};
  std::string join_policy{};

  std::uint64_t node_count{0};
  std::uint64_t export_count{0};
  std::uint64_t parameter_count{0};

  std::uint64_t internal_edge_count{0};
  std::uint64_t export_edge_count{0};
  std::uint64_t total_edge_count{0};

  std::uint64_t weakly_connected_components{0};
  std::uint64_t isolated_node_count{0};
  bool has_internal_cycle{false};
  std::uint64_t topological_order_coverage{0};
  std::uint64_t acyclic_longest_path_nodes{0};

  double internal_edge_density{0.0};
  double cyclomatic_complexity{0.0};
  double mean_in_degree{0.0};
  double mean_out_degree{0.0};
  std::uint64_t max_in_degree{0};
  std::uint64_t max_out_degree{0};
  double branching_factor{0.0};

  std::uint64_t distinct_node_kind_count{0};
  double node_kind_entropy{0.0};
  double in_degree_entropy{0.0};
  double out_degree_entropy{0.0};
  double edge_kind_transition_entropy{0.0};
  double parameter_key_entropy{0.0};
  double parameter_value_token_entropy{0.0};
};

[[nodiscard]] network_analytics_report_t summarize_module_network_analytics(
    const torch::nn::Module& model,
    const network_analytics_options_t& options = {});

[[nodiscard]] network_design_analytics_report_t summarize_network_design_analytics(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction);

[[nodiscard]] std::string network_analytics_to_key_value_text(
    const network_analytics_report_t& report,
    const network_analytics_options_t& options,
    std::string_view checkpoint_filename = {});

[[nodiscard]] std::string network_design_analytics_to_key_value_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label = {});

[[nodiscard]] std::string network_design_analytics_to_pretty_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label = {},
    bool use_color = true);

[[nodiscard]] bool write_network_analytics_file(
    const torch::nn::Module& model,
    const std::filesystem::path& output_file,
    const network_analytics_options_t& options = {},
    std::string* error = nullptr);

[[nodiscard]] bool write_network_analytics_sidecar_for_checkpoint(
    const torch::nn::Module& model,
    const std::filesystem::path& checkpoint_file,
    std::filesystem::path* out_sidecar_file = nullptr,
    const network_analytics_options_t& options = {},
    std::string* error = nullptr);

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
