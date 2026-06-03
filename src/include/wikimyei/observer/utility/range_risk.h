// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/utility/channel_consensus.h"
#include "wikimyei/observer/utility/feature_semantics.h"
#include "wikimyei/observer/utility/nodelift_potential_surface.h"

namespace cuwacunu::wikimyei::observer {

struct range_risk_options_t {
  std::int64_t high_coord{kline_coord(registry::kline_feature_e::high)};
  std::int64_t low_coord{kline_coord(registry::kline_feature_e::low)};
  double adverse_log_return_threshold{-0.02};
  double upside_log_return_threshold{0.02};
  affine_target_transform_t high_transform{};
  affine_target_transform_t low_transform{};
};

struct range_risk_t {
  torch::Tensor adverse_excursion_prob{}; // [B,A]
  torch::Tensor upside_excursion_prob{};  // [B,A]
};

[[nodiscard]] inline torch::Tensor
range_risk_standard_normal_cdf(const torch::Tensor &x) {
  return 0.5 * (1.0 + torch::erf(x / std::sqrt(2.0)));
}

[[nodiscard]] inline torch::Tensor
mixture_less_equal_probability(const torch::Tensor &log_weight,
                               const torch::Tensor &mu,
                               const torch::Tensor &sigma, double threshold) {
  auto z = (mu.new_full({}, threshold) - mu) / sigma.clamp_min(1.0e-12);
  return (log_weight.exp() * range_risk_standard_normal_cdf(z)).sum(/*dim=*/-1);
}

[[nodiscard]] inline range_risk_t
compute_range_risk(const channel_consensus_t &consensus,
                   const std::vector<std::int64_t> &asset_graph_indices,
                   std::int64_t projection_reference_graph_index,
                   const range_risk_options_t &options = {}) {
  TORCH_CHECK(consensus.log_weight.defined() && consensus.mu.defined() &&
                  consensus.sigma.defined(),
              "[range_risk] consensus mixture tensors must be defined");
  TORCH_CHECK(!asset_graph_indices.empty(),
              "[range_risk] asset_graph_indices must not be empty");
  TORCH_CHECK(projection_reference_graph_index >= 0 &&
                  projection_reference_graph_index <
                      consensus.log_weight.size(1),
              "[range_risk] projection_reference_graph_index out of range");
  TORCH_CHECK(options.high_coord >= 0 &&
                  options.high_coord < consensus.log_weight.size(2),
              "[range_risk] high_coord out of range");
  TORCH_CHECK(options.low_coord >= 0 &&
                  options.low_coord < consensus.log_weight.size(2),
              "[range_risk] low_coord out of range");

  auto index = torch::tensor(asset_graph_indices,
                             torch::TensorOptions()
                                 .dtype(torch::kInt64)
                                 .device(consensus.log_weight.device()));

  auto low_log_weight =
      consensus.log_weight.select(2, options.low_coord).index_select(1, index);
  auto low_mu =
      consensus.mu.select(2, options.low_coord).index_select(1, index);
  auto low_sigma =
      consensus.sigma.select(2, options.low_coord).index_select(1, index);
  auto low_projection_reference_log_weight =
      consensus.log_weight.select(2, options.low_coord)
          .select(1, projection_reference_graph_index);
  auto low_projection_reference_mu =
      consensus.mu.select(2, options.low_coord)
          .select(1, projection_reference_graph_index);
  auto low_projection_reference_sigma =
      consensus.sigma.select(2, options.low_coord)
          .select(1, projection_reference_graph_index);
  if (options.low_transform.enabled) {
    low_mu = low_mu * options.low_transform.scale + options.low_transform.loc;
    low_sigma = low_sigma * std::abs(options.low_transform.scale);
    low_projection_reference_mu =
        low_projection_reference_mu * options.low_transform.scale +
        options.low_transform.loc;
    low_projection_reference_sigma =
        low_projection_reference_sigma * std::abs(options.low_transform.scale);
  }

  auto high_log_weight =
      consensus.log_weight.select(2, options.high_coord).index_select(1, index);
  auto high_mu =
      consensus.mu.select(2, options.high_coord).index_select(1, index);
  auto high_sigma =
      consensus.sigma.select(2, options.high_coord).index_select(1, index);
  auto high_projection_reference_log_weight =
      consensus.log_weight.select(2, options.high_coord)
          .select(1, projection_reference_graph_index);
  auto high_projection_reference_mu =
      consensus.mu.select(2, options.high_coord)
          .select(1, projection_reference_graph_index);
  auto high_projection_reference_sigma =
      consensus.sigma.select(2, options.high_coord)
          .select(1, projection_reference_graph_index);
  if (options.high_transform.enabled) {
    high_mu =
        high_mu * options.high_transform.scale + options.high_transform.loc;
    high_sigma = high_sigma * std::abs(options.high_transform.scale);
    high_projection_reference_mu =
        high_projection_reference_mu * options.high_transform.scale +
        options.high_transform.loc;
    high_projection_reference_sigma = high_projection_reference_sigma *
                                      std::abs(options.high_transform.scale);
  }

  const auto K = low_log_weight.size(-1);
  auto low_diff_weight =
      (low_log_weight.unsqueeze(-1) +
       low_projection_reference_log_weight.unsqueeze(1).unsqueeze(1))
          .reshape({low_log_weight.size(0), low_log_weight.size(1), K * K});
  low_diff_weight =
      low_diff_weight - torch::logsumexp(low_diff_weight, /*dim=*/-1, true);
  auto low_diff_mu = (low_mu.unsqueeze(-1) -
                      low_projection_reference_mu.unsqueeze(1).unsqueeze(1))
                         .reshape({low_mu.size(0), low_mu.size(1), K * K});
  auto low_diff_sigma =
      (low_sigma.pow(2).unsqueeze(-1) +
       low_projection_reference_sigma.pow(2).unsqueeze(1).unsqueeze(1))
          .sqrt()
          .reshape({low_sigma.size(0), low_sigma.size(1), K * K});

  auto high_diff_weight =
      (high_log_weight.unsqueeze(-1) +
       high_projection_reference_log_weight.unsqueeze(1).unsqueeze(1))
          .reshape({high_log_weight.size(0), high_log_weight.size(1), K * K});
  high_diff_weight =
      high_diff_weight - torch::logsumexp(high_diff_weight, /*dim=*/-1, true);
  auto high_diff_mu = (high_mu.unsqueeze(-1) -
                       high_projection_reference_mu.unsqueeze(1).unsqueeze(1))
                          .reshape({high_mu.size(0), high_mu.size(1), K * K});
  auto high_diff_sigma =
      (high_sigma.pow(2).unsqueeze(-1) +
       high_projection_reference_sigma.pow(2).unsqueeze(1).unsqueeze(1))
          .sqrt()
          .reshape({high_sigma.size(0), high_sigma.size(1), K * K});

  auto adverse = mixture_less_equal_probability(
      low_diff_weight, low_diff_mu, low_diff_sigma,
      options.adverse_log_return_threshold);
  auto high_le = mixture_less_equal_probability(
      high_diff_weight, high_diff_mu, high_diff_sigma,
      options.upside_log_return_threshold);
  auto upside = 1.0 - high_le;

  if (consensus.active_mask.defined()) {
    auto low_active = consensus.active_mask.select(2, options.low_coord)
                          .index_select(1, index);
    auto high_active = consensus.active_mask.select(2, options.high_coord)
                           .index_select(1, index);
    auto low_projection_reference_active =
        consensus.active_mask.select(2, options.low_coord)
            .select(1, projection_reference_graph_index)
            .unsqueeze(1)
            .expand_as(low_active);
    auto high_projection_reference_active =
        consensus.active_mask.select(2, options.high_coord)
            .select(1, projection_reference_graph_index)
            .unsqueeze(1)
            .expand_as(high_active);
    low_active = low_active.logical_and(low_projection_reference_active);
    high_active = high_active.logical_and(high_projection_reference_active);
    adverse = adverse.masked_fill(low_active.logical_not(), 0.0);
    upside = upside.masked_fill(high_active.logical_not(), 0.0);
  }

  range_risk_t result{};
  result.adverse_excursion_prob = std::move(adverse);
  result.upside_excursion_prob = std::move(upside);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
