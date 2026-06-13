#Cuwacunu Config

`src/config/.config` is the central fresh config file for migrated runtime
paths. Grammar files live under `src/config/grammar/*.bnf`; authored DSL
payloads live at the config root.

Current migrated sections:

- `UJCAMEI`: source registry, source retrieval channel, and source cursor DSL
  paths.
- `KIKIJYEBA`: protocol, topology graph, and replay environment DSL paths.
- `ACCOUNTING`: global accounting/reporting defaults shared across waves.
- `WIKIMYEI`: expression, representation, inference, observer, and policy DSL
  paths.
- `JKIMYEI`: training orchestration DSL paths.
- `HERO`: Config Hero, Runtime Hero, Lattice Hero, Marshal Hero policy paths,
  Runtime wave selection, and Lattice target/split paths.
- `GUI`: `cuwacunu_cmd`/`iinuji_cmd` terminal Shell Logs defaults plus image
  and animation asset paths.

The `[ACCOUNTING]` section currently defines
`accounting_numeraire_node_id`. This value names the ordinary graph node used
as the accounting/valuation/projection-reference numeraire across Runtime
waves and replay reports. It is intentionally not a Runtime wave setting: waves
change source ranges and job movement, while this default keeps multi-wave
reporting comparable. It does not create a separate cash node, policy output,
or execution route. Runtime replay, Marshal rollout, and direct
`cuwacunu_exec --replay-from-job-dir` use this value when no explicit debug or
recovery override is supplied.

The `[GUI]` section restores the migrated terminal surface contract. Shell Logs
defaults are controlled by `iinuji_logs_buffer_capacity`,
`iinuji_logs_show_date`, `iinuji_logs_show_thread`,
`iinuji_logs_show_metadata`, `iinuji_logs_metadata_filter`,
`iinuji_logs_show_color`, `iinuji_logs_auto_follow`, and
`iinuji_logs_mouse_capture`. Visual assets are controlled by
`iinuji_loading_logo_path` for the legacy hello bootstrap logo and static Home
fallback, `iinuji_closing_logo_path` for the bundled farewell splash logo, and
`iinuji_home_animation_path` for the animated F1 Home showcase. Relative GUI
asset paths resolve against the directory that contains the active `.config`;
invalid assets fall back to the bundled waajacamaya resources or text wordmark.

`hero.runtime.wave.dsl` is a Runtime-owned wave catalog. The active block is
selected by `[HERO].runtime_wave_id`, and each selected block names the active
protocol with `PROTOCOL`, the protocol-level focal runtime component with
`TARGET`, and the shared source cursor with `SOURCE_CURSOR_ID`.
Use `wave` for component execution profiles: representation and MDN train/run
jobs, plus graph-node allocation policy train profiles, that move across a
source range and mutate only their selected `TARGET` in train mode. Policy is a
component wave too, but its Runtime driver is contract-backed: the persisted
policy-training contract binds the completed replay/evidence source, causal
schedule, policy DSL/net/features/jkimyei identity, action distribution, reward
contract, execution profile, graph order, target-node universe, and
checkpoint/update evidence. Environment replay profiles remain
historical-world profiles: they drive reset/step/reward over a replay source
and call Cajtucu paper execution.
The default graph-first path is now the strict channel-preserving pair:
`wikimyei.representation.encoding.vicreg` and
`wikimyei.inference.expected_value.mdn`. The MDN path consumes the
`[B,N,C,De]` representation contract and freezes VICReg when the MDN target
trains. The active MDN uses a shared slot trunk, low-rank channel adapters, and
one shared feature-conditioned head to emit `[B,N,C,Df,K]` one-step mixtures.
The net file carries `FEATURE_EMBEDDING_DIM` and `CHANNEL_ADAPTER_RANK` as
architecture identity fields.
`CONTEXT_MODE=channel_context_strict` keeps the strict baseline;
`channel_context_plus_global` is reserved for the explicit
post-representation global branch and requires `GLOBAL_CONTEXT_DIM>0` in the
MDN net file. Channel-bearing config surfaces have their own BNF paths in
`.config`; keep those BNF files in sync with the VICReg/MDN DSL, net,
and Jkimyei keys. The active runtime uses channel-preserving VICReg and strict
channel-context MDN targets.

`wikimyei.observer.belief.dsl`,
`wikimyei.policy.portfolio.spot_distributional_utility.dsl`, and
`wikimyei.policy.portfolio.graph_node_allocation.{dsl,net,features.dsl,jkimyei}` extend the
graph-first contract after inference. The belief observer is deterministic: it
consumes MDN future NodeLift-potential distributions, resolves feature meaning
through the dock/kline registry, and declares that portfolio returns must be
numeraire-relative NodeLift projections rather than raw node potentials. The
portfolio policy consumes only post-projection `AllocationBelief`; it emits one
unified target-weight vector over ordered graph nodes. The accounting numeraire
is one of those graph nodes, resolved from
`[ACCOUNTING].accounting_numeraire_node_id` into `BasePolicy`, not an external
cash bucket, a separate policy output, or a routing hub.

The graph-node allocation policy config is the bounded PPO V0 policy component
contract. Its DSL
binds `kikijyeba.environment.policy_input.v1`,
`target_node_weights_simplex.v1`,
`masked_dirichlet_simplex.v1`,
`kikijyeba.environment.action.target_node_weights.v1`, and
`kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_drawdown.v1`; its `.net`
binds the V1 feature manifest, shared node/global/risk encoder shape,
mask-aware pooling, separate policy/value heads, `node_weight_logits` output
head, and default action distribution. The `.features.dsl` freezes actor-visible
node/global/risk feature names and evidence-only identity fields. V1 policy
input exposes compressed `AllocationBelief` distributional summaries, portfolio
weights, execution/cost state, masks, an accounting-numeraire node flag, and a
compact cross-node risk block;
raw observation anchor indexes and raw knowledge timestamps remain evidence
identity fields and are not actor-visible global features. It does not expose
raw MDN tensors or the full scenario bank by default. Its
`.jkimyei` is `policy_graph_node_allocation_ppo_v0`, uses `OPTIMIZER =
ppo_clip`, requires `causal_walk_forward_training.v1`, and keeps live capital
forbidden. Runtime supports the guarded `policy_kind=ppo_policy_adapter.v1`
trainer that writes actor/critic/optimizer, rollout/update, validation, and
Lattice-readable fact artifacts without policy-quality or market-readiness
authority. Without `report_path` or `replay_job_dir`, that Runtime path uses an
explicit smoke fallback; with `report_path`, it ingests an existing Kikijyeba
replay report into PPO rollout evidence; with `replay_job_dir`, Runtime
dispatches fresh on-policy replay collection through `cuwacunu_exec` and passes
an explicit actor checkpoint artifact into the graph-node allocation policy
forward path. Replay samples label whether policy-distribution log-probability
evidence was present. Replay reports produced by the graph-node allocation
policy component
now carry old policy-input/action-distribution evidence on each step; older
reports remain explicitly labeled as missing that evidence. With
`replay_job_dir`, Runtime can
dispatch `cuwacunu_exec --replay-from-job-dir` with graph-node allocation and
`on_policy_sample`, then ingest the generated report as
`collection_source=kikijyeba_on_policy_replay`. Runtime now writes bounded
post-update actor/critic checkpoints with learned graph-node logits and a value
estimate derived from PPO V0 rollout evidence. PPO policy-training requests must
bind policy DSL/net/features/jkimyei digests, target-node universe digest,
actor/critic architecture and checkpoint digests, optimizer state, PPO config,
rollout collection evidence, PPO update-report evidence, validation rollout
evidence, and PPO hyperparameters before they can satisfy policy-training
artifact readiness. Runtime folds those policy-source and environment bindings
into a `policy_operator_surface_digest`, which is the policy-training
component-spawn identity used by Runtime layout and Lattice exposure.
Policy acceptance remains a separate post-training Environment evidence
sidecar: `hero.environment.certify.policy_acceptance args_path=...` reads an
explicit certification request file, an
existing `runtime.policy_training.fact`, plus an existing
`lattice.tsodao_settings_protection.fact`, derives their fact digests from
parsed sidecars, requires the named
`policy_acceptance_governance_thresholds_v0.v1` acceptance-policy digest,
validates the assembled acceptance fact, and writes
`lattice.policy_acceptance.fact` only when issue mode includes a matching
`args_digest`. That
sidecar is for Lattice `policy_acceptance_contract_ready`; it is not a policy
selector, quality judge, market-readiness claim, deployment approval, or live
authority.
Paper-online readiness remains another explicit Environment evidence sidecar:
`hero.environment.certify.paper_online_readiness args_path=...` reads an
explicit certification request file and an
existing `lattice.policy_acceptance.fact`, derives the accepted policy identity
from the parsed sidecar, requires session lifecycle, clock/staleness,
idempotency, duplicate-protection, durable paper-ledger recovery, direct-edge
universe, locked Cajtucu execution-profile, reward/report artifact,
operator-abort, and kill-switch policies, and writes
`lattice.paper_online_readiness.fact` only when
issue mode includes a matching `args_digest`. That sidecar is for Lattice
`paper_online_readiness_contract_ready`; it does not start a paper-online
session, select a policy, approve deployment, route through a broker, or
authorize live capital.

