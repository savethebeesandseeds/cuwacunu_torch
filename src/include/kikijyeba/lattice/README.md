#Kikijyeba Lattice

The Kikijyeba lattice language records latent lineage state
    : typed values,
      domains,
      runtime report facts,
      and the cursor context that says which wave and source range produced
              those facts
                  .

          Namespace : `cuwacunu::kikijyeba::lattice`.

    Core parser aliases,
      left - hand - side helpers,
      and the lattice decoder live directly in this namespace.The file
              extension or
          grammar flavor is not a C++ namespace
                  .

              Runtime component `
                  .lls` reports live under
`cuwacunu::kikijyeba::lattice::runtime_report`.

              Normal runtime reports and
              debug `.lls` reports are different evidence grades.The current
                  target evaluator can work from normal job manifests,
      job state,
      component reports,
      and checkpoints; it does not require every wave to run with
`debug`. Debug `.lls` facts should become the richer lineage layer for cursor
coverage, exposure accounting, and leakage checks.

The exposure ledger lives under `cuwacunu::kikijyeba::lattice::exposure`. It is
the normalized bridge between job-shaped runtime artifacts and cursor-indexed
lattice reasoning. Runtime now emits `lattice.exposure.fact` sidecars after
successful terminal jobs and records that sidecar path in `job.state`. The
scanner prefers those sidecars and falls back to deriving an exposure fact from
`job.manifest`, `job.state`, `representation.report`, and `inference.report`
when a sidecar is absent. Exposure facts record contract/component identity,
Ujcamei graph-anchor cursor range, observed/target/evaluation/selection use
flags, optimizer effort, valid-target evidence, checkpoint inputs, and
checkpoint outputs. Each fact also has an explicit `fact_type`; v0 primarily
emits `exposure`, but the lattice should eventually hold other fact types such
as metrics and source split declarations. Current runtime manifests emit
`graph_anchor_row_index_v1` footprints derived from the resolved graph-anchor
range and the Hx/Hf source windows:
observed input uses `[anchor_begin - (Hx - 1), anchor_end)`, while future
supervision uses `[anchor_begin + 1, anchor_end + Hf)`. Older artifacts without
these fields still fall back to `anchor_range_v0`. Exposure facts also preserve
`source_input_length` and `source_future_length` when runtime provides them.
Split protection uses those values, or the observed/target footprints as a
fallback, to expand protected holdout intervals by the relevant context/future
windows before forbidden-overlap checks. Runtime manifests also emit
`graph_anchor_key_window_v1` source-key windows when the graph cursor has a
regular reference key step. Those key windows are audit metadata that map row
footprints back to source keys/timestamps; v0 overlap math still uses row-index
intervals so split checks remain deterministic. The manifest also carries
active source-file receipts (`edge|instrument|interval|record_type|source`) so a
lattice fact can be traced back to the concrete Ujcamei files that supplied the
wave.

Exposure facts also carry Ujcamei graph-anchor domain health. This reports the
candidate anchor count, accepted anchor count/fraction, skipped anchors by
reason, duplicate anchors, common/reference key bounds, and a warning level. The
graph-first source path already clips training to the accepted common anchor
domain; these fields make that clipping visible to Runtime/Lattice Hero. The v0
policy is warning-oriented: zero accepted anchors is an error, low accepted
fraction, failed fetch probes, and high missing-edge coverage are scan warnings.
Target satisfaction is unchanged by these warnings unless a target later adds an
explicit requirement.

For node-centered MDN jobs, the scanner also derives
`kikijyeba.lattice.node_exposure.v1` facts from the existing
`inference.report` per-node fields. These facts link back to the parent
exposure fact digest and carry node id/index, routed row count, active row
count, trained row count, evaluated row count, valid target count, valid target
fraction over the completed anchor range, mean NLL, and the same
contract/graph/source/component identity as the parent MDN fact. This is
intentionally MDN-only for now: VICReg is a shared encoder, so per-node VICReg
support should wait until NodeLift or representation reports emit honest
node-support summaries.

