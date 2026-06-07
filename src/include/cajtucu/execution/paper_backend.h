// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <sstream>
#include <vector>

#include "cajtucu/execution/types.h"

namespace cuwacunu::cajtucu::execution {

namespace paper_detail {

struct rebalance_leg_t {
  std::int64_t index{-1};
  double remaining_notional_numeraire{0.0};
  double delta_weight{0.0};
};

[[nodiscard]] inline std::string
fallback_trace_id(const execution_intent_t &intent) {
  std::ostringstream out;
  out << "cajtucu_paper_trace:" << intent.environment_run_id << ":"
      << intent.episode_id << ":" << intent.anchor_key << ":"
      << intent.timestamp_ms;
  return out.str();
}

[[nodiscard]] inline std::int64_t
node_index_or_throw(const std::vector<node_id_t> &node_ids,
                    const node_id_t &node_id) {
  const auto it = std::find(node_ids.begin(), node_ids.end(), node_id);
  if (it == node_ids.end()) {
    throw std::runtime_error("[cajtucu.paper] node is not present in ledger: " +
                             node_id);
  }
  return static_cast<std::int64_t>(it - node_ids.begin());
}

[[nodiscard]] inline execution_order_t
rejected_order(const execution_intent_t &intent, const node_id_t &sell_node_id,
               const node_id_t &buy_node_id, double sell_delta_weight,
               double buy_delta_weight, double requested_notional_numeraire,
               const std::string &reason) {
  execution_order_t order{};
  order.order_id =
      intent.intent_id + ":order:" + std::to_string(intent.timestamp_ms) + ":" +
      sell_node_id + ":" + buy_node_id + ":" + std::to_string(reason.size());
  order.sell_node_id = sell_node_id;
  order.buy_node_id = buy_node_id;
  order.sell_delta_weight = sell_delta_weight;
  order.buy_delta_weight = buy_delta_weight;
  order.requested_notional_numeraire = requested_notional_numeraire;
  order.valid = false;
  order.reject_reason = reason;
  return order;
}

[[nodiscard]] inline paper_fill_t
rejected_fill(const execution_order_t &order) {
  paper_fill_t fill{};
  fill.order_id = order.order_id;
  fill.sell_node_id = order.sell_node_id;
  fill.buy_node_id = order.buy_node_id;
  fill.edge_id = order.edge_id;
  fill.status = fill_status_t::rejected;
  fill.reject_reason =
      order.reject_reason.empty() ? "rejected" : order.reject_reason;
  return fill;
}

[[nodiscard]] inline double
fill_price_buy_per_sell(const market_execution_state_t &market,
                        const detail::direct_edge_choice_t &choice,
                        double spread_rate, double slippage_rate) {
  const double mid = market.edge_mid_price.to(torch::kFloat64)
                         .index({choice.edge_index})
                         .item<double>();
  const double adjustment = 0.5 * spread_rate + slippage_rate;
  if (choice.from_is_edge_base) {
    return mid * (1.0 - adjustment);
  }
  return 1.0 / (mid * (1.0 + adjustment));
}

inline void apply_fill_to_ledger(execution_ledger_t &ledger,
                                 const paper_fill_t &fill, double eps) {
  if (fill.status == fill_status_t::rejected) {
    return;
  }
  validate_execution_ledger(ledger);
  auto units = ledger.units.to(torch::kFloat64).clone();
  const auto sell_i = node_index_or_throw(ledger.node_ids, fill.sell_node_id);
  const auto buy_i = node_index_or_throw(ledger.node_ids, fill.buy_node_id);
  const double sell_units = units.index({sell_i}).item<double>();
  if (sell_units + eps < fill.filled_sell_quantity) {
    throw std::runtime_error("[cajtucu.paper] direct-pair sell exceeds units");
  }
  units.index_put_({sell_i},
                   std::max(0.0, sell_units - fill.filled_sell_quantity));
  const double buy_units = units.index({buy_i}).item<double>();
  units.index_put_({buy_i}, buy_units + fill.filled_buy_quantity);
  ledger.units = std::move(units);
  ledger.cumulative_fee_numeraire += fill.fee_numeraire;
  ledger.cumulative_spread_cost_numeraire += fill.spread_cost_numeraire;
  ledger.cumulative_slippage_numeraire += fill.slippage_numeraire;
  ledger.cumulative_turnover_numeraire += fill.gross_notional_numeraire;
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
        ledger_before.accounting_numeraire_node_id !=
            intent.accounting_numeraire_node_id) {
      throw std::runtime_error(
          "[cajtucu.paper] ledger and intent universes must match");
    }

