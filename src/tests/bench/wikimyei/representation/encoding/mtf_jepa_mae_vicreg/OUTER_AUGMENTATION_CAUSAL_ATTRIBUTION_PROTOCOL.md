# OAA-1 — Outer Augmentation Causal Attribution protocol

Date frozen: 2026-08-29, before implementing or executing OAA-1 training

## Decision question and causal limit

OAA-1 asks whether any semantically eligible launcher-owned outer input
augmentation rescues, mitigates, or further harms the current JEPA-only and
VICReg-only integrations at the certified FSPA-4 representation boundary.

The isolated path is exactly:

```text
clean sequence rows
  -> exact production outer-augmentation helper (training intervention only)
  -> MTF encoder/objective copy
  -> structured_cdsb_sparse_v1
  -> fixed RMC lightweight probes
```

No downstream model, graph, NodeLift, MDN/readout head, observer, policy,
execution system, or end-to-end path may be constructed. Probe targets are
evaluation-only: they may not influence representation training, but the
precommitted development gate may use them once to select at most one
confirmation candidate.

OCA-1 is frozen evidence that its harmful JEPA and VICReg results used exactly
zero outer-augmentation calls. Outer augmentation therefore cannot be the
cause of the harm already observed by OCA-1. OAA-1 can establish only whether
outer preprocessing rescues, mitigates, or worsens those integrations. If it
does not rescue them, the next causal boundary is the module-owned JEPA mask
construction and VICReg weak-view construction.

## Frozen evidence and custody

Require these immutable inputs before any optimizer step:

- OCA-1 authoritative log SHA-256
  `3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d`;
- OCA-1 protocol SHA-256
  `56bac408b28046e4e014ccd22aab675da9d00b0feb28ed2d25d1debacd57ead2`;
- FSPA-4 protocol SHA-256
  `4cf4f81ffac1665f85bd233203ccf2f039617ec8d52b41a40258002b42999b00`;
- semantic qualifier capture SHA-256
  `bded59aa609a703ff776cbaee4a5f02c38706015d7ba6a2ae2aded4c613cc728`;
- production augmentation helper header SHA-256
  `5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502`;
- OCA-1 `anchor_challenge` completed-cache SHA-256 values for seeds 17/31/47,
  in order:
  `5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92`,
  `bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39`,
  `aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6`;
- certified seed-17/31/47 FSPA-4 archive SHA-256 values, in order:
  `5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434`,
  `a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775`,
  `b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392`.

Parse rather than infer the OCA fields `outer_augmentation_calls=0`, the four
objective verdicts, and `execution_status=oca1_measurements_complete`. Load
each FSPA-4 archive into a fresh model and require exact metadata, state,
structured output, and one evaluation replay. Parse the existing semantic
capture and require exactly one each of
`attribution.neutral_reference.result=QUALIFIED`,
`attribution.gaussian_jitter_only.result=QUALIFIED`,
`attribution.amplitude_scale_only.result=QUALIFIED`,
`attribution.frequency_mask_only.result=NOT_QUALIFIED`,
`attribution.frequency_gain_jitter_only.result=QUALIFIED`,
`attribution.candidate_safe_stack.result=QUALIFIED`,
`attribution.dilation_only.result=NOT_QUALIFIED`,
`attribution.warp_only.result=NOT_QUALIFIED`,
`attribution.temporal_dilation_plus_warp.result=NOT_QUALIFIED`, and
`attribution.full_active_stack.result=NOT_QUALIFIED`, plus exactly one global
`result=NOT_QUALIFIED`.

The earlier 32-update outer-augmentation training screen is historical
evidence only. It used the information-destroying `all_tokens` readout and the
old JEPA+MAE trajectory, so its quality result cannot substitute for OAA-1.

## Stage 0 — treatment inventory and semantic stop gate

The production `light_phase_safe_v2` profile has six non-neutral operations:

1. Gaussian jitter, standard deviation `0.001`;
2. amplitude scale, uniform in `[0.98,1.02)`;
3. frequency-bin mask, probability `0.02`;
4. frequency-gain jitter, standard deviation `0.01`;
5. time dilation, uniform in `[0.98,1.02)`;
6. time warp, maximum fraction `0.01`.

