# MTF outer-augmentation representation training protocol

Date preregistered: 2026-08-26, before implementing or executing this
training screen

## Question and boundary

The launcher-owned `light_phase_safe_v2` outer-input profile is not
semantically qualified. Its frequency-bin mask breaks temporal order and
cross-channel coupling, while dilation and warp remove valid history and the
terminal causal anchor. A three-transform subset—Gaussian jitter, amplitude
scale, and frequency-gain jitter—passed the existing 16-seed transform-only
semantic qualifier.

This experiment asks whether replacing the full active outer profile with that
qualified subset improves the clean held-out representation learned by the
JEPA/MAE-only module. A contemporaneous neutral arm distinguishes improvement
over the current active profile from improvement over no outer augmentation.

This is a test-only module experiment. It changes no production default, DSL,
launcher implementation, graph assembly, NodeLift, MDN/readout, Runtime,
checkpoint, observer, policy, certified/final range, or end-to-end path. It
cannot establish market usefulness, production safety, or that augmentation is
the representation system's root cause.

Pinned prior evidence and implementation boundary:

- semantic findings SHA-256:
  `4962e318229940fb2c19d8fb286ca4281b1e45db6c2b5a672fab291a48b81f8a`;
- semantic qualifier source SHA-256:
  `a51d01a51366eb2ce70ff08fad8567121c209e0d50e485e7f341896024bfa5be`;
- production launcher header SHA-256:
  `5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502`;
- accepted neutral JEPA/MAE reference log SHA-256:
  `bd382eb9d638bfc9ce42257eaca0ab5cb2cd4f44a96f2c4c946eb27e9e7dd038`.

## Three contemporaneous arms

All arms use the exact same objective and model:

```text
JEPA=1.0, MAE=0.25, TF=0, outer VICReg=0
C=3, H=30, F=9, D=32, all_tokens served width=96
internal VICReg weak-view branch still executes with accepted settings
```

Construct every model from the same accepted `jepa_mae_only` model config,
including its behaviorally inactive outer-augmentation fields and inactive
`lambda_channel_vicreg=1.0`. Never construct a model from an arm-specific
preprocessing config. Instead, make a separate copy used only as the argument
to `apply_mtf_training_augmentations`; only the outer fields listed below may
differ in that copy.

Only that launcher-owned preprocessing copy differs:

| arm | Gaussian jitter | amplitude scale | frequency mask | frequency gain jitter | dilation | warp |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `jepa_mae_only` | 0 | `1/1` | 0 | 0 | `1/1` | 0 |
| `jepa_mae_plus_full_active_outer_augmentation` | 0.001 | `0.98/1.02` | 0.02 | 0.01 | `0.98/1.02` | 0.01 |
| `jepa_mae_plus_qualified_outer_augmentation` | 0.001 | `0.98/1.02` | 0 | 0.01 | `1/1` | 0 |

For all three arms, freeze feature/history dropout, crop, amplitude shift,
phase jitter, channel/cross-channel/node/edge dropout, and magnitude-
normalization noise at zero. Freeze the descriptive `augmentation_profile`
strings to `no_input_augmentation_v1`, `light_phase_safe_v2`, and
`attribution_candidate_safe_stack`, respectively. Every numeric scalar and all
non-outer preprocessing-copy fields must be asserted. Do not add leave-one-in
training arms.

The full active arm reproduces the current `light_phase_safe_v2` numeric
profile. The qualified arm reproduces exactly the transform-only
`candidate_safe_stack`. No unqualified or untested nonzero transform may enter
that arm.

