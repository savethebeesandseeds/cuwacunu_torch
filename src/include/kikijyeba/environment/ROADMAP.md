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

### Paper-Online Session Contract And Runner V1

Milestone:

```text
paper_online_session_contract.v1 support
paper_online_session_runner.v1 support
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
  and validator surface before any live-online surface exists.
- Session admission requires a fresh paper-online-readiness proof and rejects
  stale, missing, or mismatched readiness evidence.
- The bounded runner only writes paper session artifacts from an existing
  admission fact; it does not start live trading, route to a broker, select
  policies/checkpoints, or claim market/deployment readiness.
- Historical replay remains the training/evaluation environment; the runner is
  a bounded paper-online artifact producer for admitted sessions.

First implementation slice:

- `paper_online_session_contract.h` defines the contract IDs, durable session
  files, lifecycle transitions, readiness evidence packet, and admission
  validators.
- `hero.environment.inspect.schema` reports the session contract vocabulary for
  operator inspection without adding a session runner.
- `hero.environment.certify.paper_online_session_admission mode=check|issue`
  accepts a compact `admission_request` object or `admission_request_path`,
  reads an existing `lattice.paper_online_readiness.fact`, derives bound
  policy/checkpoint/profile evidence, and returns `admission_ready` plus issues.
  Issue mode writes `lattice.paper_online_session_admission.fact` after preview
  digest binding, without writing session state.
- `hero.environment.paper_online.session mode=validate|run` accepts a compact
  `session_request` object or `session_request_path`, reads the existing
  admission and readiness sidecars, validates finite step/timing/target/ledger
  recovery/idempotency/staleness constraints, and in run mode writes durable
  session artifacts plus `lattice.exposure.fact` and
  `lattice.paper_online_session.fact`.
- Lattice exposure fact families include `paper_online_session`, with summary,
  preview, digest-prefix resolution, and runtime-index counts available through
  the standard Lattice inspection tools.
- Contract tests reject stale readiness proof timing, missing proof digest,
  readiness-not-ready evidence, locked execution-profile drift, missing durable
  artifact slots, and execution-authority drift.
- Tool tests cover clean inline and path-based admission requests, stale proof
  rejection, readiness fact digest mismatch rejection, validate-only sessions,
  successful bounded run, duplicate-run refusal, duplicate-action rejection,
  staleness rejection, and missing ledger-recovery rejection.

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