When a job writes a checkpoint, runtime also emits a transitional
`lattice.checkpoint.fact` sidecar. This records component/contract/graph/source
identity, the producer job/wave, the direct exposure digest, input checkpoint
paths, a preview `checkpoint_id`, and a `checkpoint_file_digest` when the
checkpoint file is available. In V1, those ids/digests are audit/indexing
metadata; checkpoint closure authority is still path-first and exposure-fact
based. The DB/checkpoint-identity phase should promote checkpoint ids and file
digests into primary closure identity before relying on cached closure results.
The runtime-root scanner now parses checkpoint facts into the in-memory ledger
when they are present.

The ledger can compute checkpoint exposure closures by following input
checkpoint links. Closure resolution is fail-closed: if an output checkpoint
names an input checkpoint and the ledger has no producer fact for that input,
target evaluation treats the closure as incomplete instead of silently ignoring
the missing upstream lineage. This is the basis for leakage checks such as:
`output_checkpoint_exposure_closure` must not overlap validation or test cursor
footprints under forbidden uses like observed input, target supervision, or
selection signal.

Read-only lattice targets live under
`cuwacunu::kikijyeba::lattice::target`. A target is a desired state over
contract-scoped evidence, not a wave and not a scheduler. The v0 evaluator reads
runtime job manifests, job state, component reports, and checkpoints, then
returns a status plus an optional suggested wave. It does not execute.

Named split policy lives under `cuwacunu::kikijyeba::lattice::split`.
`kikijyeba.lattice.splits.dsl` defines graph-anchor cursor ranges and roles
outside the target language. Targets may use `TRAIN_SPLIT` / `OVER_SPLIT` to
resolve their anchor-index training range. Validation/test splits may also
declare `PROTECT_FROM_USES`, and targets can use `PROTECT_SPLIT` to apply that
holdout protection without repeating low-level forbidden-use lists. This keeps
train/validation/test definitions centralized instead of duplicating raw ranges
across target blocks. The split policy has a deterministic fingerprint. Target
evaluation reports that fingerprint, and exposure facts can carry it; once a
fact records a split-policy fingerprint, evaluation will not accept it under a
different split policy.

When a target requires contract or component matching, the active contract and
component fingerprints must be supplied by the caller. Graph-anchor ranged
targets and exposure requirements also require the active graph order
fingerprint and source cursor token, because anchor indices only have meaning
inside that cursor/order identity. Missing active identity blocks the target
instead of silently accepting stale evidence. Target evaluation scans matching
jobs newest-first and accepts the newest satisfying candidate; failed, stale, or
checkpoint-missing attempts do not hide an older compatible ready artifact.
`MAX_WAVES` is a planning guard over active non-dry-run train attempts with the
same contract/component/graph/cursor identity, not a readiness failure: existing
satisfying evidence can still pass, but exhausted targets stop recommending
another wave.
The target decoder rejects duplicate `TARGET_ID`s. Reusing the same component in
multiple targets is valid when ranges/splits differ; v0 exposes a validation
report that warns when duplicate target selectors appear so accidental duplicate
component/range targets are visible without blocking legitimate future
multi-range targets.
The target language also supports lightweight `LATTICE_PROFILE` and
`LATTICE_GUARD` blocks. Profiles name reusable readiness defaults; guards remain
available for low-level reusable forbidden exposure policies, while ordinary
holdout protection should prefer split-level `PROTECT_FROM_USES` plus target
`PROTECT_SPLIT`. The decoder lowers these forms into the existing v0 scalar
target fields, so they improve authoring and inspection without changing
evaluator semantics. Preferred aliases are `SUBJECT_COMPONENT`, `OVER_SPLIT`,
`WAVE_MODE`, and `PLAN_MAX_ATTEMPTS`; older names still load for compatibility
and mixed old/new spellings produce validation warnings.
The first clause blocks are also supported and lower into the same v0 evaluator
model:

```text
LATTICE_DEPENDS
  declares the upstream target relationship, currently used by node MDN to bind
  to the loaded representation checkpoint proof.

LATTICE_REQUIRES
  declares artifact, metric, optimizer-effort, node-head-count,
  valid-target-fraction, and exposure-coverage requirements.

LATTICE_FORBIDS
  declares forbidden exposure overlap. V0 supports source-row-footprint checks
  over the checkpoint closure.

LATTICE_PLAN
  declares the suggested wave mode/range and planning attempt limit.

LATTICE_WARN
  declares non-blocking warning checks over exposure load, effort density, or
  anchor-domain health. Warning clauses report evidence shape; they do not
  change readiness status or execute anything.
```

