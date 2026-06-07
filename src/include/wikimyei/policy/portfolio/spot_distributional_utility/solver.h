// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/data_quality.h"
#include "wikimyei/observer/utility/transaction_cost.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/allocation_numeraire_fallback.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility {

struct solver_options_t {
  std::int64_t iterations{160};
  double learning_rate{0.05};
  allocation_numeraire_fallback::allocation_numeraire_fallback_mode_t
      invalid_belief_mode{
          allocation_numeraire_fallback::allocation_numeraire_fallback_mode_t::
              turnover_limited_numeraire};
};

namespace detail {

[[nodiscard]] inline std::int64_t accounting_numeraire_index_or_throw(
    const std::vector<node_id_t> &node_ids,
    const node_id_t &accounting_numeraire_node_id) {
  const auto it =
      std::find(node_ids.begin(), node_ids.end(), accounting_numeraire_node_id);
  TORCH_CHECK(it != node_ids.end(),
              "[spot_distributional_utility] accounting numeraire must be "
              "present in node_ids");
  return static_cast<std::int64_t>(it - node_ids.begin());
}

[[nodiscard]] inline torch::Tensor
fill_residual_to_numeraire(const torch::Tensor &weights,
                           double allocation_budget,
                           std::int64_t accounting_numeraire_index) {
  auto out = weights.to(torch::kFloat64).clone();
  TORCH_CHECK(accounting_numeraire_index >= 0 &&
                  accounting_numeraire_index < out.size(0),
              "[spot_distributional_utility] accounting numeraire index out "
              "of range");
  const double sum = out.sum().item<double>();
  const double residual = allocation_budget - sum;
  if (std::abs(residual) <= 1.0e-12) {
    return out;
  }
  out.index_put_({accounting_numeraire_index},
                 out.index({accounting_numeraire_index}) + residual);
  return out.clamp_min(0.0);
}

[[nodiscard]] inline torch::Tensor
project_weights(const torch::Tensor &candidate, const torch::Tensor &current,
                const torch::Tensor &min_weight,
                const torch::Tensor &max_weight, double allocation_budget,
                double max_turnover_l1,
                std::int64_t accounting_numeraire_index = -1) {
  auto w = candidate.to(torch::kFloat64);
  w = torch::maximum(w, min_weight);
  w = torch::minimum(w, max_weight);

  const double sum = w.sum().item<double>();
  if (sum > 0.0) {
    w = w * (allocation_budget / sum);
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
    if (post_sum > 0.0) {
      w = w * (allocation_budget / post_sum);
    }
  }
  if (accounting_numeraire_index >= 0) {
    w = fill_residual_to_numeraire(w, allocation_budget,
                                   accounting_numeraire_index);
  }
  return w.detach();
}

[[nodiscard]] inline torch::Tensor vector_or(const torch::Tensor &value,
                                             const torch::Tensor &fallback,
                                             const char *name) {
  if (!value.defined()) {
    return fallback.clone();
  }
  TORCH_CHECK(value.dim() == 1 && value.size(0) == fallback.size(0),
              "[spot_distributional_utility] ", name, " must be [A]");
  return value.to(fallback.options());
}

[[nodiscard]] inline torch::Tensor
bool_mask_or_true(const torch::Tensor &mask, std::int64_t A,
                  const torch::Device &device, const char *name) {
  if (!mask.defined()) {
    return torch::ones(
        {A}, torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  TORCH_CHECK(mask.dim() == 1 && mask.size(0) == A,
              "[spot_distributional_utility] ", name, " must be [A]");
  return mask.to(torch::TensorOptions().dtype(torch::kBool).device(device));
}

[[nodiscard]] inline torch::Tensor tail_mean_loss(const torch::Tensor &losses,
                                                  double alpha) {
  const auto S = losses.size(0);
  const auto tail_count = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::ceil((1.0 - alpha) * S)));
  auto top = std::get<0>(losses.topk(tail_count, /*dim=*/0,
                                     /*largest=*/true, /*sorted=*/false));
  return top.mean();
}

} // namespace detail