    const auto M = static_cast<std::int64_t>(intent.node_ids.size());
    const auto E = market.graph.num_edges();
    const auto tensor_options = torch::TensorOptions().dtype(torch::kFloat64);
    const auto current = intent.current_weights.to(tensor_options);
    const auto target = intent.target_weights.to(tensor_options);
    const auto delta = target - current;
    const auto fee =
        detail::vector_or_full(market.edge_fee_rate, E, 0.0, tensor_options)
            .clamp_min(0.0);
    const auto spread =
        detail::vector_or_full(market.edge_spread_rate, E, 0.0, tensor_options)
            .clamp_min(0.0);
    const auto slippage = detail::vector_or_full(market.edge_slippage_rate, E,
                                                 0.0, tensor_options)
                              .clamp_min(0.0);
    const auto min_notional =
        detail::vector_or_full(market.min_notional_numeraire, E, 0.0,
                               tensor_options)
            .clamp_min(0.0);
    const auto max_notional =
        detail::vector_or_full(market.max_notional_numeraire, E,
                               std::numeric_limits<double>::infinity(),
                               tensor_options)
            .clamp_min(0.0);
    const auto tradable = detail::bool_vector_or_true(
        market.edge_tradable_mask, E, torch::Device(torch::kCPU));

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

    trace.ledger_before =
        mark_to_market(ledger_before, market, ledger_before.timestamp_ms);
    trace.ledger_after = trace.ledger_before;
    trace.ledger_intent_equity_difference_numeraire =
        trace.ledger_before.equity_value_numeraire -
        intent.equity_value_numeraire;
    trace.ledger_intent_equity_mismatch =
        std::abs(trace.ledger_intent_equity_difference_numeraire) >
        options_.equity_mismatch_tolerance *
            std::max(1.0, std::abs(trace.ledger_before.equity_value_numeraire));
    if (trace.ledger_intent_equity_mismatch) {
      trace.warnings.push_back("ledger_intent_equity_mismatch");
    }
    const double relative_equity_mismatch =
        std::abs(trace.ledger_intent_equity_difference_numeraire) /
        std::max(1.0, std::abs(trace.ledger_before.equity_value_numeraire));
    if (relative_equity_mismatch > options_.equity_mismatch_fail_tolerance) {
      trace.failures.push_back("ledger_intent_equity_mismatch_exceeds_limit");
      trace.valid = false;
      validate_execution_trace(trace);
      return trace;
    }

    const double execution_equity = trace.ledger_before.equity_value_numeraire;
    const auto prices = node_price_numeraire_vector(
        market, intent.node_ids, intent.accounting_numeraire_node_id);

    std::vector<paper_detail::rebalance_leg_t> sells;
    std::vector<paper_detail::rebalance_leg_t> buys;
    sells.reserve(static_cast<std::size_t>(M));
    buys.reserve(static_cast<std::size_t>(M));
    for (std::int64_t i = 0; i < M; ++i) {
      const double d = delta.index({i}).item<double>();
      if (d < -options_.min_delta_weight) {
        sells.push_back(
            {.index = i,
             .remaining_notional_numeraire = std::abs(d) * execution_equity,
             .delta_weight = d});
      } else if (d > options_.min_delta_weight) {
        buys.push_back({.index = i,
                        .remaining_notional_numeraire = d * execution_equity,
                        .delta_weight = d});
      }
    }
    const auto by_notional_then_node =
        [&](const paper_detail::rebalance_leg_t &lhs,
            const paper_detail::rebalance_leg_t &rhs) {
          if (std::abs(lhs.remaining_notional_numeraire -
                       rhs.remaining_notional_numeraire) > options_.eps) {
            return lhs.remaining_notional_numeraire >
                   rhs.remaining_notional_numeraire;
          }
          return intent.node_ids[static_cast<std::size_t>(lhs.index)] <
                 intent.node_ids[static_cast<std::size_t>(rhs.index)];
        };
    std::sort(sells.begin(), sells.end(), by_notional_then_node);
    std::sort(buys.begin(), buys.end(), by_notional_then_node);

