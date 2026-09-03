# VVA-1B — VICReg View–Loss Boundary Triad protocol

Original registration: 2026-08-31, before implementing the VVA-1B treatment
seam or executing any VVA-1B optimizer update.  Audit amendment: 2026-08-31,
after inspecting the implemented seam but still before executing any VVA-1B
optimizer update or observing any VVA-1B endpoint.  The amendment makes the
already intended custody, mechanics, and decision predicates executable; it
does not change the three arms, contrasts, compute, endpoint, or thresholds.

## Why this is a new versioned experiment

The completed experiment historically named VVA-1 switched time dropout and
Gaussian jitter independently.  It proved that clean identical views do not
rescue the current VICReg representation, but it did not train the missing
`tied_weak` arm.  Therefore it could not separate corrupted-input training
from disagreement between two independently sampled weak views.

VVA-1B preserves that completed protocol, source, log, and findings exactly.
It answers only the missing view--loss boundary question.  GPV-1 is also
settled evidence and is not rerun or re-estimated here.

## Decision question and isolated boundary

VVA-1B asks which of three mechanisms explains the current VICReg quality
deficit:

1. disagreement between independently sampled weak views;
2. training the representation on weakly corrupted inputs; or
3. the unchanged global-pool, nonlinear-projector, variance/covariance loss
   surface even when both views are clean and identical.

The three arms are:

| arm | branch A | branch B | isolated meaning |
| --- | --- | --- | --- |
| `V0_current` | ordinary weak draw A | independent weak draw B | complete current VICReg treatment |
| `V1_tied_weak` | ordinary weak draw A | a clone of weak draw A | corrupted-input training without cross-view discrepancy |
| `V2_clean_identical` | canonical clean input | a clone of canonical clean input | unchanged pooling/projector/variance/covariance surface alone |

The causal contrasts are:

```text
pairing effect:    V1 - V0
corruption effect: V2 - V1
complete package:  V2 - V0
```

This remains representation-module-only.  No downstream head, graph, NodeLift,
MDN/readout, observer, policy, launcher-owned outer augmentation, execution
system, or end-to-end path may be constructed.  Evaluation labels never affect
representation training.

## Frozen custody

Before any authoritative update, require exact SHA-256 custody of:

- the completed VVA-1 protocol:
  `8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616`;
- the completed VVA-1 findings:
  `8e651276f444bbafe4f534132245b48b718b16d98de0f31b62a21bec0b6851f0`;
- the completed VVA-1 harness:
  `5b807c5dfd9bb371a40e1ee062c72f0754b817b91158d8cbdfe16ee3b9642d31`;
- the completed VVA-1 log:
  `d73635a87d96f6d251a8a008b442657066893d3074194bf7f9de055ff61d9d33`;
- IMA-1 findings, including the zero-update current/tied/clean mechanics:
  `ee53b9a97bf1b80153f7fd22ecf5c6dd9857cb0b3dccdb183729e5cfa05854d6`;
- OAA-1 findings, including the conclusion that launcher-owned outer
  augmentation did not rescue VICReg:
  `42abd19f65f9a41ce50bed1d481ecf983750499a34a6f9d9799e232d7503a9c7`;
- IMA-5 findings, including the closed present JEPA branch and forbidden
  IMA-5B training:
  `734aed1fdd289f647c6323e535fa49edddde7efb36064255a85e3c678e0f4299`;
- the GPV-1 protocol:
  `01c6b1d9fcc95a0c831426a481c866cb196f413030d2f3c195b5219d84d57a2a`;
- GPV-1 findings:
  `fed02d5efb021d745d0ba72310c634c84be7a2aaf1427d538da9d28560a662d7`;
- the GPV-1 authoritative log:
  `eb0b8a5821a9aa613ae60508574d10abc18dd37155c2bc68b0ead1d8a68eef27`;
- the immutable pre-seam representation-header bytes:
  `93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea`;
- FSPA-4 seed-17/31/47 archives:
  `5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434`,
  `a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775`,
  and `b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392`;
- OCA-1 `anchor_challenge` seed-17/31/47 completed caches:
  `5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92`,
  `bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39`,
  and `aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6`.

Parse the settled logs/findings rather than inferring their conclusions.
Require V0 objective mask `8`, 512 completed steps, exact metadata, exact
initialization, and passing receipts.  Parse both the VVA-1 and GPV-1 logs and
require their confirmation fields to say unopened with zero confirmation rows.

