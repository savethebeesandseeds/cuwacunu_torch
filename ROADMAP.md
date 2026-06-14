# Active Roadmap

This is the short operator-facing roadmap. Keep it finite. Do not use this file
as a history log for completed Hero cleanup, Lattice recovery, Runtime replay
plumbing, Marshal refactors, Cajtucu paper-engine hardening, or anti-leakage
work.

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
Cajtucu owns execution, paper fills, ledgers, and execution traces.
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
Tsodao is not implemented as a full authority yet. Its current surface is
  settings-protection evidence only; later it protects selected settings/weights
  and controls what can be shared or open sourced after evidence exists.
```

## Current Stable Baseline

Hero public surfaces are intentionally small:

```text
Config Hero:
  hero.config.status
  hero.config.inspect.*
  hero.config.apply.*

Runtime Hero:
  hero.runtime.status
  hero.runtime.inspect
  hero.runtime.run
  hero.runtime.reset

Environment Hero:
  hero.environment.status
  hero.environment.inspect
  hero.environment.certify
  hero.environment.rollout

Lattice Hero:
  hero.lattice.status
  hero.lattice.inspect
  hero.lattice.evaluate
  hero.lattice.compare

Marshal Hero:
  hero.marshal.status
  hero.marshal.prepare.train
  hero.marshal.prepare.evaluate
  hero.marshal.rollout
  hero.marshal.paper_online.session_handoff
  hero.marshal.inspect.*
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

replay_environment_artifact_ready
  Lattice artifact proof for replay report completeness, lineage, time law,
  projection counters, and Cajtucu execution evidence.

policy_training_artifact_ready
  Lattice artifact proof for policy-training identity, range separation,
  parent evidence, selector/test discipline, causal schedule evidence, PPO
  artifact lineage, policy DSL/net/features/jkimyei/universe identity,
  checkpoint identity, and authority denials.

tsodao_settings_protection_ready
  Lattice artifact proof that a Tsodao protected-settings bundle is digest-bound
  to an existing `policy_training_artifact_ready` fact and that the protected
  settings still match the referenced policy-training artifact. This is a
  settings-protection proof only, not policy selection, policy quality,
  market-readiness, deployment, or live authority.

policy_acceptance_contract_ready
  Lattice artifact proof for a policy-acceptance sidecar emitted from existing
  policy-training and Tsodao settings-protection evidence. The proof binds
  the named `policy_acceptance_governance_thresholds_v0.v1` acceptance policy,
  mandatory baselines, after-cost metrics, uncertainty policy, selector/test
  discipline, negative tests, threshold-selection audit, promotion criteria,
  and authority denials. It is still not policy selection, policy quality,
  market-readiness, deployment, or live authority.

causal_walk_forward_training.v1
  Time-aware anti-leakage schedule. A snapshot used inside block B_k must have
  usable_from_key <= B_k.block_cursor_begin.

kikijyeba.runtime.policy_training_job_contract.v1
  Runtime policy-training handoff contract with bounded no-op and PPO V0
  execute paths. The PPO path writes rollout/update/checkpoint/validation/
  comparison evidence and Lattice-readable facts without policy-quality,
  market-readiness, deployment-readiness, paper-online, or live claims.
  PPO policy-training contracts bind the policy DSL, net, feature manifest,
  jkimyei, target-node universe, action distribution config, reward, execution,
  graph order, and causal schedule into one policy operator surface digest.
```

## Completed Pre-Paper-Online Policy Stack

These milestones are complete enough to stop carrying their full plans here:

```text
pre_ppo_readiness_gate.v1
  Runtime-mediated cost-aware replay completed from fresh Runtime artifacts;
  replay_environment_artifact_ready is Lattice-satisfied.

rl_policy_contract_design.v1
  policy_input_t, raw_policy_output_t, trainable policy adapter, unified
  target_node_weights[A], post-execution reward, and graph_node_allocation
  policy surfaces exist.

