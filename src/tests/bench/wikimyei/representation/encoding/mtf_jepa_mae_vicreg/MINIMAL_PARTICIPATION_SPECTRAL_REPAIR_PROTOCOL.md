# FSPA-4 — Minimal Participation Spectral Repair Protocol

Status before execution: **sealed design; no FSPA-4 endpoint observed**.

## Frozen question

FSPA-2 produced a useful representation but missed the `0.25` participation
rank floor in channels 0 and 2. FSPA-3 proved that full whitening is excessive:
it made geometry nearly isotropic but reduced sample-efficient semantic access.

FSPA-4 asks whether the two requirements can coexist when only the dominant
covariance directions are attenuated, by the smallest label-free amount needed
for a modest safety margin above the frozen geometry threshold.

## Unchanged isolation boundary

```text
sequence -> encoder -> structured_cdsb_sparse_v1 -> fixed lightweight probe
```

No MDN, graph, observer, policy, execution, downstream, or end-to-end component
may be constructed. No task label participates in representation training or
in the spectral fit. Outer augmentation remains neutral with zero calls.

Architecture, normalization, data rows, masks, seeds `17,31,47`, row schedule,
probe splits, ridge grid, shuffle controls, bootstrap table, raw control,
readout, and every RMC threshold remain frozen. FSPA-2 teacher training remains
exactly 1,024 direct-alignment updates. Conditional student distillation remains
exactly 512 updates with a fresh Adam `1e-3` optimizer.

## SSL-only minimal spectral cap

Capture each teacher's `[256,3,32]` SSL representation exactly once. For each
seed and channel, compute the centered CPU-float64 covariance with divisor 255
and eigenvalues `lambda`.

The precommitted SSL participation target is **0.30**. This is the frozen `0.25`
gate plus a `0.05` distribution-shift margin. It is not tuned against any probe
or development endpoint.

If the original SSL participation is already at least `0.30`, use identity.
Otherwise find, by 64 deterministic bisection iterations, the largest spectral
cap `c` such that `min(lambda,c)` has participation at least `0.30`. Renormalize
the capped eigenvalues to preserve the original covariance trace:

```text
lambda_capped = min(lambda, c)
lambda_target = lambda_capped * sum(lambda) / sum(lambda_capped)
W = V diag(sqrt(lambda_target / lambda)) V^T
repair(X) = mu + (X-mu) W
```

Require finite positive eigenvalues, a finite positive cap, a full-rank
transform, and preserved mean and covariance trace within `1e-9` relative
error. A repaired channel must achieve SSL participation in
`[0.30,0.300001]`; an identity channel must retain its original value at or
above `0.30`. This construction leaves weak directions unchanged up to one
common trace-preserving scale and alters only eigenvalues above the maximal
admissible cap.

## Stage A — mandatory no-training shadow

Apply the fixed repair to the teacher's original and reversed clean features
and evaluate the complete unchanged RMC development gate against identical
initialization. If any semantic, raw-control, reversal, shuffle, or geometry
condition fails, stop without student training or confirmation.

## Stage B — conditional distillation

Only after a collective Stage-A pass, cache the repaired SSL output as a
detached target and continue from the exact teacher checkpoint for 512 served-
encoder-only updates:

```text
structured_cdsb_sparse_v1(encoder(sequence)) ~= cached_repaired_teacher_output
```

There is no trainable adapter or head. The final output is the unchanged
structured readout, with the repair absorbed into encoder parameters. Require
all FSPA-3 student mechanics: exact shapes and masks, finite nonzero gradients
and updates, lower last-eight than first-eight loss, served parameters changed,
and predictor, MAE-decoder, VICReg-head, and target-EMA parameters unchanged.

## Decision

Apply the complete unchanged RMC development gate to the final student. Open
the untouched confirmation rows once only if development passes. A confirmation
pass yields `representation_certified_fspa4_minimal_spectral_repair_v1`.

If the shadow fails, conclude that this probe-access/geometry tradeoff is not
resolved by a fixed covariance coordinate repair. If the shadow passes but the
student fails, conclude that the representation is compatible with the target
geometry but the bounded encoder absorption step remains unresolved.