    std::size_t order_seq = 0;
    struct pair_order_result_t {
      bool missing_direct_pair{false};
      bool fill_executed{false};
      bool rejected{false};
      double requested_notional_numeraire{0.0};
      double routed_notional_numeraire{0.0};
      double filled_buy_quantity{0.0};
    };

    const auto append_rejected_order =
        [&](const node_id_t &sell_node, const node_id_t &buy_node,
            double sell_delta_weight, double buy_delta_weight,
            double requested_notional, const std::string &reason,
            const detail::direct_edge_choice_t *choice) {
          auto order = paper_detail::rejected_order(
              intent, sell_node, buy_node, sell_delta_weight, buy_delta_weight,
              requested_notional, reason);
          order.order_id =
              intent.intent_id + ":order:" + std::to_string(order_seq++);
          if (choice != nullptr && choice->found) {
            const auto edge = market.graph.directed_edge(choice->edge_index);
            order.edge_id = edge.edge_id;
            order.edge_base_node_id = edge.base_node_id;
            order.edge_quote_node_id = edge.quote_node_id;
          }
          trace.orders.push_back(order);
          trace.fills.push_back(paper_detail::rejected_fill(order));
          ++trace.rejected_fill_count;
          trace.requested_notional_numeraire += requested_notional;
          trace.turnover_weight +=
              requested_notional / std::max(options_.eps, execution_equity);
        };

