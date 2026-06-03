# Lattice Roadmap

This roadmap is now a final review packet for the recovered Lattice expansion.
It is not a backlog for an open-ended goal.

Use it to finish documentation cleanup, run focused validation, and preserve the
architecture boundary that was restored during the interrupted-goal recovery.

## Operating Rules

- Pick one bounded milestone at a time.
- Do not expand roadmap evidence by adding `TARGET_KIND` values.
- Runtime and components write durable evidence.
- Lattice reads evidence, normalizes facts, proves explicit target claims, and
  emits warnings.
- Marshal coordinates and inspects; it does not prove, execute, select
  checkpoints, allocate, or make market/deployment decisions.
- Policy gates remain disabled until thresholds, uncertainty, baselines,
  leakage policy, selector policy, and negative tests are explicit.
- A severe warning may be loud, but it must not become a hidden readiness gate.

## Architecture Boundary

```text
Runtime / components
  write reports, facts, sidecars, checkpoints, replay artifacts

Lattice evidence catalog
  scans durable evidence
  normalizes fact families
  builds summaries
  preserves lineage
  emits warnings

Lattice target proof engine
  proves explicit readiness or artifact claims
  emits proof certificates
  never runs components
  never ranks checkpoints by quality
  never allocates

Future policy gate layer
  remains disabled until decision policy is explicit and tested

Marshal
  reaches dispatchable readiness targets only
  inspects non-dispatchable evidence panels
  never proves or executes
```

## Recovery Ledger

The broad lattice expansion goal has been split into finite completed slices.
Do not reopen these as one umbrella goal.

1. Evidence catalog boundary: complete for the active catalog families.
   Fact families are read-only evidence rows, not target kinds, wave sources,
   checkpoint selectors, policy gates, Marshal reachability surfaces, or
   allocation/deployment authority.

2. Runtime fact producers and fact contracts: complete for the active fact
   families. Durable sidecar/scanner contracts exist for `source_analytics`,
   `target_transform`, `forecast_baseline`, `forecast_eval`,
   `observer_belief`, and `allocation_engine`, with malformed cases preserved
   as warnings or proof failures according to declared authority.

3. Artifact-readiness proof engine: complete for the active proofable families.
   `target_transform_contract_ready`, `forecast_baseline_artifact_ready`,
   `forecast_eval_artifact_ready`, `observer_belief_artifact_ready`, and
   `allocation_artifact_ready` prove narrow artifact existence, identity,
   lineage, support, non-mutation, and completeness. They do not prove quality
   or dispatch Runtime waves.

4. Warning surface cleanup: complete for active warning surfaces. Warning
   envelopes are structured, typed, non-blocking, and visible in Hero output;
   severity does not change target status, plan readiness, suggested waves,
   checkpoint selection, Marshal reachability, or policy-gate status.

5. Hero and Marshal boundary audit: complete for current Lattice surfaces.
   Hero exposes fact summaries, previews, target evaluation, and disabled
   policy-gate reservations as read/proof/inspection data. Marshal routes
   non-dispatchable artifact targets and fact-family panels to read-only
   inspection.

6. Policy gates stay reserved: complete for current reserved/fail-closed
   surfaces. `LATTICE_POLICY_GATE` loads only disabled
   `forecast_quality_acceptance` and `allocation_acceptance` reservations
   bound to matching artifact-readiness proof targets. `ENABLED=true`,
   `TARGET_CLASS=policy_gate`, `performance_acceptance`,
   `validation_performance`, `market_readiness`, and `deployment_readiness`
   fail closed. `POLICY_KIND=performance_acceptance`, `market_readiness`, and
   `deployment_readiness` remain unsupported future decision surfaces.

7. Final documentation and review packet: complete enough for this recovery
   phase. The compact review packet exists, related Markdown now agrees on
   catalog/proof/policy/Marshal boundaries, and the colleague review identified
   hardening tests as the next bounded action before compatibility cleanup.

## Active Milestone

### 8. Lattice Boundary Hardening

Goal:

```text
Turn the recovered architecture boundary into regression tests before deleting
runtime-data compatibility paths or refactoring target-spec types.
```

Deliverables:

- Prove `artifact_readiness` targets stay non-dispatchable and do not inherit
  trainable target machinery.
- Prove warning severity and triggered warning counts do not create target
  deficits, plan readiness, suggested waves, Marshal reachability, checkpoint
  selection, or policy status.
