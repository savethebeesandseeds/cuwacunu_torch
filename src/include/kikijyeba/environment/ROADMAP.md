# Kikijyeba Environment Roadmap

This file keeps the environment work finite. The current milestone is not a
general simulator, an RL system, paper trading, live trading, or a Cajtucu
output layer.

## 1. Completed Cleanup

The environment recovery cleanup is complete when:

- `kikijyeba.environment.replay.v1` is historical replay only.
- Runtime replay artifact writing is best-effort sidecar evidence.
- Replay artifacts cannot fail the core MDN Runtime job.
- Review state is captured in `README.md` and this roadmap, not scratch dumps or
  temporary handoff files.
- Policy identity and method identity are separated in replay action/report
  evidence.
- The canonical RL/control vocabulary is documented:
  `s_t`, `o_t`, `p_t`, `b_t`, `pi`, `a_t`, `r_{t+1}`, and `I_t`.
- Lattice and Marshal changes are left to their concurrent workstream.
- Focused environment and Runtime replay tests pass.

Focused validation:

```sh
git diff --check
make -C src/tests/bench/kikijyeba/environment -j12 run-test_kikijyeba_environment_contract
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_kikijyeba_job_runner
```

## 2. Current Milestone Statement

Closure statement:

```text
kikijyeba.environment.replay.v1 is a compiled and tested historical replay
contract scaffold. It validates replay frames and compares policies under the
same action/reward/report grammar. It is not yet proof that the NodeLift
projection bridge is economically valid.
```

Pause-safe interface checkpoint:

```text
environment_interface_stabilization_v1 is complete when the public V1 surface
is stable enough to wait for trained artifacts:

- world_iface_t / policy_adapter_iface_t define the control boundary.
- observation_t is the V1 policy input.
- action_t remains target risky-node weights plus a graph-node base reserve.
- transition_t carries next observation, reward, done, and step evidence.
- reward_t remains decomposed.
- step_info_t is transition evidence, not policy input.
- experience_trace_t round-trips as deterministic text evidence.
- Runtime can write the experience trace as a replay-report sidecar.
- Kikijyeba policy_id and Wikimyei method_id remain separate.
```

## 3. Next Finite Milestone

The active finite milestone is `projection_validation_v2`: projection
validation over multiple resolved Ujcamei validation windows:

```text
projected_log_return = phi_asset - phi_reference
```

must be compared against realized executable asset/base log returns.

Status note:

```text
The first trained-artifact validation run completed and showed clean replay
time-law counters, but weak projection skill. The next run should not tune the
allocator. It should deepen projection diagnostics and repeat across disjoint
validation windows.
```

Implemented v2 reporting slice:

```text
aggregate metric scope labels
policy-wise metric summaries
zero-return projection baseline
model skill versus zero-return baseline
per-node MAE/RMSE/bias/correlation/directional accuracy/coverage
per-node realized volatility and normalized error
per-node interval width and zero-baseline skill
```

Acceptance criteria:

- Build replay bundles from resolved Ujcamei/component-stream cursor identity.
- Preserve requested range and accepted cursor identity separately.
- Reject future leakage into observations.
- Emit realized asset/base return evidence after action/execution.
- Report projection MAE, RMSE, signed bias, correlation, directional accuracy,
  and coverage when scenarios are available.
- Label aggregate report scope so top-level means are not confused with one
  policy's score.
- Report policy-wise summaries separately from experiment-level aggregates.
- Add per-node projection diagnostics:
  MAE, RMSE, signed bias, correlation, directional accuracy, coverage, realized
  volatility, and normalized error.
- Add projection baselines:
  zero-return now; previous edge return and moving-average edge return later.
- Add semantic bridge comparisons where evidence exists:
  raw `phi_asset - phi_reference`, reconstructed edge return with residual, and
  direct executable edge return.
- Treat transaction costs as nonzero in later allocator comparisons.
- Keep portfolio PnL secondary until the projection bridge is measured.

Anything beyond this, including PPO and paper/live operation, requires a
separate finite roadmap item after projection validation and paper execution
evidence are stable.

## 4. Lattice And Marshal Bridge

This environment should produce evidence regardless of which policy acted.
Do not make the fact surface depend on `spot_distributional_utility`, PPO, or
any future policy family.

Policy-neutral environment facts should describe the experience:

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

The environment writes those facts through Runtime artifacts. Lattice may read
them and prove narrow artifact claims, but the environment does not ask Lattice
to judge reward quality, profitability, market readiness, policy acceptance, or
online safety.

The first Lattice target should be non-dispatchable:

```text
replay_environment_artifact_ready
```

It should prove only that replay evidence is complete, lineage-bound,
time-law-clean, schema-consistent, and projection counters are present when the
contract requires them.

Future trainable policies need a separate target class. A later dispatchable
target may be:

```text
policy_training_artifact_ready
```

That target should be reachable by Marshal only when Runtime has a concrete
bounded job contract for environment episodes, including max episodes, workers,
attempts, source cursor ranges, reward digest, resume ledger, terminal stop
condition, and policy artifact output schema.

Keep policy acceptance separate and disabled until baselines, selector/eval
splits, reward thresholds, uncertainty policy, and leakage-negative tests are
explicit.

## 5. Experience Driver And Cajtucu Boundary

The terminology split is now part of the stable interface checkpoint. Keep
current behavior fixed while preserving this future split:

```text
Ujcamei
    input/source data.

Kikijyeba Environment
    experience driver, world loop, reward, and transition evidence.

Wikimyei Policy
    belief, policy, and allocation math.

Cajtucu
    output execution: intents, paper fills, ledger mutation, and execution
    traces.
```

Use `historical_replay_environment` for the current mode. Reserve
`experience_buffer` or `replay_buffer` for later trainable-policy transition
storage; do not call the current historical world `experience replay`.

Document future modes without implementing them:

```text
historical_replay
    implemented V1 mode; historical Ujcamei stream plus Cajtucu paper
    execution.

paper_online
    future mode; live market stream plus Cajtucu paper execution.

live_online
    future mode; live market stream plus live execution/fills.
```

Cajtucu now exists as `src/include/cajtucu` with `cajtucu.execution.paper.v1`.
Kikijyeba replay calls that backend for output mechanics, then computes reward
from the resulting ledger state after historical realization is revealed. The
Kikijyeba-side `output/experience_trace.h` remains an environment trace for
episode/report handoff, not the Cajtucu execution trace itself.

## 6. Deferred Work

Keep these out of the current environment cleanup:

- PPO or any learned policy implementation.
- Paper/live operation.
- Live broker adapters and broker credentials.
- Lattice/Marshal implementation changes.
- New portfolio allocation methods.
