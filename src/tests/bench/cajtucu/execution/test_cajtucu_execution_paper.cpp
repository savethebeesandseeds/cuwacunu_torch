#include "cajtucu/execution/assembly.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace exec = cuwacunu::cajtucu::execution;
namespace graph = cuwacunu::kikijyeba::topology::graph;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void close(double actual, double expected, double tolerance,
           const std::string &message) {
  if (std::abs(actual - expected) > tolerance) {
    throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                             " expected=" + std::to_string(expected));
  }
}

exec::market_execution_state_t market() {
  exec::market_execution_state_t out{};
  out.market_source_id = "fixture.edge_market_state";
  out.timestamp_ms = 100;
  out.graph.node_ids = {"BTC", "ETH", "USDT"};
  out.graph.edge_ids = {"BTC/USDT", "ETH/USDT"};
  out.graph.base_index = {0, 1};
  out.graph.quote_index = {2, 2};
  out.edge_mid_price = torch::tensor({100.0, 50.0}, torch::kFloat64);
  out.edge_fee_rate = torch::tensor({0.001, 0.001}, torch::kFloat64);
  out.edge_spread_rate = torch::tensor({0.002, 0.002}, torch::kFloat64);
  out.edge_slippage_rate = torch::tensor({0.003, 0.003}, torch::kFloat64);
  out.min_notional_base = torch::tensor({1.0, 1.0}, torch::kFloat64);
  out.max_notional_base = torch::tensor({1000.0, 1000.0}, torch::kFloat64);
  out.edge_tradable_mask = torch::tensor({true, true}, torch::kBool);
  exec::validate_market_execution_state(out);
  return out;
}

exec::market_execution_state_t reverse_market() {
  exec::market_execution_state_t out{};
  out.market_source_id = "fixture.reverse_edge_market_state";
  out.timestamp_ms = 100;
  out.graph.node_ids = {"USDT", "BTC", "ETH"};
  out.graph.edge_ids = {"USDT/BTC", "USDT/ETH"};
  out.graph.base_index = {0, 0};
  out.graph.quote_index = {1, 2};
  out.edge_mid_price = torch::tensor({0.01, 0.02}, torch::kFloat64);
  out.edge_fee_rate = torch::tensor({0.001, 0.001}, torch::kFloat64);
  out.edge_spread_rate = torch::tensor({0.002, 0.002}, torch::kFloat64);
  out.edge_slippage_rate = torch::tensor({0.003, 0.003}, torch::kFloat64);
  out.min_notional_base = torch::tensor({1.0, 1.0}, torch::kFloat64);
  out.max_notional_base = torch::tensor({1000.0, 1000.0}, torch::kFloat64);
  out.edge_tradable_mask = torch::tensor({true, true}, torch::kBool);
  exec::validate_market_execution_state(out);
  return out;
}

exec::market_execution_state_t market_without_eth_edge() {
  exec::market_execution_state_t out{};
  out.market_source_id = "fixture.missing_edge_market_state";
  out.timestamp_ms = 100;
  out.graph.node_ids = {"BTC", "ETH", "USDT"};
  out.graph.edge_ids = {"BTC/USDT"};
  out.graph.base_index = {0};
  out.graph.quote_index = {2};
  out.edge_mid_price = torch::tensor({100.0}, torch::kFloat64);
  out.edge_fee_rate = torch::tensor({0.001}, torch::kFloat64);
  out.edge_spread_rate = torch::tensor({0.002}, torch::kFloat64);
  out.edge_slippage_rate = torch::tensor({0.003}, torch::kFloat64);
  out.min_notional_base = torch::tensor({1.0}, torch::kFloat64);
  out.max_notional_base = torch::tensor({1000.0}, torch::kFloat64);
  out.edge_tradable_mask = torch::tensor({true}, torch::kBool);
  exec::validate_market_execution_state(out);
  return out;
}

