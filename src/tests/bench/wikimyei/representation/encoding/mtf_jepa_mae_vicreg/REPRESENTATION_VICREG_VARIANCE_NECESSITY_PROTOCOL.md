# MTF VICReg variance-component necessity protocol

Date preregistered: 2026-08-26, before implementing or executing the
variance-disabled arm

## Question and boundary

The accepted module-only objective-repair screen found that projected
channel-stratified VICReg remained below `jepa_mae_only` on step-32 macro
probe AULC and worsened all three served covariance-geometry measures. At the
same checkpoint its effective served-trunk variance-component gradient was
about 2,550 times the similarity component and 104 times the covariance
component. Gradient magnitude is not causal proof.

This experiment asks one narrower question: is the currently weighted VICReg
variance component a necessary contributor to that 32-update deficit under
the exact projected channel-stratified recipe?

This is a test-only module experiment. It changes no production default, DSL,
launcher augmentation, graph assembly, NodeLift, MDN/readout, Runtime,
checkpoint, observer, policy, or end-to-end path. It cannot establish that
variance pressure is sufficient, generally harmful, or the representation
system's root cause.

## Three contemporaneous arms

Run only these three arms. Do not rerun the other attribution or TF-repair
arms.

| arm | JEPA | MAE | TF | outer VICReg | channel multiplier | sim/var/cov |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `jepa_mae_only` | 1.0 | 0.25 | 0 | 0 | 0.25 | `25/25/1` (inactive) |
| `jepa_mae_plus_projected_channel_stratified_vicreg` | 1.0 | 0.25 | 0 | 0.05 | 0.25 | `25/25/1` |
| `jepa_mae_plus_projected_channel_stratified_vicreg_no_variance` | 1.0 | 0.25 | 0 | 0.05 | 0.25 | `25/0/1` |

The full and variance-disabled stratified arms must differ in exactly one
configuration scalar: `vicreg_var_weight`, from `25.0` to `0.0`. Do not set
the variance floor to zero, disable the VICReg branch, detach the projector,
or suppress raw variance diagnostics. Both arms keep global VICReg disabled,
channel VICReg enabled, per-channel stratification enabled, the shared
projector, internal weak views, JEPA/MAE masks, maximum context/target time
overlap `0.50`, and the effective component weights:

```text
full:        0.05 * 0.25 * {25,25,1} = {0.3125,0.3125,0.0125}
no-variance: 0.05 * 0.25 * {25, 0,1} = {0.3125,0,     0.0125}
```

Keep the exact active `C=3,H=30,F=9,D=32` architecture, `all_tokens` serving
width 96, synthetic datasets and splits, normalization, optimizer, target
EMA, batch size 96, and probe implementation from the accepted repair screen.
Use seeds `17,31,47`, 32 updates, checkpoints `0,16,32`, and the probe sample
ladder `32,64,128,256`.

## Pairing and ablation mechanics

All mechanics are conjunctive. A failure invalidates the scientific result
and stops the run before interpretation.

- Pair exact named initial parameter values, batch row indices, CPU/CUDA
  forward RNG, actual target and context masks, both actual weak-view tensors
  and masks, and step-zero embeddings, probes, selected ridge penalties, and
  served/projector geometry.
- Every channel must contribute at least two jointly valid rows; this fixed
  batch must use all `96 * 3` channel rows with equal channel weight.
- At step zero, the full and no-variance arms must have identical raw
  similarity, variance, and covariance losses and raw served/head component
  gradients. Before the outer `0.05` weight,
  `L_full - L_no_variance` must equal `0.25 * 25 * L_variance`; the total
  arm-gradient difference must equal `0.05 * 0.25 * 25 * g_variance`, each to
  relative error at most `1e-5`.
- At checkpoints `0,16,32`, each arm's direct gradient must reconstruct from
  JEPA, MAE, and its effective similarity/variance/covariance components to
  relative error at most `1e-5`. The no-variance arm's effective variance
  weight and gradient must be exactly zero while its raw variance loss and
  raw variance gradient remain finite and reported.
- Every update remains exactly one Adam step followed by one target-EMA step.
  No update may clip and every emitted loss, gradient, probe, geometry, and
  update surface must be finite.
