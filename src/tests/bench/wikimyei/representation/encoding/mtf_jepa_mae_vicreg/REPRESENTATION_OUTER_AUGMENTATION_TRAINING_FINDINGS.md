# MTF outer-augmentation representation-training findings

Date executed: 2026-08-26

## Result

The preregistered matched training screen completed under valid mechanics. The
final classification is:

```text
qualified_candidate_not_supported
```

Replacing the full active launcher augmentation with the semantically
qualified jitter/amplitude/frequency-gain subset did not materially improve
clean held-out representation quality. It also did not improve over neutral
preprocessing. The frozen replacement point, confidence-bound, and positive-
seed clauses all failed.

This result separates two facts that must not be conflated:

1. The full `light_phase_safe_v2` input profile is semantically harmful. In
   this run it removed substantial valid support and often removed the terminal
   causal anchor.
2. That semantic harm was not a material cause of the measured 32-update
   representation deficit. Neutral, qualified, and full-active training all
   ended on nearly the same clean probe and geometry trajectory.

The current representation problem therefore remains inside the learned
module/objective path. The outer profile is unsafe on semantic grounds, but
replacing it is not a demonstrated representation repair.

No next experiment, longer run, production edit, launcher-default change, or
end-to-end run is authorized by this result.

## Frozen boundary

The run followed `REPRESENTATION_OUTER_AUGMENTATION_TRAINING_PROTOCOL.md`.
Every arm used the accepted JEPA/MAE-only model and objective:

```text
JEPA=1.0, MAE=0.25, TF=0, outer VICReg=0
C=3, H=30, F=9, D=32, all_tokens served width=96
seeds=17,31,47; updates=32; checkpoints=0,16,32
```

Only the CPU preprocessing copy differed:

- `jepa_mae_only`: no outer input augmentation;
- `jepa_mae_plus_full_active_outer_augmentation`: the numeric
  `light_phase_safe_v2` profile;
- `jepa_mae_plus_qualified_outer_augmentation`: Gaussian jitter `0.001`,
  amplitude scale `[0.98,1.02]`, and frequency-gain jitter `0.01`, with
  frequency mask, dilation, and warp disabled.

The exact production `apply_mtf_training_augmentations` helper made one served
call and one discarded deterministic replay per update and arm. Clean
checkpoints, probe fitting, geometry, and gradient diagnostics received no
outer augmentation. Nothing constructed NodeLift, MDN/readout, Runtime,
observer, policy, a production graph, or an end-to-end job.

## Primary representation endpoint

Fixed-seed-mean clean macro probe AULC:

| arm | step 0 | step 16 | step 32 | step 32 - step 0 |
| --- | ---: | ---: | ---: | ---: |
| neutral JEPA/MAE | 0.519262499 | 0.512269336 | 0.515543283 | -0.003719216 |
| full active outer augmentation | 0.519262499 | 0.511810964 | 0.515051206 | -0.004211293 |
| qualified outer augmentation | 0.519262499 | 0.512277006 | 0.515529523 | -0.003732975 |

The equal-width raw-history control AULC remained `0.602286582`.

Frozen step-32 paired contrasts:

| contrast | point | paired 95% interval | positive seeds |
| --- | ---: | ---: | ---: |
| qualified - full active | +0.000478317 | [-0.000634436, +0.001563587] | 1/3 |
| qualified - neutral | -0.000013759 | [-0.000028852, +0.000001070] | 0/3 |
| full active - neutral | -0.000492077 | [-0.001565530, +0.000612379] | 1/3 |

Per-seed qualified-minus-full-active values were `-0.001071614`,
`+0.002507586`, and `-0.000001020` for seeds `17,31,47`. The corresponding
qualified-minus-neutral values were `-0.000010740`, `-0.000027374`, and
`-0.000003164`.

The qualified-minus-full-active point is far below the frozen `+0.0024`
materiality floor, its lower bound is not positive, and only one seed moved in
the positive direction. The qualified-minus-neutral independent improvement
subgate also failed all three clauses. Neutral noninferiority passed because
its lower bound remained far above `-0.005`.

## Loss reduction did not mean representation improvement

The fixed-seed means of the first and last eight training losses were:

| arm | first 8 | last 8 |
| --- | ---: | ---: |
| neutral | 0.472496 | 0.254899 |
| full active | 0.463816 | 0.253931 |
| qualified | 0.472625 | 0.254939 |

