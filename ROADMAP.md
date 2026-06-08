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

Stable replay and policy-training contracts:

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
  parent evidence, selector/test discipline, and causal schedule evidence. PPO
  adapter facts bind forecast-eval and observer-belief parents plus replay
  lineage through either a replay-environment fact digest or a replay report
  digest; allocation-engine parent facts are required only for policy kinds
  that declare that parent.

kikijyeba.runtime.policy_training_job_contract.v1
  Runtime policy-training handoff contract with bounded no-op and PPO V0
  execute paths. The PPO path writes actor/critic/optimizer, rollout/update,
  validation, quality-comparison, and Lattice-readable fact artifacts. It can
  ingest existing Kikijyeba replay reports into PPO rollout samples only when
  old-policy action-distribution evidence is bound. When given `replay_job_dir`,
  Runtime now dispatches `cuwacunu_exec --replay-from-job-dir` with the
  `graph_node_allocation` trainable policy in `on_policy_sample` mode, passes
  an explicit actor checkpoint artifact for policy forward, and then ingests
  the generated Kikijyeba/Cajtucu report as fresh PPO rollout evidence.

causal_walk_forward_training.v1
  Time-aware anti-leakage schedule. A snapshot used inside block B_k must have
  usable_from_key <= B_k.block_cursor_begin.

pre_ppo_readiness_gate.v1
  Complete. Runtime-mediated cost-aware replay has run from fresh Runtime
  artifacts, `replay_environment_artifact_ready` is Lattice-satisfied, live
  capital is forbidden, and the policy-facing action surface is unified
  `target_node_weights[A]`.

rl_policy_contract_design.v1
  Complete enough to support the next contract step. `policy_input_t`,
  `raw_policy_output_t`, `target_node_weights_simplex.v1`,
  `trainable_policy_adapter_iface_t`, fake trainable policy smoke, and
  `wikimyei.policy.portfolio.graph_node_allocation.{dsl,net,jkimyei}` exist.
  The checked-in graph-node allocation `.jkimyei` now declares bounded
  `policy_graph_node_allocation_ppo_v0` with `OPTIMIZER=ppo_clip`,
  `PPO_EXECUTION_ALLOWED=true`, causal schedule required, raw MDN forbidden,
  and live capital forbidden.
```

Runtime handoffs can carry proof-clean checkpoint inputs as launch-time
overrides. Operators should not copy `latest_satisfying` resolutions into static
`.jkimyei` files.

## PPO V0 Evidence Path

The stochastic policy surface, Runtime PPO V0 execution path, cost-aware
validation, comparison report, and Lattice artifact-readiness proof are now
implemented as evidence-only machinery. The next work is cleanup, network
architecture review, and later governance/readiness gates before any broader
policy claims, paper-online, or live path.

### 1. Action Distribution And Actor-Feature Contract

```text
action_distribution_contract.v1
policy_input_actor_feature_cleanup.v1
```

Status:

```text
implemented
```

Goal:

```text
Give the learned graph-node allocation policy a real stochastic action
distribution, and make the actor-visible input tensor safe before PPO training
exists.
```

Current law to preserve:

```text
market/source data
  -> representation / encoder
  -> MDN / forecast distribution
  -> observer / AllocationBelief
  -> policy_input_t
  -> graph_node_allocation policy
  -> target_node_weights[A]
  -> Cajtucu execution
  -> post-execution reward
```

PPO does not replace representation, MDN, or observer. It lives downstream of
AllocationBelief and learns allocation behavior from a causal, curated policy
input.

Distribution surface:

```text
action_distribution_iface_t:
  sample()
  deterministic_action()
  log_prob()
  entropy()
  approximate_kl() or exact kl when available
  evidence_payload()

masked_dirichlet_simplex.v1:
  V0 default candidate. Native simplex distribution, exact log_prob/entropy,
  and clean first PPO evidence.

