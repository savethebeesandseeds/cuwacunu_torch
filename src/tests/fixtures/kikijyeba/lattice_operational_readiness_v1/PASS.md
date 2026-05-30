# Lattice Operational Readiness V1 PASS

This is the audit receipt for the completed Lattice Operational Readiness V1
gate. It records the real runtime evidence chain that proved the lattice can
serve as a read-only evidence authority for readiness, checkpoint lineage,
holdout leakage protection, validation evaluation, warnings, and MDN node
support.

This is not a portable runtime fixture. The referenced `/cuwacunu/.runtime`
artifacts remain runtime evidence, not source-tree evidence.

Historical note: this V1 receipt predates the channel-only active target DSL and
uses node-MDN target names. Keep it as an audit record; do not treat these names
as active dispatch targets, active fixture inputs, or current Hero runtime-root
policy.

Receipt refreshed after warning availability/direction/summary, affine
source-key audit, evidence-order vocabulary, unit-typing hardening, mathematical
readiness crosswalks, and derived-query projection self-checks on 2026-05-22.

## Scope

Included in the V1 gate:

- train-core VICReg readiness
- train-core node-MDN readiness with exact VICReg checkpoint binding
- validation and test holdout leakage guards
- validation run-mode evaluation with exact MDN and VICReg checkpoint inputs
- checkpoint closure completeness
- anchor-domain health and exposure-load warning visibility
- MDN node exposure previews
- Lattice Hero read/explain/evaluate/plan/scan/closure operator surface

Explicitly deferred beyond V1:

- DB/index
- performance targets
- checkpoint id/file digest as primary closure identity
- structured source receipt facts
- first-class selection-signal events
- VICReg node-support facts

Validation eval readiness remains a clean evaluation proof, not a performance
gate. `node_mdn_validation_eval_ready` proves evaluation coverage, zero
optimizer mutation, exact MDN checkpoint binding, and matching representation
lineage over `validation_holdout`; it does not approve model quality,
deployability, or performance.

Compact `source_file_receipts` remain V1 source-audit metadata. They trace the
runtime facts back to Ujcamei source files, but row-index footprints remain the
coverage/leakage authority and receipt strings do not alter contract identity.
MDN `node_exposure` rows remain V1 MDN-head support visibility. They do not
claim VICReg per-node readiness, and future shared-representation support must
come from NodeLift or representation-emitted support facts rather than
synthetic backfill from downstream MDN rows.

## Runtime Root

```text
runtime_root = /cuwacunu/.runtime/cuwacunu_exec
job_count = 3
exposure_fact_count = 3
node_exposure_fact_count = 6
checkpoint_fact_count = 2
runtime_hero_allow_execute = false
runtime_hero_allow_train_execute = false
```

## Active Identity

```text
protocol_contract_fingerprint = 6b9e26010523a447
graph_order_fingerprint = 507df53170ffab6a
source_cursor_token = version=ujcamei.graph_anchor_cursor_report.v1|graph=507df53170ffab6a|reference_edge=BTCUSDT|Hx=30|Hf=1|edges=3|accepted=2247|candidates=2248|skipped=1|first=1580687999999|last=1774742399999
vicreg_assembly_fingerprint = c64c128291976c40
mdn_assembly_fingerprint = ba6d2c88f6fa3098
split_policy_fingerprint = 72367e40daf6375a
```

## Target Fingerprints

```text
vicreg_train_core_ready = 4609df9bc42fd1ae
node_mdn_train_core_ready = 3b4100744c3790ea
node_mdn_train_core_no_validation_leakage = aa4a6ea0ce298416
node_mdn_train_core_no_test_leakage = 2bd43320566d4ed2
node_mdn_validation_eval_ready = d5ff2be2c386f561
```

These target fingerprints identify the proof obligations only. Advisory
planning knobs, runtime model-state input hints, and non-blocking warning
clauses remain visible in target explanations and suggested waves, but they do
not alter these fingerprints.
The Hero `contract_identity_boundary_vocabulary` records that protocol,
graph/source-cursor, component, and target proof identity are separate from
runtime-loaded checkpoint paths, advisory plan inputs, warning visibility, and
audit-only source receipt/key-window metadata.

## Target Results

```text
vicreg_train_core_ready = satisfied
node_mdn_train_core_ready = satisfied
node_mdn_train_core_no_validation_leakage = satisfied
node_mdn_train_core_no_test_leakage = satisfied
node_mdn_validation_eval_ready = satisfied
```

## Certificate Digests

```text
vicreg_train_core_ready = 730a0f9ae962c2a9
node_mdn_train_core_ready = d00d822089cbbe2a
node_mdn_train_core_no_validation_leakage = d4d8616bfdcf3bac
node_mdn_train_core_no_test_leakage = c5e8e773aeda004e
node_mdn_validation_eval_ready = 5e8b2847f876fd04
```