The observer and deterministic policy DSLs currently require projection
validation and explicitly disable live capital; this keeps
`phi_asset - phi_reference` as a research bridge until it is checked against
realized tradable numeraire-denominated returns. These components are registered
as Wikimyei assemblies in the
graph-first bundle and therefore appear in the dock-binding/Lattice audit
surface with stable assembly fingerprints.

`kikijyeba.environment.replay.dsl` defines the first operating-world contract
around those observer/allocation outputs. It is deliberately an environment
contract, not a new allocator and not a replacement for Runtime: Runtime still
resolves waves and source ranges, while replay V1 binds historical-replay world
mode, consumes resolved Ujcamei/component-stream cursor identity, enforces the
time-`t` observation law, reveals the next realization only after simulated
action/execution, computes decomposed reward after ledger update, and emits
audit-only replay artifacts. Actions use
`kikijyeba.environment.action.target_node_weights.v1`: target weights over the
ordered graph-node action universe, including the accounting numeraire node.
Reports persist bundle/policy task
identity, requested/resolved parallelism, aggregate time-law/projection
counters, and `direct_edge_realized_return_truth_v1` evidence for realized
asset/numeraire returns. Runtime may write replay artifacts for MDN run jobs, but
that writer is best-effort sidecar evidence: failures are recorded in job state
and do not fail the MDN job. Replay V1 has no allocation, execution,
market-readiness, deployment, or live-capital authority.
Treat this as a `historical_replay_environment`, not as future
`experience replay` buffer storage. Paper-online and live-online modes are not
implemented by this DSL. Cajtucu is the output/execution pillar used by replay
for paper fills, ledger mutation, and execution traces; it is not the
environment driver and does not compute reward.

`cajtucu.execution.paper.dsl` defines the first Cajtucu paper execution
contract. It consumes graph-node target-weight execution intents, market
execution state, and an execution ledger; it emits direct node-pair paper
orders, fills or rejects, a mutated ledger, and an execution trace. The backend
matches overweight nodes to underweight nodes, sizes order notional from marked
ledger equity, rejects synthetic direct-pair markets by default, and returns
invalid traces for large equity mismatches or invalid sell-price cost models
before ledger mutation. Research replay must explicitly opt in when synthetic
markets are needed. The trace records market source identity and warns when
replay used synthetic direct node-pair edges or when intent equity disagrees
with mark-to-market ledger equity. Paper V1 has no credential surface, no
network I/O, no broker API, and no live-capital authority. It is the backend
that historical replay calls today and that a later paper-online environment
should reuse with live market data.
`MODE=run` executes the target dependency closure without optimizer steps.
`MODE=train` mutates only `TARGET`; upstream dependencies run frozen.
Wave source ranges are selected through protocol-neutral `SOURCE_CURSOR_ID`
entries in `src/config/ujcamei.source.cursor.dsl`. Cursors may use
`SOURCE_RANGE=all`, `SOURCE_RANGE=anchor_index`, or `SOURCE_RANGE=source_key` for
stable integral graph-anchor source keys such as kline millisecond close times.
Runtime resolves source-key cursors into anchor-index intervals before execution
and records both coordinates in manifests, states, reports, and component stream
`.lls` payloads.

MDN training is graph-slot centered and expects a frozen representation encoder.
`wikimyei.inference.expected_value.mdn.jkimyei` should normally provide
`INPUT_REPRESENTATION_CHECKPOINT`; `ALLOW_UNTRAINED_REPRESENTATION=true` is an
explicit smoke-mode escape hatch for running the pipeline before a VICReg
checkpoint exists. `INPUT_REPRESENTATION_CHECKPOINT` and
`INPUT_MDN_CHECKPOINT` are runtime model-state inputs. They are recorded and
proven through manifests/reports/exposure facts/checkpoint lineage, and must not
change the protocol contract fingerprint. `ALLOW_UNTRAINED_REPRESENTATION`
is runtime model-state admission policy and also does not change the protocol
contract fingerprint. Keep the repository default MDN
`.jkimyei` checkpoint fields empty; operational waves should receive concrete
checkpoint paths through Marshal-resolved Runtime handoffs or explicit
Runtime-local debug overlays. MDN run/evaluation also requires
`INPUT_MDN_CHECKPOINT`; the runtime rejects evaluation with a fresh untrained
channel-context MDN.

`wikimyei.representation.vicreg.net` owns both architecture and VICReg objective
settings. Loss weights, variance floor, loss epsilon, minimum valid rows,
non-finite loss policy, and augmentation probabilities must be declared there;
they are part of the channel graph-first protocol contract and checkpoint audit
surface.

`wikimyei.representation.mtf_jepa_mae_vicreg.dsl`,
`wikimyei.representation.mtf_jepa_mae_vicreg.net`, and
`wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei` define the separate
experimental MTF-JEPA-MAE-VICReg representation family. These files register
`wikimyei.representation.encoding.mtf_jepa_mae_vicreg` as its own component
identity and training surface. They do not replace the production VICReg/MDN
defaults and they do not make downstream forecast or MDN claims.

Protocol variants live under `kikijyeba.protocol.*.dsl` and are selected by
`[KIKIJYEBA].kikijyeba_protocol_dsl_path` in `.config`. The default protocol is
`cwu_02v`, which docks the MTF-JEPA-MAE-VICReg representation. `cwu_01v` remains
available as the VICReg baseline and declares `PROTOCOL_STATUS = legacy`,
`SUCCESSOR_PROTOCOL = cwu_02v`, and a warning recommending the newer protocol
for new training/evaluation runs. Wave files remain launch profiles for
target/mode/cursor/source-order choices; they should not be the authority for
which representation architecture is docked into a protocol.

