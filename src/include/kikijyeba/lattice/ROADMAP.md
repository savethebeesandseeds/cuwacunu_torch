# Lattice Roadmap

This roadmap records the work after Operational Readiness V1. Keep it as a
dispatch list for future `/goal` runs, not as an invitation to expand V1.

## Current Roadmap State

As of `2026-05-23`, the finite completed milestones are:

```text
V1   complete: read-only operational readiness authority
V2   complete: proof-system core, channel runtime evidence, fact inventory,
               checkpoint identity, source receipts, selection signals,
               representation support, certificates, algebra, causal closure,
               deficit vectors, unit typing, support matrices, and geometry
               warnings
V3-A complete: read-only audit index query layer
V3-B complete: performance fast paths for interactive audit work
V3-C complete: validation performance evidence policy
V3-D complete: MDN distribution calibration diagnostics
V3-E complete: Datalog-style derived query pilot
V3-F complete: Pareto evidence comparison
V3-J complete: Hero MCP schema compatibility hygiene
```

Future work should be opened only from the `Next Finite Dispatch Goals` section
near the end of this file. Earlier V2 sections are kept as completed receipts
and design history, not as active instructions.

Active boundaries remain:

```text
- runtime/components write evidence; the lattice reads and proves from it
- Runtime Hero remains the executor
- cache/index rows are rebuildable read models, not source of truth
- validation performance does not become readiness without explicit metrics,
  uncertainty policy, leakage policy, and negative tests
- no scalar "lattice score" replaces proof vectors or Pareto dimensions
```

## V1 Frozen Boundary

V1 is complete as a read-only evidence authority.

It proves readiness from runtime artifacts, exposure facts, checkpoint closure,
holdout protection, validation evaluation binding, warning visibility, and MDN
node-support visibility. It is not a scheduler, performance judge, DB source of
truth, or production checkpoint-identity authority.

Runtime/components write what happened. The lattice reads those artifacts,
normalizes them into evidence, evaluates targets, emits proof certificates, and
explains suggested next waves without executing them.

## Archived Pre-V2 Gate: Wikimyei Migration Settlement

This gate is complete and kept here as design history. V2 fact-schema work
started only after the Wikimyei migration report shape was inventoried.

Current inventory artifact:

```text
src/include/kikijyeba/lattice/V2_FACT_INVENTORY.md
src/include/kikijyeba/lattice/FACT_EMISSION_CONTRACT.md
```

Achievable goal:

```text
Inventory migrated Wikimyei reports and classify each emitted field as:
  runtime identity
  model-state input/output
  exposure support
  checkpoint lineage
  source receipt
  diagnostic warning metric
  future performance metric
```

Acceptance:

```text
- each migrated Wikimyei component has a report-field inventory
- runtime model-state fields remain outside contract identity
- fields that can become lattice facts are named
- fields that must stay diagnostics are named
- no new target authority is added during the inventory
```

## Archived Completed V2 Dispatch Packet

This section records the completed V2 dispatch sequence. Do not open new goals
from this section unless a regression requires repairing a specific completed
V2 artifact.

Do not dispatch all future lattice work as one open-ended goal. Use the finite
V3 packets at the end of this roadmap.

Mathematical road-ahead from the V2 review:

```text
The lattice should grow as a finite sequence of proof-system upgrades:
  runtime facts
    -> abstract evidence state
    -> monotone target predicates
    -> proof certificates
    -> read-only deficit-based planning
```

Dispatch rule:

```text
Each mathematical upgrade must be opened as its own bounded /goal. A goal is
complete only when the code/docs/tests name the invariant, expose it through
Hero or a receipt, and include at least one negative check proving it fails
closed. Do not merge performance judging, DB indexing, or scheduling into these
goals.
```

Completed achievable sequence:

```text
1. Proof Certificate v1:
   every satisfied target returns a compact certificate for identity, coverage,
   closure, leakage, dependencies, warnings, and support.

2. Exposure Algebra v1:
   unique coverage is idempotent interval union, repeated load is additive
   measure, and leakage is protected-split intersection after Hx/Hf dilation.

3. Causal Closure v1:
   checkpoint cleanliness is labeled reachability over runtime jobs, exposure
   facts, checkpoint facts, loaded checkpoints, and selection-signal edges.

4. Deficit Vector v1:
   unsatisfied targets expose missing proof obligations by dimension, and
   suggested waves remain read-only advice derived from those deficits.

5. Target Unit Typing v1:
   target numeric fields carry units such as fraction, cursor_epoch, count,
   rate, and loss so nonsensical clauses fail at load/compile time.

6. Support Matrix v1:
   MDN and future representation support are summarized by node and use, with
   weakest-node and imbalance warnings kept out of hard readiness by default.

7. Representation Geometry Warnings v1:
   VICReg/shared-representation checkpoints may emit collapse-health metrics as
   warnings, not validation performance gates.
```

Promoted beyond V2 only through the finite V3 packets below:

```text
- validation performance gates with uncertainty bounds
- MDN distributional calibration gates
- Datalog-style derived query engine
- Pareto/nondominated checkpoint ranking
- source-key/order-preserving map audits as proof authority
- DB/index as anything other than a rebuildable cache over durable evidence
```

### V2 Dispatch Goal A: Migration Clearance And Fact Inventory

Achievable goal:

```text
Refresh the Wikimyei fact-emission inventory against the settled migrated
reports and mark each candidate field as authority, visibility, diagnostic, or
deferred.
```

Acceptance:

```text
- migrated node and channel Wikimyei reports are inventoried
- model-state inputs and output checkpoints remain runtime evidence, not
  protocol contract identity
- every promoted V2 fact has an emitting component/report field or sidecar
- diagnostic metrics are explicitly kept out of readiness unless a target rule
  promotes them
- V2_FACT_INVENTORY.md records the final classification and known gaps
```

Current status:

```text
Complete for V2. V2_FACT_INVENTORY.md and FACT_EMISSION_CONTRACT.md classify
node and channel report fields, model-state inputs, checkpoints, exposure
support, source receipts, warning diagnostics, and deferred performance metrics.
Promoted fact families now have component/report fields or sidecars, and known
post-V2 gaps remain listed under V3 candidates.
```

### V2 Dispatch Goal A1: Component Fact-Emission Contract

Achievable goal:

```text
Define the narrow contract by which Wikimyei components report facts to the
lattice after migration, while keeping the lattice read-only.
```

Acceptance:

```text
- each component names the runtime report fields and sidecars it owns
- promoted facts are explicitly separated from raw report fields, diagnostics,
  derived summaries, and proof certificates
- components write runtime artifacts/facts; the lattice only scans, normalizes,
  evaluates, and explains them
- fact rows bind to active contract identity, graph order, source cursor, split
  policy, and relevant checkpoint identity when applicable
- model-state inputs and loaded checkpoint paths are runtime evidence, not
  protocol contract identity
- missing or malformed promoted facts fail closed or warn according to the
  target policy; they never silently satisfy readiness
- the contract states whether each fact is authority, visibility, warning, or
  deferred performance evidence
```

Current status:

```text
FACT_EMISSION_CONTRACT.md records the current read-only boundary between
component report fields, runtime artifacts, normalized lattice facts, derived
summaries, target proof certificates, and rebuildable DB/index cache rows. The
contract has been refreshed for channel representation/MDN reports, runtime
model-state checkpoint inputs, NodeLift/representation support, source receipts,
selection signals, and warning-only diagnostics.
```

### V2 Dispatch Goal A2: Shared VICReg Support Vocabulary

Achievable goal:

```text
Clarify how shared VICReg/NodeLift support should be reported without implying
one VICReg model or one readiness gate per node.
```

Acceptance:

```text
- the roadmap and fact inventory state that VICReg remains a shared encoder
- per-node representation support means rows/features contributing to the shared
  representation path, not separate per-node VICReg checkpoints
- support denominators come from NodeLift or representation reports, not from
  downstream MDN target rows
- support summaries start as visibility and warning evidence only
- future hard gates require an explicit target rule and negative tests before
  they can affect readiness
```

Current status:

```text
Complete for V2. The roadmap, fact inventory, contract, scanner, and Hero output
state that VICReg is a shared encoder. Node-indexed representation support means
per-node support for the shared representation path, comes only from
NodeLift/representation payloads, is never backfilled from MDN node rows, and
remains visibility/warning evidence unless an explicit future target promotes
it.
```

### V2 Dispatch Goal B: Channel Routing Preflight

Achievable goal:

```text
Prove that Runtime Hero and the runtime job decoder route node and channel
Wikimyei waves unambiguously before producing new channel evidence.
```

Acceptance:

```text
- direct Runtime Hero and MCP Runtime Hero agree on active wave decoding after
  any required server refresh
- a wave whose intent is channel execution cannot silently decode to the node
  inference_mdn or representation_vicreg job kind
- channel-vs-node routing comes from an explicit profile/component selector, not
  from a wave-id naming convention alone
- runtime policy remains dry-run/non-executing unless the user deliberately
  enables execution through Runtime Hero
- CHANNEL_DISPATCH.md records the preflight result and any required config/code
  fix before channel evidence is requested
```

Current status:

```text
The runtime runner maps the migrated canonical VICReg/MDN wave targets to
channel_representation_vicreg and channel_inference_mdn. Runtime Hero's rebuilt
direct binary now previews the same channel job kind for the active channel MDN
wave, and run-test_hero_runtime_wave_preview covers canonical channel targets
plus the legacy node compatibility target. Long-running MCP Hero servers may
still need restart/refresh before their cached tool output reflects the rebuilt
binaries.
```

### V2 Dispatch Goal C: Channel Runtime Evidence Chain

Achievable goal:

```text
Produce the channel train-core and validation-eval runtime artifacts needed for
the channel lattice targets, using Runtime Hero or another approved executor.
```

Precondition:

```text
An explicit runtime execution decision is required before non-dry-run channel
training can run. Runtime Hero policy must allow non-dry-run execution and
train execution, and the executor must pass the required confirmation. Dry-run
artifacts prove wiring only; they cannot satisfy channel readiness.
```

Acceptance:

```text
- channel VICReg train-core evidence satisfies vicreg_train_core_ready
- channel MDN train-core evidence satisfies channel_mdn_train_core_ready
- channel validation/test leakage guards satisfy from channel checkpoint closure
- channel validation eval loads the exact trained channel MDN checkpoint and the
  correct representation checkpoint
- lattice target statuses move because runtime artifacts exist, not because the
  lattice executed anything
- PASS.md and CHANNEL_DISPATCH.md record target statuses, checkpoint identities,
  closure authority, warnings, and deferred items
```

Current status:

```text
Complete for V2. Runtime Hero produced completed channel VICReg train-core,
channel MDN train-core, and channel validation-eval artifacts under
/cuwacunu/.runtime/cuwacunu_exec. Direct Lattice Hero evaluates
vicreg_train_core_ready, channel_mdn_train_core_ready,
channel_mdn_train_core_no_validation_leakage,
channel_mdn_train_core_no_test_leakage, and
channel_mdn_validation_eval_ready as satisfied with
proof_certificate_check.passed=true. Runtime Hero policy was restored to
default_dry_run=true, allow_execute=false, and allow_train_execute=false.
```

### V2 Dispatch Goal D: V2 Receipt Freeze

Achievable goal:

```text
Freeze the V2 proof receipt after node and channel evidence are either proven or
explicitly deferred.
```

Acceptance:

```text
- direct Hero and MCP Hero target catalogs agree after rebuild/restart
- V2 PASS records all satisfied, blocked, missing_report, and deferred targets
- stable checkpoint_id/checkpoint_file_digest authority is recorded wherever
  available, with legacy_path fallback called out explicitly
- source receipts, selection-signal facts, representation support summaries, and
  certificate checks are included in the receipt
- deferred V3/math work remains listed as roadmap work, not hidden inside V2
  completion
```

Current status:

```text
Complete for V2. PASS.md, COMPLETION_AUDIT.md, and CHANNEL_DISPATCH.md record
the final channel target statuses, checkpoint identity authority, structured
fact counts, warnings, certificate checks, and deferred V3 items.
```

## Archived Completed V2 Core Goal

This section records the completed V2 Core goal. It should not be used as the
next dispatch source. Future goals start from the finite V3 packets below.

Achievable goal:

```text
Finalize Lattice V2 Core as a read-only proof system over stable checkpoint
identity, structured provenance facts, explicit causal uses, representation
support visibility, and machine-checkable target certificates.
```

Included work:

```text
1. Re-run the Wikimyei fact-emission inventory against the settled migration.
2. Finish stable checkpoint_id/checkpoint_file_digest closure authority.
3. Promote compact source receipts into structured audit-only receipt facts.
4. Add first-class selection_signal events for model/checkpoint selection.
5. Add NodeLift/representation support facts for the shared VICReg path.
6. Emit proof certificates that expose identity, coverage, closure, leakage,
   dependency, warning, and node-support proof sections.
7. Name the exposure algebra used by the certificate: idempotent coverage,
   additive load, and protected-split dilation for leakage.
8. Write a V2 PASS artifact that records the final target statuses, fact counts,
   warning state, closure authority, and known deferred items.
```

Acceptance:

```text
- all new authority comes from runtime/component reports and sidecars
- the lattice remains read-only and never executes waves
- runtime model-state inputs stay outside contract identity
- every promoted fact binds to active contract, graph order, and source cursor
- row-index intervals remain the authority for coverage and leakage
- source receipts remain provenance/audit evidence, not overlap authority
- checkpoint closure prefers stable id/digest and fails closed on mismatch
- selector-derived leakage paths are visible and fail closed when forbidden
- representation support facts are visibility/warnings first, not per-node
  hard VICReg readiness gates
- satisfied targets include machine-checkable certificate sections
- negative tests cover wrong checkpoint, stale identity, malformed receipt,
  forbidden selector path, unresolved lineage, and protected-split leakage
```

Completed dispatch order:

```text
1. Refresh the Wikimyei fact inventory after migration settlement.
2. Finish V2 Core fact authority: checkpoint identity, receipts, selection
   events, and representation support visibility.
3. Add the mathematical proof kernel: evidence partial order, certificates,
   exposure algebra, and labeled causal closure.
4. Add explainability hardening: deficit vectors, unit-typed target values, and
   node-support balance warnings.
5. Add representation geometry warnings only after representation support facts
   are visible and stable.
6. Add DB/index work only after the fact vocabulary and certificate vocabulary
   are stable.
```

Each item above was completed as bounded work. Do not dispatch "finish V2" as a
new goal; use the finite V3 packets below.

Out of scope for V2 Core unless explicitly promoted:

```text
- DB/index as a cache layer
- validation performance gates
- MDN distribution calibration gates
- hard representation geometry gates
- Datalog query engine
- Pareto checkpoint ranking
- source-key map audits as proof authority
```

## V2 Goal 1: Stable Checkpoint Identity

Promote checkpoint identity from V1 path-first closure plus audit previews to a
stable identity policy based on `checkpoint_id` and `checkpoint_file_digest`.

Achievable goal:

```text
Make checkpoint_id/checkpoint_file_digest primary closure identity while keeping
path compatibility for legacy V1 evidence.
```

Acceptance:

```text
- checkpoint facts carry stable id and file digest
- closure can resolve by checkpoint id/digest
- path-only closure remains accepted only as legacy compatibility
- wrong digest or mismatched id fails closed
- Hero explains whether closure authority was id/digest or path compatibility
- tests cover correct id, missing id, wrong digest, and legacy path fallback
```

Current status:

```text
Implemented in the V2 worktree. Closure promotes canonical checkpoint_id plus
checkpoint_file_digest when the checkpoint fact binds to the current producer
exposure digest and the file digest matches runtime bytes. Stale V1 sidecars or
placeholder ids downgrade to explicit legacy_path compatibility. Wrong digest
or mismatched id fails closed.
```

## V2 Goal 2: Structured Source Receipt Facts

Turn compact source receipt strings into structured lattice facts.

Achievable goal:

```text
Emit and ingest source receipt facts for graph-anchor evidence.
```

Acceptance:

```text
- source receipt facts record edge, instrument, interval, record type, and source
- facts bind to contract/graph/source cursor identity
- lattice keeps row-index intervals as overlap authority
- source-key/source-file receipt data remains audit/provenance evidence
- malformed or missing receipts warn or block according to declared policy
- Hero exposes receipt summaries and receipt-provenance warnings
```

Current status:

```text
Implemented in the V2 worktree as audit-only source_receipt facts. Exposure
ledger scans parse compact source_file_receipts into structured receipt rows
bound to the parent exposure digest and active graph/source identity. Missing
receipts are counted as audit absence; malformed non-empty receipts produce scan
warnings and malformed summary counts. Row-index intervals remain the only
coverage/leakage authority.
```

## V2 Goal 3: First-Class Selection-Signal Events

Represent checkpoint/model selection as explicit evidence instead of only a
forbidden causal-use label.

Achievable goal:

```text
Add selection_signal facts/events and make leakage checks reason over selector
paths.
```

Acceptance:

```text
- selection events identify source evaluation, selected checkpoint, and selector
- validation/test-derived selection paths are visible
- forbidden selector paths into reported checkpoints fail closed
- non-selector evaluation remains allowed when read-only and non-mutating
- Hero explains selector lineage separately from evaluation_metric coverage
- tests cover clean eval, forbidden validation selector, and forbidden test selector
```

Current status:

```text
Implemented in the V2 worktree as read-only selection_signal_event facts derived
from exposure facts with use_selection_signal=true. The event records the parent
exposure digest, selector id/kind/rule, selected checkpoint path, output/input
checkpoint paths, active identity, and selector footprint. Forbidden
selection_signal leakage witnesses now carry selector id, selector kind,
selection event digest, and selected checkpoint path. Non-selector validation
evaluation remains read-only and non-mutating.
```

## V2 Goal 4: NodeLift / Representation Node-Support Facts

Add node-indexed support facts for the shared representation pipeline. This is
not "one VICReg per node"; it is support visibility for a shared encoder.

Achievable goal:

```text
Emit NodeLift and/or representation node-support facts for the shared VICReg
representation path.
```

Acceptance:

```text
- facts report per-node valid lifted rows or valid projection rows
- facts report per-node masked/dropped/support counts when available
- facts bind to the parent representation exposure fact
- MDN node-support facts are not reused as VICReg evidence
- synthetic backfill from downstream MDN rows is rejected
- Hero exposes node-support balance for shared representation visibility
- V2 starts with warnings/visibility, not hard per-node readiness
```

Current status:

```text
Implemented in the V2 worktree as aggregate and node-indexed
representation_support facts for shared VICReg support visibility. The channel
representation launcher emits honest per-node denominators, valid lifted row and
feature counts, valid lifted cell counts, valid projection row counts, and
valid lifted feature fractions. The scanner derives shared-representation node
facts only from representation reports and NodeLift .lls sidecars when support
payloads exist, binds them to the parent representation exposure digest, and
exposes weakest-node plus balance metrics through Hero. Downstream MDN
node-support rows are not reused as VICReg evidence. These facts remain
visibility/warning evidence only, not hard per-node VICReg readiness gates.
```

