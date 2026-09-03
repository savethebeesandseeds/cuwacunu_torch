# IMA-5 Protocol Amendment A1 — Explicit CUDA Device Index

## Incident

The first invocation of the frozen IMA-5A audit stopped before seed capture
with the exact guarded error:

```text
support_permitted_teacher_target_alignment_error=outer augmentation model device is not cuda:0
```

The temporary failure log has SHA-256
`dcfb095f2de71965eefccb86b7fc2c4fb99c75d12203bb316dc3dcf4cdd79ace`.
No authoritative IMA-5A log was created.  The failure occurred while
serializing the frozen checkpoint configuration, before a teacher fit,
candidate-target measurement, predictor update, encoder update, or EMA update.
It therefore consumed zero scientific attempts.

## Root cause and bounded correction

The audit constructed `torch::Device(torch::kCUDA)`, whose index is
unspecified.  The inherited frozen FSPA/OCA checkpoint manifest correctly
requires the explicit device `cuda:0`, as every authoritative predecessor
harness uses.

The only authorized source correction is:

```cpp
torch::Device(torch::kCUDA)
    -> torch::Device(torch::kCUDA, 0)
```

The candidate definition, data rows, captures, seeds, thresholds, metrics,
regularization grid, optimization schedule, confirmation seal, and all stop
gates remain byte-unchanged.  The corrected source must bind this amendment's
checksum, pass the CPU self-test, receive a new source checksum, and then may
rerun IMA-5A once.

