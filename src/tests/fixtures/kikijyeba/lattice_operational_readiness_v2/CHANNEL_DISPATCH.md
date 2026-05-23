# Channel V2 Dispatch Note

Generated: 2026-05-23T13:37:36Z

This note records the channel evidence dispatch that closed the remaining V2
gap after the node V2 Core PASS.

## Final Runtime State

Runtime Hero reports:

```text
runtime_root = /cuwacunu/.runtime/cuwacunu_exec
job_count = 6
default_dry_run = true
allow_execute = false
allow_train_execute = false
allow_dev_nuke = false
```

The runtime root was preserved. Existing node evidence was not deleted or
dev-nuked. Runtime Hero was temporarily enabled only for the bounded channel
execution chain and then restored to conservative policy.

The active wave file remains on the final validation-eval wave:

```text
wave_id = cwu_01v_channel_validation_eval_mdn_1800_2050
target_component = wikimyei.inference.expected_value.mdn
mode = run|debug
action = run
job_kind = channel_inference_mdn
anchor_index_begin = 1800
anchor_index_end = 2050
train_target = false
```

The repository MDN checkpoint input fields are empty defaults after execution.
Exact model-state bindings are proven from runtime evidence, not by leaving
checkpoint paths in persistent config.

## Execution Decision

For the actual dispatch, Runtime Hero was intentionally opened with:

```text
default_dry_run = true
execute argument dry_run = false
allow_execute = true
allow_train_execute = true
confirm_execute = true
```

Lattice Hero remained read-only for the full run. It was used only for status,
scan, planning, and target evaluation.

## Executed Chain

1. Channel VICReg train-core:

```text
TARGET = wikimyei.representation.encoding.vicreg
MODE = train|debug
SOURCE_RANGE = anchor_index
ANCHOR_INDEX_BEGIN = 0
ANCHOR_INDEX_END = 1600
job_id = cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg
status = completed
checkpoint_written = true
optimizer_steps = 25
```

Re-evaluation:

```text
vicreg_train_core_ready = satisfied
proof_certificate_check.passed = true
checkpoint_id = d8db43d817356ac5
checkpoint_file_digest = afcf62a4d2a8b75a
closure_complete = true
```

2. Channel MDN train-core:

```text
TARGET = wikimyei.inference.expected_value.mdn
MODE = train|debug
SOURCE_RANGE = anchor_index
ANCHOR_INDEX_BEGIN = 0
ANCHOR_INDEX_END = 1600
PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready
loaded_representation_checkpoint = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg/channel_representation.report.vicreg.pt
job_id = cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn
status = completed
checkpoint_written = true
optimizer_steps = 25
```

Re-evaluation:

```text
channel_mdn_train_core_ready = satisfied
proof_certificate_check.passed = true
checkpoint_id = a9d9082a1c03bcba
checkpoint_file_digest = cc44e4cf307ef47a
closure_complete = true

channel_mdn_train_core_no_validation_leakage = satisfied
proof_certificate_check.passed = true
protected_range = [1771,2051)
overlap_found = false

channel_mdn_train_core_no_test_leakage = satisfied
proof_certificate_check.passed = true
protected_range = [2071,2248)
overlap_found = false
```

3. Channel validation eval:

```text
TARGET = wikimyei.inference.expected_value.mdn
MODE = run|debug
SOURCE_RANGE = anchor_index
ANCHOR_INDEX_BEGIN = 1800
ANCHOR_INDEX_END = 2050
PLAN_INPUT_MDN_CHECKPOINT = latest_satisfying:channel_mdn_train_core_no_test_leakage
PLAN_INPUT_REPRESENTATION_CHECKPOINT = latest_satisfying:vicreg_train_core_ready
loaded_mdn_checkpoint = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn/channel_inference.report.channel_mdn.pt
loaded_representation_checkpoint = /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_vicreg_0000_1600.train.channel_representation_vicreg/channel_representation.report.vicreg.pt
job_id = cwu_01v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn
status = completed
optimizer_steps = 0
checkpoint_written = false
checkpoint_write_count = 0
lattice_checkpoint_fact_written = false
```

Re-evaluation:

```text
channel_mdn_validation_eval_ready = satisfied
proof_certificate_check.passed = true
mdn_checkpoint_match = true
representation_checkpoint_match = true
closure_complete = true
unresolved_input_checkpoints = []
```

## Guardrails Verified

```text
- completed channel evidence exists under /cuwacunu/.runtime/cuwacunu_exec
- dry_run/running/failed rows do not count as readiness evidence
- channel targets did not satisfy from stale node evidence
- validation eval used the trained channel MDN, not a fresh MDN
- validation eval used the exact MDN checkpoint selected by the no-test-leakage guard
- validation eval used the exact satisfying channel VICReg checkpoint
- validation eval did not mutate model state and wrote no checkpoint
- checkpoint closure has no unresolved lineage
- checkpoint input fields and ALLOW_UNTRAINED_REPRESENTATION do not alter protocol contract identity
- source receipts remain audit-only
- representation_support remains visibility-only
- Lattice Hero did not execute waves
```

All channel evidence binds to:

```text
protocol_contract_fingerprint = 8aaff747e54c7c4f
graph_order_fingerprint = 507df53170ffab6a
```

## Follow-up Scope

This dispatch is complete. It records channel runtime execution only. Later
ROADMAP.md V2 work may close DB/index or mathematical proof items in the PASS
and completion audit, but those are not channel execution events and should not
be folded into this dispatch note.
