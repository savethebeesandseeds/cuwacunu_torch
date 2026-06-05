# Lattice Roadmap

This roadmap is the active Lattice work index. It is not a history log for the
recovered Lattice expansion or the completed Hero surface collapse.

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

Lattice Hero public tools:

```text
hero.lattice.status
hero.lattice.inspect
hero.lattice.evaluate
hero.lattice.compare
```

Active proof/evidence boundaries:

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

## Active Milestone: Environment Evidence Targets

Goal:

```text
Add policy-neutral replay environment evidence and proof shapes before any
trainable policy or PPO work.
```

Design the following fact families:

```text
replay_environment_run
replay_episode
replay_time_law
replay_action_schema
replay_reward_definition
replay_policy_identity
replay_projection_validation
replay_execution_trace
replay_artifact_lineage
```

First target:

```text
replay_environment_artifact_ready
```

Status:

```text
Implemented as a non-dispatchable artifact_readiness target over the
replay_environment fact family.
```

Target class:

```text
artifact_readiness
non_dispatchable
```

Core fact fields:

```text
identity
  fact_schema_id
  fact_schema_version
  runtime_job_dir
  runtime_job_digest
  runtime_run_id
  environment_run_id
  rollout_id
  experiment_id
  report_path
  report_digest
  experience_trace_path
  experience_trace_digest

contract_identity
  environment_assembly_id
  action_schema_id
  reward_contract_id
  execution_backend_id
  execution_profile_digest
  policy_set_digest

source_range_identity
  requested_range
  accepted_range
  accepted_cursor_wave_id
  accepted_cursor_kind
  accepted_cursor_scope
  accepted_source_range_policy
  accepted_source_order_policy
  accepted_batch_cursor_token
  graph_order_fingerprint
  source_order_policy
  bounded_parallelism

time_law
  future_observation_violation_count
  mixed_future_realization_key_count
  realization_before_action_violation_count
  action_timestamp_violation_count

projection_counters
  projection_validation_step_count
  projection_metric_present_count
  projection_interval_width_present_count
  projection_baseline_skill_present_count

cajtucu_counters
  cajtucu_trace_present_count
  cajtucu_valid_trace_count
  cajtucu_invalid_trace_count
  cajtucu_rejected_fill_count
  cajtucu_partial_fill_count
  cajtucu_synthetic_market_step_count
  cajtucu_total_transaction_cost_base_present

denials
  allocation_authority=false
  execution_authority=false
  live_capital_allowed=false
  target_satisfaction_claimed=false
  policy_quality_claimed=false
  market_readiness_claimed=false
```

It proves:

- replay run artifact exists
- episode artifacts exist
- report digest identity is bound
- requested range and accepted cursor identity are preserved
- graph/source order identity is bound
- action schema digest is present
- reward definition digest is present
- acting policy identity is present
- Cajtucu execution trace counters are present
- observation/action/execution time-law counters are present and clean
- projection-validation counters are present when required
- declared parallelism and replay bounds are observed
- parent forecast, observer, allocation, source, and environment digests bind

It does not prove:

- reward quality
- profitability
- projection economic validity
- policy quality
- policy acceptance
- checkpoint ranking
- market readiness
- deployment readiness

Acceptance:

- Fact schema and summary are documented.
- Scanner derives or reads replay environment evidence only from durable Runtime
  replay artifacts.
- Scanner binds execution_profile_digest and policy_set_digest from the replay
  report.
- Scanner reads Cajtucu execution feasibility, cost anatomy, target tracking,
  ledger integrity, projection, and time-law counters.
- Missing or malformed replay evidence fails closed as artifact proof failure,
  not as dispatch advice.
- `hero.lattice.inspect` can inspect the facts.
- `hero.lattice.evaluate operation=target` evaluates the active artifact target
  from durable replay evidence.
- Marshal can read the proof/evidence but cannot dispatch from it.

Implementation checkpoint:

```text
Goal: replay_environment_artifact_ready.v1

Implemented:
- replay_environment facts bind report digest, Runtime run id, environment run
  id, experiment id, execution_profile_digest, policy_set_digest, accepted
  cursor evidence, graph/source identity, replay contract fields, projection
  counters, Cajtucu trace/order/cost/target-tracking counters, and authority
  denials.
- replay_environment_fact_issues fails closed on missing digests, missing trace
  evidence, invalid traces, synthetic execution markets, missing order evidence,
  invalid fill/notional metrics, missing cost metrics, leakage counters, and
  incomplete replay/projection evidence.
- hero.lattice.inspect can expose replay_environment facts and summaries.
- hero.lattice.evaluate can evaluate replay_environment_artifact_ready without
  dispatching Runtime or Marshal work.

Still out of scope:
- proving policy quality
- proving projection economic validity
- proving market readiness
- selecting policies or checkpoints
- PPO or any trainable policy work
```

