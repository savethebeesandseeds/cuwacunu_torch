# Active Roadmap

This is the short operator-facing roadmap. Keep it finite. Do not use this file
as a history log for completed Hero cleanup, Lattice recovery, Runtime replay
plumbing, Marshal contract refactors, Cajtucu paper-engine hardening, or
policy-training anti-leakage work.

Subsystem details live in:

```text
src/include/kikijyeba/environment/ROADMAP.md
src/include/cajtucu/ROADMAP.md
src/include/hero/lattice_hero/lattice/ROADMAP.md
src/include/hero/marshal_hero/marshal/ROADMAP.md
src/include/hero/runtime_hero/runtime/README.md
src/include/wikimyei/inference/expected_value/mdn/README.md
```

## North Star

```text
Runtime executes and writes durable evidence.
Lattice proves explicit target satisfaction from Runtime evidence.
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

## Current Stable Baseline

Hero public surfaces are intentionally small:

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

Stable pre-PPO contracts:

```text
cajtucu.execution.paper.v1
  Paper execution, ledger mutation, rejects, partials, costs, and traces.

kikijyeba.environment.replay.v1
  Historical reset/step/reward environment with Cajtucu paper execution.

cost_aware_paper_replay_rollout.v1
  Completed Runtime job -> Marshal rollout -> Runtime replay -> Kikijyeba
  historical replay -> Cajtucu paper execution -> report and experience trace.
  Validation rollouts bind nonzero execution costs, profile/policy digests,
  bounded steps/parallelism, direct-pair feasibility counters, transaction-cost
  anatomy, target tracking, ledger integrity, time-law counters, and explicit
  non-authority flags.

replay_environment_artifact_ready
  Lattice artifact proof for replay report completeness, lineage, time law,
  projection counters, and Cajtucu execution evidence.

policy_training_artifact_ready
  Lattice artifact proof for policy-training identity, range separation,
  parent evidence, selector/test discipline, and causal schedule evidence.

kikijyeba.runtime.policy_training_job_contract.v1
  Runtime policy-training handoff contract with bounded no-op pre-PPO execute
  smoke. PPO execution is still refused.

causal_walk_forward_training.v1
  Time-aware anti-leakage schedule. A snapshot used inside block B_k must have
  usable_from_key <= B_k.block_cursor_begin.
```

Runtime handoffs can carry proof-clean checkpoint inputs as launch-time
overrides. Operators should not copy `latest_satisfying` resolutions into static
`.jkimyei` files.

## Remaining Before PPO

### 1. RL Policy Contract Design V1

Milestone:

```text
rl_policy_contract_design.v1
```

Goal:

```text
Define the trainable-policy game before PPO exists:

  what a learned policy may observe
  what raw policy output means
  how raw output becomes a valid target-weight action
  what reward contract the environment computes
  how causality, artifact identity, and execution cost bind into the policy
  contract
```

This milestone should produce real contract types, builders, adapters, identity
strings, and tests using fake trainable policies. It should not implement PPO.

Current implementation checkpoint:

```text
policy_input_t exists as kikijyeba.environment.policy_input.v1
raw_policy_output_t emits node_weight_logits[A]
target_node_weights_simplex.v1 adapts logits into target_node_weights[A]
trainable_policy_adapter_iface_t exists with a fake trainable policy fixture
wikimyei.policy.portfolio.graph_node_allocation.{dsl,net,jkimyei} exists
policy_input_t binds accounting_numeraire_node_id and causal_schedule_digest
Runtime/Lattice policy-training evidence binds policy_input_schema_id,
  action_adapter_id, and reward_contract_id
PPO execution remains disabled
```

Canonical vocabulary:

```text
s_t
  hidden world state: replay cursor, ledger internals, hidden future frames,
  and environment bookkeeping

o_t
  observation_t: full time-t Kikijyeba observation

p_t
  NodeLiftPotentialBelief: lifted potential belief, not directly tradable

b_t
  AllocationBelief: post-projection, portfolio-ready belief

x_t
  policy_input_t: curated, versioned, causal view for trainable policies

u_t
  raw_policy_output_t: unconstrained neural output

a_t
  action_t: target graph-node weights over the allocation universe. The
  accounting numeraire is one graph node with an accounting/valuation role, not
  a separate cash symbol or policy-output scalar outside the graph.

e_t
  Cajtucu execution_trace_t: fills, rejects, partials, costs, and ledger
  mutation

r_{t+1}
  reward_t: computed after execution and realization

I_t
  step_info_t/evidence: diagnostics, projection validation, costs, warnings,
  and failures
```

Expected control path:

```text
observation_t
  -> make_policy_input(observation_t)
  -> policy_input_t
  -> trainable policy / fake trainable policy
  -> raw_policy_output_t
  -> target_node_weights_simplex action adapter
  -> action_t
  -> Cajtucu paper execution
  -> ledger-based reward
