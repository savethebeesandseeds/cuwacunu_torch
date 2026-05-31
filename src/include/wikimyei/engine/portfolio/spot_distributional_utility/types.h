// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"

namespace cuwacunu::wikimyei::engine::portfolio {

using node_id_t = cuwacunu::wikimyei::observer::belief::node_id_t;
using timestamp_ms_t = cuwacunu::wikimyei::observer::belief::timestamp_ms_t;

struct decision_diagnostics_t {
  std::vector<std::string> notes{};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};

  [[nodiscard]] bool ok() const { return failures.empty(); }
};

struct PortfolioState {
  timestamp_ms_t timestamp_ms{0};
  node_id_t accounting_node_id{};
  node_id_t reserve_node_id{};

  torch::Tensor current_weights{}; // [A], risky assets only
  torch::Tensor current_units{};   // [A]

  double base_reserve_weight{1.0};
  double equity_value_base{0.0};

  torch::Tensor unrealized_pnl{}; // optional [A]
  double drawdown{0.0};
};

struct MarketState {
  timestamp_ms_t timestamp_ms{0};

  torch::Tensor executable_mid{};     // [A]
  torch::Tensor bid_ask_spread{};     // [A]
  torch::Tensor fee_rate{};           // [A]
  torch::Tensor estimated_slippage{}; // [A]
  torch::Tensor min_notional{};       // [A]
  torch::Tensor max_notional{};       // [A]
  torch::Tensor tradable_mask{};      // [A], bool

  std::string routing_state{};
};

struct PortfolioConstraints {
  double min_base_reserve_weight{0.0};

  torch::Tensor max_weight{}; // [A]
  torch::Tensor min_weight{}; // [A], usually zero

  double max_turnover_l1{1.0};
  double cvar_alpha{0.95};

  double lambda_cvar{1.0};
  double lambda_concentration{0.0};
  double lambda_uncertainty{0.0};
  double lambda_turnover{0.0};

  double scenario_growth_floor{1.0e-6};
};

struct TargetPortfolio {
  timestamp_ms_t timestamp_ms{0};

  std::vector<node_id_t> node_ids{}; // same order as AllocationBelief.node_ids
  node_id_t base_reserve_node_id{};

  torch::Tensor target_weights{}; // [A], risky assets only
  double target_base_reserve_weight{1.0};

  torch::Tensor delta_weights{}; // [A]

  double expected_log_growth{0.0};
  double expected_arithmetic_return{0.0};
  double cvar_loss{0.0};
  double turnover{0.0};
  double estimated_transaction_cost{0.0};

  bool valid{false};
  decision_diagnostics_t diagnostics{};
};

namespace detail {

inline void require_vector_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined()) {
    TORCH_CHECK(!required, "[portfolio] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A, "[portfolio] ", name,
              " must be [A]");
}

inline void require_finite_vector(const torch::Tensor &tensor, std::int64_t A,
                                  const char *name, bool required) {
  require_vector_shape(tensor, A, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(torch::isfinite(tensor).all().template item<bool>(),
              "[portfolio] ", name, " contains non-finite values");
}

inline void require_nonnegative_vector(const torch::Tensor &tensor,
                                       std::int64_t A, const char *name,
                                       bool required) {
  require_finite_vector(tensor, A, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(!tensor.to(torch::kFloat64).lt(0.0).any().template item<bool>(),
              "[portfolio] ", name, " contains negative values");
}

inline void require_positive_vector(const torch::Tensor &tensor, std::int64_t A,
                                    const char *name, bool required) {
  require_finite_vector(tensor, A, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.to(torch::kFloat64).gt(0.0).all().template item<bool>(),
              "[portfolio] ", name, " must be positive");
}

inline void require_bool_vector(const torch::Tensor &tensor, std::int64_t A,
                                const char *name, bool required) {
  require_vector_shape(tensor, A, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.scalar_type() == torch::kBool, "[portfolio] ", name,
              " must be bool");
}

inline void require_finite_scalar(double value, const char *name) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(std::string("[portfolio] ") + name +
                             " must be finite");
  }
}

inline void require_unique_node_ids(const std::vector<node_id_t> &node_ids,
                                    const char *name) {
  std::unordered_set<std::string> seen;
  seen.reserve(node_ids.size());
  for (const auto &node_id : node_ids) {
    if (node_id.empty()) {
      throw std::runtime_error(std::string("[portfolio] empty ") + name +
                               " entry");
    }
    if (!seen.insert(node_id).second) {
      throw std::runtime_error(std::string("[portfolio] duplicate ") + name +
                               " entry: " + node_id);
    }
  }
}

inline torch::Tensor vector_or_zeros(const torch::Tensor &tensor,
                                     std::int64_t A,
                                     const torch::TensorOptions &options) {
  if (tensor.defined()) {
    require_vector_shape(tensor, A, "vector", true);
    return tensor.to(options);
  }
  return torch::zeros({A}, options);
}