Protocol variants also bind the active observer, deterministic allocation
policy, and graph-node allocation policy component:
`OBSERVER = wikimyei.observer.belief` and
`ALLOCATION_POLICY = wikimyei.policy.portfolio.spot_distributional_utility`,
plus `POLICY_COMPONENT = wikimyei.policy.portfolio.graph_node_allocation`.
The protocol owns this stack identity and its fingerprint; the individual
Wikimyei policy DSL owns SDU parameters, graph-node allocation policy
configuration, and Marshal/Environment rollout requests own baseline comparison
sets such as numeraire/current/equal-weight/SDU.

Wave profiles declare one exact `PROTOCOL`, and Runtime rejects a wave whose
`WAVE_SETTINGS.PROTOCOL` does not match the active protocol selected by
`[KIKIJYEBA].kikijyeba_protocol_dsl_path`. Broad wave targets such as
`wikimyei.representation.encoding`, `wikimyei.inference.expected_value`, and
`wikimyei.policy.portfolio` resolve through that protocol to concrete component
families.

Config Hero starts from:

- `[HERO].config_hero_dsl_path`
- `src/config/hero.config.dsl`
- `src/config/man/hero.config.man`

Runtime Hero starts from:

- `[HERO].runtime_hero_dsl_path`
- `[HERO].runtime_hero_profile`
- `src/config/hero.runtime.dsl`
- `src/config/man/hero.runtime.man`

`src/config/hero.runtime.dsl` is the single Runtime Hero policy surface. Its
default `operator_default` profile permits confirmed non-live wave/train
execution while keeping developer reset available only through the guarded
`hero.runtime.reset` path with explicit confirmation. Intentional long-running
training may select `runtime_hero_profile = long_train_operator` in an
operator-local `.config` or pass `--profile long_train_operator` to
`hero_runtime.mcp`. That profile keeps execute/train enabled with explicit
confirmation and a longer runtime budget, but keeps `allow_dev_nuke=false`.

Reusable wave profiles now live inside the single canonical wave catalog:

- `src/config/hero.runtime.wave.dsl`
- `src/config/ujcamei.source.cursor.dsl`
- checked-in wave ids include `train_core_vicreg`,
  `train_core_mtf_jepa_mae_vicreg`, `train_core_channel_mdn`,
  `cwu_02v_channel_validation_eval_mdn_1800_2050`,
  `cwu_01v_validation_eval_channel_mdn`,
  `cwu_02v_validation_eval_channel_mdn`, and `policy_training_pre_ppo_noop`

Operator-local `.config` files point `[HERO].runtime_wave_dsl_path` at this
same file and select the active block with `[HERO].runtime_wave_id`. The
checked-in `WAVE_SELECTION.ACTIVE_WAVE_ID` is only a fallback for direct file
decoding. Cursor ranges live in `ujcamei.source.cursor.dsl`; cursor ids
intentionally avoid protocol names because source availability is independent of
the active protocol. Runtime execution requests may still adjust the effective
launch range through an `execution_request_path` entry inside
`hero.runtime.run` `args_path`, or the equivalent
`cuwacunu_exec --source-range ...` flags, without mutating the wave or cursor
catalog. Effective ranges are not protocol identity.

Environment Hero starts from:

- `[HERO].environment_hero_dsl_path`
- `[HERO].environment_hero_profile`
- `src/config/hero.environment.dsl`
- `src/config/man/hero.environment.man`

Environment owns execution-environment admission evidence and rollout surfaces.
`hero.environment.certify.*` checks or issues policy-acceptance and paper-online
readiness prerequisite sidecars; `hero.environment.inspect.schema` reads the
Environment policy schema and `hero.environment.inspect.job` reads job-local
sidecars with direct job selectors;
`hero.environment.rollout` validates or replays bounded historical replay
rollouts while delegating low-level replay execution to Runtime.

Lattice Hero starts from:

- `[HERO].lattice_hero_dsl_path`
- `src/config/hero.lattice.dsl`
- `src/config/man/hero.lattice.man`

Marshal Hero starts from:

- `[HERO].marshal_hero_dsl_path`
- `src/config/hero.marshal.dsl`
- `src/config/man/hero.marshal.man`
- `src/include/hero/marshal_hero/hero_marshal.def`
- `src/include/hero/marshal_hero/hero_marshal.h`
- `src/include/hero/marshal_hero/hero_marshal_tools.h`

`src/config/hero.marshal.dsl` is intentionally minimal. It exists for Hero
policy-path symmetry and MCP harness consistency; it does not give Marshal
independent execution, scheduling, proof, model-selection, checkpoint-selection,
or config-editing authority. Marshal exposes a small deterministic coordination
surface over Lattice target state and Runtime policy/wave evidence, while
Runtime remains the executor and Lattice remains the proof authority.
`hero.marshal.inspect.facts` is the read-only path for Lattice
artifact-readiness targets and fact-family summaries. It can relay current
Lattice inspect panels
for concrete fact rows by digest, digest prefix, or fact index, but that preview
remains audit-only and cannot satisfy a target, select a checkpoint, or create
Marshal reachability. Preview rows include the typed Lattice catalog
`identity_envelope` so inspection tools can read fact identity, parent lineage,
support counters, and authority flags without family-specific parsing. Artifact
proof failures
now include `fact_preview_hint` metadata so operators can navigate from a
failed proof to the concrete catalog fact without changing proof or dispatch
authority. Current Marshal routing sends `target_class=artifact_readiness`
operators to inspect evidence instead of preparing Runtime handoffs, dispatch
validation, or dry-run previews.

The lattice target DSL is now profile/guard aware. `LATTICE_PROFILE` captures
reusable readiness defaults, `LATTICE_GUARD` remains available for low-level
forbidden exposure policy, and `LATTICE_TARGET` binds those pieces to a concrete
split or source range. Validation/test holdout protection should usually live in
`hero.lattice.splits.dsl` through `PROTECT_FROM_USES`; targets can opt into
that default with `PROTECT_SPLIT`. The preferred target spellings are
`SUBJECT_COMPONENT`, `OVER_SPLIT`, `WAVE_MODE`, and `PLAN_MAX_ATTEMPTS`;
compatibility aliases still load so older flat targets remain valid.
The DSL also accepts clause blocks keyed by `TARGET_ID`: `LATTICE_DEPENDS`,
`LATTICE_REQUIRES`, `LATTICE_FORBIDS`, `LATTICE_PLAN`, and `LATTICE_WARN`. In
v0 these clauses are proof-language syntax that lower into the existing target
evaluator fields; they do not execute waves and they do not make the lattice a
scheduler. `LATTICE_WARN` is explicitly non-blocking.
Artifact-readiness targets use `TARGET_CLASS=artifact_readiness`,
`SUBJECT_FACT_FAMILY`, and optional `PROOF_KIND` rather than expanding
`TARGET_KIND`. The default target catalog declares the first cwu_02v
validation-scope artifact proofs as `target_transform_contract_ready`,
`forecast_baseline_artifact_ready`, `forecast_eval_artifact_ready`,
`observer_belief_artifact_ready`, and `allocation_artifact_ready`. Policy gates
are still disabled:
`LATTICE_POLICY_GATE` currently accepts only `forecast_quality_acceptance` and
`allocation_acceptance` reservations with `ENABLED=false`, and each reservation
must bind to the matching artifact-readiness proof target. Reserved gates must
declare the future decision-policy inputs now: metric and baseline definitions,
threshold, uncertainty policy/model, support minimum, selector split,
anti-leakage policy, tie policy, negative tests, calibration requirements,
holdout declaration, and threshold-selection audit. Hero reports input-contract
completeness and missing-policy-input lists while keeping decision-policy
authority false. Policy, performance, market-readiness, and
deployment-readiness target classes are not active readiness proofs.
`replay_environment` now has the non-dispatchable
`replay_environment_artifact_ready` artifact-readiness target. Lattice derives
these facts from durable Runtime replay reports, binds Cajtucu paper-execution
and policy-set identity, and proves only artifact completeness, lineage,
time-law cleanliness, projection coverage, execution trace/cost evidence, and
authority denials.