These clauses are intentionally not execution machinery. The compiler keeps a
compiled proof object beside the lowered v0 target: applied profiles, applied
guards, dependency clauses, requirement clauses, forbidden-exposure clauses,
plan clauses, warning clauses, language warnings, and a deterministic
`target_spec_fingerprint`. The
existing evaluator still consumes the lowered v0 target, while
`hero.lattice.explain_target` exposes the preserved proof object without
scanning runtime evidence.

Targets can optionally bind readiness to the exposure ledger with cursor-indexed
checks. `MIN_OBSERVED_INPUT_COVERAGE` and
`MIN_TARGET_SUPERVISION_COVERAGE` require a checkpoint exposure closure to cover
a fraction of an explicit `SOURCE_RANGE=anchor_index` interval. In
`LATTICE_REQUIRES KIND=exposure_coverage`, `CURSOR_EPOCHS` is the calmer
authoring alias for `MIN_COVERAGE`: it means the selected exposure use covers
that fraction of the named Ujcamei graph-anchor cursor range. Coverage uses the
target component's graph-anchor coverage interval, so an MDN readiness target
cannot satisfy its own training coverage from an upstream VICReg checkpoint.
Evaluation targets use the same cursor-epoch language with
`USE=evaluation_metric` / `MIN_EVALUATION_METRIC_COVERAGE`; those checks do not
require optimizer mutation and are intended for run-mode validation evidence
after the training checkpoint has passed its no-leakage guard.
The evaluator also reports derived exposure-load summaries for these coverage
requirements. Unique coverage answers how much of the cursor range has been
covered at least once; `cursor_exposure_load` counts repeated completed
exposure over the same range. For example, two completed passes over
`train_core` still have unique coverage `1.0`, but exposure load `2.0` cursor
epochs.

`LATTICE_WARN` adds non-blocking warning semantics over these summaries. Warning
clauses can report high `exposure_load`, high `effort_density`, or suspicious
`anchor_domain_health` without changing target status, `plan_ready`, suggested
waves, or `PLAN_MAX_ATTEMPTS` accounting. Warning results are returned beside
the normal evaluation as `warning_results`; only triggered warnings are mirrored
in the short `warnings` list. This keeps repeated exposure neutral: the lattice
reports cursor load and effort density, but does not call the model
overtrained.
Targets may also set `CHECKPOINT_SOURCE = latest_satisfying:<target_id>`.
This is for read-only guard targets that inspect a checkpoint produced by
another target instead of matching their own runtime job. For example,
`node_mdn_train_core_no_validation_leakage` inspects the latest checkpoint from
`node_mdn_train_core_ready`, then applies validation split protection to that
checkpoint closure. `node_mdn_train_core_no_test_leakage` chains from that
validation-clean target and applies `test_holdout` protection to the same
checkpoint. If the source target is not satisfied, the guard target blocks and
forwards the source target's suggested wave; it still does not execute anything.

Evaluation targets can bind to a previously proven checkpoint with
`EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>`. For example,
`node_mdn_validation_eval_ready` proves run-mode validation evidence only if the
MDN report loaded the exact checkpoint selected by
`node_mdn_train_core_no_test_leakage`, and the representation checkpoint loaded
for the run matches that checkpoint lineage. Suggested waves may carry
`PLAN_INPUT_MDN_CHECKPOINT` and `PLAN_INPUT_REPRESENTATION_CHECKPOINT` model
state hints, but those are runtime inputs, not contract identity, and they still
must be proven by the resulting report/facts.

Forbidden exposure ranges are a different question: they inspect the full
checkpoint closure, including upstream components, and reject a checkpoint when
its closure overlaps a protected source-row footprint under uses such as
`observed_input`, `target_supervision`, `evaluation_metric`, or
`selection_signal`. This split is intentional: target supervision coverage
answers "which anchors trained this component?", while target supervision
leakage answers "which future source rows did this checkpoint depend on?" These
checks return `exposure_failed`. The evaluator can build the ledger from normal
runtime job directories automatically; when that is disabled, missing ledger
wiring returns `blocked` because the target cannot be evaluated honestly.

