# MTF objective and mask attribution findings

Date: 2026-08-26

## Scope and result

This is a module-only causal localization of the active
MTF-JEPA-MAE-VICReg training objective. It uses the exact active
`C=3,H=30,F=9,D=32` architecture and 96-wide `all_tokens` serving surface,
three seeds (`17,31,47`), 32 paired optimizer updates, and clean frozen-encoder
probes at steps `0,16,32`. Launcher augmentation, the graph, MDN/readout, data
loading, checkpoints, and production reports are absent. Internal VICReg weak
views remain active.

The supported diagnosis is:

- TF alignment and VICReg each reduce low-label sequence-probe sample
  efficiency when added separately to the fixed JEPA/MAE background.
- VICReg is the dominant incremental served-representation rank-collapse
  pressure in this run. It is not the sole geometric problem: JEPA/MAE-only
  already worsens geometry from initialization.
- TF alignment dominates the initial served-trunk gradient and harms probe
  AULC, but improves rank geometry relative to JEPA/MAE-only. It is therefore
  not accurate to call TF the collapse mechanism.
- JEPA/MAE-only is the least harmful tested objective, not a qualified recipe:
  it still does not improve AULC over initialization and still fails the
  absolute geometry gates.
- Allowing every non-target token as context does not rescue the full
  objective. This intervention does not implicate the tested soft
  context-overlap policy as the primary cause of the early failure, but it
  cannot rule out smaller mask-policy effects.

Production configuration was not changed.

## Pairing and mechanics

Every seed passed exact within-seed equality checks for named initial
parameters, step-zero embeddings, probe predictions, selected ridge values,
AULC, geometry, and target masks. The five arms with overlap threshold `0.50`
also had exactly equal context masks. All branch-gradient decompositions
reconstructed the direct total-loss gradient within the enforced `1e-5`
relative tolerance. No optimizer step in any arm or seed reached the gradient
clip.

The active mask selected exactly 6 of 72 valid tokens per sample (`8.3333%`)
and retained 54 context tokens. There were no hard forbids. The soft policy
retained 12 forbidden non-target tokens per sample after relaxing roughly
51.2-51.5 soft forbids per sample to satisfy the 75% minimum-context rule.
The overlap-allowed arm preserved the identical six targets and retained all
66 non-target context tokens.

The sealed v2 four-arm execution and the v3 conditional execution reproduced
the original arm contrasts exactly. The final executable build had SHA-256
`5bd1699f51f76fc2b70d375cada707fe3f888a45489b362769a543eb529e1ac7`.

Command:

```bash
CUBLAS_WORKSPACE_CONFIG=:4096:8 \
  ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_representation \
  --experiment objective-mask-attribution --device cuda
```

## Primary four-arm attribution

The equal-width raw-history control reached AULC `0.6022866`. Mean step-zero
representation AULC was `0.5192625`; the full objective fell to `0.4992997` by
step 32, a mean change of `-0.0199628` and a gap of `-0.1029869` from the raw
control.

| Arm | Step-32 AULC | Step 32 - step 0 | Arm - full, paired 95% interval | Positive seeds |
| --- | ---: | ---: | ---: | ---: |
| full objective | 0.499300 | -0.019963 | reference | - |
| TF/VICReg only | 0.497883 | -0.021380 | -0.001417 `[-0.004219,+0.001676]` | 1/3 |
| JEPA/MAE only | 0.515543 | -0.003719 | +0.016244 `[+0.010152,+0.022615]` | 3/3 |
| full, overlap allowed | 0.500249 | -0.019014 | +0.000949 `[-0.000265,+0.002350]` | 1/3 |

The TF/VICReg-only arm is sufficient to reproduce the full AULC decline,
while removing TF/VICReg rescues the full arm in every seed. This localizes the
early harmful pressure to the TF/VICReg group. Increasing context from 54 to
66 tokens does not produce a reliable rescue with target selection held exact.

## Triggered TF/VICReg split

The preregistered group result triggered a second paired split with JEPA/MAE
held fixed. Both additions were harmful to AULC in all three seeds:

| Added branch | Step-32 AULC | Addition - JEPA/MAE, paired 95% interval | Positive seeds |
| --- | ---: | ---: | ---: |
| none: JEPA/MAE only | 0.515543 | reference | - |
| TF alignment | 0.506709 | -0.008834 `[-0.014559,-0.003763]` | 0/3 |
| VICReg | 0.509513 | -0.006030 `[-0.011221,-0.000921]` | 0/3 |
| TF alignment plus VICReg | 0.499300 | -0.016244 versus JEPA/MAE | 0/3 |