Fresh Config Hero is intentionally a small policy-controlled config file
surface. Its public tools are `hero.config.status`,
`hero.config.inspect.*`, and `hero.config.apply.*`. It can inspect, read, and
execute guarded writes/deletes for managed config files under configured roots;
mutating tools run an internal preflight before changing state. It does not own
domain-specific DSL validation.
Validation for Ujcamei, Kikijyeba, Wikimyei, and Jkimyei remains with those
domain modules. `hero.config.inspect.bundle` is the read-only provenance tool
over the same global config: it records every `_path`/`_filename` entry with
canonical path, content digest, stable `config_bundle_id`, and per-capture
`config_receipt_id`.

Agent-facing Config Hero calls should prefer `hero.config.inspect.map` for
global path discovery, `hero.config.inspect.bundle` when runtime spawn evidence
needs a config receipt, and `hero.config.inspect.resolve_path` for path
preflight before reading or mutating. Reads use `hero.config.inspect.file_read`
and return `sha256`; replacements and deletions go through
`hero.config.apply.write` or `hero.config.apply.delete` with direct arguments
and require `expected_sha256` by default for
existing targets.
Config inspect tools expose their small selectors directly. Config apply tools
also expose their operation-specific arguments directly; there is no Config
Apply `mode`, `args_path`, or `args_digest`.

Runtime Hero is the agent-facing control and inspection surface for
`/cuwacunu/.build/exec/cuwacunu_exec`. It decodes the active wave, performs
guarded dry-runs/executions, and reads runtime artifacts under the canonical
execution root `/cuwacunu/.runtime/cuwacunu_exec`. The parent
`/cuwacunu/.runtime` is disposable runtime state; new runtime subtrees should
live under the canonical execution root unless a policy explicitly introduces a
separate owner. Runtime manifests record config
provenance and component spawn links: `config_bundle_id`, `config_receipt_id`,
`component_spawn_registry_id`, `component_family_id`,
`component_operator_surface_digest` for representation/MDN component waves,
`component_spawn_fingerprint`, scoped `component_spawn_id`, and
`component_spawn_label`. Component spawn identity is protocol/component scoped;
wave source cursors and concrete source ranges remain job, checkpoint, and
Lattice evidence lineage. Default runtime launches now split `job_stable_id`
from immutable `job_attempt_id`: rerunning the same wave against the same
component spawn writes a fresh attempt directory instead of replacing the prior
report/checkpoint. Explicit `--job-dir` launches are guarded by a no-overwrite
check unless resume support is implemented.

Developer runtime reset is owned by Runtime Hero as `hero.runtime.reset`, not by
Config Hero. `mode=plan` reports the exact runtime-root entries it
would clear. `mode=execute` requires `allow_dev_nuke=true` plus a
digest-pinned `args_path` reset request, where per-call selectors such as
`runtime_root` and `backup` live. Checked-in policies allow clearing
`/cuwacunu/.runtime` but keep backup snapshots disabled by default so a reset
does not leave legacy backup folders under the disposable runtime tree.
Operators can explicitly enable backups to the configured `/tmp` backup root.

