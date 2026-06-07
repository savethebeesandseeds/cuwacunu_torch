#include "cajtucu/execution/assembly.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace exec = cuwacunu::cajtucu::execution;

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

bool contains_warning_prefix(const exec::execution_trace_t &trace,
                             const std::string &prefix) {
  return std::any_of(trace.warnings.begin(), trace.warnings.end(),
                     [&](const std::string &warning) {
                       return warning.rfind(prefix, 0) == 0;
                     });
}

exec::market_execution_state_t direct_pair_market() {
  exec::market_execution_state_t out{};
  out.market_source_id = "fixture.direct_pair_market_state";
  out.timestamp_ms = 100;
  out.graph.node_ids = {"BTC", "ETH", "USDT"};
  out.graph.edge_ids = {"BTC/USDT", "ETH/USDT", "ETH/BTC"};
  out.graph.base_index = {0, 1, 1};
  out.graph.quote_index = {2, 2, 0};
  out.edge_mid_price = torch::tensor({100.0, 50.0, 0.5}, torch::kFloat64);
  out.edge_fee_rate = torch::tensor({0.001, 0.001, 0.001}, torch::kFloat64);
  out.edge_spread_rate = torch::tensor({0.002, 0.002, 0.002}, torch::kFloat64);
  out.edge_slippage_rate =
      torch::tensor({0.003, 0.003, 0.003}, torch::kFloat64);
  out.min_notional_numeraire = torch::tensor({1.0, 1.0, 1.0}, torch::kFloat64);
  out.max_notional_numeraire =
      torch::tensor({1000.0, 1000.0, 1000.0}, torch::kFloat64);
  out.edge_tradable_mask = torch::tensor({true, true, true}, torch::kBool);
  exec::validate_market_execution_state(out);
  return out;
}

exec::market_execution_state_t reverse_direct_pair_market() {
  auto out = direct_pair_market();
  out.market_source_id = "fixture.reverse_direct_pair_market_state";
  out.graph.edge_ids = {"BTC/USDT", "ETH/USDT", "BTC/ETH"};
  out.graph.base_index = {0, 1, 0};
  out.graph.quote_index = {2, 2, 1};
  out.edge_mid_price = torch::tensor({100.0, 50.0, 2.0}, torch::kFloat64);
  exec::validate_market_execution_state(out);
  return out;
}

exec::market_execution_state_t missing_pair_market() {
  auto out = direct_pair_market();
  out.market_source_id = "fixture.missing_pair_market_state";
  out.graph.edge_ids = {"BTC/USDT", "ETH/USDT"};
  out.graph.base_index = {0, 1};
  out.graph.quote_index = {2, 2};
  out.edge_mid_price = torch::tensor({100.0, 50.0}, torch::kFloat64);
  out.edge_fee_rate = torch::tensor({0.001, 0.001}, torch::kFloat64);
  out.edge_spread_rate = torch::tensor({0.002, 0.002}, torch::kFloat64);
  out.edge_slippage_rate = torch::tensor({0.003, 0.003}, torch::kFloat64);
  out.min_notional_numeraire = torch::tensor({1.0, 1.0}, torch::kFloat64);
  out.max_notional_numeraire = torch::tensor({1000.0, 1000.0}, torch::kFloat64);
  out.edge_tradable_mask = torch::tensor({true, true}, torch::kBool);
  exec::validate_market_execution_state(out);
  return out;
}

exec::execution_ledger_t make_ledger(
    torch::Tensor units = torch::tensor({2.0, 12.0, 200.0}, torch::kFloat64)) {
  exec::execution_ledger_t out{};
  out.timestamp_ms = 100;
  out.accounting_numeraire_node_id = "USDT";
  out.node_ids = {"BTC", "ETH", "USDT"};
  out.units = units.to(torch::kFloat64);
  const auto prices = torch::tensor({100.0, 50.0, 1.0}, torch::kFloat64);
  const auto values = out.units * prices;
  out.equity_value_numeraire = values.sum().item<double>();
  out.weights = values / out.equity_value_numeraire;
  exec::validate_execution_ledger(out);
  return out;
}