masked_logistic_normal_simplex.v1:
  stronger endpoint candidate. More expressive allocation covariance, but must
  prove active-coordinate transforms, Jacobian/log_prob, KL/entropy semantics,
  and mask behavior before becoming default.
```

Input cleanup:

```text
Remove raw observation_anchor_index and knowledge_timestamp_ms from
actor-visible global feature tensors. Keep them as policy_input_t identity and
evidence fields.

Optional actor-visible time context must be explicit:
  time_context_block.v1
  transformed/cyclical/staleness features only
  disabled by default
```

Actor-visible blocks should be named and digest-bound:

```text
node_universe_block.v1
portfolio_state_block.v1
allocation_belief_return_risk_block.v1
allocation_belief_model_quality_block.v1
execution_cost_liquidity_block.v1
compact_cross_node_risk_block.v1
optional time_context_block.v1
```

Acceptance:

- `action_distribution_iface_t` exists.
- `masked_dirichlet_simplex.v1` exists and is the V0 default candidate.
- `masked_logistic_normal_simplex.v1` exists as a tested candidate, not
  necessarily the default.
- Distribution evidence can carry sample, deterministic action, log_prob,
  entropy, KL/approx-KL, active node indices, and distribution-specific
  parameters.
- `K=0` active nodes fail closed.
- `K=1` active node produces deterministic weight 1 with log_prob/entropy 0.
- Inactive nodes are exactly zero.
- Active sampled weights are finite and sum to one.
- Old/new log-prob recomputation is tested for PPO-ratio safety.
- Raw anchor index and timestamp are absent from actor-visible tensors.
- Identity fields remain present in `policy_input_t` evidence.
- `.dsl`, `.net`, `.jkimyei`, and Runtime policy-training identity bind
  `action_distribution_id`.
- Runtime PPO execution supports the bounded `ppo_policy_adapter.v1` path with
  smoke fallback, optional existing-replay-report ingestion, and
  Runtime-dispatched on-policy Kikijyeba/Cajtucu replay collection via
  `replay_job_dir`.

Out of scope:

```text
policy quality claims
paper-online
live execution
```

### 2. PPO Artifact And Update Evidence Contract

Milestone:

```text
ppo_policy_artifact_contract.v1
```

Status:

```text
implemented
```

Goal:

```text
Define the checkpoint, rollout-collection, PPO-update, and evaluation evidence
that Runtime PPO V0 writes and Lattice can inspect without judging policy
quality.
```

Required evidence categories:

```text
collection identity:
  rollout_id, episode_id, step_index, block identity, policy checkpoint digest,
  policy_input_digest, snapshot family, environment/action/reward/execution
  contract IDs

action distribution evidence:
  active nodes, masks, raw output, distribution params, sampled action,
  old_log_prob, entropy, value estimate

transition/reward evidence:
  Cajtucu trace digest, executed/rejected/partial notional, costs, turnover,
  equity before/after execution/realization, reward components, done/truncated

PPO update evidence:
  old_log_prob, new_log_prob, probability ratio, advantage, normalized
  advantage, return target, value estimate, policy/value losses, entropy,
  KL/approx-KL, clip fraction, gradient norm, explained variance