The pre-seam header hash is historical custody, not permission to lose the
post-seam source identity.  Retain the pre-seam bytes or their already-bound
artifact as immutable evidence.  After implementation, hash the complete
post-seam representation header independently and bind that hash in every
Stage-0 log, completed-cell cache, executable manifest, authoritative log, and
findings report.  If the seam is harness-only, the post-seam header hash must
equal the pre-seam hash.  If the header changes, publish an exact diff audit
and require that it contains only the experiment policy, post-draw
substitution, and diagnostic fields authorized below; no parameter, default,
pool, projector, loss, validity, or optimizer change is permitted.

The new protocol, source, executable, transitive scientific/build manifest,
authoritative log, and completed-cell caches receive independent checksums.

## Experiment-only post-draw seam

Add one view-pairing policy with `independent_weak` as the production default.
Every arm must first execute the exact production forward RNG prelude from the
OCA path: unchanged tokenization, `create_masks()`, and every draw before the
two VICReg weak augmentations.  Calling the weak-augmentation helper directly
from the initial forward seed is forbidden because it skips prelude draws.
After that exact prelude, every arm must execute, in this order:

```text
drawn_a = ordinary weak augmentation
drawn_b = ordinary weak augmentation
clean   = canonicalized input

V0: used_a = drawn_a; used_b = drawn_b
V1: used_a = drawn_a; used_b = clone(drawn_a)
V2: used_a = clean;    used_b = clone(clean)
```

Both ordinary weak draws are consumed in every arm.  `mask_ratio_time=0.10`,
`mask_ratio_channel=0`, time-dropout scale `0.10`, and jitter standard deviation
`0.005` remain unchanged in every arm.  JEPA mask generation is untouched.
The resolved time-dropout probability must be exactly `0.01`, feature dropout
must be exactly inactive, architectural dropout must remain `0`, and both the
drawn and used objects must be retained for audit.

Every arm performs exactly two separate encoder calls and exactly two separate
projector calls.  Reusing branch-A encoded or projected tensors as branch B is
forbidden.  Instrument and assert both call counts.  The debug output must
return and hash all eight treatment tensors separately: drawn-A data/mask,
drawn-B data/mask, used-A data/mask, and used-B data/mask.

The seam may add no parameters and may change no default output, gradient,
generator state, or update when the policy is `independent_weak`.

## Stage 0 — zero-authoritative-update gate

Run all checks at seeds `17`, `31`, and `47` before opening authoritative
training.

### Treatment identity and RNG

Require:

- V0 used views equal the ordinary production weak draws bit-exactly;
- V0 drawn/used A and B differ on at least one valid data value on every
  diagnostic seed/batch, establishing that the current arm is genuinely
  independent rather than accidentally tied;
- V1 used A and B data/masks are bit-exact, and both equal drawn A;
- V2 used A and B equal canonical clean data/mask bit-exactly;
- drawn A and drawn B data/mask hashes are bit-exact across V0/V1/V2 before
  substitution, including the discarded draw B in V1 and V2;
- CPU and CUDA post-view generator states are identical across V0/V1/V2;
- JEPA target/context masks and retained rows are identical across arms;
- branch token masks and sample-valid masks are reported; V1/V2 branch masks
  must be bit-exact, and feature-mask and token-mask Hamming differences must
  be emitted for every arm;
- all values are finite and all invalid positions are zero;
- the active global validity mask is reported and all 96 diagnostic rows are
  jointly valid;
- resolved time dropout is exactly `0.01`, Gaussian jitter is exactly `0.005`,
  feature dropout is exactly `0`, and encoder/projector call counts are exactly
  two per arm.

### Loss and gradient identity

For V1 and V2 require:

```text
projected A == projected B within 1e-7 maximum absolute error
unweighted invariance loss <= 1e-12
tokenizer invariance-gradient norm <= 1e-12
encoder invariance-gradient norm <= 1e-12
projector invariance-gradient norm <= 1e-12
```

Also require pooled A/B equality within `atol=1e-7, rtol=1e-6` for V1 and V2.
Discrete state, row indices, masks, input views, hashes, and CPU/CUDA generator
states use bit equality.  Production-versus-seam floating outputs, losses, and
gradients use `atol=1e-7, rtol=1e-6`.  Stable component-gradient reconstruction
uses maximum absolute residual `5e-5` and relative L2 residual `1e-4`.

For every arm report unweighted and weighted invariance, variance, and
covariance values.  Reconstruct loss using the identical production operation
order and require bit equality; if an independent expression is additionally
reported, apply `atol=1e-7, rtol=1e-6` rather than a float32-inappropriate
`1e-10` tolerance.  Report tokenizer, encoder, and projector gradient norms for
each component and pairwise component-gradient cosines.  A cosine involving a
gradient with norm at most `1e-12` is emitted as `active=false` and
`cosine_defined=false`; it must not produce NaN or a fabricated zero cosine.

