// SPDX-License-Identifier: MIT
#pragma once

#include <sstream>

#include "cajtucu/execution/types.h"

namespace cuwacunu::cajtucu::execution {

namespace paper_detail {

[[nodiscard]] inline std::string
fallback_trace_id(const execution_intent_t &intent) {
  std::ostringstream out;
  out << "cajtucu_paper_trace:" << intent.environment_run_id << ":"
      << intent.episode_id << ":" << intent.anchor_key << ":"
      << intent.timestamp_ms;
  return out.str();
}

[[nodiscard]] inline execution_order_t
rejected_order(const execution_intent_t &intent, std::int64_t i,
               double delta_weight, double requested_notional,
               const std::string &reason) {
  execution_order_t order{};
  order.order_id = intent.intent_id + ":order:" + std::to_string(i);
  order.node_id = intent.node_ids[static_cast<std::size_t>(i)];
  order.settlement_node_id = intent.base_reserve_node_id;
  order.side =
      delta_weight >= 0.0 ? order_side_t::buy_asset : order_side_t::sell_asset;
  order.delta_weight = delta_weight;
  order.requested_notional_base = requested_notional;
  order.valid = false;
  order.reject_reason = reason;
  return order;
}

[[nodiscard]] inline paper_fill_t
rejected_fill(const execution_order_t &order) {
  paper_fill_t fill{};
  fill.order_id = order.order_id;
  fill.node_id = order.node_id;
  fill.edge_id = order.edge_id;
  fill.side = order.side;
  fill.status = fill_status_t::rejected;
  fill.reject_reason =
      order.reject_reason.empty() ? "rejected" : order.reject_reason;
  return fill;
}

inline void apply_fill_to_ledger(execution_ledger_t &ledger,
                                 const paper_fill_t &fill, double eps) {
  if (fill.status == fill_status_t::rejected) {
    return;
  }
  validate_execution_ledger(ledger);
  auto units = ledger.units.to(torch::kFloat64).clone();
  const auto it =
      std::find(ledger.node_ids.begin(), ledger.node_ids.end(), fill.node_id);
  if (it == ledger.node_ids.end()) {
    throw std::runtime_error(
        "[cajtucu.paper] fill node is not present in ledger");
  }
  const auto i = static_cast<std::int64_t>(it - ledger.node_ids.begin());
  const double old_units = units.index({i}).item<double>();
  if (fill.side == order_side_t::buy_asset) {
    units.index_put_({i}, old_units + fill.filled_quantity_asset);
    ledger.base_reserve_units -= fill.gross_notional_base + fill.fee_base;
  } else {
    if (old_units + eps < fill.filled_quantity_asset) {
      throw std::runtime_error("[cajtucu.paper] sell fill exceeds units");
    }
    units.index_put_({i},
                     std::max(0.0, old_units - fill.filled_quantity_asset));
    ledger.base_reserve_units += fill.gross_notional_base - fill.fee_base;
  }
  ledger.units = std::move(units);
  ledger.cumulative_fee_base += fill.fee_base;
  ledger.cumulative_spread_cost_base += fill.spread_cost_base;
  ledger.cumulative_slippage_base += fill.slippage_base;
  ledger.cumulative_turnover_base += fill.gross_notional_base;
  ++ledger.fill_count;
}

} // namespace paper_detail

class paper_execution_backend_t final : public execution_backend_iface_t {
public:
  explicit paper_execution_backend_t(paper_execution_options_t options = {})
      : options_(options) {
    validate_paper_execution_options(options_);
  }

  [[nodiscard]] std::string backend_id() const override {
    return kCajtucuPaperBackendIdV1;
  }

