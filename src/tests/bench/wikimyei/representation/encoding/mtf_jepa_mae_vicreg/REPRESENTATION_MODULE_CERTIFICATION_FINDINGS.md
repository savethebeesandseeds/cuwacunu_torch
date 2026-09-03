# RMC-1 — Representation Module Certification Findings

Date executed: 2026-08-28

## Human conclusion

The repaired sparse readout is mechanically sound and retains strong sequence
order information, but the current JEPA+MAE training recipe does **not** improve
the representation. Clean predictive AULC fell from initialization `0.59306`
to `0.58925` with neutral preprocessing. Only one of three seeds improved.

The qualified safe augmentation is not the explanation. Its final AULC was
`0.58927`, only `+0.0000184` above neutral, far below the frozen `+0.0024`
materiality threshold. The next repair must therefore change the encoder
objective or optimization, not tune augmentation and not involve a downstream
model.

Terminal classification:

```text
encoder_training_not_working
```

The untouched confirmation set was not opened.

## Decisive results

| arm | initialization AULC | trained AULC | trained - initialization, 95% CI | positive seeds | reversal AULC | result |
|---|---:|---:|---:|---:|---:|---|
| neutral | 0.59306 | 0.58925 | -0.003814 [-0.009362,+0.001618] | 1/3 | 0.93083 | fail |
| qualified safe augmentation | 0.59306 | 0.58927 | -0.003796 [-0.009348,+0.001634] | 1/3 | 0.93099 | fail |

The equal-width raw-history control was `0.60229`. Neutral trained-minus-raw
was `-0.01304` with interval `[-0.03091,+0.00451]`, failing the frozen
noninferiority gate.

The neutral learned family deltas were:

- multiscale state: `+0.00913`;
- order/regime: `+0.01845`;
- cross-channel: `-0.02573`;
- future: `-0.01710`.

Training therefore sharpened some local/order information while sacrificing
cross-channel and forecast-relevant information. This explains why loss could
fall while total representation quality did not improve.

## Geometry and sequence retention

Reversal decoding passed: the neutral fixed-seed mean was `0.93083` with 95%
interval `[0.91829,0.94189]`, and its trained-minus-initialization interval was
`[-0.00716,+0.00954]`. Continuous and reversal shuffle controls also passed.

Geometry did not pass. Active-dimension fraction remained `1.0`, but several
effective-rank fractions were below `0.25`, and every participation-rank
fraction was below `0.25` (`0.1116` to `0.1928`). The representation is not
dead, but its variance remains too concentrated.

## Validity

- all three paired-seed mechanics gates passed;
- initialization parameters and sparse representations were exact across
  arms;
- clean rows, masks, JEPA masks, weak-view support masks, and RNG schedules
  were exact;
- qualified augmentation preserved every support cell and terminal anchor;
- the prior `all_tokens` training results reproduced to `1e-9`, proving that
  only the evaluation readout changed;
- the sparse selector passed same-object purity, deterministic replay,
  complete-v1 identity, shape, mask, finiteness, parameter, and RNG checks;
- the known-unsafe full augmentation was not retrained;
- no downstream model was constructed.

## Supported next repair

Proceed with **FSPA-1 — Frozen Sequence Projection Alignment Repair**. Train
the structured representation directly to retain a frozen, equal-width causal
projection of its own input history. This uses no labels or downstream model,
removes the demonstrated JEPA/MAE objective mismatch from the optimizer, and
directly targets information preservation and healthy geometry. Keep outer
augmentation neutral.
