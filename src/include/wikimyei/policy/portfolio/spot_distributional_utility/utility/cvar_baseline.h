// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <torch/torch.h>

#include "wikimyei/observer/utility/transaction_cost.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/allocation_numeraire_fallback.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/solver_utils.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    baseline::cvar {

namespace allocation_numeraire_fallback = cuwacunu::wikimyei::policy::
    portfolio::spot_distributional_utility::allocation_numeraire_fallback;
namespace solver = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::solver_utils;

struct solver_options_t {
  std::int64_t iterations{140};
  double learning_rate{0.04};
  allocation_numeraire_fallback::allocation_numeraire_fallback_mode_t
      invalid_belief_mode{
          allocation_numeraire_fallback::allocation_numeraire_fallback_mode_t::
              turnover_limited_numeraire};
};

[[nodiscard]] inline TargetPortfolio
solve(const cuwacunu::wikimyei::observer::belief::AllocationBelief &belief,
      const PortfolioState &portfolio, const MarketState &market,
      const PortfolioConstraints &constraints,
      const solver_options_t &options = {}) {
  if (!solver::belief_quality_valid(belief)) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.warnings.push_back(
        "cvar delegated to allocation_numeraire_fallback");
    return guarded;
  }

  auto ctx = solver::make_solve_context(belief, portfolio, market, constraints);
  auto scenarios = belief.scenarios.to(ctx.tensor_options);
  auto mu = belief.expected_arithmetic_return.to(ctx.tensor_options);

  auto objective_for = [&](const torch::Tensor &w) {
    auto growth = 1.0 + scenarios.matmul(w);
    auto loss =
        -torch::log(growth.clamp_min(constraints.scenario_growth_floor));
    auto cvar = solver::tail_mean_loss(loss, constraints.cvar_alpha);
    auto delta = w - ctx.current;
    auto tc =
        (ctx.linear_cost * delta.abs() + ctx.quadratic_impact * delta.pow(2))
            .sum();
    return -(mu * w).sum() + constraints.lambda_cvar * cvar + tc +
           constraints.lambda_turnover * delta.abs().sum() +
           constraints.lambda_concentration * w.pow(2).sum();
  };

  auto w = solver::project_weights(ctx.current, ctx.current, ctx.min_weight,
                                   ctx.max_weight, ctx.allocation_budget,
                                   constraints.max_turnover_l1,
                                   ctx.accounting_numeraire_index);
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
          ctx.max_weight, ctx.allocation_budget, constraints.max_turnover_l1,
          ctx.accounting_numeraire_index);
    }
  }
  w = w.detach();

  auto growth = 1.0 + scenarios.matmul(w);
  if (growth.min().item<double>() < constraints.scenario_growth_floor) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.failures.push_back(
        "scenario growth floor infeasible for cvar");
    return guarded;
  }
  auto log_growth = torch::log(growth);
  auto cvar = solver::tail_mean_loss(-log_growth, constraints.cvar_alpha)
                  .item<double>();
  auto tc = cuwacunu::wikimyei::observer::compute_transaction_cost(
      w - ctx.current, ctx.linear_cost, ctx.quadratic_impact);

  TargetPortfolio out{};
  out.timestamp_ms = belief.timestamp_ms;
  out.node_ids = belief.node_ids;
  out.target_weights = w.to(portfolio.current_weights.options());
  out.delta_weights = (w - ctx.current).to(portfolio.current_weights.options());
  out.expected_log_growth = log_growth.mean().item<double>();
  out.expected_arithmetic_return = (mu * w).sum().item<double>();
  out.cvar_loss = cvar;
  out.turnover = (w - ctx.current).abs().sum().item<double>();
  out.estimated_transaction_cost = tc.total_cost;
  out.valid = true;
  out.diagnostics.notes.push_back(
      "wikimyei.policy.portfolio.spot_distributional_utility.baseline.cvar."
      "v1");
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::baseline::cvar