The combined AULC effect is close to additive. Adding the two individual
effects to the JEPA/MAE baseline predicts `0.5006794`; the observed full result
is `0.4992997`, leaving a small additional interaction of `-0.0013796`.
Therefore the sample-efficiency result is not explained by a TF/VICReg
interaction alone. This residual is descriptive and has no dedicated
confidence interval; some individual endpoints, especially terminal
order/regime, are more visibly non-additive.

No step-32 final family score regressed by more than `0.02` when either branch
was added to JEPA/MAE. TF changed the four final family means by approximately
`[-0.0061,+0.0244,-0.0038,+0.0003]`; VICReg changed them by
`[-0.0051,+0.0148,-0.0061,+0.0077]`. The negative primary effect is therefore
loss of labeled-sample efficiency across the fixed ladder, not a claim that
every final 256-label family probe is worse.

## Collapse geometry separates the branches

| Arm | Mean effective-rank fraction, step 32 | Mean participation-rank fraction, step 32 | Mean worst-channel top-eigenvalue share, step 32 |
| --- | ---: | ---: | ---: |
| initialization | 0.120865 | 0.083191 | 0.653662 |
| full objective | 0.084650 | 0.057709 | 0.825980 |
| TF/VICReg only | 0.082021 | 0.055942 | 0.843431 |
| JEPA/MAE only | 0.098293 | 0.067797 | 0.751273 |
| full, overlap allowed | 0.083954 | 0.057237 | 0.826223 |
| JEPA/MAE plus TF | 0.110550 | 0.076896 | 0.725249 |
| JEPA/MAE plus VICReg | 0.060315 | 0.043488 | 0.897808 |

Adding TF to JEPA/MAE improves all three rank-concentration measures at step 32
even though it reduces AULC. Adding VICReg sharply worsens them: effective rank
drops from `0.0983` to `0.0603` and the worst-channel largest eigenvalue rises
from `0.7513` to `0.8978`. TF partially counteracts that geometric collapse in
the full objective, but its marginal AULC cost remains. JEPA/MAE-only itself
worsens effective rank by `0.0226`, participation rank by `0.0154`, and the
worst-channel top-eigenvalue share by `0.0976` from initialization, so VICReg
must not be described as the sole source of poor geometry.

All arms fail the preregistered effective-rank (`0.25`) and participation-rank
(`0.20`) gates. JEPA/MAE-only and JEPA/MAE-plus-TF remain under the
largest-eigenvalue ceiling (`0.80`); the full and VICReg-containing arms do
not. Every arm retains all nominally active coordinates, showing why marginal
active-dimension counts alone would have missed the collapse.

## Gradient mechanism

Production-weighted served-trunk gradient norms in the full arm, averaged over
the three seeds, were:

| Step | JEPA | MAE | TF align | VICReg | JEPA+MAE group | TF+VICReg group |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 0.01503 | 0.02525 | 0.17284 | 0.000026 | 0.02854 | 0.17284 |
| 16 | 0.03332 | 0.01042 | 0.04595 | 0.00170 | 0.03637 | 0.04606 |
| 32 | 0.02325 | 0.00670 | 0.01728 | 0.02576 | 0.02650 | 0.03153 |

TF alignment is about six times the combined JEPA/MAE trunk norm at
initialization. It also initially points against JEPA and MAE (mean cosines
`-0.113` and `-0.220`). By step 32, VICReg has grown from effectively zero to
the largest individual branch norm. Meanwhile the canonical cancellation
ratio falls from `0.787` to `0.666`.

This demonstrates why scalar component-loss shares were misleading. VICReg's
raw scalar loss is large while its initial trunk gradient is negligible; TF
alignment initially controls the update. Later, VICReg becomes strong enough
to dominate the served-trunk branch norms and is the branch associated with
the worst rank collapse.

## Implementation-level mechanism audit

The TF-alignment result is consistent with its exact gradient path. The branch
pairs all 36 time tokens per sample with frequency tokens having identical
channel, scale, start, and width: 21 scale-8, 9 scale-16, 3 scale-32, and 3
scale-64 pairs. It minimizes symmetric cosine distance directly between the
two online full-latent streams. There is no projector, stop-gradient target,
scale balancing, whitening, or gradient calibration. It therefore reconciles
two independently initialized tokenizer coordinate systems through the same
encoder that is served, while short windows dominate the pair count. This
explains the large initial transient without proving that every aligned
direction is task-useful.

