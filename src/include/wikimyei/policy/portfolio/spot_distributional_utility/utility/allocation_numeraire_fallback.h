// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    allocation_numeraire_fallback {

enum class allocation_numeraire_fallback_mode_t {
  hard_numeraire,
  turnover_limited_numeraire,
};

[[nodiscard]] inline TargetPortfolio
solve(const cuwacunu::wikimyei::observer::belief::AllocationBelief &belief,
      const PortfolioState &portfolio, const PortfolioConstraints &constraints,
      allocation_numeraire_fallback_mode_t mode =
          allocation_numeraire_fallback_mode_t::hard_numeraire) {
  const auto A = cuwacunu::wikimyei::observer::belief::asset_count(belief);
  validate_portfolio_state(portfolio, A);
  validate_portfolio_constraints(constraints, A);

  auto options = portfolio.current_weights.options().dtype(torch::kFloat64);
  auto current = portfolio.current_weights.to(options).clamp_min(0.0);
  auto target = torch::zeros({A}, options);
  const auto numeraire_it =
      std::find(belief.node_ids.begin(), belief.node_ids.end(),
                belief.base_policy.accounting_numeraire_id);
  TORCH_CHECK(numeraire_it != belief.node_ids.end(),
              "[allocation_numeraire_fallback] accounting numeraire must be "
              "present in belief node_ids");
  const auto numeraire_index =
      static_cast<std::int64_t>(numeraire_it - belief.node_ids.begin());
  if (mode == allocation_numeraire_fallback_mode_t::turnover_limited_numeraire) {
    const double total_turnover_to_numeraire =
        current.abs().sum().item<double>();
    if (total_turnover_to_numeraire > constraints.max_turnover_l1 &&
        total_turnover_to_numeraire > 0.0) {
      const double keep_fraction =
          std::max(0.0, 1.0 - constraints.max_turnover_l1 /
                                  total_turnover_to_numeraire);
      target = current * keep_fraction;
    }
  }
  const double allocated_sum = target.sum().item<double>();
  target.index_put_({numeraire_index}, target.index({numeraire_index}) +
                                            std::max(0.0, 1.0 - allocated_sum));

  TargetPortfolio out{};
  out.timestamp_ms = belief.timestamp_ms;
  out.node_ids = belief.node_ids;
  out.target_weights = target;
  out.delta_weights = out.target_weights - current;
  out.turnover = out.delta_weights.abs().sum().item<double>();
  out.valid = true;
  out.diagnostics.notes.push_back(
      mode == allocation_numeraire_fallback_mode_t::hard_numeraire
          ? "allocation_numeraire_fallback.hard_numeraire"
          : "allocation_numeraire_fallback.turnover_limited_numeraire");
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::allocation_numeraire_fallback