exec::execution_intent_t make_intent(torch::Tensor current_weights,
                                     torch::Tensor target_weights,
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
  out.node_ids = {"BTC", "ETH", "USDT"};
  out.accounting_numeraire_node_id = "USDT";
  out.current_weights = current_weights.to(torch::kFloat64);
  out.target_weights = target_weights.to(torch::kFloat64);
  out.current_units = torch::tensor({2.0, 12.0, 200.0}, torch::kFloat64);
  out.equity_value_numeraire = equity;
  exec::validate_execution_intent(out);
  return out;
}

exec::execution_intent_t default_pair_rebalance_intent(double equity = 1000.0) {
  return make_intent(torch::tensor({0.2, 0.6, 0.2}, torch::kFloat64),
                     torch::tensor({0.6, 0.2, 0.2}, torch::kFloat64), equity);
}

void test_direct_pair_rebalance_executes_once_without_numeraire_route() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               direct_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.valid, "direct-pair rebalance should be valid");
  check(trace.orders.size() == 1 && trace.fills.size() == 1,
        "direct-pair rebalance creates one order/result");
  check(trace.orders[0].sell_node_id == "ETH" &&
            trace.orders[0].buy_node_id == "BTC",
        "rebalance sells ETH directly into BTC");
  check(trace.orders[0].edge_id == "ETH/BTC",
        "rebalance uses the direct ETH/BTC edge");
  close(trace.requested_notional_numeraire, 400.0, 1.0e-9,
        "direct-pair requested notional");
  close(trace.routed_notional_numeraire, 400.0, 1.0e-9,
        "direct-pair routed notional");
  close(trace.total_fee_numeraire, 0.4, 1.0e-9, "direct-pair fee");
  close(trace.total_spread_cost_numeraire, 0.4, 1.0e-9,
        "direct-pair spread cost");
  close(trace.total_slippage_numeraire, 1.2, 1.0e-9, "direct-pair slippage");
  close(trace.total_transaction_cost_numeraire, 2.0, 1.0e-9,
        "direct-pair total cost");
  close(trace.fills[0].filled_sell_quantity, 8.0, 1.0e-9,
        "direct-pair sells expected ETH units");
  close(trace.fills[0].filled_buy_quantity, 3.98, 1.0e-9,
        "direct-pair buys BTC net of fee/spread/slippage");
  close(trace.ledger_after.units.index({0}).item<double>(), 5.98, 1.0e-9,
        "direct-pair increases BTC units");
  close(trace.ledger_after.units.index({1}).item<double>(), 4.0, 1.0e-9,
        "direct-pair reduces ETH units");
  close(trace.ledger_after.units.index({2}).item<double>(), 200.0, 1.0e-9,
        "direct-pair leaves numeraire node units unchanged");
  close(trace.ledger_after.equity_value_numeraire, 998.0, 1.0e-9,
        "direct-pair equity reflects one execution cost");
}

void test_reverse_direct_pair_orientation_prices_buy_per_sell() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               reverse_direct_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.valid, "reverse direct-pair rebalance should be valid");
  check(trace.orders[0].edge_id == "BTC/ETH",
        "rebalance may use the reverse direct pair edge");
  close(trace.orders[0].fill_price_buy_per_sell, 1.0 / (2.0 * (1.0 + 0.004)),
        1.0e-12, "reverse edge price is BTC per ETH after costs");
}

