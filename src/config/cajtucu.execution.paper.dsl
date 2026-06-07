/*
  cajtucu.execution.paper.dsl
  ===========================
  Authored contract for the first Cajtucu output/execution method.

  V1 is paper-only. It converts target-weight execution intents into direct
  node-pair orders, simulated paper fills, ledger mutation, and durable
  execution traces. It has no live broker API, no credentials, no network calls,
  and no real-capital authority.
*/
CAJTUCU_EXECUTION_PAPER {
  VERSION = cajtucu.execution.paper.v1;
  COMPONENT_ASSEMBLY_ID = paper_execution_v1;
  INPUT_INTENT_SCHEMA = cajtucu.execution.intent.v1;
  OUTPUT_TRACE_SCHEMA = cajtucu.execution.trace.v1;
  LEDGER_SCHEMA = cajtucu.execution.ledger.v1;
  BACKEND_KIND = deterministic_paper_execution;
  ACTION_SOURCE = kikijyeba_environment_target_weights;
  ACTION_SCHEMA = kikijyeba.environment.action.target_node_weights.v1;
  MARKET_SOURCE_POLICY = explicit_market_execution_state;
  SYNTHETIC_MARKET_SOURCE_POLICY = allowed_with_trace_warning;
  ROUTE_POLICY = direct_node_pair_only;
  EXECUTION_PAIR_POLICY = direct_node_pair_only;
  ACCOUNTING_NUMERAIRE_SOURCE = global_config_accounting_section;
  EXECUTION_ORDER_POLICY = sells_before_buys;
  ORDER_MODEL = target_weight_delta_to_pair_notional;
  FILL_PRICE_MODEL = mid_plus_half_spread_plus_slippage;
  INVALID_SELL_PRICE_POLICY = invalid_trace_before_ledger_mutation;
  MISSING_DIRECT_PAIR_POLICY = skip_pair_warn;
  FEE_MODEL = linear_fee_rate_per_edge;
  MIN_NOTIONAL_POLICY = reject_when_enabled;
  MAX_NOTIONAL_POLICY = reject_or_partial_fill;
  PARTIAL_FILL_POLICY = configurable_default_false;
  LEDGER_POLICY = mutate_graph_node_units_from_pair_fills;
  MARK_TO_MARKET_POLICY = direct_edge_mid_price;
  EQUITY_MISMATCH_POLICY = warn_and_size_from_mark_to_market_ledger;
  TRACE_POLICY = include_intent_orders_fills_ledgers_costs_source_identity;
  EXECUTION_AUTHORITY = false;
  LIVE_EXECUTION_ALLOWED = false;
  BROKER_CREDENTIALS_ALLOWED = false;
  NETWORK_IO_ALLOWED = false;
  DEFAULT_ALLOW_PARTIAL_FILLS = false;
  DEFAULT_ALLOW_SYNTHETIC_DIRECT_EDGES = false;
  DEFAULT_EQUITY_MISMATCH_TOLERANCE = 0.000001;
  DEFAULT_EQUITY_MISMATCH_FAIL_TOLERANCE = 0.01;
  EPS = 0.000000000001;
};