Also report:

- view-to-clean and view-to-view normalized perturbation norms;
- feature-mask differences;
- pooled and projected view differences;
- projected dimensions below the frozen standard-deviation floor `1.0`;
- projected effective rank, participation rank, and largest-PC share.

For each seed and arm, use the update-0 scheduled SSL batch and a disposable
clone of the seed-matched FSPA-4 archive.  Make one actual-dose full-loss
shadow step and one actual-dose shadow step for each active weighted component,
each with a fresh Adam optimizer at `1e-3`, global clip norm `5.0`, followed by
one EMA update at `tau=0.990`.  An inactive invariance component performs no
step and must report exact zero parameter change.  Evaluate, before and after
each discarded shadow step, the frozen clean reversal/order AULC, the
`order_regime` and `cross_channel` family AULCs, and all four per-channel RMC
geometry measures.  These virtual steps are descriptive only, are never
selected or gated by direction, and are excluded from authoritative update
counts.  No normalized or dose-matched shadow update is authorized.

### Control-source decision and mandatory seam parity

The historical OCA V0 cache stores its final model, ordered row hashes, losses,
and aggregate update receipts, but it does not store the ordered JEPA-mask,
drawn/used-view, or pre/post-RNG hashes required to prove equivalence to this
new treatment seam.  This is an evidence insufficiency, not an observed model
or mechanics mismatch.  Consequently VVA-1B precommits before implementation
to:

```text
control_source=joint_retrain_vva1b
authoritative_new_arms=V0,V1,V2
authoritative_adam_updates=3 * 3 * 512 = 4608
authoritative_ema_updates=3 * 3 * 512 = 4608
historical_v0_reused=false
```

The historical V0 cache remains mandatory custody and its frozen clean
evaluation must reproduce, but it is descriptive evidence only and is never
the active VVA-1B control.

The explicit `independent_weak` seam must still match the unmodified default
path at every seed in inputs, masks, outputs, component losses, total loss,
all trainable gradients, RNG poststate, clipping decision, Adam update,
optimizer state, parameters, and EMA state.  Perform disposable default-versus-
explicit shadow comparisons at precommitted update indices `{0,255,511}` using
the corresponding rows and forward seeds.  A mismatch in any field is
`invalid_numeric_or_mechanics` and stops before training.  It may never be
converted into authority for the 4,608-update path.  The 4,608 path is
authorized only because the historical cache lacks sufficient pairing
receipts while the fresh default/seam comparison passes exactly under the
tolerances above.

## Stage 1 — bounded paired training

Freeze for all three newly trained arms:

- seed-matched FSPA-4 initialization;
- seeds `17,31,47`;
- exactly 512 Adam updates, learning rate `1e-3`;
- global gradient clipping at norm `5.0`;
- batch size 96 and the exact OCA row schedule;
- the exact OCA forward seed schedule;
- objective mask `8`, `lambda_vicreg=0.05`, and all other outer objective
  coefficients zero;
- `lambda_global_vicreg=0.25`, global VICReg enabled, channel VICReg disabled,
  component weights `{invariance,variance,covariance}={25,25,1}`, variance
  floor `1.0`, and variance epsilon `0.0001`;
- the unchanged masked global mean over the 72 canonical encoder tokens and
  the unchanged `32 -> 64 -> 64 -> 64` three-Linear projector with GELU after
  its first two Linears;
- one Adam step followed by one target-EMA update at `tau=0.990`;
- unchanged global pooling, nonlinear projector, VICReg component weights,
  variance floor, validity reduction, optimizer, tokenizer, encoder, masks,
  normalization, clean evaluation, and `structured_cdsb_sparse_v1` readout;
- no gradient-dose matching and no coefficient search.

Train V0/V1/V2 interleaved per seed in the same executable with equal rows and
reset CPU/CUDA RNG before each arm.  Persist each completed seed atomically in
a versioned immutable cache containing all three newly trained models, full
receipts, manifests, custody hashes, and a checksum.  A restart loads only
fully validated completed seeds; temporary or partial files are never treated
as evidence.