inline torch::Tensor vector_or_full(const torch::Tensor &tensor, std::int64_t A,
                                    double value,
                                    const torch::TensorOptions &options) {
  if (tensor.defined()) {
    require_vector_shape(tensor, A, "vector", true);
    return tensor.to(options);
  }
  return torch::full({A}, value, options);
}

} // namespace detail

inline void validate_portfolio_state(const PortfolioState &state,
                                     std::int64_t A) {
  if (state.accounting_node_id.empty()) {
    throw std::runtime_error("[PortfolioState] accounting_node_id is required");
  }
  if (state.reserve_node_id.empty()) {
    throw std::runtime_error("[PortfolioState] reserve_node_id is required");
  }
  if (state.accounting_node_id != state.reserve_node_id) {
    throw std::runtime_error("[PortfolioState] V1 requires accounting_node_id "
                             "and reserve_node_id to match");
  }
  detail::require_nonnegative_vector(state.current_weights, A,
                                     "current_weights", true);
  detail::require_nonnegative_vector(state.current_units, A, "current_units",
                                     false);
  detail::require_finite_vector(state.unrealized_pnl, A, "unrealized_pnl",
                                false);
  detail::require_finite_scalar(state.base_reserve_weight,
                                "base_reserve_weight");
  detail::require_finite_scalar(state.equity_value_base, "equity_value_base");
  detail::require_finite_scalar(state.drawdown, "drawdown");
  if (state.base_reserve_weight < -1.0e-9 ||
      state.base_reserve_weight > 1.0 + 1.0e-9) {
    throw std::runtime_error(
        "[PortfolioState] base_reserve_weight must be in [0,1]");
  }
  if (state.equity_value_base < 0.0) {
    throw std::runtime_error(
        "[PortfolioState] equity_value_base must be nonnegative");
  }
  if (state.drawdown < 0.0) {
    throw std::runtime_error("[PortfolioState] drawdown must be nonnegative");
  }
  const auto risky_weight_sum =
      state.current_weights.to(torch::kFloat64).sum().item<double>();
  if (risky_weight_sum + state.base_reserve_weight > 1.0 + 1.0e-6) {
    throw std::runtime_error(
        "[PortfolioState] current weights plus base reserve exceed one");
  }
}

inline void validate_market_state(const MarketState &state, std::int64_t A) {
  detail::require_positive_vector(state.executable_mid, A, "executable_mid",
                                  false);
  detail::require_nonnegative_vector(state.bid_ask_spread, A, "bid_ask_spread",
                                     false);
  detail::require_nonnegative_vector(state.fee_rate, A, "fee_rate", false);
  detail::require_nonnegative_vector(state.estimated_slippage, A,
                                     "estimated_slippage", false);
  detail::require_nonnegative_vector(state.min_notional, A, "min_notional",
                                     false);
  detail::require_nonnegative_vector(state.max_notional, A, "max_notional",
                                     false);
  detail::require_bool_vector(state.tradable_mask, A, "tradable_mask", false);
  if (state.min_notional.defined() && state.max_notional.defined()) {
    auto min_n = state.min_notional.to(torch::kFloat64);
    auto max_n = state.max_notional.to(torch::kFloat64);
    TORCH_CHECK((max_n >= min_n).all().template item<bool>(),
                "[MarketState] max_notional must be >= min_notional");
  }
}

inline void
validate_portfolio_constraints(const PortfolioConstraints &constraints,
                               std::int64_t A) {
  detail::require_finite_scalar(constraints.min_base_reserve_weight,
                                "min_base_reserve_weight");
  detail::require_finite_scalar(constraints.max_turnover_l1, "max_turnover_l1");
  detail::require_finite_scalar(constraints.cvar_alpha, "cvar_alpha");
  detail::require_finite_scalar(constraints.lambda_cvar, "lambda_cvar");
  detail::require_finite_scalar(constraints.lambda_concentration,
                                "lambda_concentration");
  detail::require_finite_scalar(constraints.lambda_uncertainty,
                                "lambda_uncertainty");
  detail::require_finite_scalar(constraints.lambda_turnover, "lambda_turnover");
  detail::require_finite_scalar(constraints.scenario_growth_floor,
                                "scenario_growth_floor");
  if (constraints.min_base_reserve_weight < 0.0 ||
      constraints.min_base_reserve_weight > 1.0) {
    throw std::runtime_error(
        "[PortfolioConstraints] min_base_reserve_weight must be in [0,1]");
  }
  if (constraints.max_turnover_l1 < 0.0) {
    throw std::runtime_error(
        "[PortfolioConstraints] max_turnover_l1 must be nonnegative");
  }
  if (constraints.cvar_alpha <= 0.0 || constraints.cvar_alpha >= 1.0) {
    throw std::runtime_error(
        "[PortfolioConstraints] cvar_alpha must be in (0,1)");
  }
  if (constraints.scenario_growth_floor <= 0.0) {
    throw std::runtime_error(
        "[PortfolioConstraints] scenario_growth_floor must be positive");
  }
  if (constraints.lambda_cvar < 0.0 || constraints.lambda_concentration < 0.0 ||
      constraints.lambda_uncertainty < 0.0 ||
      constraints.lambda_turnover < 0.0) {
    throw std::runtime_error(
        "[PortfolioConstraints] lambda values must be nonnegative");
  }
  detail::require_nonnegative_vector(constraints.max_weight, A, "max_weight",
                                     false);
  detail::require_nonnegative_vector(constraints.min_weight, A, "min_weight",
                                     false);
  if (constraints.max_weight.defined() && constraints.min_weight.defined()) {
    auto max_w = constraints.max_weight.to(torch::kFloat64);
    auto min_w = constraints.min_weight.to(torch::kFloat64);
    TORCH_CHECK((max_w >= min_w).all().template item<bool>(),
                "[PortfolioConstraints] max_weight must be >= min_weight");
  }
  if (constraints.min_weight.defined()) {
    const double min_sum =
        constraints.min_weight.to(torch::kFloat64).sum().item<double>();
    if (min_sum > 1.0 - constraints.min_base_reserve_weight + 1.0e-9) {
      throw std::runtime_error(
          "[PortfolioConstraints] min_weight exceeds risky budget");
    }
  }
}