`hero.lattice.targets.dsl` sits one level above waves and is pointed to by
`[HERO].lattice_targets_dsl_path`. It declares read-only readiness targets over
contract-scoped runtime evidence and may recommend the next wave, but it does
not execute anything. `hero.lattice.splits.dsl` is pointed to by
`[HERO].lattice_splits_dsl_path` and defines named graph-anchor cursor ranges such as
`train_core`, `validation_holdout`, and `test_holdout`. Targets can refer to
those names with `TRAIN_SPLIT` / `OVER_SPLIT`, and validation/test splits can
declare default `PROTECT_FROM_USES` so targets can simply use `PROTECT_SPLIT`.
V1 split ranges are graph-anchor index intervals; purge/embargo fields are
enforced for split protection: `auto_from_Hx` expands the protected footprint
left by the observed context window, and `auto_from_Hf` expands it right by the
future target window. The evaluator derives those windows from runtime exposure
facts, preferring `source_input_length` / `source_future_length` when present
and otherwise falling back to observed/target source-row footprints. Leakage
checks still use observed/target source-row footprints from runtime exposure
facts. The split policy has a deterministic fingerprint. It appears in Lattice
Hero evaluation output, and exposure facts that record a split-policy
fingerprint are rejected when evaluated under a different split policy.
Concrete named-split exposure facts that omit the split-policy fingerprint are
also rejected during evaluation filtering when a split policy is active. Proof
certificate self-check applies the same split-policy identity rule to concrete
named-split facts and causal exposures while the proof is split-policy-bound.
Target evaluation requires the active contract/component fingerprints when a
target asks for identity matching. Anchor-index targets and exposure checks also
require the active graph-order fingerprint and source cursor token because
anchor indices are only comparable inside that identity. It scans compatible
runtime jobs newest-first, accepts the newest satisfying evidence, and uses
`MAX_WAVES` only as a guard on further wave recommendations for active
non-dry-run train attempts; stale contract jobs, wrong graph/cursor jobs, and
missing job.state files do not consume that budget.
Lattice target v0 can evaluate normal job artifacts; it does not require every
wave to run with `MODE=...|debug`. Debug `.lls` facts are intended as richer
lineage evidence for cursor coverage, exposure accounting, and leakage checks.
The new lattice exposure ledger normalizes those normal job artifacts into
cursor-indexed facts. Facts are a subset of runtime evidence, and not every
evidence artifact is a `.lls` file. New runtime manifests emit
`graph_anchor_row_index_v1`
source footprints: observed input covers
`[anchor_begin - (Hx - 1), anchor_end)`, while future supervision covers
`[anchor_begin + 1, anchor_end + Hf)`. Older artifacts without those fields
still fall back to `anchor_range_v0`. When the graph-anchor cursor has a regular
reference key step, manifests also emit `graph_anchor_key_window_v1` key
windows. Those key windows are for source-key/timestamp audit; v0 exposure
coverage and forbidden-overlap checks still use row-index intervals. Lattice
Hero exposes a structured `source_key_window_audit` preview so agents can inspect
whether the auxiliary row-to-source-key map is complete, numeric, monotone,
order-preserving, and affine-consistent with the inferred regular key step
without making it the leakage authority. The audit also counts missing endpoint
pairs, irregular anchor steps, and row/source-key mismatches as explicit gap
warnings. `hero.lattice.inspect.exposure` exposes `source_key_map_audit_summary`,
which binds available audits to graph-order identity, source-cursor identity,
and source-receipt parent facts while preserving row-index authority. Manifests
also include
compact active source-file receipts so runtime evidence can be traced back to the
concrete Ujcamei source files without inspecting the original DSL. Lattice Hero
reports `source_receipt_policy_vocabulary` to make that boundary explicit:
compact receipts are audit metadata, not coverage/leakage proof authority,
contract identity inputs, or replacements for row-index footprints. Structured
source receipt facts remain future index/audit work.
`source_receipt_policy_summary` self-checks that boundary with one row-index
coverage/leakage authority row, three audit-only receipt rows, no contract
identity authority, no structured receipt facts in V1, and declared compact and
future structured receipt fields.
Manifests, job state, component reports, and exposure facts also carry
graph-anchor domain health: candidate anchors, accepted anchors/fraction,
skipped anchors by reason, duplicate anchors, common/reference key bounds, and
a warning level. This does not change graph-first training semantics. Ujcamei
still restricts waves to the accepted common graph-anchor cursor domain; the new
fields make source-range clipping visible through Runtime/Lattice Hero.
`WAVE_SETTINGS.SOURCE_ORDER` controls only the yield order inside that resolved
graph-anchor domain. If omitted, `MODE=train` defaults to `random_per_epoch`
and `MODE=run` defaults to `sequential`. `sequential` preserves canonical
accepted-anchor order; `random_per_epoch` uses a Torch `RandomSampler` over
graph-anchor indices per training epoch while each selected anchor still fetches
every graph edge together. The random order is seeded from the target
component's training `SEED` (`vicreg_training.seed` or
`channel_mdn_training.seed`) so graph-anchor sampling is reproducible by config.
An explicit train-wave `SOURCE_ORDER=sequential` is allowed for
reproducible/debug runs, but Runtime reports it as a warning because stochastic
graph-anchor train loading is disabled.
Targets may now require exposure coverage over explicit anchor-index ranges or
forbid checkpoint exposure overlap with protected ranges. Those checks are
evaluated from the checkpoint exposure closure and fail as `exposure_failed`
when coverage is too low or forbidden exposure is found. The target evaluator can
auto-build this ledger from runtime job directories, so normal wave artifacts are
enough for v0 readiness checks. Coverage and leakage use different coordinates:
readiness coverage is measured over target-component-local graph-anchor coverage
intervals, while forbidden exposure checks use full-closure observed/target
source-row footprints. Checkpoint closure is fail-closed when an input checkpoint
has no producer fact, and channel MDN exposure readiness verifies that the exact
loaded representation checkpoint has a compatible VICReg producer fact. A
completed train job only counts as mutating a component when it records
optimizer effort and the manifest explicitly lists the component in
`mutated_components`.
For exposure-backed targets, evaluation also returns exposure-load summaries.
Unique coverage measures the union of completed anchors; `cursor_exposure_load`
counts repeats. This lets the lattice report, for example, "one cursor epoch of
coverage but two cursor epochs of exposure" without marking the target failed.
`LATTICE_WARN` is the explicit non-failing warning layer over those summaries.
It can flag high exposure load, high optimizer effort density, or source-domain
health issues while leaving target status, planning, and max-attempt accounting
unchanged. Warning results name their `evidence_basis` so agents can distinguish
exposure-load summaries, filtered node-support summaries,
representation-health facts, and anchor-domain facts. Availability booleans mark
which summary envelope is a real warning basis and which is only an empty
placeholder. Every warning kind may use `SPLIT` or explicit
`ANCHOR_INDEX_BEGIN`/`ANCHOR_INDEX_END`; warning measurements are scoped to that
anchor interval, defaulting to the target range. Warning interval overlap may
use anchor-range visibility when trusted completed coverage is unavailable; that
does not make untrusted coverage satisfy readiness. Hero JSON exposes the
resolved interval as `warning_results[].warning_anchor_range`;
`hero.lattice.inspect.target` exposes the same pre-evaluation scope as
`warning_scope_previews[].resolved_warning_anchor_range`; and
`warning_anchor_scope_policy_vocabulary` describes the visibility-only fallback
rules for both surfaces. Repeated exposure is therefore visible without being
treated as an implicit readiness failure.
Targets can use `CHECKPOINT_SOURCE = latest_satisfying:<target_id>` to inspect
the checkpoint proven by another satisfied target. This is how read-only guards
such as `channel_mdn_train_core_no_validation_leakage` check validation leakage
on the current train-core MDN checkpoint without creating a fake training
target. `channel_mdn_train_core_no_test_leakage` then applies the same
split-backed guard to `test_holdout`, so the operational chain has both
validation and test holdout cleanliness before validation evaluation. Hero JSON includes
`checkpoint_selection_policy_vocabulary`: `latest_satisfying` is a deterministic
readiness selector over the referenced satisfied target, not a best-model,
Pareto, performance, or deployment selector.
`checkpoint_selection_policy_summary` self-checks the same boundary: 4 selector
policy rows, 3 deterministic readiness-selection rows, 2 exact runtime binding
rows, and zero performance, Pareto, contract-identity, or runtime-executor
authority.
If the source target is not satisfied, the guard blocks and reports the source
target's suggested wave.
Validation evaluation targets use `MIN_EVALUATION_METRIC_COVERAGE` /
`USE=evaluation_metric` over the validation split. These targets expect
run-mode evidence, allow zero optimizer steps and no new checkpoint, and depend
on the holdout-clean guard for the training checkpoint they evaluate.
For MDN reports with node-support fields, the lattice scanner also derives
read-only `node_exposure` facts from `node_ids`, per-node
routed/active/trained/evaluated row counts, `valid_target_count_by_node`, and
`mean_nll_by_node`. These facts are per-node MDN support evidence linked to the
parent exposure fact. Derived
node-support summaries include support-row/unique-node counts, exposure-use and
mutation incidence counts, and aggregate/weakest-node Wilson support bounds for
statistical visibility. Mutating node-support summaries must be backed by the
checkpoint closure's causal MDN exposure, while non-mutating validation/evaluation
node support is row-parent exposure evidence tied to the exact evaluated
checkpoint dependency. Hero JSON includes `valid_target_uncertainty_vocabulary`
for the Wilson method, field bindings, and no-trials policy.
`valid_target_uncertainty_summary` self-checks the three support scopes,
Wilson-95/binomial method, success/opportunity and interval field bindings,
non-claiming no-trials policy, and support-visibility-only boundary. Node-support
warnings filter those support rows by
`USE` and `EFFECT`, defaulting to target-supervision rows that mutated the MDN
component. When a node-support warning declares `SPLIT` or explicit anchor
bounds, it derives a warning-range node matrix instead of reusing only the
target-wide certificate matrix. These warnings do not claim per-node VICReg
readiness because VICReg is a shared encoder. Hero JSON includes
`node_support_scope_policy_vocabulary` to make that scope boundary explicit: V1
MDN node-support rows are MDN-head support visibility, future VICReg node-local
support must come from NodeLift or representation-emitted support facts, and
synthetic backfill from MDN rows is not allowed.
`node_support_scope_policy_summary` self-checks the same boundary: four policy
rows, three available V1 visibility rows, one deferred representation-node row,
no VICReg per-node readiness authority, no synthetic backfill, and no runtime
executor authority.
`LATTICE_WARN KIND=mdn_distribution_calibration` adds the first non-blocking
distributional MDN warning surface. V3-D evaluates aggregate `mean_nll`,
runtime-emitted channel, target-feature, channel/target-feature, and per-node
max-NLL summaries as warning-only diagnostics. PIT KS statistic, predictive
interval coverage error, tail coverage error, and calibration slope error remain
future metrics until runtime emits samples and uncertainty. Hero JSON includes
`mdn_distribution_calibration_vocabulary` and
`mdn_distribution_calibration_summary` for the warning-only metric policy, plus
`mdn_distribution_calibration_diagnostic_vocabulary` and
`mdn_distribution_calibration_diagnostic_summary` to self-check exact checkpoint
binding, representation checkpoint binding, validation split, active identity,
sample count, uncertainty method, warning-only effect, and zero performance-gate
authority.
`representation_geometry_vocabulary` names the VICReg representation-health
warning metrics, including loss components, gradient norms, projection support,
finite-parameter checks, and embedding-spectrum geometry. These remain V1
non-blocking warning visibility, not performance or geometry gates.
`representation_geometry_summary` self-checks the 18-row boundary: all metrics
are V1-visible `representation_health` warnings, seven are embedding-geometry
metrics, eleven are future hard-gate candidates, and none grant active
performance-gate authority.
`representation_geometry_gate_review_summary` reviews observed VICReg geometry
distributions and records that no default threshold has been promoted. Hard
geometry checks require explicit `LATTICE_REQUIRES KIND=representation_geometry`
syntax, and missing geometry facts fail that opt-in gate closed.
`performance_uncertainty_policy_vocabulary` records the statistical boundary:
V1 support intervals are visibility, and future performance gates must compare
conservative confidence bounds with an explicit uncertainty method and
selection-leakage policy instead of raw point estimates.
`performance_uncertainty_policy_summary` self-checks that six-row boundary: one
V1 support-visibility row, five deferred future performance-gate rows, no V1
performance-gate authority, no point-estimate gates, confidence bounds required,
and selection-leakage policy required.
`validation_performance_evidence_policy_vocabulary` is the V3-C validation
performance evidence contract: it names metric family, split field, checkpoint
identity fields, exact evaluated-checkpoint binding, sample-count field,
point-estimate field, uncertainty method, confidence-bound fields, target
syntax, selection-leakage policy, and fail-closed cases for each finite row.
`validation_performance_evidence_policy_summary` self-checks that V3-C does not
grant performance-gate authority, available point estimates are bounded or
warning-only, mean NLL stays warning-only until uncertainty is emitted, exact
checkpoint binding and active identity are required, and missing uncertainty,
wrong checkpoint binding, stale identity, and selector leakage fail closed.
Hero JSON also includes `validation_performance_scope_policy_vocabulary`:
validation eval readiness proves clean run-mode evaluation, exact checkpoint
binding, and trusted evaluation coverage, but it does not claim model quality,
deployability, or performance approval. Future performance gates require
explicit metrics, uncertainty, and selection-leakage policy.
`validation_performance_scope_policy_summary` self-checks that boundary: three
V1 validation-readiness/visibility rows, one deferred future performance row, no
V1 performance-gate authority, uncertainty required for future gates, and no
runtime executor authority.

