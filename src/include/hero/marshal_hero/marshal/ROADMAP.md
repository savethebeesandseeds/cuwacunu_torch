# Marshal Roadmap

Marshal is intentionally small:

```text
hero.marshal.status
hero.marshal.prepare.*
hero.marshal.rollout
hero.marshal.inspect
```

This roadmap tracks future finite coordination work. It does not retain the
completed V2 surface-collapse, cost-aware rollout, or pre-PPO rehearsal history.

## Boundary

```text
Lattice proves.
Runtime executes.
Environment admits environment evidence and certified sidecars.
Marshal coordinates, validates, delegates, records, and explains.
Cajtucu paper-executes replay mechanics under Runtime replay.
```

Marshal must not become:

```text
proof authority
model selector
checkpoint selector
policy trainer
reward judge
config editor
unbounded scheduler
allocation engine
market-readiness authority
deployment authority
```

## Current Stable Surface

```text
status
  read-only Marshal visibility and boundary state

prepare
  one dispatchable Lattice target -> bounded Runtime handoff plan/dry-run/execute

rollout
  completed Runtime job evidence -> bounded replay rollout through Runtime

inspect
  read-only Runtime/Lattice/Marshal evidence explanation
```

Marshal can coordinate bounded cost-aware replay through Runtime and inspect the
resulting Runtime/Lattice evidence. Direct `cuwacunu_exec` remains an explicit
debug path with warning, not the recommended proof-clean operator path.

## Next Marshal Work

### Paper-Online Session Handoff Contract

Milestone:

```text
paper_online_session_contract.v1 handoff support
```

Marshal role:

- Prepare and inspect bounded paper-online-session handoffs without becoming
  proof authority, Environment authority, or executor.
- Require an existing `paper_online_readiness_contract_ready` proof before a
  future session handoff can proceed, and align the handoff fields with the
  Environment `paper_online_session_contract.v1` admission validator.
- Bind policy artifact identity, accepted settings protection, Cajtucu paper
  execution profile, graph-node universe, accounting numeraire, session policy,
  stale-data policy, durable paper-ledger path, idempotency policy, and operator
  abort/kill-switch requirements in plan/dry-run output.
- Delegate any future execution only through Runtime and Environment/Cajtucu
  contracts.
- Quote Lattice readiness deficits and explain next safe actions.
- Preserve authority denials: no policy selection, tuning, reward judgment,
  market readiness, deployment readiness, or live execution.

Acceptance sketch:

- Operators can see what a paper-online session would require before any
  online-paper session can run.
- Marshal packets expose missing prerequisites and next safe actions.
- Marshal never writes policy, ledger, or paper-online execution artifacts
  itself.

## Future Milestone: Tsodao Handoff

Tsodao should protect approved settings after evidence exists. Marshal may
eventually prepare or inspect Tsodao-bound handoffs, but only after Tsodao has a
finite contract for:

- settings identity
- evidence digest binding
- promotion criteria
- allowed mutation policy
- rollback/revocation evidence

Marshal must not use Tsodao as a hidden optimizer.

## Validation

Use the smallest relevant checks for the touched surface.

Marshal dispatch:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
```

Marshal rollout:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_rollout
```

Hero schema compatibility:

```text
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
```

Build:

```text
make -C src/main/hero -j12 build-marshal-hero
```