```

Acceptance:

- Runtime can write PPO V0 artifacts, and the policy artifact/checkpoint,
  rollout collection, PPO update report, and validation rollout evidence
  contracts are concrete. The smoke fallback uses replay-step-compatible
  per-step fields while declaring `replay_backed_step_count=0`; when an existing
  Kikijyeba replay report is supplied, Runtime ingests replay-backed step
  anchors, target weights, reward components, Cajtucu trace ids, costs, rejects,
  partials, and missing-pair counters into the PPO rollout collection.
  Replay-backed PPO training fails closed when old-policy action-distribution
  evidence is missing; the fallback fixture path remains explicitly labeled
  `runtime_ppo_v0_smoke_fixture`.
- Kikijyeba trainable-policy actions now carry `policy_input_digest`,
  `action_distribution_id`, active-node identity, old log probability, entropy,
  value estimate, and an evidence-bound flag into episode step reports and
  replay report text. Runtime ingestion can therefore bind old-policy
  distribution evidence when replay was produced by the trainable policy bridge,
  while older replay reports are rejected for executable PPO training.
- Runtime policy-training plan/dry-run can validate a PPO-shaped graph-node
  allocation contract that binds:
  `ppo_policy_artifact_contract_id`, `policy_family_id`,
  `policy_checkpoint_schema_id`, `policy_input_feature_manifest_digest`,
  `action_distribution_config_digest`, `snapshot_family_digest`,
  actor/critic architecture digests, actor/critic checkpoint digests,
  optimizer-state digest, `ppo_config_digest`, GAE and
  advantage-normalization identity, rollout-collection schema/digest, PPO
  update-report schema/digest, validation rollout-report digest, and PPO
  hyperparameters.
- Missing actor/critic architecture, actor/critic checkpoint digest, PPO config,
  rollout collection, PPO update evidence, or supported action-distribution
  identity fails closed.
- Lattice can inspect the schema-level evidence without claiming policy
  quality, selection, market readiness, or deployment readiness.
- `requested_mode=execute` supports `policy_kind=ppo_policy_adapter.v1` with
  smoke fallback, existing replay-report ingestion, and Runtime-dispatched
  fresh on-policy replay collection from completed Runtime replay jobs.

Out of scope:

```text
policy selection
Tsodao protection
```

### 3. PPO V0 Runtime Job

Milestone:

```text
ppo_v0_graph_node_allocation_runtime_job.v1
```

Status:

```text
implemented
```

Goal:

```text
Implement the first Runtime-owned PPO training job for
wikimyei.policy.portfolio.graph_node_allocation.
```

PPO V0 must use:

```text
causal_walk_forward_training.v1
Runtime-owned execution
policy_input_t from AllocationBelief, not raw MDN
unified target_node_weights[A]
selected action distribution from action_distribution_contract.v1
PPO-Clip + GAE
advantage normalization
entropy and KL diagnostics
post-execution ledger reward
Cajtucu paper execution
```

Acceptance:

- Runtime preview now exercises the end-to-end PPO V0 evidence rehearsal:
  completed replay job directory -> Runtime `requested_mode=execute` ->
  Runtime-dispatched `cuwacunu_exec --replay-from-job-dir` collection path
  (covered by the fake exec fixture in focused tests) -> fresh
  `kikijyeba_on_policy_replay` rollout collection -> PPO update report ->
  post-update actor/critic checkpoints -> generated
  `runtime.policy_training.fact`.
- The generated policy-training fact references concrete forecast-eval and
  observer-belief parent fact digests plus replay-environment lineage. For PPO
  adapter facts, replay lineage may bind through
  `parent_replay_environment_report_digest`; allocation-engine parent facts are
  not required. Lattice satisfies `policy_training_artifact_ready` for that
  exact generated fact while preserving evidence-only authority.
- A tiny causal PPO training run writes a checkpoint and training report.
- The existing Runtime smoke executor is extended so rollout samples can come
  from existing Kikijyeba replay reports, not only fixture samples.
- Trainable-policy episode/replay reports preserve old-policy
  action-distribution evidence from the sampled action.
- Runtime-generated on-policy rollout collection uses
  `trainable_policy_adapter_iface_t::sample_action()` to produce fresh actions,
  old log probabilities, value estimates, Cajtucu traces, and rewards for the
  current policy. Runtime passes a checkpoint artifact into the graph-node
  allocation policy forward path and writes bounded post-update actor/critic
  checkpoints with learned node-logit and value parameters derived from PPO V0
  rollout evidence.
- The job writes all evidence required by `ppo_policy_artifact_contract.v1`.
- PPO uses fresh on-policy trajectories generated by the current policy through
  replay blocks.
- Old trajectories are diagnostic only unless a future off-policy contract
  exists.
- No private simulator bypasses Kikijyeba/Cajtucu.
- Cajtucu paper traces, costs, rejects, partials, missing-pair counters, and
  post-execution reward components are bound into per-step PPO evidence.
- Marshal can prepare/delegate the bounded Runtime handoff, but Marshal does
  not train.
- Lattice can prove `policy_training_artifact_ready` for the artifact without
  selecting or judging the policy.

### 4. PPO V0 Cost-Aware Evaluation Rollout

Milestone:

```text
ppo_v0_cost_aware_evaluation_rollout.v1
```

Status:

```text
implemented
```

Goal:

```text
Evaluate the first PPO checkpoint through the same cost-aware replay path used
for deterministic policies.
```

Current contract:

- Runtime no longer writes a deferred PPO validation stub. After the bounded
  PPO V0 update, `hero.runtime.run` dispatches a second Kikijyeba replay from
  the completed Runtime replay job using the **post-update actor checkpoint**
  and deterministic policy action mode.
- The evaluation replay stays cost-aware: Runtime passes a nonzero
  `--replay-linear-transaction-cost-rate` to `cuwacunu_exec`, using the request
  value when provided and otherwise a small validation default. Explicit zero or
  nonfinite evaluation cost fails closed.
- The artifact schema is
  `kikijyeba.runtime.ppo_cost_aware_validation_rollout.v1`. It binds the
  policy checkpoint digest, policy input/action/action-distribution/reward
  identities, Cajtucu execution profile digest, causal schedule digest,
  snapshot family digest, raw validation replay report path/digest, and
  after-cost execution counters.
- The evaluation report remains evidence-only. It denies policy quality,
  market readiness, deployment readiness, live execution, and Lattice quality
  authority.

Mandatory baselines:

```text
numeraire-only graph-node allocation
current_weight_no_trade
equal_weight
spot_distributional_utility
```

Report requirements:

```text
after-cost return
drawdown
realized volatility
turnover
fee/spread/slippage/cost anatomy
rejects and partial fills
missing direct pair count
numeraire fallback count
target tracking error
reward component summaries
projection validation metrics
policy distribution diagnostics
```

Acceptance:

- The validation artifact is a real cost-aware deterministic replay report, not
  `kikijyeba.runtime.ppo_validation_rollout_stub.v1`.
- The validation replay uses the post-update actor checkpoint, while the
  training rollout collection still uses the pre-update collection checkpoint
  and `on_policy_sample`.
- The report includes final equity, total log growth, max drawdown, realized
  volatility when available, turnover, fee/spread/slippage/cost anatomy,
  rejects/partials, missing direct pair count, numeraire fallback count, target
  tracking error, reward summaries, and Cajtucu trace evidence.
- The validation rollout digest is bound into the Runtime policy-training fact,
  and `policy_training_artifact_ready` remains an artifact-integrity proof
  rather than a profitability, selection, market-readiness, or deployment
  claim.

Acceptance:

- Evaluation is after-cost by default.
- Metrics are reported as evidence, not as Lattice policy selection.
- Lattice may prove the evaluation report is complete and clean; it must not
  crown a winner.

### 5. Policy Quality Report V1

Milestone:

```text
policy_quality_report.v1
```

Status:

```text
implemented
```

Goal:

```text
Produce explicit comparison evidence for PPO versus baselines without turning
Lattice into a policy selector.
```

Acceptance:

- Runtime writes `kikijyeba.runtime.policy_quality_report.v1` beside the PPO
  rollout/update/validation artifacts.
- The report uses a separate raw `policy_quality_replay.report` so PPO
  validation remains PPO-only, while the comparison replay runs the post-update
  graph-node allocation checkpoint and all required baselines under identical
  replay source, target universe, reward, execution profile, causal schedule,
  snapshot family, accounting numeraire, and nonzero transaction-cost
  assumptions.
- Required comparison set:

  ```text
  wikimyei.policy.portfolio.graph_node_allocation.v1
  numeraire_only.v1
  current_weight_no_trade.v1
  equal_weight.v1
  kikijyeba.environment.policy.spot_distributional_utility.v1
  ```

- The report records policy-wise final equity, total log growth, max drawdown,
  realized volatility when available, turnover, transaction cost, fee/spread/
  slippage cost anatomy, rejected/partial notional and orders, missing direct
  pair count, numeraire fallback count, target-tracking error, projection
  metrics, reward summary, and Cajtucu trace counters.
- The report records PPO-minus-baseline deltas for log growth, final equity,
  drawdown, turnover, and transaction cost.
- `policy_quality_report_digest` is bound into Runtime policy-training output
  and the Lattice-readable `runtime.policy_training.fact`.
- The report is explicitly evidence-only:

  ```text
  policy_selected=false
  comparison_ranked=false
  winner_declared=false
  policy_quality_claimed=false
  lattice_policy_selector=false
  market_readiness_claimed=false
  deployment_readiness_claimed=false
  live_execution_allowed=false
  report_integrity_only=true
  ```

- Lattice may inspect/bind the digest as policy-training artifact evidence, but
  it still proves artifact integrity only. It does not rank policies, crown a
  winner, or claim profitability, market readiness, deployment readiness, or
  live-capital safety.
- Tsodao protection remains deferred until a human/operator-approved setting
  selection exists.

### 6. Fresh PPO V0 End-To-End Evidence Run

Milestone:

```text
fresh_ppo_v0_end_to_end_evidence_run.v1
```

Status:

```text
implemented
```

Goal:

```text
Prove that the current PPO V0 path can start from a clean Runtime root, collect
fresh on-policy replay evidence from a completed replay job, write PPO update
and checkpoint artifacts, run post-update cost-aware validation, produce policy
quality comparison evidence, and satisfy Lattice artifact readiness without
claiming policy quality or market readiness.
```

Acceptance:

- The focused Runtime test first exercises `hero.runtime.reset` as the guarded
  fresh-run envelope: plan mode detects stale Runtime state, execute mode
  requires `confirm_dev_nuke=true`, and stale state is removed before the PPO
  run begins.
- The fresh PPO run uses a completed Runtime replay job directory as source
  evidence and refuses non-completed or artifact-incomplete replay sources.
- Runtime dispatches `cuwacunu_exec --replay-from-job-dir` in
  `--replay-on-policy-sample` mode with the collection actor checkpoint,
  action-distribution identity, causal schedule digest, snapshot-family digest,
  execution-profile digest, and accounting numeraire bound.
- The generated on-policy rollout collection is marked
  `collection_source=kikijyeba_on_policy_replay` and binds old-policy
  distribution evidence: policy input digest, active node identity, old log
  probability, entropy, value estimate, target weights, Cajtucu trace id,
  costs, rejects, partials, missing-pair counters, rewards, and terminal flags.
- Runtime writes post-update actor checkpoint, critic checkpoint, optimizer
  state, rollout collection, PPO update report, validation rollout report,
  policy quality report, and `runtime.policy_training.fact`.
- The post-update validation replay uses the post-update actor checkpoint and
  remains cost-aware with explicit nonzero execution-cost assumptions.
- The policy quality replay binds PPO and all mandatory baselines under the
  same replay source, action universe, execution profile, reward contract,
  causal schedule, snapshot family, accounting numeraire, and cost profile.
- Lattice satisfies `policy_training_artifact_ready` for the generated
  policy-training fact with forecast-eval and observer-belief parent evidence
  plus replay-environment report lineage bound. PPO adapter facts do not require
  allocation-engine parent evidence.
- The full evidence chain remains non-promotional:

  ```text
  policy_selected=false
  comparison_ranked=false
  winner_declared=false
  policy_quality_claimed=false
  market_readiness_claimed=false
  deployment_readiness_claimed=false
  live_execution_allowed=false
  report_integrity_only=true
  ```

### 7. Policy Network Architecture Review

Milestone:

```text
policy_network_architecture_review.v1
```

Status:

```text
completed
```

Goal:

```text
Review the learned graph-node allocation network architecture before expanding
PPO beyond the current V0 evidence path.
```

Current law to preserve:

```text
PPO does not replace representation, MDN, or observer.

