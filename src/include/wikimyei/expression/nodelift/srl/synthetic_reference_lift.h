// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "ujcamei/graph/graph.h"
#include "ujcamei/source/types/kline_feature_registry.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl {

inline constexpr const char *kCanonicalToken =
    "wikimyei.expression.nodelift.srl.v1";
inline constexpr int64_t kFeatureWidth =
    cuwacunu::ujcamei::source::types::kKlineFeatureWidth;
inline constexpr int64_t kPriceWidth =
    cuwacunu::ujcamei::source::types::kKlinePriceFeatureWidth;
inline constexpr int64_t kActivityWidth =
    cuwacunu::ujcamei::source::types::kKlineActivityFeatureWidth;
inline constexpr std::array<int64_t, kPriceWidth> kDefaultPriceCoords =
    cuwacunu::ujcamei::source::types::kKlinePriceFeatureCoords;
inline constexpr std::array<int64_t, kActivityWidth> kDefaultActivityCoords =
    cuwacunu::ujcamei::source::types::kKlineActivityFeatureCoords;

struct graph_t {
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  torch::Tensor base_index{};  // [L], int64
  torch::Tensor quote_index{}; // [L], int64
  bool allow_duplicate_edge_ids{false};
  std::string graph_order_fingerprint{};

  [[nodiscard]] int64_t num_nodes() const;
  [[nodiscard]] int64_t num_edges() const;
  [[nodiscard]] std::string computed_graph_order_fingerprint() const {
    auto base_cpu = base_index.to(torch::kCPU).to(torch::kInt64).contiguous();
    auto quote_cpu = quote_index.to(torch::kCPU).to(torch::kInt64).contiguous();
    std::vector<cuwacunu::ujcamei::graph::node_index_t> base;
    std::vector<cuwacunu::ujcamei::graph::node_index_t> quote;
    base.reserve(static_cast<std::size_t>(base_cpu.numel()));
    quote.reserve(static_cast<std::size_t>(quote_cpu.numel()));
    const auto *base_ptr = base_cpu.data_ptr<int64_t>();
    const auto *quote_ptr = quote_cpu.data_ptr<int64_t>();
    for (int64_t e = 0; e < base_cpu.numel(); ++e) {
      base.push_back(base_ptr[e]);
      quote.push_back(quote_ptr[e]);
    }
    return cuwacunu::ujcamei::graph::compute_graph_order_fingerprint(
        node_ids, edge_ids, base, quote);
  }
  void validate() const;
};

struct nodelift_input_t {
  torch::Tensor edge_features{}; // [B,L,C,Hx,9]
  torch::Tensor edge_mask{};     // [B,L,C,Hx]
  std::optional<torch::Tensor>
      edge_coord_mask{}; // [B,L,C,Hx,9], normalized to bool
};

enum class gauge_policy_t {
  uniform_per_component,
};

enum class precision_policy_t {
  identity,
};

enum class activity_mode_t {
  support_mean,
};

struct nodelift_options_t {
  std::array<int64_t, kPriceWidth> price_coords = kDefaultPriceCoords;
  std::array<int64_t, kActivityWidth> activity_coords = kDefaultActivityCoords;
  gauge_policy_t gauge_policy{gauge_policy_t::uniform_per_component};
  precision_policy_t precision_policy{precision_policy_t::identity};
  activity_mode_t activity_mode{activity_mode_t::support_mean};
  bool return_activity_total{true};
  bool return_activity_support{true};
  bool return_activity_coverage{true};
  bool return_coarse_masks{true};
  double eps{1e-12};
  double activity_max_exp_arg{40.0};
  std::optional<torch::Device> output_device{};
  std::optional<c10::ScalarType> output_dtype{};
};

struct nodelift_diagnostics_t {
  torch::Tensor valid_edge_count{};            // [B,C,Hx,9], int64
  torch::Tensor component_count{};             // [B,C,Hx,4], int64
  torch::Tensor recoverable_component_count{}; // [B,C,Hx,4], int64
  torch::Tensor cycle_dimension{};             // [B,C,Hx,4], int64
  torch::Tensor residual_energy{};             // [B,C,Hx,4]
  torch::Tensor failed_solve_count{};          // [B,C,Hx,4], int64
};

struct nodelift_output_t {
  torch::Tensor node_features{};                    // [B,C,Hx,N,9]
  torch::Tensor node_mask{};                        // [B,C,Hx,N,9], bool
  std::optional<torch::Tensor> node_mask_any{};     // [B,C,Hx,N], bool
  std::optional<torch::Tensor> node_mask_all{};     // [B,C,Hx,N], bool
  torch::Tensor price_residual{};                   // [B,C,Hx,L,4]
  torch::Tensor price_residual_mask{};              // [B,C,Hx,L,4], bool
  std::optional<torch::Tensor> activity_total{};    // [B,C,Hx,N,5]
  std::optional<torch::Tensor> activity_support{};  // [B,C,Hx,N,5], int64
  std::optional<torch::Tensor> activity_coverage{}; // [B,C,Hx,N,5]
  nodelift_diagnostics_t diagnostics{};
};

[[nodiscard]] nodelift_output_t
featurewise_node_lift(const graph_t &graph, const nodelift_input_t &input,
                      const nodelift_options_t &options = {});

} // namespace cuwacunu::wikimyei::expression::nodelift::srl
