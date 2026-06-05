# Active Roadmap

This is the short operator-facing roadmap. Keep it finite. Do not use this
file as a history log for completed Hero cleanup, Lattice recovery, Runtime
replay plumbing, Marshal contract refactors, or Cajtucu paper-engine hardening.

Detailed subsystem records live in:

```text
src/include/kikijyeba/environment/ROADMAP.md
src/include/kikijyeba/lattice/ROADMAP.md
src/include/kikijyeba/marshal/ROADMAP.md
src/include/cajtucu/ROADMAP.md
src/include/wikimyei/inference/expected_value/mdn/README.md
.deprecated/src_legacy/MIGRATION_INVENTORY.md
```

## North Star

```text
Runtime executes and writes durable evidence.
Lattice proves target satisfaction from Runtime evidence.
Marshal prepares bounded handoffs, delegates to Runtime, records, and explains.
Kikijyeba Environment drives reset/step/reward/trajectory evidence.
Wikimyei owns representation, belief, and policy/allocation math.
Cajtucu owns output execution, paper fills, ledgers, and execution traces.
Tsodao later protects approved settings and promotion contracts.
```

Boundary rules:

```text
Runtime is not proof authority.
Lattice is not an executor, scheduler, optimizer, or policy selector.
Marshal is not proof authority, config editor, checkpoint selector, policy
  trainer, reward judge, or unbounded scheduler.
Kikijyeba Environment is not a broker or policy trainer by itself.
Cajtucu is not a policy, evaluator, readiness oracle, or live-capital authority.
Tsodao is not an optimizer; it protects selected settings after evidence exists.
```

## Stable Checkpoint

Status: current base.

The Hero public surfaces have been collapsed and tested enough to use as a
stable checkpoint:

```text
Config Hero:
  hero.config.status
  hero.config.inspect
  hero.config.apply

Runtime Hero:
  hero.runtime.status
  hero.runtime.inspect
  hero.runtime.run
  hero.runtime.reset

Lattice Hero:
  hero.lattice.status
  hero.lattice.inspect
  hero.lattice.evaluate
  hero.lattice.compare

Marshal Hero:
  hero.marshal.status
  hero.marshal.prepare
  hero.marshal.rollout
  hero.marshal.inspect
```

Do not reopen Hero cleanup as an umbrella goal. Future Hero changes must be
finite, local, and tied to one tool family or one public contract.

Current config-surface baseline:

```text
Runtime Hero uses one policy file:
  src/config/hero.runtime.dsl

Runtime profile selection order:
  --profile
  [HERO].runtime_hero_profile
  runtime_profile in the policy file

Checked-in profiles:
  locked_default
    wave/train execution disabled
    guarded developer reset available

  train_operator
    wave/train execution enabled with confirmation
    developer reset disabled

Protocol identity binds:
  GRAPH_TOPOLOGY
  NODELIFT
  REPRESENTATION
  INFERENCE
  OBSERVER
  ALLOCATION_POLICY
  REPRESENTATION_CONTRACT

Policy parameter DSLs still own policy parameters.
Marshal rollout still owns comparison policy sets and Cajtucu execution profile.
Kikijyeba replay DSL still owns world/reset-step/time-law/reward contract.
```

## Active Sequence

### 1. Cost-Aware Paper Replay Rollout V1

Goal:

```text
completed Runtime job
  -> hero.marshal.rollout
  -> Runtime replay
  -> Kikijyeba replay environment
  -> Cajtucu paper execution
  -> trajectory/report artifacts
  -> Lattice-readable evidence
```

Milestone name:

```text
cost_aware_paper_replay_rollout.v1
```

Acceptance:

- Rollouts run over multiple validation windows from completed Runtime job
  evidence.
- Environment assembly is `kikijyeba.environment.replay.v1`.
- Execution backend is `cajtucu.execution.paper.v1`.
- Cajtucu paper execution costs are nonzero and profile-bound.
- Execution profile digest is recorded in the rollout plan, Runtime replay
  handoff, report, and Marshal receipt/summary.
- Synthetic direct-edge markets are forbidden for validation rollouts; research
  replay must opt in explicitly and mark validation satisfaction false.
- Required policies include base reserve, current-weight/no-trade,
  equal-weight, and deterministic Wikimyei SDU.
- Baseline policies and deterministic Wikimyei policy share the same execution
  profile, reward contract, source range, and accounting assumptions.
- Reports separate projection metrics, portfolio metrics, and execution
  feasibility metrics.