Every trajectory must record all 512 row, target/context-mask, drawn-A,
drawn-B, used-A, used-B, branch-token-mask, global-validity-mask, and pre/post-
RNG hashes; component and total losses; gradient and served-update norms;
clipping; optimizer/EMA counts; final parameter-partition deltas; and
finiteness.  Require the ordinary drawn objects, rows, JEPA masks, and RNG
schedule to be bit-exact across all three arms at every update.  Require all
96 global rows valid, one finite positive loss, finite gradients, exactly one
Adam and one EMA update, and the expected view semantics on every update.
After update 512, tokenizer/encoder and VICReg-projector partitions must have
nonzero deltas; predictor and MAE-decoder partitions must remain exact; target
tokenizer/encoder state may change only through the recorded EMA updates.  A
mechanically invalid trajectory invalidates the experiment.

## Frozen data and evaluation identity

Use exactly the inherited generated-group splits:

| role | first group key | groups |
| --- | ---: | ---: |
| SSL training and normalizer fit | `0` | `256` |
| ridge probe fit | `1,000,000` | `256` |
| ridge-alpha selection | `2,000,000` | `128` |
| development endpoint | `3,000,000` | `256` |
| conditional confirmation | `9,000,000` | `256` |

Fit normalization only on the SSL split.  Freeze sample ladder
`{32,64,128,256}`, ridge grid `{1e-5,1e-4,1e-3,1e-2,1e-1,1}`, per-target
validation-only alpha selection, and smallest-alpha tie breaking.  Bind the
following inherited stable hashes in self-test, preflight, cache, and
authoritative output:

```text
normalization.mean=ebf16130302b08d1
normalization.inv_std=5119e32115e11e61
ssl.normalized={groups=33f4cd8e310fc6e2,data=f0249ea4c1ab0bd8,mask=68c14e56f93e72b6,target=1500317bf51b7d5a}
fit.normalized={groups=974760d5271987e2,data=3541ed3b5052ec33,mask=68c14e56f93e72b6,target=98b9551af0036ecb}
selection.normalized={groups=1625dc5d75ab0162,data=ced27c3675d6abfc,mask=7e6e36e18d0f4936,target=0f6ccf650380cf7f}
development.normalized={groups=ca87fc6f1c72f9e2,data=6abf9f0719e31621,mask=68c14e56f93e72b6,target=286cb42ec5dc8460}
bootstrap.table=408205cac33d403d
```

The bootstrap hash names the exact inherited 256-replicate table over 256
development groups.  Confirmation data must not be generated or hashed unless
an eligible classification opens it.  If opened, emit its full unnormalized
and normalized group/data/mask/target hashes before evaluation and never train
on it.

## Endpoints and bootstrap estimand

Evaluate clean inputs only with `structured_cdsb_sparse_v1`.  The primary
endpoint is macro probe AULC.  Retain all four family AULCs, reversal/order and
shuffled controls, raw equal-width control, and per-channel effective rank,
participation rank, largest-PC share, and active-dimension fraction.

Use the existing deterministic 256-replicate paired generated-group bootstrap.
Each replicate resamples held-out development groups identically across arms
and seeds, recomputes the nonlinear endpoint per seed, and then averages the
three fixed seeds.  Its percentile interval is therefore group-level
uncertainty and need not contain the arithmetic mean of the three displayed
full-sample seed endpoints.

For every pairwise contrast report point estimate, paired 95% interval, all
three seed deltas, and all four family deltas.

## Frozen causal, candidate, and safety predicates

Keep causal attribution separate from candidate safety.  For any paired
contrast `X`, define:

```text
mechanism_effect_supported(X) :=
    X.point >= +0.005 AND X.lower95 > 0 AND 3/3 seed deltas > 0

mechanism_ruled_out_as_rescue_sized(X) := X.upper95 < +0.005

practically_equivalent(X) :=
    X.lower95 >= -0.005 AND X.upper95 <= +0.005

materially_negative(X) :=
    X.point <= -0.005 AND X.upper95 < 0 AND 3/3 seed deltas < 0
```

Family or safeguard outcomes never veto or hide
`mechanism_effect_supported`; they affect only eligibility and safety.  Apply
the rescue-size rule separately to `V1-V0`, `V2-V1`, and `V2-V0`.

The four named families are `multiscale_state`, `order_regime`,
`cross_channel`, and `future`.  In this protocol, “channel” means the
`cross_channel` family plus the per-channel geometry tests below; there is no
unnamed aggregate channel gate.  The inherited safeguards are exactly:

- final-minus-equal-width-raw paired lower bound at least `-0.01`;
- reversal final point at least `0.90`, reversal final lower bound at least
  `0.85`, and trained-minus-initialization reversal-retention lower bound at
  least `-0.02`;