inline void validate_target_portfolio(
    const TargetPortfolio &target, std::int64_t A,
    const torch::Tensor &current_weights = torch::Tensor(),
    bool require_valid = true, double base_reserve_tolerance = 1.0e-8,
    double delta_tolerance = 1.0e-8) {
  if (target.node_ids.size() != static_cast<std::size_t>(A)) {
    throw std::runtime_error(
        "[TargetPortfolio] node_ids size must match asset count");
  }
  detail::require_unique_node_ids(target.node_ids, "target node_ids");
  if (target.base_reserve_node_id.empty()) {
    throw std::runtime_error(
        "[TargetPortfolio] base_reserve_node_id is required");
  }
  for (const auto &node_id : target.node_ids) {
    if (node_id == target.base_reserve_node_id) {
      throw std::runtime_error(
          "[TargetPortfolio] base_reserve_node_id must not be duplicated in "
          "risky node_ids");
    }
  }
  if (require_valid && !target.valid) {
    throw std::runtime_error("[TargetPortfolio] target is invalid");
  }
  detail::require_nonnegative_vector(target.target_weights, A, "target_weights",
                                     true);
  detail::require_finite_vector(target.delta_weights, A, "delta_weights",
                                false);
  detail::require_finite_scalar(target.target_base_reserve_weight,
                                "target_base_reserve_weight");
  detail::require_finite_scalar(target.expected_log_growth,
                                "expected_log_growth");
  detail::require_finite_scalar(target.expected_arithmetic_return,
                                "expected_arithmetic_return");
  detail::require_finite_scalar(target.cvar_loss, "cvar_loss");
  detail::require_finite_scalar(target.turnover, "turnover");
  detail::require_finite_scalar(target.estimated_transaction_cost,
                                "estimated_transaction_cost");
  if (target.target_base_reserve_weight < -base_reserve_tolerance ||
      target.target_base_reserve_weight > 1.0 + base_reserve_tolerance) {
    throw std::runtime_error(
        "[TargetPortfolio] target_base_reserve_weight must be in [0,1]");
  }
  if (target.turnover < -delta_tolerance) {
    throw std::runtime_error("[TargetPortfolio] turnover must be nonnegative");
  }
  if (target.estimated_transaction_cost < -delta_tolerance) {
    throw std::runtime_error(
        "[TargetPortfolio] estimated_transaction_cost must be nonnegative");
  }
  const auto target_weights = target.target_weights.to(torch::kFloat64);
  const double risky_sum = target_weights.sum().item<double>();
  const double total_weight = risky_sum + target.target_base_reserve_weight;
  if (std::abs(total_weight - 1.0) > base_reserve_tolerance) {
    throw std::runtime_error(
        "[TargetPortfolio] target weights plus base reserve must equal one");
  }
  if (current_weights.defined()) {
    detail::require_nonnegative_vector(current_weights, A, "current_weights",
                                       true);
    auto implied_delta = target_weights - current_weights.to(torch::kFloat64);
    if (target.delta_weights.defined()) {
      const double err =
          (target.delta_weights.to(torch::kFloat64) - implied_delta)
              .abs()
              .max()
              .item<double>();
      if (err > delta_tolerance) {
        throw std::runtime_error(
            "[TargetPortfolio] delta_weights do not match target-current");
      }
    }
    const double implied_turnover = implied_delta.abs().sum().item<double>();
    if (std::abs(target.turnover - implied_turnover) > delta_tolerance) {
      throw std::runtime_error(
          "[TargetPortfolio] turnover does not match target-current");
    }
  }
}

} // namespace cuwacunu::wikimyei::engine::portfolio