action_distribution_contract.v1
  masked_dirichlet_simplex.v1 is the default stochastic simplex policy;
  masked_logistic_normal_simplex.v1 remains a tested candidate.

policy_input_actor_feature_cleanup.v1
  Actor-visible tensors are separated from evidence-only identity fields.

ppo_policy_artifact_contract.v1
  Runtime writes checkpoint, rollout collection, PPO update, validation, and
  comparison evidence that Lattice can inspect without judging quality.

fresh_ppo_v0_end_to_end_evidence_run.v1
  A clean Runtime-root PPO evidence run can collect on-policy replay evidence,
  write artifacts, run validation/comparison replay, and satisfy
  policy_training_artifact_ready without promotional claims.

ppo_policy_training_activation.v1
  The current learned-policy path uses
  `src/config/policy_training_ppo_v0.contract` and the
  `policy_training_ppo_v0` Runtime wave. Runtime completed a non-noop
  `policy_kind=ppo_policy_adapter.v1` run from fresh representation, MDN,
  replay, observer, and causal-schedule evidence; wrote actor/critic
  checkpoints, optimizer state and Torch optimizer archive, rollout/update/
  validation/policy-quality reports, and `runtime.policy_training.fact`; and
  Lattice satisfied `policy_training_artifact_ready` for PPO fact
  `1095cf66c1a0c48b` with certificate `cert_070bdb168a73`. Live-capital
  authority remains disabled.

policy_network_architecture_review.v1
  The policy remains downstream of representation/MDN/observer and consumes
  AllocationBelief-derived policy_input_t, not raw MDN tensors.

graph_node_allocation_network_v0_contract.v1
  The V0 network contract is named and feature-manifest-bound:
  node_features[A,28], global_features[6], risk_features[10], shared node/global
  encoders, mask-aware pooling, separate actor/critic heads, Dirichlet default.

graph_node_allocation_torch_policy_module_v0.v1
  Runtime replay/PPO evidence uses the Wikimyei Torch module-backed policy path;
  the fake trainable policy remains only a local contract/test fixture.

graph_node_allocation_torch_checkpoint_artifact_v0.v1
  Actor checkpoints are reloadable artifact directories with checkpoint.meta and
  module_state.pt. Runtime writes and reloads module state through the
  graph-node allocation policy adapter.

policy_training_resume_lattice_closure.v1
  Lattice parses, digests, displays, and proves PPO optimizer archive,
  CUDA-verification, and resume-mode evidence. `policy_training_artifact_ready`
  fails closed when a PPO fact lacks optimizer Torch archive evidence,
  `device_policy=require_cuda` / CUDA tensor-state proof, or internally
  consistent `fresh_spawn`, `resume_weights`, or
  `resume_weights_and_optimizer` lineage.

digest_hash_id_surface_cleanup.v1
  The five cleanup slices are complete: Hero fact inspect naming and prefix
  safety, short-ref display helpers, Marshal machine-payload boundaries,
  Runtime hash/digest naming cleanup, and cross-module vocabulary cleanup.
  Full digests remain canonical proof identity; short refs are display/query
  helpers only.

ppo_runtime_lattice_operator_surface_cleanup.v1
  Runtime PPO execution packets and Lattice policy-training fact JSON expose
  short operator refs beside full digest fields, and Lattice policy-training
  facts include operator diagnostics for optimizer archive, CUDA verification,
  resume-mode, parent-lineage, and authority-denial failures. Machine payloads
  and proof digests remain canonical; refs and diagnostics are display-only.

tsodao_settings_protection_contract.v1
  Lattice scans `kikijyeba.lattice.tsodao_settings_protection.v1` facts and
  proves `tsodao_settings_protection_ready` only when the protected settings
  bundle references an existing policy-training readiness fact, all protected
  policy/training/action/reward/execution/causal/checkpoint digests match that
  fact, evidence digests are bound, and Tsodao declares no optimization,
  selection, quality, readiness, deployment, execution, or live authority.

