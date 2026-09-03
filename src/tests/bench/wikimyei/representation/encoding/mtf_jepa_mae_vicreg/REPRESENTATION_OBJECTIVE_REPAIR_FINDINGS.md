# MTF objective repair findings

Date executed: 2026-08-26

## Result

Both preregistered repair mechanisms failed their independent conjunctive
gates. The combined arm was therefore not authorized or implemented, and no
64/128-update extension, canonical training run, launcher-augmentation run,
production edit, or end-to-end run was performed.

This is a valid scientific failure, not a mechanical failure:

- the fixed-TF and global-VICReg references reproduced their harmful AULC
  directions;
- all pairing, finite-value, gradient-decomposition, optimizer-update,
  diagnostic-neutrality, and no-clipping contracts passed;
- gradient-matched TF warmup corrected the intended initial gradient-scale
  mismatch but did not materially rescue representation quality;
- projected channel-stratified VICReg closed the batch/channel-flattening
  loophole but did not rescue the served geometry. It slightly improved AULC
  over global VICReg while making all three relative served-geometry measures
  worse.

The result rejects these exact repairs as candidates for combination or
promotion. It does not prove that TF alignment, VICReg-style regularization, or
the encoder family is irreparable under a different loss design.

## Frozen boundary

The executed screen followed `REPRESENTATION_OBJECTIVE_REPAIR_PROTOCOL.md`.
It retained the exact active `C=3,H=30,F=9,D=32` module architecture,
`all_tokens` serving width 96, clean synthetic splits, optimizer, target EMA,
mask policy, internal weak views, and the six sealed attribution arms. It
appended only:

- `jepa_mae_plus_tf_gradient_matched_warmup`;
- `jepa_mae_plus_projected_channel_stratified_vicreg`.

Seeds were `17,31,47`, training was 32 updates, checkpoints were `0,16,32`,
and the probe sample ladder was `32,64,128,256`. All four primary repair
contrasts reused one deterministic 512-row paired group-bootstrap table. The
intervals quantify held-out generated-group uncertainty averaged over these
three fixed seeds; they do not quantify training-seed uncertainty or correct
for multiple comparisons.

Launcher-owned augmentation, graph assembly, NodeLift, MDN/readout, Runtime,
market data, checkpoints, certified/final ranges, observer, and policy were
outside this executable. Internal VICReg weak views remained active and were
paired byte-for-byte across arms.

## Reproducibility and mechanics

The accepted CUDA execution used:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-contracts
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-repair-gate-test
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-isolated
CUBLAS_WORKSPACE_CONFIG=:4096:8 \
  ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_representation \
  --experiment objective-mask-attribution --device cuda
