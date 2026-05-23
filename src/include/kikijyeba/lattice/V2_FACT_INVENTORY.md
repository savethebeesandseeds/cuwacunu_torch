# Lattice V2 Fact Inventory

Snapshot date: 2026-05-23

This file is the pre-V2 gate from `ROADMAP.md`. It records what the current
Wikimyei reports and lattice runtime dumps emit today, and it separates durable
fact candidates from diagnostics and future proof authority.

The fact-emission boundary is recorded separately in:

```text
src/include/kikijyeba/lattice/FACT_EMISSION_CONTRACT.md
```

Do not treat this inventory as V2 completion. It is the bounded review artifact
that lets V2 be dispatched without expanding indefinitely.

## Current Tree Status

Config Hero validates the active global config:

```text
global_config_path = /cuwacunu/src/config/.config
configured path entries = 29
missing configured paths = 0
```

The active Lattice Hero target DSL now compiles after separating node and
channel VICReg target/profile names:

```text
target_count = 20
split_count = 4
exposure_fact_count = 6
node_exposure_fact_count = 12
checkpoint_fact_count = 4
source_receipt_fact_count = 54
selection_signal_fact_count = 0
representation_support_fact_count = 2
```

Current scan warnings are legacy-sidecar warnings:

```text
- exposure sidecar digest differs from the fact derived from current runtime
  artifacts
- legacy sidecars are supplemented with inference/representation health from
  the component reports
```

This means V1 proof authority still comes from runtime files plus sidecars, and
the scanner preserves compatibility by supplementing old sidecars from current
reports. V2 now makes that compatibility policy explicit: the original sidecar
`fact_digest` is retained as a checkpoint lineage identity candidate, so
diagnostic supplementation does not demote a valid checkpoint fact to path-only
closure authority.

## Runtime Dumps Reviewed

The active runtime root inspected here is:

```text
/cuwacunu/.runtime/cuwacunu_exec
```

Current jobs under that root:

```text
cwu_01v_train_core_vicreg_0000_1600.train.representation_vicreg
cwu_01v_train_core_mdn_0000_1600.train.inference_mdn
cwu_01v_validation_eval_mdn_1800_2050.run.inference_mdn
cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg
cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn
cwu_01v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn
```

Each job has `job.manifest`, `job.state`, a component report, component `.lls`
debug sidecars, and a `lattice.exposure.fact`. The four train jobs also have a
`lattice.checkpoint.fact`; the two validation eval jobs do not write
checkpoints.

The current active config points to the migrated unified Wikimyei paths for
node and channel-capable VICReg/MDN code. The runtime dump contains both the
legacy node evidence and the completed V2 channel evidence chain.

The active wave currently remains on the final channel validation eval recipe.
The rebuilt direct Runtime Hero preview decodes it as
`job_kind=channel_inference_mdn`, `action=run`, and `train_target=false`. The
regression
`make -C src/tests/bench/kikijyeba/runtime run-test_hero_runtime_wave_preview -j12`
keeps canonical VICReg/MDN targets on channel job kinds while preserving
`legacy_inference_mdn` as the node compatibility target.

The completed channel evidence is:

```text
vicreg_train_core_ready = satisfied
channel_mdn_train_core_ready = satisfied
channel_mdn_train_core_no_validation_leakage = satisfied
channel_mdn_train_core_no_test_leakage = satisfied
channel_mdn_validation_eval_ready = satisfied
proof_certificate_check.passed = true for each target above
protocol_contract_fingerprint = 8aaff747e54c7c4f
vicreg_assembly_fingerprint = 759931dbbd341fcf
mdn_assembly_fingerprint = a4d5971c5c4cbb01
```

Dry-run artifacts remain wiring/audit evidence only and are not included in the
runtime root fact counts above.

## Field Classification

### Runtime Identity

These fields identify the runtime job, component/report family, and active
graph/source context:

```text
training_id
config_path
component_id
graph_order_fingerprint
wave_id
wave_mode
target_action
wave_source_cursor_kind
wave_source_cursor_scope
wave_source_range_policy
requested_anchor_index_begin
requested_anchor_index_end
runtime_report_mode
stream_plan
source_cursor_token
```

Lattice exposure/checkpoint facts normalize the proof-relevant identity as:

```text
contract_fingerprint
graph_order_fingerprint
component_assembly_fingerprint
target_component
job_id
wave_id
job_status
wave_action
cursor_domain
source_cursor_token
```

V2 note: runtime model-state paths must remain outside protocol contract
identity. They are evidence only after reports/facts prove what was loaded or
written.

### Model-State Input And Output

MDN reports currently emit:

```text
representation_checkpoint_path
representation_checkpoint_loaded
mdn_checkpoint_path
mdn_checkpoint_loaded
allow_untrained_representation
checkpoint_written
checkpoint_write_count
last_checkpoint_optimizer_step
checkpoint_path
checkpoint_format
```

Representation reports currently emit:

