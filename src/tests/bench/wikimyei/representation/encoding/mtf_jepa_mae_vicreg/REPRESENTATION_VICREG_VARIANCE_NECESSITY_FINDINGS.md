# MTF VICReg variance-component necessity findings

Date executed: 2026-08-26

## Result

The preregistered variance-necessity gate failed. The classification is
`variance_necessity_not_supported`, and no next experiment, long run,
production edit, augmentation run, or end-to-end run was authorized.

Artifact status is
`authoritative_with_behaviorally_null_manifest_deviation`: the primary
full-versus-no-variance pair is strictly conformant, while the inactive
JEPA/MAE reference retains one accepted-screen scalar described below.

The failure is narrow: disabling the VICReg variance component passed the
rescue point, positive-seed, JEPA/MAE noninferiority, family, served-geometry,
active-dimension, numeric, and mechanical clauses. It failed only the strict
positive paired-bootstrap lower bound for no-variance minus full stratified
VICReg.

At the same time, the pointwise mechanism evidence is strong. With only the
variance weight changed from `25` to `0`, the candidate recovered essentially
the entire full-stratified AULC and served-geometry deficit in every seed and
landed almost exactly on the contemporaneous `jepa_mae_only` trajectory. The
formal gate prevents calling this proof that variance is a necessary
contributor because the held-out-group interval for the rescue still crosses
zero.

This is not a representation repair. The no-variance arm merely returned to
the existing JEPA/MAE baseline, whose step-32 AULC `0.5155433` remains well
below the equal-width raw control AULC `0.6022866` and whose served covariance
geometry remains concentrated.

## Frozen boundary and provenance

The run followed
`REPRESENTATION_VICREG_VARIANCE_NECESSITY_PROTOCOL.md`, subject to the
behaviorally inactive JEPA/MAE manifest ambiguity disclosed below. It executed
only:

- `jepa_mae_only`;
- full projected channel-stratified VICReg; and
- the identical projected channel-stratified arm with
  `vicreg_var_weight=0`.

The two stratified arms differed in exactly that one scalar. Global VICReg was
disabled, channel VICReg and per-channel stratification were enabled, and the
shared projector, weak views, masks, data, optimizer, EMA, and every other
coefficient remained paired. Seeds were `17,31,47`, training was 32 updates,
and checkpoints were `0,16,32`.

The inactive `jepa_mae_only` reference preserved the accepted repair-screen
configuration: global VICReg routing was enabled, channel routing was disabled,
and `lambda_channel_vicreg=1.0`. The frozen arm table lists `0.25` for that
inactive channel multiplier. Because the arm's outer VICReg coefficient is
zero, the difference cannot affect its loss, gradients, random draws, or
trajectory, and preserving the accepted configuration maintains reference
continuity. It is nevertheless a literal table/manifest discrepancy; the
three arm configurations did not match that table scalar-for-scalar.

Provenance:

- repository HEAD: `194c59207a721ccb411aba7ab4df9440e0cc5403`;
- frozen protocol SHA-256:
  `ee6d411449634201f389ea4902037a8bfa471d62a1b3c0a75c497cfacab89045`;
- final quality-harness source SHA-256:
  `dd2e273036fd9fd9ede2cae3c107bf37da925fffd5d714c652ffeb20a0fbc0bb`;
- gate-helper source SHA-256:
  `b992f575ae7246c9166677d1e77b99f87f2f7589bb0b4707a81a6778c7e6ec19`;
- standalone gate-fixture source SHA-256:
  `bc3f527f7307b3972224eddf435639048290a1fa07bb964d7c7508fc0d3fa6ce`;
- authoritative quality executable SHA-256:
  `431a1aa525193812614d0968188195b78538b5bb3020a8643089551b23342ebb`;
- focused contract executable SHA-256:
  `1db4727cf63826c2bd68972c47eabbc382a60c72bdff1e440e2860be14567a24`;
- standalone necessity-gate executable SHA-256:
  `958f300b0e352ccfb145919d375def951336fa08667f953fe9c7be54220cefba`;
- authoritative raw log:
  `.build/tests/representation_vicreg_variance_necessity_v1_authoritative.log`,
  753,029 bytes, SHA-256
  `bd382eb9d638bfc9ce42257eaca0ab5cb2cd4f44a96f2c4c946eb27e9e7dd038`.

## Primary AULC endpoint

Step-32 fixed-seed-mean macro probe AULC:

| arm | AULC |
| --- | ---: |
| `jepa_mae_only` | 0.515543283 |
| full projected channel-stratified VICReg | 0.510709955 |
| projected channel-stratified VICReg without variance | 0.515543320 |

Primary paired contrasts:

| contrast | point | paired 95% interval | positive seeds |
| --- | ---: | ---: | ---: |
| full stratified - JEPA/MAE | -0.004833328 | [-0.010301601, +0.000572622] | 0/3 |
| no variance - full stratified | +0.004833365 | [-0.000572732, +0.010302027] | 3/3 |
| no variance - JEPA/MAE | +0.000000037 | [-0.000000617, +0.000000614] | 2/3 |

Per-seed no-variance minus full-stratified rescue was `+0.001760821`,
`+0.004893321`, and `+0.007845954` for seeds `17,31,47`. The corresponding
no-variance minus JEPA/MAE values were only `+0.000001425`, `-0.000001369`,
and `+0.000000056`.

Thus the candidate passed the frozen `+0.0024` rescue point and 2/3 direction
requirements, but its rescue lower bound was not greater than zero. The
noninferiority lower bound versus JEPA/MAE was far above `-0.005`.

The intervals reuse one deterministic 512-row paired generated-group
bootstrap table and average the three fixed seeds. They quantify held-out
generated-group uncertainty, not training-seed uncertainty, and do not correct
for multiple comparisons.

## Served representation geometry

Step-32 fixed-seed means:

| arm | effective rank | participation rank | worst top share | min active |
| --- | ---: | ---: | ---: | ---: |
| JEPA/MAE | 0.098293118 | 0.067797022 | 0.751273187 | 1.0 |
| full stratified | 0.057860468 | 0.042275660 | 0.907509011 | 1.0 |
| no variance | 0.098293189 | 0.067797064 | 0.751273107 | 1.0 |

The contemporaneous full-stratified reference reproduced all three harmful
geometry directions. No variance recovered `1.0000018`, `1.0000017`, and
`1.0000005` of the effective-rank, participation-rank, and top-share gaps.
Every metric moved in the repair direction in all three seeds and every seed
retained active fraction `1.0`.

All four no-variance minus JEPA/MAE family deltas were negligible and passed:
`+0.000000257`, `-0.000004806`, `+0.000003799`, and `-0.000000572` for
multiscale state, order/regime, cross-channel dynamics, and future.

The clean diagnostic surfaces tell the same story:

| arm | global preprojector eff/part/top | projected per-channel eff/part/top |
| --- | --- | --- |
| JEPA/MAE | 0.089937 / 0.061720 / 0.689500 | 0.045858 / 0.032664 / 0.663192 |
| full stratified | 0.052834 / 0.039844 / 0.883967 | 0.015861 / 0.015689 / 0.997948 |
| no variance | 0.089937 / 0.061720 / 0.689500 | 0.046709 / 0.033229 / 0.655467 |

Projector geometry and variance-floor fractions were descriptive rather than
gate clauses. All no-variance projected weak-view dimensions remained below
the unit variance floor, as expected when that pressure is disabled.

## Component mechanism

The test-only ablation used effective component weights:

```text
full:        {similarity, variance, covariance} = {0.3125, 0.3125, 0.0125}
no variance: {similarity, variance, covariance} = {0.3125, 0,      0.0125}
```

At step 32, fixed-seed-mean effective served-trunk gradient norms were:

| arm | similarity | variance | covariance |
| --- | ---: | ---: | ---: |
| full stratified | 1.78034e-5 | 4.54015e-2 | 4.35156e-4 |
| no variance | 4.01290e-9 | 0 | 2.93013e-12 |

The no-variance raw variance loss remained finite and large (`0.989964` mean)
while its effective gradient was exactly zero. Its remaining similarity and
covariance losses and gradients decayed to nearly zero. Operationally, the
remaining VICReg branch therefore became almost inert, explaining why its
training trajectory returned to JEPA/MAE.

This isolates the current weighted variance contribution as the practical
difference between the harmful full-stratified trajectory and the nearly
identical JEPA/MAE trajectory. It still does not distinguish variance
semantics from total gradient scale, cancellation, optimizer interactions, or
projector/trunk coupling, and the frozen statistical necessity gate did not
pass.

## Mechanical validity and invalid preflights

The authoritative run is mechanically valid:

- the full/no-variance configs differed in exactly one scalar;
- initial parameters, row indices, actual masks, and actual weak views were
  exact;