Serialize each preprocessing manifest in this exact field order:
`augmentation_profile`, `gaussian_jitter_std`, `feature_dropout_prob`,
`history_dropout_prob`, `time_crop_jitter_max`, `time_dilation_min`,
`time_dilation_max`, `time_warp_max`, `amplitude_scale_min`,
`amplitude_scale_max`, `amplitude_shift_std`, `frequency_mask_ratio`,
`frequency_jitter_std`, `phase_jitter_max`, `channel_dropout_prob`,
`cross_channel_dropout_prob`, `node_dropout_prob`, `edge_dropout_prob`, and
`magnitude_normalization_noise_std`. Emit one `field=value` per line using
classic-locale `std::defaultfloat` with precision 17 and LF endings. Its
fingerprint is lowercase, zero-padded, 16-hex-digit FNV-1a-64 over those exact
UTF-8 bytes, using offset basis `14695981039346656037` and prime
`1099511628211`, excluding any arm-specific log-key prefix. Serialize the complete
model config in its pinned struct-field
declaration order, with vectors comma-separated and enumerations by canonical
name, booleans as `true/false`, dtype as `float32`, and device as `cuda:0`,
using the same line encoding; emit and require one identical
model-config fingerprint across all arms separately from the preprocessing
fingerprints.

## Execution and pairing mechanics

Use seeds `17,31,47`, 32 Adam updates, checkpoints `0,16,32`, batch size 96,
the accepted deterministic synthetic datasets/splits and normalization, one
Adam step followed by one target-EMA step, and the accepted clean probe sample
ladder `32,64,128,256`. The model/training device is exactly CUDA:0; outer
augmentation remains exactly CPU as specified below.

Outer augmentation is training-only. For each selected CPU batch, invoke the
exact production `apply_mtf_training_augmentations` function for one served
realization plus one discarded deterministic diagnostic replay, before the
served batch is moved to CUDA and before tokenization or JEPA/MAE masking.
Exactly two augmentation calls occur per update/arm, exactly one output is
served to the optimizer, and zero calls occur during clean checkpoint or
diagnostic extraction, ridge fitting, validation selection, test prediction,
or geometry. Do not copy or reimplement the augmentation math.

All mechanics are conjunctive. A failure invalidates the scientific result.

- Pair exact named initial parameters, clean CPU batch rows and masks, model
  configuration, optimizer configuration, and target-EMA state.
- For zero-based update `u`, use
  `splitmix64(0x6f75746572617567 XOR uint64(model_seed) XOR
  (uint64(u) << 32)) & 0x7fffffffffffffff` as the augmentation seed. Preflight
  must prove all 96 `(seed,u)` values are unique and collide with none of the
  accepted module-forward or diagnostic seeds. Start every arm from the same
  augmentation seed, but do not claim common transforms use identical draws
  after profile-specific branches consume RNG.
- Because augmentation is CPU-only on this path, guard the default CPU and
  CUDA:0 generator states, seed the CPU generator directly, and assert CUDA:0
  is unchanged by both augmentation calls. Within one guard: seed, make the
  served output and capture consumed CPU/CUDA:0 state; reseed, make the
  discarded replay and capture consumed state; require byte-exact data/mask
  output and consumed states; then restore and verify both original states.
  Consumed-state and restored-state hashes are distinct reported fields.
- Every arm's masked output values must be zero. On every update, the neutral
  arm's served output must equal its clean source byte-for-byte. The qualified
  arm's data must differ from clean on at least one valid value and its mask
  must equal clean exactly.
  The full active arm may change valid support as part of the treatment, but it
  must retain at least one valid value per sample/channel. At every update,
  report overall and per-channel `augmented_valid/clean_valid` retention and
  terminal-anchor retention over clean-valid `H-1` cells, plus preserved-cell,
  added-cell, and removed-cell counts. A zero clean denominator in any required
  scope is a mechanics failure.
- Hash the clean CPU input, served CPU augmentation output, replay output, and
  actual forward input. The actual forward data/mask hash after CPU-to-CUDA
  transfer must equal the served augmentation output hash under the existing
  device-independent tensor-byte hash.