- Keep warning envelopes machine-explicit: `blocking=false`,
  `authority=diagnostic_only`, `affects_target_status=false`,
  `affects_plan=false`, `affects_marshal=false`, and
  `affects_policy_gate=false` where the output surface supports those fields.
- Prove disabled `LATTICE_POLICY_GATE` reservations do not alter target
  fingerprints, target status, proof certificates, warnings, deficits, plans,
  or dispatch advice.
- Prove `latest_satisfying:<target_id>` is deterministic readiness-recency
  selection, not quality or best-metric selection.
- Prove parked `replay_environment` evidence remains default-muted,
  audit-only, and unable to satisfy active targets.

Completion evidence:

- `make -C src/tests/bench/kikijyeba/lattice_target -j12 run-test_kikijyeba_lattice_target`
  passes.
- Run catalog or Marshal tests only if this milestone touches those subsystems.
- `git diff --check` passes.

## Future Discussion: Environment Policy Training

Status: not active.

The environment era must preserve the split between evidence, proof, dispatch,
and acceptance. `replay_environment` facts are parked audit evidence, and
`replay_environment_artifact_ready` is a reserved future proof shape rather
than an active target. It may eventually prove schema identity, report digest
identity, source order, parallelism bounds, time-law cleanliness,
projection-counter presence, and parent evidence binding. It must not become
policy training, policy selection, reward acceptance, market readiness, or
deployment authority.

Lifecycle state:

```text
replay_environment fact family
  known / parked / catalog-listed

replay_environment fact derivation
  disabled by default
  explicit audit scan only

replay_environment_artifact_ready proof template
  designed / reserved / not active yet

replay_environment_artifact_ready target instance
  absent until Runtime replay artifacts and fact contracts are stable

policy_training_artifact_ready
  future dispatchable target only with a concrete Runtime job contract

policy_acceptance
  disabled policy reservation only
```

Use this staged model:

```text
1. Environment facts
   Policy-neutral, read-only catalog rows from Runtime replay artifacts.
   They describe what happened inside the experience, independent of whether
   the actor was a baseline, deterministic Wikimyei policy, learned policy, or
   external policy.

2. Environment artifact readiness
   Reserved non-dispatchable proof template. Once active, it proves
   replay/environment artifacts are complete, schema-consistent,
   lineage-bound, time-law-clean, and bounded. It does not train, select, or
   accept a policy.

3. Policy-training artifact readiness
   Potentially dispatchable later, but only if Runtime has a concrete
   policy-training job contract and writes durable policy-training evidence.
   This proves that a training run produced usable artifacts, not that the
   policy is good.

4. Policy acceptance
   Disabled policy-gate reservation until reward thresholds, mandatory
   baselines, uncertainty, selector/eval split policy, and negative leakage
   tests are explicit.
```

Hero and Marshal surfaces should render disabled gates as
`disabled_policy_reservation` or equivalent reserved-policy language, not as an
active policy gate. A disabled reservation is review metadata and must not
alter target status, target fingerprints, proof certificates, warnings,
deficits, plans, or dispatch advice.

Policy-neutral `replay_environment` fact families should be designed before any
training target:

```text
replay_environment_run
replay_episode
replay_time_law
replay_action_schema
replay_reward_definition
replay_policy_identity
replay_projection_validation
replay_execution_simulation
replay_artifact_lineage
```

These facts may support artifact-readiness proof and operator inspection. They
must not become trainable readiness, reward acceptance, policy selection,
checkpoint ranking, market readiness, or online deployment authority.

Reserved proof wording:

```text
replay_environment_artifact_bound proves:
  replay run artifact exists
  episode artifacts exist
  requested range and accepted cursor identity are preserved
  source order and graph order identity are bound
  action schema digest is present
  reward definition identity digest is present
  acting policy identity is present
  observation/action/execution time-law checks are present and clean
  execution simulation contract is present
  projection-validation counters and metric fields are present when required
  parent forecast, observer, allocation, source, and environment digests bind
  declared parallelism and replay bounds are observed

replay_environment_artifact_bound does not prove:
  reward quality
  profitability
  projection economic validity
  policy quality
  policy acceptance
  policy selection
  deployment readiness
  market readiness
```

Apply the same wording discipline to active artifact templates:

