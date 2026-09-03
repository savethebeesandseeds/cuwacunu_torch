# IMA-4B Protocol Amendment A1 — Bounded Optimization Closure

## Why this amendment exists

The first complete 512-update execution stopped at the precommitted
`compute_censored` gate.  Its immutable failed-attempt log is
`.build/tests/representation_ima4b_v1_attempt1_compute_censored.log`, SHA-256
`627240eb9e5560b0cd49e9b7b9877b48496e0ca9637f73141fee93046b16d7cf`.

All custody and equal-compute mechanics passed.  The scientific result was not
accepted.  The censor was triggered only by the seed-17 slot-bias control:
validation `R²` rose from `-0.30087093` at step 384 to `-0.27110648` at step
512.  Its selected checkpoint was nevertheless step 320 at `-0.21822339`.
Every treatment/control selected checkpoint was before the 512-update boundary
(seed 17: 320/320; seed 31: 448/448; seed 47: 320/320).

Thus the original edge comparison detected recovery from a transient
validation dip, not a best checkpoint pressed against the compute boundary.
The original attempt remains invalid for scientific routing; this amendment
does not reinterpret it.

## One allowed continuation

Run both arms again from the frozen FSPA-4 initialization with fresh Adam
states.  Reuse no trained tensor or selected checkpoint from attempt 1.
Everything in the base protocol remains unchanged except:

- increase the equal budget from 512 to exactly 1,024 updates per arm;
- retain validation at steps `0,64,128,192,256,320,384,448,512` and add
  `640,768,896,1024`;
- select the lowest validation NMSE across all 13 checkpoints, earliest on
  ties;
- continue the same deterministic group schedule through update 1,024.

This is the only continuation.  There is no learning-rate, optimizer, batch,
architecture, loss, seed, split, metric, or verdict-threshold change.

## Corrected fail-closed edge rule

Report `compute_censored` only if both conditions hold for either arm:

1. its selected checkpoint is the final step 1,024; and
2. validation `R²(1024) - R²(896) >= 0.005`.

A model whose best validation checkpoint is earlier is optimization-closed for
this bounded gate even if its final point happens to recover from a local dip.
If the amended censor fires, stop permanently and route no scientific verdict.
No further extension is authorized.

The base protocol's custody requirements, paired bootstrap, reach/materiality
thresholds, decision names, frozen representation boundary, and prohibition on
augmentation attribution remain binding.