- Diagnostics must replay the exact weak views, consume no training RNG,
  restore train/eval and CPU/CUDA generator state, and leave online
  parameters, target-EMA parameters, and existing optimizer bytes exactly
  unchanged.
- The contemporaneous `jepa_mae_only` and full-stratified shared scientific
  surfaces must reproduce the accepted hardened screen byte-for-byte when
  compared by common key. A mismatch is a reproducibility failure, not a
  candidate result.

The production configuration and DSL must not reference the test-only arm or
alter their existing VICReg variance weight.

## Endpoints and uncertainty

The primary endpoint is step-32 fixed-seed-mean macro probe AULC. Step 16 is
descriptive. Report three paired step-32 contrasts:

1. no-variance minus full stratified, the necessity/rescue contrast;
2. no-variance minus `jepa_mae_only`, the noninferiority contrast;
3. full stratified minus `jepa_mae_only`, the contemporaneous reference.

Reuse one deterministic 512-row generated-group bootstrap resampling table
and the accepted repair-screen seed `8387496322364763509` for all contrasts.
Each replicate uses the same held-out rows for both arms and then averages the
three fixed seeds. These intervals quantify held-out generated-group
uncertainty only; they do not quantify training-seed uncertainty, correct for
multiple comparisons, or establish longer-run behavior.

At every checkpoint report the four family scores and served per-channel
effective-rank fraction, participation-rank fraction, largest-eigenvalue
share, and active-dimension fraction. Also report clean global-preprojector
and projected per-channel geometry, weak-view variance-floor fractions, raw
and effective component gradients on the served trunk and VICReg head,
branch/component cancellation, and served update norms. Projector geometry
and variance-floor fractions are explanatory diagnostics, not pass gates.

## Frozen necessity gate

All clauses are conjunctive. Invalid numeric or mechanical inputs take
precedence over reference classification.

The contemporaneous full-stratified reference must first reproduce the prior
harmful direction: its AULC point contrast versus `jepa_mae_only` is below
zero, its mean effective and participation ranks are below `jepa_mae_only`,
and its mean largest-eigenvalue share is above `jepa_mae_only`. Otherwise
classify `reference_not_reproduced`; necessity and relative recovery are
undefined.

The variance component is supported as a necessary contributor under this
exact recipe only if every following clause passes:

- no-variance minus full-stratified AULC has point estimate at least `0.0024`,
  paired 95% lower bound greater than `0`, and a positive point contrast in at
  least two of three seeds;
- no-variance minus `jepa_mae_only` AULC has paired 95% lower bound greater
  than `-0.005`;
- no step-32 no-variance family score is more than `0.02` below its paired
  `jepa_mae_only` score;
- contemporaneous mean served geometry closes at least half the
  full-stratified gap to `jepa_mae_only`: `(V0-S)/(JM-S) >= 0.50` for
  effective and participation rank, and `(S_top-V0_top)/(S_top-JM_top) >=
  0.50` for largest-eigenvalue share;
- each served-geometry metric moves in the repair direction in at least two
  of three seeds, and every seed's minimum channel active-dimension fraction
  is at least `0.75`; and
- every pairing, ablation, finite-value, gradient, update, and diagnostic
  mechanic above passes.

The accepted-value audit thresholds corresponding to half-gap recovery are
effective rank at least `0.07807679313736055`, participation rank at least
`0.05503634092132174`, and largest-eigenvalue share at most
`0.8293910990218041`. These are audit references only; the gate uses the
contemporaneous paired ratios.

If all AULC rescue clauses pass but a noninferiority, family, geometry, active
dimension, or mechanical safeguard fails, classify `partial_amelioration`,
not necessity. Otherwise a valid failure is `necessity_not_supported`.

## Stopping and interpretation

Run the gate once at step 32. Regardless of outcome, this protocol authorizes
no 64/128/512-update extension, coefficient search, normalization or coupling
variant, augmentation experiment, production edit, or end-to-end run.

A full pass supports only this statement: the currently weighted variance
gradient is a necessary contributor to the observed 32-update deficit under
this exact projected channel-stratified synthetic module recipe. Removing it
also changes total gradient scale, cancellation, and the optimizer trajectory,
so a pass cannot distinguish variance semantics from scale or interactions
with similarity, covariance, or projector coupling. Any subsequent dose,
normalization, or projector-versus-trunk experiment requires a new protocol.
