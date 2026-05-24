# Lattice Operational Readiness V2 PASS

Generated: 2026-05-23T13:37:36Z

This receipt records the completed V2 pass for the accepted runtime root:

```text
runtime_root = /cuwacunu/.runtime/cuwacunu_exec
global_config = /cuwacunu/src/config/.config
```

Runtime Hero was the only executor. Lattice Hero remained read-only: it scanned
runtime/component artifacts and evaluated targets, but did not execute waves.

## Active Identity

Direct Lattice Hero reports:

```text
target_count = 20
split_count = 4
split_policy_fingerprint = 72367e40daf6375a
protocol_contract_fingerprint = 8aaff747e54c7c4f
graph_order_fingerprint = 507df53170ffab6a
vicreg_assembly_fingerprint = 759931dbbd341fcf
mdn_assembly_fingerprint = a4d5971c5c4cbb01
source_cursor_token = version=ujcamei.graph_anchor_cursor_report.v1|graph=507df53170ffab6a|reference_edge=BTCUSDT|Hx=30|Hf=1|edges=3|accepted=2247|candidates=2248|skipped=1|first=1580687999999|last=1774742399999
```

Fact counts under the accepted root:

```text
job_count = 6
exposure_fact_count = 6
node_exposure_fact_count = 12
checkpoint_fact_count = 4
source_receipt_fact_count = 54
selection_signal_fact_count = 0
representation_support_fact_count = 2
```

Source receipts remain audit-only; selection signals remain read-only event
visibility; representation_support remains visibility-only unless a future
target explicitly promotes it.

## Channel Evidence

Channel VICReg train-core:

```text
target = vicreg_train_core_ready
status = satisfied
proof_certificate_check.passed = true
job_dir = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg
checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg/channel_representation.report.vicreg.pt
checkpoint_written = true
optimizer_steps = 25
contract = 8aaff747e54c7c4f
closure_complete = true
resolution_authority = checkpoint_id_file_digest
legacy_path_fallback = false
root_checkpoint_id = d8db43d817356ac5
root_checkpoint_file_digest = afcf62a4d2a8b75a
unresolved_input_checkpoints = []
```

Channel MDN train-core:

```text
target = channel_mdn_train_core_ready
status = satisfied
proof_certificate_check.passed = true
job_dir = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn
checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn/channel_inference.report.channel_mdn.pt
checkpoint_written = true
optimizer_steps = 25
contract = 8aaff747e54c7c4f
representation_checkpoint_loaded = true
representation_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg/channel_representation.report.vicreg.pt
closure_complete = true
resolution_authority = checkpoint_id_file_digest
legacy_path_fallback = false
root_checkpoint_id = a9d9082a1c03bcba
root_checkpoint_file_digest = cc44e4cf307ef47a
unresolved_input_checkpoints = []
```

The MDN train-core target carries one non-blocking warning:

```text
optimizer_steps_per_cursor_epoch over train_core is 25 above threshold 10
```

Leakage guards:

```text
channel_mdn_train_core_no_validation_leakage = satisfied
proof_certificate_check.passed = true
protected_split = validation_holdout
protected_range = [1771,2051)
overlap_found = false
closure_complete = true

channel_mdn_train_core_no_test_leakage = satisfied
proof_certificate_check.passed = true
protected_split = test_holdout
protected_range = [2071,2248)
overlap_found = false
closure_complete = true
```

Channel validation evaluation:

```text
target = channel_mdn_validation_eval_ready
status = satisfied
proof_certificate_check.passed = true
job_dir = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn
checkpoint_written = false
checkpoint_path =
optimizer_steps = 0
contract = 8aaff747e54c7c4f
mdn_checkpoint_loaded = true
mdn_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn/channel_inference.report.channel_mdn.pt
representation_checkpoint_loaded = true
representation_checkpoint_path = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg/channel_representation.report.vicreg.pt
evaluated_checkpoint_source = channel_mdn_train_core_no_test_leakage
mdn_checkpoint_match = true
representation_checkpoint_match = true
closure_complete = true
unresolved_input_checkpoints = []
```

The validation eval wrote no checkpoint fact and did not mutate model state.

## Node Evidence

