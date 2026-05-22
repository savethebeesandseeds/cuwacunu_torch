#Cuwacunu Config

`src/config/.config` is the central fresh config file for migrated runtime
paths. Grammar files live under `src/config/grammar/*.bnf`; authored DSL
payloads live at the config root.

Current migrated sections:

- `UJCAMEI`: source registry and source retrieval channel DSL paths.
- `KIKIJYEBA`: topology graph, wave settings, and lattice target DSL paths.
- `WIKIMYEI`: expression, representation, and inference DSL paths.
- `JKIMYEI`: training orchestration DSL paths.
- `HERO`: Config Hero, Runtime Hero, and Lattice Hero policy paths.

`kikijyeba.settings.wave.dsl` selects the focal runtime component with `TARGET`.
The current graph-first targets are `wikimyei.representation.encoding.vicreg`
for node representation training and `wikimyei.inference.expected_value.mdn`
for node ExpectedValue MDN. `MODE=run` executes the target dependency closure
without optimizer steps. `MODE=train` mutates only `TARGET`; upstream
dependencies run frozen.

MDN training is node-centered and expects a frozen representation encoder.
`wikimyei.inference.expected_value.mdn.jkimyei` should normally provide
`INPUT_REPRESENTATION_CHECKPOINT`; `ALLOW_UNTRAINED_REPRESENTATION=true` is an
explicit smoke-mode escape hatch for running the pipeline before a VICReg
checkpoint exists. `INPUT_REPRESENTATION_CHECKPOINT` and
`INPUT_MDN_CHECKPOINT` are runtime model-state inputs. They are recorded and
proven through manifests/reports/exposure facts/checkpoint lineage, and must not
change the protocol contract fingerprint.

Config Hero starts from:

- `[HERO].config_hero_dsl_path`
- `src/config/hero.config.dsl`
- `src/config/man/hero.config.man`

Runtime Hero starts from:

- `[HERO].runtime_hero_dsl_path`
- `src/config/hero.runtime.dsl`
- `src/config/man/hero.runtime.man`

Lattice Hero starts from:

- `[HERO].lattice_hero_dsl_path`
- `src/config/hero.lattice.dsl`
- `src/config/man/hero.lattice.man`

The lattice target DSL is now profile/guard aware. `LATTICE_PROFILE` captures
reusable readiness defaults, `LATTICE_GUARD` remains available for low-level
forbidden exposure policy, and `LATTICE_TARGET` binds those pieces to a concrete
split or source range. Validation/test holdout protection should usually live in
`kikijyeba.lattice.splits.dsl` through `PROTECT_FROM_USES`; targets can opt into
that default with `PROTECT_SPLIT`. The preferred target spellings are
`SUBJECT_COMPONENT`, `OVER_SPLIT`, `WAVE_MODE`, and `PLAN_MAX_ATTEMPTS`;
compatibility aliases still load so older flat targets remain valid.
The DSL also accepts clause blocks keyed by `TARGET_ID`: `LATTICE_DEPENDS`,
`LATTICE_REQUIRES`, `LATTICE_FORBIDS`, `LATTICE_PLAN`, and `LATTICE_WARN`. In
v0 these clauses are proof-language syntax that lower into the existing target
evaluator fields; they do not execute waves and they do not make the lattice a
scheduler. `LATTICE_WARN` is explicitly non-blocking.

Fresh Config Hero is intentionally a small policy-controlled config file
surface. It lists, reads, writes, and deletes managed config files under
configured roots; it does not own domain-specific DSL validation. Validation for
Ujcamei, Kikijyeba, Wikimyei, and Jkimyei remains with those domain modules.

Agent-facing Config Hero tools should prefer `hero.config.map` for global path
discovery and `hero.config.resolve` for path preflight before reading or
mutating. Reads return `sha256`; replacements and deletions require
`expected_sha256` by default, and both mutating file tools support
`dry_run=true`.

Runtime Hero is the agent-facing control and inspection surface for
`/cuwacunu/.build/exec/cuwacunu_exec`. It decodes the active wave, performs
guarded dry-runs/executions, and reads runtime artifacts under
`.runtime/cuwacunu_exec`.

Developer runtime reset is owned by Runtime Hero as `hero.runtime.dev_nuke`, not
by Config Hero. The tool defaults to dry-run, reports the exact runtime-root
entries it would clear, and requires `allow_dev_nuke=true` plus
`confirm_dev_nuke=true` for non-dry-run reset. Backup snapshots are controlled by
`dev_nuke_backup_enabled` and `dev_nuke_backup_root`, and allowed roots are
constrained by `allowed_dev_nuke_roots`.

