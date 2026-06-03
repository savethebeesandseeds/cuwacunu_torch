# Cajtucu Roadmap

## Current Milestone

```text
cajtucu.execution.paper.v1
```

Goal:

```text
Introduce a backend-neutral execution contract and a paper backend that converts
target-weight intents into rebalance orders, simulated fills/rejects, ledger
updates, and durable execution traces.
```

The milestone is intentionally not:

```text
live broker execution
paper-online world mode
PPO
allocator tuning
evaluation authority
Lattice target satisfaction
```

## Acceptance

- Cajtucu defines execution intents, orders, paper fills, ledgers, traces, and a
  backend interface.
- The paper backend supports fee, spread, slippage, min/max notional, tradable
  masks, insufficient reserve/unit rejects, and optional partial fills.
- The paper backend executes sells before buys.
- The paper backend rejects synthetic direct-edge market sources by default;
  research replay must explicitly opt in when synthetic markets are needed.
- The paper backend returns an invalid trace, without ledger mutation, for
  large equity mismatches and invalid sell-price cost models.
- Kikijyeba replay can call the Cajtucu paper backend instead of relying on a
  private target-weight simulator.
- Replay rewards are based on the portfolio state after Cajtucu ledger mutation
  and the revealed historical return.
- Execution traces include policy/method/run/episode/anchor/backend identity.
- Execution traces include market source identity and explicitly warn when
  replay uses synthetic direct asset/reserve edges.
- Execution traces warn when intent equity and mark-to-market ledger equity
  disagree beyond tolerance, and paper order sizing uses marked ledger equity.
- V1 exposes no credential or live broker surface.

## Deferred

- `kikijyeba.environment.paper_online.v1`
- live broker adapters
- broker credential handling
- order-book depth simulation
- route/path execution beyond direct reserve edges
- learned policies and PPO
