# Hero Runtime

This folder contains the executable job layer.

Runtime does not implement source loading, component math, or optimizer loops.
It runs a wave against a compiled protocol contract. The runner writes a job
manifest, resolves the graph-anchor wave plan, delegates execution to the
Jkimyei representation or inference launchers, writes Runtime-owned terminal
facts, and writes a mutable job state.

The manifest records both sides of the launch:

- protocol contract identity: source/topology/assembly/dock/training
  compatibility fingerprint, excluding runtime model-state inputs and
  model-state admission flags,
- wave identity: target component family, mode, and resolved graph-wide Ujcamei
  cursor range.

Runtime artifacts are organized under one disposable root:

```text
/cuwacunu/.runtime/
  cuwacunu_exec/
    system/
      runtime_layout.v1.lls
      component_spawn_registry.v1.lls
    components/
      <component_family>/
        spawns/
          <component_spawn_id>_<component_spawn_fingerprint>/
            component_spawn.ref
            jobs/
              <wave_action>/
                <job_id>/
                  job.manifest
                  job.state
                  runtime.result.fact
                  runtime.checkpoint_io.fact
                  runtime.health_measurement.fact
    indexes/
      lattice_runtime_index.v1.lls
    cache/
```

`cuwacunu_exec` is the canonical execution root. Runtime Hero's `dev_nuke`
may dry-run or clear either this execution root or the whole disposable
`/cuwacunu/.runtime` tree when an operator policy enables it. Checked-in
policies keep actual reset disabled and keep backup snapshots off by default,
so cleanup does not create backup clutter under the runtime tree. When enabled,
snapshots are written under `/cuwacunu/.backups/runtime_dev_nuke`; Runtime first
tries an atomic move and falls back to recursive copy then remove if rename is
not available across filesystems.

The folder path is for legibility and retrieval; it is not proof authority.
The authoritative identity remains in `job.manifest`, Runtime terminal facts,
checkpoint facts, and Lattice proof certificates. The spawn path segment keeps
the short `component_spawn_id` and the full component-spawn fingerprint visible
so repeated runs of the same configured component instance naturally group
together.

The component-spawn fingerprint uses the component family, active protocol
contract, graph order, and component assembly. It deliberately excludes the
wave id, action, mode, source cursor token, and requested/completed source
range; those are job/evidence/checkpoint lineage fields so train and eval waves
can operate on the same component spawn while Lattice still audits source
coverage and leakage.

Runtime roots that contain `system/runtime_layout.v1.lls` are treated as
component-layout roots. Runtime, Lattice, and Marshal job discovery scan
`components/` for those roots and do not accept old flat child job folders as
part of the execution root. Unmarked temporary fixture roots may still be
scanned recursively by tests, but the marked runtime layout is authoritative
for real execution evidence.

`job_runner_t` resolves the Jkimyei delegate from the protocol-resolved
`TARGET` by default. `wikimyei.representation.encoding` resolves to the active
representation family for the selected protocol, while
`wikimyei.inference.expected_value` resolves to the channel-context MDN
launcher. The latter runs the active representation as a frozen dependency and
mutates only Channel MDN in `MODE=train`. `MODE=run` executes the target
dependency closure without optimizer steps; `MODE=train` mutates only the target
component and runs upstream dependencies frozen.

`wave_settings.h` decodes the selected block from the
`hero.runtime.wave.dsl` catalog. The selected wave declares the exact
`PROTOCOL`, a broad protocol-level `TARGET`, mode flags such as `run | debug`,
and either `SOURCE_CURSOR_ID` or `SOURCE_SPLIT`. Cursor ids are resolved from
`ujcamei.source.cursor.dsl`, where protocol-neutral full-domain and ad hoc
source windows are defined for runtime `.lls` reports. `SOURCE_SPLIT` resolves a
named train/validation/test selector from `ujcamei.source.splits.dsl`; absolute
selectors provide bounds at decode time, while fraction selectors materialize
against the active accepted-anchor domain during wave planning. A cursor range is
larger than a batch: the stream generator yields graph-anchor batches until the
requested range is exhausted. `SOURCE_RANGE=source_key` is the stable cursor
authoring form for key-valued sources; it resolves to the accepted graph-anchor
index domain before execution while preserving requested source-key bounds in
runtime evidence.

