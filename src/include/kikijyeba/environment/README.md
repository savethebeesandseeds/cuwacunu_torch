# Kikijyeba Environment

`kikijyeba.environment` is the operating-world contract for historical replay.
It coordinates observations, policy actions, Cajtucu paper execution, rewards,
and replay evidence. It does not own source formation, Wikimyei model/policy
math, portfolio policy internals, Lattice proof, broker credentials, paper-online
operation, or live execution.

The V1 target is:

```text
kikijyeba.environment.replay.v1
```

Its authored contract is:

```text
src/config/kikijyeba.environment.replay.dsl
```

## V1 Contract

- Historical replay only: `world_mode_t` exposes `historical_replay` as the
  only supported mode.
- RL-compatible grammar: `reset(EpisodeSpec)` returns an observation, and
  `step(Action)` returns a transition.
- Spawn model: one replay world per episode; bundle/policy tasks can run in
  parallel, but steps inside one episode remain sequential.
- Source range: `EpisodeSpec` carries requested range intent and accepted
  Ujcamei/component-stream cursor identity separately.
- Time law: observations contain only time-`t` knowledge; realization is
  revealed only after action and Cajtucu paper execution.
- Action shape: one target-weight vector over the ordered graph-node universe,
  including the accounting numeraire node, under
  `kikijyeba.environment.action.target_node_weights.v1`.
- Reward: decomposed post-execution reward from log growth, drawdown,
  transaction cost, turnover, and invalid-action evidence.
- Projection gate: replay can require projected log-return scenarios and
  compares them against realized asset/base returns.
- Realized-return evidence: source-built replay bundles use
  `direct_edge_realized_return_truth_v1` and record the direct graph edges and
  signs used to derive realized asset/base log returns.
- Authority boundary: replay artifacts are audit evidence only. They do not
  authorize allocation, execution, market readiness, deployment, or live
  capital.

## Main Pieces

- `control/types.h` and `control/interfaces.h` define the RL/control contract:
  observations, actions, rewards, transitions, world interface, and policy
  interface.
- `environment.h` is the umbrella include for the environment contract.
- `output/experience_trace.h` converts replay reports into environment
  experience traces and provides a deterministic text write/read contract. The
  trace keeps transition-level Cajtucu cost, reject/partial, missing-pair,
  numeraire-fallback, and target-tracking evidence for later audit.
- `policy/baseline.h` provides numeraire-only, equal-weight, fixed-weight, and
  current-weight policies.
- `policy/allocation.h` adapts the deterministic Wikimyei allocation method to
  the same target-weight action schema.
- `run/episode_runner.h` and `run/experiment_runner.h` run episodes and compare
  policy adapters under the same reward/report grammar.
- `replay/spec.h`, `replay/world.h`, `replay/source.h`,
  `replay/artifact_source.h`, and `replay/bundle_source.h` define the
  historical replay world and its source/bundle assembly path.
- `runtime/replay_source.h` and `runtime/experiment_driver.h` are the Runtime
  read/write bridge for job-local replay artifacts, post-job experiments, and
  optional experience-trace sidecars.

## Canonical RL / Control Vocabulary

Use this notation when discussing replay, paper mode, learned policies, or
deterministic allocation policies:

```text
s_t
    Internal world state. This is not exposed directly to the policy. It
    includes replay cursor state, ledger internals, hidden future frames, and
    simulator bookkeeping.

o_t
    Policy observation. In code this is `observation_t`; it contains only
    time-t knowledge.

p_t
    Raw NodeLift potential belief. In code this is
    `wikimyei::observer::belief::NodeLiftPotentialBelief`. It is not tradable
    and is not portfolio-ready by itself.

b_t
    Allocation belief. In code this is
    `wikimyei::observer::belief::AllocationBelief`. It is the post-projection,
    portfolio-ready belief state inside `o_t`.

pi
    Policy. In code this is anything implementing `policy_adapter_iface_t`,
    with `act(observation_t) -> action_t`.

a_t
    Action. In code this is `action_t`: target weights over the ordered
    graph-node action universe.

r_{t+1}
    Reward. In code this is `reward_t`, computed after action, simulated
    execution, ledger update, and next realization.

I_t
    Transition evidence. In code this is `step_info_t`: projection validation,
    fills, costs, risk gate evidence, decision reports, warnings, and failures.
```

The official step model is:

```text
s_t --observe--> o_t
o_t --policy pi--> a_t
(s_t, a_t) --world.step--> s_{t+1}, o_{t+1}, r_{t+1}, I_t
```

`wikimyei.policy.portfolio.spot_distributional_utility` is a deterministic
allocation method / solver. It becomes a policy only through the Kikijyeba
adapter `spot_distributional_utility_policy_t`, which implements
`policy_adapter_iface_t`.

Reports keep these identities separate:

