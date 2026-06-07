// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/utility/covariance_coupler.h"
#include "wikimyei/observer/utility/nodelift_potential_surface.h"

namespace cuwacunu::wikimyei::observer {

struct nodelift_return_projection_options_t {
  covariance_coupler_options_t coupling_options{};
};

struct nodelift_return_projection_t {
  torch::Tensor projected_log_weight{};       // [A,Kp]
  torch::Tensor projected_log_return_mu{};    // [A,Kp]
  torch::Tensor projected_log_return_sigma{}; // [A,Kp]
  torch::Tensor active_mask{};                // [A], bool

  torch::Tensor potential_samples{};           // [S,M], target nodes plus reference if absent
  torch::Tensor numeraire_relative_log_return{};    // [S,A]
  torch::Tensor arithmetic_return_scenarios{}; // [S,A]

  torch::Tensor expected_log_return{};        // [A]
  torch::Tensor expected_arithmetic_return{}; // [A]
  torch::Tensor marginal_variance{};          // [A]
  torch::Tensor marginal_volatility{};        // [A]
  torch::Tensor covariance{};                 // [A,A]
  torch::Tensor correlation{};                // [A,A]
};

namespace detail {

[[nodiscard]] inline torch::Tensor
index_tensor(const std::vector<std::int64_t> &indices,
             const torch::Device &device, const char *name) {
  TORCH_CHECK(!indices.empty(), "[nodelift_return_projection] ", name,
              " must not be empty");
  return torch::tensor(
      indices, torch::TensorOptions().dtype(torch::kInt64).device(device));
}

[[nodiscard]] inline torch::Tensor
variance_along_rows(const torch::Tensor &samples) {
  const auto S = samples.size(0);
  auto centered = samples - samples.mean(/*dim=*/0, /*keepdim=*/true);
  const double denom = static_cast<double>(std::max<std::int64_t>(S - 1, 1));
  return centered.pow(2).sum(/*dim=*/0) / denom;
}

} // namespace detail

[[nodiscard]] inline nodelift_return_projection_t project_single_anchor(
    const nodelift_potential_surface_t &surface, std::int64_t anchor_slot,
    const std::vector<std::int64_t> &asset_graph_indices,
    std::int64_t projection_reference_graph_index,
    const torch::Tensor &empirical_correlation,
    const nodelift_return_projection_options_t &options = {}) {
  TORCH_CHECK(
      surface.log_weight.defined() && surface.potential_mu.defined() &&
          surface.potential_sigma.defined(),
      "[nodelift_return_projection] potential surface tensors required");
  TORCH_CHECK(
      surface.log_weight.dim() == 3,
      "[nodelift_return_projection] potential surface must be [B,G,Kc]");
  TORCH_CHECK(anchor_slot >= 0 && anchor_slot < surface.log_weight.size(0),
              "[nodelift_return_projection] anchor_slot out of range");
  TORCH_CHECK(projection_reference_graph_index >= 0 &&
                  projection_reference_graph_index < surface.log_weight.size(1),
              "[nodelift_return_projection] projection_reference_graph_index "
              "out of range");

  std::vector<std::int64_t> projection_indices = asset_graph_indices;
  auto reference_position_it =
      std::find(projection_indices.begin(), projection_indices.end(),
                projection_reference_graph_index);
  if (reference_position_it == projection_indices.end()) {
    projection_indices.push_back(projection_reference_graph_index);
    reference_position_it = std::prev(projection_indices.end());
  }
  const auto reference_projection_position = static_cast<std::int64_t>(
      reference_position_it - projection_indices.begin());
  const auto A = static_cast<std::int64_t>(asset_graph_indices.size());
  const auto M = static_cast<std::int64_t>(projection_indices.size());
  TORCH_CHECK(
      M == A || M == A + 1,
      "[nodelift_return_projection] projection universe shape mismatch");
  TORCH_CHECK(empirical_correlation.defined() &&
                  empirical_correlation.dim() == 2 &&
                  empirical_correlation.size(0) == M &&
                  empirical_correlation.size(1) == M,
              "[nodelift_return_projection] empirical_correlation must be "
              "[M,M] ordered as target nodes plus projection reference when "
              "absent from target nodes");

  auto asset_index = detail::index_tensor(
      asset_graph_indices, surface.log_weight.device(), "asset_graph_indices");
  auto anchor_log_weight = surface.log_weight.select(/*dim=*/0, anchor_slot);
  auto anchor_mu = surface.potential_mu.select(/*dim=*/0, anchor_slot);
  auto anchor_sigma = surface.potential_sigma.select(/*dim=*/0, anchor_slot);

  auto asset_log_weight = anchor_log_weight.index_select(0, asset_index);
  auto asset_mu = anchor_mu.index_select(0, asset_index);
  auto asset_sigma = anchor_sigma.index_select(0, asset_index);
  auto projection_reference_log_weight =
      anchor_log_weight.select(/*dim=*/0, projection_reference_graph_index);
  auto projection_reference_mu =
      anchor_mu.select(/*dim=*/0, projection_reference_graph_index);
  auto projection_reference_sigma =
      anchor_sigma.select(/*dim=*/0, projection_reference_graph_index);

  const auto K = asset_log_weight.size(1);
  auto log_weight = asset_log_weight.unsqueeze(2) +
                    projection_reference_log_weight.view({1, 1, K});
  log_weight = log_weight.reshape({A, K * K});
  log_weight = log_weight - torch::logsumexp(log_weight, /*dim=*/1, true);
  auto mu = (asset_mu.unsqueeze(2) - projection_reference_mu.view({1, 1, K}))
                .reshape({A, K * K});
  auto sigma = (asset_sigma.pow(2).unsqueeze(2) +
                projection_reference_sigma.pow(2).view({1, 1, K}))
                   .clamp_min(0.0)
                   .sqrt()
                   .reshape({A, K * K});

  auto active =
      surface.active_mask.defined()
          ? surface.active_mask.select(/*dim=*/0, anchor_slot)
                .index_select(0, asset_index)
          : torch::ones({A}, torch::TensorOptions()
                                 .dtype(torch::kBool)
                                 .device(surface.log_weight.device()));
  if (surface.active_mask.defined()) {
    auto projection_reference_active =
        surface.active_mask.select(/*dim=*/0, anchor_slot)
            .select(/*dim=*/0, projection_reference_graph_index)
            .to(torch::TensorOptions()
                    .dtype(torch::kBool)
                    .device(surface.log_weight.device()));
    active = active.logical_and(projection_reference_active.expand_as(active));
  }
  auto inactive = active.logical_not().unsqueeze(1);
  log_weight =
      log_weight.masked_fill(inactive, -std::log(static_cast<double>(K * K)));
  mu = mu.masked_fill(inactive, 0.0);
  sigma = sigma.masked_fill(inactive, 0.0);

  auto coupled = sample_single_anchor_marginals(
      surface.log_weight, surface.potential_mu, surface.potential_sigma,
      anchor_slot, projection_indices, empirical_correlation,
      options.coupling_options);
  auto asset_potential = coupled.samples.narrow(/*dim=*/1, 0, A);
  auto projection_reference_potential =
      coupled.samples.select(/*dim=*/1, reference_projection_position)
          .unsqueeze(1);
  auto log_return = asset_potential - projection_reference_potential;
  auto scenarios = torch::exp(log_return) - 1.0;
  scenarios = scenarios.masked_fill(active.logical_not().unsqueeze(0), 0.0);
  log_return = log_return.masked_fill(active.logical_not().unsqueeze(0), 0.0);

  for (std::int64_t a = 0; a < A; ++a) {
    if (asset_graph_indices[static_cast<std::size_t>(a)] !=
        projection_reference_graph_index) {
      continue;
    }
    using torch::indexing::Slice;
    log_weight.index_put_({a, Slice()},
                          -std::log(static_cast<double>(K * K)));
    mu.index_put_({a, Slice()}, 0.0);
    sigma.index_put_({a, Slice()}, 0.0);
    log_return.index_put_({Slice(), a}, 0.0);
    scenarios.index_put_({Slice(), a}, 0.0);
    active.index_put_({a}, true);
  }

  auto expected_log = log_return.mean(/*dim=*/0);
  auto expected_arithmetic = scenarios.mean(/*dim=*/0);
  auto variance = detail::variance_along_rows(scenarios).clamp_min(0.0);
  auto covariance = scenario_covariance(scenarios);

  nodelift_return_projection_t result{};
  result.projected_log_weight = std::move(log_weight);
  result.projected_log_return_mu = std::move(mu);
  result.projected_log_return_sigma = std::move(sigma);
  result.active_mask = std::move(active);
  result.potential_samples = std::move(coupled.samples);
  result.numeraire_relative_log_return = std::move(log_return);
  result.arithmetic_return_scenarios = std::move(scenarios);
  result.expected_log_return = std::move(expected_log);
  result.expected_arithmetic_return = std::move(expected_arithmetic);
  result.marginal_variance = std::move(variance);
  result.marginal_volatility = result.marginal_variance.sqrt();
  result.covariance = std::move(covariance);
  result.correlation = covariance_to_correlation(result.covariance);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
