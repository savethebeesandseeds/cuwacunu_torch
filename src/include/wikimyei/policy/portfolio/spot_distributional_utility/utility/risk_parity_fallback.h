// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>

#include <torch/torch.h>

#include "wikimyei/observer/utility/transaction_cost.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/allocation_numeraire_fallback.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/solver_utils.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    fallback::risk_parity {

namespace allocation_numeraire_fallback = cuwacunu::wikimyei::policy::
    portfolio::spot_distributional_utility::allocation_numeraire_fallback;
namespace solver = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::solver_utils;

struct solver_options_t {
  double volatility_floor{1.0e-8};
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
  TORCH_CHECK(options.volatility_floor > 0.0,
              "[risk_parity] volatility_floor must be > 0");
  if (!solver::belief_quality_valid(belief)) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.warnings.push_back(
        "risk_parity delegated to allocation_numeraire_fallback");
    return guarded;
  }

  auto ctx = solver::make_solve_context(belief, portfolio, market, constraints);
  auto covariance = belief.covariance.to(ctx.tensor_options);
  auto volatility = covariance.diagonal().clamp_min(0.0).sqrt().clamp_min(
      options.volatility_floor);
  auto desirability = (1.0 / volatility) * ctx.confidence *
                      ctx.active_mask.to(ctx.tensor_options);
  if (desirability.sum().item<double>() <= 0.0) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.warnings.push_back(
        "risk_parity found no active risk budget");
    return guarded;
  }

  auto candidate = desirability / desirability.sum() * ctx.allocation_budget;
  auto w = solver::project_weights(candidate, ctx.current, ctx.min_weight,
                                   ctx.max_weight, ctx.allocation_budget,
                                   constraints.max_turnover_l1,
                                   ctx.accounting_numeraire_index);

  auto scenarios = belief.scenarios.to(ctx.tensor_options);
  auto growth = 1.0 + scenarios.matmul(w);
  if (growth.min().item<double>() < constraints.scenario_growth_floor) {
    auto guarded = allocation_numeraire_fallback::solve(
        belief, portfolio, constraints, options.invalid_belief_mode);
    guarded.diagnostics.failures.push_back(
        "scenario growth floor infeasible for risk_parity");
    return guarded;
  }
  auto log_growth = torch::log(growth);
  auto cvar = solver::tail_mean_loss(-log_growth, constraints.cvar_alpha)
                  .item<double>();
  auto mu = belief.expected_arithmetic_return.to(ctx.tensor_options);
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
      "wikimyei.policy.portfolio.spot_distributional_utility.fallback."
      "risk_parity.v1");
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::fallback::risk_parity