## Runtime Evidence Chain

### VICReg Train Core

```text
job_id = cwu_01v_train_core_vicreg_0000_1600.train.representation_vicreg
wave_id = cwu_01v_train_core_vicreg_0000_1600
wave_action = train
anchor_range = [0,1600)
coverage_precision = contiguous_completed_range_v1
optimizer_steps = 25
checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_vicreg_0000_1600.train.representation_vicreg/representation.report.vicreg.pt
```

### Node MDN Train Core

```text
job_id = cwu_01v_train_core_mdn_0000_1600.train.inference_mdn
wave_id = cwu_01v_train_core_mdn_0000_1600
wave_action = train
anchor_range = [0,1600)
coverage_precision = contiguous_completed_range_v1
optimizer_steps = 25
valid_target_count = 4622
valid_target_fraction = 0.320972
target_supervision_unique_coverage = 1.0
target_supervision_exposure_load = 1.0
representation_checkpoint_loaded = true
representation_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_vicreg_0000_1600.train.representation_vicreg/representation.report.vicreg.pt
mdn_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_mdn_0000_1600.train.inference_mdn/inference.report.mdn.pt
```

### Node MDN Validation Eval

```text
job_id = cwu_01v_validation_eval_mdn_1800_2050.run.inference_mdn
wave_id = cwu_01v_validation_eval_mdn_1800_2050
wave_action = run
anchor_range = [1800,2050)
coverage_precision = contiguous_completed_range_v1
optimizer_steps = 0
checkpoint_written = false
valid_target_count = 685
valid_target_fraction = 0.305122
evaluation_metric_unique_coverage = 1.0
evaluation_metric_exposure_load = 1.0
mdn_checkpoint_loaded = true
mdn_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_mdn_0000_1600.train.inference_mdn/inference.report.mdn.pt
representation_checkpoint_loaded = true
representation_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_vicreg_0000_1600.train.representation_vicreg/representation.report.vicreg.pt
```

## Checkpoint Closure

```text
checkpoint = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_train_core_mdn_0000_1600.train.inference_mdn/inference.report.mdn.pt
closure_complete = true
closure_fact_count = 2
unresolved_input_checkpoints = []
closure_contains = node-MDN train-core exposure + upstream VICReg train-core exposure
```

## Source-Key Audit

Source-key windows remain audit metadata only. Coverage and leakage authority
still uses row-index intervals, but each causal exposure now carries a structured
row-to-source-key audit:

```text
source_key_window_audit_schema = kikijyeba.lattice.source_key_window_audit.v1
reference_key_step = 86400000
vicreg_train_core_ready.source_key_audit_count = 1
vicreg_train_core_ready.source_key_audit_issue_count = 0
vicreg_train_core_ready.source_key_affine_inconsistent_count = 0
node_mdn_train_core_ready.source_key_audit_count = 2
node_mdn_train_core_ready.source_key_audit_issue_count = 0
node_mdn_train_core_ready.source_key_affine_inconsistent_count = 0
node_mdn_train_core_no_validation_leakage.source_key_audit_count = 2
node_mdn_train_core_no_validation_leakage.source_key_audit_issue_count = 0
node_mdn_train_core_no_validation_leakage.source_key_affine_inconsistent_count = 0
node_mdn_train_core_no_test_leakage.source_key_audit_count = 2
node_mdn_train_core_no_test_leakage.source_key_audit_issue_count = 0
node_mdn_train_core_no_test_leakage.source_key_affine_inconsistent_count = 0
node_mdn_validation_eval_ready.source_key_audit_count = 2
node_mdn_validation_eval_ready.source_key_audit_issue_count = 0
node_mdn_validation_eval_ready.source_key_affine_inconsistent_count = 0
```

## MDN Node Support

Train-core node exposure:

```text
summary: node_count=3 unique_node_count=3 mutating_support_row_count=3 non_mutating_support_row_count=0
BTC: routed=1600 active=1600 trained=1600 evaluated=0 valid_target_count=4622 valid_target_fraction=0.320972
USDT: routed=1600 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
ETH: routed=1600 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
```

Validation node exposure:

```text
summary: node_count=6 unique_node_count=3 mutating_support_row_count=3 non_mutating_support_row_count=3
BTC: routed=250 active=250 trained=0 evaluated=250 valid_target_count=685 valid_target_fraction=0.304444
USDT: routed=250 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
ETH: routed=250 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
```

## Warning Summary

