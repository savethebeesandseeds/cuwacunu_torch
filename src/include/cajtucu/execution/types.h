// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/topology/graph/graph.h"

namespace cuwacunu::cajtucu::execution {

namespace graph = cuwacunu::kikijyeba::topology::graph;

inline constexpr const char *kCajtucuExecutionIntentSchemaV1 =
    "cajtucu.execution.intent.v1";
inline constexpr const char *kCajtucuExecutionLedgerSchemaV1 =
    "cajtucu.execution.ledger.v1";
inline constexpr const char *kCajtucuExecutionTraceSchemaV1 =
    "cajtucu.execution.trace.v1";
inline constexpr const char *kCajtucuPaperBackendIdV1 =
    "cajtucu.execution.paper.v1";
inline constexpr const char *kCajtucuMarketSourceUnknown = "unknown";

using timestamp_ms_t = std::int64_t;
using node_id_t = std::string;

enum class order_side_t {
  buy_asset,
  sell_asset,
};

[[nodiscard]] inline const char *order_side_name(order_side_t side) {
  switch (side) {
  case order_side_t::buy_asset:
    return "buy_asset";
  case order_side_t::sell_asset:
    return "sell_asset";
  }
  return "unknown";
}

enum class fill_status_t {
  filled,
  partially_filled,
  rejected,
};

[[nodiscard]] inline const char *fill_status_name(fill_status_t status) {
  switch (status) {
  case fill_status_t::filled:
    return "filled";
  case fill_status_t::partially_filled:
    return "partially_filled";
  case fill_status_t::rejected:
    return "rejected";
  }
  return "unknown";
}

struct market_execution_state_t {
  std::string market_source_id{kCajtucuMarketSourceUnknown};
  bool synthetic_direct_edges{false};
  timestamp_ms_t timestamp_ms{0};
  graph::market_graph_t graph{};

  torch::Tensor edge_mid_price{};     // [E], quote per edge base
  torch::Tensor edge_fee_rate{};      // optional [E]
  torch::Tensor edge_spread_rate{};   // optional [E]
  torch::Tensor edge_slippage_rate{}; // optional [E]
  torch::Tensor min_notional_base{};  // optional [E], reserve units
  torch::Tensor max_notional_base{};  // optional [E], reserve units
  torch::Tensor edge_tradable_mask{}; // optional [E], bool
};

struct execution_intent_t {
  std::string intent_schema_id{kCajtucuExecutionIntentSchemaV1};
  std::string intent_id{};
  std::string action_id{};
  std::string policy_id{};
  std::string method_id{};
  std::string runtime_run_id{};
  std::string environment_run_id{};
  std::string episode_id{};
  std::string anchor_key{};
  timestamp_ms_t timestamp_ms{0};

  std::vector<node_id_t> node_ids{};
  node_id_t base_reserve_node_id{};

  torch::Tensor current_weights{}; // [A]
  torch::Tensor target_weights{};  // [A]
  double current_base_reserve_weight{1.0};
  double target_base_reserve_weight{1.0};
  double equity_value_base{0.0};
};

struct execution_order_t {
  std::string order_id{};
  node_id_t node_id{};
  node_id_t settlement_node_id{};
  std::string edge_id{};
  node_id_t edge_base_node_id{};
  node_id_t edge_quote_node_id{};
  order_side_t side{order_side_t::buy_asset};

  double delta_weight{0.0};
  double requested_notional_base{0.0};
  double routed_notional_base{0.0};
  double quantity_asset{0.0};
  double fill_price_base{std::numeric_limits<double>::quiet_NaN()};
  double fee_base{0.0};
  double spread_cost_base{0.0};
  double slippage_base{0.0};

  bool valid{true};
  std::string reject_reason{};
};

struct paper_fill_t {
  std::string order_id{};
  node_id_t node_id{};
  std::string edge_id{};
  order_side_t side{order_side_t::buy_asset};
  fill_status_t status{fill_status_t::filled};

  double requested_quantity_asset{0.0};
  double filled_quantity_asset{0.0};
  double gross_notional_base{0.0};
  double fill_price_base{std::numeric_limits<double>::quiet_NaN()};
  double fee_base{0.0};
  double spread_cost_base{0.0};
  double slippage_base{0.0};
  std::string reject_reason{};
};

struct execution_ledger_t {
  std::string ledger_schema_id{kCajtucuExecutionLedgerSchemaV1};
  timestamp_ms_t timestamp_ms{0};
  node_id_t base_reserve_node_id{};
  std::vector<node_id_t> node_ids{};