All objectives optimized strongly while every arm's fixed-seed-mean clean AULC
ended below initialization. Seed 31 declined by approximately `0.0180` to
`0.0206` AULC in all three arms and dominated the small positive changes in
seeds 17 and 47. Lowering the current JEPA/MAE loss is therefore not a reliable
proxy for improving sequence representation.

The full-active arm's slightly smaller loss is not evidence of better
learning: it was also trained on substantially less valid support and ended
with the lowest mean AULC.

## What the augmentations actually did

There were 96 update observations per arm. Neutral and qualified preprocessing
retained support exactly:

| arm | overall retention min/mean/max | terminal retention min/mean/max | removed cells per batch min/mean/max |
| --- | --- | --- | --- |
| neutral | 1 / 1 / 1 | 1 / 1 / 1 | 0 / 0 / 0 |
| full active | 0.866667 / 0.937153 / 0.966667 | 0 / 0.354167 / 1 | 2592 / 4887 / 10368 |
| qualified | 1 / 1 / 1 | 1 / 1 / 1 | 0 / 0 / 0 |

No arm added support. The qualified arm changed at least one clean-valid value
on every update while leaving the mask exact. The full arm sometimes retained
none of the clean-valid terminal cells, even though every sample/channel still
contained some valid input elsewhere.

This reproduces the transform-only semantic diagnosis inside the actual
training path. Frequency mask/dilation/warp are not benign merely because the
representation endpoint was insensitive to them in this short screen.

Despite the support removal, the full-active token-level JEPA target/context
mask XOR counts versus neutral were zero on every update. The transformations
changed values and feature-level support without changing which higher-level
tokens the current tokenizer considered valid enough for JEPA masking.

## Families and served geometry

All eight final family-delta floors passed. Qualified-minus-neutral family
deltas were essentially zero:

```text
multiscale_state  +0.000000073
order_regime      -0.000030411
cross_channel     +0.000017704
future            -0.000043812
```

Qualified-minus-full-active family deltas were all small and negative, from
`-0.000281` to `-0.002748`, still well above the `-0.02` floor.

Step-32 served geometry:

| arm | effective rank | participation rank | worst top share | min active |
| --- | ---: | ---: | ---: | ---: |
| neutral | 0.098293118 | 0.067797022 | 0.751273187 | 1.0 |
| full active | 0.099196365 | 0.068367375 | 0.749378119 | 1.0 |
| qualified | 0.098276935 | 0.067786406 | 0.751308951 | 1.0 |

The qualified better-reference ratios were `0.990731`, `0.991502`, and
`0.992296`; all passed the frozen `0.90` floor. There were no harmful active-
versus-neutral mean gaps, so all four active-gap clauses were not applicable
and passed. Every qualified seed retained active-dimension fraction `1.0`.

These are only relative preservation clauses. All three arms still worsened
from the shared step-zero geometry (`0.120865` effective, `0.083191`
participation, `0.653662` worst top share). The candidate preserved a poor
JEPA/MAE trajectory; it did not repair it.

## Mechanical validity and audits

The authoritative result is mechanically valid:

- model manifests and initial named parameters were exact across arms;
- clean row values, row hashes, CPU data/masks, clean input hashes, and full
  clean JEPA mask plans were exact across arms;
- CPU augmentation used the frozen independent seed domain and exactly one
  served plus one discarded replay call per update;
- served/replay data, masks, and full consumed generator states were exact;
- original CPU and CUDA:0 generator states were restored exactly;
- preview replay compared the complete mask-plan surface and full generator
  states, not only hashes;
- actual JEPA masks equaled the augmented preview; the support-only
  counterfactual was exact;
- module-forward pre-states were exact across all arms, and neutral/qualified
  post-states, JEPA masks, and internal weak-view feature masks were exact;
- all updates were finite, used one Adam then one EMA step, and had clip factor
  exactly one;
- all checkpoint diagnostics restored RNG, parameters/EMA, optimizer,
  train/eval state, and repeated weak views exactly;
- step-zero clean embeddings, probe predictions, selected penalties, and all
  three geometry surfaces were exact;
- the historical isolated contracts, exhaustive pure-gate fixtures, protocol
  pin check, and neutral-auditor self-test passed.

