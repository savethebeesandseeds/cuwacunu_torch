# Kikijyeba Environment Roadmap

This roadmap tracks environment work only. Kikijyeba owns reset, step, time law,
reward, trajectory evidence, and the replay/paper-online world contracts. It is
not a broker, proof engine, Runtime launcher, policy trainer, or optimizer.

## Current State

```text
kikijyeba.environment.replay.v1
```

Implemented baseline:

- Historical replay world with `reset(EpisodeSpec)` and `step(action_t)`.
- Deterministic policy adapters may consume `observation_t` directly.
- Trainable policy adapters consume `policy_input_t`, a curated causal lens over
  observation, portfolio, mask, snapshot, accounting-numeraire,
  causal-schedule, execution-profile, and reward-contract identity.
- Policy output is one target-weight vector over the ordered graph-node action
  universe.
- Realization is revealed only after action and Cajtucu paper execution.
- Reward is computed after execution and historical realization.
- Runtime can produce replay artifacts from completed model runs.
- Marshal can prepare/delegate bounded rollout handoffs.
- Cajtucu paper execution is integrated as the replay execution/ledger backend.
- Cost-aware paper replay rollouts run from completed Runtime job evidence,
  bind execution/profile/policy identity, require nonzero validation costs,
  emit report and experience-trace Cajtucu evidence, and keep Marshal/Lattice
  authority boundaries explicit.
- Lattice can prove `replay_environment_artifact_ready` from durable replay
  reports.

Stable interface:

```text
world_iface_t / policy_adapter_iface_t
reset(EpisodeSpec) -> observation_t
step(action_t) -> transition_t
observation_t = full deterministic-policy observation
policy_input_t = V1 trainable-policy input
action_t = target weights over ordered graph nodes
reward_t = decomposed post-execution/post-realization reward
step_info_t = evidence/info, not policy input
```

## Active Milestone: Pre-PPO Environment Rehearsal

Milestone:

```text
pre_ppo_full_pipeline_rehearsal.v1
```

Environment role:

- Consume proof-clean Runtime artifacts produced by representation/MDN waves.
- Run the same replay world used by future PPO evaluation.
- Preserve the time law under full pipeline execution.
- Emit replay reports and experience traces that Lattice can inspect.
- Keep policy quality and market-readiness claims out of the environment.

Acceptance:

- A fresh end-to-end rehearsal can train upstream models, run no-op
  policy-training artifact smoke, and run cost-aware replay without introducing
  a second execution physics.
- Causal policy-training schedule evidence remains separate from environment
  trajectory evidence but both bind the same environment/action/reward/execution
  contracts.
- No PPO optimizer is implemented in this milestone.

## Future Environment Work

```text
kikijyeba.environment.paper_online.v1
  Live market stream, paper ledger, no real capital.

kikijyeba.environment.live_guarded.v1
  Only after Tsodao, Lattice readiness, Runtime authority, and Cajtucu broker
  adapter contracts are mature.
```

Paper-online must reuse the action, execution, ledger, reward, and evidence
contracts proven in historical replay.

## Explicit Non-Goals

- PPO implementation
- broker execution
- live capital
- allocator tuning
- policy acceptance
- market readiness
- deployment readiness