Node-centered MDN readiness also verifies the exact representation checkpoint
recorded by the MDN report when exposure requirements are active. It is not
enough for some VICReg target to be ready elsewhere; the loaded checkpoint must
have an active compatible VICReg producer fact under the same contract, graph
order, source cursor, and VICReg assembly identity.
Split-backed MDN training targets use trained-node-head evidence, while
evaluation-style targets may still require evaluated-node-head evidence.

Exposure facts carry graph order identity and job status. A job only counts as
mutating a component when it completed a `train` wave, recorded optimizer effort,
and its manifest explicitly lists the target component in `mutated_components`.
Coverage is not credited from the requested anchor range alone. V1 exposure
facts carry `coverage_precision`; coverage checks use `completed_anchor_range`
only when the fact is marked `contiguous_completed_range_v1`. Requested ranges
without enough completion evidence are marked `requested_range_untrusted_v0` and
do not satisfy readiness coverage.

Lattice Hero is the read-only operating surface for this layer. It currently
loads lattice targets from the active global config, builds an in-memory
exposure ledger from runtime artifact files, evaluates/plans targets, previews
facts, and inspects checkpoint closures. It intentionally does not execute
waves.

## Operative Notes

For day-to-day operation, treat the lattice as an inspector over runtime
evidence:

```text
Runtime writes what happened.
Exposure facts say what data was seen and how it was used.
Checkpoint closure says what lineage a checkpoint inherits.
Targets say what must be true.
Lattice Hero reads, explains, evaluates, and plans.
Runtime Hero executes.
```

The active global config points to one target DSL and one split DSL:

```text
kikijyeba.lattice.targets.dsl
kikijyeba.lattice.splits.dsl
```

There is one active file of each kind per config in v0, but each file may carry
multiple blocks. `kikijyeba.lattice.targets.dsl` contains many
`LATTICE_TARGET` declarations plus reusable profiles, dependencies,
requirements, forbids, and plan clauses. `kikijyeba.lattice.splits.dsl`
contains one split policy and many named graph-anchor cursor ranges such as
`train_core`, `validation_holdout`, and `test_holdout`.

Agent runbook:

1. `hero.lattice.status`
   Always start here. Report target/split paths, split-policy fingerprint,
   fact counts, warnings, and inferred active identity before making claims
   about target state.
2. `hero.lattice.list_targets`
   Use this to discover the target ids compiled from the active target DSL.
3. `hero.lattice.explain_target`
   Use this before editing or evaluating a target. It is the source of truth for
   what the compiled proof obligation means without scanning runtime evidence.
4. `hero.lattice.scan_exposure`
   Use this before evaluating unfamiliar runtime roots. Report warnings
   verbatim. The preview ledger is derived from `job.manifest`, `job.state`,
   reports, checkpoints, and lattice fact sidecars. It includes
   `anchor_domain_health` so agents can see whether the graph-anchor cursor was
   heavily clipped by source availability before interpreting readiness. For
   MDN jobs it also previews derived `node_exposure` facts so agents can inspect
   per-node target support without making shared-encoder claims for VICReg.
5. `hero.lattice.evaluate_target`
   Use this when the question is "is this target satisfied?" A satisfied
   train-core node-MDN target proves the exact loaded representation checkpoint
   has compatible upstream VICReg producer lineage. A validation evaluation
   target proves run-mode `evaluation_metric` coverage over the validation
   split, not a new training checkpoint. For exposure-backed targets, the result
   also includes `exposure_summaries` with unique coverage, repeated
   cursor-epoch load, and optimizer-step density.
6. `hero.lattice.plan_target`
   Use this when the question is "what wave should be suggested next?" Treat
   the returned wave as advice only. It does not execute anything.
7. `hero.lattice.checkpoint_closure`
   Use this for checkpoint lineage. Missing upstream producer evidence must
   fail closed and should be reported as a blocker, not ignored.

For an empty runtime root, graph-anchor targets cannot infer active identity.
Pass `protocol_contract_fingerprint`, `graph_order_fingerprint`,
`source_cursor_token`, and the component assembly fingerprints explicitly when
evaluating/planning, or first run a Runtime Hero dry-run to create
identity-bearing runtime artifacts.