exec::execution_ledger_t ledger(double base_reserve_units = 1000.0,
                                torch::Tensor units = torch::Tensor()) {
  exec::execution_ledger_t out{};
  out.timestamp_ms = 100;
  out.base_reserve_node_id = "USDT";
  out.node_ids = {"BTC", "ETH"};
  out.units = units.defined() ? units.to(torch::kFloat64)
                              : torch::zeros({2}, torch::kFloat64);
  out.weights = torch::zeros({2}, torch::kFloat64);
  out.base_reserve_units = base_reserve_units;
  out.base_reserve_weight = 1.0;
  out.equity_value_base = base_reserve_units;
  exec::validate_execution_ledger(out);
  return out;
}

exec::execution_intent_t intent(torch::Tensor current_weights,
                                torch::Tensor target_weights,
                                double current_reserve, double target_reserve,
                                double equity = 1000.0) {
  exec::execution_intent_t out{};
  out.intent_id = "intent_fixture";
  out.action_id = "action_fixture";
  out.policy_id = "policy_fixture";
  out.method_id = "method_fixture";
  out.runtime_run_id = "runtime_fixture";
  out.environment_run_id = "environment_fixture";
  out.episode_id = "episode_fixture";
  out.anchor_key = "anchor_fixture";
  out.timestamp_ms = 101;
  out.node_ids = {"BTC", "ETH"};
  out.base_reserve_node_id = "USDT";
  out.current_weights = current_weights.to(torch::kFloat64);
  out.target_weights = target_weights.to(torch::kFloat64);
  out.current_base_reserve_weight = current_reserve;
  out.target_base_reserve_weight = target_reserve;
  out.equity_value_base = equity;
  exec::validate_execution_intent(out);
  return out;
}

bool contains_warning(const exec::execution_trace_t &trace,
                      const std::string &warning) {
  return std::find(trace.warnings.begin(), trace.warnings.end(), warning) !=
         trace.warnings.end();
}

bool contains_failure_prefix(const exec::execution_trace_t &trace,
                             const std::string &prefix) {
  return std::any_of(trace.failures.begin(), trace.failures.end(),
                     [&](const std::string &failure) {
                       return failure.rfind(prefix, 0) == 0;
                     });
}

void test_buy_updates_ledger_and_trace() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      market(), ledger());
  exec::validate_execution_trace(trace);
  check(trace.backend_id == std::string(exec::kCajtucuPaperBackendIdV1),
        "paper backend id is stable");
  check(trace.orders.size() == 1 && trace.fills.size() == 1,
        "buy creates one order and fill");
  check(trace.rejected_fill_count == 0, "buy should not reject");
  close(trace.routed_notional_base, 500.0, 1.0e-9, "buy routed notional");
  close(trace.total_fee_base, 0.5, 1.0e-9, "buy fee");
  close(trace.total_spread_cost_base, 0.5, 1.0e-9, "buy spread cost");
  close(trace.total_slippage_base, 1.5, 1.0e-9, "buy slippage cost");
  close(trace.total_transaction_cost_base, 2.5, 1.0e-9, "buy total cost");
  close(trace.ledger_after.base_reserve_units, 499.5, 1.0e-9,
        "buy reserve after fee");
  check(trace.ledger_after.units.index({0}).item<double>() > 0.0,
        "buy increases BTC units");
}

void test_reverse_edge_orientation_prices_asset_in_reserve_units() {
  exec::paper_execution_backend_t backend{};
  const auto prices =
      exec::asset_price_base_vector(reverse_market(), {"BTC", "ETH"}, "USDT");
  close(prices.index({0}).item<double>(), 100.0, 1.0e-9,
        "reverse BTC asset price");
  close(prices.index({1}).item<double>(), 50.0, 1.0e-9,
        "reverse ETH asset price");
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      reverse_market(), ledger());
  exec::validate_execution_trace(trace);
  check(trace.orders.size() == 1, "reverse market creates one order");
  close(trace.orders[0].fill_price_base, 100.4, 1.0e-9,
        "reverse buy fill price uses inverted edge mid");
}