`TARGET` is the focal component family, not the whole execution chain. For
example, `TARGET=wikimyei.inference.expected_value` still runs NodeLift and the
active frozen representation because MDN depends on node encodings.
`MODE=train` mutates only the target; `MODE=run` applies no optimizer step.
`debug` is only a report modifier and may be combined with either primary mode.
Wave settings are not topology and are not training policy. Topology lives in
`kikijyeba/topology`; training policy lives in `.jkimyei` files consumed by
Jkimyei.

Operator profile vocabulary:

- `component_wave`: a Runtime wave selected from `hero.runtime.wave.dsl`; use it
  for representation/MDN train or run jobs over a source range.
- `policy_training_profile`: a Runtime policy-training contract; use it for
  graph-node allocation policy training, where replay lineage, causal schedule,
  policy DSL/net/features/jkimyei, action distribution, reward, execution
  profile, target-node universe, PPO update evidence, and checkpoint lineage are
  one bound surface.
- `environment_replay_profile`: a Kikijyeba historical replay environment
  profile; use it for reset/step/reward worlds that consume completed Runtime
  artifacts and call Cajtucu paper execution.

PPO V0 resume is explicit and contract-bound. `resume_mode=fresh_spawn`
creates a new graph-node allocation actor; `resume_weights` loads a parent
actor checkpoint after metadata compatibility checks; and
`resume_weights_and_optimizer` also loads the bound Torch optimizer archive.
Resume compatibility requires the parent checkpoint to match the current policy
family, module/network contract, feature manifest, action distribution/config,
PPO config, causal schedule, snapshot family, reward contract, execution
profile, graph order, and actor/critic architecture digests. PPO V0 execution
requires `device_policy=require_cuda`; Runtime records CUDA verification for
module parameters, forward tensors, loss tensors, and optimizer state.
The generated `runtime.policy_training.fact` carries the optimizer-state schema,
Torch optimizer archive digest, CUDA verification fields, and resume-mode
lineage so Lattice can reject stale or incomplete PPO readiness evidence.
Runtime PPO execution packets also include display-only refs such as
`actor_checkpoint_ref`, `optimizer_torch_state_ref`, and
`ppo_update_report_ref` beside the full digest fields. These refs are operator
handles only; full digests remain the machine identity used by Lattice.

`hero.runtime.run` accepts `runtime_handoff_path` plus
`runtime_handoff_digest` for concrete operator handoffs. The handoff object
binds schema/id/digest, target id, base config path/digest, concrete wave
fields, concrete checkpoint inputs, Runtime policy path/digest identity, and
dry-run/execute intent. Runtime rejects non-empty unresolved-symbol lists and
symbolic model-state selectors such as `latest_satisfying:*` before launching
`cuwacunu_exec`. When a handoff is accepted, Runtime passes the handoff id and
digest into the job runner so `job.manifest`, `runtime.result.fact`, and the
derived lattice exposure sidecar can echo the same identity. Runtime also passes
accepted
`PLAN_INPUT_REPRESENTATION_CHECKPOINT` and `PLAN_INPUT_MDN_CHECKPOINT` values as
launch-time checkpoint overrides to `cuwacunu_exec`; readiness-grade handoffs do
not require copying those materialized paths back into static `.jkimyei` files.

Reusable wave profiles live as multiple `WAVE_SETTINGS` blocks in the single
`hero.runtime.wave.dsl` catalog. Operator configs select the active block
with `[HERO].runtime_wave_id`; the canonical multi-wave catalog does not carry
a file-level `WAVE_SELECTION` fallback. Profiles should keep protocol, target, mode,
cursor-or-split, and source-order intent stable. Full-domain and ad hoc cursor
definitions live in `ujcamei.source.cursor.dsl`; named train/validation/test
split intent lives in `ujcamei.source.splits.dsl` and is selected with
`SOURCE_SPLIT`. Lattice split proof/protection policy lives separately in
`hero.lattice.split_policy.dsl`. Cursor definitions intentionally avoid protocol
names because the source cursor catalog is data identity, not protocol identity.
Concrete launch range overrides belong
in the runtime handoff artifact, or may be supplied directly to `cuwacunu_exec`
with `--source-range`, `--anchor-index-begin/end`, or
`--source-key-begin/end` for developer recovery. Runtime applies the handoff
overlay in memory, validates the
effective wave, and records the resolved range in the manifest/state sidecars
without editing the profile.