Historical node V2 Core evidence remains present under the same runtime root
for replay/audit compatibility only. Active V2 readiness is channel-based:

```text
historical_node_vicreg_train_core_ready = satisfied
node_mdn_train_core_ready = satisfied
node_mdn_train_core_no_validation_leakage = satisfied
node_mdn_train_core_no_test_leakage = satisfied
node_mdn_validation_eval_ready = satisfied
```

Direct Lattice Hero reports certificate checks passed for those node targets.
The only scan warnings are legacy-sidecar compatibility warnings for the node
jobs; channel jobs are current V2 runtime artifacts.

## Contract Boundary

Runtime model-state checkpoint inputs remain runtime evidence and do not alter
protocol contract identity. The channel train and validation jobs above all
bind to the same active contract fingerprint:

```text
protocol_contract_fingerprint = 8aaff747e54c7c4f
```

The repository MDN `.jkimyei` checkpoint fields were restored to empty defaults
after execution. The exact loaded checkpoints are proven from runtime reports,
exposure facts, and checkpoint closure, not from persistent config paths.

## Runtime Policy

Runtime Hero policy was restored after execution:

```text
default_dry_run = true
allow_execute = false
allow_train_execute = false
allow_dev_nuke = false
```

## Deferred Items

Roadmap V2 addendum generated at `2026-05-23T19:09:26Z`:

```text
representation_support_fact_count = 2
runtime_index_cache_schema = kikijyeba.lattice.runtime_index_cache.v1
runtime_index_cache_result_source = live_scan
runtime_index_cache_row_count = 78
cache_used_for_target_satisfaction = false
```

This addendum closes the roadmap items that were still deferred when this PASS
receipt originally froze the channel runtime evidence:

```text
- NodeLift / representation support now supports node-indexed facts when
  component reports emit honest per-node support payloads. The accepted runtime
  artifacts predate those per-node payloads, so the active receipt still has two
  aggregate representation_support facts.
- DB/index now exists as a rebuildable runtime index cache over runtime
  artifacts and lattice fact digests. Lattice target evaluation still uses live
  runtime evidence, so stale or missing cache rows never upgrade satisfaction.
```

Still outside V2:

```text
- validation performance gates
- MDN distribution calibration gates
- hard representation geometry gates
- Datalog-style derived query engine
- Pareto/nondominated checkpoint ranking
- source-key/order-preserving map audits as proof authority
```

## Validation Commands

```text
PASS: make -C src/main/exec build-cuwacunu-exec -j12
PASS: make -C src/main/hero clean
PASS: make -C src/main/hero build-runtime-hero build-lattice-hero -j12
PASS: make -C src/tests/bench/kikijyeba/protocol/pipeline_builder all -j12
PASS: make -C src/tests/bench/kikijyeba/protocol/pipeline_builder run-test_kikijyeba_protocol_pipeline_builder -j12
PASS: direct Runtime Hero execute for channel VICReg train-core with dry_run=false and confirm_execute=true
PASS: direct Lattice Hero evaluate_target vicreg_train_core_ready
PASS: direct Runtime Hero execute for channel MDN train-core with dry_run=false and confirm_execute=true
PASS: direct Lattice Hero evaluate_target channel_mdn_train_core_ready
PASS: direct Lattice Hero evaluate_target channel_mdn_train_core_no_validation_leakage
PASS: direct Lattice Hero evaluate_target channel_mdn_train_core_no_test_leakage
PASS: direct Runtime Hero execute for channel validation eval with dry_run=false and confirm_execute=true
PASS: direct Lattice Hero evaluate_target channel_mdn_validation_eval_ready
PASS: direct Runtime Hero status after policy restore
PASS: make -C src/tests/bench/kikijyeba/lattice_exposure run-test_kikijyeba_lattice_exposure -j12
PASS: make -C src/tests/bench/kikijyeba/lattice_target run-test_kikijyeba_lattice_target -j12
PASS: make -C src/tests/bench/jkimyei/training/channel_graph_first_launchers run-test_jkimyei_channel_graph_first_launchers -j12
PASS: make -C src/main/hero build-lattice-hero -j12
PASS: direct Lattice Hero index_status reports live_scan fallback and cache_used_for_target_satisfaction=false
```
