# Cajtucu Roadmap

Cajtucu is the output and execution pillar. It is not a policy, evaluator,
Lattice proof engine, Runtime launcher, optimizer, or live-capital authority.

## Current State

```text
cajtucu.execution.paper.v1
```

Implemented baseline:

- Backend-neutral execution intent, order, fill/result, ledger, and trace
  contracts.
- Direct graph-node pair paper execution only.
- Sells execute before buys.
- Fee, spread, slippage, min/max notional, tradability, insufficient sell
  units, partial-fill option, and reject paths are represented.
- Missing direct pair edges use an explicit policy: default skip-and-warn,
  strict invalid trace, or opt-in accounting-numeraire fallback with visible
  two-leg paper costs.
- Large equity mismatch fails closed.
- Invalid sell-price cost model fails closed.
- Synthetic direct edges are rejected by default and require explicit research
  replay opt-in.
- No credentials, broker network calls, or live execution authority.

## Active Milestone: Cost-Aware Replay Evidence

Milestone:

```text
cost_aware_paper_replay_rollout.v1 support
```

Goal:

```text
Make Cajtucu paper traces strong enough for rollout evidence and future
policy-training comparisons.
```

Acceptance:

- Replay reports include Cajtucu execution profile text/digest identity.
- Validation rollouts require at least one nonzero cost assumption: fee, spread,
  slippage, or linear transaction cost.
- Zero-cost paper replay is research-only and labeled
  `frictionless_research_replay`.
- Synthetic direct-pair markets are forbidden for validation rollout,
  paper-online, and live. Research replay must opt in explicitly with a reason.
- Execution traces expose transaction cost, spread/slippage assumptions, reject
  counts, partial-fill counts, invalid trace counts, and synthetic-market
  counters.
- Kikijyeba reward uses Cajtucu ledger state after execution and before
  historical realization.
- Baselines and deterministic Wikimyei policy can be compared under identical
  nonzero execution costs.
- Cajtucu remains independent from Wikimyei internals; it consumes execution
  intent, market execution state, and ledger state.

Required rollout metric groups:

```text
execution_feasibility
fill_quality
cost_anatomy
target_tracking
ledger_integrity
```

The report must expose enough detail to distinguish:

```text
belief or projection weakness
policy target weakness
execution infeasibility
cost/turnover drag
ledger/accounting failure
time-law leakage
```

## Next Milestone: Paper-Online Readiness Design

Do not implement paper-online yet. First define the extra ledger and market-data
constraints needed for online paper operation:

```text
paper_online_readiness_contract.v1
```

Required design points:

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
- direct-pair universe validation before session start
- synthetic execution markets forbidden
- locked Cajtucu execution profile digest
- reward/report artifact path policy
- operator abort/kill-switch semantics
- clock/timestamp policy
- trace persistence as first-class Runtime evidence

Paper-online must reuse the same backend interface as historical replay.

## Future Milestones

```text
cajtucu.execution.paper_online.v1
  live market stream, paper fills, no capital.

cajtucu.execution.route.v2
  routed/multi-hop execution beyond direct pairs.

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
