# MTF objective repair protocol

Date preregistered: 2026-08-26, before executing either repair arm

## Question and boundary

The sealed objective/mask attribution screen localized two independently harmful
additions to the otherwise paired `jepa_mae_only` arm: fixed-strength TF
alignment and global pooled VICReg. This module-only experiment asks whether
two mechanism-specific changes remove those harms without discarding the
useful representation geometry associated with regularization.

This is a repair screen, not a production qualification. It changes no active
configuration, launcher augmentation, source, graph, MDN/readout, checkpoint,
or report path. The two mechanisms are test-only and independently switchable.
The exact active `C=3,H=30,F=9,D=32` architecture, `all_tokens` serving surface,
data, probe battery, optimizer, batch size, target EMA, weak views, masks, and
all non-tested coefficients remain frozen to the attribution experiment.

## Independent repair arms

Append the following arms without changing or reordering the six sealed
attribution arms. Run seeds `17,31,47` for 32 updates and observe steps
`0,16,32`.

| Arm | JEPA | MAE | TF alignment | VICReg | repair mechanism |
| --- | ---: | ---: | ---: | ---: | --- |
| `jepa_mae_only` | 1.0 | 0.25 | 0 | 0 | common reference |
| `jepa_mae_plus_tf` | 1.0 | 0.25 | 0.10 | 0 | sealed fixed-TF reference |
| `jepa_mae_plus_tf_gradient_matched_warmup` | 1.0 | 0.25 | scheduled | 0 | gradient-matched TF warmup |
| `jepa_mae_plus_vicreg` | 1.0 | 0.25 | 0 | 0.05 | sealed global-VICReg reference |
| `jepa_mae_plus_projected_channel_stratified_vicreg` | 1.0 | 0.25 | 0 | 0.05 | projected per-channel statistics |

All arms retain maximum context/target time overlap `0.50`.

### Gradient-matched TF warmup

Use the following test-only coefficient at state `s`, where `s` is the number
of completed updates:

```text
lambda_TF(s) = 0.0165124 + (0.10 - 0.0165124) * min(s, 16) / 16
```

Thus checkpoint/state 0 and its update `0 -> 1` use `0.0165124`; state 15 and
its update use `0.094782025`; and checkpoint/state 16 and every later update
use `0.10`. Assert the exact schedule values at states `0,15,16,32`. The
initial coefficient is the sealed fixed-seed-mean coefficient that
matches the served-trunk norm of the weighted TF gradient to the combined
weighted JEPA+MAE gradient. The same step-dependent value must drive both the
optimizer loss and its direct-versus-decomposed gradient diagnostic.

### Projected channel-stratified VICReg

Keep the existing shared VICReg projector and weak-view construction, but
replace the active global pooling/statistics path only in this arm:

1. mean-pool tokens within each channel, preserving a `[B,C,D]` tensor;
2. apply the shared projector to each `[B,C]` item;
3. for each channel separately, compute VICReg similarity, variance, and
   covariance statistics across the `B` examples;
4. average each component over channels.

Do not flatten batch and channel into one sample axis: fixed channel identity
offsets must not satisfy the within-channel variance floor. Disable global
VICReg in this arm and give the channel loss internal multiplier `0.25`, equal
to the sealed active global multiplier; the outer VICReg coefficient remains
`0.05`. The default configuration and existing global/channel modes must be
unchanged.

## Mechanical and pairing contracts

The attribution protocol's exact initialization, data, row-order, forward-RNG,
target-mask, context-mask, weak-view-draw, optimizer-step, EMA-step, and
step-zero equality contracts remain mandatory. Hash and require equality of
initial parameters, batches, both masks, and both weak-view tensors; also
require exact step-zero embeddings, probes, and geometry. In addition:

- the warmup schedule is monotone and has the exact step `0`, `16`, and `32`
  coefficients above;
- its fixed-seed-mean step-zero served-trunk
  `||lambda_TF gTF|| / ||gJEPA+0.25 gMAE||` lies in `[0.80,1.25]`;
  no individual seed may lie outside `[0.50,2.00]`;
- channel-stratified VICReg equals a manual per-channel loop for similarity,
  variance, covariance, and total loss within floating-point tolerance;
- adding constant, distinct channel identity offsets cannot by itself remove
  the channel-stratified variance penalty;
- every channel has at least two jointly valid rows; channels are
  equal-weighted, never silently dropped or row-weighted;
- each arm's direct total gradient agrees with its coefficient-weighted branch
  decomposition to relative error at most `1e-5` at steps `0,16,32`;
- all reported losses, gradients, probes, and geometry are finite, and no
  optimizer update is clipped.

Failure of any mechanical or pairing contract invalidates the corresponding
scientific comparison.

## Endpoints and uncertainty

Reuse the attribution screen's clean `encode()` checkpoints, disjoint ridge
probe battery, sample ladder `32,64,128,256`, four sequence families, centered
per-channel covariance geometry, and 512 paired final-group bootstrap
replicates. The primary endpoint is step-32 macro probe AULC. Report family
scores, effective-rank fraction, participation-rank fraction,
largest-eigenvalue share, and active-dimension fraction.

For the warmup arm, report paired AULC contrasts against both fixed TF and
`jepa_mae_only`. For the channel-stratified arm, report them against both global
VICReg and `jepa_mae_only`. Construct one deterministic 512-row bootstrap index
table and reuse it for all four primary contrasts. A replicate resamples final
generated groups once, evaluates both arms on the same rows, and then averages
the three fixed seeds. Intervals therefore quantify held-out group uncertainty
for this fixed three-seed experiment; they do not quantify training-seed
uncertainty or correct for multiple comparisons. Step-32 AULC is primary;
step-16 behavior is descriptive.

