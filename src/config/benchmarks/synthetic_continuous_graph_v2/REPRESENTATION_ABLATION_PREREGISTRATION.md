# Representation ablation preregistration

Status: fixed on 2026-07-16 before the canonical v2 representation training
run completed and before any trained or untrained v2 representation probe,
production forecast, validation metric, or certified-development metric was
available. The raw-history production-MDN isolation was already known.

This protocol is conditional. It is used only if the canonical trained raw
96-value encoder affine decoder fails the strong information-preservation gate
on validation. If that decoder passes the strong validation gate, none of the
representation ablations below is run; localization proceeds downstream.

## Fixed scientific motivation

For a 30-step history, the canonical time scales `8,16,32,64` generate two
content-identical full-history windows at scales 32 and 64. Each time token
contains only window mean and standard deviation; it contains no ordered raw
samples, endpoint, or slope. Frequency tokens retain magnitude but not complex
phase, and the time-frequency alignment loss explicitly aligns time latents to
that phase-poor branch. `USE_RAW_RECONSTRUCTION_TARGETS=true` reconstructs
token descriptors, not the raw sequence.

Prior v1 evidence found that broad outer-augmentation removal, VICReg-view
removal, and VICReg-gradient removal did not improve task decodability. Those
arms are therefore excluded from this first v2 screen.

## Fixed challenger set

Every challenger starts from the sealed canonical v2 representation policy,
uses seed 17, 3,000 optimizer steps, train anchors `[0,2496)`, and no forecast
labels during representation training. Exactly one declared field changes:

1. `endpoint_scale`: `TIME_SCALES=8,16,32,64` becomes `8,16,32,1`.
   Strides remain `4,8,16,32`. The scale-1 plan contributes exact first and
   current-endpoint tokens while preserving model dimensions and nearly the
   same token count.
2. `time_only`: `USE_FREQUENCY_TOKENS=true` becomes `false`. This tests the
   system effect of removing phase-blind frequency tokens from global mixing
   and downstream pooling.
3. `no_tf_alignment`: `LAMBDA_TF_ALIGN=0.10` becomes `0.00`. Frequency tokens
   and their computation remain present, isolating explicit alignment
   pressure.

No other augmentation, loss, architecture, seed, step, batch, split, or
optimizer field may change.

## Evaluation and selection

For the canonical checkpoint and all three challengers, Runtime captures the
raw `representation_edge_features.probe` on train `[0,2496)` and validation
`[2560,2816)` only. Each arm independently uses the already fixed affine
decoder, train-only standardization, ridge grid, and validation selection
order. No challenger sees certified development during this screen.

The checkpoint selected for one certified confirmation is the arm with the
lexicographically best validation metrics: highest direction, then highest
pairwise rank, then highest correlation, then lowest RMSE, using tolerance
`1e-12`. An exact tie prefers, in order, canonical, `endpoint_scale`,
`time_only`, then `no_tf_alignment`. There is no refit. Certified development
`[2880,3264)` is captured once for that selected arm only, under an immutable
attempt receipt and sources frozen before exposure.

Passing still requires direction, pairwise rank, and correlation at least
`0.95` and RMSE no greater than `0.25` target RMS on both validation and the
single certified confirmation. Direction at least `0.80` and rank at least
`0.78` is partial preservation only. A challenger below the strong gate may
identify a relative improvement, but it cannot establish representation
success or authorize final evaluation.

No ablation reads, scores, fits, tunes, or authorizes access to `[3328,4096)`.