Agent invariants:

```text
MUST NOT execute waves from Lattice Hero.
MUST report Lattice Hero warnings and blocked/exposure_failed reasons verbatim.
MUST provide active identity explicitly when evaluating an empty runtime root.
MUST use Runtime Hero, not Lattice Hero, for execution.
SHOULD prefer explain_target before changing target DSL.
SHOULD prefer OVER_SPLIT, PROTECT_SPLIT, and CURSOR_EPOCHS when authoring.
```

## Operational Readiness V1 Gate

Lattice Operational Readiness V1 means the lattice is ready to serve as the
read-only evidence authority for a compiled graph-first contract. It does not
mean the lattice is a scheduler, a performance judge, or a DB-backed audit
system.

The V1 gate is satisfied only when these checks are true for the active
contract/runtime root:

```text
1. Identity
   Graph-anchor readiness targets require protocol contract fingerprint, graph
   order fingerprint, source cursor token, and the relevant component assembly
   fingerprint. Stale contract, graph, cursor, or assembly evidence cannot
   satisfy a target.

2. Readiness
   vicreg_train_core_ready and node_mdn_train_core_ready are satisfied from
   completed normal runtime evidence, not debug-only reports. MDN readiness
   proves the exact representation checkpoint loaded by the MDN report.

3. Holdout cleanliness
   node_mdn_train_core_no_validation_leakage and
   node_mdn_train_core_no_test_leakage inspect the checkpoint closure from the
   train-core MDN target and fail closed on protected split overlap. Split
   protection expands holdout footprints by Hx/Hf purge policy before checking
   source-row exposure overlap.

4. Validation evaluation
   node_mdn_validation_eval_ready is satisfied only by run/evaluation evidence
   over validation_holdout. It must prove evaluation_metric coverage, zero
   training mutation, the exact MDN checkpoint selected by the no-test leakage
   guard, and the matching representation checkpoint lineage.

5. Warnings
   Source-domain health, repeated cursor exposure load, and optimizer effort
   density are visible as summaries and non-blocking warning results. Warnings
   do not change status, planning, or max-attempt accounting.

6. Node support
   MDN node_exposure previews show routed, active, trained, and evaluated row
   support plus valid targets and mean NLL by node. They are MDN support facts,
   not VICReg per-node readiness claims.

7. Hero surface
   hero.lattice.status, list_targets, explain_target, scan_exposure,
   evaluate_target, plan_target, and checkpoint_closure are enough for an agent
   to understand evidence, failure reasons, warnings, closure completeness, and
   the next suggested wave without executing anything.
```

DB/index work, performance gates, stable checkpoint ids, structured source
receipt facts, first-class selection-signal events, and VICReg node-support
facts are intentionally outside this V1 gate.

The V1 acceptance receipt is recorded in
`src/tests/fixtures/kikijyeba/lattice_operational_readiness_v1/PASS.md`. That
file is an audit receipt for the completed train-core/validation evidence chain;
it is not a portable runtime fixture and it does not make `.runtime` artifacts
part of the source tree.

When the target language feels too low-level, prefer the calmer authoring layer:

```text
OVER_SPLIT = train_core;
PROTECT_SPLIT = validation_holdout;
CURSOR_EPOCHS = 0.95;
```

The compiler lowers those fields into the same explicit proof clauses used by
the evaluator. Do not bypass the proof layer by treating targets as schedules or
campaigns; a target may recommend a wave, but it must not dispatch it.

The future lattice DB should be an index/cache over durable evidence, not the
source of truth. Runtime writes immutable files first: `job.manifest`,
`job.state`, component reports, checkpoints, `lattice.exposure.fact`, and
`lattice.checkpoint.fact`. Lattice Hero can then ingest those files into a DB for
fast queries, rebuild that DB from disk when it is missing or stale, and use a
sync token over runtime/fact file metadata to avoid unnecessary rescans. Runtime
should not write directly to the DB in v0 because that would couple training
execution to one query backend and make recovery harder.

DEV_WARNING: the current graph-first protocol contract fingerprint still
includes some Jkimyei training selectors. Runtime model-state fields such as
loaded checkpoint paths are excluded from contract identity and must be proven
through job reports, exposure facts, and checkpoint lineage.