Direct `cuwacunu_exec` use remains possible for developer recovery and narrow
debugging, but it is not the preferred operator path. Non-dry-run launches
without Runtime/Marshal handoff identity print a warning because they do not
prove Marshal validated Lattice advice, Runtime policy, active wave bounds,
checkpoint/source identity, or handoff lineage. Direct `--replay-from-job-dir`
launches also warn because they do not prove Marshal validated rollout bounds,
Runtime policy, or Cajtucu execution-profile identity. Artifacts from direct
debug/recovery launches may be unsuitable for readiness claims; prefer
`hero.marshal.prepare`, `hero.marshal.rollout`, or `hero.runtime.run` for
operator evidence.

Every completed or failed job writes compact terminal sidecars:

```text
runtime.result.fact
runtime.checkpoint_io.fact
runtime.health_measurement.fact
lattice.exposure.fact
lattice.source_analytics.fact
lattice.checkpoint.fact     # when a checkpoint exists
```

These facts normalize the end-of-job result, checkpoint I/O, and model-health
measurements from `job.manifest`, `job.state`, and the component report. They
are Runtime evidence, not Lattice proof authority; Lattice still proves by
reading Runtime artifacts, and Marshal uses these facts as the preferred
operator-report surface with raw reports as fallback.
Runtime terminal facts use explicit digest/id names on operator-visible fields:
`source_report_digest`, `config_bundle_id`, `runtime_policy_digest`, and
`checkpoint_artifact_digest`. Runtime rejects historical Marshal handoff
objects that use `base_config.hash` or `runtime_policy.hash`, and Lattice
readers require `checkpoint_artifact_digest` rather than historical
`checkpoint_artifact_hash` sidecars.
The Runtime-emitted `lattice.source_analytics.fact` is deliberately narrow:
it binds source cursor, graph order, split/window, parent exposure digest,
anchor acceptance fraction, missingness fraction, duplicate-anchor count, and a
source-health level. If source data analytics `.lls` reports are already
job-local, Runtime also copies their entropy, information-density,
compression-ratio, and power-spectrum visibility metrics into the fact. It is
visibility evidence only and carries no readiness, coverage, leakage, or
contract-identity authority.

For channel MDN jobs, terminal facts also carry compact performance visibility
fields when the component report provides them: per-channel NLL,
per-target-feature NLL, per-channel/target-feature support counts, sigma health,
mixture entropy/usage, finite-parameter state, nonfinite-output count,
checkpoint I/O, and whether the run mutated model state. These are measurements
for reports and warnings; they are not performance gates by themselves.
For non-mutating channel MDN `MODE=run` jobs, Runtime also writes
target-transform, forecast-baseline, and forecast-eval Lattice facts. The
forecast-eval fact copies the report's stratified NLL and valid-target support
surfaces as inspection evidence only.

For non-dry-run channel MDN `MODE=run` jobs, Runtime also installs a
best-effort Kikijyeba replay artifact observer by default. The observer receives
each streamed MDN pulse, builds replay forecast records through
`wikimyei.observer.belief`, derives the matching graph-anchor future
realization batch from the representation stream, and writes pulse-local
artifacts under:

```text
artifacts/kikijyeba.environment.replay.v1/pulses/
```

