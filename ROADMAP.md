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

### Non-Anticipative Provenance Status

```text
The first anchor-v1 enforcement chain now exists for:

  checkpoint generation influence
    -> forecast_eval
    -> replay_environment
    -> policy_training / PPO execution proof
    -> Marshal readiness gate

Do not promote smoke/research evidence into policy acceptance, paper-online
readiness, deployment readiness, or "complete training" unless the relevant
Lattice no-lookahead proof passes and Marshal observes the structured passing
certificate state for the exact claim/evidence snapshot.

Contract-source ownership now has an anchor-v1 home. The rule comes from
the `NO_LOOKAHEAD_CONTRACT` block in
`src/config/kikijyeba.protocol.cwu_02v.dsl`, and the
representation, MDN, and policy `.jkimyei` files reference that canonical
contract by id and digest instead of restating independent local copies. The
rule remains: every artifact consumed by a training, replay, evaluation,
readiness, or publish claim must have transitive influence that excludes
information forbidden for that claim.

The serving architecture is:

  component `.jkimyei` order / serving dependency:
    representation -> MDN -> policy

  possible implementation schedule:
    consume prior approved bundle for slice S
    train/update new state from S
    publish new bundle only for future admissible slices

The original concern was same-slice upstream checkpoint leakage and artifact
laundering: a leaky checkpoint can flow through forecasts, replay outputs,
policy-training inputs, and readiness claims even if the final consumer does not
read that checkpoint directly. Anchor-v1 now enforces that concrete path over
accepted anchor frontiers and fails readiness-grade policy facts when the
transitive parent proof is incomplete, stale, drifted, or inconsistent with
Runtime/Lattice evidence. Snapshot-bundle publishability adds the promotion
guard above that: readiness-grade policy facts must expose a Lattice-approved
`snapshot_bundle_manifest.v1` generation vector for representation, MDN, and
policy rather than loose latest checkpoint paths. Anchor-v1 also carries a
scalar label/reward availability frontier through checkpoint, forecast, replay,
policy-training, and Marshal certificate state. Anchor-v1 policy readiness also
binds an explicit embargo/purged-window policy fingerprint and half-open anchor
window; consumed parent influence and label/reward availability must clear the
window begin. `causal_provenance_generalization.v1` is the current aggregate
state over those checked subproofs: no-lookahead artifact provenance, scalar
label/reward availability, scalar embargo/purged-window admissibility, inline
artifact-production closure, trained-against interface stability, and snapshot
bundle publishability. It is an anchor-v1 aggregate, not proof of full
interval-set/fold/horizon algebra. Remaining broader work includes full
label/reward horizon semantics beyond the scalar frontier, interval-set/fold
embargo generalization, bundle interface-stability generalization, and
generalized artifact-production provenance.

Coordination document: `not_forward_leaking_problem.md`.

Range-expression follow-up: the anchor-v1 examples and current operator
fixtures still use raw accepted-anchor index intervals such as `[0,1600)` and
`[1800,2050)`. Those concrete intervals are useful for deterministic tests but
are brittle as the source corpus grows. Add a stable range-expression layer
before broader performance work: named split windows, percentage/quantile ranges
over an immutable cursor snapshot, source-time boundaries, or another durable
range spec that materializes to half-open anchor intervals for Runtime/Lattice.
Lattice should prove against both the materialized interval and the range-spec
digest, not ad hoc integers alone.

Known issue: `missing_ranges_exposure.v1`. No-lookahead proves that a learner
did not know forbidden information; missing-ranges exposure asks whether the
learner used enough admissible information. The clean anchor-v1 protocol creates
a real data-efficiency/staleness tradeoff: representation and MDN can be fit on
historical ranges before the policy target, while policy trains on later
certified replay artifacts. For every target slice, first compute the latest
admissible upstream fit frontier. A range gap should be classified as
`usable_but_unused`, `forbidden_by_embargo_or_label_availability`, or
`unknown_availability`, not merely reported as missing. If `[1600,1800)` is
admissible before target `[1800,2050)`, representation/MDN should normally train
through it; if it is blocked by horizon, embargo, split policy, or unknown label
availability, it should not be counted as usable exposure. Future approach:
expanding walk-forward upstream training, a certified replay bank, rolling
policy experience over many certified chunks, bootstrap lanes for cold start,
and temporal out-of-fold/cross-fit only after richer label/reward and embargo
semantics exist. Do not solve this by letting upstream models train on the same
target slice before producing same-slice policy inputs.

Performance notes: the current hardening mostly adds evidence emission, digest
binding, manifest lookup, and Lattice proof evaluation at job/artifact
boundaries. It should not materially slow the inner PPO/MDN/representation math,
but it can make readiness checks slower if the catalog scan is broad or digests
are recomputed too often. Runtime now writes
`policy_training_anchor_v1_performance_profile.v1` as diagnostic evidence beside
the PPO update, validation, and policy-quality reports. Early policy-quality
numbers from small windows are exploratory only; apparent performance may drop
versus old runs because anchor-v1 removes future-corpus leakage and forces causal
exposure discipline.
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

Current completed baseline stack:

```text
policy_training_anchor_v1_performance_profile.v1
  Runtime emits a durable diagnostic report at
  `policy_training_anchor_v1_performance_profile.report` and binds the report
  path/digest in `policy_training.report` and the Runtime execution packet.
  The first profile pass identified update instability:
  `sample_count_low=true`, `ppo_kl_target_exceeded=true`, and
  `ppo_clip_fraction_high=true`.