- After augmentation, reset CPU/CUDA RNG to the accepted paired module-forward
  seed. Under a fresh reset before each call, preview (a) clean values with
  clean support, (b) augmented values with augmented support, and (c) clean
  values with augmented support. Repeat (b) and require exact preview output
  and post-preview generator state. The actual JEPA masks must equal (b), and
  (b) must equal the support-only counterfactual (c). Retain (a) as the clean
  base mask plan for cross-arm pairing.
- Require clean base mask plans, batch-row hashes, and module-forward generator
  states before the actual forward to be exact across all arms. Require the
  post-forward state, actual JEPA masks, and internal weak-view feature masks
  to be exact between neutral and qualified arms. Full-active post-forward
  state and its mask XOR/counts versus neutral are descriptive because its
  support-dependent sampler may consume a different number of draws; do not
  force them equal across arms.
- Weak-view data hashes are descriptive and expected to differ when outer
  values differ. Do not use their inequality as a mechanics failure.
- Step-zero clean embeddings, probe predictions/AULC/selected penalties,
  served geometry, clean global-preprojector geometry, and clean projected
  per-channel geometry must be exact across all arms.
- Every update must be finite, must execute exactly one Adam then one EMA step,
  and must have clip factor exactly one. Diagnostics must restore RNG,
  train/eval, parameters, EMA, and existing optimizer bytes.
- Keep checkpoint component-gradient diagnostics on the accepted clean,
  canonical diagnostic batch; do not feed them arm-augmented input.
- Re-run the semantic qualifier before training and parse, rather than infer
  from process exit: `attribution.candidate_safe_stack.result=QUALIFIED` and
  `attribution.full_active_stack.result=NOT_QUALIFIED` must both reproduce.
  A fresh, separate immutable qualifier capture must also contain exactly one
  `schema_id=mtf_augmentation_semantic_qualification.v1`, exactly one of each
  named arm result, and exactly one global `result=NOT_QUALIFIED`. Record the
  qualifier source, pinned launcher header, executable, and capture hashes;
  exit status zero alone is never acceptance.
- The contemporaneous neutral arm's common scientific outputs must reproduce
  the accepted neutral JEPA/MAE reference byte-for-byte by key. The accepted
  path is
  `.build/tests/representation_vicreg_variance_necessity_v1_authoritative.log`;
  it has 753029 bytes, schema
  `wikimyei.mtf_jepa_mae_vicreg.vicreg_variance_necessity.v1`, and the pinned
  full-file SHA-256 above. Select only keys matching
  `^(seed_(17|31|47)\.arm\.jepa_mae_only\.|summary\.arm\.jepa_mae_only\.)`.
  Sorted ordinally by key and canonicalized as UTF-8 `key\n`, the required
  2046-key set SHA-256 is
  `7998b81e3aa42e585c75a6ddcc9a3e00e2bc09819d8595add52570ea8b168864`;
  canonicalized as UTF-8 `key=value\n`, its accepted-value SHA-256 is
  `f3696f996bccd1dd7485a7959fdb5415817e1f7bce2357dbbbf9e2e9654ff5fc`.
  The new log must contain that exact key set once each and the same value
  hash; missing, renamed, duplicated, or additional selected keys fail. The
  audit is a separate immutable artifact and cannot modify either log. Failure
  forces `invalid_numeric_or_mechanics`.

Every new arm-level augmentation, replay, retention, RNG, input-binding,
manifest, and fingerprint key—including those for the neutral arm—must live
under `outer_augmentation.seed_<seed>.arm.<arm>.*`; new aggregate keys must live
under `outer_augmentation.summary.*`. No new diagnostic may use either frozen
neutral-reference selector prefix.

From final preflight through capture of the immutable training log, the pinned
production launcher header must retain its recorded SHA-256, and a pre/post
workspace status snapshot must be identical outside the predeclared test-only
paths. No production source, configuration, or DSL file may be edited.

## Frozen workflow and artifacts

Run this ordered workflow once:

1. freeze and record this protocol's SHA-256;
2. implement only the isolated experiment branch, pure gate, fixture tests,
   Makefile wiring, neutral-log auditor, and reporting under test-only paths;