The top-level `runtime_replay_batches.index` records every pulse packet. CLI
options may disable this sidecar writer or provide explicit replay accounting
numeraire and target-node lists when the graph cannot infer a unique
graph-node action universe. Replay artifact setup/write failures are recorded in
`job.state` as
`replay_artifact_error`; they do not fail the MDN job. These artifacts are
replay evidence for `kikijyeba.environment.replay.v1`; they do not change MDN
inference outputs, optimizer steps, checkpoint behavior, or Lattice proof
authority. The environment read-side companion can consume a
completed Runtime job directory, load every indexed pulse packet, and expose the
packets as replay bundles for historical replay experiments. The Kikijyeba
environment driver can then run enabled replay policies against those bundles
and write a replay experiment report plus
`runtime_replay_experiments.index`. The report includes top-level
observation/action/execution time-law counters, future-observation and
realization-key violation counts, and projection-validation step counts derived
from the per-step replay evidence. This remains a post-job replay adapter and
does not change Runtime's MDN execution semantics. The executable surface is
`cuwacunu_exec --replay-from-job-dir <job_dir>`, which consumes an already
completed job directory instead of launching a new graph-first wave. Runtime
Hero exposes the same low-level path through a non-catalog Runtime replay
executor. Operators should prefer `hero.environment.rollout`, which carries the
direct rollout selectors, validates rollout admission, and delegates back to
Runtime when replay is allowed. Runtime checks the job is completed and has
replay batch evidence, then delegates to the executable with the requested
replay policy options and Cajtucu paper profile hints such as
`allow_synthetic_direct_edges`, `linear_transaction_cost_rate`, and
`max_parallel_jobs`. When the
execution request sets `validation_rollout=true`, Runtime treats replay as
validation-grade: it
requires positive `max_steps`, positive `max_parallel_jobs`, nonzero
`linear_transaction_cost_rate`, `execution_profile_digest`,
`policy_set_digest`, and rejects synthetic direct execution edges at the request
boundary. Execute mode also gates the produced replay report: validation replay
fails closed if the report is missing, has no completed episodes, lacks profile
or policy-set identity, contains `cajtucu_synthetic_market_step_count > 0`,
contains `cajtucu_numeraire_fallback_pair_count > 0`, or contains
`cajtucu_invalid_trace_count > 0`. The matching read path is named too:
`hero.runtime.inspect.artifact.job` can inspect `replay_batch_index`,
`replay_experiment_index`, and `replay_experiment_report`, and
`hero.runtime.inspect.artifact.path` reads explicit runtime artifact paths.
`hero.runtime.inspect.job` summarizes those replay artifacts beside the regular
Runtime manifest/state/report summaries. Runtime inspect tools now expose their
small selectors directly: `hero.runtime.inspect.wave` accepts optional
`config_path`, `hero.runtime.inspect.jobs` accepts `root`, `limit`, and
`include_artifacts`, and job/artifact selectors such as `job_dir`, `artifact`,
`include_text`, and `max_bytes` are direct arguments.