```

The Makefile now also exposes the core ordered workflow—contracts, gate
fixtures, then the CUDA screen—as `run-repair-screen`. The default isolated
smoke invocation above remains an additional regression check.

Provenance:

- repository HEAD at execution:
  `194c59207a721ccb411aba7ab4df9440e0cc5403`;
- frozen protocol SHA-256:
  `b8d498b320568dd8d59c87675fa8fb53410f0f8441c5e311d631c8f42cfc838b`;
- module-header SHA-256:
  `3562e214f8ff3cfd71e1d48899d849f1d38afd7e0c3fa8334cfb13ad35790d91`;
- quality-harness SHA-256:
  `579736ad4fdcd463d0162c6b11bd6ab5c76207917cd37f65b2ec28932fdc6e3b`;
- gate-helper SHA-256:
  `fc1b2f1d949fda5b353786374ec52ce3797c39d15a41dcff2fbb0c6bdacb6f1f`;
- accepted quality executable SHA-256:
  `f84269c065754bcaac0819e0d1e0bb058a24181cf34ee28f41bd585ade1221ae`;
- focused contract executable SHA-256:
  `e2aca7c2c531115806d9fcfd99f522cd9960adfeb7b05c993cf3ccf9643a341c`;
- standalone gate executable SHA-256:
  `6af1153b73ac43ae57988baf6426ffd78d9bfd44bf187839af44313bbfdf8417`;
- unchanged default smoke executable SHA-256:
  `2d5612c2280fefaf45cbd880cda6a7b4781214f86b0a9602155abd4899b9f7cf`;
- accepted raw log:
  `.build/tests/representation_objective_repair_v5_hardened.log`, 1,673,240
  bytes, SHA-256
  `057b69ff9e3102eb9684f063ebced40cc13d4b0af91ab4fb327fd26995999e20`.

The focused contracts passed manual unequal-row per-channel arithmetic,
channel-offset resistance, exact outer/inner loss weighting, attached
projector autograd, default/global-path equality, exact debug weak-view and
forward-mask replay, and explicit rejection of every zero- or one-row channel.
The standalone gate fixtures passed all exact threshold, strict-boundary,
non-finite, reference-reversal, invalid-mechanics, family, direction, and
geometry-denominator cases. The unchanged default isolated smoke suite also
passed.

For every seed, all eight arms had exactly equal initial named parameters,
step-zero embeddings/probes/geometry, batch rows, actual target masks, the
applicable context masks, and both actual weak-view tensors and masks. Every
training forward used all `96 * 3` channel rows for the stratified arm. No
update was clipped; every update norm was finite; and every update remained one
Adam step followed by one target-EMA step.

All 72 checkpoint diagnostics replayed the same weak views, restored CPU/CUDA
generator and train/eval state, and left every model and target-EMA parameter
exactly unchanged. At all 48 trained checkpoints the Adam step and moment
tensors also had an exact before/after byte digest; optimizer state did not yet
exist for the 24 step-zero diagnostics. All emitted losses, probes, geometry,
and gradient surfaces passed centralized finite checks. Direct arm gradients
and the separately weighted branch/component decompositions remained within
the frozen `1e-5` relative-error limit.

A pre-hardening pass and the accepted hardened pass had the same 483 shared
scientific keys and byte-identical values. The pre-hardening log SHA-256 was
`0f8b89c05d129562fe7b1e1e9c139e1c0d7ff6e95e8e68c3a206377d2afe343a`.
Only the hardened pass is authoritative.

## Primary representation endpoint

Step-32 fixed-seed-mean macro probe AULC:

| arm | AULC |
| --- | ---: |
| `jepa_mae_only` | 0.5155433 |
| fixed `jepa_mae_plus_tf` | 0.5067094 |
| TF gradient-matched warmup | 0.5072907 |
| global `jepa_mae_plus_vicreg` | 0.5095132 |
| projected channel-stratified VICReg | 0.5107100 |

Primary paired contrasts:

| contrast | point | paired 95% interval | positive seeds |
| --- | ---: | ---: | ---: |
| fixed TF - JEPA/MAE | -0.008834 | [-0.014797, -0.003608] | 0/3 |
| TF warmup - fixed TF | +0.000581 | [-0.001860, +0.003143] | 1/3 |
| TF warmup - JEPA/MAE | -0.008253 | [-0.013936, -0.002975] | 0/3 |
| global VICReg - JEPA/MAE | -0.006030 | [-0.011307, -0.000437] | 0/3 |
| stratified VICReg - global VICReg | +0.001197 | [-0.000778, +0.002999] | 2/3 |
| stratified VICReg - JEPA/MAE | -0.004833 | [-0.010302, +0.000573] | 0/3 |

The harmful reference directions therefore reproduced. At step 16 the five
means were still close (`0.512269`, `0.511973`, `0.511593`, `0.512892`, and
`0.512385` in the table order); the decisive separations arose by step 32.

## TF warmup gate

The warmup schedule and its initial gradient calibration worked exactly as
designed. The step-zero weighted TF-to-JEPA/MAE served-gradient ratios were
`0.933833`, `0.861116`, and `1.272046`, with fixed-seed mean `1.022332`. The
mean and all individual ratio clauses passed.

The scientific gate failed:

- warmup-versus-fixed-TF rescue point, positive lower bound, and positive-seed
  count all failed;
- warmup-versus-JEPA/MAE noninferiority failed;
- all four family safeguards passed;
- active-dimension and mechanical clauses passed;
- effective-rank, participation-rank, and top-share retention ratios were
  `0.3882`, `0.3651`, and `0.1782`, all below `0.50`;
- effective and participation rank moved in the repair direction in 2/3
  seeds, but largest-eigenvalue share did so in only 1/3.

Step-32 served geometry makes the shortfall concrete:

| arm | mean effective rank | mean participation rank | mean worst top share | min active fraction |
| --- | ---: | ---: | ---: | ---: |
| JEPA/MAE | 0.098293 | 0.067797 | 0.751273 | 1.0 |
| fixed TF | 0.110550 | 0.076896 | 0.725249 | 1.0 |
| TF warmup | 0.103051 | 0.071119 | 0.746636 | 1.0 |

The early six-fold scale transient was therefore not a sufficient explanation
for fixed TF's AULC harm. Ramping to the same final coefficient largely
returned to the harmful AULC trajectory and preserved too little of TF's rank
benefit. This does not distinguish a persistently smaller/adaptive TF weight
from a redesign of what TF alignment makes invariant.

## Channel-stratified VICReg gate

The stratified arm met its implementation target: each channel contributed
separate across-example statistics with equal channel weight, the global path
was disabled, the shared projector remained attached, and the effective
component weights were exactly `0.05 * 0.25 * {25,25,1}`.

The scientific gate failed:

- stratified-versus-global rescue point and positive lower bound failed; its
  2/3 positive-seed clause passed;
- stratified-versus-JEPA/MAE noninferiority failed;
- all four family safeguards passed;
- active-dimension and mechanical clauses passed;
- effective-rank, participation-rank, and top-share recovery ratios were
  `-0.0646`, `-0.0499`, and `-0.0662`;
- all three served-geometry measures moved in the wrong direction in all three
  seeds.

Step-32 served geometry:

| arm | mean effective rank | mean participation rank | mean worst top share | min active fraction |
| --- | ---: | ---: | ---: | ---: |
| JEPA/MAE | 0.098293 | 0.067797 | 0.751273 | 1.0 |
| global VICReg | 0.060315 | 0.043488 | 0.897808 | 1.0 |
| channel-stratified VICReg | 0.057860 | 0.042276 | 0.907509 | 1.0 |

The projector diagnostics explain why fixing channel mixing was insufficient.
Across all seeds, channels, views, and checkpoints, all 54 measured projected
weak-view variance-floor fractions were exactly `1.0`: every projected feature
dimension remained below the unit standard-deviation floor. At step 32 the
stratified arm's fixed-seed-mean effective component-gradient norms were:

| component | served trunk | VICReg head |
| --- | ---: | ---: |
| similarity | 0.0000178 | 0.0000124 |
| variance | 0.0454015 | 0.0292465 |
| covariance | 0.0004352 | 0.0002713 |

The effective variance gradient was about `2,550x` the similarity gradient and
`104x` the covariance gradient on the served trunk. This is component-gradient
dominance, not proof by itself that the variance term alone caused the probe
failure.

Clean step-32 global preprojector geometry also worsened from global to
stratified VICReg (effective rank `0.054606 -> 0.052834`, participation rank
`0.040663 -> 0.039844`, top share `0.874653 -> 0.883967`). The projected
per-channel surface was nearly one-dimensional in both arms: over the nine
seed/channel surfaces, global versus stratified means were respectively
`0.015785 -> 0.015861` effective-rank fraction,
`0.015665 -> 0.015689` participation-rank fraction, and
`0.998734 -> 0.997948` top-eigenvalue share. The small projector-surface change
did not transfer into healthier served geometry.

## Family safeguards and augmentation scope

No family breached the frozen `-0.02` floor versus JEPA/MAE. TF-warmup family
deltas were `+0.000185`, `+0.013088`, `+0.001741`, and `+0.005379` for
multiscale state, order/regime, cross-channel dynamics, and future. Stratified
VICReg deltas were `-0.013590`, `+0.017355`, `-0.005146`, and `+0.004181`.
Passing these safeguards did not override the failed primary and geometry
clauses.

This experiment does not clear augmentation globally. Launcher augmentation
was absent and remains a separate training comparison. Earlier isolated work
showed that the full launcher profile violates semantic/support gates, while a
jitter/amplitude/frequency-gain subset passed transform-only gates but has not
been tested for learned-representation benefit. Earlier matched weak-view-on
versus weak-view-off evidence found only `+0.0001254` AULC difference with
effectively unchanged collapse geometry, so the active internal jitter/dropout
is not the leading explanation for this failure; JEPA/MAE masking and broader
augmentation interactions remain separate questions.

## Supported decision and next boundary

Do not combine or promote the two tested repairs. Keep the production recipe
unchanged and do not spend an end-to-end run on either candidate.

The next objective investigation, if separately authorized and frozen, should
remain module-only and split the newly localized mechanisms rather than add
more branches at once:

1. Start with one VICReg component-necessity screen: JEPA/MAE, the current full
   projected channel-stratified branch, and an otherwise identical arm with
   only the variance component disabled. A rescue would establish variance
   pressure as a necessary contributor, not as a sufficient cause or a
   production repair. Only then test variance dose/normalization or
   projector-only versus served-trunk coupling.
2. For TF, test persistent gradient control rather than another wakeup curve:
   compare a fixed low or continually norm-balanced coefficient with the
   current `0.10` endpoint, while retaining the same AULC and rank endpoints.
3. Treat the semantically qualified launcher-augmentation subset as its own
   paired JEPA/MAE-background training experiment. Do not mix it into either
   loss redesign until its representation effect is measured.

These are recommendations for a new preregistration, not authority to execute
them. The present module-first plan terminates here with
`combined_arm_authorized=false` and
`repair_gate.classification=independent_repairs_failed`.

## Executed variance-component follow-up

The first recommendation above was subsequently frozen and executed. See
`REPRESENTATION_VICREG_VARIANCE_NECESSITY_PROTOCOL.md` and
`REPRESENTATION_VICREG_VARIANCE_NECESSITY_FINDINGS.md`.

Disabling only the variance component returned the stratified trajectory
almost exactly to JEPA/MAE and recovered its measured geometry damage, but the
strict paired-bootstrap rescue lower bound still crossed zero. The
preregistered classification is therefore
`variance_necessity_not_supported`, not proof of necessity or a repair. The
authoritative artifact retains one behaviorally null inactive-JEPA/MAE
manifest scalar from the accepted reference; the variance findings disclose
the exact boundary. The follow-up set `next_experiment_authorized=false`; the
unexecuted dose,
normalization, coupling, TF, and augmentation recommendations above remain
historical proposals rather than current execution authority.
