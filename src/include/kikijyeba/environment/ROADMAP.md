# Kikijyeba Environment Roadmap

This roadmap tracks environment work only. Kikijyeba owns reset, step, time law,
reward, trajectory evidence, and replay/paper-online world contracts. It is not
a broker, proof engine, Runtime launcher, policy trainer, or optimizer.

## Current State

```text
kikijyeba.environment.replay.v1
hero.environment.certify.policy_acceptance
hero.environment.certify.paper_online_readiness
```

Stable environment contract:

```text
world_iface_t / policy_adapter_iface_t
reset(EpisodeSpec) -> observation_t
step(action_t) -> transition_t
observation_t = full deterministic-policy observation
policy_input_t = trainable-policy input lens
action_t = target weights over ordered graph nodes
reward_t = decomposed post-execution/post-realization reward
step_info_t = evidence/info, not policy input
```

Implemented baseline:

- Historical replay reveals realization only after action and Cajtucu paper
  execution.
- Trainable policies consume `policy_input_t`, a curated causal lens over
  observation, portfolio state, masks, snapshot identity, accounting numeraire,
  causal schedule, execution profile, and reward contract.
- Policy output is one `target_node_weights[A]` vector over the ordered
  graph-node action universe.
- No separate reserve/cash action bucket exists.
- Reward is computed after Cajtucu execution and historical realization.
- Cost-aware paper replay binds execution/profile/policy identity, requires
  nonzero validation costs, and emits report plus experience-trace evidence.
- The graph-node allocation policy component path is module-backed; the fake
  trainable policy is only a local contract/test fixture.
- Actor checkpoint reload goes through `checkpoint.meta` and `module_state.pt`.
- Lattice can prove `replay_environment_artifact_ready` from durable replay
  reports.
- Environment certification emits policy-acceptance and paper-online-readiness
  sidecars in check/issue mode, with preview-digest binding and no policy
  selection, proof, deployment, broker, or live-capital authority.

## Next Environment Work

### Paper-Online Session Contract V1

Milestone:

```text
paper_online_session_contract.v1 support
```

Environment role:

- Define the durable paper-online session world contract that consumes proven
  `paper_online_readiness_contract_ready` evidence.
- Reuse the proven replay action, execution, ledger, reward, and evidence
  semantics where possible.
- Define session state files, clock/staleness enforcement, duplicate-action
  protection, idempotent execution-intent handling, persistent paper-ledger
  recovery, and operator abort/kill-switch transitions.
- Preserve the graph-node allocation surface: one `target_node_weights[A]`
  vector over graph nodes, no hidden reserve action, and no direct
  policy-to-broker path.

Acceptance sketch:

- `kikijyeba.environment.paper_online.v1` has a concrete session-state contract
  and validator surface before any online-paper runner exists.
- Session admission requires a fresh paper-online-readiness proof and rejects
  stale, missing, or mismatched readiness evidence.
- Historical replay remains the training/evaluation environment until a bounded
  paper-online session runner is explicitly implemented.

First implementation slice:

- `paper_online_session_contract.h` defines the contract IDs, durable session
  files, lifecycle transitions, readiness evidence packet, and admission
  validators.
- `hero.environment.inspect.schema` reports the session contract vocabulary for
  operator inspection without adding a session runner.
- Contract tests reject stale readiness proof timing, missing proof digest,
  readiness-not-ready evidence, locked execution-profile drift, missing durable
  artifact slots, and execution-authority drift.

## Future Environment Work

```text
kikijyeba.environment.paper_online.v1
  Live market stream, persistent paper ledger, no real capital.

kikijyeba.environment.live_guarded.v1
  Only after Tsodao, Lattice readiness, Runtime authority, and Cajtucu broker
  adapter contracts are mature.
```

Paper-online must reuse the action, execution, ledger, reward, and evidence
contracts proven in historical replay.

## Explicit Non-Goals

- PPO optimizer ownership
- broker execution
- live capital
- allocator tuning
- policy acceptance
- market readiness
- deployment readiness