## Completed Milestone: Policy Training Artifact Readiness

Status: implemented as a non-dispatchable artifact-readiness proof. Runtime now
has a contract-only policy-training handoff surface, but executable training is
still refused until a dedicated trainable-policy runner exists.

Target:

```text
policy_training_artifact_ready
```

Fact family:

```text
policy_training
```

Proof kind:

```text
policy_training_artifact_bound
```

Implemented contract:

- scanner accepts `lattice.policy_training.fact`, `policy_training.fact`,
  `runtime.policy_training.fact`, and the policy-training artifact directory
  sidecar
- fact digest/canonical text bind policy identity, checkpoint identity, range
  digests, schema digests, causal schedule identity, typed cursor-key ordering,
  ledger-derived no-future snapshot source, selector policy, sealed-test
  evidence, and parent forecast/observer/allocation/replay digests
- summary resolves parent fact digests against forecast-eval, observer-belief,
  allocation-engine, and replay-environment facts
- target proof fails closed on missing anti-leakage, selector, sealed-test,
  Runtime job-kind, checkpoint, parent evidence, or authority-denial fields
- Hero inspect/evaluate can expose the fact family and target through the
  Lattice read surface

This target may become dispatch-related only after Runtime defines an executable
trainable-policy runner on top of the existing contract-only handoff:

```text
producer component family
Runtime job kind
environment contract id
episode bundle identity and source ranges
max workers, episodes, attempts, and wall-clock budget
resume ledger identity
terminal stop condition
policy artifact/checkpoint schema
reward definition digest
  execution profile digest
  causal policy-training schedule schema/digest
  anti-leakage and selector split policy
post-run Lattice target to recheck
```

The policy artifact evidence binds:

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
```

Required anti-leakage proofs:

- training, validation, and test ranges are disjoint
- training_schedule_mode is causal_walk_forward_training.v1
- causal schedule schema/digest/cursor-key kind are bound
- no-future snapshot source is derived_from_artifact_fit_use_ledgers
- every snapshot used by a policy-training block was usable before that block
  began
- opaque cursor keys are rejected
- target-label, reward, and trajectory availability are no later than artifact
  usable_from_key
- offline_full_window_research is not accepted as readiness evidence
- normalization/scalers are fit only on training evidence
- replay buffers contain only training-range frames
- reward baselines are fit only on training evidence
- validation-based checkpoint selection records selector policy and candidates
- test evidence is first accessed only after selected checkpoint identity exists

It proves only that a bounded policy-training run produced complete artifacts
with anti-leakage structure. It must not prove that the policy is good,
accepted, profitable, market-ready, or deployable.

## Completed Milestone: Runtime Policy-Training Job Contract

Milestone:

```text
runtime_policy_training_job_contract.v1
```

Goal:

```text
Define the finite Runtime contract for future policy_training facts and
checkpoints under the existing Kikijyeba environment and Cajtucu paper execution
contracts.
```

Status: implemented as a Runtime Hero contract plus bounded pre-PPO executable
smoke path:

```text
hero.runtime.run operation=wave requested_mode=plan|dry_run|execute with
policy-training contract fields
```

Acceptance:

- Runtime can plan/dry-run a bounded policy-training job without implementing
  PPO yet.
- Runtime can execute only the bounded `noop_policy_training.v1` smoke trainer.
  That smoke writes policy-training contract/report artifacts, a deterministic
  no-op checkpoint, terminal Runtime facts, and `runtime.policy_training.fact`.
- PPO execution is explicitly refused until a dedicated Runtime PPO trainer
  exists.
- The job contract declares training/validation/test range inputs, replay batch
  identity, environment/action/reward/execution profile digests, episode/worker
  limits, resume/idempotency identity, terminal stop condition, selector policy,
  and policy checkpoint artifact schema.
- The planned artifact payload matches `policy_training_artifact_contract.v1`.
- Marshal remains a coordinator and cannot train or select.
- Lattice remains proof authority for artifacts only.
- Execute mode does not optimize policies, select policies, or claim target
  satisfaction.

Next optional Lattice refinement before executable PPO:

```text
policy_training_causal_schedule_ready
```

This target would prove only schedule schema identity, schedule digest binding,
block/fit/use ledger completeness, typed cursor ordering, no same-block
snapshot use, no future snapshot use, and exclusion of full-window research from
readiness. `policy_training_artifact_ready` would continue to prove the broader
policy artifact lineage and anti-leakage contract.

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

Marshal boundary:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
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
