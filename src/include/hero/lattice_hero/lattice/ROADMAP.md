# Lattice Roadmap

This roadmap is the active Lattice work index. It is not a history log for the
recovered Lattice expansion, Hero surface collapse, or completed artifact
readiness contracts.

## Operating Rules

- Runtime and components write durable evidence.
- Lattice reads evidence, normalizes facts, proves explicit target claims, and
  emits warnings.
- Lattice does not execute, schedule, optimize, select checkpoints by quality,
  allocate, or make market/deployment decisions.
- Marshal coordinates and inspects; it does not prove.
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
tsodao_settings_protection_ready
policy_acceptance_contract_ready
paper_online_readiness_contract_ready
```

`replay_environment_artifact_ready` proves replay report completeness, lineage,
boundedness, schema identity, time-law cleanliness, projection counter coverage,
Cajtucu execution evidence, execution profile binding, policy set binding, and
authority denials.

`policy_training_artifact_ready` proves policy-training artifact identity,
checkpoint identity, training/validation/test range separation, causal schedule
identity, ledger-derived no-future snapshot use, typed cursor-key ordering,
normalization/replay-buffer/reward-baseline isolation, selector/test discipline,
parent evidence binding, PPO artifact lineage, optimizer archive evidence,
Runtime CUDA verification, resume-mode consistency, and authority denials.

For PPO adapter facts, Lattice may bind forecast-eval and observer-belief
parents plus replay lineage through either a replay-environment fact digest or a
replay report digest. Allocation-engine parent facts are required only for
policy kinds that declare that parent.

For PPO adapter facts, Lattice requires the readable optimizer-state receipt and
the resumable Torch optimizer archive to be schema/digest-bound. It also
requires `device_policy=require_cuda`, `runtime_device_kind=cuda`, CUDA
availability, module-parameter, forward-input, loss, and optimizer-state CUDA
checks, and `cuda_verification_passed=true`. Resume evidence is mode-specific:
`fresh_spawn` must not bind `resume_*` artifacts or load optimizer state,
`resume_weights` must bind and load a parent actor checkpoint, and
`resume_weights_and_optimizer` must also bind and load the optimizer state.
Fresh on-policy collection may load the separately recorded
`input_policy_checkpoint_digest`; that is collection lineage, not resume
lineage.

`tsodao_settings_protection_ready` proves that a protected settings bundle is
bound to a specific `policy_training_artifact_ready` fact. The proof resolves
the referenced policy-training fact digest, checks that the protected
policy/training/action/reward/execution/causal/checkpoint digests match the
policy-training artifact, requires bound proof/evidence digests, and rejects any
Tsodao claim to optimize settings, select policy checkpoints, judge
quality/performance, declare market/deployment readiness, execute, or authorize
live capital.

`policy_acceptance_contract_ready` proves a read-only policy acceptance artifact
binding. It resolves the accepted `policy_training_artifact_ready` fact and the
referenced `tsodao_settings_protection_ready` fact, checks policy/checkpoint,
report, reward, execution, accounting, the named
`policy_acceptance_governance_thresholds_v0.v1` acceptance policy, mandatory
baseline set, after-cost metric, uncertainty, selector/test split,
negative-test, threshold-audit, tie-policy, and promotion-criteria fields, and
rejects any policy-selection, checkpoint-selection, optimizer, allocation,
execution, quality/performance, market/deployment readiness, or live-capital
authority.

`paper_online_readiness_contract_ready` proves a read-only paper-online readiness
artifact binding. It resolves the referenced policy-acceptance and
Tsodao-settings-protection evidence, checks accepted policy identity, session
lifecycle, clock/staleness, idempotency, duplicate-action/execution protection,
persistent paper-ledger recovery, direct-edge universe validation, locked
Cajtucu execution profile, reward/report artifact policy, abort/kill-switch
semantics, and rejects any policy-selection, broker, market/deployment
readiness, paper-online execution, or live-capital authority claim.

Lattice does not prove:

- policy quality
- profitability
- checkpoint ranking
- market readiness
- deployment readiness
- PPO economic correctness

## Current Proof Closure

### PPO Runtime State Evidence Closure

Milestone:

```text
policy_training_resume_lattice_closure.v1
```

Status:

```text
complete
```

Lattice role:

- Keep `policy_training_artifact_ready` focused on artifact integrity after
  Runtime introduced real Torch autograd PPO updates and explicit resume modes.
- Require module-state, optimizer-state receipt, Torch optimizer archive, PPO
  config, action distribution, feature manifest, causal schedule, rollout
  collection, update report, validation report, comparison report, CUDA
  verification, and resume-mode evidence to remain bound.
- Preserve all authority denials: no policy quality, no selection, no market
  readiness, no deployment readiness, and no live-capital proof.

Acceptance sketch:

- PPO facts without optimizer Torch archive evidence fail closed.
- PPO facts without Runtime CUDA verification fail closed.
- Unsupported or internally inconsistent `resume_mode` evidence fails closed.
- Autograd PPO facts do not weaken old causal, range, parent, checkpoint, or
  authority requirements.

### Tsodao Settings Protection

Milestone:

```text
tsodao_settings_protection_contract.v1
```

Status:

```text
complete
```

Lattice role:

- Normalize `kikijyeba.lattice.tsodao_settings_protection.v1` facts from
  Runtime job sidecars.
- Prove only protected-settings artifact binding through
  `tsodao_settings_protection_ready`.
- Cross-check protected settings against the referenced
  `policy_training_artifact_ready` fact digest.
- Preserve all authority denials: no setting search, policy selection,
  policy-quality claim, market/deployment readiness, execution authority, or
  live-capital authority.

Acceptance sketch:

- Valid protected-settings facts satisfy `tsodao_settings_protection_ready`.
- Missing referenced policy-training facts fail closed.
- Protected setting digest drift fails closed.
- Any Tsodao optimizer/selector/readiness/execution/live authority flag fails
  closed.

### Policy Acceptance Contract

Milestone:

```text
policy_acceptance_contract.v1
```

Status:

```text
complete
```

Lattice role:

- Normalize `kikijyeba.lattice.policy_acceptance.v1` facts from Runtime job
  sidecars.
- Prove only the acceptance artifact contract through
  `policy_acceptance_contract_ready`.
- Cross-check the accepted policy-training digest against
  `policy_training_artifact_ready` evidence and the protected-settings digest
  against `tsodao_settings_protection_ready` evidence.
- Require the named `policy_acceptance_governance_thresholds_v0.v1` policy,
  mandatory baselines, after-cost metrics, uncertainty policy, selector/test
  split discipline, negative leakage tests, threshold-selection audit, tie
  policy, cost/slippage assumptions, promotion criteria, and explicit
  non-selection/non-deployment authority denials.

Acceptance sketch:

- Valid acceptance facts satisfy `policy_acceptance_contract_ready`.
- Stale or arbitrary acceptance-policy digests fail closed.
- Missing policy-training or Tsodao parent facts fail closed.
- Accepted policy/checkpoint/report/reward/execution digest drift fails closed.
- Failed acceptance metric, baseline, uncertainty, negative-test, or promotion
  criteria fail closed.
- Any policy-selection, checkpoint-selection, optimizer, allocation, execution,
  quality/performance, market/deployment readiness, or live authority flag fails
  closed.

### Paper-Online Readiness Contract

Milestone:

```text
paper_online_readiness_contract.v1
```

Status:

```text
complete
```

Lattice role:

- Normalize `kikijyeba.lattice.paper_online_readiness.v1` facts from Environment
  certification sidecars.
- Prove only the readiness artifact contract through
  `paper_online_readiness_contract_ready`.
- Cross-check policy-acceptance and Tsodao settings-protection lineage before a
  future paper-online session can consume the evidence.
- Preserve all authority denials: no session execution, broker access, market
  readiness, deployment readiness, or live-capital proof.

Acceptance sketch:

- Valid readiness facts satisfy `paper_online_readiness_contract_ready`.
- Missing or stale policy-acceptance parent facts fail closed.
- Missing session, clock/staleness, ledger-recovery, idempotency, execution
  profile, graph universe, abort/kill-switch, or authority-denial evidence
  fails closed.
- Any paper-online fact claiming policy selection, execution authority beyond
  paper mode, deployment readiness, market readiness, broker access, or live
  capital authority fails closed.

## Next Lattice Work

### Paper-Online Session Evidence Contract

Milestone:

```text
paper_online_session_contract.v1 proof support
```

Lattice role:

- Define the read-only artifact target shape for a future paper-only session
  report that consumes the Environment `paper_online_session_contract.v1`
  validator surface.
- Prove only session contract identity, readiness-proof binding, durable
  session state, market-data staleness enforcement, ledger recovery,
  idempotency/duplicate rejection, Cajtucu paper-execution trace binding,
  operator abort/kill-switch evidence, and authority denials.
- Do not prove policy quality, market readiness, deployment readiness, or live
  execution safety.

Acceptance sketch:

- Missing readiness-proof lineage, session state, staleness enforcement,
  ledger recovery, idempotency, Cajtucu paper trace evidence, abort/kill-switch,
  or authority-denial evidence fails closed.
- Any session fact claiming policy selection, broker access, deployment
  readiness, market readiness, or live capital authority fails closed.

## Optional Lattice Work

### Optional Causal Schedule Subtarget

Milestone:

```text
policy_training_causal_schedule_ready.v1
```

Add only if `policy_training_artifact_ready` becomes too dense to explain
cleanly.

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
