# Marshal Roadmap

Marshal is intentionally small:

```text
hero.marshal.status
hero.marshal.prepare
hero.marshal.rollout
hero.marshal.inspect
```

This roadmap tracks future finite coordination work. It does not retain the
completed V2 surface-collapse history.

## Boundary

```text
Lattice proves.
Runtime executes.
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

The current surface uses `next_safe_actions`, public authority flags, and the
collapsed Lattice tool names:

```text
hero.lattice.inspect
hero.lattice.evaluate
hero.lattice.compare
```

## Active Milestone: Cost-Aware Rollout Handoff

Goal:

```text
Make hero.marshal.rollout the repeatable operator path for bounded cost-aware
replay evidence.
```

Milestone name:

```text
cost_aware_paper_replay_rollout.v1
```

Acceptance:

- `rollout requested_mode=plan` returns a bounded replay plan with no Runtime
  execution.
- `rollout requested_mode=execute` delegates to Runtime replay.
- Request binds:
  - rollout id
  - rollout attempt id
  - idempotency key
  - completed Runtime job dir
  - replay batch index path
  - graph order fingerprint
  - asset universe digest
  - base reserve node
  - risky node set
  - non-empty policy set
  - positive max steps
  - finite parallelism
  - Cajtucu paper execution profile digest
- Execution profile digest appears in the rollout plan, Runtime replay handoff,
  report, and Marshal inspection/summary.
- Validation rollouts require nonzero execution costs.
- Zero-cost rollouts are research-only and labeled
  `frictionless_research_replay`.
- Synthetic execution markets are rejected in validation rollout. Research
  replay must opt in explicitly with a reason and mark validation satisfaction
  false.
- Required policy set includes base reserve, current-weight/no-trade,
  equal-weight, and deterministic Wikimyei SDU.
- Runtime reports separate projection quality, portfolio outcome, execution
  feasibility, cost, target-tracking, and time-law metrics.
- Marshal records handoff identity but does not produce metrics, select
  policies, or prove target satisfaction.

Marshal rollout must not become:

```text
PPO training
policy selection
allocator tuning
Lattice proof
paper-online scheduler
live execution path
```

## Next Milestone: Environment Evidence Inspection

Goal:

```text
Add read-only Marshal inspection panels for replay_environment evidence once
Lattice exposes the fact family.
```

Marshal may inspect:

- replay run identity
- episode counts
- time-law counters
- projection-validation counters
- Cajtucu execution trace counters
- execution profile digest and policy set digest
- requested/executed/rejected turnover
- target tracking error
- fee, spread, slippage, and total transaction cost
- report digest/lineage evidence
- policy set identity
- reward definition identity

Marshal must not:

- dispatch from non-dispatchable environment artifact targets
- judge reward quality
- choose policies
- tune PPO
- rank checkpoints
- claim market readiness

## Future Milestone: Policy Training Handoff

Status: Lattice artifact-readiness target exists and Runtime has a
contract-only policy-training handoff surface. Execution handoff remains blocked
until Runtime implements a dedicated trainable-policy runner.

Potential public path:

```text
hero.marshal.prepare
  target_id=policy_training_artifact_ready
```

Today this target is non-dispatchable artifact evidence. Marshal may inspect it
and report missing/complete artifact evidence, but it must not train or create a
policy-training job.

Implemented prerequisite:

```text
runtime_policy_training_job_contract.v1
```

The target may become dispatchable only if Lattice target advice points to a
bounded executable Runtime policy-training job with:

```text
max episodes
max workers
max attempts
wall-clock budget
source ranges
training/validation/test split policy
causal_walk_forward_training.v1 schedule digest
typed cursor-key ordering
derived no_future_snapshot_use evidence
reward definition digest
execution profile digest
resume ledger identity
policy artifact output contract
post-run Lattice target to recheck
```

Trainable policy handoff must execute through Runtime first. Marshal may prepare
the handoff only after Runtime owns the training runner and the request binds
causal schedule identity, typed cursor-key ordering, and ledger-derived
no-future snapshot evidence. `offline_full_window_research` remains diagnostic
evidence only and must not be dispatched as policy-training readiness.

Marshal may coordinate the handoff. It must not train, select, tune, or accept
the policy.

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