- Reports include policy-wise final equity, total log growth, drawdown,
  requested/executed/rejected turnover, transaction costs, target tracking
  error, invalid trace count, rejects, partial fills, and synthetic-market step
  count.
- Reports include Cajtucu trace presence/validity counters and cost anatomy:
  fee, spread, slippage, and total transaction cost.
- Aggregate metrics declare scope: step, episode, policy, bundle, or experiment.
- Time-law counters remain explicit and fail closed on leakage.
- Marshal does not claim Lattice satisfaction, policy quality, training
  authority, market readiness, or deployment readiness.

### 2. Environment Lattice Targets

Goal:

```text
Add policy-neutral Lattice evidence and narrow proof targets for replay
environment health before any trainable policy is introduced.
```

The cost-aware rollout report is the evidence producer. The next milestone is
to make Lattice read those durable Runtime/Kikijyeba/Cajtucu artifacts and
prove only artifact readiness.

Initial targets/facts should cover:

- no future observation leakage
- train/validation/test range separation
- resolved cursor identity and accepted range identity
- action schema identity
- reward definition identity
- Cajtucu execution trace validity
- rollout completeness and boundedness
- projection-validation coverage
- policy identity and method identity

The first target is now active as a non-dispatchable artifact proof:

```text
replay_environment_artifact_ready
```

It proves artifact completeness, lineage, schema identity, report digest
identity, source order, bounded parallelism, time-law cleanliness, and required
projection and Cajtucu counters. It does not prove profitability, policy
quality, projection economic validity, market readiness, or deployment
readiness.

Implementation checkpoint:

```text
Goal: replay_environment_artifact_ready.v1

Status:
Implemented as a Lattice artifact-readiness target and fact proof path.

Bound evidence:
- replay report digest and runtime/environment/experiment identity
- execution_profile_digest and policy_set_digest
- time-law step counters and leakage counters
- projection-validation counters
- Cajtucu trace validity, reject, synthetic-market, order, notional, cost, and
  target-tracking counters
- authority denials

Verified:
- Lattice scanner/fact summary tests
- Lattice target proof tests
- environment replay contract tests
- runtime job-runner replay report tests
- Lattice Hero build
- live hero.lattice.evaluate proves replay_environment_artifact_ready from a
  completed Runtime job's cost-aware Cajtucu replay report
```

### 3. Policy Training Artifact Contract V1

Goal:

```text
Define the artifact and anti-leakage contract for trainable policies before
implementing PPO.
```

Milestone name:

```text
policy_training_artifact_contract.v1
```

The policy artifact contract must bind:

```text
policy_id
policy_kind
policy_architecture_digest
training_config_digest
training_range_digest
validation_range_digest
test_range_digest
environment_contract_id
observation_schema_digest
action_schema_digest
reward_contract_digest
execution_profile_digest
training_schedule_mode
causal_schedule_schema_id
causal_schedule_digest
causal_schedule_cursor_key_kind
causal_schedule_no_future_snapshot_use_source
causal_schedule_no_future_snapshot_use
normalization_fit_range_digest
replay_buffer_source_range_digest
early_stopping_policy_digest
hyperparameter_selection_policy_digest
random_seed
parent_checkpoint_digest
checkpoint_digest
parent forecast/observer/allocation/environment evidence digests
```

Hard rule:

```text
Training may see only training ranges.
Validation may compare or select under explicit selector policy.
Test remains sealed until final report.

For readiness-grade policy training, every representation, MDN,
observer/belief, normalization, calibration, covariance/coupler, replay-buffer,
reward-baseline, and policy snapshot used in a rollout block must have been
usable before that block began. Train/validation/test disjointness is necessary,
but not sufficient.
```

No optimizer state, normalization, calibration, covariance/coupler, reward
baseline, replay buffer, early-stop decision, or hyperparameter choice may be
fit using future validation/test information. Target-label, reward, and
trajectory availability must be no later than the artifact `usable_from_key`.

Full-window same-range upstream training is quarantined as:

```text
offline_full_window_research
```

It can remain useful for diagnostics and ablations, but it cannot satisfy
policy_training_artifact_ready, policy-training readiness, rollout readiness, or
market-readiness claims.

Implementation checkpoint:

```text
Goal: policy_training_artifact_contract.v1

Status:
Implemented at the Lattice artifact-readiness layer.

Bound evidence:
- policy identity, kind, architecture digest, config digest, random seed, parent
  checkpoint digest, and produced checkpoint digest
- training, validation, and test range digests
- environment, observation, action, reward, and Cajtucu execution profile
  identities
- causal walk-forward schedule mode/schema/digest and no-future-snapshot-use
  evidence derived from artifact fit/use ledgers
- typed cursor-key ordering and rejected opaque cursor keys
- normalization-fit range, replay-buffer source range, reward-baseline
  isolation, label/reward/trajectory availability, early-stopping policy,
  hyperparameter selector policy, and sealed test-access evidence
- parent forecast-eval, observer-belief, allocation-engine, and replay-
  environment fact digests
- authority denials

Verified:
- Lattice exposure scanner/fact summary tests
- Lattice target proof tests
- active Lattice target policy_training_artifact_ready
```

PPO or any trainable policy still must enter as a Runtime-owned job before
Marshal can dispatch it. The current Runtime surface validates the
policy-training contract only; executable training remains future work. Marshal
coordinates bounded handoffs, Runtime owns execution when a runner exists, and
Lattice reads the artifacts.

### 4. Runtime Policy-Training Job Contract V1

Goal:

```text
Define the bounded Runtime contract for future policy_training artifacts without
giving Marshal, Lattice, or the policy itself hidden execution or selection
authority.
```

Milestone name:

```text
runtime_policy_training_job_contract.v1
```

The Runtime contract should define:

- job kind and producer component family
- training/validation/test range inputs and disjointness checks
- environment contract id and replay batch identity
- observation/action/reward/execution profile digests
- causal walk-forward schedule mode/schema/digest
- causal schedule cursor-key ordering and derived no-future-snapshot source
- max episodes, workers, attempts, and wall-clock budget
- resume ledger and idempotency identity
- policy checkpoint artifact schema and digest rules
- selector policy and sealed-test access rules
- terminal stop condition and post-run Lattice target to recheck

Implementation checkpoint:

```text
Status:
Implemented as a Runtime contract-only job surface.

Runtime additions:
- runtime_job_kind_t::policy_training
- job manifest naming/chain helpers for policy_training
- kikijyeba.runtime.policy_training_job_contract.v1
- hero.runtime.run operation=policy_training requested_mode=plan|dry_run
- deterministic contract digest and contract packet
- execute-mode refusal:
  E_RUNTIME_POLICY_TRAINING_EXECUTION_NOT_IMPLEMENTED

Contract checks:
- policy id/kind/architecture/config digests are required
- training, validation, and test range digests are required and must differ
- normalization fit range must equal training range
- replay buffer source range must equal training range
- observation/action/reward/execution profile digests are required
- training_schedule_mode must be causal_walk_forward_training.v1 for readiness
- causal schedule schema/digest/cursor-key kind are required
- causal_schedule_no_future_snapshot_use_source must be
  derived_from_artifact_fit_use_ledgers
- causal_schedule_no_future_snapshot_use must be true
- offline_full_window_research cannot satisfy readiness
- early-stopping and hyperparameter-selection policy digests are required
- parent replay-environment fact digest is required
- max episodes, max steps, max parallel jobs, and wall-clock bounds must be
  positive
- live execution is forbidden

Verified:
- Runtime Hero focused wave/rollout/policy-training handler test
- Runtime MCP schema compatibility
- live hero.runtime.run operation=policy_training positive plan smoke
- live execute refusal smoke
- live train/validation-overlap refusal smoke
```

No PPO optimizer exists yet. Runtime can validate the handoff contract, but it
does not train, mutate policy checkpoints, select policies, or claim Lattice
target satisfaction. Marshal still cannot train; it may only prepare a bounded
handoff once the next Runtime trainer exists. Lattice proves only artifact
completeness and anti-leakage structure, not policy quality.

### 5. Causal Policy-Training Schedule Contract V1

Goal:

```text
Prevent future-within-training leakage before PPO or any other trainable policy
can produce readiness evidence.
```

Milestone name:

```text
causal_policy_training_schedule_contract.v1
```

Operative law:

```text
At block B_k start:
  use SnapshotFamily_{k-1}

During B_k:
  observations use prior representation/MDN/observer snapshots
  actions use the prior policy snapshot
  Cajtucu executes under the bound paper execution profile
  rewards use the bound reward contract
  trajectory evidence is recorded

After B_k closes:
  representation, MDN, observer fitted components, and policy may be updated
  to SnapshotFamily_k

At B_{k+1}:
  SnapshotFamily_k may become usable
```

Required contract fields:

```text
artifact_fit_ledger:
  artifact_digest
  artifact_kind
  fit_input_cutoff_key
  fit_target_cutoff_key
  normalization_cutoff_key
  calibration_cutoff_key
  covariance_fit_cutoff_key
  replay_buffer_cutoff_key
  fit_cutoff_key
  target_label_available_from_key
  reward_available_from_key
  trajectory_usable_from_key
  usable_from_key
  embargo_policy_id
  embargo_steps
  training_block_id
  parent_artifact_digests

artifact_use_ledger:
  block_id
  block_cursor_begin
  block_cursor_end
  representation_snapshot_digest_used
  mdn_snapshot_digest_used
  observer_belief_snapshot_digest_used
  policy_snapshot_digest_used
  normalization_snapshot_digest_used
  calibration_snapshot_digest_used
  covariance_coupler_snapshot_digest_used
  replay_buffer_snapshot_digest_used
  reward_baseline_snapshot_digest_used
  no_future_snapshot_use_source
```

Acceptance:

- same-block snapshot use is rejected
- snapshots with usable_from_key after block_cursor_begin are rejected
- cursor keys require explicit ordering kind; opaque keys are rejected
- no_future_snapshot_use is derived from artifact fit/use ledgers rather than
  trusted as a caller assertion
- normalization, calibration, covariance/coupler, replay-buffer, and
  reward-baseline future-use are rejected
- target-label, reward, and trajectory availability must be no later than the
  artifact `usable_from_key`
- schedule-less policy-training contracts are rejected
- offline_full_window_research is allowed only as non-readiness diagnostic
  evidence
- batch_final_refit_candidate must declare validation_no_longer_proof and
  sealed_test_required
- Lattice policy_training_artifact_ready requires causal schedule identity and
  no_future_snapshot_use

### 6. PPO V0

Goal:

```text
Implement PPO as a policy inside the existing environment, not as a separate
simulator or shortcut around Cajtucu/Runtime/Lattice.
```

PPO must consume the same policy input and emit the same action:

```text
observation_t or narrowed policy_input_t
  -> action_t = target risky-node weights + graph-node reserve weight
```

Acceptance:

- PPO training runs only on bounded historical training windows.
- PPO training must use causal_walk_forward_training.v1 schedule evidence.
- PPO rollout/evaluation uses Runtime replay and Cajtucu paper execution.
- Baselines and PPO share the same reward and execution profile.
- Lattice proves training/validation/test separation and policy artifact
  lineage.
- Marshal coordinates bounded handoffs but does not train, select, prove, or
  tune policies by itself.

### 7. Tsodao Settings Protection

Goal:

```text
Introduce Tsodao after trainable-policy artifacts and selection evidence exist,
so approved settings cannot drift silently.
```

Tsodao should protect:

- approved environment profiles
- approved Cajtucu execution profiles
- approved reward definitions
- approved policy-training configs
- approved hyperparameters
- approved selector/evaluation split policies
- promotion criteria and evidence digests

Tsodao does not search for optimal settings. It records and protects settings
that were selected by an explicit evidence process.

For V1, Tsodao should protect final promoted settings and record candidate
setting digests as lineage. It should not guard every experimental candidate.

### 8. Paper-Online Readiness Contract V1

Goal:

```text
Define what must be true before any online paper session starts.
```

Milestone name:

```text
paper_online_readiness_contract.v1
```

Required design points:

- market data staleness policy
- persistent paper ledger recovery
- idempotent execution intent handling
- duplicate action/execution protection
- session start/stop semantics
- direct-edge universe validation
- synthetic execution markets forbidden
- Cajtucu paper execution profile locked by digest
- reward/report artifact path policy
- operator abort/kill-switch semantics
- clock and timestamp policy

This is a readiness contract only. It does not run paper-online.

### 9. Paper-Online

Goal:

```text
live market stream
  -> Kikijyeba paper_online world
  -> policy
  -> Cajtucu paper execution
  -> ledger
  -> Runtime/Lattice/Marshal evidence
```

No real capital. The same action, execution, ledger, reward, and evidence
contracts from replay must be reused.

### 10. Live

Goal:

```text
Only after replay, policy training, Tsodao settings protection, paper-online,
and Lattice readiness proofs are mature.
```

Live requires:

- Cajtucu broker adapter
- no direct policy-to-broker path
- strict execution limits and kill switches
- Tsodao-approved settings
- Lattice readiness proof
- Marshal handoff receipt
- Runtime execution authority

## Not Active

Do not start these without a new finite goal:

```text
PPO before policy-training anti-leakage contract
paper-online before replay evidence and Cajtucu traces are mature
live broker APIs before paper-online
allocator tuning against one validation window
checkpoint ranking by validation metric
market-readiness or deployment-readiness policy gates
unbounded Marshal scheduling
Codex-in-the-loop public evaluation
```