- raw similarity, variance, and covariance losses and gradients were exact
  across the two arms at step zero;
- common JEPA/MAE branch losses and gradients were exact;
- the full-minus-no-variance loss and gradient reconstructed the effective
  raw variance contribution with maximum seed error below `8.5e-7`;
- all 27 checkpoint direct arm reconstructions were at most `1.47e-7` and all
  single-graph explicit VICReg component reconstructions were exact at the
  emitted precision;
- all diagnostic weak-view, parameter/EMA, train/eval, RNG, and optimizer
  neutrality checks passed;
- served, clean global-preprojector, and clean projected-per-channel geometry
  were compared exactly at step zero for every arm, rather than inferred from
  embedding equality;
- the ablation preflight restored CPU/CUDA generator state and matched one
  weak-view digest across all 12 total, branch, and component replay forwards
  in each seed;
- all values were finite, all updates used one Adam then one EMA step, and all
  nine trajectories had zero clipped updates;
- the focused module contracts and standalone gate fixtures passed.

Two numerical checks were hardened before the authoritative run. Directly
subtracting two large float32 total-gradient vectors to isolate the much
smaller variance vector produced descriptive residuals up to about `3e-5`.
Likewise, summing vectors from independent component backward passes produced
residuals up to `2.46e-5` for the near-zero no-variance branch. The stable
gates use algebraically equivalent exact common-branch plus outer-VICReg
difference and single-graph explicit weighted-component reconstruction. The
unstable direct/separate-backward values remain emitted descriptively.

An independent audit rejected the first complete candidate log before final
acceptance because it repurposed 54 historical component-error keys: 36 in the
two reference arms and 18 in the no-variance arm. That noncompliant artifact is
preserved as
`.build/tests/representation_vicreg_variance_necessity_v1_preaudit_key_noncompliant.log`,
743,626 bytes, SHA-256
`15506c9adeffb6041c3e0415eba35802ef73fe6ec921a2f4e95f467d86eccec2`.
The corrected harness restored the old keys to their separate-backward
semantics and added new `single_graph` keys for the gated residuals. The
corrected bounded screen was then regenerated; all common values between the
two complete runs were identical except those 54 deliberately restored keys.
No scientific surface or gate value changed.

The stopped preflight logs are not scientific runs:

- initial direct-subtraction preflight SHA-256
  `e0c2fd65d70a6847ef9c597727dc3da287fcc9ac5f29bf3b0970384e03d48fc4`;
- first checkpoint-reconstruction preflight SHA-256
  `66c40ceb5001e2fecd0ee8f1974caa9fe96841e0c68ca3b66d6d5232b33750a6`;
- clause-exposing diagnostic preflight SHA-256
  `1872f854dcea3086dc239019f08d01929b41f5e82b9c8ffacd4cccb55d2c2d93`.

All stopped before a scientific result was accepted. They motivated only
numerically stable diagnostic expressions; no arm, data, coefficient,
threshold, endpoint, or scientific gate changed.

An explicit old/new reference audit selected every common per-seed output
under the JEPA/MAE and full projected channel-stratified arm prefixes, every
corresponding summary output, and all seven per-seed and aggregate
full-stratified-minus-JEPA/MAE contrast outputs. All 4,015 common keys were
byte-identical. This includes the 36 historical component-error keys that
invalidated the earlier candidate log. The accepted repair log also has two
old-only fixed-seed-mean step-32-minus-step-0 summary keys not emitted by this
screen; both reconstruct to the exact historical text from the six
byte-identical per-seed delta keys. Thus the selected reference probe, AULC,
family, geometry, loss, and raw/effective-gradient surfaces reproduce the
accepted hardened screen.

## Supported decision

Do not claim that the variance component passed the preregistered necessity
test, and do not promote the variance-disabled arm. It removes the measured
VICReg harm by making the remaining branch nearly inert, but it does not
improve representation beyond JEPA/MAE or approach the raw control.

The present protocol terminates with:

```text
variance_necessity_gate.pass=false
variance_necessity_gate.classification=variance_necessity_not_supported
variance_component_necessity_supported=false
next_experiment_authorized=false
```

If work continues under a new authorization, it should not simply delete
variance and call the module repaired. The next design question is whether a
different anti-collapse mechanism, normalization, or projector/trunk coupling
can improve beyond JEPA/MAE while retaining served rank. Launcher augmentation
and JEPA/MAE masking remain separate module-only investigations and should not
be mixed into that loss redesign.