ppo_update_stability.v1
  Runtime now accepts a durable `ppo_learning_rate` contract field and the
  repo policy-training contract pins the PPO V0 update at `0.00002`. The fresh
  CUDA-backed run
  `policy_training_ppo_v0_caa36cdf586d3e35.attempt_caa36cdf586d3e35_1781850941430403_61014`
  reports `ppo_approx_kl=0.0107564`, `ppo_kl_target_exceeded=false`,
  `ppo_clip_fraction=0.08`, and `ppo_clip_fraction_high=false`. The remaining
  profile warning is `sample_count_low=true`; the next performance tag is
  `increase_certified_replay_exposure`. This remains diagnostic evidence only,
  not a policy selector, market-readiness claim, deployment-readiness claim, or
  live authority.

certified_replay_exposure_greenpath.v1
  Runtime now derives a `certified_replay_bank_manifest.v1` from the completed
  source replay job used by PPO V0 execution, binds the bank manifest digest,
  policy experience-set digest, chunk count, sample count, and coverage range in
  the rollout report, policy report, performance profile, execution packet, and
  `runtime.policy_training.fact`, and Lattice parses those fields as structured
  policy-training exposure evidence. The fresh CUDA-backed run
  `policy_training_ppo_v0_efb4ccf407674ffb.attempt_efb4ccf407674ffb_1781839605640527_47522`
  writes certified replay bank digest
  `07bef592e2978e4c5d9b1c2af59e6f0d5740bacc53bc7e0999f8eafcba250ff9`, with 4
  chunks, 250 certified samples, and coverage `[1800,2050)`. It reports
  `ppo_approx_kl=0.0216163`, `ppo_clip_fraction=0.196`,
  `ppo_clip_fraction_high=false`, and still flags `sample_count_low=true`; the
  next performance tag remains `increase_certified_replay_exposure`. This is an
  exposure-accounting greenpath, not a relaxation of no-lookahead and not a
  policy-quality or readiness claim.

certified_replay_scaling_performance_profile.v1
  Runtime now classifies whether low PPO sample count is caused by a rollout
  cap, missing replay-bank binding, certified replay source capacity, or PPO
  update instability. The fresh CUDA-backed run
  `policy_training_ppo_v0_efb4ccf407674ffb.attempt_efb4ccf407674ffb_1781842241525735_75051`
  consumed all 250 certified replay samples from 4 chunks over `[1800,2050)`,
  reports `certified_replay_sample_count_low=true`,
  `certified_replay_source_capacity_limited=true`,
  `policy_training_uses_all_certified_replay=true`, and a 774-sample deficit
  to the 1024-sample diagnostic threshold. It also reports
  `ppo_approx_kl=0.0424796`, `ppo_kl_target_exceeded=true`, and
  `ppo_clip_fraction_high=false`, which correctly tagged
  `ppo_update_stability`. The follow-up stable run
  `policy_training_ppo_v0_caa36cdf586d3e35.attempt_caa36cdf586d3e35_1781850941430403_61014`
  lowers PPO learning rate to `0.00002`, reports
  `ppo_approx_kl=0.0107564`, `ppo_kl_target_exceeded=false`,
  `ppo_clip_fraction=0.08`, and moves the bottleneck to
  `certified_replay_source_capacity` with recommendation
  `generate_broader_certified_replay_source`.