Policy is represented as a first-class component wave for
`wikimyei.policy.portfolio.graph_node_allocation`, with a specialized
Runtime driver. `hero.runtime.run mode=dry_run` synthesizes the
policy-training packet for an active policy component wave from the selected
wave, split policy, configured policy profile, and completed compatible replay
artifacts under the Runtime root; the same `hero.runtime.run` surface with
`contract_path` and optional `contract_digest` runs a materialized proof
contract. A Marshal handoff may also carry `policy_training_execution_lock`,
which Runtime expands to the same internal contract path/digest before
execution. The selected policy-training `WAVE_SETTINGS` block
may bind `POLICY_ID`, `POLICY_KIND`, `TRAINING_SCHEDULE_MODE`, and
`LIVE_EXECUTION_ALLOWED`; if the active config wave is not policy training,
Runtime uses the unique policy-training block in the same catalog and rejects an
ambiguous catalog. Sparse materialized contracts may omit the active protocol id
when `config_path` is present, plus default schema/job-kind,
action-distribution, resume-mode, causal schedule, cursor-key, and selector-split
values. The contract still supplies causal schedule evidence, parent evidence,
range digests, and finite bounds. Runtime returns a deterministic
`kikijyeba.runtime.policy_training_job_contract.v1` packet with a contract
digest. The contract binds protocol/source identity,
policy identity, architecture/training-config digests, disjoint
train/validation/test range digests, normalization and replay-buffer ranges,
observation/action-distribution/reward/execution-profile digests, causal
walk-forward schedule mode/digest, typed cursor-key ordering,
selector/checkpoint lineage, parent evidence digests, random seed, and finite
episode/step/wall-clock bounds. Runtime derives fixed contract constants for
the Environment replay contract, action schema, policy input schema, action
adapter, reward contract, PPO artifact contract, graph-node allocation policy
family, checkpoint/update/rollout schemas, optimizer-state schema, CUDA policy,
GAE estimator, causal-schedule schema, and no-future snapshot source.
No-lookahead and component-order contract refs, plus representation/MDN bundle
component ids, are derived from the active protocol DSL before validation, then
carried into policy `.jkimyei` and snapshot-bundle evidence.
Protocol-contract and graph-order fingerprints are derived from the configured
graph-first protocol bundle; contractless dry-runs select a completed MDN replay
job by validation anchor range plus protocol/graph compatibility and derive the
resolved source cursor token from that job.
PPO-shaped policy kinds additionally bind policy DSL/net/features/jkimyei
digests, with `training_config_digest` derived from the configured policy
`.jkimyei`, target-node universe digest, action-distribution config,
actor/critic architecture digests, PPO config,
advantage-normalization policy, and PPO hyperparameters before execution.
Runtime-owned actor/critic checkpoint, optimizer-state, rollout collection,
PPO update-report, validation rollout, and policy-quality report digests are
emitted after artifact writes instead of being authored in sparse contracts.
For PPO V0, the configured graph-node allocation `.jkimyei` owns
`LEARNING_RATE` and the PPO hyperparameter fields. Runtime derives
`ppo_learning_rate` from that profile, applies it to the combined policy/value
Adam optimizer, and mirrors the actual update rate into the actor/critic
learning-rate report fields.
Runtime folds those policy-source and environment bindings into
`policy_operator_surface_digest`; policy-training Runtime jobs use that digest as
the graph-node allocation policy component-spawn fingerprint, so sparse durable
PPO contracts do not author `component_assembly_fingerprint`.
Representation and MDN component-wave manifests also carry a
`component_operator_surface_digest`, computed from the durable component
operator surface: target family, protocol/graph/source cursor identity, wave
mode/range/order, component assembly fingerprints, dock binding, training IDs,
and checkpoint input paths. That generic digest is emitted beside the existing
family-specific assembly fingerprints; it does not replace current component
spawn matching or policy-training identity.
Runtime rejects contracts where normalization or replay-buffer inputs are not
training-bound, where train/validation/test ranges overlap by digest, where the
causal schedule does not bind a typed cursor-key ordering and ledger-derived
`no_future_snapshot_use` source, where target-label/reward/trajectory
availability extends beyond artifact `usable_from_key`, where
`offline_full_window_research` is presented as readiness evidence, or where live
execution is requested.
Policy-training evidence now carries the anchor-v1
`label_or_reward_availability_end_exclusive_max` frontier beside the accepted
anchor influence frontier. Runtime emits parent-policy availability inputs and
output-policy availability summaries in policy-training sidecars so Lattice can
join checkpoint, forecast, replay, parent-policy, and own-fit ranges without
trusting a standalone declaration.
Policy-training sidecars also emit Runtime-owned
`embargo_policy_fingerprint` and the half-open
`embargo_purged_window_anchor_range`. Lattice uses that window as the
readiness-grade parent admissibility cut for consumed influence and label/reward
availability; missing window metadata fails closed.
Policy-training sidecars additionally carry the
`causal_provenance_generalization.v1` aggregate metadata: causal atom and
half-open interval schema ids, scalar label/reward and embargo policy
fingerprints, inline artifact-production closure digest, trained-against
interface-stability contract digest, and causal provenance closure digest. These
fields are evidence for Lattice to check against recomputed parent closures;
Runtime does not self-certify the aggregate.
In this mode, `mode=execute` is allowed under Runtime execute/train
policy for the replay/paper `policy_kind=ppo_policy_adapter.v1` trainer.
The PPO V0 path writes actor/critic checkpoints, optimizer state,
rollout/update/validation reports,
`policy_training.report`, terminal Runtime facts, and a Lattice-readable
`runtime.policy_training.fact`. With no `report_path`, it uses an explicit
smoke fallback and declares `replay_backed_step_count=0`. With `report_path`,
it ingests an existing Kikijyeba replay report into PPO rollout samples,
including anchors, target weights, reward components, Cajtucu trace ids, costs,
rejects, partials, missing-pair counters, and source labels. If the replay
report was produced by the graph-node allocation policy component, Runtime can
also bind the
old-policy action-distribution evidence carried on the step report:
`policy_input_digest`, `action_distribution_id`, active nodes, old log
probability, entropy, value estimate, and actor-visible policy-input tensor
payloads. Replay-backed PPO training now fails closed when probability evidence
or tensor payloads are missing; only the explicit smoke fallback may run without
replay-backed probability evidence. With
`replay_job_dir`, Runtime dispatches `cuwacunu_exec --replay-from-job-dir` using
the `graph_node_allocation` policy component and `on_policy_sample` collection,
passes an explicit actor checkpoint artifact to the replay policy, and then
ingests the generated replay report with
`collection_source=kikijyeba_on_policy_replay`. Runtime records the collection
checkpoint path/digest and writes separate post-update actor/critic checkpoints.
For contract-backed runs, the same completed replay job is also the canonical
source for `source_cursor_token`, parent forecast/observer/allocation fact
digests, the parent forecast artifact digest, parent replay report digest,
replay-report `execution_profile_digest` and `policy_set_digest`, including
digest-bound replay reports under configured Runtime roots, the replay
batch-index digest, and policy-execution parent replay input mirrors when
replay-environment facts are available. Runtime derives the PPO minibatch size,
max-gradient norm,
and policy-training `max_steps` from the configured graph-node allocation
`.jkimyei`, derives
`linear_transaction_cost_rate` and policy-training `max_parallel_jobs` from the
configured Environment rollout profile, derives `initial_equity_numeraire`,
`max_node_weight`, and `max_turnover_l1` from the configured Environment replay
contract,
derives the policy id/kind and training-schedule mode from protocol
`POLICY_COMPONENT` plus the configured graph-node allocation `.jkimyei` task,
derives the causal schedule digest, snapshot-family digest, and selector policy
digests from Runtime policy-training defaults and active protocol identity,
derives the split-policy fingerprint and
training/validation/test range digests from the wave's
`TRAIN_SPLIT`/`VALIDATION_SPLIT`/`TEST_SPLIT` bindings plus
`[UJCAMEI].ujcamei_source_splits_dsl_path` bound to
`[HERO].lattice_split_policy_dsl_path`,
derives the PPO advantage-normalization policy from the Runtime PPO update
semantics, derives the Runtime-owned purged-embargo policy fingerprint, derives
the policy-execution target anchor range from `validation_range_digest`,
defaults an omitted embargo purged window from that target range, and derives
the policy-execution target id from the fixed
`policy_training_artifact_ready` artifact-readiness target and rejects authored
target drift. Runtime
derives representation/MDN bundle checkpoint and generation-vector inputs from
the replay forecast fact when `replay_job_dir` is present. Runtime
derives default policy-execution consumed artifact lists from
the policy-execution input locks and rejects authored lists that omit those
derived artifacts. Runtime also derives default policy-execution consumed
checkpoint lists from bundle checkpoint inputs and rejects authored lists that
omit them. Runtime derives default policy-execution consumed generation-vector
lists from bundle generation inputs and replay forecast lineage, and rejects
authored lists that omit them; parent policy generation-vector lineage remains
in its dedicated field.
Trained-against representation and MDN mirrors are also derived from
the bundle generation inputs and rejected when authored values drift. Policy
output fit and valid-from anchors are derived from the policy-execution target
range and rejected when authored values drift.
The actor checkpoint binds the graph-node allocation Torch module contract and
stores a sibling `module_state.pt` archive mutated by the Torch autograd
PPO-Clip/GAE update loop.
The optimizer receipt binds `kikijyeba.runtime.ppo_optimizer_state.v1` and a
sibling `ppo_v0_optimizer_state.pt` Torch archive used by
`resume_weights_and_optimizer`.
Runtime then dispatches a second deterministic replay from the same completed
replay job using the post-update actor checkpoint and a nonzero
transaction-cost rate, writing
`kikijyeba.runtime.ppo_cost_aware_validation_rollout.v1` as the validation
artifact. That report binds policy/action/reward/execution/causal identities,
the raw validation replay report digest, final equity/log-growth/drawdown
metrics when available, fee/spread/slippage costs, turnover, rejects/partials,
missing-pair counters, numeraire-fallback counters, target-tracking error, and
Cajtucu trace evidence. Runtime then writes
`kikijyeba.runtime.policy_quality_report.v1` from a separate raw comparison
replay report. That comparison replay keeps PPO validation PPO-only while
running the post-update graph-node allocation checkpoint and required baselines
(`numeraire_only.v1`, `current_weight_no_trade.v1`, `equal_weight.v1`, and
`kikijyeba.environment.policy.spot_distributional_utility.v1`) under identical
replay source, target universe, reward, execution profile, causal schedule,
snapshot family, accounting numeraire, and transaction-cost assumptions. The
quality report records policy-wise after-cost metrics, execution-feasibility
counters, target-tracking/projection metrics, and PPO-minus-baseline deltas, but
it declares `policy_selected=false`, `comparison_ranked=false`,
`winner_declared=false`, `policy_quality_claimed=false`, and
`lattice_policy_selector=false`. Its digest is bound into the generated
`runtime.policy_training.fact`; it remains comparison evidence, not policy
selection or market-readiness authority. The generated fact is now exercised
against concrete forecast-eval and observer-belief parent fact digests plus
replay-environment lineage. PPO adapter facts may bind that replay lineage
through `parent_replay_environment_report_digest`; allocation-engine parent
fact digests are required only for policy kinds that declare that parent.
Lattice can satisfy `policy_training_artifact_ready` from that generated
evidence without granting policy-quality, selection, market-readiness,
deployment, or live-capital authority.
Runtime execution results may report the bounded trainer role; the
Lattice-readable artifact fact remains evidence-only and carries no
live-capital, policy-quality, market-readiness, deployment, or target-proof
authority.