    const auto execute_direct_pair_order = [&](const node_id_t &sell_node,
                                               const node_id_t &buy_node,
                                               double sell_delta_weight,
                                               double buy_delta_weight,
                                               double requested_notional) {
      pair_order_result_t result{};
      result.requested_notional_numeraire = requested_notional;
      const auto choice =
          detail::find_direct_pair_edge(market.graph, sell_node, buy_node);
      if (!choice.found) {
        result.missing_direct_pair = true;
        return result;
      }

      const auto edge = market.graph.directed_edge(choice.edge_index);
      const auto sell_index =
          paper_detail::node_index_or_throw(intent.node_ids, sell_node);
      const auto buy_index =
          paper_detail::node_index_or_throw(intent.node_ids, buy_node);
      const double min_n =
          min_notional.index({choice.edge_index}).item<double>();
      if (options_.enforce_min_notional && requested_notional < min_n) {
        append_rejected_order(sell_node, buy_node, sell_delta_weight,
                              buy_delta_weight, requested_notional,
                              "below_min_notional", &choice);
        result.rejected = true;
        return result;
      }
      if (!tradable.index({choice.edge_index}).item<bool>()) {
        append_rejected_order(sell_node, buy_node, sell_delta_weight,
                              buy_delta_weight, requested_notional,
                              "edge_not_tradable", &choice);
        result.rejected = true;
        return result;
      }

      double routed_notional = requested_notional;
      bool partial_fill = false;
      const double max_n =
          max_notional.index({choice.edge_index}).item<double>();
      if (std::isfinite(max_n) && routed_notional > max_n) {
        if (!options_.allow_partial_fills) {
          append_rejected_order(sell_node, buy_node, sell_delta_weight,
                                buy_delta_weight, requested_notional,
                                "above_max_notional", &choice);
          result.rejected = true;
          return result;
        }
        routed_notional = max_n;
        partial_fill = true;
      }

      const double sell_price_numeraire =
          prices.index({sell_index}).item<double>();
      const double buy_price_numeraire =
          prices.index({buy_index}).item<double>();
      double sell_quantity = routed_notional / sell_price_numeraire;
      const double units_available =
          trace.ledger_after.units.to(torch::kFloat64)
              .index({sell_index})
              .item<double>();
      if (sell_quantity > units_available + options_.eps) {
        if (!options_.allow_partial_fills || units_available <= options_.eps) {
          append_rejected_order(sell_node, buy_node, sell_delta_weight,
                                buy_delta_weight, requested_notional,
                                "insufficient_sell_units", &choice);
          result.rejected = true;
          return result;
        }
        sell_quantity = units_available;
        routed_notional = sell_quantity * sell_price_numeraire;
        partial_fill = true;
      }

      const double fee_rate = fee.index({choice.edge_index}).item<double>();
      const double spread_rate =
          spread.index({choice.edge_index}).item<double>();
      const double slippage_rate =
          slippage.index({choice.edge_index}).item<double>();
      const double fill_price = paper_detail::fill_price_buy_per_sell(
          market, choice, spread_rate, slippage_rate);
      if (!std::isfinite(fill_price) || fill_price <= options_.eps) {
        append_rejected_order(sell_node, buy_node, sell_delta_weight,
                              buy_delta_weight, requested_notional,
                              "invalid_direct_pair_fill_price", &choice);
        trace.failures.push_back("invalid_execution_cost_model:" + sell_node +
                                 "->" + buy_node);
        result.rejected = true;
        return result;
      }

      const double gross_buy_quantity = sell_quantity * fill_price;
      const double fee_numeraire = routed_notional * fee_rate;
      const double fee_buy_quantity = fee_numeraire / buy_price_numeraire;
      const double filled_buy_quantity =
          std::max(0.0, gross_buy_quantity - fee_buy_quantity);
      if (filled_buy_quantity <= options_.eps) {
        append_rejected_order(sell_node, buy_node, sell_delta_weight,
                              buy_delta_weight, requested_notional,
                              "fee_exceeds_direct_pair_output", &choice);
        result.rejected = true;
        return result;
      }

      execution_order_t order{};
      order.order_id =
          intent.intent_id + ":order:" + std::to_string(order_seq++);
      order.sell_node_id = sell_node;
      order.buy_node_id = buy_node;
      order.edge_id = edge.edge_id;
      order.edge_base_node_id = edge.base_node_id;
      order.edge_quote_node_id = edge.quote_node_id;
      order.sell_delta_weight = sell_delta_weight;
      order.buy_delta_weight = buy_delta_weight;
      order.requested_notional_numeraire = requested_notional;
      order.routed_notional_numeraire = routed_notional;
      order.sell_quantity = sell_quantity;
      order.buy_quantity = filled_buy_quantity;
      order.fill_price_buy_per_sell = fill_price;
      order.fee_numeraire = fee_numeraire;
      order.spread_cost_numeraire = routed_notional * 0.5 * spread_rate;
      order.slippage_numeraire = routed_notional * slippage_rate;

      paper_fill_t fill{};
      fill.order_id = order.order_id;
      fill.sell_node_id = sell_node;
      fill.buy_node_id = buy_node;
      fill.edge_id = edge.edge_id;
      fill.status =
          partial_fill || routed_notional + options_.eps < requested_notional
              ? fill_status_t::partially_filled
              : fill_status_t::filled;
      fill.requested_sell_quantity = requested_notional / sell_price_numeraire;
      fill.filled_sell_quantity = sell_quantity;
      fill.filled_buy_quantity = filled_buy_quantity;
      fill.gross_notional_numeraire = routed_notional;
      fill.fill_price_buy_per_sell = fill_price;
      fill.fee_numeraire = order.fee_numeraire;
      fill.spread_cost_numeraire = order.spread_cost_numeraire;
      fill.slippage_numeraire = order.slippage_numeraire;

      trace.orders.push_back(order);
      trace.fills.push_back(fill);
      if (fill.status == fill_status_t::partially_filled) {
        ++trace.partial_fill_count;
      }
      paper_detail::apply_fill_to_ledger(trace.ledger_after, fill,
                                         options_.eps);
      trace.requested_notional_numeraire += requested_notional;
      trace.turnover_weight +=
          requested_notional / std::max(options_.eps, execution_equity);
      trace.routed_notional_numeraire += routed_notional;
      trace.total_fee_numeraire += fill.fee_numeraire;
      trace.total_spread_cost_numeraire += fill.spread_cost_numeraire;
      trace.total_slippage_numeraire += fill.slippage_numeraire;

      result.fill_executed = true;
      result.routed_notional_numeraire = routed_notional;
      result.filled_buy_quantity = filled_buy_quantity;
      return result;
    };

    const auto record_missing_direct_pair =
        [&](const node_id_t &sell_node, const node_id_t &buy_node,
            double sell_delta_weight, double buy_delta_weight,
            double requested_notional, const std::string &reason,
            bool invalid_trace) {
          append_rejected_order(sell_node, buy_node, sell_delta_weight,
                                buy_delta_weight, requested_notional, reason,
                                nullptr);
          ++trace.missing_direct_pair_count;
          trace.warnings.push_back("missing direct pair for " + sell_node +
                                   "->" + buy_node);
          if (invalid_trace) {
            trace.failures.push_back("missing_direct_pair:" + sell_node + "->" +
                                     buy_node);
          }
        };