void test_sell_reduces_units() {
  exec::paper_execution_backend_t backend{};
  auto before = ledger(500.0, torch::tensor({5.0, 0.0}, torch::kFloat64));
  before = exec::mark_to_market(before, market(), 100);
  auto trace = backend.execute(
      intent(torch::tensor({0.5, 0.0}, torch::kFloat64),
             torch::tensor({0.25, 0.0}, torch::kFloat64), 0.5, 0.75),
      market(), before);
  exec::validate_execution_trace(trace);
  check(trace.orders.size() == 1 && trace.fills.size() == 1,
        "sell creates one order and fill");
  check(trace.fills[0].side == exec::order_side_t::sell_asset,
        "sell fill side is sell_asset");
  check(trace.ledger_after.units.index({0}).item<double>() <
            before.units.index({0}).item<double>(),
        "sell reduces BTC units");
  check(trace.ledger_after.base_reserve_units > before.base_reserve_units,
        "sell increases reserve units");
}

void test_sells_execute_before_buys_to_fund_rebalance() {
  exec::paper_execution_backend_t backend{};
  auto before = ledger(100.0, torch::tensor({10.0, 0.0}, torch::kFloat64));
  before = exec::mark_to_market(before, market(), 100);
  auto trace = backend.execute(
      intent(torch::tensor({0.9, 0.0}, torch::kFloat64),
             torch::tensor({0.2, 0.7}, torch::kFloat64), 0.1, 0.1, 1100.0),
      market(), before);
  exec::validate_execution_trace(trace);
  check(trace.valid, "sells-before-buys rebalance remains valid");
  check(trace.rejected_fill_count == 0,
        "sell proceeds fund the later buy without rejection");
  check(trace.orders.size() == 2, "rebalance emits sell and buy orders");
  check(trace.orders[0].node_id == "BTC" &&
            trace.orders[0].side == exec::order_side_t::sell_asset,
        "first execution order sells BTC");
  check(trace.orders[1].node_id == "ETH" &&
            trace.orders[1].side == exec::order_side_t::buy_asset,
        "second execution order buys ETH");
  check(trace.ledger_after.base_reserve_units < before.base_reserve_units,
        "rebalance spends reserve after sell-funded buy");
}

void test_nontradable_edge_rejects_fill() {
  exec::paper_execution_backend_t backend{};
  auto m = market();
  m.edge_tradable_mask = torch::tensor({false, true}, torch::kBool);
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      m, ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "nontradable edge rejects fill");
  check(trace.fills[0].reject_reason == "edge_not_tradable",
        "nontradable reject reason");
}

void test_below_min_notional_rejects_fill() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.0005, 0.0}, torch::kFloat64), 1.0, 0.9995),
      market(), ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "below min notional rejects fill");
  check(trace.fills[0].reject_reason == "below_min_notional",
        "below min notional reject reason");
}

void test_above_max_notional_rejects_without_partial() {
  exec::paper_execution_backend_t backend{};
  auto m = market();
  m.max_notional_base = torch::tensor({100.0, 1000.0}, torch::kFloat64);
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      m, ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "above max rejects without partial");
  check(trace.fills[0].reject_reason == "above_max_notional",
        "above max reject reason");
}

void test_above_max_notional_partial_fill_when_enabled() {
  exec::paper_execution_options_t options{};
  options.allow_partial_fills = true;
  exec::paper_execution_backend_t backend(options);
  auto m = market();
  m.max_notional_base = torch::tensor({100.0, 1000.0}, torch::kFloat64);
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      m, ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 0, "partial max cap should not reject");
  check(trace.partial_fill_count == 1, "partial max cap counts once");
  check(trace.fills[0].status == exec::fill_status_t::partially_filled,
        "partial max cap fill status");
  close(trace.routed_notional_base, 100.0, 1.0e-9,
        "partial max cap routed notional");
}