void test_missing_direct_pair_rejects_without_numeraire_fallback() {
  exec::paper_execution_options_t options{};
  options.missing_direct_pair_policy =
      exec::missing_direct_pair_policy_t::invalid_trace;
  exec::paper_execution_backend_t backend(options);
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               missing_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(!trace.valid, "missing direct pair returns invalid trace");
  check(trace.rejected_fill_count == 1, "missing direct pair rejects once");
  check(trace.missing_direct_pair_count == 1,
        "missing direct pair counter is recorded");
  check(trace.fills[0].reject_reason == "missing_direct_pair",
        "missing direct pair reject reason");
  check(contains_failure_prefix(trace, "missing_direct_pair:ETH->BTC"),
        "missing direct pair failure records route");
  close(trace.ledger_after.units.index({0}).item<double>(), 2.0, 1.0e-9,
        "missing pair preserves BTC units");
  close(trace.ledger_after.units.index({1}).item<double>(), 12.0, 1.0e-9,
        "missing pair preserves ETH units");
}

void test_missing_direct_pair_warns_and_skips_by_default() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               missing_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.valid, "default missing direct pair policy warns and skips");
  check(trace.rejected_fill_count == 1, "skipped missing pair records reject");
  check(trace.missing_direct_pair_count == 1,
        "skipped missing pair counter is recorded");
  check(trace.numeraire_fallback_pair_count == 0,
        "default skip policy does not route through numeraire");
  check(trace.fills[0].reject_reason == "missing_direct_pair_skipped",
        "skipped missing pair reject reason");
  check(trace.failures.empty(), "skipped missing pair does not fail trace");
  close(trace.ledger_after.units.index({0}).item<double>(), 2.0, 1.0e-9,
        "skipped missing pair preserves BTC units");
  close(trace.ledger_after.units.index({1}).item<double>(), 12.0, 1.0e-9,
        "skipped missing pair preserves ETH units");
}

void test_missing_direct_pair_can_route_via_numeraire_when_enabled() {
  exec::paper_execution_options_t options{};
  options.missing_direct_pair_policy =
      exec::missing_direct_pair_policy_t::route_via_accounting_numeraire_warn;
  exec::paper_execution_backend_t backend(options);
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               missing_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.valid, "numeraire fallback trace should remain valid");
  check(trace.rejected_fill_count == 0, "numeraire fallback should not reject");
  check(trace.missing_direct_pair_count == 1,
        "numeraire fallback records the missing direct pair");
  check(trace.numeraire_fallback_pair_count == 1,
        "numeraire fallback counter is recorded");
  check(trace.orders.size() == 2 && trace.fills.size() == 2,
        "numeraire fallback executes two explicit legs");
  check(trace.orders[0].sell_node_id == "ETH" &&
            trace.orders[0].buy_node_id == "USDT" &&
            trace.orders[0].edge_id == "ETH/USDT",
        "numeraire fallback first leg sells ETH into USDT");
  check(trace.orders[1].sell_node_id == "USDT" &&
            trace.orders[1].buy_node_id == "BTC" &&
            trace.orders[1].edge_id == "BTC/USDT",
        "numeraire fallback second leg buys BTC from USDT");
  check(contains_warning_prefix(
            trace,
            "missing_direct_pair_routed_via_accounting_numeraire:ETH->BTC"),
        "numeraire fallback warning records original pair");
  close(trace.requested_notional_numeraire, 798.0, 1.0e-9,
        "numeraire fallback requested notional counts both legs");
  close(trace.routed_notional_numeraire, 798.0, 1.0e-9,
        "numeraire fallback routed notional counts both legs");
  close(trace.total_fee_numeraire, 0.798, 1.0e-9,
        "numeraire fallback pays two fee events");
  close(trace.total_spread_cost_numeraire, 0.798, 1.0e-9,
        "numeraire fallback pays two spread events");
  close(trace.total_slippage_numeraire, 2.394, 1.0e-9,
        "numeraire fallback pays two slippage events");
  close(trace.total_transaction_cost_numeraire, 3.99, 1.0e-9,
        "numeraire fallback records double-leg cost");
  close(trace.ledger_after.units.index({0}).item<double>(), 5.96016342629482,
        1.0e-9, "numeraire fallback increases BTC units");
  close(trace.ledger_after.units.index({1}).item<double>(), 4.0, 1.0e-9,
        "numeraire fallback reduces ETH units");
  close(trace.ledger_after.units.index({2}).item<double>(), 200.0, 1.0e-9,
        "numeraire fallback returns intermediate USDT balance");
}

