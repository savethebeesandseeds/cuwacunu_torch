# Cajtucu

Cajtucu is the output and execution pillar.

It owns the mechanics that turn a policy decision into output evidence:

```text
execution intent
  -> rebalance orders
  -> paper or broker backend
  -> fills / rejects / partial fills
  -> execution ledger update
  -> execution trace
```

V1 is paper-only. It has no live broker API, no credentials, no network
execution, and no real-capital authority.

Boundary:

```text
Ujcamei      input/source stream
Wikimyei    belief, policy, and allocation math
Kikijyeba   environment/world loop, reset/step/reward
Cajtucu     output execution, paper fills, ledger, trace
Marshal     bounded orchestration
Lattice     read-only evidence/proof surface
```

The first implemented method is:

```text
cajtucu.execution.paper.v1
```

Its authored contract is:

```text
src/config/cajtucu.execution.paper.dsl
```

Paper V1 consumes:

```text
execution_intent_t
market_execution_state_t
execution_ledger_t
```

and emits:

```text
execution_trace_t
```

The trace records the market source used by the paper backend. If Kikijyeba
replay falls back to synthetic direct asset/reserve edges instead of a real
edge-market state, that fact is carried as trace evidence and a warning.
Synthetic execution markets are rejected by default and must be explicitly
enabled for research replay. When intent equity and mark-to-market ledger equity
differ, paper V1 records the
mismatch and sizes order notionals from the marked ledger equity. A large
equity mismatch produces an invalid trace without execution.

Paper V1 executes sell orders before buy orders so a rebalance can free reserve
before funding new positions. It rejects pathological sell pricing before any
ledger mutation when spread plus slippage would make the sell price invalid.