```text
anchor_domain_warning_level = none
accepted_anchor_fraction = 0.999555
skipped_failed_fetch_probe = 0
vicreg_train_core_ready.warning_result_count = 3
vicreg_train_core_ready.triggered_warning_count = 0
vicreg_train_core_ready.unavailable_warning_count = 0
node_mdn_train_core_ready.warning_result_count = 8
node_mdn_train_core_ready.triggered_warning_count = 4
node_mdn_train_core_ready.unavailable_warning_count = 0
node_mdn_train_core_ready.warning_summary.warning_result_count = 8
node_mdn_train_core_ready.warning_summary.triggered_warning_count = 4
node_mdn_train_core_ready.warning_summary.clear_measured_warning_count = 4
node_mdn_train_core_ready.warning_summary.unavailable_warning_count = 0
node_mdn_train_core_ready.warning_summary.relation_counts = above_threshold:1 not_above_threshold:3 below_threshold:3 not_below_threshold:1 unavailable:0 no_threshold:0 unknown_direction:0
node_mdn_train_core_ready.warning_result = high_target_supervision_train_core_load clear evidence_basis=exposure_load_summary threshold_direction=above threshold_triggered=false threshold_relation=not_above_threshold measurement_available=true exposure_summary_available=true node_support_summary_available=false message="target_supervision exposure load over train_core is 1 cursor_epochs, not above threshold 3"
node_mdn_train_core_ready.warning_result = high_mdn_optimizer_effort_density warning evidence_basis=exposure_load_summary threshold_direction=above threshold_triggered=true threshold_relation=above_threshold measurement_available=true exposure_summary_available=true node_support_summary_available=false message="optimizer_steps_per_cursor_epoch over train_core is 25 above threshold 10"
node_mdn_train_core_ready.warning_result = low_mdn_anchor_acceptance_fraction clear evidence_basis=anchor_domain_facts threshold_direction=below threshold_triggered=false threshold_relation=not_below_threshold measurement_available=true exposure_summary_available=true node_support_summary_available=false message="anchor-domain accepted fraction is 0.999555, not below threshold 0.8"
node_mdn_train_core_ready.warning_result = mdn_fetch_probe_skips_present clear evidence_basis=anchor_domain_facts threshold_direction=above threshold_triggered=false threshold_relation=not_above_threshold measurement_available=true exposure_summary_available=true node_support_summary_available=false message="anchor-domain failed fetch probes total 0, not above threshold 0"
node_mdn_train_core_ready.warning_result = imbalanced_mdn_node_target_support clear evidence_basis=filtered_node_support_summary threshold_direction=above threshold_triggered=false threshold_relation=not_above_threshold measurement_available=true exposure_summary_available=false node_support_summary_available=true node_count=3 mutating_support_row_count=3 message="node-support valid_target_count_gini over train_core is 0.666667, not above threshold 0.75; weakest node USDT has valid_target_count 0"
node_mdn_train_core_ready.warning_result = low_mdn_node_target_support_entropy warning evidence_basis=filtered_node_support_summary threshold_direction=below threshold_triggered=true threshold_relation=below_threshold measurement_available=true exposure_summary_available=false node_support_summary_available=true node_count=3 mutating_support_row_count=3 message="node-support valid_target_count_normalized_entropy over train_core is 0 below threshold 0.25; weakest node USDT has valid_target_count 0"
node_mdn_train_core_ready.warning_result = low_mdn_node_target_support_confidence warning evidence_basis=filtered_node_support_summary threshold_direction=below threshold_triggered=true threshold_relation=below_threshold measurement_available=true exposure_summary_available=false node_support_summary_available=true node_count=3 mutating_support_row_count=3 message="node-support valid_target_wilson_lower_95 over train_core is 0.313396 below threshold 0.9; weakest node USDT"
node_mdn_train_core_ready.warning_result = weak_mdn_node_target_support warning evidence_basis=filtered_node_support_summary threshold_direction=below threshold_triggered=true threshold_relation=below_threshold measurement_available=true exposure_summary_available=false node_support_summary_available=true node_count=3 mutating_support_row_count=3 message="node-support weakest_valid_target_count over train_core is 0 below threshold 1; weakest node USDT"
hero.lattice.status.warning = sidecar digest differs from derived runtime artifact fact for VICReg train-core job
hero.lattice.status.warning = historical VICReg sidecar supplemented with representation health from runtime report
```

