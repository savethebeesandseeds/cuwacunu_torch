// SPDX-License-Identifier: MIT
#pragma once

#include <utility>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct transaction_cost_t {
  torch::Tensor per_asset_cost{}; // [A]
  double total_cost{0.0};
};

[[nodiscard]] inline transaction_cost_t compute_transaction_cost(
    const torch::Tensor &delta_weights, const torch::Tensor &linear_cost,
    const torch::Tensor &quadratic_impact = torch::Tensor()) {
  TORCH_CHECK(delta_weights.defined() && delta_weights.dim() == 1,
              "[transaction_cost] delta_weights must be [A]");
  TORCH_CHECK(linear_cost.defined() &&
                  linear_cost.sizes() == delta_weights.sizes(),
              "[transaction_cost] linear_cost must be [A]");
  auto delta = delta_weights.to(torch::kFloat64);
  auto cost = linear_cost.to(torch::kFloat64).clamp_min(0.0) * delta.abs();
  if (quadratic_impact.defined()) {
    TORCH_CHECK(quadratic_impact.sizes() == delta_weights.sizes(),
                "[transaction_cost] quadratic_impact must be [A]");
    cost = cost +
           quadratic_impact.to(torch::kFloat64).clamp_min(0.0) * delta.pow(2);
  }

  transaction_cost_t result{};
  result.per_asset_cost = std::move(cost);
  result.total_cost = result.per_asset_cost.sum().item<double>();
  return result;
}

} // namespace cuwacunu::wikimyei::observer