Runtime now writes canonical `lattice.exposure.fact` and
`lattice.source_analytics.fact` sidecars after terminal jobs, plus a
`lattice.checkpoint.fact` sidecar when a checkpoint exists. The source-analytics
sidecar records source-range health and anchor acceptance visibility only and
can reuse job-local source data analytics `.lls` metrics when they already
exist; it does not grant readiness, coverage, leakage, or contract-identity
authority.
The exposure scanner prefers exposure sidecars, ingests source-analytics and
checkpoint fact sidecars, and falls back to manifest/state/report derivation
for older jobs. Exposure facts include `coverage_precision`: readiness coverage
is credited only from `contiguous_completed_range_v1` facts, while requested
ranges that cannot prove completion are treated as
`requested_range_untrusted_v0`.
For non-mutating channel-MDN run jobs, Runtime also emits forecast evidence
sidecars for target transform, deterministic baseline references, and forecast
evaluation. The baseline family is emitted as separate previous-value,
zero-return, moving-average, and last-valid-channel catalog facts. The
forecast-eval fact carries aggregate and stratified NLL/support surfaces,
including horizon-level support, as visibility evidence. Lattice derives
skill-versus-baseline summaries only when the referenced baseline fact resolves
under the same identity. Forecast-baseline summaries also expose distinct
baseline-kind coverage for previous-value, zero-return, moving-average, and
last-valid-channel references. These remain warning/summary diagnostics, not a
performance gate or checkpoint selector.