- shuffled continuous-target upper bound at most `0.05`;
- shuffled reversal interval wholly inside `[0.40,0.60]`;
- for every seed and channel: effective-rank fraction at least `0.25`,
  participation-rank fraction at least `0.25`, largest-PC share at most
  `0.80`, and active-dimension fraction at least `0.75`.

Emit every named predicate separately.  A `new_safeguard_failure` means that
V0 passes a named threshold and the candidate fails it.  For an already-failed
V0 threshold, the candidate does not create a new failure, but this never
counts as complete safety.  Define:

```text
candidate_eligible(Vi) :=
    mechanism_effect_supported(Vi-V0)
    AND every (Vi-V0) family delta >= -0.02
    AND no new_safeguard_failure

all_safeguards_pass(Vi) := every threshold listed above passes

objective_made_safe(Vi) :=
    candidate_eligible(Vi)
    AND lower95(Vi-FSPA4) > -0.005
    AND all_safeguards_pass(Vi)

representation_rescue(Vi) :=
    candidate_eligible(Vi)
    AND mechanism_effect_supported(Vi-FSPA4)
    AND at least 3/4 (Vi-FSPA4) family deltas > 0
    AND every (Vi-FSPA4) family delta >= -0.02
    AND all_safeguards_pass(Vi)
```

The frozen FSPA-4 development reference is seed AULC
`{17:0.62830368194544461,31:0.64703100859121065,47:0.64931117183778853}`
and mean `0.64154862079148123`; authenticate and re-evaluate its archives
rather than merely copying these numbers.

## Executable decision routes

First emit all three mechanism predicates; then assign the mechanism route in
the following priority order:

1. any mechanics/numeric/custody failure -> `invalid_numeric_or_mechanics`;
2. supported `V1-V0` and supported `V2-V1` ->
   `pairing_and_corruption_contribute`;
3. supported `V1-V0` and practically equivalent `V2-V1` ->
   `independent_pairing_principal_defect`;
4. practically equivalent `V1-V0` and supported `V2-V1` ->
   `corrupted_input_principal_defect`;
5. supported `V1-V0`, practically equivalent `V2-V0`, and
   materially negative `V2-V1` -> `opposing_view_effects_cancel`;
6. materially negative `V1-V0` ->
   `independent_view_invariance_mitigates_deeper_defect`;
7. both consecutive contrasts are practically equivalent and all three arms
   are materially negative versus FSPA-4 -> `intrinsic_view_line_closed`;
8. all three mechanism contrasts are ruled out as rescue-sized ->
   `no_rescue_sized_view_effect`;
9. otherwise -> `mixed_or_imprecise_view_effect`.

Mechanism attribution does not itself promote an arm.  Among V1 and V2, retain
only `candidate_eligible` arms; choose the greatest `Vi-V0` point estimate,
breaking exact floating-point equality in favor of V2.  Then assign exactly
one terminal candidate classification:

- no eligible arm, but at least one mechanism is supported ->
  `supported_mechanism_without_safe_candidate`;
- no eligible arm and no mechanism is supported -> `no_candidate`;
- eligible arm but neither safety predicate passes -> `mitigation_only`;
- `objective_made_safe` true and `representation_rescue` false ->
  `objective_made_safe`;
- `representation_rescue` true -> `representation_rescue`.

Additionally emit whether each arm is `materially_harmful_vs_fspa4` using
`materially_negative(Vi-FSPA4)`.  In particular, harmful clean V2 proves that
augmentation is not necessary for the damage and retains the intrinsic
pool/projector/loss-surface boundary; safe-but-not-rescued V2 means removing
the views makes VICReg tolerable but not useful; rescued V2 makes the current
internal-view package a primary causal failure.

Only confirmed `objective_made_safe` or `representation_rescue` opens untouched
confirmation rows.  Confirmation performs no training and must reproduce the
same classification.  Otherwise confirmation remains sealed and no treatment
is promoted.

No result directly edits production defaults.  Rollback remains FSPA-4 with
`structured_cdsb_sparse_v1`; operational `all_tokens` rollback remains
available.  Do not reopen JEPA, downstream compatibility, outer augmentation,
or predictor-capacity search.

## Completion evidence

VVA-1B is complete only when all of the following exist and pass independent
audit:

- frozen protocol and checksum;
- source and executable checksums;
- passing CPU self-test and CUDA Stage-0 log with checksums;
- immutable completed-cell caches for every newly trained seed;
- authoritative measurement log and checksum;
- explicit update counts, custody, mechanics, selection, confirmation, and
  rollback receipts;
- a permanent human findings report that distinguishes causal attribution,
  mitigation, safety, and rescue.
