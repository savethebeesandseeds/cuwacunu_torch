# FSPA-1 — Frozen Sequence Projection Alignment Findings

Date executed: 2026-08-28

## Result

FSPA-1 failed the unchanged RMC development gate. The direct alignment loss
worked mechanically and decreased in every seed, but 128 updates did not make
the structured representation approximate the fixed raw projection well
enough.

```text
development_gate_failed
confirmation_opened=false
```

Mean clean predictive AULC fell from `0.59306` at initialization to `0.57817`.
The paired trained-minus-initialization result was `-0.01489` with 95% interval
`[-0.02690,-0.00286]`; only one seed improved.

The failure was structured:

- multiscale state: `+0.01257`;
- order/regime: `+0.02138`;
- cross-channel: `-0.11553`;
- future: `+0.02202`.

Reversal AULC improved in every seed to a mean `0.94775`, with
trained-minus-initialization interval `[+0.00781,+0.02913]`. Shuffle controls
passed. The objective therefore learned real temporal information, but not a
balanced representation.

Geometry remained underfit: effective-rank fractions were `0.128` to `0.177`
and participation-rank fractions were `0.098` to `0.129`, despite the fixed
target's known healthy values. Alignment loss was also still large at the end:
last-eight means were `0.817`, `0.859`, and `0.896`, down from first-eight
means `1.140`, `1.262`, and `1.297`.

All mechanics passed. Gradients and updates were finite and nonzero, inactive
JEPA predictor, MAE decoder, and VICReg heads remained exactly unchanged, no
augmentation or labels were used, and no downstream model was constructed.

## Supported next step

Run one convergence test with the same objective, data, seeds, optimizer,
readout, neutral augmentation policy, probes, and gates, changing only the
budget from 128 to 1,024 updates. The fixed target is healthy and the loss was
still far from convergence, so this distinguishes ordinary under-training from
an architecture/readout optimization barrier.