## V2 Core PASS Artifact

Current receipt:

```text
src/tests/fixtures/kikijyeba/lattice_operational_readiness_v2/PASS.md
```

Remaining channel dispatch note:

```text
src/tests/fixtures/kikijyeba/lattice_operational_readiness_v2/CHANNEL_DISPATCH.md
```

Current completion audit:

```text
src/tests/fixtures/kikijyeba/lattice_operational_readiness_v2/COMPLETION_AUDIT.md
```

Current status:

```text
The V2 PASS receipt is frozen for the active node and channel runtime evidence.
It records structured source receipts, first-class selection-signal stream
availability, representation support visibility, stable
checkpoint_id/checkpoint_file_digest closure authority, machine-checkable proof
certificate checks, and known deferred items. The channel evidence chain is now
present under the accepted runtime root: channel VICReg train-core, channel MDN
train-core, validation/test leakage guards, and channel validation eval all
evaluate as satisfied by direct Lattice Hero. Historical receipt artifacts may
only contain aggregate representation support if they predate node-indexed
channel report emission.
```

V2 follow-up status:

```text
No V2 channel evidence goal remains. Future goals start from the finite V3
dispatch packets below.
```

## V2 Goal 5: DB/Index As Rebuildable Cache

Add an index only after the fact vocabulary is stable.

Achievable goal:

```text
Build a read-only DB/index over runtime artifacts and lattice facts.
```

Acceptance:

```text
- runtime files and sidecars remain the source of truth
- DB rows are rebuildable from runtime artifacts
- DB does not execute waves and does not write evidence
- cache invalidation is based on runtime/fact file metadata or digests
- Hero can report whether results came from live scan or cache
- stale/missing cache never upgrades target satisfaction
```

Current status:

```text
Implemented in the V2 worktree as a read-only runtime index cache. The cache is
`kikijyeba.lattice.runtime_index_cache.v1`, stores rebuildable row digests for
exposure, node, checkpoint, source-receipt, selection-signal, and
representation-support facts, and validates against a runtime metadata digest.
`hero.lattice.index_status` reports cache validity and returns valid cache rows
or a live-scan fallback. Target evaluation remains live-scan based, so stale or
missing cache rows never upgrade satisfaction.
```

## V2 Mathematical Dispatch Rules

The mathematical work should be dispatched as small goals. Do not bundle all of
it into one `/goal`, and do not let it reopen V1. Each goal below must preserve
the read-only lattice boundary: runtime/components emit evidence, the lattice
reads and proves from it, and Runtime Hero remains the executor.

## V2 Mathematical Goal 1: Evidence Partial Order and Safe Joins

Make the word "lattice" explicit by defining when one evidence state is stronger
than another, and how compatible summaries may be joined without weakening proof
soundness.

Achievable goal:

```text
Define a versioned evidence-state partial order for identity, coverage, load,
closure, leakage, warnings, and support summaries.
```

Acceptance:

```text
- the evidence state has named dimensions: active identity, coverage, exposure
  load, checkpoint closure, leakage predicates, warning metrics, and support
  summaries
- stronger evidence cannot reduce unique coverage, cannot hide unresolved
  lineage, cannot erase forbidden leakage, and cannot downgrade identity checks
- clean evidence growth preserves satisfied target status
- joining summaries is allowed only when active identity and split policy match
- joins keep coverage idempotent and load additive
- Hero or tests can explain when two evidence states are incompatible
- no runtime execution, scheduling, or DB authority is added
```

Current status:

```text
Implemented in the V2 worktree. The target layer exposes product evidence-state
dimensions, evidence-order vectors, join-law vocabulary, incompatible-join
diagnostics, and monotonicity invariants. The focused lattice target regression
checks identity-scoped joins, idempotent coverage, additive load, lineage and
leakage non-erasure, and read-only boundaries.
```

## V2 Mathematical Goal 2: Abstract Evidence Soundness Contract

Document and test the relationship between concrete runtime artifacts and the
lattice summaries used by target evaluation. This is the bridge to a future DB
cache, but it does not build the DB.

Achievable goal:

```text
Specify the lattice scan as a conservative abstraction from runtime artifacts to
proof evidence, and add focused checks that summaries never upgrade uncertain
or missing evidence into satisfaction.
```

Acceptance:

```text
- concrete evidence is defined as runtime reports, sidecars, exposure facts,
  checkpoint facts, and component debug summaries
- abstract evidence is defined as the scanner output consumed by target
  evaluation and Hero
- missing identity, missing lineage, malformed promoted facts, and unknown
  checkpoint inputs remain blocked or failed, never satisfied
- summary/cache-like derived values are marked as derived from concrete facts
- stale or incompatible summaries cannot be joined into active proof evidence
- the contract is written so a future DB/index can cache summaries without
  becoming the source of truth
```

Current status:

```text
Implemented in the V2 worktree. Concrete inputs are runtime reports, sidecars,
exposure/checkpoint facts, and component debug summaries; abstract outputs are
the scanner/evaluator/Hero summaries. Tests cover missing identity, unresolved
lineage, malformed promoted facts, unknown checkpoint inputs, stale/incompatible
summary joins, and the read-only DB/cache boundary.
```

## V2 Goal 6: Machine-Checkable Proof Certificates

Turn target results from status-plus-explanation into compact proof objects that
Hero can serialize and focused tests can check.

Achievable goal:

```text
Return a versioned proof certificate for each target evaluation.
```

Acceptance:

```text
- certificates include target id, target spec fingerprint, split policy
  fingerprint, active contract identity, and graph/source identity
- coverage proof separates unique coverage from repeated exposure load
- closure proof names the root checkpoint, closure authority, closure facts,
  unresolved inputs, and any identity mismatch
- leakage proof names the protected split, dilation policy, forbidden uses, and
  empty/non-empty intersections
- dependency proof records exact loaded model-state inputs
- warning proof records non-blocking warnings without changing status
- node-support proof is present when node exposure facts are involved
- Hero can emit the certificate as JSON
- tests fail if a satisfied target lacks a required certificate section
```

Current status:

```text
Implemented in the V2 worktree. `evaluate_target` and `plan_target` emit
versioned proof certificates with identity, coverage, closure, leakage,
dependency, warning, node-support, and digest/check sections. Focused target
tests assert required sections and fail-closed certificate validation.
```

## V2 Goal 7: Exposure Algebra and Protected-Split Dilation