3. build without training; run the historical contracts and the pure gate
   fixtures, including exact threshold boundaries, non-finite/invalid inputs,
   applicable and non-applicable geometry gaps, and classification precedence;
4. make the fresh semantic-qualifier capture and parse every required key;
5. record the final preflight workspace snapshot and SHA-256 manifest; then
   invoke the training experiment exactly once;
6. hash the raw log, run the read-only neutral-reference audit, apply its
   invalidity override if needed, create the post-run receipt, record findings,
   and stop.

The new test-only files are
`representation_outer_augmentation_training_gate.h`,
`test_representation_outer_augmentation_training_gate.cpp`, and
`audit_representation_outer_augmentation_neutral_reference.py`; the isolated
experiment branch stays in
`quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp`. Directly include
the production launcher helper and link its existing test object bundle if
required; do not extract or duplicate its math.

Use these canonical immutable artifacts:

- `.build/tests/representation_outer_augmentation_semantic_recheck_v1.log`;
- `.build/tests/representation_outer_augmentation_training_v1_preflight.log`;
- `.build/tests/representation_outer_augmentation_training_v1_authoritative.log`;
- `.build/tests/representation_outer_augmentation_training_v1_neutral_reference_audit.log`;
- `.build/tests/representation_outer_augmentation_training_v1_preflight_manifest.sha256`;
- `.build/tests/representation_outer_augmentation_training_v1_postrun_receipt.sha256`.

The immutable preflight manifest must bind the protocol, both relevant
production headers, both Makefiles, semantic qualifier source/executable,
semantic qualifier capture, preflight log, representation harness
source/executable, pure gate source/test/executable, neutral auditor, every
initially dirty path outside the predeclared test paths, and the final preflight
workspace-status snapshot. The separate post-run
receipt must bind the preflight-manifest hash, authoritative-log hash,
neutral-audit hash, post-run workspace snapshot, and recomputed source/binary
hashes. Those recomputed hashes must match preflight. The raw training log
cannot contain its own hash; record that hash only in the post-run receipt,
neutral audit, and later findings.

## Endpoints and uncertainty

The primary endpoint is step-32 fixed-seed-mean clean macro probe AULC. A
seed's AULC is the arithmetic mean of clean macro probe scores at sample counts
`32,64,128,256`; the fixed-seed mean is the arithmetic mean of seeds
`17,31,47`. Step 16 is descriptive. Report these paired step-32 contrasts:

1. qualified minus full active—the primary replacement contrast;
2. qualified minus neutral—the representation-improvement/noninferiority
   contrast;
3. full active minus neutral—the descriptive active-profile reference.

Reuse one deterministic 512-row generated-group bootstrap resampling table and
seed `8387496322364763509` for every contrast. Each row must resample the same
held-out groups for both arms and then average the three fixed training seeds.
Report point, paired 95% interval, and positive-seed count. These intervals
measure generated-group uncertainty, not training-seed uncertainty, and do not
correct for multiple comparisons.

Report all four step-32 families for every arm and candidate deltas against
both references. A family delta is the difference of the three-seed mean clean
family R2 at the final 256-sample probe point. Report, per seed and arm, clean
served mean effective-rank fraction, mean participation-rank fraction,
worst-channel top-eigenvalue share, and minimum active-dimension fraction.
Keep the clean global-preprojector,
projected per-channel, component-gradient, losses, updates, and augmentation
retention surfaces descriptive.

## Frozen gates and classifications

Numeric and mechanical validity take precedence. Contrasts must be finite,
ordered, contain their point estimate, and have positive-seed counts in
`[0,3]`. Geometry fractions must be finite in `[0,1]`; required ratios must be
finite.

The `+0.0024` threshold is reused unchanged as the generic materiality floor
from the prior frozen module repair screens; it is not estimated from an
outer-active training result.

The qualified candidate supports replacing the full active profile only if all
of these clauses pass:

- qualified minus full-active AULC point is at least `+0.0024`, its paired 95%
  lower bound is greater than zero, and at least two of three seed contrasts
  are positive;
- qualified minus neutral AULC paired 95% lower bound is greater than `-0.005`;
- none of the eight step-32 fixed-seed-mean qualified-minus-reference family
  deltas—four versus full active and four versus neutral—is below `-0.02`;
- First average each served geometry metric across the three seeds. Let the
  better reference be `max(neutral,active)` for effective and participation
  rank and `min(neutral,active)` for top share. Required ratios are
  `candidate_effective/better_effective`,
  `candidate_participation/better_participation`, and
  `(1-candidate_top)/(1-better_top)`; all three denominators must be strictly
  positive. Each ratio must be at least `0.90`.
- Apply active-gap repair separately to all four served metrics. Effective
  rank, participation rank, and active-dimension fraction are high-is-good;
  top share is low-is-good. For a high-is-good metric, a harmful gap exists
  exactly when `mean_active < mean_neutral`, its closure is
  `(mean_candidate-mean_active)/(mean_neutral-mean_active)`, and a seed moves
  in the repair direction exactly when `candidate_seed > active_seed`. For
  top share, a harmful gap exists exactly when `mean_active > mean_neutral`,
  closure is `(mean_active-mean_candidate)/(mean_active-mean_neutral)`, and
  repair direction is `candidate_seed < active_seed`. Every applicable
  closure must be at least `0.50` and have at least two repairing seeds. A
  metric with no harmful gap is explicitly not applicable and passes;
- every candidate seed has minimum active-dimension fraction at least `0.75`.

The `0.0024`, `-0.02`, `0.90`, `0.50`, and `0.75` equality boundaries are
inclusive. Both confidence lower-bound tests are strict, so equality at zero
or `-0.005`, respectively, fails. Positive seed directions and harmful-gap
definitions are also strict as written.

Define `neutral_improvement_contrast_pass` independently of the replacement
gate: qualified minus neutral AULC must have point at least `+0.0024`, paired
95% lower bound greater than zero, and at least two of three positive seed
contrasts. Define `representation_improvement_pass` as the conjunction of the
complete replacement gate and `neutral_improvement_contrast_pass`.

Classify exactly one of:

- `invalid_numeric_or_mechanics`;
- `qualified_candidate_representation_improvement_supported`;
- `qualified_candidate_harm_mitigation_only` when replacement passes but the
  independent neutral-improvement contrast does not;
- `qualified_candidate_neutral_aulc_improvement_only_replacement_not_supported`
  when
  `neutral_improvement_contrast_pass` is true but replacement does not;
- `qualified_candidate_not_supported` otherwise.

After non-training unit/contract tests and the semantic qualifier pass, permit
exactly one invocation that performs training. A smoke-training run, partial
seed run, rerun after an operational failure, or second accepted log is not
authorized. Preflight ends before optimizer step 1. Any process that completes
one optimizer update or emits a post-step scientific endpoint is the sole
terminal attempt even if it later fails, is interrupted, or is invalid; its
log cannot be replaced under this protocol. A launch failure before those
events also stops for new authorization. The single invocation always stops
after 32 updates. Set
`next_experiment_authorized=false` regardless of classification. Do not tune
transform doses, train leave-one-in arms, extend to 64/128 updates, combine
with TF/VICReg redesigns, edit production, or run end-to-end. A positive result
may motivate only a separately frozen and separately authorized continuation.

## Required reporting

Emit the exact arm manifest, augmentation scalar fingerprint, qualifier
classification, pairing/replay/RNG mechanics, per-update augmentation hashes
and retention, losses/gradients/updates, checkpoints, all three contrasts,
families, geometry, every gate clause, classification, and stopping booleans.
Always emit `next_experiment_authorized=false`, `long_run_authorized=false`,
and `production_or_end_to_end_authorized=false`.
Record source/executable/log hashes and any stopped preflight separately from
the single scientific training attempt.