    for (auto &sell : sells) {
      for (auto &buy : buys) {
        if (sell.remaining_notional_numeraire <= options_.eps) {
          break;
        }
        if (buy.remaining_notional_numeraire <= options_.eps) {
          continue;
        }
        const auto &sell_node =
            intent.node_ids[static_cast<std::size_t>(sell.index)];
        const auto &buy_node =
            intent.node_ids[static_cast<std::size_t>(buy.index)];
        double requested_notional = std::min(sell.remaining_notional_numeraire,
                                             buy.remaining_notional_numeraire);
        if (requested_notional <= options_.eps) {
          continue;
        }

        const auto direct_result =
            execute_direct_pair_order(sell_node, buy_node, sell.delta_weight,
                                      buy.delta_weight, requested_notional);
        if (!direct_result.missing_direct_pair) {
          sell.remaining_notional_numeraire -= requested_notional;
          buy.remaining_notional_numeraire -= requested_notional;
          continue;
        }

        if (options_.missing_direct_pair_policy ==
            missing_direct_pair_policy_t::invalid_trace) {
          record_missing_direct_pair(sell_node, buy_node, sell.delta_weight,
                                     buy.delta_weight, requested_notional,
                                     "missing_direct_pair", true);
          sell.remaining_notional_numeraire -= requested_notional;
          buy.remaining_notional_numeraire -= requested_notional;
          continue;
        }
        if (options_.missing_direct_pair_policy ==
            missing_direct_pair_policy_t::skip_pair_warn) {
          record_missing_direct_pair(sell_node, buy_node, sell.delta_weight,
                                     buy.delta_weight, requested_notional,
                                     "missing_direct_pair_skipped", false);
          sell.remaining_notional_numeraire -= requested_notional;
          buy.remaining_notional_numeraire -= requested_notional;
          continue;
        }

        ++trace.missing_direct_pair_count;
        trace.warnings.push_back(
            "missing_direct_pair_routed_via_accounting_numeraire:" + sell_node +
            "->" + buy_node);
        const auto &numeraire = intent.accounting_numeraire_node_id;
        const bool fallback_possible =
            sell_node != numeraire && buy_node != numeraire &&
            detail::find_direct_pair_edge(market.graph, sell_node, numeraire)
                .found &&
            detail::find_direct_pair_edge(market.graph, numeraire, buy_node)
                .found;
        if (!fallback_possible) {
          append_rejected_order(sell_node, buy_node, sell.delta_weight,
                                buy.delta_weight, requested_notional,
                                "missing_numeraire_fallback_pair", nullptr);
          trace.warnings.push_back(
              "missing accounting numeraire fallback for " + sell_node + "->" +
              buy_node);
          sell.remaining_notional_numeraire -= requested_notional;
          buy.remaining_notional_numeraire -= requested_notional;
          continue;
        }

        ++trace.numeraire_fallback_pair_count;
        const auto first_leg = execute_direct_pair_order(
            sell_node, numeraire, sell.delta_weight, 0.0, requested_notional);
        if (!first_leg.fill_executed) {
          trace.warnings.push_back("accounting numeraire fallback first leg "
                                   "did not fill for " +
                                   sell_node + "->" + buy_node);
          sell.remaining_notional_numeraire -= requested_notional;
          buy.remaining_notional_numeraire -= requested_notional;
          continue;
        }
        const auto second_leg = execute_direct_pair_order(
            numeraire, buy_node, 0.0, buy.delta_weight,
            first_leg.filled_buy_quantity);
        if (!second_leg.fill_executed) {
          trace.warnings.push_back("accounting numeraire fallback second leg "
                                   "did not fill for " +
                                   sell_node + "->" + buy_node);
        }
        sell.remaining_notional_numeraire -= requested_notional;
        buy.remaining_notional_numeraire -= requested_notional;
      }
    }

    trace.total_transaction_cost_numeraire = trace.total_fee_numeraire +
                                             trace.total_spread_cost_numeraire +
                                             trace.total_slippage_numeraire;
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
