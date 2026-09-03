# IMA-4B Protocol Amendment A2 — Checkpoint-Index Correction

## Review finding

Independent post-run review found that the A1 harness stored the selected
checkpoint as an update number, then recovered its validation-array entry with
`selected_step / 64`.  That mapping is valid through step 512 but not after A1
changed the later validation spacing to 128 updates.

The v1a1 run was outcome-equivalent but is superseded.  Its selected
checkpoints were all step 320 or 448, where the mapping is correct, and every
later validation checkpoint was worse.  Nevertheless, final custody requires
correct code on all possible result paths.  The preserved v1a1 log is:

```text
.build/tests/representation_ima4b_v1a1_superseded_selection_index.log
SHA-256 106e77b544952ee12f39aa378ea7ce18caab7489c070832669371ed3478ac5a7
```

## Required correction and rerun

- Store `selected_validation_index` explicitly for each arm.
- Compare each new validation metric with that exact array entry.
- Update the stored step and index together when a checkpoint wins.
- Rerun both arms from the frozen FSPA-4 initialization for the complete A1
  1,024-update budget.  Reuse no trained tensor or checkpoint.

No architecture, data, mask, seed, optimizer, learning rate, clipping, batch,
validation schedule, metric, bootstrap, or primary verdict rule changes.

## Explicit C-exceedance rule

The base protocol names `linear_C_underestimated_predictability` but did not
numerically define “materially exceeds C.”  For completeness, that verdict
requires all three conditions:

- mean `TRUE_SLOT_ATTENTION - C >= 0.02`;
- the contrast is positive in all three seeds; and
- the paired two-sided 95% interval lower bound is above zero.

This clarification cannot affect the observed IMA-4B result because treatment
was below C in every seed.

All base-protocol and A1 custody, compute-closure, and stopping rules remain
binding.  The v1a2 rerun is final; no further continuation is authorized.
