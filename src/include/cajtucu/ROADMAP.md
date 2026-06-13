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
- Direct graph-node pair paper execution.
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
- Cost-aware replay reports expose execution feasibility, fill quality, cost
  anatomy, target tracking, ledger integrity, missing-pair counters, and
  fallback counters.
- Paper-online readiness policy names persistent ledger recovery, duplicate
  intent protection, direct-edge validation, stale-market policy, locked
  execution-profile identity, and operator abort/kill-switch evidence before
  any session runner exists.
- No credentials, broker network calls, or live execution authority.

## Next Cajtucu Work

### Paper-Online Session Contract V1

Milestone:

```text
paper_online_session_contract.v1 support
```

Cajtucu role:

- Define how a future online-paper session reuses the paper V1 direct-pair
  intent, order, fill, ledger, and trace contracts.
- Keep persistent ledger recovery, duplicate intent protection, idempotency
  keys, stale-market rejection/warning policy, locked execution-profile
  identity, and operator abort/kill-switch traces explicit in the session
  evidence.
- Keep the accounting numeraire a valuation/reporting node, not a hidden route
  or reserve action.

Acceptance sketch:

- The session contract names required online-paper trace fields before any live
  market stream is consumed.
- Missing direct pairs, stale market data, duplicate intents, ledger recovery,
  rejects, partials, and cost anatomy remain visible evidence, not silent
  success.
- No Cajtucu path becomes a policy, optimizer, Lattice proof, broker API, or
  live-capital authority.

## Future Milestones

```text
paper_online_readiness_contract.v1
  Ledger, market-data, idempotency, session, stale-data, and trace-persistence
  readiness contract before online paper operation.

paper_online_session_contract.v1
  Durable paper-only session state, intake, idempotency, ledger recovery, and
  Cajtucu trace contract before a session runner is enabled.

cajtucu.execution.paper_online.v1
  Live market stream, paper fills, no capital.

cajtucu.execution.route.v2
  Routed/multi-hop execution beyond direct pairs.

cajtucu.execution.broker.v1
  Live broker adapter, credentials, order lifecycle, and kill switches.
```

Live broker work requires:

- Tsodao-approved settings
- Lattice readiness proof
- Marshal handoff receipt
- Runtime execution authority
- no direct policy-to-broker path

## Explicit Non-Goals

- PPO ownership
- policy training
- allocator tuning
- reward judging
- market readiness
- deployment readiness
- hidden live broker API in paper mode