void test_rejects_insufficient_reserve_without_partial() {
  exec::paper_execution_backend_t backend{};
  auto low_reserve_high_equity = ledger(
      /*base_reserve_units=*/10.0, torch::tensor({9.9, 0.0}, torch::kFloat64));
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      market(), low_reserve_high_equity);
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "insufficient reserve rejects fill");
  check(trace.fills[0].status == exec::fill_status_t::rejected,
        "insufficient reserve fill status is rejected");
}

void test_insufficient_units_reject_sell_without_partial() {
  exec::paper_execution_backend_t backend{};
  auto before = ledger(0.0, torch::tensor({0.1, 0.0}, torch::kFloat64));
  auto trace = backend.execute(
      intent(torch::tensor({1.0, 0.0}, torch::kFloat64),
             torch::tensor({0.0, 0.0}, torch::kFloat64), 0.0, 1.0, 10.0),
      market(), before);
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "insufficient units rejects sell");
  check(trace.fills[0].reject_reason == "insufficient_asset_units",
        "insufficient units reject reason");
}

void test_insufficient_units_partial_sell_when_enabled() {
  exec::paper_execution_options_t options{};
  options.allow_partial_fills = true;
  exec::paper_execution_backend_t backend(options);
  auto before = ledger(0.0, torch::tensor({0.1, 0.0}, torch::kFloat64));
  auto trace = backend.execute(
      intent(torch::tensor({1.0, 0.0}, torch::kFloat64),
             torch::tensor({0.0, 0.0}, torch::kFloat64), 0.0, 1.0, 10.0),
      market(), before);
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 0, "partial sell should not reject");
  check(trace.partial_fill_count == 1, "partial sell counts once");
  check(trace.fills[0].status == exec::fill_status_t::partially_filled,
        "partial sell fill status");
  close(trace.ledger_after.units.index({0}).item<double>(), 0.0, 1.0e-9,
        "partial sell liquidates available units");
}

void test_missing_direct_edge_fails_closed_before_execution() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.0, 0.5}, torch::kFloat64), 1.0, 0.5),
      market_without_eth_edge(), ledger());
  exec::validate_execution_trace(trace);
  check(!trace.valid, "missing direct edge returns invalid trace");
  check(trace.rejected_fill_count == 1, "missing direct edge rejects fill");
  check(trace.fills[0].reject_reason == "missing_direct_reserve_edge",
        "missing direct edge reject reason");
  check(contains_failure_prefix(trace, "missing_direct_reserve_edge:ETH"),
        "missing direct edge failure is recorded");
}

void test_equity_mismatch_is_warned() {
  exec::paper_execution_options_t options{};
  options.equity_mismatch_fail_tolerance = 0.20;
  exec::paper_execution_backend_t backend(options);
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5, 900.0),
      market(), ledger());
  exec::validate_execution_trace(trace);
  check(trace.ledger_intent_equity_mismatch, "equity mismatch flag is set");
  close(trace.ledger_intent_equity_difference_base, 100.0, 1.0e-9,
        "equity mismatch difference");
  close(trace.requested_notional_base, 500.0, 1.0e-9,
        "equity mismatch sizes from ledger equity");
  close(trace.routed_notional_base, 500.0, 1.0e-9,
        "equity mismatch routes from ledger equity");
  check(contains_warning(trace, "ledger_intent_equity_mismatch"),
        "equity mismatch warning is present");
}

void test_large_equity_mismatch_fails_closed_before_execution() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5, 100.0),
      market(), ledger());
  exec::validate_execution_trace(trace);
  check(!trace.valid, "large equity mismatch invalidates trace");
  check(trace.orders.empty() && trace.fills.empty(),
        "large equity mismatch prevents execution");
  check(contains_failure_prefix(trace,
                                "ledger_intent_equity_mismatch_exceeds_limit"),
        "large equity mismatch failure is recorded");
}