void test_nontradable_direct_pair_rejects_fill() {
  exec::paper_execution_backend_t backend{};
  auto m = direct_pair_market();
  m.edge_tradable_mask = torch::tensor({true, true, false}, torch::kBool);
  auto trace =
      backend.execute(default_pair_rebalance_intent(), m, make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "nontradable edge rejects fill");
  check(trace.fills[0].reject_reason == "edge_not_tradable",
        "nontradable reject reason");
}

void test_below_min_notional_rejects_pair_fill() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(
      make_intent(torch::tensor({0.2, 0.6, 0.2}, torch::kFloat64),
                  torch::tensor({0.2005, 0.5995, 0.2}, torch::kFloat64)),
      direct_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "below min notional rejects fill");
  check(trace.fills[0].reject_reason == "below_min_notional",
        "below min notional reject reason");
}

void test_above_max_notional_partial_pair_fill_when_enabled() {
  exec::paper_execution_options_t options{};
  options.allow_partial_fills = true;
  exec::paper_execution_backend_t backend(options);
  auto m = direct_pair_market();
  m.max_notional_numeraire =
      torch::tensor({1000.0, 1000.0, 100.0}, torch::kFloat64);
  auto trace =
      backend.execute(default_pair_rebalance_intent(), m, make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 0, "partial max cap should not reject");
  check(trace.partial_fill_count == 1, "partial max cap counts once");
  check(trace.fills[0].status == exec::fill_status_t::partially_filled,
        "partial max cap fill status");
  close(trace.routed_notional_numeraire, 100.0, 1.0e-9,
        "partial max cap routed notional");
}

void test_insufficient_sell_units_rejects_without_partial() {
  exec::paper_execution_backend_t backend{};
  auto thin_ledger =
      make_ledger(torch::tensor({2.0, 1.0, 750.0}, torch::kFloat64));
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               direct_pair_market(), thin_ledger);
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 1, "insufficient sell units rejects fill");
  check(trace.fills[0].reject_reason == "insufficient_sell_units",
        "insufficient sell units reject reason");
}

void test_insufficient_sell_units_partial_when_enabled() {
  exec::paper_execution_options_t options{};
  options.allow_partial_fills = true;
  exec::paper_execution_backend_t backend(options);
  auto thin_ledger =
      make_ledger(torch::tensor({2.0, 1.0, 750.0}, torch::kFloat64));
  auto trace = backend.execute(default_pair_rebalance_intent(),
                               direct_pair_market(), thin_ledger);
  exec::validate_execution_trace(trace);
  check(trace.rejected_fill_count == 0, "partial sell should not reject");
  check(trace.partial_fill_count == 1, "partial sell counts once");
  close(trace.ledger_after.units.index({1}).item<double>(), 0.0, 1.0e-9,
        "partial sell liquidates available sell node units");
}

void test_equity_mismatch_is_warned_and_sizes_from_ledger() {
  exec::paper_execution_options_t options{};
  options.equity_mismatch_fail_tolerance = 0.20;
  exec::paper_execution_backend_t backend(options);
  auto trace = backend.execute(default_pair_rebalance_intent(900.0),
                               direct_pair_market(), make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.ledger_intent_equity_mismatch, "equity mismatch flag is set");
  close(trace.ledger_intent_equity_difference_numeraire, 100.0, 1.0e-9,
        "equity mismatch difference");
  close(trace.requested_notional_numeraire, 400.0, 1.0e-9,
        "equity mismatch sizes from ledger equity");
  close(trace.routed_notional_numeraire, 400.0, 1.0e-9,
        "equity mismatch routes from ledger equity");
  check(contains_warning(trace, "ledger_intent_equity_mismatch"),
        "equity mismatch warning is present");
}

