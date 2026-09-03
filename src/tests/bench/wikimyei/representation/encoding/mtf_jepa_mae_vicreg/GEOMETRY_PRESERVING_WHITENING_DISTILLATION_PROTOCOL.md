# FSPA-3 — Geometry-Preserving Whitening Distillation Protocol

Status before execution: **sealed design; no FSPA-3 endpoint observed**.

## Frozen question

FSPA-2 proved that direct sequence-projection training improves clean semantic
quality in every seed and passes every RMC semantic and control gate. Its sole
failure was covariance participation rank in structured channels 0 and 2.

FSPA-3 asks one narrow question: can that already-useful representation be
spread across a healthier covariance geometry without discarding its sequence
information?

## Boundary and invariants

The complete experiment remains:

```text
sequence -> encoder -> structured_cdsb_sparse_v1 -> fixed lightweight probe
```

It constructs no MDN, graph, observer, policy, execution, downstream, or
end-to-end component. It uses no task label for representation training or for
the whitening fit. Outer augmentation remains neutral and is never called.

Architecture, normalization, data rows, masks, three model seeds `17,31,47`,
row schedule, probe splits, ridge grid, shuffle controls, bootstrap table, raw
control, readout policy, and every RMC threshold remain unchanged. No threshold
may be relaxed after observing an endpoint.

## Teacher

For each seed, reproduce FSPA-2 exactly: identical initialization, fixed
orthonormal raw-history projection, direct MSE, Adam `1e-3`, batch size `96`,
and exactly `1,024` updates. Require exact initial-AULC reproduction and all
FSPA-2 mechanics. This is the teacher checkpoint; it is not reselected.

Capture the teacher's `[256,3,32]` structured output on the SSL rows exactly
once. No probe, development, confirmation, or label row participates in the
whitening fit.

## Fixed whitening transform

Fit one transform independently for each seed and channel using CPU float64.
For SSL matrix `X` with 256 rows:

1. `mu = mean(X, rows)`;
2. `S = (X-mu)^T (X-mu) / 255`;
3. compute `S = V diag(lambda) V^T`;
4. set `floor = 1e-4 * max(lambda)`;
5. set `W = V diag(max(lambda, floor)^(-1/2)) V^T`;
6. define `whiten(X) = (X-mu) W`.

Require finite means, matrices, eigenvalues, outputs, strictly positive floor,
and full matrix rank under the same floor. The transform is fixed after the SSL
fit and is applied identically to original and reversed clean features.

## Stage A — no-training shadow

Apply the fixed transform to the teacher's structured representations on the
existing train, validation, and development rows. Evaluate the complete,
unchanged RMC learned-gain, family, raw-control, reversal, shuffle, and geometry
gate against the identical untrained initialization.

If the shadow fails any gate, stop. Do not train a student and do not open
confirmation. This means the proposed geometry repair is not information
preserving under the frozen evaluation.

## Stage B — bounded encoder distillation

Only if every seed's Stage-A shadow collectively passes the RMC development
gate, continue from the exact teacher checkpoint. Cache the whitened SSL output
as a detached target. Optimize the same representation model for exactly
`512` additional updates with a fresh Adam `1e-3` optimizer and direct MSE:

```text
structured_cdsb_sparse_v1(encoder(sequence)) ~= cached_whitened_teacher_output
```

The row schedule restarts at step zero and remains identical across the frozen
seed rule. There is no trainable adapter or head. The final served object is the
ordinary encoder output through the unchanged `structured_cdsb_sparse_v1`
readout; whitening is therefore absorbed into encoder parameters and is not a
runtime postprocessor.

Require 512 completed updates, finite nonzero gradients and served-parameter
updates, lower last-eight than first-eight loss, exact target shape and mask,
and exact inactivity of predictor, MAE-decoder, and VICReg-head parameters.

## Decision and confirmation stop gate

Apply the complete unchanged RMC gate to the untransformed final student output
at development.

- If it fails, do not open confirmation. Report the exact failed dimensions.
- If it passes, open the untouched confirmation rows once and apply the same
  gate with no further training or selection.
- Only a confirmation pass yields
  `representation_certified_fspa3_whitened_distillation_v1`.

Failure of either shadow or student does not invalidate FSPA-2's established
semantic result; it means only that this bounded geometry repair did not finish
the certificate.
