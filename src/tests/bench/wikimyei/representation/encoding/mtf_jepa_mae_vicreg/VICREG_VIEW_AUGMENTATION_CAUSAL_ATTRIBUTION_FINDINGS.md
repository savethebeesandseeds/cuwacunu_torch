# VVA-1 — VICReg View-Augmentation Causal Attribution findings

Date executed: 2026-08-30

## Human conclusion

**The active VICReg paired-view augmentations are not the cause of the
representation failure.**

The harmful certified VICReg run used only two active internal view
corruptions: 1% time-cell dropout and Gaussian jitter with standard deviation
`0.005`. We removed each one separately and both together while keeping the
encoder, projector, VICReg loss, initial checkpoint, rows, seeds, random draws,
optimizer, and evaluation fixed.

Removing both produced a small mean improvement, but it was not reproducible:
seed 31 improved strongly while seeds 17 and 47 became slightly worse. The
clean-identical-view model still finished about `0.0261` AULC below the FSPA-4
representation anchor and failed the protected family, order-retention, and
geometry gates. Therefore the damage remains when the paired views are
perfectly clean and identical.

This closes the active paired-view augmentation explanation at this certified
quality boundary. It does **not** say that every possible augmentation is
harmless. It says the actual time dropout and jitter used in the harmful run
do not causally explain that run's representation deficit.

The next isolated boundary is the global VICReg pooling/projector/variance
coupling. Do not return to downstream testing, outer augmentation, JEPA, or
predictor search.

## What changed in our understanding

Before VVA-1, internal paired-view corruption was a plausible cause because:

- outer augmentations had failed to rescue VICReg;
- the internal weak-view functions still applied dropout and jitter; and
- zero-update IMA-1 had proved only identity and RNG mechanics, not trained
  representation quality.

VVA-1 now establishes:

1. Feature-element dropout was not active in the harmful certified run:
   `mask_ratio_channel=0`, so its resolved probability was exactly zero.
2. The two active effects can be switched independently without changing the
   random-number schedule.
3. The current two-effect profile is bit-exact to the unchanged production
   implementation in inputs, masks, outputs, loss, gradients, and RNG state.
4. Removing time dropout is not reproducibly beneficial.
5. Removing jitter is numerically almost neutral.
6. Removing both does not recover the FSPA-4 representation and does not
   improve all three seeds.

The remaining VICReg failure is therefore inside the objective surface after
view construction, not in these view corruptions.

## Frozen design

VVA-1 used the complete two-factor inventory:

| mask | profile | time dropout | Gaussian jitter |
| ---: | --- | ---: | ---: |
| 0 | `clean_identical` | off | off |
| 1 | `time_only` | on | off |
| 2 | `jitter_only` | off | on |
| 3 | `current_time_jitter` | on | on |

The current mask-3 models were loaded from the completed OCA-1 cache and were
not retrained. Masks 0, 1, and 2 used exactly:

- seeds `17,31,47`;
- 512 updates per arm;
- VICReg-only objective mask `8`;
- Adam learning rate `1e-3` and clip norm `5.0`;
- the FSPA-4 seed-matched initialization;
- the exact OCA row and forward-RNG schedules;
- global VICReg and its existing projector/component weights;
- clean `structured_cdsb_sparse_v1` evaluation.

This was `3 × 3 × 512 = 4,608` new optimizer updates. No downstream model or
launcher-owned outer augmentation was constructed. Confirmation contained no
training and remained sealed because no candidate qualified.

## Primary representation-quality result

Fixed-seed mean clean macro probe AULC:

| profile | AULC | difference from FSPA-4 |
| --- | ---: | ---: |
| FSPA-4 anchor | `0.641549` | — |
| current time + jitter | `0.611449` | `-0.030100` |
| clean identical | `0.615420` | `-0.026129` |
| time only | `0.611597` | `-0.029952` |
| jitter only | `0.614945` | `-0.026604` |

Every view profile was below FSPA-4 in all three seeds. The paired 95% interval
for clean-identical minus FSPA-4 was `[-0.038533,-0.013659]`. Clean-identical
also failed the family floor, order-retention, and geometry safeguards.

Direct removals from the current profile:

| contrast | point | paired 95% interval | positive seeds | gate |
| --- | ---: | ---: | ---: | --- |
| remove time | `+0.003496` | `[-0.000246,+0.007406]` | 1/3 | fail |
| remove jitter | `+0.000148` | `[-0.000301,+0.000598]` | 2/3 | fail |
| remove both | `+0.003971` | `[+0.000303,+0.007822]` | 1/3 | fail |