```

Deliverables:

- `policy_input_t` and schema id:

  ```text
  kikijyeba.environment.policy_input.v1
  ```

  The input should be a curated causal view, not raw `observation_t`. It should
  include `AllocationBelief`-derived features, portfolio state, execution/cost
  state, previous action/target context, masks, and artifact identity.

- `policy_input_builder`:

  ```text
  observation_t -> policy_input_t
  ```

  The builder must enforce observation time law, graph identity, allocation-node
  order, accounting-numeraire identity, snapshot/artifact identity, and no
  access to current-action `step_info_t`.

- `raw_policy_output_t`:

  A minimal trainable-policy output representation for target-node-weight
  policies. PPO V0 should start with one logit per target graph node:

  ```text
  node_weight_logits: [M]
  ```

  A two-head exposure/allocation adapter remains possible later, but it should
  not hard-code special reserve treatment into the default contract. The model
  receives risk/cost/liquidity features per node and should learn how much
  weight to place on the accounting-numeraire node.

- target-node-weight action adapter:

  ```text
  kikijyeba.environment.action_adapter.target_node_weights_simplex.v1
  ```

  The adapter converts raw neural output into a valid long-only spot target over
  graph nodes. The reserve/settlement asset is included in the same target
  vector as a graph node; it is not a separate policy-output scalar:

  ```text
  target_node_ids [M] include allocatable nodes and the accounting numeraire node
  target_node_weights [M] >= 0
  sum(target_node_weights) = 1
  accounting_numeraire_node_id is metadata from graph/BasePolicy/global
  accounting role binding
  invalid/nontradable/executable masks are respected
  action_schema_id = kikijyeba.environment.action.target_node_weights.v1
  ```

- `trainable_policy_adapter_iface_t`:

  A trainable-policy wrapper compatible with the existing
  `policy_adapter_iface_t::act(observation_t) -> action_t`, while making the
  trainable path explicit:

  ```text
  observation_t -> policy_input_t -> raw_policy_output_t -> action_t
  ```

- named reward contract:

  ```text
  kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_drawdown.v1
  ```

  The initial formula may match the existing decomposed reward, but the reward
  identity must be bindable by Runtime and Lattice before PPO.

- fake trainable policy tests:

  Use deterministic fake/random-logit trainable policies to prove the contract
  runs through Kikijyeba replay and Cajtucu paper execution without adding PPO.

Recommended `policy_input_t` contents:

- identity and causality:

  ```text
  policy_input_schema_id
  environment_assembly_id
  observation_anchor_key
  observation_anchor_index
  knowledge_timestamp_ms
  graph_order_fingerprint
  allocation_belief_digest or artifact identity
  execution_profile_digest
  reward_contract_id
  causal_schedule_digest, when in policy-training context
  snapshot_family_digest, when in policy-training context
  ```

- universe:

  ```text
  target_node_ids [M]
  accounting_numeraire_node_id, as role metadata only
  valid_mask [M]
  tradable_mask [M]
  executable_mask [M]
  ```

- per-node features:

  ```text
  expected_log_return
  expected_arithmetic_return
  volatility / marginal variance
  downside metrics: VaR, CVaR, adverse excursion, downside probability
  confidence
  liquidity_score
  capacity_weight_limit
  linear_cost
  quadratic_impact
  projection_validation_score
  residual_quality_score
  surprise/calibration scores, only when causally available
  current_weight
  previous_target_weight
  previous_action_delta
  ```

- global features:

  ```text
  current accounting-numeraire node weight
  equity_value_numeraire
  drawdown
  total non-numeraire exposure
  previous turnover
  previous transaction cost
  execution profile/cost summary
  ```

- optional risk/distribution blocks:

  ```text
  covariance [A,A]
  correlation [A,A]
  compact downside/scenario summaries
  fixed-size scenario sketch
  ```

Default input stance:

```text
PPO V0 should not consume raw MDN tensors by default.
PPO V0 should use AllocationBelief-derived features plus compact
distributional/risk summaries.
Raw MDN or full NodeLiftPotentialBelief features require an explicit future
policy_input schema revision.
```

Discussion gates:

At the end of this milestone, pause and decide:

- Is `AllocationBelief` plus compact risk/downside summary enough for PPO V0,
  or should the V0 input include full scenario-bank features?
- Should NodeLiftPotentialBelief-derived mixture features enter V0 as optional
  summaries, or wait for `policy_input.v2`?
- Should invalid neural output always be projected into a valid action by the
  adapter, or should some invalid outputs be passed through and penalized by the
  environment?
- Should `target_node_weights_simplex.v1` remain a pure masked-softmax adapter,
  or should a later contract add explicit caps/projections before PPO V0?
- Should rejected notional, partial unfilled notional, or target tracking error
  be active reward penalties in V1, or report-only counters until after PPO
  diagnostics?
- Should reward normalization be an optimizer-local transform only, or become a
  first-class policy-training artifact with its own identity immediately?
- Should PPO V0 use a fixed risky universe per checkpoint only, or should the
  contract prepare for variable-size graph policies now?

Acceptance:

- A fake trainable policy can consume `policy_input_t`, emit
  `raw_policy_output_t`, adapt to `action_t`, and step through
  `kikijyeba.environment.replay.v1` with Cajtucu paper execution.
- `policy_input_t` contains no future realized returns and no current-action
  `step_info_t`.
- `policy_input_t` rejects missing or mismatched graph/order/accounting-numeraire
  identity.
- Nontradable or non-executable assets are masked before action adaptation.
- Fully masked action universes fail closed; there is no hidden special reserve
  fallback in the policy-output adapter.
- Raw NaN or non-finite policy output fails closed.
- Adapted target node weights are finite, long-only, and sum to one within
  tolerance.
- The trainable-policy surface has no separate cash output scalar.
- Reward identity is present in reports/artifacts and maps to the current
  ledger/post-realization reward computation.
- Reward uses actual Cajtucu ledger/execution evidence, not requested target
  fantasy.
- Policy-training artifacts can bind `policy_input_schema_id`,
  `action_adapter_id`, and `reward_contract_id`.
- PPO remains unimplemented and explicitly refused.

Out of scope:

- PPO optimizer.
- policy quality claims.
- reward tuning against validation performance.
- variable-universe neural architecture, unless explicitly accepted as a V0
  design decision.
- paper-online.
- live execution.

### 2. Pre-PPO Full Pipeline Rehearsal

Milestone:

```text
pre_ppo_full_pipeline_rehearsal.v1
```

Goal:

```text
fresh workspace/build
  -> train representation/MDN through Runtime waves
  -> materialize proof-clean checkpoint handoffs
  -> run fake trainable policy contract tests
  -> run no-op policy-training contract smoke
  -> run cost-aware paper replay rollout
  -> prove replay and policy-training artifact readiness with Lattice