The optimizer-effort warning, low-confidence/low-entropy/weak-node support
warnings, and Hero scan warnings are non-blocking. Node-support warning metrics
are measured from `filtered_node_support_summary`, specifically the
target-supervision rows that mutated the MDN component. The availability flags
mark which summary envelope is a real warning basis and which is an empty
placeholder. `unavailable_warning_count` is derived from structured
`measurement_available=false`, not message text. Triggered warning counts and
warning-message projection are derived from structured `threshold_triggered`.
`warning_summary` aggregates triggered/clear/unavailable counts and
per-relation counts, and `threshold_relation` records
above/below/clear/unavailable comparison state without parsing messages.
Evidence-order metadata exposes relation and dimension vocabularies from the
same C++ source used by the Pareto comparator. `warning_count` remains a
compatibility alias; comparison uses structured `triggered_warning_count`.
Hero output also exposes `checkpoint_selection_policy_vocabulary`, making
explicit that `latest_satisfying` is deterministic readiness selection over a
referenced satisfied target, not a best-model, Pareto, performance, deployment,
or scalar-score selector.
Hero output also exposes `checkpoint_selection_policy_summary`, making that
boundary self-checking with 4 policy rows, 3 deterministic readiness selectors,
2 exact runtime binding rows, and zero performance/Pareto/identity/executor
authority.
Hero output also exposes `exposure_measure_algebra_vocabulary`, making explicit
that unique coverage is idempotent interval union while repeated exposure load
is additive interval measure.
Hero output also exposes `exposure_measure_algebra_summary`, making that
coverage/load partition self-checking with exactly two measures, distinct units,
and no dual-role measure.
Hero output also exposes `source_key_coordinate_policy_vocabulary`, making
explicit that row-index intervals are authoritative for coverage/leakage while
source-key windows are audit-only order-preserving and affine map checks.
Hero output also exposes `source_key_coordinate_policy_summary`, making that
coordinate boundary self-checking with one row-index coverage/leakage authority
row, three audit-only source-key rows, declared order-preserving/affine fields,
and no source-key audit row with coverage or leakage authority.
Hero output also exposes `source_receipt_policy_summary`, making the receipt
boundary self-checking with one row-index coverage/leakage authority row, three
audit-only receipt rows, no contract identity authority, no structured receipt
facts in V1, and declared compact/future structured receipt fields.
Hero output also exposes `leakage_rule_vocabulary`, making explicit that
leakage is protected-split dilation plus forbidden-use intersection over
complete checkpoint closure.
Hero output also exposes `leakage_rule_summary`, making the single V1 dilation
rule, complete closure requirement, protected-range/base containment, and causal
witness basis self-checking.
Hero output also exposes `causal_edge_vocabulary`, making checkpoint closure
readable as a labeled causal graph over source-row uses, loaded checkpoints,
created checkpoints, and component mutation.
Hero output also exposes `causal_edge_summary`, making those edge families
self-checking with 7 edge labels, 4 source-row use edges, 2 checkpoint
reachability edges, 1 mutation edge, and zero checkpoint/leakage edge overlap.
Hero output also exposes `selection_signal_policy_vocabulary`, making explicit
that V1 treats known `selection_signal` exposure as leakage-relevant when
forbidden but does not yet claim first-class selector event provenance.
Hero output also exposes `selection_signal_policy_summary`, making that
boundary self-checking: known selection-signal is forbidden and causal-anchor
based, selector event streams and arbitrary selector-path proofs remain
future/non-claiming, all rows require closure completeness, and Lattice has no
executor authority.
Hero output also exposes `derived_query_rule_vocabulary`, making the read-only
Datalog-style relations behind identity, dependency, coverage, closure,
leakage, warning, planning, and target satisfaction explicit without giving the
lattice execution authority. Rule rows declare both their source proof projection
shape and the emitted result projection shape, and the live self-check compares
result rows against those declarations.
Hero output also exposes `db_cache_policy_vocabulary`, making explicit that
future DB/cache rows are rebuildable read models over runtime files/facts, not
evidence authority or executors.
Hero output also exposes `plan_advice_scope_policy_vocabulary`, making explicit
that `plan_basis`, `suggested_wave`, and `PLAN_INPUT_*` are advisory deficit
projections only, while `PLAN_MAX_ATTEMPTS`/`MAX_WAVES` guard recommendation
availability and Runtime Hero remains the executor.
Hero output also exposes `deficit_vector_planning_summary`, making explicit that
the planning vector has 12 ordered deficit priority classes, 5 plan-advice
policy rows, zero evidence or contract-identity authority, zero Lattice
execution authority, and one Runtime Hero executor boundary.
Hero output also exposes `operational_readiness_v1_scope_vocabulary`, the
machine-readable V1 scope table: included read-only evidence-authority claims
and intentionally deferred DB/index, performance, checkpoint identity,
structured source receipt, selection-event, and VICReg node-support work.
Hero output also exposes `operational_readiness_v1_scope_summary`, making that
scope boundary self-checking: 12 rows partitioned into 6 included read-only
claims and 6 deferred items, zero runtime-executor/performance/DB authority, and
no empty claim-boundary fields.
Hero output also exposes `operational_readiness_v1_gate_vocabulary` and
`operational_readiness_v1_gate_summary`, making the concrete V1 gate checklist
and its self-check visible: 10 required read-only gates, zero runtime-executor/
performance/DB authority, no empty pass/fail conditions, no empty Hero-surface
rows, and all five required V1 targets referenced.
Hero output also exposes `contract_identity_boundary_vocabulary`, making the
model-state amendment explicit: checkpoint paths prove loaded runtime state and
dependency bindings, but do not split protocol contract or target proof
identity.
Hero output also exposes `contract_identity_boundary_summary`, self-checking
that runtime model-state, plan advice, warning visibility, and audit metadata
have zero overlap with protocol contract or target proof identity while active
identity surfaces are present.
Hero output also exposes `evidence_abstraction_vocabulary`, making the
abstract-interpretation layer from concrete runtime/fact inputs to proof
summaries explicit, including soundness conditions, conservative policies, and
join semantics for future cache/index work, with source receipts modeled as
non-claiming audit traceability.
Hero output also exposes `product_evidence_state_vocabulary`, making the
factorized evidence state explicit: identity, coverage lattice, exposure-load
monoid, closure graph, leakage predicate, metric/warning vector, node-support
matrix, source-receipt audit set, and deficit vector with identity-scoped joins.
Hero output also exposes `join_law_vocabulary`, making safe joins explicit:
coverage/closure/leakage/warning/deficit evidence joins idempotently within one
identity scope, while repeated exposure load and distinct node-support rows are
additive with duplicate and cache requirements.
Hero output also exposes `node_support_scope_policy_vocabulary`, making the
MDN-only node-support scope and future shared-representation support boundary
explicit.
Hero output also exposes `node_support_scope_policy_summary`, confirming four
policy rows, no VICReg per-node readiness authority, no synthetic backfill, and
no runtime executor authority.
Hero output also exposes `representation_geometry_vocabulary`, making VICReg
representation-health and embedding-geometry warning metrics visible with their
units, bad-direction semantics, and V1 non-blocking scope.
Hero output also exposes `representation_geometry_summary`, confirming 18
VICReg health/geometry warning metrics, 7 embedding-geometry metrics, 11 future
hard-gate candidates, and 0 active performance gates.
Hero output also exposes `performance_uncertainty_policy_vocabulary`, making
explicit that future performance gates must compare conservative uncertainty
bounds under declared methods, not raw point estimates, and must account for
selection leakage.
Hero output also exposes `performance_uncertainty_policy_summary`, confirming
the six-row boundary: one V1 support-visibility row, five deferred future
performance-gate rows, no V1 performance-gate authority, no point-estimate
gates, confidence bounds required, and selection-leakage policy required.
Hero output also exposes `mathematical_readiness_v1_vocabulary`, a compact
crosswalk from the mathematical strengthening points to the concrete Hero
surfaces, proof basis, V1 read-only status, scope boundaries, and future work.
Hero output also exposes `mathematical_readiness_v1_summary`, which self-checks
that the 16 mathematical V1 items are included, read-only, non-executing,
non-performance, non-DB-source-of-truth, and backed by Hero surfaces.
Hero output also exposes `mdn_distribution_calibration_vocabulary`, making
explicit that V1 can warn on MDN mean NLL while CRPS, PIT, predictive interval,
tail coverage, and calibration slope metrics are future non-gating checks that
report unavailable until runtime emits them.
Hero output also exposes `mdn_distribution_calibration_summary`, confirming only
mean NLL is V1-visible, five calibration metrics remain future work, all rows
are non-blocking warnings, and none are performance gates.
Hero output also exposes `validation_performance_scope_policy_vocabulary`,
making explicit that validation evaluation readiness is not a V1 performance
gate and future performance targets need explicit metrics, uncertainty, and
selection-leakage policy.
Hero output also exposes `validation_performance_scope_policy_summary`,
confirming three V1 readiness/visibility rows, one deferred future performance
row, no V1 performance-gate authority, uncertainty required for future gates,
and no runtime executor authority.
Hero output also exposes `target_numeric_dimension_vocabulary`, making the DSL
unit/bounds/integrality/threshold-direction table for numeric target and warning
fields explicit.
Hero output also exposes `target_numeric_dimension_summary`, confirming the
table has declared units/kinds, unique context/field pairs, well-formed bounds,
known threshold directions, required V1 rows, and separate coverage/load units.
Hero output also exposes `monotonicity_invariant_vocabulary`, making explicit
that target satisfaction is monotone only under clean evidence growth and that
forbidden overlaps, deficits, warnings, and Pareto tradeoffs are weakening or
incomparable dimensions.
Hero output also exposes `proof_obligation_vocabulary`, mapping certificate
fields to derived relations, required scopes, status effects, deficit kinds,
non-claiming conditions, digest participation, and planner relevance.
Hero output also exposes `proof_certificate_digest_policy_vocabulary`, making
explicit that the required certificate digest binds canonical proof fields while
warnings, deficits, plan advice, evidence-order projections, policy
vocabularies, and the digest field itself remain outside the digest.
Hero output also exposes `proof_certificate_digest_policy_summary`, the compact
self-check that reports 11 policy rows, 8 hashed proof surfaces, 3 excluded
report/policy surfaces, zero advisory digest overlap, and zero visibility-only
status authority.
Hero output also exposes `operational_readiness_v1_gate_vocabulary`, making the
V1 acceptance gates machine-readable: identity binding, train-core readiness,
validation/test leakage guards, validation exact-checkpoint evaluation,
warnings/node-support visibility, and Hero read/plan/closure inspection.
Hero output also exposes `checkpoint_identity_policy_vocabulary`, making
explicit that V1 checkpoint closure authority is path/exposure-fact based,
checkpoint ids/file digests are preview-only audit metadata, exact loaded
checkpoint bindings use runtime paths, and id/digest promotion is future
DB/cache work rather than contract identity.
Hero output also exposes `valid_target_uncertainty_vocabulary`, making Wilson
95% support intervals, success/opportunity field bindings, and the no-trials
non-claiming policy explicit.
Hero output also exposes `valid_target_uncertainty_summary`, making the support
uncertainty boundary self-checking across the three support scopes,
Wilson-95/binomial method, field bindings, no-trials non-claiming policy, and
support-visibility-only status.
Warnings did not change target status, planning,
attempt accounting, proof-obligation identity, or proof certificate self-check
results.