Make the existing coverage, load, and leakage logic explicit as named algebraic
operations.

Achievable goal:

```text
Formalize exposure coverage as idempotent interval union, exposure load as
additive interval measure, and leakage as protected-split intersection after
Hx/Hf dilation.
```

Acceptance:

```text
- unique coverage is computed from interval union and repeated facts do not
  increase it
- repeated exposure load is additive and remains available for warnings
- target readiness uses coverage, not load, unless explicitly declared
- warnings may use repeated load and optimizer-step density
- protected split footprints are described as dilation by Hx/Hf and optional
  embargo
- tests cover repeated coverage, left-context leakage, future-target leakage,
  and clean read-only validation evaluation
- Hero explanations expose coverage units and load units separately
```

Current status:

```text
Implemented in the V2 worktree. Exposure summaries name
`idempotent_interval_union` for unique coverage and
`additive_interval_measure` for load, protected splits are Hx/Hf dilations, and
Hero JSON exposes coverage/load units separately. Exposure and target tests
cover repeated facts, repeated load, protected-split leakage, and clean
read-only validation evaluation.
```

## V2 Goal 8: Labeled Causal Closure

Model checkpoint lineage as labeled reachability over runtime jobs, exposure
facts, and checkpoint facts.

Achievable goal:

```text
Make checkpoint cleanliness a labeled causal-path query, not only a path/interval
scan.
```

Acceptance:

```text
- closure records edges such as observed_input, target_supervision,
  evaluation_metric, selection_signal, created_checkpoint, loaded_checkpoint,
  and mutated_component
- leakage checks distinguish read-only evaluation from optimizer mutation and
  checkpoint selection
- forbidden protected-split paths into a checkpoint fail closed
- unresolved producer or loaded-checkpoint edges fail closed
- Hero explains the causal path that satisfies or blocks cleanliness
- tests cover clean eval, mutating validation use, selector leakage, missing
  producer evidence, and inherited upstream leakage
```

Current status:

```text
Implemented in the V2 worktree. Checkpoint closure records causal uses and
checkpoint edges, distinguishes read-only evaluation from mutation and
selection-signal paths, prefers checkpoint id/file digest authority, and fails
closed on unresolved or mismatched producer edges. Focused exposure/target tests
cover clean eval, mutating holdout use, selector leakage, missing producer
evidence, and inherited upstream leakage.
```

## V2 Goal 9: Deficit Vectors for Planning

Keep planning read-only, but make missing proof obligations explicit and
deterministic.

Achievable goal:

```text
Return a deficit vector for each unsatisfied target and derive suggested waves
from that vector.
```

Acceptance:

```text
- deficits include coverage, closure, lineage, leakage, dependency,
  node-support, warning, and identity dimensions where applicable
- each deficit records required value, observed value, and blocking evidence
- suggested waves identify the highest-priority missing proof obligation
- suggested waves remain advice only and do not execute runtime work
- Hero reports why a target is blocked without collapsing dimensions into a
  scalar score
- tests cover at least one coverage deficit, checkpoint deficit, leakage
  deficit, and dependency deficit
```

Current status:

```text
Implemented in the V2 worktree. Unsatisfied target results include structured
deficit vectors with dimension, priority, required/observed values, and blocking
evidence. Suggested waves are deterministic advice derived from those deficits
and never execute runtime work. Focused target tests cover coverage,
checkpoint/closure, leakage, dependency, warning, node-support, and identity
deficits.
```

## V2 Goal 10: Unit-Typed Target Values

Prevent target DSL clauses from mixing fractions, cursor epochs, counts, losses,
and rates.

Achievable goal:

```text
Add unit metadata and validation for lattice target numeric fields.
```

Acceptance:

```text
- coverage fractions are constrained to [0,1]
- cursor epochs, counts, and optimizer steps are non-negative
- rates such as steps_per_cursor_epoch are not accepted where fractions are
  required
- loss/calibration metrics are not accepted where coverage/load units are
  required
- compiler or loader diagnostics name the offending field and expected unit
- existing valid targets load unchanged or with explicit compatible defaults
- tests cover incompatible unit, out-of-range fraction, and valid legacy syntax
```

Current status:

```text
Implemented in the V2 worktree. Target numeric dimensions distinguish coverage
fractions, cursor epochs, counts, optimizer effort, rates, loss/NLL, and
calibration fractions. Loader diagnostics name the offending field and unit, and
focused target tests cover invalid units, out-of-range fractions, negative
counts/rates, and valid legacy syntax.
```

## V2 Goal 11: Node-Support Matrices and Balance Warnings

Generalize node-support visibility from lists of facts into per-node/per-use
support summaries.

Achievable goal:

```text
Build node-support matrices for MDN and future representation support facts,
then emit balance warnings without making them hard readiness gates.
```

Acceptance:

```text
- support summaries are indexed by node and exposure use
- MDN matrices include target_supervision and evaluation_metric support when
  facts are present
- future representation matrices use NodeLift/representation facts, not MDN
  backfill
- summaries expose weakest node, minimum support, maximum support, and a balance
  metric such as coefficient of variation or Gini
- warnings are visible but do not alter V2 readiness status by default
- Hero exposes matrix summaries compactly
- tests cover balanced support, missing-node support, and imbalanced support
```

Current status:

```text
Implemented in the V2 worktree. MDN node exposure facts are summarized by node
and exposure use, include weakest-node, Wilson support, coefficient of
variation, Gini, and entropy metrics, and drive non-blocking
node_support_balance/node_support_floor warnings. Shared-representation support
uses NodeLift/representation support facts, not MDN backfill, and Hero exposes
weakest-node plus projection-row balance through representation_support_summary.
Focused target and exposure tests cover balanced, missing-node, and imbalanced
support.
```

## V2 Goal 12: Representation Geometry Warnings

Inspect shared representation checkpoints for collapse-like geometry symptoms
without turning the lattice into a performance judge.

Achievable goal:

```text
Add optional VICReg representation geometry warning facts and lattice warnings.
```

Acceptance:

```text
- geometry facts may report variance floor, covariance off-diagonal penalty,
  effective rank, condition number, isotropy, and norm distribution summaries
- missing geometry facts do not fail V2 readiness unless a target explicitly
  requires them
- low-rank or collapsed-dimension symptoms surface as warnings first
- metrics bind to checkpoint identity and active contract identity
- Hero distinguishes geometry warnings from validation performance
- tests cover clear geometry, warning geometry, and identity mismatch
```

