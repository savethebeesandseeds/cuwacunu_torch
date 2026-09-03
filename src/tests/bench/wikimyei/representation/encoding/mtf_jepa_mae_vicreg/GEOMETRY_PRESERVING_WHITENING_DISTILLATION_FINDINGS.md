# FSPA-3 — Geometry-Preserving Whitening Distillation Findings

Date executed: 2026-08-28

## Decision

FSPA-3 stopped at its no-training shadow gate. Full per-channel ZCA whitening
repaired covariance geometry, but it made the useful sequence information much
less accessible to the frozen low-data probe. Per protocol, no student was
trained and confirmation remained sealed.

```text
teacher_reproduced=true
whitening_mechanics_passed=true
shadow_semantics_passed=false
shadow_geometry_passed=true
student_opened=false
confirmation_opened=false
```

## What worked

The FSPA-2 teacher reproduced exactly at the important boundary:

- all three initial AULCs reproduced to `1e-9`;
- all three 1,024-step teacher mechanics receipts passed;
- final teacher AULCs were `0.625867`, `0.641517`, and `0.649833`;
- the complete FSPA-2 semantic result reproduced.

The SSL-only whitening fit was finite and full-rank in all nine seed/channel
cases. SSL participation rank rose from `0.1780–0.2892` to `1.0`. On
development rows, participation rank remained `0.6606–0.7500`, effective rank
was `0.7984–0.8556`, top-eigenvalue share was `0.0736–0.1054`, and every
dimension was active. Thus the geometry repair itself worked decisively.

## Why the shadow failed

Shadow clean AULC by seed was:

| seed | initialization | full-whitening shadow | change |
|---:|---:|---:|---:|
| 17 | 0.603103 | 0.550283 | -0.052821 |
| 31 | 0.583349 | 0.588357 | +0.005009 |
| 47 | 0.592733 | 0.590405 | -0.002328 |

Mean AULC was `0.576348`. The shadow-minus-initialization contrast was
`-0.016713`, with paired 95% interval `[-0.030986,-0.002048]`; only one seed
improved. Final-minus-raw was `-0.025938`, with interval
`[-0.042796,-0.010006]`.

Family changes relative to initialization were:

- multiscale state: `-0.049533`;
- order/regime: `-0.009292`;
- cross-channel: `+0.005701`;
- future: `-0.013730`.

Reversal behavior and shuffle controls still passed. The failure was therefore
not numerical collapse or lost temporal ordering; it was degraded
sample-efficient access to the predictive factors.

## Interpretation

ZCA whitening is affine and invertible under the verified positive eigenvalue
floor, so it cannot erase information in the mathematical sense. However, the
frozen ridge probe is intentionally low-data and regularized; it is not
invariant to an arbitrary anisotropic change of coordinates. Full whitening
amplified weak directions and diluted the useful high-signal directions far
beyond what was required to cross the participation floor.

## Supported next step

Use one label-free, analytically minimal **partial** covariance shrinkage. Fit
it only on SSL outputs and choose the smallest shrinkage strength that raises
SSL participation to a precommitted safety target just above `0.25`. Apply the
same no-training shadow stop gate before any distillation. This directly tests
whether the geometry threshold and FSPA-2 semantic quality can coexist without
the unnecessary overcorrection observed here.
