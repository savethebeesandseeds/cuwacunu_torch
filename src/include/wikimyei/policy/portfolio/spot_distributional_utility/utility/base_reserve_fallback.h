// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    base_reserve_fallback {

enum class base_reserve_fallback_mode_t {
  hard_base_reserve,
  turnover_limited_base_reserve,
};

[[nodiscard]] inline TargetPortfolio
solve(const cuwacunu::wikimyei::observer::belief::AllocationBelief &belief,
      const PortfolioState &portfolio, const PortfolioConstraints &constraints,
      base_reserve_fallback_mode_t mode =
          base_reserve_fallback_mode_t::hard_base_reserve) {
  const auto A = cuwacunu::wikimyei::observer::belief::asset_count(belief);
  validate_portfolio_state(portfolio, A);
  validate_portfolio_constraints(constraints, A);

  auto options = portfolio.current_weights.options().dtype(torch::kFloat64);
  auto current = portfolio.current_weights.to(options).clamp_min(0.0);
  auto target = torch::zeros({A}, options);
  if (mode == base_reserve_fallback_mode_t::turnover_limited_base_reserve) {
    const double total_turnover_to_base_reserve =
        current.abs().sum().item<double>();
    if (total_turnover_to_base_reserve > constraints.max_turnover_l1 &&
        total_turnover_to_base_reserve > 0.0) {
      const double keep_fraction =
          std::max(0.0, 1.0 - constraints.max_turnover_l1 /
                                  total_turnover_to_base_reserve);
      target = current * keep_fraction;
    }
  }
  const double risky_sum = target.sum().item<double>();

  TargetPortfolio out{};
  out.timestamp_ms = belief.timestamp_ms;
  out.node_ids = belief.node_ids;
  out.base_reserve_node_id = belief.base_policy.reserve_asset_id;
  out.target_weights = target;
  out.target_base_reserve_weight = std::max(0.0, 1.0 - risky_sum);
  out.delta_weights = out.target_weights - current;
  out.turnover = out.delta_weights.abs().sum().item<double>();
  out.valid = true;
  out.diagnostics.notes.push_back(
      mode == base_reserve_fallback_mode_t::hard_base_reserve
          ? "base_reserve_fallback.hard_base_reserve"
          : "base_reserve_fallback.turnover_limited_base_reserve");
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::base_reserve_fallback
