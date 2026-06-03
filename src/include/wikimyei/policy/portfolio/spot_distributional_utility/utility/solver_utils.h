// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/data_quality.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    solver_utils {

namespace belief_ns = cuwacunu::wikimyei::observer::belief;

struct solve_context_t {
  std::int64_t A{0};
  torch::Device device{torch::kCPU};
  torch::TensorOptions tensor_options{
      torch::TensorOptions().dtype(torch::kFloat64)};

  torch::Tensor current{};          // [A]
  torch::Tensor min_weight{};       // [A]
  torch::Tensor max_weight{};       // [A]
  torch::Tensor confidence{};       // [A]
  torch::Tensor active_mask{};      // [A], bool
  torch::Tensor linear_cost{};      // [A]
  torch::Tensor quadratic_impact{}; // [A]

  double risky_budget{1.0};
};

[[nodiscard]] inline torch::Tensor
project_weights(const torch::Tensor &candidate, const torch::Tensor &current,
                const torch::Tensor &min_weight,
                const torch::Tensor &max_weight, double risky_budget,
                double max_turnover_l1) {
  auto w = candidate.to(torch::kFloat64);
  w = torch::maximum(w, min_weight);
  w = torch::minimum(w, max_weight);

  const double sum = w.sum().item<double>();
  if (sum > risky_budget && sum > 0.0) {
    w = w * (risky_budget / sum);
    w = torch::maximum(w, min_weight);
    w = torch::minimum(w, max_weight);
  }

  auto delta = w - current;
  const double turnover = delta.abs().sum().item<double>();
  if (max_turnover_l1 >= 0.0 && turnover > max_turnover_l1 && turnover > 0.0) {
    w = current + delta * (max_turnover_l1 / turnover);
    w = torch::maximum(w, min_weight);
    w = torch::minimum(w, max_weight);
    const double post_sum = w.sum().item<double>();
    if (post_sum > risky_budget && post_sum > 0.0) {
      w = w * (risky_budget / post_sum);
    }
  }
  return w.detach();
}

[[nodiscard]] inline torch::Tensor
bool_mask_or_true(const torch::Tensor &mask, std::int64_t A,
                  const torch::Device &device, const char *name) {
  if (!mask.defined()) {
    return torch::ones(
        {A}, torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  TORCH_CHECK(mask.dim() == 1 && mask.size(0) == A, "[portfolio_solver] ", name,
              " must be [A]");
  return mask.to(torch::TensorOptions().dtype(torch::kBool).device(device));
}

[[nodiscard]] inline torch::Tensor tail_mean_loss(const torch::Tensor &losses,
                                                  double alpha) {
  TORCH_CHECK(losses.defined() && losses.dim() == 1,
              "[portfolio_solver] losses must be [S]");
  TORCH_CHECK(alpha > 0.0 && alpha < 1.0,
              "[portfolio_solver] alpha must be in (0,1)");
  const auto S = losses.size(0);
  const auto tail_count = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::ceil((1.0 - alpha) * S)));
  auto top = std::get<0>(losses.topk(tail_count, /*dim=*/0,
                                     /*largest=*/true, /*sorted=*/false));
  return top.mean();
}

[[nodiscard]] inline solve_context_t
make_solve_context(const belief_ns::AllocationBelief &belief,
                   const PortfolioState &portfolio, const MarketState &market,
                   const PortfolioConstraints &constraints) {
  const auto A = belief_ns::asset_count(belief);
  validate_portfolio_state(portfolio, A);
  validate_market_state(market, A);
  validate_portfolio_constraints(constraints, A);
  if (portfolio.accounting_node_id !=
      belief.base_policy.accounting_numeraire_id) {
    throw std::runtime_error(
        "[portfolio_solver] portfolio accounting_node_id must match belief "
        "accounting_numeraire_id");
  }
  if (portfolio.reserve_node_id != belief.base_policy.reserve_asset_id) {
    throw std::runtime_error(
        "[portfolio_solver] portfolio reserve_node_id must match belief "
        "reserve_asset_id");
  }
  TORCH_CHECK(belief.scenarios.defined(),
              "[portfolio_solver] belief scenarios are required");

  const auto device = belief.scenarios.device();
  auto tensor_options =
      torch::TensorOptions().dtype(torch::kFloat64).device(device);

  solve_context_t ctx{};
  ctx.A = A;
  ctx.device = device;
  ctx.tensor_options = tensor_options;
  ctx.current = portfolio.current_weights.to(tensor_options).clamp_min(0.0);
  ctx.min_weight = constraints.min_weight.defined()
                       ? constraints.min_weight.to(tensor_options)
                       : torch::zeros({A}, tensor_options);
  ctx.max_weight =
      constraints.max_weight.defined()
          ? constraints.max_weight.to(tensor_options)
          : torch::full({A}, 1.0 - constraints.min_base_reserve_weight,
                        tensor_options);
  auto configured_max_weight = ctx.max_weight.clone();
  auto capacity = belief.capacity_weight_limit.defined()
                      ? belief.capacity_weight_limit.to(tensor_options)
                      : torch::full({A}, 1.0, tensor_options);
  ctx.confidence = belief.confidence.to(tensor_options).clamp(0.0, 1.0);
  auto valid_mask =
      bool_mask_or_true(belief.valid_mask, A, device, "valid_mask");
  auto tradable_mask =
      bool_mask_or_true(belief.tradable_mask, A, device, "tradable_mask");
  auto market_tradable = bool_mask_or_true(market.tradable_mask, A, device,
                                           "market.tradable_mask");
  ctx.active_mask =
      valid_mask.logical_and(tradable_mask).logical_and(market_tradable);
  ctx.max_weight = torch::minimum(ctx.max_weight, capacity);
  ctx.max_weight =
      torch::minimum(ctx.max_weight, ctx.confidence * configured_max_weight);
  ctx.max_weight =
      ctx.max_weight.masked_fill(ctx.active_mask.logical_not(), 0.0);
  ctx.min_weight =
      ctx.min_weight.masked_fill(ctx.active_mask.logical_not(), 0.0);
  ctx.min_weight = torch::minimum(ctx.min_weight, ctx.max_weight);
  ctx.risky_budget = std::max(0.0, 1.0 - constraints.min_base_reserve_weight);
  ctx.linear_cost = belief.linear_cost.defined()
                        ? belief.linear_cost.to(tensor_options).clamp_min(0.0)
                        : torch::zeros({A}, tensor_options);
  ctx.quadratic_impact =
      belief.quadratic_impact.defined()
          ? belief.quadratic_impact.to(tensor_options).clamp_min(0.0)
          : torch::zeros({A}, tensor_options);
  return ctx;
}

[[nodiscard]] inline bool
belief_quality_valid(const belief_ns::AllocationBelief &belief) {
  const auto quality =
      cuwacunu::wikimyei::observer::check_allocation_belief(belief);
  return belief.valid && quality.valid;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::solver_utils
