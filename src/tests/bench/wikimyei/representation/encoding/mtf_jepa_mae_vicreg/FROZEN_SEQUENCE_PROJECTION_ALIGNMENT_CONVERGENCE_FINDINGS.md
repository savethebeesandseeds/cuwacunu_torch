# FSPA-2 — Frozen Sequence Projection Alignment Convergence Findings

Date executed: 2026-08-28

## Result

FSPA-2 established that the isolated encoder can learn a materially better
sequence representation. All semantic, control, and mechanics gates passed in
all three seeds. The unchanged RMC development gate was withheld only because
the per-channel covariance participation threshold failed in channels 0 and 2.

```text
semantic_gates_passed=true
geometry_gate_passed=false
development_gate_passed=false
confirmation_opened=false
```

Clean predictive AULC improved in every seed:

| seed | initialization | update 1,024 | gain |
|---:|---:|---:|---:|
| 17 | 0.603103 | 0.625867 | +0.022764 |
| 31 | 0.583349 | 0.641517 | +0.058168 |
| 47 | 0.592733 | 0.649833 | +0.057100 |

The mean trained-minus-initialization gain was `+0.046011`, with paired 95%
interval `[+0.032483,+0.060085]`. The final representation also exceeded the
frozen raw control by `+0.036786`, with interval
`[+0.021410,+0.051556]`.

Three of four task families improved, and no family crossed its frozen
`-0.02` floor:

- multiscale state: `-0.019609`;
- order/regime: `+0.023173`;
- cross-channel: `+0.078413`;
- future: `+0.102065`.

Reversal AULC was `0.945801`. Its trained-minus-initialization retention was
`+0.016276`, with interval `[+0.000814,+0.031576]`. Both continuous-target and
reversal shuffle controls passed.

## Sole blocker

Effective rank, top-eigenvalue share, and active-dimension fraction passed for
every seed and channel. Participation rank passed in channel 1 but fell below
the frozen `0.25` threshold in channels 0 and 2:

| seed | channel 0 | channel 1 | channel 2 |
|---:|---:|---:|---:|
| 17 | 0.1999 | 0.2971 | 0.2128 |
| 31 | 0.1936 | 0.2900 | 0.2160 |
| 47 | 0.1908 | 0.3081 | 0.2079 |

All channels remained fully active. Effective-rank fractions ranged from
`0.2868` to `0.4326`, while top-eigenvalue shares ranged from `0.1990` to
`0.2980`.

## Mechanics

All mechanics passed. The last-eight alignment-loss means were `0.29250`,
`0.28744`, and `0.30136`, down from approximately `1.14`, `1.26`, and `1.30`.
Gradients and updates were finite and nonzero. Predictor, MAE-decoder, and
VICReg-head parameters remained exactly unchanged. No labels, augmentation,
or downstream model were used.

## Supported next step

Do not continue blindly toward the raw projection. The multiscale family is
already only `0.000391` above its frozen loss floor, and complete convergence
to the raw projection could sacrifice the semantic gains just established.

Instead, run one geometry-preserving repair: fit a fixed invertible per-channel
whitening transform on the FSPA-2 teacher's SSL representations, verify the
transform as a no-training shadow, and only if that shadow passes distill it
into a cloned encoder. Keep the structured readout and all RMC gates unchanged.