Lattice Hero is the read-only agent surface for these checks. It provides
`hero.lattice.status`, `hero.lattice.inspect.*`, `hero.lattice.evaluate.*`, and
`hero.lattice.compare`. Inspect tools report compiled target proof objects,
fingerprints, derived query vocabulary, proof obligation vocabulary, proof digest
policy, checkpoint selection policy, plan-advice policy, contract identity
boundary, mathematical readiness crosswalk, operational scope/gate crosswalks,
static mathematical policy vocabularies, deficit priority and evidence-order
vocabularies, and numeric dimension vocabulary without scanning runtime
evidence; evaluation and planning then read the in-memory exposure ledger built
from runtime files. A
future lattice DB should be a
rebuildable query/index cache over immutable runtime/fact files, not the primary
writer of evidence.
Hero MCP schema hygiene is separate from lattice proof authority. Sourced Hero
catalog generation validates each tool `inputSchema` as a harness-safety gate:
the root must be `type=object`, and top-level `oneOf`, `anyOf`, `allOf`,
`enum`, and `not` are rejected with a message naming the tool, schema path, and
offending construct. The schema smoke covers Config, Runtime, Lattice, and
Marshal catalogs and specifically protects the prior Lattice closure schema
failure mode.
Target compilation applies small dimensional checks: fractions stay in `[0,1]`,
counts are non-negative integral thresholds, and representation condition-number
thresholds must be at least `1`. Representation-health warnings also reject
inverted one-sided directions, so high-bad metrics use `ABOVE` and low-bad
metrics use `BELOW`. Anchor-domain warning clauses carry exactly one metric
threshold.
Opt-in representation-geometry gates use the same units through `VALUE`, with
low-bad metrics using `OP=ge` and high-bad metrics using `OP=le`.
Evaluations expose proof certificates, deficits, plan basis, and an
`evidence_order_vector` that keeps Pareto evidence dimensions separate instead
of emitting a single score. The vector separates total warning results,
triggered warning count, unavailable warning-measurement count, source-key audit
counts/issues/affine mismatches, unresolved lineage count, selector-leakage
participation state, node-support imbalance maxima, and the compatibility
`warning_count` alias for triggered warnings. `hero.lattice.compare`
evaluates two targets from one scan and reports the vectors, per-dimension
dominance reasons, and the relation `left_dominates`, `right_dominates`,
`equivalent`, `incomparable`, `selector_leaked`, or `unavailable`; it emits no
scalar score and makes no deployment decision. Hero JSON includes
`exposure_measure_algebra_vocabulary` so clients can distinguish idempotent
unique coverage from additive repeated load.
`exposure_measure_algebra_summary` self-checks that boundary with exactly two
measures, one idempotent coverage measure, one additive load measure, no
dual-role measure, and distinct coverage-fraction versus cursor-epoch units.
It also includes `source_key_coordinate_policy_vocabulary` so clients can see
that row-index intervals are the coverage/leakage authority and source-key
windows are audit-only order-preserving/affine/gap map checks.
`source_key_coordinate_policy_summary` self-checks the coordinate boundary:
exactly one row-index coverage/leakage authority row, four audit-only
source-key rows, declared order-preserving/affine/gap fields, and no source-key
audit row with coverage or leakage authority.
It also includes `leakage_rule_vocabulary` so clients can identify protected
split dilation as the leakage predicate. `leakage_rule_summary` self-checks that
the V1 leakage rule is a single protected-split temporal dilation with empty
forbidden-use intersection over complete checkpoint closure and labeled
causal-exposure witnesses. Hero JSON also includes `causal_edge_vocabulary` so
clients can render closure as a labeled causal graph. `causal_edge_summary`
self-checks the seven V1 edge labels, four source-row use edges, two checkpoint
reachability edges, one mutation edge, and the separation between checkpoint
reachability and direct forbidden-use leakage edges. Hero JSON also includes
`selection_signal_policy_vocabulary` so clients can see that V1 forbids known
selection-signal causal exposure without claiming first-class selector event
provenance. `selection_signal_policy_summary` self-checks the four-row
boundary: known selection-signal is forbidden and causal-anchor based,
first-class selector event streams and arbitrary selector-path proofs remain
future/non-claiming, all rows require closure completeness, and Lattice has no
executor authority. Hero JSON also includes `derived_query_rule_vocabulary` so
clients can name the
read-only Datalog-style relations behind identity,
dependency, aggregate dependency, coverage, aggregate coverage, closure,
checkpoint ancestry, unresolved lineage, guarded closure-cleanliness, leakage,
guarded no-overlap, warning, cache-staleness, planning, certificate-check,
blocking-deficit, and status/satisfaction proofs. Rule rows also declare
`projection_scope`, `projection_quantifier`, and
`empty_projection_policy`, making target-wide, per-row, compact aggregate,
all-row, any-witness, none-exists, and zero-row relation shapes discoverable
before evaluation. They also declare the emitted result shape with
`result_projection_scope`, `result_projection_quantifier`, and
`result_empty_projection_policy`, so Hero can compare live result rows against the
vocabulary instead of a private hardcoded shape table.
`derived_query_rule_vocabulary_digest_schema` and
`derived_query_rule_vocabulary_digest` bind that declared rule vocabulary so
clients can compare explain/evaluate/plan surfaces without recomputing the
canonical text. `derived_query_projection_semantics_vocabulary` defines the
allowed projection-scope, quantifier, empty-policy, and compatibility-alias
values used by rule vocabulary and live result rows.
`evaluate_target` and `plan_target` also expose `derived_query_results`, a
read-only projection of those relation truth values from the existing proof
result; it is query metadata only and does not affect status, evidence
authority, or certificate digests. The envelope declares
`schema=kikijyeba.lattice.derived_query_results.v1`,
`rule_vocabulary_digest_schema`, and `result_projection_digest_schema`, and
includes `projection_target_id`, `projection_target_spec_fingerprint`,
`projection_certificate_digest`, `projection_status`,
`projection_basis_complete`, `projection_basis_field_count`,
`result_projection_digest_rule_vocabulary_bound`,
`result_projection_digest_basis_field_count`, `rule_vocabulary_digest`,
`result_projection_digest`, `result_count`, `known_count`, `unknown_count`,
`known_true_count`, `known_false_count`, `aggregate_projection_count`, and
`single_projection_count` for whole-set sanity checks. The result projection
digest binds the rule-vocabulary schema and digest plus target id, target spec
fingerprint, certificate digest, status, and emitted rows; the digest basis
field count is `6`. The basis check requires target id, target spec fingerprint,
certificate digest, and status before the projection can self-check cleanly.
It names
`summary_source=emitted_result_rows` and reports `summary_self_check_passed`,
`summary_issue_count`, and `summary_issues` for count/quantifier, projection
scope, declared rule/result projection-shape compatibility, empty-policy
compatibility, vocabulary-coverage, basis, and projection-semantics consistency.
It
also reports `rule_vocabulary_relation_count`, `result_covers_rule_vocabulary`,
`unique_result_relation_count`, `missing_rule_relation_count`,
`extra_result_relation_count`, `duplicate_result_relation_count`, plus
`missing_rule_relations`, `extra_result_relations`, and
`duplicate_result_relations`, so agents can detect stale, extra, missing, or
non-unique projections relative to the declared rule vocabulary.
Live result rows include `rule_projection_scope` and
`rule_projection_quantifier` for the declared relation shape, plus
`result_projection_scope` and `result_projection_quantifier` for the emitted
compact projection. `projection_scope` and `projection_quantifier` remain
compatibility aliases for the result fields, so compact booleans such as
dependency, coverage, warning, and plan-deficit projections state whether they
summarize all rows or any witness. Compact rows also expose
`rule_empty_projection_policy`, `result_empty_projection_policy`,
`empty_projection_policy`, `projection_row_source`, `projection_row_count`,
`projection_true_count`, and `projection_false_count`, making empty, all-pass,
partial, and any-witness summaries auditable without parsing messages.
`db_cache_policy_vocabulary`
keeps the future DB boundary explicit:
runtime files/facts are source of truth, cache rows are rebuildable read models,
cached plans are advice rather than evidence, checkpoint ids/digests are cache
previews until v2 identity promotion, and Runtime Hero remains the only
executor. `evidence_abstraction_vocabulary` names the concrete runtime/fact
inputs, abstract proof outputs, soundness condition, conservative policy, and
join semantics for identity, coverage/load, closure, leakage, node support,
source-receipt audit, warnings, planning, and evidence order; a future DB/cache
must preserve those rules rather than becoming evidence authority.
`product_evidence_state_vocabulary` names the product proof factors, including
identity, coverage lattice, exposure-load monoid, closure graph, leakage
predicate, metric/warning vector, node-support matrix, source-receipt audit set,
and deficit vector, with their partial orders and identity-scoped join rules.
Lattice inspect modes include read-only audit queries over rebuildable index
rows. They filter by relation/key/digest and report whether a stored cache
answer matches a fresh live scan; invalid or mismatched cache rows fall back to
live scan and do not affect target satisfaction. A separate
`validation_strength=header_only`, `allow_unproven_cache=true`,
`compare_live_scan=false` mode gives agents a fast read-only cache query while
marking the answer unproven and still non-authoritative for target readiness.
Multi-target readiness checks and cache files use `watched_file_metadata_digest`,
`row_set_digest`, and `relation_counts`; `watched_file_manifest` is a bounded
freshness mode, while `header_only` is the explicit unproven fast lane.
The runtime metadata digests are metadata freshness checks over path, size, and
mtime records. They are not content digests and do not make cache rows proof
authority for target satisfaction.
Derived-query inspect/evaluate panels remain read-only witness surfaces for a
finite rule set: `target_satisfied`, `checkpoint_ancestor`,
`forbidden_overlap`, `stale_cache`, and `unresolved_lineage`. Responses include
the rule row, result source, concrete witnesses, and fail-closed flags; cache
rows remain non-authoritative for target satisfaction.
`join_law_vocabulary` makes those joins cache-safe and machine-readable:
coverage/closure/leakage/warnings/deficits join idempotently inside one
identity scope, repeated exposure load and distinct node-support rows are
additive, and unsafe joins fail closed instead of claiming readiness.
`mdn_distribution_calibration_vocabulary` names warning-only aggregate,
channel, target-feature, channel/target-feature, per-node, and future MDN
calibration metrics.
`mdn_distribution_calibration_summary` keeps the warning-only metric policy
visible.
`mdn_distribution_calibration_diagnostic_vocabulary` and
`mdn_distribution_calibration_diagnostic_summary` self-check the V3-D
diagnostic binding fields: exact MDN checkpoint, representation checkpoint,
validation split, active identity, sample count, uncertainty method, and
warning-only/non-performance effect. `representation_geometry_vocabulary` names
the V1 VICReg health and geometry warning metrics plus their bad-direction
semantics.
`representation_geometry_summary` checks that those metrics remain non-blocking
representation-health visibility rather than performance or geometry gates.
`representation_geometry_gate_review_summary` records observed distributions,
no promoted default thresholds, opt-in target syntax, and fail-closed missing
geometry behavior.
`evidence_retention_policy_vocabulary`,
`evidence_retention_audit_scenario_vocabulary`, and
`evidence_retention_policy_summary` define the V3-K retention boundary:
runtime reports, sidecars, checkpoint material, and selection-signal evidence
remain replay authority; proof certificates, PASS files, compact receipts, and
cache rows remain non-authoritative audit/read-model metadata. Archive manifests
must bind active identity, split policy, source cursor, graph order, and
checkpoint identity, and pruning must warn or refuse unresolved lineage.
`benchmark_regression_budget_vocabulary` and
`benchmark_regression_budget_summary` define the V3-L performance budget:
benchmark rows are finite, split library-function, long-lived MCP, and direct
CLI timing layers, and label proof modes as `header_only`,
`watched_file_manifest`, `full_runtime_metadata_digest`, `live_scan`, or
`live_parity`. Header-only fast audit rows forbid live scans and metadata
digests; proof/parity rows may be slower, but that cost is named. Cache rows
remain non-authoritative for target satisfaction in every benchmark mode.
`performance_uncertainty_policy_vocabulary` names the conservative-bound rule
for future support, loss, calibration, PIT, and node-stratified performance
gates.
`performance_uncertainty_policy_summary` checks that the V1 row is support
visibility only and every future gate row requires confidence bounds and
selection-leakage policy while disallowing point-estimate gates.
`mathematical_readiness_v1_vocabulary` is the compact audit crosswalk: it maps
the V1 mathematical claims to the Hero fields that prove or expose them, while
preserving the read-only/no-performance/no-DB-source-of-truth boundary.
`mathematical_readiness_v1_summary` is the machine-checkable companion: it counts
the 16 V1 mathematical items and reports that all are included, read-only,
non-executing, non-performance, non-DB-source-of-truth, and backed by at least
one Hero surface.
`validation_performance_scope_policy_vocabulary` records that V1
validation evaluation is readiness evidence, not a performance gate.
`validation_performance_scope_policy_summary` checks that future validation
performance authority stays deferred until explicit metrics, uncertainty, and
selection-leakage policy exist.
`target_numeric_dimension_vocabulary`
exposes the unit/bounds table used by the target compiler for numeric target and
warning fields, including integrality and threshold direction.
`target_numeric_dimension_summary` self-checks that the table has V1's expected
rows, unique context/field pairs, declared units and numeric kinds, well-formed
bounds, known threshold directions, and separate coverage/load units.
`monotonicity_invariant_vocabulary` names the clean-growth assumptions behind
target satisfaction, coverage union, additive load, leakage cleanliness,
deficits, warnings, Pareto order, and visibility-only dimensions.
`checkpoint_selection_policy_vocabulary` keeps checkpoint-source resolution
separate from that Pareto order: V1 `latest_satisfying` does not optimize or
rank model quality.
`checkpoint_selection_policy_summary` makes that separation self-checking by
counting the selection policy rows and proving no row is a performance selector,
Pareto optimizer, contract identity authority, or runtime executor.
It also includes `proof_obligation_vocabulary`, which maps certificate fields to derived
relations, required scopes, status effects, deficit kinds, non-claiming
conditions, digest participation, and planner relevance.
`proof_certificate_digest_policy_vocabulary` makes the digest boundary explicit:
the required canonical certificate digest binds proof content such as
target/split identity, identity matches, dependency checkpoint bindings,
artifact proofs, coverage/load, closure, leakage, checkpoint-preview audit
fields, and MDN node support, while warnings, deficits, plan advice,
evidence-order projections, policy vocabularies, and the digest field itself
stay outside the digest.
`proof_certificate_digest_policy_summary` self-checks that boundary by counting
the 12 digest-policy rows, 9 hashed proof surfaces, 3 excluded report/policy
surfaces, zero advisory digest overlap, and zero visibility-only status
authority.
`checkpoint_identity_policy_vocabulary` states that V1 closure authority is
path/exposure-fact based, checkpoint ids/file digests are audit previews, exact
validation bindings use runtime-loaded checkpoint paths, and id/digest promotion
is future DB/cache work rather than protocol contract identity.
`contract_identity_boundary_vocabulary` separates protocol/graph/component and
target proof identity from runtime-loaded checkpoint paths, advisory plan
inputs, warning visibility, and audit-only source receipt/key-window metadata.
`contract_identity_boundary_summary` self-checks that runtime model-state,
planning advice, warning visibility, and audit metadata have zero overlap with
protocol contract or target proof identity while the active identity surfaces are
present.
Hero JSON also includes
`dimension_vocabulary` and relation
vocabulary metadata from the same C++ source used by the comparator; comparison
uses `triggered_warning_count` while `warning_count` remains compatibility
metadata. Warning results carry a structured `measurement_available`
bit; unavailable-warning counts come from that bit rather than diagnostic message
wording.
Warning results also carry `threshold_direction`, so clients can distinguish
above-threshold and below-threshold warnings without parsing message text.
They also carry `threshold_triggered` and `threshold_relation`, so trigger
counts and projections do not depend on parsing status strings. Hero JSON
includes `warning_threshold_relation_vocabulary` for the allowed relation
values. It also includes `warning_summary`, an aggregate projection with total,
triggered, unavailable, clear measured, compatibility `warning_count`, and
per-relation counts.
Deficits carry deterministic priority metadata, and `plan_basis` exposes the
primary plan-relevant deficit key/message/priority/class derived from those
priorities plus the addressed deficit priority classes. Hero JSON also includes
the deficit priority vocabulary. `plan_advice_scope_policy_vocabulary` records
that `plan_basis`, `suggested_wave`, and `PLAN_INPUT_*` are advisory projections
from deficits, not evidence, scheduler authority, contract identity, or
execution. `PLAN_INPUT_*` carries symbolic `latest_satisfying` source-target
hints; the resulting runtime report remains the proof of the concrete
checkpoint path that was loaded. `PLAN_MAX_ATTEMPTS`/`MAX_WAVES` only guard
whether another suggestion is advertised.
`deficit_vector_planning_summary` self-checks the combined planning boundary:
12 ordered deficit priority classes, 5 plan-advice policy rows, 4 advisory
planning surfaces, zero evidence/contract-identity authority, zero Lattice
execution authority, and a single Runtime Hero executor boundary.
Split-backed MDN training targets should use trained-node-head evidence;
evaluated-node-head evidence is for run/evaluation targets.
`operational_readiness_v1_scope_vocabulary` summarizes the V1 gate itself:
read-only evidence authority is included; DB/index, performance gates,
checkpoint id/file digest primary identity, structured source receipts,
first-class selection events, and VICReg node-support facts are deferred.
`operational_readiness_v1_scope_summary` self-checks that those 12 scope rows
partition into 6 included read-only claims and 6 deferred items, with zero
runtime-executor/performance/DB authority and no empty claim-boundary fields.
`operational_readiness_v1_gate_vocabulary` exposes the concrete acceptance
checklist for V1: identity binding, train-core VICReg/MDN readiness, validation
and test leakage guards, validation exact-checkpoint evaluation, warnings and
MDN node-support visibility, and Hero read/plan/closure inspection.
`operational_readiness_v1_gate_summary` self-checks that the 10 V1 gates are
required, read-only, non-executing, non-performance, non-DB-source-of-truth, have
pass/fail conditions and Hero surfaces, and reference all five required V1
targets.
DEV_WARNING: the current graph-first contract fingerprint still includes some
Jkimyei training selectors. Runtime model-state fields such as loaded checkpoint
paths are excluded from contract identity and must be proven through job
reports, exposure facts, and checkpoint lineage. Use the
`contract_identity_boundary_vocabulary` Hero field as the machine-readable
boundary when deciding whether a field is contract identity, target proof
identity, model-state input, plan advice, warning visibility, or audit metadata.
The companion `contract_identity_boundary_summary` should report zero identity
overlap for model-state, advice, warning, and audit rows.
Compact source-file receipt strings are the same kind of non-contract audit
metadata: they can identify source files for review, but target identity and
overlap proof remain anchored by the active graph/order/source identity and
row-index facts.

The old Hero split into default/temp/objective/optim file surfaces is not
part of the fresh path. Retired identity receipts, TSODAO optim checkpoints,
and runtime reset controls are also outside Config Hero v2.