```text
checkpoint_written
checkpoint_write_count
last_checkpoint_optimizer_step
checkpoint_path
checkpoint_format
```

Lattice exposure facts normalize this as:

```text
output_checkpoint
input_checkpoints
mutated_component
optimizer_steps
```

V2 note: these are the correct raw materials for exact checkpoint binding.
They are not contract identity and should not become target identity.

### Exposure Support

Reports expose graph-anchor and source-domain support:

```text
source_anchor_count
source_candidate_anchor_count
source_skipped_anchor_count
source_skipped_outside_common_range
source_skipped_missing_edge_coverage
source_skipped_failed_fetch_probe
source_duplicate_anchor_count
source_accepted_anchor_fraction
source_anchor_domain_warning_level
source_anchor_domain_warnings
source_common_left_key
source_common_right_key
source_reference_left_key
source_reference_right_key
source_first_anchor_key
source_last_anchor_key
wave_streamed_anchor_count
wave_first_anchor_key
wave_last_anchor_key
wave_pulses_attempted
wave_pulses_completed
wave_pulses_skipped
```

Lattice exposure facts add the proof intervals:

```text
anchor_begin
anchor_end
observed_begin
observed_end
target_begin
target_end
footprint_precision
completed_anchor_begin
completed_anchor_end
coverage_precision
source_input_length
source_future_length
use_observed_input
use_target_supervision
use_evaluation_metric
use_selection_signal
```

V2 note: row-index intervals remain the authority for coverage and leakage.
Source-key fields and structured source receipt facts are audit/provenance
coordinates, not coverage, leakage, or contract-identity authority.

### Checkpoint Lineage

Checkpoint facts currently emit:

```text
checkpoint_id
checkpoint_path
checkpoint_file_digest
component
contract_fingerprint
graph_order_fingerprint
source_cursor_token
component_assembly_fingerprint
created_by_job_id
created_by_wave_id
created_by_exposure_fact_id
input_checkpoints
direct_exposure_digest
closure_digest
fact_digest
```

V2 note: `checkpoint_id` and `checkpoint_file_digest` are now the preferred
closure authority when complete checkpoint facts are available and verified.
Path-only closure remains explicit `legacy_path` compatibility for V1 evidence.
Wrong digest or mismatched id must fail closed.

### Source Receipts

Exposure facts currently carry compact receipt strings:

```text
source_file_receipts
```

The compact payload includes edge, instrument, interval, record type, and source
path entries. V2 now derives `kikijyeba.lattice.source_receipt.v1` rows from
that payload during exposure-ledger scans.

The structured receipt facts bind each receipt to the parent exposure fact
digest, contract fingerprint, graph order fingerprint, source cursor token,
split policy fingerprint, component assembly fingerprint, job/wave ids,
anchor/completed ranges, and source-key audit window fields. They are
provenance/audit facts only: row-index intervals remain coverage and leakage
authority, and receipt fields do not alter contract identity.

Receipt scan policy:

```text
valid compact receipts -> structured audit facts
missing receipts -> summary audit absence only
malformed non-empty receipts -> scan warning and malformed summary count
```

### MDN Node Support

MDN reports currently expose node-indexed support:

```text
routed_row_count_by_node
active_row_count_by_node
trained_row_count_by_node
evaluated_row_count_by_node
valid_target_count_by_node
mean_nll_by_node
```

The lattice derives `kikijyeba.lattice.node_exposure.v1` rows from these fields
and can summarize MDN-head support by node.

V2 note: these are MDN support facts only. They must not be reused as VICReg or
NodeLift support evidence.

### NodeLift / Representation Support

Current debug `.lls` sidecars expose useful aggregate support fields:

```text
nodelift:
  observed_node_mask_fraction
  observed_residual_energy_mean
  observed_valid_edge_count_mean
  future_node_mask_fraction
  future_residual_energy_mean
  future_valid_edge_count_mean

representation:
  original_rows
  kept_rows
  dropped_rows
  valid_channel_time_fraction
  kept_valid_channel_time_fraction

representation training:
  valid_projection_rows
  optimizer_steps
  original_rows
  kept_rows
  dropped_rows
```

These are batch/runtime support diagnostics for the shared representation path.
The V2 worktree now promotes the available aggregate payload into
`kikijyeba.lattice.representation_support.v1` facts when representation reports
or NodeLift `.lls` sidecars provide support fields. These facts bind to the
parent representation exposure digest and active contract/graph/source identity,
remain `visibility_only`, and do not reuse downstream MDN node rows.

Channel representation reports can now emit node-indexed shared-representation
support fields:

```text
node_ids
node_support_denominator
node_valid_lifted_row_count
node_valid_lifted_feature_count
node_valid_lifted_cell_any_count
node_valid_lifted_cell_all_count
node_valid_projection_row_count
node_valid_lifted_feature_fraction
```

When those payloads are present, the scanner derives one
`representation_support` fact per node with
`support_scope=shared_representation_node`, links it to the parent
representation exposure digest, and keeps it visibility-only. Hero exposes the
shared-representation weakest node plus projection-row balance metrics
coefficient-of-variation and Gini. The scanner still rejects synthetic VICReg
support backfill from downstream MDN rows.