  torch::Tensor units{};   // [A]
  torch::Tensor weights{}; // [A], mark-to-market risky weights

  double base_reserve_units{0.0};
  double base_reserve_weight{1.0};
  double equity_value_base{0.0};

  double cumulative_fee_base{0.0};
  double cumulative_spread_cost_base{0.0};
  double cumulative_slippage_base{0.0};
  double cumulative_turnover_base{0.0};
  std::int64_t fill_count{0};
};

struct execution_trace_t {
  std::string trace_schema_id{kCajtucuExecutionTraceSchemaV1};
  std::string trace_id{};
  std::string backend_id{kCajtucuPaperBackendIdV1};

  execution_intent_t intent{};
  std::vector<execution_order_t> orders{};
  std::vector<paper_fill_t> fills{};

  execution_ledger_t ledger_before{};
  execution_ledger_t ledger_after{};

  double requested_notional_base{0.0};
  double routed_notional_base{0.0};
  double total_fee_base{0.0};
  double total_spread_cost_base{0.0};
  double total_slippage_base{0.0};
  double total_transaction_cost_base{0.0};
  double turnover_weight{0.0};
  std::string market_source_id{kCajtucuMarketSourceUnknown};
  bool synthetic_direct_edges{false};
  double ledger_intent_equity_difference_base{0.0};
  bool ledger_intent_equity_mismatch{false};
  std::int64_t rejected_fill_count{0};
  std::int64_t partial_fill_count{0};

  bool valid{true};
  std::vector<std::string> warnings{};
  std::vector<std::string> failures{};
};

struct paper_execution_options_t {
  double min_delta_weight{1.0e-8};
  bool enforce_min_notional{true};
  bool allow_partial_fills{false};
  bool allow_negative_base_reserve{false};
  bool allow_synthetic_direct_edges{false};
  bool live_execution_allowed{false};
  double equity_mismatch_tolerance{1.0e-6};
  double equity_mismatch_fail_tolerance{1.0e-2};
  double eps{1.0e-12};
};

namespace detail {

inline void require_finite(double value, const char *name) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(std::string("[cajtucu.execution] non-finite ") +
                             name);
  }
}

inline void require_nonnegative(double value, const char *name) {
  require_finite(value, name);
  if (value < 0.0) {
    throw std::runtime_error(std::string("[cajtucu.execution] negative ") +
                             name);
  }
}