## Proof Certificate Checks

```text
vicreg_train_core_ready.proof_certificate_check = passed
node_mdn_train_core_ready.proof_certificate_check = passed
node_mdn_train_core_no_validation_leakage.proof_certificate_check = passed
node_mdn_train_core_no_test_leakage.proof_certificate_check = passed
node_mdn_validation_eval_ready.proof_certificate_check = passed
```

## Derived Query Projection Checks

`evaluate_target` and `plan_target` expose `derived_query_results` as a
read-only projection of relation truth values from the emitted proof rows.
The projection is not target evidence, does not execute waves, and does not
contribute to the proof certificate digest. It is self-checked so clients can
detect stale or internally inconsistent query summaries before using them as a
future cache/index view.

```text
explain_target.derived_query_rule_vocabulary_digest_schema = kikijyeba.lattice.derived_query_rule_vocabulary.v1
explain_target.derived_query_rule_vocabulary_digest = a51a88c6d4b71706
explain_target.contract_identity_boundary_summary.schema = kikijyeba.lattice.contract_identity_boundary.summary.v1
explain_target.contract_identity_boundary_summary.boundary_count = 8
explain_target.contract_identity_boundary_summary.protocol_contract_identity_count = 1
explain_target.contract_identity_boundary_summary.target_spec_identity_count = 1
explain_target.contract_identity_boundary_summary.active_runtime_identity_count = 3
explain_target.contract_identity_boundary_summary.runtime_model_state_count = 1
explain_target.contract_identity_boundary_summary.plan_advice_count = 1
explain_target.contract_identity_boundary_summary.warning_visibility_count = 1
explain_target.contract_identity_boundary_summary.audit_metadata_count = 1
explain_target.contract_identity_boundary_summary.runtime_model_state_identity_overlap_count = 0
explain_target.contract_identity_boundary_summary.plan_advice_identity_overlap_count = 0
explain_target.contract_identity_boundary_summary.warning_identity_overlap_count = 0
explain_target.contract_identity_boundary_summary.audit_identity_overlap_count = 0
explain_target.contract_identity_boundary_summary.empty_policy_field_count = 0
explain_target.contract_identity_boundary_summary.model_state_is_not_contract_identity = true
explain_target.contract_identity_boundary_summary.advice_warning_audit_not_identity = true
explain_target.contract_identity_boundary_summary.summary_self_check_passed = true
explain_target.contract_identity_boundary_summary.summary_issue_count = 0
explain_target.operational_readiness_v1_scope_summary.schema = kikijyeba.lattice.operational_readiness_v1.scope_summary.v1
explain_target.operational_readiness_v1_scope_summary.item_count = 12
explain_target.operational_readiness_v1_scope_summary.included_in_v1_count = 6
explain_target.operational_readiness_v1_scope_summary.deferred_beyond_v1_count = 6
explain_target.operational_readiness_v1_scope_summary.included_and_deferred_count = 0
explain_target.operational_readiness_v1_scope_summary.read_only_evidence_authority_count = 6
explain_target.operational_readiness_v1_scope_summary.runtime_executor_authority_count = 0
explain_target.operational_readiness_v1_scope_summary.performance_gate_authority_count = 0
explain_target.operational_readiness_v1_scope_summary.db_source_of_truth_authority_count = 0
explain_target.operational_readiness_v1_scope_summary.empty_claim_field_count = 0
explain_target.operational_readiness_v1_scope_summary.included_deferred_partition_complete = true
explain_target.operational_readiness_v1_scope_summary.included_items_are_read_only = true
explain_target.operational_readiness_v1_scope_summary.summary_self_check_passed = true
explain_target.operational_readiness_v1_scope_summary.summary_issue_count = 0
explain_target.operational_readiness_v1_gate_summary.schema = kikijyeba.lattice.operational_readiness_v1.gate_summary.v1
explain_target.operational_readiness_v1_gate_summary.gate_count = 10
explain_target.operational_readiness_v1_gate_summary.required_for_v1_count = 10
explain_target.operational_readiness_v1_gate_summary.read_only_evidence_authority_count = 10
explain_target.operational_readiness_v1_gate_summary.runtime_executor_authority_count = 0
explain_target.operational_readiness_v1_gate_summary.performance_gate_authority_count = 0
explain_target.operational_readiness_v1_gate_summary.db_source_of_truth_authority_count = 0
explain_target.operational_readiness_v1_gate_summary.empty_hero_surface_count = 0
explain_target.operational_readiness_v1_gate_summary.empty_pass_condition_count = 0
explain_target.operational_readiness_v1_gate_summary.empty_fail_condition_count = 0
explain_target.operational_readiness_v1_gate_summary.unique_target_id_count = 5
explain_target.operational_readiness_v1_gate_summary.all_v1_targets_referenced = true
explain_target.operational_readiness_v1_gate_summary.summary_self_check_passed = true
explain_target.operational_readiness_v1_gate_summary.summary_issue_count = 0
explain_target.mathematical_readiness_v1_summary.schema = kikijyeba.lattice.mathematical_readiness_v1.summary.v1
explain_target.mathematical_readiness_v1_summary.item_count = 16
explain_target.mathematical_readiness_v1_summary.included_in_v1_count = 16
explain_target.mathematical_readiness_v1_summary.deferred_beyond_v1_count = 0
explain_target.mathematical_readiness_v1_summary.read_only_count = 16
explain_target.mathematical_readiness_v1_summary.runtime_executor_count = 0
explain_target.mathematical_readiness_v1_summary.performance_gate_count = 0
explain_target.mathematical_readiness_v1_summary.db_source_of_truth_count = 0
explain_target.mathematical_readiness_v1_summary.empty_hero_surface_count = 0
explain_target.mathematical_readiness_v1_summary.summary_self_check_passed = true
explain_target.mathematical_readiness_v1_summary.summary_issue_count = 0

node_mdn_train_core_ready.derived_query_results.schema = kikijyeba.lattice.derived_query_results.v1
node_mdn_train_core_ready.derived_query_results.rule_vocabulary_digest_schema = kikijyeba.lattice.derived_query_rule_vocabulary.v1
node_mdn_train_core_ready.derived_query_results.result_projection_digest_schema = kikijyeba.lattice.derived_query_results_projection.v1
node_mdn_train_core_ready.derived_query_results.projection_target_id = node_mdn_train_core_ready
node_mdn_train_core_ready.derived_query_results.projection_target_spec_fingerprint = 3b4100744c3790ea
node_mdn_train_core_ready.derived_query_results.projection_certificate_digest = d00d822089cbbe2a
node_mdn_train_core_ready.derived_query_results.projection_status = satisfied
node_mdn_train_core_ready.derived_query_results.projection_basis_complete = true
node_mdn_train_core_ready.derived_query_results.projection_basis_field_count = 4
node_mdn_train_core_ready.derived_query_results.result_projection_digest_rule_vocabulary_bound = true
node_mdn_train_core_ready.derived_query_results.result_projection_digest_basis_field_count = 6
node_mdn_train_core_ready.derived_query_rule_vocabulary_digest_schema = kikijyeba.lattice.derived_query_rule_vocabulary.v1
node_mdn_train_core_ready.derived_query_rule_vocabulary_digest = a51a88c6d4b71706
node_mdn_train_core_ready.derived_query_results.summary_source = emitted_result_rows
node_mdn_train_core_ready.derived_query_results.summary_self_check_passed = true
node_mdn_train_core_ready.derived_query_results.summary_issue_count = 0
node_mdn_train_core_ready.derived_query_results.result_count = 15
node_mdn_train_core_ready.derived_query_results.rule_vocabulary_digest = a51a88c6d4b71706
node_mdn_train_core_ready.derived_query_results.result_projection_digest = 9938b7b75505b04a
node_mdn_train_core_ready.derived_query_results.result_projection_digest_basis includes rule_vocabulary_digest_schema and rule_vocabulary_digest
node_mdn_train_core_ready.derived_query_results.unique_result_relation_count = 15
node_mdn_train_core_ready.derived_query_results.rule_vocabulary_relation_count = 15
node_mdn_train_core_ready.derived_query_results.result_covers_rule_vocabulary = true
node_mdn_train_core_ready.derived_query_results.duplicate_result_relation_count = 0

node_mdn_validation_eval_ready.plan.derived_query_results.schema = kikijyeba.lattice.derived_query_results.v1
node_mdn_validation_eval_ready.plan.derived_query_results.rule_vocabulary_digest_schema = kikijyeba.lattice.derived_query_rule_vocabulary.v1
node_mdn_validation_eval_ready.plan.derived_query_results.result_projection_digest_schema = kikijyeba.lattice.derived_query_results_projection.v1
node_mdn_validation_eval_ready.plan.derived_query_results.projection_target_id = node_mdn_validation_eval_ready
node_mdn_validation_eval_ready.plan.derived_query_results.projection_target_spec_fingerprint = d5ff2be2c386f561
node_mdn_validation_eval_ready.plan.derived_query_results.projection_certificate_digest = 5e8b2847f876fd04
node_mdn_validation_eval_ready.plan.derived_query_results.projection_status = satisfied
node_mdn_validation_eval_ready.plan.derived_query_results.projection_basis_complete = true
node_mdn_validation_eval_ready.plan.derived_query_results.projection_basis_field_count = 4
node_mdn_validation_eval_ready.plan.derived_query_results.result_projection_digest_rule_vocabulary_bound = true
node_mdn_validation_eval_ready.plan.derived_query_results.result_projection_digest_basis_field_count = 6
node_mdn_validation_eval_ready.plan.derived_query_rule_vocabulary_digest_schema = kikijyeba.lattice.derived_query_rule_vocabulary.v1
node_mdn_validation_eval_ready.plan.derived_query_rule_vocabulary_digest = a51a88c6d4b71706
node_mdn_validation_eval_ready.plan.derived_query_results.summary_source = emitted_result_rows
node_mdn_validation_eval_ready.plan.derived_query_results.summary_self_check_passed = true
node_mdn_validation_eval_ready.plan.derived_query_results.summary_issue_count = 0
node_mdn_validation_eval_ready.plan.derived_query_results.result_count = 15
node_mdn_validation_eval_ready.plan.derived_query_results.rule_vocabulary_digest = a51a88c6d4b71706
node_mdn_validation_eval_ready.plan.derived_query_results.result_projection_digest = 185bb413cc8629b3
node_mdn_validation_eval_ready.plan.derived_query_results.result_projection_digest_basis includes rule_vocabulary_digest_schema and rule_vocabulary_digest
node_mdn_validation_eval_ready.plan.derived_query_results.unique_result_relation_count = 15
node_mdn_validation_eval_ready.plan.derived_query_results.rule_vocabulary_relation_count = 15
node_mdn_validation_eval_ready.plan.derived_query_results.result_covers_rule_vocabulary = true
node_mdn_validation_eval_ready.plan.derived_query_results.duplicate_result_relation_count = 0
```