market/source data
  -> representation / encoder
  -> MDN / forecast distribution
  -> observer / AllocationBelief
  -> policy_input_t
  -> graph_node_allocation policy network
  -> stochastic simplex target_node_weights[A]
  -> Cajtucu execution
  -> post-execution reward
```

Review questions:

```text
1. Is the current `.net` surface sufficient for PPO V0, or should it be
   narrowed before it fossilizes?
2. Should the V0 default remain a shared node encoder + global encoder +
   mask-aware pooling + separate actor/critic heads?
3. Which actor-visible feature blocks are stable enough to keep, and which
   should stay evidence-only?
4. Is the compact cross-node risk block adequate before adding attention,
   graph-message passing, temporal memory, or full scenario-bank encoders?
5. Should `masked_dirichlet_simplex.v1` remain the first default distribution,
   with `masked_logistic_normal_simplex.v1` kept as a tested endpoint
   candidate?
6. What architecture digests, feature manifests, distribution parameters, and
   actor/critic checkpoint evidence must Lattice inspect without turning into a
   policy-quality judge?
```

Acceptance:

- The review explicitly confirms that the learned policy consumes
  AllocationBelief-derived `policy_input_t`, not raw MDN tensors.
- The review produces a named V0 network contract for
  `wikimyei.policy.portfolio.graph_node_allocation`.
- Actor-visible tensors and evidence-only identity fields are separated.
- The chosen actor/critic architecture can emit all distribution and value
  evidence required by PPO without private simulator shortcuts.
- Future architecture candidates are listed as deferred research or V1/V2
  extensions, not mixed into PPO V0 by accident.
- No policy-quality, market-readiness, paper-online, or live-execution claim is
  made by the network review.

Review outcome:

```text
Accepted. The next implementation milestone is
graph_node_allocation_network_v0_contract.v1.