The fresh augmentation semantic qualifier contained exactly one required
schema, candidate, full-active, and global result. It reproduced candidate
`QUALIFIED`, full active `NOT_QUALIFIED`, and global `NOT_QUALIFIED`.

The post-run neutral audit selected exactly 2,046 common neutral keys. The new
log had the exact accepted key set and value hash, with zero missing, extra, or
duplicate keys. `audit.pass=true`; no invalidity override was applied.

The preflight and post-run Git-status snapshots were byte-identical. The
preflight manifest verified after the run, so all pinned source, binary,
production-header, Makefile, and initially dirty workspace hashes remained
unchanged through capture of the authoritative log.

One non-scientific stopped artifact is preserved. The first qualifier command
accidentally invoked the older module-learning qualifier, which emitted none of
the four required augmentation-semantic keys. It was never accepted by
preflight and no representation-training update had begun. Its log is
`.build/tests/representation_outer_augmentation_semantic_recheck_v1_stopped_wrong_executable.log`,
30,018 bytes, SHA-256
`bf725fc9e480e9ff8b23c9e5a2b6676bd86dcfe88df52e940ac67d4c724f42cf`.
The correct fresh capture was then made with the pinned semantic-qualifier
source before the zero-optimizer preflight and sole scientific run.

## Provenance

- repository HEAD: `194c59207a721ccb411aba7ab4df9440e0cc5403`;
- frozen protocol SHA-256:
  `35d0e1e508ce8bf982942d0256fd3db553d3317e98bce60431a4e318a8284e99`;
- production augmentation launcher header SHA-256:
  `5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502`;
- final representation-harness source SHA-256:
  `cb935f5dde57e76afe3751fc624f0a2b9d730422d7f80345988ee06704c0eb63`;
- representation-harness executable SHA-256:
  `151809bfadab87d15165002f09377c706515c512c0332f6a8bada9b0c4aea50e`;
- gate header SHA-256:
  `b5950ae89cb5afa114045e61c29b339be2e1e18f4a7dbb131bca0f68d57f2547`;
- gate-fixture source SHA-256:
  `3b8ca36ee503340a2278ffc7c68a3235dc863d3a6a643371cec640d496dcc7a8`;
- gate executable SHA-256:
  `1333216371070cb1a96d27e72916b7cf94f605d020460a587b9057813749031e`;
- neutral auditor SHA-256:
  `e5fb20ec4130de7fb919f477c63f248255f633c128463f438d25c67d13f57360`;
- correct semantic capture: 25,874 bytes, SHA-256
  `bded59aa609a703ff776cbaee4a5f02c38706015d7ba6a2ae2aded4c613cc728`;
- zero-optimizer preflight: 1,990 bytes, SHA-256
  `e50c0748efb90a8d503f6a823ac453c7a3fb8bbc77a53bc2cc3a20efc688d22b`;
- authoritative raw log: 3,201,743 bytes, SHA-256
  `4b3e0497b525d4c59acc10692b91819c19f6c9b2f291d8502c1ab3183de9d8e7`;
- neutral-reference audit: 2,578 bytes, SHA-256
  `56a40d2d62617b51189e73b8d52718a53e4d79717dab95b8af521de81acecfce`;
- preflight manifest SHA-256:
  `77b3c44e1293b2cd54acfa61d4639a2b56b5d415d233a07da9475ae772dd71e7`;
- post-run receipt SHA-256:
  `8aca6f19b49cb5e9ae83b2cb8c0ad0c3afa71dc0c1c39392264bca52c587480e`.

## Supported decision and possible future question

Do not promote the full active augmentation: its semantic failures are
confirmed. Do not promote the qualified subset as a representation repair:
it was indistinguishable from neutral at the frozen endpoint and failed both
improvement gates. Do not spend another end-to-end run on this distinction.

The strongest remaining module-level fact is objective/representation
misalignment: JEPA/MAE loss falls sharply while clean sequence-probe AULC and
rank geometry worsen. If a future, explicitly authorized protocol is frozen,
the next causal question should decompose the core neutral-input objective into
paired JEPA-only, MAE-only, and JEPA+MAE arms with the same three seeds, 32
updates, masks, optimizer, EMA, and clean endpoints. That would determine
which core branch—or their interaction—produces the remaining deterioration
before considering architecture changes.

That suggestion is not authorization. This protocol terminates with:

```text
next_experiment_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
```