Current status:

```text
Implemented in the V2 worktree as warning-only representation health/geometry
evidence. VICReg reports expose loss components, projection support,
finite-parameter checks, effective rank, condition number, isotropy, variance
floor, and related health fields. Hero exposes representation geometry
vocabulary/summary separately from validation performance, and focused target
tests cover clear warnings, warning-triggering geometry, missing facts, and
identity mismatch.
```

## V3-A: Read-Only Audit Index Query Layer

Objective:

```text
Promote the V2 rebuildable index/cache from status-only visibility to a
queryable read-only audit surface, while preserving live runtime evidence as the
source of truth.
```

Acceptance:

```text
- runtime files and sidecars remain the source of truth
- Lattice Hero does not write index files, evidence, or runtime state
- audit queries can filter index rows by relation, key, key substring, digest,
  and digest prefix
- valid stored-cache query answers are proven against live-scan query answers
- missing, stale, or mismatched cache rows fall back to live scan
- stale or mismatched cache rows never upgrade target satisfaction
- fixture tests cover cache query, query/live parity, and tampered-cache failure
- active runtime Hero smoke shows query answer parity against live scan
- explicit unproven-cache fast path is opt-in, marked unproven, and never
  upgrades target satisfaction
```

Current status:

```text
Implemented in the V3-A worktree. The exposure ledger exposes
`lattice_runtime_index_query_t`, query results, relation counts, and
cache/live parity proofs. `hero.lattice.index_query` is read-only and reports
cache validation, stored-cache parity, selected-answer parity, result source,
and filtered rows. `validation_strength=header_only` avoids the recursive
runtime metadata digest for interactive cache inspection, and
`allow_unproven_cache=true` with `compare_live_scan=false` enables a fast audit
query that is explicitly marked unproven. Target evaluation still does not use
cache rows for satisfaction.
```

## V3-B: Lattice Performance Fast Paths

Objective:

```text
Keep the lattice read-only while removing repeated proof work from interactive
audit paths.
```

Acceptance:

```text
- multi-target readiness can evaluate several targets from one runtime scan
- long-lived Hero sessions reuse a watched-file-guarded exposure scan
- index validation has header_only, watched_file_manifest, and full metadata
  strengths
- cache files carry row_set_digest and relation_counts for cheap internal
  integrity checks
- checkpoint closure uses lookup indexes instead of repeated full fact scans
- checkpoint file digests stream bytes instead of loading whole checkpoint files
- the exposure scanner exposes scan options for tools that do not need every
  derived fact family
- tests cover cache integrity, watched validation, stale failure, and
  header-only structural validation
```

Current status:

```text
Complete in the V3-B worktree. `hero.lattice.evaluate_targets` batches target
evaluation behind one scan. The runtime index cache now stores
watched_file_metadata_digest, row_set_digest, and relation_counts. Validation
can run in `header_only`, `watched_file_manifest`, or
`full_runtime_metadata_digest` mode. Long-lived Hero processes reuse a session
scan guarded by watched runtime metadata. Checkpoint closure uses ledger lookup
maps, and checkpoint byte digests are streamed. The scanner exposes
exposure_scan_options_t and a per-job artifact bundle so future tools can avoid
unneeded fact derivation.
```

Direct-CLI benchmark on `2026-05-23` against
`/cuwacunu/.runtime/cuwacunu_exec`:

```text
status                         mean 355.600 ms
evaluate_target_channel_mdn    mean 491.600 ms
evaluate_targets_batch_5       mean 982.600 ms
index_status_full              mean 152.600 ms
index_status_watched           mean 162.200 ms
index_status_header            mean  40.000 ms
index_query_parity             mean 789.600 ms
index_query_fast_watched       mean 175.000 ms
index_query_fast_header        mean  36.400 ms
checkpoint_closure             mean 342.000 ms
```

Interpretation:

```text
- row queries are not the bottleneck
- proof/parity modes intentionally remain slower because they rebuild or compare
  live evidence
- the practical interactive fast path is:
  compare_live_scan=false
  allow_unproven_cache=true
  validation_strength=header_only
- batch target evaluation cuts repeated scan cost when agents need several
  readiness targets at once
- watched_file_manifest is a freshness mode, not a substitute for the explicit
  unproven header-only fast lane
```

Future performance receipts must keep these timing layers separate:

```text
library function benchmark:
  exposure/index/evaluator functions without process startup

long-lived MCP benchmark:
  one Hero process, repeated tool calls, session cache behavior visible

direct CLI benchmark:
  process startup, policy load, argument parsing, and one tool call
```

Do not open a broad performance goal unless it names the slow path, records the
baseline timing layer, and defines the expected proof-boundary behavior.

## Next Finite Dispatch Goals

These are the only active roadmap items. Each item is bounded enough to become
one `/goal`. Complete them in order unless the user explicitly promotes a later
item.

Recommended dependency order:

```text
V3-C before V3-D and V3-F:
  performance evidence policy must exist before calibration or comparison can
  make stronger claims.

V3-D before any hard MDN calibration gate:
  diagnostics and sample/uncertainty reporting come before thresholds.

V3-E complete after V3-B:
  the derived-query pilot reads existing proof results and index/cache rows.

V3-F after V3-C, preferably after V3-D:
  Pareto comparison needs explicit performance/calibration dimensions before it
  can compare them honestly.

V3-G is independent unless promoted into proof authority:
  source-key audits stay auxiliary while row-index intervals remain authority.

V3-H waits for observed geometry-warning history:
  no hard representation geometry gate should be proposed from invented
  thresholds.

V3-I is optional after V3-A/V3-B:
  SQLite may replace the cache file format only as a rebuildable read model.

V3-J complete before adding or changing Hero MCP tool schemas:
  schema compatibility is a harness-safety gate.

V3-K should run before pruning or compacting runtime evidence:
  retention policy must preserve replayable proof authority.

V3-L can run after V3-B and after any new fast-path change:
  benchmarks must preserve separate library, long-lived MCP, and direct-CLI
  timing layers and must name the proof mode being measured.
```

### V3-C: Validation Performance Evidence Policy

Objective:

```text
Define validation performance evidence without turning V3 into an automatic
deployment or model-quality authority.
```

Acceptance:

```text
- validation performance facts name metric family, split, checkpoint identity,
  evaluated checkpoint binding, sample count, and uncertainty method
- first supported metrics are finite, explicit, and documented
- point estimates are reported with uncertainty bounds
- target syntax distinguishes readiness evaluation from performance acceptance
- selection-signal leakage policy is checked before any performance result can
  satisfy a target
- missing uncertainty, wrong checkpoint binding, stale identity, and selector
  leakage fail closed
- default posture is warning/visibility unless a target explicitly opts into a
  performance gate
```

Out of scope:

```text
- automatic deployment approval
- scalar ranking across checkpoints
- DB source-of-truth promotion
```

Current status:

```text
Implemented in the V3-C worktree. Hero exposes
validation_performance_evidence_policy_vocabulary and summary rows that bind
finite validation metrics to split, checkpoint identity, exact evaluated
checkpoint binding, sample counts, uncertainty methods, target syntax,
selection-leakage policy, and fail-closed cases. V3-C grants no active
performance-gate authority: available point estimates are bounded or
warning-only, mean NLL remains warning-only until uncertainty is emitted, and
TARGET_CLASS=validation_performance fails closed until an explicit future gate
is implemented.
```

### V3-D: MDN Distribution Calibration Diagnostics

Objective:

```text
Expose distributional calibration diagnostics for MDN validation runs as
read-only warning evidence before hard gates are considered.
```

Acceptance:

```text
- diagnostics include a finite first set such as NLL, PIT histogram summary,
  predictive interval coverage, tail coverage, and per-node calibration
- diagnostics bind to exact evaluated MDN checkpoint, representation
  checkpoint, split policy, active identity, and validation split
- diagnostics carry sample counts and uncertainty where applicable
- Hero explains calibration warnings separately from readiness status
- tests cover clean diagnostics, missing diagnostics, wrong checkpoint binding,
  and warning-triggering calibration
```

Out of scope:

```text
- hard calibration gates by default
- choosing thresholds without observed validation history
- replacing checkpoint lineage or leakage checks
```

Current status:

```text
Implemented in the V3-D worktree. Hero exposes
mdn_distribution_calibration_diagnostic_vocabulary and summary rows for a
finite warning-only diagnostic set: aggregate mean NLL, channel max NLL,
horizon max NLL, per-node max NLL, PIT KS, predictive interval coverage error,
tail coverage error, and calibration slope error. Available NLL diagnostics
bind to exact evaluated MDN checkpoint, representation checkpoint, split policy,
active identity, validation split, sample count, uncertainty method, and
warning-only effect. Future PIT/interval/tail/slope rows remain unavailable
until runtime emits samples and uncertainty, and no V3-D row grants a hard
calibration or performance gate.
```

### V3-E: Datalog-Style Derived Query Pilot

Objective:

```text
Add a small read-only derived-query layer over existing lattice facts and cache
rows, without making the query engine an executor or source of truth.
```

Acceptance:

```text
- derived relations are named and versioned
- initial rules cover a finite set such as satisfied target, checkpoint
  ancestor, forbidden overlap, stale cache, and unresolved lineage
- every derived row points back to concrete runtime facts or target results
- unsafe or incomplete joins fail closed
- Hero can explain the rule and concrete witnesses for each derived result
- tests cover true derivation, false derivation, stale cache rejection, and
  missing witness rejection
```

Out of scope:

```text
- replacing target evaluation
- writing evidence to a DB
- executing waves from query output
```

Current status:

```text
Implemented in the V3-E worktree. `hero.lattice.derived_query` answers a finite
read-only rule set over existing target results, checkpoint closure, leakage
proofs, and runtime-index cache validation: target_satisfied,
checkpoint_ancestor, forbidden_overlap, stale_cache, and unresolved_lineage.
Each result includes the versioned rule vocabulary row, result source, concrete
witnesses, fail-closed flags, and explicit confirmation that cache rows are not
used for target satisfaction. Missing checkpoint ancestry witnesses, unchecked
leakage/closure proofs, unresolved lineage, and stale or missing cache evidence
remain non-authoritative and fail closed. The direct Hero smoke receipt is
`src/tests/fixtures/kikijyeba/lattice_operational_readiness_v3_e/DERIVED_QUERY_AUDIT.md`.
```

### V3-F: Pareto Evidence Comparison

Objective:

```text
Compare clean satisfying checkpoints by evidence vectors without collapsing the
lattice into one scalar score.
```

Acceptance:

```text
- comparison dimensions are explicit: cleanliness, closure completeness,
  coverage, validation evidence, support balance, warnings, and unresolved
  lineage
- dominance relation is partial and reports incomparable cases
- latest_satisfying remains deterministic and separate from best/nondominated
  views
- selection-signal leakage is checked before a checkpoint can participate
- Hero reports the vector and dominance reason
- tests cover left dominates, right dominates, equivalent, incomparable, and
  selector-leaked exclusion
```

Out of scope:

```text
- scalar lattice_score
- automatic model deployment
- hidden performance weighting
```

Current status:

```text
Implemented in the V3-F worktree. `hero.lattice.compare_evidence` evaluates
two targets from one runtime scan and compares their evidence_order_vector rows
with the same Pareto partial order used by the target layer. The output reports
both vectors, clean-checkpoint participation/exclusion reasons, per-dimension
dominance rows, relation/dimension vocabularies, and an explicit dominance
reason. Selection-signal leakage excludes a checkpoint before dominance, and
`latest_satisfying` remains a deterministic readiness selector separate from
best/nondominated views. The surface emits no scalar lattice score and makes no
deployment decision. The focused V3-F receipt is
`src/tests/fixtures/kikijyeba/lattice_operational_readiness_v3_f/PARETO_COMPARISON_AUDIT.md`.
```

### V3-G: Source-Key Map Audit

Objective:

```text
Strengthen source-key and row-to-source-key audits while keeping row-index
intervals as the coverage/leakage authority unless explicitly promoted later.
```

Acceptance:

```text
- source-key audit records monotonicity, order preservation, affine-step
  consistency, gaps, and irregular-key warnings
- audit results bind to source cursor, graph order, and source receipt facts
- row-index intervals remain the proof authority for coverage and leakage
- non-monotone or inconsistent source-key maps warn or block only under an
  explicit target rule
- tests cover monotone affine keys, irregular monotone keys, nonmonotone keys,
  and row/source-key mismatch
```

Out of scope:

```text
- replacing row-index overlap truth
- treating source-key strings as contract identity
- using source receipts as coverage authority
```

Current status:

```text
Not started. Source-key coordinate policy exists as audit vocabulary, but
runtime source-key maps are not yet checked for monotonicity, affine-step
consistency, gaps, or row/source-key mismatch in a dedicated audit.
```