Key decisions:
  PPO stays downstream of representation / MDN / observer.
  The current bounded Runtime PPO path remains evidence V0, not the final
    serious policy network.
  The `.net` surface is too thin and should be expanded before it fossilizes.
  A feature manifest should be frozen now.
  Actor-visible tensors and evidence-only identity fields must be separated.
  masked_dirichlet_simplex.v1 remains the first serious default.
  masked_logistic_normal_simplex.v1 remains a tested endpoint candidate.
  compact_cross_node_risk_block.v1 should be added as a compact risk surface,
    without full covariance/scenario-bank actor tensors in V0.
```

### 8. Graph-Node Allocation Network V0 Contract

Milestone:

```text
graph_node_allocation_network_v0_contract.v1
```

Status:

```text
completed
```

Goal:

```text
Turn the reviewed policy-network architecture into a concrete V0 contract for
wikimyei.policy.portfolio.graph_node_allocation, while keeping the current
Runtime PPO path evidence-only and preserving the representation/MDN/observer
stack.
```

Current law to preserve:

```text
representation / encoder
  -> MDN / forecast distribution
  -> observer / AllocationBelief
  -> policy_input_t
  -> graph_node_allocation network
  -> stochastic simplex target_node_weights[A]
  -> Cajtucu paper execution
  -> post-execution reward