inline void require_vector_shape(const torch::Tensor &tensor, std::int64_t n,
                                 const char *name, bool required) {
  if (!tensor.defined()) {
    TORCH_CHECK(!required, "[cajtucu.execution] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == n, "[cajtucu.execution] ",
              name, " must be [N]");
}

inline void require_finite_vector(const torch::Tensor &tensor, std::int64_t n,
                                  const char *name, bool required) {
  require_vector_shape(tensor, n, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.is_floating_point(), "[cajtucu.execution] ", name,
              " must be floating point");
  TORCH_CHECK(torch::isfinite(tensor).all().item<bool>(),
              "[cajtucu.execution] ", name, " contains non-finite values");
}

inline void require_nonnegative_vector(const torch::Tensor &tensor,
                                       std::int64_t n, const char *name,
                                       bool required) {
  require_finite_vector(tensor, n, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(!tensor.to(torch::kFloat64).lt(0.0).any().item<bool>(),
              "[cajtucu.execution] ", name, " contains negative values");
}

inline void require_positive_vector(const torch::Tensor &tensor, std::int64_t n,
                                    const char *name, bool required) {
  require_finite_vector(tensor, n, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.to(torch::kFloat64).gt(0.0).all().item<bool>(),
              "[cajtucu.execution] ", name, " must be positive");
}

inline void require_bool_vector(const torch::Tensor &tensor, std::int64_t n,
                                const char *name, bool required) {
  require_vector_shape(tensor, n, name, required);
  if (!tensor.defined()) {
    return;
  }
  TORCH_CHECK(tensor.scalar_type() == torch::kBool, "[cajtucu.execution] ",
              name, " must be bool");
}

inline void require_unique_node_ids(const std::vector<node_id_t> &node_ids,
                                    const char *name) {
  std::unordered_set<std::string> seen;
  seen.reserve(node_ids.size());
  for (const auto &node_id : node_ids) {
    if (node_id.empty()) {
      throw std::runtime_error(std::string("[cajtucu.execution] empty ") +
                               name);
    }
    if (!seen.insert(node_id).second) {
      throw std::runtime_error(std::string("[cajtucu.execution] duplicate ") +
                               name + ": " + node_id);
    }
  }
}

[[nodiscard]] inline torch::Tensor
vector_or_full(const torch::Tensor &tensor, std::int64_t n, double value,
               torch::TensorOptions options) {
  if (tensor.defined()) {
    require_vector_shape(tensor, n, "vector", true);
    return tensor.to(options);
  }
  return torch::full({n}, value, options);
}

[[nodiscard]] inline torch::Tensor
bool_vector_or_true(const torch::Tensor &tensor, std::int64_t n,
                    const torch::Device &device) {
  if (tensor.defined()) {
    require_vector_shape(tensor, n, "bool vector", true);
    return tensor.to(torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  return torch::ones({n},
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

[[nodiscard]] inline double edge_scalar_or(const torch::Tensor &tensor,
                                           graph::edge_index_t edge_index,
                                           double fallback) {
  if (!tensor.defined()) {
    return fallback;
  }
  return tensor.to(torch::kFloat64).index({edge_index}).item<double>();
}

} // namespace detail

inline void
validate_paper_execution_options(const paper_execution_options_t &options) {
  detail::require_finite(options.min_delta_weight, "min_delta_weight");
  detail::require_finite(options.eps, "eps");
  if (options.min_delta_weight < 0.0) {
    throw std::runtime_error(
        "[cajtucu.execution] min_delta_weight must be nonnegative");
  }
  if (options.eps <= 0.0) {
    throw std::runtime_error("[cajtucu.execution] eps must be positive");
  }
  detail::require_finite(options.equity_mismatch_tolerance,
                         "equity_mismatch_tolerance");
  detail::require_finite(options.equity_mismatch_fail_tolerance,
                         "equity_mismatch_fail_tolerance");
  if (options.equity_mismatch_tolerance < 0.0) {
    throw std::runtime_error(
        "[cajtucu.execution] equity_mismatch_tolerance must be nonnegative");
  }
  if (options.equity_mismatch_fail_tolerance <
      options.equity_mismatch_tolerance) {
    throw std::runtime_error(
        "[cajtucu.execution] equity_mismatch_fail_tolerance must be at least "
        "equity_mismatch_tolerance");
  }
  if (options.live_execution_allowed) {
    throw std::runtime_error(
        "[cajtucu.execution] paper v1 cannot allow live execution");
  }
}

inline void
validate_market_execution_state(const market_execution_state_t &market) {
  if (market.market_source_id.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] market_source_id is required");
  }
  market.graph.validate();
  const auto E = market.graph.num_edges();
  detail::require_positive_vector(market.edge_mid_price, E, "edge_mid_price",
                                  true);
  detail::require_nonnegative_vector(market.edge_fee_rate, E, "edge_fee_rate",
                                     false);
  detail::require_nonnegative_vector(market.edge_spread_rate, E,
                                     "edge_spread_rate", false);
  detail::require_nonnegative_vector(market.edge_slippage_rate, E,
                                     "edge_slippage_rate", false);
  detail::require_nonnegative_vector(market.min_notional_base, E,
                                     "min_notional_base", false);
  detail::require_nonnegative_vector(market.max_notional_base, E,
                                     "max_notional_base", false);
  detail::require_bool_vector(market.edge_tradable_mask, E,
                              "edge_tradable_mask", false);
}

inline void validate_execution_intent(const execution_intent_t &intent) {
  if (intent.intent_schema_id != kCajtucuExecutionIntentSchemaV1) {
    throw std::runtime_error(
        "[cajtucu.execution] unsupported execution intent schema");
  }
  if (intent.intent_id.empty() || intent.policy_id.empty() ||
      intent.base_reserve_node_id.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] intent id, policy id, and reserve node required");
  }
  detail::require_unique_node_ids(intent.node_ids, "intent node_id");
  if (std::count(intent.node_ids.begin(), intent.node_ids.end(),
                 intent.base_reserve_node_id) != 0) {
    throw std::runtime_error(
        "[cajtucu.execution] reserve node cannot appear in risky node_ids");
  }
  const auto A = static_cast<std::int64_t>(intent.node_ids.size());
  detail::require_nonnegative_vector(intent.current_weights, A,
                                     "current_weights", true);
  detail::require_nonnegative_vector(intent.target_weights, A, "target_weights",
                                     true);
  detail::require_finite(intent.current_base_reserve_weight,
                         "current_base_reserve_weight");
  detail::require_finite(intent.target_base_reserve_weight,
                         "target_base_reserve_weight");
  detail::require_finite(intent.equity_value_base, "equity_value_base");
  if (intent.current_base_reserve_weight < -1.0e-9 ||
      intent.target_base_reserve_weight < -1.0e-9 ||
      intent.equity_value_base <= 0.0) {
    throw std::runtime_error(
        "[cajtucu.execution] invalid reserve weight or equity");
  }
  const double current_sum =
      intent.current_weights.to(torch::kFloat64).sum().item<double>() +
      intent.current_base_reserve_weight;
  const double target_sum =
      intent.target_weights.to(torch::kFloat64).sum().item<double>() +
      intent.target_base_reserve_weight;
  if (current_sum > 1.0 + 1.0e-6 || target_sum > 1.0 + 1.0e-6) {
    throw std::runtime_error(
        "[cajtucu.execution] risky weights plus reserve exceed one");
  }
}

inline void validate_execution_ledger(const execution_ledger_t &ledger) {
  if (ledger.ledger_schema_id != kCajtucuExecutionLedgerSchemaV1) {
    throw std::runtime_error(
        "[cajtucu.execution] unsupported execution ledger schema");
  }
  if (ledger.base_reserve_node_id.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] ledger reserve node is required");
  }
  detail::require_unique_node_ids(ledger.node_ids, "ledger node_id");
  if (std::count(ledger.node_ids.begin(), ledger.node_ids.end(),
                 ledger.base_reserve_node_id) != 0) {
    throw std::runtime_error(
        "[cajtucu.execution] ledger reserve node cannot be risky node");
  }
  const auto A = static_cast<std::int64_t>(ledger.node_ids.size());
  detail::require_nonnegative_vector(ledger.units, A, "ledger units", true);
  detail::require_nonnegative_vector(ledger.weights, A, "ledger weights", true);
  detail::require_nonnegative(ledger.base_reserve_units, "base_reserve_units");
  detail::require_finite(ledger.base_reserve_weight, "base_reserve_weight");
  detail::require_nonnegative(ledger.equity_value_base, "equity_value_base");
  detail::require_nonnegative(ledger.cumulative_fee_base,
                              "cumulative_fee_base");
  detail::require_nonnegative(ledger.cumulative_spread_cost_base,
                              "cumulative_spread_cost_base");
  detail::require_nonnegative(ledger.cumulative_slippage_base,
                              "cumulative_slippage_base");
  detail::require_nonnegative(ledger.cumulative_turnover_base,
                              "cumulative_turnover_base");
  if (ledger.base_reserve_weight < -1.0e-9 ||
      ledger.base_reserve_weight > 1.0 + 1.0e-9) {
    throw std::runtime_error(
        "[cajtucu.execution] ledger base reserve weight must be in [0,1]");
  }
}

[[nodiscard]] inline double
asset_price_base(const market_execution_state_t &market,
                 const std::string &asset_node_id,
                 const std::string &base_reserve_node_id) {
  validate_market_execution_state(market);
  const auto choice = detail::find_direct_base_reserve_edge(
      market.graph, asset_node_id, base_reserve_node_id);
  if (!choice.found) {
    throw std::runtime_error("[cajtucu.execution] no direct reserve edge for " +
                             asset_node_id);
  }
  const double edge_mid = market.edge_mid_price.to(torch::kFloat64)
                              .index({choice.edge_index})
                              .item<double>();
  if (choice.asset_is_edge_base) {
    return edge_mid;
  }
  return 1.0 / edge_mid;
}

[[nodiscard]] inline torch::Tensor
asset_price_base_vector(const market_execution_state_t &market,
                        const std::vector<node_id_t> &node_ids,
                        const std::string &base_reserve_node_id) {
  std::vector<double> prices;
  prices.reserve(node_ids.size());
  for (const auto &node_id : node_ids) {
    prices.push_back(asset_price_base(market, node_id, base_reserve_node_id));
  }
  return torch::tensor(prices, torch::TensorOptions().dtype(torch::kFloat64));
}

[[nodiscard]] inline execution_ledger_t
mark_to_market(execution_ledger_t ledger,
               const market_execution_state_t &market,
               timestamp_ms_t timestamp_ms) {
  validate_execution_ledger(ledger);
  auto prices = asset_price_base_vector(market, ledger.node_ids,
                                        ledger.base_reserve_node_id);
  auto units = ledger.units.to(torch::kFloat64);
  auto values = units * prices;
  const double risky_value = values.sum().item<double>();
  const double equity = ledger.base_reserve_units + risky_value;
  if (!std::isfinite(equity) || equity <= 0.0) {
    throw std::runtime_error(
        "[cajtucu.execution] mark-to-market equity must be positive");
  }
  ledger.timestamp_ms = timestamp_ms;
  ledger.weights = values / equity;
  ledger.base_reserve_weight = ledger.base_reserve_units / equity;
  ledger.equity_value_base = equity;
  validate_execution_ledger(ledger);
  return ledger;
}

inline void validate_execution_trace(const execution_trace_t &trace) {
  if (trace.trace_schema_id != kCajtucuExecutionTraceSchemaV1) {
    throw std::runtime_error(
        "[cajtucu.execution] unsupported execution trace schema");
  }
  if (trace.trace_id.empty() || trace.backend_id.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] trace id and backend id are required");
  }
  validate_execution_intent(trace.intent);
  validate_execution_ledger(trace.ledger_before);
  validate_execution_ledger(trace.ledger_after);
  detail::require_nonnegative(trace.requested_notional_base,
                              "requested_notional_base");
  detail::require_nonnegative(trace.routed_notional_base,
                              "routed_notional_base");
  detail::require_nonnegative(trace.total_fee_base, "total_fee_base");
  detail::require_nonnegative(trace.total_spread_cost_base,
                              "total_spread_cost_base");
  detail::require_nonnegative(trace.total_slippage_base, "total_slippage_base");
  detail::require_nonnegative(trace.total_transaction_cost_base,
                              "total_transaction_cost_base");
  detail::require_nonnegative(trace.turnover_weight, "turnover_weight");
  detail::require_finite(trace.ledger_intent_equity_difference_base,
                         "ledger_intent_equity_difference_base");
  if (trace.market_source_id.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] trace market_source_id is required");
  }
  if (trace.ledger_intent_equity_mismatch &&
      std::abs(trace.ledger_intent_equity_difference_base) <= 0.0) {
    throw std::runtime_error(
        "[cajtucu.execution] equity mismatch flag requires nonzero "
        "difference");
  }
  if (trace.rejected_fill_count < 0 || trace.partial_fill_count < 0) {
    throw std::runtime_error(
        "[cajtucu.execution] fill counters must be nonnegative");
  }
  if (trace.orders.size() != trace.fills.size()) {
    throw std::runtime_error(
        "[cajtucu.execution] trace orders and fills must align one-to-one");
  }
  std::int64_t rejected_count = 0;
  std::int64_t partial_count = 0;
  for (const auto &fill : trace.fills) {
    if (fill.status == fill_status_t::rejected) {
      ++rejected_count;
    } else if (fill.status == fill_status_t::partially_filled) {
      ++partial_count;
    }
  }
  if (rejected_count != trace.rejected_fill_count ||
      partial_count != trace.partial_fill_count) {
    throw std::runtime_error(
        "[cajtucu.execution] fill counters do not match trace fills");
  }
  if (trace.valid != trace.failures.empty()) {
    throw std::runtime_error(
        "[cajtucu.execution] trace valid flag must match failures");
  }
}

class execution_backend_iface_t {
public:
  virtual ~execution_backend_iface_t() = default;

  [[nodiscard]] virtual std::string backend_id() const = 0;

  [[nodiscard]] virtual execution_trace_t
  execute(const execution_intent_t &intent,
          const market_execution_state_t &market,
          const execution_ledger_t &ledger_before) const = 0;
};

} // namespace cuwacunu::cajtucu::execution