Policy acceptance sidecar emission is Environment-owned and evidence-only:
`hero.environment.certify.policy_acceptance mode=check|issue` takes direct
`policy_training_job_dir`, optional `tsodao_settings_protection_job_dir`,
`acceptance_id`, and `certification_evidence` arguments, loads the existing
policy-training and Tsodao settings-protection sidecars, assembles a
`kikijyeba.lattice.policy_acceptance.v1` fact, validates it with the same
Lattice exposure validators used by proof readers, and writes
`lattice.policy_acceptance.fact` under the policy-training job directory only in
issue mode after `expected_preview_digest` matches the canonical preview digest
returned by check mode. The
acceptance policy digest must identify the named
`policy_acceptance_governance_thresholds_v0.v1` contract, which fixes the V0
mandatory baselines, after-cost metric identity, zero-delta threshold,
block-bootstrap uncertainty policy, validation/sealed-test discipline,
reject-ties rule, cost/slippage assumptions, negative tests,
threshold-selection audit, and promotion criteria. Environment derives
`accepted_policy_training_fact_digest` and
`tsodao_settings_protection_fact_digest` from parsed sidecars rather than
trusting caller-supplied parent digests. Issue refuses invalid assembled
facts and refuses to overwrite an existing `lattice.policy_acceptance.fact`.
This operation does not select a checkpoint, judge policy quality, prove target
satisfaction, promote market readiness, approve deployment, or authorize live
capital. It only emits the sidecar that Lattice may later prove with
`policy_acceptance_contract_ready`.