All other outer fields remain exactly neutral. Edge dropout is unsupported and
must remain zero. The profile string is descriptive; the scalar fields are the
operative treatment.

Reuse the completed 16-seed semantic qualifier:

- eligible: Gaussian jitter, amplitude scale, frequency-gain jitter, and the
  stack containing exactly those three;
- ineligible: frequency mask, dilation, warp, their temporal combination, and
  the full active profile.

Ineligible transforms stop without training. Frequency mask already broke
temporal order and cross-channel coupling. Dilation and warp already removed
valid history and sometimes the terminal causal anchor. Training cannot make
an input transform semantically valid, so spending optimizer updates on these
arms is forbidden.

Run only one no-optimizer smoke per eligible profile. Require deterministic
served/replay equality, finite values, zero masked cells, exact support and
terminal-anchor retention, restored CPU/CUDA generator states, and actual
forward-input bytes equal to the served augmented object.

## Stage 1 — bounded objective-by-augmentation interaction

Use two objective strata, matching OCA-1 exactly:

| stratum | active coefficient | frozen inactive coefficients |
|---|---:|---|
| JEPA only | `lambda_jepa=1.0` | MAE=0, TF=0, VICReg=0 |
| VICReg only | `lambda_vicreg=0.05` | JEPA=0, MAE=0, TF=0 |

Within each stratum compare these five profiles:

| profile | Gaussian | amplitude scale | frequency gain |
|---|---:|---:|---:|
| identity | 0 | `1/1` | 0 |
| gaussian_only | `0.001` | `1/1` | 0 |
| amplitude_only | 0 | `0.98/1.02` | 0 |
| frequency_gain_only | 0 | `1/1` | `0.01` |
| candidate_safe_stack | `0.001` | `0.98/1.02` | `0.01` |

The four non-identity profile ids are fixed as Gaussian `1`, amplitude `2`,
frequency gain `3`, and safe stack `4`.

Reuse the completed OCA-1 identity models and receipts from the
`anchor_challenge` seed caches. Do not retrain identity. Train only the four
non-identity profiles crossed with the two objectives: eight new arms. Require
the three pinned cache hashes above and extract exactly objective masks `1`
(JEPA) and `8` (VICReg) from the frozen cache mask order `{4,1,2,8,15}`.

For every new arm freeze:

- exact seed-matched FSPA-4 starting archive;
- seeds `17,31,47`;
- exactly 512 Adam updates at learning rate `1e-3`;
- gradient clipping at norm `5.0`;
- batch size 96 and the exact OCA row schedule;
- one Adam step followed by one target-EMA update;
- architecture, internal JEPA masks, VICReg weak views, objective definitions,
  structured readout, RMC data rows, normalization, probes, bootstrap rows,
  metrics, thresholds, raw control, reversal, shuffles, and geometry.

This is 12,288 new model updates. No longer run opens unless the frozen gate
below explicitly requires confirmation; confirmation adds no training.

For zero-based update `u`, derive the augmentation seed exactly as
`splitmix64(0x6f6161315f617567 XOR uint64(model_seed) XOR
(uint64(profile_id) << 56) XOR (uint64(u) << 24)) & 0x7fffffffffffffff`.
Generate each profile's served CPU object once and share that exact retained
object between the JEPA and VICReg strata. Generate one discarded replay under
the same seed. Restore the incoming generators, then reset CPU and CUDA before
each model call to the exact OCA forward seed
`splitmix64(0x6f626a5f6d61736b XOR uint64(model_seed) XOR
(uint64(u) << 32)) & 0x7fffffffffffffff`. Thus the treatment changes only the
actual input values, never the module-owned random schedule.

Require on every update:

- exact clean row indices, row hash, data, and masks across all arms;
- exact served augmentation between the two objective strata for a profile;
- exact served/replay output and consumed RNG state;
- exact restoration of incoming CPU/CUDA RNG state;
- actual forward data/mask hash equal to the served augmentation hash;
- exact support and terminal-anchor retention;
- exact JEPA target/context masks versus identity;
- exact VICReg weak-view support masks versus identity; weak-view values are
  descriptive because their common base input is the treatment;
- finite losses, gradients, updates, and outputs;
- exactly one Adam and one EMA update, with clip norm at most `5.0`;
- nonzero served update and expected objective-head activity;
- unchanged inactive heads.

## Evaluation and causal contrasts

Evaluate clean inputs only after update 512. For each objective/profile pair,
compute two paired contrasts from the same final predictions:

1. candidate minus its cached identity-objective model: the causal effect of
   outer augmentation on that objective;
2. candidate minus the untouched FSPA-4 anchor: whether the complete
   objective-plus-augmentation intervention actually rescues representation.

The primary endpoint is fixed-seed-mean clean macro probe AULC. Reuse the OCA
generated-group bootstrap table and report point, paired 95% interval, and
positive-seed count. Also report all four family deltas, raw-control margin,
reversal/order and shuffle controls, structured geometry, losses, gradients,
updates, support retention, masks, and weak-view diagnostics.

## Gates, classifications, and confirmation

Numeric or mechanical invalidity takes precedence.

`outer_augmentation_rescues_objective` requires all of:

- candidate-minus-anchor AULC point at least `+0.005`;
- its paired 95% lower bound strictly greater than zero;
- all three seeds improve over the anchor;
- candidate-minus-identity-objective paired 95% lower bound strictly greater
  than zero;
- at least three of four family deltas versus anchor are positive and none is
  below `-0.02`;
- every frozen raw, reversal, shuffle, geometry, semantic, and mechanics gate
  passes.

Define a reproducible new safeguard failure as the existing
`oca_seed_safeguards_pass(candidate, anchor, raw)` composite failing on all
three paired seeds while that same composite passes for the cached
identity-objective model on all three paired seeds.

Classify in this fixed precedence: numeric/mechanical invalidity, rescue,
worsening, mitigation, then not supported. `outer_augmentation_mitigates_harm_only`
requires the complete mechanics and semantic gates, no reproducible new
safeguard failure, and candidate-minus-identity-objective lower bound greater
than zero, but fails one or more rescue clauses against the anchor.

`outer_augmentation_worsens_objective` requires valid mechanics and either a
candidate-minus-identity-objective upper bound below zero or a reproducible
new safeguard failure. Otherwise classify `outer_augmentation_not_supported`.
Invalid runs classify `invalid_numeric_or_mechanics`.

Select at most one rescue candidate by greatest candidate-minus-anchor AULC,
with fixed tie order JEPA before VICReg and gaussian, amplitude, frequency
gain, then safe stack. Only if a rescue candidate exists, open the untouched
confirmation rows once and evaluate the retained trained models without
additional updates. Promotion requires the same rescue and safeguard clauses
on confirmation. No production default changes in OAA-1.

The confirmation split is exactly 256 rows beginning at synthetic group
`5,000,000`, inherited from OCA-1, and remains unopened until one rescue is
selected. If no candidate confirms, stop and authorize intrinsic augmentation
attribution: JEPA context/target masking separately from VICReg weak-view
dropout/jitter. Do not interpret this as evidence against JEPA or VICReg as
general methods.

## Durable artifacts and rollback

Write only completed-seed caches under `.build/tests/oaa1/`. Each atomic cache
must bind this protocol hash, OCA/FSPA protocol hashes, archive hashes,
production-helper hash, semantic-capture hash, seed, data hashes, objective,
ordered profile manifests, augmentation seed domain, steps, optimizer, clip
norm, complete models, receipts, row hashes, augmentation hashes, masks, and
mechanics flags. Commit with a same-directory unique temporary file followed
by atomic rename and a SHA-256 marker. A mismatch is fatal, never a cache miss.

Canonical rollback is the untouched seed-matched FSPA-4 archive with
`structured_cdsb_sparse_v1`. The operational `all_tokens` rollback remains
available but is not a representation recommendation.
