# Kikijyeba Environment Roadmap

This file keeps environment work finite. The current environment is historical
replay plus Cajtucu paper execution. It is not PPO, paper-online, live trading,
broker execution, or a policy optimizer.

## Current State

```text
kikijyeba.environment.replay.v1
```

Status:

```text
Historical replay contract scaffold is implemented and tested.
Runtime can produce replay artifacts from completed MDN run jobs.
Marshal can prepare/delegate bounded rollouts from completed Runtime jobs.
Cajtucu paper execution is integrated as the output/ledger backend for replay.
```

Stable interface:

```text
world_iface_t / policy_adapter_iface_t
reset(EpisodeSpec) -> observation_t
step(action_t) -> transition_t
observation_t = V1 policy input
action_t = target risky-node weights + graph-node reserve weight
reward_t = decomposed post-execution/post-realization reward
step_info_t = evidence/info, not policy input
```

Canonical notation:

```text
s_t = internal world state
o_t = observation_t
p_t = NodeLiftPotentialBelief
b_t = AllocationBelief
pi  = policy_adapter_iface_t
a_t = action_t
r_{t+1} = reward_t
I_t = step_info_t / evidence
```

## Active Milestone: Cost-Aware Paper Replay Rollout V1

Goal:

```text
Make replay rollouts reusable cost-aware evidence, not one-off performance
reports.
```

Milestone name:

```text
cost_aware_paper_replay_rollout.v1
```

Acceptance:

- Multi-window rollouts can be run from completed Runtime job evidence.
- Every rollout binds runtime job id, environment run id, accepted cursor
  identity, graph fingerprint, action schema, reward definition, policy set,
  Cajtucu execution profile, and execution profile digest.
- Environment assembly is `kikijyeba.environment.replay.v1`.
- Execution backend is `cajtucu.execution.paper.v1`.
- Validation rollouts require nonzero execution costs. Zero-cost rollouts are
  research-only and must be labeled `frictionless_research_replay`.
- Synthetic direct-edge markets are forbidden for validation rollouts. Research
  replay may opt in with an explicit reason and must mark validation
  satisfaction false.
- Required policy set covers base reserve, current-weight/no-trade,
  equal-weight, and deterministic Wikimyei SDU.
- Reports separate:
  - projection diagnostics
  - portfolio outcome diagnostics
  - execution feasibility diagnostics
  - time-law diagnostics
- Reports carry policy-wise final equity, total log growth, drawdown,
  requested/executed/rejected turnover, transaction cost, target tracking error,
  invalid trace count, rejected order/fill count, partial fill count, and
  synthetic-market step count.
- Reports carry Cajtucu trace presence/validity counters and cost anatomy:
  fee, spread, slippage, and total transaction cost.
- Aggregate metrics declare whether they are per-step, per-episode, per-policy,
  per-bundle, or experiment-wide.
- Baselines and deterministic Wikimyei policy run under the same accounting and
  execution assumptions.

## Implemented Checkpoint: Environment Lattice Evidence

Goal:

```text
Emit policy-neutral replay environment facts that Lattice can inspect and prove
as artifact readiness.
```

Cost-aware replay reports are the evidence producer. Lattice now scans those
emitted report fields into replay_environment facts and evaluates the
non-dispatchable replay_environment_artifact_ready target.

Fact families should describe what happened, independent of policy quality:

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

The first proof target should be non-dispatchable:

```text
replay_environment_artifact_ready
```

It may prove:

- schema identity
- environment assembly identity
- action schema identity
- reward contract identity
- execution backend identity
- execution profile digest
- policy set digest
- report digest identity
- lineage binding
- requested and accepted range identity
- resolved cursor identity
- source order
- bounded parallelism
- clean time-law counters
- required projection counters
- Cajtucu trace presence/validity counters

It must not prove:

- projection economic validity
- reward quality
- profitability
- policy quality
- policy acceptance
- market readiness
- deployment readiness

Implemented:

```text
Goal: replay_environment_artifact_ready.v1

Status:
Completed at the artifact-readiness layer. The target proves report
completeness, lineage, boundedness, time-law cleanliness, projection coverage,
Cajtucu trace/cost/order evidence, execution profile binding, policy set
binding, and authority denials. It does not prove projection economics,
portfolio quality, policy acceptance, market readiness, or deployment readiness.
```

## Policy Training Contract Before PPO

PPO must wait until policy-training artifacts are causally safe and Runtime can
own executable training. The artifact contract exists at the Lattice layer, and
Runtime now has a contract-only policy-training handoff surface. Execute mode
is still refused until a dedicated trainable-policy runner exists.

Milestone name:

```text
policy_training_artifact_contract.v1
```

Required contract fields:

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
parent evidence digests
```

Anti-leakage rules:

```text
training sees training ranges only
validation compares/selects only under explicit selector policy
test remains sealed until final report
normalization, reward baselines, replay buffers, early stopping, and
hyperparameters must not be fit from validation/test future evidence
representation, MDN, observer/belief, normalization, and policy snapshots used
inside a training block must have usable_from_key <= block_cursor_begin
opaque cursor keys are rejected unless a sortable kind is declared
no_future_snapshot_use must be derived from artifact fit/use ledgers
target-label, reward, and trajectory availability must be no later than
artifact usable_from_key
offline_full_window_research is diagnostics only and cannot satisfy readiness
```

Trainable policy execution must be a Runtime job kind before Marshal can
dispatch it. Marshal may prepare bounded handoffs only after Runtime defines the
training job contract.

Implemented checkpoint:

```text
policy_training_artifact_contract.v1
  Lattice fact family: policy_training
  Lattice target: policy_training_artifact_ready
  Status: implemented as non-dispatchable artifact-readiness proof
```

Implemented Runtime checkpoint:

```text
runtime_policy_training_job_contract.v1
  Runtime Hero: hero.runtime.run operation=policy_training
  Modes: plan|dry_run
  Execute: refused until a dedicated trainable-policy runner exists
```

The Runtime contract should bind the environment id, observation/action/reward
schemas, replay batch identity, Cajtucu execution profile digest, training/
validation/test range digests, normalization/replay-buffer isolation, selector
policy, causal walk-forward schedule digest, typed cursor-key ordering,
ledger-derived no-future-snapshot source, sealed-test access, and policy
checkpoint output identity. It does not implement PPO yet.

## PPO V0

When the policy-training contract is ready, implement PPO as a policy adapter:

```text
policy_adapter_iface_t::act(observation_t or policy_input_t) -> action_t
```

PPO must not get a private simulator. It must use:

- Kikijyeba reset/step
- Cajtucu paper execution
- the same reward contract as baselines
- Runtime-owned execution
- Lattice-readable policy-training evidence
- causal_walk_forward_training.v1 schedule evidence
- Marshal bounded handoff only after a finite Runtime training job exists

## Deferred Modes

```text
historical_replay
  implemented V1 mode; historical Ujcamei stream plus Cajtucu paper execution.

paper_online
  future mode; live market stream plus Cajtucu paper execution.

live_online
  future mode; live market stream plus live Cajtucu broker execution.
```

Before `paper_online`, define `paper_online_readiness_contract.v1` covering
market-data staleness, persistent paper-ledger recovery, idempotent execution
intents, duplicate-action protection, session start/stop semantics, direct-edge
universe validation, locked Cajtucu profile identity, artifact path policy,
operator abort/kill-switch semantics, and clock/timestamp policy.

Do not implement paper-online, live-online, broker APIs, PPO, allocator tuning,
or Tsodao from this roadmap without a separate finite milestone.