The VICReg audit found no obvious sign or reduction error. Instead, the active
anti-collapse loss and the served representation are different surfaces. The
active branch applies a nonlinear `32 -> 64 -> 64 -> 64` projector to the
global mean over all tokens and all channels. Serving uses three separate
per-channel 32-wide pools. At the pooling boundary, global VICReg sends the
same direct gradient to every channel, leaving zero-sum channel-differential
directions invisible. The shared encoder also broadcasts a transformed global
token mean back into every token. A nonlinear projector can satisfy projected
variance/covariance using a low-dimensional sample-global factor while the
96-wide served per-channel representation concentrates.

Two details make the existing diagnostics insufficient:

- the variance penalty is positive but has zero gradient at exact collapse,
  and the current unit test checks only its positive value;
- the reported latent standard deviation mixes token positions, domains, and
  channels, so learned identity offsets can look healthy while centered
  within-channel serving geometry collapses.

Simply enabling the existing channel-VICReg switch is not a clean remedy. It
flattens batch and channel rows before its covariance statistics, allowing
between-channel identity offsets to supply variance. A discriminating test
must compute statistics separately over samples within each channel and then
average channels.

## Frozen next module-only recommendation

The next experiment should add only two mechanism-targeted arms, retaining the
same seeds, data, masks, RNG, 32 updates, and clean endpoints:

1. `jepa_mae_plus_tf_gradient_matched_warmup`: keep VICReg at zero and ramp TF
   from `0.0165124` to `0.10` over the first 16 updates. The starting value is
   `0.10 * 0.02854 / 0.17284`, matching the observed TF served-gradient norm to
   the JEPA/MAE group rather than allowing a six-fold initial transient.
2. `jepa_mae_plus_projected_channel_stratified_vicreg`: disable global VICReg,
   project each served per-channel pool with the shared head, compute
   variance/covariance separately across samples within each channel, average
   channels, and use internal multiplier `0.25` so effective weights match the
   active global branch.

The TF arm succeeds only if it remains materially better than fixed-TF AULC,
is non-inferior to JEPA/MAE-only under a frozen margin, and retains TF's rank
improvement. The VICReg arm succeeds only if it improves both AULC and served
per-channel geometry versus global VICReg. That arm must additionally report
pre-projector global geometry, served per-channel geometry, projector
geometry/variance-floor fraction, and sim/variance/covariance gradient norms
split between projector and served trunk. These comparisons should precede
any production configuration edit or end-to-end retraining.

Execution note, 2026-08-26: this recommendation was subsequently frozen and
executed in `REPRESENTATION_OBJECTIVE_REPAIR_PROTOCOL.md` and
`REPRESENTATION_OBJECTIVE_REPAIR_FINDINGS.md`. Both independent repairs failed
their conjunctive gates under valid mechanics, so no combined arm, longer
extension, production edit, or end-to-end run was authorized.

## Supported decision and limits

The current full objective should not be promoted to another expensive
end-to-end run. The next module-only design should treat two problems
separately:

1. remove or redesign VICReg pressure on the served representation before
   expecting the geometry to recover;
2. reduce, normalize, or delay TF-alignment pressure so its geometric benefit
   is retained without overwhelming early sequence-probe sample efficiency.

Simply switching to JEPA/MAE-only is a diagnostic baseline, not a solution. Its
32-step point estimate remains below initialization by `0.0037192` and below
the raw equal-width control by `0.0867433` AULC, and it fails the absolute rank
gates. No arm-versus-initialization confidence interval was computed, so the
small first difference is not a significance claim.

This experiment does not test true mask-off or target-selection causality.
`full_overlap_allowed` still selects six targets and excludes those targets
from context; it only removes soft overlap forbids for the remaining tokens.
Zero mask ratios cannot provide a true control because the current masker
forces at least one time target. The 32-step result is a causal localization of
early objective pressure, not production qualification or proof of market
usefulness.

The paired bootstrap resamples final generated groups and averages three fixed
model seeds. It does not estimate seed/training uncertainty, apply a
multiplicity correction, or turn the post-trigger conditional split into a
release test. These limits are additional reasons to retain the diagnostic
classification.
