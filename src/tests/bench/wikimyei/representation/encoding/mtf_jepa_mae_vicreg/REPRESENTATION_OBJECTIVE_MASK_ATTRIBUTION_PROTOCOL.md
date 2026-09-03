# MTF objective and mask attribution protocol

Date preregistered: 2026-08-25

## Question and scope

This module-only experiment asks which part of the active
MTF-JEPA-MAE-VICReg objective first drives the loss of decodable sequence
information and rank concentration already observed by the isolated
representation-quality screen. It excludes source loading, launcher-owned
augmentation, NodeLift, the graph, MDN/readout training, checkpoints, and
reports. It does not qualify production performance.

The exact active `C=3,H=30,F=9,D=32` architecture, active `all_tokens`
96-wide serving surface, Adam learning rate, batch of 96 model rows, gradient
clip, and target-EMA update remain fixed. Module-internal VICReg weak views
remain active. All four loss branches execute in every arm; zero-valued
coefficients remove only their optimizer gradient, preserving stochastic draw
order.

## Paired arms

Run 32 updates for seeds `17,31,47`, observing steps `0,16,32`.

| Arm | JEPA | MAE | TF align | VICReg | maximum context/target time overlap |
| --- | ---: | ---: | ---: | ---: | ---: |
| `full_soft` | 1.0 | 0.25 | 0.10 | 0.05 | 0.50 |
| `no_jepa_mae_gradient` | 0 | 0 | 0.10 | 0.05 | 0.50 |
| `jepa_mae_only` | 1.0 | 0.25 | 0 | 0 | 0.50 |
| `full_overlap_allowed` | 1.0 | 0.25 | 0.10 | 0.05 | 1.00 |

`full_overlap_allowed` disables the soft time-overlap exclusion because the
implementation forbids a context token only when overlap is greater than the
threshold. It leaves target selection unchanged and still excludes target
tokens themselves from context. It therefore tests the context-overlap policy
and the resulting context quantity; it is not a true mask-off arm and does not
test a policy that never relaxes soft forbids.

## Pairing and integrity contracts

Generate and normalize the self-supervised, probe-fit, ridge-selection, and
final-test tensors once. Within each seed, reset CPU and CUDA RNGs before each
model construction and require exact equality of every initial parameter.
Use identical deterministic row permutations. Derive a stateless forward seed
from `(model seed, update)` and reset both generators immediately before every
arm forward.

Preview masks with the same seed, hash their CPU-contiguous boolean content,
then rewind the seed before training. Require identical target masks across all
arms and identical context masks across the first three arms. The overlap arm
may change context masks but not targets. Step-zero embeddings, probe
predictions, AULC, and geometry must be exactly equal within a seed. Each
successful update performs exactly one optimizer step followed by exactly one
target-EMA update.

These checks make arm differences attributable to objective coefficients or
the declared context policy rather than initialization, batches, target
selection, or stochastic weak views.

## Gradient attribution

At steps `0,16,32`, reuse a fixed diagnostic batch and RNG draw. Compute the
production-weighted branch gradients

```text
gJ = 1.0 grad(L_JEPA)
gM = 0.25 grad(L_MAE)
gT = 0.10 grad(L_TF)
gV = 0.05 grad(L_VICReg)
```

on the served trunk (`tokenizer.*` and `encoder.*`), with tokenizer and encoder
norms also reported separately. Report all six pairwise cosines, the masked
group norm `||gJ+gM||`, unmasked group norm `||gT+gV||`, full norm,
cancellation ratio `||sum(g)|| / sum(||g||)`, the optimizer pre-clip norm, and
clip factor. Undefined zero-norm cosines are invalid values, not zero.

For every arm and checkpoint, reconstruct its actual coefficient-weighted
gradient from the branch vectors and require relative error at most `1e-5`
against a direct backward pass of total loss. Gradient diagnostics are
observational: they may not alter parameters, optimizer state, or target EMA.