  [[nodiscard]] execution_trace_t
  execute(const execution_intent_t &intent,
          const market_execution_state_t &market,
          const execution_ledger_t &ledger_before) const override {
    validate_paper_execution_options(options_);
    validate_execution_intent(intent);
    validate_market_execution_state(market);
    validate_execution_ledger(ledger_before);
    if (ledger_before.node_ids != intent.node_ids ||
        ledger_before.base_reserve_node_id != intent.base_reserve_node_id) {
      throw std::runtime_error(
          "[cajtucu.paper] ledger and intent universes must match");
    }

    const auto A = static_cast<std::int64_t>(intent.node_ids.size());
    const auto E = market.graph.num_edges();
    auto tensor_options = torch::TensorOptions().dtype(torch::kFloat64);
    auto current = intent.current_weights.to(tensor_options);
    auto target = intent.target_weights.to(tensor_options);
    auto delta = target - current;
    auto fee =
        detail::vector_or_full(market.edge_fee_rate, E, 0.0, tensor_options)
            .clamp_min(0.0);
    auto spread =
        detail::vector_or_full(market.edge_spread_rate, E, 0.0, tensor_options)
            .clamp_min(0.0);
    auto slippage = detail::vector_or_full(market.edge_slippage_rate, E, 0.0,
                                           tensor_options)
                        .clamp_min(0.0);
    auto min_notional =
        detail::vector_or_full(market.min_notional_base, E, 0.0, tensor_options)
            .clamp_min(0.0);
    auto max_notional =
        detail::vector_or_full(market.max_notional_base, E,
                               std::numeric_limits<double>::infinity(),
                               tensor_options)
            .clamp_min(0.0);
    auto tradable = detail::bool_vector_or_true(market.edge_tradable_mask, E,
                                                torch::Device(torch::kCPU));

    execution_trace_t trace{};
    trace.trace_id = intent.intent_id.empty()
                         ? paper_detail::fallback_trace_id(intent)
                         : "trace:" + intent.intent_id;
    trace.backend_id = backend_id();
    trace.intent = intent;
    trace.market_source_id = market.market_source_id;
    trace.synthetic_direct_edges = market.synthetic_direct_edges;
    trace.ledger_before = ledger_before;
    trace.ledger_after = ledger_before;
    if (trace.synthetic_direct_edges) {
      trace.warnings.push_back("synthetic_direct_edges_market_source");
      if (!options_.allow_synthetic_direct_edges) {
        trace.failures.push_back("synthetic_direct_edges_disallowed");
        trace.valid = false;
        validate_execution_trace(trace);
        return trace;
      }
    }
    bool route_support_complete = true;
    for (std::int64_t i = 0; i < A; ++i) {
      const auto &node_id = intent.node_ids[static_cast<std::size_t>(i)];
      const auto choice = detail::find_direct_base_reserve_edge(
          market.graph, node_id, intent.base_reserve_node_id);
      if (choice.found) {
        continue;
      }
      route_support_complete = false;
      const double d = delta.index({i}).item<double>();
      const double requested_notional =
          std::abs(d) * std::max(0.0, ledger_before.equity_value_base);
      auto order = paper_detail::rejected_order(
          intent, i, d, requested_notional, "missing_direct_reserve_edge");
      trace.orders.push_back(order);
      trace.fills.push_back(paper_detail::rejected_fill(order));
      ++trace.rejected_fill_count;
      trace.requested_notional_base += requested_notional;
      trace.turnover_weight += std::abs(d);
      trace.warnings.push_back("missing direct reserve edge for " + node_id);
      trace.failures.push_back("missing_direct_reserve_edge:" + node_id);
    }
    if (!route_support_complete) {
      trace.total_transaction_cost_base = 0.0;
      trace.ledger_intent_equity_difference_base =
          trace.ledger_before.equity_value_base - intent.equity_value_base;
      trace.ledger_intent_equity_mismatch =
          std::abs(trace.ledger_intent_equity_difference_base) >
          options_.equity_mismatch_tolerance *
              std::max(1.0, std::abs(trace.ledger_before.equity_value_base));
      if (trace.ledger_intent_equity_mismatch) {
        trace.warnings.push_back("ledger_intent_equity_mismatch");
      }
      trace.valid = false;
      validate_execution_trace(trace);
      return trace;
    }

    trace.ledger_before =
        mark_to_market(ledger_before, market, ledger_before.timestamp_ms);
    trace.ledger_after = trace.ledger_before;
    trace.ledger_intent_equity_difference_base =
        trace.ledger_before.equity_value_base - intent.equity_value_base;
    trace.ledger_intent_equity_mismatch =
        std::abs(trace.ledger_intent_equity_difference_base) >
        options_.equity_mismatch_tolerance *
            std::max(1.0, std::abs(trace.ledger_before.equity_value_base));
    if (trace.ledger_intent_equity_mismatch) {
      trace.warnings.push_back("ledger_intent_equity_mismatch");
    }
    const double relative_equity_mismatch =
        std::abs(trace.ledger_intent_equity_difference_base) /
        std::max(1.0, std::abs(trace.ledger_before.equity_value_base));
    if (relative_equity_mismatch > options_.equity_mismatch_fail_tolerance) {
      trace.failures.push_back("ledger_intent_equity_mismatch_exceeds_limit");
      trace.valid = false;
      validate_execution_trace(trace);
      return trace;
    }
    const double execution_equity_base = trace.ledger_before.equity_value_base;

    for (std::int64_t i = 0; i < A; ++i) {
      const double d = delta.index({i}).item<double>();
      if (d >= -options_.min_delta_weight) {
        continue;
      }
      const double requested_notional = std::abs(d) * execution_equity_base;
      const auto choice = detail::find_direct_base_reserve_edge(
          market.graph, intent.node_ids[static_cast<std::size_t>(i)],
          intent.base_reserve_node_id);
      const auto e = choice.edge_index;
      const double spread_rate = spread.index({e}).item<double>();
      const double slippage_rate = slippage.index({e}).item<double>();
      const double execution_adjustment = 0.5 * spread_rate + slippage_rate;
      if (execution_adjustment < 1.0) {
        continue;
      }
      auto order = paper_detail::rejected_order(
          intent, i, d, requested_notional,
          "invalid_sell_price_after_spread_slippage");
      const auto edge = market.graph.directed_edge(e);
      order.edge_id = edge.edge_id;
      order.edge_base_node_id = edge.base_node_id;
      order.edge_quote_node_id = edge.quote_node_id;
      trace.orders.push_back(order);
      trace.fills.push_back(paper_detail::rejected_fill(order));
      ++trace.rejected_fill_count;
      trace.requested_notional_base += requested_notional;
      trace.turnover_weight += std::abs(d);
      trace.warnings.push_back("invalid sell price for " + order.node_id);
      trace.failures.push_back("invalid_execution_cost_model:" + order.node_id);
    }
    if (!trace.failures.empty()) {
      trace.valid = false;
      validate_execution_trace(trace);
      return trace;
    }

    std::vector<std::int64_t> execution_indices;
    execution_indices.reserve(static_cast<std::size_t>(A));
    for (std::int64_t i = 0; i < A; ++i) {
      const double d = delta.index({i}).item<double>();
      if (d < -options_.min_delta_weight) {
        execution_indices.push_back(i);
      }
    }
    for (std::int64_t i = 0; i < A; ++i) {
      const double d = delta.index({i}).item<double>();
      if (d > options_.min_delta_weight) {
        execution_indices.push_back(i);
      }
    }

    for (const auto i : execution_indices) {
      const double d = delta.index({i}).item<double>();
      const double requested_notional = std::abs(d) * execution_equity_base;
      trace.turnover_weight += std::abs(d);
      trace.requested_notional_base += requested_notional;
      const auto choice = detail::find_direct_base_reserve_edge(
          market.graph, intent.node_ids[static_cast<std::size_t>(i)],
          intent.base_reserve_node_id);
      if (!choice.found) {
        auto order = paper_detail::rejected_order(
            intent, i, d, requested_notional, "no_direct_market_edge");
        trace.orders.push_back(order);
        trace.fills.push_back(paper_detail::rejected_fill(order));
        ++trace.rejected_fill_count;
        trace.warnings.push_back("no direct market edge for " + order.node_id);
        continue;
      }

      if (!tradable.index({choice.edge_index}).item<bool>()) {
        auto order = paper_detail::rejected_order(
            intent, i, d, requested_notional, "edge_not_tradable");
        const auto edge = market.graph.directed_edge(choice.edge_index);
        order.edge_id = edge.edge_id;
        order.edge_base_node_id = edge.base_node_id;
        order.edge_quote_node_id = edge.quote_node_id;
        trace.orders.push_back(order);
        trace.fills.push_back(paper_detail::rejected_fill(order));
        ++trace.rejected_fill_count;
        trace.warnings.push_back("edge not tradable for " + order.node_id);
        continue;
      }

      const auto e = choice.edge_index;
      const auto edge = market.graph.directed_edge(e);
      const double min_n = min_notional.index({e}).item<double>();
      if (options_.enforce_min_notional && requested_notional < min_n) {
        auto order = paper_detail::rejected_order(
            intent, i, d, requested_notional, "below_min_notional");
        order.edge_id = edge.edge_id;
        order.edge_base_node_id = edge.base_node_id;
        order.edge_quote_node_id = edge.quote_node_id;
        trace.orders.push_back(order);
        trace.fills.push_back(paper_detail::rejected_fill(order));
        ++trace.rejected_fill_count;
        trace.warnings.push_back("below min notional for " + order.node_id);
        continue;
      }

      const double fee_rate = fee.index({e}).item<double>();
      const double spread_rate = spread.index({e}).item<double>();
      const double slippage_rate = slippage.index({e}).item<double>();
      const double edge_mid =
          market.edge_mid_price.to(torch::kFloat64).index({e}).item<double>();
      const double asset_mid_base =
          choice.asset_is_edge_base ? edge_mid : 1.0 / edge_mid;
      const bool buy = d > 0.0;
      const double execution_adjustment = 0.5 * spread_rate + slippage_rate;
      const double fill_price =
          buy ? asset_mid_base * (1.0 + execution_adjustment)
              : asset_mid_base * (1.0 - execution_adjustment);

      double routed_notional = requested_notional;
      bool partial_fill = false;
      const double max_n = max_notional.index({e}).item<double>();
      if (std::isfinite(max_n) && routed_notional > max_n) {
        if (!options_.allow_partial_fills) {
          auto order = paper_detail::rejected_order(
              intent, i, d, requested_notional, "above_max_notional");
          const auto edge = market.graph.directed_edge(e);
          order.edge_id = edge.edge_id;
          order.edge_base_node_id = edge.base_node_id;
          order.edge_quote_node_id = edge.quote_node_id;
          trace.orders.push_back(order);
          trace.fills.push_back(paper_detail::rejected_fill(order));
          ++trace.rejected_fill_count;
          trace.warnings.push_back("above max notional for " + order.node_id);
          continue;
        }
        routed_notional = max_n;
        partial_fill = true;
      }

      auto order = execution_order_t{};
      order.order_id = intent.intent_id + ":order:" + std::to_string(i);
      order.node_id = intent.node_ids[static_cast<std::size_t>(i)];
      order.settlement_node_id = intent.base_reserve_node_id;
      order.edge_id = edge.edge_id;
      order.edge_base_node_id = edge.base_node_id;
      order.edge_quote_node_id = edge.quote_node_id;
      order.side = buy ? order_side_t::buy_asset : order_side_t::sell_asset;
      order.delta_weight = d;
      order.requested_notional_base = requested_notional;
      order.fill_price_base = fill_price;

      const double units_available =
          trace.ledger_after.units.to(torch::kFloat64)
              .index({i})
              .item<double>();
      if (buy) {
        const double max_affordable_notional =
            options_.allow_negative_base_reserve
                ? routed_notional
                : trace.ledger_after.base_reserve_units /
                      std::max(options_.eps, 1.0 + fee_rate);
        if (routed_notional > max_affordable_notional + options_.eps) {
          if (!options_.allow_partial_fills ||
              max_affordable_notional <= options_.eps ||
              (options_.enforce_min_notional &&
               max_affordable_notional < min_n)) {
            order.valid = false;
            order.reject_reason = "insufficient_base_reserve";
            trace.orders.push_back(order);
            trace.fills.push_back(paper_detail::rejected_fill(order));
            ++trace.rejected_fill_count;
            trace.warnings.push_back("insufficient reserve for " +
                                     order.node_id);
            continue;
          }
          routed_notional = max_affordable_notional;
          partial_fill = true;
        }
      } else {
        const double requested_quantity = routed_notional / fill_price;
        if (requested_quantity > units_available + options_.eps) {
          if (!options_.allow_partial_fills ||
              units_available <= options_.eps) {
            order.valid = false;
            order.reject_reason = "insufficient_asset_units";
            trace.orders.push_back(order);
            trace.fills.push_back(paper_detail::rejected_fill(order));
            ++trace.rejected_fill_count;
            trace.warnings.push_back("insufficient units for " + order.node_id);
            continue;
          }
          routed_notional = units_available * fill_price;
          partial_fill = true;
        }
      }

      order.routed_notional_base = routed_notional;
      order.quantity_asset = routed_notional / fill_price;
      order.fee_base = routed_notional * fee_rate;
      order.spread_cost_base = routed_notional * 0.5 * spread_rate;
      order.slippage_base = routed_notional * slippage_rate;

      paper_fill_t fill{};
      fill.order_id = order.order_id;
      fill.node_id = order.node_id;
      fill.edge_id = order.edge_id;
      fill.side = order.side;
      fill.status =
          partial_fill || routed_notional + options_.eps < requested_notional
              ? fill_status_t::partially_filled
              : fill_status_t::filled;
      fill.requested_quantity_asset = requested_notional / fill_price;
      fill.filled_quantity_asset = order.quantity_asset;
      fill.gross_notional_base = routed_notional;
      fill.fill_price_base = fill_price;
      fill.fee_base = order.fee_base;
      fill.spread_cost_base = order.spread_cost_base;
      fill.slippage_base = order.slippage_base;

      trace.orders.push_back(order);
      trace.fills.push_back(fill);
      if (fill.status == fill_status_t::partially_filled) {
        ++trace.partial_fill_count;
      }
      paper_detail::apply_fill_to_ledger(trace.ledger_after, fill,
                                         options_.eps);
      trace.routed_notional_base += routed_notional;
      trace.total_fee_base += fill.fee_base;
      trace.total_spread_cost_base += fill.spread_cost_base;
      trace.total_slippage_base += fill.slippage_base;
    }

    trace.total_transaction_cost_base = trace.total_fee_base +
                                        trace.total_spread_cost_base +
                                        trace.total_slippage_base;
    trace.ledger_after =
        mark_to_market(trace.ledger_after, market, intent.timestamp_ms);
    trace.valid = trace.failures.empty();
    validate_execution_trace(trace);
    return trace;
  }

private:
  paper_execution_options_t options_{};
};

} // namespace cuwacunu::cajtucu::execution