policy_training_anchor_v1_readiness_greenpath.v1
  Completed closeout target before performance exploration. The green path is a
  fresh post-dev_nuke Runtime PPO execution that satisfies
  `policy_training_artifact_ready` with a passing no-lookahead certificate,
  complete/admissible provenance, matching causal closure, generation-backed
  checkpoint facts, replay lineage, snapshot-bundle state, and zero target
  deficits. This is still not policy acceptance, paper-online readiness,
  deployment readiness, complete training, or live authority.

policy_training_anchor_v1_performance_baseline.v1
  Completed diagnostic baseline after the green anchor-v1 policy-training path
  and refreshed after `increase_certified_replay_exposure.v2`. The selected
  policy-training fact is now `517fb94fec140323`, the Lattice certificate
  digest is `8f85723bd45b674f`, and the baseline report lives at
  `policy_training_anchor_v1_performance_baseline.md`. The current PPO run
  `policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435`
  consumed 17 certified replay chunks and 1047 certified replay samples over
  `[1200,2247)`, writes policy generation
  `policy_generation_50be38004d0880cc`, and is valid from anchor `2247`.
  The update is not obviously unstable (`ppo_approx_kl=0.0141982`,
  `ppo_clip_fraction=0.0706781`), and the profile now reports
  `sample_count_low=false`, `certified_replay_source_capacity_limited=false`,
  and zero sample deficit to the 1024-sample diagnostic threshold. The next
  performance tag moved to `policy_quality_comparison`. This remains
  diagnostic evidence only, not policy acceptance or deployment readiness.

increase_certified_replay_exposure.v1
  First bounded increment completed. Durable config now selects the wider
  existing certified replay source
  `cwu_02v_channel_validation_eval_mdn_1600_2247.run.channel_inference_mdn.attempt_000011`.
  Runtime binds the replay job-dir digest, certified replay bank manifest
  digest, and policy experience-set digest into the policy-execution consumed
  artifact closure; Lattice accepts that bank-backed replay input as the replay
  identity for `policy_training_artifact_ready`. The result is a fresh green
  proof over 647 samples instead of the earlier 617-sample baseline. The
  remaining bottleneck is still certified replay source capacity, so the next
  bounded increment should generate broader no-lookahead-clean replay evidence
  rather than tune PPO first.

increase_certified_replay_exposure.v2
  Completed bounded exposure milestone. Durable config trains the upstream
  representation and MDN on `[0,1170)`, leaves the `[1170,1200)` context/purge
  gap, and generates forecast/replay over `[1200,2247)` from generation-backed
  upstream checkpoints. Runtime then executes the policy contract against the
  broader certified replay source, producing 17 replay chunks and 1047 policy
  samples. A narrow combined Lattice evidence root satisfies
  `policy_training_artifact_ready` with `proof_certificate_check_passed=true`,
  `deficits=[]`, no-lookahead provenance complete/admissible, embargo complete,
  causal provenance complete/admissible, and
  `snapshot_bundle_publishability.v1` complete/admissible. This closes the
  certified-replay source-capacity bottleneck for the 1024-sample diagnostic
  threshold; the next performance work should compare policy quality and PPO
  behavior rather than expand replay exposure first. This remains
  exposure/performance evidence only, not a policy-quality gate or
  paper-online/deployment readiness claim.

training_order_lattice_correctness.v1
  Anchor-v1 now proves non-anticipative artifact provenance over the concrete
  checkpoint -> forecast_eval -> replay_environment -> policy execution path,
  including Runtime evidence, Lattice recomputation, checkpoint generation
  manifests, and Marshal certificate-state gating.

