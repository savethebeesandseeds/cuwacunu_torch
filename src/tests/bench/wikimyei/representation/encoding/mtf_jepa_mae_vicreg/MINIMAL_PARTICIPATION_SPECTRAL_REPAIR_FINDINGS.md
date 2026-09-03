# FSPA-4 — Minimal Participation Spectral Repair Findings

Date executed: 2026-08-28

## Decision

FSPA-4 passed the frozen development gate and the untouched confirmation gate.
The isolated representation recipe is certified as:

```text
representation_certified_fspa4_minimal_spectral_repair_v1
```

The final certified object is the ordinary encoder output through
`structured_cdsb_sparse_v1`. The spectral transform was used only to define a
detached, label-free distillation target and was absorbed into encoder weights;
there is no runtime adapter or downstream component.

## Stage A: the minimal shadow worked

Every SSL-only spectral fit was finite, positive, full-rank, and preserved mean
and covariance trace to approximately `1e-15`. Each channel reached the sealed
SSL participation target `0.30`. Cap-to-maximum-eigenvalue ratios ranged from
`0.2681` to `0.9098`, so the repair changed only the oversized directions and
was far smaller than FSPA-3 full whitening.

The no-training shadow passed the complete RMC development gate:

- mean AULC: `0.645843`;
- shadow-minus-initialization: `+0.052782`, 95% interval
  `[+0.039468,+0.066977]`, three of three seeds positive;
- shadow-minus-raw: `+0.043557`, interval
  `[+0.028413,+0.058414]`;
- reversal AULC: `0.948730`;
- learned family changes: `-0.005252`, `+0.024299`, `+0.089888`,
  `+0.102191`;
- all semantic, raw, reversal, shuffle, and geometry gates passed.

This resolved FSPA-3's apparent tradeoff: the useful representation and the
required covariance geometry can coexist when the correction is minimal.

## Stage B: the encoder absorbed the repair

All three bounded 512-step student mechanics receipts passed:

| seed | first-eight MSE | last-eight MSE | served max change |
|---:|---:|---:|---:|
| 17 | 0.047419 | 0.002473 | 0.135763 |
| 31 | 0.032072 | 0.002940 | 0.114236 |
| 47 | 0.029040 | 0.003198 | 0.143145 |

Gradients and updates were finite and nonzero. Predictor, MAE-decoder,
VICReg-head, and target-EMA parameters remained exactly unchanged.

The untransformed student output passed development:

| seed | initialization AULC | student AULC |
|---:|---:|---:|
| 17 | 0.603103 | 0.628304 |
| 31 | 0.583349 | 0.647031 |
| 47 | 0.592733 | 0.649311 |

Mean student AULC was `0.641549`. Student-minus-initialization was `+0.048487`
with interval `[+0.034669,+0.062732]`; student-minus-raw was `+0.039262` with
interval `[+0.024006,+0.054310]`. Family changes were `-0.017019`, `+0.024088`,
`+0.095604`, and `+0.091275`. Reversal AULC was `0.943522`.

Development participation rank ranged from `0.269779` to `0.326417` across all
nine seed/channel cases. Every effective-rank, participation-rank,
top-eigenvalue, and active-dimension threshold passed.

## Untouched confirmation

Confirmation was opened only after the student development pass.

| seed | initialization AULC | student AULC | gain |
|---:|---:|---:|---:|
| 17 | 0.579908 | 0.608105 | +0.028198 |
| 31 | 0.558613 | 0.612580 | +0.053966 |
| 47 | 0.547512 | 0.623738 | +0.076225 |

The confirmation mean was `0.614808`. Student-minus-initialization was
`+0.052797`, with paired 95% interval `[+0.037586,+0.069006]`; all three seeds
improved. Student-minus-raw was `+0.018443`, with interval
`[+0.000826,+0.035473]`.

Confirmation family changes were `+0.001033`, `-0.000943`, `+0.094461`, and
`+0.116635`. Reversal AULC was `0.942383`, its 95% lower bound was `0.928548`,
and its retention lower bound was `-0.006287`, above the frozen `-0.02` margin.
Both shuffle controls passed.

Confirmation participation rank ranged from `0.264712` to `0.317268`,
effective rank from `0.363689` to `0.438633`, top-eigenvalue share from
`0.182623` to `0.203331`, and active-dimension fraction was `1.0` everywhere.
All nine geometry cases passed.

## Scope

This certifies the isolated architecture/readout/training recipe on the sealed
causal sequence benchmark across three fixed seeds. It proves that the encoder
can preserve order, expose predictive factors sample-efficiently, outperform
identical initialization and the raw control, and maintain non-collapsed
per-channel geometry without labels or downstream assistance.

It does not certify the former JEPA+MAE training objective, non-neutral
augmentations, a downstream stack, or universal performance on unseen real
datasets. Those remain separate questions and were deliberately excluded.