```text
policy_id
    Identifies the Kikijyeba policy adapter, for example
    `kikijyeba.environment.policy.spot_distributional_utility.v1`.

method_id
    Identifies the underlying method used by the adapter when one exists, for
    example `wikimyei.policy.portfolio.spot_distributional_utility.v1`.
```

For deterministic V1 policies, `observation_t` can still be consumed directly.
For trainable policies, `policy_input_t` is now the narrower contracted view:
it binds causal observation identity, graph order, snapshot family, execution
profile, accounting numeraire identity, causal schedule digest, reward
contract, graph-node masks, current weights, previous target weights, and
compact feature tensors. It does not expose future realizations, same-action
`step_info_t`, or raw MDN tensors by default.

The pre-PPO trainable output surface is also explicit:

```text
policy_input_t
    -> raw node_weight_logits[A]
    -> target_node_weights_simplex.v1
    -> action_t target_node_weights[A]
```

`target_node_weights[A]` covers the ordered graph-node action universe. The
accounting numeraire, when present, is just another graph node in the vector.
There is no separate cash scalar output and no broker/order/fill output from
the policy. Cajtucu handles paper execution after the environment receives the
target-node action.

## Experience Driver And Output Boundary

`run/episode_runner.h` and `run/experiment_runner.h` are the V1 experience
driver layer. They run one sequential world loop per episode and compare policy
adapters under the same observation/action/reward/evidence grammar. Experiments
may parallelize episode, bundle, or policy tasks, but a single episode remains
step-sequential because each ledger state depends on the previous transition.

Use `historical_replay_environment` for the current Ujcamei-backed world. Do
not call it `experience replay`: reserve `experience_buffer` or
`replay_buffer` for future trainable-policy transition storage.

Mode terminology:

```text
historical_replay
    implemented V1 mode; historical Ujcamei stream plus Cajtucu paper
    execution.

paper_online
    future mode; live market stream plus Cajtucu paper execution.

live_online
    future mode; live market stream plus live execution/fills.
```

Cajtucu now owns output execution mechanics. In replay, `world.step` converts
the policy action into an execution intent, calls `cajtucu.execution.paper.v1`,
then computes reward from the post-execution ledger after historical realization
is revealed.

Future Cajtucu surfaces may include:

```text
experience_trace
transition_dataset
policy_comparison_report
training_replay_buffer
evaluation_summary
```

Kikijyeba remains the experience driver and Runtime remains the evidence
writer. Cajtucu does not compute RL reward and has no live broker authority in
V1.

## Review Boundary

Treat these as the core environment slice:

```text
src/include/kikijyeba/environment/
src/tests/bench/kikijyeba/environment/
src/config/kikijyeba.environment.replay.dsl
src/config/grammar/kikijyeba.environment.replay.dsl.bnf
src/config/man/kikijyeba.config.man
```

Runtime replay artifact/report visibility is an integration touchpoint, but
Runtime remains the launcher and evidence writer. Lattice and Marshal changes
belong to their own workstream and are not part of the environment core.

## Runtime Boundary

Runtime may write replay artifacts for non-dry-run MDN `MODE=run` jobs. That
writer is best-effort sidecar evidence: failures are recorded in
`job.state:replay_artifact_error` and must not fail the core MDN job.

The post-job entrypoint is:

```text
cuwacunu_exec --replay-from-job-dir <job_dir>
```

It consumes an already completed Runtime job directory and writes a replay
experiment report/index. It is not a replacement for normal Runtime execution.
Research replay may explicitly opt into synthetic direct node-pair execution
markets with `--replay-allow-synthetic-direct-edges`; Cajtucu rejects those
markets by default. Replay can also add a simple linear fee with
`--replay-linear-transaction-cost-rate <value>`.

## Current Status

This directory is a tested historical replay scaffold and a pause-safe
environment interface checkpoint. The stable V1 surface is:

```text
world_iface_t
policy_adapter_iface_t
observation_t
action_t
transition_t
reward_t
step_info_t
experience_trace_t
```

This is enough for deterministic replay-policy comparison and durable evidence
writing while model training proceeds. It is not yet proof that NodeLift
projection semantics are economically valid. The next validation milestone is
to test:

```text
projected_log_return = phi_asset - phi_reference
```

against realized executable asset/base returns over resolved Ujcamei data.

That validation requires trained forecast artifacts. After `dev_nuke`, the
model-training side must first produce serious trained MDN artifacts, using
Marshal/Lattice target work as appropriate, before this environment can run a
meaningful projection-validity study.

Until those trained artifacts exist, keep environment work limited to interface
stability, naming consistency, trace/report evidence hygiene, Cajtucu paper
execution plumbing, and focused contract tests. Do not add PPO, paper/live
operation, live broker adapters, or market-readiness claims in this pause state.

## Focused Validation

```sh
git diff --check
make -C src/tests/bench/kikijyeba/environment -j12 run-test_kikijyeba_environment_contract
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_kikijyeba_job_runner
```

Lattice and Marshal checks are intentionally outside this environment cleanup
slice.
