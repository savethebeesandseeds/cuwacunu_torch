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

Milestone:

```text
cost_aware_paper_replay_rollout.v1
```

Goal:

```text
Make hero.marshal.rollout the repeatable operator path for bounded cost-aware
replay evidence.
```

Acceptance:

- `rollout requested_mode=plan` returns a bounded replay plan with no Runtime
  execution.
- `rollout requested_mode=execute` delegates to Runtime replay.
- Request binds rollout id, attempt id, idempotency key, completed Runtime job
  dir, replay batch index path, graph order fingerprint, asset universe digest,
  accounting numeraire node, ordered target-node universe, non-empty policy set,
  positive max steps, finite parallelism, and Cajtucu paper execution profile
  digest.
- Execution profile digest appears in the rollout plan, Runtime replay handoff,
  report, and Marshal inspection/summary.
- Validation rollouts require nonzero execution costs.
- Zero-cost rollouts are research-only and labeled
  `frictionless_research_replay`.
- Synthetic execution markets are rejected in validation rollout. Research
  replay must opt in explicitly with a reason and mark validation satisfaction
  false.
- Required policy set includes numeraire-only, current-weight/no-trade,
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

## Active Milestone: Pre-PPO Rehearsal Coordination

Milestone:

```text
pre_ppo_full_pipeline_rehearsal.v1
```

Marshal role:

- Prepare or delegate bounded Runtime waves where a Lattice target supplies
  dispatch advice.
- Preserve proof-clean checkpoint inputs as Runtime handoff fields, not static
  config edits.
- Coordinate cost-aware rollout through `hero.marshal.rollout`.
- Inspect Lattice/Runtime evidence after the run.
- Refuse to train PPO or select winning checkpoints.

Acceptance:

- The rehearsal can be followed by operators using Marshal/Runtime/Lattice Hero
  tools without manual checkpoint copying into `.jkimyei` profiles.
- Direct `cuwacunu_exec` remains an explicit debug path with warning, not the
  recommended proof-clean path.
- Marshal packets continue to expose authority denials and `next_safe_actions`.

## Future Milestone: Policy Training Handoff

Current prerequisite: Lattice artifact-readiness target exists and Runtime has a
causal policy-training contract plus a narrow `noop_policy_training.v1` pre-PPO
smoke execute path. PPO execution handoff remains blocked until Runtime
implements a dedicated trainable-policy runner.

Potential public path:

```text
hero.marshal.prepare
  target_id=policy_training_artifact_ready
```

Today this target is non-dispatchable artifact evidence. Marshal may inspect it
and report missing/complete artifact evidence, but it must not train or create a
PPO job.

Trainable policy handoff may become dispatchable only if Lattice target advice
points to a bounded executable Runtime policy-training job with:

- max episodes, workers, attempts, and wall-clock budget
- source ranges and split policy
- causal walk-forward schedule digest
- typed cursor-key ordering
- ledger-derived no-future snapshot evidence
- reward definition digest
- execution profile digest
- resume ledger identity
- policy artifact output contract
- post-run Lattice target to recheck

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