### V3-H: Representation Geometry Gate Promotion Review

Objective:

```text
Decide whether warning-only representation geometry should become an explicit
target gate, using observed warning evidence and negative tests.
```

Acceptance:

```text
- review existing geometry warning distributions from real VICReg runs
- propose finite thresholds only when they are justified by observed data
- any hard gate remains opt-in target syntax
- missing geometry facts do not silently satisfy a gate
- tests cover missing geometry, clear geometry, warning geometry, and hard-gate
  failure when explicitly enabled
```

Out of scope:

```text
- making all VICReg readiness depend on geometry by default
- treating geometry as validation performance
- inventing thresholds without evidence
```

Current status:

```text
Not started. Representation geometry remains warning-only visibility. Promotion
requires observed distributions, opt-in target syntax, and missing/warning
negative tests.
```

### V3-I: SQLite Read-Model Cache Format

Objective:

```text
Evaluate and, only if justified by measured need, replace or supplement the
`.lls` runtime index cache with a SQLite read-model cache while preserving
runtime files and sidecars as the source of truth.
```

Acceptance:

```text
- schema records runtime_root, cache schema version, watched metadata digest,
  row_set_digest, relation counts, source-of-truth flags, and build timestamp
- row table supports relation/key/digest lookups without changing target
  satisfaction behavior
- rebuild writes a temporary database in one transaction and publishes it
  atomically
- corrupt, stale, mismatched, or partial databases fail closed or fall back to
  live scan according to the selected proof mode
- cache rows never write evidence and never upgrade target satisfaction
- benchmarks compare `.lls` cache, SQLite cache, long-lived MCP, and direct CLI
  timing layers
- tests cover fresh cache query, stale metadata rejection, row-set digest
  mismatch, authority-flag mismatch, and live-scan fallback
```

Out of scope:

```text
- DB as source of truth
- runtime components writing directly to the DB
- target satisfaction from DB rows alone
- replacing runtime reports, facts, or sidecars
```

Current status:

```text
Not started and optional. V3-B showed row queries are not the current
bottleneck; this packet should only be dispatched if a measured cache-format or
query-indexing need appears.
```

### V3-J: Hero MCP Schema Compatibility Hygiene

Objective:

```text
Prevent Hero MCP tool schemas from breaking Codex or other MCP harnesses.
```

Acceptance:

```text
- Hero MCP schema generation rejects top-level oneOf, anyOf, allOf, enum, and
  other harness-incompatible root parameter shapes
- every Hero tool parameter schema has top-level type object
- schema smoke covers Config, Runtime, Hashimyei, and Lattice catalogs
- regression covers hero_lattice_checkpoint_closure and any other tool that
  previously emitted an incompatible schema
- failure output names the tool, schema path, and offending construct
- docs state that schema hygiene is a harness-safety gate, not a lattice proof
  condition
```

Out of scope:

```text
- changing lattice evidence semantics
- changing runtime execution policy
- adding new tool behavior only to satisfy schema tests
```

Current status:

```text
Implemented in the V3-J worktree. `hero/mcp_schema_compat.h` validates MCP
tool parameter schemas before sourced Hero catalogs are emitted, requiring
top-level `type=object` and rejecting top-level oneOf, anyOf, allOf, enum, and
not. Failure messages include the tool name, schema path, offending construct,
and message. The focused runtime bench `test_hero_mcp_schema_compat` covers
bad sample schemas, generated Config, Runtime, Hashimyei, and Lattice tool
catalogs, and the `hero.lattice.checkpoint_closure` regression. This remains a
harness-safety gate only; it does not change lattice evidence semantics or
runtime execution policy.
```

### V3-K: Evidence Retention And Compaction Policy

Objective:

```text
Define how runtime evidence can be archived or compacted without losing the
ability to replay lattice proof authority.
```

Acceptance:

```text
- policy classifies reports, sidecars, checkpoints, source receipts, selection
  signals, proof certificates, cache rows, and human receipts by retention role
- compact receipts are explicitly audit metadata, not replacement proof
  authority, unless a future proof format is promoted with negative tests
- retention rules preserve enough runtime evidence to recompute target status,
  checkpoint closure, leakage witnesses, and warning summaries
- pruning refuses or warns when required upstream lineage would become
  unresolved
- archive manifests bind to active identity, split policy, source cursor,
  graph order, and checkpoint identity where applicable
- tests or fixture audits cover complete archive replay, missing lineage after
  pruning, stale cache after archive movement, and compact receipt
  non-authority
```

Out of scope:

```text
- deleting live runtime evidence by default
- treating PASS.md or compact receipts as source of truth
- production backup/restore scheduling
```

Current status:

```text
Not started. Runtime evidence is still treated as durable source material; no
pruning or compaction should be done under lattice authority until this packet
is implemented.
```

### V3-L: Lattice Benchmark And Regression Budget

Objective:

```text
Keep lattice performance work finite by recording timing layers, proof modes,
and regression guardrails for interactive audit paths.
```

Acceptance:

```text
- benchmark library-level lattice calls without process startup
- benchmark long-lived MCP calls with session scan reuse
- benchmark direct CLI calls separately from library and MCP timings
- each timing row records proof mode: header_only, watched_file_manifest,
  full_runtime_metadata_digest, live_scan, or live_parity
- committed receipt records runtime root, active identity, row counts, target
  counts, relation counts, and baseline timings
- regression smoke catches accidental full live scans on header-only fast audit
  queries
- proof/parity modes may remain slower, but their cost is named and separated
  from fast audit mode
- no target satisfaction path is changed to trust cache rows as authority
```

Out of scope:

```text
- broad or indefinite performance optimization
- hiding proof/parity cost behind one aggregate timing
- replacing runtime files or sidecars as source of truth
- using benchmarks as model-quality or deployment evidence
```

Current status:

```text
Not started. V3-B added fast paths and one direct-CLI benchmark receipt, but
there is not yet a committed regression budget that tracks library, long-lived
MCP, and direct-CLI timing layers separately.
```

## Not Yet Dispatchable

Do not open goals for these until a user explicitly narrows them into a finite
packet with acceptance criteria:

```text
- production scheduler behavior inside Lattice Hero
- DB as source of truth
- automatic checkpoint deployment
- broad performance optimization beyond the finite V3-L benchmark/regression
  packet
- full Datalog engine replacement for target evaluation
- source-key proof authority replacing row-index proof authority
- automated evidence pruning or compaction without a retention policy
```