void test_synthetic_market_source_is_warned() {
  exec::paper_execution_options_t options{};
  options.allow_synthetic_direct_edges = true;
  exec::paper_execution_backend_t backend(options);
  auto m = market();
  m.market_source_id = "fixture.synthetic_direct_edges";
  m.synthetic_direct_edges = true;
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.0, 0.0}, torch::kFloat64), 1.0, 1.0),
      m, ledger());
  exec::validate_execution_trace(trace);
  check(trace.market_source_id == "fixture.synthetic_direct_edges",
        "market source id is preserved");
  check(trace.synthetic_direct_edges, "synthetic market flag is preserved");
  check(contains_warning(trace, "synthetic_direct_edges_market_source"),
        "synthetic market source warning is present");
}

void test_synthetic_market_source_can_be_rejected() {
  exec::paper_execution_backend_t backend{};
  auto m = market();
  m.market_source_id = "fixture.synthetic_direct_edges";
  m.synthetic_direct_edges = true;
  auto trace = backend.execute(
      intent(torch::tensor({0.0, 0.0}, torch::kFloat64),
             torch::tensor({0.5, 0.0}, torch::kFloat64), 1.0, 0.5),
      m, ledger());
  exec::validate_execution_trace(trace);
  check(!trace.valid, "synthetic market can be rejected by option");
  check(trace.orders.empty() && trace.fills.empty(),
        "synthetic rejection prevents execution");
  check(contains_failure_prefix(trace, "synthetic_direct_edges_disallowed"),
        "synthetic rejection failure is recorded");
}

void test_invalid_sell_price_rejects_before_ledger_mutation() {
  exec::paper_execution_backend_t backend{};
  auto m = market();
  m.edge_spread_rate = torch::tensor({2.0, 0.002}, torch::kFloat64);
  auto before = ledger(500.0, torch::tensor({5.0, 0.0}, torch::kFloat64));
  before = exec::mark_to_market(before, market(), 100);
  auto trace = backend.execute(
      intent(torch::tensor({0.5, 0.0}, torch::kFloat64),
             torch::tensor({0.0, 0.0}, torch::kFloat64), 0.5, 1.0),
      m, before);
  exec::validate_execution_trace(trace);
  check(!trace.valid, "invalid sell price invalidates trace");
  check(trace.rejected_fill_count == 1,
        "invalid sell price returns rejected result");
  check(trace.fills[0].reject_reason ==
            "invalid_sell_price_after_spread_slippage",
        "invalid sell price reject reason");
  close(trace.ledger_after.units.index({0}).item<double>(),
        before.units.index({0}).item<double>(), 1.0e-9,
        "invalid sell price preserves units");
  close(trace.ledger_after.base_reserve_units, before.base_reserve_units,
        1.0e-9, "invalid sell price preserves reserve");
}

void test_live_execution_flag_is_impossible_in_v1() {
  exec::paper_execution_options_t options{};
  options.live_execution_allowed = true;
  bool rejected = false;
  try {
    exec::paper_execution_backend_t backend(options);
    (void)backend;
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected, "paper v1 rejects live execution flag");
}

} // namespace

int main() {
  test_buy_updates_ledger_and_trace();
  test_reverse_edge_orientation_prices_asset_in_reserve_units();
  test_sell_reduces_units();
  test_sells_execute_before_buys_to_fund_rebalance();
  test_nontradable_edge_rejects_fill();
  test_below_min_notional_rejects_fill();
  test_above_max_notional_rejects_without_partial();
  test_above_max_notional_partial_fill_when_enabled();
  test_rejects_insufficient_reserve_without_partial();
  test_insufficient_units_reject_sell_without_partial();
  test_insufficient_units_partial_sell_when_enabled();
  test_missing_direct_edge_fails_closed_before_execution();
  test_equity_mismatch_is_warned();
  test_large_equity_mismatch_fails_closed_before_execution();
  test_synthetic_market_source_is_warned();
  test_synthetic_market_source_can_be_rejected();
  test_invalid_sell_price_rejects_before_ledger_mutation();
  test_live_execution_flag_is_impossible_in_v1();
  return 0;
}