```

Acceptance:

- The run starts from a clean enough build/artifact state to expose hidden
  dependency or stale-artifact assumptions. A `dev_nuke`-style cleanup may be
  used if the operator chooses.
- Runtime waves are launched through Runtime Hero, preferably via Marshal
  handoff when a Lattice target is involved.
- Direct `cuwacunu_exec` use remains allowed for debugging but emits a warning
  that it bypasses Marshal/Lattice guardrails.
- Representation and MDN training use launch-time checkpoint handoff overrides
  where upstream proof-clean inputs are required.
- `rl_policy_contract_design.v1` contracts are present and exercised by fake
  trainable policy tests before any PPO optimizer exists.
- Causal schedule evidence is present for policy-training artifacts.
- `offline_full_window_research` does not satisfy readiness.
- Lattice proves `replay_environment_artifact_ready` and
  `policy_training_artifact_ready` from durable artifacts.
- PPO remains unimplemented and explicitly refused.

The rehearsal is about system correctness and anti-leakage plumbing, not market
performance.

## Deferred Until After Pre-PPO Rehearsal

### PPO V0

PPO must enter as a policy inside the existing Runtime/Kikijyeba/Cajtucu
contracts, not as a private simulator.

Required shape:

```text
observation_t
  -> policy_input_t
  -> PPO policy adapter
  -> raw_policy_output_t
  -> target_node_weights_simplex action adapter
  -> action_t = target graph-node weights, including the accounting-numeraire node
```

PPO acceptance will require:

- Runtime-owned training job kind.
- `rl_policy_contract_design.v1` accepted.
- `policy_input_schema_id`, `action_adapter_id`, and `reward_contract_id`
  bound into policy-training artifacts.
- `causal_walk_forward_training.v1` schedule evidence.
- same reward/action/execution contracts as replay baselines.
- Cajtucu paper execution for rollout/evaluation.
- Lattice artifact proofs for range separation, parent evidence, checkpoint
  lineage, and no-future snapshot use.
- Marshal handoff only; Marshal does not train, select, prove, or tune.

### Tsodao Settings Protection

Tsodao should protect approved settings after evidence exists:

- environment profiles
- Cajtucu execution profiles
- reward definitions
- policy-training configs and hyperparameters
- selector/evaluation split policies
- promotion criteria and evidence digests

Tsodao records and protects selected settings. It does not search for settings.

### Paper-Online Readiness Contract V1

Before online paper sessions, define:

- market data staleness policy
- persistent paper ledger recovery
- idempotent execution intent handling
- duplicate action/execution protection
- session start/stop semantics
- direct-edge universe validation
- synthetic execution markets forbidden
- locked Cajtucu execution profile digest
- reward/report artifact path policy
- operator abort/kill-switch semantics
- clock and timestamp policy

This is a readiness contract only. It does not run paper-online.

### Paper-Online

```text
live market stream
  -> Kikijyeba paper_online world
  -> policy
  -> Cajtucu paper execution
  -> ledger
  -> Runtime/Lattice/Marshal evidence
```

No real capital. Replay contracts for action, execution, ledger, reward, and
evidence must be reused.

### Live

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
PPO before rl_policy_contract_design.v1
PPO before pre-PPO full pipeline rehearsal
paper-online before replay evidence and Cajtucu traces are mature
live broker APIs before paper-online
allocator tuning against one validation window
checkpoint ranking by validation metric
market-readiness or deployment-readiness policy gates
unbounded Marshal scheduling
Codex-in-the-loop public evaluation
```