## Commands Verified

```text
PASS: make -C src/main/hero build-lattice-hero -j12
PASS: make -C src/tests/bench/wikimyei/config/graph_first_specs run -j12
PASS: make -C src/tests/bench/kikijyeba/lattice_exposure run -j12
PASS: make -C src/tests/bench/kikijyeba/lattice_target run -j12
PASS: hero.runtime.status showed allow_execute=false and allow_train_execute=false
PASS: hero.lattice.status showed 3 exposure facts, 6 node exposure facts, and 2 checkpoint facts
PASS: hero.lattice.checkpoint_closure showed complete=true and unresolved_input_checkpoints=[]
PASS: hero.lattice.evaluate_target for all five V1 targets showed proof_certificate_check.passed=true
PASS: hero.lattice.evaluate_target and hero.lattice.plan_target showed derived_query_results.summary_self_check_passed=true
PASS: derived_query_results.projection_basis_complete=true with projection_basis_field_count=4
PASS: derived_query_results.result_projection_digest_rule_vocabulary_bound=true with result_projection_digest_basis_field_count=6
PASS: derived_query_results summary self-check validates vocabulary coverage, count/quantifier semantics, projection-scope vocabulary, declared result projection fields, rule/result projection-shape compatibility, empty-policy compatibility, row basis fields, and projection semantics vocabulary values
PASS: contract_identity_boundary_summary self-check shows 8 boundaries, active/proof identity present, runtime model-state/advice/warning/audit identity-overlap counts all zero, and no empty policy fields
PASS: operational_readiness_v1_scope_summary self-check shows 12 rows partitioned into 6 included read-only claims and 6 deferred items, zero runtime-executor/performance/DB-authority rows, and no empty claim-boundary fields
PASS: operational_readiness_v1_gate_summary self-check shows 10 required read-only gates, zero runtime-executor/performance/DB-authority gates, all five V1 targets referenced, and no empty pass/fail or Hero-surface rows
PASS: mathematical_readiness_v1_summary self-check shows 16 included read-only items, zero runtime-executor/performance/DB-authority items, and zero empty Hero-surface rows
PASS: focused certificate regression rejects missing certificate_digest
PASS: focused certificate regression rejects concrete named-split evidence without split-policy identity
PASS: focused evaluator regression rejects concrete named-split coverage evidence without split-policy identity while preserving historical unknown-split evidence
```
