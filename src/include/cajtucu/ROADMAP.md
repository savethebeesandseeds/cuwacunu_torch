# Cajtucu Roadmap

Cajtucu is the output and execution pillar. It is not a policy, evaluator,
Lattice proof engine, Runtime launcher, or live-capital authority.

## Current State

```text
cajtucu.execution.paper.v1
```

Status:

```text
Paper execution V1 is implemented and integrated with Kikijyeba replay.
```

Stable V1 behavior:

- Backend-neutral execution intent, order, fill/result, ledger, and trace types.
- Direct base-reserve-edge paper execution only.
- Sells execute before buys.
- Fee, spread, slippage, min/max notional, tradability, insufficient reserve,
  insufficient units, partial-fill option, and reject paths are represented.
- Missing direct reserve edges produce invalid traces with evidence.
- Large equity mismatch fails closed.
- Invalid sell-price cost model fails closed.
- Synthetic direct edges are rejected by default and must be explicit research
  replay opt-ins.
- No credentials, broker network calls, or live execution authority.

## Active Milestone: Cost-Aware Replay Evidence

Goal:

```text
Make Cajtucu paper traces useful enough for rollout evidence and future policy
training comparisons.
```

Milestone name:

```text
cost_aware_paper_replay_rollout.v1 support
```

Acceptance:

- Replay reports include Cajtucu execution profile text/digest identity.
- Validation rollouts require at least one nonzero cost assumption: fee, spread,
  slippage, or linear transaction cost.
- Zero-cost paper replay is allowed only as research and must be labeled
  `frictionless_research_replay`.
- Synthetic direct-edge markets are forbidden for validation rollout,
  paper-online, and live. Research replay must opt in explicitly with a reason.
- Execution traces expose transaction cost, spread/slippage assumptions, reject
  counts, partial-fill counts, invalid trace counts, and synthetic-market
  counters.
- Kikijyeba reward uses Cajtucu ledger state after execution and before
  historical realization.
- Policy comparisons can be run with nonzero transaction costs.
- Cajtucu remains independent from Wikimyei internals; it consumes execution
  intent, market execution state, and ledger state.

Required rollout metrics:

```text
execution_feasibility
  valid_trace_count
  invalid_trace_count
  missing_direct_reserve_edge_count
  nontradable_edge_reject_count
  below_min_notional_reject_count
  above_max_notional_reject_count
  insufficient_reserve_reject_count
  insufficient_units_reject_count
  invalid_sell_price_count
  large_equity_mismatch_count
  synthetic_market_step_count

fill_quality
  requested_order_count
  executed_order_count
  rejected_order_count
  partial_order_count
  requested_notional_base
  executed_notional_base
  rejected_notional_base
  partial_notional_base
  fill_ratio

cost_anatomy
  fee_cost_base
  spread_cost_base
  slippage_cost_base
  total_transaction_cost_base
  cost_as_fraction_of_equity
  cost_as_fraction_of_gross_return

target_tracking
  target_weight_error_l1
  target_weight_error_linf
  post_execution_risky_weight_sum
  post_execution_base_reserve_weight
  reserve_shortfall_count

ledger_integrity
  ledger_before_equity
  ledger_after_execution_equity
  ledger_after_realization_equity
  ledger_equity_reconciliation_error
  base_reserve_units_before
  base_reserve_units_after
  unit_nonnegativity_violation_count
```

## Next Milestone: Paper-Online Readiness Design

Do not implement paper-online yet. First define the extra ledger and market-data
constraints needed for online paper operation:

```text
paper_online_readiness_contract.v1
```

- precision, tick size, lot size, min notional, and dust thresholds
- stale market data policy
- market source identity and timestamps
- idempotent intent/trace ids
- duplicate-execution guard
- duplicate-action protection
- session start/stop semantics
- fee asset and fee conversion
- cost basis
- realized and unrealized PnL
- per-node mark prices and mark timestamps
- direct-edge universe validation before session start
- synthetic execution markets forbidden
- locked Cajtucu execution profile digest
- reward/report artifact path policy
- operator abort/kill-switch semantics
- clock/timestamp policy
- richer reject/partial counters
- trace persistence as first-class Runtime evidence

Paper-online must reuse the same backend interface as historical replay.

## Future Milestones

```text
cajtucu.execution.paper_online.v1
  live market stream, paper fills, no capital.

cajtucu.execution.route.v2
  routed/multi-hop execution beyond direct reserve edges.

cajtucu.execution.broker.v1
  live broker adapter, credentials, order lifecycle, and kill switches.
```

Live broker work requires:

- Tsodao-approved settings
- Lattice readiness proof
- Marshal handoff receipt
- Runtime execution authority
- no direct policy-to-broker path

## Explicit Non-Goals

- PPO
- policy training
- allocator tuning
- reward judging
- market readiness
- deployment readiness
- hidden live broker API in paper mode
