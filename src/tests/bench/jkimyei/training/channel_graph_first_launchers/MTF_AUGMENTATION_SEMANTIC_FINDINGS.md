# MTF augmentation semantic findings

Date: 2026-08-25

## Scope

This report qualifies the launcher-owned MTF training augmentation stack in
isolation. It uses the production `apply_mtf_training_augmentations` function,
an `F=9` labeled fixture, active history supports `4/10/30`, and deterministic
augmentation seeds 17 through 32. It performs no model training and makes no
claim about market usefulness.

The qualifier checks valid-support retention, preservation of the terminal
"now" anchor and value, temporal order, phase/future direction, trend
direction, cross-channel coupling and lag, fixed-seed reproducibility,
dirty-masked-sentinel parity, and a mask-only synthetic-label probe. Quality
failure is reported in machine-readable output and does not become a process
failure.

Command:

```bash
make -C src/tests/bench/jkimyei/training/channel_graph_first_launchers \
  run-quality_jkimyei_mtf_jepa_mae_vicreg_augmentation_semantics
```

## Leave-one-in attribution

| Arm | Support H4/H10/H30 | Terminal support | Order concordance | Coupling NRMSE | Result |
| --- | --- | ---: | ---: | ---: | --- |
| neutral | 1 / 1 / 1 | 1 | 1 | 3.59e-8 | qualified |
| Gaussian jitter `.001` | 1 / 1 / 1 | 1 | 1 | 0.00275 | qualified |
| amplitude scale `.98-1.02` | 1 / 1 / 1 | 1 | 1 | 0.01636 | qualified |
| frequency mask `.02` | 1 / 1 / 1 | 1 | 0.94840 | 0.12317 | not qualified |
| frequency gain jitter `.01` | 1 / 1 / 1 | 1 | 1 | 0.00881 | qualified |
| dilation `.98-1.02` | .75 / .90 / .97083 | .5625 | 1 | 5.73e-8 | not qualified |
| warp `.01` | .76563 / .90 / .96667 | .4375 | 1 | 5.89e-8 | not qualified |
| dilation plus warp | .50 / .79375 / .93958 | .3125 | 1 | 7.24e-8 | not qualified |
| candidate safe stack | 1 / 1 / 1 | 1 | 1 | 0.01887 | qualified |
| full active stack | .50 / .79375 / .94375 | .375 | .94807 | .12196 | not qualified |

The candidate safe stack combines only Gaussian jitter `.001`, amplitude
scale `.98-1.02`, and frequency gain jitter `.01`. Across all 16 draws it also
preserved future-slope direction, phase, and lag under the preregistered gates;
its worst coupling NRMSE was `0.01991`.

## Diagnosis

Frequency-bin masking independently breaks temporal-order and cross-channel
coupling gates. Its worst draw reached order concordance `0.9375` and coupling
NRMSE `0.15017`. The implementation FFTs the full zero-padded 30-step axis and
samples gains independently by sample and channel, so short histories receive
padding-dependent global mixing and channels do not share the same distortion.

Dilation and warp independently fail support and terminal-anchor preservation.
They use fractional interpolation with an AND validity rule, and neither pins
the right endpoint. The combined temporal arm retained as little as
`.25/.70/.90` of the 4/10/30-step supports; some draws removed the terminal
cell completely and made the short-channel lag unrecoverable.

The full active stack therefore fails support, terminal-anchor, temporal-order,
short-channel future-direction, and cross-channel-coupling gates. Its worst
draw reached temporal-order concordance `0.88672`, future-direction accuracy
`0.875`, coupling NRMSE `0.14169`, and an unrecoverable lag. It remains
deterministic under a fixed seed and safely ignores dirty values behind the
mask. No association with the synthetic future label was detected from the
mask alone (`0.5` accuracy), although the mask exposes structural history
identity.

## Supported decision

`light_phase_safe_v2` is not semantically qualified for a causal sequence
representation. Before any production retraining, set frequency masking to
zero, dilation to identity, and warp to zero, then requalify the resulting
profile. The tested candidate safe stack is suitable for the next matched
training experiment, but its synthetic qualification does not by itself prove
that any retained transform improves learned representations.

This does not support switching every augmentation off. The existing
single-seed task-specific diagnostic
`synthetic_mtf_outer_input_augmentation_off_v1.report` found that removing the
entire outer stack worsened return decodability on both raw-representation and
post-MDN surfaces. That consumed diagnostic is not a semantic qualification or
a population causal result, but it is evidence that some augmentation may be
useful regularization. The next comparison should therefore be active stack
versus the qualified candidate stack, not active stack versus no augmentation.

Outer augmentation is also not the sole explanation for the existing module
collapse: that collapse occurs when launcher augmentation is absent. The next
localization boundary is the intrinsic JEPA/MAE mask policy and the individual
loss branches.