void test_large_equity_mismatch_fails_closed_before_execution() {
  exec::paper_execution_backend_t backend{};
  auto trace = backend.execute(default_pair_rebalance_intent(100.0),
                               direct_pair_market(), make_ledger());
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
  auto m = direct_pair_market();
  m.market_source_id = "fixture.synthetic_direct_edges";
  m.synthetic_direct_edges = true;
  auto trace = backend.execute(
      make_intent(torch::tensor({0.2, 0.6, 0.2}, torch::kFloat64),
                  torch::tensor({0.2, 0.6, 0.2}, torch::kFloat64)),
      m, make_ledger());
  exec::validate_execution_trace(trace);
  check(trace.market_source_id == "fixture.synthetic_direct_edges",
        "market source id is preserved");
  check(trace.synthetic_direct_edges, "synthetic market flag is preserved");
  check(contains_warning(trace, "synthetic_direct_edges_market_source"),
        "synthetic market source warning is present");
}

void test_synthetic_market_source_can_be_rejected() {
  exec::paper_execution_backend_t backend{};
  auto m = direct_pair_market();
  m.market_source_id = "fixture.synthetic_direct_edges";
  m.synthetic_direct_edges = true;
  auto trace =
      backend.execute(default_pair_rebalance_intent(), m, make_ledger());
  exec::validate_execution_trace(trace);
  check(!trace.valid, "synthetic market can be rejected by option");
  check(trace.orders.empty() && trace.fills.empty(),
        "synthetic rejection prevents execution");
  check(contains_failure_prefix(trace, "synthetic_direct_edges_disallowed"),
        "synthetic rejection failure is recorded");
}

void test_invalid_pair_price_rejects_before_ledger_mutation() {
  exec::paper_execution_backend_t backend{};
  auto m = direct_pair_market();
  m.edge_spread_rate = torch::tensor({0.002, 0.002, 2.0}, torch::kFloat64);
  auto before = make_ledger();
  auto trace = backend.execute(default_pair_rebalance_intent(), m, before);
  exec::validate_execution_trace(trace);
  check(!trace.valid, "invalid pair price invalidates trace");
  check(trace.rejected_fill_count == 1,
        "invalid pair price returns rejected result");
  check(trace.fills[0].reject_reason == "invalid_direct_pair_fill_price",
        "invalid pair price reject reason");
  close(trace.ledger_after.units.index({0}).item<double>(),
        before.units.index({0}).item<double>(), 1.0e-9,
        "invalid pair price preserves buy node units");
  close(trace.ledger_after.units.index({1}).item<double>(),
        before.units.index({1}).item<double>(), 1.0e-9,
        "invalid pair price preserves sell node units");
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
  test_direct_pair_rebalance_executes_once_without_numeraire_route();
  test_reverse_direct_pair_orientation_prices_buy_per_sell();
  test_missing_direct_pair_rejects_without_numeraire_fallback();
  test_missing_direct_pair_warns_and_skips_by_default();
  test_missing_direct_pair_can_route_via_numeraire_when_enabled();
  test_nontradable_direct_pair_rejects_fill();
  test_below_min_notional_rejects_pair_fill();
  test_above_max_notional_partial_pair_fill_when_enabled();
  test_insufficient_sell_units_rejects_without_partial();
  test_insufficient_sell_units_partial_when_enabled();
  test_equity_mismatch_is_warned_and_sizes_from_ledger();
  test_large_equity_mismatch_fails_closed_before_execution();
  test_synthetic_market_source_is_warned();
  test_synthetic_market_source_can_be_rejected();
  test_invalid_pair_price_rejects_before_ledger_mutation();
  test_live_execution_flag_is_impossible_in_v1();
  return 0;
}