`kikijyeba.lattice.targets.dsl` sits one level above waves. It declares
read-only readiness targets over contract-scoped runtime evidence and may
recommend the next wave, but it does not execute anything.
`kikijyeba.lattice.splits.dsl` defines named graph-anchor cursor ranges such as
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
cursor-indexed facts. New runtime manifests emit `graph_anchor_row_index_v1`
source footprints: observed input covers
`[anchor_begin - (Hx - 1), anchor_end)`, while future supervision covers
`[anchor_begin + 1, anchor_end + Hf)`. Older artifacts without those fields
still fall back to `anchor_range_v0`. When the graph-anchor cursor has a regular
reference key step, manifests also emit `graph_anchor_key_window_v1` key
windows. Those key windows are for source-key/timestamp audit; v0 exposure
coverage and forbidden-overlap checks still use row-index intervals. Manifests
also include compact active source-file receipts so runtime evidence can be
traced back to the concrete Ujcamei source files without inspecting the original
DSL.
Manifests, job state, component reports, and exposure facts also carry
graph-anchor domain health: candidate anchors, accepted anchors/fraction,
skipped anchors by reason, duplicate anchors, common/reference key bounds, and
a warning level. This does not change graph-first training semantics. Ujcamei
still restricts waves to the accepted common graph-anchor cursor domain; the new
fields make source-range clipping visible through Runtime/Lattice Hero.
Targets may now require exposure coverage over explicit anchor-index ranges or
forbid checkpoint exposure overlap with protected ranges. Those checks are
evaluated from the checkpoint exposure closure and fail as `exposure_failed`
when coverage is too low or forbidden exposure is found. The target evaluator can
auto-build this ledger from runtime job directories, so normal wave artifacts are
enough for v0 readiness checks. Coverage and leakage use different coordinates:
readiness coverage is measured over target-component-local graph-anchor coverage
intervals, while forbidden exposure checks use full-closure observed/target
source-row footprints. Checkpoint closure is fail-closed when an input checkpoint
has no producer fact, and node MDN exposure readiness verifies that the exact
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
unchanged. Repeated exposure is therefore visible without being treated as an
implicit readiness failure.
Targets can use `CHECKPOINT_SOURCE = latest_satisfying:<target_id>` to inspect
the checkpoint proven by another satisfied target. This is how read-only guards
such as `node_mdn_train_core_no_validation_leakage` check validation leakage on
the current train-core MDN checkpoint without creating a fake training target.
`node_mdn_train_core_no_test_leakage` then applies the same split-backed guard
to `test_holdout`, so the operational chain has both validation and test
holdout cleanliness before validation evaluation.
If the source target is not satisfied, the guard blocks and reports the source
target's suggested wave.
Validation evaluation targets use `MIN_EVALUATION_METRIC_COVERAGE` /
`USE=evaluation_metric` over the validation split. These targets expect
run-mode evidence, allow zero optimizer steps and no new checkpoint, and depend
on the holdout-clean guard for the training checkpoint they evaluate.
For node-centered MDN reports, the lattice scanner also derives read-only
`node_exposure` facts from `node_ids`, per-node routed/active/trained/evaluated
row counts, `valid_target_count_by_node`, and `mean_nll_by_node`. These facts
are per-node MDN support evidence linked to the parent exposure fact; they do
not claim per-node VICReg readiness because VICReg is a shared encoder.

Runtime now writes canonical `lattice.exposure.fact` sidecars after successful
terminal jobs and a `lattice.checkpoint.fact` sidecar when a checkpoint exists.
The exposure scanner prefers exposure sidecars, ingests checkpoint fact sidecars,
and falls back to manifest/state/report derivation for older jobs. Exposure
facts include `coverage_precision`: readiness coverage is credited only from
`contiguous_completed_range_v1` facts, while requested ranges that cannot prove
completion are treated as `requested_range_untrusted_v0`.

Lattice Hero is the read-only agent surface for these checks. It provides
`hero.lattice.status`, `hero.lattice.list_targets`,
`hero.lattice.explain_target`, `hero.lattice.evaluate_target`,
`hero.lattice.plan_target`, `hero.lattice.scan_exposure`, and
`hero.lattice.checkpoint_closure`. `explain_target` reports the compiled target
proof object and fingerprint without scanning runtime evidence; evaluation and
planning then read the in-memory exposure ledger built from runtime files. A
future lattice DB should be a rebuildable query/index cache over immutable
runtime/fact files, not the primary writer of evidence.
Split-backed MDN training targets should use trained-node-head evidence;
evaluated-node-head evidence is for run/evaluation targets.

DEV_WARNING: the current graph-first contract fingerprint still includes some
Jkimyei training selectors. Runtime model-state fields such as loaded checkpoint
paths are excluded from contract identity and must be proven through job
reports, exposure facts, and checkpoint lineage.

The legacy Hero split into default/temp/objective/optim file surfaces is not
part of the fresh path. Hashimyei identity receipts, TSODAO optim checkpoints,
and runtime reset controls are also outside Config Hero v2.
