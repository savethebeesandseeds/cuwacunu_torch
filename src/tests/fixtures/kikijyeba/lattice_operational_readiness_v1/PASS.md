# Lattice Operational Readiness V1 PASS

This is the audit receipt for the completed Lattice Operational Readiness V1
gate. It records the real runtime evidence chain that proved the lattice can
serve as a read-only evidence authority for readiness, checkpoint lineage,
holdout leakage protection, validation evaluation, warnings, and MDN node
support.

This is not a portable runtime fixture. The referenced `/cuwacunu/.runtime`
artifacts remain runtime evidence, not source-tree evidence.

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
vicreg_train_core_ready = e27413efbda3bd67
node_mdn_train_core_ready = 366e19011d81b265
node_mdn_train_core_no_validation_leakage = 68a554ba3eb472e6
node_mdn_train_core_no_test_leakage = 0f7dddb0b6579478
node_mdn_validation_eval_ready = 5a28ce61330acf2d
```

## Target Results

```text
vicreg_train_core_ready = satisfied
node_mdn_train_core_ready = satisfied
node_mdn_train_core_no_validation_leakage = satisfied
node_mdn_train_core_no_test_leakage = satisfied
node_mdn_validation_eval_ready = satisfied
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

## MDN Node Support

Train-core node exposure:

```text
BTC: routed=1600 active=1600 trained=1600 evaluated=0 valid_target_count=4622 valid_target_fraction=0.320972
USDT: routed=1600 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
ETH: routed=1600 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
```

Validation node exposure:

```text
BTC: routed=250 active=250 trained=0 evaluated=250 valid_target_count=685 valid_target_fraction=0.304444
USDT: routed=250 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
ETH: routed=250 active=0 trained=0 evaluated=0 valid_target_count=0 valid_target_fraction=unknown
```

## Warning Summary

```text
anchor_domain_warning_level = none
accepted_anchor_fraction = 0.999555
skipped_failed_fetch_probe = 0
node_mdn_train_core_ready.warning = optimizer_steps_per_cursor_epoch over train_core is 25 above threshold 10
```

The optimizer-effort warning is non-blocking. It did not change target status,
planning, or attempt accounting.

## Commands Verified

```text
PASS: make -C src/main/hero build-lattice-hero -j12
PASS: make -C src/tests/bench/kikijyeba/lattice_exposure run -j12
PASS: make -C src/tests/bench/kikijyeba/lattice_target run -j12
PASS: hero.runtime.status showed allow_execute=false and allow_train_execute=false
PASS: hero.lattice.status showed 3 exposure facts, 6 node exposure facts, and 2 checkpoint facts
PASS: hero.lattice.checkpoint_closure showed complete=true and unresolved_input_checkpoints=[]
```