canonical_jkimyei_no_lookahead_contract_refs.v1
  Complete for anchor-v1. Representation, MDN, and policy `.jkimyei` metadata
  reference one canonical no-lookahead/order contract by id and digest; Runtime
  policy facts carry the policy `.jkimyei` contract refs; Lattice rejects
  readiness-grade policy facts when those refs are missing, drifted, or
  inconsistent with the contract digest carried by Runtime evidence.

snapshot_bundle_publishability.v1
  Gate implemented for the anchor-v1 policy-training readiness path. Marshal
  consumes the structured
  `snapshot_bundle_publishability.v1` state alongside
  `no_lookahead_artifact_provenance.v1`; certificate refs or unresolved
  `latest_satisfying` selectors are not sufficient.

label_reward_availability_frontier.v1
  Gate implemented for anchor-v1 policy-training readiness. Lattice treats
  accepted-anchor influence and label/reward availability as separate frontiers:
  both must be present, recomputable or inherited from parent evidence, and no
  later than the policy target begin. Runtime emits parent-policy and output
  policy availability state; Marshal refuses readiness-grade handoff when the
  Lattice certificate state is missing, unchecked, incomplete, unbound, or not
  admissible. This is a scalar anchor-v1 frontier, not full label/reward horizon
  algebra.

embargo_purged_window_correctness.v1
  Gate implemented for anchor-v1 policy-training readiness. Policy facts carry
  an `embargo_policy_fingerprint` plus a half-open
  `embargo_purged_window_anchor_range`; Lattice includes both in the
  no-lookahead closure digest and fails readiness-grade proof when the window is
  missing, not claim-bound, or parent influence/availability overlaps the
  window begin. Runtime emits the fields in policy-training sidecars and Marshal
  gates on the structured Lattice state. This is still scalar anchor-v1
  protection, not full interval-set, fold/out-of-fold, horizon-group, or
  purged-CV correctness.

causal_provenance_generalization.v1
  Gate implemented for the policy-training readiness path as an aggregate over
  the anchor-v1 subproofs above. Runtime policy-training facts carry a
  `causal_provenance_schema`, causal atom/interval schema ids, scalar
  label/reward and embargo policy fingerprints, inline artifact-production
  closure digest, trained-against interface-stability contract digest, and a
  causal provenance closure digest. Lattice requires those fields to agree with
  the recomputed no-lookahead closure and snapshot-bundle state. Marshal treats
  the parsed causal state as readiness-grade gate material; a passing
  no-lookahead certificate without this aggregate state is not enough.
```

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
  The learned-policy path uses the `policy_training_ppo_v0` Runtime wave.
  Runtime derives sparse dry-run activation from the selected wave, configured
  policy `.jkimyei`, split policy, and completed compatible replay artifacts
  instead of keeping a checked-in activation contract. Historical pre-anchor-v1 PPO
  evidence proved that Runtime could run non-noop
  `policy_kind=ppo_policy_adapter.v1` training and write actor/critic
  checkpoints, optimizer state, rollout/update/validation/policy-quality
  reports, and `runtime.policy_training.fact`. That historical satisfied-target
  result is no longer a current readiness claim. Under the stricter anchor-v1
  gates, the latest closeout run
  `policy_training_ppo_v0_4853702313be2dbf.attempt_4853702313be2dbf_1781707121886925_59037`
  is inspectable performance evidence only: Lattice blocks
  `policy_training_artifact_ready` because upstream forecast/replay provenance,
  generation-vector closure, snapshot bundle publishability, and causal closure
  agreement are incomplete. Live-capital authority remains disabled.

policy_network_architecture_review.v1
  BLOCKED FOR REVIEW: this milestone recorded the downstream-policy path:
  policy remains downstream of representation/MDN/observer and consumes
  AllocationBelief-derived policy_input_t. That may be correct for serving-time
  dependency, but it is not a Lattice-enforced training-order proof. Revisit it
  under `training_order_lattice_correctness.v1`.

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