## Representation endpoints and uncertainty

At steps `0,16,32`, put the model in evaluation mode and call clean `encode()`
on unchanged held-out data. Reuse the existing disjoint ridge-probe battery and
sample ladder `32,64,128,256` for multiscale state, order/regime,
cross-channel dynamics, and causal future state. Report macro AULC, final
family scores, and centered per-channel effective-rank fraction,
participation-rank fraction, largest-eigenvalue share, and active-dimension
fraction. Also report the fixed orthonormal equal-width raw-history control.

At step 32, use 512 paired final-group bootstrap replicates. Each replicate
resamples final groups once, evaluates both arms on the same rows, averages the
three fixed seeds, and reports the 95% interval for each arm-minus-`full_soft`
AULC contrast. Also report the number of seeds with a positive rescue.

The established absolute geometry guard remains visible:

- effective-rank fraction at least `0.25`;
- participation-rank fraction at least `0.20`;
- largest-eigenvalue share at most `0.80`;
- active-dimension fraction at least `0.75`.

Initialization already fails parts of this guard, so changes from the exact
paired step-zero state are required alongside absolute values.

## Preregistered attribution rules

A rescue requires a positive arm-minus-full 95% AULC interval, a positive
point estimate in at least two of three seeds, no family regression worse than
`0.02`, and geometry moving in the non-collapse direction. The existing
`0.02` release margin is reported separately; this 32-step diagnostic cannot
grant release qualification.

- `JEPA/MAE primary`: full training is harmful from initialization,
  `no_jepa_mae_gradient` rescues it, and `jepa_mae_only` reproduces the harmful
  probe and geometry direction.
- `TF/VICReg primary`: the symmetric pattern.
- `objective interaction`: full training is harmful while both objective
  subsets are materially better or neither subset independently reproduces the
  full failure.
- `both independently harmful`: both subset arms significantly deteriorate
  from initialization.
- `soft-overlap policy implicated`: `full_overlap_allowed` rescues the full
  arm with identical target masks and improved geometry.
- `onset_not_reached`: the full arm is not measurably harmful by step 32;
  continue the same frozen trajectories to 64 and then 128 updates before
  changing the hypothesis.

If the overlap arm rescues, a later matched-context random-forbid arm is still
required to distinguish which tokens are hidden from how many are visible. A
true target/context mask-bypass experiment would require an explicit external
mask pathway or test-only forward surface; zero mask ratios cannot provide it
because the current masker forces at least one time target. No result from this
protocol may be described as proving target-selection causality.

## Triggered TF/VICReg split

The first sealed four-arm execution produced a positive three-seed rescue for
`jepa_mae_only` versus `full_soft` (mean AULC `+0.01624357`, paired 95%
interval `[+0.01015179,+0.02261505]`, positive in three of three seeds), while
`no_jepa_mae_gradient` and `full_overlap_allowed` did not rescue. This met the
diagnostic trigger to split the implicated TF/VICReg group; it did not meet or
replace a release-qualification claim.

Append the following two arms without changing the original four, seeds,
steps, data, pairing, or endpoints:

| Arm | JEPA | MAE | TF align | VICReg | maximum overlap |
| --- | ---: | ---: | ---: | ---: | ---: |
| `jepa_mae_plus_tf` | 1.0 | 0.25 | 0.10 | 0 | 0.50 |
| `jepa_mae_plus_vicreg` | 1.0 | 0.25 | 0 | 0.05 | 0.50 |

Compare each appended arm directly with `jepa_mae_only` using the same paired
512-replicate interval and positive-seed rule. A significantly harmful
`jepa_mae_plus_tf` contrast implicates TF alignment; the symmetric result
implicates VICReg. If neither appended arm is harmful but the full arm is,
classify the failure as a TF/VICReg interaction. If both are harmful,
classify both as independent contributors. This conditional addendum was
recorded after the four-arm trigger and before observing either appended arm.