```text
forecast_eval_artifact_bound
  Say: declared selection-signal fact digests are present and lineage-bound
  when the contract requires them.
  Do not imply: checkpoint selection, ranking, acceptance, or recommendation.

observer_belief_artifact_bound
  Say: scenario/covariance diagnostic fields are present and lineage-bound.
  Do not imply: confidence, liquidity, data quality, tradability, or return
  validity is acceptable.

allocation_artifact_bound
  Say: objective, constraint, cost, CVaR, turnover, reserve, and
  fallback/de-risk fields are present and lineage-bound.
  Do not imply: feasibility, optimality, suitability, execution readiness,
  portfolio readiness, or market readiness.
```

A future policy-training target may be dispatchable only when it has a concrete
Runtime execution contract:

```text
producer component family
Runtime job kind
environment/replay contract version
episode bundle identity and source ranges
max environment jobs, workers, episodes, and attempts
resume ledger identity and terminal stop condition
report schema
checkpoint or policy-artifact contract
parent forecast/observer/allocation/evidence digests
reward definition digest
anti-leakage and selector-split policy
post-run Lattice target to recheck
```

Parallel environment rollout is Runtime evidence production. Lattice may prove
that the produced episode/training evidence is complete, bounded, time-law
clean, lineage-bound, and leakage-clean; it must not run workers, allocate
capital, rank policies, or choose the best checkpoint.

For the current deterministic policy, there is no policy-training target. The
useful Lattice work is environment artifact readiness and projection-validation
evidence over the same action/reward/report grammar used by baselines and future
learned policies.

Do not add `environment_ready`, `policy_good`, `policy_accepted`,
`market_ready`, or similar targets as shorthand. First define fact families,
artifact contracts, proof kind, target class, and policy-gate reservation
boundaries.

Open questions for the next design session:

```text
Which policy artifacts are checkpoints, and which are deterministic artifacts?
What reward definition digest is authoritative?
Which split selects policy checkpoints, and which split evaluates them?
What baselines are mandatory before any acceptance gate is even reserved?
How are episode independence and replay/live separation proven?
What negative tests catch time-law and selector leakage?
```

## Documentation Audit

Keep these files aligned during final cleanup:

- `src/include/kikijyeba/lattice/LEGACY_AUDIT.md`
  marks which legacy/compatibility surfaces are runtime-data cleanup candidates
  after `dev_nuke` and which ones are active API or target behavior.
- `src/include/kikijyeba/lattice/README.md`
  must describe fact families as read-only evidence, artifact-readiness targets
  as narrow proofs, `latest_satisfying` as deterministic readiness selection,
  warnings as non-blocking, and policy gates as reserved/fail-closed.
- `src/config/README.md`
  must match the current config/DSL behavior and avoid implying policy,
  performance, market-readiness, deployment, allocation, scheduler, or
  execution authority.
- `src/config/man/hero.lattice.man`
  must match the Lattice Hero tool behavior and schema fields.
- `src/main/hero/README.md`
  must keep Hero as read/proof/inspection and Marshal as bounded coordination.
- `src/include/kikijyeba/lattice/ROADMAP.md`
  must stay finite and should not accumulate historical completion logs.

## Completion Audit

Use this mapping when reviewing the final diff.

- Evidence catalog boundary:
  catalog families expose identity, lineage, support, and authority flags; fact
  rows do not satisfy trainable readiness by themselves.
- Target proof boundary:
  active artifact-readiness targets prove only artifact integrity and
  completeness; they are non-dispatchable and do not select checkpoints.
- Warning boundary:
  warnings expose diagnostics and inspection hints; they never block readiness
  or become hidden policy gates.
- Hero/Marshal boundary:
  Hero reports proof and catalog state; Marshal inspects or prepares bounded
  handoffs only for dispatchable readiness targets.
- Policy-gate boundary:
  disabled reservations are review metadata; enabled or unsupported decision
  surfaces fail closed.
- `latest_satisfying` boundary:
  readiness-based checkpoint selection is not a best-model, Pareto,
  performance, deployment, or scalar-score selector.

## Validation Groups

Run only the smallest group needed for the final review. For a full final
packet, run the focused set below.

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

Marshal boundary:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
```

Diff hygiene:

```text
git diff --check
```

## Explicit Non-Goals

Do not add these as active work without a new bounded roadmap section and
explicit policy requirements:

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

## Next Candidate Milestone

Only open this after `dev_nuke` has removed old `.runtime` records and the
boundary-hardening tests pass:

```text
Runtime-Data Compatibility Cleanup
  remove runtime-data compatibility readers and fallback paths marked in
  LEGACY_AUDIT.md, while keeping active trainable targets, Marshal
  reachability, artifact-readiness proofs, and parked environment support
  intact.
```