At steps `0,16,32`, report the TF weighted norm ratio above, branch cosines,
cancellation, and served-parameter update norm. For VICReg, additionally
report clean global-preprojector geometry, clean served per-channel geometry,
clean projected per-channel geometry, weak-view per-channel variance-floor
fraction using the loss's biased variance plus epsilon, and weighted
similarity/variance/covariance gradient norms split between
`vicreg_stability_head.*` and the served tokenizer/encoder trunk. The two
per-channel calculations must reuse the same pair of weak views and must not
detach the channel pools. Emit raw component gradients and gradients weighted
exactly once by `0.05 * 0.25 * {25,25,1}`. Projector geometry is centered
across examples within each channel at `D=64`; the variance-floor fraction is
`sqrt(var(unbiased=false) + 1e-4) < 1` on the exact diagnostic weak views.
Diagnostics must restore train/eval state, consume no training RNG, and change
no parameters, optimizer state, or EMA state. Every training update remains
exactly one optimizer step followed by one target-EMA step.

## Frozen independent pass gates

All clauses for a mechanism are conjunctive.

The contemporaneous references must first reproduce the sealed harmful
directions: fixed TF below `jepa_mae_only`, and global VICReg below
`jepa_mae_only`. If either direction reverses, classify its mechanism
`reference_not_reproduced`; its relative geometry recovery and repair gate are
undefined rather than passed by a favorable denominator.

The TF warmup passes only if:

- warmup minus fixed-TF AULC has paired 95% lower bound greater than `0` and a
  point estimate at least `0.0044` (half the sealed fixed-TF harm), with a
  positive point contrast in at least two of three seeds;
- warmup minus `jepa_mae_only` AULC has paired 95% lower bound greater than
  `-0.005`;
- no step-32 sequence-family score is more than `0.02` below
  `jepa_mae_only`;
- contemporaneous step-32 mean geometry retains at least half of each
  fixed-TF improvement over `jepa_mae_only`: `(warmup - JM)/(fixed TF - JM)`
  is at least `0.50` for effective and participation rank, and
  `(JM top - warmup top)/(JM top - fixed TF top)` is at least `0.50` for
  largest-eigenvalue share. The sealed-value audit thresholds are respectively
  `0.1044215`, `0.0723465`, and at most `0.7382610`, and every metric must move
  in the repair direction in at least two of three seeds; and
- active-dimension fraction is at least `0.75`, in addition to all mechanical
  contracts above.

The projected channel-stratified VICReg arm passes only if:

- channel-stratified minus global-VICReg AULC has paired 95% lower bound
  greater than `0` and a point estimate at least `0.0030` (half the sealed
  global-VICReg harm), with a positive point contrast in at least two of three
  seeds;
- channel-stratified minus `jepa_mae_only` AULC has paired 95% lower bound
  greater than `-0.005`;
- no step-32 sequence-family score is more than `0.02` below
  `jepa_mae_only`;
- contemporaneous step-32 mean geometry closes at least half of each
  global-VICReg gap to `jepa_mae_only`: `(stratified - global)/(JM - global)`
  is at least `0.50` for effective and participation rank, and
  `(global top - stratified top)/(global top - JM top)` is at least `0.50` for
  largest-eigenvalue share. The sealed-value audit thresholds are respectively
  `0.0793040`, `0.0556425`, and at most `0.8245405`, and every metric must move
  in the repair direction in at least two of three seeds; and
- active-dimension fraction is at least `0.75`, in addition to all mechanical
  contracts above.

The `0.005` noninferiority margin is one quarter of the existing `0.02`
release-effect margin. Passing here licenses only the conditional module
factorial below; it does not license a production change.

## Conditional repaired factorial

Run no combined arm unless both independent mechanisms pass every gate. If
they do, append one arm combining the exact accepted TF schedule and exact
accepted channel-stratified VICReg path; rerun the same paired three-seed
`0/16/32` screen without changing any other field.

The combined arm passes only if:

- combined minus sealed `full_soft` AULC has paired 95% lower bound greater
  than `0`, point estimate at least `0.0081` (half the sealed full-objective
  deficit), and a positive point contrast in at least two of three seeds;
- combined minus `jepa_mae_only` AULC has paired 95% lower bound at least `0`
  and a positive point contrast in at least two of three seeds;
- no step-32 family score is more than `0.02` below `jepa_mae_only`;
- contemporaneous mean effective and participation rank are each at least the
  `jepa_mae_only` value, largest-eigenvalue share is at most its
  `jepa_mae_only` value, active-dimension fraction is at least `0.75`, and all
  three directional geometry contrasts hold in at least two of three seeds;
- every mechanical, pairing, and gradient-decomposition contract remains
  valid.

Also report the AULC interaction
`combined - warmup - stratified + jepa_mae_only`. Do not promote the combined
arm if this point estimate is below `-0.005`, even if its aggregate gates
otherwise pass. The established absolute geometry guard
`0.25/0.20/0.80/0.75` remains reported but is not a 32-step repair gate because
the common `jepa_mae_only` reference already fails its rank floors.

Only a passing combined arm may continue along the same frozen trajectories
to 64 and then 128 updates. A 512-update or canonical representation-quality
run, launcher-augmentation experiment, production default change, or
end-to-end run remains a separate later decision with a new recorded protocol.