### Diagnostic Warning Metrics

Representation reports expose health and training diagnostics:

```text
last_loss
mean_loss
last_invariance_loss
mean_invariance_loss
last_variance_loss
mean_variance_loss
last_covariance_loss
mean_covariance_loss
last_grad_norm
max_grad_norm
last_valid_projection_rows
total_valid_projection_rows
adapter_original_rows
adapter_kept_rows
adapter_dropped_rows
adapter_valid_channel_time_count
adapter_kept_valid_channel_time_count
mean_adapter_valid_channel_time_fraction
mean_adapter_kept_valid_channel_time_fraction
finite_parameter_check
```

MDN reports expose inference/training diagnostics:

```text
last_loss
mean_loss
last_valid_target_count
total_valid_target_count
last_valid_row_count
total_valid_row_count
last_skipped_node_head_count
total_skipped_node_head_count
last_active_node_head_count
total_active_node_head_count
last_trained_node_head_count
total_trained_node_head_count
last_evaluated_node_head_count
total_evaluated_node_head_count
last_valid_node_target_fraction
mean_valid_node_target_fraction
mean_node_mask_coverage
mean_future_mask_coverage
mean_price_residual_energy
mean_node_context_norm_mean
max_node_context_norm_max
mean_mixture_entropy_mean
min_mixture_entropy_min
mean_sigma_mean
min_sigma_min
max_sigma_max
finite_parameter_check
```

V1/V2 note: these metrics can support warnings and health visibility. They
should not become performance gates without explicit V3 uncertainty and
selection-leakage policy.

## V2 Gap Register

The current state supports V1 and the V2 pre-inventory, but it does not yet
complete V2.

```text
Stable checkpoint identity:
  checkpoint_id/file_digest authority is implemented for closure; remaining
  audit work is to keep runtime emissions and Hero dumps under review during
  the Wikimyei migration. Live node VICReg and node MDN train-core closures now
  resolve with resolution_authority=checkpoint_id_file_digest and
  legacy_path_fallback=false, even when legacy exposure sidecars are
  supplemented from current reports.

Structured source receipt facts:
  compact source_file_receipts are parsed into structured audit-only receipt
  facts during lattice exposure scans; malformed non-empty receipts warn, while
  missing receipts remain audit absence in the summary.

First-class selection-signal events:
  use_selection_signal remains the causal use label, and the exposure ledger now
  derives structured selection_signal_event facts from it. Each event identifies
  the parent exposure digest, selector id/kind/rule, selected checkpoint path,
  output/input checkpoint paths, active identity, and selector footprint. Known
  selector paths remain read-only evidence and fail closed when the protected
  split forbids selection_signal.

NodeLift / representation node-support facts:
  aggregate and node-indexed representation_support facts now exist for shared
  VICReg support visibility when component reports emit honest payload fields;
  Hero exposes weakest-node and balance summaries, and MDN node rows are still
  rejected as VICReg/NodeLift backfill.

DB/index:
  a rebuildable `kikijyeba.lattice.runtime_index_cache.v1` read model now
  records row digests for scanned exposure, node, checkpoint, source-receipt,
  selection-signal, and representation-support facts. Cache validity is bound to
  runtime file metadata, `hero.lattice.index_status` reports cache versus
  live-scan source, and target satisfaction remains live-scan based.

Channel runtime evidence:
  current runtime evidence is node train-core plus node validation eval. The
  migrated channel targets compile in the direct rebuilt Lattice Hero binary,
  and Runtime Hero's rebuilt direct wave preview now agrees that the active
  train-core VICReg wave is channel_representation_vicreg. A /tmp dry-run
  validates the channel runtime wiring, but vicreg_train_core_ready remains
  missing_report until non-dry-run channel VICReg train-core runtime evidence is
  produced. The dispatch path is recorded in
  src/tests/fixtures/kikijyeba/lattice_operational_readiness_v2/CHANNEL_DISPATCH.md.

Dry-run fact boundary:
  runtime may write lattice.exposure.fact sidecars for dry-run attempts as
  audit/wiring evidence. The lattice does not let dry_run/running/failed rows
  contribute readiness coverage, repeated exposure load, or completed causal
  exposure witnesses. The focused runtime regression
  `make -C src/tests/bench/kikijyeba/runtime run-test_kikijyeba_job_runner`
  covers this boundary and the strict channel representation/MDN smoke path.
```

## Guardrails For V2 Implementation

Use these rules for each V2 goal:

```text
- components report what happened
- lattice facts normalize component/runtime evidence
- lattice targets decide what the evidence proves
- new fact families start as visibility unless a target proof rule is explicit
- no DB/cache row may become source of truth
- no diagnostic or performance metric may affect readiness without explicit
  uncertainty and selection-leakage policy
- runtime model-state paths remain outside contract identity
```
