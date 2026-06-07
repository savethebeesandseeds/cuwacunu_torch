# Lattice Roadmap

This roadmap is the active Lattice work index. It is not a history log for the
recovered Lattice expansion, completed Hero surface collapse, or completed
artifact-readiness contracts.

## Operating Rules

- Runtime and components write durable evidence.
- Lattice reads evidence, normalizes facts, proves explicit target claims, and
  emits warnings.
- Lattice does not execute, schedule, optimize, select checkpoints by quality,
  allocate, or make market/deployment decisions.
- Marshal coordinates and inspects; it does not prove.
- Policy gates remain disabled until thresholds, uncertainty, baselines,
  selector policy, leakage policy, and negative tests are explicit.
- Warnings are diagnostic. They do not become hidden readiness gates.

## Current Stable Surface

```text
hero.lattice.status
hero.lattice.inspect
hero.lattice.evaluate
hero.lattice.compare
```

Stable proof/evidence boundaries:

```text
fact families
  read-only catalog evidence, not target kinds

artifact_readiness targets
  narrow non-dispatchable proof targets for artifact existence, identity,
  lineage, support, non-mutation, and completeness

warnings
  non-blocking diagnostic evidence

latest_satisfying
  deterministic readiness-recency selection, not best-model selection

policy gates
  reserved/fail-closed only
```

## Current Proof Baseline

Implemented non-dispatchable artifact targets:

```text
replay_environment_artifact_ready
policy_training_artifact_ready
```

`replay_environment_artifact_ready` proves replay report completeness, lineage,
boundedness, schema identity, time-law cleanliness, projection counter coverage,
Cajtucu execution evidence, execution profile binding, policy set binding, and
authority denials.

It does not prove:

- reward quality
- profitability
- projection economic validity
- policy quality
- policy acceptance
- checkpoint ranking
- market readiness
- deployment readiness

`policy_training_artifact_ready` proves policy-training artifact identity,
checkpoint identity, training/validation/test range separation, causal schedule
identity, ledger-derived no-future snapshot use, typed cursor-key ordering,
normalization/replay-buffer/reward-baseline isolation, selector/test discipline,
parent evidence binding, and authority denials.

It does not prove:

- policy quality
- policy acceptance
- profitability
- market readiness
- deployment readiness
- PPO correctness

## Active Milestone: Pre-PPO Proof Rehearsal

Milestone:

```text
pre_ppo_full_pipeline_rehearsal.v1
```

Lattice role:

- Prove `replay_environment_artifact_ready` from a fresh cost-aware replay
  report.
- Prove `policy_training_artifact_ready` from a fresh no-op policy-training
  artifact.
- Confirm no proof path accepts schedule-less policy-training evidence.
- Confirm `offline_full_window_research` remains non-readiness evidence.
- Confirm direct Runtime/exec debug paths do not become proof authority.

Acceptance:

- All proofs are derived from durable artifacts, not caller assertions.
- `causal_schedule_no_future_snapshot_use` is derived from artifact fit/use
  ledgers.
- Opaque cursor keys fail closed unless a sortable ordering kind or resolved
  ordering map is bound.
- Lattice may report latest satisfying artifacts by recency, but never as
  best-model or policy-quality selection.

## Optional Refinement Before Executable PPO

Milestone:

```text
policy_training_causal_schedule_ready.v1
```

Add only if the broad `policy_training_artifact_ready` target becomes too dense
to explain cleanly.

Scope:

- schedule schema identity
- schedule digest binding
- block/fit/use ledger completeness
- typed cursor ordering
- no same-block snapshot use
- no future snapshot use
- exclusion of full-window research from readiness

`policy_training_artifact_ready` would continue to prove the broader policy
artifact lineage, range, selector, checkpoint, and parent-evidence contract.

## Reserved: Policy Acceptance And Tsodao

Policy acceptance remains a disabled policy-gate topic until these are explicit:

```text
mandatory baselines
reward thresholds
uncertainty policy
selector split
test split
negative leakage tests
cost/slippage assumptions
promotion criteria
Tsodao settings-protection contract
```

Tsodao should later protect approved settings and evidence digests. Lattice may
prove that Tsodao-bound settings match evidence, but Lattice must not search for
or choose those settings.

## Validation Groups

Run only the smallest group needed for the current change.

Catalog and fact contracts:

```text
make -C src/tests/bench/kikijyeba/lattice_exposure -j12 run-test_kikijyeba_lattice_exposure
```

Target proof engine:

```text
make -C src/tests/bench/kikijyeba/lattice_target -j12 run-test_kikijyeba_lattice_target
```

Hero schema compatibility:

```text
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
```

Diff hygiene:

```text
git diff --check
```

## Explicit Non-Goals

- scalar lattice score
- best checkpoint selector
- forecast quality approval
- portfolio or allocation approval
- market readiness
- deployment readiness
- scheduler authority
- Runtime execution authority inside Lattice
- Marshal proof authority
- DB/cache rows as source of truth
