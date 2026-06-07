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
  torch::Tensor min_notional_numeraire{}; // optional [E]
  torch::Tensor max_notional_numeraire{}; // optional [E]
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
  double requested_notional_numeraire{0.0};
  double routed_notional_numeraire{0.0};
  double estimated_cost_numeraire{0.0};
};

struct skipped_rebalance_intent_t {
  std::string node_id{};
  double delta_weight{0.0};
  double requested_notional_numeraire{0.0};
  std::string reason{};
};

struct spot_rebalance_plan_t {
  portfolio::timestamp_ms_t timestamp_ms{0};
  portfolio::node_id_t accounting_numeraire_node_id{};
  std::vector<portfolio::node_id_t> node_ids{};

  std::vector<routed_order_intent_t> orders{};
  std::vector<skipped_rebalance_intent_t> skipped{};

  double requested_turnover_weight{0.0};
  double routed_turnover_weight{0.0};
  double requested_notional_numeraire{0.0};
  double routed_notional_numeraire{0.0};
  double estimated_transaction_cost_numeraire{0.0};

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
  detail::require_nonnegative_edge_vector(market.min_notional_numeraire, E,
                                          "min_notional_numeraire", false);
  detail::require_nonnegative_edge_vector(market.max_notional_numeraire, E,
                                          "max_notional_numeraire", false);
  detail::require_bool_edge_vector(market.edge_tradable_mask, E,
                                   "edge_tradable_mask", false);
  if (market.min_notional_numeraire.defined() &&
      market.max_notional_numeraire.defined()) {
    auto min_n = market.min_notional_numeraire.to(torch::kFloat64);
    auto max_n = market.max_notional_numeraire.to(torch::kFloat64);
    TORCH_CHECK((max_n >= min_n).all().template item<bool>(),
                "[spot_rebalance_router] max_notional_numeraire must be >= "
                "min_notional_numeraire");
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
  TORCH_CHECK(portfolio_state.equity_value_numeraire > 0.0,
              "[spot_rebalance_router] equity_value_numeraire must be "
              "positive");

  auto tensor_options = portfolio_state.current_weights.options()
                            .dtype(torch::kFloat64)
                            .device(torch::kCPU);
  auto current = portfolio_state.current_weights.to(tensor_options);
  auto target_weights = target.target_weights.to(tensor_options);
  auto delta = target.delta_weights.defined()
                   ? target.delta_weights.to(tensor_options)
                   : (target_weights - current);

  spot_rebalance_plan_t plan{};
  plan.timestamp_ms = target.timestamp_ms;
  plan.accounting_numeraire_node_id =
      portfolio_state.accounting_numeraire_node_id;
  plan.node_ids = target.node_ids;
  plan.valid = true;

  for (std::int64_t i = 0; i < A; ++i) {
    const auto d = delta.index({i}).item<double>();
    const auto requested_notional =
        std::abs(d) * portfolio_state.equity_value_numeraire;
    plan.requested_turnover_weight += std::abs(d);
    plan.requested_notional_numeraire += requested_notional;
  }
  plan.routed_turnover_weight = plan.requested_turnover_weight;
  plan.routed_notional_numeraire = plan.requested_notional_numeraire;
  plan.diagnostics.notes.push_back(
      "wikimyei.policy.portfolio.spot_distributional_utility.execution."
      "target_weight_summary.v1");
  plan.diagnostics.warnings.push_back(
      "routing delegated to cajtucu.execution.paper.v1");
  return plan;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::execution::spot_rebalance_router