Paper-online readiness sidecar emission is also Environment-owned and
evidence-only: `hero.environment.certify.paper_online_readiness
mode=check|issue` takes direct `policy_acceptance_job_dir`, `readiness_id`, and
`certification_evidence` arguments, loads an existing policy-acceptance sidecar,
derives the accepted policy/checkpoint/reward/execution/accounting identity from
parsed evidence, and assembles
`kikijyeba.lattice.paper_online_readiness.v1`. The fact must bind market-data
staleness, clock/timestamp, session lifecycle, durable paper-ledger recovery,
idempotency, duplicate-action/execution protection, direct-edge universe,
synthetic-market forbiddance, locked Cajtucu execution profile, reward/report
artifact path, operator-abort, and kill-switch policies. Issue writes
`lattice.paper_online_readiness.fact` only when the assembled contract is clean
and `expected_preview_digest` matches the canonical preview digest, and refuses
to overwrite
an existing sidecar. This operation does not start a
paper-online session, execute broker orders, claim market/deployment readiness,
or authorize live capital; it only emits evidence for
`paper_online_readiness_contract_ready`.

The readiness-grade schedule is `causal_walk_forward_training.v1`: each rollout
block must use a snapshot family whose artifacts declare `usable_from_key <=
block_cursor_begin`. Artifacts fitted on a block may be updated after that block
closes, but they cannot generate observations, beliefs, actions, rewards, or
policy-training trajectories inside that same block. The proof source is
`derived_from_artifact_fit_use_ledgers`; caller assertions are not enough.
Schedules must declare an explicit cursor-key ordering and reject opaque keys
without an ordering map. Target-label, reward, and trajectory availability must
be no later than the artifact `usable_from_key`. Full-window training is
quarantined as `offline_full_window_research`; it remains useful for diagnostics
but cannot satisfy policy-training readiness.
