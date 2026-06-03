// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <exception>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/data_quality.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/utility/base_reserve_fallback.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    risk::risk_gate {

namespace belief = cuwacunu::wikimyei::observer::belief;
namespace observer = cuwacunu::wikimyei::observer;
namespace portfolio = cuwacunu::wikimyei::policy::portfolio;
namespace base_reserve_fallback = cuwacunu::wikimyei::policy::portfolio::
    spot_distributional_utility::base_reserve_fallback;

struct risk_gate_thresholds_t {
  double min_mean_confidence{0.05};
  double min_mean_liquidity{0.02};
  double min_tradable_fraction{1.0e-12};
  double max_drawdown{std::numeric_limits<double>::infinity()};
  double max_mean_surprise{std::numeric_limits<double>::infinity()};
  double min_mean_calibration_score{0.0};
  double max_mean_abs_correlation{0.999};
  base_reserve_fallback::base_reserve_fallback_mode_t fallback_mode{
      base_reserve_fallback::base_reserve_fallback_mode_t::
          turnover_limited_base_reserve};
};

struct risk_gate_result_t {
  bool allow_trading{true};
  bool force_base_reserve_fallback{false};
  base_reserve_fallback::base_reserve_fallback_mode_t fallback_mode{
      base_reserve_fallback::base_reserve_fallback_mode_t::
          turnover_limited_base_reserve};
  std::vector<std::string> reasons{};
  std::vector<std::string> warnings{};
};

namespace detail {

[[nodiscard]] inline double mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto x = tensor.detach().to(torch::kFloat64);
  auto finite = torch::isfinite(x);
  if (!finite.any().item<bool>()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return x.index({finite}).mean().item<double>();
}

[[nodiscard]] inline double bool_fraction(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return 0.0;
  }
  return tensor.to(torch::kBool).to(torch::kFloat64).mean().item<double>();
}

[[nodiscard]] inline double
mean_abs_offdiagonal_correlation(const torch::Tensor &correlation) {
  if (!correlation.defined() || correlation.dim() != 2 ||
      correlation.size(0) != correlation.size(1) || correlation.size(0) <= 1) {
    return 0.0;
  }
  auto corr = correlation.detach().to(torch::kFloat64);
  auto mask = torch::ones(
      corr.sizes(),
      torch::TensorOptions().dtype(torch::kBool).device(corr.device()));
  mask.diagonal().fill_(false);
  auto values = corr.abs().index({mask});
  if (values.numel() == 0) {
    return 0.0;
  }
  return values.mean().item<double>();
}

inline void trip(risk_gate_result_t &result, std::string reason) {
  result.allow_trading = false;
  result.force_base_reserve_fallback = true;
  result.reasons.push_back(std::move(reason));
}

} // namespace detail

[[nodiscard]] inline risk_gate_result_t
evaluate(const belief::AllocationBelief &belief_state,
         const portfolio::PortfolioState &portfolio_state,
         const risk_gate_thresholds_t &thresholds = {}) {
  risk_gate_result_t result{};
  result.fallback_mode = thresholds.fallback_mode;

  try {
    belief::validate_allocation_belief_contract(belief_state);
  } catch (const std::exception &ex) {
    detail::trip(result, std::string("belief_contract_invalid: ") + ex.what());
    return result;
  }
  try {
    portfolio::validate_portfolio_state(portfolio_state,
                                        belief::asset_count(belief_state));
  } catch (const std::exception &ex) {
    detail::trip(result, std::string("portfolio_state_invalid: ") + ex.what());
    return result;
  }
  if (portfolio_state.accounting_node_id !=
      belief_state.base_policy.accounting_numeraire_id) {
    detail::trip(result, "accounting_numeraire_mismatch");
  }
  if (portfolio_state.reserve_node_id !=
      belief_state.base_policy.reserve_asset_id) {
    detail::trip(result, "reserve_asset_mismatch");
  }

  const auto quality = observer::check_allocation_belief(belief_state);
  if (!belief_state.valid || !quality.valid) {
    detail::trip(result, "belief_data_quality_failed");
    for (const auto &failure : quality.diagnostics.failures) {
      result.warnings.push_back(failure);
    }
  }

  const double tradable_fraction =
      detail::bool_fraction(belief_state.tradable_mask);
  if (tradable_fraction < thresholds.min_tradable_fraction) {
    detail::trip(result, "tradable_fraction_below_threshold");
  }

  const double mean_confidence = detail::mean_or_nan(belief_state.confidence);
  if (!std::isfinite(mean_confidence) ||
      mean_confidence < thresholds.min_mean_confidence) {
    detail::trip(result, "mean_confidence_below_threshold");
  }

  if (belief_state.liquidity_score.defined()) {
    const double mean_liquidity =
        detail::mean_or_nan(belief_state.liquidity_score);
    if (!std::isfinite(mean_liquidity) ||
        mean_liquidity < thresholds.min_mean_liquidity) {
      detail::trip(result, "mean_liquidity_below_threshold");
    }
  }

  if (std::isfinite(thresholds.max_drawdown) &&
      portfolio_state.drawdown > thresholds.max_drawdown) {
    detail::trip(result, "drawdown_above_threshold");
  }

  if (belief_state.surprise.defined() &&
      std::isfinite(thresholds.max_mean_surprise)) {
    const double mean_surprise = detail::mean_or_nan(belief_state.surprise);
    if (std::isfinite(mean_surprise) &&
        mean_surprise > thresholds.max_mean_surprise) {
      detail::trip(result, "mean_surprise_above_threshold");
    }
  }

  if (belief_state.calibration_score.defined()) {
    const double mean_calibration =
        detail::mean_or_nan(belief_state.calibration_score);
    if (!std::isfinite(mean_calibration) ||
        mean_calibration < thresholds.min_mean_calibration_score) {
      detail::trip(result, "mean_calibration_below_threshold");
    }
  }

  const double mean_abs_corr =
      detail::mean_abs_offdiagonal_correlation(belief_state.correlation);
  if (mean_abs_corr > thresholds.max_mean_abs_correlation) {
    detail::trip(result, "correlation_shock_above_threshold");
  }

  return result;
}

[[nodiscard]] inline portfolio::TargetPortfolio
enforce(const belief::AllocationBelief &belief_state,
        const portfolio::PortfolioState &portfolio_state,
        const portfolio::PortfolioConstraints &constraints,
        const risk_gate_thresholds_t &thresholds = {}) {
  auto gate = evaluate(belief_state, portfolio_state, thresholds);
  if (!gate.force_base_reserve_fallback) {
    portfolio::TargetPortfolio out{};
    out.timestamp_ms = belief_state.timestamp_ms;
    out.node_ids = belief_state.node_ids;
    out.base_reserve_node_id = belief_state.base_policy.reserve_asset_id;
    out.valid = true;
    out.diagnostics.notes.push_back("risk_gate.allow_trading");
    return out;
  }
  auto guarded = base_reserve_fallback::solve(belief_state, portfolio_state,
                                              constraints, gate.fallback_mode);
  guarded.diagnostics.failures.insert(guarded.diagnostics.failures.end(),
                                      gate.reasons.begin(), gate.reasons.end());
  guarded.diagnostics.warnings.insert(guarded.diagnostics.warnings.end(),
                                      gate.warnings.begin(),
                                      gate.warnings.end());
  return guarded;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::risk::risk_gate
