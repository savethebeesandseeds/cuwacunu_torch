// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <torch/torch.h>

#include "wikimyei/engine/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/base_reserve_fallback.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/solver_utils.h"
#include "wikimyei/observer/utility/transaction_cost.h"

namespace cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::
    baseline::mean_variance {

namespace base_reserve_fallback = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::base_reserve_fallback;
namespace solver = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::solver_utils;

struct solver_options_t {
  std::int64_t iterations{120};
  double learning_rate{0.05};
  double risk_aversion{1.0};
  base_reserve_fallback::base_reserve_fallback_mode_t invalid_belief_mode{
      base_reserve_fallback::base_reserve_fallback_mode_t::
          turnover_limited_base_reserve};
};

[[nodiscard]] inline TargetPortfolio
solve(const cuwacunu::wikimyei::observer::belief::AllocationBelief &belief,
      const PortfolioState &portfolio, const MarketState &market,
      const PortfolioConstraints &constraints,
      const solver_options_t &options = {}) {
  if (!solver::belief_quality_valid(belief)) {
    auto guarded = base_reserve_fallback::solve(belief, portfolio, constraints,
                                                options.invalid_belief_mode);
    guarded.diagnostics.warnings.push_back(
        "mean_variance delegated to base_reserve_fallback");
    return guarded;
  }

  auto ctx = solver::make_solve_context(belief, portfolio, market, constraints);
  auto mu = belief.expected_arithmetic_return.to(ctx.tensor_options);
  auto covariance = belief.covariance.to(ctx.tensor_options);
  auto uncertainty = (1.0 - ctx.confidence).clamp(0.0, 1.0);

  auto objective_for = [&](const torch::Tensor &w) {
    auto expected_return = (mu * w).sum();
    auto variance =
        w.unsqueeze(0).matmul(covariance).matmul(w.unsqueeze(1)).squeeze();
    auto delta = w - ctx.current;
    auto tc =
        (ctx.linear_cost * delta.abs() + ctx.quadratic_impact * delta.pow(2))
            .sum();
    return -expected_return + options.risk_aversion * variance + tc +
           constraints.lambda_turnover * delta.abs().sum() +
           constraints.lambda_concentration * w.pow(2).sum() +
           constraints.lambda_uncertainty * (uncertainty * w).sum();
  };

  auto w = solver::project_weights(ctx.current, ctx.current, ctx.min_weight,
                                   ctx.max_weight, ctx.risky_budget,
                                   constraints.max_turnover_l1);
  for (std::int64_t i = 0; i < options.iterations; ++i) {
    w = w.detach();
    w.set_requires_grad(true);
    auto objective = objective_for(w);
    objective.backward();
    auto grad = w.grad();
    {
      torch::NoGradGuard ng;
      w = solver::project_weights(
          w - options.learning_rate * grad, ctx.current, ctx.min_weight,
          ctx.max_weight, ctx.risky_budget, constraints.max_turnover_l1);
    }
  }
  w = w.detach();

  auto scenarios = belief.scenarios.to(ctx.tensor_options);
  auto growth = 1.0 + scenarios.matmul(w);
  if (growth.min().item<double>() < constraints.scenario_growth_floor) {
    auto guarded = base_reserve_fallback::solve(belief, portfolio, constraints,
                                                options.invalid_belief_mode);
    guarded.diagnostics.failures.push_back(
        "scenario growth floor infeasible for mean_variance");
    return guarded;
  }
  auto log_growth = torch::log(growth);
  auto cvar = solver::tail_mean_loss(-log_growth, constraints.cvar_alpha)
                  .item<double>();
  auto expected_arithmetic = (mu * w).sum().item<double>();
  auto tc = cuwacunu::wikimyei::observer::compute_transaction_cost(
      w - ctx.current, ctx.linear_cost, ctx.quadratic_impact);

  TargetPortfolio out{};
  out.timestamp_ms = belief.timestamp_ms;
  out.node_ids = belief.node_ids;
  out.base_reserve_node_id = belief.base_policy.reserve_asset_id;
  out.target_weights = w.to(portfolio.current_weights.options());
  out.target_base_reserve_weight = std::max(0.0, 1.0 - w.sum().item<double>());
  out.delta_weights = (w - ctx.current).to(portfolio.current_weights.options());
  out.expected_log_growth = log_growth.mean().item<double>();
  out.expected_arithmetic_return = expected_arithmetic;
  out.cvar_loss = cvar;
  out.turnover = (w - ctx.current).abs().sum().item<double>();
  out.estimated_transaction_cost = tc.total_cost;
  out.valid = true;
  out.diagnostics.notes.push_back(
      "wikimyei.engine.portfolio.spot_distributional_utility.baseline."
      "mean_variance.v1");
  return out;
}

} // namespace
  // cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility::baseline::mean_variance