```

Implemented V0 contract shape:

```text
Inputs:
  AllocationBelief-derived policy_input_t
  node_features[A,28]
  global_features[6]
  compact_cross_node_risk_block.v1 as risk_features[10]

Architecture:
  shared node encoder
  global encoder
  mask-aware mean/max pooling
  fusion MLP
  separate actor head -> node_weight_logits[A]
  separate critic head -> state_value

Action distribution:
  masked_dirichlet_simplex.v1 as first serious default
  masked_logistic_normal_simplex.v1 as tested candidate/endpoint upgrade

Action:
  kikijyeba.environment.action.target_node_weights.v1

Reward:
  kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_drawdown.v1
```

Delivered:

- `wikimyei.policy.portfolio.graph_node_allocation.net` now declares the V0
  network contract: node/global/risk encoder layers, pooling, fusion,
  activation, normalization, dropout, shared encoder policy, separate
  actor/critic heads, policy/value head dimensions, and distribution parameter
  heads.
- `POLICY_HEAD_HIDDEN_DIM` and `VALUE_HEAD_HIDDEN_DIM` replaced the old
  allocation-head-only surface.
- `wikimyei.policy.portfolio.graph_node_allocation.features.dsl` freezes the
  actor-visible feature names/order/transforms and evidence-only identity
  exclusions.
- `policy_input_t` now carries node features `[A,28]`, global features `[6]`,
  and `risk_features[10]`.
- `is_accounting_numeraire_node` is a per-node actor-visible feature, explicitly
  not a reserve bucket, mandatory floor, or hidden cash channel.
- Raw `equity_value_numeraire` is transformed into
  `log_equity_value_numeraire` before actor ingestion.
- `compact_cross_node_risk_block.v1` is present as a compact actor-visible risk
  block; full covariance/scenario-bank actor tensors remain deferred.
- Dirichlet concentration config and logistic-normal candidate
  log-std/coordinate/entropy config are explicit contract fields.
- The graph-first config bundle parses, validates, and fingerprints the feature
  manifest alongside the `.net` contract.

Deferred from this milestone:

- Previous-turnover, transaction-cost, rejected-notional, partial-unfilled,
  min-notional, max-notional, and dust-threshold actor features remain a later
  policy-input extension.
- Runtime/Lattice checkpoint lineage fields already exist for the bounded PPO
  path; the next milestone instantiates this contract as a real Torch
  policy-module path.

Acceptance:

- The `.dsl`, `.net`, `.jkimyei`, C++ specs, BNF/man/docs, and focused tests
  agree on the V0 network contract.
- The policy network contract still consumes `policy_input_t` downstream of
  AllocationBelief and does not expose raw MDN tensors by default.
- Actor-visible features are explicitly separated from evidence-only identity.
- Feature ordering and transforms are manifest-bound and digest-bound.
- `target_node_weights[A]` remains the only policy action output; no reserve,
  cash, exposure, order, route, fill, or ledger-mutation output appears.
- `masked_dirichlet_simplex.v1` remains the default unless the review explicitly
  promotes logistic-normal after density/log-prob/KL/mask evidence tests.
- The current bounded Runtime PPO path remains a smoke/evidence harness until
  the graph-node allocation Torch module is wired into the replay policy path.
- Lattice can inspect artifact identity, feature manifest binding,
  architecture digests, distribution config, checkpoint digests, causal
  cleanliness, and report presence without proving policy quality.
- No policy-quality, checkpoint-selection, market-readiness, paper-online, or
  live-execution claim is made.

Verification:

```text
make -C src/tests/bench/wikimyei/config/graph_first_specs -j12 run-test_wikimyei_graph_first_specs
make -C src/tests/bench/kikijyeba/environment -j12 run-test_kikijyeba_environment_contract
git diff --check
```

Out of scope:

```text
Set Transformer / attention over nodes
tradable-pair graph encoder
temporal memory
full scenario-bank actor input
raw MDN auxiliary branch
distributional or risk-sensitive critic
Tsodao protection
paper-online
live execution
```

### 9. Graph-Node Allocation Torch Policy Module V0

Milestone:

```text
graph_node_allocation_torch_policy_module_v0.v1
```

Status:

```text
completed
```

Goal:

```text
Instantiate the reviewed V0 graph-node allocation network as a real Wikimyei
Torch module and make Runtime replay/PPO evidence use that module-backed policy
path instead of the fake trainable-policy fixture.
```

Delivered:

- Added `wikimyei.policy.portfolio.graph_node_allocation.torch_policy_module.v0`
  under the graph-node allocation policy folder.
- The module consumes `node_features[A,28]`, `global_features[6]`,
  `risk_features[10]`, and the active executable graph-node mask.
- The module implements the V0 architecture contract: shared node encoder,
  global encoder, risk encoder, mask-aware mean/max pooling, fusion MLP,
  policy head producing `node_weight_logits[A]`, value head producing
  `state_value`, and a scalar Dirichlet concentration head producing
  `action_distribution_params[1]`.
- Added `graph_node_allocation_torch_policy_t` as the Kikijyeba trainable
  policy adapter from `policy_input_t` to the Torch module and then to
  `raw_policy_output_t`.
- Runtime replay now instantiates the module-backed graph-node allocation
  policy instead of using `fake_trainable_policy_t` for the PPO-facing path.
- Runtime actor checkpoints now record the module contract, network contract,
  feature dimensions, feature-manifest digest, architecture digests, and
  bounded module-head delta/bias fields.
- The bounded PPO V0 update remains evidence-only: it mutates checkpoint head
  deltas/biases from replay evidence but does not claim full autograd PPO
  optimization, policy quality, market readiness, paper-online readiness, or
  live readiness.

Acceptance:

- The environment contract test proves module forward output shape, finite
  value/distribution parameters, action-distribution binding, executable-mask
  behavior, and checkpoint logit-bias application.
- The Runtime preview test proves collection and post-update actor checkpoints
  use the module forward mode and bounded module-head delta terminology.
- The old fake trainable policy remains only as a local contract/test fixture,
  not the Runtime replay graph-node policy path.

Verification:

```text
make -C src/tests/bench/kikijyeba/environment -j12 run-test_kikijyeba_environment_contract
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_runtime_wave_preview
git diff --check
```

Deferred:

```text
full autograd PPO optimizer
trainable checkpoint serialization/loading of real Torch parameters
Set Transformer / graph topology encoder / temporal memory
logistic-normal promotion to default action distribution
policy-quality or readiness claims
```

### 10. Digest / Hash / ID Surface Cleanup

Milestone:

```text
digest_hash_id_surface_cleanup.v1
```

Status:

```text
planned
```

Goal:

```text
Clean up public digest/hash/id naming before broader PPO expansion and later
operator workflows depend on these surfaces.
```

Detailed scope lives in [CLEANUP_DIGEST.md](CLEANUP_DIGEST.md). The current
first slice standardized Lattice/Marshal fact preview selectors around
`fact_digest`, `fact_digest_prefix`, and `fact_index`, with prefix lookup
failing closed on zero or ambiguous matches. The remaining cleanup should audit
other public/runtime-facing `hash`, `digest`, `fingerprint`, `id`, and `ref`
fields, keep full content digests canonical for proof, and add human-sized
short references only as display/query helpers where they do not weaken lineage.

## Deferred Until After PPO Artifact Evidence

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
PPO expansion beyond the current V0 evidence path before
  graph_node_allocation_network_v0_contract.v1
policy-quality or checkpoint-selection claims before Tsodao protection
paper-online before replay evidence and Cajtucu traces are mature
paper-online before paper_online_readiness_contract.v1
live broker APIs before paper-online
allocator tuning against one validation window
checkpoint ranking by validation metric
market-readiness or deployment-readiness policy gates
unbounded Marshal scheduling
Codex-in-the-loop public evaluation
```