[[nodiscard]] inline TargetPortfolio
solve(const cuwacunu::wikimyei::observer::belief::AllocationBelief &belief,
      const PortfolioState &portfolio, const MarketState &market,
      const PortfolioConstraints &constraints,
      const solver_options_t &options = {}) {
  namespace belief_ns = cuwacunu::wikimyei::observer::belief;
  const auto A = belief_ns::asset_count(belief);
  validate_portfolio_state(portfolio, A);
  validate_market_state(market, A);
  validate_portfolio_constraints(constraints, A);
  if (portfolio.accounting_numeraire_node_id !=
      belief.base_policy.accounting_numeraire_id) {
    throw std::runtime_error(
        "[spot_distributional_utility] portfolio accounting_numeraire_node_id "
        "must match belief accounting_numeraire_id");
  }
  if (portfolio.node_ids != belief.node_ids) {
    throw std::runtime_error(
        "[spot_distributional_utility] portfolio node_ids must match belief "
        "node_ids");
  }

  const auto quality =
      cuwacunu::wikimyei::observer::check_allocation_belief(belief);
  if (!belief.valid || !quality.valid) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.valid = true;
    guarded.diagnostics.warnings.push_back(
        "spot_distributional_utility delegated to "
        "allocation_numeraire_fallback");
    for (const auto &failure : quality.diagnostics.failures) {
      guarded.diagnostics.failures.push_back(failure);
    }
    return guarded;
  }

  auto scenarios = belief.scenarios.to(torch::kFloat64);
  const auto device = scenarios.device();
  auto tensor_options =
      torch::TensorOptions().dtype(torch::kFloat64).device(device);
  auto current = portfolio.current_weights.to(tensor_options).clamp_min(0.0);
  const auto accounting_numeraire_index =
      detail::accounting_numeraire_index_or_throw(
          belief.node_ids, belief.base_policy.accounting_numeraire_id);
  auto min_weight = constraints.min_weight.defined()
                        ? constraints.min_weight.to(tensor_options)
                        : torch::zeros({A}, tensor_options);
  auto max_weight = constraints.max_weight.defined()
                        ? constraints.max_weight.to(tensor_options)
                        : torch::full({A}, 1.0, tensor_options);
  auto capacity = belief.capacity_weight_limit.defined()
                      ? belief.capacity_weight_limit.to(tensor_options)
                      : torch::full({A}, 1.0, tensor_options);
  auto configured_max_weight = max_weight.clone();
  auto confidence = belief.confidence.to(tensor_options).clamp(0.0, 1.0);
  auto valid_mask =
      detail::bool_mask_or_true(belief.valid_mask, A, device, "valid_mask");
  auto tradable_mask = detail::bool_mask_or_true(belief.tradable_mask, A,
                                                 device, "tradable_mask");
  auto market_tradable = detail::bool_mask_or_true(
      market.tradable_mask, A, device, "market.tradable_mask");
  auto active =
      valid_mask.logical_and(tradable_mask).logical_and(market_tradable);
  max_weight = torch::minimum(max_weight, capacity);
  max_weight = torch::minimum(max_weight, confidence * configured_max_weight);
  max_weight = max_weight.masked_fill(active.logical_not(), 0.0);
  max_weight.index_put_({accounting_numeraire_index}, 1.0);
  min_weight = min_weight.masked_fill(active.logical_not(), 0.0);
  min_weight = torch::minimum(min_weight, max_weight);
  const double allocation_budget = 1.0;

  auto linear_cost = belief.linear_cost.defined()
                         ? belief.linear_cost.to(tensor_options).clamp_min(0.0)
                         : torch::zeros({A}, tensor_options);
  auto quadratic_impact =
      belief.quadratic_impact.defined()
          ? belief.quadratic_impact.to(tensor_options).clamp_min(0.0)
          : torch::zeros({A}, tensor_options);
  auto uncertainty = (1.0 - confidence).clamp(0.0, 1.0);

  auto objective_for = [&](const torch::Tensor &w) {
    auto growth = 1.0 + scenarios.matmul(w);
    auto log_loss =
        -torch::log(growth.clamp_min(constraints.scenario_growth_floor));
    auto mean_loss = log_loss.mean();
    auto cvar = detail::tail_mean_loss(log_loss, constraints.cvar_alpha);
    auto delta = w - current;
    auto tc =
        (linear_cost * delta.abs() + quadratic_impact * delta.pow(2)).sum();
    auto concentration = w.pow(2).sum();
    auto uncertainty_penalty = (uncertainty * w).sum();
    return mean_loss + constraints.lambda_cvar * cvar + tc +
           constraints.lambda_turnover * delta.abs().sum() +
           constraints.lambda_concentration * concentration +
           constraints.lambda_uncertainty * uncertainty_penalty;
  };

  auto w = detail::project_weights(
      current, current, min_weight, max_weight, allocation_budget,
      constraints.max_turnover_l1, accounting_numeraire_index);
  for (std::int64_t i = 0; i < options.iterations; ++i) {
    w = w.detach();
    w.set_requires_grad(true);
    auto objective = objective_for(w);
    objective.backward();
    auto grad = w.grad();
    {
      torch::NoGradGuard ng;
      w = detail::project_weights(w - options.learning_rate * grad, current,
                                  min_weight, max_weight, allocation_budget,
                                  constraints.max_turnover_l1,
                                  accounting_numeraire_index);
    }
  }
  w = w.detach();
  auto growth = 1.0 + scenarios.matmul(w);
  if (growth.min().item<double>() < constraints.scenario_growth_floor) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.failures.push_back(
        "scenario growth floor infeasible for optimized target");
    return guarded;
  }

  auto log_growth = torch::log(growth);
  auto log_loss = -log_growth;
  auto cvar = detail::tail_mean_loss(log_loss, constraints.cvar_alpha);
  auto expected_arithmetic =
      (belief.expected_arithmetic_return.to(tensor_options) * w).sum();
  auto tc = cuwacunu::wikimyei::observer::compute_transaction_cost(
      w - current, linear_cost, quadratic_impact);

  TargetPortfolio out{};
  out.timestamp_ms = belief.timestamp_ms;
  out.node_ids = belief.node_ids;
  out.target_weights = w.to(portfolio.current_weights.options());
  out.delta_weights = (w - current).to(portfolio.current_weights.options());
  out.expected_log_growth = log_growth.mean().item<double>();
  out.expected_arithmetic_return = expected_arithmetic.item<double>();
  out.cvar_loss = cvar.item<double>();
  out.turnover = (w - current).abs().sum().item<double>();
  out.estimated_transaction_cost = tc.total_cost;
  out.valid = true;
  out.diagnostics.notes.push_back(
      "wikimyei.policy.portfolio.spot_distributional_utility.v1");
  if (belief.projection_validation_required && !belief.projection_validated) {
    out.diagnostics.warnings.push_back(
        "projection_validation_required_before_live_capital");
  }
  if (!belief.live_capital_allowed) {
    out.diagnostics.warnings.push_back("live_capital_allowed=false");
  }
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility
