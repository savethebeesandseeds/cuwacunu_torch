// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/topology/graph/graph.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/types.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
    execution::spot_rebalance_router {

namespace graph = cuwacunu::kikijyeba::topology::graph;
namespace portfolio = cuwacunu::wikimyei::policy::portfolio;

enum class order_side_t {
  buy_edge_base,
  sell_edge_base,
};

[[nodiscard]] inline const char *order_side_name(order_side_t side) {
  switch (side) {
  case order_side_t::buy_edge_base:
    return "buy_edge_base";
  case order_side_t::sell_edge_base:
    return "sell_edge_base";
  }
  return "unknown";
}

struct spot_edge_market_state_t {
  graph::market_graph_t graph{};

  torch::Tensor edge_mid_price{};     // optional [E], quote per edge base
  torch::Tensor edge_fee_rate{};      // optional [E]
  torch::Tensor edge_spread_rate{};   // optional [E]
  torch::Tensor edge_slippage_rate{}; // optional [E]
  torch::Tensor min_notional_base{};  // optional [E], numeraire/base units
  torch::Tensor max_notional_base{};  // optional [E], numeraire/base units
  torch::Tensor edge_tradable_mask{}; // optional [E], bool
};

struct spot_rebalance_router_options_t {
  double min_delta_weight{1.0e-8};
  bool enforce_min_notional{true};
  bool allow_partial_max_notional{false};
};

struct routed_order_intent_t {
  std::string node_id{};
  std::string edge_id{};
  std::string edge_base_node_id{};
  std::string edge_quote_node_id{};
  order_side_t side{order_side_t::buy_edge_base};

  double delta_weight{0.0};
  double requested_notional_base{0.0};
  double routed_notional_base{0.0};
  double estimated_cost_base{0.0};
};

struct skipped_rebalance_intent_t {
  std::string node_id{};
  double delta_weight{0.0};
  double requested_notional_base{0.0};
  std::string reason{};
};

struct spot_rebalance_plan_t {
  portfolio::timestamp_ms_t timestamp_ms{0};
  portfolio::node_id_t base_reserve_node_id{};
  std::vector<portfolio::node_id_t> node_ids{};

  std::vector<routed_order_intent_t> orders{};
  std::vector<skipped_rebalance_intent_t> skipped{};

  double requested_turnover_weight{0.0};
  double routed_turnover_weight{0.0};
  double requested_notional_base{0.0};
  double routed_notional_base{0.0};
  double estimated_transaction_cost_base{0.0};

  bool valid{false};
  portfolio::decision_diagnostics_t diagnostics{};
};

namespace detail {

inline void require_edge_vector(const torch::Tensor &tensor, std::int64_t E,
                                const char *name, bool required) {
  if (!tensor.defined()) {
    TORCH_CHECK(!required, "[spot_rebalance_router] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == E,
              "[spot_rebalance_router] ", name, " must be [E]");
}

inline void require_finite_edge_vector(const torch::Tensor &tensor,
                                       std::int64_t E, const char *name,
                                       bool required) {
  require_edge_vector(tensor, E, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(torch::isfinite(tensor).all().template item<bool>(),
              "[spot_rebalance_router] ", name, " contains non-finite values");
}

inline void require_nonnegative_edge_vector(const torch::Tensor &tensor,
                                            std::int64_t E, const char *name,
                                            bool required) {
  require_finite_edge_vector(tensor, E, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(!tensor.to(torch::kFloat64).lt(0.0).any().template item<bool>(),
              "[spot_rebalance_router] ", name, " contains negative values");
}

inline void require_positive_edge_vector(const torch::Tensor &tensor,
                                         std::int64_t E, const char *name,
                                         bool required) {
  require_finite_edge_vector(tensor, E, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.to(torch::kFloat64).gt(0.0).all().template item<bool>(),
              "[spot_rebalance_router] ", name, " must be positive");
}

inline void require_bool_edge_vector(const torch::Tensor &tensor,
                                     std::int64_t E, const char *name,
                                     bool required) {
  require_edge_vector(tensor, E, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.scalar_type() == torch::kBool, "[spot_rebalance_router] ",
              name, " must be bool");
}

[[nodiscard]] inline torch::Tensor
vector_or_full(const torch::Tensor &tensor, std::int64_t E, double value,
               torch::TensorOptions options) {
  if (tensor.defined()) {
    require_edge_vector(tensor, E, "edge vector", true);
    return tensor.to(options);
  }
  return torch::full({E}, value, options);
}

[[nodiscard]] inline torch::Tensor
bool_vector_or_true(const torch::Tensor &tensor, std::int64_t E,
                    const torch::Device &device) {
  if (tensor.defined()) {
    require_edge_vector(tensor, E, "edge_tradable_mask", true);
    return tensor.to(torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  return torch::ones({E},
                     torch::TensorOptions().dtype(torch::kBool).device(device));
}

struct direct_edge_choice_t {
  bool found{false};
  graph::edge_index_t edge_index{-1};
  bool asset_is_edge_base{false};
};

[[nodiscard]] inline direct_edge_choice_t
find_direct_base_reserve_edge(const graph::market_graph_t &market_graph,
                              const std::string &asset_node_id,
                              const std::string &base_reserve_node_id) {
  direct_edge_choice_t fallback{};
  for (graph::edge_index_t e = 0; e < market_graph.num_edges(); ++e) {
    const auto edge = market_graph.directed_edge(e);
    if (edge.base_node_id == asset_node_id &&
        edge.quote_node_id == base_reserve_node_id) {
      return {.found = true, .edge_index = e, .asset_is_edge_base = true};
    }
    if (!fallback.found && edge.base_node_id == base_reserve_node_id &&
        edge.quote_node_id == asset_node_id) {
      fallback = {.found = true, .edge_index = e, .asset_is_edge_base = false};
    }
  }
  return fallback;
}

} // namespace detail

inline void
validate_spot_edge_market_state(const spot_edge_market_state_t &market) {
  market.graph.validate();
  const auto E = market.graph.num_edges();
  detail::require_positive_edge_vector(market.edge_mid_price, E,
                                       "edge_mid_price", false);
  detail::require_nonnegative_edge_vector(market.edge_fee_rate, E,
                                          "edge_fee_rate", false);
  detail::require_nonnegative_edge_vector(market.edge_spread_rate, E,
                                          "edge_spread_rate", false);
  detail::require_nonnegative_edge_vector(market.edge_slippage_rate, E,
                                          "edge_slippage_rate", false);
  detail::require_nonnegative_edge_vector(market.min_notional_base, E,
                                          "min_notional_base", false);
  detail::require_nonnegative_edge_vector(market.max_notional_base, E,
                                          "max_notional_base", false);
  detail::require_bool_edge_vector(market.edge_tradable_mask, E,
                                   "edge_tradable_mask", false);
  if (market.min_notional_base.defined() &&
      market.max_notional_base.defined()) {
    auto min_n = market.min_notional_base.to(torch::kFloat64);
    auto max_n = market.max_notional_base.to(torch::kFloat64);
    TORCH_CHECK((max_n >= min_n).all().template item<bool>(),
                "[spot_rebalance_router] max_notional_base must be >= "
                "min_notional_base");
  }
}

[[nodiscard]] inline spot_rebalance_plan_t
plan_rebalance(const portfolio::TargetPortfolio &target,
               const portfolio::PortfolioState &portfolio_state,
               const spot_edge_market_state_t &market,
               const spot_rebalance_router_options_t &options = {}) {
  TORCH_CHECK(options.min_delta_weight >= 0.0,
              "[spot_rebalance_router] min_delta_weight must be nonnegative");
  const auto A = static_cast<std::int64_t>(target.node_ids.size());
  portfolio::validate_portfolio_state(portfolio_state, A);
  validate_spot_edge_market_state(market);
  portfolio::validate_target_portfolio(target, A,
                                       portfolio_state.current_weights);
  TORCH_CHECK(!portfolio_state.reserve_node_id.empty(),
              "[spot_rebalance_router] reserve_node_id required");
  TORCH_CHECK(target.base_reserve_node_id == portfolio_state.reserve_node_id,
              "[spot_rebalance_router] target base_reserve_node_id must match "
              "portfolio reserve_node_id in v1");
  TORCH_CHECK(portfolio_state.equity_value_base > 0.0,
              "[spot_rebalance_router] equity_value_base must be positive");

  const auto E = market.graph.num_edges();
  auto tensor_options = portfolio_state.current_weights.options()
                            .dtype(torch::kFloat64)
                            .device(torch::kCPU);
  auto current = portfolio_state.current_weights.to(tensor_options);
  auto target_weights = target.target_weights.to(tensor_options);
  auto delta = target.delta_weights.defined()
                   ? target.delta_weights.to(tensor_options)
                   : (target_weights - current);
  auto fee =
      detail::vector_or_full(market.edge_fee_rate, E, 0.0, tensor_options)
          .clamp_min(0.0);
  auto spread =
      detail::vector_or_full(market.edge_spread_rate, E, 0.0, tensor_options)
          .clamp_min(0.0);
  auto slippage =
      detail::vector_or_full(market.edge_slippage_rate, E, 0.0, tensor_options)
          .clamp_min(0.0);
  auto min_notional =
      detail::vector_or_full(market.min_notional_base, E, 0.0, tensor_options)
          .clamp_min(0.0);
  auto max_notional =
      detail::vector_or_full(market.max_notional_base, E,
                             std::numeric_limits<double>::infinity(),
                             tensor_options)
          .clamp_min(0.0);
  auto edge_tradable = detail::bool_vector_or_true(market.edge_tradable_mask, E,
                                                   torch::Device(torch::kCPU));

  spot_rebalance_plan_t plan{};
  plan.timestamp_ms = target.timestamp_ms;
  plan.base_reserve_node_id = target.base_reserve_node_id;
  plan.node_ids = target.node_ids;
  plan.valid = true;

  for (std::int64_t i = 0; i < A; ++i) {
    const auto d = delta.index({i}).item<double>();
    const auto requested_notional =
        std::abs(d) * portfolio_state.equity_value_base;
    plan.requested_turnover_weight += std::abs(d);
    plan.requested_notional_base += requested_notional;
    if (std::abs(d) <= options.min_delta_weight) {
      continue;
    }

    const auto &asset = target.node_ids[static_cast<std::size_t>(i)];
    const auto choice = detail::find_direct_base_reserve_edge(
        market.graph, asset, plan.base_reserve_node_id);
    if (!choice.found) {
      plan.valid = false;
      plan.skipped.push_back({.node_id = asset,
                              .delta_weight = d,
                              .requested_notional_base = requested_notional,
                              .reason = "no_direct_market_edge"});
      plan.diagnostics.failures.push_back("no direct market edge for " + asset);
      continue;
    }

    const auto e = choice.edge_index;
    if (!edge_tradable.index({e}).item<bool>()) {
      plan.valid = false;
      plan.skipped.push_back({.node_id = asset,
                              .delta_weight = d,
                              .requested_notional_base = requested_notional,
                              .reason = "edge_not_tradable"});
      plan.diagnostics.failures.push_back("edge not tradable for " + asset);
      continue;
    }

    const auto min_n = min_notional.index({e}).item<double>();
    if (options.enforce_min_notional && requested_notional < min_n) {
      plan.skipped.push_back({.node_id = asset,
                              .delta_weight = d,
                              .requested_notional_base = requested_notional,
                              .reason = "below_min_notional"});
      plan.diagnostics.warnings.push_back("below min notional for " + asset);
      continue;
    }

    const auto max_n = max_notional.index({e}).item<double>();
    double routed_notional = requested_notional;
    if (std::isfinite(max_n) && routed_notional > max_n) {
      if (!options.allow_partial_max_notional) {
        plan.valid = false;
        plan.skipped.push_back({.node_id = asset,
                                .delta_weight = d,
                                .requested_notional_base = requested_notional,
                                .reason = "above_max_notional"});
        plan.diagnostics.failures.push_back("above max notional for " + asset);
        continue;
      }
      routed_notional = max_n;
      plan.diagnostics.warnings.push_back("partial max-notional route for " +
                                          asset);
    }

    const bool increasing_asset = d > 0.0;
    const order_side_t side =
        choice.asset_is_edge_base
            ? (increasing_asset ? order_side_t::buy_edge_base
                                : order_side_t::sell_edge_base)
            : (increasing_asset ? order_side_t::sell_edge_base
                                : order_side_t::buy_edge_base);
    const auto edge = market.graph.directed_edge(e);
    const double cost_rate = fee.index({e}).item<double>() +
                             0.5 * spread.index({e}).item<double>() +
                             slippage.index({e}).item<double>();
    const double estimated_cost = routed_notional * cost_rate;
    const double routed_weight =
        routed_notional / portfolio_state.equity_value_base;

    plan.orders.push_back({
        .node_id = asset,
        .edge_id = edge.edge_id,
        .edge_base_node_id = edge.base_node_id,
        .edge_quote_node_id = edge.quote_node_id,
        .side = side,
        .delta_weight = d,
        .requested_notional_base = requested_notional,
        .routed_notional_base = routed_notional,
        .estimated_cost_base = estimated_cost,
    });
    plan.routed_turnover_weight += routed_weight;
    plan.routed_notional_base += routed_notional;
    plan.estimated_transaction_cost_base += estimated_cost;
  }

  if (plan.orders.empty() &&
      plan.requested_turnover_weight > options.min_delta_weight &&
      !plan.skipped.empty()) {
    plan.diagnostics.warnings.push_back("no orders routed");
  }
  if (plan.valid) {
    plan.diagnostics.notes.push_back(
        "wikimyei.policy.portfolio.spot_distributional_utility.execution."
        "spot_rebalance_router.v1");
  }
  return plan;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::execution::spot_rebalance_router