policy_acceptance_environment_certification.v1
  Environment exposes `hero.environment.certify.policy_acceptance
  mode=check|issue`. It loads existing `runtime.policy_training.fact` and
  `lattice.tsodao_settings_protection.fact` sidecars, derives parent fact
  digests from parsed evidence, validates the assembled
  `kikijyeba.lattice.policy_acceptance.v1` fact with Lattice exposure
  validators, and writes `lattice.policy_acceptance.fact` only in issue mode
  after `expected_preview_digest` matches the check preview. Issue refuses
  invalid assembled facts and refuses to overwrite an existing acceptance
  sidecar. Environment remains an evidence/admission surface only; Runtime
  remains the low-level executor; Lattice proves
  `policy_acceptance_contract_ready`.

policy_finalization_before_paper_online.v1
  The graph-node allocation policy lane is finalized as an evidence-grade
  pre-paper-online artifact path. Lattice requires the named V0
  acceptance-governance threshold contract, canonical mandatory baselines,
  after-cost metric identity, block-bootstrap uncertainty policy,
  validation/sealed-test discipline, reject-ties policy, negative-test evidence,
  threshold-selection audit, promotion-criteria evidence, Tsodao
  settings-protection lineage, and authority denials. This closes the current
  PPO policy artifact lane without claiming policy quality, selection, market
  readiness, paper-online readiness, deployment readiness, or live authority.

policy_operator_surface_and_identity_standardization.v1
  Policy training now has the same operator discipline as representation and MDN
  training without pretending it is the same kind of wave. Component waves remain
  the source-range/profile surface for representation and MDN jobs;
  policy-training profiles bind replay/rollout, causal schedule, reward,
  execution, policy DSL/net/features/jkimyei, action distribution, target-node
  universe, and checkpoint/evidence lineage; environment replay profiles remain
  historical-world/reset-step-reward profiles. Runtime policy jobs use the policy
  operator surface digest as their component-spawn identity, and Lattice
  `policy_training_artifact_ready` requires those policy-source and universe
  bindings before readiness can be satisfied.

component_policy_surface_reconciliation_audit.v1
  A root audit now records the common operator model across component waves,
  policy-training profiles, and environment replay profiles. The recommended
  direction is partial unification: keep the three profiles separate because
  they have different semantics, but add a future generic operator-surface
  digest for older representation/MDN component lanes so they read closer to
  the already-standardized policy operator surface.

component_operator_surface_digest_contract.v1
  Representation and MDN component-wave manifests now carry
  `component_operator_surface_digest`, computed from durable operator-surface
  inputs such as target family, protocol/graph/source cursor identity, wave
  mode/range/order, component assembly fingerprints, dock binding, training IDs,
  and checkpoint input paths. Runtime terminal facts and Lattice exposure facts
  expose the digest beside the existing family-specific assembly fingerprints,
  and Marshal inspect reports it as display/diagnostic evidence. Existing
  component-spawn matching and readiness checks remain family-specific.

paper_online_readiness_contract.v1
  Environment exposes `hero.environment.certify.paper_online_readiness
  mode=check|issue`. It loads an existing `lattice.policy_acceptance.fact`,
  derives accepted policy identity from parsed evidence, assembles a
  `kikijyeba.lattice.paper_online_readiness.v1` fact, validates session
  lifecycle, clock/staleness, idempotency, duplicate-protection, persistent
  paper-ledger recovery, direct-edge universe, locked Cajtucu execution-profile,
  reward/report artifact, operator-abort, and kill-switch policy bindings, and
  writes `lattice.paper_online_readiness.fact` only in issue mode after
  `expected_preview_digest` matches the check preview. Lattice proves
  `paper_online_readiness_contract_ready`; Environment still does not run
  paper-online, Runtime still does not select policies or prove readiness, and
  no surface claims market/deployment readiness or live-capital authority.