The per-seed remove-both changes were:

- seed 17: `-0.008690`;
- seed 31: `+0.021418`;
- seed 47: `-0.000815`.

The positive held-out-row interval for the mean remove-both contrast must not
be mistaken for seed reproducibility. The frozen gate required all three
fixed-seed directions to be positive. Only one was positive, so the causal
claim fails.

## Factorial effects

| effect | point | paired 95% interval | positive seeds | gate |
| --- | ---: | ---: | ---: | --- |
| time off main effect | `+0.003659` | `[-0.000014,+0.007501]` | 1/3 | fail |
| jitter off main effect | `+0.000311` | `[-0.000215,+0.000830]` | 3/3 | fail |
| time × jitter interaction | `+0.000326` | `[-0.000852,+0.001478]` | 2/3 | descriptive |

Time removal missed both the positive lower-bound and three-seed clauses.
Jitter removal had the same sign in all seeds, but its magnitude was an order
of magnitude below the `+0.0025` causal floor and its interval crossed zero.
The interaction was small and unsupported.

## Mechanical validity

The authoritative run is mechanically valid:

- protocol, module, OCA/OAA/IMA evidence, three FSPA archives, and three OCA
  current-model caches matched their frozen hashes;
- all four profile manifests differed only in time-dropout scale and jitter
  standard deviation;
- mask 3 exactly matched the unchanged default manifest, outputs, loss,
  gradients, and CPU/CUDA RNG poststate;
- feature dropout was exactly inactive;
- all profile forwards consumed the same RNG schedule and produced identical
  JEPA masks;
- every profile had the required view effect and zero outside its mask;
- all nine new seed/profile receipts completed 512 Adam and 512 EMA updates;
- all nine row schedules, RNG schedules, masks, view semantics, finite-value
  checks, and parameter partitions passed;
- all nine trajectories had zero clipped updates;
- no production default changed and no confirmation rows were opened.

One execution incident did not enter the scientific result. An initial
candidate was stopped after seed 17 when a CPU-bound gradient diagnostic was
identified; its partial log is retained as
`.build/tests/representation_vva1_v1_stopped_cpu_gradient_diagnostic.log`.
Two later logging-path attempts were stopped before a seed result. Their
orphaned CUDA processes were identified by exact PID and terminated before the
retained run continued. The authoritative run started from zero, used a
continuous independent process and log, and completed all 4,608 updates.

An independent post-run audit recomputed every primary contrast and found no
outcome-changing discrepancy. It did identify one bounded harness limitation:
the helper for detecting a newly introduced safeguard failure is stricter than
necessary about the reference passing the complete safeguard bundle. That
helper could admit a false positive candidate in a different run, but it cannot
convert any VVA-1 profile into a candidate here because every profile had
already failed the causal-effect and anchor-quality gates. The audit therefore
confirmed closure as a negative attribution and rejected promotion.

## Decision and next boundary

The frozen terminal classification is:

```text
view_augmentation_not_causal_at_quality_boundary
```

No profile was eligible, no candidate was selected, confirmation remained
unopened, and there is no view-recipe promotion.

The authorized next goal is:

> **GPV-1 — Global Pool–Projector–Variance Causal Decomposition:** determine
> whether current VICReg harm comes from erasing token/channel structure in
> the global pool, distortion through the nonlinear projector, variance-floor
> pressure, or their interaction, while keeping the now-cleared paired views
> fixed.

Rollback remains FSPA-4 with `structured_cdsb_sparse_v1`; operational
`all_tokens` rollback remains available. VVA-1 authorizes no production
change and no downstream or end-to-end run.

## Authoritative artifacts

- frozen protocol SHA-256:
  `8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616`;
- final harness source SHA-256:
  `5b807c5dfd9bb371a40e1ee062c72f0754b817b91158d8cbdfe16ee3b9642d31`;
- executable SHA-256:
  `7903a0f215c700fdf635b16643befe1d20781826b7caca6025b7eae1e7704aba`;
- authoritative log:
  `.build/tests/representation_vva1_v1_authoritative.log`, 68,456 bytes,
  SHA-256
  `d73635a87d96f6d251a8a008b442657066893d3074194bf7f9de055ff61d9d33`;
- authoritative terminal status:
  `execution_status=vva1_measurements_complete`.