paper_online_session_contract.v1
  Environment exposes `hero.environment.certify.paper_online_session_admission
  mode=check|issue`. It consumes a compact `admission_request` object or
  `admission_request_path`, reads `lattice.paper_online_readiness.fact`,
  validates durable session state/event/intent/ledger/report paths, rejects
  stale or mismatched readiness proof, and writes
  `lattice.paper_online_session_admission.fact` only in issue mode after preview
  digest binding. Admission writes no session state, starts no runner, and keeps
  broker, live, and direct policy-to-broker authority denied.

paper_online_session_runner.v1
  Environment exposes `hero.environment.paper_online.session mode=validate|run`
  behind `allow_paper_online_session_run` and
  `paper_online_session_max_steps`. It consumes a compact `session_request`
  object or `session_request_path`, reads existing admission and readiness
  sidecars, validates finite step/timing/target/staleness/ledger-recovery and
  duplicate-action/intent constraints, and in run mode refuses overwrite before
  writing bounded paper-only session artifacts plus `lattice.exposure.fact` and
  `lattice.paper_online_session.fact`. Lattice now indexes and renders the
  `paper_online_session` fact family. The runner binds
  `cajtucu.execution.paper.v1` paper execution identity only; no broker orders,
  live capital, policy/checkpoint selection, market-readiness claim, or
  deployment authority is introduced.

paper_online_session_marshal_handoff.v1
  Marshal exposes `hero.marshal.paper_online.session_handoff mode=plan|dry_run`
  with a compact `handoff_request` object or `handoff_request_path`. Plan mode
  assembles Environment-compatible admission/session payload digests without
  external execution. Dry-run mode calls only Environment admission check and,
  after admission evidence exists, Environment session validate. It writes a
  durable Marshal receipt that records handoff/request digests, validation
  results, sidecar visibility, authority denials, and next safe actions.
  Marshal still does not issue admission, run the session, execute Cajtucu,
  route broker orders, select policy/checkpoint, prove Lattice targets, or
  authorize live capital.

paper_online_session_marshal_run_handoff.v1
  Marshal extends `hero.marshal.paper_online.session_handoff` with
  `mode=run` while keeping the same compact `handoff_request` or
  `handoff_request_path` surface. Run mode repeats local validation, readiness
  sidecar visibility, Environment admission check, and Environment session
  validate before delegating `hero.environment.paper_online.session mode=run`.
  Successful delegation reports `dispatch_state=executed`; failed delegated run
  reports `dispatch_state=run_blocked`. Dry-run and run receipts now include
  Environment session-run call digests, session fact visibility, authority
  denials, and next safe actions. Marshal still does not issue admission,
  execute Cajtucu directly, route broker orders, select policy/checkpoint,
  prove Lattice targets, or authorize live capital.

marshal_dispatch_readiness_repair.v1
  Post-dev-nuke train-core dispatch now has a proof-clean first step again:
  Lattice produces suggested waves when no completed Runtime candidate exists,
  Marshal prepare/inspect returns promptly, representation train-core plan and
  dry-run preflight are handoff-ready through the canonical `src/config/.config`,
  the duplicate train-core `.config` overlays are removed, and
  MDN correctly waits on the representation checkpoint-source dependency.
```

## Next Goal Queue

No next implementation goal is selected here. Choose a new finite milestone
explicitly before moving beyond the completed learned-policy PPO activation.

## Deferred Future Work

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
policy-quality/checkpoint-selection claims beyond policy_acceptance_contract.v1
paper-online before paper_online_readiness_contract.v1
live broker APIs before paper-online
allocator tuning against one validation window
checkpoint ranking by validation metric
market-readiness or deployment-readiness policy gates
unbounded Marshal scheduling
Codex-in-the-loop public evaluation
```
